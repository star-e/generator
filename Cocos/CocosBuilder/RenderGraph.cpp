/*
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2021 Xiamen Yaji Software Co., Ltd.

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

#include "RenderGraph.h"

#include <Cocos/AST/DSL.h>
#include <Cocos/AST/SyntaxGraphs.h>

namespace Cocos::Meta {

void buildRenderGraph(ModuleBuilder& builder) {
    MODULE(Camera,
        .mAPI = "CC_DLL",
        .mFolder = "cocos/core/renderer/scene",
        .mFilePrefix = "camera") {
        NAMESPACE(cc) {
            IMPORT_CLASS(Camera);
        }
    }
    MODULE(Ambient,
        .mAPI = "CC_DLL",
        .mFolder = "cocos/core/renderer/scene",
        .mFilePrefix = "ambient") {
        NAMESPACE(cc) {
            IMPORT_CLASS(Ambient);
        }
    }
    MODULE(Fog,
        .mAPI = "CC_DLL",
        .mFolder = "cocos/core/renderer/scene",
        .mFilePrefix = "fog") {
        NAMESPACE(cc) {
            IMPORT_CLASS(Fog);
        }
    }

    MODULE(Gfx,
        .mAPI = "CC_DLL",
        .mFolder = "cocos/core/gfx",
        .mFilePrefix = "index") {
        NAMESPACE(cc) {
            PMR_MAP(PmrMap);
            PROJECT_TS(PmrMap, Map);
            PMR_MAP(PmrUnorderedMap);
            PROJECT_TS(PmrUnorderedMap, Map);

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
        .mAPI = "CC_DLL",
        .mFolder = "cocos/core/pipeline",
        .mFilePrefix = "types") {
        NAMESPACE(cc) {
            NAMESPACE(render) {
                ENUM(UpdateFrequency, (PerInstance, PerBatch, PerQueue, PerPass, Count));

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

                VARIANT(ResourceType, (Constants_,
                    Buffer_,
                    Texture1D_, Texture1DArray_,
                    Texture2D_, Texture2DArray_, Texture2DMS_, Texture2DMSArray_,
                    Texture3D_, TextureCube_, TextureCubeArray_,
                    RaytracingAccelerationStructure_,
                    SamplerState_, SamplerComparisonState_));

                TAGS((_), Typeless_,
                    Float4_, Float3_, Float2_, Float1_,
                    Half4_, Half3_, Half2_, Half1_,
                    Fixed4_, Fixed3_, Fixed2_, Fixed1_,
                    Uint4_, Uint3_, Uint2_, Uint1_,
                    Int4_, Int3_, Int2_, Int1_,
                    Bool4_, Bool3_, Bool2_, Bool1_);

                VARIANT(ValueType, (Typeless_,
                    Float4_, Float3_, Float2_, Float1_,
                    Half4_, Half3_, Half2_, Half1_,
                    Fixed4_, Fixed3_, Fixed2_, Fixed1_,
                    Uint4_, Uint3_, Uint2_, Uint1_,
                    Int4_, Int3_, Int2_, Int1_,
                    Bool4_, Bool3_, Bool2_, Bool1_));

                TAGS((_), Raster_, Compute_, Copy_, Move_, Raytrace_);

                TAGS((_), Managed_, Persistent_, Backbuffer_, Memoryless_);
                VARIANT(ResourceResidency, (Managed_, Persistent_, Backbuffer_, Memoryless_));

                ENUM(QueueHint, (Opaque, Cutout, Transparent, Count));
                ENUM(ResourceDimension, (Buffer, Texture1D, Texture2D, Texture3D));

                STRUCT(SampleDesc) {
                    PUBLIC(
                        (uint32_t, mCount, 1)
                        (uint32_t, mQuality, 0)
                    );
                }

                ENUM(NodeType, (Internal, Leaf));
            } // namespace render
        }
    } // module CoreRender

    MODULE(DescriptorLayout,
        .mAPI = "CC_DLL",
        .mFolder = "cocos/core/pipeline",
        .mFilePrefix = "layout-graph") {
        NAMESPACE(cc) {
            NAMESPACE(render) {
                //-----------------------------------------------------------
                // Constant
                STRUCT(Constant) {
                    PUBLIC(
                        (ValueType, mType, _)
                        (uint32_t, mValueID, 0xFFFFFFFF)
                    );
                }

                STRUCT(ConstantBuffer) {
                    PUBLIC(
                        (uint32_t, mSize, 0)
                        (std::pmr::vector<Constant>, mConstants, _)
                    );
                }

                //-----------------------------------------------------------
                // Descriptor
                VARIANT(DescriptorType, (CBuffer_, RWBuffer_, RWTexture_, Buffer_, Texture_, Sampler_), LESS | EQUAL);

                STRUCT(DescriptorBlock) {
                    PUBLIC(
                        (DescriptorType, mType, _)
                        (uint32_t, mCapacity, 0)
                        (std::pmr::vector<uint32_t>, mAttributeIDs, _)
                    );
                }

                STRUCT(DescriptorArray) {
                    PUBLIC(
                        (uint32_t, mCapacity, 0)
                        (uint32_t, mAttributeID, 0xFFFFFFFF)
                    );
                }

                STRUCT(UnboundedDescriptor) {
                    PUBLIC(
                        (DescriptorType, mType, _)
                        (std::pmr::vector<DescriptorArray>, mDescriptors, _)
                    );
                }

                STRUCT(DescriptorTable) {
                    PUBLIC(
                        (uint32_t, mSlot, 0)
                        (uint32_t, mCapacity, 0)
                        (std::pmr::vector<DescriptorBlock>, mBlocks, _)
                    );
                }

                STRUCT(DescriptorSet) {
                    PUBLIC(
                        (std::pmr::vector<DescriptorTable>, mTables, _)
                        (UnboundedDescriptor, mUnbounded, _)
                    )
                }

                STRUCT(LayoutData) {
                    PUBLIC(
                        ((PmrMap<ParameterType, ConstantBuffer>), mConstantBuffers, _)
                        ((PmrMap<ParameterType, DescriptorSet>), mDescriptorSets, _)
                    );
                }
                //-----------------------------------------------------------
                // Shader Program
                STRUCT(ShaderProgramData) {
                    PUBLIC(
                        ((PmrMap<UpdateFrequency, LayoutData>), mLayouts, _)
                    );
                }

                //-----------------------------------------------------------
                // Descriptor Layout Graph
                STRUCT(GroupNodeData) {
                    PUBLIC(
                        (NodeType, mNodeType, _)
                    );
                    CNTR(mNodeType);
                }

                STRUCT(ShaderNodeData) {
                    PUBLIC(
                        (std::pmr::string, mRootSignature, _)
                        (std::pmr::vector<ShaderProgramData>, mShaderPrograms, _)
                        ((PmrMap<std::pmr::string, uint32_t>), mShaderIndex, _)
                    );
                }

                TAGS((_), Group_, Shader_);

                GRAPH(LayoutGraph, _, _) {
                    OBJECT_DESCRIPTOR();
                    NAMED_GRAPH();
                    ALIAS_REFERENCE_GRAPH();
                    ADDRESSABLE_GRAPH();

                    COMPONENT_GRAPH(
                        (Update, UpdateFrequency, mUpdateFrequencies)
                        (Layout, LayoutData, mLayouts)
                    );

                    POLYMORPHIC_GRAPH(
                        (Group_, GroupNodeData, mGroupNodes)
                        (Shader_, ShaderNodeData, mShaderNodes)
                    );
                }
            }
        }
    }

    MODULE(RenderGraph,
        .mAPI = "CC_DLL",
        .mFolder = "cocos/core/pipeline",
        .mFilePrefix = "render-graph",
        .mTypescriptInclude = R"(import { Mat4 } from '../math';
import { legacyCC } from '../global-exports';
import { RenderScene } from '../renderer/scene';
)") {
        NAMESPACE(cc) {
            NAMESPACE(render) {
                FLAGS(ResourceFlags, 
                    (None, 0)
                    (AllowRenderTarget, 0x1)
                    (AllowDepthStencil, 0x2)
                    (AllowUnorderedAccess, 0x4)
                    (DenyShaderResource, 0x8)
                    (AllowCrossAdapter, 0x10)
                    (AllowSimultaneousAccess, 0x20)
                    (VideoDecodeReferenceOnly, 0x40)
                );

                ENUM(TextureLayout, (Unknown, RowMajor, UndefinedSwizzle, StandardSwizzle));

                STRUCT(ResourceDesc) {
                    PUBLIC(
                        (ResourceDimension, mDimension, _)
                        (uint32_t, mAlignment, 0)
                        (uint32_t, mWidth, 0)
                        (uint32_t, mHeight, 0)
                        (uint16_t, mDepthOrArraySize, 0)
                        (uint16_t, mMipLevels, 0)
                        (gfx::Format, mFormat, Format.UNKNOWN)
                        (gfx::SampleCount, mSampleCount, SampleCount.ONE)
                        (TextureLayout, mLayout, _)
                        (ResourceFlags, mFlags, _)
                    );
                }

                STRUCT(ResourceTraits) {
                    PUBLIC(
                        (ResourceResidency, mResidency, _)
                    );
                    CNTR(mResidency);
                }

                GRAPH(ResourceGraph, _, _) {
                    OBJECT_DESCRIPTOR();
                    NAMED_GRAPH();
                    COMPONENT_GRAPH(
                        (Desc, ResourceDesc, mDescs)
                        (Traits, ResourceTraits, mTraits)
                    );
                }

                //STRUCT(NodeValue) {
                //    PUBLIC(
                //        (std::pmr::u8string, mNodeName, _)
                //        (std::pmr::string, mValueName, _)
                //    );
                //}
                //PROJECT_TS((PmrMap<NodeValue, uint32_t>), (Map<string, number>));

                //GRAPH(NodeGraph, NodeValue, _) {
                //    PUBLIC(
                //        ((PmrMap<NodeValue, uint32_t>), mIndex, _)
                //    );
                //}
                                
                //STRUCT(ShaderParameterIndex) {
                //    PUBLIC(
                //        (UpdateFrequency, mUpdateFrequency, _)
                //        (ParameterType, mParameterType, ParameterType.Table)
                //    );
                //}

                //STRUCT(ShaderParameter) {
                //    PUBLIC(
                //        (ShaderParameterIndex, mIndex, _)
                //        (ResourceType, mResourceType, _)
                //        (ValueType, mValueType, _)
                //        (uint32_t, mNumDescriptors, 1)
                //        (std::pmr::string, mValue, "")
                //    );
                //}

                ENUM(AttachmentType, (RenderTarget, DepthStencil));
                ENUM(AccessType, (Read, ReadWrite, Write));

                STRUCT(RasterView) {
                    PUBLIC(
                        (std::pmr::string, mSlotName, _)
                        (AccessType, mAccessType, AccessType.Write)
                        (AttachmentType, mAttachmentType, _)
                        (gfx::LoadOp, mLoadOp, LoadOp.LOAD)
                        (gfx::StoreOp, mStoreOp, StoreOp.STORE)
                        (gfx::ClearFlagBit, mClearFlags, ClearFlagBit.ALL)
                        (gfx::Color, mClearColor, _)
                    );
                    CNTR(mSlotName, mAccessType, mAttachmentType, mLoadOp, mStoreOp, mClearFlags, mClearColor);
                }

                ENUM(ClearValueType, (Float, Int));

                STRUCT(ComputeView) {
                    PUBLIC(
                        (std::pmr::string, mName, _)
                        (AccessType, mAccessType, AccessType.Read)
                        (gfx::ClearFlagBit, mClearFlags, ClearFlagBit.NONE)
                        (gfx::Color, mClearColor, _)
                        (ClearValueType, mClearValueType, _)
                    );
                }

                STRUCT(RasterSubpass) {
                    PUBLIC(
                        ((PmrMap<std::pmr::string, RasterView>), mRasterViews, _)
                        ((PmrMap<std::pmr::string, std::pmr::vector<ComputeView>>), mComputeViews, _)
                    );
                }

                GRAPH(SubpassGraph, RasterSubpass, _) {
                    NAMED_GRAPH();
                }
                
                STRUCT(RasterPassData) {
                    PUBLIC(
                        ((PmrMap<std::pmr::string, RasterView>), mRasterViews, _)
                        ((PmrMap<std::pmr::string, std::pmr::vector<ComputeView>>), mComputeViews, _)
                        (SubpassGraph, mSubpassGraph, _)
                    );
                }

                STRUCT(ComputePassData) {
                    PUBLIC(
                        ((PmrMap<std::pmr::string, std::pmr::vector<ComputeView>>), mComputeViews, _)
                    );
                }

                STRUCT(CopyPair) {
                    PUBLIC(
                        (std::pmr::string, mSource, _)
                        (std::pmr::string, mTarget, _)
                        (uint32_t, mMipLevels, 0xFFFFFFFF)
                        (uint32_t, mNumSlices, 0xFFFFFFFF)
                        (uint32_t, mSourceMostDetailedMip, 0)
                        (uint32_t, mSourceFirstSlice, 0)
                        (uint32_t, mSourcePlaneSlice, 0)
                        (uint32_t, mTargetMostDetailedMip, 0)
                        (uint32_t, mTargetFirstSlice, 0)
                        (uint32_t, mTargetPlaneSlice, 0)
                    );
                    CNTR(mSource, mTarget, mMipLevels, mNumSlices,
                        mSourceMostDetailedMip, mSourceFirstSlice, mSourcePlaneSlice,
                        mTargetMostDetailedMip, mTargetFirstSlice, mTargetPlaneSlice);
                }

                STRUCT(CopyPassData) {
                    PUBLIC(
                        (std::pmr::vector<CopyPair>, mCopyPairs, _)
                    );
                }

                STRUCT(MovePair) {
                    PUBLIC(
                        (std::pmr::string, mSource, _)
                        (std::pmr::string, mTarget, _)
                        (uint32_t, mMipLevels, 0xFFFFFFFF)
                        (uint32_t, mNumSlices, 0xFFFFFFFF)
                        (uint32_t, mTargetMostDetailedMip, 0)
                        (uint32_t, mTargetFirstSlice, 0)
                        (uint32_t, mTargetPlaneSlice, 0)
                    );
                    CNTR(mSource, mTarget, mMipLevels, mNumSlices,
                        mTargetMostDetailedMip,
                        mTargetFirstSlice, mTargetPlaneSlice);
                }

                STRUCT(MovePassData) {
                    PUBLIC(
                        (std::pmr::vector<MovePair>, mMovePairs, _)
                    );
                }

                STRUCT(RaytracePassData) {
                    PUBLIC(
                        ((PmrMap<std::pmr::string, std::pmr::vector<ComputeView>>), mComputeViews, _)
                    );
                }

                TAGS((_), Queue_, Scene_, Dispatch_, Blit_);

                STRUCT(RenderQueueData) {
                    PUBLIC(
                        (QueueHint, mHint, _)
                    );
                    CNTR(mHint);
                }

                STRUCT(SceneData) {
                    PUBLIC(
                        (std::pmr::string, mName, _)
                        (Camera*, mCamera, nullptr)
                        (std::pmr::vector<std::pmr::string>, mScenes, _)
                    );
                    CNTR(mName);
                }

                STRUCT(Dispatch) {
                    PUBLIC(
                        (std::pmr::string, mShader, _)
                        (uint32_t, mThreadGroupCountX, 0)
                        (uint32_t, mThreadGroupCountY, 0)
                        (uint32_t, mThreadGroupCountZ, 0)
                    );
                    CNTR(mShader, mThreadGroupCountX, mThreadGroupCountY, mThreadGroupCountZ);
                }

                STRUCT(Blit) {
                    PUBLIC(
                        (std::pmr::string, mShader, _)
                    );
                    CNTR(mShader);
                }

                STRUCT(RenderData) {
                    PUBLIC(
                        ((PmrUnorderedMap<uint32_t, std::pmr::vector<uint8_t>>), mConstants, _)
                        ((PmrUnorderedMap<uint32_t, gfx::Buffer>), mBuffers, _)
                        ((PmrUnorderedMap<uint32_t, gfx::Texture>), mTextures, _)
                        ((PmrUnorderedMap<uint32_t, gfx::Sampler>), mSamplers, _)
                    );
                }

                GRAPH(RenderGraph, _, _) {
                    NAMED_GRAPH();
                    REFERENCE_GRAPH();
                    //ADDRESSABLE_GRAPH();

                    COMPONENT_GRAPH(
                        (Layout, std::pmr::string, mLayoutNodes)
                        (Data, RenderData, mData)
                    );

                    POLYMORPHIC_GRAPH(
                        (Raster_, RasterPassData, mRasterPasses)
                        (Compute_, ComputePassData, mComputePasses)
                        (Copy_, CopyPassData, mCopyPasses)
                        (Move_, MovePassData, mMovePasses)
                        (Raytrace_, RaytracePassData, mCopyPasses)
                        (Queue_, RenderQueueData, mRenderQueues)
                        (Scene_, SceneData, mScenes)
                        (Blit_, Blit, mBlits)
                        (Dispatch_, Dispatch, mDispatches)
                    );
                    PUBLIC(
                        ((PmrUnorderedMap<std::pmr::string, uint32_t>), mIndex, _)
                    );
                }

                STRUCT(Setter) {
                    PRIVATE(
                        (RenderData&, mData, _)
                    );
                    EXPLICIT_CNTR(mData);
                    TS_FUNCTIONS(R"(setMat4 (name: string, mat: Mat4): void {

}
setMatrix4x4 (name: string, data: number[]): void {

}
setMatrix3x4 (name: string, data: number[]): void {

}
setFloat4 (name: string, data: number[]): void {

}
setFloat2 (name: string, data: number[]): void {

}
setFloat (name: string, data: number): void {

}
setInt4 (name: string, data: number[]): void {

}
setInt2 (name: string, data: number[]): void {

}
setInt (name: string, data: number): void {

}
setUint4 (name: string, data: number[]): void {

}
setUint2 (name: string, data: number[]): void {

}
setUint (name: string, data: number): void {

}
setCBuffer (name: string, buffer: Buffer): void {

}
setBuffer (name: string, buffer: Buffer): void {

}
setTexture (name: string, texture: Texture): void {

}
setRWBuffer (name: string, buffer: Buffer): void {

}
setRWTexture (name: string, texture: Texture): void {

}
setSampler (name: string, sampler: Sampler): void {

}
protected _setCameraValues (camera: Readonly<Camera>, cfg: Readonly<RenderConfig>, scene: Readonly<RenderScene>) {
    this.setMat4('cc_matView', camera.matView);
    this.setMat4('cc_matViewInv', camera.node.worldMatrix);
    this.setMat4('cc_matProj', camera.matProj);
    this.setMat4('cc_matProjInv', camera.matProjInv);
    this.setMat4('cc_matViewProj', camera.matViewProj);
    this.setMat4('cc_matViewProjInv', camera.matViewProjInv);
    this.setFloat4('cc_cameraPos', [camera.position.x, camera.position.y, camera.position.z, 0.0]);
    this.setFloat4('cc_screenScale', [cfg.shadingScale, cfg.shadingScale, 1.0 / cfg.shadingScale, 1.0 / cfg.shadingScale]);
    this.setFloat4('cc_exposure', [camera.exposure, 1.0 / camera.exposure, cfg.isHDR ? 1.0 : 0.0, 0.0]);

    const mainLight = scene.mainLight;
    if (mainLight) {
        this.setFloat4('cc_mainLitDir', [mainLight.direction.x, mainLight.direction.y, mainLight.direction.z, 0.0]);
        let r = mainLight.color.x;
        let g = mainLight.color.y;
        let b = mainLight.color.z;
        if (mainLight.useColorTemperature) {
            r *= mainLight.colorTemperatureRGB.x;
            g *= mainLight.colorTemperatureRGB.y;
            b *= mainLight.colorTemperatureRGB.z;
        }
        let w = mainLight.illuminance;
        if (cfg.isHDR) {
            w *= camera.exposure;
        }
        this.setFloat4('cc_mainLitColor', [r, g, b, w]);
    } else {
        this.setFloat4('cc_mainLitDir', [0, 0, 1, 0]);
        this.setFloat4('cc_mainLitColor', [0, 0, 0, 0]);
    }

    const ambient = cfg.ambient;
    const skyColor = ambient.skyColor;
    if (cfg.isHDR) {
        skyColor.w = ambient.skyIllum * camera.exposure;
    } else {
        skyColor.w = ambient.skyIllum;
    }
    this.setFloat4('cc_ambientSky', [skyColor.x, skyColor.y, skyColor.z, skyColor.w]);
    this.setFloat4('cc_ambientGround', [ambient.groundAlbedo.x, ambient.groundAlbedo.y, ambient.groundAlbedo.z, ambient.groundAlbedo.w]);

    const fog = cfg.fog;
    const colorTempRGB = fog.colorArray;
    this.setFloat4('cc_fogColor', [colorTempRGB.x, colorTempRGB.y, colorTempRGB.z, colorTempRGB.w]);
    this.setFloat4('cc_fogBase', [fog.fogStart, fog.fogEnd, fog.fogDensity, 0.0]);
    this.setFloat4('cc_fogAdd', [fog.fogTop, fog.fogRange, fog.fogAtten, 0.0]);
    this.setFloat4('cc_nearFar', [camera.nearClip, camera.farClip, 0.0, 0.0]);
    this.setFloat4('cc_viewPort', [camera.viewport.x, camera.viewport.y, camera.viewport.z, camera.viewport.w]);
}
)");
                }

                STRUCT(RenderConfig) {
                    PUBLIC(
                        (Ambient, mAmbient, _)
                        (Fog, mFog, _)
                        (bool, mIsHDR, false)
                        (float, mShadingScale, 1.0)
                    );
                }

                STRUCT(RasterQueue) {
                    INHERITS(Setter);
                    PRIVATE(
                        (RenderGraph&, mRenderGraph, _)
                        (const uint32_t, mVertID, 0xFFFFFFFF)
                        (RenderQueueData&, mQueue, _)
                        (RenderConfig&, mConfig, _)
                    );
                    EXPLICIT_CNTR(mRenderGraph, mVertID, mQueue, mConfig);
                    TS_FUNCTIONS(R"(addCamera (camera: Camera, name = 'Camera'): RasterQueue {
    const sceneData = new SceneData(name);
    sceneData.camera = camera;
    this._renderGraph.addVertex<RenderGraphValue.scene>(
        RenderGraphValue.scene, sceneData, name, '', new RenderData(), this._vertID,
    );
    super._setCameraValues(camera, this._config,
        camera.scene ? camera.scene : legacyCC.director.getScene().renderScene);
    return this;
}
addFullscreenQuad (shader: string, layoutName = '', name = 'Quad') {
    this._renderGraph.addVertex<RenderGraphValue.blit>(
        RenderGraphValue.blit, new Blit(shader), name, layoutName, new RenderData(), this._vertID,
    );
}
)");
                }
                                                
                STRUCT(RasterPass) {
                    INHERITS(Setter);
                    PRIVATE(
                        (RenderGraph&, mRenderGraph, _)
                        (const uint32_t, mVertID, 0xFFFFFFFF)
                        (RasterPassData&, mPass, _)
                        (RenderConfig&, mConfig, _)
                    );
                    EXPLICIT_CNTR(mRenderGraph, mVertID, mPass, mConfig);
                    TS_FUNCTIONS(R"(addRasterView (name: string, view: RasterView) {
    this._pass.rasterViews.set(name, view);
}
addComputeView (name: string, view: ComputeView) {
    if (this._pass.computeViews.has(name)) {
        this._pass.computeViews.get(name)?.push(view);
    } else {
        this._pass.computeViews.set(name, [view]);
    }
}
addQueue (hint: QueueHint = QueueHint.Opaque, layoutName = '', name = 'Queue') {
    if (layoutName === '') {
        switch (hint) {
        case QueueHint.Opaque:
            layoutName = 'Opaque';
            break;
        case QueueHint.Cutout:
            layoutName = 'Cutout';
            break;
        case QueueHint.Transparent:
            layoutName = 'Transparent';
            break;
        default:
            throw Error('cannot infer layoutName from QueueHint');
        }
    }
    const queue = new RenderQueueData(hint);
    const data = new RenderData();
    const queueID = this._renderGraph.addVertex<RenderGraphValue.queue>(
        RenderGraphValue.queue, queue, name, layoutName, data, this._vertID,
    );
    return new RasterQueue(data, this._renderGraph, queueID, queue, this._config);
}
addFullscreenQuad (shader: string, layoutName = '', name = 'Quad') {
    this._renderGraph.addVertex<RenderGraphValue.blit>(
        RenderGraphValue.blit,
        new Blit(shader),
        name, layoutName, new RenderData(), this._vertID,
    );
}
)");
                }

                STRUCT(ComputeQueue) {
                    INHERITS(Setter);
                    PRIVATE(
                        (RenderGraph&, mRenderGraph, _)
                        (const uint32_t, mVertID, 0xFFFFFFFF)
                        (RenderQueueData&, mQueue, _)
                        (RenderConfig&, mConfig, _)
                    );
                    EXPLICIT_CNTR(mRenderGraph, mVertID, mQueue, mConfig);
                    TS_FUNCTIONS(R"(addDispatch (shader: string,
    threadGroupCountX: number,
    threadGroupCountY: number,
    threadGroupCountZ: number,
    layoutName = '',
    name = 'Dispatch') {
    this._renderGraph.addVertex<RenderGraphValue.dispatch>(
        RenderGraphValue.dispatch,
        new Dispatch(shader, threadGroupCountX, threadGroupCountY, threadGroupCountZ),
        name, layoutName, new RenderData(), this._vertID,
    );
}
)");
                }

                STRUCT(ComputePass) {
                    INHERITS(Setter);
                    PRIVATE(
                        (RenderGraph&, mRenderGraph, _)
                        (const uint32_t, mVertID, 0xFFFFFFFF)
                        (ComputePassData&, mPass, _)
                        (RenderConfig&, mConfig, _)
                    );
                    EXPLICIT_CNTR(mRenderGraph, mVertID, mPass, mConfig);
                    TS_FUNCTIONS(R"(addComputeView (name: string, view: ComputeView) {
    if (this._pass.computeViews.has(name)) {
        this._pass.computeViews.get(name)?.push(view);
    } else {
        this._pass.computeViews.set(name, [view]);
    }
}
addDispatch (shader: string,
    threadGroupCountX: number,
    threadGroupCountY: number,
    threadGroupCountZ: number,
    layoutName = '',
    name = 'Dispatch') {
    this._renderGraph.addVertex<RenderGraphValue.dispatch>(
        RenderGraphValue.dispatch,
        new Dispatch(shader, threadGroupCountX, threadGroupCountY, threadGroupCountZ),
        name, layoutName, new RenderData(), this._vertID,
    );
}
)");
                }

                STRUCT(MovePass) {
                    PRIVATE(
                        (RenderGraph&, mRenderGraph, _)
                        (const uint32_t, mVertID, 0xFFFFFFFF)
                        (MovePassData&, mPass, _)
                    );
                    EXPLICIT_CNTR(mRenderGraph, mVertID, mPass);
                    TS_FUNCTIONS(R"(addMove (pair: MovePair) {
    this._pass.movePairs.push(pair);
}
)");
                }
                
                STRUCT(CopyPass) {
                    PRIVATE(
                        (RenderGraph&, mRenderGraph, _)
                        (const uint32_t, mVertID, 0xFFFFFFFF)
                        (CopyPassData&, mPass, _)
                    );
                    EXPLICIT_CNTR(mRenderGraph, mVertID, mPass);
                    TS_FUNCTIONS(R"(addCopy (pair: CopyPair) {
    this._pass.copyPairs.push(pair);
}
)");
                }
            } // namespace render
        }
    }
}

void buildRenderExecutor(ModuleBuilder& builder) {
    MODULE(RenderExecutor,
        .mAPI = "CC_DLL",
        .mFolder = "cocos/core/pipeline",
        .mFilePrefix = "executor") {
        NAMESPACE(cc) {
            NAMESPACE(render) {
                GRAPH(DeviceResourceGraph, _, _) {
                    OBJECT_DESCRIPTOR();
                    NAMED_GRAPH();
                    COMPONENT_GRAPH(
                        (RefCount, int32_t, mRefCounts)
                    );
                    POLYMORPHIC_GRAPH(
                        (Buffer_, gfx::Buffer, mBuffers)
                        (Texture_, gfx::Texture, mTextures)
                    );
                }
            }
        }
    }
}

} // namespace Cocos::Meta
