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

// clang-format off

namespace Cocos::Meta {

void buildRenderGraph(ModuleBuilder& builder, Features features) {
    MODULE(RenderGraph,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "RenderGraph",
        .mJsbHeaders = R"(#include "cocos/bindings/auto/jsb_gfx_auto.h"
#include "cocos/bindings/auto/jsb_scene_auto.h"
)",
        .mTypescriptFolder = "cocos/rendering/custom",
        .mTypescriptFilePrefix = "render-graph",
        .mRequires = { "RenderCommon", "Gfx", "Camera" },
        .mHeader = R"(#include "cocos/renderer/gfx-base/GFXBuffer.h"
#include "cocos/renderer/gfx-base/GFXTexture.h"
#include "cocos/renderer/gfx-base/GFXFramebuffer.h"
#include "cocos/renderer/gfx-base/GFXRenderPass.h"
#include "cocos/renderer/gfx-base/GFXSwapchain.h"
#include "cocos/renderer/gfx-base/states/GFXSampler.h"
#include "cocos/math/Geometry.h"
#include "cocos/core/assets/Material.h"
)",
//        .mTypescriptInclude = R"(import { equalObjectArray, equalValueArray, equalObjectMap, equalValueMap } from './utils';
//)",
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        STRUCT(ClearValue, .mFlags = VALUE_OBJECT | JSB) {
            PUBLIC(
                (double, mX, 0)
                (double, mY, 0)
                (double, mZ, 0)
                (double, mW, 0)
            );
            CNTR(mX, mY, mZ, mW);
        }

        STRUCT(RasterView, .mFlags = JSB | PMR_DEFAULT | POOL_OBJECT) {
            PUBLIC(
                (ccstd::pmr::string, mSlotName, _)
                (ccstd::pmr::string, mSlotName1, _)
                (AccessType, mAccessType, AccessType::WRITE)
                (AttachmentType, mAttachmentType, _)
                (gfx::LoadOp, mLoadOp, gfx::LoadOp::LOAD)
                (gfx::StoreOp, mStoreOp, gfx::StoreOp::STORE)
                (gfx::ClearFlagBit, mClearFlags, gfx::ClearFlagBit::ALL)
                (gfx::Color, mClearColor, _)
                (uint32_t, mSlotID, 0)
                (gfx::ShaderStageFlagBit, mShaderStageFlags, gfx::ShaderStageFlagBit::NONE)
            );
            builder.setMemberFlags(vertID, "mClearColor", NOT_ELEMENT);
            builder.setMemberFlags(vertID, "mSlotID", NOT_ELEMENT);
            TS_INIT(mAccessType, AccessType.WRITE);
            TS_INIT(mLoadOp, LoadOp.LOAD);
            TS_INIT(mStoreOp, StoreOp.STORE);
            TS_INIT(mClearFlags, ClearFlagBit.ALL);
            CNTR(mSlotName, mAccessType, mAttachmentType, mLoadOp, mStoreOp, mClearFlags, mClearColor, mShaderStageFlags);
            CNTR(mSlotName, mSlotName1, mAccessType, mAttachmentType, mLoadOp, mStoreOp, mClearFlags, mClearColor, mShaderStageFlags);
        }

        STRUCT(ComputeView, .mFlags = JSB | PMR_DEFAULT | POOL_OBJECT) {
            PUBLIC(
                (ccstd::pmr::string, mName, _)
                (AccessType, mAccessType, AccessType::READ)
                (uint32_t, mPlane, 0)
                (gfx::ClearFlagBit, mClearFlags, gfx::ClearFlagBit::NONE)
                (ClearValueType, mClearValueType, ClearValueType::NONE)
                (ClearValue, mClearValue, _)
                (gfx::ShaderStageFlagBit, mShaderStageFlags, gfx::ShaderStageFlagBit::NONE)
            );
            builder.setMemberFlags(vertID, "mClearValue", NOT_ELEMENT);
            MEMBER_FUNCTIONS(R"(
bool isRead() const {
    return accessType != AccessType::WRITE;
}
bool isWrite() const {
    return accessType != AccessType::READ;
}
)");
            TS_INIT(mClearFlags, ClearFlagBit.NONE);
            CNTR(mName, mAccessType, mClearFlags, mClearValueType, mClearValue, mShaderStageFlags);
            CNTR(mName, mAccessType, mPlane, mClearFlags, mClearValueType, mClearValue, mShaderStageFlags);
        }

