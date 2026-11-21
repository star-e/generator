#include "cocos/renderer/pipeline/custom/RenderInterfaceTypes.h"
#include "cocos/renderer/pipeline/custom/PrivateTypes.h"
#include "cocos/renderer/pipeline/custom/FGDispatcherTypes.h"
#include "cocos/renderer/pipeline/custom/NativeTypes.h"
#include "cocos/renderer/pipeline/custom/NativePipelineTypes.h"
#include "ShiftProbe.h"

#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <limits>
#include <type_traits>
#include <utility>

using namespace cc::render;

static_assert(std::is_base_of_v<BasicPipeline, Pipeline>);
static_assert(std::is_base_of_v<Customization, CustomPipelineContext>);
static_assert(std::is_same_v<decltype(std::declval<ProgramProxy&>().getName()), const ccstd::string&>);
static_assert(!std::is_copy_constructible_v<BufferPool>);
static_assert(std::is_move_constructible_v<DescriptorSetContext> && !std::is_copy_constructible_v<DescriptorSetContext>);
static_assert(std::tuple_size_v<decltype(BuiltinCascadedShadowMap::layers)> == 4);

template<class... T>
constexpr bool concreteImplementations = ((!std::is_abstract_v<T> && std::is_final_v<T>) && ...);
static_assert(concreteImplementations<NativeSceneBuilder, NativeRenderQueueBuilder, NativeRenderSubpassBuilder,
    NativeMultisampleRenderSubpassBuilder, NativeComputeSubpassBuilder, NativeRenderPassBuilder,
    NativeMultisampleRenderPassBuilder, NativeComputeQueueBuilder, NativeComputePassBuilder,
    NativeProgramLibrary, NativePipeline, NativeProgramProxy, NativeRenderingModule>);
static_assert(std::is_base_of_v<Setter, NativeRenderQueueBuilder> && std::is_base_of_v<NativeSetter, NativeRenderQueueBuilder>);
static_assert(std::is_nothrow_constructible_v<NativeRenderQueueBuilder, const PipelineRuntime*, RenderGraph*, RenderGraph::Vertex,
                                              const LayoutGraphData*, LayoutGraphData::Vertex>);
static_assert(std::is_nothrow_constructible_v<NativeMultisampleRenderPassBuilder, const PipelineRuntime*, RenderGraph*, RenderGraph::Vertex,
                                              const LayoutGraphData*, LayoutGraphData::Vertex, RenderGraph::Vertex, LayoutGraphData::Vertex>);
static_assert(!std::is_default_constructible_v<NativeRenderQueueBuilder>);
static_assert(!std::is_constructible_v<cc::CombinedCtor, uint32_t>);
static_assert(!std::is_constructible_v<cc::MoveArgumentPmr, const cc::MoveArgumentPmr&, cc::MoveArgumentPmr::allocator_type>);
static_assert(std::is_constructible_v<cc::ExplicitEmptyDerived, uint32_t> && !std::is_nothrow_constructible_v<cc::ExplicitEmptyDerived, uint32_t>);
static_assert(!std::is_default_constructible_v<DescriptorSetKey> && !std::is_move_constructible_v<ResourceGroup>);
static_assert(std::is_nothrow_constructible_v<NativePipeline, const NativePipeline::allocator_type&>);
static_assert(!std::is_default_constructible_v<NativeRenderContext>);

static_assert(!std::is_convertible_v<const FrustumCullingID&, uint32_t>);
static_assert(!std::is_convertible_v<const LightBoundsCullingID&, uint32_t>);
static_assert(!std::is_convertible_v<const NativeRenderQueueID&, uint32_t>);

cc::CustomBaseCtor::CustomBaseCtor(uint32_t value) : id(value) {}

