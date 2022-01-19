/*
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2022 Xiamen Yaji Software Co., Ltd.

http://www.cocos.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated engine source code (the "Software"), a limited,
worldwide, royalty-free, non-assignable, revocable and non-exclusive license
to use Cocos Creator solely to develop games on your target platforms. You shall
not use Cocos Creator software for developing other software or tools that's
used for developing games. You are not granted to publish, distribute,
sublicense, and/or sell copies of Cocos Creator.

The software or tools in this License Agreement are licensed, not sold.
Xiamen Yaji Software Co., Ltd. reserves all rights not expressly granted to you.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "CocosModules.h"
#include <Cocos/AST/DSL.h>
#include <Cocos/AST/SyntaxGraphs.h>

namespace Cocos::Meta {

void buildRenderCommon(ModuleBuilder& builder, Features features) {
    MODULE(Camera,
        .mFolder = "cocos/core/renderer/scene",
        .mFilePrefix = "camera",
        .mAPI = "CC_DLL") {
        NAMESPACE(cc) {
            IMPORT_CLASS(Camera);
        }
    }
    MODULE(Ambient,
        .mFolder = "cocos/core/renderer/scene",
        .mFilePrefix = "ambient",
        .mAPI = "CC_DLL") {
        NAMESPACE(cc) {
            IMPORT_CLASS(Ambient);
        }
    }
    MODULE(Fog,
        .mFolder = "cocos/core/renderer/scene",
        .mFilePrefix = "fog",
        .mAPI = "CC_DLL") {
        NAMESPACE(cc) {
            IMPORT_CLASS(Fog);
        }
    }
    MODULE(Shadows,
        .mFolder = "cocos/core/renderer/scene",
        .mFilePrefix = "shadows",
        .mAPI = "CC_DLL") {
        NAMESPACE(cc) {
            IMPORT_CLASS(Shadows);
        }
    }
    MODULE(PipelineSceneData,
        .mFolder = "cocos/core/pipeline",
        .mFilePrefix = "pipeline-scene-data",
        .mAPI = "CC_DLL") {
        NAMESPACE(cc) {
            IMPORT_CLASS(PipelineSceneData);
        }
    }

    MODULE(Gfx,
        .mFolder = "cocos/core/gfx",
        .mFilePrefix = "index",
        .mAPI = "CC_DLL") {
        NAMESPACE(cc) {
            NAMESPACE(gfx) {
                // define
                IMPORT_ENUM(ObjectType);
                IMPORT_ENUM(Status);
                IMPORT_ENUM(API);
                IMPORT_ENUM(SurfaceTransform);
                IMPORT_ENUM(Feature);
                IMPORT_ENUM(Format);
                IMPORT_ENUM(FormatType);
                IMPORT_ENUM(Type);

                IMPORT_FLAGS(BufferUsageBit);
                IMPORT_FLAGS(BufferFlagBit);
                IMPORT_FLAGS(MemoryAccessBit);
                IMPORT_FLAGS(MemoryUsageBit);

                IMPORT_ENUM(TextureType);

                IMPORT_FLAGS(TextureUsageBit);
                IMPORT_FLAGS(TextureFlagBit);

                IMPORT_ENUM(SampleCount);
                IMPORT_ENUM(VsyncMode);
                IMPORT_ENUM(Filter);
                IMPORT_ENUM(Address);
                IMPORT_ENUM(ComparisonFunc);
                IMPORT_ENUM(StencilOp);
                IMPORT_ENUM(BlendFactor);
                IMPORT_ENUM(BlendOp);

                IMPORT_FLAGS(ColorMask);
                IMPORT_FLAGS(ShaderStageFlagBit);

                IMPORT_ENUM(LoadOp);
                IMPORT_ENUM(StoreOp);

                IMPORT_ENUM(AccessType);
                IMPORT_ENUM(ResolveMode);

                IMPORT_ENUM(PipelineBindPoint);
                IMPORT_ENUM(PrimitiveMode);

                IMPORT_ENUM(PolygonMode);
                IMPORT_ENUM(ShadeModel);
                IMPORT_ENUM(CullMode);

                IMPORT_FLAGS(DynamicStateFlagBit);
                IMPORT_FLAGS(StencilFace);
                IMPORT_FLAGS(DescriptorType);

                IMPORT_ENUM(QueueType);
                IMPORT_ENUM(QueryType);
                IMPORT_ENUM(CommandBufferType);

                IMPORT_FLAGS(ClearFlagBit);

                IMPORT_CLASS(Size);
                IMPORT_CLASS(DeviceCaps);
                IMPORT_CLASS(Offset);
                IMPORT_CLASS(Rect);
                IMPORT_CLASS(Extent);
                IMPORT_CLASS(TextureSubresLayers);
                IMPORT_CLASS(TextureSubresRange);
                IMPORT_CLASS(TextureCopy);
                IMPORT_CLASS(TextureBlit);
                IMPORT_CLASS(BufferTextureCopy);
                IMPORT_CLASS(Viewport);
                IMPORT_CLASS(Color);
                IMPORT_CLASS(BindingMappingInfo);
                IMPORT_CLASS(SwapchainInfo);
                IMPORT_CLASS(DeviceInfo);
                IMPORT_CLASS(BufferInfo);
                IMPORT_CLASS(BufferViewInfo);
                IMPORT_CLASS(DrawInfo);
                IMPORT_CLASS(DispatchInfo);
                IMPORT_CLASS(IndirectBuffer);
                IMPORT_CLASS(TextureInfo);
                IMPORT_CLASS(TextureViewInfo);
                IMPORT_CLASS(SamplerInfo);
                IMPORT_CLASS(Uniform);
                IMPORT_CLASS(UniformBlock);
                IMPORT_CLASS(UniformSamplerTexture);
                IMPORT_CLASS(UniformSampler);
                IMPORT_CLASS(UniformTexture);
                IMPORT_CLASS(UniformStorageImage);
                IMPORT_CLASS(UniformStorageBuffer);
                IMPORT_CLASS(UniformInputAttachment);
                IMPORT_CLASS(ShaderStage);
                IMPORT_CLASS(Attribute);
                IMPORT_CLASS(ShaderInfo);
                IMPORT_CLASS(InputAssemblerInfo);
                IMPORT_CLASS(ColorAttachment);
                IMPORT_CLASS(DepthStencilAttachment);
                IMPORT_CLASS(SubpassInfo);
                IMPORT_CLASS(SubpassDependency);
                IMPORT_CLASS(RenderPassInfo);
                IMPORT_CLASS(GlobalBarrierInfo);
                IMPORT_CLASS(TextureBarrierInfo);
                IMPORT_CLASS(FramebufferInfo);
                IMPORT_CLASS(DescriptorSetLayoutBinding);
                IMPORT_CLASS(DescriptorSetLayoutInfo);
                IMPORT_CLASS(DescriptorSetInfo);
                IMPORT_CLASS(PipelineLayoutInfo);
                IMPORT_CLASS(InputState);
                IMPORT_CLASS(CommandBufferInfo);
                IMPORT_CLASS(QueueInfo);
                IMPORT_CLASS(QueryPoolInfo);
                IMPORT_CLASS(FormatInfo);
                IMPORT_CLASS(MemoryStatus);
                IMPORT_CLASS(DynamicStencilStates);
                IMPORT_CLASS(DynamicStates);

                IMPORT_CLASS(GFXObject);

                IMPORT_FLAGS(AttributeName);
                // define end

                IMPORT_CLASS(DescriptorSet);
                IMPORT_CLASS(Buffer);
                IMPORT_CLASS(CommandBuffer);
                IMPORT_CLASS(Device);
                IMPORT_CLASS(Swapchain);
                IMPORT_CLASS(Framebuffer);
                IMPORT_CLASS(InputAssembler);
                IMPORT_CLASS(DescriptorSetLayout);
                IMPORT_CLASS(PipelineLayout);

                IMPORT_CLASS(RasterizerState);
                IMPORT_CLASS(DepthStencilState);
                IMPORT_CLASS(BlendTarget);
                IMPORT_CLASS(BlendState);
                IMPORT_CLASS(PipelineStateInfo);
                IMPORT_CLASS(PipelineState);

                IMPORT_CLASS(Queue);

                IMPORT_CLASS(RenderPass);
                IMPORT_CLASS(Sampler);

                IMPORT_CLASS(Shader);
                IMPORT_CLASS(Texture);

                IMPORT_CLASS(GlobalBarrier);
                IMPORT_CLASS(TextureBarrier);
            } // namespace gfx
        }
    }

    MODULE(RenderCommon,
        .mFolder = "cocos/core/pipeline",
        .mFilePrefix = "types",
        .mAPI = "CC_DLL") {
        NAMESPACE(cc) {
            NAMESPACE(render) {
                ENUM(UpdateFrequency) {
                    ENUMS(PerInstance, PerBatch, PerQueue, PerPass, Count);
                }

                TAGS((_), CBV_, UAV_, SRV_, SSV_, RTV_, DSV_, IBV_, VBV_, SOV_, Constants_, Table_);
                VARIANT(ParameterType, (Constants_, CBV_, UAV_, SRV_, Table_, SSV_), EQUAL | LESS);

                TAGS((_), Bounded_, Unbounded_);
                VARIANT(Boundedness, (Bounded_, Unbounded_), EQUAL | LESS);

                TAGS((_), CBuffer_, Buffer_, Texture_, RWBuffer_, RWTexture_, Sampler_,
                    Texture1D_, Texture1DArray_,
                    Texture2D_, Texture2DArray_, Texture2DMS_, Texture2DMSArray_,
                    Texture3D_, TextureCube_, TextureCubeArray_,
                    RaytracingAccelerationStructure_,
                    SamplerState_, SamplerComparisonState_);

                VARIANT(ResourceType, (Constants_, Buffer_, Texture1D_, Texture1DArray_, Texture2D_, Texture2DArray_, Texture2DMS_, Texture2DMSArray_, Texture3D_, TextureCube_, TextureCubeArray_, RaytracingAccelerationStructure_, SamplerState_, SamplerComparisonState_));

                TAGS((_), Typeless_,
                    Float4_, Float3_, Float2_, Float1_,
                    Half4_, Half3_, Half2_, Half1_,
                    Fixed4_, Fixed3_, Fixed2_, Fixed1_,
                    Uint4_, Uint3_, Uint2_, Uint1_,
                    Int4_, Int3_, Int2_, Int1_,
                    Bool4_, Bool3_, Bool2_, Bool1_);

                VARIANT(ValueType, (Typeless_, Float4_, Float3_, Float2_, Float1_, Half4_, Half3_, Half2_, Half1_, Fixed4_, Fixed3_, Fixed2_, Fixed1_, Uint4_, Uint3_, Uint2_, Uint1_, Int4_, Int3_, Int2_, Int1_, Bool4_, Bool3_, Bool2_, Bool1_));

                TAGS((_), Raster_, Compute_, Copy_, Move_, Raytrace_);

                TAGS((_), Managed_, Persistent_, Backbuffer_, Memoryless_);
                VARIANT(ResourceResidency, (Managed_, Persistent_, Backbuffer_, Memoryless_));

                ENUM(QueueHint) {
                    ENUMS(Opaque, Cutout, Transparent, Count);
                }
                ENUM(ResourceDimension) {
                    ENUMS(Buffer, Texture1D, Texture2D, Texture3D);
                }

                STRUCT(SampleDesc) {
                    PUBLIC(
                        (uint32_t, mCount, 1)(uint32_t, mQuality, 0));
                }

                ENUM(NodeType) {
                    ENUMS(Internal, Leaf);
                }
            } // namespace render
        }
    } // module CoreRender
}

}
