#include "FailingResource.h"
#include "ShiftProbe.h"
#include "ShiftSerialization.h"
#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"
#include <boost/container/pmr/global_resource.hpp>
#include <boost/container/pmr/monotonic_buffer_resource.hpp>

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace {
using cc::render::LayoutGraph;
using cc::render::DescriptorDB;
using cc::render::Descriptors_;
using cc::render::RenderPassType;
using cc::render::RenderPhase;
using cc::render::RenderPhase_;
using cc::render::RenderStage_;
using Vertex = LayoutGraph::Vertex;

bool usesResource(const DescriptorDB& db, const LayoutGraph& g) {
    const auto* resource = g.get_allocator().resource();
    return db.get_allocator().resource() == resource && db.blocks.get_allocator().resource() == resource &&
           db.groupBlocks.get_allocator().resource() == resource;
}

bool hasChildren(const LayoutGraph& g, Vertex parent, std::initializer_list<Vertex> expected) {
    if (g.num_children(parent) != expected.size() || g.out_degree(parent) != expected.size()) return false;
    const auto outgoing = g.out_edges(parent);
    auto edge = outgoing.begin();
    auto next = expected.begin();
    for (const auto link : g.child_links(parent)) {
        if (next == expected.end() || edge == outgoing.end() || link.source != parent || g.child(link) != *next ||
            g.source(*edge) != parent || g.target(*edge) != *next ||
            g.get_parent(*next) != parent) return false;
        const auto [found, exists] = g.edge(parent, *next);
        if (!exists || g.source(found) != parent || g.target(found) != *next || g.edge(*next, parent).second) return false;
        ++edge;
        ++next;
    }
    return next == expected.end() && edge == outgoing.end();
}

bool checkHierarchy() {
    boost::container::pmr::monotonic_buffer_resource firstArena, secondArena;
    LayoutGraph graph{&firstArena};
    const auto root = graph.add_vertex("main");
    graph.getDescriptors(root).blocks[{}].count = 7;
    const auto ui = graph.add_vertex("ui", LayoutGraph::null_vertex());
    graph.get(Descriptors_{}, ui).groupBlocks[{}].capacity = 11;
    const auto shadow = graph.add_vertex(RenderPhase_{}, "shadow", root);
    constexpr std::string_view shader{"a shader name long enough to allocate its own storage"};
    graph.value(RenderPhase_{}, shadow).shaders.emplace(shader);
    const auto cascade = graph.add_vertex(RenderStage_{}, "cascade", shadow);
    graph.value(RenderStage_{}, cascade) = RenderPassType::RENDER_SUBPASS;
    const auto overlay = graph.add_vertex(RenderPhase_{}, "overlay", ui);
    const auto color = graph.add_vertex(RenderStage_{}, "color", root);
    const auto none = LayoutGraph::null_vertex();
    for (const auto& endpoints : {std::pair{root, root}, std::pair{root, cascade}, std::pair{none, root},
                                  std::pair{root, none}, std::pair{root, graph.num_vertices()}}) {
        const auto [missing, exists] = graph.edge(endpoints.first, endpoints.second);
        if (exists || graph.source(missing) != none || graph.target(missing) != none) return false;
    }
    static_assert(std::is_same_v<decltype(graph.value(RenderPhase_{}, shadow)), RenderPhase&>);
    static_assert(std::is_same_v<decltype(std::as_const(graph).value(RenderStage_{}, root)), const RenderPassType&>);
    static_assert(noexcept(graph.holds<RenderStage_>(root)));
    auto intact = [&](const LayoutGraph& g) {
        const auto none = LayoutGraph::null_vertex();
        const auto& shaders = g.value(RenderPhase_{}, shadow).shaders;
        const auto item = shaders.find(shader);
        return g.num_vertices() == 6 && g.get_parent(root) == none && g.get_parent(ui) == none &&
               g.kind(root) == LayoutGraph::Kind::RenderStage && g.kind(shadow) == LayoutGraph::Kind::RenderPhase &&
               g.value(RenderStage_{}, root) == RenderPassType::SINGLE_RENDER_PASS &&
               g.value(RenderStage_{}, cascade) == RenderPassType::RENDER_SUBPASS &&
               g.holds<RenderPhase_>(overlay) && !g.holds<RenderStage_>(overlay) && !g.holds<RenderPhase_>(none) &&
               !g.holds<RenderPhase_>(g.num_vertices()) && g.value(RenderPhase_{}, overlay).shaders.empty() &&
               g.locate("/main/shadow/cascade") == cascade && item != shaders.end() &&
               item->get_allocator().resource() == g.get_allocator().resource() &&
               shaders.get_allocator().resource() == g.get_allocator().resource() &&
               g.get_parent(none) == none && hasChildren(g, root, {shadow, color}) &&
               hasChildren(g, ui, {overlay}) && hasChildren(g, shadow, {cascade}) &&
               hasChildren(g, cascade, {}) && hasChildren(g, color, {}) && hasChildren(g, overlay, {}) &&
               g.getName(root) == "main" && g.getName(color) == "color" &&
               g.get(Descriptors_{}, root).blocks.at({}).count == 7 && g.getDescriptors(root).groupBlocks.empty() &&
               g.getDescriptors(ui).groupBlocks.at({}).capacity == 11 && g.getDescriptors(ui).blocks.empty() &&
               g.getDescriptors(color).blocks.empty() && usesResource(g.getDescriptors(root), g) &&
               usesResource(g.getDescriptors(ui), g);
    };
    try {
        graph.value(RenderPhase_{}, root);
        return false;
    } catch (const std::bad_variant_access&) {}
    try {
        graph.add_vertex("invalid", graph.num_vertices());
        return false;
    } catch (const std::out_of_range&) {
        if (!intact(graph)) return false;
    }
    LayoutGraph copied{graph, &secondArena};
    copied.value(RenderPhase_{}, shadow).shaders.clear();
    if (graph.value(RenderPhase_{}, shadow).shaders.size() != 1) return false;
    copied.value(RenderPhase_{}, shadow).shaders.emplace(shader);
    copied.getDescriptors(root).blocks.at({}).count = 13;
    if (graph.getDescriptors(root).blocks.at({}).count != 7) return false;
    copied.getDescriptors(root).blocks.at({}).count = 7;
    LayoutGraph assigned{&firstArena};
    assigned = copied;
    if (!intact(assigned) || !intact(copied)) return false;
    assigned = std::move(copied);
    if (!intact(assigned) || copied.num_vertices() != 0) return false;
    LayoutGraph moved{std::move(assigned), &firstArena};
    return intact(moved) && assigned.num_vertices() == 0 && intact(graph);
}

bool checkFailedInsertion() {
    const std::string longName(160, 'x');
    for (int failAfter = 0; ; ++failAfter) {
        FailingResource resource;
        LayoutGraph graph{&resource};
        const auto root = graph.add_vertex("root");
        const auto first = graph.add_vertex(RenderPhase_{}, "first", root);
        graph.value(RenderPhase_{}, first).shaders.emplace(longName.c_str());
        graph.getDescriptors(root).blocks[{}].count = 3;
        graph.getDescriptors(first).groupBlocks[{}].capacity = 9;
        auto intact = [&] {
            const auto& shaders = graph.value(RenderPhase_{}, first).shaders;
            return shaders.size() == 1 && *shaders.begin() == longName.c_str() && graph.holds<RenderStage_>(root) &&
                   graph.getDescriptors(root).blocks.at({}).count == 3 && graph.getDescriptors(root).groupBlocks.empty() &&
                   graph.getDescriptors(first).groupBlocks.at({}).capacity == 9 && graph.getDescriptors(first).blocks.empty();
        };
        resource.allocationsBeforeFailure = failAfter;
        try {
            const auto added = graph.add_vertex(RenderPhase_{}, longName, root);
            return failAfter > 0 && added == 2 && hasChildren(graph, root, {first, added}) && intact() &&
                   graph.value(RenderPhase_{}, added).shaders.empty() &&
                   graph.getDescriptors(added).blocks.empty() && usesResource(graph.getDescriptors(added), graph);
        } catch (const std::bad_alloc&) {
            if (resource.allocationsBeforeFailure != 0) throw;
            if (graph.num_vertices() != 2 || !hasChildren(graph, root, {first}) || !hasChildren(graph, first, {}) ||
                graph.getName(root) != "root" || graph.getName(first) != "first" || !intact()) return false;
        }
        resource.allocationsBeforeFailure = -1;
        const auto retry = graph.add_vertex(RenderPhase_{}, longName, root);
        if (retry != 2 || !intact() || !graph.getDescriptors(retry).blocks.empty() ||
            graph.getName(retry) != longName.c_str() || !hasChildren(graph, root, {first, retry})) return false;
        // A leftover component after rollback would shift the next appended data by one.
        graph.getDescriptors(retry).blocks[{}].count = 4;
        graph.value(RenderPhase_{}, retry).shaders.emplace("retry");
        const auto next = graph.add_vertex(RenderPhase_{}, "next");
        if (!graph.getDescriptors(next).blocks.empty() || graph.getDescriptors(retry).blocks.at({}).count != 4 ||
            !graph.value(RenderPhase_{}, next).shaders.empty() || graph.value(RenderPhase_{}, retry).shaders.size() != 1) return false;
    }
}

using cc::render::LayoutGraphData;
constexpr auto frequency = cc::render::UpdateFrequency::PER_PASS;
constexpr auto visibility = cc::gfx::ShaderStageFlagBit::VERTEX;
constexpr std::string_view layoutName{"vertex name long enough to allocate from the graph resource"};

auto addLayout(LayoutGraphData& graph, unsigned i) {
    const auto parent = i ? (i - 1) / 2 : LayoutGraphData::null_vertex();
    // Exercise the view into an existing name during growth as well.
    const auto name = i ? std::string_view{graph.getName(parent)} : layoutName;
    return i % 2 ? graph.add_vertex(RenderPhase_{}, name, parent) : graph.add_vertex(RenderStage_{}, name, parent);
}

void fillLayout(LayoutGraphData& graph, unsigned i) {
    graph.getLayout(i).descriptorSets[frequency].descriptorSetLayoutData.capacity = i + 1;
    graph.visit_object(i,
        [&](cc::render::RenderPhaseData& phase) {
            phase.rootSignature.assign(layoutName.data(), layoutName.size());
            phase.shaderPrograms.emplace_back().layout.descriptorGroups[frequency].descriptorSetLayoutData.slot = i;
        },
        [&](cc::render::RenderStageData& stage) {
            stage.descriptorVisibility.emplace(cc::render::NameLocalID{i}, visibility);
        });
}

bool checkLayoutValues(const LayoutGraphData& graph, unsigned count) {
    const auto alloc = graph.get_allocator();
    if (graph.num_vertices() != count) return false;
    for (const auto i : graph.vertices()) {
        if (graph.get_parent(i) != (i ? (i - 1) / 2 : LayoutGraphData::null_vertex()) ||
            std::string_view{graph.getName(i)} != layoutName || graph.getName(i).get_allocator() != alloc) return false;
        unsigned child = 2 * i + 1;
        for (const auto link : graph.child_links(i)) {
            if (link.source != i || graph.child(link) != child++ || child > count) return false;
        }
        const unsigned children = (2 * i + 1 < count) + (2 * i + 2 < count);
        if (child != 2 * i + 1 + children || graph.num_children(i) != children) return false;
        const auto& sets = graph.getLayout(i).descriptorSets;
        const auto& data = sets.at(frequency).descriptorSetLayoutData;
        if (sets.size() != 1 || sets.get_allocator() != alloc || data.get_allocator() != alloc || data.capacity != i + 1)
            return false;
        if (i % 2) {
            if (!graph.holds<RenderPhase_>(i)) return false;
            const auto& phase = graph.value(RenderPhase_{}, i);
            if (phase.rootSignature.get_allocator() != alloc || std::string_view{phase.rootSignature} != layoutName ||
                phase.shaderPrograms.size() != 1 || phase.shaderPrograms.get_allocator() != alloc) return false;
            const auto& nested = phase.shaderPrograms.front().layout.descriptorGroups.at(frequency).descriptorSetLayoutData;
            if (nested.slot != i || nested.get_allocator() != alloc) return false;
        } else {
            if (!graph.holds<RenderStage_>(i)) return false;
            const auto& entries = graph.value(RenderStage_{}, i).descriptorVisibility;
            if (entries.size() != 1 || entries.at(cc::render::NameLocalID{i}) != visibility || entries.get_allocator() != alloc)
                return false;
        }
    }
    return true;
}

enum class InsertionResult { complete = 0, mismatch = 1, retried = 42, debugProxyTermination = 43 };

InsertionResult insertWithFailure(unsigned count, int failAfter) {
    static_assert(!std::is_constructible_v<LayoutGraphData, const LayoutGraphData&, LayoutGraphData::allocator_type>);
    static_assert(!std::is_copy_assignable_v<LayoutGraphData> && std::is_move_assignable_v<LayoutGraphData>);
    FailingResource resource;
    LayoutGraphData graph{&resource};
    for (unsigned i = 0; i != count; ++i) fillLayout(graph, addLayout(graph, i));
    resource.allocationsBeforeFailure = failAfter;
    bool failed = false;
    try {
        if (addLayout(graph, count) != count) return InsertionResult::mismatch;
    } catch (const std::bad_alloc&) {
        failed = true;
        if (resource.allocationsBeforeFailure != 0 || !checkLayoutValues(graph, count)) return InsertionResult::mismatch;
    }
    resource.allocationsBeforeFailure = -1;
    if (failed && addLayout(graph, count) != count) return InsertionResult::mismatch;
    if (!graph.getLayout(count).descriptorSets.empty()) return InsertionResult::mismatch;
    fillLayout(graph, count);
    fillLayout(graph, addLayout(graph, count + 1));
    if (!checkLayoutValues(graph, count + 2)) return InsertionResult::mismatch;
    if (failed) return InsertionResult::retried;
    LayoutGraphData moved{std::move(graph)};
    FailingResource other;
    LayoutGraphData assigned{&other};
    assigned = std::move(moved);
    if (failAfter == 0 || graph.num_vertices() || moved.num_vertices() || assigned.get_allocator().resource() != &other ||
        !checkLayoutValues(assigned, count + 2)) return InsertionResult::mismatch;
    return InsertionResult::complete;
}

bool checkMoveOnlyGrowth() {
    for (const auto count : {3U, 15U, 31U}) {
        for (int failAfter = 0; ; ++failAfter) {
            const auto result = insertWithFailure(count, failAfter);
            if (result == InsertionResult::complete) break;
            if (result != InsertionResult::retried) return false;
        }
    }
    return true;
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

bool checkGraphPaths(const char* sampleFile) {
    FailingResource resource;
    LayoutGraph graph{&resource};
    const auto& view = std::as_const(graph);
    const auto none = LayoutGraph::null_vertex();
    static_assert(noexcept(view.locate("/main")) && noexcept(view.exists("/main")));
    static_assert(std::is_same_v<decltype(view.path(none)), ccstd::pmr::string>);
    if (view.exists("/main") || !view.path(none).empty() || view.path(none).get_allocator().resource() != &resource) return false;

    const auto main = graph.add_vertex("main");
    const auto shadow = graph.add_vertex("shadow", main);
    graph.value(RenderStage_{}, main) = RenderPassType::RENDER_PASS;
    graph.value(RenderStage_{}, shadow) = RenderPassType::RENDER_SUBPASS;
    const auto light = graph.add_vertex(RenderPhase_{}, "lighting", main);
    const auto leaf = graph.add_vertex(RenderPhase_{}, "leaf", shadow);
    const auto ui = graph.add_vertex("ui");
    graph.add_vertex("shadow", main);
    constexpr std::string_view binaryName{"x\0y", 3};
    constexpr std::string_view binaryPath{"/ui/x\0y", 7};
    const auto binary = graph.add_vertex(RenderPhase_{}, binaryName, ui);
    const auto uiLeaf = graph.add_vertex(RenderPhase_{}, "leaf", ui);
    const auto invalid = view.num_vertices();
    if (view.path(main) != "/main" || view.path(leaf) != "/main/shadow/leaf" ||
        std::string_view{view.path(binary)} != binaryPath) return false;
    try {
        view.path(invalid);
        return false;
    } catch (const std::out_of_range&) {}

    struct Query { std::string_view text; Vertex start; Vertex expected; };
    const Query queries[] = {
        {"/main/shadow/leaf", ui, leaf}, {"shadow/leaf", main, leaf}, {"../lighting", shadow, light},
        {"//main/./shadow//leaf/", none, leaf}, {"/ui", invalid, ui}, {"../../ui", shadow, ui},
        {"../../../ui", shadow, none}, {".", shadow, shadow}, {"..", main, none}, {"..", invalid, none},
        {"", shadow, none}, {"/", main, none}, {"./", none, none}, {"missing/../lighting", main, none},
        {"lighting", ui, none}, {binaryPath, none, binary}, {"main/shadow/leaf", none, leaf},
        {"/ui/leaf", shadow, uiLeaf}, {"../leaf", binary, uiLeaf},
    };
    resource.allocationsBeforeFailure = 0;
    for (const auto& query : queries) {
        if (view.locate(query.text, query.start) != query.expected ||
            view.exists(query.text, query.start) != (query.expected != none)) return false;
    }
    resource.allocationsBeforeFailure = -1;
    if (sampleFile) {
        // Replay this tree and the native query results in the TypeScript consumer.
        std::ofstream file(sampleFile, std::ios::binary);
        cc::render::BinaryOutputArchive samples(file, &resource);
        samples.writeNumber(view.num_vertices());
        for (const auto v : view.vertices()) {
            samples.writeNumber(view.get_parent(v));
            samples.writeString(view.getName(v));
            view.visit_object(v,
                [&](RenderPassType pass) {
                    samples.writeBool(false);
                    samples.writeNumber(static_cast<unsigned>(pass));
                },
                [&](const RenderPhase&) { samples.writeBool(true); });
            const auto path = view.path(v);
            samples.writeString(path);
            samples.writeNumber(view.locate(path));
        }
        samples.writeNumber(std::size(queries));
        for (const auto& query : queries) {
            samples.writeNumber(query.start);
            samples.writeString(query.text);
            samples.writeNumber(view.locate(query.text, query.start));
        }
        file.close();
        if (!file) return false;
    }
    const std::string renamed(160, 'p');
    graph.getName(main).assign(renamed.data(), renamed.size());
    const auto newPath = "/" + renamed + "/shadow/leaf";
    const auto actual = view.path(leaf);
    return actual.get_allocator().resource() == &resource && std::string_view{actual} == std::string_view{newPath} &&
           view.locate(newPath) == leaf && view.exists(newPath) && !view.exists("/main/shadow/leaf");
}

bool checkFailedAssignment(bool move) {
    boost::container::pmr::monotonic_buffer_resource arena;
    FailingResource resource;
    LayoutGraph source{&arena};
    const auto root = source.add_vertex("original");
    const auto leaf = source.add_vertex(RenderPhase_{}, "leaf", root);
    source.value(RenderPhase_{}, leaf).shaders.emplace("original");
    source.getDescriptors(leaf).blocks[{}].count = 42;
    for (int failAfter = 0; ; ++failAfter) {
        resource.allocationsBeforeFailure = -1;
        LayoutGraph target{&resource};
        target.add_vertex("previous");
        target.getDescriptors(0).groupBlocks[{}].capacity = 8;
        resource.allocationsBeforeFailure = failAfter;
        try {
            if (move) target = std::move(source);
            else target = source;
            return hasChildren(target, root, {leaf}) && target.getName(leaf) == "leaf" &&
                   target.getName(leaf).get_allocator().resource() == &resource &&
                   target.value(RenderPhase_{}, leaf).shaders.size() == 1 &&
                   target.value(RenderPhase_{}, leaf).get_allocator().resource() == &resource &&
                   target.getDescriptors(leaf).blocks.at({}).count == 42 && usesResource(target.getDescriptors(leaf), target);
        } catch (const std::bad_alloc&) {
            if (resource.allocationsBeforeFailure != 0) throw;
            if (target.num_vertices() != 1 || target.getName(0) != "previous" || !hasChildren(target, 0, {}) ||
                source.num_vertices() != 2 || source.getName(leaf) != "leaf" || !hasChildren(source, root, {leaf}) ||
                !target.holds<RenderStage_>(0) || source.value(RenderPhase_{}, leaf).shaders.size() != 1 ||
                target.getDescriptors(0).groupBlocks.at({}).capacity != 8 || source.getDescriptors(leaf).blocks.at({}).count != 42)
                return false;
        }
    }
}

bool checkIndexedNames() {
    FailingResource arena, other;
    cc::IndexedNames graph{&arena};
    try {
        (void)graph.add_vertex("orphan", 42);
        return false;
    } catch (const std::out_of_range&) {}
    if (graph.contains("orphan") || graph.num_vertices()) return false;
    const auto root = graph.add_vertex("");
    const auto child = graph.emplace_vertex(cc::Count_{}, std::make_tuple(42U), std::forward_as_tuple("child"), std::make_tuple(7U), root);
    std::stringstream bytes;
    cc::render::BinaryOutputArchive writer(bytes, &arena);
    cc::save(writer, graph);
    cc::IndexedNames copy{graph, &other};
    copy.setName(root, "replaced");
    if (copy.contains("") || !graph.contains("")) return false;
    cc::render::BinaryInputArchive reader(bytes, &other);
    cc::load(reader, copy);
    if (copy.contains("replaced") || copy.vertex("") != root || copy.find("child") != child ||
        copy.getCount(child) != 42 || copy.value(cc::Count_{}, child) != 7 || copy.get_parent(child) != root) return false;
    cc::IndexedNames moved{std::move(copy), &arena};
    if (copy.contains("child") || copy.num_vertices() || moved.vertex("child") != child) return false;
    const std::string longName(160, 'x');
    for (int n = 0; ; ++n) {
        arena.allocationsBeforeFailure = n;
        try {
            moved.setName(child, longName);
            break;
        } catch (const InjectedFailure&) {
            if (moved.contains(longName) || moved.vertex("child") != child || moved.getName(child) != "child") return false;
        }
    }
    arena.allocationsBeforeFailure = -1;
    return moved.vertex(longName) == child && !moved.contains("child") && graph.vertex("child") == child;
}

bool checkFlatComponents() {
    boost::container::pmr::monotonic_buffer_resource sourceArena, targetArena;
    cc::Toggles toggles{&sourceArena};
    const auto enabled = toggles.emplace_vertex(cc::Count_{}, std::make_tuple(true));
    const auto disabled = toggles.add_vertex(cc::render::ComputeTag{});
    const auto selectFlag = [](bool& flag) -> bool& { return flag; };
    const auto unexpectedLayout = [](LayoutGraph&) -> bool& { throw std::runtime_error("expected bool"); };
    if (!toggles.visit_object(enabled, selectFlag, unexpectedLayout)) return false;
    if (&toggles.visit_object(disabled, selectFlag, unexpectedLayout) != &toggles.value(cc::render::ComputeTag{}, disabled)) return false;
    if (toggles.num_vertices() != 2 || toggles.holds<cc::Count_>(2) || !toggles.holds<cc::Count_>(enabled) ||
        !toggles.value(cc::Count_{}, enabled) || toggles.value(cc::render::ComputeTag{}, disabled)) return false;
    cc::FlatComponents source{&sourceArena};
    if (source.version != 1) return false;
    source.version = 4;
    const auto a = source.add_vertex("a");
    source.getCount(a) = 12;
    source.getDescriptors(a).blocks[{}].count = 5;
    const auto b = source.emplace_vertex(std::tuple<>{}, std::forward_as_tuple(source.getName(a)), std::forward_as_tuple(source.getCount(a)));
    if (a != 0 || b != 1 || source.getName(b) != "a" || source.get(cc::Count_{}, b) != 12) return false;
    cc::FlatComponents target{&targetArena};
    target = std::move(source);
    return source.num_vertices() == 0 && target.num_vertices() == 2 && target.version == 4 &&
           target.getName(b).get_allocator().resource() == &targetArena && target.getCount(a) == 12 &&
           target.getDescriptors(a).blocks.get_allocator().resource() == &targetArena &&
           target.getDescriptors(a).blocks.at({}).count == 5 && target.getDescriptors(b).blocks.empty();
}
} // namespace

int checkStageAllocation(unsigned count, int failAfter) {
#if defined(_MSVC_STL_VERSION) && _ITERATOR_DEBUG_LEVEL != 0
    // The process is isolated because unordered_map's noexcept _Hash_vec allocates a debug proxy.
    std::set_terminate([] {
        if (auto exception = std::current_exception()) {
            try { std::rethrow_exception(exception); }
            catch (const InjectedFailure&) { std::_Exit(static_cast<int>(InsertionResult::debugProxyTermination)); }
            catch (...) {}
        }
        std::_Exit(static_cast<int>(InsertionResult::mismatch));
    });
#endif
    return static_cast<int>(insertWithFailure(count, failAfter));
}

static bool checkLayoutEdges() {
    boost::container::pmr::monotonic_buffer_resource arena;
    LayoutGraph g{&arena};
    const auto root = g.add_vertex("root");
    const auto first = g.add_vertex("same");
    const auto last = g.add_vertex("same", root);
    const auto nested = g.add_vertex("nested", first);
    const auto [link, added] = g.add_edge(root, first);
    if (!added || g.parent(link) != root || g.child(link) != first || !hasChildren(g, root, {first, last})) return false;
    if (g.add_edge(root, first).second || g.add_edge(last, first).second || g.add_edge(nested, root).second ||
        g.add_edge(root, root).second || g.add_edge(root, g.null_vertex()).second) return false;
    const auto incoming = g.in_edges(first);
    if (std::distance(incoming.begin(), incoming.end()) != 1 || g.source(*incoming.begin()) != root ||
        g.target(*incoming.begin()) != first || g.in_degree(root) || g.degree(first) != 2 || g.num_edges() != 3) return false;
    const auto adjacent = g.adjacent_vertices(root);
    const std::initializer_list<Vertex> expected{first, last};
    if (!std::equal(adjacent.begin(), adjacent.end(), expected.begin(), expected.end())) return false;
    const std::initializer_list<Vertex> targets{first, last, nested};
    auto next = targets.begin();
    for (const auto e : g.edges()) {
        if (next == targets.end() || g.target(e) != *next++ || !g.has_link(e.source, e.target)) return false;
    }
    if (next != targets.end()) return false;
    g.remove_edge(link);
    if (g.get_parent(first) != g.null_vertex() || g.in_degree(first) || g.num_edges() != 2 ||
        g.path(nested) != "/same/nested" || g.locate("/root/same") != last) return false;
    g.remove_edge({root, last});
    g.remove_edge({root, last});
    return hasChildren(g, root, {}) && g.add_edge(root, last).second && hasChildren(g, root, {last});
}

bool checkLayoutGraph(const char* pathSamples) {
    if (!checkLayoutEdges()) return false;
    if (!checkChildLookup() || !checkGraphPaths(pathSamples) || !checkFlatComponents() || !checkIndexedNames() || !checkMoveOnlyGrowth()) return false;
    if (!checkHierarchy() || !checkFailedInsertion() || !checkFailedAssignment(false) || !checkFailedAssignment(true)) return false;
    using cc::render::LayoutGraph;
    using cc::render::Name_;
    using Name = ccstd::pmr::string;
    static_assert(std::is_same_v<LayoutGraph::Vertex, uint32_t>);
    static_assert(std::is_same_v<decltype(std::declval<LayoutGraph&>().getName(0)), Name&>);
    static_assert(std::is_same_v<decltype(std::declval<LayoutGraph&>().get(Name_{}, 0)), Name&>);
    static_assert(std::is_same_v<decltype(std::declval<const LayoutGraph&>().getName(0)), const Name&>);
    static_assert(std::is_same_v<decltype(std::declval<const LayoutGraph&>().get(Name_{}, 0)), const Name&>);
    static_assert(std::is_same_v<decltype(std::declval<LayoutGraph&>().getDescriptors(0)), DescriptorDB&>);
    static_assert(std::is_same_v<decltype(std::declval<LayoutGraph&>().get(Descriptors_{}, 0)), DescriptorDB&>);
    static_assert(std::is_same_v<decltype(std::declval<const LayoutGraph&>().getDescriptors(0)), const DescriptorDB&>);
    static_assert(std::is_same_v<decltype(std::declval<const LayoutGraph&>().get(Descriptors_{}, 0)), const DescriptorDB&>);

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
            view.getName(v).get_allocator().resource() != &arena ||
            &view.get(Descriptors_{}, v) != &view.getDescriptors(v) || !usesResource(view.getDescriptors(v), view)) return false;
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
    for (unsigned i = 0; i < 260; ++i) {
        if (!tiny.add_edge(0, limit - 1).second) return false;
    }
    if (tiny.out_degree(0) != 260 || tiny.num_edges() != 260) return false;
    tiny.remove_edge(0, limit - 1);
    if (tiny.num_edges() != 0) return false;
    try {
        tiny.add_vertex();
        return false;
    } catch (const std::length_error&) {
        return tiny.num_vertices() == limit && tiny.get(Name_{}, limit - 1) == "entry";
    }
}
