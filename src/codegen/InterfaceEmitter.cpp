#include "codegen/InterfaceEmitter.h"
#include "generated/TypeGraphTypes.h"

#include <set>
#include <sstream>
#include <stdexcept>

namespace cc::codegen {
namespace {

constexpr auto lintBegin = "// NOLINTBEGIN(misc-include-cleaner, bugprone-easily-swappable-parameters)\n";
constexpr auto lintEnd = "// NOLINTEND(misc-include-cleaner, bugprone-easily-swappable-parameters)\n";

const TypeNode& baseType(const TypeGraph& graph, TypeId id) {
    while (const auto* pointer = std::get_if<PointerType>(&graph.node(id).data)) id = pointer->pointee;
    return graph.node(id);
}

const std::string& tsType(const TypeGraph& graph, const TypeUse& use) {
    const auto& name = graph.node(use.type).typescriptName;
    if (!name) throw std::runtime_error(use.location.describe() + ": no TS mapping for " + baseType(graph, use.type).qualifiedName);
    return *name;
}

void cppDeclarator(std::ostream& out, const TypeGraph& graph, TypeId id, const std::string& name) {
    size_t pointers = 0;
    while (const auto* pointer = std::get_if<PointerType>(&graph.node(id).data)) {
        ++pointers;
        id = pointer->pointee;
    }
    out << graph.node(id).qualifiedName << ' ' << std::string(pointers, '*') << name;
}

void cppMethod(std::ostream& out, const TypeGraph& graph, const MethodInfo& method) {
    out << "    virtual ";
    cppDeclarator(out, graph, method.result.type, method.name);
    out << '(';
    for (size_t i = 0; i < method.parameters.size(); ++i) {
        const auto& param = method.parameters[i];
        if (i) out << ", ";
        cppDeclarator(out, graph, param.type.type, param.name);
    }
    out << ')';
    if (method.isConst) out << " const";
    if (method.isNoexcept) out << " noexcept";
    out << " = 0;\n";
}

void cppInterface(std::ostream& out, const TypeGraph& graph, const TypeNode& decl) {
    const auto& name = decl.name;
    out << "class " << name << " {\npublic:\n"
        << "    " << name << "() noexcept = default;\n"
        << "    " << name << '(' << name << "&& rhs) = delete;\n"
        << "    " << name << '(' << name << " const& rhs) = delete;\n"
        << "    " << name << "& operator=(" << name << "&& rhs) = delete;\n"
        << "    " << name << "& operator=(" << name << " const& rhs) = delete;\n"
        << "    virtual ~" << name << "() noexcept = default;\n\n";
    for (const auto& method : std::get<InterfaceType>(decl.data).methods) cppMethod(out, graph, method);
    out << "};\n\n";
}

void tsInterface(std::ostream& out, const TypeGraph& graph, const TypeNode& decl) {
    out << "export interface " << decl.name << " {\n";
    for (const auto& method : std::get<InterfaceType>(decl.data).methods) {
        if (!method.typescriptName) continue;
        out << "    " << *method.typescriptName << " (";
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            const auto& param = method.parameters[i];
            if (i) out << ", ";
            out << param.name << ": " << tsType(graph, param.type);
        }
        out << "): " << tsType(graph, method.result) << ";\n";
    }
    out << "}\n";
}

} // namespace

Files emitInterfaces(const TypeGraph& graph, ModuleId owner, const OutputLayout& layout,
                     const std::string& preamble) {
    const auto& module = graph.modules.at(owner.value);
    std::set<std::string> headers;
    auto collectHeader = [&](const TypeUse& type) {
        const auto& base = baseType(graph, type.type);
        if (base.qualifiedName.empty()) throw std::runtime_error(type.location.describe() + ": unsupported C++ interface type");
        const auto& header = base.cppHeader;
        if (!header.empty()) headers.insert(header);
    };
    for (auto id : module.declarations) {
        const auto& decl = graph.node(id);
        const auto* interfaceType = std::get_if<InterfaceType>(&decl.data);
        if (!interfaceType) throw std::runtime_error("expected an interface declaration for " + decl.qualifiedName);
        for (const auto& method : interfaceType->methods) {
            collectHeader(method.result);
            for (const auto& param : method.parameters) collectHeader(param.type);
        }
    }

    std::ostringstream fwd, header, impl, ts;
    const auto fwdName = layout.prefix + "Fwd.h";
    const auto typeName = layout.prefix + "Types.h";
    fwd << preamble << "#pragma once\n// clang-format off\n";
    header << preamble << "// clang-format off\n" << lintBegin << "#pragma once\n";
    for (const auto& include : headers) header << "#include <" << include << ">\n";
    header << "#include \"" << (layout.includeDir / fwdName).generic_string() << "\"\n";
    impl << preamble << "// clang-format off\n" << lintBegin << "#include \"" << typeName << "\"\n";
    ts << preamble << "/* eslint-disable max-len */\n";
    for (auto* out : {&fwd, &header, &impl}) {
        for (const auto& name : module.namespaces) *out << "\nnamespace " << name << " {\n";
        *out << '\n';
    }
    for (size_t i = 0; i < module.declarations.size(); ++i) {
        const auto& decl = graph.node(module.declarations[i]);
        fwd << "class " << decl.name << ";\n";
        cppInterface(header, graph, decl);
        if (i) ts << '\n';
        tsInterface(ts, graph, decl);
    }
    fwd << '\n';
    for (auto* out : {&fwd, &header, &impl}) {
        for (auto it = module.namespaces.rbegin(); it != module.namespaces.rend(); ++it) {
            *out << "} // namespace " << *it << "\n\n";
        }
        if (out != &fwd) *out << lintEnd;
        *out << "// clang-format on\n";
    }
    const auto dir = layout.nativeRoot / layout.includeDir;
    return {{dir / fwdName, fwd.str()}, {dir / typeName, header.str()},
            {dir / (layout.prefix + "Types.cpp"), impl.str()}, {layout.typescript, ts.str()}};
}

} // namespace cc::codegen
