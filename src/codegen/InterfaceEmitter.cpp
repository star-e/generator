#include "codegen/Documentation.h"
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
#include <utility>

namespace cc::codegen {

namespace {

constexpr auto lintBegin = "// NOLINTBEGIN(misc-include-cleaner, bugprone-easily-swappable-parameters)\n";
constexpr auto lintEnd = "// NOLINTEND(misc-include-cleaner, bugprone-easily-swappable-parameters)\n";

struct ClassScope {
    const TypeGraph& graph;
    const TypeModule& module;
    bool spaceBeforeReference;
    std::set<const TypeNode*>* forwardTypes;
    std::set<const TypeNode*> classes;
    std::set<std::string_view> members;

    ClassScope(const TypeGraph& graph, const TypeNode& owner, bool spaceBeforeReference, std::set<const TypeNode*>* forwardTypes = nullptr)
    : graph(graph), module(graph.modules.at(owner.module.value)), spaceBeforeReference(spaceBeforeReference), forwardTypes(forwardTypes) {
        collect(owner);
    }

    const TypeNode* forwardType(TypeId id, bool byConstReference = false) const {
        if (!forwardTypes) return nullptr;
        const auto& data = graph.node(id).data;
        if (const auto* ptr = std::get_if<PointerType>(&data)) id = ptr->pointee;
        else if (const auto* ref = std::get_if<ReferenceType>(&data)) id = ref->referred;
        else if (!byConstReference) return nullptr;
        if (const auto* q = std::get_if<QualifiedType>(&graph.node(id).data)) id = q->base;
        const auto& type = graph.node(id);
        return type.cppForwardHeader.empty() ? nullptr : &type;
    }

    void collect(const TypeNode& node) {
        if (!classes.insert(&node).second) return;
        const auto addMembers = [&](const auto& declarations) {
            for (const auto& member : declarations) members.insert(member.name);
        };
        if (const auto* iface = std::get_if<InterfaceType>(&node.data)) {
            addMembers(iface->methods);
            for (auto base : iface->bases) collect(graph.node(base));
        } else if (const auto* record = std::get_if<RecordType>(&node.data)) {
            addMembers(record->methods);
            addMembers(record->fields);
            addMembers(record->staticFields);
            for (auto base : record->bases) collect(graph.node(base));
            if (record->implements) collect(graph.node(*record->implements));
        }
    }

    std::string name(const TypeNode& type, const MethodInfo& method) const {
        auto result = cppName(graph, type, module);
        const auto separator = result.find("::");
        const auto first = std::string_view(result).substr(0, separator);
        const bool hiddenByClass = std::ranges::any_of(classes, [&](const TypeNode* base) { return base != &type && base->name == first; });
        const bool hiddenByValue = separator == std::string::npos &&
            (members.contains(first) || std::ranges::find(method.parameters, first, &ParameterInfo::name) != method.parameters.end());
        if (hiddenByClass || hiddenByValue) return "::" + type.qualifiedName;
        return result;
    }

