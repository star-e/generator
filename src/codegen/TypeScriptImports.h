#pragma once

#include "model/TypeName.h"
#include <filesystem>
#include <map>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>

namespace cc::codegen {

using TypeImports = std::map<std::string, std::set<std::string>>;

template<class Visitor>
void visitTypeScriptImportSources(const TypeGraph& graph, const TypeNode& node, const Visitor& visit) {
    if (node.typescriptContainer == ContainerKind::None) {
        visit(node);
        return;
    }
    const auto& arguments = std::get<TemplateInstanceType>(node.data).arguments;
    for (size_t i = 0; i < arguments.size(); ++i) {
        const auto& argument = graph.node(std::get<TypeId>(arguments[i]));
        if (!usesJsonMapKey(node, i, argument)) visitTypeScriptImportSources(graph, argument, visit);
    }
}

inline const std::string& boundTypeScriptName(const TypeGraph& graph, TypeId id, ModuleId owner, TypeImports& imports,
                                              const std::string& context) {
    const auto& node = graph.node(id);
    if (!node.typescriptName) throw std::runtime_error(context + ": no TS mapping for " + qualifiedTypeName(graph, id));
    visitTypeScriptImportSources(graph, node, [&](const TypeNode& source) {
        if (source.module == owner) return;
        if (!source.typescriptModule.empty()) imports[source.typescriptModule].insert(*source.typescriptName);
        else if (node.typescriptContainer != ContainerKind::None && source.module != ModuleId{} &&
                 (std::holds_alternative<RecordType>(source.data) || std::holds_alternative<EnumType>(source.data) ||
                  std::holds_alternative<InterfaceType>(source.data) || std::holds_alternative<GraphType>(source.data))) {
            throw std::runtime_error(context + ": missing TypeScript import mapping for " + source.qualifiedName);
        }
    });
    return *node.typescriptName;
}

inline const std::string& boundTypeScriptName(const TypeGraph& graph, const TypeUse& use, ModuleId owner, TypeImports& imports) {
    return boundTypeScriptName(graph, use.type, owner, imports, use.location.describe());
}

inline std::string typeScriptContainerInitializer(const TypeNode& node) {
    if (node.typescriptContainer == ContainerKind::Array) return "[]";
    if (isAssociativeContainer(node.typescriptContainer)) return "new " + *node.typescriptName + "()";
    return "new " + *node.typescriptName + "(0)";
}

inline std::string modulePath(const std::filesystem::path& target, const std::filesystem::path& from) {
    const auto path = target.lexically_relative(from.parent_path()).generic_string();
    return path.starts_with("../") ? path : "./" + path;
}

inline void writeImports(std::ostream& out, const TypeImports& imports, const std::filesystem::path& from, bool typeOnly) {
    for (const auto& [path, names] : imports) {
        out << (typeOnly ? "import type { " : "import { ");
        for (auto it = names.begin(); it != names.end(); ++it) {
            if (it != names.begin()) out << ", ";
            out << *it;
        }
        out << " } from '" << modulePath(path, from) << "';\n";
    }
}

} // namespace cc::codegen
