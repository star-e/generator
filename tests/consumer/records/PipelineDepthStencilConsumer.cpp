#include "cocos/renderer/gfx-base/GFXSwapchain.h"
#include "cocos/renderer/gfx-base/GFXTexture.h"
#include "cocos/renderer/pipeline/custom/RenderGraphTypes.h"
#include "cocos/renderer/pipeline/custom/details/GslUtils.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <stdexcept>

namespace cc::render::depth_stencil_test {
// These entry points only use the resource graph and the framebuffer capability.
struct NativePipeline {
    ResourceGraph& resourceGraph;
    bool defaultFramebufferHasDepthStencil{false};
    uint32_t addDepthStencil(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency);
    void updateDepthStencil(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format);
};

#include "PipelineDepthStencil.inc"
} // namespace cc::render::depth_stencil_test

bool checkPipelineDepthStencil(cc::gfx::Device& device) {
    using namespace cc::render;
    namespace gfx = cc::gfx;
    boost::container::pmr::monotonic_buffer_resource arena;
    ResourceGraph graph{&arena};
    depth_stencil_test::NativePipeline pipeline{graph};
    const ccstd::string name{"camera/depth-stencil"};
    const auto format = gfx::Format::DEPTH_STENCIL;
    const auto residency = ResourceResidency::MANAGED;
    const auto root = pipeline.addDepthStencil(name, format, 64, 32, residency);
    const auto depth = graph.vertex(name + "/depth");
    const auto stencil = graph.vertex(name + "/stencil");
    if (graph.vertex(name) != root || depth == stencil || graph.num_children(root) != 2 ||
        graph.get_parent(depth) != root || graph.get_parent(stencil) != root) return false;

    const auto checkMounted = [&](uint32_t width, uint32_t height) {
        graph.mount(&device, root);
        const auto* texture = graph.getTexture(root);
        if (!texture || texture->getWidth() != width || texture->getHeight() != height) return false;
        for (const auto child : {depth, stencil}) {
            const auto& desc = graph.getDesc(child);
            const auto* view = graph.getTexture(child);
            if (desc.width != width || desc.height != height || desc.format != format ||
                graph.getTraits(child).residency != residency || graph.getSampler(child).minFilter != gfx::Filter::POINT ||
                !view || !view->isTextureView()) return false;
            const auto& info = view->getViewInfo();
            if (info.texture != texture || info.format != format || info.planeCount != 1 ||
                info.basePlane != (child == depth ? 0U : 1U)) return false;
        }
        return true;
    };
    if (!checkMounted(64, 32)) return false;
    if (pipeline.addDepthStencil(name, format, 128, 64, residency) != root || !checkMounted(128, 64)) return false;
    pipeline.updateDepthStencil(name, 32, 16, gfx::Format::UNKNOWN);
    if (!checkMounted(32, 16) || graph.num_vertices() != 3 || graph.vertex(name + "/depth") != depth ||
        graph.vertex(name + "/stencil") != stencil) return false;

    const auto existing = graph.add_vertex(ManagedTexture_{}, "occupied/stencil");
    try {
        pipeline.addDepthStencil("occupied", format, 8, 8, residency);
        return false;
    } catch (const std::invalid_argument&) {
        return graph.num_vertices() == 4 && !graph.contains("occupied") && !graph.contains("occupied/depth") &&
               graph.vertex("occupied/stencil") == existing;
    }
}
