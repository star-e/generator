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
#include "BuilderTypes.h"
#include "SyntaxUtils.h"
#include "SyntaxGraphs.h"
#include "BuilderUtils.h"

namespace Cocos::Meta {

void outputContainer(std::ostream& oss, std::pmr::string& space,
    SyntaxGraph::vertex_descriptor paramID,
    const SyntaxGraph& g,
    int level) {
    if (g.isTypescriptMap(paramID)) {
        OSS << "for (const [k" << level << ", v" << level << " of ";
        if (level == 1) {
            oss << "v";
        } else {
            oss << "v" << level - 1;
        }
        oss << ") {\n";
        {
            INDENT();
            const auto& inst = get<Instance>(paramID, g);
            const auto& paramPath = inst.mParameters.at(1);
            const auto paramID = locate(paramPath, g);
            if (g.isInstantiation(paramID)) {
                outputContainer(oss, space, paramID, g, level + 1);
            } else {
                OSS << "v" << level << ".disassemble();\n";
            }
        }
        OSS << "}\n";
    } else if (g.isTypescriptArray(paramID, g.get_allocator().resource())) {
        OSS << "for (const v" << level << " of ";
        if (level == 1) {
            oss << "v";
        } else {
            oss << "v" << level - 1;
        }
        oss << ") {\n";
        {
            INDENT();
            const auto& inst = get<Instance>(paramID, g);
            const auto& paramPath = inst.mParameters.at(0);
            const auto paramID = locate(paramPath, g);
            if (g.isInstantiation(paramID)) {
                outputContainer(oss, space, paramID, g, level + 1);
            } else {
                OSS << "v" << level << ".disassemble();\n";
            }
        }
        OSS << "}\n";
    } else {
        Expects(false);
    }
}

void outputDisassembleMembers(std::ostream& oss, std::pmr::string& space,
    const ModuleBuilder& builder,
    const SyntaxGraph& g,
    const SyntaxGraph::vertex_descriptor vertID,
    const std::pmr::vector<Member>& members,
    std::pmr::memory_resource* scratch) {
    const auto& traits = get(g.traits, g, vertID);
    const int maxParams = 4;
    const auto& name = g.getTypescriptTypename(vertID, scratch, scratch);

    for (const auto& m : members) {
        if (m.mFlags & IMPL_DETAIL)
            continue;

        auto path = m.mTypePath;
        Expects(!path.empty());
        Expects(path.front() == '/');
        Expects(validateTypename(path));
        auto memberID = locate(path, g);
        Ensures(memberID != g.null_vertex());

        auto memberName = "this." + g.getMemberName(m.mMemberName, m.mPublic);

        if (g.isPoolObject(memberID)) {
            if (g.isInstantiation(memberID)) {
                const auto& inst = get<Instance>(memberID, g);
                if (g.isTypescriptMap(memberID)) {
                    OSS << "for (const [_, v] of " << memberName << ") {\n";
                    {
                        INDENT();
                        const auto& paramPath = inst.mParameters.at(1);
                        const auto paramID = locate(paramPath, g);
                        if (g.isInstantiation(paramID)) {
                            outputContainer(oss, space, paramID, g, 1);
                        } else {
                            OSS << "v.disassemble();\n";
                        }
                    }
                    OSS << "}\n";
                    OSS << memberName << ".clear();\n";
                } else if (g.isTypescriptArray(memberID, scratch)) {
                    OSS << "for (const v of " << memberName << ") {\n";
                    {
                        INDENT();
                        const auto& paramPath = inst.mParameters.at(0);
                        const auto paramID = locate(paramPath, g);
                        if (g.isInstantiation(paramID)) {
                            outputContainer(oss, space, paramID, g, 1);
                        } else {
                            OSS << "v.disassemble();\n";
                        }
                    }
                    OSS << "}\n";
                    OSS << memberName << ".length = 0;\n";
                } else {
                    OSS << "// disassemble container\n";
                }
            } else {
                OSS << "// " << memberName << " is value object, do not disassemble\n";
            }
        } else {
            if (g.isTypescriptValueType(memberID)) {
                OSS << memberName;
                oss << " = " << g.getTypescriptInitialValue(memberID, m, scratch, scratch) << ";\n"; 
            } else {
                if (m.mNullable) {
                    OSS << memberName << " = null;\n";
                } else {
                    OSS << "// " << memberName
                        << " is object, should initialize\n";
                }
            }
        }
    }
}

bool typescriptMemberNeedAssign(const SyntaxGraph& g, const Member& m, uint32_t memberID) {
    return g.isTypescriptValueType(memberID)
        || m.mNullable
        || g.isTypescriptPointer(memberID);
}

void outputConstructionParams(
    std::ostream& oss, std::pmr::string& space, int& count,
    const ModuleBuilder& builder,
    const bool bChangeLine,
    const SyntaxGraph& g,
    const std::pmr::vector<Member>& members,
    const Constructor& cntr,
    bool bReset,
    bool bArgument,
    bool bPublicFormat,
    std::pmr::memory_resource* scratch) {
    auto outputComma = [&]() {
        if (bChangeLine) {
            if (count++ == 0)
                oss << "\n";
            INDENT();
            OSS;
        } else {
            if (count++) {
                oss << ", ";
            }
        }
    };
    for (const auto& id : cntr.mIndices) {
        for (uint32_t i = 0; const auto& m : members) {
            if (i == id) {
                auto memberID = locate(m.mTypePath, g);

                bool bSkip = bReset
                    && (!typescriptMemberNeedAssign(g, m, memberID)
                        || g.isTypescriptTypedArray(memberID)
                        || g.isTypescriptArray(memberID, scratch)
                        || g.isTypescriptMap(memberID)
                        || g.isTypescriptSet(memberID));

                if (bSkip) {
                    ++i;
                    continue;
                }
                outputComma();

                auto memberType = g.getTypescriptTypename(memberID, scratch, scratch);
                if (bArgument) {
                    oss << g.getMemberName(m.mMemberName, true);
                } else {
                    if (cntr.mHasDefault) {
                        oss << builder.getTypedMemberName(bPublicFormat, m, true);
                        oss << " = " << g.getTypescriptInitialValue(memberID, m, scratch, scratch);
                    } else {
                        oss << builder.getTypedMemberName(bPublicFormat, m, true, true);
                    }
                }
                if (bChangeLine) {
                    oss << ",\n";
                }
            }
            ++i;
        }
    }
}

void outputMembers(std::ostream& oss, std::pmr::string& space,
    const ModuleBuilder& builder,
    const ModuleInfo& moduleInfo,
    const SyntaxGraph& g,
    const SyntaxGraph::vertex_descriptor vertID,
    const std::pmr::vector<std::pmr::string>& inherits,
    const std::pmr::vector<Member>& members,
    const std::pmr::vector<std::pmr::string>& functions,
    const std::pmr::vector<Constructor>& cntrs,
    const std::pmr::vector<Method>& methods,
    bool bPublicFormat,
    std::pmr::memory_resource* scratch) {
    const auto& traits = get(g.traits, g, vertID);
    const int maxParams = 4;
    const auto& name = g.getTypescriptTypename(vertID, scratch, scratch);
    {
        int count = 0;
        bool bChangeLine = false;

        if (!cntrs.empty()) {
            auto& cntr = cntrs.front();
            bChangeLine = false;
            if (cntr.mIndices.size() > maxParams) {
                bChangeLine = true;
            }
            count = 0;
            { // constructor
                OSS;
                // if (traits.mFlags & POOL_OBJECT) {
                //     oss << "private: ";
                // }
                oss << "constructor (";
                if (!inherits.empty()) {
                    Expects(inherits.size() == 1);
                    const auto& base = inherits.front();
                    auto baseID = locate(base, g);
                    visit_vertex(
                        baseID, g,
                        [&](const Composition_ auto& s) {
                            if (s.mConstructors.empty()) {
                                return;
                            }
                            outputConstructionParams(oss, space, count, builder, bChangeLine,
                                g, s.mMembers, s.mConstructors.front(), false, false, bPublicFormat, scratch);
                        },
                        [&](const auto&) {
                        });
                }
                outputConstructionParams(oss, space, count, builder, bChangeLine,
                    g, members, cntr, false, false, bPublicFormat, scratch);

                if (bChangeLine) {
                    OSS << ") {\n";
                } else {
                    oss << ") {\n";
                }
                {
                    INDENT();
                    if (!inherits.empty()) {
                        const auto& base = inherits.front();
                        auto baseID = locate(base, g);
                        visit_vertex(
                            baseID, g,
                            [&](const Composition_ auto& s) {
                                if (s.mConstructors.empty()) {
                                    return;
                                }
                                const auto& cntr = s.mConstructors.front();
                                const auto& members = s.mMembers;
                                OSS << "super(";
                                for (int count = 0; const auto& id : cntr.mIndices) {
                                    for (uint32_t i = 0; const auto& m : members) {
                                        if (i == id) {
                                            auto memberID = locate(m.mTypePath, g);
                                            if (count++)
                                                oss << ", ";
                                            oss << g.getMemberName(m.mMemberName, true);
                                        }
                                    }
                                }
                                oss << ");\n";
                            },
                            [&](const auto&) {
                            });
                    }

                    for (uint32_t i = 0; const auto& m : members) {
                        for (const auto& id : cntr.mIndices) {
                            if (i == id) {
                                auto memberID = locate(m.mTypePath, g);
                                OSS << "this." << g.getMemberName(m.mMemberName, m.mPublic)
                                    << " = " << g.getMemberName(m.mMemberName, true) << ";\n";
                            }
                        }
                        ++i;
                    }
                }
                OSS << "}\n";
            }
        }
        if ((moduleInfo.mFeatures & TsPool)
            && !g.isInterface(vertID)
            && !holds_tag<Graph_>(vertID, g)
            && !(traits.mFlags & SKIP_RESET)) { // reset
            const Constructor* pCntr = nullptr;
            if (!cntrs.empty()) {
                pCntr = &cntrs.front();
            }
            count = 0;
            OSS;
            oss << "reset (";
            if (!inherits.empty()) {
                Expects(inherits.size() == 1);
                const auto& base = inherits.front();
                auto baseID = locate(base, g);
                visit_vertex(
                    baseID, g,
                    [&](const Composition_ auto& s) {
                        if (s.mConstructors.empty()) {
                            return;
                        }
                        outputConstructionParams(oss, space, count, builder, bChangeLine,
                            g, s.mMembers, s.mConstructors.front(), true, false, bPublicFormat, scratch);
                    },
                    [&](const auto&) {
                    });
            }
            if (pCntr) {
                outputConstructionParams(oss, space, count, builder, bChangeLine,
                    g, members, *pCntr, true, false, bPublicFormat, scratch);
            }
            if (bChangeLine) {
                OSS << "): void {\n";
            } else {
                oss << "): void {\n";
            }
            {
                INDENT();
                if (!inherits.empty()) {
                    const auto& base = inherits.front();
                    auto baseID = locate(base, g);
                    visit_vertex(
                        baseID, g,
                        [&](const Composition_ auto& s) {
                            if (s.mConstructors.empty()) {
                                return;
                            }
                            const auto& cntr = s.mConstructors.front();
                            const auto& members = s.mMembers;
                            Expects(false);
                            OSS << "super(";
                            for (int count = 0; const auto& id : cntr.mIndices) {
                                for (uint32_t i = 0; const auto& m : members) {
                                    if (i == id) {
                                        auto memberID = locate(m.mTypePath, g);
                                        if (count++)
                                            oss << ", ";
                                        oss << g.getMemberName(m.mMemberName, true);
                                    }
                                }
                            }
                            oss << ");\n";
                        },
                        [&](const auto&) {
                        });
                }

                for (uint32_t i = 0; const auto& m : members) {
                    bool bFound = false;
                    if (pCntr) {
                        for (const auto& id : pCntr->mIndices) {
                            if (i == id) {
                                bFound = true;
                                auto memberID = locate(m.mTypePath, g);
                                const auto& memberTraits = get(g.traits, g, memberID);
                                if (g.isTypescriptValueType(memberID) || m.mNullable || g.isTypescriptPointer(memberID)) {
                                    OSS << "this." << g.getMemberName(m.mMemberName, m.mPublic)
                                        << " = " << g.getMemberName(m.mMemberName, true) << ";\n";
                                } else if (g.isTypescriptTypedArray(memberID)) {
                                    OSS << "// " << g.getMemberName(m.mMemberName, m.mPublic)
                                        << ": " << g.getTypescriptTypename(memberID, scratch, scratch)
                                        << " size unchanged\n";
                                } else if (g.isTypescriptArray(memberID, scratch)) {
                                    OSS << "this." << g.getMemberName(m.mMemberName, m.mPublic) << ".length = 0;\n";
                                } else if (g.isTypescriptMap(memberID) || g.isTypescriptSet(memberID) || holds_tag<Graph_>(memberID, g)) {
                                    OSS << "this." << g.getMemberName(m.mMemberName, m.mPublic) << ".clear();\n";
                                } else {
                                    OSS << "this." << g.getMemberName(m.mMemberName, m.mPublic) << ".reset();\n";
                                }
                            }
                        }
                    }
                    if (!bFound) {
                        auto memberID = locate(m.mTypePath, g);
                        if (typescriptMemberNeedAssign(g, m, memberID)) {
                            OSS << "this." << g.getMemberName(m.mMemberName, m.mPublic)
                                << " = " << g.getTypescriptInitialValue(memberID, m, scratch, scratch) << ";\n";
                        } else if (g.isTypescriptTypedArray(memberID)) {
                            OSS << "// " << g.getMemberName(m.mMemberName, m.mPublic)
                                << ": " << g.getTypescriptTypename(memberID, scratch, scratch)
                                << " size unchanged\n";
                        } else if (g.isTypescriptArray(memberID, scratch)) {
                            OSS << "this." << g.getMemberName(m.mMemberName, m.mPublic) << ".length = 0;\n";
                        } else if (g.isTypescriptMap(memberID) || g.isTypescriptSet(memberID) || holds_tag<Graph_>(memberID, g)) {
                            OSS << "this." << g.getMemberName(m.mMemberName, m.mPublic) << ".clear();\n";
                        } else {
                            OSS << "this." << g.getMemberName(m.mMemberName, m.mPublic) << ".reset();\n";
                        }
                    }
                    ++i;
                }
            }
            OSS << "}\n";
        }
    }
    
    if (false && !methods.empty()) {
        OSS << "// API\n";
    }
    const auto maxParams2 = 2;
    for (uint32_t methodID = 0; const auto& method : methods) {
        if (method.mSkip || method.mSetter) {
            ++methodID;
            continue;
        }

        outputMethodComment(oss, space, g, vertID, method);
        OSS;
        if (!method.mPure) {
            oss << "public ";
        }
        if (method.mGetter) {
            auto methodName = method.mFunctionName.substr(3);
            methodName = camelToVariable(methodName, scratch);
            if (methodID + 1 < methods.size()) {
                const auto& nextMethod = methods[methodID + 1];
                if (nextMethod.mSetter) {
                    oss << methodName;
                } else {
                    oss << "readonly " << methodName;
                }
                if (method.mOptionalMethod) {
                    oss << "?";
                }
                oss << ": ";
                oss << g.getTypedParameterName(method.mReturnType, true, true, method.mOptional, true);
                oss << ";\n";
            } else {
                oss << "readonly " << methodName;
                if (method.mOptionalMethod) {
                    oss << "?";
                }
                oss << ": ";
                oss << g.getTypedParameterName(method.mReturnType, true, true, method.mOptional, true);
                oss << ";\n";
            }
        } else if (method.mSetter) {
            auto methodName = method.mFunctionName.substr(3);
            methodName = camelToVariable(methodName, scratch);
            oss << "set " << methodName << " (";
        } else {
            oss << method.mFunctionName;
            if (method.mOptionalMethod) {
                oss << "?";
            }
            oss << " (";
        }
        const bool bChangeLine = method.mParameters.size() > maxParams2
            || (method.mParameters.size() > (maxParams2 + 1) && method.mParameters.back().mName.back() == '_');
        for (int32_t count = 0; const auto& param : method.mParameters) {
            if (param.mName.back() == '_') {
                continue;
            }
            INDENT();
            if (bChangeLine) {
                if (count++) {
                    oss << ",";
                }
                oss << "\n";
                OSS;
            } else {
                if (count++) {
                    oss << ", ";
                }            
            }

            auto paramID = locate(param.mTypePath, g);
            oss << param.name();
            if (!param.mDefaultValue.empty()) {
                oss << "?";
            }
            oss << ": " << g.getTypedParameterName(param, true, true, param.mOptional);
        }
        if (!method.mGetter) {
            oss << ")";
            if (!method.mSetter) {
                oss << ": ";
                oss << g.getTypedParameterName(method.mReturnType, true, true, method.mOptional, true);

            } else {
                oss << ": void";
            }
            oss << ";";
            if (false && method.mConst) {
                oss << " /*const*/";
            }
            oss << "\n";
        }
        ++methodID;
    }

    if (!functions.empty()) {
        for (const auto& func : functions) {
            copyString(oss, space, func);
        }
    }

    const auto commentBegin = bPublicFormat ? "/* " : "/*";
    const auto commentEnd = bPublicFormat ? " */" : "*/";
    for (uint32_t i = 0; const auto& m : members) {
        if (m.mFlags & IMPL_DETAIL)
            continue;
        auto path = m.mTypePath;
        Expects(!path.empty());
        Expects(path.front() == '/');
        Expects(validateTypename(path));
        auto memberID = locate(path, g);

        bool bNeedIntial = true;
        if (!cntrs.empty()) {
            auto& cntr = cntrs.front();
            for (const auto& id : cntr.mIndices) {
                if (i == id) {
                    bNeedIntial = false;
                }
            }
        }

        OSS;
        if (!m.mPublic) {
            oss << "private ";
        }
        bool bTypscriptPointer = g.isTypescriptPointer(memberID);
        if (m.mMutable) {
            oss << commentBegin << "mutable" << commentEnd << " ";
        } else if (!m.mPointer && !bTypscriptPointer && !m.mOptional && !m.mNullable && (m.mReference || m.mConst || !g.isTypescriptValueType(memberID))) {
            oss << "readonly ";
        } else if (bTypscriptPointer) {
            oss << commentBegin << "refcount" << commentEnd << " ";
        } else if (m.mPointer) {
            oss << commentBegin << "pointer" << commentEnd << " ";
        } else if (m.mReference) {
            oss << commentBegin << "reference" << commentEnd << " ";
        }
        if (bNeedIntial) {
            if (m.mOptional || g.isOptional(memberID) || (traits.mStructInterface && sEnableMake)) {
                oss << builder.getTypedMemberName(bPublicFormat, m, m.mPublic, true) << ";";
                if (g.isTypescriptValueType(memberID)) {
                    oss << " " << commentBegin
                        << g.getTypescriptInitialValue(memberID, m, scratch, scratch)
                        << commentEnd << "\n";
                } else {
                    oss << "\n";
                }
            } else {
                oss << builder.getTypedMemberName(bPublicFormat, m, m.mPublic);
                oss << " = " << g.getTypescriptInitialValue(memberID, m, scratch, scratch) << ";\n";
            }
        } else {
            oss << builder.getTypedMemberName(bPublicFormat, m, m.mPublic, true) << ";\n";
        }

        ++i;
    }
    if (kOutputPoolDebug) {
        OSS << "_pool?: boolean;\n";
    }

    if (traits.mStructInterface && sEnableMake) {
        OSS << "[name: string]: unknown;\n";
    }
}

namespace {

void outputSaveCollection(std::ostream& oss, std::pmr::string& space, std::string_view ns,
    const SyntaxGraph& g, SyntaxGraph::vertex_descriptor vertID,
    std::string_view varName,
    uint32_t depth,
    std::pmr::memory_resource* scratch);

void outputSaveMap(std::ostream& oss, std::pmr::string& space, std::string_view ns,
    const SyntaxGraph& g, SyntaxGraph::vertex_descriptor vertID,
    std::string_view varName,
    uint32_t depth,
    std::pmr::memory_resource* scratch);

void outputSaveSerializable(std::ostream& oss, std::pmr::string& space, std::string_view ns,
    const SyntaxGraph& g, SyntaxGraph::vertex_descriptor vertID,
    std::string_view varName,
    uint32_t depth, bool isKey,
    std::pmr::memory_resource* scratch) {
    if (g.isTypescriptPointer(vertID)) {
        OSS << "// skip, " << varName << ": "
            << g.getTypescriptTypename(vertID, scratch, scratch) << "\n";
        return;
    }
    const auto& traits = get(g.traits, g, vertID);
    if (g.isTypescriptBoolean(vertID)) {
        OSS << "ar.writeBool(" << varName << ");\n";
    } else if (g.isTypescriptNumber(vertID) || holds_tag<Enum_>(vertID, g)) {
        OSS << "ar.writeNumber(" << varName << ");\n";
    } else if (g.isTypescriptString(vertID)) {
        OSS << "ar.writeString(" << varName << ");\n";
    } else if (g.isTypescriptMap(vertID)) {
        outputSaveMap(oss, space, ns, g, vertID, varName, depth, scratch);
    } else if (g.isTypescriptArray(vertID, scratch) || g.isTypescriptSet(vertID)) {
        outputSaveCollection(oss, space, ns, g, vertID, varName, depth, scratch);
    } else if (holds_tag<Struct_>(vertID, g)
        || holds_tag<Value_>(vertID, g)
        || holds_tag<Graph_>(vertID, g)) {
        const auto& memberName = get(g.names, g, vertID);
        if (isKey && (traits.mFlags & STRING_KEY)) {
            OSS << "save" << memberName << "(ar, JSON.parse(" << varName << ") as "
                << g.getTypescriptTypename(vertID, scratch, scratch) << ");\n";
        } else {
            OSS << "save" << memberName << "(ar, " << varName << ");\n";
        }
    } else {
        OSS << "// skip: " << varName << ": "
            << g.getTypescriptTypename(vertID, scratch, scratch) << "\n";
    }
}

void outputSaveCollection(std::ostream& oss, std::pmr::string& space, std::string_view ns,
    const SyntaxGraph& g, SyntaxGraph::vertex_descriptor vertID,
    std::string_view varName,
    uint32_t depth,
    std::pmr::memory_resource* scratch) {
    ++depth;
    if (g.isTypescriptSet(vertID)) {
        OSS << "ar.writeNumber(" << varName << ".size); // "
            << g.getTypescriptTypename(vertID, scratch, scratch) << "\n";
    } else {
        OSS << "ar.writeNumber(" << varName << ".length); // "
            << g.getTypescriptTypename(vertID, scratch, scratch) << "\n";
    }
    OSS << "for (const v" << depth << " of " << varName << ") {\n";
    {
        INDENT();
        const auto& instance = get<Instance>(vertID, g);
        Expects(!instance.mParameters.empty());
        const auto& paramPath = instance.mParameters.front();
        const auto paramID = locate(paramPath, g);
        std::pmr::string paramName("v", scratch);
        paramName.append(std::to_string(depth));
        outputSaveSerializable(oss, space, ns, g, paramID, paramName, depth, false, scratch);
    }
    OSS << "}\n";
}

void outputSaveMap(std::ostream& oss, std::pmr::string& space, std::string_view ns,
    const SyntaxGraph& g, SyntaxGraph::vertex_descriptor vertID,
    std::string_view varName,
    uint32_t depth,
    std::pmr::memory_resource* scratch) {
    ++depth;
    OSS << "ar.writeNumber(" << varName << ".size); // "
        << g.getTypescriptTypename(vertID, scratch, scratch) << "\n";
    OSS << "for (const [k" << depth << ", v" << depth << "] of " << varName << ") {\n";
    {
        INDENT();
        const auto& instance = get<Instance>(vertID, g);
        Expects(!instance.mParameters.empty());
        Expects(instance.mParameters.size() == 2);
        const auto& keyPath = instance.mParameters.at(0);
        const auto& valuePath = instance.mParameters.at(1);
        const auto keyID = locate(keyPath, g);
        const auto valueID = locate(valuePath, g);
        std::pmr::string keyName("k", scratch);
        std::pmr::string valueName("v", scratch);
        keyName.append(std::to_string(depth));
        valueName.append(std::to_string(depth));
        outputSaveSerializable(oss, space, ns, g, keyID, keyName, depth, true, scratch);
        outputSaveSerializable(oss, space, ns, g, valueID, valueName, depth, false, scratch);
    }
    OSS << "}\n";
}

void outputLoadCollection(std::ostream& oss, std::pmr::string& space, std::string_view ns,
    const SyntaxGraph& g, SyntaxGraph::vertex_descriptor vertID,
    std::string_view varName,
    uint32_t depth,
    std::pmr::memory_resource* scratch);

void outputLoadMap(std::ostream& oss, std::pmr::string& space, std::string_view ns,
    const SyntaxGraph& g, SyntaxGraph::vertex_descriptor vertID,
    std::string_view varName,
    uint32_t depth,
    std::pmr::memory_resource* scratch);

void outputLoadSerializable(std::ostream& oss, std::pmr::string& space, std::string_view ns,
    const SyntaxGraph& g, SyntaxGraph::vertex_descriptor vertID,
    std::string_view varName,
    uint32_t depth,
    std::pmr::memory_resource* scratch) {
    if (g.isTypescriptPointer(vertID)) {
        OSS << "// skip, " << varName << ": "
            << g.getTypescriptTypename(vertID, scratch, scratch) << "\n";
        return;
    }
    const auto& traits = get(g.traits, g, vertID);
    if (g.isTypescriptBoolean(vertID)) {
        OSS << varName << " = ar.readBool();\n";
    } else if (g.isTypescriptNumber(vertID) || holds_tag<Enum_>(vertID, g)) {
        OSS << varName << " = ar.readNumber();\n";
    } else if (g.isTypescriptString(vertID)) {
        OSS << varName << " = ar.readString();\n";
    } else if (g.isTypescriptMap(vertID)) {
        outputLoadMap(oss, space, ns, g, vertID, varName, depth, scratch);
    } else if (g.isTypescriptArray(vertID, scratch) || g.isTypescriptSet(vertID)) {
        outputLoadCollection(oss, space, ns, g, vertID, varName, depth, scratch);
    } else if (holds_tag<Struct_>(vertID, g) || holds_tag<Value_>(vertID, g)) {
        const auto& memberName = get(g.names, g, vertID);
        OSS << "load" << memberName << "(ar, " << varName << ");\n";
    } else {
        OSS << "// skip: " << varName << ": "
            << g.getTypescriptTypename(vertID, scratch, scratch) << "\n";
    }
}

void outputLoadCollection(std::ostream& oss, std::pmr::string& space, std::string_view ns,
    const SyntaxGraph& g, SyntaxGraph::vertex_descriptor vertID,
    std::string_view varName,
    uint32_t depth,
    std::pmr::memory_resource* scratch) {
    ++depth;
    // size
    const auto& name = get(g.names, g, vertID);
    std::pmr::string sizeName("sz", scratch);
    if (depth == 1) {
        OSS << sizeName << " = ar.readNumber(); // "
            << g.getTypescriptTypename(vertID, scratch, scratch) << "\n";
    } else {
        sizeName.append(std::to_string(depth));
        OSS << "const " << sizeName << " = ar.readNumber(); // "
            << g.getTypescriptTypename(vertID, scratch, scratch) << "\n";
    }

    // elememt
    const auto& instance = get<Instance>(vertID, g);
    Expects(!instance.mParameters.empty());
    const auto& paramPath = instance.mParameters.front();
    const auto paramID = locate(paramPath, g);
    
    std::pmr::string counterName("i", scratch);
    counterName.append(std::to_string(depth));

    std::pmr::string paramName(scratch);
    bool bArray = g.isTypescriptArray(vertID, scratch);
    paramName.append("v");
    paramName.append(std::to_string(depth));

    if (bArray) {
        OSS << varName << ".length = " << sizeName << ";\n";
    }

    // for each element
    OSS << "for (let " << counterName << " = 0; "
        << counterName << " !== " << sizeName << "; ++" << counterName << ") {\n";
    {
        INDENT();
        std::pmr::string lhsName(scratch);
        if (bArray) {
            lhsName.append(varName);
            lhsName.append("[");
            lhsName.append(counterName);
            lhsName.append("]");
        } else {
            lhsName.append("const v");
            lhsName.append(std::to_string(depth));
        }
        if (g.isTypescriptValueType(paramID)) {
            outputLoadSerializable(oss, space, ns, g, paramID, lhsName, depth, scratch);
        } else {
            OSS << "const " << paramName << " = new "
                << g.getTypescriptTypename(paramID, scratch, scratch) << "();\n";
            outputLoadSerializable(oss, space, ns, g, paramID, paramName, depth, scratch);
        }
        if (bArray) {
            if (!g.isTypescriptValueType(paramID)) {
                OSS << lhsName << " = " << paramName << ";\n";
            }
        } else {
            OSS << varName << ".add(" << paramName << ");\n";
        }
    }
    OSS << "}\n";
}

void outputLoadMap(std::ostream& oss, std::pmr::string& space, std::string_view ns,
    const SyntaxGraph& g, SyntaxGraph::vertex_descriptor vertID,
    std::string_view varName,
    uint32_t depth,
    std::pmr::memory_resource* scratch) {
    ++depth;
    // size
    const auto& name = get(g.names, g, vertID);
    std::pmr::string sizeName("sz", scratch);
    if (depth == 1) {
        OSS << sizeName << " = ar.readNumber(); // "
            << g.getTypescriptTypename(vertID, scratch, scratch) << "\n";
    } else {
        sizeName.append(std::to_string(depth));
        OSS << "const " << sizeName << " = ar.readNumber(); // "
            << g.getTypescriptTypename(vertID, scratch, scratch) << "\n";
    }

    // elememt
    const auto& instance = get<Instance>(vertID, g);
    Expects(!instance.mParameters.empty());
    Expects(instance.mParameters.size() == 2);
    const auto& keyPath = instance.mParameters.at(0);
    const auto& valuePath = instance.mParameters.at(1);
    const auto keyID = locate(keyPath, g);
    const auto valueID = locate(valuePath, g);
    std::pmr::string keyName("k", scratch);
    std::pmr::string valueName("v", scratch);
    keyName.append(std::to_string(depth));
    valueName.append(std::to_string(depth));

    std::pmr::string counterName("i", scratch);
    counterName.append(std::to_string(depth));

    std::pmr::string paramName(varName, scratch);
    paramName.append("[");
    paramName.append(counterName);
    paramName.append("]");

    // for each element
    OSS << "for (let " << counterName << " = 0; "
        << counterName << " !== " << sizeName << "; ++" << counterName << ") {\n";
    {
        INDENT();
        if (g.isTypescriptValueType(keyID)) {
            OSS << "const " << keyName;
            std::pmr::string space2;
            outputLoadSerializable(oss, space2, ns, g, keyID, "", depth, scratch);
        } else {
            OSS << "const " << keyName << " = new "
                << g.getTypescriptTypename(keyID, scratch, scratch) << "();\n";
            outputLoadSerializable(oss, space, ns, g, keyID, keyName, depth, scratch);
        }

        if (g.isTypescriptValueType(valueID)) {
            OSS << "const " << valueName;
            std::pmr::string space2;
            outputLoadSerializable(oss, space2, ns, g, valueID, "", depth, scratch);
        } else {
            OSS << "const " << valueName << " = new "
                << g.getTypescriptTypename(valueID, scratch, scratch) << "();\n";
            outputLoadSerializable(oss, space, ns, g, valueID, valueName, depth, scratch);
        }
        const auto& keyTraits = get(g.traits, g, keyID);
        if (keyTraits.mFlags & STRING_KEY) {
            OSS << varName << ".set(JSON.stringify(" << keyName << "), " << valueName << ");\n";
        } else {
            OSS << varName << ".set(" << keyName << ", " << valueName << ");\n";
        }
    }
    OSS << "}\n";
}

std::pmr::string generateGraphSerialization_ts(
    std::string_view ns,
    const SyntaxGraph& g, SyntaxGraph::vertex_descriptor vertID,
    const Graph& s, std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);
    Expects(s.isVector());

