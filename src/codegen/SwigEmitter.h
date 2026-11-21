#pragma once

#include "generated/TypeGraphCoreFwd.h"
#include "generated/TypeGraphFwd.h"
#include "model/Binding.h"

namespace cc::codegen {

std::string emitSwig(const TypeGraph& graph, const TypeModule& module, const std::string& name,
                     const std::filesystem::path& header, const SwigBindings& bindings,
                     const std::vector<std::filesystem::path>& jsbHeaders);

} // namespace cc::codegen
