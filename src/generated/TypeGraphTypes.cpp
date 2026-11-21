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
    }
    {
        using CheckedType = bool;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: bool");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: bool");
        static_assert(sizeof(CheckedType) == 1ULL, "fixed layout size: bool");
        static_assert(alignof(CheckedType) == 1ULL, "fixed layout alignment: bool");
    }
    {
        using CheckedType = ::cc::codegen::ModuleId;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::cc::codegen::ModuleId");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::cc::codegen::ModuleId");
        static_assert(sizeof(CheckedType) == 4ULL, "fixed layout size: ::cc::codegen::ModuleId");
        static_assert(alignof(CheckedType) == 4ULL, "fixed layout alignment: ::cc::codegen::ModuleId");
    }
    {
        using CheckedType = ::cc::codegen::ContainerKind;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::cc::codegen::ContainerKind");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::cc::codegen::ContainerKind");
        static_assert(sizeof(CheckedType) == 4ULL, "fixed layout size: ::cc::codegen::ContainerKind");
        static_assert(alignof(CheckedType) == 4ULL, "fixed layout alignment: ::cc::codegen::ContainerKind");
    }
    {
        using CheckedType = ::uint32_t;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::uint32_t");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::uint32_t");
        static_assert(sizeof(CheckedType) == 4ULL, "fixed layout size: ::uint32_t");
        static_assert(alignof(CheckedType) == 4ULL, "fixed layout alignment: ::uint32_t");
    }
    {
        using CheckedType = ::cc::codegen::MapKeyKind;
        static_assert(std::is_trivially_copyable_v<CheckedType>, "fixed layout trivially copyable: ::cc::codegen::MapKeyKind");
        static_assert(std::is_standard_layout_v<CheckedType>, "fixed layout standard layout: ::cc::codegen::MapKeyKind");
        static_assert(sizeof(CheckedType) == 4ULL, "fixed layout size: ::cc::codegen::MapKeyKind");
        static_assert(alignof(CheckedType) == 4ULL, "fixed layout alignment: ::cc::codegen::MapKeyKind");
    }
    return true;
}
static_assert(validateFixedLayout_CocosTypeGraph());
} // namespace
