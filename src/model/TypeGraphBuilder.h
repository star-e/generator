#pragma once

#include "generated/TypeGraphTypes.h"
#include "model/Interface.h"

namespace cc::codegen {

struct ModuleInput {
    Module syntax;
    Projection projection;
};

// Register all declarations first, then resolve signatures against their lexical scopes.
TypeGraph buildTypeGraph(const std::vector<ModuleInput>& inputs, const TypeTable& externalTypes);

} // namespace cc::codegen
