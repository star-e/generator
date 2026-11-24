#include "cocos/base/GraphDFS.h"
#include "cocos/base/GraphView.h"
#include "ShiftProbe.h"
#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"
#include "cocos/renderer/pipeline/custom/RenderGraphTypes.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>

#include <algorithm>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
using cc::graph::depthFirstSearch;
using cc::graph::depthFirstVisit;
using cc::graph::GraphColor;
using cc::render::LayoutGraph;
using Vertex = LayoutGraph::vertex_descriptor;

template<class Graph>
constexpr bool checkIteratorTypes() {
    using G = const Graph&;
    static_assert(std::is_same_v<typename Graph::vertex_iterator, decltype(std::declval<G>().vertices().begin())>);
    static_assert(std::is_same_v<typename Graph::out_edge_iterator, decltype(std::declval<G>().out_edges(0).begin())>);
    static_assert(std::is_same_v<typename Graph::in_edge_iterator, decltype(std::declval<G>().in_edges(0).begin())>);
    static_assert(std::is_same_v<typename Graph::adjacency_iterator, decltype(std::declval<G>().adjacent_vertices(0).begin())>);
    static_assert(std::is_same_v<typename Graph::children_iterator, decltype(std::declval<G>().child_links(0).begin())>);
    static_assert(std::is_same_v<typename Graph::parent_iterator, decltype(std::declval<G>().parent_links(0).begin())>);
    static_assert(std::is_same_v<typename Graph::edge_descriptor, typename Graph::out_edge_iterator::value_type>);
    static_assert(std::is_same_v<typename Graph::edge_descriptor, cc::graph::EdgeDescriptor<typename Graph::vertex_descriptor>>);
    static_assert(std::is_same_v<typename Graph::ownership_descriptor, typename Graph::children_iterator::value_type>);
    return true;
}
static_assert(checkIteratorTypes<LayoutGraph>());
static_assert(checkIteratorTypes<cc::render::RenderGraph>());
static_assert(checkIteratorTypes<cc::render::ResourceGraph>());

// The hierarchy cannot represent cycles. This graph also enumerates vertex zero last.
struct Digraph {
    using vertex_descriptor = Vertex;
    using edge_descriptor = cc::graph::EdgeDescriptor<Vertex>;
    using vertex_iterator = std::vector<Vertex>::const_iterator;
    using out_edge_iterator = std::vector<edge_descriptor>::const_iterator;
    std::vector<std::vector<edge_descriptor>> rows;
    std::vector<Vertex> order;
    const auto& vertices() const { return order; }
    const auto& out_edges(Vertex v) const { return rows.at(v); }
    Vertex source(edge_descriptor e) const { return e.source; }
    Vertex target(edge_descriptor e) const { return e.target; }
};

struct Trace {
    explicit Trace(const std::vector<GraphColor>& map) : colors(map) {}

    template<class G> void initializeVertex(Vertex v, const G&) { vertex("init", v, GraphColor::WHITE); }
    template<class G> void startVertex(Vertex v, const G&) { events.push_back("start" + std::to_string(v)); }
    template<class G> void discoverVertex(Vertex v, const G&) { vertex("open", v, GraphColor::GRAY); }
    template<class G> void finishVertex(Vertex v, const G&) { vertex("close", v, GraphColor::BLACK); }
    template<class E, class G> void examineEdge(E e, const G& g) { edge("examine", e, g); }
    template<class E, class G> void treeEdge(E e, const G& g) { edge("tree", e, g); }
    template<class E, class G> void backEdge(E e, const G& g) { edge("back", e, g); }
    template<class E, class G> void forwardOrCrossEdge(E e, const G& g) { edge("forward/cross", e, g); }
    template<class E, class G> void finishEdge(E e, const G& g) { edge("finish", e, g); }

    void write(cc::render::OutputArchive& samples) {
        samples.writeNumber(static_cast<double>(events.size()));
        for (const auto& event : events) samples.writeString(event);
        events.clear();
    }

    void expect(const std::vector<std::string>& expected) {
        if (events != expected) throw std::runtime_error("Unexpected filtered RenderGraph DFS events");
        events.clear();
    }

private:
    void vertex(const char* name, Vertex v, GraphColor expected) {
        if (colors.at(v) != expected) throw std::runtime_error(std::string("Wrong vertex color at ") + name);
        events.push_back(name + std::to_string(v));
    }
    template<class E, class G> void edge(const char* name, E e, const G& g) {
        events.push_back(name + std::to_string(g.source(e)) + ">" + std::to_string(g.target(e)));
    }
    const std::vector<GraphColor>& colors;
    std::vector<std::string> events;
};

