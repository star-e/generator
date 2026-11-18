#pragma once

#include "generated/TypeGraphTypes.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace cc::codegen {

enum class EnumTarget { cpp, typescript };

inline std::string cppEnumInitializer(const Enumerator& member) {
    const auto& init = member.initializer;
    const auto* signedValue = std::get_if<int64_t>(&member.value);
    const auto* unsignedValue = std::get_if<uint64_t>(&member.value);
    // The magnitude of INT64_MIN has no signed C++ literal; wide decimal and negative hex need explicit types.
    if (init.starts_with('-') && signedValue && *signedValue == INT64_MIN) return "(-9223372036854775807LL - 1)";
    if (init.starts_with("-0x") || init.starts_with("-0X")) return init + "LL";
    if (init.front() >= '1' && init.front() <= '9' && unsignedValue && *unsignedValue > INT64_MAX) return init + "ULL";
    return init;
}

inline void writeEnumMembers(std::ostream& out, const EnumType& enumeration, EnumTarget target) {
    for (const auto& member : enumeration.values) {
        if (target == EnumTarget::typescript) {
            const bool safe = std::visit([](auto n) {
                constexpr int64_t maxSafe = 9007199254740991;
                return !std::cmp_less(n, -maxSafe) && !std::cmp_greater(n, maxSafe);
            }, member.value);
            if (!safe) throw std::runtime_error(member.name + ": enum value exceeds the TypeScript safe integer range");
        }
        out << "    " << member.name;
        if (!member.initializer.empty()) {
            out << " = " << (target == EnumTarget::cpp ? cppEnumInitializer(member) : member.initializer);
        }
        out << ",\n";
    }
}

} // namespace cc::codegen
