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

#include "TypescriptBuilder.h"
#include "BuilderMacros.h"
#include "SyntaxUtils.h"
#include "SyntaxGraphs.h"
#include "TypescriptGraph.h"
#include "BuilderUtils.h"

namespace Cocos::Meta {

void outputTypescript(std::ostream& oss, std::pmr::string& space,
    CodegenContext& codegen,
    const ModuleBuilder& builder,
    const ModuleInfo& moduleInfo,
    std::string_view scope,
    SyntaxGraph::vertex_descriptor vertID,
    std::pmr::set<std::pmr::string>& imports,
    std::pmr::memory_resource* scratch) {
    const auto& g = builder.mSyntaxGraph;
    auto name = g.getTypescriptTypename(vertID, scratch, scratch);
    const auto& traits = get(g.traits, g, vertID);
    const auto& comment = get(g.comments, g, vertID);
    if (traits.mImport)
        return;

    if (traits.mFlags & IMPL_DETAIL)
        return;

    if (!traits.mExport) {
        return;
    }

    if (g.isTypescriptData(name))
        return;

    auto& currScope = codegen.mScopes.back();

    visit_vertex(
        vertID, g,
        [&](const Enum& e) {
            if (currScope.mCount++)
                oss << "\n";
            if (!comment.mComment.empty()) {
                outputComment(oss, space, comment.mComment);
            }
            OSS << "export enum " << name << " {\n";
            {
                INDENT();
                for (const auto& v : e.mValues) {
                    if (!v.mExport) {
                        continue;
                    }
                    if (!v.mComment.empty()) {
                        outputEnumComment(oss, space, g, vertID, v);
                    }
                    OSS << v.mName;
                    if (!v.mValue.empty()) {
                        oss << " = " << v.mValue;
                    }
                    oss << ",\n";
                }
            }
            OSS << "}\n";

            if (!e.mIsFlags) {
                oss << "\n";
                OSS << "export function get" << name << "Name (e: " << name << "): string {\n";
                {
                    INDENT();
                    OSS << "switch (e) {\n";
                    for (const auto& v : e.mValues) {
                        if (v.mAlias) {
                            continue;
                        }
                        OSS << "case " << name << "." << v.mName << ":\n";
                        INDENT();
                        OSS << "return '" << v.mName << "';\n";
                    }
                    OSS << "default:\n";
                    OSS << "    return '';\n";
                    OSS << "}\n";
                }
                OSS << "}\n";
            }
        },
        [&](const Graph& s) {
            if (currScope.mCount++)
                oss << "\n";
            if (!comment.mComment.empty()) {
                outputComment(oss, space, comment.mComment);
            }
            auto content = generateGraph(builder, moduleInfo, s, vertID, name, imports, scratch);
            copyString(oss, space, content);
        }, 
        [&](const Struct& s) {
            if (currScope.mCount++)
                oss << "\n";

            if (!comment.mComment.empty()) {
                outputComment(oss, space, comment.mComment);
            }

            if (traits.mFlags & DECORATOR) {
                OSS << "@ccclass('cc." << name << "')\n";
            }
            OSS << "export";
            if (traits.mInterface) {
                oss << " interface " << name;
            } else {
                oss << " class " << name;
            }
            const auto& inherits = get(g.inherits, g, vertID);
            for (int count = 0; const auto& base : inherits.mBases) {
                auto superID = locate(base.mTypePath, g);
                const auto& traits = get(g.traits, g, superID);
                if (traits.mFlags & IMPL_DETAIL) {
                    continue;
                }
                const auto& name = get(g.names, g, superID);
                if (count++ == 0) {
                    if (traits.mInterface) {
                        oss << " extends ";
                    } else {
                        if (base.mVirtualBase) {
                            oss << " implements ";
                        } else {
                            oss << " extends ";
                        }
                    }
                } else {
                    oss << ", ";
                }
                oss << name;
            }
            oss << " {\n";
            {
                INDENT();
                if (false && (traits.mFlags & POOL_OBJECT)) {
                    OSS << "private static _pool: " << name << "[] = [];\n";
                    OSS << "static create (): " << name << " {\n";
                    {
                        INDENT();
                        OSS << "if (" << name << "._pool.length) {\n";
                        OSS << "    return " << name << "._pool.pop()!;\n";
                        OSS << "}\n";
                        OSS << "return new " << name << "();\n";
                    }
                    OSS << "}\n";
                    OSS << "disassemble (): void {\n";
                    {
                        INDENT();
                        outputDisassembleMembers(oss, space, builder, g, vertID, s.mMembers, scratch);
                        OSS << name << "._pool.push(this);\n";
                    }
                    OSS << "}\n";
                }
                std::pmr::vector<std::pmr::string> bases(scratch);
                bases.reserve(inherits.mBases.size());
                for (const auto& base : inherits.mBases) {
                    bases.emplace_back(base.mTypePath);
                }
                outputMembers(oss, space, builder, moduleInfo, g, vertID,
                    bases, s.mMembers,
                    s.mTypescriptFunctions, s.mConstructors, s.mMethods, scratch);
            }
            OSS << "}\n";
            
            if (false) {
                outputFunctions(oss, space, g, vertID, scratch);
            }
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