struct RenderActions : cc::graph::DefaultDfsVisitor {
    explicit RenderActions(const cc::render::RenderGraph& graph) : graph(graph) {}
    const cc::render::RenderGraph& graph;
    std::vector<std::string> events;

    void record(const char* action, Vertex v) {
        using namespace cc::render;
        graph.visit_object(v,
            [&](const RasterPass&) { events.emplace_back(std::string(action) + "Pass"); },
            [&](const RenderQueue&) { events.emplace_back(std::string(action) + "Queue"); },
            [&](const SceneData&) { events.emplace_back(std::string(action) + "Scene"); },
            [](const auto&) { throw std::runtime_error("Unexpected render action"); });
    }
    template<class G> void discoverVertex(Vertex v, const G&) { record("begin", v); }
    template<class G> void finishVertex(Vertex v, const G&) { record("end", v); }
};

void checkRenderGraphViews(cc::render::OutputArchive& samples, boost::container::pmr::memory_resource* scratch) {
    using namespace cc::render;
    RenderGraph graph{scratch};
    const auto pass = graph.add_vertex(RasterPassTag{}, "pass");
    const auto queue = graph.add_vertex(Queue_{}, "queue", pass);
    const auto scene = graph.add_vertex(Scene_{}, "scene", queue);
    const auto compute = graph.add_vertex(ComputeTag{}, "compute");
    graph.add_edge(pass, compute);

    cc::graph::ReferenceGraphView refs{graph};
    static_assert(std::is_same_v<decltype(refs)::vertex_iterator, RenderGraph::vertex_iterator>);
    static_assert(std::is_same_v<decltype(refs)::out_edge_iterator, RenderGraph::children_iterator>);
    std::vector<GraphColor> colors(refs.num_vertices(), GraphColor::WHITE);
    Trace trace(colors);
    depthFirstVisit(refs, pass, trace, colors, {}, scratch);
    trace.write(samples);

    RenderActions actions{graph};
    std::fill(colors.begin(), colors.end(), GraphColor::WHITE);
    depthFirstVisit(refs, pass, actions, colors, {}, scratch);
    if (actions.events != std::vector<std::string>{"beginPass", "beginQueue", "beginScene", "endScene", "endQueue", "endPass"})
        throw std::runtime_error("Unexpected payload dispatch order");

    const auto alternate = graph.add_vertex(Queue_{}, "alternate", pass);
    graph.remove_reference(queue, scene);
    graph.add_reference(alternate, scene);
    colors.assign(refs.num_vertices(), GraphColor::WHITE);
    depthFirstVisit(refs, pass, trace, colors, {}, scratch);
    trace.write(samples);

    for (const auto v : {pass, scene, compute}) graph.getValid(v) = true;
    cc::graph::FilteredGraphView visible{refs, [&](Vertex v) { return graph.getValid(v); }};
    colors.assign(refs.num_vertices(), GraphColor::WHITE);
    depthFirstVisit(visible, pass, trace, colors, {}, scratch);
    trace.expect({"start0", "open0", "close0"});

    graph.getValid(alternate) = true;
    const auto vertices = visible.vertices();
    static_assert(std::is_same_v<decltype(visible)::vertex_iterator, decltype(vertices.begin())>);
    if (std::vector<Vertex>(vertices.begin(), vertices.end()) != std::vector<Vertex>{pass, scene, compute, alternate})
        throw std::runtime_error("Filtered vertices lost their original IDs or order");
    const auto excludedEdges = visible.out_edges(queue);
    if (excludedEdges.begin() != excludedEdges.end()) throw std::runtime_error("Excluded queue has visible edges");
    depthFirstVisit(visible, queue, trace, colors, {}, scratch);
    trace.expect({});
    depthFirstVisit(visible, pass, trace, colors, {}, scratch);
    trace.expect({"start0", "open0", "examine0>4", "tree0>4", "open4", "examine4>2", "tree4>2", "open2",
                  "close2", "finish4>2", "close4", "finish0>4", "close0"});
    if (colors.at(queue) != GraphColor::WHITE || colors.at(compute) != GraphColor::WHITE)
        throw std::runtime_error("Rooted reference traversal reached an excluded queue or execution dependency");

    depthFirstSearch(visible, trace, colors, queue, scratch);
    trace.expect({"init0", "init2", "init3", "init4", "start0", "open0", "examine0>4", "tree0>4", "open4",
                  "examine4>2", "tree4>2", "open2", "close2", "finish4>2", "close4", "finish0>4", "close0",
                  "start3", "open3", "close3"});

    cc::graph::FilteredGraphView dependencies{graph, [&](Vertex v) { return graph.getValid(v); }};
    colors.assign(graph.num_vertices(), GraphColor::WHITE);
    depthFirstVisit(dependencies, pass, trace, colors, {}, scratch);
    trace.expect({"start0", "open0", "examine0>3", "tree0>3", "open3", "close3", "finish0>3", "close0"});

    for (const auto v : graph.vertices()) graph.getValid(v) = false;
    depthFirstSearch(visible, trace, colors, pass, scratch);
    trace.expect({});
}
} // namespace

