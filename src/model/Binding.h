#pragma once

#include <generated/SyntaxTypes.h> // IWYU pragma: export
#include <algorithm>
#include <iterator>
#include <string_view>

namespace cc::codegen {

inline bool isUnclampedIntegerTypedArrayName(std::string_view name) {
    static constexpr std::string_view names[]{
        "Int8Array", "Uint8Array", "Int16Array", "Uint16Array", "Int32Array", "Uint32Array"};
    return std::ranges::find(names, name) != std::ranges::end(names);
}

inline bool isTypedArrayName(std::string_view name) {
    return isUnclampedIntegerTypedArrayName(name) || name == "Uint8ClampedArray" || name == "Float32Array" || name == "Float64Array";
}

} // namespace cc::codegen
