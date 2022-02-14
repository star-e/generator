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
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "RenderExecutor",
        .mTypescriptFolder = "cocos/core/pipeline/custom",
        .mTypescriptFilePrefix = "executor",
        .mRequires = { "RenderCommon", /*"RenderGraph", */"Gfx" },
        .mHeader = R"(#include <cocos/renderer/gfx-base/GFXBuffer.h>
#include <cocos/renderer/gfx-base/GFXTexture.h>
)") {
        NAMESPACE(cc) {
            NAMESPACE(render) {
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
