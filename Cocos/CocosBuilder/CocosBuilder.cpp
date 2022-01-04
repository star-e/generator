/*
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2021 Xiamen Yaji Software Co., Ltd.

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

#include <Cocos/AST/CppDefaultValues.h>
#include <Cocos/AST/TypescriptDefaultValues.h>
#include <Cocos/FileUtils.h>
#include "RenderCommon.h"
#include "LayoutGraph.h"
#include "RenderGraph.h"

using namespace Cocos;
using namespace Cocos::Meta;

int main() {
    ModuleBuilder builder(std::pmr::get_default_resource());

    // output folder
    std::filesystem::path outputFolder = "../../../engine";
    
    // type registration
    {
        // register c++/stl types
        addCppDefaultValues(builder);

        // relate c++ to ts
        projectTypescriptDefaultValues(builder);

        // build render pipeline
        buildRenderCommon(builder);
        buildLayoutGraph(builder);
        buildRenderGraph(builder);

        // build executor modules
        buildRenderExecutor(builder);
    }

    // output ts files
    {
        // common types, shared by different modules
        builder.outputModule(outputFolder, "RenderCommon",
            Features::Typescripts);

        // descriptor layout graph
        builder.outputModule(outputFolder, "DescriptorLayout",
            Features::Typescripts);

        // render graph
        builder.outputModule(outputFolder, "RenderGraph",
            Features::Typescripts);

        // executor
        builder.outputModule(outputFolder, "RenderExecutor",
            Features::Typescripts);
    }

    // copy graph interface
    {
        auto content = readFile("graph.ts");
        updateFile(outputFolder / "cocos/core/pipeline/graph.ts", content);
    }

    return 0;
}
