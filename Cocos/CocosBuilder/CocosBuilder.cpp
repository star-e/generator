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

#include <Cocos/AST/CppDefaultValues.h>
#include <Cocos/AST/TypescriptDefaultValues.h>
#include <Cocos/FileUtils.h>
#include <Cocos/Indent.h>
#include "CocosModules.h"

using namespace Cocos;
using namespace Cocos::Meta;

int main() {
    std::pmr::unsynchronized_pool_resource pool;
    std::pmr::memory_resource* scratch = &pool;

    // output folder
    std::filesystem::path typescriptFolder = "../../../engine";
    std::filesystem::path cppFolder = "../../../engine-native";

    ModuleBuilder builder("cocos", cppFolder, typescriptFolder, scratch);

    // type registration
    {
        // register c++/stl types
        addCppDefaultValues(builder);

        // relate c++ to ts
        projectTypescriptDefaultValues(builder);

        // build render pipeline
        buildRenderCommon(builder,
            Typescripts | Fwd | Types | Names | Reflection);
        buildLayoutGraph(builder,
            Typescripts | Fwd | Types | Names | Reflection | Graphs);
        buildRenderGraph(builder,
            Typescripts | Fwd | Types | Names | Reflection | Graphs);

        // build executor modules
        buildRenderExecutor(builder,
            Typescripts | Fwd | Types | Names | Reflection | Graphs);
    }

    builder.compile();

    std::pmr::set<std::pmr::string> files(scratch);
    // output files
    {
        // common types, shared by different modules
        builder.outputModule("RenderCommon", files);

        // descriptor layout graph
        builder.outputModule("LayoutGraph", files);

        // render graph
        builder.outputModule("RenderGraph", files);

        // executor
        builder.outputModule("RenderExecutor", files);
    }

    // copy graph interface
    {
        auto content = readFile("graph.ts");
        updateFile(typescriptFolder / "cocos/core/pipeline/graph.ts", content);
    }
    {
        auto content = readFile("GraphTypes.h");
        updateFile(cppFolder / "cocos/renderer/pipeline/GraphTypes.h", content);
    }
    {
        auto content = readFile("GraphImpl.h");
        updateFile(cppFolder / "cocos/renderer/pipeline/GraphImpl.h", content);
    }
    {
        auto content = readFile("Map.h");
        updateFile(cppFolder / "cocos/renderer/pipeline/Map.h", content);
    }
    // update cmakelists
    if (true) {
        auto cmake = readFile(cppFolder / "CMakeLists.txt", true);
        const auto pos = cmake.find("##### renderer");
        Expects(pos != cmake.npos);
        const auto beg = [&]() {
            std::string_view prefix = "cocos_source_files(\n";
            auto start = cmake.find(prefix, pos);
            if (start != cmake.npos) {
                start += prefix.size();
            }
            return start;
        }();
        Expects(pos != cmake.npos);
        const auto end = cmake.find(")", pos);

        std::pmr::string content(std::string_view(cmake).substr(beg, end - beg), scratch);
        for (const auto& file : files) {
            std::pmr::string line(file, scratch);
            line.append("\n");
            boost::algorithm::replace_all(content, line, "");
        }
        boost::algorithm::replace_all(content, " ", "");

        for (const auto& file : files) {
            content.append(file);
            content.append("\n");
        }
        std::pmr::string space("                 ");
        pmr_ostringstream oss(std::ios::out, scratch);
        copyString(oss, space, content);

        cmake.replace(beg, end - beg, oss.str());

        updateFile(cppFolder / "CMakeLists.txt", cmake);
    }

    return 0;
}
