extern module Gfx;
cpp { header "cocos/renderer/gfx-base/GFXDef-common.h"; }

namespace cc::gfx {
    flags BarrierType : uint32_t { FULL, SPLIT_BEGIN, SPLIT_END }
    enum class LoadOp : uint32_t { LOAD, CLEAR, DISCARD }
    enum class StoreOp : uint32_t { STORE, DISCARD }
    flags ClearFlagBit : uint32_t {
        NONE = 0,
        COLOR = 0x1,
        DEPTH = 0x2,
        STENCIL = 0x4,
        DEPTH_STENCIL = 0x6,
        ALL = 0x7,
    }

    enum class TextureType : uint32_t {
        TEX1D,
        TEX2D,
        TEX3D,
        CUBE,
        TEX1D_ARRAY,
        TEX2D_ARRAY,
    }

    flags TextureFlagBit : uint32_t {
        NONE = 0,
        GEN_MIPMAP = 0x1,
        GENERAL_LAYOUT = 0x2,
        EXTERNAL_OES = 0x4,
        EXTERNAL_NORMAL = 0x8,
        LAZILY_ALLOCATED = 0x10,
        MUTABLE_VIEW_FORMAT = 0x40,
        MUTABLE_STORAGE = 0x80,
    }

    enum class SampleCount : uint32_t {
        X1 = 0x01,
        X2 = 0x02,
        X4 = 0x04,
        X8 = 0x08,
        X16 = 0x10,
        X32 = 0x20,
        X64 = 0x40
    }

    flags AccessFlagBit : uint32_t {
        NONE = 0,
        INDIRECT_BUFFER = 1 << 0,
        INDEX_BUFFER = 1 << 1,
        VERTEX_BUFFER = 1 << 2,
        VERTEX_SHADER_READ_UNIFORM_BUFFER = 1 << 3,
        VERTEX_SHADER_READ_TEXTURE = 1 << 4,
        VERTEX_SHADER_READ_OTHER = 1 << 5,
        FRAGMENT_SHADER_READ_UNIFORM_BUFFER = 1 << 6,
        FRAGMENT_SHADER_READ_TEXTURE = 1 << 7,
        FRAGMENT_SHADER_READ_COLOR_INPUT_ATTACHMENT = 1 << 8,
        FRAGMENT_SHADER_READ_DEPTH_STENCIL_INPUT_ATTACHMENT = 1 << 9,
        FRAGMENT_SHADER_READ_OTHER = 1 << 10,
        COLOR_ATTACHMENT_READ = 1 << 11,
        DEPTH_STENCIL_ATTACHMENT_READ = 1 << 12,
        COMPUTE_SHADER_READ_UNIFORM_BUFFER = 1 << 13,
        COMPUTE_SHADER_READ_TEXTURE = 1 << 14,
        COMPUTE_SHADER_READ_OTHER = 1 << 15,
        TRANSFER_READ = 1 << 16,
        HOST_READ = 1 << 17,
        PRESENT = 1 << 18,
        VERTEX_SHADER_WRITE = 1 << 19,
        FRAGMENT_SHADER_WRITE = 1 << 20,
        COLOR_ATTACHMENT_WRITE = 1 << 21,
        DEPTH_STENCIL_ATTACHMENT_WRITE = 1 << 22,
        COMPUTE_SHADER_WRITE = 1 << 23,
        TRANSFER_WRITE = 1 << 24,
        HOST_PREINITIALIZED = 1 << 25,
        HOST_WRITE = 1 << 26,
        SHADING_RATE = 1 << 27,
    }

    enum class Type : uint32_t {
        UNKNOWN,
        BOOL,
        BOOL2,
        BOOL3,
        BOOL4,
        INT,
        INT2,
        INT3,
        INT4,
        UINT,
        UINT2,
        UINT3,
        UINT4,
        FLOAT,
        FLOAT2,
        FLOAT3,
        FLOAT4,
        MAT2,
        MAT2X3,
        MAT2X4,
        MAT3X2,
        MAT3,
        MAT3X4,
        MAT4X2,
        MAT4X3,
        MAT4,
        SAMPLER1D,
        SAMPLER1D_ARRAY,
        SAMPLER2D,
        SAMPLER2D_ARRAY,
        SAMPLER3D,
        SAMPLER_CUBE,
        SAMPLER,
        TEXTURE1D,
        TEXTURE1D_ARRAY,
        TEXTURE2D,
        TEXTURE2D_ARRAY,
        TEXTURE3D,
        TEXTURE_CUBE,
        IMAGE1D,
        IMAGE1D_ARRAY,
        IMAGE2D,
        IMAGE2D_ARRAY,
        IMAGE3D,
        IMAGE_CUBE,
        SUBPASS_INPUT,
        COUNT,
    }

    enum class ResolveMode : uint32_t {
        NONE,
        SAMPLE_ZERO,
        AVERAGE,
        MIN,
        MAX,
    }

    flags ShaderStageFlagBit : uint32_t {
        NONE = 0x0,
        VERTEX = 0x1,
        CONTROL = 0x2,
        EVALUATION = 0x4,
        GEOMETRY = 0x8,
        FRAGMENT = 0x10,
        COMPUTE = 0x20,
        ALL = 0x3f,
    }

    flags MemoryAccessBit : uint32_t {
        NONE = 0,
        READ_ONLY = 0x1,
        WRITE_ONLY = 0x2,
        READ_WRITE = 0x3,
    }

