#pragma once

#include "generated/SyntaxTypes.h"
#include "generated/TypeGraphFwd.h"
#include "model/Binding.h"
#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace cc::codegen {

enum class ArchiveRole { none, input, output };

struct Interface {
    std::vector<Method> methods;
    std::vector<TypeRef> bases;
    ArchiveRole archiveRole{};
};

struct Record {
    bool isClass{};
    bool isFinal{};
    std::vector<TypeRef> bases;
    std::optional<TypeRef> implements;
    std::optional<bool> destructorNoexcept;
    bool destructorPrivate{};
    std::string cppMembers;
    bool moveOnly{};
    bool immovable{};
    bool serializable{};
    std::set<std::string> comparisons;
    bool hashable{};
    std::vector<Field> fields;
    std::vector<Field> staticFields;
    std::vector<Constructor> constructors;
    std::vector<Method> methods;
    std::string destructorDocumentation;
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
    std::string documentation;
};

struct Enum {
    std::vector<EnumMember> values;
    std::optional<TypeRef> underlying;
    bool isFlags{};
};

struct ExternalDeclaration {
    bool isStruct{};
    bool isString{};
    bool isPmr{};
    bool constructorConstRef{};
    ContainerKind container{};
    std::vector<std::string> templateParameters;
    bool variadicTemplate{};
};
struct TagDeclaration {};
struct AliasDeclaration { TypeRef target; };

struct GraphComponent {
    TypeRef tag;
    TypeRef value;
    std::string member;
    std::string kindName;
};

struct GraphDeclaration {
    bool immovable{};
    bool serializable{};
    std::set<std::string> comparisons;
    bool hashable{};
    std::string vertexAlias;
    std::optional<TypeRef> vertexType;
    std::optional<TypeRef> indexedStorage;
    bool indexed{};
    bool vertexList{};
    bool mutableGraph{};
    bool incidence{};
    bool bidirectional{};
    bool hierarchy{};
    bool separateEdges{};
    std::optional<TypeRef> nameComponent;
    std::optional<SourceLocation> nameIndex;
    std::optional<TypeRef> indexedComponent;
    bool removeVertex{};
    std::optional<SourceLocation> parentGraph;
    std::optional<SourceLocation> addressableGraph;
    std::vector<GraphComponent> components;
    std::vector<GraphComponent> alternatives;
    std::vector<Field> fields;
    std::vector<Method> methods;
};

struct Declaration {
    std::string name;
    SourceLocation location;
    std::variant<Interface, Record, Enum, ExternalDeclaration, TagDeclaration, GraphDeclaration, AliasDeclaration> data;
    std::string externalScope;
    std::string cppHeader;
    bool cppOnly{};
    std::string documentation;
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
