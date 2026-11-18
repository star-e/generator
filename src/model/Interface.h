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

struct Record {
    std::vector<Field> fields;
};

struct Enum {
    std::vector<std::string> values;
};

struct ExternalDeclaration {};

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
};
using TypeTable = std::map<std::string, ExternalType>;
using Projection = std::map<std::string, std::map<std::string, std::optional<std::string>>>;
} // namespace cc::codegen
