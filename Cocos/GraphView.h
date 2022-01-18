#pragma once
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>

namespace Cocos::Impl {

template<class GraphT>
struct AddressableView {
    AddressableView(GraphT& g) noexcept
        : mGraph(g)
    {}

    // GraphT
    using directed_category = boost::bidirectional_tag;
    using vertex_descriptor = typename GraphT::vertex_descriptor;
    using edge_descriptor = typename GraphT::ownership_descriptor;
    using edge_parallel_category = boost::allow_parallel_edge_tag;
    struct traversal_category
        : virtual boost::incidence_graph_tag
        , virtual boost::bidirectional_graph_tag
        , virtual boost::adjacency_graph_tag
        , virtual boost::vertex_list_graph_tag
        , virtual boost::edge_list_graph_tag
    {};

    static vertex_descriptor null_vertex() noexcept {
        return GraphT::null_vertex();
    }
    
    // IncidenceGraph
    using out_edge_iterator = typename GraphT::children_iterator;
    using degree_size_type = typename GraphT::children_size_type;

    // BidirectionalGraph
    using in_edge_iterator = typename GraphT::parent_iterator;

    // AdjacencyGraph
    using adjacency_iterator = typename boost::adjacency_iterator_generator<
        AddressableView, vertex_descriptor, out_edge_iterator
    >::type;

    // VertexListGraph
    using vertex_iterator = typename GraphT::vertex_iterator;
    using vertices_size_type = typename GraphT::vertices_size_type;

    // EdgeListGraph
    using edge_iterator = typename GraphT::ownership_iterator;
    using edges_size_type = typename GraphT::ownerships_size_type;

    // Member
    GraphT& mGraph;
};

// IncidenceGraph
template<class GraphT>
inline typename AddressableView<GraphT>::vertex_descriptor
source(const typename AddressableView<GraphT>::edge_descriptor& e, const AddressableView<GraphT>& g) noexcept {
    return parent(e, g.mGraph);
}

template<class GraphT>
inline typename AddressableView<GraphT>::vertex_descriptor
target(const typename AddressableView<GraphT>::edge_descriptor& e, const AddressableView<GraphT>& g) noexcept {
    return child(e, g.mGraph);
}

template<class GraphT>
inline std::pair<
    typename AddressableView<GraphT>::out_edge_iterator,
    typename AddressableView<GraphT>::out_edge_iterator
>
out_edges(typename AddressableView<GraphT>::vertex_descriptor u, const AddressableView<GraphT>& g) {
    return children(u, g.mGraph);
}

template<class GraphT>
inline typename AddressableView<GraphT>::degree_size_type
out_degree(typename AddressableView<GraphT>::vertex_descriptor u, const AddressableView<GraphT>& g) noexcept {
    return num_children(u, g.mGraph);
}

template<class GraphT>
inline std::pair<typename AddressableView<GraphT>::edge_descriptor, bool>
edge(typename AddressableView<GraphT>::vertex_descriptor u,
    typename AddressableView<GraphT>::vertex_descriptor v,
    const AddressableView<GraphT>& g
) noexcept {
    return ownership(u, v, g.mGraph);
}

// BidirectionalGraph
template<class GraphT>
inline std::pair<typename AddressableView<GraphT>::in_edge_iterator, typename AddressableView<GraphT>::in_edge_iterator>
in_edges(typename AddressableView<GraphT>::vertex_descriptor u, const AddressableView<GraphT>& g) noexcept {
    return parents(u, g.mGraph);
}

template<class GraphT>
inline typename AddressableView<GraphT>::degree_size_type
in_degree(typename AddressableView<GraphT>::vertex_descriptor u, const AddressableView<GraphT>& g) noexcept {
    return num_parents(u, g.mGraph);
}

template<class GraphT>
inline typename AddressableView<GraphT>::degree_size_type
degree(typename AddressableView<GraphT>::vertex_descriptor u, const AddressableView<GraphT>& g) noexcept {
    return out_degree(u, g) + in_degree(u, g);
}

// AdjacencyGraph
template<class GraphT>
inline std::pair<
    typename AddressableView<GraphT>::adjacency_iterator,
    typename AddressableView<GraphT>::adjacency_iterator
>
adjacent_vertices(typename AddressableView<GraphT>::vertex_descriptor u, const AddressableView<GraphT>& g) noexcept {
    auto r = out_edges(u, g);
    return std::make_pair(
        typename AddressableView<GraphT>::adjacency_iterator(r.first, &g),
        typename AddressableView<GraphT>::adjacency_iterator(r.second, &g)
    );
}

// VertexListGraph
template<class GraphT>
inline std::pair<
    typename AddressableView<GraphT>::vertex_iterator,
    typename AddressableView<GraphT>::vertex_iterator
>
vertices(const AddressableView<GraphT>& g) noexcept {
    return vertices(g.mGraph);
}

template<class GraphT>
inline typename AddressableView<GraphT>::vertices_size_type
num_vertices(const AddressableView<GraphT>& g) noexcept {
    return num_vertices(g.mGraph);
}

// EdgeListGraph
template<class GraphT>
inline std::pair<
    typename AddressableView<GraphT>::edge_iterator,
    typename AddressableView<GraphT>::edge_iterator
>
edges(const AddressableView<GraphT>& g) noexcept {
    return ownerships(g.mGraph);
}

template<class GraphT>
inline typename AddressableView<GraphT>::edges_size_type
num_edges(const AddressableView<GraphT>& g) noexcept {
    return num_ownerships(g.mGraph);
}

}

namespace boost {

template<class GraphT, class TagT>
struct property_map<Cocos::Impl::AddressableView<GraphT>, TagT> {
    using const_type = typename property_map<GraphT, TagT>::const_type;
    using type = typename property_map<GraphT, TagT>::type;
};

}

namespace Cocos::Impl {

template<class TagT, class GraphT>
typename boost::property_map<AddressableView<GraphT>, TagT>::const_type
get(TagT t, const AddressableView<GraphT>& g) {
    return get(t, g.mGraph);
}

template<class TagT, class GraphT>
typename boost::property_map<AddressableView<GraphT>, TagT>::type
get(TagT t, AddressableView<GraphT>& g) {
    return get(t, g.mGraph);
}

template<class TagT, class GraphT>
[[nodiscard]] inline decltype(auto)
get(TagT tag, const AddressableView<GraphT>& g, typename AddressableView<GraphT>::vertex_descriptor v) noexcept {
    return get(get(tag, g), v);
}

template<class TagT, class GraphT>
[[nodiscard]] inline decltype(auto)
get(TagT tag, AddressableView<GraphT>& g, typename AddressableView<GraphT>::vertex_descriptor v) noexcept {
    return get(get(tag, g), v);
}

template<class TagT, class GraphT, class... Args>
inline void put(TagT tag, AddressableView<GraphT>& g,
    typename AddressableView<GraphT>::vertex_descriptor v,
    Args&&... args
) {
    put(get(tag, g), v, std::forward<Args>(args)...);
}

}
