#include "FailingResource.h"
#include "ShiftProbe.h"
#include "ShiftSerialization.h"
#include "cocos/base/GraphIterator.h"
#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"
#include <boost/container/pmr/global_resource.hpp>
#include <boost/container/pmr/monotonic_buffer_resource.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <limits>
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
using Vertex = LayoutGraph::vertex_descriptor;
static_assert(std::is_same_v<decltype(LayoutGraph::phases), ccstd::pmr::vector<RenderPhase>>);

struct AddLayoutVertex {
    template<class... Args>
    auto operator()(Args&&... args) const -> decltype(std::declval<LayoutGraph&>().add_vertex(std::forward<Args>(args)...));
};

using Empty = std::tuple<>;
static_assert(std::is_invocable_v<AddLayoutVertex, RenderStage_, std::piecewise_construct_t, Empty, Empty, Empty>);
static_assert(!std::is_invocable_v<AddLayoutVertex, std::string_view>);
static_assert(!std::is_invocable_v<AddLayoutVertex, RenderStage_, Empty, Empty, Empty>);
static_assert(!std::is_invocable_v<AddLayoutVertex, Descriptors_, std::piecewise_construct_t, Empty, Empty, Empty>);
static_assert(!std::is_invocable_v<AddLayoutVertex, RenderStage_, std::piecewise_construct_t, int, Empty, Empty>);

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
    const auto root = graph.add_vertex(RenderStage_{}, "main");
    graph.getDescriptors(root).blocks[{}].count = 7;
    const auto ui = graph.add_vertex(RenderStage_{}, "ui", LayoutGraph::null_vertex());
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
        graph.add_vertex(RenderStage_{}, "invalid", graph.num_vertices());
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
    if (!intact(assigned)) return false;
    LayoutGraph moved{std::move(assigned), &firstArena};
    return intact(moved) && intact(graph);
}

bool insertDescriptorsWithFailure(FailingResource& resource, int failAfter) {
    const std::string longName(160, 'x');
    LayoutGraph graph{&resource};
    const auto root = graph.add_vertex(RenderStage_{}, "root");
    const auto first = graph.add_vertex(RenderPhase_{}, "first", root);
    graph.value(RenderPhase_{}, first).shaders.emplace(longName.c_str());
    graph.getDescriptors(root).blocks[{}].count = 3;
    graph.getDescriptors(first).groupBlocks[{}].capacity = 9;
    resource.allocationsBeforeFailure = failAfter;
    const auto added = graph.add_vertex(RenderPhase_{}, longName, root);
    const auto& shaders = graph.value(RenderPhase_{}, first).shaders;
    return added == 2 && hasChildren(graph, root, {first, added}) && graph.getName(added) == longName.c_str() &&
           shaders.size() == 1 && *shaders.begin() == longName.c_str() && graph.getDescriptors(root).blocks.at({}).count == 3 &&
           graph.getDescriptors(first).groupBlocks.at({}).capacity == 9 && graph.value(RenderPhase_{}, added).shaders.empty() &&
           graph.getDescriptors(added).blocks.empty() && usesResource(graph.getDescriptors(added), graph);
}

using cc::render::LayoutGraphData;
static_assert(std::is_same_v<decltype(LayoutGraphData::stages), ccstd::pmr::vector<cc::render::RenderStageData>>);
static_assert(std::is_same_v<decltype(LayoutGraphData::phases), ccstd::pmr::vector<cc::render::RenderPhaseData>>);
constexpr auto frequency = cc::render::UpdateFrequency::PER_PASS;
constexpr auto visibility = cc::gfx::ShaderStageFlagBit::VERTEX;
constexpr std::string_view layoutName{"vertex name long enough to allocate from the graph resource"};