    auto typePath = g.getTypePath(vertID, scratch);
    auto typeName = g.getDependentName(ns, vertID, scratch, scratch);
    auto cppName = getCppPath(typeName, scratch);
    const auto& name = get(g.names, g, vertID);

    oss << "\n";
    OSS << "export function save" << cppName << " (ar: OutputArchive, g: " << cppName << "): void {\n";
    {
        INDENT();
        OSS << "const numVertices = g.numVertices();\n";
        OSS << "const numEdges = g.numEdges();\n";
        const auto sizeID = locate("/uint32_t", g);
        outputSaveSerializable(oss, space, ns, g, sizeID, "numVertices", 0, false, scratch);
        outputSaveSerializable(oss, space, ns, g, sizeID, "numEdges", 0, false, scratch);
        if (s.isPolymorphic()) {
            for (const auto& c : s.mPolymorphic.mConcepts) {
                std::pmr::string numTypes("num", scratch);
                numTypes.append(c.mMemberName.substr(1));
                OSS << "let " << numTypes << " = 0;\n";
            }
            OSS << "for (const v of g.vertices()) {\n";
            {
                INDENT();
                OSS << "switch (g.id(v)) {\n";
                for (const auto& c : s.mPolymorphic.mConcepts) {
                    std::pmr::string numTypes("num", scratch);
                    numTypes.append(c.mMemberName.substr(1));
                    const auto tagID = locate(c.mTag, g);
                    const auto& tagName = get(g.names, g, tagID);
                    OSS << "case " << name << "Value." << convertTag(tagName) << ":\n";
                    INDENT();
                    OSS << numTypes << " += 1;\n";
                    OSS << "break;\n";
                }
                OSS << "default:\n";
                OSS << "    break;\n";
                OSS << "}\n";
            }
            OSS << "}\n";
            for (const auto& c : s.mPolymorphic.mConcepts) {
                std::pmr::string numTypes("num", scratch);
                numTypes.append(c.mMemberName.substr(1));
                outputSaveSerializable(oss, space, ns, g, sizeID, numTypes, 0, false, scratch);
            }
        }
        OSS << "for (const v of g.vertices()) {\n";
        {
            INDENT();
            if (s.isPolymorphic()) {
                outputSaveSerializable(oss, space, ns, g, sizeID, "g.id(v)", 0, false, scratch);
            }
            if (s.isAddressable()) {
                outputSaveSerializable(oss, space, ns, g, sizeID, "g.getParent(v)", 0, false, scratch);
            }
            for (const auto& c : s.mComponents) {
                const auto componentID = locate(c.mValuePath, g);
                std::pmr::string componentVar("g.get", scratch);
                componentVar.append(convertTag(c.mName));
                componentVar.append("(v)");
                outputSaveSerializable(oss, space, ns, g, componentID, componentVar, 0, false, scratch);
            }
            if (s.isPolymorphic()) {
                OSS << "switch (g.id(v)) {\n";
                for (const auto& c : s.mPolymorphic.mConcepts) {
                    const auto objectID = locate(c.mValue, g);
                    const auto tagID = locate(c.mTag, g);
                    const auto& tagName = get(g.names, g, tagID);
                    std::pmr::string objectVar("g.get", scratch);
                    objectVar.append(convertTag(tagName));
                    objectVar.append("(v)");

                    OSS << "case " << name << "Value." << convertTag(tagName) << ":\n";
                    {
                        INDENT();
                        outputSaveSerializable(oss, space, ns, g, objectID, objectVar, 0, false, scratch);
                        OSS << "break;\n";
                    }
                }
                OSS << "default:\n";
                OSS << "    break;\n";
                OSS << "}\n";
            }
        }
        OSS << "}\n";

        Expects(s.isAliasGraph());

        for (const auto& m : s.mMembers) {
            if (m.mFlags & GenerationFlags::NO_SERIALIZATION)
                continue;

            auto memberID = locate(m.mTypePath, g);
            if ((m.mFlags & GenerationFlags::NO_SERIALIZATION)
                || (m.mFlags & GenerationFlags::IMPL_DETAIL)) {
                continue;
            }
            std::pmr::string memberVar("g.", scratch);
            memberVar.append(m.getMemberName());
            outputSaveSerializable(oss, space, ns, g, memberID, memberVar, 0, false, scratch);
        }
    }
    oss << "}\n";

