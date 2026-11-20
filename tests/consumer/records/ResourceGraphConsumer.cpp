#include "cocos/renderer/pipeline/custom/RenderGraphTypes.h"
#include "cocos/renderer/gfx-empty/EmptyFramebuffer.h"
#include "cocos/renderer/gfx-empty/EmptyRenderPass.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <type_traits>
#include <utility>

namespace {
template<class... Fields>
ccstd::hash_t hashFields(const Fields&... fields) {
    ccstd::hash_t seed = 0;
    (ccstd::hash_combine(seed, fields), ...);
    return seed;
}

bool checkRenderViews() {
    using namespace cc::render;
    namespace gfx = cc::gfx;
    const ClearValue value{1, 2, 3, 4};
    if (value == ClearValue{} || value == ClearValue{1, 2, 3, 5} ||
        ccstd::hash<ClearValue>{}(value) != hashFields(value.x, value.y, value.z, value.w)) return false;

    boost::container::pmr::monotonic_buffer_resource arena, other;
    const gfx::Color color{0.25F, 0.5F, 0.75F, 1.F};
    RasterView raster{"color", "resolve", AccessType::WRITE, AttachmentType::RENDER_TARGET,
                      gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, gfx::ClearFlagBit::COLOR, color,
                      gfx::ShaderStageFlagBit::FRAGMENT, &arena};
    RasterView rasterCopy{raster, &other};
    rasterCopy.slotID = 9;
    const auto rasterHash = hashFields(raster.slotName, raster.slotName1, raster.accessType, raster.attachmentType,
                                       raster.loadOp, raster.storeOp, raster.clearFlags, raster.clearColor, raster.shaderStageFlags);
    if (rasterCopy.slotName1.get_allocator().resource() != &other || rasterCopy.slotName1 != "resolve" ||
        raster != rasterCopy || ccstd::hash<RasterView>{}(raster) != rasterHash ||
        ccstd::hash<RasterView>{}(rasterCopy) != rasterHash) return false;
    rasterCopy.clearColor.w = 0.F;
    if (raster == rasterCopy) return false;
    RasterView singleSlot{"color", raster.accessType, raster.attachmentType, raster.loadOp, raster.storeOp,
                          raster.clearFlags, color, raster.shaderStageFlags, &arena};
    if (!singleSlot.slotName1.empty() || singleSlot == raster) return false;

    ComputeView compute{"storage", AccessType::READ_WRITE, 3, gfx::ClearFlagBit::COLOR,
                        ClearValueType::FLOAT_TYPE, value, gfx::ShaderStageFlagBit::COMPUTE, &arena};
    ComputeView computeCopy{compute, &other};
    if (computeCopy.clearValue != value || computeCopy.get_allocator().resource() != &other ||
        !compute.isRead() || !compute.isWrite()) return false;
    computeCopy.clearValue = {};
    const auto computeHash = hashFields(compute.name, compute.accessType, compute.plane, compute.clearFlags,
                                        compute.clearValueType, compute.shaderStageFlags);
    if (compute != computeCopy || ccstd::hash<ComputeView>{}(compute) != computeHash ||
        ccstd::hash<ComputeView>{}(computeCopy) != computeHash) return false;
    computeCopy.clearValueType = ClearValueType::INT_TYPE;
    if (compute == computeCopy) return false;
    ComputeView firstPlane{"storage", AccessType::READ, compute.clearFlags, compute.clearValueType,
                           value, compute.shaderStageFlags, &arena};
    if (firstPlane.plane != 0 || !firstPlane.isRead() || firstPlane.isWrite()) return false;
    firstPlane.accessType = AccessType::WRITE;
    return !firstPlane.isRead() && firstPlane.isWrite();
}

bool checkPersistentRenderPassAndFramebuffer() {
    using cc::render::PersistentRenderPassAndFramebuffer;
    static_assert(!std::is_default_constructible_v<PersistentRenderPassAndFramebuffer>);
    boost::container::pmr::monotonic_buffer_resource arena, other;
    PersistentRenderPassAndFramebuffer empty{&arena};
    if (empty.get_allocator().resource() != &arena || empty.renderPass.get() || empty.framebuffer.get() ||
        !empty.clearColors.empty() || empty.clearDepth != 0.F || empty.clearStencil != 0) return false;

    cc::IntrusivePtr<cc::gfx::RenderPass> pass{new cc::gfx::EmptyRenderPass};
    cc::IntrusivePtr<cc::gfx::Framebuffer> framebuffer{new cc::gfx::EmptyFramebuffer};
    const auto passRefs = pass->getRefCount();
    const auto framebufferRefs = framebuffer->getRefCount();
    {
        PersistentRenderPassAndFramebuffer cached{pass, framebuffer, &arena};
        if (cached.clearColors.get_allocator().resource() != &arena ||
            pass->getRefCount() != passRefs + 1 || framebuffer->getRefCount() != framebufferRefs + 1) return false;
        cached.clearColors.push_back({0.25F, 0.5F, 0.75F, 1.F});
        cached.clearDepth = 0.5F;
        cached.clearStencil = 7;
        PersistentRenderPassAndFramebuffer copy{cached, &other};
        if (copy.clearColors.get_allocator().resource() != &other || copy.clearColors.size() != 1) return false;
        cached.clearColors.front().x = 1.F;
        PersistentRenderPassAndFramebuffer moved{std::move(copy), &arena};
        if (moved.clearColors.get_allocator().resource() != &arena || moved.clearColors.size() != 1 ||
            moved.clearColors.front().x != 0.25F || moved.renderPass.get() != pass.get() ||
            moved.framebuffer.get() != framebuffer.get() || moved.clearDepth != 0.5F || moved.clearStencil != 7) return false;
    }
    return pass->getRefCount() == passRefs && framebuffer->getRefCount() == framebufferRefs;
}

bool checkViewHierarchy() {
    using namespace cc::render;
    boost::container::pmr::monotonic_buffer_resource arena;
    ResourceGraph g{&arena};
    const auto none = ResourceGraph::null_vertex();
    const auto depth = g.add_vertex(SubresourceView_{}, "depth");
    const auto texture = g.add_vertex(ManagedTexture_{}, "texture");
    const auto stencil = g.add_vertex(SubresourceView_{}, "stencil", texture);
    const auto mip = g.add_vertex(FormatView_{}, "depth-mip", depth);

    // Attach an existing subtree to a parent with a larger vertex descriptor.
    const auto [link, attached] = g.add_edge(texture, depth);
    if (!attached || g.source(link) != texture || g.target(link) != depth) return false;
    if (g.add_edge(texture, depth).second || g.add_edge(stencil, depth).second ||
        g.add_edge(texture, texture).second || g.add_edge(mip, texture).second ||
        g.add_edge(none, texture).second) return false;
    try {
        (void)g.add_vertex(SubresourceView_{}, "orphan", g.num_vertices());
        return false;
    } catch (const std::out_of_range&) {}
    if (g.contains("orphan") || g.num_vertices() != 4 || g.num_edges() != 3 || g.get_parent(texture) != none ||
        g.get_parent(depth) != texture || g.get_parent(stencil) != texture || g.get_parent(mip) != depth ||
        g.vertex("texture") != texture || g.vertex("depth-mip") != mip) return false;

    if (g.num_children(texture) != 2 || g.out_degree(texture) != 2 || g.in_degree(depth) != 1) return false;
    bool sawDepth = false, sawStencil = false;
    for (const auto e : g.child_links(texture)) {
        if (g.parent(e) != texture || !g.edge(texture, g.child(e)).second) return false;
        sawDepth |= g.child(e) == depth;
        sawStencil |= g.child(e) == stencil;
    }
    if (!sawDepth || !sawStencil) return false;
    g.remove_edge(link);
    return !g.has_link(texture, depth) && g.get_parent(depth) == none && g.get_parent(mip) == depth &&
           g.has_link(texture, stencil) && g.num_edges() == 2 && g.vertex("depth") == depth;
}
} // namespace

