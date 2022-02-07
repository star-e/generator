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

void buildRenderCompiler(ModuleBuilder& builder, Features features) {
    MODULE(RenderCompiler,
        .mFolder = "cocos/renderer/pipeline",
        .mFilePrefix = "RenderCompiler",
        .mRequires = { "RenderGraph" },
) {
        NAMESPACE(cc) {
            NAMESPACE(render) {
                ENUM_CLASS(DependencyType) {
                    ENUMS(Order, Value);
                }
                STRUCT(RenderPass, .mFlags = NO_SERIALIZATION) {
                    PUBLIC(
                        (PmrFlatSet<uint32_t>, mOutputs, _)(PmrFlatSet<uint32_t>, mInputs, _));
                }

                PMR_GRAPH(RenderDependencyGraph, _, DependencyType, .mFlags = NO_MOVE_NO_COPY | NO_SERIALIZATION) {
                    COMPONENT_GRAPH(
                        (Pass_, RenderPass, mPasses)(ValueID_, PmrFlatSet<uint32_t>, mValueIDs)(PassID_, RenderGraph::vertex_descriptor, mPassIDs));
                    COMPONENT_BIMAP(PmrUnorderedMap, mPassIndex, PassID_);

                    PUBLIC(
                        ((PmrFlatMap<PmrString, uint32_t>), mValueIndex, _)(boost::container::pmr::vector<PmrString>, mValueNames, _));
                }

                STRUCT(RenderValueNode, .mFlags = EQUAL | HASH_COMBINE | NO_SERIALIZATION) {
                    PUBLIC(
                        (uint32_t, mPassID, 0xFFFFFFFF)(uint32_t, mValueID, 0xFFFFFFFF));
                    CNTR(mPassID, mValueID);
                }

                PMR_GRAPH(RenderValueGraph, _, _, .mFlags = NO_MOVE_NO_COPY | NO_SERIALIZATION) {
                    COMPONENT_GRAPH(
                        (Node_, RenderValueNode, mNodes));
                    COMPONENT_BIMAP(PmrUnorderedMap, mIndex, Node_);
                }
            }
        }
    }
}

}