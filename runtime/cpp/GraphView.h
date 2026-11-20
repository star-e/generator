#pragma once

#include <iterator>
#include <type_traits>
#include <utility>

namespace cc::graph {

template<class Graph, class Predicate>
class FilteredGraphView;

namespace detail {
// Plain graphs accept every existing vertex. A view can exclude a root supplied by its caller.
template<class Graph, class Vertex>
constexpr bool acceptsVertex(const Graph&, Vertex) noexcept { return true; }

template<class Graph, class Predicate, class Vertex>
bool acceptsVertex(const FilteredGraphView<Graph, Predicate>& view, Vertex v) { return view.contains_vertex(v); }
} // namespace detail

// Both views borrow the graph and its descriptors. Give the base graph (or view) a name
// so that it outlives the view and all iterators used during traversal.
template<class Graph>
class ReferenceGraphView {
public:
    using Edge = typename Graph::Link;
    using Vertex = std::decay_t<decltype(std::declval<const Graph&>().child(std::declval<Edge>()))>;

    explicit ReferenceGraphView(const Graph& graph) noexcept : graph_(graph) {}
    ReferenceGraphView(const Graph&&) = delete;

    decltype(auto) vertices() const { return graph_.vertices(); }
    auto num_vertices() const { return graph_.num_vertices(); }
    decltype(auto) out_edges(Vertex v) const { return graph_.child_links(v); }
    auto out_degree(Vertex v) const { return graph_.num_children(v); }
    Vertex source(Edge link) const { return graph_.parent(link); }
    Vertex target(Edge link) const { return graph_.child(link); }

private:
    const Graph& graph_;
};

template<class Graph, class Predicate>
class FilteredGraphView {
public:
    using Edge = typename Graph::Edge;
    using Vertex = std::decay_t<decltype(std::declval<const Graph&>().target(std::declval<Edge>()))>;

    FilteredGraphView(const Graph& graph, Predicate keep) : graph_(graph), keep_(std::move(keep)) {}
    FilteredGraphView(const Graph&&, Predicate) = delete;

    // v must be a vertex of the underlying graph. Predicates see original vertex IDs.
    bool contains_vertex(Vertex v) const { return detail::acceptsVertex(graph_, v) && keep_(v); }
    Vertex source(Edge e) const { return graph_.source(e); }
    Vertex target(Edge e) const { return graph_.target(e); }

    auto vertices() const {
        const auto& vertices = graph_.vertices();
        return Range<decltype(vertices.begin()), false>{*this, vertices.begin(), vertices.end()};
    }
    auto out_edges(Vertex v) const {
        const auto& edges = graph_.out_edges(v);
        return Range<decltype(edges.begin()), true>{*this, contains_vertex(v) ? edges.begin() : edges.end(), edges.end()};
    }

private:
    // Iterators borrow the view and underlying graph, so DFS can retain them after the range expires.
    template<class BaseIterator, bool Edges>
    class Range {
    public:
        class Iterator {
        public:
            using iterator_category = std::input_iterator_tag;
            using difference_type = typename std::iterator_traits<BaseIterator>::difference_type;
            using value_type = typename std::iterator_traits<BaseIterator>::value_type;
            using pointer = void;
            using reference = decltype(*std::declval<const BaseIterator&>());

            Iterator() = default;
            Iterator(const FilteredGraphView& view, BaseIterator current, BaseIterator last)
            : view_(&view), current_(current), last_(last) { skipExcluded(); }

            reference operator*() const { return *current_; }
            Iterator& operator++() { ++current_; skipExcluded(); return *this; }
            Iterator operator++(int) { auto previous = *this; ++*this; return previous; }
            bool operator==(const Iterator& other) const { return current_ == other.current_; }
            bool operator!=(const Iterator& other) const { return !(*this == other); }

        private:
            void skipExcluded() {
                while (current_ != last_) {
                    if constexpr (Edges) {
                        if (view_->contains_vertex(view_->target(*current_))) break;
                    } else {
                        if (view_->contains_vertex(*current_)) break;
                    }
                    ++current_;
                }
            }
            const FilteredGraphView* view_{};
            BaseIterator current_{}, last_{};
        };

        Range(const FilteredGraphView& view, BaseIterator first, BaseIterator last)
        : first_(view, first, last), last_(view, last, last) {}
        Iterator begin() const { return first_; }
        Iterator end() const { return last_; }

    private:
        Iterator first_, last_;
    };

    const Graph& graph_;
    Predicate keep_;
};

} // namespace cc::graph
