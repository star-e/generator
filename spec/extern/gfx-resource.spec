extern module GfxResource;
namespace cc::gfx {
    struct SamplerInfo;
    class Buffer;
    class Texture;
    class Framebuffer;
    class Swapchain;
}

binding typescript {
    SamplerInfo => SamplerInfo from "cocos/gfx";
    Buffer => Buffer from "cocos/gfx";
    Texture => Texture from "cocos/gfx";
    Framebuffer => Framebuffer from "cocos/gfx";
    Swapchain => Swapchain from "cocos/gfx";
}
