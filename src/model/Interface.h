#pragma once

#include "model/Binding.h"
#include <map>
#include <optional>
#include <set>
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
    bool cppOnly{};
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
    bool transient{};
};

struct Constructor {
    SourceLocation location;
    std::vector<std::string> fields;
};

struct Record {
    bool moveOnly{};
    bool serializable{};
    std::set<std::string> comparisons;
    bool hashable{};
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
    std::optional<std::string> deprecationMessage;
};

struct Enum {
    std::vector<EnumMember> values;
    std::optional<TypeRef> underlying;
    bool isFlags{};
};

struct ExternalDeclaration { bool isStruct{}; };
struct TagDeclaration {};

struct GraphComponent {
    TypeRef tag;
    TypeRef value;
    std::string member;
};

struct GraphDeclaration {
    std::string vertexAlias;
    std::optional<TypeRef> vertexType;
    bool indexed{};
    bool vertexList{};
    bool mutableGraph{};
    bool hierarchy{};
    std::optional<TypeRef> nameComponent;
    std::optional<SourceLocation> parentGraph;
    std::optional<SourceLocation> addressableGraph;
    std::vector<GraphComponent> components;
};

struct Declaration {
    std::string name;
    SourceLocation location;
    std::variant<Interface, Record, Enum, ExternalDeclaration, TagDeclaration, GraphDeclaration> data;
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
    ModuleBindings bindings;
};

struct ExternalType {
    std::string header;
    std::optional<std::string> typescript;
    bool builtin{};
    std::string typescriptModule;
};
using TypeTable = std::map<std::string, ExternalType>;
} // namespace cc::codegen
