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

namespace {

constexpr bool sFormat = true;
constexpr bool sBoost = true;

bool outputNamespaces(std::ostream& oss, std::pmr::string& space,
    CodegenContext& context, std::string_view typePath) {
    bool bNewNamespace = false;
    size_t end = 0;
    auto pos = typePath.find_first_of('/');
    for (size_t i = 0; i != context.mNamespaces.size(); ++i) {
        if (pos == std::string_view::npos) {
            break;
        }
        auto pos1 = typePath.find_first_of('/', pos + 1);
        auto ns = typePath.substr(pos + 1, pos1 - (pos + 1));
        if (ns != context.mNamespaces[i]) {
            break;
        }

        if (pos1 != std::string_view::npos) {
            pos = pos1;
        } else {
            pos = std::string_view::npos;
        }
        ++end;
    }

    for (auto i = context.mNamespaces.size(); i-- > end;) {
        oss << "\n";
        OSS << "} // namespace " << context.mNamespaces[i] << "\n";
        context.mNamespaces.pop_back();
        bNewNamespace = true;
    }

    while (pos != std::string_view::npos) {
        auto pos1 = typePath.find_first_of('/', pos + 1);
        auto name = typePath.substr(pos + 1, pos1 - (pos + 1));
        oss << "\n";
        OSS << "namespace " << name << " {\n";
        context.mNamespaces.emplace_back(name);

        if (pos1 != std::string_view::npos) {
            pos = pos1;
        } else {
            pos = std::string_view::npos;
        }
        bNewNamespace = true;
    }

    return bNewNamespace;
}

}

