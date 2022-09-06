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

void buildCocosCommon(ModuleBuilder& builder, Features features) {
    NAMESPACE(cc) {
        CONTAINER(IntrusivePtr, .mFlags = JSB);
        PROJECT_TS(IntrusivePtr, _);
    }
    MODULE(PassUtils,
        .mTypescriptFolder = "cocos/core/renderer/core",
        .mTypescriptFilePrefix = "pass-utils",
    ) {
        ADD_FEATURES(ToJs);
        NAMESPACE(cc) {
            IMPORT_CLASS(MacroRecord);
        }
    }

    MODULE(Scene,
        .mTypescriptFolder = "cocos/core/renderer",
        .mTypescriptFilePrefix = "scene", ) {
        ADD_FEATURES(ToJs);
        NAMESPACE(cc) {
            NAMESPACE(scene) {
                IMPORT_CLASS(Pass);
                IMPORT_CLASS(SubModel);
                IMPORT_CLASS(Model);
                IMPORT_CLASS(Light, .mFlags = JSB);
            }
        }
    }

    MODULE(RenderWindow,
        .mTypescriptFolder = "cocos/core/renderer/core",
        .mTypescriptFilePrefix = "render-window", ) {
        ADD_FEATURES(ToJs);
        NAMESPACE(cc) {
            NAMESPACE(scene) {
                IMPORT_CLASS(RenderWindow);
            }
        }
    }

    MODULE(RenderScene,
        .mTypescriptFolder = "cocos/core/renderer/core",
        .mTypescriptFilePrefix = "render-scene",
    ) {
        ADD_FEATURES(ToJs);
        NAMESPACE(cc) {
            NAMESPACE(scene) {
                IMPORT_CLASS(RenderScene);
            }
        }
    }

	MODULE(Math,
        .mTypescriptFolder = "cocos/core",
        .mTypescriptFilePrefix = "math", ) {
        ADD_FEATURES(ToJs);

        NAMESPACE(cc) {
            IMPORT_CLASS(Mat4);
            IMPORT_CLASS(Mat3);
            IMPORT_CLASS(Quaternion);
            PROJECT_TS(Quaternion, Quat);
            IMPORT_CLASS(Color);
            IMPORT_CLASS(Vec4);
            IMPORT_CLASS(Vec3);
            IMPORT_CLASS(Vec2);
            IMPORT_CLASS(Size);
            IMPORT_CLASS(Rect);
        }
    }

    MODULE(Camera,
        .mFolder = "cocos/scene",
        .mFilePrefix = "Camera.h",
        .mTypescriptFolder = "cocos/core/renderer/scene",
        .mTypescriptFilePrefix = "camera",
    ) {
        ADD_FEATURES(ToJs);

        NAMESPACE(cc) {
            NAMESPACE(scene) {
                IMPORT_STRUCT(Camera);
            }
        }
    }
    MODULE(Ambient,
        .mTypescriptFolder = "cocos/core/renderer/scene",
        .mTypescriptFilePrefix = "ambient",
    ) {
        NAMESPACE(cc) {
            IMPORT_CLASS(Ambient);
        }
    }
    MODULE(Fog,
        .mTypescriptFolder = "cocos/core/renderer/scene",
        .mTypescriptFilePrefix = "fog",
    ) {
        NAMESPACE(cc) {
            IMPORT_CLASS(Fog);
        }
    }
    MODULE(Shadows,
        .mTypescriptFolder = "cocos/core/renderer/scene",
        .mTypescriptFilePrefix = "shadows",
    ) {
        NAMESPACE(cc) {
            IMPORT_CLASS(Shadows);
        }
    }

    MODULE(GeometryRenderer,
        .mTypescriptFolder = "cocos/core/pipeline",
        .mTypescriptFilePrefix = "geometry-renderer", ) {
        ADD_FEATURES(ToJs);

        NAMESPACE(cc) {
            NAMESPACE(pipeline) {
                IMPORT_CLASS(GeometryRenderer);
                IMPORT_CLASS(RenderQueue);
            }
        }
    }

    MODULE(PipelineSceneData,
        .mFolder = "cocos/renderer/pipeline",
        .mFilePrefix = "PipelineSceneData.h",
        .mTypescriptFolder = "cocos/core/pipeline",
        .mTypescriptFilePrefix = "pipeline-scene-data", ) {
        ADD_FEATURES(ToJs);

        NAMESPACE(cc) {
            NAMESPACE(pipeline) {
                IMPORT_CLASS(PipelineSceneData);
                IMPORT_CLASS(RenderPipeline);
            }
        }
    }

    MODULE(GlobalDSManager,
        .mTypescriptFolder = "cocos/core/pipeline",
        .mTypescriptFilePrefix = "global-descriptor-set-manager",
    ) {
        ADD_FEATURES(ToJs);

        NAMESPACE(cc) {
            NAMESPACE(pipeline) {
                IMPORT_CLASS(GlobalDSManager);
            }
        }
    }

    MODULE(Gfx,
        .mFolder = "cocos/renderer/gfx-base",
        .mFilePrefix = "GFXDef-common.h",
        .mTypescriptFolder = "cocos",
        .mTypescriptFilePrefix = "gfx",
    ) {
        ADD_FEATURES(ToJs);

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

                IMPORT_ENUM(AccessFlagBit);
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
                IMPORT_ENUM(PassType);
                IMPORT_ENUM(BarrierType);

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
                VALUE(Viewport);
                VALUE(Color);
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
        } // namespace cc
        PROJECT_TS(std::unique_ptr<cc::gfx::Buffer>, Buffer);
        PROJECT_TS(std::unique_ptr<cc::gfx::Texture>, Texture);
        PROJECT_TS(std::unique_ptr<cc::gfx::Sampler>, Sampler);
    }

    MODULE(FrameGraph,
        .mFolder = "cocos/renderer/frame-graph",
        .mFilePrefix = "FrameGraph.h") {
        NAMESPACE(cc) {
            NAMESPACE(framegraph) {
                IMPORT_CLASS(FrameGraph);
                VALUE(TextureHandle);
            }
        }
    }

    MODULE(Assets,
        .mFolder = "cocos/core/assets",
        .mFilePrefix = "EffectAsset.h",
        .mTypescriptFolder = "cocos/core",
        .mTypescriptFilePrefix = "assets",
        ) {
        ADD_FEATURES(ToJs);
        NAMESPACE(cc) {
            IMPORT_CLASS(EffectAsset) {
            }
            IMPORT_CLASS(RenderTexture) {
            }
            IMPORT_CLASS(Material) {
            }
        }
    }
}

}
