#pragma once
#include <functional>
#include <compare>
#include <gsl/gsl_assert>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/graph/adjacency_iterator.hpp>
#include <boost/graph/properties.hpp>
#include <Cocos/Concept.h>
#include <Cocos/GraphTypes.h>

namespace Cocos::Impl {

//using BidirEdge = Graph::EdgeDescriptorWithProperty<boost::bidirectional_tag, uint32_t>;

template<class T>
concept PropertyEdge_ = requires(T e) {
    e.get_property();
};

template<class G>
concept Graph_ = requires(G) {
    typename G::vertex_descriptor;
    typename G::edge_descriptor;
    typename G::directed_category;
    typename G::edge_parallel_category;
    typename G::traversal_category;
};

template<class G>
concept IncidenceGraph_ = Graph_<G> && requires(G) {
    typename G::out_edge_iterator;
    typename G::degree_size_type;
};

template<class G>
concept BidirectionalGraph_ = IncidenceGraph_<G> && requires(G) {
    typename G::in_edge_iterator;
};

template<class T>
concept BidirectionalEdgeListGraph_ = BidirectionalGraph_<T> && requires(T g) {
    g.mEdges;
};

template<class T>
concept AssociativeContainer_ = requires(T) {
    typename T::key_type;
};

// OwnershipIterator (Bidirectional, !EdgeProperty)
template<class VertexIterator, class OutEdgeIterator, class Graph>
class OwnershipIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename OutEdgeIterator::value_type;
    using reference = typename OutEdgeIterator::reference;
    using pointer = typename OutEdgeIterator::pointer;
    using difference_type = typename OutEdgeIterator::difference_type;
    using distance_type = difference_type;

    OwnershipIterator() = default;
    template<class G>
    OwnershipIterator(VertexIterator b, VertexIterator c, VertexIterator e, const G& g) noexcept
        : mBegin(b), mCurr(c), mEnd(e), mG(&g)
    {
        if (mCurr != mEnd) {
            while (mCurr != mEnd && num_children(*mCurr, *mG) == 0)
                ++mCurr;
            if (mCurr != mEnd)
                mEdges = children(*mCurr, *mG);
        }
    }

    OwnershipIterator& operator++() noexcept {
        ++mEdges->first;
        if (mEdges->first == mEdges->second) {
            ++mCurr;
            while (mCurr != mEnd && num_children(*mCurr, *mG) == 0)
                ++mCurr;
            if (mCurr != mEnd)
                mEdges = children(*mCurr, *mG);
        }
        return *this;
    }
    OwnershipIterator operator++(int) noexcept {
        OwnershipIterator tmp = *this;
        ++(*this);
        return tmp;
    }
    value_type operator*() const noexcept {
        return *mEdges->first;
    }
    bool operator==(const OwnershipIterator& x) const noexcept {
        return mCurr == x.mCurr && (mCurr == mEnd || mEdges->first == x.mEdges->first);
    }
    bool operator!=(const OwnershipIterator& x) const noexcept {
        return mCurr != x.mCurr || (mCurr != mEnd && mEdges->first != x.mEdges->first);
    }
protected:
    VertexIterator mBegin = {};
    VertexIterator mCurr = {};
    VertexIterator mEnd = {};
    std::optional<std::pair<OutEdgeIterator, OutEdgeIterator>> mEdges;
    const Graph* mG = nullptr;
};

//--------------------------------------------------------------------
// PropertyMap
//--------------------------------------------------------------------
template<class Category, class Graph, class Value, class Reference>
struct VectorVertexBundlePropertyMap
    : public boost::put_get_helper<
        Reference, VectorVertexBundlePropertyMap<Category, Graph, Value, Reference>
    >
{
    using value_type = Value;
    using reference = Reference;
    using key_type = typename Graph::vertex_descriptor;
    using category = Category;

    VectorVertexBundlePropertyMap(Graph& g) noexcept
        : mGraph(&g)
    {}

    inline reference operator[](const key_type& v) const noexcept {
        return mGraph->mVertices[v].mProperty;
    }
    inline reference operator()(const key_type& v) const noexcept {
        return this->operator[](v);
    }

    Graph* mGraph = nullptr;
};

