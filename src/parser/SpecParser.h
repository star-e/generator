#pragma once

#include "model/Interface.h"

namespace cc::codegen {

Module parseModule(const std::filesystem::path& file, const std::string& source);
std::filesystem::path parseBindingPath(const std::string& text, const std::string& context);

} // namespace cc::codegen