namespace {
bool checkForwardedConstructors() {
    cc::MoveProbe item;
    item.value = 31;
    cc::CombinedCtor combined{5, true, std::move(item)};
    cc::OptionalBaseCtor local{11};
    if (cc::ExplicitEmptyDerived{42}.id != 42 || combined.RequiredCtor::count != 5 || !combined.enabled || combined.item.value != 31 || combined.count != 9 ||
        local.DefaultCtor::count != 7 || local.count != 11) return false;

    boost::container::pmr::monotonic_buffer_resource source, destination;
    cc::MoveProbe sourceItem;
    sourceItem.value = 17;
    cc::MoveArgumentPmr mixed{std::move(sourceItem), &source};
    cc::MoveArgumentPmr mixedMove{std::move(mixed), &destination};
    if (mixedMove.item.value != 17 || mixedMove.get_allocator().resource() != &destination) return false;
    cc::PmrDerivedCtor shortForm{5, &source};
    cc::PmrDerivedCtor full{std::allocator_arg, &source, 7, ccstd::pmr::string("label", &destination), 13};
    cc::PmrDerivedCtor copy{full, &destination};
    cc::PmrDerivedCtor moved{std::move(copy), &source};
    return shortForm.AllocatorCtorProbe::count == 5 && shortForm.count == 9 && full.get_allocator().resource() == &source &&
           copy.get_allocator().resource() == &destination && moved.get_allocator().resource() == &source &&
           moved.AllocatorCtorProbe::count == 7 && moved.countIn == "label" && moved.count == 13;
}

bool checkNativeContext(boost::container::pmr::memory_resource* memory) {
    NativeRenderContext context{nullptr, memory};
    const DescriptorSetKey key{7, UpdateFrequency::PER_PASS};
    context.graphNodeDescriptorSets.emplace(key, nullptr);
    const ccstd::pmr::vector<bool> passes({false, true}, memory);
    const RenderGraphFilter filter{&passes};
    RenderGraph graph{memory};
    const auto pass = graph.add_vertex("before");
    LayoutGraphData layouts{memory};
    NativeSetter setter{nullptr, &graph, pass, &layouts, 19};
    if (setter.renderGraph != &graph || setter.nodeID != pass || setter.layoutGraph != &layouts || setter.layoutID != 19) return false;
    cc::RenderNodeProbe node{nullptr, &graph, pass};
    RenderNode& api = node;
    api.setName("after");
    api.setCustomBehavior("custom");
    if (api.getName() != "after" || graph.getData(pass).custom != "custom") return false;
    NativeProgramProxy proxy;
    const ProgramProxy& program = proxy;
    if (program.getShader() != nullptr) return false;
    return !filter(0) && filter(1) && context.sceneCulling.gpuCullingPassID == RenderGraph::null_vertex() &&
           context.graphNodeDescriptorSets.count(key) == 1 && context.graphNodeDescriptorSets.count({8, key.frequency}) == 0 &&
           context.get_allocator().resource() == memory && context.sceneCulling.get_allocator().resource() == memory &&
           context.lightResources.get_allocator().resource() == memory;
}

bool checkCullingIndices() {
    const auto invalid = (std::numeric_limits<uint32_t>::max)();
    const NativeRenderQueueQuery empty{};
    if (static_cast<uint32_t>(empty.frustumCulledResultID) != invalid ||
        static_cast<uint32_t>(empty.lightBoundsCulledResultID) != invalid ||
        static_cast<uint32_t>(empty.renderQueueTarget) != invalid) return false;

    boost::container::pmr::monotonic_buffer_resource source, destination;
    FrustumCulling frustums{&source};
    LightBoundsCulling lights{&source};
    ccstd::pmr::unordered_map<NativeRenderQueueKey, NativeRenderQueueID> queues{&source};
    const NativeRenderQueueQuery query{FrustumCullingID{0}, LightBoundsCullingID{19}, NativeRenderQueueID{0x80000000U}};
    FrustumCullingKey frustumKey{};
    frustumKey.lightLevel = 3;
    const LightBoundsCullingKey lightKey{query.frustumCulledResultID};
    const NativeRenderQueueKey queueKey{query.frustumCulledResultID, query.lightBoundsCulledResultID, 7};
    frustums.resultIndex.emplace(frustumKey, query.frustumCulledResultID);
    lights.resultIndex.emplace(lightKey, query.lightBoundsCulledResultID);
    queues.emplace(queueKey, query.renderQueueTarget);

    FrustumCulling copied{frustums, &destination};
    LightBoundsCulling moved{std::move(lights), &destination};
    const auto frustumID = copied.resultIndex.at(frustumKey);
    const auto lightID = moved.resultIndex.at(LightBoundsCullingKey{frustumID});
    const auto queueID = queues.at(NativeRenderQueueKey{frustumID, lightID, 7});
    return copied.get_allocator().resource() == &destination && moved.get_allocator().resource() == &destination &&
           static_cast<uint32_t>(frustumID) == 0 && static_cast<uint32_t>(lightID) == 19 &&
           static_cast<uint32_t>(queueID) == 0x80000000U &&
           moved.resultIndex.count(LightBoundsCullingKey{FrustumCullingID{1}}) == 0 &&
           queues.count(NativeRenderQueueKey{frustumID, lightID, 8}) == 0;
}
} // namespace

bool checkDispatcherGraphs();
bool checkRenderQueues();

bool checkMigratedModules() {
    boost::container::pmr::monotonic_buffer_resource memory;
    ProgramGroup programs(&memory);
    PipelineCustomization customization(&memory);
    FGRenderPassInfo pass(&memory);
    pass.orderedViews.emplace_back("color");
    FGRenderPassInfo copy(pass, &memory);
    Barrier barrier;
    CustomRenderGraphContext context;
    return checkForwardedConstructors() && checkNativeContext(&memory) && checkDispatcherGraphs() && checkRenderQueues() && checkCullingIndices() && programs.programInfos.empty() && customization.contexts.empty() && context.renderGraph == nullptr &&
           copy.orderedViews == pass.orderedViews && copy.get_allocator().resource() == &memory &&
           barrier.type == cc::gfx::BarrierType::FULL && PipelineCapabilities{}.subpass == SubpassCapabilities::NONE;
}
