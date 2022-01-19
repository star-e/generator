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

#pragma once
#include "BuilderTypes.h"

namespace Cocos::Meta {

std::pmr::string generateFwd_h(const SyntaxGraph& g, std::string_view moduleName,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch);

std::pmr::string generateNames_h(const SyntaxGraph& g,
    const ModuleGraph& mg,
    std::string_view moduleName,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch);

std::pmr::string generateTypes_h(std::string_view projectName,
    const SyntaxGraph& g,
    const ModuleGraph& mg,
    std::string_view moduleName,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch);

std::pmr::string generateTypes_cpp(std::string_view projectName,
    const SyntaxGraph& g,
    const ModuleGraph& mg,
    std::string_view moduleName0,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch);

std::pmr::string generateGraphs_h(std::string_view projectName,
    const SyntaxGraph& g,
    const ModuleGraph& mg,
    std::string_view moduleName,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch);

std::pmr::string generateReflection_h(std::string_view projectName,
    const SyntaxGraph& g,
    const ModuleGraph& mg,
    std::string_view moduleName,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch);

std::pmr::string generateReflection_cpp(std::string_view projectName,
    const SyntaxGraph& g,
    const ModuleGraph& mg,
    std::string_view moduleName,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch);

}