    std::string defaultValue(const ParameterInfo& param, const MethodInfo& method) const {
        auto value = param.defaultValue;
        if (!value.starts_with("::")) return value;
        // The model stores each enumerator of a flags expression with its canonical type prefix.
        const auto& type = graph.node(param.type.type);
        const auto prefix = "::" + type.qualifiedName + "::";
        const auto relative = name(type, method) + "::";
        for (size_t pos = 0; (pos = value.find(prefix, pos)) != std::string::npos; pos += relative.size()) {
            value.replace(pos, prefix.size(), relative);
        }
        return value;
    }
};

void cppDeclarator(std::ostream& out, const ClassScope& scope, const MethodInfo& method, const TypeUse& use,
                   const std::string& name, std::set<std::string>& headers, const TypeNode* forwardType = nullptr) {
    const auto& graph = scope.graph;
    auto id = use.type;
    size_t pointers = 0;
    while (const auto* pointer = std::get_if<PointerType>(&graph.node(id).data)) {
        ++pointers;
        id = pointer->pointee;
    }
    auto type = formatTypeName(graph, id, [&](const TypeNode& decl) {
        if (decl.qualifiedName.empty()) throw std::runtime_error(use.location.describe() + ": unsupported C++ interface type");
        if (decl.module != ModuleId{} && &graph.modules.at(decl.module.value) != &scope.module && decl.cppHeader.empty()) {
            throw std::runtime_error(use.location.describe() + ": missing C++ header for imported type " + decl.qualifiedName);
        }
        if (&decl == forwardType) scope.forwardTypes->insert(&decl);
        else if (!decl.cppHeader.empty()) headers.insert(decl.cppHeader);
        return scope.name(decl, method);
    }, scope.spaceBeforeReference);
    out << formatCppDeclarator(std::move(type), std::string(pointers, '*') + name, scope.spaceBeforeReference);
}

void cppParameters(std::ostream& out, const ClassScope& scope, const MethodInfo& method, size_t count, bool named,
                   std::set<std::string>& headers) {
    out << '(';
    for (size_t i = 0; i < count; ++i) {
        const auto& param = method.parameters[i];
        if (i) out << ", ";
        if (param.byConstReference) out << "const ";
        const auto* forward = param.defaultValue.empty() || param.defaultValue == "nullptr"
            ? scope.forwardType(param.type.type, param.byConstReference) : nullptr;
        cppDeclarator(out, scope, method, param.type, (param.byConstReference ? "&" : "") + (named ? param.name : ""), headers, forward);
        if (!method.isPureVirtual && !param.defaultValue.empty()) out << " = " << scope.defaultValue(param, method);
    }
    out << ')';
    if (method.isConst) out << " const";
}

using VirtualMethods = std::map<std::string, const MethodInfo*>;

std::string methodSignature(const TypeGraph& graph, const MethodInfo& method) {
    std::set<const TypeNode*> expanding;
    auto typeName = [&](auto& self, const TypeNode& type) -> std::string {
        const auto* alias = std::get_if<AliasType>(&type.data);
        if (!alias) return type.qualifiedName;
        if (!expanding.insert(&type).second) throw std::runtime_error(method.result.location.describe() + ": cyclic type alias " + type.qualifiedName);
        auto name = formatTypeName(graph, alias->target, [&](const TypeNode& part) { return self(self, part); });
        expanding.erase(&type);
        return name;
    };
    // Return types may be covariant. Conversion operators already include their target in the name.
    std::string key = method.name + "(";
    for (size_t i = 0; i < method.parameters.size(); ++i) {
        const auto& param = method.parameters[i];
        if (i) key += ", ";
        if (param.byConstReference) key += "const ";
        key += formatTypeName(graph, param.type.type, [&](const TypeNode& part) { return typeName(typeName, part); });
        if (param.byConstReference) key += "&";
    }
    return key + (method.isConst ? ") const" : ")");
}

void collectVirtualMethods(const TypeGraph& graph, TypeId id, VirtualMethods& methods) {
    const auto& data = graph.node(id).data;
    if (const auto* iface = std::get_if<InterfaceType>(&data)) {
        for (auto base : iface->bases) collectVirtualMethods(graph, base, methods);
        for (const auto& method : iface->methods)
            if (method.isPureVirtual) methods[methodSignature(graph, method)] = &method;
    } else if (const auto* base = std::get_if<RecordType>(&data)) {
        for (auto parent : base->bases) collectVirtualMethods(graph, parent, methods);
        if (base->implements) collectVirtualMethods(graph, *base->implements, methods);
    }
}

void cppMethod(std::ostream& out, const ClassScope& scope, const MethodInfo& method, std::set<std::string>& headers,
               const VirtualMethods& inherited) {
    const bool overrides = !method.isStatic && !inherited.empty() && inherited.contains(methodSignature(scope.graph, method));
    writeDocumentation(out, method.documentation, "    ");
    out << "    " << (method.isPureVirtual && !overrides ? "virtual " : method.isStatic ? "static " : "");
    if (method.isExplicitConversion) out << "explicit operator ";
    // Checking a covariant override may require the complete return type.
    cppDeclarator(out, scope, method, method.result, method.isExplicitConversion ? "" : method.name, headers,
                  overrides ? nullptr : scope.forwardType(method.result.type));
    cppParameters(out, scope, method, method.parameters.size(), true, headers);
    if (method.isNoexcept) out << " noexcept";
    if (overrides) out << " override";
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

void cppForwardingOverloads(std::ostream& out, const ClassScope& scope, const TypeNode& decl, const MethodInfo& method,
                           std::set<std::string>& headers) {
    const auto& graph = scope.graph;
    const auto& params = method.parameters;
    const auto& methods = std::get<InterfaceType>(decl.data).methods;
    const auto shadowingParameter = std::ranges::find(params, method.name, &ParameterInfo::name);
    const auto* resultForward = scope.forwardType(method.result.type);
    for (size_t count = requiredParameterCount(method); count < params.size(); ++count) {
        out << "    ";
        cppDeclarator(out, scope, method, method.result, method.name, headers, resultForward);
        cppParameters(out, scope, method, count, true, headers);
        out << " {\n        ";
        if (graph.node(method.result.type).qualifiedName != "void") out << "return ";
        if (std::ranges::count(methods, method.name, &MethodInfo::name) > 1) {
            // A literal such as nullptr or {} need not select this overload on its own.
            out << "(this->*static_cast<";
            cppDeclarator(out, scope, method, method.result, "(" + decl.name + "::*)", headers, resultForward);
            cppParameters(out, scope, method, params.size(), false, headers);
            out << ">(&" << decl.name << "::" << method.name << "))";
        } else {
            if (shadowingParameter < params.begin() + count) out << "this->";
            out << method.name;
        }
        out << '(';
        for (size_t i = 0; i < params.size(); ++i) {
            if (i) out << ", ";
            const auto& p = params[i];
            if (i >= count) out << scope.defaultValue(p, method);
            else forwardArgument(out, graph, p, headers);
        }
        out << ");\n    }\n";
    }
}

void cppInterface(std::ostream& out, const TypeGraph& graph, const TypeNode& decl, std::set<std::string>& headers,
                  std::set<const TypeNode*>& forwardTypes, bool spaceBeforeReference) {
    const auto& name = decl.name;
    out << '\n';
    writeDocumentation(out, decl.documentation);
    out << "class " << name;
    const auto& bases = std::get<InterfaceType>(decl.data).bases;
    ClassScope scope(graph, decl, spaceBeforeReference, &forwardTypes);
    // Overrides of an external base cannot be inspected for covariant return types.
    if (std::ranges::any_of(scope.classes, [](const auto* type) { return std::holds_alternative<ExternalTypeNode>(type->data); }))
        scope.forwardTypes = nullptr;
    VirtualMethods inherited;
    bool needsSpecialMembers = bases.empty();
    bool inheritsVirtualDestructor = false;
    for (auto base : bases) {
        collectVirtualMethods(graph, base, inherited);
        const auto& type = graph.node(base);
        if (const auto* external = std::get_if<ExternalTypeNode>(&type.data)) {
            needsSpecialMembers = true;
            inheritsVirtualDestructor |= external->hasVirtualDestructor;
        } else {
            inheritsVirtualDestructor = true;
        }
        collectCppHeader(graph, graph.modules.at(decl.module.value), type, headers);
        out << (base == bases.front() ? " : public " : ", public ") << cppName(graph, type, scope.module);
    }
    out << " {\npublic:\n";
    if (needsSpecialMembers) {
        const auto moveParam = formatCppDeclarator(name + "&&", "rhs", spaceBeforeReference);
        const auto copyParam = formatCppDeclarator(name + " const&", "rhs", spaceBeforeReference);
        const auto assign = formatCppDeclarator(name + "&", "operator=", spaceBeforeReference);
        // Let an external base determine the default constructor's exception specification.
        out << "    " << name << "()" << (bases.empty() ? " noexcept" : "") << " = default;\n"
            << "    " << name << '(' << moveParam << ") = delete;\n"
            << "    " << name << '(' << copyParam << ") = delete;\n"
            << "    " << assign << '(' << moveParam << ") = delete;\n"
            << "    " << assign << '(' << copyParam << ") = delete;\n"
            << "    " << (inheritsVirtualDestructor ? "" : "virtual ") << '~' << name << "() noexcept"
            << (inheritsVirtualDestructor ? " override" : "") << " = default;\n\n";
    }
    const auto& methods = std::get<InterfaceType>(decl.data).methods;
    for (const auto& method : methods) cppMethod(out, scope, method, headers, inherited);
    for (const auto& method : methods) {
        if (method.isPureVirtual) cppForwardingOverloads(out, scope, decl, method, headers);
    }
    out << "};\n";
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

std::string_view tsParameterPrefix(size_t index, size_t count) {
    if (count <= 2) return index == 0 ? "" : ", ";
    return index == 0 ? "\n        " : ",\n        ";
}

void tsInterface(std::ostream& out, const TypeGraph& graph, const TypeNode& decl, TypeImports& imports, const InterfaceBindings& bindings) {
    writeDocumentation(out, decl.documentation);
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
            const auto& doc = method.documentation.empty() && setter != methods.end() ? setter->documentation : method.documentation;
            writeDocumentation(out, doc, "    ");
            out << (setter == methods.end() ? "    readonly " : "    ") << *method.typescriptName << ": " << result << suffix;
            continue;
        }
        if (binding.optional.size() > method.parameters.size()) throw std::runtime_error(context + ": unknown optional parameter");
        const size_t firstExplicit = method.parameters.size() - binding.optional.size();
        const size_t firstOptional = std::min(firstExplicit, requiredParameterCount(method));
        writeDocumentation(out, method.documentation, "    ");
        out << "    " << *method.typescriptName << (method.optionalMethod ? "?(" : " (");
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            const auto& param = method.parameters[i];
            if (i >= firstExplicit && binding.optional[i - firstExplicit] != param.name)
                throw std::runtime_error(context + ": optional parameters must match the trailing parameters in order");
            out << tsParameterPrefix(i, method.parameters.size()) << param.name << (i >= firstOptional ? "?: " : ": ")
                << tsSignatureType(graph, param.type, decl.module, imports);
        }
        out << "): " << result << suffix;
    }
    for (const auto& callback : iface.typescriptCallbacks) {
        writeDocumentation(out, callback.documentation, "    ");
        out << "    " << callback.name << (callback.optional ? "?(" : " (");
        for (size_t i = 0; i < callback.parameters.size(); ++i) {
            const auto& param = callback.parameters[i];
            out << tsParameterPrefix(i, callback.parameters.size()) << param.name << (param.optional ? "?: " : ": ")
                << tsCallbackType(graph, param.type, decl.module, imports);
        }
        out << "): " << tsCallbackType(graph, callback.result, decl.module, imports) << (callback.optionalResult ? " | undefined;\n" : ";\n");
    }
    out << "}\n";
}

} // namespace

