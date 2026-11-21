#pragma once

#include <ostream>
#include <string_view>

namespace cc::codegen {

inline void writeDocumentation(std::ostream& out, std::string_view text, std::string_view indent = {}) {
    if (text.empty()) return;
    out << indent << "/**\n";
    while (!text.empty()) {
        const auto end = text.find('\n');
        const auto line = text.substr(0, end);
        out << indent << " *";
        if (!line.empty()) out << ' ' << line;
        out << '\n';
        if (end == text.npos) break;
        text.remove_prefix(end + 1);
    }
    out << indent << " */\n";
}

} // namespace cc::codegen
