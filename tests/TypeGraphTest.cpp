#include "model/TypeGraphBuilder.h"
#include "parser/SpecParser.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
using namespace cc::codegen;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

const MethodInfo& scratch(const TypeGraph& graph, TypeId owner) {
    for (const auto& method : std::get<InterfaceType>(graph.node(owner).data).methods) {
        if (method.name == "scratch") return method;
    }
    throw std::runtime_error("scratch method missing");
}

void checkArchive(const std::filesystem::path& spec) {
    std::ifstream stream(spec, std::ios::binary);
    if (!stream) throw std::runtime_error("cannot open " + spec.string());
    const std::string source{std::istreambuf_iterator<char>(stream), {}};
    const Projection projection{{"InputArchive", {{"scratch", std::nullopt}}},
                                {"OutputArchive", {{"scratch", std::nullopt}, {"writeBool", "b"}}}};
    const TypeTable types{{"void", {"", "void", true}}, {"bool", {"", "boolean", true}},
                          {"double", {"", "number", true}}, {"std::string_view", {"string_view", "string"}},
                          {"boost::container::pmr::memory_resource", {"boost/container/pmr/memory_resource.hpp", {}}}};
    const auto graph = buildTypeGraph({{parseModule(spec, source), projection}}, types);
    const auto input = graph.findType("cc::render::InputArchive");
    const auto output = graph.findType("cc::render::OutputArchive");
    const auto& first = scratch(graph, input);
    const auto& second = scratch(graph, output);
    require(first.result.type == second.result.type, "return types must share one pointer ID");
    require(first.result.location.line != second.result.location.line, "each use must retain its source position");
    require(first.result.location.file == spec && second.result.location.file == spec, "wrong source file");
    require(first.isConst && second.isNoexcept && !first.typescriptName, "lost method qualifiers or visibility");
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

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("expected archive declaration path");
        checkArchive(argv[1]);
        checkTypes();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
