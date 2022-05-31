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
        .mHeader = R"(#include "gfx-base/GFXDef-common.h")",
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        ENUM_CLASS(PassType) {
            ENUMS(    
                RASTER,
                COMPUTE,
                COPY,
                MOVE,
                RAYTRACE,
                PRESENT);
        }

        STRUCT(NullTag) {
        };

        STRUCT(Range) {
            PUBLIC(
                (uint32_t, mMipLevels, 0xFFFFFFFF)
                (uint32_t, mNumSlices, 0xFFFFFFFF)
                (uint32_t, mMostDetailedMip, 0xFFFFFFFF)
                (uint32_t, mFirstSlice, 0xFFFFFFFF)
                (uint32_t, mPlaneSlice, 0xFFFFFFFF)
            );
        };

        STRUCT(AccessStatus) {
            PUBLIC(
                (uint32_t, mVertID, 0xFFFFFFFF)
                (gfx::ShaderStageFlagBit, mVisibility, gfx::ShaderStageFlagBit::NONE)
                (gfx::MemoryAccessBit, mAccess, gfx::MemoryAccessBit::NONE)
                (PassType, mPassType, PassType::RASTER)
                (Range, mRange, _)
            );
        }

        STRUCT(ResourceTransition) {
            PUBLIC(
                (AccessStatus, mLastStatus, _)
                (AccessStatus, mCurrStatus, _)
            );
        }

        //resourceStatus
        STRUCT(ResourceAccessNode) {
            PUBLIC(
                (std::vector<AccessStatus>, mAttachemntStatus, _)
                (uint32_t, mCircuitFlag, _)
            );
        }

        PMR_GRAPH(ResourceAccessGraph, _, _, .mFlags = NO_MOVE_NO_COPY) {
            PUBLIC(
                (ccstd::pmr::vector<ccstd::pmr::string>, mResourceNames, _)
                ((PmrUnorderedStringMap<ccstd::pmr::string, uint32_t>), mResourceIndex, _)
                (RenderGraph::vertex_descriptor, mPresentPassID, 0xFFFFFFFF, "present pass")
                (ccstd::pmr::vector<RenderGraph::vertex_descriptor>, mExternalPasses, _)
            );
            COMPONENT_GRAPH(
                (PassID_, RenderGraph::vertex_descriptor, mPassID)
                (AccessNode_, ResourceAccessNode, mAccess)
            );
            COMPONENT_BIMAP(PmrUnorderedMap, mPassIndex, PassID_);
        }

        GRAPH(EmptyGraph, _, _, .mFlags = NO_MOVE_NO_COPY) {

        }

        STRUCT(Barrier) {
            PUBLIC(
                (RenderGraph::vertex_descriptor, mResourceID, 0xFFFFFFFF, "resource ID")
                (AccessStatus, mBeginStatus, _)
                (AccessStatus, mEndStatus, _)
            );
        }

        STRUCT(BarrierNode) {
            PUBLIC(
                (std::vector<Barrier>, mFrontBarriers, _)
                (std::vector<Barrier>, mRearBarriers, _)
            );
        }

        STRUCT(FrameGraphDispatcher, .mFlags = NO_MOVE_NO_COPY | NO_DEFAULT_CNTR) {
            PUBLIC(
                (ResourceGraph&, mResourceGraph, _)
                (RenderGraph&, mGraph, _)
                (LayoutGraphData&, mLayoutGraph, _)
                (boost::container::pmr::memory_resource*, mScratch, nullptr)
                ((PmrFlatMap<ccstd::pmr::string, ResourceTransition>), mExternalResMap, _)
                (EmptyGraph, mRelationGraph, _)
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
void enablePassReorder(bool enable);

// how much paralell-execution weights during pass reorder,
// eg:0.3 means 30% of effort aim to paralellize execution, other 70% aim to decrease memory using.
// 0 by default 
void setParalellWeight(float paralellExecWeight);

void enableMemoryAliasing(bool enable);

void run();
)");
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
