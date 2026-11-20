#include "cocos/renderer/pipeline/custom/RenderGraphTypes.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <type_traits>

bool checkResourceGraph() {
    using namespace cc::render;
    static_assert(!std::is_move_constructible_v<ResourceGraph> && !std::is_copy_constructible_v<ResourceGraph>);
    static_assert(std::is_same_v<decltype(ResourceDesc::mipLevels), uint16_t>);
    boost::container::pmr::monotonic_buffer_resource arena;
    ResourceGraph g{&arena};
    const auto v = g.add_vertex(Managed_{}, "color");
    const auto& desc = g.getDesc(v);
    if (g.get(Name_{}, v) != g.getName(v) || g.kind(v) != ResourceGraph::Kind::Managed) return false;
    if (g.num_vertices() != 1 || g.getName(v) != "color" || !g.holds<Managed_>(v) ||
        desc.dimension != ResourceDimension::BUFFER || desc.sampleCount != cc::gfx::SampleCount::X1 ||
        desc.viewType != cc::gfx::TextureType::TEX2D || g.getTraits(v).residency != ResourceResidency::MANAGED ||
        g.getStates(v).states != cc::gfx::AccessFlagBit::NONE || g.value(Managed_{}, v).unused != 0) return false;
    g.getDesc(v).width = 256;
    if (g.get(Desc_{}, v).width != 256 || g.getSampler(v).minFilter != cc::gfx::Filter::LINEAR) return false;
    for (auto residency : {ResourceResidency::PERSISTENT, ResourceResidency::EXTERNAL, ResourceResidency::BACKBUFFER})
        if (!ResourceTraits{residency}.hasSideEffects()) return false;
    for (auto residency : {ResourceResidency::MANAGED, ResourceResidency::MEMORYLESS, ResourceResidency::CUSTOM})
        if (ResourceTraits{residency}.hasSideEffects()) return false;
    return true;
}
