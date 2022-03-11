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

void buildNativePipeline(ModuleBuilder& builder, Features features) {
    MODULE(NativePipeline,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "NativePipeline",
        .mRequires = { "RenderInterface", "RenderCompiler" },
        .mHeader = R"(#include "cocos/renderer/pipeline/GlobalDescriptorSetManager.h"
)"
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        STRUCT(NativePipeline, .mFlags = CUSTOM_CNTR) {
            INHERITS(Pipeline);
            PUBLIC(
                (gfx::Device*, mDevice, nullptr)
                (MacroRecord, mMacros, _)
                (std::string, mConstantMacros, _)
                (pipeline::GlobalDSManager*, mGlobalDSManager, nullptr)
                (scene::Model*, mProfiler, nullptr)
                (IntrusivePtr<pipeline::PipelineSceneData>, mPipelineSceneData, _)
            );
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
