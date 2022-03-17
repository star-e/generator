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
)",
        .mTypescriptFolder = "cocos/core/pipeline/custom",
        .mTypescriptFilePrefix = "render-graph",
        .mRequires = { "RenderCommon", "Gfx", "Camera", "PipelineSceneData" },
        .mHeader = R"(#include "cocos/renderer/gfx-base/GFXBuffer.h"
#include "cocos/renderer/gfx-base/GFXTexture.h"
#include "cocos/renderer/gfx-base/GFXSwapchain.h"
#include "cocos/renderer/gfx-base/states/GFXSampler.h"
)",
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        STRUCT(ResourceDesc) {
            PUBLIC(
                (ResourceDimension, mDimension, _)
                (uint32_t, mAlignment, 0)
                (uint32_t, mWidth, 0)
                (uint32_t, mHeight, 0)
                (uint16_t, mDepthOrArraySize, 0)
                (uint16_t, mMipLevels, 0)
                (gfx::Format, mFormat, gfx::Format::UNKNOWN)
                (gfx::SampleCount, mSampleCount, gfx::SampleCount::ONE)
                (gfx::TextureFlagBit, mTextureFlags, gfx::TextureFlagBit::NONE)
                (ResourceFlags, mFlags, ResourceFlags::NONE)
            );
            TS_INIT(mFormat, Format.UNKNOWN);
            TS_INIT(mSampleCount, SampleCount.ONE);
            TS_INIT(mTextureFlags, TextureFlagBit.NONE);
            TS_INIT(mFlags, ResourceFlags.NONE);
        }

        STRUCT(ResourceTraits) {
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

        STRUCT(RenderSwapchain) {
            PUBLIC(
                (gfx::Swapchain*, mSwapchain, nullptr)
                (uint32_t, mCurrentID, 0)
                (uint32_t, mNumBackBuffers, 0)
            );
            TS_INIT(mSwapchain, null);
            CNTR_NO_DEFAULT(mSwapchain);
        }

        STRUCT(ResourceStates) {
            PUBLIC(
                (gfx::AccessFlagBit, mStates, gfx::AccessFlagBit::NONE)
            );
            TS_INIT(mStates, AccessFlagBit.NONE);
        }

        STRUCT(ManagedResource) {
            PUBLIC(
                (uint32_t, mUnused, 0)
            );
        }

        TAGS((_), Managed_, PersistentBuffer_, PersistentTexture_, Swapchain_);

        PMR_GRAPH(ResourceGraph, _, _) {
            NAMED_GRAPH(Name_);
            COMPONENT_GRAPH(
                (Name_, PmrString, mNames)
                (Desc_, ResourceDesc, mDescs)
                (Traits_, ResourceTraits, mTraits)
                (States_, ResourceStates, mStates)
            );
            COMPONENT_BIMAP(PmrUnorderedMap, mValueIndex, Name_);
            POLYMORPHIC_GRAPH(
                (Managed_, ManagedResource, mResources)
                (PersistentBuffer_, IntrusivePtr<gfx::Buffer>, mBuffers)
                (PersistentTexture_, IntrusivePtr<gfx::Texture>, mTextures)
                (Swapchain_, RenderSwapchain, mSwapchains)
            );
        }
        
        PROJECT_TS(IntrusivePtr<gfx::Buffer>, Buffer);
        PROJECT_TS(IntrusivePtr<gfx::Texture>, Texture);

        //STRUCT(NodeValue) {
        //    PUBLIC(
        //        (std::pmr::u8string, mNodeName, _)
        //        (PmrString, mValueName, _)
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
        //        (PmrString, mValue, "")
        //    );
        //}

        ENUM_CLASS(AttachmentType) {
            ENUMS(RENDER_TARGET, DEPTH_STENCIL);
        }
        ENUM_CLASS(AccessType) {
            ENUMS(READ, READ_WRITE, WRITE);
        }

        STRUCT(RasterView, .mFlags = JSB | PMR_DEFAULT) {
            PUBLIC(
                (PmrString, mSlotName, _)
                (AccessType, mAccessType, AccessType::WRITE)
                (AttachmentType, mAttachmentType, _)
                (gfx::LoadOp, mLoadOp, gfx::LoadOp::LOAD)
                (gfx::StoreOp, mStoreOp, gfx::StoreOp::STORE)
                (gfx::ClearFlagBit, mClearFlags, gfx::ClearFlagBit::ALL)
                (gfx::Color, mClearColor, _)
            );
            TS_INIT(mAccessType, AccessType.WRITE);
            TS_INIT(mLoadOp, LoadOp.LOAD);
            TS_INIT(mStoreOp, StoreOp.STORE);
            TS_INIT(mClearFlags, ClearFlagBit.ALL);
            CNTR(mSlotName, mAccessType, mAttachmentType, mLoadOp, mStoreOp, mClearFlags, mClearColor);
        }

        ENUM_CLASS(ClearValueType) {
            ENUMS(FLOAT_TYPE, INT_TYPE);
        }

        STRUCT(ComputeView, .mFlags = JSB | PMR_DEFAULT) {
            PUBLIC(
                (PmrString, mName, _)
                (AccessType, mAccessType, AccessType::READ)
                (gfx::ClearFlagBit, mClearFlags, gfx::ClearFlagBit::NONE)
                (gfx::Color, mClearColor, _)
                (ClearValueType, mClearValueType, _)
            );
            MEMBER_FUNCTIONS(R"(
bool isRead() const {
    return accessType != AccessType::WRITE;
}
bool isWrite() const {
    return accessType != AccessType::READ;
}
)");
            TS_INIT(mAccessType, AccessType.READ);
            TS_INIT(mClearFlags, ClearFlagBit.NONE);
        }

        STRUCT(RasterSubpass) {
            PUBLIC(
                ((PmrTransparentMap<PmrString, RasterView>), mRasterViews, _)
                ((PmrTransparentMap<PmrString, boost::container::pmr::vector<ComputeView>>), mComputeViews, _)
            );
        }

        PMR_GRAPH(SubpassGraph, _, _) {
            NAMED_GRAPH(Name_);
            COMPONENT_GRAPH(
                (Name_, PmrString, mNames)
                (Subpass_, RasterSubpass, mSubpasses)
            );
        }
                
        STRUCT(RasterPass) {
            PUBLIC(
                ((PmrTransparentMap<PmrString, RasterView>), mRasterViews, _)
                ((PmrTransparentMap<PmrString, boost::container::pmr::vector<ComputeView>>), mComputeViews, _)
                (SubpassGraph, mSubpassGraph, _)
            );
        }

        STRUCT(ComputePass) {
            PUBLIC(
                ((PmrTransparentMap<PmrString, boost::container::pmr::vector<ComputeView>>), mComputeViews, _)
            );
        }

        STRUCT(CopyPair, .mFlags = PMR_DEFAULT | JSB) {
            PUBLIC(
                (PmrString, mSource, _)
                (PmrString, mTarget, _)
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

        STRUCT(CopyPass) {
            PUBLIC(
                (boost::container::pmr::vector<CopyPair>, mCopyPairs, _)
            );
        }

        STRUCT(MovePair, .mFlags = PMR_DEFAULT | JSB) {
            PUBLIC(
                (PmrString, mSource, _)
                (PmrString, mTarget, _)
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

        STRUCT(MovePass) {
            PUBLIC(
                (boost::container::pmr::vector<MovePair>, mMovePairs, _)
            );
        }

        STRUCT(RaytracePass) {
            PUBLIC(
                ((PmrTransparentMap<PmrString, boost::container::pmr::vector<ComputeView>>), mComputeViews, _)
            );
        }

        TAGS((_), Queue_, Scene_, Dispatch_, Blit_, Present_);

        STRUCT(RenderQueue) {
            PUBLIC(
                (QueueHint, mHint, QueueHint::RENDER_OPAQUE)
            );
            TS_INIT(mHint, QueueHint.RENDER_OPAQUE);
            CNTR(mHint);
        }

        STRUCT(SceneData) {
            PUBLIC(
                (PmrString, mName, _)
                (scene::Camera*, mCamera, nullptr)
                (boost::container::pmr::vector<PmrString>, mScenes, _)
            );
            CNTR(mName);
        }

        STRUCT(Dispatch) {
            PUBLIC(
                (PmrString, mShader, _)
                (uint32_t, mThreadGroupCountX, 0)
                (uint32_t, mThreadGroupCountY, 0)
                (uint32_t, mThreadGroupCountZ, 0)
            );
            CNTR(mShader, mThreadGroupCountX, mThreadGroupCountY, mThreadGroupCountZ);
        }

        STRUCT(Blit) {
            PUBLIC(
                (PmrString, mShader, _)
            );
            CNTR(mShader);
        }

        STRUCT(Present) {
            PUBLIC(
                (uint32_t, mSyncInterval, 0)
                (uint32_t, mFlags, 0)
            );
            CNTR(mSyncInterval, mFlags);
        }

        STRUCT(PresentPass) {
            PUBLIC(
                ((PmrTransparentMap<PmrString, Present>), mPresents, _)
            );
        }

        STRUCT(RenderData, .mFlags = NO_COPY) {
            PUBLIC(
                ((PmrUnorderedMap<uint32_t, boost::container::pmr::vector<uint8_t>>), mConstants, _)
                ((PmrUnorderedMap<uint32_t, std::unique_ptr<gfx::Buffer>>), mBuffers, _)
                ((PmrUnorderedMap<uint32_t, std::unique_ptr<gfx::Texture>>), mTextures, _)
                ((PmrUnorderedMap<uint32_t, std::unique_ptr<gfx::Sampler>>), mSamplers, _)
            );
        }

        PMR_GRAPH(RenderGraph, _, _, .mFlags = NO_COPY) {
            NAMED_GRAPH(Name_);
            REFERENCE_GRAPH();

            COMPONENT_GRAPH(
                (Name_, PmrString, mNames)
                (Layout_, PmrString, mLayoutNodes)
                (Data_, RenderData, mData)
            );

            POLYMORPHIC_GRAPH(
                (Raster_, RasterPass, mRasterPasses)
                (Compute_, ComputePass, mComputePasses)
                (Copy_, CopyPass, mCopyPasses)
                (Move_, MovePass, mMovePasses)
                (Present_, PresentPass, mPresentPasses)
                (Raytrace_, RaytracePass, mRaytracePasses)
                (Queue_, RenderQueue, mRenderQueues)
                (Scene_, SceneData, mScenes)
                (Blit_, Blit, mBlits)
                (Dispatch_, Dispatch, mDispatches)
            );
            PUBLIC(
                ((PmrUnorderedMap<PmrString, uint32_t>), mIndex, _)
            );
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

} // namespace Cocos::Meta