template<class Category, class Graph, class Value, class Reference>
struct PointerVertexBundlePropertyMap
    : public boost::put_get_helper<
        Reference, PointerVertexBundlePropertyMap<Category, Graph, Value, Reference>
    >
{
    using value_type = Value;
    using reference = Reference;
    using key_type = typename Graph::vertex_descriptor;
    using category = Category;

    PointerVertexBundlePropertyMap(Graph& g) noexcept
        : mGraph(&g)
    {}

    inline reference operator[](const key_type& v) const noexcept {
        auto* sv = static_cast<typename Graph::vertex_type*>(v);
        return sv->mProperty;
    }
    inline reference operator()(const key_type& v) const noexcept {
        return this->operator[](v);
    }

    Graph* mGraph = nullptr;
};

template<class Category, class Graph, class Value, class Reference, class MemberPointer>
struct VectorVertexBundleMemberPropertyMap
    : public boost::put_get_helper<
        Reference, VectorVertexBundleMemberPropertyMap<Category, Graph, Value, Reference, MemberPointer>
    >
{
    using value_type = Value;
    using reference = Reference;
    using key_type = typename Graph::vertex_descriptor;
    using category = Category;

    VectorVertexBundleMemberPropertyMap(Graph& g, MemberPointer ptr) noexcept
        : mGraph(&g)
        , mMemberPointer(ptr)
    {}

    inline reference operator[](const key_type& v) const noexcept {
        return mGraph->mVertices[v].mProperty.*mMemberPointer;
    }
    inline reference operator()(const key_type& v) const noexcept {
        return this->operator[](v);
    }

    Graph* mGraph = nullptr;
    MemberPointer mMemberPointer = {};
};

template<class Category, class Graph, class Value, class Reference, class MemberPointer>
struct PointerVertexBundleMemberPropertyMap
    : public boost::put_get_helper<
        Reference, PointerVertexBundleMemberPropertyMap<Category, Graph, Value, Reference, MemberPointer>
    >
{
    using value_type = Value;
    using reference = Reference;
    using key_type = typename Graph::vertex_descriptor;
    using category = Category;

    PointerVertexBundleMemberPropertyMap(Graph& g, MemberPointer ptr) noexcept
        : mGraph(&g)
        , mMemberPointer(ptr)
    {}

    inline reference operator[](const key_type& v) const noexcept {
        auto* sv = static_cast<typename Graph::vertex_type*>(v);
        return sv->mProperty.*mMemberPointer;
    }
    inline reference operator()(const key_type& v) const noexcept {
        return this->operator[](v);
    }

    Graph* mGraph = nullptr;
    MemberPointer mMemberPointer = {};
};

template<class Category, class Graph, class Container, class Value, class Reference>
struct VectorVertexComponentPropertyMap
    : public boost::put_get_helper<
        Reference, VectorVertexComponentPropertyMap<Category, Graph, Container, Value, Reference>
    >
{
    using value_type = Value;
    using reference = Reference;
    using key_type = typename Graph::vertex_descriptor;
    using category = Category;

    VectorVertexComponentPropertyMap(Container& c) noexcept
        : mContainer(&c)
    {}

    inline reference operator[](const key_type& v) const noexcept {
        return (*mContainer)[v];
    }
    inline reference operator()(const key_type& v) const noexcept {
        return this->operator[](v);
    }

    Container* mContainer = nullptr;
};

template<class Category, class Graph, class Container, class Value, class Reference, class MemberPointer>
struct VectorVertexComponentMemberPropertyMap
    : public boost::put_get_helper<
        Reference, VectorVertexComponentMemberPropertyMap<Category, Graph, Container, Value, Reference, MemberPointer>
    >
{
    using value_type = Value;
    using reference = Reference;
    using key_type = typename Graph::vertex_descriptor;
    using category = Category;

    VectorVertexComponentMemberPropertyMap(Container& c, MemberPointer ptr) noexcept
        : mContainer(&c)
        , mMemberPointer(ptr)
    {}

    inline reference operator[](const key_type& v) const noexcept {
        return (*mContainer)[v].*mMemberPointer;
    }
    inline reference operator()(const key_type& v) const noexcept {
        return this->operator[](v);
    }

    Container* mContainer = nullptr;
    MemberPointer mMemberPointer = {};
};

