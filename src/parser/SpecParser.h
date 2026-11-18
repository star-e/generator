#pragma once

#include "model/Interface.h"

namespace cc::codegen {

Module parseModule(const std::filesystem::path& file, const std::string& source);

} // namespace cc::codegen
