#include "codegen/InterfaceEmitter.h"
#include "codegen/TypeScriptImports.h"
#include "generated/TypeGraphTypes.h"
#include "model/TypeName.h"

#include <set>
#include <sstream>
#include <stdexcept>

namespace cc::codegen {
namespace {

constexpr auto lintBegin = "// NOLINTBEGIN(misc-include-cleaner, bugprone-easily-swappable-parameters)\n";
constexpr auto lintEnd = "// NOLINTEND(misc-include-cleaner, bugprone-easily-swappable-parameters)\n";

const std::string& tsType(const TypeGraph& graph, const TypeUse& use, TypeImports& imports) {
    const auto& node = graph.node(use.type);
    if (!node.typescriptName) throw std::runtime_error(use.location.describe() + ": no TS mapping for " + qualifiedTypeName(graph, use.type));
    if (!node.typescriptModule.empty()) imports[node.typescriptModule].insert(*node.typescriptName);
    return *node.typescriptName;
}

void cppDeclarator(std::ostream& out, const TypeGraph& graph, const TypeUse& use, const std::string& name,
                   ModuleId owner, std::set<std::string>& headers) {
    auto id = use.type;
    size_t pointers = 0;
    while (const auto* pointer = std::get_if<PointerType>(&graph.node(id).data)) {
        ++pointers;
        id = pointer->pointee;
    }
    const auto type = formatTypeName(graph, id, [&](const TypeNode& decl) {
        if (decl.qualifiedName.empty()) throw std::runtime_error(use.location.describe() + ": unsupported C++ interface type");
        if (decl.module != ModuleId{} && decl.module != owner && decl.cppHeader.empty()) {
            throw std::runtime_error(use.location.describe() + ": missing C++ header for imported type " + decl.qualifiedName);
        }
        if (!decl.cppHeader.empty()) headers.insert(decl.cppHeader);
        return qualifyIfShadowed(graph, decl, graph.modules.at(owner.value), decl.qualifiedName);
    });
    out << type << ' ' << std::string(pointers, '*') << name;
}

void cppMethod(std::ostream& out, const TypeGraph& graph, const MethodInfo& method, ModuleId owner, std::set<std::string>& headers) {
    out << "    virtual ";
    cppDeclarator(out, graph, method.result, method.name, owner, headers);
    out << '(';
    for (size_t i = 0; i < method.parameters.size(); ++i) {
        const auto& param = method.parameters[i];
        if (i) out << ", ";
        cppDeclarator(out, graph, param.type, param.name, owner, headers);
    }
    out << ')';
    if (method.isConst) out << " const";
    if (method.isNoexcept) out << " noexcept";
    out << " = 0;\n";
}

void cppInterface(std::ostream& out, const TypeGraph& graph, const TypeNode& decl, std::set<std::string>& headers) {
    const auto& name = decl.name;
    out << "class " << name << " {\npublic:\n"
        << "    " << name << "() noexcept = default;\n"
        << "    " << name << '(' << name << "&& rhs) = delete;\n"
        << "    " << name << '(' << name << " const& rhs) = delete;\n"
        << "    " << name << "& operator=(" << name << "&& rhs) = delete;\n"
        << "    " << name << "& operator=(" << name << " const& rhs) = delete;\n"
        << "    virtual ~" << name << "() noexcept = default;\n\n";
    for (const auto& method : std::get<InterfaceType>(decl.data).methods) cppMethod(out, graph, method, decl.module, headers);
    out << "};\n\n";
}

void tsInterface(std::ostream& out, const TypeGraph& graph, const TypeNode& decl, TypeImports& imports) {
    out << "export interface " << decl.name << " {\n";
    for (const auto& method : std::get<InterfaceType>(decl.data).methods) {
        if (!method.typescriptName) continue;
        out << "    " << *method.typescriptName << " (";
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            const auto& param = method.parameters[i];
            if (i) out << ", ";
            out << param.name << ": " << tsType(graph, param.type, imports);
        }
        out << "): " << tsType(graph, method.result, imports) << ";\n";
    }
    out << "}\n";
}

} // namespace

Files emitInterfaces(const TypeGraph& graph, ModuleId owner, const OutputLayout& layout,
                     const std::string& preamble) {
    const auto& module = graph.modules.at(owner.value);
    std::set<std::string> headers;
    TypeImports typeImports;
    std::ostringstream cppBody, tsBody;
    for (auto id : module.declarations) {
        const auto& decl = graph.node(id);
        if (!std::holds_alternative<InterfaceType>(decl.data)) throw std::runtime_error("expected an interface declaration for " + decl.qualifiedName);
        cppInterface(cppBody, graph, decl, headers);
        if (id != module.declarations.front()) tsBody << '\n';
        tsInterface(tsBody, graph, decl, typeImports);
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
    writeImports(ts, typeImports, layout.typescript, true);
    if (!typeImports.empty()) ts << '\n';
    for (auto* out : {&fwd, &header, &impl}) {
        for (const auto& name : module.namespaces) *out << "\nnamespace " << name << " {\n";
        *out << '\n';
    }
    for (auto id : module.declarations) fwd << "class " << graph.node(id).name << ";\n";
    header << cppBody.view();
    ts << tsBody.view();
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
