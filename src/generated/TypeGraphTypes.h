#pragma once

// clang-format off
#include "TypeGraphFwd.h" // IWYU pragma: export
#include "TypeGraphDependencies.h" // IWYU pragma: export
// clang-format on

namespace cc {

namespace codegen {

struct TypeId {
    bool operator==(const TypeId& rhs) const noexcept {
        return std::tie(value) == std::tie(rhs.value);
    }

    uint32_t value = ~0U;
};

struct ModuleId {
    bool operator==(const ModuleId& rhs) const noexcept {
        return std::tie(value) == std::tie(rhs.value);
    }

    uint32_t value = ~0U;
};

struct TypeUse {
    TypeId type;
    SourceLocation location;
};

struct ParameterInfo {
    TypeUse type;
    std::string name;
    bool byConstReference = false;
};

struct MethodInfo {
    TypeUse result;
    std::string name;
    std::vector<ParameterInfo> parameters;
    bool isConst = false;
    bool isNoexcept = false;
    bool cppOnly = false;
    std::optional<std::string> typescriptName;
    std::string cppExpression;
    bool isStatic = false;
};

struct FieldInfo {
    TypeUse type;
    std::string name;
    std::string initializer;
    bool isReadonly = false;
    bool isNullable = false;
    bool transient = false;
    std::string typescriptInitializer;
    bool cppOnly = false;
    bool notElement = false;
};

struct Enumerator {
    std::string name;
    std::string initializer;
    std::variant<int64_t, uint64_t> value;
    std::optional<std::string> deprecationMessage;
};

struct NamespaceType {};
struct TagType {};
struct BuiltinType {};

enum class ContainerKind : uint32_t {
    None,
    Array,
    Map,
    Set,
};

struct ExternalTypeNode {
    bool isStruct = false;
    bool isString = false;
    bool isPmr = false;
    bool constructorConstRef = false;
    ContainerKind container = ContainerKind::None;
};

struct EnumType {
    TypeId underlying;
    std::vector<Enumerator> values;
    bool isFlags = false;
};

struct RecordType {
    bool moveOnly = false;
    bool requireAllocator = false;
    bool serializable = false;
    bool compareEqual = false;
    bool compareLess = false;
    bool compareGreater = false;
    bool hashable = false;
    std::vector<std::vector<uint32_t>> constructors;
    std::vector<TypeId> bases;
    std::vector<FieldInfo> fields;
    std::vector<MethodInfo> methods;
};

struct InterfaceType {
    std::vector<TypeId> bases;
    std::vector<MethodInfo> methods;
};

struct ComponentInfo {
    TypeId tag;
    TypeId valueType;
    std::string name;
};

struct GraphType {
    bool immovable = false;
    bool serializable = false;
    bool compareEqual = false;
    bool compareLess = false;
    bool compareGreater = false;
    bool hashable = false;
    TypeId vertexType;
    bool indexedVertices = false;
    bool vertexList = false;
    bool incidence = false;
    bool bidirectional = false;
    bool mutableGraph = false;
    bool hierarchy = false;
    std::optional<uint32_t> nameComponentIndex;
    bool nameIndex = false;
    bool parentGraph = false;
    bool addressableGraph = false;
    std::vector<ComponentInfo> components;
    std::vector<ComponentInfo> alternatives;
    std::vector<FieldInfo> fields;
    std::vector<MethodInfo> methods;
};

struct AliasType {
    TypeId target;
};

struct PointerType {
    bool operator==(const PointerType& rhs) const noexcept {
        return std::tie(pointee) == std::tie(rhs.pointee);
    }

    TypeId pointee;
};

struct ReferenceType {
    bool operator==(const ReferenceType& rhs) const noexcept {
        return std::tie(referred, rvalue) == std::tie(rhs.referred, rhs.rvalue);
    }

    TypeId referred;
    bool rvalue = false;
};

struct QualifiedType {
    bool operator==(const QualifiedType& rhs) const noexcept {
        return std::tie(base, isConst, isVolatile) == std::tie(rhs.base, rhs.isConst, rhs.isVolatile);
    }

    TypeId base;
    bool isConst = false;
    bool isVolatile = false;
};

struct FunctionType {
    bool operator==(const FunctionType& rhs) const noexcept {
        return std::tie(result, parameters, isNoexcept) == std::tie(rhs.result, rhs.parameters, rhs.isNoexcept);
    }

    TypeId result;
    std::vector<TypeId> parameters;
    bool isNoexcept = false;
};

struct TemplateParameterType {
    TypeId owner;
    uint32_t index = 0;
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
    bool typescriptJsonMapKey = false;
    bool typescriptNumber = false;
    std::vector<TypeId> templateParameters;
};

struct TypeModule {
    std::string name;
    std::vector<std::string> namespaces;
    std::vector<TypeId> declarations;
    std::vector<ModuleId> imports;
};

struct Node_ {};

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