        STRUCT(ResourceDesc, .mFlags = POOL_OBJECT) {
            PUBLIC(
                (ResourceDimension, mDimension, _)
                (uint32_t, mAlignment, 0)
                (uint32_t, mWidth, 0)
                (uint32_t, mHeight, 0)
                (uint16_t, mDepthOrArraySize, 0)
                (uint16_t, mMipLevels, 0)
                (gfx::Format, mFormat, gfx::Format::UNKNOWN)
                (gfx::SampleCount, mSampleCount, gfx::SampleCount::X1)
                (gfx::TextureFlagBit, mTextureFlags, gfx::TextureFlagBit::NONE)
                (ResourceFlags, mFlags, ResourceFlags::NONE)
            );
            TS_INIT(mFormat, Format.UNKNOWN);
            TS_INIT(mSampleCount, SampleCount.X1);
            TS_INIT(mTextureFlags, TextureFlagBit.NONE);
            TS_INIT(mFlags, ResourceFlags.NONE);
        }

        STRUCT(ResourceTraits, .mFlags = POOL_OBJECT) {
            PUBLIC(
                (ResourceResidency, mResidency, _)
            );
            CNTR(mResidency);
            MEMBER_FUNCTIONS(R"(
bool hasSideEffects() const noexcept {
    return residency == ResourceResidency::PERSISTENT ||
           residency == ResourceResidency::EXTERNAL ||
           residency == ResourceResidency::BACKBUFFER;
}
)");
        }

        STRUCT(RenderSwapchain, .mFlags = POOL_OBJECT) {
            PUBLIC(
                ([[optional]] gfx::Swapchain*, mSwapchain, nullptr)
                ([[optional]] scene::RenderWindow*, mRenderWindow, nullptr)
                (uint32_t, mCurrentID, 0)
                (uint32_t, mNumBackBuffers, 0)
                (uint32_t, mGeneration, 0xFFFFFFFF)
            );
            CNTR(mSwapchain);
        }

        STRUCT(ResourceStates, .mFlags = POOL_OBJECT) {
            PUBLIC(
                (gfx::AccessFlagBit, mStates, gfx::AccessFlagBit::NONE)
            );
            TS_INIT(mStates, AccessFlagBit.NONE);
        }

        STRUCT(ManagedBuffer, .mFlags = POOL_OBJECT) {
            PUBLIC(
                ([[optional]] IntrusivePtr<gfx::Buffer>, mBuffer, _)
                (uint64_t, mFenceValue, 0)
            );
            CNTR(mBuffer);
        }

        STRUCT(ManagedTexture, .mFlags = POOL_OBJECT) {
            PUBLIC(
                ([[optional]] IntrusivePtr<gfx::Texture>, mTexture, _)
                (uint64_t, mFenceValue, 0)
            );
            CNTR(mTexture);
            MEMBER_FUNCTIONS(R"(
bool checkResource(const ResourceDesc &desc) const;
)");
        }

        STRUCT(ManagedResource, .mFlags = POOL_OBJECT) {
            PUBLIC(
                (uint32_t, mUnused, 0)
            );
        }

        STRUCT(Subpass, .mFlags = POOL_OBJECT) {
            PUBLIC(
                ((PmrTransparentMap<ccstd::pmr::string, RasterView>), mRasterViews, _)
                ((PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>), mComputeViews, _)
                (ccstd::pmr::vector<ResolvePair>, mResolvePairs, _)
            );
        }

        PMR_GRAPH(SubpassGraph, _, _, .mFlags = POOL_OBJECT) {
            NAMED_GRAPH(Name_);
            COMPONENT_GRAPH(
                (Name_, ccstd::pmr::string, mNames)
                (Subpass_, Subpass, mSubpasses)
            );
        }

        STRUCT(RasterSubpass, .mFlags = NO_DEFAULT_CNTR) {
            PUBLIC(
                ((PmrTransparentMap<ccstd::pmr::string, RasterView>), mRasterViews, _)
                ((PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>), mComputeViews, _)
                (ccstd::pmr::vector<ResolvePair>, mResolvePairs, _)
                (gfx::Viewport, mViewport, _)
                (uint32_t, mSubpassID, 0xFFFFFFFF)
                (uint32_t, mCount, 1)
                (uint32_t, mQuality, 0)
                (bool, mShowStatistics, false)
            );
            CNTR_NO_DEFAULT(mSubpassID, mCount, mQuality);
        }