std::pmr::string generateFwd_h(const SyntaxGraph& g,
    std::string_view moduleName0,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios_base::out, mr);
    std::pmr::string space(scratch);
    CodegenContext context(scratch);

    BreakType prevType = BreakType::Default;
    int count = 0;
    bool prevConstraints = false;
    auto lineBreak = [&](BreakType type = BreakType::Default, bool bForce = false) {
        if (count++ == 0) {
            prevType = type;
            return;
        }
        if (bForce) {
            oss << "\n";
            return;
        }
        if (prevConstraints) {
            oss << "\n";
            prevConstraints = false;
            prevType = type;
            return;
        }
        switch (type) {
        case BreakType::Default: {
            oss << "\n";
            break;
        }
        case BreakType::Alias: {
            if (prevType != BreakType::Alias) {
                oss << "\n";
            }
            break;
        }
        case BreakType::Enum: {
            if (prevType != BreakType::Enum) {
                oss << "\n";
            }
            break;
        }
        case BreakType::Tag: {
            if (prevType != BreakType::Tag) {
                oss << "\n";
            }
            break;
        }
        case BreakType::Variant: {
            if (prevType != BreakType::Variant) {
                oss << "\n";
            }
            break;
        }
        case BreakType::Struct: {
            if (prevType != BreakType::Struct) {
                oss << "\n";
            }
            break;
        }
        default:
            break;
        }
        prevType = type;
    };

    bool usesHashCombine = g.moduleUsesHashCombine(moduleName0);
    if (usesHashCombine) {
        oss << "#include <functional>\n";
    }

    for (const auto& vertID : make_range(vertices(g))) {
        const auto& moduleName = get(g.modulePaths, g, vertID);
        if (moduleName != moduleName0) {
            continue;
        }
        if (!holds_tag<Define_>(vertID, g)) {
            continue;
        }
        const auto& def = get<Define>(vertID, g);
        const auto& name = get(g.names, g, vertID);
        oss << "\n";
        OSS << "#define " << name << " " << def.mContent << "\n";
    }

    for (int i = 0; const auto& vertID : make_range(vertices(g))) {
        const auto& moduleName = get(g.modulePaths, g, vertID);
        if (moduleName != moduleName0) {
            continue;
        }
        if (!holds_tag<Concept_>(vertID, g)) {
            continue;
        }
        const auto& c = get<Concept>(vertID, g);
        const auto& name = get(g.names, g, vertID);

        auto ns = g.getNamespace(vertID, scratch);

        auto outputNs = [&]() {
            auto bNewNamespace = outputNamespaces(oss, space, context, ns);
            if (bNewNamespace) {
                oss << "\n";
                count = 0;
            }
        };

        outputNs();
        lineBreak();
        oss << "template <typename T> struct Is" << convertTag(name) << " { static constexpr bool value = false; };\n";
        oss << "template <typename T> concept " << name << " = ";
        if (!c.mParentPath.empty()) {
            auto parentID = locate(c.mParentPath, g);
            auto superType = g.getDependentName(ns, parentID, scratch, scratch);
            oss << getCppPath(superType, scratch) << "<T> && ";
        }
        oss << "Is" << convertTag(name) << "<T>::value;\n";
    }

    PmrMap<std::pmr::string,
        std::pmr::vector<SyntaxGraph::vertex_descriptor>>
        constrains(scratch);

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

        auto typePath = g.getTypePath(vertID, scratch);
        auto ns = g.getNamespace(vertID, scratch);

        auto outputNs = [&]() {
            auto bNewNamespace = outputNamespaces(oss, space, context, ns);
            if (bNewNamespace) {
                oss << "\n";
                count = 0;
            }
        };

        const auto& name = get(g.names, g, vertID);
        const auto& constraints = get(g.constraints, g, vertID);

        auto outputConstraints = [&]() {
            pmr_ostringstream oss(std::ios_base::out, mr);
            std::pmr::string space(scratch);
            if (!constraints.mConcepts.empty()) {
                for (const auto& conceptPath : constraints.mConcepts) {
                    const auto conceptID0 = locate(conceptPath, g);
                    auto conceptID = conceptID0;
                    std::pmr::vector<std::pmr::string> outputs(scratch);

                    while (conceptID != g.null_vertex()) {
                        const auto& conceptName = g.getDependentName(ns, conceptID, scratch, scratch);
                        std::pmr::string traitName(conceptName, scratch);
                        auto pos = traitName.rfind('/');
                        if (pos != traitName.npos) {
                            traitName.insert(pos + 1, "Is");
                        } else {
                            traitName.insert(0, "Is");
                        }
                        traitName.pop_back();
                        outputs.emplace_back(std::move(traitName));
                        const auto& c = get_by_tag<Concept_>(conceptID, g);
                        if (!c.mParentPath.empty()) {
                            conceptID = locate(c.mParentPath, g);
                        } else {
                            conceptID = g.null_vertex();
                        }
                    }

                    for (auto iter = outputs.rbegin(); iter != outputs.rend(); ++iter) {
                        auto ns = g.getNamespace(conceptID0, scratch);
                        if (ns == g.getNamespace(vertID, scratch)) {
                            OSS << "template <> struct " << getCppPath(*iter, scratch) << "<"
                                << name << "> { static constexpr bool value = true; };\n";
                            prevConstraints = true;
                        } else {
                            constrains[conceptPath].emplace_back(vertID);
                        }
                    }
                }
            }
            return oss.str();
        };

        std::pmr::string constraintsContent(scratch);

        visit_vertex(
            vertID, g,
            [&](const Alias& s) {
                if (s.mTypePath.empty())
                    return;

                outputNs();
                lineBreak(BreakType::Alias);

                auto typeID = locate(s.mTypePath, g);
                auto typeName = g.getDependentName(ns, typeID, scratch, scratch);
                OSS << "using " << name << " = " << getCppPath(typeName, scratch) << ";\n";
            },
            [&](const Enum& e) {
                if (!e.mUnderlyingType.empty() || traits.mClass) {
                    outputNs();
                    lineBreak(BreakType::Enum);
                    OSS << "enum ";
                    if (traits.mClass)
                        oss << "class ";
                    oss << name;
                    if (!e.mUnderlyingType.empty()) {
                        oss << " : " << e.mUnderlyingType;
                    }
                    oss << ";\n";
                }
            },
            [&](const Tag& t) {
                outputNs();
                lineBreak(BreakType::Struct);
                OSS << "struct " << name << ";\n";
                constraintsContent = outputConstraints();
            },
            [&](const Variant& v) {
                outputNs();
                lineBreak(BreakType::Variant);
                if (sBoost) {
                    OSS << "using " << name << " = boost::variant2::variant<";
                } else {
                    OSS << "using " << name << " = std::variant<";
                }
                for (auto count = 0; const auto& e : v.mVariants) {
                    if (count++)
                        oss << ", ";
                    OSS << getCppPath(getDependentPath(ns, e), scratch);
                }
                oss << ">;\n";
            },
            [&](const Composition_ auto&) {
                auto parentID = parent(vertID, g);
                if (holds_tag<Struct_>(parentID, g)
                    || holds_tag<Graph_>(parentID, g))
                    return;

                outputNs();
                lineBreak(BreakType::Struct);
                if (traits.mClass) {
                    OSS << "class " << name << ";\n";
                } else {
                    OSS << "struct " << name << ";\n";
                }
                constraintsContent = outputConstraints();
            },
            [&](const auto&) {
            });
        copyString(oss, space, constraintsContent);
    }

    outputNamespaces(oss, space, context, "");

    if (!constrains.empty()) {
        for (const auto& [conceptPath, typeIDs] : constrains) {
            auto conceptID = locate(conceptPath, g);
            auto ns = g.getNamespace(conceptID, scratch);
            outputNamespaces(oss, space, context, ns);
            auto conceptPath = g.getDependentName(ns, conceptID, scratch, scratch);
            auto conceptName = getCppPath(conceptPath, scratch);
            Expects(!conceptName.empty() && conceptName.back() == '_');
            conceptName.pop_back();
            for (int count = 0; const auto& typeID : typeIDs) {
                if (count++ == 0)
                    oss << "\n";
                auto typePath = g.getDependentName(ns, typeID, scratch, scratch);
                auto name = getCppPath(typePath, scratch);
                OSS << "template <> struct Is" << conceptName << "<"
                    << name << "> { static constexpr bool value = true; };\n";
            }
        }
        outputNamespaces(oss, space, context, "");
    }

    if (usesHashCombine) {
        oss << "\n";
        OSS << "namespace std {\n";

        for (const auto& vertID : make_range(vertices(g))) {
            const auto& moduleName = get(g.modulePaths, g, vertID);
            if (moduleName != moduleName0) {
                continue;
            }

            const auto& traits = get(g.traits, g, vertID);
            if (traits.mFlags & GenerationFlags::HASH_COMBINE) {
                auto name = getCppPath(g.getDependentName("/std", vertID, scratch, scratch), scratch);
                oss << "\n";
                OSS << "template <>\n";
                OSS << "struct hash<" << name << "> {\n";
                {
                    INDENT();
                    OSS << "size_t operator()(const " << name << "& v) const noexcept;\n";
                }
                OSS << "};\n";
            }
        }
        oss << "\n";
        OSS << "}\n";
    }

    return oss.str();
}

