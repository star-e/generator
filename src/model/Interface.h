#pragma once

#include "generated/SyntaxTypes.h"
#include <map>
#include <optional>
#include <string>

namespace cc::codegen {

struct ExternalType {
    std::string header;
    std::optional<std::string> typescript;
    bool builtin{};
    std::string typescriptModule;
};
using TypeTable = std::map<std::string, ExternalType>;
} // namespace cc::codegen
