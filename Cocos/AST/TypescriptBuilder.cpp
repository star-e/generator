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
#include "Cocos/AST/CodeConfigs.h"

namespace Cocos::Meta {

std::pmr::string outputImports_ts(
    const ModuleBuilder& builder,
    const ModuleInfo& moduleInfo,
    Features features,
    bool importType,
    const std::string_view tsModule,
    const PmrSet<std::pmr::string>& imported,
    std::pmr::set<std::pmr::string>& moduleImports,
    std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);
    const auto& typescriptFolder = builder.mTypescriptFolder;
    std::filesystem::path tsPath = typescriptFolder / moduleInfo.mTypescriptFolder / moduleInfo.mTypescriptFilePrefix;

    const auto& mg = builder.mModuleGraph;
    const auto& g = builder.mSyntaxGraph;
    const auto targetID = locate(tsModule, mg);
    const auto targetPath = get_path(targetID, mg, scratch);
    const auto targetName = get(mg.names, mg, targetID);
    const auto& target = get(mg.modules, mg, targetID);
    moduleImports.emplace(targetPath);
    if (importType) {
        OSS << "import type { ";
    } else {
        OSS << "import { ";
    }
    int count = 0;
    for (const auto& type : imported) {
        auto vertID = locate(type, g);
        auto tsName = g.getTypescriptTypename(type, scratch, scratch);

        if (count++)
            oss << ", ";
        oss << tsName;

        if ((features & Features::Serialization) && (target.mFeatures & Features::Serialization)) {
            if (holds_tag<Struct_>(vertID, g)) {
                if (count++) {
                    oss << ", ";
                }
                oss << "save" << tsName;
                oss << ", load" << tsName;
            }
        }
        const auto& traits = get(g.traits, g, vertID);
        if (false && (traits.mFlags & EQUAL) && !(traits.mFlags & NO_EQUAL)) {
            if (count++) {
                oss << ", ";
            }
            oss << "equal" << tsName;
        }
    }

    if (!importType) {
        if ((features & TsPool) && (target.mFeatures & TsPool)) {
            oss << ", " << targetName << "ObjectPool";
        }
    }

    oss << " } from '";

    std::filesystem::path tsPath1 = typescriptFolder / target.mTypescriptFolder / target.mTypescriptFilePrefix;
    oss << getRelativePath(tsPath.generic_string(), tsPath1.generic_string(), scratch);
    oss << "';\n";

    if (count == 0) {
        return std::pmr::string{ scratch };
    }

    return oss.str();
}

