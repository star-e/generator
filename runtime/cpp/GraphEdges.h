#pragma once

#include <cstddef>
#include <iterator>

namespace cc::detail {

template<class Vertex>
struct DirectedEdge {
    Vertex source, target;
};

// A single row yields incident edges. All outgoing rows yield the graph's edges in vertex order.
template<bool Incoming, class Rows>
class DirectedEdgeRange {
    using Vertex = typename Rows::value_type::value_type;

public:
    class Iterator {
    public:
        using value_type = DirectedEdge<Vertex>;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;
        using pointer = void;
        using reference = value_type;

        Iterator() = default;
        Iterator(const Rows& rows, std::size_t first, std::size_t last) noexcept
        : rows_(&rows), row_(first), last_(last) { skipEmpty(); }

        value_type operator*() const noexcept {
            const auto vertex = static_cast<Vertex>(row_);
            const auto neighbor = (*rows_)[row_][offset_];
            if constexpr (Incoming) return {neighbor, vertex};
            return {vertex, neighbor};
        }
        Iterator& operator++() noexcept { ++offset_; skipEmpty(); return *this; }
        Iterator operator++(int) noexcept { auto previous = *this; ++*this; return previous; }
        bool operator==(const Iterator& rhs) const noexcept {
            return rows_ == rhs.rows_ && row_ == rhs.row_ && offset_ == rhs.offset_;
        }
        bool operator!=(const Iterator& rhs) const noexcept { return !(*this == rhs); }

    private:
        void skipEmpty() noexcept {
            while (row_ < last_ && offset_ == (*rows_)[row_].size()) {
                ++row_;
                offset_ = 0;
            }
        }
        const Rows* rows_{};
        std::size_t row_{}, last_{}, offset_{};
    };

    DirectedEdgeRange(const Rows& rows, std::size_t first, std::size_t last) noexcept
    : rows_(rows), first_(first), last_(last) {}
    Iterator begin() const noexcept { return {rows_, first_, last_}; }
    Iterator end() const noexcept { return {rows_, last_, last_}; }

private:
    const Rows& rows_;
    std::size_t first_, last_;
};

} // namespace cc::detail
