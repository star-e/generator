#include "codegen/InterfaceEmitter.h"
#include "codegen/RecordEmitter.h"
#include "codegen/TypeScriptImports.h"
#include "generated/TypeGraphTypes.h"
#include "model/TypeGraphBuilder.h"
#include "model/TypeName.h"

#include <algorithm>
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

void cppParameters(std::ostream& out, const TypeGraph& graph, const MethodInfo& method, size_t count, bool named,
                   ModuleId owner, std::set<std::string>& headers, bool defaults = true) {
    out << '(';
    for (size_t i = 0; i < count; ++i) {
        const auto& param = method.parameters[i];
        if (i) out << ", ";
        if (param.byConstReference) out << "const ";
        cppDeclarator(out, graph, param.type, (param.byConstReference ? "&" : "") + (named ? param.name : ""), owner, headers);
        if (defaults && !method.isPureVirtual && !param.defaultValue.empty()) out << " = " << param.defaultValue;
    }
    out << ')';
    if (method.isConst) out << " const";
}

void cppMethod(std::ostream& out, const TypeGraph& graph, const MethodInfo& method, ModuleId owner, std::set<std::string>& headers) {
    out << "    " << (method.isPureVirtual ? "virtual " : method.isStatic ? "static " : "");
    if (method.isExplicitConversion) out << "explicit operator ";
    cppDeclarator(out, graph, method.result, method.isExplicitConversion ? "" : method.name, owner, headers);
    cppParameters(out, graph, method, method.parameters.size(), true, owner, headers);
    if (method.isNoexcept) out << " noexcept";
    if (method.isPureVirtual) out << " = 0";
    if (method.cppBody.empty()) out << ";\n";
    else out << ' ' << method.cppBody << '\n';
}

void forwardArgument(std::ostream& out, const TypeGraph& graph, const ParameterInfo& p, std::set<std::string>& headers) {
    const auto& type = graph.node(p.type.type);
    if (p.byConstReference || std::holds_alternative<ReferenceType>(type.data) || std::holds_alternative<PointerType>(type.data) ||
        std::holds_alternative<EnumType>(type.data) || std::holds_alternative<AliasType>(type.data) || isArithmeticType(type.qualifiedName)) {
        out << p.name;
    } else {
        headers.insert("utility");
        out << "std::move(" << p.name << ')';
    }
}

void cppForwardingOverloads(std::ostream& out, const TypeGraph& graph, const TypeNode& decl, const MethodInfo& method,
                           std::set<std::string>& headers) {
    const auto& params = method.parameters;
    const auto& methods = std::get<InterfaceType>(decl.data).methods;
    for (size_t count = requiredParameterCount(method); count < params.size(); ++count) {
        out << "    ";
        cppDeclarator(out, graph, method.result, method.name, decl.module, headers);
        cppParameters(out, graph, method, count, true, decl.module, headers);
        out << " { ";
        if (graph.node(method.result.type).qualifiedName != "void") out << "return ";
        if (std::ranges::count(methods, method.name, &MethodInfo::name) > 1) {
            // A literal such as nullptr or {} need not select this overload on its own.
            out << "(this->*static_cast<";
            cppDeclarator(out, graph, method.result, "(" + decl.name + "::*)", decl.module, headers);
            cppParameters(out, graph, method, params.size(), false, decl.module, headers);
            out << ">(&" << decl.name << "::" << method.name << "))";
        } else {
            out << "this->" << method.name;
        }
        out << '(';
        for (size_t i = 0; i < params.size(); ++i) {
            if (i) out << ", ";
            const auto& p = params[i];
            if (i >= count) out << p.defaultValue;
            else forwardArgument(out, graph, p, headers);
        }
        out << "); }\n";
    }
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
    const auto& methods = std::get<InterfaceType>(decl.data).methods;
    for (const auto& method : methods) cppMethod(out, graph, method, decl.module, headers);
    for (const auto& method : methods) {
        if (method.isPureVirtual) cppForwardingOverloads(out, graph, decl, method, headers);
    }
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

std::string tsCallbackType(const TypeGraph& graph, const CallbackTypeInfo& type, ModuleId owner, TypeImports& imports) {
    if (type.declaration.type != TypeId{}) return boundTypeScriptName(graph, type.declaration, owner, imports);
    if (type.arguments.empty()) return type.name;
    std::string result = type.name + "<";
    for (size_t i = 0; i < type.arguments.size(); ++i) {
        if (i) result += ", ";
        result += tsCallbackType(graph, type.arguments[i], owner, imports);
    }
    return result + ">";
}

void tsInterface(std::ostream& out, const TypeGraph& graph, const TypeNode& decl, TypeImports& imports, const InterfaceBindings& bindings) {
    out << "export interface " << decl.name;
    const auto& iface = std::get<InterfaceType>(decl.data);
    bool firstBase = true;
    for (auto base : iface.bases) {
        const auto& type = graph.node(base);
        if (type.cppOnly || (std::holds_alternative<ExternalTypeNode>(type.data) && !type.typescriptName)) continue;
        out << (firstBase ? " extends " : ", ") << boundTypeScriptName(graph, base, decl.module, imports, decl.qualifiedName);
        firstBase = false;
    }
    out << " {\n";
    const auto& methods = iface.methods;
    for (const auto& method : methods) {
        if (!method.typescriptName || method.accessor == AccessorKind::Setter) continue;
        const InterfaceMethodBinding defaults;
        const auto it = bindings.find({decl.name, method.name});
        const auto& binding = it == bindings.end() ? defaults : it->second;
        const auto& result = tsSignatureType(graph, method.result, decl.module, imports);
        const auto context = method.result.location.describe() + ": " + decl.name + "." + method.name;
        const auto suffix = method.resultPresence == ResultPresence::Nullable ? " | null;\n" :
                            method.resultPresence == ResultPresence::Optional ? " | undefined;\n" : ";\n";
        if (method.accessor == AccessorKind::Getter) {
            if (!binding.optional.empty())
                throw std::runtime_error(context + ": getter requires a const nullary value method");
            const auto setter = std::ranges::find_if(methods, [&](const MethodInfo& m) {
                return m.accessor == AccessorKind::Setter && m.typescriptName == method.typescriptName;
            });
            if (setter != methods.end() && tsSignatureType(graph, setter->parameters.front().type, decl.module, imports) != result)
                throw std::runtime_error(context + ": setter " + setter->name + " must return void and accept the property type");
            out << (setter == methods.end() ? "    readonly " : "    ") << *method.typescriptName << ": " << result << suffix;
            continue;
        }
        if (binding.optional.size() > method.parameters.size()) throw std::runtime_error(context + ": unknown optional parameter");
        const size_t firstExplicit = method.parameters.size() - binding.optional.size();
        const size_t firstOptional = std::min(firstExplicit, requiredParameterCount(method));
        out << "    " << *method.typescriptName << (method.optionalMethod ? "? (" : " (");
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            const auto& param = method.parameters[i];
            if (i) out << ", ";
            if (i >= firstExplicit && binding.optional[i - firstExplicit] != param.name)
                throw std::runtime_error(context + ": optional parameters must match the trailing parameters in order");
            out << param.name << (i >= firstOptional ? "?: " : ": ") << tsSignatureType(graph, param.type, decl.module, imports);
        }
        out << "): " << result << suffix;
    }
    for (const auto& callback : iface.typescriptCallbacks) {
        out << "    " << callback.name << (callback.optional ? "? (" : " (");
        for (size_t i = 0; i < callback.parameters.size(); ++i) {
            if (i) out << ", ";
            const auto& param = callback.parameters[i];
            out << param.name << (param.optional ? "?: " : ": ") << tsCallbackType(graph, param.type, decl.module, imports);
        }
        out << "): " << tsCallbackType(graph, callback.result, decl.module, imports) << (callback.optionalResult ? " | undefined;\n" : ";\n");
    }
    out << "}\n";
}

} // namespace

