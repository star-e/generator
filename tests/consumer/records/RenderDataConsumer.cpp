#include "cocos/renderer/pipeline/custom/RenderGraphTypes.h"
#include "cocos/renderer/gfx-empty/EmptyBuffer.h"
#include "cocos/renderer/gfx-empty/EmptyTexture.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <type_traits>

namespace {
using namespace cc::render;
namespace gfx = cc::gfx;
using Arena = boost::container::pmr::monotonic_buffer_resource;

bool checkRenderData() {
    static_assert(!std::is_default_constructible_v<RenderData> && !std::is_copy_assignable_v<RenderData>);
    static_assert(!std::is_constructible_v<RenderData, const RenderData&, RenderData::allocator_type>);
    static_assert(std::is_move_assignable_v<RenderData> && std::is_move_constructible_v<RenderData>);
    cc::IntrusivePtr<gfx::Buffer> buffer{new gfx::EmptyBuffer};
    cc::IntrusivePtr<gfx::Texture> texture{new gfx::EmptyTexture};
    gfx::Sampler sampler{gfx::SamplerInfo{}};
    Arena destination;
    const auto bufferRefs = buffer->getRefCount(), textureRefs = texture->getRefCount();
    {
        RenderData data = [&] {
            Arena scratch;
            RenderData source{&scratch};
            source.constants[3] = {'a', '\0', 'z'};
            source.buffers.emplace(4, buffer);
            source.textures.emplace(5, texture);
            source.samplers.emplace(6, &sampler);
            source.custom = "custom-render-node-data";
            return RenderData{std::move(source), &destination};
        }();
        if (data.get_allocator().resource() != &destination || data.constants.at(3).get_allocator().resource() != &destination ||
            data.custom.get_allocator().resource() != &destination || data.custom != "custom-render-node-data") return false;
        const auto& bytes = data.constants.at(3);
        if (bytes.size() != 3 || bytes[0] != 'a' || bytes[1] != '\0' || bytes[2] != 'z' ||
            data.buffers.at(4).get() != buffer.get() || data.textures.at(5).get() != texture.get() ||
            data.samplers.at(6) != &sampler || buffer->getRefCount() != bufferRefs + 1 ||
            texture->getRefCount() != textureRefs + 1) return false;
    }
    return buffer->getRefCount() == bufferRefs && texture->getRefCount() == textureRefs;
}
} // namespace

bool checkRenderNodeData() {
    using namespace cc::render;
    static_assert(std::is_same_v<decltype(SceneData::scene), const cc::scene::RenderScene*>);
    static_assert(std::is_same_v<decltype(SceneData::camera), const cc::scene::Camera*>);
    static_assert(std::is_same_v<decltype(SceneData::shadingLight), cc::IntrusivePtr<cc::scene::Light>>);
    static_assert(std::is_same_v<decltype(Dispatch::material), cc::IntrusivePtr<cc::Material>>);
    static_assert(std::is_same_v<decltype(Blit::camera), const cc::scene::Camera*>);
    static_assert(std::is_same_v<decltype(Blit::models)::value_type, cc::IntrusivePtr<cc::scene::Model>>);
    static_assert(std::is_same_v<decltype(RenderQueue::viewport), gfx::Viewport>);
    static_assert(std::is_trivially_copy_constructible_v<gfx::Viewport> && std::is_trivially_copy_constructible_v<gfx::Color>);
    static_assert(any(CullingFlags::CAMERA_FRUSTUM & (CullingFlags::LIGHT_BOUNDS | CullingFlags::CAMERA_FRUSTUM)));
    static_assert(static_cast<uint8_t>(BlitType::DRAW_3D) == 3);
    Arena arena;
    const gfx::Color color{0.F, 0.25F, 0.5F, 1.F};
    ClearView clear{std::allocator_arg, &arena, "color", gfx::ClearFlagBit::COLOR, color};
    RenderQueue queue{3, 5};
    RenderQueue blend{QueueHint::BLEND, 7, 8};
    LightInfo light;
    light.level = 2;
    SceneData scene{nullptr, nullptr, SceneFlags::OPAQUE, light, CullingFlags::LIGHT_FRUSTUM, nullptr};
    Dispatch dispatch{nullptr, 1, 2, 3, 4};
    Blit quad{std::allocator_arg, &arena, nullptr, 7, SceneFlags::BLEND, nullptr, BlitType::FULLSCREEN_QUAD};
    ccstd::pmr::vector<cc::IntrusivePtr<cc::scene::Model>> models{0U, &arena};
    models.emplace_back();
    Blit draw{nullptr, BlitType::DRAW_3D, std::move(models), &arena};
    return clear.slotName == "color" && clear.clearColor.y == color.y && clear.get_allocator().resource() == &arena &&
           queue.hint == QueueHint::RENDER_OPAQUE && queue.phaseID == 3 && queue.passLayoutID == 5 &&
           queue.viewport.maxDepth == 1.F && blend.hint == QueueHint::BLEND &&
           scene.light.level == 2 && scene.cullingFlags == CullingFlags::LIGHT_FRUSTUM &&
           dispatch.passID == 1 && dispatch.threadGroupCountX == 2 && dispatch.threadGroupCountY == 3 && dispatch.threadGroupCountZ == 4 &&
           quad.passID == 7 && quad.models.empty() && draw.blitType == BlitType::DRAW_3D && draw.models.size() == 1 && checkRenderData();
}
