#pragma once

#include "generated/TypeGraphTypes.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace cc::codegen {

enum class EnumTarget { cpp, typescript };

inline std::string enumInitializer(const Enumerator& member, EnumTarget target) {
    auto init = member.initializer;
    if (init.find(" << ") != std::string::npos) {
        const auto value = std::visit([](auto n) { return static_cast<uint64_t>(n); }, member.value);
        // A positive result fitting int32_t also guarantees a shift count below 31.
        // Otherwise use the checked value to avoid JS shift wrapping and C++ int promotion.
        if (value > 0 && value <= INT32_MAX) return init;
        init = std::to_string(value);
    }
    if (target == EnumTarget::typescript) return init;
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
        if (member.deprecationMessage) {
            auto message = *member.deprecationMessage;
            // Keep a literal block-comment terminator inside the documentation.
            for (auto pos = message.find("*/"); pos != std::string::npos; pos = message.find("*/", pos + 1)) {
                message.replace(pos, 2, "*&#47;");
            }
            out << "    /**\n     * @deprecated";
            if (!message.empty()) out << ' ' << message;
            out << "\n     */\n";
        }
        out << "    " << member.name;
        if (!member.initializer.empty()) {
            out << " = " << enumInitializer(member, target);
        }
        out << ",\n";
    }
}

} // namespace cc::codegen
