#pragma once

#include "generated/TypeGraphFwd.h"
#include "model/Binding.h"
#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace cc::codegen {

struct Parameter {
    TypeRef type;
    std::string name;
    bool byConstReference{};
    std::string defaultValue;
};

struct Method {
    TypeRef result;
    std::string name;
    std::vector<Parameter> parameters;
    bool isConst{};
    bool isNoexcept{};
    bool cppOnly{};
    std::string cppBody;
    bool isStatic{};
    bool isPureVirtual{};
    bool isExplicitConversion{};
    AccessorKind accessor{};
    bool optionalMethod{};
    ResultPresence resultPresence{};
};

enum class ArchiveRole { none, input, output };

struct Interface {
    std::vector<Method> methods;
    std::vector<TypeRef> bases;
    ArchiveRole archiveRole{};
};

struct Field {
    TypeRef type;
    std::string name;
    std::string initializer;
    bool isReadonly{};
    bool isNullable{};
    bool transient{};
    bool cppOnly{};
    bool notElement{};
    bool isMutable{};
};

struct Constructor {
    SourceLocation location;
    std::vector<std::string> fields;
    bool defaults{};
    bool cppOnly{};
    bool custom{};
    bool isNoexcept{};
};

struct Record {
    bool isClass{};
    bool isFinal{};
    std::vector<TypeRef> bases;
    std::optional<TypeRef> implements;
    std::optional<bool> destructorNoexcept;
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

struct ExternalDeclaration {
    bool isStruct{};
    bool isString{};
    bool isPmr{};
    bool constructorConstRef{};
    ContainerKind container{};
    std::vector<std::string> templateParameters;
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
