#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

namespace cc::codegen {

struct SourceLocation {
    std::filesystem::path file;
    std::size_t line{};
    std::size_t column{};
    std::string describe() const;
};

} // namespace cc::codegen