std::pmr::string generateNames_h(const SyntaxGraph& g,
    const ModuleGraph& mg,
    std::string_view moduleName0,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios_base::out, mr);
    std::pmr::string space(scratch);
    CodegenContext context(scratch);

    int count = 0;
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

        auto typePath = g.getTypePath(vertID, scratch);
        auto ns = g.getNamespace(vertID, scratch);

        auto outputNs = [&]() {
            auto bNewNamespace = outputNamespaces(oss, space, context, ns);
            if (bNewNamespace) {
                oss << "\n";
                count = 0;
            }
        };

        const auto& name = get(g.names, g, vertID);
        const auto& constraints = get(g.constraints, g, vertID);

        visit_vertex(
            vertID, g,
            [&](const Enum& s) {
                if (s.mIsFlags)
                    return;
                if (!s.mHasName)
                    return;

                outputNs();
                OSS << "inline const char* getName(" << name << " e) noexcept {\n";
                OSS << "    switch (e) {\n";
                for (const auto& e : s.mValues) {
                    INDENT();
                    OSS << "    case ";
                    if (traits.mClass) {
                        oss << name << "::";
                    }
                    oss << e.mName << ": return \"" << e.mName << "\";\n";
                }
                OSS << "    }\n";
                OSS << "    return \"\";\n";
                OSS << "}\n";
            },
            [&](const Tag& t) {
                std::pmr::string name2(name, scratch);
                Expects(name2.size());
                name2.resize(name2.size() - 1);
                outputNs();
                OSS << "inline const char* getName(const " << name << "& /*v*/) noexcept { "
                    << "return \"" << name2 << "\"; }\n";
            },
            [&](const Composition_ auto&) {
                auto parentID = parent(vertID, g);
                if (holds_tag<Struct_>(parentID, g)
                    || holds_tag<Graph_>(parentID, g))
                    return;

                outputNs();
                OSS << "inline const char* getName(const " << name << "& /*v*/) noexcept { "
                    << "return \"" << name << "\"; }\n";
            },
            [&](const auto&) {
            });
    }
    outputNamespaces(oss, space, context, "");

    return oss.str();
}

namespace {

struct VisitorTypes_h : boost::dfs_visitor<> {
    using VisitGraph = Impl::AddressableView<SyntaxGraph>;
    bool needOutput(SyntaxGraph::vertex_descriptor vertID, const SyntaxGraph& g) {
        const auto& moduleName = get(g.modulePaths, g, vertID);
        if (moduleName != mModuleName)
            return false;

        const auto& traits = get(g.traits, g, vertID);
        if (traits.mImport)
            return false;

        if (traits.mUnknown)
            return false;

        if (holds_tag<Value_>(vertID, g))
            return false;

        return visit_vertex(
            vertID, g,
            [&](const Identifier_ auto&) {
                return false;
            },
            [&](const Data_ auto&) {
                return true;
            },
            [&](const Algebra_ auto&) {
                return true;
            },
            [&](const Composition_ auto&) {
                return true;
            },
            [&](const Template_ auto&) {
                return false;
            },
            [&](const Instantiation_ auto&) {
                return false;
            });
    }
    void lineBreak(BreakType type = BreakType::Default) {
        switch (type) {
        case BreakType::Default: {
            oss << "\n";
            break;
        }
        case BreakType::Tag: {
            if (mPrevType != BreakType::Tag) {
                oss << "\n";
            }
            break;
        }
        default:
            break;
        }
        mPrevType = type;
    };

    void outputOverride(CppStructBuilder& cpp, const SyntaxGraph& g,
        const std::pmr::vector<std::pmr::string>& basePaths) {
        auto apiDLL = mAPI;
        for (const auto& basePath : basePaths) {
            auto baseID = locate(basePath, g);
            const auto& baseTraits = get(g.traits, g, baseID);
            visit_vertex(
                baseID, g,
                [&](const Composition_ auto& s) {
                    if (!baseTraits.mInterface)
                        return;
                    for (const auto& func : s.mMemberFunctions) {
                        size_t pos = 0;
                        std::pmr::string str(func, scratch);
                        while (true) {
                            pos = str.find("virtual ", pos);
                            if (pos == str.npos)
                                break;

                            pos = str.find(";", pos);
                            if (pos == str.npos) {
                                pos = str.find("}");
                            }
                            auto pos0 = str.rfind(")", pos);
                            auto pos1 = str.find("=", pos0);

                            if (pos1 != str.npos && pos1 < pos) {
                                str.replace(pos1, pos - pos1, "override");
                            }
                        }
                        boost::algorithm::replace_all(str, "virtual ", apiDLL);
                        boost::algorithm::replace_all(str, "[[no_sender]] ", apiDLL);
                        boost::algorithm::replace_all(str, "[[sender]] ", apiDLL);
                        boost::algorithm::replace_all(str, "[[dll]] ", apiDLL);
                        copyCppString(oss, space, str);
                    }
                },
                [&](const auto&) {
                });
            const auto& childBases = get(g.inherits, g, baseID).mBases;
            outputOverride(cpp, g, childBases);
        }
    }

    template <Composition_ T>
    void outputMembers(CppStructBuilder& cpp,
        const SyntaxGraph& g, SyntaxGraph::vertex_descriptor vertID,
        const T& s) {
        const auto& name = get(g.names, g, vertID);
        const auto& traits = get(g.traits, g, vertID);
        const auto& inherits = get(g.inherits, g, vertID);
        int count = 0;
        const bool bEmpty = s.mMembers.empty();
        {
            if (!bEmpty && g.isPmr(vertID)) {
                if (count++)
                    oss << "\n";
                OSS << "using allocator_type = boost::container::pmr::polymorphic_allocator<char>;\n";
                if (mDLL) {
                    OSS << mAPI << cpp.generateGetAllocatorSignature(true) << "; // NOLINT\n";
                } else {
                    OSS << cpp.generateGetAllocatorSignature(true) << " { // NOLINT\n";
                    {
                        INDENT();
                        copyString(oss, space, cpp.generateGetAllocatorBody());
                    }
                    OSS << "}\n";
                }
            }
            // cntrs
            auto cntrs = cpp.generateHeaderConstructors();
            copyString(oss, space, cntrs);
            if (!cntrs.empty())
                ++count;
        }

        if (traits.mFlags & SPACESHIP) {
            OSS << "auto operator<=>(const " << name << "&) const = default;\n";
        }

        // virtual functions
        if (!traits.mInterface) {
            outputOverride(cpp, g, inherits.mBases);
        }

        // member functions
        if (!s.mMemberFunctions.empty()) {
            ++count;
            copyString(oss, cpp.generateMemberFunctions(space));
            if (s.mMemberFunctions.back().ends_with(":\n")) {
                count = 0;
            }
        }

        // members
        if (!s.mMembers.empty()) {
            Expects(!s.mMembers.empty());
            if (count++)
                oss << "\n";
            copyString(oss, space, cpp.generateMembers());
        }
    }

