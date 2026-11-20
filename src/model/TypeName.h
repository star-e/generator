#pragma once

#include "generated/TypeGraphTypes.h"

namespace cc::codegen {

inline bool isAssociativeContainer(ContainerKind kind) {
    return kind == ContainerKind::Map || kind == ContainerKind::Set;
}

inline bool usesStringMapKey(const TypeNode& container, size_t position, const TypeNode& argument) {
    return container.typescriptContainer == ContainerKind::Map && position == 0 && argument.typescriptMapKey != MapKeyKind::Native;
}

// In a TS collection, a native resource handle is represented by its referenced object.
inline const TypeNode& typeScriptElement(const TypeGraph& graph, TypeId id) {
    const auto& node = graph.node(id);
    const auto* instance = std::get_if<TemplateInstanceType>(&node.data);
    if (const auto* ptr = std::get_if<PointerType>(&node.data)) {
        id = ptr->pointee;
    } else if (!node.typescriptName && instance && graph.node(instance->declaration).qualifiedName == "cc::IntrusivePtr" &&
               instance->arguments.size() == 1 && std::holds_alternative<TypeId>(instance->arguments.front())) {
        id = std::get<TypeId>(instance->arguments.front());
    } else {
        return node;
    }
    if (const auto* q = std::get_if<QualifiedType>(&graph.node(id).data)) id = q->base;
    const auto& target = graph.node(id);
    return std::holds_alternative<ExternalTypeNode>(target.data) && !target.typescriptModule.empty() ? target : node;
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
