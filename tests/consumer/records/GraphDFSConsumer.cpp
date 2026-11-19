#include "cocos/base/GraphDFS.h"
#include "ShiftProbe.h"
#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using cc::graph::depthFirstSearch;
using cc::graph::depthFirstVisit;
using cc::graph::DfsFrame;
using cc::graph::GraphColor;
using cc::render::LayoutGraph;
using Vertex = LayoutGraph::Vertex;

// The hierarchy cannot represent cycles. This graph also enumerates vertex zero last.
struct Digraph {
    struct Edge { Vertex source, target; };
    std::vector<std::vector<Edge>> rows;
    std::vector<Vertex> order;
    const auto& vertices() const { return order; }
    const auto& out_edges(Vertex v) const { return rows.at(v); }
    Vertex source(Edge e) const { return e.source; }
    Vertex target(Edge e) const { return e.target; }
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
    std::vector<DfsFrame<Digraph>> stack;
    depthFirstSearch(graph, trace, colors, stack, 0);
    trace.write(samples);
    std::fill(colors.begin(), colors.end(), GraphColor::WHITE);
    colors[2] = GraphColor::BLACK;
    depthFirstVisit(graph, 0, trace, colors, stack, [](Vertex v, const Digraph&) { return v == 1; });
    if (colors[3] != GraphColor::WHITE || colors[4] != GraphColor::WHITE) return false;
    trace.write(samples);
    depthFirstSearch(Digraph{}, trace, colors, stack, 0);
    trace.write(samples);

    cc::HierarchyOnly hierarchy{&scratch};
    const auto top = hierarchy.add_vertex();
    const auto leaf = hierarchy.add_vertex(top);
    std::vector<DfsFrame<cc::HierarchyOnly>> hierarchyStack;
    cc::graph::DefaultDfsVisitor quiet;
    colors.assign(2, GraphColor::WHITE);
    depthFirstVisit(hierarchy, top, quiet, colors, hierarchyStack);
    if (colors.at(leaf) != GraphColor::BLACK) return false;

    LayoutGraph forest{&scratch};
    const auto root = forest.add_vertex("first");
    const auto other = forest.add_vertex("second");
    const auto child = forest.add_vertex("child", root);
    forest.add_vertex("sibling", root);
    forest.add_vertex("grandchild", child);
    forest.add_vertex("child", other);
    samples.writeNumber(forest.num_vertices());
    for (const auto v : forest.vertices()) samples.writeNumber(forest.get_parent(v));
    colors.assign(forest.num_vertices(), GraphColor::BLACK);
    ccstd::pmr::vector<DfsFrame<LayoutGraph>> layoutStack{&scratch};
    depthFirstSearch(forest, trace, colors, layoutStack);
    trace.write(samples);

    constexpr Vertex length = 25000;
    samples.writeNumber(length);
    LayoutGraph chain{&scratch};
    auto parent = LayoutGraph::null_vertex();
    for (Vertex v = 0; v != length; ++v) parent = chain.add_vertex("v", parent);
    colors.assign(length, GraphColor::WHITE);
    depthFirstVisit(chain, 0, trace, colors, layoutStack);
    trace.write(samples);
    file.close();
    return !file.fail() && stack.empty() && layoutStack.empty();
}
