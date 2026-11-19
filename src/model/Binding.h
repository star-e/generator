#pragma once

#include "model/SourceLocation.h"
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace cc::codegen {

struct PoolBinding {
    std::string member;
    std::set<std::string> keepOnReset;
};

enum class BindingTarget { type, interfaceType, classType, enumeration, constEnum };
enum class ResetBinding { constructor, defaults };

struct TypeScriptBinding {
    SourceLocation location;
    std::string source;
    BindingTarget kind{};
    std::string target;
    std::filesystem::path from;
    std::map<std::string, std::string> methods;
    std::map<std::string, std::string> renamedFrom;
    std::optional<PoolBinding> pool;
    std::optional<ResetBinding> reset;
    bool names{};
    bool registerEnum{};
    bool helpers{};
};

struct ModuleBindings {
    std::vector<TypeScriptBinding> typescript;
    std::filesystem::path poolModule;
    std::optional<SourceLocation> jsb;
    std::map<std::string, SourceLocation> jsbRecords;
    std::set<std::filesystem::path> jsbIncludes;
};

} // namespace cc::codegen