        STRUCT(ComputeSubpass, .mFlags = NO_DEFAULT_CNTR) {
            PUBLIC(
                ((PmrTransparentMap<ccstd::pmr::string, RasterView>), mRasterViews, _)
                ((PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>), mComputeViews, _)
                (uint32_t, mSubpassID, 0xFFFFFFFF)
            );
            CNTR_NO_DEFAULT(mSubpassID);
        }

        STRUCT(RasterPass, .mFlags = EQUAL | HASH_COMBINE) {
            PUBLIC(
                ((PmrTransparentMap<ccstd::pmr::string, RasterView>), mRasterViews, _)
                ((PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>), mComputeViews, _)
                ((PmrTransparentMap<ccstd::pmr::string, uint32_t>), mAttachmentIndexMap, _)
                ((PmrTransparentMap<ccstd::pmr::string, gfx::ShaderStageFlagBit>), mTextures, _)
                (SubpassGraph, mSubpassGraph, _)
                (uint32_t, mWidth, 0)
                (uint32_t, mHeight, 0)
                (uint32_t, mCount, 1)
                (uint32_t, mQuality, 0)
                (gfx::Viewport, mViewport, _)
                (ccstd::pmr::string, mVersionName, _)
                (uint64_t, mVersion, 0)
                (uint64_t, mHashValue, 0)
                (bool, mShowStatistics, false)
            );
            MEMBER_FLAGS(mAttachmentIndexMap, NOT_ELEMENT);
            MEMBER_FLAGS(mViewport, NOT_ELEMENT);
            MEMBER_FLAGS(mVersionName, NOT_ELEMENT);
            MEMBER_FLAGS(mVersion, NOT_ELEMENT);
            MEMBER_FLAGS(mHashValue, NOT_ELEMENT);
            MEMBER_FLAGS(mShowStatistics, NOT_ELEMENT);
        }

        STRUCT(PersistentRenderPassAndFramebuffer) {
            PUBLIC(
                (IntrusivePtr<gfx::RenderPass>, mRenderPass, _)
                (IntrusivePtr<gfx::Framebuffer>, mFramebuffer, _)
                (ccstd::pmr::vector<gfx::Color>, mClearColors, _)
                (float, mClearDepth, 0)
                (uint8_t, mClearStencil, 0)
            );
            CNTR_NO_DEFAULT(mRenderPass, mFramebuffer);
        }
        PROJECT_TS(IntrusivePtr<gfx::RenderPass>, RenderPass);

        TAGS((_), Managed_, ManagedBuffer_, ManagedTexture_, PersistentBuffer_, PersistentTexture_,
            Framebuffer_, Swapchain_, Sampler_, FormatView_, SubresourceView_);

        STRUCT(FormatView) {
            PUBLIC(
                (gfx::Format, mFormat, gfx::Format::UNKNOWN)
            );
        }

        STRUCT(SubresourceView) {
            PUBLIC(
                ([[optional]] IntrusivePtr<gfx::Texture>, mTextureView, _)
                (gfx::Format, mFormat, gfx::Format::UNKNOWN)
                (uint16_t, mIndexOrFirstMipLevel, 0)
                (uint16_t, mNumMipLevels, 0)
                (uint16_t, mFirstArraySlice, 0)
                (uint16_t, mNumArraySlices, 0)
                (uint16_t, mFirstPlane, 0)
                (uint16_t, mNumPlanes, 0)
            );
        }

