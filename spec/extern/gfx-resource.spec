extern module GfxResource;
namespace cc::gfx {
    header "cocos/renderer/gfx-base/GFXDef-common.h" {
        struct ResourceRange;
        struct RenderPassInfo;
        struct FramebufferInfo;
        struct BufferInfo;
        struct TextureInfo;
        struct ShaderInfo;
        struct Attribute;
        struct SamplerInfo;
        struct Viewport;
    }

    header "cocos/renderer/gfx-base/GFX{name}.h" {
        class CommandBuffer;
        class Shader;
        class PipelineState;
        class InputAssembler;
        class Device;
        class RenderPass;
        class Buffer;
        class Texture;
        class Framebuffer;
        class Swapchain;
    }

    header "cocos/renderer/gfx-base/GFXObject.h" {
        class GFXObject;
    }

    header "cocos/renderer/gfx-base/GFXDef.h" {
        @constRef struct Color;
    }

    header "cocos/renderer/gfx-base/states/GFXSampler.h" {
        class Sampler;
    }
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
