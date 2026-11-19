#include "ShiftProbe.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"
#include <boost/container/pmr/global_resource.hpp>
#include <boost/container/pmr/monotonic_buffer_resource.hpp>

#include <initializer_list>
#include <iterator>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {
using cc::render::LayoutGraph;
using Vertex = LayoutGraph::Vertex;

bool hasChildren(const LayoutGraph& g, Vertex parent, std::initializer_list<Vertex> expected) {
    if (g.num_children(parent) != expected.size()) return false;
    auto next = expected.begin();
    for (const auto link : g.child_links(parent)) {
        if (next == expected.end() || link.source != parent || g.child(link) != *next ||
            g.get_parent(*next) != parent) return false;
        ++next;
    }
    return next == expected.end();
}

bool checkHierarchy() {
    boost::container::pmr::monotonic_buffer_resource firstArena, secondArena;
    LayoutGraph graph{&firstArena};
    const auto root = graph.add_vertex("main");
    const auto ui = graph.add_vertex("ui", LayoutGraph::null_vertex());
    const auto shadow = graph.add_vertex("shadow", root);
    const auto cascade = graph.add_vertex("cascade", shadow);
    const auto overlay = graph.add_vertex("overlay", ui);
    const auto color = graph.add_vertex("color", root);
    auto intact = [&](const LayoutGraph& g) {
        const auto none = LayoutGraph::null_vertex();
        return g.num_vertices() == 6 && g.get_parent(root) == none && g.get_parent(ui) == none &&
               g.get_parent(none) == none && hasChildren(g, root, {shadow, color}) &&
               hasChildren(g, ui, {overlay}) && hasChildren(g, shadow, {cascade}) &&
               hasChildren(g, cascade, {}) && hasChildren(g, color, {}) && hasChildren(g, overlay, {}) &&
               g.getName(root) == "main" && g.getName(color) == "color";
    };
    try {
        graph.add_vertex("invalid", graph.num_vertices());
        return false;
    } catch (const std::out_of_range&) {
        if (!intact(graph)) return false;
    }
    LayoutGraph copied{graph, &secondArena};
    LayoutGraph assigned{&firstArena};
    assigned = copied;
    if (!intact(assigned) || !intact(copied)) return false;
    assigned = std::move(copied);
    if (!intact(assigned) || copied.num_vertices() != 0) return false;
    LayoutGraph moved{std::move(assigned), &firstArena};
    return intact(moved) && assigned.num_vertices() == 0 && intact(graph);
}

// Fail each allocation in turn, including allocations after the new adjacency row was appended.
class FailingResource final : public boost::container::pmr::memory_resource {
public:
    int allocationsBeforeFailure{-1};
private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if (allocationsBeforeFailure == 0) throw std::bad_alloc();
        if (allocationsBeforeFailure > 0) --allocationsBeforeFailure;
        return boost::container::pmr::new_delete_resource()->allocate(bytes, alignment);
    }
    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        boost::container::pmr::new_delete_resource()->deallocate(p, bytes, alignment);
    }
    bool do_is_equal(const memory_resource& other) const noexcept override { return this == &other; }
};

bool checkFailedInsertion() {
    const std::string longName(160, 'x');
    for (int failAfter = 0; ; ++failAfter) {
        FailingResource resource;
        LayoutGraph graph{&resource};
        const auto root = graph.add_vertex("root");
        const auto first = graph.add_vertex("first", root);
        resource.allocationsBeforeFailure = failAfter;
        try {
            const auto added = graph.add_vertex(longName, root);
            return failAfter > 0 && added == 2 && hasChildren(graph, root, {first, added});
        } catch (const std::bad_alloc&) {
            if (resource.allocationsBeforeFailure != 0) throw;
            if (graph.num_vertices() != 2 || !hasChildren(graph, root, {first}) || !hasChildren(graph, first, {}) ||
                graph.getName(root) != "root" || graph.getName(first) != "first") return false;
        }
        resource.allocationsBeforeFailure = -1;
        const auto retry = graph.add_vertex(longName, root);
        if (retry != 2 || graph.getName(retry) != longName.c_str() || !hasChildren(graph, root, {first, retry})) return false;
    }
}

bool checkChildLookup() {
    using cc::render::Name_;
    FailingResource resource;
    LayoutGraph graph{&resource};
    const auto& view = std::as_const(graph);
    const auto none = LayoutGraph::null_vertex();
    static_assert(noexcept(view.locate_child(none, "main")));
    if (view.locate_child(none, "main") != none) return false;

    const auto main = graph.add_vertex("main");
    const auto shadow = graph.add_vertex("shadow", main);
    const auto grandchild = graph.add_vertex("lighting", shadow);
    const auto other = graph.add_vertex("lighting");
    const auto otherShadow = graph.add_vertex("shadow", other);
    const auto laterRoot = graph.add_vertex("main");
    const auto laterShadow = graph.add_vertex("shadow", main);
    const auto leaf = graph.add_vertex();
    const std::string_view binaryName{"x\0y", 3};
    const auto binary = graph.add_vertex(binaryName, main);

    if (view.locate_child(none, "main") != main || view.locate_child(none, "lighting") != other ||
        view.locate_child(none, "") != leaf || view.locate_child(none, "shadow") != none ||
        view.locate_child(main, "shadow") != shadow || view.locate_child(other, "shadow") != otherShadow ||
        view.locate_child(shadow, "lighting") != grandchild || view.locate_child(main, "lighting") != none ||
        view.locate_child(leaf, "shadow") != none || view.locate_child(view.num_vertices(), "shadow") != none ||
        view.locate_child(main, binaryName) != binary || view.locate_child(main, "x") != none) return false;

    const std::string newName(160, 'r');
    graph.getName(shadow).assign(newName.data(), newName.size());
    graph.get(Name_{}, main) = "renamed";
    resource.allocationsBeforeFailure = 0;
    return view.locate_child(none, "main") == laterRoot && view.locate_child(none, "renamed") == main &&
           view.locate_child(main, newName) == shadow && view.locate_child(main, "shadow") == laterShadow &&
           view.locate_child(other, newName) == none;
}

bool checkFailedAssignment(bool move) {
    boost::container::pmr::monotonic_buffer_resource arena;
    FailingResource resource;
    LayoutGraph source{&arena};
    const auto root = source.add_vertex("original");
    const auto leaf = source.add_vertex("leaf", root);
    for (int failAfter = 0; ; ++failAfter) {
        resource.allocationsBeforeFailure = -1;
        LayoutGraph target{&resource};
        target.add_vertex("previous");
        resource.allocationsBeforeFailure = failAfter;
        try {
            if (move) target = std::move(source);
            else target = source;
            return hasChildren(target, root, {leaf}) && target.getName(leaf) == "leaf" &&
                   target.getName(leaf).get_allocator().resource() == &resource;
        } catch (const std::bad_alloc&) {
            if (resource.allocationsBeforeFailure != 0) throw;
            if (target.num_vertices() != 1 || target.getName(0) != "previous" || !hasChildren(target, 0, {}) ||
                source.num_vertices() != 2 || source.getName(leaf) != "leaf" || !hasChildren(source, root, {leaf})) return false;
        }
    }
}
} // namespace

bool checkLayoutGraph() {
    if (!checkChildLookup()) return false;
    if (!checkHierarchy() || !checkFailedInsertion() || !checkFailedAssignment(false) || !checkFailedAssignment(true)) return false;
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
