extern module GfxResource;
namespace cc::gfx {
    struct SamplerInfo;
    struct Viewport;
    @constRef struct Color;
    class RenderPass;
    class Buffer;
    class Texture;
    class Framebuffer;
    class Sampler;
    class Swapchain;
}

binding typescript {
    SamplerInfo => SamplerInfo from "cocos/gfx";
    Viewport => Viewport from "cocos/gfx";
    Color => Color from "cocos/gfx";
    RenderPass => RenderPass from "cocos/gfx";
    Buffer => Buffer from "cocos/gfx";
    Texture => Texture from "cocos/gfx";
    Framebuffer => Framebuffer from "cocos/gfx";
    Sampler => Sampler from "cocos/gfx";
    Swapchain => Swapchain from "cocos/gfx";
}