template<class Category, class Graph, class ComponentPointer, class Value, class Reference>
struct VectorVertexIteratorComponentPropertyMap
    : public boost::put_get_helper<
        Reference, VectorVertexIteratorComponentPropertyMap<Category, Graph, ComponentPointer, Value, Reference>
    >
{
    using value_type = Value;
    using reference = Reference;
    using key_type = typename Graph::vertex_descriptor;
    using category = Category;

    VectorVertexIteratorComponentPropertyMap(Graph& g, ComponentPointer component) noexcept
        : mGraph(&g)
        , mComponentPointer(component)
    {}

    inline reference operator[](const key_type& v) const noexcept {
        return *(mGraph->mVertices[v].*mComponentPointer);
    }
    inline reference operator()(const key_type& v) const noexcept {
        return this->operator[](v);
    }

    Graph* mGraph = nullptr;
    ComponentPointer mComponentPointer = {};
};

template<class Category, class Graph, class ComponentPointer, class Value, class Reference, class MemberPointer>
struct VectorVertexIteratorComponentMemberPropertyMap
    : public boost::put_get_helper<
        Reference, VectorVertexIteratorComponentMemberPropertyMap<Category, Graph, ComponentPointer, Value, Reference, MemberPointer>
    >
{
    using value_type = Value;
    using reference = Reference;
    using key_type = typename Graph::vertex_descriptor;
    using category = Category;

    VectorVertexIteratorComponentMemberPropertyMap(Graph& g, ComponentPointer component, MemberPointer ptr) noexcept
        : mGraph(&g)
        , mComponentPointer(component)
        , mMemberPointer(ptr)
    {}

    inline reference operator[](const key_type& v) const noexcept {
        return (*(mGraph->mVertices[v].*mComponentPointer)).*mMemberPointer;
    }
    inline reference operator()(const key_type& v) const noexcept {
        return this->operator[](v);
    }

    Graph* mGraph = nullptr;
    ComponentPointer mComponentPointer = {};
    MemberPointer mMemberPointer = {};
};

template<class Category, class VertexDescriptor, class Container, class Value, class Reference>
struct VectorPathPropertyMap
    : public boost::put_get_helper<
        Reference, VectorPathPropertyMap<Category, VertexDescriptor, Container, Value, Reference>
    >
{
    using value_type = Value;
    using reference = Reference;
    using key_type = VertexDescriptor;
    using category = Category;

    VectorPathPropertyMap(Container& c) noexcept
        : mContainer(&c)
    {}

    inline reference operator[](const key_type& v) const noexcept {
        return (*mContainer)[v].mPathIterator->first;
    }
    inline reference operator()(const key_type& v) const noexcept {
        return this->operator[](v);
    }

    Container* mContainer = nullptr;
};

template<class Category, class Graph, class Value, class Reference>
struct EdgeBundlePropertyMap
    : public boost::put_get_helper<
        Reference, EdgeBundlePropertyMap<Category, Graph, Value, Reference>
    >
{
    using value_type = Value;
    using reference = Reference;
    using key_type = typename Graph::edge_descriptor;
    using category = Category;

    EdgeBundlePropertyMap(Graph& g) noexcept
        : mGraph(&g)
    {}

    inline reference operator[](const key_type& e) const noexcept {
        return *static_cast<typename Graph::edge_property_type*>(e.get_property());
    }
    inline reference operator()(const key_type& e) const noexcept {
        return this->operator[](e);
    }

    Graph* mGraph = nullptr;
};

