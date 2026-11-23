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
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include "TypeGraphFwd.h"
#include "generated/SourceLocation.h"
#include "model/GraphSupport.h"

namespace cc {

namespace codegen {

using TemplateArgument = std::variant<TypeId, std::string>;

using TypeData = std::variant<NamespaceType, TagType, BuiltinType, ExternalTypeNode, EnumType, RecordType, InterfaceType, GraphType, AliasType, PointerType, ReferenceType, QualifiedType, FunctionType, TemplateParameterType, TemplateInstanceType>;

struct TypeId {
    uint32_t value{0xFFFFFFFF};
};

inline bool operator==(const TypeId& lhs, const TypeId& rhs) noexcept {
    return std::forward_as_tuple(lhs.value) ==
           std::forward_as_tuple(rhs.value);
}

inline bool operator!=(const TypeId& lhs, const TypeId& rhs) noexcept {
    return !(lhs == rhs);
}

struct ModuleId {
    uint32_t value{0xFFFFFFFF};
};

inline bool operator==(const ModuleId& lhs, const ModuleId& rhs) noexcept {
    return std::forward_as_tuple(lhs.value) ==
           std::forward_as_tuple(rhs.value);
}

inline bool operator!=(const ModuleId& lhs, const ModuleId& rhs) noexcept {
    return !(lhs == rhs);
}

struct TypeUse {
    TypeId type;
    SourceLocation location;
};

struct ParameterInfo {
    TypeUse type;
    std::string name;
    bool byConstReference{false};
    std::string defaultValue;
};

enum class AccessorKind : uint32_t {
    None,
    Getter,
    Setter,
};

enum class ResultPresence : uint32_t {
    Required,
    Nullable,
    Optional,
};

struct MethodInfo {
    TypeUse result;
    std::string name;
    std::vector<ParameterInfo> parameters;
    bool isConst{false};
    bool isNoexcept{false};
    bool cppOnly{false};
    std::optional<std::string> typescriptName;
    std::string cppBody;
    bool isStatic{false};
    bool isPureVirtual{false};
    bool isExplicitConversion{false};
    bool explicitCppOnly{false};
    std::string declaredProperty;
    AccessorKind accessor{AccessorKind::None};
    bool optionalMethod{false};
    ResultPresence resultPresence{ResultPresence::Required};
    bool cppPrivate{false};
    std::string documentation;
};

struct FieldInfo {
    TypeUse type;
    std::string name;
    std::string initializer;
    bool isReadonly{false};
    bool isNullable{false};
    bool transient{false};
    std::string typescriptInitializer;
    bool cppOnly{false};
    bool notElement{false};
    bool isMutable{false};
    bool cppPrivate{false};
    std::string documentation;
};

struct NamespaceType {
};

struct TagType {
};

struct BuiltinType {
};

enum class ContainerKind : uint32_t {
    None,
    Array,
    Map,
    Set,
};

enum class MapKeyKind : uint32_t {
    Native,
    String,
    Json,
};

struct ExternalTypeNode {
    bool isStruct{false};
    bool isString{false};
    bool isPmr{false};
    bool constructorConstRef{false};
    ContainerKind container{ContainerKind::None};
    bool hasVirtualDestructor{false};
    bool isAlias{false};
};

struct Enumerator {
    std::string name;
    std::string initializer;
    std::variant<std::int64_t, uint64_t> value;
    std::string documentation;
};

struct EnumType {
    TypeId underlying;
    std::vector<Enumerator> values;
    bool isFlags{false};
};

struct ConstructorInfo {
    std::vector<uint32_t> fields;
    bool defaults{false};
    bool cppOnly{false};
    bool custom{false};
    bool isNoexcept{false};
    bool cppPrivate{false};
    std::string documentation;
};

struct RecordType {
    bool moveOnly{false};
    bool immovable{false};
    bool serializable{false};
    bool compareEqual{false};
    bool compareLess{false};
    bool compareGreater{false};
    bool hashable{false};
    std::vector<ConstructorInfo> constructors;
    std::optional<ConstructorInfo> typescriptConstructor;
    std::vector<TypeId> bases;
    std::optional<TypeId> implements;
    bool isClass{false};
    bool isFinal{false};
    std::optional<bool> destructorNoexcept;
    bool destructorPrivate{false};
    std::string cppMembers;
    std::vector<FieldInfo> fields;
    std::vector<FieldInfo> staticFields;
    std::vector<MethodInfo> methods;
    std::string destructorDocumentation;
};

struct CallbackTypeInfo {
    TypeUse declaration;
    std::string name;
    std::vector<CallbackTypeInfo> arguments;
};

struct CallbackParameterInfo {
    std::string name;
    bool optional{false};
    CallbackTypeInfo type;
};

struct CallbackInfo {
    std::string name;
    bool optional{false};
    std::vector<CallbackParameterInfo> parameters;
    CallbackTypeInfo result;
    bool optionalResult{false};
    std::string documentation;
};

struct InterfaceType {
    std::vector<TypeId> bases;
    std::vector<MethodInfo> methods;
    std::vector<CallbackInfo> typescriptCallbacks;
};

struct ComponentInfo {
    TypeId tag;
    TypeId valueType;
    std::string name;
    std::string publicName;
};

struct GraphType {
    bool immovable{false};
    bool serializable{false};
    bool compareEqual{false};
    bool compareLess{false};
    bool compareGreater{false};
    bool hashable{false};
    TypeId vertexType;
    bool indexedVertices{false};
    std::optional<TypeId> indexedStorage;
    bool vertexList{false};
    bool edgeList{false};
    bool incidence{false};
    bool bidirectional{false};
    bool mutableGraph{false};
    bool hierarchy{false};
    bool separateEdges{false};
    std::optional<uint32_t> nameComponentIndex;
    bool nameIndex{false};
    std::optional<uint32_t> keyComponentIndex;
    bool removeVertex{false};
    bool parentGraph{false};
    bool addressableGraph{false};
    std::vector<ComponentInfo> components;
    std::vector<ComponentInfo> alternatives;
    std::vector<FieldInfo> fields;
    std::vector<MethodInfo> methods;
};

struct AliasType {
    TypeId target;
};

struct PointerType {
    TypeId pointee;
};

inline bool operator==(const PointerType& lhs, const PointerType& rhs) noexcept {
    return std::forward_as_tuple(lhs.pointee) ==
           std::forward_as_tuple(rhs.pointee);
}

inline bool operator!=(const PointerType& lhs, const PointerType& rhs) noexcept {
    return !(lhs == rhs);
}

struct ReferenceType {
    TypeId referred;
    bool rvalue{false};
};

inline bool operator==(const ReferenceType& lhs, const ReferenceType& rhs) noexcept {
    return std::forward_as_tuple(lhs.referred, lhs.rvalue) ==
           std::forward_as_tuple(rhs.referred, rhs.rvalue);
}

inline bool operator!=(const ReferenceType& lhs, const ReferenceType& rhs) noexcept {
    return !(lhs == rhs);
}

struct QualifiedType {
    TypeId base;
    bool isConst{false};
    bool isVolatile{false};
};

inline bool operator==(const QualifiedType& lhs, const QualifiedType& rhs) noexcept {
    return std::forward_as_tuple(lhs.base, lhs.isConst, lhs.isVolatile) ==
           std::forward_as_tuple(rhs.base, rhs.isConst, rhs.isVolatile);
}

inline bool operator!=(const QualifiedType& lhs, const QualifiedType& rhs) noexcept {
    return !(lhs == rhs);
}

struct FunctionType {
    TypeId result;
    std::vector<TypeId> parameters;
    bool isNoexcept{false};
};

inline bool operator==(const FunctionType& lhs, const FunctionType& rhs) noexcept {
    return std::forward_as_tuple(lhs.result, lhs.parameters, lhs.isNoexcept) ==
           std::forward_as_tuple(rhs.result, rhs.parameters, rhs.isNoexcept);
}

inline bool operator!=(const FunctionType& lhs, const FunctionType& rhs) noexcept {
    return !(lhs == rhs);
}

struct TemplateParameterType {
    TypeId owner;
    uint32_t index{0};
};

struct TemplateInstanceType {
    TypeId declaration;
    std::vector<TemplateArgument> arguments;
};

inline bool operator==(const TemplateInstanceType& lhs, const TemplateInstanceType& rhs) noexcept {
    return std::forward_as_tuple(lhs.declaration, lhs.arguments) ==
           std::forward_as_tuple(rhs.declaration, rhs.arguments);
}

inline bool operator!=(const TemplateInstanceType& lhs, const TemplateInstanceType& rhs) noexcept {
    return !(lhs == rhs);
}

struct TypeNode {
    std::string name;
    std::string qualifiedName;
    ModuleId module;
    TypeData data;
    std::string cppHeader;
    std::optional<std::string> typescriptName;
    std::string typescriptModule;
    std::string typescriptArchiveModule;
    ContainerKind typescriptContainer{ContainerKind::None};
    MapKeyKind typescriptMapKey{MapKeyKind::Native};
    bool typescriptNumber{false};
    bool typescriptTypeOnly{false};
    bool cppOnly{false};
    std::vector<TypeId> templateParameters;
    bool variadicTemplate{false};
    std::string documentation;
};

struct CppBoolConstant {
    std::string name;
    bool value{false};
    SourceLocation location;
    std::string documentation;
};

struct TypeModule {
    std::string name;
    std::vector<std::string> namespaces;
    std::vector<TypeId> declarations;
    std::vector<ModuleId> imports;
    TypeId inputArchive;
    TypeId outputArchive;
    std::vector<CppBoolConstant> cppConstants;
};

struct Node_ {};

struct TypeGraph : public cc::codegen::graph::BidirectionalStorage<uint32_t> {
    using vertex_descriptor = uint32_t;
    void reserve(vertex_descriptor capacity) {
        cc::codegen::graph::BidirectionalStorage<uint32_t>::reserve(capacity);
        this->nodes.reserve(capacity);
    }

    cc::codegen::TypeNode& getNode(vertex_descriptor v) noexcept { return this->nodes[v]; }
    cc::codegen::TypeNode& get(const ::cc::codegen::Node_&, vertex_descriptor v) noexcept { return getNode(v); }

    cc::codegen::TypeNode const& getNode(vertex_descriptor v) const noexcept { return this->nodes[v]; }
    cc::codegen::TypeNode const& get(const ::cc::codegen::Node_&, vertex_descriptor v) const noexcept { return getNode(v); }

    TypeId appendNode(TypeNode node, TypeId parent);
    const TypeNode& node(TypeId id) const;
    TypeId findType(const std::string& qualifiedName) const;
    TypeId makePointer(TypeId pointee);
    TypeId makeReference(TypeId referred, bool rvalue);
    TypeId makeQualified(TypeId base, bool isConst, bool isVolatile);
    TypeId makeFunction(TypeId result, std::vector<TypeId> parameters, bool isNoexcept);
    TypeId makeTemplateInstance(TypeId declaration, std::vector<TemplateArgument> arguments);
    std::vector<cc::codegen::TypeNode> nodes;
    std::vector<TypeModule> modules;
    std::map<std::string, TypeId> nameIndex;
};

} // namespace codegen

} // namespace cc
