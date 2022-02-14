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

void buildRenderExample(ModuleBuilder& builder, Features features) {
    MODULE(RenderExample,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "RenderExample",
        .mRequires = { "RenderGraph", "LayoutGraph" },
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);
        NAMESPACE_BEG(example);

        ENUM_CLASS(DependencyType) {
            ENUMS(Order, Value);
        }
        STRUCT(RenderPassNode) {
            PUBLIC(
                (PmrFlatSet<uint32_t>, mOutputs, _)
                (PmrFlatSet<uint32_t>, mInputs, _)
            );
        }

        STRUCT(RenderPassTraits) {
            PUBLIC(
                (bool, mKeep, false)
            );
        }

        PMR_GRAPH(RenderDependencyGraph, _, DependencyType, .mFlags = NO_MOVE_NO_COPY) {
            COMPONENT_GRAPH(
                (Pass_, RenderPassNode, mPasses)
                (ValueID_, PmrFlatSet<uint32_t>, mValueIDs)
                (PassID_, RenderGraph::vertex_descriptor, mPassIDs)
                (Traits_, RenderPassTraits, mTraits)
            );
            COMPONENT_BIMAP(PmrUnorderedMap, mPassIndex, PassID_);

            PUBLIC(
                ((PmrUnorderedMap<PmrString, uint32_t>), mValueIndex, _)
                (boost::container::pmr::vector<PmrString>, mValueNames, _)
                (boost::container::pmr::vector<ResourceGraph::vertex_descriptor>, mResourceHandles, _)
            );
        }

        STRUCT(RenderValueNode, .mFlags = EQUAL | HASH_COMBINE) {
            PUBLIC(
                (uint32_t, mPassID, 0xFFFFFFFF)
                (uint32_t, mValueID, 0xFFFFFFFF)
            );
            CNTR(mPassID, mValueID);
        }

        PMR_GRAPH(RenderValueGraph, _, _, .mFlags = NO_MOVE_NO_COPY) {
            COMPONENT_GRAPH(
                (Node_, RenderValueNode, mNodes)
            );
            COMPONENT_BIMAP(PmrUnorderedMap, mIndex, Node_);
        }

        STRUCT(RenderCompiler, .mFlags = NO_MOVE_NO_COPY | NO_DEFAULT_CNTR) {
            PUBLIC(
                (ResourceGraph&, mResourceGraph, _)
                (RenderGraph&, mGraph, _)
                (LayoutGraph&, mLayoutGraph, _)
                (boost::container::pmr::memory_resource*, mScratch, nullptr)
            );
            CNTR(mResourceGraph, mGraph, mLayoutGraph, mScratch);
            MEMBER_FUNCTIONS(R"(
int validate() const;
int audit(std::ostream& oss) const;
int compile();
)");
        }

        NAMESPACE_END(example);
        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
