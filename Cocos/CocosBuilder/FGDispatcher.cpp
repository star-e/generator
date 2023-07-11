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
#include <variant>)",
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

        STRUCT(BufferRange) {
            PUBLIC(
                (uint32_t, mOffset, 0)
                (uint32_t, mSize, 0)
            );
        };

        STRUCT(TextureRange) {
            PUBLIC(
                (uint32_t, mFirstSlice, 0)
                (uint32_t, mNumSlices, 1)
                (uint32_t, mMipLevel, 0)
                (uint32_t, mLevelCount, 1)
            );
        };
        
        VARIANT(Range, (BufferRange, TextureRange), LESS)
        VARIANT(ResourceUsage, (gfx::BufferUsageBit, gfx::TextureUsageBit), LESS)

        STRUCT(AccessStatus) {
            PUBLIC(
                (uint32_t, mVertID, 0xFFFFFFFF)
                (gfx::ShaderStageFlagBit, mVisibility, gfx::ShaderStageFlagBit::NONE)
                (gfx::MemoryAccessBit, mAccess, gfx::MemoryAccessBit::NONE)
                (gfx::PassType, mPassType, gfx::PassType::RASTER)
                (gfx::AccessFlagBit, mAccessFlag, gfx::AccessFlagBit::NONE)
                (ResourceUsage, mUsage, _)
                (Range, mRange, _)
            );
        }

        STRUCT(ResourceTransition) {
            PUBLIC(
                (AccessStatus, mLastStatus, _)
                (AccessStatus, mCurrStatus, _)
            );
        }

        STRUCT(ResourceAccessNode) {
            PUBLIC(
                (std::vector<AccessStatus>, mAttachmentStatus, _)
                (ResourceAccessNode*, mNextSubpass, nullptr)
            );
        }

        STRUCT(LayoutAccess) {
            PUBLIC(
                (gfx::AccessFlagBit, mPrevAccess, gfx::AccessFlagBit::NONE)
                (gfx::AccessFlagBit, mNextAccess, gfx::AccessFlagBit::NONE)
            );
        }

        STRUCT(FGRenderPassInfo) {
            PUBLIC(
                (std::vector<LayoutAccess>, mColorAccesses, _)
                (LayoutAccess, mDsAccess, _)
                (LayoutAccess, mDsResolveAccess, _)
                (gfx::RenderPassInfo, mRpInfo, _)
                (std::vector<std::string>, mOrderedViews, _)
                (bool, mNeedResolve, false)
            );
        }

        PMR_GRAPH(ResourceAccessGraph, _, _, .mFlags = NO_MOVE_NO_COPY) {
            PUBLIC(
                (ccstd::pmr::vector<ccstd::pmr::string>, mResourceNames, _)
                ((PmrUnorderedStringMap<ccstd::pmr::string, uint32_t>), mResourceIndex, _)
                (ResourceAccessGraph::vertex_descriptor, mPresentPassID, 0xFFFFFFFF, "present pass")
                ((PmrFlatMap<ResourceAccessGraph::vertex_descriptor, LeafStatus>), mLeafPasses, _)
                ((PmrFlatSet<ResourceAccessGraph::vertex_descriptor>), mCulledPasses, _)
                ((PmrFlatMap<uint32_t, ResourceTransition>), mAccessRecord, _)
                ((PmrFlatMap<ccstd::pmr::string, ResourceLifeRecord>), mResourceLifeRecord, _)
                (ccstd::pmr::vector<ResourceAccessGraph::vertex_descriptor>, mTopologicalOrder, _)
                ((PmrFlatMap<ResourceAccessGraph::vertex_descriptor, FGRenderPassInfo>), mRpInfos, _)
                ((PmrFlatMap<RenderGraph::vertex_descriptor, uint32_t>), mSubpassIndex, _)
            );
            COMPONENT_GRAPH(
                (PassID_, RenderGraph::vertex_descriptor, mPassID)
                (AccessNode_, ResourceAccessNode, mAccess)
            );
            COMPONENT_BIMAP(PmrUnorderedMap, mPassIndex, PassID_);
            MEMBER_FUNCTIONS(R"(
~ResourceAccessGraph() {
    for (auto& node : access) {
        auto* resNode = node.nextSubpass;
        node.nextSubpass = nullptr;
        while(resNode) {
            auto* oldResNode = resNode;
            resNode = resNode->nextSubpass;
            oldResNode->nextSubpass = nullptr;
            delete oldResNode;
        }
    }
}

)");
        }

        PMR_GRAPH(RelationGraph, _, _, .mFlags = NO_MOVE_NO_COPY) {
            COMPONENT_GRAPH(
                (DescID_, ResourceAccessGraph::vertex_descriptor, mDescID)
            );
            COMPONENT_BIMAP(PmrUnorderedMap, mVertexMap, DescID_);
        }

        STRUCT(Barrier) {
            PUBLIC(
                (ResourceGraph::vertex_descriptor, mResourceID, 0xFFFFFFFF, "resource ID")
                (gfx::BarrierType, mType, gfx::BarrierType::FULL)
                (gfx::GFXObject*, mBarrier, nullptr)
                (AccessStatus, mBeginStatus, _)
                (AccessStatus, mEndStatus, _)
            );
        }

        STRUCT(BarrierPair) {
            PUBLIC(
                (std::vector<Barrier>, mFrontBarriers, _)
                (std::vector<Barrier>, mRearBarriers, _)
            );
        }

        STRUCT(BarrierNode) {
            PUBLIC(
                (BarrierPair, mBlockBarrier, _)
                (std::vector<BarrierPair>, mSubpassBarriers, _)
            );
        }

        STRUCT(FrameGraphDispatcher, .mFlags = NO_MOVE_NO_COPY | NO_DEFAULT_CNTR) {
            PUBLIC(
                (ResourceAccessGraph, mResourceAccessGraph, _)
                (ResourceGraph&, mResourceGraph, _)
                (const RenderGraph&, mGraph, _)
                (const LayoutGraphData&, mLayoutGraph, _)
                (boost::container::pmr::memory_resource*, mScratch, nullptr)
                ((PmrFlatMap<ccstd::pmr::string, ResourceTransition>), mExternalResMap, _)
                (RelationGraph, mRelationGraph, _)
            );
            
            PRIVATE(
                (bool, mEnablePassReorder, false)
                (bool, mEnableAutoBarrier, true)
                (bool, mEnableMemoryAliasing, false)
                (bool, mAccessGraphBuilt, false)
                (float, mParalellExecWeight, 0.0F)
            );

            CNTR(mResourceGraph, mGraph, mLayoutGraph, mScratch);

            
            MEMBER_FUNCTIONS(R"(
using BarrierMap = PmrMap<ResourceAccessGraph::vertex_descriptor, BarrierNode>;

void enablePassReorder(bool enable);

// how much paralell-execution weights during pass reorder,
// eg:0.3 means 30% of effort aim to paralellize execution, other 70% aim to decrease memory using.
// 0 by default 
void setParalellWeight(float paralellExecWeight);

void enableMemoryAliasing(bool enable);

void run();

const ResourceAccessNode& getAttachmentStatus(RenderGraph::vertex_descriptor renderGraphVertID) const;

inline const BarrierMap& getBarriers() const { return barrierMap; }

BarrierMap barrierMap;
)");
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
