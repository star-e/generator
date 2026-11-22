#pragma once

#include "generated/SyntaxTypes.h"
#include "generated/TypeGraphTypes.h"

namespace cc::codegen {

struct ModuleInput {
    Module syntax;
    std::string nativeHeader;
    std::string forwardHeader;
    std::string typescriptModule;
    bool typescriptSettings{};

    bool modelOnly() const noexcept { return nativeHeader.empty() && typescriptModule.empty(); }
};

// Register declarations before binding direct imports and resolving type uses in dependency order.
TypeGraph buildTypeGraph(const std::vector<ModuleInput>& inputs, std::vector<ModuleId>* dependencyOrder = nullptr);

std::string componentAccessorSuffix(std::string tagName);
bool isArithmeticType(const std::string& type);
std::optional<uint64_t> indexedVertexLimit(const std::string& type);

} // namespace cc::codegen
