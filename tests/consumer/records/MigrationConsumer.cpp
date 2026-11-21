#include "cocos/renderer/pipeline/custom/RenderInterfaceTypes.h"
#include "cocos/renderer/pipeline/custom/PrivateTypes.h"
#include "cocos/renderer/pipeline/custom/FGDispatcherTypes.h"
#include "cocos/renderer/pipeline/custom/NativeTypes.h"
#include "cocos/renderer/pipeline/custom/NativePipelineTypes.h"

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

static_assert(!std::is_convertible_v<const FrustumCullingID&, uint32_t>);
static_assert(!std::is_convertible_v<const LightBoundsCullingID&, uint32_t>);
static_assert(!std::is_convertible_v<const NativeRenderQueueID&, uint32_t>);

namespace {
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
    return checkDispatcherGraphs() && checkRenderQueues() && checkCullingIndices() && programs.programInfos.empty() && customization.contexts.empty() && context.renderGraph == nullptr &&
           copy.orderedViews == pass.orderedViews && copy.get_allocator().resource() == &memory &&
           barrier.type == cc::gfx::BarrierType::FULL && PipelineCapabilities{}.subpass == SubpassCapabilities::NONE;
}
