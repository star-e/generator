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
#include "SyntaxTypes.h"
#include "BuilderTypes.h"

namespace Cocos::Meta {

void outputTypescript(std::ostream& oss, std::pmr::string& space,
    CodegenContext& codegen,
    const ModuleBuilder& builder, std::string_view scope,
    SyntaxGraph::vertex_descriptor vertID,
    std::pmr::set<std::pmr::string>& imports,
    std::pmr::memory_resource* scratch);

void outputDisassembleMembers(std::ostream& oss, std::pmr::string& space,
    const ModuleBuilder& builder,
    const SyntaxGraph& g,
    const SyntaxGraph::vertex_descriptor vertID,
    const std::pmr::vector<Member>& members,
    std::pmr::memory_resource* scratch);

void outputMembers(std::ostream& oss, std::pmr::string& space,
    const ModuleBuilder& builder,
    const SyntaxGraph& g,
    const SyntaxGraph::vertex_descriptor vertID,
    const std::pmr::vector<std::pmr::string>& inherits,
    const std::pmr::vector<Member>& members,
    const std::pmr::vector<std::pmr::string>& functions,
    const std::pmr::vector<Constructor>& cntrs,
    const std::pmr::vector<Method>& methods,
    std::pmr::memory_resource* scratch);

void outputFunctions(std::ostream& oss, std::pmr::string& space,
    const SyntaxGraph& g,
    const SyntaxGraph::vertex_descriptor vertID,
    std::pmr::memory_resource* scratch);

std::pmr::string generateSerialization_ts(
    std::string_view projectName, const SyntaxGraph& g,
    const ModuleGraph& mg,
    std::string_view moduleName0,
    bool nvp,
    std::pmr::memory_resource* mr,
    std::pmr::memory_resource* scratch);

}
