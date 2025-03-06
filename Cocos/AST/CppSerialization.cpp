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

#include "CppBuilder.h"
#include "SyntaxGraphs.h"
#include "SyntaxUtils.h"
#include <Cocos/GraphView.h>

namespace Cocos::Meta {

std::pmr::string generateSerialization_h(
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

    oss << "\n";
    oss << "namespace cc {\n";
    oss << "\n";
    oss << "namespace render {\n";

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

        auto typePath = g.getTypePath(vertID);
        auto typeName = g.getDependentName(ns, vertID);
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

                numTrival = 0;
                oss << "\n";
                oss << "void save(OutputArchive& ar, const " << cppName << "& v);\n";
                oss << "void load(InputArchive& ar, " << cppName << "& v);\n";
            },
            [&](const Graph& s) {
                const bool bDLL = !moduleInfo.mAPI.empty();
                CppGraphBuilder builder(&g, &mg, vertID,
                    moduleID, ns, bDLL, projectName, scratch);
                copyString(oss, space, builder.generateGraphSerialization_h(nvp));
            },
            [&](const auto&) {
            });
    }

    oss << "\n";
    oss << "} // namespace render\n";
    oss << "\n";
    oss << "} // namespace cc\n";

    return oss.str();
}

std::pmr::string generateSerialization_cpp(
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

    oss << "\n";
    oss << "namespace cc {\n";
    oss << "\n";
    oss << "namespace render {\n";

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

        auto typePath = g.getTypePath(vertID);
        auto typeName = g.getDependentName(ns, vertID);
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

                numTrival = 0;
                oss << "\n";
                oss << "void save(OutputArchive& ar, const " << cppName << "& v) {\n";
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
                            if (g.isTypescriptPointer(memberID) || m.mPointer) {
                                OSS << "// skip, " << m.getMemberName() << ": "
                                    << g.getDependentCppName(ns, memberID) << "\n";
                                continue;
                            }
                            OSS << "save(ar, v." << m.getMemberName() << ");\n";
                        }
                    }
                }
                oss << "}\n";
                oss << "\n";
                oss << "void load(InputArchive& ar, " << cppName << "& v) {\n";
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
                            if (g.isTypescriptPointer(memberID) || m.mPointer) {
                                OSS << "// skip, " << m.getMemberName() << ": "
                                    << g.getDependentCppName(ns, memberID) << "\n";
                                continue;
                            }
                            OSS << "load(ar, v." << m.getMemberName() << ");\n";
                        }
                    }
                }
                oss << "}\n";
            },
            [&](const Graph& s) {
                 const bool bDLL = !moduleInfo.mAPI.empty();
                 CppGraphBuilder builder(&g, &mg, vertID,
                     moduleID, ns, bDLL, projectName, scratch);
                 copyString(oss, space, builder.generateGraphSerialization_cpp(nvp));
            },
            [&](const auto&) {
            });
    }

    oss << "\n";
    oss << "} // namespace render\n";
    oss << "\n";
    oss << "} // namespace cc\n";

    return oss.str();
}

}
