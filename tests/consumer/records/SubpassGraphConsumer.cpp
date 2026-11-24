#include "FailingResource.h"
#include "cocos/renderer/pipeline/custom/RenderGraphTypes.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <utility>
#include <vector>

namespace {
using cc::render::SubpassGraph;
using Vertex = SubpassGraph::vertex_descriptor;
using Endpoints = std::pair<Vertex, Vertex>;

template<class Range>
std::vector<Endpoints> endpoints(const SubpassGraph& graph, const Range& range) {
    std::vector<Endpoints> result;
    for (const auto edge : range) result.emplace_back(graph.source(edge), graph.target(edge));
    return result;
}

bool checkEdgeAllocationFailure() {
    for (int failAfter : {0, 1}) {
        FailingResource arena;
        SubpassGraph g{&arena};
        const auto u = g.add_vertex("input"), v = g.add_vertex("output");
        arena.allocationsBeforeFailure = failAfter;
        try {
            g.add_edge(u, v);
            return false;
        } catch (const InjectedFailure&) {}
    }
    return true;
}
} // namespace

bool checkSubpassGraph() {
    using namespace cc::render;
    boost::container::pmr::monotonic_buffer_resource arena, other;
    SubpassGraph g{&arena};
    const auto a = g.add_vertex("first"), b = g.add_vertex("second"), c = g.add_vertex("resolve");
    auto& subpass = g.get(Subpass_{}, c);
    subpass.rasterViews["color"].slotName = "output";
    subpass.computeViews["storage"].emplace_back().plane = 1;
    subpass.resolvePairs.emplace_back().target = "resolved";
    ccstd::hash_t subpassKey{};
    ccstd::hash_combine(subpassKey, subpass.rasterViews);
    ccstd::hash_combine(subpassKey, subpass.computeViews);
    ccstd::hash_combine(subpassKey, subpass.resolvePairs);
    if (ccstd::hash<Subpass>{}(subpass) != subpassKey) return false;
    SubpassGraph copy{g, &other};
    const auto& cloned = copy.getSubpass(c);
    if (cloned.get_allocator().resource() != &other || cloned.rasterViews.at("color").get_allocator().resource() != &other ||
        cloned.computeViews.at("storage").front().get_allocator().resource() != &other ||
        cloned.resolvePairs.front().get_allocator().resource() != &other) return false;
    // The engine hashes its name vector first and its Subpass vector second.
    ccstd::pmr::vector<ccstd::pmr::string> names{0U, &other};
    ccstd::pmr::vector<Subpass> subpasses{0U, &other};
    for (auto v : g.vertices()) {
        names.emplace_back(g.getName(v));
        subpasses.emplace_back(g.getSubpass(v));
    }
    ccstd::hash_t key{};
    ccstd::hash_combine(key, names);
    ccstd::hash_combine(key, subpasses);
    const auto firstEdge = g.add_edge(a, c);
    if (!firstEdge.second || !g.add_edge(b, c).second || !g.add_edge(a, c).second || !g.add_edge(c, c).second ||
        g.add_edge(c, g.null_vertex()).second || g.edge(g.null_vertex(), c).second) return false;
    if (g != copy || ccstd::hash<SubpassGraph>{}(g) != key || ccstd::hash<SubpassGraph>{}(copy) != key) return false;
    copy.getSubpass(c).resolvePairs.front().target.clear();
    if (g == copy || subpass.resolvePairs.front().target != "resolved") return false;
    copy = g;
    if (copy != g || copy.in_degree(c) != 4 || copy.get_allocator().resource() != &other) return false;
    copy.getName(a) = "changed";
    if (g == copy || g.degree(c) != 5 || g.out_degree(a) != 2 ||
        endpoints(g, g.in_edges(c)) != std::vector<Endpoints>{{a,c}, {b,c}, {a,c}, {c,c}} ||
        endpoints(g, g.out_edges(a)) != std::vector<Endpoints>{{a,c}, {a,c}}) return false;
    const auto& adjacent = g.adjacent_vertices(a);
    if (adjacent.size() != 2 || adjacent[0] != c || adjacent[1] != c) return false;
    g.remove_edge(firstEdge.first);
    if (g.in_degree(c) != 3 || g.out_degree(a) != 1 || !g.edge(a, c).second) return false;
    g.add_edge(a, c);
    g.remove_edge(a, c);
    g.remove_edge(firstEdge.first);
    g.remove_edge(c, c);
    return endpoints(g, g.in_edges(c)) == std::vector<Endpoints>{{b,c}} && g.out_degree(c) == 0 &&
           !g.edge(a, c).second && copy.in_degree(c) == 4 && checkEdgeAllocationFailure();
}
