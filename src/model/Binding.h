#pragma once

#include "model/TypeRef.h"
#include "generated/TypeGraphFwd.h"
#include <algorithm>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace cc::codegen {

struct PoolBinding {
    std::string member;
    std::set<std::string> keepOnReset;
    bool assignFields{};
};

struct PoolDependency {
    std::string member;
    std::string module;
};

enum class BindingTarget { type, interfaceType, classType, enumeration, constEnum, numberField };
struct ResetBinding {
    enum class Kind { constructor, defaults, fields };
    Kind kind{Kind::constructor};
    std::vector<std::string> fields;
};

struct TypeScriptImport {
    std::string name;
    std::filesystem::path from;
};

struct InterfaceMethodBinding {
    std::string target;
    AccessorKind accessor{};
    bool nullable{};
    std::vector<std::string> optional;
};
using MethodBindings = std::map<std::string, InterfaceMethodBinding>;
using InterfaceBindings = std::map<std::pair<std::string, std::string>, InterfaceMethodBinding>;

struct CallbackParameter {
    std::string name;
    bool optional{};
    TypeRef type;
};

struct Callback {
    std::string name;
    bool optional{};
    std::vector<CallbackParameter> parameters;
    TypeRef result;
    bool optionalResult{};
};

struct TypeScriptBinding {
    TypeRef source;
    BindingTarget kind{};
    std::string target;
    std::filesystem::path from;
    std::filesystem::path archiveFrom;
    bool externalTarget{};
    bool externalInterface{};
    MethodBindings methods;
    std::vector<Callback> callbacks;
    std::map<std::string, std::string> renamedFrom;
    std::string numberField;
    std::map<std::string, std::string> fieldDefaults;
    std::map<std::string, TypeScriptImport> fieldResets;
    std::optional<TypeScriptImport> base;
    std::optional<PoolBinding> pool;
    std::optional<std::vector<std::string>> constructor;
    bool requiredConstructor{};
    std::optional<ResetBinding> reset;
    MapKeyKind mapKey{};
    bool names{};
    bool registerEnum{};
    bool helpers{};
};

inline bool isTypedArrayName(std::string_view name) {
    static constexpr std::string_view names[]{
        "Int8Array", "Uint8Array", "Uint8ClampedArray", "Int16Array", "Uint16Array",
        "Int32Array", "Uint32Array", "Float32Array", "Float64Array"};
    return std::ranges::find(names, name) != std::ranges::end(names);
}

struct ModuleBindings {
    std::vector<TypeScriptBinding> typescript;
    std::filesystem::path typescriptFrom;
    std::map<std::string, SourceLocation> typescriptExports;
    std::filesystem::path poolModule;
    std::vector<PoolDependency> poolDependencies;
    std::optional<SourceLocation> jsb;
    std::map<std::string, SourceLocation> jsbRecords;
    std::set<std::filesystem::path> jsbIncludes;
};

} // namespace cc::codegen
