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

// clang-format off

void buildFGDispatcher(ModuleBuilder& builder, Features features) {
    MODULE(FGDispatcher,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "FGDispatcher",
        .mJsbHeaders = R"(#include "cocos/bindings/auto/jsb_gfx_auto.h")",
        .mRequires = { "RenderGraph", "LayoutGraph", "Gfx" },
        .mHeader = R"(#include "gfx-base/GFXDef-common.h"
)",
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        STRUCT(NullTag) {
        };

        STRUCT(ResourceLifeRecord) {
            PUBLIC(
                (uint32_t, mStart, 0)
                (uint32_t, mEnd, 0)
            );
        };

        STRUCT(LeafStatus) {
            PUBLIC(
                (bool, mIsExternal, false)
                (bool, mNeedCulling, false)
            );
        };

        STRUCT(AccessStatus) {
            PUBLIC(
                (gfx::AccessFlagBit, mAccessFlag, gfx::AccessFlagBit::NONE)
                (gfx::ResourceRange, mRange, _)
            );
        }

        STRUCT(ResourceAccessNode) {
            PUBLIC(
                ((PmrFlatMap<ccstd::pmr::string, AccessStatus>), mResourceStatus, _)
            );
        }

        STRUCT(LayoutAccess, .mTrivial = true) {
            PUBLIC(
                (gfx::AccessFlagBit, mPrevAccess, gfx::AccessFlagBit::NONE)
                (gfx::AccessFlagBit, mNextAccess, gfx::AccessFlagBit::NONE)
            );
        }

        STRUCT(AttachmentInfo) {
            PUBLIC(
                (ccstd::pmr::string, mParentName, _)
                (uint32_t, mAttachmentIndex, 0)
                (uint32_t, mIsResolveView, 0)
            );
        };

        STRUCT(FGRenderPassInfo) {
            PUBLIC(
                (ccstd::vector<LayoutAccess>, mColorAccesses, _)
                (LayoutAccess, mDsAccess, _)
                (LayoutAccess, mDsResolveAccess, _)
                (gfx::RenderPassInfo, mRpInfo, _)
                (ccstd::pmr::vector<ccstd::pmr::string>, mOrderedViews, _)
                ((PmrTransparentMap<ccstd::pmr::string, AttachmentInfo>), mViewIndex, _)
                (uint32_t, mResolveCount, 0)
                (uint32_t, mUniqueRasterViewCount, 0)
            );
        }

        STRUCT(Barrier) {
            PUBLIC(
                (ResourceGraph::vertex_descriptor, mResourceID, 0xFFFFFFFF, "resource ID")
                (gfx::BarrierType, mType, gfx::BarrierType::FULL)
                (gfx::GFXObject*, mBarrier, nullptr)
                (RenderGraph::vertex_descriptor, mBeginVert, 0xFFFFFFFF)
                (RenderGraph::vertex_descriptor, mEndVert, 0xFFFFFFFF)
                (AccessStatus, mBeginStatus, _)
                (AccessStatus, mEndStatus, _)
            );
        }

        STRUCT(BarrierNode) {
            PUBLIC(
                (ccstd::vector<Barrier>, mFrontBarriers, _)
                (ccstd::vector<Barrier>, mRearBarriers, _)
            );
        }

        STRUCT(SliceNode) {
            PUBLIC(
                (bool, mFull, false)
                (ccstd::vector<uint32_t>, mMips, _)
            );
        }

        STRUCT(TextureNode) {
            PUBLIC(
                (bool, mFull, false)
                (ccstd::vector<SliceNode>, mSlices, _)
            );
        }

        STRUCT(ResourceNode) {
            PUBLIC(
                (bool, mFull, false)
                (ccstd::vector<TextureNode>, mPlanes, _)
            );
        }

        PMR_GRAPH(ResourceAccessGraph, _, _, .mFlags = NO_MOVE_NO_COPY) {
            PUBLIC(
                (ccstd::pmr::vector<ccstd::pmr::string>, mResourceNames, _)
                ((PmrUnorderedStringMap<ccstd::pmr::string, uint32_t>), mResourceIndex, _)
                (ResourceAccessGraph::vertex_descriptor, mPresentPassID, 0xFFFFFFFF, "present pass")
                ((PmrFlatMap<ResourceAccessGraph::vertex_descriptor, LeafStatus>), mLeafPasses, _)
                ((PmrFlatSet<ResourceAccessGraph::vertex_descriptor>), mCulledPasses, _)
                ((PmrFlatMap<ccstd::pmr::string, ResourceLifeRecord>), mResourceLifeRecord, _)
                (ccstd::pmr::vector<ResourceAccessGraph::vertex_descriptor>, mTopologicalOrder, _)
                ((PmrTransparentMap<ccstd::pmr::string, PmrFlatMap<uint32_t, AccessStatus>>), mResourceAccess, _)

                ((PmrFlatMap<ccstd::pmr::string, PmrFlatMap<ccstd::pmr::string, ccstd::pmr::string>>), mMovedTarget, _)
                ((PmrFlatMap<ccstd::pmr::string, AccessStatus>), mMovedSourceStatus, _)
                ((PmrFlatMap<ccstd::pmr::string, ResourceNode>), mMovedTargetStatus, _)
            );
            COMPONENT_GRAPH(
                (PassID_, RenderGraph::vertex_descriptor, mPassID)
                (PassNode_, ResourceAccessNode, mPassResource)
                (RenderPassInfo_, FGRenderPassInfo, mRpInfo)
                (Barrier_, BarrierNode, mBarrier)
            );
            COMPONENT_BIMAP(PmrUnorderedMap, mPassIndex, PassID_);
            MEMBER_FUNCTIONS(R"(
                LayoutAccess getAccess(ccstd::pmr::string, RenderGraph::vertex_descriptor vertID);
            )");
        }

        PMR_GRAPH(RelationGraph, _, _, .mFlags = NO_MOVE_NO_COPY) {
            COMPONENT_GRAPH(
                (DescID_, ResourceAccessGraph::vertex_descriptor, mDescID)
            );
            COMPONENT_BIMAP(PmrUnorderedMap, mVertexMap, DescID_);
        }

        STRUCT(RenderingInfo) {
            PUBLIC(
                (gfx::RenderPassInfo, mRenderpassInfo, _)
                (gfx::FramebufferInfo, mFramebufferInfo, _)
                (ccstd::pmr::vector<gfx::Color>, mClearColors, _)
                (float, mClearDepth, 0)
                (uint8_t, mClearStencil, 0)
            );
        };

        STRUCT(FrameGraphDispatcher, .mFlags = NO_MOVE_NO_COPY | NO_DEFAULT_CNTR) {
            PUBLIC(
                (ResourceAccessGraph, mResourceAccessGraph, _)
                (ResourceGraph&, mResourceGraph, _)
                (const RenderGraph&, mRenderGraph, _)
                (const LayoutGraphData&, mLayoutGraph, _)
                (boost::container::pmr::memory_resource*, mScratch, nullptr)
                (RelationGraph, mRelationGraph, _)
            );
            
            PRIVATE(
                (bool, mEnablePassReorder, false)
                (bool, mEnableAutoBarrier, true)
                (bool, mEnableMemoryAliasing, false)
                (bool, mAccessGraphBuilt, false)
                (float, mParalellExecWeight, 0.0F)
            );

            CNTR(mResourceGraph, mRenderGraph, mLayoutGraph, mScratch);

            
            MEMBER_FUNCTIONS(R"(

void enablePassReorder(bool enable);

// how much paralell-execution weights during pass reorder,
// eg:0.3 means 30% of effort aim to paralellize execution, other 70% aim to decrease memory using.
// 0 by default 
void setParalellWeight(float paralellExecWeight);

void enableMemoryAliasing(bool enable);

void run();

const BarrierNode& getBarrier(RenderGraph::vertex_descriptor u) const;

const ResourceAccessNode& getAccessNode(RenderGraph::vertex_descriptor u) const;

const gfx::RenderPassInfo& getRenderPassInfo(RenderGraph::vertex_descriptor u) const;
    
RenderingInfo getRenderPassAndFrameBuffer(RenderGraph::vertex_descriptor u, const ResourceGraph& resg) const;
    
LayoutAccess getResourceAccess(ResourceGraph::vertex_descriptor r, RenderGraph::vertex_descriptor p) const;

// those resource been moved point to another resID
ResourceGraph::vertex_descriptor realResourceID(const ccstd::pmr::string& name) const;

PmrFlatMap<NameLocalID, ResourceGraph::vertex_descriptor> buildDescriptorIndex(
    const PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>&computeViews,
    const PmrTransparentMap<ccstd::pmr::string, RasterView>& rasterViews,
    boost::container::pmr::memory_resource* scratch) const;

PmrFlatMap<NameLocalID, ResourceGraph::vertex_descriptor> buildDescriptorIndex(
    const PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>&computeViews,
    boost::container::pmr::memory_resource* scratch) const;
)");
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