auto addLayout(LayoutGraphData& graph, unsigned i) {
    const auto parent = i ? (i - 1) / 2 : LayoutGraphData::null_vertex();
    return i % 2 ? graph.add_vertex(RenderPhase_{}, layoutName, parent) : graph.add_vertex(RenderStage_{}, layoutName, parent);
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

enum class InsertionResult { complete = 0, mismatch = 1, discarded = 42, debugProxyTermination = 43, moveTermination = 44 };

[[noreturn]] void exitOnAllocationFailure(InsertionResult result) noexcept {
    if (auto exception = std::current_exception()) {
        try { std::rethrow_exception(exception); }
        catch (const InjectedFailure&) { std::_Exit(static_cast<int>(result)); }
        catch (...) {}
    } else if (FailingResource::failureInjected) {
        // MSVC can enter terminate from a defaulted noexcept assignment with no current exception.
        std::_Exit(static_cast<int>(result));
    }
    std::_Exit(static_cast<int>(InsertionResult::mismatch));
}

InsertionResult insertWithFailure(FailingResource& resource, unsigned count, int failAfter) {
    static_assert(!std::is_constructible_v<LayoutGraphData, const LayoutGraphData&, LayoutGraphData::allocator_type>);
    static_assert(!std::is_copy_assignable_v<LayoutGraphData> && std::is_nothrow_move_assignable_v<LayoutGraphData>);
    LayoutGraphData graph{&resource};
    for (unsigned i = 0; i != count; ++i) fillLayout(graph, addLayout(graph, i));
    resource.allocationsBeforeFailure = failAfter;
    if (addLayout(graph, count) != count) return InsertionResult::mismatch;
    resource.allocationsBeforeFailure = -1;
    if (!graph.getLayout(count).descriptorSets.empty()) return InsertionResult::mismatch;
    fillLayout(graph, count);
    fillLayout(graph, addLayout(graph, count + 1));
    if (!checkLayoutValues(graph, count + 2)) return InsertionResult::mismatch;
    const auto* layouts = graph.layouts.data();
    LayoutGraphData moved{std::move(graph)};
    if (moved.layouts.data() != layouts || moved.get_allocator().resource() != &resource ||
        !checkLayoutValues(moved, count + 2)) return InsertionResult::mismatch;
    FailingResource other;
    LayoutGraphData assigned{&other};
    assigned = std::move(moved);
    if (failAfter == 0 || assigned.get_allocator().resource() != &other ||
        !checkLayoutValues(assigned, count + 2)) return InsertionResult::mismatch;
    return InsertionResult::complete;
}

bool checkChildLookup() {
    using cc::render::Name_;
    FailingResource resource;
    LayoutGraph graph{&resource};
    const auto& view = std::as_const(graph);
    const auto none = LayoutGraph::null_vertex();
    static_assert(noexcept(view.locate_child(none, "main")));
    if (view.locate_child(none, "main") != none) return false;

    const auto main = graph.add_vertex(RenderStage_{}, "main");
    const auto shadow = graph.add_vertex(RenderStage_{}, "shadow", main);
    const auto grandchild = graph.add_vertex(RenderStage_{}, "lighting", shadow);
    const auto other = graph.add_vertex(RenderStage_{}, "lighting");
    const auto otherShadow = graph.add_vertex(RenderStage_{}, "shadow", other);
    const auto laterRoot = graph.add_vertex(RenderStage_{}, "main");
    const auto laterShadow = graph.add_vertex(RenderStage_{}, "shadow", main);
    const auto leaf = graph.add_vertex(RenderStage_{});
    const std::string_view binaryName{"x\0y", 3};
    const auto binary = graph.add_vertex(RenderStage_{}, binaryName, main);

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

    const auto main = graph.add_vertex(RenderStage_{}, "main");
    const auto shadow = graph.add_vertex(RenderStage_{}, "shadow", main);
    graph.value(RenderStage_{}, main) = RenderPassType::RENDER_PASS;
    graph.value(RenderStage_{}, shadow) = RenderPassType::RENDER_SUBPASS;
    const auto light = graph.add_vertex(RenderPhase_{}, "lighting", main);
    const auto leaf = graph.add_vertex(RenderPhase_{}, "leaf", shadow);
    const auto ui = graph.add_vertex(RenderStage_{}, "ui");
    graph.add_vertex(RenderStage_{}, "shadow", main);
    constexpr std::string_view binaryName{"x\0y", 3};
    constexpr std::string_view binaryPath{"/ui/x\0y", 7};
    const auto binary = graph.add_vertex(RenderPhase_{}, binaryName, ui);
    const auto uiLeaf = graph.add_vertex(RenderPhase_{}, "leaf", ui);
    const auto invalid = view.num_vertices();
    if (view.locate("..", invalid) != none || view.exists("..", invalid) ||
        view.locate("/ui", invalid) != ui || !view.exists("/ui", invalid)) return false;
    if (view.path(main) != "/main" || view.path(leaf) != "/main/shadow/leaf" ||
        std::string_view{view.path(binary)} != binaryPath) return false;
    try {
        view.path(invalid);
        return false;
    } catch (const std::out_of_range&) {}

    struct Query { std::string_view text; Vertex start; Vertex expected; };
    const Query queries[] = {
        {"/main/shadow/leaf", ui, leaf}, {"shadow/leaf", main, leaf}, {"../lighting", shadow, light},
        {"//main/./shadow//leaf/", none, leaf}, {"/ui", main, ui}, {"../../ui", shadow, ui},
        {"../../../ui", shadow, none}, {".", shadow, shadow}, {"..", main, none},
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

InsertionResult assignWithFailure(FailingResource& resource, bool move, int failAfter) {
    static_assert(std::is_nothrow_move_assignable_v<LayoutGraph>);
    boost::container::pmr::monotonic_buffer_resource arena;
    LayoutGraph source{&arena};
    const auto root = source.add_vertex(RenderStage_{}, "original");
    const auto leaf = source.add_vertex(RenderPhase_{}, "leaf", root);
    source.value(RenderPhase_{}, leaf).shaders.emplace("original");
    source.getDescriptors(leaf).blocks[{}].count = 42;
    LayoutGraph target{&resource};
    target.add_vertex(RenderStage_{}, "previous");
    target.getDescriptors(0).groupBlocks[{}].capacity = 8;
    resource.allocationsBeforeFailure = failAfter;
    if (move) {
        std::set_terminate([] { exitOnAllocationFailure(InsertionResult::moveTermination); });
        target = std::move(source);
    } else {
        target = source;
    }
    if (!hasChildren(target, root, {leaf}) || target.getName(leaf) != "leaf" ||
        target.getName(leaf).get_allocator().resource() != &resource || target.value(RenderPhase_{}, leaf).shaders.size() != 1 ||
        target.value(RenderPhase_{}, leaf).get_allocator().resource() != &resource ||
        target.getDescriptors(leaf).blocks.at({}).count != 42 || !usesResource(target.getDescriptors(leaf), target))
        return InsertionResult::mismatch;
    return InsertionResult::complete;
}

bool checkIndexedNames() {
    boost::container::pmr::monotonic_buffer_resource arena, other;
    cc::IndexedNames graph{&arena};
    try {
        (void)graph.add_vertex(cc::Count_{}, "orphan", 42);
        return false;
    } catch (const std::out_of_range&) {}
    if (graph.contains("orphan") || graph.num_vertices()) return false;
    const auto root = graph.add_vertex(cc::Count_{}, "");
    const auto child = graph.add_vertex(cc::Count_{}, std::piecewise_construct, std::make_tuple(7U),
                                       std::make_tuple(42U), std::forward_as_tuple("child"), root);
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
    if (moved.vertex("child") != child) return false;
    const std::string longName(160, 'x');
    moved.setName(child, longName);
    return moved.vertex(longName) == child && !moved.contains("child") && graph.vertex("child") == child;
}

bool checkFlatComponents() {
    boost::container::pmr::monotonic_buffer_resource sourceArena, targetArena;
    cc::Toggles toggles{&sourceArena};
    const auto enabled = toggles.add_vertex(cc::Count_{}, std::piecewise_construct, std::make_tuple(true));
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
    const auto b = source.add_vertex(std::piecewise_construct, std::tuple<>{},
                                     std::make_tuple(source.getName(a)), std::make_tuple(source.getCount(a)));
    if (a != 0 || b != 1 || source.getName(b) != "a" || source.get(cc::Count_{}, b) != 12) return false;
    cc::FlatComponents target{&targetArena};
    target = std::move(source);
    return target.num_vertices() == 2 && target.version == 4 &&
           target.getName(b).get_allocator().resource() == &targetArena && target.getCount(a) == 12 &&
           target.getDescriptors(a).blocks.get_allocator().resource() == &targetArena &&
           target.getDescriptors(a).blocks.at({}).count == 5 && target.getDescriptors(b).blocks.empty();
}
} // namespace

int checkLayoutAllocation(const std::string& sample, int failAfter) {
    FailingResource::failureInjected = false;
#if defined(_MSVC_STL_VERSION) && _ITERATOR_DEBUG_LEVEL != 0
    // Some noexcept STL constructors allocate an iterator proxy.
    std::set_terminate([] { exitOnAllocationFailure(InsertionResult::debugProxyTermination); });
#endif
    FailingResource resource;
    try {
        if (sample == "descriptors")
            return static_cast<int>(insertDescriptorsWithFailure(resource, failAfter) ? InsertionResult::complete : InsertionResult::mismatch);
        if (sample == "copy" || sample == "move")
            return static_cast<int>(assignWithFailure(resource, sample == "move", failAfter));
        return static_cast<int>(insertWithFailure(resource, std::stoul(sample), failAfter));
    } catch (const InjectedFailure&) {
        return static_cast<int>(resource.outstandingAllocations() == 0 ? InsertionResult::discarded : InsertionResult::mismatch);
    }
}

static bool checkForestIterators() {
    using Vertex = std::uint8_t;
    using Link = cc::graph::EdgeDescriptor<Vertex>;
    using Rows = std::array<cc::graph::TreeEdge<Vertex>, 3>;
    using Child = cc::graph::ChildIterator<Link, false>;
    using Edge = cc::graph::EdgeIterator<Link, Rows>;
    constexpr auto none = (std::numeric_limits<Vertex>::max)();
    Rows rows{};
    rows[1] = {none, 2, 2, none, 1};
    rows[2].parent = 1;
    const Rows copy = rows;
    Child child{rows.data(), 2};
    Edge edge{rows, 0};
    if (child == Child{copy.data(), 2} || edge == Edge{copy, 0}) return false;
    const auto childLink = *child++;
    const auto edgeLink = *edge++;
    cc::graph::ParentLinkIterator<Link> parent{childLink};
    return childLink.source == 1 && childLink.target == 2 && child == Child{rows.data(), none} &&
           edgeLink.source == 1 && edgeLink.target == 2 && edge == Edge{rows, 3} &&
           (*parent++).source == 1 && (*parent).source == none && (*parent).target == 2;
}

static bool checkLayoutEdges() {
    boost::container::pmr::monotonic_buffer_resource arena;
    LayoutGraph g{&arena};
    const auto root = g.add_vertex(RenderStage_{}, "root");
    const auto first = g.add_vertex(RenderStage_{}, "same");
    const auto last = g.add_vertex(RenderStage_{}, "same", root);
    const auto nested = g.add_vertex(RenderStage_{}, "nested", first);
    const auto [link, added] = g.add_edge(root, first);
    if (!added || g.parent(link) != root || g.child(link) != first || !hasChildren(g, root, {first, last})) return false;
    if (g.add_edge(root, first).second || g.add_edge(last, first).second || g.add_edge(nested, root).second ||
        g.add_edge(root, root).second || g.add_edge(root, g.null_vertex()).second) return false;
    const auto incoming = g.in_edges(first);
    if (std::distance(incoming.begin(), incoming.end()) != 1 || g.source(*incoming.begin()) != root ||
        g.target(*incoming.begin()) != first || g.in_degree(root) || g.degree(first) != 2) return false;
    const auto adjacent = g.adjacent_vertices(root);
    const std::initializer_list<Vertex> expected{first, last};
    if (!std::equal(adjacent.begin(), adjacent.end(), expected.begin(), expected.end())) return false;
    if (!hasChildren(g, first, {nested}) || !g.has_link(first, nested)) return false;
    g.remove_edge(link);
    if (g.get_parent(first) != g.null_vertex() || g.in_degree(first) || !hasChildren(g, root, {last}) ||
        g.path(nested) != "/same/nested" || g.locate("/root/same") != last) return false;
    g.remove_edge({root, last});
    g.remove_edge({root, last});
    return hasChildren(g, root, {}) && g.add_edge(root, last).second && hasChildren(g, root, {last});
}

bool checkLayoutGraph(const char* pathSamples) {
    if (!checkForestIterators() || !checkLayoutEdges()) return false;
    if (!checkChildLookup() || !checkGraphPaths(pathSamples) || !checkFlatComponents() || !checkIndexedNames()) return false;
    if (!checkHierarchy()) return false;
    using cc::render::LayoutGraph;
    using cc::render::Name_;
    using Name = ccstd::pmr::string;
    static_assert(std::is_same_v<LayoutGraph::vertex_descriptor, uint32_t>);
    static_assert(std::is_nothrow_move_constructible_v<LayoutGraph>);
    static_assert(std::is_same_v<decltype(LayoutGraph::names), ccstd::pmr::vector<Name>>);
    static_assert(std::is_same_v<decltype(LayoutGraph::descriptors), ccstd::pmr::vector<DescriptorDB>>);
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
    const auto first = graph.add_vertex(RenderStage_{}, longName);
    const auto second = graph.add_vertex(RenderStage_{}, "lighting");
    if (first != 0 || second != 1 || view.num_vertices() != 2) return false;
    if (view.getName(first) != longName.c_str() || view.get(Name_{}, second) != "lighting") return false;
    const auto vertices = view.vertices();
    if (std::distance(vertices.begin(), vertices.end()) != 2) return false;
    LayoutGraph::vertex_descriptor expected{};
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
    static_assert(std::is_same_v<cc::TinyNames::vertex_descriptor, uint8_t>);
    cc::TinyNames tiny{&arena};
    constexpr auto limit = (std::numeric_limits<cc::TinyNames::vertex_descriptor>::max)();
    for (unsigned i = 0; i < limit; ++i) {
        if (tiny.add_vertex("entry") != i) return false;
    }
    for (unsigned i = 0; i < 260; ++i) {
        if (!tiny.add_edge(0, limit - 1).second) return false;
    }
    if (tiny.out_degree(0) != 260 || tiny.num_edges() != 260) return false;
    if (!tiny.add_edge(limit - 1, 0).second) return false;
    tiny.remove_edge(0, limit - 1);
    const auto edges = tiny.edges();
    auto iter = edges.begin();
    if (iter == edges.end() || (*iter).source != limit - 1 || (*iter).target != 0 || ++iter != edges.end()) return false;
    tiny.remove_edge(limit - 1, 0);
    if (tiny.num_edges() != 0) return false;
    try {
        tiny.add_vertex();
        return false;
    } catch (const std::length_error&) {
        return tiny.num_vertices() == limit && tiny.get(Name_{}, limit - 1) == "entry";
    }
}
