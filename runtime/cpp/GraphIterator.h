#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace cc::graph {

template<class Iterator>
struct IteratorRange {
    Iterator first, last;

    Iterator begin() const noexcept { return first; }
    Iterator end() const noexcept { return last; }
};

template<class Vertex>
class VertexIterator {
public:
    using value_type = Vertex;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using pointer = void;
    using reference = value_type;

    VertexIterator() = default;
    explicit VertexIterator(Vertex value) noexcept : current_(value) {}

    Vertex operator*() const noexcept { return current_; }
    VertexIterator& operator++() noexcept {
        ++current_;
        return *this;
    }
    VertexIterator operator++(int) noexcept {
        auto previous = *this;
        ++*this;
        return previous;
    }
    bool operator==(VertexIterator rhs) const noexcept { return current_ == rhs.current_; }
    bool operator!=(VertexIterator rhs) const noexcept { return !(*this == rhs); }

private:
    Vertex current_{};
};

template<class Vertex>
struct VertexRange {
    VertexIterator<Vertex> last;

    VertexIterator<Vertex> begin() const noexcept { return {}; }
    VertexIterator<Vertex> end() const noexcept { return last; }
};

// Children are linked in sibling order. Adjacency traversal yields just the child descriptor.
template<class Graph, class Access, bool Adjacent>
class ChildIterator {
    using Link = typename Graph::Link;
    using Vertex = decltype(Link::target);

public:
    using value_type = std::conditional_t<Adjacent, Vertex, Link>;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using pointer = void;
    using reference = value_type;

    ChildIterator() = default;
    ChildIterator(const Graph* graph, Vertex v) noexcept : graph_(graph), current_(v) {}

    value_type operator*() const noexcept {
        if constexpr (Adjacent) {
            return current_;
        } else {
            return {graph_->get_parent(current_), current_};
        }
    }
    ChildIterator& operator++() noexcept {
        current_ = Access::rows(*graph_)[current_].nextSibling;
        return *this;
    }
    ChildIterator operator++(int) noexcept {
        auto previous = *this;
        ++*this;
        return previous;
    }
    bool operator==(const ChildIterator& rhs) const noexcept {
        return graph_ == rhs.graph_ && current_ == rhs.current_;
    }
    bool operator!=(const ChildIterator& rhs) const noexcept { return !(*this == rhs); }

private:
    const Graph* graph_{};
    Vertex current_{Graph::null_vertex()};
};

template<class Graph>
struct ParentLinkIterator {
    using Link = typename Graph::Link;
    using value_type = Link;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using pointer = void;
    using reference = value_type;

    Link link{Graph::null_vertex(), Graph::null_vertex()};

    Link operator*() const noexcept { return link; }
    ParentLinkIterator& operator++() noexcept {
        link.source = Graph::null_vertex();
        return *this;
    }
    ParentLinkIterator operator++(int) noexcept {
        auto previous = *this;
        ++*this;
        return previous;
    }
    bool operator==(const ParentLinkIterator& rhs) const noexcept {
        return link.source == rhs.link.source && link.target == rhs.link.target;
    }
    bool operator!=(const ParentLinkIterator& rhs) const noexcept { return !(*this == rhs); }
};

// Visit sources in vertex order, and each source's outgoing edges in sibling order.
template<class Graph, class Access>
class EdgeIterator {
    using Link = typename Graph::Link;
    using Vertex = decltype(Link::source);

public:
    using value_type = Link;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using pointer = void;
    using reference = value_type;

    EdgeIterator() = default;
    EdgeIterator(const Graph* graph, Vertex first) noexcept : graph_(graph), source_(first) { seek(); }

    Link operator*() const noexcept { return {source_, target_}; }
    EdgeIterator& operator++() noexcept {
        target_ = Access::rows(*graph_)[target_].nextSibling;
        if (target_ == Graph::null_vertex()) {
            ++source_;
            seek();
        }
        return *this;
    }
    EdgeIterator operator++(int) noexcept {
        auto previous = *this;
        ++*this;
        return previous;
    }
    bool operator==(const EdgeIterator& rhs) const noexcept {
        return graph_ == rhs.graph_ && source_ == rhs.source_ && target_ == rhs.target_;
    }
    bool operator!=(const EdgeIterator& rhs) const noexcept { return !(*this == rhs); }

private:
    void seek() noexcept {
        const auto& rows = Access::rows(*graph_);
        while (source_ < rows.size()) {
            target_ = rows[source_].firstChild;
            if (target_ != Graph::null_vertex()) break;
            ++source_;
        }
    }

    const Graph* graph_{};
    Vertex source_{}, target_{Graph::null_vertex()};
};

} // namespace cc::graph