void emitImplementationMethods(std::ostream& out, const TypeGraph& graph, const TypeNode& node, std::set<std::string>& headers,
                               bool spaceBeforeReference) {
    const auto& record = std::get<RecordType>(node.data);
    if (record.bases.empty() && !record.implements) return;
    const ClassScope scope(graph, node, spaceBeforeReference);
    VirtualMethods overrides, implementation;
    std::map<std::string, TypeId> providers;
    for (auto base : record.bases) {
        if (std::holds_alternative<InterfaceType>(graph.node(base).data)) {
            collectVirtualMethods(graph, base, overrides);
        } else {
            VirtualMethods supplied;
            collectVirtualMethods(graph, base, supplied);
            for (const auto& [key, method] : supplied) {
                auto [it, inserted] = providers.try_emplace(key, base);
                if (!inserted) it->second = TypeId{};
            }
        }
    }
    if (record.implements) collectVirtualMethods(graph, *record.implements, implementation);
    implementation.insert(overrides.begin(), overrides.end());
    for (const auto& method : record.methods) implementation.erase(methodSignature(graph, method));
    for (const auto& [key, candidate] : implementation) {
        const bool overrideMethod = overrides.contains(key);
        const auto provider = providers.find(key);
        if (provider != providers.end() && provider->second == TypeId{})
            throw std::runtime_error(node.qualifiedName + ": ambiguous implementation of " + key);
        if (!overrideMethod && provider != providers.end()) continue;
        const auto& method = overrideMethod ? *overrides.at(key) : *candidate;
        out << "    ";
        if (method.isExplicitConversion) out << "explicit operator ";
        cppDeclarator(out, scope, method, method.result, method.isExplicitConversion ? "" : method.name, headers);
        cppParameters(out, scope, method, method.parameters.size(), true, headers);
        if (method.isNoexcept) out << " noexcept";
        if (overrideMethod) out << " override";
        if (provider == providers.end()) { out << ";\n"; continue; }
        out << " { ";
        if (graph.node(method.result.type).qualifiedName != "void") out << "return ";
        out << scope.name(graph.node(provider->second), method) << "::" << method.name << '(';
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            if (i) out << ", ";
            forwardArgument(out, graph, method.parameters[i], headers);
        }
        out << "); }\n";
    }
}

void emitInterface(const TypeGraph& graph, const TypeNode& node, std::ostream* cpp, std::set<std::string>& headers,
                   std::set<const TypeNode*>& forwardTypes,
                   std::ostream* script, TypeImports& imports, const RecordOutput& output) {
    if (cpp) cppInterface(*cpp, graph, node, headers, forwardTypes, output.spaceBeforeReference);
    if (script) tsInterface(*script, graph, node, imports, output.interfaceBindings);
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
    }
    fwd << '\n';
    for (auto id : module.declarations) fwd << "class " << graph.node(id).name << ";\n";
    header << body;
    for (auto* out : {&fwd, &header}) {
        *out << '\n';
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
