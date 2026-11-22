/*
 Copyright (c) 2021-2024 Xiamen Yaji Software Co., Ltd.

 https://www.cocos.com

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

/**
 * ========================= !DO NOT CHANGE THE FOLLOWING SECTION MANUALLY! =========================
 * The following section is auto-generated.
 * ========================= !DO NOT CHANGE THE FOLLOWING SECTION MANUALLY! =========================
 */
#pragma once // NOLINT(portability-avoid-pragma-once)
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include "SyntaxFwd.h"
#include "generated/SourceLocation.h"
#include "generated/TypeGraphTypes.h"

namespace cc {

namespace codegen {

using MethodBindings = std::map<std::string, InterfaceMethodBinding>;

using InterfaceBindings = std::map<std::pair<std::string, std::string>, InterfaceMethodBinding>;

using TypeScriptClassMethods = std::vector<std::variant<TypeScriptClassMethod, std::string>>;

struct TypeRef {
    SourceLocation location;
    std::string name;
    bool pointer{false};
    std::vector<TypeRef> arguments;
    bool constPointee{false};
    bool reference{false};
    bool hasTemplateArguments{false};
};

struct Parameter {
    TypeRef type;
    std::string name;
    bool byConstReference{false};
    std::string defaultValue;
};

struct Method {
    TypeRef result;
    std::string name;
    std::vector<Parameter> parameters;
    bool isConst{false};
    bool isNoexcept{false};
    bool cppOnly{false};
    std::string cppBody;
    bool isStatic{false};
    bool isPureVirtual{false};
    bool isExplicitConversion{false};
    bool explicitCppOnly{false};
    AccessorKind accessor{AccessorKind::None};
    bool optionalMethod{false};
    ResultPresence resultPresence{ResultPresence::Required};
    bool cppPrivate{false};
    std::string documentation;
};

struct Field {
    TypeRef type;
    std::string name;
    std::string initializer;
    bool isReadonly{false};
    bool isNullable{false};
    bool transient{false};
    bool cppOnly{false};
    bool notElement{false};
    bool isMutable{false};
    bool cppPrivate{false};
    std::string documentation;
};

struct Constructor {
    SourceLocation location;
    std::vector<std::string> fields;
    bool defaults{false};
    bool cppOnly{false};
    bool custom{false};
    bool isNoexcept{false};
    bool cppPrivate{false};
    std::string documentation;
};

enum class ArchiveRole {
    none,
    input,
    output,
};

struct Interface {
    std::vector<Method> methods;
    std::vector<TypeRef> bases;
    ArchiveRole archiveRole{ArchiveRole::none};
};

struct Record {
    bool isClass{false};
    bool isFinal{false};
    std::vector<TypeRef> bases;
    std::optional<TypeRef> implements;
    std::optional<bool> destructorNoexcept;
    bool destructorPrivate{false};
    std::string cppMembers;
    bool moveOnly{false};
    bool immovable{false};
    bool serializable{false};
    std::set<std::string> comparisons;
    bool hashable{false};
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
    bool isFlags{false};
};

struct ExternalDeclaration {
    bool isStruct{false};
    bool isString{false};
    bool isPmr{false};
    bool constructorConstRef{false};
    ContainerKind container{ContainerKind::None};
    std::vector<std::string> templateParameters;
    bool variadicTemplate{false};
};

struct TagDeclaration {
};

struct AliasDeclaration {
    TypeRef target;
};

struct GraphComponent {
    TypeRef tag;
    TypeRef value;
    std::string member;
    std::string kindName;
};

struct GraphDeclaration {
    bool immovable{false};
    bool serializable{false};
    std::set<std::string> comparisons;
    bool hashable{false};
    std::string vertexAlias;
    std::optional<TypeRef> vertexType;
    std::optional<TypeRef> indexedStorage;
    bool indexed{false};
    bool vertexList{false};
    bool mutableGraph{false};
    bool incidence{false};
    bool bidirectional{false};
    bool hierarchy{false};
    bool separateEdges{false};
    std::optional<TypeRef> nameComponent;
    std::optional<SourceLocation> nameIndex;
    std::optional<TypeRef> indexedComponent;
    bool removeVertex{false};
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
    std::string cppForwardHeader;
    bool cppOnly{false};
    std::string documentation;
};

struct ModuleImport {
    SourceLocation location;
    std::string name;
};

struct PoolBinding {
    std::string member;
    std::set<std::string> keepOnReset;
    bool assignFields{false};
};

struct PoolDependency {
    std::string member;
    std::string module;
};

enum class BindingTarget {
    type,
    interfaceType,
    classType,
    enumeration,
    constEnum,
    numberField,
};

struct TypeScriptImport {
    std::string name;
    std::filesystem::path from;
};

struct TypeScriptNamedImport {
    std::set<std::string> names;
    std::string from;
    bool typeOnly{false};
};

struct InterfaceMethodBinding {
    std::string target;
    AccessorKind accessor{AccessorKind::None};
    bool nullable{false};
    std::vector<std::string> optional;
};

struct CallbackParameter {
    std::string name;
    bool optional{false};
    TypeRef type;
};

struct Callback {
    std::string name;
    bool optional{false};
    std::vector<CallbackParameter> parameters;
    TypeRef result;
    bool optionalResult{false};
    std::string documentation;
};

struct TypeScriptMethodParameter {
    std::string name;
    std::string type;
    bool optional{false};
};

struct TypeScriptClassMethod {
    std::string name;
    std::vector<TypeScriptMethodParameter> parameters;
    std::string result;
    std::string body;
    std::string documentation;
};

struct TypeScriptBinding {
    TypeRef source;
    BindingTarget kind{BindingTarget::type};
    std::string target;
    std::filesystem::path from;
    std::filesystem::path archiveFrom;
    bool externalTarget{false};
    bool externalInterface{false};
    MethodBindings methods;
    std::vector<Callback> callbacks;
    std::map<std::string, std::string> renamedFrom;
    std::string numberField;
    std::map<std::string, std::string> fieldDefaults;
    std::optional<TypeScriptImport> base;
    TypeScriptClassMethods classMethods;
    std::optional<PoolBinding> pool;
    std::optional<std::vector<std::string>> constructor;
    bool requiredConstructor{false};
    MapKeyKind mapKey{MapKeyKind::Native};
    bool names{false};
    bool registerEnum{false};
    bool helpers{false};
};

struct SwigMember {
    SourceLocation location;
    std::string owner;
    std::string name;
};

struct SwigBindings {
    std::vector<std::filesystem::path> includes;
    std::vector<std::pair<SwigMember, std::string>> macros;
    std::vector<SwigMember> releaseInGC;
};

struct ModuleBindings {
    std::vector<TypeScriptBinding> typescript;
    std::vector<TypeScriptNamedImport> typescriptImports;
    std::filesystem::path typescriptFrom;
    std::map<std::string, SourceLocation> typescriptExports;
    std::filesystem::path poolModule;
    std::vector<PoolDependency> poolDependencies;
    std::optional<SourceLocation> jsb;
    std::map<std::string, SourceLocation> jsbRecords;
    std::set<std::filesystem::path> jsbIncludes;
    std::optional<SwigBindings> swig;
};

struct Module {
    std::string name;
    std::vector<std::string> namespaces;
    std::vector<Declaration> declarations;
    std::vector<ModuleImport> imports;
    ModuleBindings bindings;
    std::string cppHeader;
    bool external{false};
    bool pool{false};
    std::vector<CppBoolConstant> cppConstants;
};

} // namespace codegen

} // namespace cc
