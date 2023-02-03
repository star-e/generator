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
#include <iostream>

using namespace Cocos;
using namespace Cocos::Meta;

int main() {
    std::pmr::unsynchronized_pool_resource pool;
    std::pmr::memory_resource* scratch = &pool;

    // output folder
    std::filesystem::path typescriptFolder = "../../../cocos-engine";
    std::filesystem::path cppFolder = "../../../cocos-engine/native";

    ModuleBuilder builder("cc", cppFolder, typescriptFolder, scratch, scratch);
    builder.init();

    // type registration
    {
        // register c++/stl types
        addCppDefaultValues(builder);

        // relate c++ to ts
        projectTypescriptDefaultValues(builder);

        // build cocos common
        buildCocosCommon(builder,
            Fwd | Types | Names | Typescripts);

        // build render pipeline
        buildRenderCommon(builder,
            Fwd | Types | Names | Typescripts | Jsb | Serialization);
        buildLayoutGraph(builder,
            Fwd | Types | Names | Graphs | Typescripts | Serialization);
        buildRenderGraph(builder,
            Fwd | Types | Graphs | Typescripts);
        buildFGDispatcher(builder, Types | Graphs);

        // build render compiler (native)
        buildArchiveInterface(builder, Fwd | Types | Typescripts | Interface);
        buildRenderInterface(builder, Fwd | Types | Typescripts | Interface | ToJs);
        buildPrivateInterface(builder, Fwd | Types | Typescripts | Interface);

        // web
        buildPrivateTypes(builder, Fwd | Types | Typescripts);

        // native
        if (false) {
            buildRenderCompiler(builder, Fwd | Types | Names | Graphs);
        }
        buildNativePipeline(builder, Fwd | Types | Graphs);

        // build render example
        if (false) {
            buildExecutorExample(builder, Typescripts | Jsb);
            buildRenderExample(builder,
                Fwd | Types | Names | Graphs);
        }
    }

    builder.compile();

    std::pmr::set<std::pmr::string> files(scratch);
    // output files
    {
        // manually added
        files.emplace("cocos/renderer/pipeline/custom/NativeFactory.cpp");
        files.emplace("cocos/renderer/pipeline/custom/NativePipeline.cpp");
        files.emplace("cocos/renderer/pipeline/custom/NativeExecutor.cpp");
        files.emplace("cocos/renderer/pipeline/custom/NativeRenderGraph.cpp");
        files.emplace("cocos/renderer/pipeline/custom/NativeRenderQueue.cpp");
        files.emplace("cocos/renderer/pipeline/custom/NativeProgramLibrary.cpp");
        files.emplace("cocos/renderer/pipeline/custom/NativeRenderingModule.cpp");
        files.emplace("cocos/renderer/pipeline/custom/LayoutGraphUtils.cpp");
        files.emplace("cocos/renderer/pipeline/custom/LayoutGraphUtils.h");
        files.emplace("cocos/renderer/pipeline/custom/RenderingModule.h");
        files.emplace("cocos/renderer/pipeline/custom/NativePools.cpp");
        files.emplace("cocos/renderer/pipeline/custom/NativeUtils.cpp");
        files.emplace("cocos/renderer/pipeline/custom/NativeUtils.h");

        // common types, shared by different modules
        builder.outputModule("RenderCommon", files);

        // descriptor layout graph
        builder.outputModule("LayoutGraph", files);

        // render graph
        builder.outputModule("RenderGraph", files);
        builder.outputModule("ArchiveInterface", files);
        builder.outputModule("RenderInterface", files);
        builder.outputModule("PrivateInterface", files);

        builder.outputModule("FGDispatcher", files);

        // private
        builder.outputModule("PrivateTypes", files);

        // native
        if (false) {
            builder.outputModule("RenderCompiler", files);
        }
        builder.outputModule("NativePipeline", files);

        // render example
        if (false) {
            builder.outputModule("ExecutorExample", files);
            builder.outputModule("RenderExample", files);
        }
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

    // clang-tidy
    if (true) {
        for (const auto& file : files) {
            std::cout << "clang-tidy '" << file << "'" << std::endl;
        }
    }

    return 0;
}