template<class Category, class Graph, class Value, class Reference, class MemberPointer>
struct EdgeBundleMemberPropertyMap
    : public boost::put_get_helper<
        Reference, EdgeBundleMemberPropertyMap<Category, Graph, Value, Reference, MemberPointer>
    >
{
    using value_type = Value;
    using reference = Reference;
    using key_type = typename Graph::edge_descriptor;
    using category = Category;

    EdgeBundleMemberPropertyMap(Graph& g, MemberPointer ptr) noexcept
        : mGraph(&g)
        , mMemberPointer(ptr)
    {}

    inline reference operator[](const key_type& e) const noexcept {
        auto& p = *static_cast<typename Graph::edge_property_type*>(e.get_property());
        return p.*mMemberPointer;
    }
    inline reference operator()(const key_type& e) const noexcept {
        return this->operator[](e);
    }

    Graph* mGraph = nullptr;
    MemberPointer mMemberPointer = {};
};

// Conceptual Graph
template<class Tag, class Handle>
struct ValueHandle : Tag {
    ValueHandle() noexcept = default;
    ValueHandle(ValueHandle&& rhs) noexcept
        : mValue(std::move(rhs.mValue))
    {}
    ValueHandle(const ValueHandle& rhs) noexcept
        : mValue(rhs.mValue)
    {}
    ValueHandle& operator=(ValueHandle&& rhs) noexcept {
        mValue = std::move(rhs.mValue);
        return *this;
    }
    ValueHandle& operator=(const ValueHandle& rhs) noexcept {
        mValue = rhs.mValue;
        return *this;
    }

    ValueHandle(const Handle& handle) noexcept : mValue(handle) {}
    ValueHandle(Handle&& handle) noexcept : mValue(std::move(handle)) {}
    template<class... Args>
    ValueHandle(Args&&... args) noexcept
        : mValue(std::forward<Args>(args)...)
    {}

    Handle mValue = {};
};

template<class Sequence, class Predicate>
void sequenceEraseIf(Sequence& c, Predicate&& p) noexcept {
    if (!c.empty())
        c.erase(std::remove_if(c.begin(), c.end(), p), c.end());
}

// notice: Predicate might be different from associative key
// when Predicate is associative key, it is slower than erase [lower_bound, upper_bound)
template<class AssociativeContainer, class Predicate>
void associativeEraseIf(AssociativeContainer& c, Predicate&& p) noexcept {
    auto next = c.begin();
    for (auto i = next; next != c.end(); i = next) {
        ++next;
        if (p(*i))
            c.erase(i);
    }
}

// notice: Predicate might be different from associative key
// when Predicate is associative key, it is slower than erase [lower_bound, upper_bound)
template<class AssociativeContainer, class Predicate>
void unstableAssociativeEraseIf(AssociativeContainer& c, Predicate&& p) noexcept {
    auto n = c.size();
    while (n--) {
        for (auto i = c.begin(); i != c.end(); ++i) {
            if (p(*i)) {
                c.erase(i);
                break;
            }
        }
    }
}

template<class EdgeDescriptor, class IncidenceList>
inline void removeIncidenceEdge(EdgeDescriptor e, IncidenceList& el) noexcept {
    e.expectsNoProperty();
    for (auto i = el.begin(); i != el.end(); ++i) {
        if ((*i).get_target() == e.m_target) {
            el.erase(i);
            return;
        }
    }
}

template<PropertyEdge_ EdgeDescriptor, class IncidenceList, class EdgeProperty>
inline void removeIncidenceEdge(EdgeDescriptor e, IncidenceList& el) noexcept {
    for (auto i = el.begin(); i != el.end(); ++i) {
        if (static_cast<void*>(&(*i).get_property()) == e.get_property()) {
            el.erase(i);
            return;
        }
    }
}

template<class Graph, class IncidenceList, class VertexDescriptor>
inline void removeDirectedAllEdgeProperties(Graph& g, IncidenceList& el, VertexDescriptor v) noexcept {
    auto i = el.begin(), end = el.end();
    for (; i != end; ++i) {
        if ((*i).get_target() == v) {
            // NOTE: Wihtout this skip, this loop will double-delete
            // properties of loop edges. This solution is based on the
            // observation that the incidence edges of a vertex with a loop
            // are adjacent in the out edge list. This *may* actually hold
            // for multisets also.
            bool skip = (std::next(i) != end
                && i->get_iter() == std::next(i)->get_iter());
            g.mEdges.erase((*i).get_iter());
            if (skip)
                ++i;
        }
    }
}

