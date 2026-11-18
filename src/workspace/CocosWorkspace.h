#pragma once

#include <filesystem>
#include <optional>

namespace cc::codegen {

int generate(const std::filesystem::path& workspace,
             const std::optional<std::filesystem::path>& output, bool check, bool diff);

} // namespace cc::codegen
