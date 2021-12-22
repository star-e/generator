/*
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2021 Xiamen Yaji Software Co., Ltd.

http://www.cocos.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated engine source code (the "Software"), a limited,
worldwide, royalty-free, non-assignable, revocable and non-exclusive license
to use Cocos Creator solely to develop games on your target platforms. You shall
not use Cocos Creator software for developing other software or tools that's
used for developing games. You are not granted to publish, distribute,
sublicense, and/or sell copies of Cocos Creator.

The software or tools in this License Agreement are licensed, not sold.
Xiamen Yaji Software Co., Ltd. reserves all rights not expressly granted to you.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#pragma once
#include <Cocos/AST/SyntaxFwd.h>
#include <Cocos/GraphTypes.h>

namespace Cocos {

namespace Meta {

enum GenerationFlags : uint64_t {
    NO_FLAGS = 0,
    CUSTOM_CNTR = 1 << 0,
    CUSTOM_DTOR = 1 << 1,
    MOVE = 1 << 2,
    NO_MOVE = 1 << 3,
    EQUAL = 1 << 4,
    NO_EQUAL = 1 << 5,
    LESS = 1 << 6,
    NO_LESS = 1 << 7,
    SPACESHIP = 1 << 8,
    NO_SPACESHIP = 1 << 9,
    REFLECTION = 1 << 10,
    NO_SERIALIZATION = 1 << 11,
    HASH_COMBINE = 1 << 12,
    NO_HASH_COMBINE = 1 << 13,
    ENUM_OPERATOR = 1 << 14,
    PMR_DEFAULT = 1 << 15,
};

constexpr GenerationFlags operator|(const GenerationFlags lhs, const GenerationFlags rhs) noexcept {
    return (GenerationFlags)((uint64_t)lhs | (uint64_t)rhs);
}

constexpr GenerationFlags operator&(const GenerationFlags lhs, const GenerationFlags rhs) noexcept {
    return (GenerationFlags)((uint64_t)lhs & (uint64_t)rhs);
}

constexpr GenerationFlags& operator|=(GenerationFlags& lhs, const GenerationFlags rhs) noexcept {
    return lhs = lhs | rhs;
}

constexpr GenerationFlags& operator&=(GenerationFlags& lhs, const GenerationFlags rhs) noexcept {
    return lhs = lhs & rhs;
}

constexpr bool operator!(GenerationFlags e) noexcept {
    return e == static_cast<GenerationFlags>(0);
}

constexpr bool any(GenerationFlags e) noexcept {
    return !!e;
}

struct Requires_ {
} static constexpr Requires;
struct Composites_ {
} static constexpr Composites;
struct Reuses_ {
} static constexpr Reuses;
struct Inherits_ {
} static constexpr Inherits;
struct Sums_ {
} static constexpr Sums;
struct References_ {
} static constexpr References;
struct Optional {};
struct Container {};
struct Map {};

struct Instance {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mParameters.get_allocator().resource());
    }

    Instance(const allocator_type& alloc) noexcept;
    Instance(Instance&& rhs, const allocator_type& alloc);
    Instance(Instance const& rhs, const allocator_type& alloc);

    Instance(Instance&& rhs) = default;
    Instance(Instance const& rhs) = delete;
    Instance& operator=(Instance&& rhs) = default;
    Instance& operator=(Instance const& rhs) = default;
    ~Instance() noexcept;

    std::pmr::vector<std::pmr::string> mParameters;
};

struct Namespace {};
struct Declare {};

struct Alias {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mName.get_allocator().resource());
    }

    Alias(const allocator_type& alloc) noexcept;
    Alias(Alias&& rhs, const allocator_type& alloc);
    Alias(Alias const& rhs, const allocator_type& alloc);

    Alias(Alias&& rhs) = default;
    Alias(Alias const& rhs) = delete;
    Alias& operator=(Alias&& rhs) = default;
    Alias& operator=(Alias const& rhs) = default;
    ~Alias() noexcept;

    std::pmr::string mName;
};

struct Concept {};

struct Traits {
    bool mImport = false;
    bool mPmr = false;
    bool mNameComparison = false;
    bool mDefined = false;
    bool mClass = false;
    bool mExport = false;
    GenerationFlags mFlags = {};
    uint32_t mOverAlignment = 0;
};

struct Value {};

struct EnumValue {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mName.get_allocator().resource());
    }

    EnumValue(const allocator_type& alloc) noexcept;
    EnumValue(EnumValue&& rhs, const allocator_type& alloc);
    EnumValue(EnumValue const& rhs, const allocator_type& alloc);

    EnumValue(EnumValue&& rhs) = default;
    EnumValue(EnumValue const& rhs) = delete;
    EnumValue& operator=(EnumValue&& rhs) = default;
    EnumValue& operator=(EnumValue const& rhs) = default;
    ~EnumValue() noexcept;

    std::pmr::string mName;
    std::pmr::string mValue;
    std::pmr::string mReflectionName;
};

struct Enum {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mUnderlyingType.get_allocator().resource());
    }

    Enum(const allocator_type& alloc) noexcept;
    Enum(Enum&& rhs, const allocator_type& alloc);
    Enum(Enum const& rhs, const allocator_type& alloc);

    Enum(Enum&& rhs) = default;
    Enum(Enum const& rhs) = delete;
    Enum& operator=(Enum&& rhs) = default;
    Enum& operator=(Enum const& rhs) = default;
    ~Enum() noexcept;

    bool mIsFlags = false;
    bool mEnumOperator = true;
    std::pmr::string mUnderlyingType;
    std::pmr::vector<EnumValue> mValues;
};

struct Tag {
    bool mEntity = false;
};

struct Member {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mTypePath.get_allocator().resource());
    }

    Member(const allocator_type& alloc) noexcept;
    Member(Member&& rhs, const allocator_type& alloc);
    Member(Member const& rhs, const allocator_type& alloc);

    Member(Member&& rhs) = default;
    Member(Member const& rhs) = delete;
    Member& operator=(Member&& rhs) = default;
    Member& operator=(Member const& rhs) = default;
    ~Member() noexcept;

    std::pmr::string mTypePath;
    std::pmr::string mMemberName;
    std::pmr::string mDefaultValue;
    bool mConst = false;
    bool mPointer = false;
    bool mPublic = true;
    GenerationFlags mFlags = {};
    std::pmr::string mTypescriptType;
    std::pmr::string mTypescriptDefaultValue;
    bool mTypescriptArray = false;
};

struct Constructor {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mIndices.get_allocator().resource());
    }

    Constructor(const allocator_type& alloc) noexcept;
    Constructor(Constructor&& rhs, const allocator_type& alloc);
    Constructor(Constructor const& rhs, const allocator_type& alloc);

    Constructor(Constructor&& rhs) = default;
    Constructor(Constructor const& rhs) = delete;
    Constructor& operator=(Constructor&& rhs) = default;
    Constructor& operator=(Constructor const& rhs) = default;
    ~Constructor() noexcept;

    std::pmr::vector<uint32_t> mIndices;
};

struct Struct {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mMembers.get_allocator().resource());
    }

    Struct(const allocator_type& alloc) noexcept;
    Struct(Struct&& rhs, const allocator_type& alloc);
    Struct(Struct const& rhs, const allocator_type& alloc);

    Struct(Struct&& rhs) = default;
    Struct(Struct const& rhs) = delete;
    Struct& operator=(Struct&& rhs) = default;
    Struct& operator=(Struct const& rhs) = default;
    ~Struct() noexcept;

    std::pmr::vector<Member> mMembers;
    std::pmr::vector<Constructor> mConstructors;
    std::pmr::vector<Member> mTypescriptMembers;
    std::pmr::vector<std::pmr::string> mTypescriptFunctions;
};

struct Variant {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mVariants.get_allocator().resource());
    }

    Variant(const allocator_type& alloc) noexcept;
    Variant(Variant&& rhs, const allocator_type& alloc);
    Variant(Variant const& rhs, const allocator_type& alloc);

    Variant(Variant&& rhs) = default;
    Variant(Variant const& rhs) = delete;
    Variant& operator=(Variant&& rhs) = default;
    Variant& operator=(Variant const& rhs) = default;
    ~Variant() noexcept;

    std::pmr::vector<std::pmr::string> mVariants;
    bool mUseIndex = true;
};

struct Namespace_ {};
struct Declare_ {};
struct Concept_ {};
struct Value_ {};
struct Enum_ {};
struct Tag_ {};
struct Struct_ {};
struct Variant_ {};
struct Graph_ {};
struct Optional_ {};
struct Container_ {};
struct Map_ {};
struct Instance_ {};
struct Addressable {};

struct PolymorphicPair {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mTag.get_allocator().resource());
    }

    PolymorphicPair(const allocator_type& alloc) noexcept;
    PolymorphicPair(PolymorphicPair&& rhs, const allocator_type& alloc);
    PolymorphicPair(PolymorphicPair const& rhs, const allocator_type& alloc);

    PolymorphicPair(PolymorphicPair&& rhs) = default;
    PolymorphicPair(PolymorphicPair const& rhs) = delete;
    PolymorphicPair& operator=(PolymorphicPair&& rhs) = default;
    PolymorphicPair& operator=(PolymorphicPair const& rhs) = default;
    ~PolymorphicPair() noexcept;

    std::pmr::string mTag;
    std::pmr::string mValue;
    std::pmr::string mMemberName;
};

struct Polymorphic {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mConcepts.get_allocator().resource());
    }

    Polymorphic(const allocator_type& alloc) noexcept;
    Polymorphic(Polymorphic&& rhs, const allocator_type& alloc);
    Polymorphic(Polymorphic const& rhs, const allocator_type& alloc);

    Polymorphic(Polymorphic&& rhs) = default;
    Polymorphic(Polymorphic const& rhs) = delete;
    Polymorphic& operator=(Polymorphic&& rhs) = default;
    Polymorphic& operator=(Polymorphic const& rhs) = default;
    ~Polymorphic() noexcept;

    std::pmr::vector<PolymorphicPair> mConcepts;
};

struct Component {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mName.get_allocator().resource());
    }

    Component(const allocator_type& alloc) noexcept;
    Component(Component&& rhs, const allocator_type& alloc);
    Component(Component const& rhs, const allocator_type& alloc);

    Component(Component&& rhs) = default;
    Component(Component const& rhs) = delete;
    Component& operator=(Component&& rhs) = default;
    Component& operator=(Component const& rhs) = default;
    ~Component() noexcept;

    bool isValid() const noexcept {
        return !mMemberName.empty();
    }

    std::pmr::string getTypescriptComponentType(const SyntaxGraph& g,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const noexcept;

    std::pmr::string mName;
    std::pmr::string mValuePath;
    std::pmr::string mMemberName;
};

struct Vector_ {};
struct List_ {};

using VertexListType = std::variant<Vector_, List_>;

inline bool operator<(const VertexListType& lhs, const VertexListType& rhs) noexcept {
    return lhs.index() < rhs.index();
}

inline bool operator==(const VertexListType& lhs, const VertexListType& rhs) noexcept {
    return lhs.index() == rhs.index();
}

inline bool operator!=(const VertexListType& lhs, const VertexListType& rhs) noexcept {
    return !(lhs == rhs);
}

struct Graph2 {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mMembers.get_allocator().resource());
    }

    Graph2(const allocator_type& alloc) noexcept;
    Graph2(Graph2&& rhs, const allocator_type& alloc);
    Graph2(Graph2 const& rhs, const allocator_type& alloc);

    Graph2(Graph2&& rhs) = default;
    Graph2(Graph2 const& rhs) = delete;
    Graph2& operator=(Graph2&& rhs) = default;
    Graph2& operator=(Graph2 const& rhs) = default;
    ~Graph2() noexcept;

    std::pmr::string getTypescriptVertexDescriptorType(std::string_view tsName,
        std::pmr::memory_resource* scratch) const;
    std::string_view getTypescriptEdgeDescriptorType() const;
    std::string_view getTypescriptReferenceDescriptorType() const;

    std::pmr::string getTypescriptVertexDereference(std::string_view v,
        std::pmr::memory_resource* scratch) const;

    std::string_view getTypescriptOutEdgeList(bool bAddressable) const;
    std::string_view getTypescriptInEdgeList(bool bAddressable) const;

    bool needEdgeList() const noexcept {
        return !mEdgeProperty.empty();
    }

    bool isVector() const noexcept {
        return holds_alternative<Vector_>(mVertexListType);
    }

    bool isAddressable() const noexcept {
        Expects(mReferenceGraph || !mAddressable);
        return !!mAddressable;
    }

    bool isPolymorphic() const noexcept {
        return !mPolymorphic.mConcepts.empty();
    }

    bool hasProperties() const noexcept {
        return mNamed || !mVertexProperty.empty() || !mComponents.empty();
    }

    bool isComponentInVertex() const noexcept {
        return !mComponents.empty() && !isVector();
    }

    bool isReference() const noexcept {
        Expects(mReferenceGraph || !mAddressable);
        return mReferenceGraph;
    }

    bool isAliasGraph() const noexcept {
        return mReferenceGraph && mAliasGraph;
    }

    bool isMutableReference() const noexcept {
        return mReferenceGraph && mMutableReference;
    }

    bool needReferenceEdges() const noexcept {
        return mReferenceGraph && !mAliasGraph;
    }

    std::string_view getTypescriptNullVertex() const;

    std::pmr::string getTypescriptVertexPropertyType(const SyntaxGraph& g,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const noexcept;

    std::pmr::vector<Member> mMembers;
    std::pmr::vector<Constructor> mConstructors;
    std::pmr::vector<Member> mTypescriptMembers;
    std::pmr::vector<std::pmr::string> mTypescriptFunctions;
    std::pmr::string mVertexProperty;
    std::pmr::string mEdgeProperty;
    std::pmr::vector<Component> mComponents;
    bool mNamed = false;
    bool mReferenceGraph = false;
    bool mAliasGraph = false;
    bool mMutableReference = true;
    std::optional<Addressable> mAddressable;
    Polymorphic mPolymorphic;
    VertexListType mVertexListType;
    std::pmr::string mVertexDescriptor;
    std::pmr::string mVertexSizeType;
    std::pmr::string mDifferenceType;
    std::pmr::string mEdgeSizeType;
    std::pmr::string mEdgeDifferenceType;
    std::pmr::string mDegreeSizeType;
};

struct Typescript {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mName.get_allocator().resource());
    }

    Typescript(const allocator_type& alloc) noexcept;
    Typescript(Typescript&& rhs, const allocator_type& alloc);
    Typescript(Typescript const& rhs, const allocator_type& alloc);

    Typescript(Typescript&& rhs) = default;
    Typescript(Typescript const& rhs) = delete;
    Typescript& operator=(Typescript&& rhs) = default;
    Typescript& operator=(Typescript const& rhs) = default;
    ~Typescript() noexcept;

    std::pmr::string mName;
    bool mArray = false;
};

struct SyntaxGraph {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept;

    inline std::pmr::memory_resource* resource() const noexcept {
        return get_allocator().resource();
    }

    SyntaxGraph(const allocator_type& alloc);
    SyntaxGraph(SyntaxGraph&& rhs, const allocator_type& alloc);
    SyntaxGraph(SyntaxGraph const& rhs, const allocator_type& alloc);

    SyntaxGraph(SyntaxGraph&& rhs);
    SyntaxGraph(SyntaxGraph const& rhs) = delete;
    SyntaxGraph& operator=(SyntaxGraph&& rhs);
    SyntaxGraph& operator=(SyntaxGraph const& rhs);
    ~SyntaxGraph() noexcept;

    // Graph
    using directed_category = boost::bidirectional_tag;
    using vertex_descriptor = uint32_t;
    using edge_descriptor = Graph::EdgeDescriptor<directed_category, vertex_descriptor>;
    using edge_parallel_category = boost::allow_parallel_edge_tag;
    struct traversal_category
        : virtual boost::incidence_graph_tag
        , virtual boost::bidirectional_graph_tag
        , virtual boost::adjacency_graph_tag
        , virtual boost::vertex_list_graph_tag
        , virtual boost::edge_list_graph_tag {};

    constexpr static vertex_descriptor null_vertex() noexcept {
        return std::numeric_limits<vertex_descriptor>::max();
    }

    // IncidenceGraph
    using out_edge_type = Graph::StoredEdge<vertex_descriptor>;
    using out_edge_iterator = Graph::OutEdgeIter<
        std::pmr::vector<out_edge_type>::iterator,
        vertex_descriptor, edge_descriptor, int32_t>;
    using degree_size_type = uint32_t;

    // BidirectionalGraph
    using in_edge_type = Graph::StoredEdge<vertex_descriptor>;
    using in_edge_iterator = Graph::InEdgeIter<
        std::pmr::vector<in_edge_type>::iterator,
        vertex_descriptor, edge_descriptor, int32_t>;

    // AdjacencyGraph
    using adjacency_iterator = boost::adjacency_iterator_generator<
        SyntaxGraph, vertex_descriptor, out_edge_iterator>::type;

    // VertexListGraph
    using vertex_iterator = boost::integer_range<vertex_descriptor>::iterator;
    using vertices_size_type = uint32_t;

    // VertexList help functions
    inline std::pmr::vector<out_edge_type>& out_edge_list(vertex_descriptor v) noexcept {
        return mVertices[v].mOutEdges;
    }
    inline const std::pmr::vector<out_edge_type>& out_edge_list(vertex_descriptor v) const noexcept {
        return mVertices[v].mOutEdges;
    }

    inline std::pmr::vector<in_edge_type>& in_edge_list(vertex_descriptor v) noexcept {
        return mVertices[v].mInEdges;
    }
    inline const std::pmr::vector<in_edge_type>& in_edge_list(vertex_descriptor v) const noexcept {
        return mVertices[v].mInEdges;
    }

    inline boost::integer_range<vertex_descriptor> vertex_set() const noexcept {
        return boost::integer_range<vertex_descriptor>(0, gsl::narrow_cast<vertices_size_type>(mVertices.size()));
    }

    inline vertex_descriptor current_id() const noexcept {
        return gsl::narrow_cast<vertex_descriptor>(mVertices.size());
    }

    [[nodiscard]] inline std::pmr::vector<boost::default_color_type> colors(std::pmr::memory_resource* mr) const {
        return std::pmr::vector<boost::default_color_type>{ mVertices.size(), mr };
    }

    // EdgeListGraph
    using edge_iterator = Graph::DirectedEdgeIterator<vertex_iterator, out_edge_iterator, SyntaxGraph>;
    using edges_size_type = uint32_t;

    // AddressableGraph (Separated)
    using ownership_descriptor = Graph::EdgeDescriptor<boost::bidirectional_tag, vertex_descriptor>;

    using children_edge_type = Graph::StoredEdge<vertex_descriptor>;
    using children_iterator = Graph::OutEdgeIter<
        std::pmr::vector<children_edge_type>::iterator,
        vertex_descriptor, ownership_descriptor, int32_t>;
    using children_size_type = uint32_t;

    using parent_edge_type = Graph::StoredEdge<vertex_descriptor>;
    using parent_iterator = Graph::InEdgeIter<
        std::pmr::vector<parent_edge_type>::iterator,
        vertex_descriptor, ownership_descriptor, int32_t>;

    using ownership_iterator = Graph::OwnershipIterator<vertex_iterator, children_iterator, SyntaxGraph>;
    using ownerships_size_type = edges_size_type;

    // AddressableGraph help functions
    inline std::pmr::vector<children_edge_type>& children_list(vertex_descriptor v) noexcept {
        return mObjects[v].mChildren;
    }
    inline const std::pmr::vector<children_edge_type>& children_list(vertex_descriptor v) const noexcept {
        return mObjects[v].mChildren;
    }

    inline std::pmr::vector<parent_edge_type>& parents_list(vertex_descriptor v) noexcept {
        return mObjects[v].mParents;
    }
    inline const std::pmr::vector<parent_edge_type>& parents_list(vertex_descriptor v) const noexcept {
        return mObjects[v].mParents;
    }

    // PolymorphicGraph
    using vertex_tag_type = std::variant<Namespace_, Declare_, Concept_, Value_, Enum_, Tag_, Struct_, Graph_, Optional_, Variant_, Container_, Map_, Instance_>;
    using vertex_value_type = std::variant<Namespace*, Declare*, Concept*, Value*, Enum*, Tag*, Struct*, Graph2*, Optional*, Variant*, Container*, Map*, Instance*>;
    using vertex_const_value_type = std::variant<const Namespace*, const Declare*, const Concept*, const Value*, const Enum*, const Tag*, const Struct*, const Graph2*, const Optional*, const Variant*, const Container*, const Map*, const Instance*>;
    using vertex_handle_type = std::variant<
        Graph::ValueHandle<Namespace_, Namespace>,
        Graph::ValueHandle<Declare_, Declare>,
        Graph::ValueHandle<Concept_, Concept>,
        Graph::ValueHandle<Value_, Value>,
        Graph::ValueHandle<Enum_, vertex_descriptor>,
        Graph::ValueHandle<Tag_, vertex_descriptor>,
        Graph::ValueHandle<Struct_, vertex_descriptor>,
        Graph::ValueHandle<Graph_, vertex_descriptor>,
        Graph::ValueHandle<Optional_, Optional>,
        Graph::ValueHandle<Variant_, vertex_descriptor>,
        Graph::ValueHandle<Container_, Container>,
        Graph::ValueHandle<Map_, Map>,
        Graph::ValueHandle<Instance_, vertex_descriptor>>;

    bool isNamespace(std::string_view typePath) const noexcept;
    bool isTag(vertex_descriptor vertID) const;

    std::pmr::string getTypePath(vertex_descriptor vertID, std::pmr::memory_resource* mr) const;

    vertex_descriptor lookupIdentifier(std::string_view currentScope, std::string_view dependentName,
        std::pmr::memory_resource* scratch) const;

    std::pmr::string getTypePath(std::string_view currentScope, std::string_view dependentName,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const;

    vertex_descriptor lookupType(std::string_view currentScope, std::string_view dependentName,
        std::pmr::memory_resource* scratch) const;

    std::pmr::string getNamespace(vertex_descriptor vertID, std::pmr::memory_resource* mr) const;

    std::pair<std::string_view, std::string_view> splitTypePath(std::string_view typePath) const;

    void instantiate(std::string_view currentScope, std::string_view dependentName,
        std::pmr::memory_resource* scratch);

    vertex_descriptor getTemplate(vertex_descriptor instanceID, std::pmr::memory_resource* scratch) const;

    bool isTypescriptData(std::string_view name) const;

    bool isTypescriptArray(vertex_descriptor vertID, std::pmr::memory_resource* scratch) const;

    std::pmr::string getTypescriptTypename(vertex_descriptor vertID,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const;

    std::pmr::string getTypescriptTypename(std::string_view typePath,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const;

    std::pmr::string getTypescriptTagName(vertex_descriptor vertID,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const;

    std::pmr::string getTypescriptTagName(std::string_view typePath,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const;

    std::pmr::string getTypescriptInitialValue(
        vertex_descriptor vertID, std::string_view initial,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const;

    std::pmr::string getTypescriptGraphPolymorphicVariant(const Graph2& s,
        std::pmr::memory_resource* mr,
        std::pmr::memory_resource* scratch) const;

    bool moduleUsesGraph(std::string_view modulePath) const;

    PmrMap<std::pmr::string, PmrSet<std::pmr::string>> getImportedTypes(
        std::string_view modulePath, std::pmr::memory_resource* mr) const;

    // ContinuousContainer
    void reserve(vertices_size_type sz);

    // Members
    struct object_type {
        using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
        allocator_type get_allocator() const noexcept;

        object_type(const allocator_type& alloc);
        object_type(object_type&& rhs, const allocator_type& alloc);
        object_type(const object_type& rhs, const allocator_type& alloc);

        object_type(object_type&&) noexcept;
        object_type(object_type const&) = delete;
        object_type& operator=(object_type&&);
        object_type& operator=(object_type const&);
        ~object_type() noexcept;

        std::pmr::vector<children_edge_type> mChildren;
        std::pmr::vector<parent_edge_type> mParents;
    };

    struct vertex_type {
        using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
        allocator_type get_allocator() const noexcept;

        vertex_type(const allocator_type& alloc);
        vertex_type(vertex_type&& rhs, const allocator_type& alloc);
        vertex_type(const vertex_type& rhs, const allocator_type& alloc);

        vertex_type(vertex_type&&) noexcept;
        vertex_type(vertex_type const&) = delete;
        vertex_type& operator=(vertex_type&&);
        vertex_type& operator=(vertex_type const&);
        ~vertex_type() noexcept;

        std::pmr::vector<out_edge_type> mOutEdges;
        std::pmr::vector<in_edge_type> mInEdges;
        vertex_handle_type mHandle;
    };

    struct names_ {
    } static constexpr names = {};
    struct traits_ {
    } static constexpr traits = {};
    struct modulePaths_ {
    } static constexpr modulePaths = {};
    struct typescripts_ {
    } static constexpr typescripts = {};

    // Owners
    std::pmr::vector<object_type> mObjects;
    // Vertices
    std::pmr::vector<vertex_type> mVertices;
    // Components
    std::pmr::vector<std::pmr::string> mNames;
    std::pmr::vector<Traits> mTraits;
    std::pmr::vector<std::pmr::string> mModulePaths;
    std::pmr::vector<Typescript> mTypescripts;
    // PolymorphicGraph
    std::pmr::vector<Enum> mEnums;
    std::pmr::vector<Tag> mTags;
    std::pmr::vector<Struct> mStructs;
    std::pmr::vector<Graph2> mGraphs;
    std::pmr::vector<Variant> mVariants;
    std::pmr::vector<Instance> mInstances;
    // Path
    PmrMap<std::pmr::string, uint32_t> mPathIndex;
};

struct ModuleInfo {
    bool mExport = false;
    std::string mAPI;
    std::string mFolder;
    std::string mFilePrefix;
    std::string mExportAs;
};

struct ModuleGraph {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept;

    inline std::pmr::memory_resource* resource() const noexcept {
        return get_allocator().resource();
    }

    ModuleGraph(const allocator_type& alloc);
    ModuleGraph(ModuleGraph&& rhs, const allocator_type& alloc);
    ModuleGraph(ModuleGraph const& rhs, const allocator_type& alloc);

    ModuleGraph(ModuleGraph&& rhs);
    ModuleGraph(ModuleGraph const& rhs) = delete;
    ModuleGraph& operator=(ModuleGraph&& rhs);
    ModuleGraph& operator=(ModuleGraph const& rhs);
    ~ModuleGraph() noexcept;

    // Graph
    using directed_category = boost::bidirectional_tag;
    using vertex_descriptor = uint32_t;
    using edge_descriptor = Graph::EdgeDescriptor<directed_category, vertex_descriptor>;
    using edge_parallel_category = boost::allow_parallel_edge_tag;
    struct traversal_category
        : virtual boost::incidence_graph_tag
        , virtual boost::bidirectional_graph_tag
        , virtual boost::adjacency_graph_tag
        , virtual boost::vertex_list_graph_tag
        , virtual boost::edge_list_graph_tag {};

    constexpr static vertex_descriptor null_vertex() noexcept {
        return std::numeric_limits<vertex_descriptor>::max();
    }

    // IncidenceGraph
    using out_edge_type = Graph::StoredEdge<vertex_descriptor>;
    using out_edge_iterator = Graph::OutEdgeIter<
        std::pmr::vector<out_edge_type>::iterator,
        vertex_descriptor, edge_descriptor, int32_t>;
    using degree_size_type = uint32_t;

    // BidirectionalGraph
    using in_edge_type = Graph::StoredEdge<vertex_descriptor>;
    using in_edge_iterator = Graph::InEdgeIter<
        std::pmr::vector<in_edge_type>::iterator,
        vertex_descriptor, edge_descriptor, int32_t>;

    // AdjacencyGraph
    using adjacency_iterator = boost::adjacency_iterator_generator<
        ModuleGraph, vertex_descriptor, out_edge_iterator>::type;

    // VertexListGraph
    using vertex_iterator = boost::integer_range<vertex_descriptor>::iterator;
    using vertices_size_type = uint32_t;

    // VertexList help functions
    inline std::pmr::vector<out_edge_type>& out_edge_list(vertex_descriptor v) noexcept {
        return mVertices[v].mOutEdges;
    }
    inline const std::pmr::vector<out_edge_type>& out_edge_list(vertex_descriptor v) const noexcept {
        return mVertices[v].mOutEdges;
    }

    inline std::pmr::vector<in_edge_type>& in_edge_list(vertex_descriptor v) noexcept {
        return mVertices[v].mInEdges;
    }
    inline const std::pmr::vector<in_edge_type>& in_edge_list(vertex_descriptor v) const noexcept {
        return mVertices[v].mInEdges;
    }

    inline boost::integer_range<vertex_descriptor> vertex_set() const noexcept {
        return boost::integer_range<vertex_descriptor>(0, gsl::narrow_cast<vertices_size_type>(mVertices.size()));
    }

    inline vertex_descriptor current_id() const noexcept {
        return gsl::narrow_cast<vertex_descriptor>(mVertices.size());
    }

    [[nodiscard]] inline std::pmr::vector<boost::default_color_type> colors(std::pmr::memory_resource* mr) const {
        return std::pmr::vector<boost::default_color_type>{ mVertices.size(), mr };
    }

    // EdgeListGraph
    using edge_iterator = Graph::DirectedEdgeIterator<vertex_iterator, out_edge_iterator, ModuleGraph>;
    using edges_size_type = uint32_t;

    // AddressableGraph (Separated)
    using ownership_descriptor = Graph::EdgeDescriptor<boost::bidirectional_tag, vertex_descriptor>;

    using children_edge_type = Graph::StoredEdge<vertex_descriptor>;
    using children_iterator = Graph::OutEdgeIter<
        std::pmr::vector<children_edge_type>::iterator,
        vertex_descriptor, ownership_descriptor, int32_t>;
    using children_size_type = uint32_t;

    using parent_edge_type = Graph::StoredEdge<vertex_descriptor>;
    using parent_iterator = Graph::InEdgeIter<
        std::pmr::vector<parent_edge_type>::iterator,
        vertex_descriptor, ownership_descriptor, int32_t>;

    using ownership_iterator = Graph::OwnershipIterator<vertex_iterator, children_iterator, ModuleGraph>;
    using ownerships_size_type = edges_size_type;

    // AddressableGraph help functions
    inline std::pmr::vector<children_edge_type>& children_list(vertex_descriptor v) noexcept {
        return mObjects[v].mChildren;
    }
    inline const std::pmr::vector<children_edge_type>& children_list(vertex_descriptor v) const noexcept {
        return mObjects[v].mChildren;
    }

    inline std::pmr::vector<parent_edge_type>& parents_list(vertex_descriptor v) noexcept {
        return mObjects[v].mParents;
    }
    inline const std::pmr::vector<parent_edge_type>& parents_list(vertex_descriptor v) const noexcept {
        return mObjects[v].mParents;
    }

    // ContinuousContainer
    void reserve(vertices_size_type sz);

    // Members
    struct object_type {
        using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
        allocator_type get_allocator() const noexcept;

        object_type(const allocator_type& alloc);
        object_type(object_type&& rhs, const allocator_type& alloc);
        object_type(const object_type& rhs, const allocator_type& alloc);

        object_type(object_type&&) noexcept;
        object_type(object_type const&) = delete;
        object_type& operator=(object_type&&);
        object_type& operator=(object_type const&);
        ~object_type() noexcept;

        std::pmr::vector<children_edge_type> mChildren;
        std::pmr::vector<parent_edge_type> mParents;
    };

    struct vertex_type {
        using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
        allocator_type get_allocator() const noexcept;

        vertex_type(const allocator_type& alloc);
        vertex_type(vertex_type&& rhs, const allocator_type& alloc);
        vertex_type(const vertex_type& rhs, const allocator_type& alloc);

        vertex_type(vertex_type&&) noexcept;
        vertex_type(vertex_type const&) = delete;
        vertex_type& operator=(vertex_type&&);
        vertex_type& operator=(vertex_type const&);
        ~vertex_type() noexcept;

        std::pmr::vector<out_edge_type> mOutEdges;
        std::pmr::vector<in_edge_type> mInEdges;
    };

    struct names_ {
    } static constexpr names = {};
    struct modules_ {
    } static constexpr modules = {};

    // Owners
    std::pmr::vector<object_type> mObjects;
    // Vertices
    std::pmr::vector<vertex_type> mVertices;
    // Components
    std::pmr::vector<std::pmr::string> mNames;
    std::pmr::vector<ModuleInfo> mModules;
    // Path
    PmrMap<std::pmr::string, uint32_t> mPathIndex;
};

} // namespace Meta

} // namespace Cocos