template<class IncidenceIterator, class IncidenceList, class Predicate>
inline void sequenceRemoveIncidenceEdgeIf(IncidenceIterator first, IncidenceIterator last,
    IncidenceList& el, Predicate&& pred
) noexcept {
    // remove_if
    while (first != last && !pred(*first))
        ++first;
    if (auto i = first; first != last) {
        for (++i; i != last; ++i) {
            if (!pred(*i)) {
                *first.base() = std::move(*i.base());
                ++first;
            }
        }
    }
    el.erase(first.base(), el.end());
}

template<class IncidenceIterator, class IncidenceList, class Predicate>
inline void associativeRemoveIncidenceEdgeIf(IncidenceIterator first, IncidenceIterator last,
    IncidenceList& el, Predicate&& pred
) noexcept {
    for (auto next = first; first != last; first = next) {
        ++next;
        if (pred(*first)) {
            el.erase(first.base());
        }
    }
}

template<class Graph, class EdgeDescriptor, class EdgeProperty >
inline void removeUndirectedEdge(Graph& g, EdgeDescriptor e, EdgeProperty& p) noexcept {
    auto& out_el = g.out_edge_list(source(e, g));
    auto out_i = out_el.begin();
    decltype((*out_i).get_iter()) edge_iter_to_erase;
    for (; out_i != out_el.end(); ++out_i)
        if (&(*out_i).get_property() == &p) {
            edge_iter_to_erase = (*out_i).get_iter();
            out_el.erase(out_i);
            break;
        }
    auto& in_el = g.out_edge_list(target(e, g));
    auto in_i = in_el.begin();
    for (; in_i != in_el.end(); ++in_i)
        if (&(*in_i).get_property() == &p) {
            in_el.erase(in_i);
            break;
        }
    g.mEdges.erase(edge_iter_to_erase);
}

template<class Graph, class IncidenceIterator, class IncidenceList, class Predicate>
inline void sequenceRemoveUndirectedOutEdgeIf(Graph& g,
    IncidenceIterator first, IncidenceIterator last, IncidenceList& el,
    Predicate&& pred
) noexcept {
    // remove_if
    while (first != last && !pred(*first))
        ++first;
    auto i = first;
    bool self_loop_removed = false;
    if (first != last) {
        for (; i != last; ++i) {
            if (self_loop_removed) {
                /* With self loops, the descriptor will show up
                 * twice. The first time it will be removed, and now it
                 * will be skipped.
                 */
                self_loop_removed = false;
            } else if (!pred(*i)) {
                *first.base() = std::move(*i.base());
                ++first;
            } else {
                if (source(*i, g) == target(*i, g)) {
                    self_loop_removed = true;
                } else {
                    // Remove the edge from the target
                    removeIncidenceEdge(*i, g.out_edge_list(target(*i, g)));
                }

                // Erase the edge property
                g.mEdges.erase((*i.base()).get_iter());
            }
        }
    }
    el.erase(first.base(), el.end());
}

template<class Graph, class IncidenceIterator, class IncidenceList, class Predicate>
inline void associativeRemoveUndirectedOutEdgeIf(Graph& g,
    IncidenceIterator first, IncidenceIterator last, IncidenceList& el,
    Predicate&& pred
) noexcept {
    for (auto next = first; first != last; first = next) {
        ++next;
        if (pred(*first)) {
            if (source(*first, g) != target(*first, g)) {
                // Remove the edge from the target
                removeIncidenceEdge(*first, g.out_edge_list(target(*first, g)));
            }

            // Erase the edge property
            g.mEdges.erase((*first.base()).get_iter());

            // Erase the edge in the source
            el.erase(first.base());
        }
    }
}

// list/vector out_edge_list
template<class IncidenceList, class VertexDescriptor>
inline void reindexEdgeList(IncidenceList& el, VertexDescriptor u) {
    auto ei = el.begin();
    auto e_end = el.end();
    for (; ei != e_end; ++ei) {
        if ((*ei).get_target() > u) {
            --(*ei).get_target();
        }
    }
}

