#include "ShiftProbe.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>

#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

bool checkLayoutGraph() {
    using cc::render::LayoutGraph;
    using cc::render::Name_;
    using Name = ccstd::pmr::string;
    static_assert(std::is_same_v<LayoutGraph::Vertex, uint32_t>);
    static_assert(std::is_same_v<decltype(std::declval<LayoutGraph&>().getName(0)), Name&>);
    static_assert(std::is_same_v<decltype(std::declval<LayoutGraph&>().get(Name_{}, 0)), Name&>);
    static_assert(std::is_same_v<decltype(std::declval<const LayoutGraph&>().getName(0)), const Name&>);
    static_assert(std::is_same_v<decltype(std::declval<const LayoutGraph&>().get(Name_{}, 0)), const Name&>);

    boost::container::pmr::monotonic_buffer_resource arena;
    LayoutGraph graph{&arena};
    const auto& view = std::as_const(graph);
    if (view.num_vertices() != 0 || view.vertices().begin() != view.vertices().end()) return false;
    const std::string longName(160, 'a');
    const auto first = graph.add_vertex(longName);
    const auto second = graph.add_vertex("lighting");
    if (first != 0 || second != 1 || view.num_vertices() != 2) return false;
    if (view.getName(first) != longName.c_str() || view.get(Name_{}, second) != "lighting") return false;
    const auto vertices = view.vertices();
    if (std::distance(vertices.begin(), vertices.end()) != 2) return false;
    LayoutGraph::Vertex expected{};
    for (const auto v : vertices) {
        if (v != expected++ || &view.get(Name_{}, v) != &view.getName(v) ||
            view.getName(v).get_allocator().resource() != &arena) return false;
    }
    graph.get(Name_{}, second).assign(200, 'b');
    if (view.getName(second).size() != 200 || view.getName(second).back() != 'b') return false;
    boost::container::pmr::monotonic_buffer_resource other;
    LayoutGraph copied{graph, &other};
    if (copied.getName(first) != longName.c_str() || copied.getName(first).get_allocator().resource() != &other) return false;
    LayoutGraph moved{std::move(copied), &arena};
    if (moved.num_vertices() != 2 || moved.getName(first).get_allocator().resource() != &arena) return false;
    graph.getName(first) = "renamed";
    if (view.get(Name_{}, first) != "renamed" || graph.get_allocator().resource() != &arena) return false;

    // Fill a second graph using the same tag and a smaller descriptor, then verify failed insertion is atomic.
    static_assert(std::is_same_v<cc::TinyNames::Index, uint8_t>);
    cc::TinyNames tiny{&arena};
    constexpr auto limit = (std::numeric_limits<cc::TinyNames::Index>::max)();
    for (unsigned i = 0; i < limit; ++i) {
        if (tiny.add_vertex("entry") != i) return false;
    }
    try {
        tiny.add_vertex();
        return false;
    } catch (const std::length_error&) {
        return tiny.num_vertices() == limit && tiny.get(Name_{}, limit - 1) == "entry";
    }
}