    void discover_vertex(SyntaxGraph::vertex_descriptor vertID, const VisitGraph& g0) {
        const auto& g = g0.mGraph;
        if (!needOutput(vertID, g))
            return;

        const auto& name = get(g.names, g, vertID);
        const auto& traits = get(g.traits, g, vertID);
        auto ns = g.getNamespace(vertID, scratch);
        CppStructBuilder cpp(&g, &mModuleGraph, vertID, mModuleID, ns, mProjectName, scratch);

        auto bNewNamespace = outputNamespaces(oss, space, mContext, ns);

        visit_vertex(
            vertID, g,
            [&](const Enum& e) {
                lineBreak();
                OSS << "enum ";
                if (traits.mClass)
                    oss << "class ";
                oss << name;
                if (!e.mUnderlyingType.empty()) {
                    oss << " : " << e.mUnderlyingType;
                }
                oss << " {\n";
                if (!e.mIsFlags) {
                    for (const auto& v : e.mValues) {
                        if (v.mValue.empty()) {
                            oss << "    " << v.mName << ",\n";
                        } else {
                            oss << "    " << v.mName << " = "
                                << v.mValue << ",\n";
                        }
                    }
                } else {
                    size_t maxLength = 0;
                    for (const auto& v : e.mValues) {
                        maxLength = std::max(maxLength, v.mName.size());
                    }

                    for (const auto& v : e.mValues) {
                        oss << "    " << v.mName;
                        if (sFormat) {
                            oss << std::pmr::string(maxLength - v.mName.size(), ' ');
                        }
                        oss << " = " << v.mValue << ",\n";
                    }
                }
                OSS << "};\n";
            },
            [&](const Tag& t) {
                lineBreak(BreakType::Tag);

                if (t.mEntity) {
                    OSS << "struct " << name << " {\n";
                    OSS << "} static constexpr " << convertTag(name) << ";\n";
                } else {
                    OSS << "struct " << name << " {};\n";
                }
            },
            [&](const Variant& v) {
                lineBreak();
                if (sBoost) {
                    OSS << "using " << name << " = boost::variant2::variant<";
                } else {
                    OSS << "using " << name << " = std::variant<";
                }
                for (auto count = 0; const auto& e : v.mVariants) {
                    if (count++)
                        oss << ", ";
                    OSS << getCppPath(getDependentPath(ns, e), scratch);
                }
                oss << ">;\n";
            },
            [&](const Struct& s) {
                lineBreak();
                CppStructBuilder cpp(&g, &mModuleGraph, vertID, mModuleID, ns, mProjectName, scratch);
                if (traits.mClass) {
                    OSS << "class ";
                } else {
                    OSS << "struct ";
                }

                if (traits.mAlignment) {
                    oss << "alignas(" << traits.mAlignment << ") ";
                }

                oss << name;

                // Inheritance not implemented yet
                const auto& inherits = get(g.inherits, g, vertID);
                for (int count = 0; const auto& base : inherits.mBases) {

                    if (count++ == 0) {
                        if (!traits.mInterface)
                            oss << " final";
                        oss << " : public ";
                    } else {
                        oss << ", public ";
                    }
                    oss << cpp.getDependentName(base);
                }

                oss << " {";
                auto pos = name.find('_');
                if (pos != name.npos && pos != 0 && pos != name.size() - 1) {
                    oss << " // NOLINT";
                }
                if (num_children(vertID, g) || !s.mMembers.empty() || g.hasHeader(vertID))
                    oss << "\n";

                if (traits.mClass)
                    oss << "public:\n";
                space.append("    ");
            },
            [&](const Graph&) {
                lineBreak();
                CppStructBuilder cpp(&g, &mModuleGraph, vertID, mModuleID, ns, mProjectName, scratch);
                OSS << "struct ";

                if (traits.mAlignment) {
                    oss << "alignas(" << traits.mAlignment << ") ";
                }
                oss << name;

                // Inheritance not implemented yet

                oss << " {";
                oss << "\n";
                space.append("    ");

                CppGraphBuilder builder(&g, &mModuleGraph, vertID,
                    mModuleID, ns, mDLL, mProjectName, scratch);
                copyString(oss, space, builder.generateAllocator_h());
                copyString(oss, space, builder.generateConstructors_h());
                copyString(oss, space, builder.generateGraph_h());
                copyString(oss, space, builder.generateIncidenceGraph_h());
                copyString(oss, space, builder.generateBidirectionalGraph_h());
                copyString(oss, space, builder.generateAdjacencyGraph_h());
                copyString(oss, space, builder.generateVertexListGraph_h());
                copyString(oss, space, builder.generateEdgeListGraph_h());
                copyString(oss, space, builder.generateMutableGraph_h());
                copyString(oss, space, builder.generateReferenceGraph_h());
                copyString(oss, space, builder.generateParentGraph_h());
                copyString(oss, space, builder.generateAddressableGraph_h());
                copyString(oss, space, builder.generatePolymorphicGraph_h());
                copyString(oss, space, builder.generateMemberFunctions_h());
                copyString(oss, space, builder.generateReserve_h());
                oss << "\n";
                OSS << "// Members";
            },
            [&](const auto&) {
            });
    }

