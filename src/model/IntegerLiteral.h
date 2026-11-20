#pragma once

#include <charconv>
#include <cstdint>
#include <string_view>

namespace cc::codegen {

inline bool hasHexPrefix(std::string_view digits) {
    return digits.starts_with("0x") || digits.starts_with("0X");
}

inline std::errc parseMagnitude(std::string_view digits, uint64_t& magnitude) {
    const bool hex = hasHexPrefix(digits);
    if (hex) digits.remove_prefix(2);
    if (digits.empty() || (!hex && digits.size() > 1 && digits.front() == '0')) return std::errc::invalid_argument;
    const auto end = digits.data() + digits.size();
    const auto parsed = std::from_chars(digits.data(), end, magnitude, hex ? 16 : 10);
    return parsed.ptr == end ? parsed.ec : std::errc::invalid_argument;
}

inline bool validUnsignedFieldDefault(std::string_view type, std::string_view literal) {
    uint64_t value{};
    if (parseMagnitude(literal, value) != std::errc{}) return false;
    return (type == "uint8_t" && value <= UINT8_MAX) || (type == "uint16_t" && value <= UINT16_MAX) ||
           (type == "uint32_t" && value <= UINT32_MAX) || type == "uint64_t";
}

inline bool safeUnsignedNumber(std::string_view literal) {
    uint64_t value{};
    return parseMagnitude(literal, value) == std::errc{} && value <= 9007199254740991ULL;
}

} // namespace cc::codegen
