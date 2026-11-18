#pragma once

#include "model/CocosInterface.h"

namespace cc::codegen {

Module parseModule(const std::filesystem::path& file, const std::string& source);

} // namespace cc::codegen