    void finish_vertex(SyntaxGraph::vertex_descriptor vertID, const VisitGraph& g0) {
        const auto& g = g0.mGraph;
        if (!needOutput(vertID, g))
            return;

        const auto& name = get(g.names, g, vertID);
        const auto& traits = get(g.traits, g, vertID);
        auto ns = g.getNamespace(vertID, scratch);
        CppStructBuilder cpp(&g, &mModuleGraph, vertID, mModuleID, ns, mProjectName, scratch);

        visit_vertex(
            vertID, g,
            [&](const Enum& e) {
                if (e.mIsFlags && e.mEnumOperator) {
                    std::string_view t = e.mUnderlyingType;
                    if (t.empty()) {
                        t = "int";
                    }
                    oss << "\n";
                    OSS << "constexpr " << name << " operator|(";
                    oss << "const " << name << " lhs, ";
                    oss << "const " << name << " rhs) noexcept {\n";
                    {
                        INDENT();
                        OSS << "return static_cast<" << name << ">(static_cast<" << t << ">(lhs) | static_cast<" << t << ">(rhs));\n";
                    }
                    OSS << "}\n";

                    oss << "\n";
                    OSS << "constexpr " << name << " operator&(";
                    oss << "const " << name << " lhs, ";
                    oss << "const " << name << " rhs) noexcept {\n";
                    {
                        INDENT();
                        OSS << "return static_cast<" << name << ">(static_cast<" << t << ">(lhs) & static_cast<" << t << ">(rhs));\n";
                    }
                    OSS << "}\n";

                    oss << "\n";
                    OSS << "constexpr " << name << "& operator|=(";
                    oss << name << "& lhs, const " << name << " rhs) noexcept {\n";
                    {
                        INDENT();
                        OSS << "return lhs = lhs | rhs;\n";
                    }
                    OSS << "}\n";

                    oss << "\n";
                    OSS << "constexpr " << name << "& operator&=(";
                    oss << name << "& lhs, const " << name << " rhs) noexcept {\n";
                    {
                        INDENT();
                        OSS << "return lhs = lhs & rhs;\n";
                    }
                    OSS << "}\n";

                    oss << "\n";
                    OSS << "constexpr bool operator!(" << name << " e) noexcept {\n";
                    OSS << "    return e == static_cast<" << name << ">(0);\n";
                    OSS << "}\n";

                    oss << "\n";
                    OSS << "constexpr bool any(" << name << " e) noexcept {\n";
                    OSS << "    return !!e;\n";
                    OSS << "}\n";
                }
            },
            [&](const Tag& t) {
                if (traits.mFlags & EQUAL) {
                    OSS << cpp.generateOperatorSignature(OperatorType::Equal, true) << " { return true; }\n";
                    OSS << cpp.generateOperatorSignature(OperatorType::Unequal, true) << " { return false; }\n";
                }
            },
            [&](const Variant& v) {
                if (g.isTag(vertID)) {
                    if (traits.mFlags & LESS) {
                        oss << "\n";
                        OSS << cpp.generateOperatorSignature(OperatorType::Less, true) << " {\n";
                        {
                            INDENT();
                            OSS << "return lhs.index() < rhs.index();\n";
                        }
                        OSS << "}\n";
                    }
                    if (traits.mFlags & EQUAL) {
                        oss << "\n";
                        OSS << cpp.generateOperatorSignature(OperatorType::Equal, true) << " {\n";
                        {
                            INDENT();
                            OSS << "return lhs.index() == rhs.index();\n";
                        }
                        OSS << "}\n";
                        oss << "\n";
                        OSS << cpp.generateOperatorSignature(OperatorType::Unequal, true) << " {\n";
                        {
                            INDENT();
                            OSS << "return !(lhs == rhs);\n";
                        }
                        OSS << "}\n";
                    }
                }
            },
            [&](const Struct& s) {
                outputMembers(cpp, g, vertID, s);
                Expects(space.size() >= 4);
                space.resize(space.size() - 4);

                int numDefined = 0;
                for (const auto& e : make_range(children(vertID, g))) {
                    if (!get(g.traits, g, vertID).mUnknown) {
                        ++numDefined;
                    }
                }

                if (numDefined || !s.mMembers.empty()) {
                    OSS;
                }
                oss << "};\n";

                if (auto needDtor = g.needDtor(vertID, mDLL);
                    needDtor == ImplEnum::Separated || needDtor == ImplEnum::Inline) {
                    if (traits.mInterface) {
                        bool bDerived = !get(g.inherits, g, vertID).mBases.empty();
                        if (!bDerived && !mDLL) {
                            oss << "\n";
                            OSS << "inline " << name << "::~" << name << "() noexcept = default;\n";
                        }
                    }
                }

                if (traits.mFlags & EQUAL) {
                    oss << "\n";
                    OSS << cpp.generateOperatorSignature(OperatorType::Equal, true) << " {\n";
                    {
                        INDENT();
                        copyString(oss, space, cpp.generateOperatorBody(OperatorType::Equal));
                    }
                    OSS << "}\n";
                    oss << "\n";
                    OSS << cpp.generateOperatorSignature(OperatorType::Unequal, true) << " {\n";
                    {
                        INDENT();
                        copyString(oss, space, cpp.generateOperatorBody(OperatorType::Unequal));
                    }
                    OSS << "}\n";
                }
                if (traits.mFlags & LESS) {
                    oss << "\n";
                    OSS << cpp.generateOperatorSignature(OperatorType::Less, true) << " {\n";
                    {
                        INDENT();
                        copyString(oss, space, cpp.generateOperatorBody(OperatorType::Less));
                    }
                    OSS << "}\n";
                }
            },
            [&](const Graph& s) {
                CppGraphBuilder builder(&g, &mModuleGraph, vertID,
                    mModuleID, ns, mDLL, mProjectName, scratch);
                copyString(oss, space, builder.generateTags_h());
                copyString(oss, space, builder.generateMembers_h());
                Expects(space.size() >= 4);
                space.resize(space.size() - 4);
                oss << "};\n";
            },
            [&](const auto&) {
            });
    }

    std::ostream& oss;
    std::pmr::string& space;
    std::string_view mModuleName;
    BreakType& mPrevType;
    std::pmr::memory_resource* scratch = nullptr;
    const ModuleGraph& mModuleGraph;
    ModuleGraph::vertex_descriptor mModuleID = ModuleGraph::null_vertex();
    bool mDLL = false;
    std::string_view mAPI;
    std::string_view mProjectName;
    CodegenContext& mContext;
};

struct VisitorTypes_cpp : boost::dfs_visitor<> {
    using VisitGraph = Impl::AddressableView<SyntaxGraph>;

