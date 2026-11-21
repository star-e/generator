#pragma once

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

template<class Vertex>
struct EdgeDescriptor {
    Vertex source;
    Vertex target;
    bool operator==(const EdgeDescriptor&) const = default;
};

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
    using Edge = EdgeDescriptor<Vertex>;
    using VertexIterator = IntegerIterator<Vertex>;
    using OutEdgeIterator = graph::OutEdgeIterator<Edge, typename std::vector<Vertex>::const_iterator>;
    using InEdgeIterator = graph::InEdgeIterator<Edge, typename std::vector<Vertex>::const_iterator>;
    struct VertexType {
        std::vector<Vertex> mOutEdges;
        std::vector<Vertex> mInEdges;
    };
    std::vector<VertexType> mVertices;

    static constexpr Vertex null_vertex() noexcept { return (std::numeric_limits<Vertex>::max)(); }
    Vertex num_vertices() const noexcept { return static_cast<Vertex>(mVertices.size()); }
    IteratorPair<VertexIterator> vertices() const noexcept { return {VertexIterator(0), VertexIterator(num_vertices())}; }
    void reserve(Vertex capacity) { mVertices.reserve(capacity); }
    Vertex source(Edge e) const noexcept { return e.source; }
    Vertex target(Edge e) const noexcept { return e.target; }
    IteratorPair<OutEdgeIterator> out_edges(Vertex v) const noexcept {
        const auto& edges = mVertices[v].mOutEdges;
        return {OutEdgeIterator(v, edges.begin()), OutEdgeIterator(v, edges.end())};
    }
    IteratorPair<InEdgeIterator> in_edges(Vertex v) const noexcept {
        const auto& edges = mVertices[v].mInEdges;
        return {InEdgeIterator(v, edges.begin()), InEdgeIterator(v, edges.end())};
    }
    Vertex out_degree(Vertex v) const noexcept { return static_cast<Vertex>(mVertices[v].mOutEdges.size()); }
    Vertex in_degree(Vertex v) const noexcept { return static_cast<Vertex>(mVertices[v].mInEdges.size()); }
    Vertex degree(Vertex v) const noexcept { return in_degree(v) + out_degree(v); }
    std::pair<Edge, bool> edge(Vertex u, Vertex v) const noexcept {
        for (auto e : out_edges(u)) if (e.target == v) return {e, true};
        return {{null_vertex(), null_vertex()}, false};
    }
};

} // namespace cc::codegen::graph
