#pragma once

#include "generated/TypeGraphTypes.h"
#include "generated/WorkspaceTypes.h"

namespace cc::codegen {

// Register declarations before binding direct imports and resolving type uses in dependency order.
TypeGraph buildTypeGraph(const std::vector<ModuleInput>& inputs, std::vector<ModuleId>* dependencyOrder = nullptr);

std::string componentAccessorSuffix(std::string tagName);
bool isArithmeticType(const std::string& type);
std::optional<uint64_t> indexedVertexLimit(const std::string& type);

} // namespace cc::codegen