    oss << "\n";
    OSS << "export function load" << cppName << " (ar: InputArchive, g: " << cppName << "): void {\n";
    {
        INDENT();
        const auto sizeID = locate("/uint32_t", g);
        outputLoadSerializable(oss, space, ns, g, sizeID, "const numVertices", 0, scratch);
        outputLoadSerializable(oss, space, ns, g, sizeID, "const numEdges", 0, scratch);
        if (s.isPolymorphic()) {
            for (const auto& c : s.mPolymorphic.mConcepts) {
                std::pmr::string numTypes("const num", scratch);
                numTypes.append(c.mMemberName.substr(1));
                outputLoadSerializable(oss, space, ns, g, sizeID, numTypes, 0, scratch);
            }
        }
        OSS << "for (let v = 0; v !== numVertices; ++v) {\n";
        {
            INDENT();
            if (s.isPolymorphic()) {
                outputLoadSerializable(oss, space, ns, g, sizeID, "const id", 0, scratch);
            }
            if (s.isAddressable()) {
                outputLoadSerializable(oss, space, ns, g, sizeID, "const u", 0, scratch);
            }
            for (const auto& c : s.mComponents) {
                const auto componentID = locate(c.mValuePath, g);
                std::pmr::string componentVar(scratch);
                if (g.isTypescriptValueType(componentID)) {
                    componentVar.append("const ");
                    componentVar.append(getTagVariableName(c.mName, scratch));
                } else {
                    componentVar.append(getTagVariableName(c.mName, scratch));
                    OSS << "const " << componentVar << " = new "
                        << g.getTypescriptTypename(componentID, scratch, scratch) << "();\n";
                }
                outputLoadSerializable(oss, space, ns, g, componentID, componentVar, 0, scratch);
            }
            if (s.isPolymorphic()) {
                OSS << "switch (id) {\n";
                for (const auto& c : s.mPolymorphic.mConcepts) {
                    const auto objectID = locate(c.mValue, g);
                    const auto tagID = locate(c.mTag, g);
                    const auto& tagName = get(g.names, g, tagID);
                    std::pmr::string objectVar(scratch);
                    objectVar.append(getTagVariableName(tagName, scratch));
                    std::pmr::string typeName(name, scratch);
                    typeName.append("Value.");
                    typeName.append(convertTag(tagName));
                    OSS << "case " << typeName << ": {\n";
                    {
                        INDENT();
                        if (g.isTypescriptValueType(objectID)) {
                            outputLoadSerializable(oss, space, ns, g, objectID, "const " + objectVar, 0, scratch);
                        } else {
                            OSS << "const " << objectVar << " = new "
                                << g.getTypescriptTypename(objectID, scratch, scratch) << "();\n";
                            outputLoadSerializable(oss, space, ns, g, objectID, objectVar, 0, scratch);
                        }
                        OSS << "g.addVertex<" << typeName << ">(" << typeName << ", " << objectVar;
                        for (const auto& c : s.mComponents) {
                            const auto componentID = locate(c.mValuePath, g);
                            std::pmr::string componentVar(scratch);
                            componentVar.append(getTagVariableName(c.mName, scratch));
                            oss << ", " << componentVar;
                        }
                        if (s.isAddressable()) {
                            oss << ", u";
                        }
                        oss << ");\n";
                        OSS << "break;\n";
                    }
                    OSS << "}\n";
                }
                OSS << "default:\n";
                OSS << "    break;\n";
                OSS << "}\n";
            } else { // isPolymorphic
                OSS << "g.addVertex(";
                for (uint32_t count = 0; const auto& c : s.mComponents) {
                    if (count++) {
                        oss << ", ";
                    }
                    const auto componentID = locate(c.mValuePath, g);
                    std::pmr::string componentVar(scratch);
                    componentVar.append(getTagVariableName(c.mName, scratch));
                    oss << componentVar;
                }
                if (s.isAddressable()) {
                    oss << "u";
                }
                oss << ");\n";
            } // isPolymorphic
        }
        OSS << "}\n";

        Expects(s.isAliasGraph());

        for (uint32_t count = 0; const auto& m : s.mMembers) {
            if (m.mFlags & GenerationFlags::NO_SERIALIZATION)
                continue;

            auto memberID = locate(m.mTypePath, g);
            if ((m.mFlags & GenerationFlags::NO_SERIALIZATION)
                || (m.mFlags & GenerationFlags::IMPL_DETAIL)) {
                continue;
            }
            if (count++ == 0) {
                OSS << "let sz = 0;\n";
            }
            std::pmr::string memberVar("g.", scratch);
            memberVar.append(m.getMemberName());
            outputLoadSerializable(oss, space, ns, g, memberID, memberVar, 0, scratch);
        }
    }
    oss << "}\n";
    return oss.str();
}

} // namespace

