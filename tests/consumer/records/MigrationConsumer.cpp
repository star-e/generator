#include "cocos/renderer/pipeline/custom/RenderInterfaceTypes.h"
#include "cocos/renderer/pipeline/custom/PrivateTypes.h"
#include "cocos/renderer/pipeline/custom/FGDispatcherRecords.h"
#include "cocos/renderer/pipeline/custom/NativeTypes.h"
#include "cocos/renderer/pipeline/custom/NativePipelineRecords.h"

#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <type_traits>

using namespace cc::render;

static_assert(std::is_base_of_v<BasicPipeline, Pipeline>);
static_assert(std::is_base_of_v<Customization, CustomPipelineContext>);
static_assert(std::is_same_v<decltype(std::declval<ProgramProxy&>().getName()), const ccstd::string&>);
static_assert(!std::is_copy_constructible_v<BufferPool>);
static_assert(std::is_move_constructible_v<DescriptorSetContext> && !std::is_copy_constructible_v<DescriptorSetContext>);
static_assert(std::tuple_size_v<decltype(BuiltinCascadedShadowMap::layers)> == 4);

bool checkMigratedModules() {
    boost::container::pmr::monotonic_buffer_resource memory;
    ProgramGroup programs(&memory);
    PipelineCustomization customization(&memory);
    FGRenderPassInfo pass(&memory);
    pass.orderedViews.emplace_back("color");
    FGRenderPassInfo copy(pass, &memory);
    Barrier barrier;
    CustomRenderGraphContext context;
    return programs.programInfos.empty() && customization.contexts.empty() && context.renderGraph == nullptr &&
           copy.orderedViews == pass.orderedViews && copy.get_allocator().resource() == &memory &&
           barrier.type == cc::gfx::BarrierType::FULL && PipelineCapabilities{}.subpass == SubpassCapabilities::NONE;
}