void outputTypescript(std::ostream& oss, std::pmr::string& space,
    CodegenContext& codegen,
    const ModuleBuilder& builder,
    const ModuleInfo& moduleInfo,
    std::string_view scope,
    SyntaxGraph::vertex_descriptor vertID,
    std::pmr::set<std::pmr::string>& imports,
    bool bPublicFormat,
    std::pmr::memory_resource* scratch) {
    const auto funcSpace = bPublicFormat ? "" : " ";
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
            OSS << "export ";
            if (!(traits.mFlags & TS_ENUM_OBJECT)) {
                oss << "const ";
            }
            oss << "enum " << name << " {\n";
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
        },
        [&](const Graph& s) {
            if (currScope.mCount++)
                oss << "\n";
            if (!comment.mComment.empty()) {
                outputComment(oss, space, comment.mComment);
            }
            auto content = generateGraph(builder, moduleInfo, s, vertID, name, imports,
                bPublicFormat, scratch);
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
            if (traits.mInterface || (traits.mStructInterface && sEnableMake)) {
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
                    OSS << "static create" << funcSpace << "(): " << name << " {\n";
                    {
                        INDENT();
                        OSS << "if (" << name << "._pool.length) {\n";
                        OSS << "    return " << name << "._pool.pop()!;\n";
                        OSS << "}\n";
                        OSS << "return new " << name << "();\n";
                    }
                    OSS << "}\n";
                    OSS << "disassemble" << funcSpace << "(): void {\n";
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
                    s.mTypescriptFunctions, s.mConstructors, s.mMethods,
                    bPublicFormat, scratch);
            }
            OSS << "}\n";

            if (traits.mStructInterface) {
                if (sEnableMake) {
                    oss << "\n";
                    OSS << "export function make" << name << funcSpace << "(): " << name << " {\n";
                    {
                        INDENT();
                        OSS << "return {\n";
                        for (const auto& m : s.mMembers) {
                            INDENT();
                            const auto memberID = locate(m.mTypePath, g);
                            const auto& memberType = get(g.names, g, memberID);
                            const auto& memberTraits = get(g.traits, g, memberID);
                            const auto& memberName = g.getMemberName(m.mMemberName, true);

                            if (m.mOptional || g.isOptional(memberID)) {
                                // noop
                            } else if (memberTraits.mStructInterface && sEnableMake) {
                                OSS << memberName << ": make" << memberType << "(),\n";
                            } else {
                                if (g.isTypescriptValueType(memberID)) {
                                    OSS << memberName << ": "
                                        << g.getTypescriptInitialValue(memberID, m, scratch, scratch) << ",\n";
                                } else if (m.mNullable) {
                                    OSS << memberName << ": null,\n";
                                } else {
                                    Expects(false);
                                }
                            }
                        }
                        OSS << "};\n";
                    }
                    OSS << "}\n";
                }

                oss << "\n";
                OSS << "export function fillRequired" << name << funcSpace << "(value: " << name << "): void {\n";
                {
                    INDENT();
                    for (const auto& m : s.mMembers) {
                        const auto memberID = locate(m.mTypePath, g);
                        const auto& memberType = get(g.names, g, memberID);
                        const auto& memberTraits = get(g.traits, g, memberID);
                        const auto& memberName = g.getMemberName(m.mMemberName, true);

                        if (m.mOptional || g.isOptional(memberID)) {
                            // noop
                        } else if (memberTraits.mStructInterface) {
                            if (sEnableMake) {
                                OSS << "if (!value." << memberName << ") {\n";
                                OSS << "    (value." << memberName << " as " << memberType << ") = make" << memberType << "();\n";
                                OSS << "} else {\n";
                                OSS << "    fillRequired" << memberType << "(value." << memberName << ");\n";
                                OSS << "}\n";
                            } else {
                                const auto typescriptFullName = memberType + (m.mNullable ? " | null" : "");
                                OSS << "if (!value." << memberName << ") {\n";
                                OSS << "    (value." << memberName << " as " << memberType << ") = "
                                    << g.getTypescriptInitialValue(memberID, m, scratch, scratch) << ";\n";
                                OSS << "} else {\n";
                                OSS << "    fillRequired" << memberType << "(value." << memberName << ");\n";
                                OSS << "}\n";
                            }
                        } else {
                            if (sEnableOptionalAssign) {
                                OSS << "value." << memberName << " ??= "
                                    << g.getTypescriptInitialValue(memberID, m, scratch, scratch) << ";\n";
                            } else {
                                OSS << "if (value." << memberName << " === undefined) {\n";
                                OSS << "    value." << memberName << " = "
                                    << g.getTypescriptInitialValue(memberID, m, scratch, scratch) << ";\n";
                                OSS << "}\n";
                            }
                        }
                    }
                }
                OSS << "}\n";
            }

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
                    for (const auto& param : s.mVariants) {
                        OSS << g.getTypescriptTagName(param.mTypePath, scratch, scratch) << ",\n";
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

static const uint32_t sPoolBatchSize = 16;

void outputTypescriptPool(std::ostream& oss, std::pmr::string& space,
    CodegenContext& codegen,
    const ModuleBuilder& builder,
    std::string_view typeModulePath,
    const ModuleInfo& moduleInfo,
    std::string_view scope,
    std::pmr::set<std::pmr::string>& moduleImports,
    bool bPublicFormat,
    std::pmr::memory_resource* scratch) {
    const auto& g = builder.mSyntaxGraph;
    const auto& mg = builder.mModuleGraph;

    const auto moduleID = locate(typeModulePath, mg);
    Ensures(moduleID != ModuleGraph::null_vertex());

    if (sEnablePoolSettings) {
        oss << "\n";
        OSS << "export class " << typeModulePath.substr(1) << "ObjectPoolSettings {\n";
        {
            INDENT();
            OSS << "constructor (batchSize: number) {\n";
            {
                INDENT();
                for (const auto& vertID : make_range(vertices(g))) {
                    if (!g.isPoolType(vertID, typeModulePath)) {
                        continue;
                    }
                    auto name = g.getTypescriptTypename(vertID, scratch, scratch);
                    OSS << "this." << camelToVariable(name, scratch) << "BatchSize = batchSize;\n";
                }
            }
            OSS << "}\n";
            for (const auto& vertID : make_range(vertices(g))) {
                if (!g.isPoolType(vertID, typeModulePath)) {
                    continue;
                }
                auto name = g.getTypescriptTypename(vertID, scratch, scratch);
                OSS << camelToVariable(name, scratch) << "BatchSize = " << sPoolBatchSize << ";\n";
            }
        }
        OSS << "}\n";
    }

    if (sUseCreatePool) {
        oss << "\n";
        OSS << "function createPool<T> (Constructor: new() => T): RecyclePool<T> {\n";
        OSS << "    return new RecyclePool<T>(() => new Constructor(), 16);\n";
        OSS << "}\n";
    }

    oss << "\n";
    OSS << "export class " << typeModulePath.substr(1) << "ObjectPool {\n";
    {
        INDENT();
        bool hasPools = false;
        for (const auto& importedPath : moduleImports) {
            const auto importedID = locate(importedPath, mg);
            const auto& info = get(mg.modules, mg, importedID);
            const auto& name = get(mg.names, mg, importedID);
            if (info.mFeatures & TsPool) {
                hasPools = true;
                break;
            }
        }
        {

            OSS << "constructor (";
            int count = 0;
            if (sEnablePoolSettings) {
                oss << "settings: " << typeModulePath.substr(1) << "ObjectPoolSettings";
                ++count;
            }
            for (const auto& importedPath : moduleImports) {
                const auto importedID = locate(importedPath, mg);
                const auto& info = get(mg.modules, mg, importedID);
                const auto& name = get(mg.names, mg, importedID);
                if (info.mFeatures & TsPool) {
                    if (count++) {
                        oss << ", ";
                    }
                    oss << camelToVariable(name, scratch) << ": " << name << "ObjectPool";
                }
            }
            oss << ") {\n";
            {
                INDENT();
                for (const auto& importedPath : moduleImports) {
                    const auto importedID = locate(importedPath, mg);
                    const auto& info = get(mg.modules, mg, importedID);
                    const auto& name = get(mg.names, mg, importedID);
                    if (info.mFeatures & TsPool) {
                        OSS << "this." << camelToVariable(name, scratch) << " = "
                            << camelToVariable(name, scratch) << ";\n";
                    }
                }
                if (sEnablePoolSettings) {
                    for (const auto& vertID : make_range(vertices(g))) {
                        if (!g.isPoolType(vertID, typeModulePath)) {
                            continue;
                        }
                        auto name = g.getTypescriptTypename(vertID, scratch, scratch);
                        OSS << "this._" << camelToVariable(name, scratch)
                            << " = new RecyclePool<" << name << ">(() => new " << name << "(), settings."
                            << camelToVariable(name, scratch) << "BatchSize);\n";
                    }
                }
            }
            OSS << "}\n";
        }
        {
            OSS << gNameReset << " (): void {\n";
            {
                INDENT();
                for (const auto& vertID : make_range(vertices(g))) {
                    if (!g.isPoolType(vertID, typeModulePath)) {
                        continue;
                    }
                    auto name = g.getTypescriptTypename(vertID, scratch, scratch);
                    OSS << "this._" << camelToVariable(name, scratch) << ".reset();\n";
                }
            }
            OSS << "}\n";
        }
        if (true) { // create
            for (const auto& vertID : make_range(vertices(g))) {
                if (!g.isPoolType(vertID, typeModulePath)) {
                    continue;
                }
                const auto& traits = get(g.traits, g, vertID);
                auto name = g.getTypescriptTypename(vertID, scratch, scratch);

                // get cntr
                const Constructor* pCntr = nullptr;
                const Struct* pStruct = nullptr;
                if (holds_tag<Struct_>(vertID, g)) {
                    const auto& s = get_by_tag<Struct_>(vertID, g);
                    pStruct = &s;
                    if (!s.mConstructors.empty()) {
                        pCntr = &s.mConstructors.front();
                    }
                }

                // format
                {
                    int count = 0;
                    OSS << "create" << name << " (";
                    if (pCntr) {
                        outputConstructionParams(oss, space, count, builder, true,
                            g, pStruct->mMembers, *pCntr, true, false, bPublicFormat, true, scratch);
                    }
                    if (kOutputPoolDebug) {
                        if (count) {
                            INDENT();
                            OSS << "isDebug = true,\n";
                        } else {
                            oss << "isDebug = true";
                        }
                    }
                    if (count) {
                        OSS;
                    }
                    oss << "): " << name << " {\n";
                }
                {
                    INDENT();
                    if (kOutputPoolDebug) {
                        OSS << "let v: " << name << ";\n";
                        OSS << "if (isDebug) {\n";
                        OSS << "    v = new " << name << "();\n";
                        OSS << "} else {\n";
                        OSS << "    v = this._" << camelToVariable(name, scratch) << ".add();\n";
                        OSS << "    v._pool = true;\n";
                        OSS << "}\n";
                    } else {
                        OSS << "const v = this._" << camelToVariable(name, scratch) << ".add();\n";
                    }

                    if (pStruct) {
                        if (traits.mFlags & SKIP_RESET) {
                            for (uint32_t i = 0; const auto& m : pStruct->mMembers) {
                                const auto& member = g.getMemberName(m.mMemberName, true);
                                OSS << "v." << member << " = " << member << ";\n";
                            }
                        } else if (pCntr){
                            int count = 0;
                            OSS << "v." << gNameReset << "(";
                            outputConstructionParams(oss, space, count, builder, false,
                                g, pStruct->mMembers, *pCntr, true, true, bPublicFormat, false, scratch);
                            oss << ");\n";
                        } else {
                            OSS << "v." << gNameReset << "();\n"; 
                        }
                    } else {
                        OSS << "v.clear();\n"; 
                    }
                    OSS << "return v;\n";
                }
                OSS << "}\n";
            }
        }

        for (const auto& importedPath : moduleImports) {
            const auto importedID = locate(importedPath, mg);
            const auto& info = get(mg.modules, mg, importedID);
            const auto& name = get(mg.names, mg, importedID);
            if (info.mFeatures & TsPool) {
                OSS << "public readonly " << camelToVariable(name, scratch) << ": " << name << "ObjectPool;\n";
            }
        }

        for (const auto& vertID : make_range(vertices(g))) {
            if (!g.isPoolType(vertID, typeModulePath)) {
                continue;
            }
            auto name = g.getTypescriptTypename(vertID, scratch, scratch);
            OSS << "private readonly _" << camelToVariable(name, scratch)
                << ": RecyclePool<" << name << ">";
            if (sEnablePoolSettings) {
                oss << ";\n";
            } else {
                if (sUseCreatePool) {
                    oss << " = createPool(" << name << ");\n";
                } else {
                    oss << " = new RecyclePool<" << name
                        << ">(() => new " << name << "(), " << sPoolBatchSize << ");\n";
                }
            }
        }
        if (kOutputPoolDebug) {
            OSS << "public debug = false;\n";
        }
    }

    OSS << "}\n";
}

}
