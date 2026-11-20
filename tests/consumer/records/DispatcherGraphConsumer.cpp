#include "cocos/renderer/pipeline/custom/FGDispatcherRecords.h"
#include "FailingResource.h"
#include "IndexedKeys.h"
#include "IndexedKeysSerialization.h"
#include "cocos/renderer/pipeline/custom/BinaryArchive.h"

#include <array>
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace {
using namespace cc::render;

static_assert(std::is_same_v<decltype(std::declval<ResourceAccessGraph&>().getPassID(0)), const RenderGraph::Vertex&>);
static_assert(std::is_same_v<decltype(std::declval<RelationGraph&>().get(DescID_{}, 0)), const ResourceAccessGraph::Vertex&>);

bool checkIndexedArchive() {
    boost::container::pmr::monotonic_buffer_resource arena, other;
    cc::IndexedKeys graph{&arena};
    const auto root = graph.add_vertex(-5, "root");
    const auto child = graph.add_vertex(cc::SignedKey_{}, 0, "child", root);
    try {
        graph.add_vertex(-5, "duplicate", root);
        return false;
    } catch (const std::invalid_argument&) {}
    if (graph.num_vertices() != 2 || graph.contains("duplicate") || graph.num_children(root) != 1) return false;

    std::stringstream bytes;
    BinaryOutputArchive writer(bytes, &arena);
    cc::save(writer, graph);
    cc::IndexedKeys decoded{&other};
    decoded.add_vertex(41, "old");
    BinaryInputArchive reader(bytes, &other);
    cc::load(reader, decoded);
    if (decoded.contains(41) || decoded.contains("old") || decoded.vertex(-5) != root || decoded.vertex("root") != root ||
        decoded.vertex(0) != child || decoded.vertex("child") != child || decoded.get_parent(child) != root) return false;
    cc::IndexedKeys copy{decoded, &arena};
    cc::IndexedKeys moved{std::move(copy), &other};
    return copy.num_vertices() == 0 && !copy.contains(0) && moved.vertex(0) == child && moved.vertex("root") == root;
}

bool checkAllocationRollback() {
#if !defined(_ITERATOR_DEBUG_LEVEL) || _ITERATOR_DEBUG_LEVEL == 0
    for (int budget = 0; budget < 64; ++budget) {
        FailingResource arena;
        RelationGraph g{&arena};
        const auto first = g.add_vertex(40);
        g.add_edge(first, first);
        arena.allocationsBeforeFailure = budget;
        try {
            return g.add_vertex(41) == 1 && g.vertex(41) == 1 && budget > 0;
        } catch (const InjectedFailure&) {
            arena.allocationsBeforeFailure = -1;
            if (g.num_vertices() != 1 || g.contains(41) || g.vertex(40) != first ||
                !g.edge(first, first).second || g.in_degree(first) != 1 ||
                g.add_vertex(41) != 1 || g.vertex(41) != 1) return false;
        }
    }
    return false;
#else
    return true;
#endif
}
} // namespace

bool checkDispatcherGraphs() {
    boost::container::pmr::monotonic_buffer_resource memory;
    ResourceAccessGraph access{&memory};
    RelationGraph relation{&memory};
    access.reserve(6);
    relation.reserve(4);
    const auto start = access.add_vertex(RenderGraph::null_vertex());
    if (access.vertex(RenderGraph::null_vertex()) != start || access.resource() != &memory) return false;
    for (unsigned id : {17U, 33U, 5U, 42U}) access.add_vertex(id);
    const auto pass = access.vertex(33);
    access.add_edge(start, pass);
    access.getRenderPassInfo(pass).resolveCount = 3;
    access.leafPasses.emplace(pass, LeafStatus{});
    try {
        access.emplace_vertex(std::make_tuple(33U), std::tuple<>{}, std::tuple<>{}, std::tuple<>{});
        return false;
    } catch (const std::invalid_argument&) {}
    if (access.getCurrentID() != 5 || access.get(PassID_{}, pass) != 33 ||
        access.getRenderPassInfo(pass).resolveCount != 3 || !access.edge(start, pass).second ||
        access.getPassNode(pass).get_allocator().resource() != &memory || access.leafPasses.count(pass) != 1) return false;
    if (access.add_vertex(8) != 5 || access.contains(99) || access.find(99) != access.null_vertex()) return false;
    try {
        access.vertex(99);
        return false;
    } catch (const std::out_of_range&) {}

    const auto a = relation.add_vertex(access.vertex(42));
    const auto b = relation.add_vertex(start);
    const auto c = relation.add_vertex(pass);
    const auto d = relation.add_vertex(access.vertex(5));
    relation.add_edge(a, b);
    relation.add_edge(b, b);
    relation.add_edge(b, c);
    relation.add_edge(d, b);
    relation.add_edge(a, c);
    relation.add_edge(a, c);
    relation.add_edge(c, d);
    relation.add_edge(d, d);
    relation.remove_vertex(b);
    if (relation.resource() != &memory || relation.getCurrentID() != 3 || relation.num_edges() != 4 ||
        relation.contains(start) || relation.vertex(access.vertex(42)) != 0 ||
        relation.vertex(pass) != 1 || relation.vertex(access.vertex(5)) != 2 ||
        relation.getDescID(1) != pass || relation.getDescID(2) != access.vertex(5)) return false;
    using Counts = std::array<std::array<unsigned, 3>, 3>;
    const Counts expected{{{{0, 2, 0}}, {{0, 0, 1}}, {{0, 0, 1}}}};
    Counts outgoing{}, incoming{}, all{};
    for (auto v : relation.vertices()) {
        for (auto e : relation.out_edges(v)) ++outgoing.at(v).at(relation.target(e));
        for (auto e : relation.in_edges(v)) ++incoming.at(relation.source(e)).at(v);
    }
    for (auto e : relation.edges()) ++all.at(e.source).at(e.target);
    if (all != expected || incoming != expected || outgoing != expected) return false;
    try {
        relation.remove_vertex(relation.null_vertex());
        return false;
    } catch (const std::out_of_range&) {}
    relation.remove_vertex(2);
    relation.remove_vertex(0);
    if (relation.vertex(pass) != 0 || relation.num_edges() != 0) return false;
    relation.remove_vertex(0);
    return relation.num_vertices() == 0 && !relation.contains(pass) &&
           relation.add_vertex(start) == 0 && relation.vertex(start) == 0 && checkAllocationRollback() && checkIndexedArchive();
}