    void discover_vertex(SyntaxGraph::vertex_descriptor vertID, const VisitGraph& g0) {
        const auto& g = g0.mGraph;
        const auto& name = get(g.names, g, vertID);
        const auto& traits = get(g.traits, g, vertID);
        auto ns = g.getNamespace(vertID, scratch);
        const auto& mg = mModuleGraph;
        const auto moduleID = mModuleID;
        CppStructBuilder cpp(&g, &mModuleGraph, vertID, moduleID, ns, mProjectName, scratch);

        visit_vertex(
            vertID, g,
            [&](const Struct& s) {
                outputNamespaces(oss, space, mContext, ns);
                CppStructBuilder cpp(&g, &mg, vertID, moduleID, ns, mProjectName, scratch);
                auto cntrs = cpp.generateCppConstructors();
                copyString(oss, space, cntrs);
            },
            [&](const Graph&) {
                outputNamespaces(oss, space, mContext, ns);
                const auto& moduleInfo = get(mg.modules, mg, moduleID);
                const bool bDLL = !moduleInfo.mAPI.empty();
                CppGraphBuilder builder(&g, &mg, vertID,
                    moduleID, ns, bDLL, mProjectName, scratch);
                const auto& cpp = builder.mStruct;
                copyString(oss, space, cpp.generateCppConstructors());
                copyString(oss, space, builder.generateReserve_cpp());
            },
            [&](const auto&) {
            });
    }

    std::ostream& oss;
    std::pmr::string& space;
    std::string_view mModuleName;
    std::pmr::memory_resource* scratch = nullptr;
    const ModuleGraph& mModuleGraph;
    ModuleGraph::vertex_descriptor mModuleID = ModuleGraph::null_vertex();
    std::string_view mProjectName;
    CodegenContext& mContext;
};

}

std::pmr::string generateTypes_h(std::string_view projectName,
    const SyntaxGraph& g,
    const ModuleGraph& mg,
    std::string_view moduleName0,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios_base::out, mr);
    std::pmr::string space(scratch);

    const auto moduleID = locate(moduleName0, mg);
    const auto& moduleInfo = get(mg.modules, mg, moduleID);
    const bool bDLL = !moduleInfo.mAPI.empty();
    std::pmr::string apiDLL(moduleInfo.mAPI, scratch);
    if (!apiDLL.empty())
        apiDLL.append("_API ");

    CodegenContext context(scratch);

    Impl::AddressableView<SyntaxGraph> g1(const_cast<SyntaxGraph&>(g));

    BreakType prevType{};
    VisitorTypes_h visitor{
        {}, oss, space, moduleName0, prevType, scratch,
        mg, moduleID, bDLL, apiDLL, projectName, context
    };

    auto colors = g.colors(scratch);
    for (int count = 0; const auto& vertID : make_range(vertices(g))) {
        if (colors[vertID] != boost::default_color_type::white_color) {
            continue;
        }
        if (!visitor.needOutput(vertID, g)) {
            continue;
        }
        if (!g.isComposition(vertID)) {
            colors[vertID] = boost::default_color_type::black_color;
            visitor.discover_vertex(vertID, g1);
            visitor.finish_vertex(vertID, g1);
            continue;
        }
        boost::depth_first_visit(g1, vertID, visitor, get(colors, g));
    }
    outputNamespaces(oss, space, context, "");

    if (g.moduleUsesHashCombine(moduleName0)) {
        oss << "\n";
        OSS << "namespace std {\n";

        for (const auto& vertID : make_range(vertices(g))) {
            const auto& moduleName = get(g.modulePaths, g, vertID);
            if (moduleName != moduleName0) {
                continue;
            }

            const auto& traits = get(g.traits, g, vertID);
            visit_vertex(
                vertID, g,
                [&](const Composition_ auto& s) {
                    if (traits.mFlags & GenerationFlags::HASH_COMBINE) {
                        auto name = getCppPath(g.getDependentName("/std", vertID, scratch, scratch), scratch);
                        oss << "\n";
                        OSS << "inline size_t hash<" << name << ">::operator()(const " << name << "& v) const noexcept {\n";
                        {
                            INDENT();
                            OSS << "size_t seed = 0;\n";
                            for (const auto& m : s.mMembers) {
                                OSS << "boost::hash_combine(seed, v." << m.mMemberName << ");\n";
                            }
                            OSS << "return seed;\n";
                        }
                        OSS << "}\n";
                    }
                },
                [&](const auto&) {
                });
        }
        oss << "\n";
        OSS << "} // namespace std\n";
    }

    return oss.str();
}

std::pmr::string generateTypes_cpp(std::string_view projectName,
    const SyntaxGraph& g,
    const ModuleGraph& mg,
    std::string_view moduleName0,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) {
    const auto moduleID = locate(moduleName0, mg);
    const auto& moduleInfo = get(mg.modules, mg, moduleID);
    const bool bDLL = !moduleInfo.mAPI.empty();
    if (!(g.moduleHasImpl(moduleName0, bDLL))) {
        return std::pmr::string(mr);
    }

    pmr_ostringstream oss(std::ios_base::out, mr);
    std::pmr::string space(scratch);

    CodegenContext context(scratch);

    auto colors = g.colors(scratch);
    Impl::AddressableView<SyntaxGraph> g1(const_cast<SyntaxGraph&>(g));
    VisitorTypes_cpp visitor{
        {}, oss, space, moduleName0, scratch,
        mg, moduleID, projectName, context
    };

    for (const auto& vertID : make_range(vertices(g))) {
        if (colors[vertID] != boost::default_color_type::white_color) {
            continue;
        }

        const auto& moduleName = get(g.modulePaths, g, vertID);
        if (moduleName != moduleName0) {
            continue;
        }

        if (!g.hasImpl(vertID, bDLL))
            continue;

        boost::depth_first_visit(g1, vertID, visitor, get(colors, g));
    }

    outputNamespaces(oss, space, context, "");

    return oss.str();
}

