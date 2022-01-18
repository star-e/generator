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

#include "TypescriptBuilder.h"
#include "BuilderMacros.h"
#include "SyntaxUtils.h"
#include "SyntaxGraphs.h"
#include "TypescriptGraph.h"

namespace Cocos::Meta {

void outputTypescript(std::ostream& oss, std::pmr::string& space,
    CodegenContext& codegen,
    const ModuleBuilder& builder, std::string_view scope,
    SyntaxGraph::vertex_descriptor vertID,
    std::pmr::memory_resource* scratch) {
    const auto& g = builder.mSyntaxGraph;
    auto name = g.getTypescriptTypename(vertID, scratch, scratch);
    const auto& traits = get(g.traits, g, vertID);
    if (traits.mImport)
        return;

    auto& currScope = codegen.mScopes.back();

    visit_vertex(
        vertID, g,
        [&](const Enum& e) {
            if (currScope.mCount++)
                oss << "\n";
            OSS << "export const enum " << name << " {\n";
            {
                INDENT();
                for (const auto& v : e.mValues) {
                    OSS << v.mName;
                    if (!v.mValue.empty()) {
                        oss << " = " << v.mValue;
                    }
                    oss << ",\n";
                }
            }
            OSS << "}\n";
        },
        [&](const Graph& s) {
            if (currScope.mCount++)
                oss << "\n";

            auto content = generateGraph(builder, s, name, scratch);
            copyString(oss, space, content);
        }, 
        [&](const Struct& s) {
            if (currScope.mCount++)
                oss << "\n";
            OSS << "export class " << name;
            const auto& constraints = get(g.constraints, g, vertID);
            for (int count = 0; const auto& conceptPath : constraints.mConcepts) {
                auto superID = locate(conceptPath, g);
                const auto& name = get(g.names, g, superID);
                if (count++ == 0) {
                    oss << " extends ";
                } else {
                    oss << ", ";
                }
                oss << name;
            }
            oss << " {\n";
            {
                INDENT();
                outputMembers(oss, space, builder, g,
                    constraints.mConcepts, s.mMembers,
                    s.mTypescriptFunctions, s.mConstructors, scratch);
            }
            OSS << "}\n";
        },
        [&](const Variant& s) {
            if (currScope.mCount++)
                oss << "\n";
            if (g.isTag(vertID)) {
                OSS << "export const enum " << name << " {\n";
                {
                    INDENT();
                    for (const auto& type : s.mVariants) {
                        OSS << g.getTypescriptTagName(type, scratch, scratch) << ",\n";
                    }
                }
                OSS << "}\n";
            } else {
                // not supported yet
                Expects(false);
            }
        },
        [&](const auto&) {
        });
}

}
