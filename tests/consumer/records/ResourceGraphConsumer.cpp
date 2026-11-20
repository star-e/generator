#include "cocos/renderer/pipeline/custom/RenderGraphTypes.h"
#include "cocos/renderer/gfx-empty/EmptyFramebuffer.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <type_traits>
#include <utility>

bool checkResourceGraph() {
    using namespace cc::render;
    static_assert(!std::is_move_constructible_v<ResourceGraph> && !std::is_copy_constructible_v<ResourceGraph>);
    static_assert(std::is_same_v<decltype(ResourceDesc::mipLevels), uint16_t>);
    static_assert(std::is_same_v<decltype(ManagedBuffer::fenceValue), uint64_t>);
    static_assert(std::is_same_v<decltype(&ManagedTexture::checkResource), bool (ManagedTexture::*)(const ResourceDesc&) const>);
    static_assert(std::is_same_v<decltype(&PersistentTexture::checkResource), bool (PersistentTexture::*)(const ResourceDesc&) const>);
    using SwapchainTexture = cc::gfx::Texture* (*)(cc::gfx::Swapchain*) noexcept;
    using WindowTexture = cc::gfx::Texture* (*)(cc::scene::RenderWindow*) noexcept;
    static_assert(std::is_same_v<decltype(static_cast<WindowTexture>(&RenderSwapchain::getColorTexture)), WindowTexture>);
    static_assert(std::is_same_v<decltype(static_cast<SwapchainTexture>(&RenderSwapchain::getColorTexture)), SwapchainTexture>);
    static_assert(std::is_same_v<decltype(&RenderSwapchain::getDepthStencilTexture), SwapchainTexture>);
    boost::container::pmr::monotonic_buffer_resource arena;
    ResourceGraph g{&arena};
    static_assert(std::is_same_v<decltype(g.getName(0)), const ccstd::pmr::string&>);
    static_assert(std::is_same_v<decltype(g.get(Name_{}, 0)), const ccstd::pmr::string&>);
    const auto none = ResourceGraph::null_vertex();
    if (g.contains("missing") || g.find("missing") != none) return false;
    try {
        (void)g.vertex("missing");
        return false;
    } catch (const std::out_of_range&) {}
    const auto v = g.add_vertex(Managed_{}, "color");
    if (v != 0 || !g.contains("color") || g.find("color") != v || g.vertex("color") != v) return false;
    try {
        (void)g.add_vertex(Framebuffer_{}, "color");
        return false;
    } catch (const std::invalid_argument&) {}
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
    const auto matches = [&](auto tag, ResourceGraph::Vertex vertex, unsigned ordinal) {
        const auto& read = std::as_const(g);
        return static_cast<unsigned>(read.kind(vertex)) == ordinal && read.holds<decltype(tag)>(vertex) &&
               &g.value(tag, vertex) == &read.value(tag, vertex);
    };
    const auto mb = g.add_vertex(ManagedBuffer_{}, "mb");
    try {
        g.setName(v, "mb");
        return false;
    } catch (const std::invalid_argument&) {}
    if (g.getName(v) != "color" || g.vertex("color") != v || g.vertex("mb") != mb || g.num_vertices() != 2) return false;
    try {
        g.setName(none, "invalid");
        return false;
    } catch (const std::out_of_range&) {}
    if (g.contains("invalid")) return false;
    g.setName(v, g.getName(v));
    g.setName(v, std::string_view{g.getName(v)}.substr(1));
    if (g.contains("color") || g.vertex("olor") != v) return false;
    constexpr std::string_view binaryName{"a\0b", 3};
    g.setName(v, binaryName);
    if (g.contains("olor") || g.contains("a") || g.vertex(binaryName) != v) return false;
    g.setName(v, "");
    if (g.contains(binaryName) || g.vertex("") != v || !g.get(Name_{}, v).empty()) return false;
    const auto mt = g.add_vertex(ManagedTexture_{}, "mt");
    const auto pb = g.add_vertex(PersistentBuffer_{}, "pb");
    const auto pt = g.add_vertex(PersistentTexture_{}, "pt");
    const auto fb = g.add_vertex(Framebuffer_{}, "fb");
    const auto sc = g.add_vertex(Swapchain_{}, "swapchain");
    const auto fmt = g.add_vertex(FormatView_{}, "fmt");
    const auto sub = g.add_vertex(SubresourceView_{}, "sub");
    if (g.num_vertices() != 9 || !matches(Managed_{}, v, 0) || !matches(ManagedBuffer_{}, mb, 1) ||
        !matches(ManagedTexture_{}, mt, 2) || !matches(PersistentBuffer_{}, pb, 3) ||
        !matches(PersistentTexture_{}, pt, 4) || !matches(Framebuffer_{}, fb, 5) || !matches(Swapchain_{}, sc, 6) ||
        !matches(FormatView_{}, fmt, 7) || !matches(SubresourceView_{}, sub, 8)) return false;
    if (g.value(ManagedBuffer_{}, mb).buffer.get() || g.value(ManagedTexture_{}, mt).texture.get() ||
        g.value(PersistentBuffer_{}, pb).buffer.get() || g.value(PersistentTexture_{}, pt).texture.get() ||
        g.value(Framebuffer_{}, fb).get() || g.value(SubresourceView_{}, sub).textureView.get()) return false;
    const auto& swapchain = g.value(Swapchain_{}, sc);
    if (swapchain.swapchain || swapchain.renderWindow || swapchain.texture || swapchain.isDepthStencil ||
        swapchain.generation != UINT32_MAX || g.value(FormatView_{}, fmt).format != cc::gfx::Format::UNKNOWN) return false;
    g.value(ManagedBuffer_{}, mb).fenceValue = UINT64_MAX;
    const auto mb2 = g.add_vertex(ManagedBuffer_{}, "mb2");
    if (g.value(ManagedBuffer_{}, mb2).fenceValue != 0 || g.value(ManagedBuffer_{}, mb).fenceValue != UINT64_MAX) return false;
    cc::IntrusivePtr<cc::gfx::Framebuffer> framebuffer{new cc::gfx::EmptyFramebuffer};
    const auto refs = framebuffer->getRefCount();
    g.value(Framebuffer_{}, fb) = framebuffer;
    if (g.value(Framebuffer_{}, fb).get() != framebuffer.get() || framebuffer->getRefCount() != refs + 1) return false;
    g.value(Framebuffer_{}, fb) = nullptr;
    if (framebuffer->getRefCount() != refs || g.holds<Managed_>(fb)) return false;
    try {
        (void)g.value(Managed_{}, fb);
        return false;
    } catch (const std::bad_variant_access&) {
        return true;
    }
}
