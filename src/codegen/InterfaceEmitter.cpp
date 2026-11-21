#include "codegen/InterfaceEmitter.h"
#include "codegen/RecordEmitter.h"
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
    out << type;
    if (pointers || !name.empty()) out << ' ' << std::string(pointers, '*') << name;
}

void cppMethod(std::ostream& out, const TypeGraph& graph, const MethodInfo& method, ModuleId owner, std::set<std::string>& headers) {
    out << "    " << (method.isPureVirtual ? "virtual " : method.isStatic ? "static " : "");
    if (method.isExplicitConversion) out << "explicit operator ";
    cppDeclarator(out, graph, method.result, method.isExplicitConversion ? "" : method.name, owner, headers);
    out << '(';
    for (size_t i = 0; i < method.parameters.size(); ++i) {
        const auto& param = method.parameters[i];
        if (i) out << ", ";
        if (param.byConstReference) out << "const ";
        cppDeclarator(out, graph, param.type, (param.byConstReference ? "&" : "") + param.name, owner, headers);
        if (!param.defaultValue.empty()) out << " = " << param.defaultValue;
    }
    out << ')';
    if (method.isConst) out << " const";
    if (method.isNoexcept) out << " noexcept";
    if (method.isPureVirtual) out << " = 0";
    if (method.cppBody.empty()) out << ";\n";
    else out << ' ' << method.cppBody << '\n';
}

void cppInterface(std::ostream& out, const TypeGraph& graph, const TypeNode& decl, std::set<std::string>& headers) {
    const auto& name = decl.name;
    out << "class " << name;
    const auto& bases = std::get<InterfaceType>(decl.data).bases;
    for (auto base : bases) {
        const auto& type = graph.node(base);
        collectCppHeader(graph, graph.modules.at(decl.module.value), type, headers);
        out << (base == bases.front() ? " : public " : ", public ") << qualifyIfShadowed(graph, type, graph.modules.at(decl.module.value), type.qualifiedName);
    }
    out << " {\npublic:\n"
        << "    " << name << "() noexcept = default;\n"
        << "    " << name << '(' << name << "&& rhs) = delete;\n"
        << "    " << name << '(' << name << " const& rhs) = delete;\n"
        << "    " << name << "& operator=(" << name << "&& rhs) = delete;\n"
        << "    " << name << "& operator=(" << name << " const& rhs) = delete;\n"
        << "    virtual ~" << name << "() noexcept = default;\n\n";
    for (const auto& method : std::get<InterfaceType>(decl.data).methods) cppMethod(out, graph, method, decl.module, headers);
    out << "};\n\n";
}

const std::string& tsSignatureType(const TypeGraph& graph, const TypeUse& use, ModuleId owner, TypeImports& imports) {
    auto id = use.type;
    for (;;) {
        const auto& node = graph.node(id);
        if (const auto* ptr = std::get_if<PointerType>(&node.data)) id = ptr->pointee;
        else if (const auto* ref = std::get_if<ReferenceType>(&node.data)) id = ref->referred;
        else if (const auto* q = std::get_if<QualifiedType>(&node.data)) id = q->base;
        else return boundTypeScriptName(graph, typeScriptElementId(graph, id), owner, imports, use.location.describe());
    }
}

