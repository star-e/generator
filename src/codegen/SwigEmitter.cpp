#include "codegen/SwigEmitter.h"
#include "model/TypeName.h"

#include <sstream>

namespace cc::codegen {
namespace {

using Path = std::filesystem::path;

std::string includeName(const Path& header) {
    // The engine puts both native/ and native/cocos/ on SWIG's include path.
    return header.begin() != header.end() && *header.begin() == "cocos"
        ? header.lexically_relative("cocos").generic_string() : header.generic_string();
}

TypeId propertyValue(const TypeGraph& graph, TypeId type) {
    if (const auto* ref = std::get_if<ReferenceType>(&graph.node(type).data)) type = ref->referred;
    if (const auto* q = std::get_if<QualifiedType>(&graph.node(type).data)) type = q->base;
    return type;
}

std::string attributeType(const TypeGraph& graph, const MethodInfo& getter) {
    const auto type = getter.result.type;
    auto name = qualifiedTypeName(graph, propertyValue(graph, type));
    if (const auto* ref = std::get_if<ReferenceType>(&graph.node(type).data)) name += ref->rvalue ? "&&" : "&";
    // Protect template argument commas from the SWIG preprocessor.
    return name.find(',') == std::string::npos ? name : "%arg(" + name + ")";
}

bool returnsObjectPointer(const TypeGraph& graph, const MethodInfo& method) {
    const auto* pointer = std::get_if<PointerType>(&graph.node(canonicalType(graph, method.result)).data);
    if (!pointer) return false;
    const auto id = canonicalType(graph, {pointer->pointee, method.result.location});
    const auto& data = graph.node(id).data;
    return std::holds_alternative<RecordType>(data) || std::holds_alternative<InterfaceType>(data) ||
           std::holds_alternative<GraphType>(data) || std::holds_alternative<ExternalTypeNode>(data) ||
           std::holds_alternative<TemplateInstanceType>(data);
}

struct SwigInterface {
    std::ostringstream ignored;
    std::ostringstream attributes;
    // Properties have no callable overloads. Keep overloads to validate name-based GC rules.
    std::map<std::string, std::vector<const MethodInfo*>> members;
};

void ignoreOverload(std::ostream& out, const TypeGraph& graph, const std::string& name, const MethodInfo& method) {
    for (auto count = requiredParameterCount(method); count <= method.parameters.size(); ++count) {
        out << "%ignore " << name << '(';
        for (size_t i = 0; i < count; ++i) {
            if (i) out << ", ";
            const auto& param = method.parameters[i];
            if (param.byConstReference) out << "const ";
            out << qualifiedTypeName(graph, param.type.type);
            if (param.byConstReference) out << '&';
        }
        out << (method.isConst ? ") const;\n" : ");\n");
    }
}

void collectMethods(SwigInterface& api, const TypeGraph& graph, const TypeNode& owner, const std::vector<MethodInfo>& methods) {
    std::set<std::string> ignored;
    for (const auto& m : methods) {
        if (m.cppPrivate) continue;
        const auto native = owner.qualifiedName + "::" + m.name;
        if (m.explicitCppOnly) {
            const bool mixed = std::ranges::any_of(methods, [&](const auto& other) {
                return other.name == m.name && !other.cppPrivate && !other.explicitCppOnly;
            });
            if (mixed) ignoreOverload(api.ignored, graph, native, m);
            else if (ignored.insert(m.name).second) api.ignored << "%ignore " << native << ";\n";
        } else if (m.declaredProperty.empty()) {
            api.members[native].push_back(&m);
        } else if (m.accessor == AccessorKind::Getter) {
            api.members.try_emplace(owner.qualifiedName + "::" + m.declaredProperty);
            api.attributes << "%attribute(" << owner.qualifiedName << ", " << attributeType(graph, m) << ", "
                           << m.declaredProperty << ", " << m.name;
            const auto setter = std::ranges::find_if(methods, [&](const auto& other) {
                return other.accessor == AccessorKind::Setter && other.declaredProperty == m.declaredProperty;
            });
            if (setter != methods.end()) {
                const auto& param = setter->parameters.front().type;
                if (canonicalType(graph, {propertyValue(graph, m.result.type), m.result.location}) !=
                    canonicalType(graph, {propertyValue(graph, param.type), param.location}))
                    throw std::runtime_error(param.location.describe() + ": SWIG setter type differs from getter for " + m.declaredProperty);
                api.attributes << ", " << setter->name;
            }
            api.attributes << ");\n";
        }
    }
}

std::string checkMember(const SwigMember& member, const std::string& scope, const SwigInterface& api,
                        std::set<std::string>& seen, const char* rule) {
    const auto name = (scope.empty() ? "" : scope + "::") + member.owner + "::" + member.name;
    if (!api.members.contains(name)) throw std::runtime_error(member.location.describe() + ": unknown SWIG " + rule + " target " + name);
    if (!seen.insert(name).second) throw std::runtime_error(member.location.describe() + ": duplicate SWIG " + rule + " for " + name);
    return name;
}

} // namespace

std::string emitSwig(const TypeGraph& graph, const TypeModule& module, const std::string& name, const Path& header,
                     const SwigBindings& bindings, const std::vector<Path>& jsbHeaders) {
    SwigInterface api;
    for (auto id : module.declarations) {
        const auto& node = graph.node(id);
        if (const auto* iface = std::get_if<InterfaceType>(&node.data)) collectMethods(api, graph, node, iface->methods);
        if (const auto* record = std::get_if<RecordType>(&node.data)) collectMethods(api, graph, node, record->methods);
        if (const auto* g = std::get_if<GraphType>(&node.data)) collectMethods(api, graph, node, g->methods);
    }
    std::ostringstream out;
    out << R"swig(// Define module
// target_namespace means the name exported to JS, could be same as which in other modules
// native2d at the last means the suffix of binding function name, different modules should use unique name
// Note: doesn't support number prefix
)swig"
        << "%module(target_namespace=\"" << (module.namespaces.empty() ? name : module.namespaces.back()) << "\") " << name << '\n'
        << R"swig(
// Disable some swig warnings, find warning number reference here ( https://www.swig.org/Doc4.1/Warnings.html )
#pragma SWIG nowarn=503,302,401,317,402

// Insert code at the beginning of generated header file (.h)
%insert(header_file) %{
#pragma once
#include <type_traits>
#include "bindings/jswrapper/SeApi.h"
#include "bindings/manual/jsb_conversions.h"
)swig"
        << "#include \"" << includeName(header) << "\"\n"
        << R"swig(%}

// Insert code at the beginning of generated source file (.cpp)
%{
)swig"
        << "#include \"bindings/auto/jsb_" << name << "_auto.h\"\n";
    for (const auto& path : bindings.includes) out << "#include \"" << path.generic_string() << "\"\n";
    for (const auto& path : jsbHeaders) out << "#include \"" << includeName(path) << "\"\n";
    out << '\n';
    std::string scope;
    for (const auto& part : module.namespaces) {
        if (!scope.empty()) scope += "::";
        scope += part;
        out << "using namespace " << scope << ";\n";
    }
    out << R"swig(%}

// ----- Ignore Section ------
// Brief: Classes, methods or attributes need to be ignored
//
// Usage:
//
//  %ignore your_namespace::your_class_name;
//  %ignore your_namespace::your_class_name::your_method_name;
//  %ignore your_namespace::your_class_name::your_attribute_name;
//
// Note:)swig" " " R"swig(
//  1. 'Ignore Section' should be placed before attribute definition and %import/%include
//  2. namespace is needed
//
)swig"
        << api.ignored.str()
        << R"swig(