        PMR_GRAPH(ResourceGraph, _, _, .mFlags = NO_MOVE_NO_COPY) {
            NAMED_GRAPH(Name_);
            ALIAS_REFERENCE_GRAPH();
            COMPONENT_GRAPH(
                (Name_, ccstd::pmr::string, mNames)
                (Desc_, ResourceDesc, mDescs)
                (Traits_, ResourceTraits, mTraits)
                (States_, ResourceStates, mStates)
                (Sampler_, gfx::SamplerInfo, mSamplerInfo)
            );
            COMPONENT_BIMAP(PmrUnorderedStringMap, mValueIndex, Name_);
            POLYMORPHIC_GRAPH(
                (Managed_, ManagedResource, mResources)
                (ManagedBuffer_, ManagedBuffer, mManagedBuffers)
                (ManagedTexture_, ManagedTexture, mManagedTextures)
                (PersistentBuffer_, IntrusivePtr<gfx::Buffer>, mBuffers)
                (PersistentTexture_, IntrusivePtr<gfx::Texture>, mTextures)
                (Framebuffer_, IntrusivePtr<gfx::Framebuffer>, mFramebuffers)
                (Swapchain_, RenderSwapchain, mSwapchains)
                (FormatView_, FormatView, mFormatViews)
                (SubresourceView_, SubresourceView, mSubresourceViews)
            );
            PUBLIC(
                ((ccstd::pmr::unordered_map<RasterPass, PersistentRenderPassAndFramebuffer>), mRenderPasses, _)
                (uint64_t, mNextFenceValue, 0)
                (uint64_t, mVersion, 0)
            );
            MEMBER_FUNCTIONS(R"(
void validateSwapchains();
void mount(gfx::Device* device, vertex_descriptor vertID);
void unmount(uint64_t completedFenceValue);
bool isTexture(vertex_descriptor resID) const noexcept;
bool isTextureView(vertex_descriptor resID) const noexcept;
gfx::Texture* getTexture(vertex_descriptor resID);
gfx::Buffer* getBuffer(vertex_descriptor resID);
void invalidatePersistentRenderPassAndFramebuffer(gfx::Texture* pTexture);
)");
        }

        PROJECT_TS(
            (ccstd::pmr::unordered_map<RasterPass, PersistentRenderPassAndFramebuffer>),
            (Map<string, PersistentRenderPassAndFramebuffer>)
        );
        PROJECT_TS(IntrusivePtr<gfx::Buffer>, Buffer);
        PROJECT_TS(IntrusivePtr<gfx::Texture>, Texture);
        PROJECT_TS(IntrusivePtr<gfx::Framebuffer>, Framebuffer);

        //STRUCT(NodeValue) {
        //    PUBLIC(
        //        (std::pmr::u8string, mNodeName, _)
        //        (ccstd::pmr::string, mValueName, _)
        //    );
        //}
        //PROJECT_TS((PmrTransparentMap<NodeValue, uint32_t>), (Map<string, number>));

        //PMR_GRAPH(NodeGraph, NodeValue, _) {
        //    PUBLIC(
        //        ((PmrTransparentMap<NodeValue, uint32_t>), mIndex, _)
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
        //        (ccstd::pmr::string, mValue, "")
        //    );
        //}

        STRUCT(ComputePass) {
            PUBLIC(
                ((PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>), mComputeViews, _)
                ((PmrTransparentMap<ccstd::pmr::string, gfx::ShaderStageFlagBit>), mTextures, _)
            );
        }

        STRUCT(ResolvePass) {
            PUBLIC(
                (ccstd::pmr::vector<ResolvePair>, mResolvePairs, _)
            );
        }

        STRUCT(CopyPass, .mFlags = NO_COPY) {
            PUBLIC(
                (ccstd::pmr::vector<CopyPair>, mCopyPairs, _)
                (ccstd::pmr::vector<UploadPair>, mUploadPairs, _)
            );
        }

        STRUCT(MovePass) {
            PUBLIC(
                (ccstd::pmr::vector<MovePair>, mMovePairs, _)
            );
        }

        STRUCT(RaytracePass) {
            PUBLIC(
                ((PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>), mComputeViews, _)
            );
        }

        TAGS((_), Queue_, Scene_, Dispatch_, Blit_, Clear_, Viewport_);

        STRUCT(ClearView, .mFlags = PMR_DEFAULT) {
            PUBLIC(
                (ccstd::pmr::string, mSlotName, _)
                (gfx::ClearFlagBit, mClearFlags, gfx::ClearFlagBit::ALL)
                (gfx::Color, mClearColor, _)
            );
            CNTR(mSlotName, mClearFlags, mClearColor);
        }

