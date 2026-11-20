#pragma once

#include "GraphView.h"

#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>

namespace cc::graph {

enum class GraphColor : std::uint8_t { WHITE, GRAY, BLACK };

struct DefaultDfsVisitor {
    template<class V, class G> void initializeVertex(V, const G&) {}
    template<class V, class G> void startVertex(V, const G&) {}
    template<class V, class G> void discoverVertex(V, const G&) {}
    template<class E, class G> void examineEdge(const E&, const G&) {}
    template<class E, class G> void treeEdge(const E&, const G&) {}
    template<class E, class G> void backEdge(const E&, const G&) {}
    template<class E, class G> void forwardOrCrossEdge(const E&, const G&) {}
    template<class E, class G> void finishEdge(const E&, const G&) {}
    template<class V, class G> void finishVertex(V, const G&) {}
};

// The graph owns its edges. Out-edge iterators must remain valid during a visit.
template<class Graph>
struct DfsFrame {
    using Edge = typename Graph::Edge;
    using Vertex = std::decay_t<decltype(std::declval<const Graph&>().target(std::declval<Edge>()))>;
    using EdgeRange = std::remove_reference_t<decltype(std::declval<const Graph&>().out_edges(std::declval<Vertex>()))>;
    using Iterator = decltype(std::declval<const EdgeRange&>().begin());

    Vertex vertex;
    Iterator next, end;
    std::optional<Edge> incoming;
};

namespace detail {
struct NeverTerminate {
    template<class V, class G> bool operator()(V, const G&) const noexcept { return false; }
};
} // namespace detail

// Colors are indexed by vertex. The caller supplies a stack of DfsFrame<Graph>,
// for example a vector using the caller's scratch allocator.
template<class Graph, class Visitor, class Colors, class Stack, class Terminator = detail::NeverTerminate>
void depthFirstVisit(const Graph& graph, typename DfsFrame<Graph>::Vertex root, Visitor& visitor,
                     Colors& colors, Stack& stack, Terminator terminate = {}) {
    using Vertex = typename DfsFrame<Graph>::Vertex;
    using Edge = typename Graph::Edge;
    const auto finish = [&](Vertex v, const std::optional<Edge>& incoming) {
        colors[v] = GraphColor::BLACK;
        visitor.finishVertex(v, graph);
        if (incoming) visitor.finishEdge(*incoming, graph);
    };
    const auto discover = [&](Vertex v, std::optional<Edge> incoming) {
        colors[v] = GraphColor::GRAY;
        visitor.discoverVertex(v, graph);
        if (terminate(v, graph)) {
            finish(v, incoming);
        } else {
            const auto& edges = graph.out_edges(v);
            stack.push_back({v, edges.begin(), edges.end(), std::move(incoming)});
        }
    };

    stack.clear();
    if (!detail::acceptsVertex(graph, root)) return;
    visitor.startVertex(root, graph);
    discover(root, std::nullopt);
    while (!stack.empty()) {
        auto& frame = stack.back();
        if (frame.next == frame.end) {
            finish(frame.vertex, frame.incoming);
            stack.pop_back();
            continue;
        }

        const Edge edge = *frame.next;
        ++frame.next;
        visitor.examineEdge(edge, graph);
        const Vertex target = graph.target(edge);
        const auto color = colors[target];
        if (color == GraphColor::WHITE) {
            visitor.treeEdge(edge, graph);
            discover(target, edge);
        } else {
            if (color == GraphColor::GRAY) visitor.backEdge(edge, graph);
            else visitor.forwardOrCrossEdge(edge, graph);
            visitor.finishEdge(edge, graph);
        }
    }
}

// With a starting vertex, visit its tree first, then the remaining white roots
// in vertex enumeration order. An empty graph produces no visitor events.
template<class Graph, class Visitor, class Colors, class Stack>
void depthFirstSearch(const Graph& graph, Visitor& visitor, Colors& colors, Stack& stack,
                      std::optional<typename DfsFrame<Graph>::Vertex> start = std::nullopt) {
    stack.clear();
    const auto& vertices = graph.vertices();
    if (vertices.begin() == vertices.end()) return;
    for (const auto v : vertices) {
        colors[v] = GraphColor::WHITE;
        visitor.initializeVertex(v, graph);
    }
    if (start) depthFirstVisit(graph, *start, visitor, colors, stack);
    for (const auto v : graph.vertices()) {
        if (colors[v] == GraphColor::WHITE) depthFirstVisit(graph, v, visitor, colors, stack);
    }
}

} // namespace cc::graph
