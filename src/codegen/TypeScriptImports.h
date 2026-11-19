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

inline const TypeNode& typeScriptImportSource(const TypeGraph& graph, TypeId id) {
    while (graph.node(id).typescriptArrayElement != TypeId{}) id = graph.node(id).typescriptArrayElement;
    return graph.node(id);
}

inline const std::string& boundTypeScriptName(const TypeGraph& graph, const TypeUse& use, ModuleId owner, TypeImports& imports) {
    const auto& node = graph.node(use.type);
    if (!node.typescriptName) throw std::runtime_error(use.location.describe() + ": no TS mapping for " + qualifiedTypeName(graph, use.type));
    const auto& source = typeScriptImportSource(graph, use.type);
    if (source.module != owner && !source.typescriptModule.empty()) imports[source.typescriptModule].insert(*source.typescriptName);
    else if (node.typescriptArrayElement != TypeId{} && source.module != ModuleId{} && source.module != owner &&
             (std::holds_alternative<RecordType>(source.data) || std::holds_alternative<EnumType>(source.data) ||
              std::holds_alternative<InterfaceType>(source.data))) {
        throw std::runtime_error(use.location.describe() + ": missing TypeScript import mapping for " + source.qualifiedName);
    }
    return *node.typescriptName;
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
