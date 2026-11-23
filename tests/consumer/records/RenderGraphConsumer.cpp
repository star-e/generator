#include "cocos/renderer/pipeline/custom/RenderGraphTypes.h"
#include "FailingResource.h"

#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

namespace {
using namespace cc::render;
using Arena = boost::container::pmr::monotonic_buffer_resource;

void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

template<class Tag>
void checkPayload(const RenderGraph& graph, RenderGraph::vertex_descriptor v, Tag tag, unsigned ordinal) {
    require(static_cast<unsigned>(graph.kind(v)) == ordinal && graph.holds<Tag>(v), "payload kind");
    const auto& expected = graph.value(tag, v);
    unsigned calls = 0;
    const bool matched = graph.visit_object(v,
        [&](decltype(expected) payload) { ++calls; return &payload == &expected; },
        [&](auto&) { ++calls; return false; });
    require(matched && calls == 1, "const visitor payload type, address and call count");
}

template<class Range>
std::vector<RenderGraph::vertex_descriptor> sources(Range range) {
    std::vector<RenderGraph::vertex_descriptor> result;
    for (auto e : range) result.push_back(e.source);
    return result;
}

void checkReferenceRollback() {
    for (int failAfter : {0, 1}) {
        FailingResource arena;
        RenderGraph graph{&arena};
        const auto pass = graph.add_vertex("pass");
        const auto queue = graph.add_vertex(Queue_{}, "queue");
        graph.add_edge(pass, queue);
        arena.allocationsBeforeFailure = failAfter;
        try {
            graph.add_reference(pass, queue);
            throw std::runtime_error("expected allocation failure");
        } catch (const InjectedFailure&) {}
        arena.allocationsBeforeFailure = -1;
        require(!graph.num_children(pass) && !graph.num_parents(queue) && graph.edge(pass, queue).second, "reference rollback");
        require(graph.add_reference(pass, queue).second && graph.get_parent(queue) == pass, "reference retry");
    }
    struct FailedConstruction {};
    struct BadPhase { operator uint32_t() const { throw FailedConstruction{}; } };
    Arena arena;
    RenderGraph graph{&arena};
    const auto root = graph.add_vertex("root");
    const auto insert = [&](auto phase) {
        return graph.emplace_vertex(Queue_{}, std::forward_as_tuple("queue"), std::tuple<>{}, std::tuple<>{},
            std::make_tuple(true), std::forward_as_tuple(phase, 13U), root);
    };
    try {
        insert(BadPhase{});
        throw std::runtime_error("expected queue construction failure");
    } catch (const FailedConstruction&) {}
    require(graph.num_vertices() == 1 && !graph.num_children(root) && !graph.num_edges(), "emplace reference rollback");
    const auto queue = insert(7U);
    require(queue == 1 && graph.num_children(root) == 1 && graph.num_parents(queue) == 1 &&
            graph.get_parent(queue) == root && graph.value(Queue_{}, queue).phaseID == 7, "emplace reference retry");
}
} // namespace

