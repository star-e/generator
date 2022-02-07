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

void buildRenderExecutor(ModuleBuilder& builder, Features features) {
    MODULE(RenderExecutor,
        .mFolder = "cocos/renderer/pipeline",
        .mFilePrefix = "RenderExecutor",
        .mTypescriptFolder = "cocos/core/pipeline",
        .mTypescriptFilePrefix = "executor",
        .mRequires = { "RenderCommon", /*"RenderGraph", */"Gfx" },
        .mHeader = R"(#include <cocos/renderer/gfx-base/GFXBuffer.h>
#include <cocos/renderer/gfx-base/GFXTexture.h>
)") {
        NAMESPACE(cc) {
            NAMESPACE(render) {
                //STRUCT(RenderPass, .mFlags = NO_SERIALIZATION) {
                //    PUBLIC(
                //        (PmrFlatSet<uint32_t>, mOutputs, _)
                //        (PmrFlatSet<uint32_t>, mInputs, _)
                //    );
                //}

                //PMR_GRAPH(RenderPassGraph, _, DependencyType, .mFlags = NO_MOVE_NO_COPY | NO_SERIALIZATION) {
                //    COMPONENT_GRAPH(
                //        (Pass_, RenderPass, mPasses)
                //        (ValueID_, PmrFlatSet<uint32_t>, mValueIDs)
                //        (PassID_, RenderGraph::vertex_descriptor, mPassIDs)
                //    );
                //    COMPONENT_BIMAP(std::pmr::unordered_map, mPassIndex, PassID_);

                //    PUBLIC(
                //        ((PmrFlatMap<std::pmr::u8string, uint32_t>), mValueIndex, _)
                //        (std::pmr::vector<std::pmr::u8string>, mValueNames, _)
                //    );
                //}

                //STRUCT(RenderValueNode, .mFlags = EQUAL | HASH_COMBINE | NO_SERIALIZATION) {
                //    PUBLIC(
                //        (uint32_t, mPassID, 0xFFFFFFFF)
                //        (uint32_t, mValueID, 0xFFFFFFFF)
                //    );
                //    CNTR(mPassID, mValueID);
                //}

                //PMR_GRAPH(RenderValueGraph, _, _, .mFlags = NO_MOVE_NO_COPY | NO_SERIALIZATION) {
                //    COMPONENT_GRAPH(
                //        (Node_, RenderValueNode, mNodes)
                //    );
                //    COMPONENT_BIMAP(std::pmr::unordered_map, mIndex, Node_);
                //}

                PMR_GRAPH(DeviceResourceGraph, _, _, .mFlags = NO_MOVE_NO_COPY) {
                    NAMED_GRAPH(Name_);
                    COMPONENT_GRAPH(
                        (Name_, std::string, mName)
                        (RefCount_, int32_t, mRefCounts)
                    );
                    POLYMORPHIC_GRAPH(
                        (Buffer_, std::unique_ptr<gfx::Buffer>, mBuffers)
                        (Texture_, std::unique_ptr<gfx::Texture>, mTextures)
                    );
                }
            }
        }
    }
}

}
