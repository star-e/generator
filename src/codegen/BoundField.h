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
    std::string documentation;
    bool move{};
    bool byConstReference{};
    bool referenceMember{};
    bool readonly{};
    bool tsKeepOnReset{};
    bool tsResetsObjectInPlace{};
    bool transient{};
    bool usesAllocator{};
    bool defaultNeedsAllocator{};
    bool containsGraph{};
    bool safeUnsignedInteger{};
    std::string tsReset;

    std::string parameterType() const { return byConstReference ? "const " + cpp + "&" : cpp; }

    std::string tsUint64Initializer(const std::string& initial) const {
        return "defineUint64(this, '" + name + "', " + initial + ")";
    }
};

} // namespace cc::codegen