        STRUCT(RenderQueue) {
            PUBLIC(
                (QueueHint, mHint, QueueHint::RENDER_OPAQUE)
                (uint32_t, mPhaseID, 0xFFFFFFFF)
                ([[optional]] gfx::Viewport, mViewport, _)
            );
            TS_INIT(mViewport, null);
            CNTR(mHint, mPhaseID);
            CNTR(mPhaseID);
        }

        STRUCT(SceneData) {
            PUBLIC(
                ([[optional]] const scene::RenderScene*, mScene, nullptr)
                ([[optional]] const scene::Camera*, mCamera, nullptr)
                (LightInfo, mLight, _)
                (SceneFlags, mFlags, SceneFlags::NONE)
                (uint32_t, mCullingID, 0xFFFFFFFF)
            );
            CNTR(mScene, mCamera, mFlags, mLight);
        }

        STRUCT(Dispatch) {
            PUBLIC(
                ([[optional]] IntrusivePtr<cc::Material>, mMaterial, _)
                (uint32_t, mPassID, 0)
                (uint32_t, mThreadGroupCountX, 0)
                (uint32_t, mThreadGroupCountY, 0)
                (uint32_t, mThreadGroupCountZ, 0)
            );
            CNTR_NO_DEFAULT(mMaterial, mPassID, mThreadGroupCountX, mThreadGroupCountY, mThreadGroupCountZ);
        }

        STRUCT(Blit) {
            PUBLIC(
                ([[optional]] IntrusivePtr<cc::Material>, mMaterial, _)
                (uint32_t, mPassID, 0)
                (SceneFlags, mSceneFlags, _)
                ([[optional]] scene::Camera*, mCamera, nullptr)
            );
            CNTR_NO_DEFAULT(mMaterial, mPassID, mSceneFlags, mCamera);
        }

        STRUCT(RenderData, .mFlags = NO_COPY) {
            PUBLIC(
                //((mutable ccstd::unordered_map<uint32_t, ccstd::vector<char>>), mConstants, _)
                ((PmrUnorderedMap<uint32_t, ccstd::pmr::vector<char>>), mConstants, _)
                ((PmrUnorderedMap<uint32_t, IntrusivePtr<gfx::Buffer>>), mBuffers, _)
                ((PmrUnorderedMap<uint32_t, IntrusivePtr<gfx::Texture>>), mTextures, _)
                ((PmrUnorderedMap<uint32_t, gfx::Sampler*>), mSamplers, _)
                (ccstd::pmr::string, mCustom, _)
            );
            //TS_INIT(mConstants, {});
        }

        PMR_GRAPH(RenderGraph, _, _, .mFlags = NO_COPY) {
            NAMED_GRAPH(Name_);
            REFERENCE_GRAPH();

            COMPONENT_GRAPH(
                (Name_, ccstd::pmr::string, mNames)
                (Layout_, ccstd::pmr::string, mLayoutNodes)
                (Data_, RenderData, mData)
                (Valid_, bool, mValid)
            );

            POLYMORPHIC_GRAPH(
                (RasterPass_, RasterPass, mRasterPasses)
                (RasterSubpass_, RasterSubpass, mRasterSubpasses)
                (ComputeSubpass_, ComputeSubpass, mComputeSubpasses)
                (Compute_, ComputePass, mComputePasses)
                (Resolve_, ResolvePass, mResolvePasses)
                (Copy_, CopyPass, mCopyPasses)
                (Move_, MovePass, mMovePasses)
                (Raytrace_, RaytracePass, mRaytracePasses)
                (Queue_, RenderQueue, mRenderQueues)
                (Scene_, SceneData, mScenes)
                (Blit_, Blit, mBlits)
                (Dispatch_, Dispatch, mDispatches)
                (Clear_, ccstd::pmr::vector<ClearView>, mClearViews)
                (Viewport_, gfx::Viewport, mViewports)
            );
            PUBLIC(
                ((PmrUnorderedStringMap<ccstd::pmr::string, uint32_t>), mIndex, _)
                (ccstd::pmr::vector<RenderGraph::vertex_descriptor>, mSortedVertices, _)
            );
            MEMBER_FUNCTIONS(R"(
vertex_descriptor getPassID(vertex_descriptor nodeID) const;
ccstd::string print(boost::container::pmr::memory_resource* scratch) const;
)");
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

} // namespace Cocos::Meta
