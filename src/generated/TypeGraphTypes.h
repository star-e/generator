#pragma once

// clang-format off
#include "TypeGraphFwd.h" // IWYU pragma: export
#include "TypeGraphDependencies.h" // IWYU pragma: export
// clang-format on

namespace cc {

namespace codegen {

struct Enumerator {
    std::string name;
    std::string initializer;
    std::variant<int64_t, uint64_t> value;
    std::string documentation;
};

struct EnumType {
    TypeId underlying;
    std::vector<Enumerator> values;
    bool isFlags = false;
};

using TemplateArgument = std::variant<TypeId, std::string>;

struct TemplateInstanceType {
    bool operator==(const TemplateInstanceType& rhs) const noexcept {
        return std::tie(declaration, arguments) == std::tie(rhs.declaration, rhs.arguments);
    }

    TypeId declaration;
    std::vector<TemplateArgument> arguments;
};

using TypeData = std::variant<NamespaceType, TagType, BuiltinType, ExternalTypeNode, EnumType, RecordType, InterfaceType, GraphType, AliasType, PointerType, ReferenceType, QualifiedType, FunctionType, TemplateParameterType, TemplateInstanceType>;

struct TypeNode {
    std::string name;
    std::string qualifiedName;
    ModuleId module;
    TypeData data;
    std::string cppHeader;
    std::optional<std::string> typescriptName;
    std::string typescriptModule;
    std::string typescriptArchiveModule;
    ContainerKind typescriptContainer = ContainerKind::None;
    MapKeyKind typescriptMapKey = MapKeyKind::Native;
    bool typescriptNumber = false;
    bool typescriptTypeOnly = false;
    bool cppOnly = false;
    std::vector<TypeId> templateParameters;
    std::string documentation;
};

struct TypeGraph {
    // Graph
    using Vertex = uint32_t;

    // VertexListGraph
    using VertexIterator = cc::codegen::graph::IntegerIterator<Vertex>;

    // Graph
    struct VertexType {
        std::vector<Vertex> mOutEdges;
        std::vector<Vertex> mInEdges;
    };

    // IncidenceGraph
    using Edge = cc::codegen::graph::EdgeDescriptor<Vertex>;
    using OutEdgeIterator = cc::codegen::graph::OutEdgeIterator<Edge, decltype(VertexType::mOutEdges)::const_iterator>;

    // BidirectionalGraph
    using InEdgeIterator = cc::codegen::graph::InEdgeIterator<Edge, decltype(VertexType::mInEdges)::const_iterator>;

    // Graph
    static constexpr Vertex null_vertex() noexcept { return (std::numeric_limits<Vertex>::max)(); }
    void reserve(uint32_t vertexCapacity);

    // IncidenceGraph
    std::pair<Edge, bool> edge(Vertex u, Vertex v) const noexcept;
    Vertex source(Edge edge) const noexcept;
    Vertex target(Edge edge) const noexcept;
    cc::codegen::graph::IteratorPair<OutEdgeIterator> out_edges(Vertex v) const noexcept;
    uint32_t out_degree(Vertex v) const noexcept;

    // BidirectionalGraph
    cc::codegen::graph::IteratorPair<InEdgeIterator> in_edges(Vertex v) const noexcept;
    uint32_t in_degree(Vertex v) const noexcept;
    uint32_t degree(Vertex v) const noexcept;

    // VertexListGraph
    cc::codegen::graph::IteratorPair<VertexIterator> vertices() const noexcept;
    uint32_t num_vertices() const noexcept;

    // ComponentGraph
    TypeNode const& get(Node_, Vertex v) const noexcept;
    TypeNode& get(Node_, Vertex v) noexcept;
    TypeNode const& getNode(Vertex v) const noexcept;
    TypeNode& getNode(Vertex v) noexcept;

    TypeId appendNode(TypeNode node, TypeId parent);
    const TypeNode& node(TypeId id) const;
    TypeId findType(const std::string& qualifiedName) const;
    TypeId makePointer(TypeId pointee);
    TypeId makeReference(TypeId referred, bool rvalue);
    TypeId makeQualified(TypeId base, bool isConst, bool isVolatile);
    TypeId makeFunction(TypeId result, std::vector<TypeId> parameters, bool isNoexcept);
    TypeId makeTemplateInstance(TypeId declaration, std::vector<TemplateArgument> arguments);

    std::vector<VertexType> mVertices;
    std::vector<TypeNode> nodes;

    std::vector<TypeModule> modules;
    std::map<std::string, TypeId> nameIndex;
};

} // namespace codegen

} // namespace cc

namespace cc {

namespace codegen {

// IncidenceGraph
inline auto TypeGraph::source(Edge edge) const noexcept -> Vertex {
    return edge.source;
}

inline auto TypeGraph::target(Edge edge) const noexcept -> Vertex {
    return edge.target;
}

inline auto TypeGraph::out_edges(Vertex v) const noexcept -> cc::codegen::graph::IteratorPair<OutEdgeIterator> {
    const auto& edges = this->mVertices[v].mOutEdges;
    return { OutEdgeIterator(v, edges.begin()), OutEdgeIterator(v, edges.end()) };
}

inline auto TypeGraph::out_degree(Vertex v) const noexcept -> uint32_t {
    return static_cast<uint32_t>(this->mVertices[v].mOutEdges.size());
}

// BidirectionalGraph
inline auto TypeGraph::in_edges(Vertex v) const noexcept -> cc::codegen::graph::IteratorPair<InEdgeIterator> {
    const auto& edges = this->mVertices[v].mInEdges;
    return { InEdgeIterator(v, edges.begin()), InEdgeIterator(v, edges.end()) };
}

inline auto TypeGraph::in_degree(Vertex v) const noexcept -> uint32_t {
    return static_cast<uint32_t>(this->mVertices[v].mInEdges.size());
}

inline auto TypeGraph::degree(Vertex v) const noexcept -> uint32_t {
    return this->in_degree(v) + this->out_degree(v);
}

// VertexListGraph
inline auto TypeGraph::vertices() const noexcept -> cc::codegen::graph::IteratorPair<VertexIterator> {
    return { VertexIterator(0), VertexIterator(static_cast<Vertex>(this->mVertices.size())) };
}

inline auto TypeGraph::num_vertices() const noexcept -> uint32_t {
    return static_cast<uint32_t>(this->mVertices.size());
}

// ComponentGraph
inline auto TypeGraph::get(Node_, Vertex v) const noexcept -> TypeNode const& {
    return this->nodes[v];
}

inline auto TypeGraph::get(Node_, Vertex v) noexcept -> TypeNode& {
    return this->nodes[v];
}

inline auto TypeGraph::getNode(Vertex v) const noexcept -> TypeNode const& {
    return this->nodes[v];
}

inline auto TypeGraph::getNode(Vertex v) noexcept -> TypeNode& {
    return this->nodes[v];
}

} // namespace codegen

} // namespace cc