void tsInterface(std::ostream& out, const TypeGraph& graph, const TypeNode& decl, TypeImports& imports, const InterfaceBindings& bindings) {
    out << "export interface " << decl.name;
    const auto& bases = std::get<InterfaceType>(decl.data).bases;
    bool firstBase = true;
    for (auto base : bases) {
        const auto& type = graph.node(base);
        if (std::holds_alternative<ExternalTypeNode>(type.data) && !type.typescriptName) continue;
        out << (firstBase ? " extends " : ", ") << boundTypeScriptName(graph, base, decl.module, imports, decl.qualifiedName);
        firstBase = false;
    }
    out << " {\n";
    for (const auto& method : std::get<InterfaceType>(decl.data).methods) {
        if (!method.typescriptName) continue;
        const InterfaceMethodBinding defaults;
        const auto it = bindings.find({decl.name, method.name});
        const auto& binding = it == bindings.end() ? defaults : it->second;
        const auto& result = tsSignatureType(graph, method.result, decl.module, imports);
        const auto context = method.result.location.describe() + ": " + decl.name + "." + method.name;
        if (binding.nullable && !std::holds_alternative<PointerType>(graph.node(method.result.type).data))
            throw std::runtime_error(context + ": nullable result requires a pointer");
        const auto suffix = binding.nullable ? " | null;\n" : ";\n";
        if (binding.getter) {
            if (!method.isConst || !method.parameters.empty() || result == "void" || !binding.optional.empty())
                throw std::runtime_error(context + ": getter requires a const nullary value method");
            out << "    readonly " << *method.typescriptName << ": " << result << suffix;
            continue;
        }
        if (binding.optional.size() > method.parameters.size()) throw std::runtime_error(context + ": unknown optional parameter");
        const size_t firstOptional = method.parameters.size() - binding.optional.size();
        out << "    " << *method.typescriptName << " (";
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            const auto& param = method.parameters[i];
            if (i) out << ", ";
            if (i >= firstOptional && binding.optional[i - firstOptional] != param.name)
                throw std::runtime_error(context + ": optional parameters must match the trailing parameters in order");
            out << param.name << (i >= firstOptional ? "?: " : ": ") << tsSignatureType(graph, param.type, decl.module, imports);
        }
        out << "): " << result << suffix;
    }
    out << "}\n";
}

} // namespace

void emitInterface(const TypeGraph& graph, const TypeNode& node, std::ostream* cpp, std::set<std::string>& headers,
                   std::ostream* script, TypeImports& imports, const InterfaceBindings& bindings) {
    if (cpp) cppInterface(*cpp, graph, node, headers);
    if (script) tsInterface(*script, graph, node, imports, bindings);
}

std::string emitHeaderSource(const TypeModule& module, const std::filesystem::path& header, const std::string& preamble) {
    std::ostringstream out;
    out << preamble << "// clang-format off\n" << lintBegin << "#include \"" << header.filename().generic_string() << "\"\n";
    for (const auto& ns : module.namespaces) out << "\nnamespace " << ns << " {\n";
    out << '\n';
    for (auto it = module.namespaces.rbegin(); it != module.namespaces.rend(); ++it) out << "} // namespace " << *it << "\n\n";
    out << lintEnd << "// clang-format on\n";
    return out.str();
}

void emitInterfaceHeaders(Files& files, const TypeGraph& graph, const TypeModule& module, const RecordOutput& output,
                          std::string_view body, const std::set<std::string>& headers, const std::string& preamble) {
    std::ostringstream fwd, header;
    fwd << preamble << "#pragma once\n// clang-format off\n";
    header << preamble << "// clang-format off\n" << lintBegin << "#pragma once\n";
    for (const auto& include : headers) header << "#include <" << include << ">\n";
    header << "#include \"" << output.forwardHeader.generic_string() << "\"\n";
    for (auto* out : {&fwd, &header}) {
        for (const auto& name : module.namespaces) *out << "\nnamespace " << name << " {\n";
        *out << '\n';
    }
    for (auto id : module.declarations) fwd << "class " << graph.node(id).name << ";\n";
    header << body;
    fwd << '\n';
    for (auto* out : {&fwd, &header}) {
        for (auto it = module.namespaces.rbegin(); it != module.namespaces.rend(); ++it) {
            *out << "} // namespace " << *it << "\n\n";
        }
        if (out != &fwd) *out << lintEnd;
        *out << "// clang-format on\n";
    }
    files.emplace(output.nativeRoot / output.forwardHeader, fwd.str());
    files.emplace(output.nativeRoot / output.header, header.str());
}

} // namespace cc::codegen
