#pragma once

#include <string>

namespace cc::codegen {

struct BoundField {
    std::string name;
    std::string cpp;
    std::string ts;
    std::string cppDefault;
    std::string tsDefault;
    std::string comment;
    bool move{};
    bool byConstReference{};
    bool readonly{};
    bool transient{};
    bool usesAllocator{};
    bool scalar{};
    bool safeUnsignedInteger{};
    bool checkDefaultConstruction{};
    std::string emptyPmrArgument;
    std::string tsReset;

    std::string allocatorArguments() const {
        return emptyPmrArgument.empty() ? "alloc" : "::cc::detail::makeEmptyPmr<" + cpp + ">(alloc, " + emptyPmrArgument + ")";
    }

    std::string parameterType() const { return byConstReference ? "const " + cpp + "&" : cpp; }
};

} // namespace cc::codegen
