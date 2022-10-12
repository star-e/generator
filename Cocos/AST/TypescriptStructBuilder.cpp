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

namespace Cocos::Meta {

void outputMembers(std::ostream& oss, std::pmr::string& space,
    const ModuleBuilder& builder,
    const SyntaxGraph& g,
    const std::pmr::vector<std::pmr::string>& inherits,
    const std::pmr::vector<Member>& members,
    const std::pmr::vector<std::pmr::string>& functions,
    const std::pmr::vector<Constructor>& cntrs,
    const std::pmr::vector<Method>& methods,
    std::pmr::memory_resource* scratch) {
    const int maxParams = 4;
    if (!cntrs.empty()) {
        auto& cntr = cntrs.front();
        bool bChangeLine = false;
        if (cntr.mIndices.size() > maxParams) {
            bChangeLine = true;
        }

        int count = 0;
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

        auto outputParams = [&](const Constructor& cntr, const std::pmr::vector<Member>& members) {
            for (const auto& id : cntr.mIndices) {
                for (uint32_t i = 0; const auto& m : members) {
                    if (i == id) {
                        outputComma();
                        auto memberID = locate(m.mTypePath, g);
                        auto memberType = g.getTypescriptTypename(memberID, scratch, scratch);
                        if (cntr.mHasDefault) {
                            oss << builder.getTypedMemberName(m, true);
                            oss << " = " << g.getTypescriptInitialValue(memberID, m, scratch, scratch);
                        } else {
                            oss << builder.getTypedMemberName(m, true, true);
                        }
                        if (bChangeLine) {
                            oss << ",\n";
                        }
                    }
                    ++i;
                }
            }
        };
        OSS << "constructor (";
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
                    outputParams(s.mConstructors.front(), s.mMembers);
                },
                [&](const auto&) {
                });
        }
        outputParams(cntr, members);

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

    if (false && !methods.empty()) {
        OSS << "// API\n";
    }
    for (uint32_t methodID = 0; const auto& method : methods) {
        if (method.mSkip || method.mSetter) {
            ++methodID;
            continue;
        }
        int32_t numParams = static_cast<int32_t>(method.mParameters.size());
        for (; numParams >= 0; --numParams) {
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
                    oss << ": ";
                    oss << g.getTypedParameterName(method.mReturnType, true, true, method.mOptional);
                    oss << ";\n";
                }
                break;
                //oss << "get " << methodName << " (";
            } else if (method.mSetter) {
                auto methodName = method.mFunctionName.substr(3);
                methodName = camelToVariable(methodName, scratch);
                oss << "set " << methodName << " (";
            } else {
                oss << method.mFunctionName << " (";
            }
            for (uint32_t count = 0; const auto& param : method.mParameters) {
                if (count == numParams)
                    break;
                if (param.mName.back() == '_')
                    continue;
                if (count++)
                    oss << ", ";
                oss << param.mName;
                auto paramID = locate(param.mTypePath, g);
                oss << ": " << g.getTypedParameterName(param, true, true, param.mOptional);
            }
            oss << ")";
            if (!method.mSetter) {
                oss << ": ";
                oss << g.getTypedParameterName(method.mReturnType, true, true, method.mOptional);
            }
            oss << ";";
            if (false && method.mConst) {
                oss << " /*const*/";
            }
            oss << "\n";

            if (numParams > 0 && method.mParameters[numParams - 1].mDefaultValue.empty())
                break;
        }
        ++methodID;
    }

    if (!functions.empty()) {
        for (const auto& func : functions) {
            copyString(oss, space, func);
        }
    }

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
        if (!m.mPointer && !bTypscriptPointer && (m.mReference || m.mConst || !g.isTypescriptValueType(memberID))) {
            oss << "readonly ";
        } else if (bTypscriptPointer) {
            oss << "/*refcount*/ ";
        } else if (m.mPointer) {
            oss << "/*pointer*/ ";
        } else if (m.mReference) {
            oss << "/*reference*/ ";
        }
        if (bNeedIntial) {
            oss << builder.getTypedMemberName(m, m.mPublic);
            oss << " = " << g.getTypescriptInitialValue(memberID, m, scratch, scratch) << ";\n";
        } else {
            oss << builder.getTypedMemberName(m, m.mPublic, true) << ";\n";
        }

        ++i;
    }
}