std::pmr::string generateGraphs_h(std::string_view projectName,
    const SyntaxGraph& g,
    const ModuleGraph& mg,
    std::string_view moduleName0,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) {
    const auto moduleID = locate(moduleName0, mg);
    const auto& moduleInfo = get(mg.modules, mg, moduleID);
    const bool bDLL = !moduleInfo.mAPI.empty();

    pmr_ostringstream oss(std::ios_base::out, mr);
    std::pmr::string space(scratch);

    CodegenContext context(scratch);

    for (const auto& vertID : make_range(vertices(g))) {
        const auto& moduleName = get(g.modulePaths, g, vertID);
        if (moduleName != moduleName0) {
            continue;
        }
        if (!holds_tag<Graph_>(vertID, g))
            continue;

        const auto& traits = get(g.traits, g, vertID);

        const bool bDLL = !moduleInfo.mAPI.empty();
        std::pmr::string apiDLL(moduleInfo.mAPI, scratch);
        apiDLL.append("_API ");

        auto typePath = g.getTypePath(vertID, scratch);
        auto ns = g.getNamespace(vertID, scratch);
        auto bNewNamespace = outputNamespaces(oss, space, context, ns);

        const auto& name = get(g.names, g, vertID);

        auto& currScope = context.mScopes.back();

        visit_vertex(
            vertID, g,
            [&](const Graph&) {
                const bool bDLL = !moduleInfo.mAPI.empty();
                CppGraphBuilder builder(&g, &mg, vertID,
                    moduleID, ns, bDLL, projectName, scratch);
                copyString(oss, space, builder.generateGraphFunctions_h());
            },
            [&](const auto&) {
            });
    }

    outputNamespaces(oss, space, context, "");

    if (g.moduleHasGraph(moduleName0)) {
        std::string_view ns = "/boost";
        auto bNewNamespace = outputNamespaces(oss, space, context, ns);

        for (const auto& vertID : make_range(vertices(g))) {
            const auto& moduleName = get(g.modulePaths, g, vertID);
            if (moduleName != moduleName0) {
                continue;
            }
            if (!holds_tag<Graph_>(vertID, g))
                continue;

            const auto& traits = get(g.traits, g, vertID);
            const auto& name = get(g.names, g, vertID);

            visit_vertex(
                vertID, g,
                [&](const Graph&) {
                    const bool bDLL = !moduleInfo.mAPI.empty();
                    CppGraphBuilder builder(&g, &mg, vertID,
                        moduleID, ns, bDLL, projectName, scratch);
                    copyString(oss, space, builder.generateGraphBoostFunctions_h());
                },
                [&](const auto&) {
                });
        }
        outputNamespaces(oss, space, context, "");
    }

    for (const auto& vertID : make_range(vertices(g))) {
        const auto& moduleName = get(g.modulePaths, g, vertID);
        if (moduleName != moduleName0) {
            continue;
        }
        if (!holds_tag<Graph_>(vertID, g))
            continue;

        const auto& traits = get(g.traits, g, vertID);

        auto ns = g.getNamespace(vertID, scratch);
        auto bNewNamespace = outputNamespaces(oss, space, context, ns);

        const auto& name = get(g.names, g, vertID);

        visit_vertex(
            vertID, g,
            [&](const Graph&) {
                const bool bDLL = !moduleInfo.mAPI.empty();
                CppGraphBuilder builder(&g, &mg, vertID,
                    moduleID, ns, bDLL, projectName, scratch);
                copyString(oss, space, builder.generateGraphPropertyMaps_h());
            },
            [&](const auto&) {
            });
    }
    outputNamespaces(oss, space, context, "");

    return oss.str();
}

std::pmr::string generateReflection_h(std::string_view projectName,
    const SyntaxGraph& g,
    const ModuleGraph& mg,
    std::string_view moduleName0,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios_base::out, mr);
    std::pmr::string space(scratch);
    CodegenContext context(scratch);

    const auto moduleID = locate(moduleName0, mg);
    const auto& moduleInfo = get(mg.modules, mg, moduleID);
    const bool bDLL = !moduleInfo.mAPI.empty();
    std::pmr::string apiDLL(moduleInfo.mAPI, scratch);
    apiDLL.append("_API");

    int count = 0;
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

        auto typePath = g.getTypePath(vertID, scratch);
        auto ns = g.getNamespace(vertID, scratch);

        auto outputNs = [&]() {
            auto bNewNamespace = outputNamespaces(oss, space, context, ns);
            if (bNewNamespace) {
                count = 0;
            }
        };

        const auto& name = get(g.names, g, vertID);
        const auto& constraints = get(g.constraints, g, vertID);

        visit_vertex(
            vertID, g,
            [&](const Enum& s) {
                if (!(traits.mFlags & REFLECTION)) {
                    return;
                }

                outputNs();

                oss << "\n";

                if (bDLL) {
                    oss << apiDLL << " ";
                }
                oss << "bool try_getType(std::string_view name, " << name << "& type) noexcept;\n";

                if (bDLL) {
                    oss << apiDLL << " ";
                }
                oss << "bool try_getType(std::u8string_view name, " << name << "& type) noexcept;\n";

                if (bDLL) {
                    oss << apiDLL << " ";
                }
                oss << "void getType(std::string_view name, " << name << "& type);\n";

                if (bDLL) {
                    oss << apiDLL << " ";
                }
                oss << "void getType(std::u8string_view name, " << name << "& type);\n";
            },
            [&](const Variant& v) {
                if (!(traits.mFlags & REFLECTION))
                    return;

                outputNs();

                oss << "\n";

                if (bDLL) {
                    oss << apiDLL << " ";
                }
                oss << "bool try_getType(std::string_view name, " << name << "& type) noexcept;\n";

                if (bDLL) {
                    oss << apiDLL << " ";
                }
                oss << "bool try_getType(std::u8string_view name, " << name << "& type) noexcept;\n";

                if (bDLL) {
                    oss << apiDLL << " ";
                }
                oss << "void getType(std::string_view name, " << name << "& type);\n";

                if (bDLL) {
                    oss << apiDLL << " ";
                }
                oss << "void getType(std::u8string_view name, " << name << "& type);\n";
            },
            [&](const auto&) {
            });
    }
    outputNamespaces(oss, space, context, "");

    return oss.str();
}

