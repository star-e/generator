#pragma once

#include "GraphDescriptor.h"

#include <iterator>
#include <limits>
#include <ranges>
#include <utility>
#include <vector>

namespace cc::codegen::graph {

template<class Vertex>
using IntegerIterator = std::ranges::iterator_t<std::ranges::iota_view<Vertex, Vertex>>;

template<class Iterator>
using IteratorPair = std::ranges::subrange<Iterator>;

using ::cc::graph::EdgeDescriptor;

// Each adjacency row stores just the vertex at the other end of the edge.
template<class Edge, class Iterator, bool Incoming>
class IncidentIterator {
public:
    using value_type = Edge;
    using difference_type = std::iter_difference_t<Iterator>;
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using Vertex = std::iter_value_t<Iterator>;

    IncidentIterator() = default;
    IncidentIterator(Vertex vertex, Iterator current) : vertex_(vertex), current_(current) {}
    Edge operator*() const {
        if constexpr (Incoming) return {*current_, vertex_};
        return {vertex_, *current_};
    }
    IncidentIterator& operator++() { ++current_; return *this; }
    IncidentIterator operator++(int) {
        auto previous = *this;
        ++*this;
        return previous;
    }
    bool operator==(const IncidentIterator&) const = default;

private:
    Vertex vertex_{};
    Iterator current_{};
};

template<class Edge, class Iterator>
using OutEdgeIterator = IncidentIterator<Edge, Iterator, false>;

template<class Edge, class Iterator>
using InEdgeIterator = IncidentIterator<Edge, Iterator, true>;

template<class Vertex>
struct BidirectionalStorage {
    using vertex_descriptor = Vertex;
    using adjacency_iterator = typename std::vector<Vertex>::const_iterator;
    using edge_descriptor = EdgeDescriptor<Vertex>;
    using vertex_iterator = IntegerIterator<Vertex>;
    using out_edge_iterator = graph::OutEdgeIterator<edge_descriptor, typename std::vector<Vertex>::const_iterator>;
    using in_edge_iterator = graph::InEdgeIterator<edge_descriptor, typename std::vector<Vertex>::const_iterator>;
    struct VertexType {
        std::vector<Vertex> mOutEdges;
        std::vector<Vertex> mInEdges;
    };
    std::vector<VertexType> mVertices;

    static constexpr Vertex null_vertex() noexcept { return (std::numeric_limits<Vertex>::max)(); }
    Vertex num_vertices() const noexcept { return static_cast<Vertex>(mVertices.size()); }
    IteratorPair<vertex_iterator> vertices() const noexcept { return {vertex_iterator(0), vertex_iterator(num_vertices())}; }
    void reserve(Vertex capacity) { mVertices.reserve(capacity); }
    Vertex source(edge_descriptor e) const noexcept { return e.source; }
    Vertex target(edge_descriptor e) const noexcept { return e.target; }
    IteratorPair<out_edge_iterator> out_edges(Vertex v) const noexcept {
        const auto& edges = mVertices[v].mOutEdges;
        return {out_edge_iterator(v, edges.begin()), out_edge_iterator(v, edges.end())};
    }
    IteratorPair<in_edge_iterator> in_edges(Vertex v) const noexcept {
        const auto& edges = mVertices[v].mInEdges;
        return {in_edge_iterator(v, edges.begin()), in_edge_iterator(v, edges.end())};
    }
    const std::vector<Vertex>& adjacent_vertices(Vertex v) const noexcept { return mVertices[v].mOutEdges; }
    Vertex out_degree(Vertex v) const noexcept { return static_cast<Vertex>(mVertices[v].mOutEdges.size()); }
    Vertex in_degree(Vertex v) const noexcept { return static_cast<Vertex>(mVertices[v].mInEdges.size()); }
    Vertex degree(Vertex v) const noexcept { return in_degree(v) + out_degree(v); }
    std::pair<edge_descriptor, bool> edge(Vertex u, Vertex v) const noexcept {
        for (auto e : out_edges(u)) if (e.target == v) return {e, true};
        return {{null_vertex(), null_vertex()}, false};
    }
};

} // namespace cc::codegen::graph
