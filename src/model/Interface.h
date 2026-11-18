#pragma once

#include "model/SourceLocation.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace cc::codegen {

struct TypeRef {
    SourceLocation location;
    std::string name;
    bool pointer{};
};

struct Parameter {
    TypeRef type;
    std::string name;
};

struct Method {
    TypeRef result;
    std::string name;
    std::vector<Parameter> parameters;
    bool isConst{};
    bool isNoexcept{};
};

struct Interface {
    std::string name;
    std::vector<Method> methods;
    SourceLocation location;
};

struct Module {
    std::string name;
    std::vector<std::string> namespaces;
    std::vector<Interface> interfaces;
};

struct ExternalType {
    std::string header;
    std::optional<std::string> typescript;
    bool builtin{};
};
using TypeTable = std::map<std::string, ExternalType>;
using Projection = std::map<std::string, std::map<std::string, std::optional<std::string>>>;
} // namespace cc::codegen