std::pmr::string generateReflection_cpp(std::string_view projectName,
    const SyntaxGraph& g,
    const ModuleGraph& mg,
    std::string_view moduleName0,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios_base::out, mr);
    std::pmr::string space(scratch);
    CodegenContext context(scratch);

    const auto moduleID = locate(moduleName0, mg);
    const auto& moduleInfo = get(mg.modules, mg, moduleID);
    const bool bDLL = !moduleInfo.mAPI.empty();
    std::pmr::string apiDLL(moduleInfo.mAPI, scratch);
    apiDLL.append("_API");

    int count = 0;
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

        auto typePath = g.getTypePath(vertID, scratch);
        auto ns = g.getNamespace(vertID, scratch);

        auto outputNs = [&]() {
            auto bNewNamespace = outputNamespaces(oss, space, context, ns);
            if (bNewNamespace) {
                count = 0;
            }
        };

        const auto& name = get(g.names, g, vertID);
        const auto& constraints = get(g.constraints, g, vertID);

        visit_vertex(
            vertID, g,
            [&](const Enum& s) {
                if (!(traits.mFlags & REFLECTION)) {
                    return;
                }
                outputNs();
                oss << "\n";
                oss << "bool try_getType(std::string_view name, " << name << "& type) noexcept {\n";
                {
                    INDENT();
                    OSS << "static const std::unordered_map<std::string_view, " << name << "> index({\n";
                    {
                        INDENT();
                        for (const auto& e : s.mValues) {
                            std::string_view enumName;
                            if (e.mReflectionName.empty()) {
                                enumName = e.mName;
                            } else {
                                enumName = e.mReflectionName;
                            }

                            if (name.empty())
                                continue;

                            OSS << "{ std::string_view(\"" << enumName << "\"), " << name << "::" << e.mName << " },\n";
                        }
                    }
                    OSS << "});\n";
                    oss << "\n";
                    OSS << "auto iter = index.find(name);\n";
                    OSS << "if (iter != index.end()) {\n";
                    OSS << "    type = iter->second;\n";
                    OSS << "    return true;\n";
                    OSS << "} else {\n";
                    OSS << "    return false;\n";
                    OSS << "}\n";
                }
                oss << "}\n";
                oss << "\n";
                oss << "void getType(std::string_view name, " << name << "& type) {\n";
                oss << "    if (!try_getType(name, type)) {\n";
                oss << "        throw std::out_of_range(\"getType(" << name << ")\");\n";
                oss << "    }\n";
                oss << "}\n";

                oss << "\n";
                oss << "bool try_getType(std::u8string_view name, " << name << "& type) noexcept {\n";
                oss << "    return try_getType(std::string_view(reinterpret_cast<const char*>(name.data()), name.size()), type);\n";
                oss << "}\n";

                oss << "\n";
                oss << "void getType(std::u8string_view name, " << name << "& type) {\n";
                oss << "    getType(std::string_view(reinterpret_cast<const char*>(name.data()), name.size()), type);\n";
                oss << "}\n";
            },
            [&](const Variant& s) {
                if (!(traits.mFlags & REFLECTION))
                    return;

                outputNs();
                oss << "\n";
                oss << "bool try_getType(std::string_view name, " << name << "& type) noexcept {\n";
                {
                    INDENT();
                    OSS << "static const std::unordered_map<std::string_view, " << name << "> index{\n";
                    {
                        INDENT();
                        for (const auto& var : s.mVariants) {
                            auto varID = locate(var, g);
                            auto varName = getCppPath(g.getDependentName(ns, varID, scratch, scratch), scratch);
                            visit_vertex(
                                varID, g,
                                [&](const Tag& t) {
                                    std::string_view name2 = varName;
                                    Expects(!name2.empty());
                                    Expects(name2.back() == '_');
                                    name2 = name2.substr(0, name2.size() - 1);
                                    OSS << "{ std::string_view(\"" << name2 << "\"), " << name << "(std::in_place_type_t<" << varName << ">()) },\n";
                                },
                                [&](const Struct& s) {
                                    if (var == "/std/monostate") {
                                        OSS << "{ std::string_view(\"\"), "
                                            << name << "(std::in_place_type_t<" << varName << ">()) },\n";
                                        OSS << "{ std::string_view(\"_\"), "
                                            << name << "(std::in_place_type_t<" << varName << ">()) },\n";
                                    } else {
                                        Expects(false);
                                    }
                                },
                                [&](const auto&) {
                                });
                        }
                    }
                    OSS << "};\n";
                    oss << "\n";
                    OSS << "auto iter = index.find(name);\n";
                    OSS << "if (iter != index.end()) {\n";
                    OSS << "    type = iter->second;\n";
                    OSS << "    return true;\n";
                    OSS << "} else {\n";
                    OSS << "    return false;\n";
                    OSS << "}\n";
                }
                oss << "}\n";

                oss << "\n";
                oss << "void getType(std::string_view name, " << name << "& type) {\n";
                oss << "    if (!try_getType(name, type)) {\n";
                oss << "        throw std::out_of_range(\"getType(" << name << ")\");\n";
                oss << "    }\n";
                oss << "}\n";

                oss << "\n";
                oss << "bool try_getType(std::u8string_view name, " << name << "& type) noexcept {\n";
                oss << "    return try_getType(std::string_view(reinterpret_cast<const char*>(name.data()), name.size()), type);\n";
                oss << "}\n";

                oss << "\n";
                oss << "void getType(std::u8string_view name, " << name << "& type) {\n";
                oss << "    getType(std::string_view(reinterpret_cast<const char*>(name.data()), name.size()), type);\n";
                oss << "}\n";
            },
            [&](const auto&) {
            });
    }
    outputNamespaces(oss, space, context, "");

    return oss.str();
}

}