std::pmr::string generateSerialization_ts(
    std::string_view projectName, const SyntaxGraph& g,
    const ModuleGraph& mg,
    std::string_view moduleName0,
    bool nvp,
    std::pmr::memory_resource* mr,
    std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios_base::out, mr);
    std::pmr::string space(scratch);

    const auto moduleID = locate(moduleName0, mg);
    const auto& moduleInfo = get(mg.modules, mg, moduleID);
    const bool bDLL = !moduleInfo.mAPI.empty();
    std::pmr::string apiDLL(moduleInfo.mAPI, scratch);
    apiDLL.append("_API");

    std::string_view ns = "/cc/render";

    int numTrival = 0;
    for (const auto& vertID : make_range(vertices(g))) {
        const auto& moduleName = get(g.modulePaths, g, vertID);
        if (moduleName != moduleName0) {
            continue;
        }

        if (holds_tag<Define_>(vertID, g)) {
            continue;
        }
        const auto& traits = get(g.traits, g, vertID);
        if (traits.mUnknown)
            continue;
        if (traits.mImport)
            continue;
        if (traits.mFlags & GenerationFlags::NO_SERIALIZATION)
            continue;

        auto typePath = g.getTypePath(vertID, scratch);
        auto typeName = g.getDependentName(ns, vertID, scratch, scratch);
        auto cppName = getCppPath(typeName, scratch);
        const auto& name = get(g.names, g, vertID);

        visit_vertex(
            vertID, g,
            [&](const Tag& t) {
            },
            [&](const Struct& s) {
                auto parentID = parent(vertID, g);
                if (holds_tag<Struct_>(parentID, g)
                    || holds_tag<Graph_>(parentID, g))
                    return;

                if (traits.mTrivial) {
                    return;
                }
                numTrival = 0;
                oss << "\n";
                oss << "export function save" << cppName << " (ar: OutputArchive, v: " << cppName << "): void {\n";
                {
                    INDENT();
                    if (nvp) {
                        Expects(false);
                    } else {
                        for (const auto& m : s.mMembers) {
                            if (m.mFlags & GenerationFlags::NO_SERIALIZATION)
                                continue;

                            auto memberID = locate(m.mTypePath, g);
                            if ((m.mFlags & GenerationFlags::NO_SERIALIZATION)
                                || (m.mFlags & GenerationFlags::IMPL_DETAIL)) {
                                continue;
                            }
                            if (m.mPointer) {
                                OSS << "// skip, v." << m.getMemberName() << ": "
                                    << g.getTypescriptTypename(memberID, scratch, scratch) << "\n";
                                continue;
                            }
                            std::pmr::string memberVar("v.", scratch);
                            memberVar.append(m.getMemberName());
                            outputSaveSerializable(oss, space, ns, g, memberID, memberVar, 0, false, scratch);
                        }
                    }
                }
                oss << "}\n";
                oss << "\n";
                oss << "export function load" << cppName << " (ar: InputArchive, v: " << cppName << "): void {\n";
                {
                    INDENT();
                    if (nvp) {
                        Expects(false);
                    } else {
                        int numContainer = 0;
                        for (const auto& m : s.mMembers) {
                            if (m.mFlags & GenerationFlags::NO_SERIALIZATION)
                                continue;

                            auto memberID = locate(m.mTypePath, g);
                            if ((m.mFlags & GenerationFlags::NO_SERIALIZATION)
                                || (m.mFlags & GenerationFlags::IMPL_DETAIL)) {
                                continue;
                            }
                            if (m.mPointer) {
                                OSS << "// skip, v." << m.getMemberName() << ": "
                                    << g.getTypescriptTypename(memberID, scratch, scratch) << "\n";
                                continue;
                            }
                            if (g.isTypescriptArray(memberID, scratch)
                                || g.isTypescriptMap(memberID)
                                || g.isTypescriptSet(memberID)) {
                                if (numContainer++ == 0) {
                                    OSS << "let sz = 0;\n";
                                }
                            }
                            std::pmr::string memberVar("v.", scratch);
                            memberVar.append(m.getMemberName());
                            outputLoadSerializable(oss, space, ns, g, memberID, memberVar, 0, scratch);
                        }
                    }
                }
                oss << "}\n";
            },
            [&](const Graph& s) {
                copyString(oss, space, generateGraphSerialization_ts(ns, g, vertID, s, scratch));
            },
            [&](const auto&) {
            });
    }

    return oss.str();
}