    enum class ViewDimension : uint32_t {
        UNKNOWN,
        BUFFER,
        TEX1D,
        TEX1D_ARRAY,
        TEX2D,
        TEX2D_ARRAY,
        TEX2DMS,
        TEX2DMS_ARRAY,
        TEX3D,
        TEXCUBE,
        TEXCUBE_ARRAY,
        RAYTRACING_ACCELERATION_STRUCTURE,
    }

    enum class SampleType : uint32_t {
        FLOAT,
        UNFILTERABLE_FLOAT,
        SINT,
        UINT,
    }

    enum class Format : uint32_t {
        UNKNOWN,

        A8,
        L8,
        LA8,

        R8,
        R8SN,
        R8UI,
        R8I,
        R16F,
        R16UI,
        R16I,
        R32F,
        R32UI,
        R32I,

        RG8,
        RG8SN,
        RG8UI,
        RG8I,
        RG16F,
        RG16UI,
        RG16I,
        RG32F,
        RG32UI,
        RG32I,

        RGB8,
        SRGB8,
        RGB8SN,
        RGB8UI,
        RGB8I,
        RGB16F,
        RGB16UI,
        RGB16I,
        RGB32F,
        RGB32UI,
        RGB32I,

        RGBA8,
        BGRA8,
        SRGB8_A8,
        RGBA8SN,
        RGBA8UI,
        RGBA8I,
        RGBA16F,
        RGBA16UI,
        RGBA16I,
        RGBA32F,
        RGBA32UI,
        RGBA32I,

        R5G6B5,
        R11G11B10F,
        RGB5A1,
        RGBA4,
        RGB10A2,
        RGB10A2UI,
        RGB9E5,

        DEPTH,
        DEPTH_STENCIL,


        BC1,
        BC1_ALPHA,
        BC1_SRGB,
        BC1_SRGB_ALPHA,
        BC2,
        BC2_SRGB,
        BC3,
        BC3_SRGB,
        BC4,
        BC4_SNORM,
        BC5,
        BC5_SNORM,
        BC6H_UF16,
        BC6H_SF16,
        BC7,
        BC7_SRGB,

        ETC_RGB8,
        ETC2_RGB8,
        ETC2_SRGB8,
        ETC2_RGB8_A1,
        ETC2_SRGB8_A1,
        ETC2_RGBA8,
        ETC2_SRGB8_A8,
        EAC_R11,
        EAC_R11SN,
        EAC_RG11,
        EAC_RG11SN,

        PVRTC_RGB2,
        PVRTC_RGBA2,
        PVRTC_RGB4,
        PVRTC_RGBA4,
        PVRTC2_2BPP,
        PVRTC2_4BPP,

        ASTC_RGBA_4X4,
        ASTC_RGBA_5X4,
        ASTC_RGBA_5X5,
        ASTC_RGBA_6X5,
        ASTC_RGBA_6X6,
        ASTC_RGBA_8X5,
        ASTC_RGBA_8X6,
        ASTC_RGBA_8X8,
        ASTC_RGBA_10X5,
        ASTC_RGBA_10X6,
        ASTC_RGBA_10X8,
        ASTC_RGBA_10X10,
        ASTC_RGBA_12X10,
        ASTC_RGBA_12X12,

        ASTC_SRGBA_4X4,
        ASTC_SRGBA_5X4,
        ASTC_SRGBA_5X5,
        ASTC_SRGBA_6X5,
        ASTC_SRGBA_6X6,
        ASTC_SRGBA_8X5,
        ASTC_SRGBA_8X6,
        ASTC_SRGBA_8X8,
        ASTC_SRGBA_10X5,
        ASTC_SRGBA_10X6,
        ASTC_SRGBA_10X8,
        ASTC_SRGBA_10X10,
        ASTC_SRGBA_12X10,
        ASTC_SRGBA_12X12,

        COUNT,
    }

    struct ResourceRange;
    struct RenderPassInfo;
    struct FramebufferInfo;
    struct BufferInfo;
    struct TextureInfo;
    struct ShaderInfo;
    struct Attribute;
    struct SamplerInfo;
    struct Viewport;
    struct UniformBlock;
    struct DescriptorSetLayoutInfo;

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
        class DescriptorSetLayout;
        class DescriptorSet;
        class PipelineLayout;
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
    LoadOp => LoadOp from "cocos/gfx";
    StoreOp => StoreOp from "cocos/gfx";
    ClearFlagBit => ClearFlagBit from "cocos/gfx";
    TextureType => TextureType from "cocos/gfx";
    TextureFlagBit => TextureFlagBit from "cocos/gfx";
    SampleCount => SampleCount from "cocos/gfx";
    AccessFlagBit => AccessFlagBit from "cocos/gfx";
    Type => Type from "cocos/gfx";
    ResolveMode => ResolveMode from "cocos/gfx";
    ShaderStageFlagBit => ShaderStageFlagBit from "cocos/gfx";
    MemoryAccessBit => MemoryAccessBit from "cocos/gfx";
    ViewDimension => ViewDimension from "cocos/gfx";
    SampleType => SampleType from "cocos/gfx";
    Format => Format from "cocos/gfx";
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
    UniformBlock => UniformBlock from "cocos/gfx" archive from "cocos/rendering/custom/serialization";
    DescriptorSetLayoutInfo => DescriptorSetLayoutInfo from "cocos/gfx" archive from "cocos/rendering/custom/serialization";
    DescriptorSetLayout => DescriptorSetLayout from "cocos/gfx";
    DescriptorSet => DescriptorSet from "cocos/gfx";
    PipelineLayout => PipelineLayout from "cocos/gfx";
}
