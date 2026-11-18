#pragma once

#include "model/SourceLocation.h"
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace cc::codegen {

struct TypeRef {
    SourceLocation location;
    std::string name;
    bool pointer{};
    std::vector<TypeRef> arguments;
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
    std::vector<Method> methods;
};

struct Field {
    TypeRef type;
    std::string name;
    std::string initializer;
    bool isReadonly{};
    bool isNullable{};
};

struct Constructor {
    SourceLocation location;
    std::vector<std::string> fields;
};

struct Record {
    std::vector<Field> fields;
    std::vector<Constructor> constructors;
    std::vector<Method> methods;
};

struct LeftShift {
    std::string count;
    SourceLocation location;
};

struct EnumMember {
    std::string name;
    std::string initializer;
    SourceLocation initializerLocation;
    std::optional<LeftShift> leftShift;
};

struct Enum {
    std::vector<EnumMember> values;
    std::optional<TypeRef> underlying;
    bool isFlags{};
};

struct ExternalDeclaration { bool isStruct{}; };

struct Declaration {
    std::string name;
    SourceLocation location;
    std::variant<Interface, Record, Enum, ExternalDeclaration> data;
};

struct ModuleImport {
    SourceLocation location;
    std::string name;
};

struct Module {
    std::string name;
    std::vector<std::string> namespaces;
    std::vector<Declaration> declarations;
    std::vector<ModuleImport> imports;
};

struct ExternalType {
    std::string header;
    std::optional<std::string> typescript;
    bool builtin{};
    std::string typescriptModule;
};
using TypeTable = std::map<std::string, ExternalType>;
using Projection = std::map<std::string, std::map<std::string, std::optional<std::string>>>;
} // namespace cc::codegen
