#pragma once

#include "generated/TypeGraphTypes.h"

#include <ostream>

namespace cc::codegen {

inline void writeEnumMembers(std::ostream& out, const EnumType& enumeration) {
    for (const auto& member : enumeration.values) {
        out << "    " << member.name;
        if (!member.initializer.empty()) out << " = " << member.initializer;
        out << ",\n";
    }
}

} // namespace cc::codegen
