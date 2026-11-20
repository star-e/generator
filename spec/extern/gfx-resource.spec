extern module GfxResource;
namespace cc::gfx {
    struct ResourceRange;
    struct RenderPassInfo;
    struct FramebufferInfo;
    struct BufferInfo;
    struct TextureInfo;
    struct ShaderInfo;
    struct Attribute;
    class CommandBuffer;
    class Shader;
    class PipelineState;
    class GFXObject;
    class InputAssembler;
    struct SamplerInfo;
    struct Viewport;
    @constRef struct Color;
    class Device;
    class RenderPass;
    class Buffer;
    class Texture;
    class Framebuffer;
    class Sampler;
    class Swapchain;
}

binding typescript {
    ShaderInfo => ShaderInfo from "cocos/gfx";
    Device => Device from "cocos/gfx";
    Shader => Shader from "cocos/gfx";
    Attribute => Attribute from "cocos/gfx";
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