namespace {

void outputArray(std::ostream& oss, std::pmr::string& space,
    const SyntaxGraph& g, SyntaxGraph::vertex_descriptor vertID,
    std::string_view varName,
    uint32_t depth) {
    
    OSS << "{ // Array\n";
    {
        INDENT();
        OSS << "ar.writeNumber(" << varName << ".length);\n";
        //OSS << "for ("
    }
    OSS << "}\n";
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
    CodegenContext context(scratch);

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
                oss << "export function save" << cppName << " (ar: OutputArchive, v: " << cppName << ") {\n";
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
                            if (g.isTypescriptPointer(memberID)) {
                                OSS << "// skip, " << m.getMemberName() << ": "
                                    << g.getDependentCppName(ns, memberID, scratch, scratch) << "\n";
                                continue;
                            }
                            if (g.isTypescriptBoolean(memberID)) {
                                OSS << "ar.writeBool(v." << m.getMemberName() << ");\n";
                            } else if (g.isTypescriptNumber(memberID) || holds_tag<Enum_>(memberID, g)) {
                                OSS << "ar.writeNumber(v." << m.getMemberName() << ");\n";
                            } else if (g.isTypescriptString(memberID)) {
                                OSS << "ar.writeString(v." << m.getMemberName() << ");\n";
                            } else if (g.isTypescriptMap(memberID)) {
                                OSS << "// Map\n";
                            } else if (g.isTypescriptArray(memberID, scratch)) {
                                OSS << "// Array\n";
                            } else if (holds_tag<Struct_>(memberID, g)) {
                                OSS << "// Struct\n";
                                const auto& memberName = get(g.names, g, memberID);
                                OSS << "save" << memberName << "(ar, v." << m.getMemberName() << ");\n";
                            } else {
                                OSS << "// skip: " << m.getMemberName() << ": "
                                        << g.getDependentCppName(ns, memberID, scratch, scratch) << "\n";
                            }
                        }
                    }
                }
                oss << "}\n";
                oss << "\n";
                oss << "export function load" << cppName << " (ar: InputArchive, v: " << cppName << ") {\n";
                {
                    INDENT();
                    //if (nvp) {
                    //    Expects(false);
                    //} else {
                    //    for (const auto& m : s.mMembers) {
                    //        if (m.mFlags & GenerationFlags::NO_SERIALIZATION)
                    //            continue;

                    //        auto memberID = locate(m.mTypePath, g);
                    //        if ((m.mFlags & GenerationFlags::NO_SERIALIZATION)
                    //            || (m.mFlags & GenerationFlags::IMPL_DETAIL)) {
                    //            continue;
                    //        }
                    //        if (g.isTypescriptPointer(memberID)) {
                    //            OSS << "// skip, " << m.getMemberName() << ": "
                    //                << g.getDependentCppName(ns, memberID, scratch, scratch) << "\n";
                    //            continue;
                    //        }
                    //        OSS << "load(ar, v." << m.getMemberName() << ");\n";
                    //    }
                    //}
                }
                oss << "}\n";
            },
            [&](const Graph& s) {
                // const bool bDLL = !moduleInfo.mAPI.empty();
                // CppGraphBuilder builder(&g, &mg, vertID,
                //     moduleID, ns, bDLL, projectName, scratch);
                // copyString(oss, space, builder.generateGraphSerialization_h(nvp));
            },
            [&](const auto&) {
            });
    }

    return oss.str();
}

}
