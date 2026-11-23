#pragma once

#include <cstddef>
#include <iterator>
#include <limits>
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

template<class Vertex>
inline constexpr Vertex nullVertex = (std::numeric_limits<Vertex>::max)();

template<class Vertex>
struct VertexLinks {
    Vertex parent{nullVertex<Vertex>};
    Vertex firstChild{nullVertex<Vertex>};
    Vertex lastChild{nullVertex<Vertex>};
    Vertex nextSibling{nullVertex<Vertex>};
    Vertex childCount{};
};

// Children are linked in sibling order. Adjacency traversal yields just the child descriptor.
template<class Link, class Rows, bool Adjacent>
class ChildIterator {
    using Vertex = decltype(Link::target);

public:
    using value_type = std::conditional_t<Adjacent, Vertex, Link>;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using pointer = void;
    using reference = value_type;

    ChildIterator() = default;
    ChildIterator(const Rows& rows, Vertex v) noexcept : rows_(&rows), current_(v) {}

    value_type operator*() const noexcept {
        if constexpr (Adjacent) {
            return current_;
        } else {
            return {(*rows_)[current_].parent, current_};
        }
    }
    ChildIterator& operator++() noexcept {
        current_ = (*rows_)[current_].nextSibling;
        return *this;
    }
    ChildIterator operator++(int) noexcept {
        auto previous = *this;
        ++*this;
        return previous;
    }
    bool operator==(const ChildIterator& rhs) const noexcept {
        return rows_ == rhs.rows_ && current_ == rhs.current_;
    }
    bool operator!=(const ChildIterator& rhs) const noexcept { return !(*this == rhs); }

private:
    const Rows* rows_{};
    Vertex current_{nullVertex<Vertex>};
};

template<class LinkType>
struct ParentLinkIterator {
    using Link = LinkType;
    using Vertex = decltype(Link::source);
    using value_type = Link;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using pointer = void;
    using reference = value_type;

    Link link{nullVertex<Vertex>, nullVertex<Vertex>};

    Link operator*() const noexcept { return link; }
    ParentLinkIterator& operator++() noexcept {
        link.source = nullVertex<Vertex>;
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
template<class Link, class Rows>
class EdgeIterator {
    using Vertex = decltype(Link::source);

public:
    using value_type = Link;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using pointer = void;
    using reference = value_type;

    EdgeIterator() = default;
    EdgeIterator(const Rows& rows, Vertex first) noexcept : rows_(&rows), source_(first) { seek(); }

    Link operator*() const noexcept { return {source_, target_}; }
    EdgeIterator& operator++() noexcept {
        target_ = (*rows_)[target_].nextSibling;
        if (target_ == nullVertex<Vertex>) {
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
        return rows_ == rhs.rows_ && source_ == rhs.source_ && target_ == rhs.target_;
    }
    bool operator!=(const EdgeIterator& rhs) const noexcept { return !(*this == rhs); }

private:
    void seek() noexcept {
        const auto& rows = *rows_;
        while (source_ < rows.size()) {
            target_ = rows[source_].firstChild;
            if (target_ != nullVertex<Vertex>) break;
            ++source_;
        }
    }

    const Rows* rows_{};
    Vertex source_{}, target_{nullVertex<Vertex>};
};

} // namespace cc::graph