// ----- Rename Section ------
// Brief: Classes, methods or attributes needs to be renamed
//
// Usage:
//
//  %rename(rename_to_name) your_namespace::original_class_name;
//  %rename(rename_to_name) your_namespace::original_class_name::method_name;
//  %rename(rename_to_name) your_namespace::original_class_name::attribute_name;
//)swig" " " R"swig(
// Note:
//  1. 'Rename Section' should be placed before attribute definition and %import/%include
//  2. namespace is needed

// ----- Module Macro Section ------
// Brief: Generated code should be wrapped inside a macro
// Usage:
//  1. Configure for class
//    %module_macro(CC_USE_GEOMETRY_RENDERER) cc::pipeline::GeometryRenderer;
//  2. Configure for member function or attribute
//    %module_macro(CC_USE_GEOMETRY_RENDERER) cc::pipeline::RenderPipeline::geometryRenderer;
// Note: Should be placed before 'Attribute Section'
)swig";
    std::set<std::string> seenMacros, seenReleases;
    for (const auto& [member, macro] : bindings.macros)
        out << "%module_macro(" << macro << ") " << checkMember(member, scope, api, seenMacros, "macro") << ";\n";
    out << R"swig(
// ----- Release Returned Cpp Object in GC Section ------
)swig";
    for (const auto& member : bindings.releaseInGC) {
        const auto target = checkMember(member, scope, api, seenReleases, "releaseInGC");
        const auto& methods = api.members.at(target);
        if (methods.empty() || !std::ranges::all_of(methods, [&](const auto* m) { return returnsObjectPointer(graph, *m); }))
            throw std::runtime_error(member.location.describe() + ": releaseInGC requires an object pointer return on every overload of " + target);
        out << "%release_returned_cpp_object_in_gc(" << target << ");\n";
    }
    out << R"swig(
// ----- Attribute Section ------
// Brief: Define attributes ( JS properties with getter and setter )
// Usage:
//  1. Define an attribute without setter
//    %attribute(your_namespace::your_class_name, cpp_member_variable_type, js_property_name, cpp_getter_name)
//  2. Define an attribute with getter and setter
//    %attribute(your_namespace::your_class_name, cpp_member_variable_type, js_property_name, cpp_getter_name, cpp_setter_name)
//  3. Define an attribute without getter
//    %attribute_writeonly(your_namespace::your_class_name, cpp_member_variable_type, js_property_name, cpp_setter_name)
//
// Note:
//  1. Don't need to add 'const' prefix for cpp_member_variable_type)swig" " " R"swig(
//  2. The return type of getter should keep the same as the type of setter's parameter
//  3. If using reference, add '&' suffix for cpp_member_variable_type to avoid generated code using value assignment
//  4. 'Attribute Section' should be placed before 'Import Section' and 'Include Section'
//
)swig"
        << api.attributes.str()
        << R"swig(
// ----- Import Section ------
// Brief: Import header files which are depended by 'Include Section'
// Note:)swig" " " R"swig(
//   %import "your_header_file.h" will not generate code for that header file
//

// ----- Include Section ------
// Brief: Include header files in which classes and methods will be bound
)swig"
        << "%include \"" << includeName(header) << "\"\n";
    return out.str();
}

} // namespace cc::codegen