namespace {

SyntaxGraph::vertex_descriptor
getChainedArrayValueID(const SyntaxGraph& g,
    SyntaxGraph::vertex_descriptor vertID,
    std::pmr::memory_resource* scratch) {
    while (vertID != g.null_vertex()) {
        if (g.isTypescriptMap(vertID) || g.isTypescriptValueType(vertID)) {
            return g.null_vertex();
        }
        if (g.isTypescriptArray(vertID, scratch)) {
            const auto& instance = get<Instance>(vertID, g);
            Expects(!instance.mParameters.empty());
            Expects(instance.mParameters.size() == 1);
            const auto& valuePath = instance.mParameters.at(0);
            vertID = locate(valuePath, g);
        } else {
            return vertID;
        }
    }
    return SyntaxGraph::null_vertex();
}

bool isEqualType(const SyntaxGraph& g,
    const SyntaxGraph::vertex_descriptor vertID,
    std::pmr::memory_resource* scratch) {
    return g.isTypescriptValueType(vertID)
        || g.isTypescriptArray(vertID, scratch)
        || g.isTypescriptMap(vertID)
        || holds_tag<Struct_>(vertID, g)
        || holds_tag<Graph_>(vertID, g);
}

} // namespace

void outputFunctions(std::ostream& oss, std::pmr::string& space,
    const SyntaxGraph& g,
    const SyntaxGraph::vertex_descriptor vertID,
    std::pmr::memory_resource* scratch) {
    std::string_view ns = "/cc/render";
    int numTrival = 0;
    
    const auto& traits = get(g.traits, g, vertID);
    if (traits.mUnknown)
        return;
    if (traits.mImport)
        return;
    if ((traits.mFlags & GenerationFlags::NO_EQUAL) || !(traits.mFlags & GenerationFlags::EQUAL))
        return;

    auto typePath = g.getTypePath(vertID, scratch);
    auto typeName = g.getDependentName(ns, vertID, scratch, scratch);
    auto cppName = getCppPath(typeName, scratch);
    const auto& name = get(g.names, g, vertID);

    visit_vertex(
        vertID, g,
        [&](const Tag& t) {
        },
        [&](const Struct& s) {
            auto parentID = parent(vertID, g);
            if (holds_tag<Struct_>(parentID, g)
                || holds_tag<Graph_>(parentID, g))
                return;

            if (traits.mTrivial) {
                return;
            }
            numTrival = 0;
            oss << "\n";
            oss << "export function equal" << cppName << " (lhs: " << cppName << ", rhs: " << cppName << "): boolean {\n";
            {
                INDENT();
                uint32_t count = 0;
                for (const auto& m : s.mMembers) {
                    if (m.mFlags & GenerationFlags::NO_EQUAL)
                        continue;
                    auto memberID = locate(m.mTypePath, g);
                    if ((m.mFlags & GenerationFlags::NO_EQUAL)
                        || (m.mFlags & GenerationFlags::IMPL_DETAIL)) {
                        continue;
                    }
                    std::pmr::string lhs("lhs.", scratch);
                    lhs.append(m.getMemberName());
                    std::pmr::string rhs("rhs.", scratch);
                    rhs.append(m.getMemberName());

                    auto memberName = g.getTypescriptTypename(memberID, scratch, scratch);
                        
                    if (count++ == 0) {
                        OSS << "return ";
                    } else if (isEqualType(g, memberID, scratch)) {
                        oss << "\n";
                        OSS << "    && ";
                    }
                    
                    if (g.isTypescriptValueType(memberID)) {
                        oss << lhs << " === " << rhs;
                    } else if (g.isTypescriptArray(memberID, scratch)) {
                        const auto& instance = get<Instance>(memberID, g);
                        Expects(!instance.mParameters.empty());
                        Expects(instance.mParameters.size() == 1);
                        const auto& valuePath = instance.mParameters.at(0);
                        const auto valueID = locate(valuePath, g);
                        if (g.isTypescriptMap(valueID)) {
                            throw std::runtime_error("array of map is not supported");
                        }
                        if (g.isTypescriptValueType(valueID)) {
                            oss << "equalValueArray(" << lhs << ", " << rhs << ")";
                        } else {
                            const auto realValueID = getChainedArrayValueID(g, valueID, scratch);
                            if (realValueID == g.null_vertex()) {
                                throw std::runtime_error("array of unknown is not supported");
                            }
                            const auto& valueName = g.getTypescriptTypename(realValueID, scratch, scratch);
                            oss << "equalObjectArray(" << lhs << ", " << rhs << ", equal" << valueName << ")";
                        }
                    } else if (g.isTypescriptMap(memberID)) {
                        const auto& instance = get<Instance>(memberID, g);
                        Expects(!instance.mParameters.empty());
                        Expects(instance.mParameters.size() == 2);
                        const auto& keyPath = instance.mParameters.at(0);
                        const auto& valuePath = instance.mParameters.at(1);
                        const auto keyID = locate(keyPath, g);
                        const auto valueID = locate(valuePath, g);
                        if (g.isTypescriptMap(valueID)) {
                            throw std::runtime_error("map of map is not supported");
                        }
                        if (g.isTypescriptValueType(valueID)) {
                            oss << "equalValueMap(" << lhs << ", " << rhs << ")";
                        } else {
                            const auto realValueID = getChainedArrayValueID(g, valueID, scratch);
                            if (realValueID == g.null_vertex()) {
                                throw std::runtime_error("array of unknown is not supported");
                            }
                            const auto& valueName = g.getTypescriptTypename(realValueID, scratch, scratch);
                            oss << "equalObjectMap(" << lhs << ", " << rhs << ", equal" << valueName << ")";
                        }
                    } else if (holds_tag<Struct_>(memberID, g) || holds_tag<Graph_>(memberID, g)) {
                        oss << "equal" << memberName << "(" << lhs << ", " << rhs << ")";
                    } else {
                        oss << "/* skip: " << m.getMemberName() << ": "
                            << g.getTypescriptTypename(vertID, scratch, scratch) << " */";
                    }
                }
                if (count) {
                    oss << ";\n";
                }
            }
            oss << "}\n";
        },
        [&](const Graph& s) {
        },
        [&](const auto&) {
        });
}

}
