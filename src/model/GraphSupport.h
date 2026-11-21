#pragma once

#include <generated/TypeGraphCore.h>

#include <cstdint>
#include <iterator>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <variant>
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

} // namespace cc::codegen::graph
