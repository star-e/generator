#pragma once

#include "generated/TypeGraphTypes.h"
#include "model/Interface.h"

namespace cc::codegen {

struct ModuleInput {
    Module syntax;
    TypeTable externalTypes;
    bool modelOnly{};
};

// Register declarations before binding direct imports and resolving type uses in dependency order.
TypeGraph buildTypeGraph(const std::vector<ModuleInput>& inputs, const TypeTable& externalTypes,
                         std::vector<ModuleId>* dependencyOrder = nullptr);

std::string componentAccessorSuffix(std::string tagName);
std::optional<uint64_t> indexedVertexLimit(const std::string& type);

} // namespace cc::codegen