// associative out_edge_list, with out_edge property
template<class IncidenceList, class VertexDescriptor>
requires AssociativeContainer_<IncidenceList> &&
requires(typename IncidenceList::value_type v) {
    v.mProperty;
}
inline void reindexEdgeList(IncidenceList& el, VertexDescriptor u) {
    // this reindex is tricky, we assume vertex descriptor is stored in *ascending order*
    // so when we decrease the number of the vertex descriptor,
    // it is always inserted before the current iter!
    // As a result, the vertex descriptor will never be decreased by twice
    for (auto iter = el.begin(); iter != el.end();) {
        if (iter->get_target() > u) {
            auto& ce = const_cast<typename IncidenceList::value_type&>(*iter);
            auto target = ce.get_target() - 1;
            auto prop = std::move(ce.mProperty);
            iter = el.erase(iter); // for c++11, erase will return next iterator
            el.emplace(target, std::move(prop)); // will insert target *before* iter
        } else {
            ++iter;
        }
    }
}

// associative out_edge_list, without out_edge property
template<AssociativeContainer_ IncidenceList, class VertexDescriptor>
inline void reindexEdgeList(IncidenceList& el, VertexDescriptor u) {
    // this reindex is tricky, we assume vertex descriptor is stored in *ascending order*
    // so when we decrease the number of the vertex descriptor,
    // it is always inserted before the current iter!
    // As a result, the vertex descriptor will never be decreased by twice
    for (auto iter = el.begin(); iter != el.end();) {
        if (iter->get_target() > u) {
            const auto& ce = *iter;
            auto target = ce.get_target() - 1;
            iter = el.erase(iter); // for c++11, erase will return next iterator
            el.emplace(target); // will insert target *before* iter
        } else {
            ++iter;
        }
    }
}

template<class Tag, class Container, class HandleDescriptor>
inline void reindexVectorHandle(Container& container, HandleDescriptor u) {
    static_assert(std::is_arithmetic_v<HandleDescriptor>);

    using handle_type = ValueHandle<Tag, HandleDescriptor>;
    for (auto& vert : container) {
        if (std::holds_alternative<handle_type>(vert.mHandle)) {
            auto& v = std::get<handle_type>(vert.mHandle).mValue;
            if (v > u) {
                --v;
            }
        }
    }
}

template<Graph_ Graph, class VertexDescriptor, class DirectedCategory>
inline void removeVectorVertex(Graph& g, VertexDescriptor u, DirectedCategory) {
    g.mVertices.erase(g.mVertices.begin() + u);
}

template<IncidenceGraph_ Graph, class VertexDescriptor>
inline void removeVectorVertex(Graph& g, VertexDescriptor u, boost::directed_tag) {
    g.mVertices.erase(g.mVertices.begin() + u);
    auto V = num_vertices(g);
    if (u != V) {
        for (VertexDescriptor v = 0; v < V; ++v) {
            reindexEdgeList(g.out_edge_list(v), u);
        }
    }
}

template<IncidenceGraph_ Graph, class VertexDescriptor>
inline void removeVectorVertex(Graph& g, VertexDescriptor u, boost::undirected_tag) {
    g.mVertices.erase(g.mVertices.begin() + u);
    VertexDescriptor V = num_vertices(g);
    for (VertexDescriptor v = 0; v < V; ++v)
        reindexEdgeList(g.out_edge_list(v), u);

    auto ei = g.mEdges.begin(), ei_end = g.mEdges.end();
    for (; ei != ei_end; ++ei) {
        if (ei->m_source > u)
            --ei->m_source;
        if (ei->m_target > u)
            --ei->m_target;
    }
}

template<BidirectionalGraph_ Graph, class VertexDescriptor>
inline void removeVectorVertex(Graph& g, VertexDescriptor u, boost::bidirectional_tag) {
    g.mVertices.erase(g.mVertices.begin() + u);
    VertexDescriptor V = num_vertices(g);
    VertexDescriptor v;
    if (u != V) {
        for (v = 0; v < V; ++v)
            reindexEdgeList(g.out_edge_list(v), u);

        for (v = 0; v < V; ++v)
            reindexEdgeList(g.in_edge_list(v), u);
    }
}

