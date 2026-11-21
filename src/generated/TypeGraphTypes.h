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
#include "generated/TypeGraphCore.h"
#include <map>
#include "model/GraphSupport.h"
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include "TypeGraphFwd.h"

namespace cc {

namespace codegen {

using TemplateArgument = std::variant<TypeId, std::string>;

using TypeData = std::variant<NamespaceType, TagType, BuiltinType, ExternalTypeNode, EnumType, RecordType, InterfaceType, GraphType, AliasType, PointerType, ReferenceType, QualifiedType, FunctionType, TemplateParameterType, TemplateInstanceType>;

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

struct TypeGraph : public cc::codegen::graph::BidirectionalStorage<uint32_t> {
    using Vertex = uint32_t;
    void reserve(Vertex capacity) {
        cc::codegen::graph::BidirectionalStorage<uint32_t>::reserve(capacity);
        this->nodes.reserve(capacity);
    }

    cc::codegen::TypeNode& getNode(Vertex v) noexcept { return this->nodes[v]; }
    cc::codegen::TypeNode& get(const ::cc::codegen::Node_&, Vertex v) noexcept { return getNode(v); }

    cc::codegen::TypeNode const& getNode(Vertex v) const noexcept { return this->nodes[v]; }
    cc::codegen::TypeNode const& get(const ::cc::codegen::Node_&, Vertex v) const noexcept { return getNode(v); }

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
