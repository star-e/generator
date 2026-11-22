#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

namespace cc::codegen {

int generate(const std::filesystem::path& workspace,
             const std::optional<std::filesystem::path>& output, bool check, bool diff,
             std::optional<std::string_view> selectedModule = std::nullopt);

} // namespace cc::codegen