template<BidirectionalEdgeListGraph_ Graph, class VertexDescriptor>
inline void removeVectorVertex(Graph& g, VertexDescriptor u, boost::bidirectional_tag) {
    g.mVertices.erase(g.mVertices.begin() + u);
    VertexDescriptor V = num_vertices(g);
    VertexDescriptor v;
    if (u != V) {
        for (v = 0; v < V; ++v)
            reindexEdgeList(g.out_edge_list(v), u);

        for (v = 0; v < V; ++v)
            reindexEdgeList(g.in_edge_list(v), u);

        auto ei = g.mEdges.begin(), ei_end = g.mEdges.end();
        for (; ei != ei_end; ++ei) {
            if (ei->m_source > u)
                --ei->m_source;
            if (ei->m_target > u)
                --ei->m_target;
        }
    }
}

template<class Graph>
inline void removeVectorOwner(Graph& g, typename Graph::vertex_descriptor u) {
    // might make children detached
    g.mObjects.erase(g.mObjects.begin() + u);
    auto V = num_vertices(g);
    if (u != V) {
        for (typename Graph::vertex_descriptor v = 0; v < V; ++v)
            reindexEdgeList(g.children_list(v), u);

        for (typename Graph::vertex_descriptor v = 0; v < V; ++v)
            reindexEdgeList(g.parents_list(v), u);
    }
}

// AddressableGraph
template<class AddressableGraph>
inline std::ptrdiff_t pathLength(typename AddressableGraph::vertex_descriptor u, const AddressableGraph& g,
    typename AddressableGraph::vertex_descriptor parentID = AddressableGraph::null_vertex()) noexcept {
    if (u == parentID)
        return 0;

    const auto& pmap = get(boost::vertex_name, g);

    std::ptrdiff_t sz = 0;
    while (u != parentID) {
        sz += std::ssize(get(pmap, u)) + 1;
        u = parent(u, g);
    }

    return sz;
}

template<class AddressableGraph, class CharT, class Allocator>
inline void pathComposite(
    std::basic_string<CharT, std::char_traits<CharT>, Allocator>& str,
    std::ptrdiff_t& sz,
    typename AddressableGraph::vertex_descriptor u, const AddressableGraph& g,
    typename AddressableGraph::vertex_descriptor parentID = AddressableGraph::null_vertex()) noexcept {

    const auto& pmap = get(boost::vertex_name, g);

    while (u != parentID) {
        Expects(sz <= std::ssize(str));

        const auto& name = get(pmap, u);
        sz -= std::ssize(name) + 1;
        Ensures(sz >= 0);
        str[sz] = '/';
        std::copy(name.begin(), name.end(), str.begin() + sz + 1);

        u = parent(u, g);
    }
    Ensures(sz == 0);
}

template<class Key>
struct ColorMap : public boost::put_get_helper<boost::default_color_type&, ColorMap<Key>> {
    using value_type = boost::default_color_type;
    using reference = boost::default_color_type&;
    using key_type = Key;
    using category = boost::lvalue_property_map_tag;

    ColorMap(std::pmr::vector<boost::default_color_type>& vec) noexcept
        : mContainer{ &vec }
    {}

    [[nodiscard]] inline reference operator[](const key_type& v) const noexcept {
        return (*mContainer)[v];
    }
    [[nodiscard]] inline reference operator()(const key_type& v) const noexcept {
        return this->operator[](v);
    }

    std::pmr::vector<boost::default_color_type>* mContainer = nullptr;
};

}

namespace std {

template<class DirectedCategory, class VertexDescriptor>
struct hash<Cocos::Impl::EdgeDescriptor<DirectedCategory, VertexDescriptor>> {
    size_t operator()(const Cocos::Impl::EdgeDescriptor<DirectedCategory, VertexDescriptor>& e) const noexcept {
        return boost::hash_value(e.get_property());
    }
};

}
