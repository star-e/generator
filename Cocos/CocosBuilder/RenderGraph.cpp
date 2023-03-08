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

        STRUCT(ResourceDesc, .mFlags = POOL_OBJECT) {
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
                (uint32_t, mSubpassID, 0xFFFFFFFF)
                (gfx::Viewport, mViewport, _)
                (bool, mShowStatistics, false)
            );
            CNTR_NO_DEFAULT(mSubpassID);
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
                (SubpassGraph, mSubpassGraph, _)
                (uint32_t, mWidth, 0)
                (uint32_t, mHeight, 0)
                (gfx::Viewport, mViewport, _)
                (ccstd::pmr::string, mVersionName, _)
                (uint64_t, mVersion, 0)
                (bool, mShowStatistics, false)
            );
            MEMBER_FLAGS(mViewport, NOT_ELEMENT);
            MEMBER_FLAGS(mVersionName, NOT_ELEMENT);
            MEMBER_FLAGS(mVersion, NOT_ELEMENT);
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
            Framebuffer_, Swapchain_, Sampler_);

        PMR_GRAPH(ResourceGraph, _, _, .mFlags = NO_MOVE_NO_COPY) {
            NAMED_GRAPH(Name_);
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
gfx::Texture* getTexture(vertex_descriptor resID);
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
            );
        }

        STRUCT(CopyPass) {
            PUBLIC(
                (ccstd::pmr::vector<CopyPair>, mCopyPairs, _)
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
            );
            CNTR(mHint, mPhaseID);
            CNTR(mPhaseID);
        }

        STRUCT(SceneData) {
            PUBLIC(
                (ccstd::pmr::string, mName, _)
                ([[optional]] scene::Camera*, mCamera, nullptr)
                (LightInfo, mLight, _)
                (SceneFlags, mFlags, SceneFlags::NONE)
                (ccstd::pmr::vector<ccstd::pmr::string>, mScenes, _)
            );
            CNTR(mName, mFlags, mLight);
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
                ((PmrUnorderedMap<uint32_t, ccstd::pmr::vector<char>>), mConstants, _)
                ((PmrUnorderedMap<uint32_t, IntrusivePtr<gfx::Buffer>>), mBuffers, _)
                ((PmrUnorderedMap<uint32_t, IntrusivePtr<gfx::Texture>>), mTextures, _)
                ((PmrUnorderedMap<uint32_t, ObserverPtr<gfx::Sampler>>), mSamplers, _)
            );
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
            );
            MEMBER_FUNCTIONS(R"(
ccstd::string print(boost::container::pmr::memory_resource* scratch) const;
)");
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

} // namespace Cocos::Meta