void emitImplementationMethods(std::ostream& out, const TypeGraph& graph, const TypeNode& node, std::set<std::string>& headers) {
    const auto& record = std::get<RecordType>(node.data);
    if (record.bases.empty() && !record.implements) return;
    using Methods = std::map<std::string, const MethodInfo*>;
    auto signature = [&](const MethodInfo& method) {
        std::ostringstream key;
        key << method.name;
        cppParameters(key, graph, method, method.parameters.size(), false, node.module, headers, false);
        return key.str();
    };
    auto collect = [&](auto& self, TypeId id, Methods& methods) -> void {
        const auto& data = graph.node(id).data;
        if (const auto* iface = std::get_if<InterfaceType>(&data)) {
            for (auto base : iface->bases) self(self, base, methods);
            for (const auto& method : iface->methods)
                if (method.isPureVirtual) methods[signature(method)] = &method;
        } else if (const auto* base = std::get_if<RecordType>(&data)) {
            for (auto parent : base->bases) self(self, parent, methods);
            if (base->implements) self(self, *base->implements, methods);
        }
    };
    Methods overrides, implementation;
    std::map<std::string, TypeId> providers;
    for (auto base : record.bases) {
        if (std::holds_alternative<InterfaceType>(graph.node(base).data)) {
            collect(collect, base, overrides);
        } else {
            Methods supplied;
            collect(collect, base, supplied);
            for (const auto& [key, method] : supplied) {
                auto [it, inserted] = providers.try_emplace(key, base);
                if (!inserted) it->second = TypeId{};
            }
        }
    }
    if (record.implements) collect(collect, *record.implements, implementation);
    implementation.insert(overrides.begin(), overrides.end());
    for (const auto& method : record.methods) implementation.erase(signature(method));
    for (const auto& [key, candidate] : implementation) {
        const bool overrideMethod = overrides.contains(key);
        const auto provider = providers.find(key);
        if (provider != providers.end() && provider->second == TypeId{})
            throw std::runtime_error(node.qualifiedName + ": ambiguous implementation of " + key);
        if (!overrideMethod && provider != providers.end()) continue;
        const auto& method = overrideMethod ? *overrides.at(key) : *candidate;
        out << "    ";
        if (method.isExplicitConversion) out << "explicit operator ";
        cppDeclarator(out, graph, method.result, method.isExplicitConversion ? "" : method.name, node.module, headers);
        cppParameters(out, graph, method, method.parameters.size(), true, node.module, headers);
        if (method.isNoexcept) out << " noexcept";
        if (overrideMethod) out << " override";
        if (provider == providers.end()) { out << ";\n"; continue; }
        out << " { ";
        if (graph.node(method.result.type).qualifiedName != "void") out << "return ";
        out << graph.node(provider->second).qualifiedName << "::" << method.name << '(';
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            if (i) out << ", ";
            forwardArgument(out, graph, method.parameters[i], headers);
        }
        out << "); }\n";
    }
}

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
