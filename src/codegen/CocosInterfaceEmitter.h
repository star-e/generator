#pragma once

#include "model/CocosInterface.h"

namespace cc::codegen {

Files emitInterfaces(const Module& module, const OutputLayout& layout, const TypeTable& types,
                     const Projection& projection, const std::string& preamble);

} // namespace cc::codegen
