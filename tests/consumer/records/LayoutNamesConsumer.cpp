#include "cocos/renderer/pipeline/custom/LayoutGraphNames.h"

bool checkLayoutNames() {
    using namespace cc::render;
    using namespace std::string_view_literals;
    const LayoutGraph::allocator_type alloc{};
    const LayoutGraph graph{alloc};
    const LayoutGraphData data{alloc};
    return getName(LayoutType::WEBGPU) == "WEBGPU"sv &&
           getName(DescriptorTypeOrder::STORAGE_IMAGE) == "STORAGE_IMAGE"sv &&
           getName(RenderPassType::RENDER_SUBPASS) == "RENDER_SUBPASS"sv &&
           getName(static_cast<RenderPassType>(255)) == ""sv &&
           getName(UpdateFrequency::PER_PASS) == "PER_PASS"sv &&
           getName(RenderStage_{}) == "RenderStage"sv && getName(RenderPhase_{}) == "RenderPhase"sv &&
           getName(Name_{}) == "Name"sv && getName(Layout_{}) == "Layout"sv &&
           getName(Descriptor{}) == "Descriptor"sv &&
           getName(graph) == "LayoutGraph"sv && getName(data) == "LayoutGraphData"sv;
}
