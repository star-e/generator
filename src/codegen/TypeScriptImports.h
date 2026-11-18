#pragma once

#include <filesystem>
#include <map>
#include <ostream>
#include <set>
#include <string>

namespace cc::codegen {

using TypeImports = std::map<std::string, std::set<std::string>>;

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