bool checkResourceGraph() {
    using namespace cc::render;
    if (!checkViewHierarchy() || !checkPersistentRenderPassAndFramebuffer() || !checkRenderViews()) return false;
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
    static_assert(std::is_same_v<decltype(g.nextFenceValue), uint64_t> && std::is_same_v<decltype(g.version), uint64_t>);
    if (g.nextFenceValue != 0 || g.version != 0) return false;
    g.nextFenceValue = UINT64_MAX - 1;
    ++g.nextFenceValue;
    g.version = UINT64_MAX;
    static_assert(std::is_same_v<decltype(g.getName(0)), const ccstd::pmr::string&>);
    static_assert(std::is_same_v<decltype(g.get(Name_{}, 0)), const ccstd::pmr::string&>);
    const auto none = ResourceGraph::null_vertex();
    if (g.contains("missing") || g.find("missing") != none) return false;
    try {
        (void)g.vertex("missing");
        return false;
    } catch (const std::out_of_range&) {}
    const auto v = g.add_vertex(Managed_{}, "color");
    if (g.nextFenceValue != UINT64_MAX || g.version != UINT64_MAX) return false;
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
        const void* visited = read.visit_object(vertex, [](auto& payload) -> const void* {
            static_assert(std::is_const_v<std::remove_reference_t<decltype(payload)>>);
            return &payload;
        });
        return static_cast<unsigned>(read.kind(vertex)) == ordinal && read.holds<decltype(tag)>(vertex) &&
               visited == &read.value(tag, vertex) && &g.value(tag, vertex) == &read.value(tag, vertex);
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
    if (!matches(ManagedBuffer_{}, mb2, 1) || g.value(ManagedBuffer_{}, mb2).fenceValue != 0 ||
        g.value(ManagedBuffer_{}, mb).fenceValue != UINT64_MAX) return false;
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
