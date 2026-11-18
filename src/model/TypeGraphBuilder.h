#pragma once

#include "generated/TypeGraphTypes.h"
#include "model/Interface.h"

namespace cc::codegen {

struct ModuleInput {
    Module syntax;
    Projection projection;
};

// Register declarations before binding direct imports and resolving type uses in dependency order.
TypeGraph buildTypeGraph(const std::vector<ModuleInput>& inputs, const TypeTable& externalTypes);

} // namespace cc::codegen
