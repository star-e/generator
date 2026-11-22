#pragma once

#include "generated/SyntaxTypes.h"
#include "model/Binding.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace cc::codegen {

struct Module {
    std::string name;
    std::vector<std::string> namespaces;
    std::vector<Declaration> declarations;
    std::vector<ModuleImport> imports;
    ModuleBindings bindings;
    std::string cppHeader;
    bool external{};
    bool pool{};
};

struct ExternalType {
    std::string header;
    std::optional<std::string> typescript;
    bool builtin{};
    std::string typescriptModule;
};
using TypeTable = std::map<std::string, ExternalType>;
} // namespace cc::codegen
