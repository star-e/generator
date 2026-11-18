#include "TypeGraphTypes.h"

namespace cc {

namespace codegen {

// Graph
void TypeGraph::reserve(uint32_t vertexCapacity) {
    this->mVertices.reserve(vertexCapacity);
    this->nodes.reserve(vertexCapacity);
}

// IncidenceGraph
auto TypeGraph::edge(Vertex u, Vertex v) const noexcept -> std::pair<Edge, bool> {
    for (auto candidate : this->out_edges(u)) {
        if (this->target(candidate) == v) {
            return { candidate, true };
        }
    }
    return { { null_vertex(), null_vertex() }, false };
}

} // namespace codegen

} // namespace cc

#include <cstddef>
#include <type_traits>

namespace {
constexpr bool validateFixedLayout_CocosTypeGraph() {
    {
        using CheckedType = ::cc::codegen::TypeId;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::cc::codegen::TypeId");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::cc::codegen::TypeId");
        static_assert(sizeof(CheckedType) == 4ULL, "fixed layout size: ::cc::codegen::TypeId");
        static_assert(alignof(CheckedType) == 4ULL, "fixed layout alignment: ::cc::codegen::TypeId");
        static_assert(offsetof(CheckedType, value) == 0ULL, "fixed layout offset value: ::cc::codegen::TypeId");
    }
    {
        using CheckedType = ::uint32_t;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::uint32_t");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::uint32_t");
        static_assert(sizeof(CheckedType) == 4ULL, "fixed layout size: ::uint32_t");
        static_assert(alignof(CheckedType) == 4ULL, "fixed layout alignment: ::uint32_t");
    }
    {
        using CheckedType = ::cc::codegen::ModuleId;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::cc::codegen::ModuleId");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::cc::codegen::ModuleId");
        static_assert(sizeof(CheckedType) == 4ULL, "fixed layout size: ::cc::codegen::ModuleId");
        static_assert(alignof(CheckedType) == 4ULL, "fixed layout alignment: ::cc::codegen::ModuleId");
        static_assert(offsetof(CheckedType, value) == 0ULL, "fixed layout offset value: ::cc::codegen::ModuleId");
    }
    {
        using CheckedType = bool;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: bool");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: bool");
        static_assert(sizeof(CheckedType) == 1ULL, "fixed layout size: bool");
        static_assert(alignof(CheckedType) == 1ULL, "fixed layout alignment: bool");
    }
    {
        using CheckedType = ::cc::codegen::NamespaceType;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::cc::codegen::NamespaceType");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::cc::codegen::NamespaceType");
        static_assert(sizeof(CheckedType) == 1ULL, "fixed layout size: ::cc::codegen::NamespaceType");
        static_assert(alignof(CheckedType) == 1ULL, "fixed layout alignment: ::cc::codegen::NamespaceType");
    }
    {
        using CheckedType = ::cc::codegen::TagType;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::cc::codegen::TagType");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::cc::codegen::TagType");
        static_assert(sizeof(CheckedType) == 1ULL, "fixed layout size: ::cc::codegen::TagType");
        static_assert(alignof(CheckedType) == 1ULL, "fixed layout alignment: ::cc::codegen::TagType");
    }
    {
        using CheckedType = ::cc::codegen::BuiltinType;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::cc::codegen::BuiltinType");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::cc::codegen::BuiltinType");
        static_assert(sizeof(CheckedType) == 1ULL, "fixed layout size: ::cc::codegen::BuiltinType");
        static_assert(alignof(CheckedType) == 1ULL, "fixed layout alignment: ::cc::codegen::BuiltinType");
    }
    {
        using CheckedType = ::cc::codegen::ExternalTypeNode;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::cc::codegen::ExternalTypeNode");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::cc::codegen::ExternalTypeNode");
        static_assert(sizeof(CheckedType) == 1ULL, "fixed layout size: ::cc::codegen::ExternalTypeNode");
        static_assert(alignof(CheckedType) == 1ULL, "fixed layout alignment: ::cc::codegen::ExternalTypeNode");
        static_assert(offsetof(CheckedType, isStruct) == 0ULL, "fixed layout offset isStruct: ::cc::codegen::ExternalTypeNode");
    }
    {
        using CheckedType = ::cc::codegen::AliasType;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::cc::codegen::AliasType");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::cc::codegen::AliasType");
        static_assert(sizeof(CheckedType) == 4ULL, "fixed layout size: ::cc::codegen::AliasType");
        static_assert(alignof(CheckedType) == 4ULL, "fixed layout alignment: ::cc::codegen::AliasType");
        static_assert(offsetof(CheckedType, target) == 0ULL, "fixed layout offset target: ::cc::codegen::AliasType");
    }
    {
        using CheckedType = ::cc::codegen::PointerType;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::cc::codegen::PointerType");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::cc::codegen::PointerType");
        static_assert(sizeof(CheckedType) == 4ULL, "fixed layout size: ::cc::codegen::PointerType");
        static_assert(alignof(CheckedType) == 4ULL, "fixed layout alignment: ::cc::codegen::PointerType");
        static_assert(offsetof(CheckedType, pointee) == 0ULL, "fixed layout offset pointee: ::cc::codegen::PointerType");
    }
    {
        using CheckedType = ::cc::codegen::ReferenceType;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::cc::codegen::ReferenceType");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::cc::codegen::ReferenceType");
        static_assert(sizeof(CheckedType) == 8ULL, "fixed layout size: ::cc::codegen::ReferenceType");
        static_assert(alignof(CheckedType) == 4ULL, "fixed layout alignment: ::cc::codegen::ReferenceType");
        static_assert(offsetof(CheckedType, referred) == 0ULL, "fixed layout offset referred: ::cc::codegen::ReferenceType");
        static_assert(offsetof(CheckedType, rvalue) == 4ULL, "fixed layout offset rvalue: ::cc::codegen::ReferenceType");
    }
    {
        using CheckedType = ::cc::codegen::QualifiedType;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::cc::codegen::QualifiedType");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::cc::codegen::QualifiedType");
        static_assert(sizeof(CheckedType) == 8ULL, "fixed layout size: ::cc::codegen::QualifiedType");
        static_assert(alignof(CheckedType) == 4ULL, "fixed layout alignment: ::cc::codegen::QualifiedType");
        static_assert(offsetof(CheckedType, base) == 0ULL, "fixed layout offset base: ::cc::codegen::QualifiedType");
        static_assert(offsetof(CheckedType, isConst) == 4ULL, "fixed layout offset isConst: ::cc::codegen::QualifiedType");
        static_assert(offsetof(CheckedType, isVolatile) == 5ULL, "fixed layout offset isVolatile: ::cc::codegen::QualifiedType");
    }
    {
        using CheckedType = ::cc::codegen::TemplateParameterType;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::cc::codegen::TemplateParameterType");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::cc::codegen::TemplateParameterType");
        static_assert(sizeof(CheckedType) == 8ULL, "fixed layout size: ::cc::codegen::TemplateParameterType");
        static_assert(alignof(CheckedType) == 4ULL, "fixed layout alignment: ::cc::codegen::TemplateParameterType");
        static_assert(offsetof(CheckedType, owner) == 0ULL, "fixed layout offset owner: ::cc::codegen::TemplateParameterType");
        static_assert(offsetof(CheckedType, index) == 4ULL, "fixed layout offset index: ::cc::codegen::TemplateParameterType");
    }
    return true;
}
static_assert(validateFixedLayout_CocosTypeGraph());
} // namespace
