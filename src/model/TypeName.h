#pragma once

#include "generated/TypeGraphTypes.h"
#include <set>
#include <stdexcept>

namespace cc::codegen {

inline TypeId canonicalType(const TypeGraph& graph, TypeUse use) {
    std::set<uint32_t> visited;
    while (const auto* alias = std::get_if<AliasType>(&graph.node(use.type).data)) {
        if (!visited.insert(use.type.value).second) throw std::runtime_error(use.location.describe() + ": cyclic type alias");
        use.type = alias->target;
    }
    return use.type;
}

inline size_t requiredParameterCount(const MethodInfo& method) {
    auto count = method.parameters.size();
    while (count && !method.parameters[count - 1].defaultValue.empty()) --count;
    return count;
}

inline void collectCppHeader(const TypeGraph& graph, const TypeModule& consumer, const TypeNode& type,
                             std::set<std::string>& headers) {
    if (!type.cppHeader.empty()) {
        headers.insert(type.cppHeader);
    } else if (type.module != ModuleId{} && &graph.modules.at(type.module.value) != &consumer &&
               (std::holds_alternative<RecordType>(type.data) || std::holds_alternative<InterfaceType>(type.data) ||
                std::holds_alternative<GraphType>(type.data) || std::holds_alternative<AliasType>(type.data) ||
                std::holds_alternative<EnumType>(type.data))) {
        throw std::runtime_error(consumer.name + ": missing C++ header for imported type " + type.qualifiedName);
    }
}

inline bool isAssociativeContainer(ContainerKind kind) {
    return kind == ContainerKind::Map || kind == ContainerKind::Set;
}

inline bool usesStringMapKey(const TypeNode& container, size_t position, const TypeNode& argument) {
    return container.typescriptContainer == ContainerKind::Map && position == 0 && argument.typescriptMapKey != MapKeyKind::Native;
}

// In a TS collection, a native resource handle is represented by its referenced object.
inline TypeId typeScriptElementId(const TypeGraph& graph, TypeId id) {
    const auto original = id;
    const auto& node = graph.node(id);
    const auto* instance = std::get_if<TemplateInstanceType>(&node.data);
    if (const auto* ptr = std::get_if<PointerType>(&node.data)) {
        id = ptr->pointee;
    } else if (!node.typescriptName && instance && graph.node(instance->declaration).qualifiedName == "cc::IntrusivePtr" &&
               instance->arguments.size() == 1 && std::holds_alternative<TypeId>(instance->arguments.front())) {
        id = std::get<TypeId>(instance->arguments.front());
    } else {
        return original;
    }
    if (const auto* q = std::get_if<QualifiedType>(&graph.node(id).data)) id = q->base;
    const auto& target = graph.node(id);
    return (std::holds_alternative<ExternalTypeNode>(target.data) || std::holds_alternative<InterfaceType>(target.data)) &&
           !target.typescriptModule.empty() ? id : original;
}

inline const TypeNode& typeScriptElement(const TypeGraph& graph, TypeId id) {
    return graph.node(typeScriptElementId(graph, id));
}

inline std::optional<std::string> typeScriptContainerName(const TypeGraph& graph, const TypeNode& container) {
    const auto& arguments = std::get<TemplateInstanceType>(container.data).arguments;
    const auto& first = typeScriptElement(graph, std::get<TypeId>(arguments.front()));
    if (!first.typescriptName) return std::nullopt;
    const auto& name = *first.typescriptName;
    if (container.typescriptContainer == ContainerKind::Set) return "Set<" + name + ">";
    if (container.typescriptContainer == ContainerKind::Map) {
        const auto& value = typeScriptElement(graph, std::get<TypeId>(arguments.at(1)));
        if (!value.typescriptName) return std::nullopt;
        return "Map<" + (usesStringMapKey(container, 0, first) ? "string" : name) + ", " + *value.typescriptName + ">";
    }
    if (first.typescriptContainer != ContainerKind::None ||
        name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$.") == std::string::npos) {
        return name + "[]";
    }
    // Preserve grouping in explicit bindings such as "A | B".
    return "Array<" + name + ">";
}

template<class NameOf>
std::string formatTypeName(const TypeGraph& graph, TypeId id, const NameOf& nameOf) {
    const auto& node = graph.node(id);
    if (const auto* q = std::get_if<QualifiedType>(&node.data)) {
        auto base = formatTypeName(graph, q->base, nameOf);
        if (std::holds_alternative<PointerType>(graph.node(q->base).data))
            return base + (q->isConst ? " const" : "") + (q->isVolatile ? " volatile" : "");
        return std::string(q->isConst ? "const " : "") + (q->isVolatile ? "volatile " : "") + base;
    }
    if (const auto* pointer = std::get_if<PointerType>(&node.data))
        return formatTypeName(graph, pointer->pointee, nameOf) + "*";
    if (const auto* ref = std::get_if<ReferenceType>(&node.data))
        return formatTypeName(graph, ref->referred, nameOf) + (ref->rvalue ? "&&" : "&");
    if (const auto* instance = std::get_if<TemplateInstanceType>(&node.data)) {
        std::string name = nameOf(graph.node(instance->declaration)) + "<";
        for (size_t i = 0; i < instance->arguments.size(); ++i) {
            if (i) name += ", ";
            const auto& arg = instance->arguments[i];
            if (const auto* type = std::get_if<TypeId>(&arg)) name += formatTypeName(graph, *type, nameOf);
            else name += std::get<std::string>(arg);
        }
        return name + ">";
    }
    return nameOf(node);
}

inline std::string qualifiedTypeName(const TypeGraph& graph, TypeId id) {
    return formatTypeName(graph, id, [](const TypeNode& type) { return type.qualifiedName; });
}

inline std::string qualifyIfShadowed(const TypeGraph& graph, const TypeNode& type, const TypeModule& module, std::string name) {
    std::string scope;
    for (const auto& ns : module.namespaces) {
        scope += ns + "::";
        const auto other = graph.findType(scope + name);
        if (other != TypeId{} && graph.node(other).qualifiedName != type.qualifiedName) return "::" + type.qualifiedName;
    }
    return name;
}

} // namespace cc::codegen
