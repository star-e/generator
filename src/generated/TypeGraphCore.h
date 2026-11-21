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
#pragma once
#include <cstdint>
#include "generated/SourceLocation.h"
#include <optional>
#include <string>
#include <tuple>
#include <vector>
#include "TypeGraphCoreFwd.h"

namespace cc {

namespace codegen {

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

struct TypeModule {
    std::string name;
    std::vector<std::string> namespaces;
    std::vector<TypeId> declarations;
    std::vector<ModuleId> imports;
    TypeId inputArchive;
    TypeId outputArchive;
};

struct Node_ {};

} // namespace codegen

} // namespace cc