bool writeDfsCases(const char* path) {
    boost::container::pmr::monotonic_buffer_resource scratch;
    std::ofstream file(path, std::ios::binary);
    cc::render::BinaryOutputArchive samples(file, &scratch);
    const Digraph graph{{{{0, 1}, {0, 2}}, {{1, 0}, {1, 2}}, {}, {{3, 2}}, {}}, {4, 3, 2, 1, 0}};
    samples.writeNumber(static_cast<double>(graph.rows.size()));
    for (const auto& row : graph.rows) {
        samples.writeNumber(static_cast<double>(row.size()));
        for (const auto e : row) samples.writeNumber(graph.target(e));
    }
    for (const auto v : graph.order) samples.writeNumber(v);

    std::vector<GraphColor> colors(graph.rows.size(), GraphColor::BLACK);
    Trace trace(colors);
    depthFirstSearch(graph, trace, colors, 0);
    trace.write(samples);
    std::fill(colors.begin(), colors.end(), GraphColor::WHITE);
    colors[2] = GraphColor::BLACK;
    depthFirstVisit(graph, 0, trace, colors, [](Vertex v, const Digraph&) { return v == 1; });
    if (colors[3] != GraphColor::WHITE || colors[4] != GraphColor::WHITE) return false;
    trace.write(samples);
    depthFirstSearch(Digraph{}, trace, colors, 0);
    trace.write(samples);

    cc::HierarchyOnly hierarchy{&scratch};
    const auto top = hierarchy.add_vertex();
    const auto leaf = hierarchy.add_vertex(top);
    cc::graph::DefaultDfsVisitor quiet;
    colors.assign(2, GraphColor::WHITE);
    depthFirstVisit(hierarchy, top, quiet, colors);
    if (colors.at(leaf) != GraphColor::BLACK) return false;
    cc::graph::ReferenceGraphView refs{hierarchy};
    cc::graph::FilteredGraphView filtered{refs, [](Vertex) { return true; }};
    std::fill(colors.begin(), colors.end(), GraphColor::WHITE);
    depthFirstVisit(filtered, top, quiet, colors);
    if (colors.at(leaf) != GraphColor::BLACK) return false;

    LayoutGraph forest{&scratch};
    const auto root = forest.add_vertex(cc::render::RenderStage_{}, "first");
    const auto other = forest.add_vertex(cc::render::RenderStage_{}, "second");
    const auto child = forest.add_vertex(cc::render::RenderStage_{}, "child", root);
    forest.add_vertex(cc::render::RenderStage_{}, "sibling", root);
    forest.add_vertex(cc::render::RenderStage_{}, "grandchild", child);
    forest.add_vertex(cc::render::RenderStage_{}, "child", other);
    samples.writeNumber(forest.num_vertices());
    for (const auto v : forest.vertices()) samples.writeNumber(forest.get_parent(v));
    colors.assign(forest.num_vertices(), GraphColor::BLACK);
    depthFirstSearch(forest, trace, colors, std::nullopt, &scratch);
    trace.write(samples);

    constexpr Vertex length = 25000;
    samples.writeNumber(length);
    LayoutGraph chain{&scratch};
    auto parent = LayoutGraph::null_vertex();
    for (Vertex v = 0; v != length; ++v) parent = chain.add_vertex(cc::render::RenderStage_{}, "v", parent);
    colors.assign(length, GraphColor::WHITE);
    depthFirstVisit(chain, 0, trace, colors, {}, &scratch);
    trace.write(samples);
    checkRenderGraphViews(samples, &scratch);
    file.close();
    return !file.fail();
}
