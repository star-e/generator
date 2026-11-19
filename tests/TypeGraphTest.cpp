#include "model/TypeGraphBuilder.h"
#include "parser/SpecParser.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <type_traits>

namespace {
using namespace cc::codegen;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void checkSourceLocation() {
    static_assert(std::is_aggregate_v<SourceLocation>);
    static_assert(std::is_same_v<decltype(SourceLocation::file), std::filesystem::path>);
    static_assert(std::is_same_v<decltype(SourceLocation::line), std::size_t>);
    static_assert(std::is_same_v<decltype(SourceLocation::column), std::size_t>);
    const SourceLocation empty;
    require(empty.file.empty() && empty.line == 0 && empty.column == 0, "source location defaults changed");
    const SourceLocation loc{"input.spec", 12, 7};
    require(loc.describe() == "input.spec:12:7", "source location format changed");
}

const MethodInfo& scratch(const TypeGraph& graph, TypeId owner) {
    for (const auto& method : std::get<InterfaceType>(graph.node(owner).data).methods) {
        if (method.name == "scratch") return method;
    }
    throw std::runtime_error("scratch method missing");
}

Module readModule(const std::filesystem::path& spec) {
    std::ifstream stream(spec, std::ios::binary);
    if (!stream) throw std::runtime_error("cannot open " + spec.string());
    return parseModule(spec, {std::istreambuf_iterator<char>(stream), {}});
}

void checkScalarBindings() {
    const TypeTable types{{"void", {.builtin = true}}, {"bool", {.builtin = true}},
                          {"double", {.builtin = true}}, {"float", {.builtin = true}},
                          {"uint8_t", {.header = "cstdint"}}, {"uint32_t", {.header = "cstdint"}},
                          {"cc::uint32_t", {}}};
    const auto graph = buildTypeGraph({}, types);
    require(graph.node(graph.findType("void")).typescriptName == "void", "wrong default void binding");
    require(graph.node(graph.findType("bool")).typescriptName == "boolean", "wrong default bool binding");
    for (const auto* name : {"double", "float", "uint8_t", "uint32_t"}) {
        const auto& type = graph.node(graph.findType(name));
        require(type.typescriptName == "number" && type.typescriptModule.empty(), "expected a TS number without import");
    }
    require(!graph.node(graph.findType("cc::uint32_t")).typescriptName, "scalar binding matched a namespace-local type");

    const auto module = parseModule("bindings.spec", R"(
        module Bindings;
        binding typescript { uint32_t => Count from "types/count"; }
    )");
    const auto bound = buildTypeGraph({{module, {}}}, types);
    const auto& count = bound.node(bound.findType("uint32_t"));
    require(count.typescriptName == "Count" && count.typescriptModule == "types/count",
            "explicit binding did not override the default scalar binding");
}

void checkArchive(const std::filesystem::path& spec) {
    const TypeTable types{{"void", {.builtin = true}}, {"bool", {.builtin = true}},
                          {"double", {.builtin = true}}, {"std::string_view", {"string_view", "string"}},
                          {"boost::container::pmr::memory_resource", {"boost/container/pmr/memory_resource.hpp", {}}}};
    const auto graph = buildTypeGraph({{readModule(spec), {}}}, types);
    const auto input = graph.findType("cc::render::InputArchive");
    const auto output = graph.findType("cc::render::OutputArchive");
    const auto& first = scratch(graph, input);
    const auto& second = scratch(graph, output);
    require(first.result.type == second.result.type, "return types must share one pointer ID");
    require(first.result.location.line != second.result.location.line, "each use must retain its source position");
    require(first.result.location.file == spec && second.result.location.file == spec, "wrong source file");
    require(first.isConst && second.isNoexcept && first.cppOnly && second.cppOnly &&
            !first.typescriptName && !second.typescriptName, "lost method qualifiers or visibility");
    const auto& pointer = std::get<PointerType>(graph.node(first.result.type).data);
    require(pointer.pointee == graph.findType("boost::container::pmr::memory_resource"), "wrong pointee");
    require(graph.modules.at(0).declarations == std::vector<TypeId>{output, input}, "declaration order changed");
    require(graph.node(output).module == ModuleId{0}, "lost module ownership");
    require(std::get<InterfaceType>(graph.node(output).data).methods.front().typescriptName == "b", "lost TS rename");
    const auto scope = graph.findType("cc::render").value;
    std::vector<TypeId> children;
    for (auto edge : graph.out_edges(scope)) {
        require(graph.source(edge) == scope, "invalid scope edge");
        children.push_back(TypeId{graph.target(edge)});
    }
    require(children == graph.modules.front().declarations, "scope children changed");
    for (auto edge : graph.in_edges(input.value)) require(graph.source(edge) == scope, "wrong parent");
    require(graph.in_degree(input.value) == 1, "missing parent");
}

void checkSettings(const std::filesystem::path& spec) {
    TypeTable externals;
    for (const auto* name : {"bool", "uint32_t", "float", "cc::gfx::SampleCount", "cc::Material",
                             "cc::Texture2D", "cc::IntrusivePtr"}) externals.emplace(name, ExternalType{});
    const auto graph = buildTypeGraph({{readModule(spec), {}}}, externals);
    const auto bloom = graph.findType("cc::render::Bloom");
    const auto bloomType = graph.findType("cc::render::BloomType");
    auto field = [&](TypeId record, const std::string& name) -> const FieldInfo& {
        const auto& fields = std::get<RecordType>(graph.node(record).data).fields;
        const auto it = std::ranges::find(fields, name, &FieldInfo::name);
        if (it == fields.end()) throw std::runtime_error("missing settings field " + name);
        return *it;
    };
    const auto& nested = field(graph.findType("cc::render::PipelineSettings"), "bloom");
    require(nested.isReadonly && nested.type.type == bloom, "PipelineSettings.bloom did not bind to Bloom");
    const auto& mode = field(bloom, "type");
    require(mode.type.type == bloomType && mode.initializer == "BloomType::KawaseDualFilter", "lost enum default");
    const auto& values = std::get<EnumType>(graph.node(bloomType).data).values;
    require(values.size() == 2 && values[0].name == "KawaseDualFilter" && values[1].name == "MipmapFilter",
            "enum values changed order");
    const auto& legacy = field(bloom, "material");
    const auto& kawase = field(bloom, "kawaseFilterMaterial");
    require(legacy.type.type == kawase.type.type && kawase.isNullable && kawase.type.location.file == spec,
            "lost shared resource type or field metadata");
    const auto& ptr = std::get<TemplateInstanceType>(graph.node(kawase.type.type).data);
    require(ptr.declaration == graph.findType("cc::IntrusivePtr") &&
            std::get<TypeId>(ptr.arguments.at(0)) == graph.findType("cc::Material"), "wrong resource type");
}

void checkImports(const std::filesystem::path& specs) {
    const auto commonSpec = specs / "render/render-common.spec";
    std::vector<ModuleInput> inputs{{readModule(commonSpec), {}},
                                    {readModule(specs / "extern/light.spec"), {}},
                                    {readModule(specs / "extern/reflection-probe.spec"), {}},
                                    {readModule(specs / "extern/gfx.spec"), {}, true},
                                    {readModule(specs / "render/archive.spec"), {}}};
    inputs.push_back({parseModule("consumer.spec", R"(
        module Consumer;
        import RenderCommon;
        namespace cc::render {
            struct PassInfo { light: LightInfo; }
            graph Passes {
                vertex_descriptor Vertex = uint32_t;
                vertex_storage: indexed;
                hierarchy_graph { storage: shared_edges; }
                addressable_graph;
                parent_graph;
                named_graph { component: cc::render::BufferTag; }
                component RasterPassTag: PassInfo { member: passes; }
                component Label_: Vertex { member: labels; }
                component BufferTag: ccstd::pmr::string { member: names; }
            }
            graph PassCounts {
                vertex_descriptor Vertex = uint32_t;
                vertex_storage: indexed;
                component cc::render::RasterPassTag: uint32_t { member: counts; }
            }
            @tag struct Label_ {}
        }
    )"), {}});
    const TypeTable types{{"cc::IntrusivePtr", {}}, {"uint32_t", {}}, {"uint8_t", {}}, {"bool", {}},
                          {"ccstd::pmr::string", {}}, {"ccstd::vector", {}}, {"void", {}}, {"double", {}},
                          {"std::string_view", {}}, {"boost::container::pmr::memory_resource", {}}};
    auto check = [&](const TypeGraph& graph) {
        const auto infoId = graph.findType("cc::render::LightInfo");
        const auto& info = graph.node(infoId);
        const auto& pass = graph.node(graph.findType("cc::render::PassInfo"));
        require(std::get<RecordType>(pass.data).fields.at(0).type.type == infoId, "wrong imported record");
        const auto raster = graph.findType("cc::render::RasterPassTag");
        const auto& tag = graph.node(raster);
        require(std::holds_alternative<TagType>(tag.data) && tag.module == info.module && !tag.typescriptName,
                "wrong tag ownership or TS binding");
        const auto& passes = std::get<GraphType>(graph.node(graph.findType("cc::render::Passes")).data);
        const auto& counts = std::get<GraphType>(graph.node(graph.findType("cc::render::PassCounts")).data);
        require(passes.hierarchy && !counts.hierarchy, "lost graph hierarchy capability");
        require(passes.parentGraph && passes.nameComponentIndex == 2 && !counts.parentGraph && !counts.nameComponentIndex,
                "parent lookup did not bind the selected name component");
        require(passes.addressableGraph && !counts.addressableGraph, "lost graph path capability");
        require(passes.components.at(0).tag == raster && counts.components.at(0).tag == raster &&
                passes.components[0].valueType == graph.findType("cc::render::PassInfo") && passes.components[0].name == "passes",
                "graphs did not reuse the imported tag");
        require(passes.components.at(1).tag == graph.findType("cc::render::Label_") &&
                passes.components[1].valueType == graph.findType("cc::render::Passes::Vertex") &&
                graph.findType("cc::render::Passes::RasterPassTag") == TypeId{}, "wrong graph component scope");
        const auto light = graph.findType("cc::scene::Light");
        const auto probe = graph.findType("cc::scene::ReflectionProbe");
        const auto& lightNode = graph.node(light);
        const auto& probeNode = graph.node(probe);
        const auto& consumer = graph.modules.at(info.module.value);
        require(consumer.name == "RenderCommon" && graph.modules.at(lightNode.module.value).name == "Light" &&
                graph.modules.at(probeNode.module.value).name == "ReflectionProbe", "wrong module ownership");
        require(std::holds_alternative<ExternalTypeNode>(lightNode.data) &&
                std::holds_alternative<ExternalTypeNode>(probeNode.data), "lost external declarations");
        const auto mode = graph.findType("cc::gfx::ResolveMode");
        const auto archive = graph.findType("cc::render::InputArchive");
        require(consumer.imports == std::vector<ModuleId>{lightNode.module, probeNode.module, graph.node(mode).module, graph.node(archive).module},
                "wrong direct imports");
        const auto& pair = std::get<RecordType>(graph.node(graph.findType("cc::render::ResolvePair")).data);
        require(pair.fields.at(3).type.type == mode && pair.fields.at(4).type.type == mode, "wrong resolve mode type");
        const auto& upload = std::get<RecordType>(graph.node(graph.findType("cc::render::UploadPair")).data);
        require(pair.compareEqual && pair.hashable && !pair.compareLess && !pair.compareGreater &&
                !upload.compareEqual && !upload.hashable, "wrong record comparison or hash policy");
        require(upload.moveOnly && !pair.moveOnly && upload.fields.at(0).isReadonly, "wrong upload record policy");

        const auto& enumeration = std::get<EnumType>(graph.node(graph.findType("cc::render::AccessType")).data);
        require(enumeration.underlying == graph.findType("uint8_t"), "lost enum underlying type");
        const auto& record = std::get<RecordType>(info.data);
        require(record.constructors == std::vector<std::vector<uint32_t>>{{0, 2, 3, 1}, {0, 2}},
                "lost native constructor overloads or argument order");
        const auto& fields = record.fields;
        require(record.serializable && pair.serializable && !upload.serializable &&
                fields.at(0).transient && fields.at(1).transient && !fields.at(2).transient, "wrong archive selection");
        require(fields.size() == 4 && fields[0].name == "light" && fields[0].isNullable &&
                fields[1].name == "probe" && fields[1].isNullable, "wrong LightInfo resources");
        const auto& ptr = std::get<TemplateInstanceType>(graph.node(fields[0].type.type).data);
        require(ptr.declaration == graph.findType("cc::IntrusivePtr") &&
                ptr.arguments == std::vector<TemplateArgument>{light}, "wrong IntrusivePtr argument");
        require(std::get<PointerType>(graph.node(fields[1].type.type).data).pointee == probe, "wrong probe pointee");
        require(fields[1].initializer == "nullptr" && fields[2].name == "level" && fields[2].initializer == "0" &&
                fields[3].name == "culledByLight" && fields[3].initializer == "false", "wrong LightInfo defaults");
    };
    check(buildTypeGraph(inputs, types));
    std::ranges::reverse(inputs);
    check(buildTypeGraph(inputs, types));
}

void checkTypes() {
    auto module = parseModule("forward.spec", R"(
        module Test;
        namespace cc::test {
            interface First { Last* get(); }
            interface Last { First* get(); }
        })");
    auto graph = buildTypeGraph({{std::move(module), {}}}, {});
    const auto first = graph.findType("cc::test::First");
    const auto last = graph.findType("cc::test::Last");
    const auto result = std::get<InterfaceType>(graph.node(first).data).methods.front().result.type;
    require(result == graph.makePointer(last), "forward reference did not bind");
    const auto qualified = graph.makeQualified(last, true, false);
    require(qualified == graph.makeQualified(qualified, true, false), "duplicate const type");
    const auto ref = graph.makeReference(last, false);
    require(ref == graph.makeReference(ref, true), "reference collapsing failed");
    const auto fn = graph.makeFunction(last, {ref, first}, true);
    require(fn == graph.makeFunction(last, {ref, first}, true), "duplicate function type");
    require(fn != graph.makeFunction(last, {first, ref}, true), "function parameter order lost");
    const auto box = graph.appendNode(TypeNode{.name = "Box", .qualifiedName = "Box", .data = RecordType{}}, {});
    const auto instance = graph.makeTemplateInstance(box, {last, std::string("3")});
    require(instance == graph.makeTemplateInstance(box, {last, std::string("3")}), "duplicate template instance");

    const auto count = graph.num_vertices();
    try {
        graph.appendNode(TypeNode{.qualifiedName = "Box"}, {});
        throw std::runtime_error("accepted duplicate declaration");
    } catch (const std::invalid_argument&) {}
    require(graph.num_vertices() == count && graph.findType("Box") == box, "failed insertion changed graph");
}

void checkEnumIntegers() {
    const auto module = parseModule("integers.spec", R"(
        module Integers;
        namespace cc {
            flags Bits : uint32_t { NONE = 0, HIGH = 0X80000000, ALL = 0xffffffff, COPY = ALL }
            enum class Signed : int64_t { MIN = -9223372036854775808, NEXT, RESTART = MIN, AFTER }
            enum class Unsigned : uint64_t { MAX = 18446744073709551615, TOP = 1<<0x3f, COPY = TOP, NEXT }
        })");
    const auto graph = buildTypeGraph({{module, {}}}, {{"uint32_t", {}}, {"int64_t", {}}, {"uint64_t", {}}});
    const auto& bits = std::get<EnumType>(graph.node(graph.findType("cc::Bits")).data);
    require(bits.isFlags && bits.values.at(1).initializer == "0X80000000" &&
            std::get<uint64_t>(bits.values.at(3).value) == UINT32_MAX, "lost flag literal or alias");
    const auto& signedEnum = std::get<EnumType>(graph.node(graph.findType("cc::Signed")).data);
    require(!signedEnum.isFlags && std::get<int64_t>(signedEnum.values.at(0).value) == INT64_MIN &&
            std::get<int64_t>(signedEnum.values.at(3).value) == INT64_MIN + 1, "incorrect signed enum value");
    const auto& unsignedEnum = std::get<EnumType>(graph.node(graph.findType("cc::Unsigned")).data);
    require(std::get<uint64_t>(unsignedEnum.values.at(0).value) == UINT64_MAX, "incorrect unsigned enum value");
    require(unsignedEnum.values.at(1).initializer == "1 << 0x3f" &&
            std::get<uint64_t>(unsignedEnum.values.at(1).value) == (uint64_t{1} << 63) &&
            std::get<uint64_t>(unsignedEnum.values.at(3).value) == (uint64_t{1} << 63) + 1,
            "lost shift expression, value or implicit successor");
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("expected spec directory");
        const std::filesystem::path specs = argv[1];
        checkSourceLocation();
        checkScalarBindings();
        checkArchive(specs / "render/archive.spec");
        checkSettings(specs / "render/render-settings.spec");
        checkImports(specs);
        checkTypes();
        checkEnumIntegers();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