bool checkRenderGraph() {
    static_assert(!std::is_copy_constructible_v<RenderGraph> && !std::is_copy_assignable_v<RenderGraph>);
    checkReferenceRollback();
    Arena destination;
    RenderGraph graph{&destination};
    {
        Arena scratch;
        RenderGraph g{&scratch};
        const auto pass = g.add_vertex(RasterPassTag{}, "pass");
        const auto queue = g.emplace_vertex(Queue_{}, std::forward_as_tuple("queue"), std::forward_as_tuple("queue-layout"),
            std::tuple<>{}, std::make_tuple(true), std::make_tuple(QueueHint::BLEND, 11U, 17U), pass);
        const auto& queuePayload = g.value(Queue_{}, queue);
        require(queuePayload.hint == QueueHint::BLEND && queuePayload.phaseID == 11 && queuePayload.passLayoutID == 17 &&
                g.getName(queue) == "queue" && g.getLayout(queue) == "queue-layout" && g.getValid(queue), "emplaced queue");
        const auto scene = g.add_vertex(Scene_{}, "scene", queue);
        const auto compute = g.add_vertex(ComputeTag{}, "pass");
        require(g.getName(pass) == g.getName(compute) && g.index.empty(), "independent name index");
        require(g.num_edges() == 0 && g.get_parent(queue) == pass && g.get_parent(scene) == queue, "initial ownership");
        require(g.add_edge(pass, compute).second && g.add_edge(queue, compute).second &&
                g.add_edge(pass, compute).second, "execution dependencies");
        const auto link = g.add_reference(pass, compute);
        require(link.second && g.add_reference(queue, compute).second && g.add_reference(pass, compute).second, "ownership references");
        require(sources(g.in_edges(compute)) == std::vector<unsigned>{pass, queue, pass} &&
                sources(g.parent_links(compute)) == std::vector<unsigned>{pass, queue, pass}, "insertion order");
        g.remove_reference(link.first);
        require(g.get_parent(compute) == queue && g.num_parents(compute) == 2 && g.in_degree(compute) == 3, "remove one reference");
        g.remove_edge(g.edge(pass, compute).first);
        require(g.edge(pass, compute).second && g.num_parents(compute) == 2, "remove one parallel edge");
        g.remove_edge(pass, compute);
        require(!g.edge(pass, compute).second && g.has_link(pass, compute), "erase only dependencies");
        g.remove_reference(queue, compute);
        require(g.edge(queue, compute).second && !g.has_link(queue, compute) && g.get_parent(compute) == pass, "erase only references");
        const auto child = *g.child_links(queue).begin();
        require(g.child(child) == scene && g.parent(child) == queue && g.get_parent(g.null_vertex()) == g.null_vertex(), "reference endpoints");
        require(!g.add_reference(g.null_vertex(), compute).second && !g.add_edge(pass, g.null_vertex()).second, "invalid endpoints");
        try {
            g.add_vertex(Scene_{}, "bad", 77);
            return false;
        } catch (const std::out_of_range&) {}
        const auto add = [&](auto tag, unsigned ordinal) {
            const auto v = g.add_vertex(tag, "");
            checkPayload(g, v, tag, ordinal);
            return v;
        };
        checkPayload(g, pass, RasterPassTag{}, 0);
        checkPayload(g, queue, Queue_{}, 8);
        checkPayload(g, scene, Scene_{}, 9);
        checkPayload(g, compute, ComputeTag{}, 3);
        add(RasterSubpassTag{}, 1);
        add(ComputeSubpassTag{}, 2);
        add(ResolveTag{}, 4);
        const auto copy = [&] {
            Arena temporary;
            CopyPass payload{&temporary};
            auto& upload = payload.uploadPairs.emplace_back();
            upload.target = "destination-upload-target";
            upload.source = {7, 9};
            RenderData data{&temporary};
            data.constants[23] = {'c', 'p'};
            data.custom = "copy-pass-custom-render-data";
            return g.emplace_vertex(CopyTag{}, std::forward_as_tuple("copy"), std::forward_as_tuple(g.getLayout(queue)),
                std::forward_as_tuple(std::move(data)), std::forward_as_tuple(g.getValid(queue)),
                std::forward_as_tuple(std::move(payload)), queue);
        }();
        checkPayload(g, copy, CopyTag{}, 5);
        const auto& data = g.getData(copy);
        require(g.get_parent(copy) == queue && g.getLayout(copy) == "queue-layout" && g.getValid(copy) &&
                data.get_allocator().resource() == &scratch && data.constants.at(23).at(1) == 'p' &&
                data.constants.at(23).get_allocator().resource() == &scratch &&
                data.custom.get_allocator().resource() == &scratch && data.custom == "copy-pass-custom-render-data", "emplaced render data");
        add(MoveTag{}, 6);
        add(RaytraceTag{}, 7);
        add(Blit_{}, 10);
        add(Dispatch_{}, 11);
        const auto clear = add(Clear_{}, 12);
        const auto viewport = add(Viewport_{}, 13);
        g.getLayout(pass) = "main-layout";
        g.getValid(scene) = true;
        g.getData(scene).constants[7] = {'s'};
        auto& visitedCopy = g.visit_object(copy,
            [](CopyPass& payload) -> CopyPass& { return payload; },
            [](auto&) -> CopyPass& { throw std::runtime_error("expected CopyPass"); });
        require(&visitedCopy == &g.value(CopyTag{}, copy), "mutable move-only payload");
        const auto& copiedUpload = visitedCopy.uploadPairs.at(0);
        require(visitedCopy.uploadPairs.get_allocator().resource() == &scratch && copiedUpload.get_allocator().resource() == &scratch &&
                copiedUpload.target == "destination-upload-target" && copiedUpload.source.at(1) == 9, "emplaced move-only payload");
        try {
            g.visit_object(g.null_vertex(), [](auto&) { throw std::runtime_error("visitor called for invalid vertex"); });
            return false;
        } catch (const std::out_of_range&) {}
        g.value(Clear_{}, clear).emplace_back().slotName = "destination-clear-slot";
        g.value(Viewport_{}, viewport).width = 640.F;
        g.index.emplace("execution-index", compute);
        g.sortedVertices = {pass, queue, scene, compute, copy, clear, viewport};
        g.globalRenderData.constants[2] = {'g'};

        RenderGraph same{std::move(g)};
        require(!g.num_vertices() && same.num_vertices() == 14, "same-resource move");
        graph = std::move(same);
        require(!same.num_vertices() && same.add_vertex("again") == 0, "reuse moved graph");
    }
    const auto& order = graph.sortedVertices;
    const auto copy = order.at(4), clear = order.at(5), viewport = order.at(6);
    const auto& upload = graph.value(CopyTag{}, copy).uploadPairs.at(0);
    const auto& clearView = graph.value(Clear_{}, clear).at(0);
    return graph.get_allocator().resource() == &destination && graph.num_vertices() == 14 &&
           graph.index.begin()->first.get_allocator().resource() == &destination && graph.index.at("execution-index") == 3 &&
           graph.edge(1, 3).second && graph.num_edges() == 1 && graph.get_parent(3) == 0 && graph.num_children(0) == 2 &&
           graph.getLayout(0) == "main-layout" && graph.getValid(2) && graph.getData(2).constants.at(7).at(0) == 's' &&
           graph.getData(2).constants.at(7).get_allocator().resource() == &destination &&
           graph.globalRenderData.constants.at(2).get_allocator().resource() == &destination &&
           graph.globalRenderData.constants.at(2).at(0) == 'g' && order.get_allocator().resource() == &destination &&
           upload.target == "destination-upload-target" && upload.get_allocator().resource() == &destination &&
           clearView.slotName == "destination-clear-slot" && clearView.get_allocator().resource() == &destination &&
           graph.value(Viewport_{}, viewport).width == 640.F;
}
