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

#include "BuilderTypes.h"
#include "SyntaxGraphs.h"
#include "SyntaxUtils.h"

namespace Cocos::Meta {

namespace {

std::string_view gNoDiscard = "";

std::pmr::string pushIncidenceList(const Graph& s,
    std::string_view el, std::string_view v,
    std::string_view res, bool isEdgeListNotNeeded, std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);

    std::pmr::string edgeProperty(scratch);
    if (isEdgeListNotNeeded) {
        edgeProperty = "";
    } else {
        visit(
            overload(
                [&](Vector_) {
                    edgeProperty = ", g.edges.end(), &g.edges";
                },
                [&](auto) {
                    edgeProperty = ", edgeIter";
                }),
            s.mEdgeListType);
    }

    visit(
        overload(
            [&]<Sequence_ T>(T) {
                if (res.empty()) {
                    OSS << el << ".emplace_back(" << v << edgeProperty << ");\n";
                } else {
                    OSS << "auto& " << res << " = " << el << ".emplace_back(" << v << edgeProperty << ");\n";
                }
            },
            [&]<Associative_ T>(T) {
                if (res.empty()) {
                    OSS << el << ".emplace(" << v << edgeProperty << ");\n";
                } else {
                    OSS << "auto " << res << " = " << el << ".emplace(" << v << edgeProperty << ");\n";
                }
            }),
        s.mOutEdgeListType);

    return oss.str();
};

std::pmr::string eraseFromIncidenceList(const Graph& s,
    std::string_view name,
    std::string_view el, std::string_view v,
    std::string_view p,
    bool bOut,
    std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);

    OSS << "// eraseFromIncidenceList\n";
    visit(
        overload(
            [&]<Sequence_ T>(T) {
                OSS << "impl::sequenceEraseIf(" << el << ", ";
                copyString(oss, space, p, true);
                oss << ");\n";
            },
            [&]<UniqueAssociative_ T>(T) {
                if (!s.needEdgeList()) {
                    if (bOut) {
                        OSS << el << ".erase(" << name << "::OutEdge(" << v << "));\n";
                    } else {
                        OSS << el << ".erase(" << name << "::InEdge(" << v << "));\n";
                    }
                } else {
                    if (bOut) {
                        OSS << el << ".erase(" << name << "::OutEdge(" << v << ", {}));\n";
                    } else {
                        OSS << el << ".erase(" << name << "::InEdge(" << v << ", {}));\n";
                    }
                }
            },
            [&](MultiSet_) {
                OSS << "impl::associativeEraseIf(" << el << ", ";
                copyString(oss, space, p, true);
                oss << ");\n";
            }),
        s.mOutEdgeListType);

    return oss.str();
}

std::pmr::string removeDirectedEdgeIf(const Graph& s,
    std::string_view outEdgeList, std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);
    visit(
        overload(
            [&]<Sequence_ T>(T) {
                OSS << "impl::sequenceRemoveIncidenceEdgeIf(first, last, " << outEdgeList << ", std::forward<Predicate>(pred));\n";
            },
            [&]<Associative_ T>(T) {
                OSS << "impl::associativeRemoveIncidenceEdgeIf(first, last, " << outEdgeList << ", std::forward<Predicate>(pred));\n";
            }),
        s.mOutEdgeListType);
    return oss.str();
}

std::pmr::string clearOutEdges(const Graph& s, std::string_view name,
    bool referenceGraph, std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);

    Expects(s.mBidirectional);
    std::pmr::string out_edge_list(scratch);
    std::pmr::string in_edge_list(scratch);

    std::pmr::string outEdgeList(scratch);
    std::pmr::string inEdgeList(scratch);

    if (referenceGraph) {
        out_edge_list = "getChildrenList";
        in_edge_list = "getParentsList";
        outEdgeList = "childrenList";
        inEdgeList = "parentsList";
    } else {
        out_edge_list = "getOutEdgeList";
        in_edge_list = "getInEdgeList";
        outEdgeList = "outEdgeList";
        inEdgeList = "inEdgeList";
    }

    OSS << "// Bidirectional (OutEdges)\n";
    OSS << "auto& " << outEdgeList << " = g." << out_edge_list << "(u);\n";
    OSS << "auto  outEnd" << std::string(std::max(0, int(outEdgeList.size()) - 6), ' ') << " = " << outEdgeList << ".end();\n";
    OSS << "for (auto iter = " << outEdgeList << ".begin(); iter != outEnd; ++iter) {\n";
    {
        INDENT();
        OSS << "auto& " << inEdgeList << " = g." << in_edge_list << "((*iter).get_target());\n";
        copyString(oss, space, eraseFromIncidenceList(s, name, inEdgeList, "u", R"([u](const auto& e) {
    return e.get_target() == u;
})", true, scratch));
        if (!referenceGraph && s.hasEdgeProperty()) {
            OSS << "g.edges.erase((*iter).get_iter());\n";
        }
    }
    OSS << "}\n";
    OSS << "" << outEdgeList << ".clear();\n";

    return oss.str();
}

std::pmr::string clearInEdges(const Graph& s, std::string_view name,
    bool referenceGraph, std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);

    Expects(s.mBidirectional);
    std::pmr::string out_edge_list(scratch);
    std::pmr::string in_edge_list(scratch);

    std::pmr::string outEdgeList(scratch);
    std::pmr::string inEdgeList(scratch);

    if (referenceGraph) {
        out_edge_list = "getChildrenList";
        in_edge_list = "getParentsList";
        outEdgeList = "childrenList";
        inEdgeList = "parentsList";
    } else {
        out_edge_list = "getOutEdgeList";
        in_edge_list = "getInEdgeList";
        outEdgeList = "outEdgeList";
        inEdgeList = "inEdgeList";
    }

    if (referenceGraph) {
        OSS << "// AddressableGraph (Parents)\n";
    } else {
        OSS << "// Bidirectional (InEdges)\n";
    }

    OSS << "auto& " << inEdgeList << " = g." << in_edge_list << "(u);\n";
    OSS << "auto  inEnd" << std::string(std::max(0, int(inEdgeList.size()) - 5), ' ') << " = " << inEdgeList << ".end();\n";
    OSS << "for (auto iter = " << inEdgeList << ".begin(); iter != inEnd; ++iter) {\n";
    {
        INDENT();
        OSS << "auto& " << outEdgeList << " = g." << out_edge_list << "((*iter).get_target());\n";
        copyString(oss, space, eraseFromIncidenceList(s, name, outEdgeList, "u", R"([u](const auto& e) {
    return e.get_target() == u;
})", true, scratch));
        if (!referenceGraph && s.hasEdgeProperty()) {
            OSS << "g.edges.erase((*iter).get_iter());\n";
        }
    }
    OSS << "}\n";
    OSS << "" << inEdgeList << ".clear();\n";

    return oss.str();
}

std::pmr::string clearVertex(const Graph& s, std::string_view name, std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);

    if (s.mUndirected) {
        OSS << "inline void clear_vertex("
            << name << "::vertex_descriptor u, " << name << "& g) noexcept {\n";
        {
            INDENT();
            if (s.hasAddressIndex()) {
                if (s.isAliasGraph()) {
                    Expects(s.mIncidence);
                    OSS << "// AddressableGraph (Alias)\n";
                    OSS << "removePathImpl(u, g);\n";
                    OSS << "CC_EXPECTS(out_degree(u, g) == 0);\n";
                    oss << "\n";
                }
            }

            if (s.mIncidence) {
                OSS << "// Undirected\n";
                OSS << "while (true) {\n";
                {
                    INDENT();
                    OSS << "auto [ei, ei_end] = out_edges(u, g);\n";
                    OSS << "if (ei == ei_end)\n";
                    OSS << "    break;\n";
                    OSS << "remove_edge(*ei, g);\n";
                }
                OSS << "}\n";
            } else {
                OSS << "// (Not)IncidenceGraph, do nothing\n";
            }
        }
        OSS << "}\n";
    } else {
        OSS << "inline void clear_out_edges("
            << name << "::vertex_descriptor u, " << name << "& g) noexcept { // NOLINT\n";
        {
            INDENT();
            if (s.mIncidence) {
                if (s.hasAddressIndex()) {
                    if (s.isAliasGraph()) {
                        Expects(s.mIncidence);
                        OSS << "// AddressableGraph (Alias)\n";
                        OSS << "// only leaf node can be cleared.\n";
                        OSS << "// clear internal node will broke tree structure.\n";
                        OSS << "CC_EXPECTS(out_degree(u, g) == 0);\n";
                        oss << "\n";
                    }
                }

                if (s.mBidirectional) {
                    copyString(oss, space, clearOutEdges(s, name, false, scratch));
                } else {
                    OSS << "// Directed\n";
                    OSS << "for (auto [vi, viend] = vertices(g); vi != viend; ++vi) {\n";
                    {
                        INDENT();
                        OSS << "auto& outEdgeList = g.getOutEdgeList(*vi);\n";
                        copyString(oss, space, eraseFromIncidenceList(
                            s, name, "outEdgeList", "u", R"([u](const auto& e) {
    return e.get_target() == u;
})", true, scratch));
                        OSS << "outEdgeList.clear();\n";
                    }
                    OSS << "}\n";
                }
            } else {
                OSS << "// (Not)IncidenceGraph, do nothing\n";
            }
        }
        OSS << "}\n";

        if (s.mBidirectional) {
            oss << "\n";
            OSS << "inline void clear_in_edges("
                << name << "::vertex_descriptor u, " << name << "& g) noexcept { // NOLINT\n";
            {
                INDENT();
                if (s.hasAddressIndex()) {
                    if (s.isAliasGraph()) {
                        Expects(s.mIncidence);
                        OSS << "// AddressableGraph (Alias)\n";
                        OSS << "CC_EXPECTS(out_degree(u, g) == 0);\n";
                        OSS << "removePathImpl(u, g);\n";
                        oss << "\n";
                    }
                }

                if (s.mIncidence) {
                    copyString(oss, space, clearInEdges(s, name, false, scratch));
                } else {
                    OSS << "// (Not)IncidenceGraph, do nothing\n";
                }
            }
            OSS << "}\n";
        }

        oss << "\n";
        OSS << "inline void clear_vertex("
            << name << "::vertex_descriptor u, " << name << "& g) noexcept { // NOLINT\n";
        {
            INDENT();
            OSS << "clear_out_edges(u, g);\n";
            if (s.mBidirectional) {
                OSS << "clear_in_edges(u, g);\n";
            }
        }
        OSS << "}\n";
    }

    return oss.str();
}

std::pmr::string removePolymorphicType(const SyntaxGraph& g, const CppGraphBuilder& builder,
    const Graph& s, std::string_view name,
    std::pmr::memory_resource* scratch) {
    Expects(s.isPolymorphic());

    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);

    const auto& cpp = builder.mStruct;
    auto ns = builder.mStruct.mCurrentNamespace;

    OSS << "inline void remove_vertex_value_impl(const "
        << name << "::VertexHandle& h, " << name << "& g) noexcept { // NOLINT\n";
    {
        INDENT();
        OSS << "using vertex_descriptor = " << name << "::vertex_descriptor;\n";

        OSS << "cc::visit(\n";
        {
            INDENT();
            OSS << "overload(\n";
            {
                INDENT();
                int count = 0;
                for (const auto& c : s.mPolymorphic.mConcepts) {
                    if (count++)
                        oss << ",\n";
                    OSS << "[&](const " << builder.handleElemType(c, ns, true) << "& h) {";
                    if (!c.isIntrusive()) {
                        INDENT();
                        oss << "\n";
                        if (c.isVector()) {
                            OSS << "g." << g.getMemberName(c.mMemberName, true) << ".erase(g."
                                << g.getMemberName(c.mMemberName, true) << ".begin() + std::ptrdiff_t(h.value));\n";
                            OSS << "if (h.value == g." << g.getMemberName(c.mMemberName, true) << ".size()) {\n";
                            OSS << "    return;\n";
                            OSS << "}\n";
                            OSS << "impl::reindexVectorHandle<"
                                << cpp.getDependentName(c.mTag)
                                << ">(g.vertices, h.value);\n";
                        } else {
                            OSS << "g." << g.getMemberName(c.mMemberName, true) << ".erase(h.value);\n";
                        }
                        UNINDENT();
                        OSS;
                    }
                    oss << "}";
                }
                oss << "),\n";
            }
            OSS << "h);\n";
        }
    }
    OSS << "}\n";

    return oss.str();
}

std::pmr::string removeVertex(const CppGraphBuilder& builder,
    const Graph& s, std::string_view name,
    std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);
    const auto& g = *builder.mStruct.mSyntaxGraph;

    OSS << "inline void remove_vertex("
        << name << "::vertex_descriptor u, " << name << "& g) noexcept { // NOLINT\n";
    {
        INDENT();

        if (s.mAddressable) {
            if (!s.isAliasGraph()) {
                OSS << "{ // AddressableGraph (Separated)\n";
                {
                    INDENT();
                    if (s.hasAddressIndex()) {
                        OSS << "removePathImpl(u, g);\n";
                    }
                    OSS << "CC_EXPECTS(numChildren(u, g) == 0);\n";

                    oss << "\n";
                    copyString(oss, space, clearInEdges(s, name, true, scratch));

                    visit(
                        overload(
                            [&](Vector_) {
                                oss << "\n";
                                OSS << "impl::removeVectorOwner(g, u);\n";
                            },
                            [&](auto) {
                                throw std::invalid_argument("only vector vertex list supports external children list");
                            }),
                        s.mVertexListType);
                }
                OSS << "}\n";
                oss << "\n";
            }
        }

        visit(
            overload(
                [&](Vector_) {
                    if (s.hasIteratorComponent() || s.isPolymorphic()) {
                        OSS << "// preserve vertex' iterators\n";
                        OSS << "auto& vert = g.vertices[u];\n";
                    }
                },
                [&](auto) {
                    OSS << "auto& vert = *static_cast<" << name << "::Vertex*>(u);\n";
                }),
            s.mVertexListType);

        if (s.isPolymorphic()) {
            std::pmr::string vertName(scratch);
            vertName = "vert";
            OSS << "remove_vertex_value_impl(" << vertName << ".handle, g);\n";
        }

        visit(
            overload(
                [&](Vector_) {
                    for (const auto& c : s.mComponents) {
                        const auto& member = g.getMemberName(c.mMemberName, true);
                        if (!s.isVector()) {
                            auto iterName = getParameterName(c.mMemberName, scratch) + "Iter";
                            OSS << "auto " << iterName << " = vert." << member << "Iter;\n";
                        }
                    }
                    if (s.hasIteratorComponent()) {
                        oss << "\n";
                    }

                    for (const auto& map : s.mVertexMaps) {
                        OSS << "{ // UuidGraph\n";
                        {
                            INDENT();
                            for (const auto& c : s.mComponents) {
                                if (c.mName != map.mComponentName)
                                    continue;

                                const auto& component = g.getMemberName(c.mMemberName, true);
                                OSS << "const auto& key = g." << component << "[u];\n";
                                OSS << "auto num = g." << g.getMemberName(map.mMemberName, true) << ".erase(key);\n";
                                OSS << "CC_ENSURES(num == 1);\n";
                                OSS << "for (auto&& pair : g." << g.getMemberName(map.mMemberName, true) << ") {\n";
                                {
                                    INDENT();
                                    OSS << "auto& v = pair.second;\n";
                                    OSS << "if (v > u) {\n";
                                    OSS << "    --v;\n";
                                    OSS << "}\n";
                                }
                                OSS << "}\n";
                                break;
                            }
                        }
                        OSS << "}\n";
                    }

                    OSS << "impl::removeVectorVertex(const_cast<" << name << "&>(g), u, "
                        << name << "::directed_category{});\n";

                    if (!s.mComponents.empty()) {
                        oss << "\n";
                        OSS << "// remove components\n";
                        for (const auto& c : s.mComponents) {
                            const auto& member = g.getMemberName(c.mMemberName, true);
                            if (s.isVector()) {
                                OSS << "g." << member << ".erase(g." << member << ".begin() + std::ptrdiff_t(u));\n";
                            } else {
                                auto iterName = getParameterPrefix(c.mMemberName, scratch) + "Iter";
                                OSS << "g." << member << ".erase(" << iterName << ");\n";
                            }
                        }
                    }
                },
                [&](auto) {
                    OSS << "g.vertices.erase(vert.mPosition);\n";
                    if (!s.mVertexMaps.empty()) {
                        throw std::runtime_error("only vector vertex list support UuidGraph");
                    }
                }),
            s.mVertexListType);
    }
    OSS << "}\n";

    return oss.str();
}

}

std::pmr::string CppGraphBuilder::generateAddEdge(bool property,
    bool generic, bool variadic, bool reference, bool function) const {

    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto vertID = mStruct.mCurrentVertex;
    auto scratch = get_allocator().resource();
    const auto& cpp = mStruct;
    auto name = cpp.getImplName();

    bool directed = !s.mUndirected;
    bool bidirectional = s.mBidirectional;
    bool edgeProperty = !s.mEdgeProperty.empty();
    bool mutablePropertyGraph = s.mMutableGraphEdge && edgeProperty;

    if (reference) {
        directed = true;
        bidirectional = true;
        edgeProperty = false;
        mutablePropertyGraph = false;
    }

    bool hasProperty = mutablePropertyGraph && edgeProperty && property;
    bool isDirectedOnly = directed && !bidirectional;
    bool isBidirectionalOnly = directed && bidirectional;
    bool isEdgeListNotNeeded = isDirectedOnly || (isBidirectionalOnly && !edgeProperty);

    // precondition, add edge
    if (isEdgeListNotNeeded) {
    } else {
        visit(
            overload(
                [&](Vector_) {
                },
                [&](List_) {
                    OSS << "auto edgeIter = g.edges.emplace(g.edges.end(), u, v";
                    if (hasProperty) {
                        if (generic) {
                            if (variadic) {
                                oss << ", std::forward<T>(args)...";
                            } else {
                                oss << ", std::forward<EdgeProperty>(p)";
                            }
                        } else {
                            oss << ", p";
                        }
                    }
                    oss << ");\n";
                },
                [&]<Associative_ T>(T) {
                    OSS << "auto [edgeIter, edgeInserted] = g.edges.emplace(u, v";
                    if (hasProperty) {
                        if (generic) {
                            if (variadic) {
                                oss << ", std::forward<T>(args)...";
                            } else {
                                oss << ", std::forward<EdgeProperty>(p)";
                            }
                        } else {
                            oss << ", p";
                        }
                    }
                    oss << ");\n";
                    OSS << "CC_ENSURES(edgeInserted);\n";
                }),
            s.mEdgeListType);
    }

    // try insert out edge
    std::pmr::string res(scratch);
    if (isEdgeListNotNeeded) {
        if (isDirectedOnly) {
            visit(
                overload(
                    [&]<Sequence_ T>(T) {
                        res = "value";
                    },
                    [&]<MultipleAssociative_ T>(T) {
                        res = "iter";
                    },
                    [&](auto) {
                        res = "[iter, inserted]";
                    }),
                s.mOutEdgeListType);
        } else {
            res = "";
        }
    } else {
        visit(
            overload(
                [](Vector_) {
                    // do nothing
                },
                [](List_) {
                    // do nothing
                },
                [&]<MultipleAssociative_ T>(T) {
                    res = "iter";
                },
                [&](auto) {
                    res = "[iter, inserted]";
                }),
            s.mOutEdgeListType);
    }

    std::pmr::string out_edge_list(scratch);
    std::pmr::string in_edge_list(scratch);
    std::pmr::string edge_descriptor(scratch);

    if (reference) {
        out_edge_list = "getChildrenList";
        in_edge_list = "getParentsList";
        edge_descriptor = "ownership_descriptor";
    } else {
        out_edge_list = "getOutEdgeList";
        in_edge_list = "getInEdgeList";
        edge_descriptor = "edge_descriptor";
    }

    OSS << "auto& outEdgeList = g." << out_edge_list << "(u);\n";
    copyString(oss, space, pushIncidenceList(
        s, "outEdgeList", "v", res, isEdgeListNotNeeded, scratch));

    bool needInserted = false;
    if (!isEdgeListNotNeeded) {
        visit(
            overload(
                  [&](Vector_) {
                  },
                  [&](List_) {
                  },
                  [&]<UniqueAssociative_ T>(T) {
                      needInserted = true;
                  },
                  [&]<MultipleAssociative_ T>(T) {
                  }),
            s.mOutEdgeListType);
    }

    if (needInserted) {
        oss << "\n";
        OSS << "if (inserted) {\n";
    }

    INDENT();
    if (!needInserted) {
        UNINDENT();
    }
    {
        // add reciprocal edges
        if (!directed) {
            OSS << "auto& outEdgeListR = g." << out_edge_list << "(v);\n";
            copyString(oss, space, pushIncidenceList(
                s, "outEdgeListR", "u", "", isEdgeListNotNeeded, scratch));
        } else if (bidirectional) {
            if (!needInserted) {
                oss << "\n";
            }
            OSS << "auto& inEdgeList = g." << in_edge_list << "(v);\n";
            copyString(oss, space, pushIncidenceList(
                s, "inEdgeList", "u", "", isEdgeListNotNeeded, scratch));
        }

        // postcondition, add edge
        if (!isEdgeListNotNeeded) {
            visit(
                overload(
                    [&](Vector_) {
                        oss << "\n";
                        OSS << "g.edges.emplace_back(u, v";
                        if (hasProperty) {
                            if (generic) {
                                if (variadic) {
                                    oss << ", std::forward<T>(args)...";
                                } else {
                                    oss << ", std::forward<EdgeProperty>(p)";
                                }
                            } else {
                                oss << ", p";
                            }
                        }
                        oss << ");\n";
                    },
                    [&](auto) {}),
                s.mEdgeListType);
        }

        if (function) {
            // return
            oss << "\n";
            if (isDirectedOnly) {
                if (edgeProperty) {
                    visit(
                        overload(
                            [&]<BackInsertionSequence_ T>(T) {
                                OSS << "return std::make_pair(" << name << "::" << edge_descriptor << "(u, v, &value.get_property()), true);\n";
                            },
                            [&]<Associative_ T>(T) {
                                OSS << "return std::make_pair(" << name << "::" << edge_descriptor << "(u, v, &iter->get_property()), true);\n";
                            }),
                        s.mOutEdgeListType);
                } else {
                    OSS << "return std::make_pair(" << name << "::" << edge_descriptor << "(u, v), true);\n";
                }
            } else if (!directed) {
                visit(
                    overload(
                        [&](Vector_) {
                            OSS << "return std::make_pair(" << name << "::" << edge_descriptor << "(u, v, &g.edges.back().get_property()), true);\n";
                        },
                        [&](auto) {
                            OSS << "return std::make_pair(" << name << "::" << edge_descriptor << "(u, v, &edgeIter->get_property()), true);\n";
                        }),
                    s.mEdgeListType);
            } else {
                Expects(bidirectional);
                if (edgeProperty) {
                    visit(
                        overload(
                            [&](Vector_) {
                                OSS << "return std::make_pair(" << name << "::" << edge_descriptor << "(u, v, &g.edges.back().get_property()), true);\n";
                            },
                            [&](auto) {
                                OSS << "return std::make_pair(" << name << "::" << edge_descriptor << "(u, v, &edgeIter->get_property()), true);\n";
                            }),
                        s.mEdgeListType);
                } else {
                    OSS << "return std::make_pair(" << name << "::" << edge_descriptor << "(u, v), true);\n";
                }
            }
        }
    }

    if (needInserted) {
        UNINDENT();
        OSS << "} else {\n";
        INDENT();

        // postcondition, remove edge
        if (!isEdgeListNotNeeded) {
            visit(
                overload(
                    [&](Vector_) {
                    },
                    [&](auto) {
                        OSS << "g.edges.erase(edgeIter);\n";
                    }),
                s.mEdgeListType);
        }

        if (function) {
            // return
            if (isEdgeListNotNeeded) {
                oss << "\n";
                OSS << "return std::make_pair(" << name << "::" << edge_descriptor << "(u, v), false);\n";
            } else {
                oss << "\n";
                visit(
                    overload(
                        [&](Vector_) {
                            OSS << "return std::make_pair(" << name << "::" << edge_descriptor << "(u, v, &g.edges.back().get_property()), true);\n";
                        },
                        [&](auto) {
                            OSS << "return std::make_pair(" << name << "::" << edge_descriptor << "(u, v, &iter->get_iter()->get_property()), false);\n";
                        }),
                    s.mEdgeListType);
            }
        }
        UNINDENT();
        OSS << "}\n";
    }
    return oss.str();
}

std::pmr::string CppGraphBuilder::generateRemoveEdges(bool bInline) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto vertID = mStruct.mCurrentVertex;
    auto scratch = get_allocator().resource();
    const auto& cpp = mStruct;

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateRemoveEdge(bool bInline) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto vertID = mStruct.mCurrentVertex;
    auto scratch = get_allocator().resource();
    const auto& cpp = mStruct;

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateRemoveEdgeIter(bool bInline) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto vertID = mStruct.mCurrentVertex;
    auto scratch = get_allocator().resource();
    const auto& cpp = mStruct;

    return oss.str();
}

std::pmr::string CppGraphBuilder::addVertex(bool propertyParam, bool piecewise, bool cpp14) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto vertID = mStruct.mCurrentVertex;
    auto scratch = get_allocator().resource();
    const auto& cpp = mStruct;
    const auto* ptr = &s;
    auto name = cpp.getImplName();
    auto ns = mStruct.mCurrentNamespace;

    if (cpp14 && s.isPolymorphic()) {
        for (const auto& c : s.mPolymorphic.mConcepts) {
            auto tagType = cpp.getDependentName(c.mTag);
            auto valueType = cpp.getDependentName(c.mValue);

            if (piecewise) {
                OSS << "template <class Tuple>\n";
                OSS << "void addVertexImpl("
                    << tagType << " /*tag*/, Tuple &&val, "
                    << name << " &g, " << name << "::Vertex &vert) {\n";

                {
                    INDENT();
                    OSS << "invoke_hpp::apply(\n";
                    {
                        INDENT();
                        OSS << "[&](auto&&... args) {\n";
                        {
                            INDENT();
                            if (c.isVector()) {
                                OSS << "vert.handle = " << handleElemType(c, ns) << "{";
                                if (c.isIntrusive()) {
                                    oss << " std::forward<decltype(args)>(args)...";
                                } else {
                                    oss << "\n";
                                    OSS << "    gsl::narrow_cast<"
                                        << name << "::vertex_descriptor>(g." << g.getMemberName(c.mMemberName, true) << ".size())";
                                }
                                oss << "};\n";

                                if (!c.isIntrusive()) {
                                    OSS << "g." << g.getMemberName(c.mMemberName, true) << ".emplace_back(std::forward<decltype(args)>(args)...);\n";
                                }
                            } else {
                                if (!c.isIntrusive()) {
                                    OSS << "vert.handle = " << handleElemType(c, ns) << "{ g." << g.getMemberName(c.mMemberName, true)
                                        << ".emplace(g." << g.getMemberName(c.mMemberName, true) << ".end(), std::forward<decltype(args)>(args)...) };\n";
                                }
                            }
                        }
                        OSS << "},\n";
                        OSS << "std::forward<Tuple>(val));\n";
                    }
                }
                OSS << "}\n";
            } else {
                OSS << "template <class ValueT>\n";
                OSS << "void addVertexImpl( // NOLINT\n";
                OSS << "    ValueT &&val, " << name << " &g, " << name << "::Vertex &vert, // NOLINT\n";
                OSS << "    std::enable_if_t<std::is_same<std::decay_t<ValueT>, " << valueType << ">::value>* dummy = nullptr) { // NOLINT\n";
                {
                    INDENT();
                    if (c.isVector()) {
                        OSS << "vert.handle = " << handleElemType(c, ns) << "{";
                        if (c.isIntrusive()) {
                            if (propertyParam) {
                                oss << " std::forward<ValueT>(val)";
                            }
                        } else {
                            oss << "\n";
                            OSS << "    gsl::narrow_cast<"
                                << name << "::vertex_descriptor>(g." << g.getMemberName(c.mMemberName, true) << ".size())";
                        }
                        oss << "};\n";

                        if (!c.isIntrusive()) {
                            OSS << "g." << g.getMemberName(c.mMemberName, true) << ".emplace_back(";
                            if (propertyParam) {
                                oss << "std::forward<ValueT>(val)";
                            }
                            oss << ");\n";
                        }
                    } else {
                        if (!c.isIntrusive()) {
                            OSS << "vert.handle = " << handleElemType(c, ns) << "{ g." << g.getMemberName(c.mMemberName, true)
                                << ".emplace(g." << g.getMemberName(c.mMemberName, true) << ".end(), ";
                            if (propertyParam) {
                                oss << "std::forward<ValueT>(val)";
                            }
                            oss << ") };\n";
                        }
                    }
                }
                OSS << "}\n";
            }
            oss << "\n";
        }
    }

    if (propertyParam && (s.hasVertexProperty()
        || !s.mComponents.empty()
        || s.isPolymorphic())) {
        int count = 0;
        OSS << "template <";
        if (s.hasVertexProperty()) {
            oss << (count++ == 0 ? "" : ", ") << "class VertexProperty";
        }
        for (int id = 0; const auto& c : s.mComponents) {
            oss << (count++ == 0 ? "" : ", ") << "class Component" << id++;
        }
        if (s.isPolymorphic()) {
            if (piecewise) {
                oss << (count++ == 0 ? "" : ", ") << "class Tag";
            }
            oss << (count++ == 0 ? "" : ", ") << "class ValueT";
        }
        oss << ">\n";
    } else if (!propertyParam) {
        int count = 0;
        if (s.isPolymorphic()) {
            OSS << "template <";
            oss << (count++ == 0 ? "" : ", ") << "class Tag";
            oss << ">\n";
        }
    }

    OSS << "inline " << name << "::vertex_descriptor\n";
    OSS << "addVertex(";
    if (piecewise) {
        if (s.isPolymorphic()) {
            oss << "Tag tag, ";
        } else {
            oss << "std::piecewise_construct_t /*tag*/, ";
        }
    } else {
        if (s.isPolymorphic() && !propertyParam) {
            oss << "Tag tag, ";
        }
    }
    {
        int count = 0;
        if (propertyParam) {
            if (s.hasVertexProperty()) {
                oss << (count++ == 0 ? "" : ", ") << "VertexProperty&& p";
            }
            for (int id = 0; const auto& c : s.mComponents) {
                oss << (count++ == 0 ? "" : ", ") << "Component" << id << "&& c" << id;
                ++id;
            }
        }

        if (s.isPolymorphic()) {
            if (propertyParam) {
                if (piecewise) {
                    oss << (count++ == 0 ? "" : ", ") << "ValueT&& val";
                } else {
                    oss << (count++ == 0 ? "" : ", ") << "ValueT&& val";
                }
            }
        }

        oss << (count++ == 0 ? "" : ", ") << name << "& g";

        if (s.mReferenceGraph) {
            oss << ", " << name << "::vertex_descriptor u = " << name << "::null_vertex()";
        }
    }
    oss << ") {\n";
    {
        INDENT();
        if (s.isVector()) {
            OSS << "auto v = gsl::narrow_cast<" << name << "::vertex_descriptor>(g.vertices.size());\n";
            if (s.mReferenceGraph) {
                if (!s.isAliasGraph()) {
                    oss << "\n";
                    OSS << "g.objects.emplace_back();\n";
                }
            }
            oss << "\n";
            if (piecewise) {
                if (s.hasVertexProperty()) {
                    OSS << "invoke_hpp::apply(\n";
                    {
                        INDENT();
                        OSS << "[&](auto&&... args) {\n";
                        OSS << "    g.vertices.emplace_back(std::forward<decltype(args)>(args)...);\n";
                        OSS << "},\n";
                        OSS << "std::forward<VertexProperty>(p));\n";
                    }
                } else {
                    OSS << "g.vertices.emplace_back();\n";
                }
            } else {
                OSS << "g.vertices.emplace_back(";
                if (s.hasVertexProperty()) {
                    if (propertyParam) {
                        oss << "std::forward<VertexProperty>(p)";
                    }
                }
                oss << ");\n";
            }

            if (s.hasIteratorComponent() || s.isPolymorphic()) {
                OSS << "auto& vert = g.vertices.back();\n";
            }
        } else {
            if (!s.mComponents.empty()) {
                throw std::runtime_error("non vector components not supported yet");
            }
            if (s.mAddressable) {
                if (!s.isAliasGraph()) {
                    throw std::invalid_argument("only vector vertex list support external children list");
                }
            }
            if (piecewise) {
                if (s.hasVertexProperty()) {
                    OSS << "invoke_hpp::apply(\n";
                    {
                        INDENT();
                        OSS << "[&](auto&&... args) {\n";
                        OSS << "    auto iter = g.vertices.emplace(g.vertices.end(), std::forward<decltype(args)>(args)...);\n";
                        OSS << "    iter->mPosition = iter;\n";
                        OSS << "},\n";
                        OSS << "std::forward<VertexProperty>(p));\n";
                    }
                } else {
                    OSS << "auto iter = g.vertices.emplace(g.vertices.end());\n";
                    OSS << "iter->mPosition = iter;\n";
                }
            } else {
                OSS << "auto iter = g.vertices.emplace(g.vertices.end()";
                if (propertyParam && s.hasVertexProperty()) {
                    oss << ", std::forward<VertexProperty>(p)";
                }
                oss << ");\n";
                OSS << "iter->mPosition = iter;\n";
            }

            if (s.isPolymorphic()) {
                OSS << "auto& vert = *iter;\n";
            }
        }

        for (const auto& map : s.mVertexMaps) {
            oss << "\n";
            OSS << "{ // UuidGraph\n";
            INDENT();
            if (map.isBimap()) {
                Expects(!map.isComponentMember());
                int componentID = -1;
                for (int i = 0; i != s.mComponents.size(); ++i) {
                    if (s.mComponents[i].mName == map.mComponentName) {
                        componentID = i;
                        break;
                    }
                }
                Expects(componentID != -1);

                const auto& mapMemberName = g.getMemberName(map.mMemberName, true);
                if (piecewise) {
                    if (true) {
                        OSS << "invoke_hpp::apply(\n";
                        {
                            INDENT();
                            OSS << "[&](const auto&... args) {\n";
                            visit(
                                overload(
                                    [&](Vector_) {
                                        OSS << "    auto res = g." << mapMemberName << ".emplace(std::piecewise_construct, std::forward_as_tuple(args...), std::forward_as_tuple(v));\n";
                                    },
                                    [&]<BackInsertionSequence_ T0>(T0) {
                                        OSS << "    auto res = g." << mapMemberName << ".emplace(std::piecewise_construct, std::forward_as_tuple(args...), std::forward_as_tuple(&(*iter)));\n";
                                    },
                                    [&]<Associative_ T1>(T1) {
                                        OSS << "    auto res = g." << mapMemberName << ".emplace(std::piecewise_construct, std::forward_as_tuple(args...), std::forward_as_tuple(&(*iter)));\n";
                                    }),
                                s.mVertexListType);
                            OSS << "    CC_ENSURES(res.second);\n";
                            OSS << "},\n";
                            OSS << "c" << componentID << ");\n";
                        }
                    } else {
                        OSS << "const auto& uuid = c" << componentID << ";\n";
                        visit(
                            overload(
                                [&](Vector_) {
                                    OSS << "auto res = g." << mapMemberName << ".emplace(std::piecewise_construct, uuid, std::forward_as_tuple(v));\n";
                                },
                                [&]<BackInsertionSequence_ T0>(T0) {
                                    OSS << "auto res = g." << mapMemberName << ".emplace(std::piecewise_construct, uuid, std::forward_as_tuple(&(*iter)));\n";
                                },
                                [&]<Associative_ T1>(T1) {
                                    OSS << "auto res = g." << mapMemberName << ".emplace(std::piecewise_construct, uuid, std::forward_as_tuple(&(*iter)));\n";
                                }),
                            s.mVertexListType);
                    }
                } else {
                    OSS << "const auto& uuid = c" << componentID << ";\n";
                    visit(
                        overload(
                            [&](Vector_) {
                                OSS << "auto res = g." << mapMemberName << ".emplace(uuid, v);\n";
                            },
                            [&]<BackInsertionSequence_ U0>(U0) {
                                OSS << "auto res = g." << mapMemberName << ".emplace(uuid, &(*iter));\n";
                            },
                            [&]<Associative_ U1>(U1) {
                                OSS << "auto res = g." << mapMemberName << ".emplace(uuid, &(*iter));\n";
                            }),
                        s.mVertexListType);
                    OSS << "CC_ENSURES(res.second);\n";
                }
            }
            UNINDENT();
            OSS << "}\n";
        }

        visit(
            overload(
                [&](Vector_) {
                    for (int id = 0; const auto& c : s.mComponents) {
                        const auto& member = g.getMemberName(c.mMemberName, true);
                        if (piecewise) {
                            oss << "\n";
                            OSS << "invoke_hpp::apply(\n";
                            {
                                INDENT();
                                OSS << "[&](auto&&... args) {\n";
                                OSS << "    g." << member << ".emplace_back(std::forward<decltype(args)>(args)...);\n";
                                OSS << "},\n";
                                OSS << "std::forward<Component" << id << ">(c" << id << "));\n";
                            }
                        } else {
                            OSS << "g." << member << ".emplace_back(";
                            if (propertyParam) {
                                oss << "std::forward<Component" << id << ">(c" << id << ")";
                            }
                            oss << ");\n";
                        }
                        ++id;
                    }
                },
                [&]<BackInsertionSequence_ T>(T) {
                    if (!s.mComponents.empty()) {
                        throw std::runtime_error("non vector components not supported yet");
                    }
                },
                [&]<Associative_ T>(T) {
                    if (!s.mComponents.empty()) {
                        throw std::runtime_error("non vector components not supported yet");
                    }
                }),
            s.mVertexListType);

        if (s.isPolymorphic()) {
            if (cpp14) {
                oss << "\n";
                OSS << "// PolymorphicGraph\n";
                OSS << "// if no matching overloaded function is found, Type is not supported by PolymorphicGraph\n";
                if (propertyParam && piecewise) {
                    OSS << "addVertexImpl(tag, std::forward<ValueT>(val), g, vert);\n";
                } else {
                    OSS << "addVertexImpl(std::forward<ValueT>(val), g, vert);\n";
                }
            } else {
                oss << "\n";
                OSS << "// PolymorphicGraph\n";
                if (propertyParam && piecewise) {
                    int count = 0;
                    for (const auto& c : s.mPolymorphic.mConcepts) {
                        if (count++)
                            oss << " else ";
                        else
                            OSS;

                        oss << "if constexpr (std::is_same_v<cc::remove_cvref_t<Tag>, "
                            << cpp.getDependentName(c.mTag) << ">) {\n";
                        {
                            INDENT();
                            OSS << "invoke_hpp::apply(\n";
                            {
                                INDENT();
                                OSS << "[&](auto&&... args) {\n";
                                {
                                    INDENT();
                                    if (c.isVector()) {
                                        OSS << "vert.handle = " << handleElemType(c, ns) << "{";
                                        if (c.isIntrusive()) {
                                            oss << " std::forward<decltype(args)>(args)...";
                                        } else {
                                            oss << "\n";
                                            OSS << "    gsl::narrow_cast<"
                                                << name << "::vertex_descriptor>(g." << g.getMemberName(c.mMemberName, true) << ".size())";
                                        }
                                        oss << "};\n";

                                        if (!c.isIntrusive()) {
                                            OSS << "g." << g.getMemberName(c.mMemberName, true) << ".emplace_back(std::forward<decltype(args)>(args)...);\n";
                                        }
                                    } else {
                                        if (!c.isIntrusive()) {
                                            OSS << "vert.handle = " << handleElemType(c, ns) << "{ g." << g.getMemberName(c.mMemberName, true)
                                                << ".emplace(g." << g.getMemberName(c.mMemberName, true) << ".end(), std::forward<decltype(args)>(args)...) };\n";
                                        }
                                    }
                                }
                                OSS << "},\n";
                                OSS << "std::forward<ValueT>(val));\n";
                            }
                        }
                        OSS << "}";
                    }
                    oss << " else {\n";
                    OSS << "    []<bool flag = false>() {\n";
                    OSS << "        static_assert(flag, \"value not found in graph\");\n";
                    OSS << "    }\n";
                    OSS << "    ();\n";
                    OSS << "}\n";
                } else { // no piecewise
                    int count = 0;
                    for (const auto& c : s.mPolymorphic.mConcepts) {
                        if (count++)
                            oss << " else ";
                        else
                            OSS;

                        if (propertyParam) {
                            oss << "if constexpr (std::is_same_v<cc::remove_cvref_t<ValueT>, "
                                << cpp.getDependentName(c.mValue) << ">) {\n";
                        } else {
                            oss << "if constexpr (std::is_same_v<cc::remove_cvref_t<Tag>, "
                                << cpp.getDependentName(c.mTag) << ">) {\n";
                        }
                        {
                            INDENT();
                            if (c.isVector()) {
                                OSS << "vert.handle = " << handleElemType(c, ns) << "{";
                                if (c.isIntrusive()) {
                                    if (propertyParam) {
                                        oss << " std::forward<ValueT>(val)";
                                    }
                                } else {
                                    oss << "\n";
                                    OSS << "    gsl::narrow_cast<"
                                        << name << "::vertex_descriptor>(g." << g.getMemberName(c.mMemberName, true) << ".size())";
                                }
                                oss << "};\n";

                                if (!c.isIntrusive()) {
                                    OSS << "g." << g.getMemberName(c.mMemberName, true) << ".emplace_back(";
                                    if (propertyParam) {
                                        oss << "std::forward<ValueT>(val)";
                                    }
                                    oss << ");\n";
                                }
                            } else {
                                if (!c.isIntrusive()) {
                                    OSS << "vert.handle = " << handleElemType(c, ns) << "{ g." << g.getMemberName(c.mMemberName, true)
                                        << ".emplace(g." << g.getMemberName(c.mMemberName, true) << ".end(), ";
                                    if (propertyParam) {
                                        oss << "std::forward<ValueT>(val)";
                                    }
                                    oss << ") };\n";
                                }
                            }
                        }
                        OSS << "}";
                    }
                    oss << " else {\n";
                    OSS << "    []<bool flag = false>() {\n";
                    OSS << "        static_assert(flag, \"value not found in graph\");\n";
                    OSS << "    }\n";
                    OSS << "    ();\n";
                    OSS << "}\n";
                } // piecewise
            }
        } // PolymorphicGraph

        if (s.mAddressable) {
            oss << "\n";
            OSS << "// AddressableGraph\n";
            OSS << "addPathImpl(u, v, g);\n";
        }

        visit(
            overload(
                [&](Vector_) {
                    oss << "\n";
                    OSS << "return v;\n";
                },
                [&]<BackInsertionSequence_ T>(T) {
                    oss << "\n";
                    OSS << "return &(*iter);\n";
                },
                [&]<Associative_ T>(T) {
                    oss << "\n";
                    OSS << "return &(*iter);\n";
                }),
            s.mVertexListType);
    }
    OSS << "}\n";

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateAddressableGraph(bool bInline) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto* ptr = &s;
    const auto vertID = mStruct.mCurrentVertex;
    auto scratch = get_allocator().resource();
    const auto& cpp = mStruct;

    auto name = cpp.getImplName();
    auto ns = mStruct.mCurrentNamespace;

     // IncidenceGraph
    oss << "\n";
    OSS << "// AddressableGraph\n";
    OSS << "inline " << name << "::vertex_descriptor\n";
    OSS << "parent(const " << name << "::ownership_descriptor& e, const " << name << "& /*g*/) noexcept {\n";
    OSS << "    return e.source;\n";
    OSS << "}\n";

    oss << "\n";
    OSS << "inline " << name << "::vertex_descriptor\n";
    OSS << "child(const " << name << "::ownership_descriptor& e, const " << name << "& /*g*/) noexcept {\n";
    OSS << "    return e.target;\n";
    OSS << "}\n";

    oss << "\n";
    OSS << "inline std::pair<" << name << "::children_iterator, " << name << "::children_iterator>\n";
    OSS << "children(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept {\n";
    OSS << "    return std::make_pair(\n";
    OSS << "        " << name << "::children_iterator(const_cast<" << name << "&>(g).getChildrenList(u).begin(), u),\n";
    OSS << "        " << name << "::children_iterator(const_cast<" << name << "&>(g).getChildrenList(u).end(), u));\n";
    OSS << "}\n";

    oss << "\n";
    OSS << "inline " << name << "::children_size_type\n";
    OSS << "numChildren(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept {\n";
    OSS << "    return gsl::narrow_cast<" << name << "::children_size_type>(g.getChildrenList(u).size());\n";
    OSS << "}\n";

    oss << "\n";
    OSS << "inline std::pair<" << name << "::ownership_descriptor, bool>\n";
    OSS << "reference(" << name << "::vertex_descriptor u, " << name << "::vertex_descriptor v, " << name << "& g) noexcept {\n";
    {
        INDENT();
        OSS << "auto& outEdgeList = g.getChildrenList(u);\n";
        visit(
            overload(
                [&]<BackInsertionSequence_ T>(T) {
                    OSS << "auto  iter        = std::find(outEdgeList.begin(), outEdgeList.end(), "
                        << name << "::OutEdge(v));\n";
                },
                [&]<Associative_ U>(U) {
                    OSS << "auto  iter        = outEdgeList.find("
                        << name << "::OutEdge(v));\n";
                }),
            s.mOutEdgeListType);
        OSS << "bool  hasEdge     = (iter != outEdgeList.end());\n";
        OSS << "return {" << name << "::ownership_descriptor(u, v), hasEdge};\n";
    }
    OSS << "}\n";

    // BidirectionalGraph
    oss << "\n";
    OSS << "inline std::pair<" << name << "::parent_iterator, " << name << "::parent_iterator>\n";
    OSS << "parents(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept {\n";
    OSS << "    return std::make_pair(\n";
    OSS << "        " << name << "::parent_iterator(const_cast<" << name << "&>(g).getParentsList(u).begin(), u),\n";
    oss << "        " << name << "::parent_iterator(const_cast<" << name << "&>(g).getParentsList(u).end(), u));\n";
    OSS << "}\n";

    oss << "\n";
    OSS << "inline " << name << "::children_size_type\n";
    OSS << "numParents(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept {\n";
    OSS << "    return gsl::narrow_cast<" << name << "::children_size_type>(g.getParentsList(u).size());\n";
    OSS << "}\n";

    oss << "\n";
    OSS << "inline " << name << "::vertex_descriptor\n";
    OSS << "parent(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept {\n";
    OSS << "    auto r = parents(u, g);\n";
    OSS << "    if (r.first == r.second) {\n";
    OSS << "        return " << name << "::null_vertex();\n";
    OSS << "    }\n";
    OSS << "    return parent(*r.first, g);\n";
    OSS << "}\n";

    oss << "\n";
    OSS << "inline bool\n";
    OSS << "ancestor(" << name << "::vertex_descriptor u, " << name
        << "::vertex_descriptor v, const " << name << "& g) noexcept {\n";
    {
        INDENT();
        copyString(oss, space, std::string(R"(CC_EXPECTS(u != v);
bool isAncestor = false;
auto r          = parents(v, g);
while (r.first != r.second) {
    v = parent(*r.first, g);
    if (u == v) {
        isAncestor = true;
        break;
    }
    r = parents(v, g);
}
return isAncestor;
)"));
    }
    OSS << "}\n";

    // EdgeListGraph
    oss << "\n";
    OSS << "inline std::pair<" << name << "::ownership_iterator, " << name << "::ownership_iterator>\n";
    OSS << "references(const " << name << "& g0) noexcept {\n";
    OSS << "    auto& g = const_cast<" << name << "&>(g0);\n";
    OSS << "    return std::make_pair(\n";
    OSS << "        " << name << "::ownership_iterator(g.getVertexList().begin(), g.getVertexList().begin(), g.getVertexList().end(), g),\n";
    OSS << "        " << name << "::ownership_iterator(g.getVertexList().begin(), g.getVertexList().end(), g.getVertexList().end(), g));\n";
    OSS << "}\n";

    oss << "\n";
    OSS << "inline " << name << "::ownerships_size_type\n";
    OSS << "numReferences(const " << name << "& g) noexcept {\n";
    {
        INDENT();
        OSS << name << "::ownerships_size_type numEdges = 0;\n";
        OSS << "auto" << std::string(std::max(0, int(name.size() + 22) - 4), ' ') << " range    = vertices(g);\n";
        OSS << "for (auto iter = range.first; iter != range.second; ++iter) {\n";
        OSS << "    numEdges += numChildren(*iter, g);\n";
        OSS << "}\n";
        OSS << "return numEdges;\n";
    }
    OSS << "}\n";

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateGraphFunctions_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto* ptr = &s;
    const auto vertID = mStruct.mCurrentVertex;
    auto scratch = get_allocator().resource();
    const auto& cpp = mStruct;

    auto name = cpp.getImplName();
    auto ns = mStruct.mCurrentNamespace;
    const auto cn = mStruct.mCurrentNamespace;

    if (s.mIncidence) {
        oss << "\n";
        OSS << "// IncidenceGraph\n";
        OSS << "inline " << name << "::vertex_descriptor\n";
        OSS << "source(const " << name << "::edge_descriptor& e, const " << name << "& /*g*/) noexcept {\n";
        OSS << "    return e.source;\n";
        OSS << "}\n";

        oss << "\n";
        OSS << "inline " << name << "::vertex_descriptor\n";
        OSS << "target(const " << name << "::edge_descriptor& e, const " << name << "& /*g*/) noexcept {\n";
        OSS << "    return e.target;\n";
        OSS << "}\n";

        oss << "\n";
        OSS << "inline std::pair<" << name << "::out_edge_iterator, " << name << "::out_edge_iterator>\n";
        OSS << "out_edges(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept { // NOLINT\n";
        OSS << "    return std::make_pair(\n";
        OSS << "        " << name << "::out_edge_iterator(const_cast<" << name << "&>(g).getOutEdgeList(u).begin(), u),\n";
        OSS << "        " << name << "::out_edge_iterator(const_cast<" << name << "&>(g).getOutEdgeList(u).end(), u));\n";
        OSS << "}\n";

        oss << "\n";
        OSS << "inline " << name << "::degree_size_type\n";
        OSS << "out_degree(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept { // NOLINT\n";
        OSS << "    return gsl::narrow_cast<" << name << "::degree_size_type>(g.getOutEdgeList(u).size());\n";
        OSS << "}\n";

        oss << "\n";
        OSS << "inline std::pair<" << name << "::edge_descriptor, bool>\n";
        OSS << "edge(" << name << "::vertex_descriptor u, " << name << "::vertex_descriptor v, const " << name << "& g) noexcept {\n";
        {
            INDENT();
            OSS << "const auto& outEdgeList = g.getOutEdgeList(u);\n";
            visit(
                overload(
                    [&]<BackInsertionSequence_ T>(T) {
                        OSS << "auto  iter        = std::find(outEdgeList.begin(), outEdgeList.end(), "
                            << name << "::OutEdge(v));\n";
                    },
                    [&]<Associative_ U>(U) {
                        OSS << "auto  iter        = outEdgeList.find("
                            << name << "::OutEdge(v));\n";
                    }),
                s.mOutEdgeListType);
            OSS << "bool  hasEdge     = (iter != outEdgeList.end());\n";
            if (s.hasEdgeProperty() || s.needEdgeList()) {
                OSS << "return {" << name << "::edge_descriptor(u, v, (hasEdge ? &(*iter).get_property() : nullptr)), hasEdge};\n";
            } else {
                OSS << "return {" << name << "::edge_descriptor(u, v), hasEdge};\n";
            }
        }
        OSS << "}\n";

        if (s.mUndirected) {
            oss << "\n";
            OSS << "// BidirectionalGraph(Undirected)\n";
            OSS << "inline std::pair<" << name << "::in_edge_iterator, " << name << "::in_edge_iterator>\n";
            OSS << "in_edges(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept { // NOLINT\n";
            OSS << "    return std::make_pair(\n";
            OSS << "        " << name << "::in_edge_iterator(const_cast<" << name << "&>(g).getOutEdgeList(u).begin(), u),\n";
            OSS << "        " << name << "::in_edge_iterator(const_cast<" << name << "&>(g).getOutEdgeList(u).end(), u));\n";
            OSS << "}\n";

            oss << "\n";
            OSS << "inline " << name << "::degree_size_type\n";
            OSS << "in_degree(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept { // NOLINT\n";
            OSS << "    return out_degree(u, g);\n";
            OSS << "}\n";

            oss << "\n";
            OSS << "inline " << name << "::degree_size_type\n";
            OSS << "degree(" << name << "::vertex_descriptor u, const " << name << " & g) noexcept {\n";
            OSS << "    return out_degree(u, g);\n";
            OSS << "}\n";
        } else if (s.mBidirectional) {
            oss << "\n";
            OSS << "// BidirectionalGraph(Directed)\n";
            OSS << "inline std::pair<" << name << "::in_edge_iterator, " << name << "::in_edge_iterator>\n";
            OSS << "in_edges(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept { // NOLINT\n";
            OSS << "    return std::make_pair(\n";
            OSS << "        " << name << "::in_edge_iterator(const_cast<" << name << "&>(g).getInEdgeList(u).begin(), u),\n";
            oss << "        " << name << "::in_edge_iterator(const_cast<" << name << "&>(g).getInEdgeList(u).end(), u));\n";
            OSS << "}\n";

            oss << "\n";
            OSS << "inline " << name << "::degree_size_type\n";
            OSS << "in_degree(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept { // NOLINT\n";
            OSS << "    return gsl::narrow_cast<" << name << "::degree_size_type>(g.getInEdgeList(u).size());\n";
            OSS << "}\n";
            oss << "\n";
            OSS << "inline " << name << "::degree_size_type\n";
            OSS << "degree(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept {\n";
            OSS << "    return in_degree(u, g) + out_degree(u, g);\n";
            OSS << "}\n";
        }
    }

    if (s.mAdjacency) {
        oss << "\n";
        OSS << "// AdjacencyGraph\n";
        OSS << "inline std::pair<" << name << "::adjacency_iterator, " << name << "::adjacency_iterator>\n";
        OSS << "adjacent_vertices(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept { // NOLINT\n";
        {
            INDENT();
            OSS << "auto edges = out_edges(u, g);\n";
            OSS << "return std::make_pair(" << name << "::adjacency_iterator(edges.first, &g), "
                << name << "::adjacency_iterator(edges.second, &g));\n";
        }
        OSS << "}\n";
    }

    if (s.mVertexList) {
        oss << "\n";
        OSS << "// VertexListGraph\n";
        OSS << "inline std::pair<" << name << "::vertex_iterator, " << name << "::vertex_iterator>\n";
        OSS << "vertices(const " << name << "& g) noexcept {\n";
        OSS << "    return std::make_pair(const_cast<" << name << "&>(g).getVertexList().begin(), const_cast<" << name << "&>(g).getVertexList().end());\n";
        OSS << "}\n";

        oss << "\n";
        OSS << "inline " << name << "::vertices_size_type\n";
        OSS << "num_vertices(const " << name << "& g) noexcept { // NOLINT\n";
        OSS << "    return gsl::narrow_cast<" << name << "::vertices_size_type>(g.getVertexList().size());\n";
        OSS << "}\n";
    }

    if (s.mEdgeList) {
        oss << "\n";
        OSS << "// EdgeListGraph\n";
        if (!s.needEdgeList()) {
            OSS << "inline std::pair<" << name << "::edge_iterator, " << name << "::edge_iterator>\n";
            OSS << "edges(const " << name << "& g0) noexcept {\n";
            OSS << "    auto& g = const_cast<" << name << "&>(g0);\n";
            OSS << "    return std::make_pair(\n";
            OSS << "        " << name << "::edge_iterator(g.getVertexList().begin(), g.getVertexList().begin(), g.getVertexList().end(), g),\n";
            OSS << "        " << name << "::edge_iterator(g.getVertexList().begin(), g.getVertexList().end(), g.getVertexList().end(), g));\n";
            OSS << "}\n";
        } else {
            OSS << "inline std::pair<" << name << "::edge_iterator, " << name << "::edge_iterator>\n";
            OSS << "edges(const " << name << "& g) noexcept {\n";
            OSS << "    return std::make_pair(\n";
            OSS << "        " << name << "::edge_iterator(const_cast<" << name << "&>(g).edges.begin()),\n";
            OSS << "        " << name << "::edge_iterator(const_cast<" << name << "&>(g).edges.end()));\n";
            OSS << "}\n";
        }

        oss << "\n";
        if (!s.needEdgeList()) {
            OSS << "inline " << name << "::edges_size_type\n";
            OSS << "num_edges(const " << name << "& g) noexcept { // NOLINT\n";
            {
                INDENT();
                OSS << name << "::edges_size_type numEdges = 0;\n";
                oss << "\n";
                OSS << "auto range = vertices(g);\n";
                OSS << "for (auto iter = range.first; iter != range.second; ++iter) {\n";
                OSS << "    numEdges += out_degree(*iter, g);\n";
                OSS << "}\n";
                OSS << "return numEdges;\n";
            }
            OSS << "}\n";
        } else {
            OSS << "inline " << name << "::edges_size_type\n";
            OSS << "num_edges(const " << name << "& g) noexcept { // NOLINT\n";
            OSS << "    return gsl::narrow_cast<" << name << "::edges_size_type>(g.edges.size());\n";
            OSS << "}\n";
        }

        if (!s.mIncidence) {
            oss << "\n";
            OSS << "inline " << name << "::vertex_descriptor\n";
            OSS << "source(const " << name << "::edge_descriptor& e, const " << name << "& g) noexcept {\n";
            OSS << "    return e.source;\n";
            OSS << "}\n";

            oss << "\n";
            OSS << "inline " << name << "::vertex_descriptor\n";
            OSS << "target(const " << name << "::edge_descriptor& e, const " << name << "& g) noexcept {\n";
            OSS << "    return e.target;\n";
            OSS << "}\n";
        }

        if (s.mMutableGraphEdge) {
            oss << "\n";
            OSS << "// MutableGraph(Edge)\n";
            OSS << "inline std::pair<" << name << "::edge_descriptor, bool>\n";
            OSS << "add_edge( // NOLINT\n";
            OSS << "    " << name << "::vertex_descriptor u,\n";
            OSS << "    " << name << "::vertex_descriptor v, " << name << "& g) {\n";
            {
                INDENT();
                copyString(oss, space, generateAddEdge(false, false, false, false));
            }
            OSS << "}\n";

            // remove edge(s)
            oss << "\n";
            OSS << "inline void remove_edge(" << name << "::vertex_descriptor u, "
                << name << "::vertex_descriptor v, " << name << "& g) noexcept { // NOLINT\n";
            {
                INDENT();
                if (false && s.mReferenceGraph && s.mAliasGraph && s.mNamed /* && s.mUniqueName*/) {
                    OSS << "Expects(detachable(v));\n";
                }
                if (s.needEdgeList()) {
                    if (s.isDirectedOnly()) {
                        Expects(false);
                    } else {
                        if (s.isVector()) {
                            OSS << "auto& s = g.vertices[u];\n";
                            OSS << "auto& t = g.vertices[v];\n";
                        } else {
                            OSS << "auto& s = *static_cast<" << name << "::Vertex*>(u);\n";
                            OSS << "auto& t = *static_cast<" << name << "::Vertex*>(v);\n";
                        }
                        // remove edges
                        oss << "\n";
                        OSS << "impl::removeDirectedAllEdgeProperties(g, s.outEdges, v);\n";
                        // remove incidence edges
                        oss << "\n";
                        OSS << "s.outEdges.erase(std::remove_if(s.outEdges.begin(), s.outEdges.end(),\n";
                        {
                            INDENT();
                            OSS << "[v](const " << name << "::OutEdge& oe) {\n";
                            OSS << "    return oe.target == v;\n";
                            OSS << "}), s.outEdges.end());\n";
                        }
                        if (s.isBidirectionalOnly()) {
                            OSS << "t.inEdges.erase(std::remove_if(t.inEdges.begin(), t.inEdges.end(),\n";
                            {
                                INDENT();
                                OSS << "[u](const " << name << "::InEdge& ie) {\n";
                                OSS << "    return ie.target == u;\n";
                                OSS << "}), t.inEdges.end());\n";
                            }
                        } else {
                            OSS << "t.outEdges.erase(std::remove_if(t.outEdges.begin(), t.outEdges.end(),\n";
                            {
                                INDENT();
                                OSS << "[u](const " << name << "::OutEdge& oe) {\n";
                                OSS << "    return oe.target == u;\n";
                                OSS << "}), t.outEdges.end());\n";
                            }
                        }
                    }
                } else {
                    if (s.isDirectedOnly()) {
                        Expects(false);
                    } else {
                        if (s.isVector()) {
                            OSS << "auto& s = g.vertices[u];\n";
                            OSS << "auto& t = g.vertices[v];\n";
                        } else {
                            OSS << "auto& s = *static_cast<" << name << "::Vertex*>(u);\n";
                            OSS << "auto& t = *static_cast<" << name << "::Vertex*>(v);\n";
                        }
                        OSS << "s.outEdges.erase(std::remove(s.outEdges.begin(), s.outEdges.end(), " << name << "::OutEdge(v)), s.outEdges.end());\n";
                        if (s.isBidirectionalOnly()) {
                            OSS << "t.inEdges.erase(std::remove(t.inEdges.begin(), t.inEdges.end(), " << name << "::InEdge(u)), t.inEdges.end());\n";
                        } else {
                            OSS << "t.outEdges.erase(std::remove_if(t.outEdges.begin(), t.outEdges.end(), " << name << "::OutEdge(u)), t.outEdges.end());\n";
                        }
                    }
                }
            }
            OSS << "}\n";

            // remove edge (iter)
            if (s.mIncidence) {
                oss << "\n";
                OSS << "inline void remove_edge(" << name << "::out_edge_iterator outIter, " << name << "& g) noexcept { // NOLINT\n";
                {
                    INDENT();
                    OSS << "auto e = *outIter;\n";
                    if (s.isVector()) {
                        OSS << "const auto u = source(e, g);\n";
                        OSS << "const auto v = target(e, g);\n";
                    } else {
                        OSS << "auto* u = source(e, g);\n";
                        OSS << "auto* v = target(e, g);\n";
                    }
                    if (false && s.mReferenceGraph && s.mAliasGraph && s.mNamed /*&& s.mUniqueName*/) {
                        OSS << "// CC_EXPECTS(detachable(v));\n";
                    }
                    if (s.isVector()) {
                        OSS << "auto& s = g.vertices[u];\n";
                        OSS << "auto& t = g.vertices[v];\n";
                    } else {
                        OSS << "using VertexType = " << name << "::vertex_descriptor;\n";
                        OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                        OSS << "auto& t = *static_cast<VertexType*>(v);\n";
                    }
                    if (s.needEdgeList()) {
                        OSS << "auto edgeIter = outIter.base()->get_iter();\n";
                        if (s.isBidirectionalOnly()) {
                            OSS << name << "::InEdge ie(u, edgeIter);\n";
                            OSS << "auto inIter = std::find(t.inEdges.begin(), t.inEdges.end(), ie);\n";
                            OSS << "CC_EXPECTS(inIter != t.inEdges.end());\n";
                            OSS << "t.inEdges.erase(inIter);\n";
                        } else {
                            OSS << name << "::OutEdge oe(u, edgeIter);\n";
                            OSS << "auto outIter = std::find_if(t.outEdges.begin(), t.outEdges.end(), oe);\n";
                            OSS << "CC_EXPECTS(outIter != t.outEdges.end());\n";
                            OSS << "t.outEdges.erase(outIter);\n";
                        }
                        OSS << "g.edges.erase(edgeIter);\n";
                    } else {
                        if (s.isBidirectionalOnly()) {
                            OSS << "auto inIter = std::find(t.inEdges.begin(), t.inEdges.end(), "
                                << name << "::InEdge(u));\n";
                            OSS << "CC_EXPECTS(inIter != t.inEdges.end());\n";
                            OSS << "t.inEdges.erase(inIter);\n";
                        } else {
                            OSS << "auto outIter = std::find_if(t.outEdges.begin(), t.outEdges.end(), "
                                << name << "::OutEdge(u));\n";
                            OSS << "CC_EXPECTS(outIter != t.outEdges.end());\n";
                            OSS << "t.outEdges.erase(outIter);\n";
                        }
                    }
                    OSS << "s.outEdges.erase(outIter.base());\n";
                }
                OSS << "}\n";
            }

            // remove edge (descriptor)
            oss << "\n";
            OSS << "inline void remove_edge(" << name << "::edge_descriptor e, " << name << "& g) noexcept { // NOLINT\n";
            {
                INDENT();
                if (s.needEdgeList()) {
                    if (s.isDirectedOnly()) {
                        Expects(false);
                    } else {
                        if (s.isVector()) {
                            OSS << "const auto u = source(e, g);\n";
                            OSS << "const auto v = target(e, g);\n";
                        } else {
                            OSS << "auto* u = source(e, g);\n";
                            OSS << "auto* v = target(e, g);\n";
                        }
                        OSS << "auto* const p = e.get_property();\n";
                        if (s.isVector()) {
                            OSS << "auto& s = g.vertices[u];\n";
                        } else {
                            OSS << "auto& s = *static_cast<" << name << "::Vertex*>(u);\n";
                        }
                        // find edgeIter
                        OSS << "auto outIter = std::find_if(s.outEdges.begin(), s.outEdges.end(), [p](const auto& oe) {\n";
                        OSS << "    return &oe.get_property() == p;\n";
                        OSS << "});\n";
                        OSS << "CC_EXPECTS(outIter != s.outEdges.end());\n";
                    }
                } else {
                    if (s.isVector()) {
                        OSS << "const auto u = source(e, g);\n";
                        OSS << "const auto v = target(e, g);\n";
                    } else {
                        OSS << "auto* u = source(e, g);\n";
                        OSS << "auto* v = target(e, g);\n";
                    }
                    if (s.isVector()) {
                        OSS << "auto& s = g.vertices[u];\n";
                    } else {
                        OSS << "auto& s = *static_cast<" << name << "::Vertex*>(u);\n";
                    }
                    OSS << "auto outIter = std::find(s.outEdges.begin(), s.outEdges.end(), "
                        << name << "::OutEdge(v));\n";
                    OSS << "CC_EXPECTS(outIter != s.outEdges.end());\n";
                }
                OSS << "remove_edge(" << name << "::out_edge_iterator(outIter, u), g);\n";
            }
            OSS << "}\n";

            if (s.mIncidence) {
                // remove out edge if
                if (false) {
                    oss << "\n";
                    OSS << "template <class Predicate>\n";
                    OSS << "inline void remove_out_edge_if(" << name << "::vertex_descriptor u, Predicate&& pred, " << name << "& g)";
                    if (!s.mUndirected && !s.hasEdgeProperty())
                        oss << " noexcept";
                    oss << " { // NOLINT\n";
                    {
                        INDENT();
                        if (s.isDirectedOnly()) {
                            OSS << "auto pair = out_edges(u, g);\n";
                            OSS << "auto& first = pair.first;\n";
                            OSS << "auto& last = pair.second;\n";
                            OSS << "auto& outEdgeList  = g.getOutEdgeList(u);\n";
                            copyString(oss, space, removeDirectedEdgeIf(s, "outEdgeList", scratch));
                        } else if (s.mUndirected) {
                            OSS << "auto pair = out_edges(u, g);\n";
                            OSS << "auto& first = pair.first;\n";
                            OSS << "auto& last = pair.second;\n";
                            OSS << "auto& outEdgeList  = g.getOutEdgeList(u);\n";
                            visit(
                                overload(
                                    [&]<Sequence_ T>(T) {
                                        OSS << "impl::sequenceRemoveUndirectedOutEdgeIf(g, first, last, outEdgeList, std::forward<Predicate>(pred));\n";
                                    },
                                    [&]<Associative_ T>(T) {
                                        OSS << "impl::associativeRemoveUndirectedOutEdgeIf(g, first, last, outEdgeList, std::forward<Predicate>(pred));\n";
                                    }),
                                s.mOutEdgeListType);
                        } else {
                            Expects(s.mBidirectional);
                            if (s.hasEdgeProperty()) {
                                OSS << "ccstd::vector<" << edgeListType(ns) << "::iterator> garbage;\n";
                            }

                            OSS << "for (auto pair = out_edges(u, g); pair.first != pair.second; ++pair.first) {\n";
                            {
                                INDENT();
                                OSS << "auto& outIter = pair.first;\n";
                                OSS << "auto& outEnd = pair.second;\n";
                                OSS << "if (pred(*outIter)) {\n";
                                {
                                    INDENT();
                                    OSS << "auto& inEdgeList = g.getInEdgeList(target(*outIter, g));\n";
                                    OSS << "auto  e          = *outIter;\n";
                                    OSS << "std::swap(e.source, e.target);\n";
                                    OSS << "impl::removeIncidenceEdge(e, inEdgeList);\n";
                                    if (s.hasEdgeProperty()) {
                                        OSS << "garbage.emplace_back((*outIter.base()).get_iter());\n";
                                    }
                                }
                                OSS << "}\n";
                            }
                            OSS << "}\n";

                            OSS << "auto pair = out_edges(u, g);\n";
                            OSS << "auto& first = pair.first;\n";
                            OSS << "auto& last = pair.second;\n";
                            OSS << "auto& outEdgeList  = g.getOutEdgeList(u);\n";
                            copyString(oss, space, removeDirectedEdgeIf(s, "outEdgeList", scratch));

                            if (s.hasEdgeProperty()) {
                                OSS << "for (const auto& v : garbage) {\n";
                                OSS << "    g.edges.erase(v);\n";
                                OSS << "}\n";
                            }
                        }
                    }
                    OSS << "}\n";
                }
                
                if (false && s.mBidirectional) {
                    // remove in edge if
                    oss << "\n";
                    OSS << "template <class Predicate>\n";
                    OSS << "inline void remove_in_edge_if(" << name << "::vertex_descriptor v, Predicate&& pred, " << name << "& g)";
                    if (!s.mUndirected && !s.hasEdgeProperty())
                        oss << " noexcept";
                    oss << " { // NOLINT\n";
                    {
                        INDENT();
                        if (s.mUndirected) {
                            OSS << "remove_out_edge_if(v, std::forward<Predicate>(pred), g);\n";
                        } else {
                            if (s.hasEdgeProperty()) {
                                OSS << "ccstd::vector<" << edgeListType(ns)
                                    << "::iterator> garbage;\n";
                            }

                            OSS << "for (auto pair = in_edges(v, g); pair.first != pair.second; ++pair.first) {\n";
                            {
                                INDENT();
                                OSS << "auto& inIter = pair.first;\n";
                                OSS << "auto& inEnd = pair.second;\n";
                                OSS << "if (pred(*inIter)) {\n";
                                {
                                    INDENT();
                                    OSS << "auto& outEdgeList = g.getOutEdgeList(source(*inIter, g));\n";
                                    OSS << "auto  e           = *inIter;\n";
                                    OSS << "impl::removeIncidenceEdge(e, outEdgeList);\n";
                                    if (s.hasEdgeProperty()) {
                                        OSS << "garbage.emplace_back((*inIter.base()).get_iter());\n";
                                    }
                                }
                                OSS << "}\n";
                            }
                            OSS << "}\n";

                            OSS << "auto pair = in_edges(v, g);\n";
                            OSS << "auto& first = pair.first;\n";
                            OSS << "auto& last = pair.second;\n";
                            OSS << "auto& inEdgeList   = g.getInEdgeList(v);\n";
                            copyString(oss, space, removeDirectedEdgeIf(s, "inEdgeList", scratch));
                            if (s.hasEdgeProperty()) {
                                OSS << "for (const auto& v : garbage) {\n";
                                OSS << "    g.edges.erase(v);\n";
                                OSS << "}\n";
                            }
                        }
                    }
                    OSS << "}\n";
                } // BidirectionalGraph

                // remove edge if
                if (false) {
                    oss << "\n";
                    OSS << "template <class Predicate>\n";
                    OSS << "inline void remove_edge_if(Predicate&& pred, " << name << "& g)";
                    if (!s.mUndirected && !s.hasEdgeProperty())
                        oss << " noexcept";
                    oss << " { // NOLINT\n";
                    {
                        INDENT();
                        if (s.isDirectedOnly()) {
                            OSS << "for (auto pair = vertices(g); pair.first != pair.second; ++pair.first) {\n";
                            OSS << "    auto& vi = pair.first;\n";
                            OSS << "    auto& viEnd = pair.second;\n";
                            OSS << "    remove_out_edge_if(*vi, std::forward<Predicate>(pred), g);\n";
                            OSS << "}\n";
                        } else {
                            OSS << "auto pair = edges(g);\n";
                            OSS << "auto& ei = pair.first;\n";
                            OSS << "auto& eiEnd = pair.second;\n";
                            OSS << "for (auto next = ei; ei != eiEnd; ei = next) {\n";
                            {
                                INDENT();
                                OSS << "++next;\n";
                                OSS << "if (pred(*ei)) {\n";
                                OSS << "    remove_edge(*ei, g);\n";
                                OSS << "}\n";
                            }
                            OSS << "}\n";
                        }
                    }
                    OSS << "}\n";
                }
            } // IncidenceGraph
        } // MutableGraph(Edge)
    } // EdgeList

    if (s.mVertexList && s.mMutableGraphVertex && !s.mReferenceGraph) {
        oss << "\n";
        OSS << "// MutableGraph(Vertex)\n";
        copyString(oss, space, clearVertex(s, name, scratch));

        if (s.isPolymorphic()) {
            oss << "\n";
            copyString(oss, space, removePolymorphicType(g, *this, s, name, scratch));
        }

        oss << "\n";
        copyString(oss, space, removeVertex(*this, s, name, scratch));
    }

    if (s.mEdgeList && s.mMutableGraphEdge && s.hasEdgeProperty()) {
        oss << "\n";
        OSS << "// MutablePropertyGraph(Edge)\n";
        OSS << "template <class EdgeProperty>\n";
        OSS << "inline std::pair<" << name << "::edge_descriptor, bool>\n";
        OSS << "add_edge( // NOLINT\n";
        OSS << "    " << name << "::vertex_descriptor u,\n";
        OSS << "    " << name << "::vertex_descriptor v,\n";
        OSS << "    EdgeProperty&& p, " << name << "& g) {\n";
        {
            INDENT();
            copyString(oss, space, generateAddEdge(true, true, false, false));
        }
        OSS << "}\n";

        oss << "\n";
        OSS << "template <class... T>\n";
        OSS << "inline std::pair<" << name << "::edge_descriptor, bool>\n";
        OSS << "add_edge( // NOLINT\n";
        OSS << "    " << name << "::vertex_descriptor u,\n";
        OSS << "    " << name << "::vertex_descriptor v,\n";
        OSS << "    " << name << "& g, T&&... args) {\n";
        {
            INDENT();
            copyString(oss, space, generateAddEdge(true, true, true, false));
        }
        OSS << "}\n";
    }

    if (s.mVertexList && s.mMutableGraphVertex && (s.hasVertexProperty() ||
        !s.mComponents.empty() || s.isPolymorphic()) && !s.mReferenceGraph) {
        oss << "\n";
        OSS << "// MutablePropertyGraph(Vertex)\n";
        copyString(oss, space, addVertex(true, false, true));
        oss << "\n";
        copyString(oss, space, addVertex(true, true, true));
    }

    if (s.mReferenceGraph) {
        copyString(oss, space, generateAddressableGraph());
    }

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateGraphBoostFunctions_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto* ptr = &s;
    const auto vertID = mStruct.mCurrentVertex;
    auto scratch = get_allocator().resource();
    const auto& cpp = mStruct;

    auto cn = mStruct.mCurrentNamespace;
    auto name = cpp.getImplName(cn);

    if (s.hasVertexProperty()) {
        oss << "\n";
        OSS << "// Vertex All\n";
        OSS << "template <>\n";
        OSS << "struct property_map<" << name << ", vertex_all_t> {\n";
        {
            INDENT();
            OSS << "using const_type = ";
            copyString(oss, space, vertexPropertyMapName(true), true);
            oss << ";\n";
            OSS << "using type = ";
            copyString(oss, space, vertexPropertyMapName(false), true);
            oss << ";\n";
        }
        OSS << "};\n";

        oss << "\n";
        OSS << "// Vertex Bundle\n";
        OSS << "template <>\n";
        OSS << "struct property_map<" << name << ", vertex_bundle_t> {\n";
        {
            INDENT();
            OSS << "using const_type = ";
            copyString(oss, space, vertexPropertyMapName(true), true);
            oss << ";\n";
            OSS << "using type = ";
            copyString(oss, space, vertexPropertyMapName(false), true);
            oss << ";\n";
        }
        OSS << "};\n";

        auto vpID = locate(s.mVertexProperty, g);
        if (holds_tag<Struct_>(vpID, g) || holds_tag<Graph_>(vpID, g)) {
            oss << "\n";
            OSS << "// Vertex BundleMember\n";
            OSS << "template <class T>\n";
            OSS << "struct property_map<" << name << ", T "
                << cpp.getDependentName(cpp.mCurrentPath) << "::*> {\n";
            {
                INDENT();
                OSS << "using const_type = ";
                copyString(oss, space, vertexPropertyMapMemberName(true), true);
                oss << ";\n";
                OSS << "using type = ";
                copyString(oss, space, vertexPropertyMapMemberName(false), true);
                oss << ";\n";
            }
            OSS << "};\n";
        }

        if (s.mNamed && !s.mNamedConcept.mComponent) {
            oss << "\n";
            OSS << "// Vertex Name\n";
            OSS << "template <>\n";
            OSS << "struct property_map<" << name << ", vertex_name_t> {\n";
            {
                INDENT();
                if (g.isString(vpID)) {
                    OSS << "using const_type = ";
                    copyString(oss, space, vertexPropertyMapName(true), true);
                    oss << ";\n";
                    OSS << "using type = ";
                    copyString(oss, space, vertexPropertyMapName(false), true);
                    oss << ";\n";
                } else {
                    OSS << "using const_type = ";
                    copyString(oss, space, vertexPropertyMapMemberName(true), true);
                    oss << ";\n";
                    OSS << "using type = ";
                    copyString(oss, space, vertexPropertyMapMemberName(false), true);
                    oss << ";\n";
                }
            }
            OSS << "};\n";
        }
    } else {
        if (s.isVector()) {
            oss << "\n";
            OSS << "// Vertex Index\n";
            OSS << "template <>\n";
            OSS << "struct property_map<" << name << ", vertex_index_t> {\n";
            {
                INDENT();
                OSS << "using const_type = identity_property_map;\n";
                OSS << "using type       = identity_property_map;\n";
            }
            OSS << "};\n";
        }
    }

    if (!s.mComponents.empty()) {
        for (const auto& c : s.mComponents) {
            auto componentName = cpp.getDependentName(c.mValuePath);
            oss << "\n";
            OSS << "// Vertex Component\n";
            {
                const auto& tagType = getTagType(c.mName, scratch);
                OSS << "template <>\n";
                OSS << "struct property_map<" << name << ", " << name << "::" << tagType << "> {\n";
                {
                    INDENT();
                    OSS << "using const_type = ";
                    copyString(oss, space, vertexComponentMapName(c, true), true);
                    oss << ";\n";
                    OSS << "using type = ";
                    copyString(oss, space, vertexComponentMapName(c, false), true);
                    oss << ";\n";
                }
                OSS << "};\n";
            }

            auto valueID = locate(c.mValuePath, g);
            const auto& traits = get(g.traits, g, valueID);
            if (!traits.mImport) {
                if (g.isComposition(valueID)) {
                    oss << "\n";
                    OSS << "// Vertex ComponentMember\n";
                    OSS << "template <class T>\n";
                    OSS << "struct property_map<" << name << ", T " << componentName << "::*> {\n";
                    {
                        INDENT();
                        OSS << "using const_type = ";
                        copyString(oss, space, vertexComponentMapMemberName(c, true), true);
                        oss << ";\n";
                        OSS << "using type = ";
                        copyString(oss, space, vertexComponentMapMemberName(c, false), true);
                        oss << ";\n";
                    }
                    OSS << "};\n";
                }

                if (g.hasString(valueID)) {
                    std::pmr::string stringName(scratch);
                    auto generate = [&]() {
                        oss << "\n";
                        OSS << "// Vertex ComponentMember(String)\n";
                        OSS << "template <>\n";
                        OSS << "struct property_map<" << name << ", " << stringName << " "
                            << componentName << "::*> {\n";
                        {
                            INDENT();
                            OSS << "using const_type = ";
                            copyString(oss, space, vertexComponentMapMemberName(c, true, stringName), true);
                            oss << ";\n";
                            OSS << "using type = ";
                            copyString(oss, space, vertexComponentMapMemberName(c, false, stringName), true);
                            oss << ";\n";
                        }
                        OSS << "};\n";
                    };

                    auto stringID = g.getFirstMemberString(valueID);
                    if (stringID != g.null_vertex()) {
                        stringName = cpp.getDependentName(stringID);
                        generate();
                    }
                    stringID = g.getFirstMemberUtf8(valueID);
                    if (stringID != g.null_vertex()) {
                        stringName = cpp.getDependentName(stringID);
                        generate();
                    }
                }
            }

            if (s.mNamed && s.mNamedConcept.mComponent && c.mName == s.mNamedConcept.mComponentName) {
                oss << "\n";
                OSS << "// Vertex Name\n";
                OSS << "template <>\n";
                OSS << "struct property_map<" << name << ", vertex_name_t> {\n";
                {
                    INDENT();
                    if (s.mNamedConcept.mComponentMemberName.empty()) {
                        OSS << "using const_type = ";
                        copyString(oss, space, vertexComponentMapName(c, true), true);
                        oss << ";\n";
                        OSS << "using type = ";
                        copyString(oss, space, vertexComponentMapName(c, false), true);
                        oss << ";\n";
                    } else {
                        OSS << "using const_type = ";
                        copyString(oss, space, vertexComponentMapMemberName(c, true), true);
                        oss << ";\n";
                        OSS << "using type = ";
                        copyString(oss, space, vertexComponentMapMemberName(c, false), true);
                        oss << ";\n";
                    }
                }
                OSS << "};\n";
            }
        }
    }

    if (s.hasEdgeProperty()) {
        oss << "\n";
        OSS << "// Edge All\n";
        OSS << "template <>\n";
        OSS << "struct property_map<" << name << ", edge_all_t> {\n";
        {
            INDENT();
            OSS << "using const_type = ";
            copyString(oss, space, edgePropertyMapName(true), true);
            oss << ";\n";
            OSS << "using type = ";
            copyString(oss, space, edgePropertyMapName(false), true);
            oss << ";\n";
        }
        OSS << "};\n";

        oss << "\n";
        OSS << "// Edge Bundle\n";
        OSS << "template <>\n";
        OSS << "struct property_map<" << name << ", edge_bundle_t> {\n";
        {
            INDENT();
            OSS << "using const_type = ";
            copyString(oss, space, edgePropertyMapName(true), true);
            oss << ";\n";
            OSS << "using type = ";
            copyString(oss, space, edgePropertyMapName(false), true);
            oss << ";\n";
        }
        OSS << "};\n";
        const auto epID = locate(s.mEdgeProperty, g);
        const auto& traits = get(g.traits, g, epID);
        if (!traits.mImport) {
            if (g.isComposition(epID)) {
                oss << "\n";
                OSS << "// Edge BundleMember\n";
                OSS << "template <class T>\n";
                OSS << "struct property_map<" << name << ", T "
                    << cpp.getDependentName(s.mEdgeProperty) << "::*> {\n";
                {
                    INDENT();
                    OSS << "using const_type = ";
                    copyString(oss, space, edgePropertyMapMemberName(true), true);
                    oss << ";\n";
                    OSS << "using type = ";
                    copyString(oss, space, edgePropertyMapMemberName(false), true);
                    oss << ";\n";
                }
                OSS << "};\n";
            }

            if (g.hasString(epID)) {
                std::pmr::string stringName(scratch);
                auto generate = [&]() {
                    oss << "\n";
                    OSS << "// Edge BundleMember(String)\n";
                    OSS << "template <>\n";
                    OSS << "struct property_map<" << name << ", " << stringName << " "
                        << cpp.getDependentName(s.mEdgeProperty) << "::*> {\n";
                    {
                        INDENT();
                        OSS << "using const_type = ";
                        copyString(oss, space, edgePropertyMapMemberName(true, stringName), true);
                        oss << ";\n";
                        OSS << "using type = ";
                        copyString(oss, space, edgePropertyMapMemberName(false, stringName), true);
                        oss << ";\n";
                    }
                    OSS << "};\n";
                };

                auto stringID = g.getFirstMemberString(epID);
                if (stringID != g.null_vertex()) {
                    stringName = cpp.getDependentName(stringID);
                    generate();
                }
                if (stringID = g.getFirstMemberUtf8(epID); stringID != g.null_vertex()) {
                    stringName = cpp.getDependentName(stringID);
                    generate();
                }
            }
        }
    }
    return oss.str();
}

std::pmr::string CppGraphBuilder::generateGraphPropertyMaps_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto* ptr = &s;
    const auto vertID = mStruct.mCurrentVertex;
    auto scratch = get_allocator().resource();
    const auto& cpp = mStruct;

    auto name = cpp.getImplName();
    auto cn = mStruct.mCurrentNamespace;
    const bool bPmr = g.isPmr(vertID);

    auto modulePath = get_path(mStruct.mCurrentModule, *mStruct.mModuleGraph, scratch);
    
    if (!g.moduleHasGraph(modulePath))
        return oss.str();

    if (s.isVector()) {
        //-----------------------------------------
        // index
        //-----------------------------------------
        oss << "\n";
        oss << "// Vertex Index\n";
        OSS << "inline boost::property_map<" << name << ", boost::vertex_index_t>::const_type\n";
        OSS << "get(boost::vertex_index_t /*tag*/, const " << name << "& /*g*/) noexcept {\n";
        {
            INDENT();
            OSS << "return {};\n";
        }
        OSS << "}\n";

        oss << "\n";
        OSS << "inline boost::property_map<" << name << ", boost::vertex_index_t>::type\n";
        OSS << "get(boost::vertex_index_t /*tag*/, " << name << "& /*g*/) noexcept {\n";
        {
            INDENT();
            OSS << "return {};\n";
        }
        OSS << "}\n";

        if (s.mColorMap) {
            oss << "\n";
            OSS << gNoDiscard << "inline impl::ColorMap<" << name << "::vertex_descriptor>\n";
            OSS << "get(ccstd::pmr::vector<boost::default_color_type>& colors, const " << name << "& /*g*/) noexcept {\n";
            OSS << "    return {colors};\n";
            OSS << "}\n";
        }
    }

    const auto vpID = locate(s.mVertexProperty, g);
    if (s.hasVertexProperty()) {
        //-----------------------------------------
        // all
        //-----------------------------------------
        oss << "\n";
        oss << "// Vertex All\n";
        OSS << "inline boost::property_map<" << name << ", boost::vertex_all_t>::const_type\n";
        OSS << "get(boost::vertex_all_t /*tag*/, const " << name << "& g) noexcept {\n";
        {
            INDENT();
            OSS << "return {g};\n";
        }
        OSS << "}\n";

        oss << "\n";
        OSS << "inline boost::property_map<" << name << ", boost::vertex_all_t>::type\n";
        OSS << "get(boost::vertex_all_t /*tag*/, " << name << "& g) noexcept {\n";
        {
            INDENT();
            OSS << "return {g};\n";
        }
        OSS << "}\n";
        //-----------------------------------------
        // bundle
        //-----------------------------------------
        oss << "\n";
        oss << "// Vertex Bundle\n";
        OSS << "inline boost::property_map<" << name << ", boost::vertex_bundle_t>::const_type\n";
        OSS << "get(boost::vertex_bundle_t /*tag*/, const " << name << "& g) noexcept {\n";
        {
            INDENT();
            OSS << "return {g};\n";
        }
        OSS << "}\n";

        oss << "\n";
        OSS << "inline boost::property_map<" << name << ", boost::vertex_bundle_t>::type\n";
        OSS << "get(boost::vertex_bundle_t /*tag*/, " << name << "& g) noexcept {\n";
        {
            INDENT();
            OSS << "return {g};\n";
        }
        OSS << "}\n";
        //-----------------------------------------
        // bundle member
        //-----------------------------------------
        if (g.isComposition(vpID)) {
            auto vertexProperty = cpp.getDependentName(vpID);
            oss << "\n";
            oss << "// Vertex BundleMember\n";
            OSS << "template <class T>\n";
            OSS << "inline typename boost::property_map<" << name << ", T " << vertexProperty
                << "::*>::const_type\n";
            OSS << "get(T " << vertexProperty << "::*memberPointer, const " << name << "& g";
            oss << ") noexcept {\n";
            {
                INDENT();
                OSS << "return {g, memberPointer};\n";
            }
            OSS << "}\n";

            oss << "\n";
            OSS << "template <class T>\n";
            OSS << "inline typename boost::property_map<" << name << ", T " << vertexProperty
                << "::*>::type\n";
            OSS << "get(T " << vertexProperty << "::*memberPointer, " << name << "& g";
            oss << ") noexcept {\n";
            {
                INDENT();
                OSS << "return {g, memberPointer};\n";
            }
            OSS << "}\n";
        }

        if (s.mNamed && !s.mNamedConcept.mComponent) {
            std::pmr::string stringName(scratch);
            if (s.mNamedConcept.mComponentMemberName.empty()) {
                stringName = cpp.getDependentName(s.mVertexProperty);
            } else {
                if (auto stringID = g.getFirstMemberString(vpID); stringID != g.null_vertex()) {
                    stringName = cpp.getDependentName(stringID);
                } else {
                    if (stringID = g.getFirstMemberUtf8(vpID); stringID != g.null_vertex()) {
                        stringName = cpp.getDependentName(stringID);
                    }
                }
            }
            oss << "\n";
            oss << "// Vertex Name\n";
            OSS << "inline boost::property_map<" << name << ", boost::vertex_name_t>::const_type\n";
            OSS << "get(boost::vertex_name_t, const " << name << "& g";
            oss << ") noexcept {\n";
            {
                INDENT();
                if (s.mNamedConcept.mComponentMemberName.empty()) {
                    OSS << "return {g};\n";
                } else {
                    OSS << "return {g, &"
                        << cpp.getDependentName(s.mVertexProperty) << "::"
                        << s.mNamedConcept.mComponentMemberName << "};\n";
                }
            }
            OSS << "}\n";

            oss << "\n";
            OSS << "inline boost::property_map<" << name << ", boost::vertex_name_t>::type\n";
            OSS << "get(boost::vertex_name_t, " << name << "& g";
            oss << ") noexcept {\n";
            {
                INDENT();
                if (s.mNamedConcept.mComponentMemberName.empty()) {
                    OSS << "return {g};\n";
                } else {
                    OSS << "return {g, &"
                        << cpp.getDependentName(s.mVertexProperty) << "::"
                        << s.mNamedConcept.mComponentMemberName << "};\n";
                }
            }
            OSS << "}\n";
        }
    }

    if (!s.mComponents.empty()) {
        for (const auto& c : s.mComponents) {
            auto vertexComponent = cpp.getDependentName(c.mValuePath);
            oss << "\n";
            oss << "// Vertex Component\n";

            const auto& member = g.getMemberName(c.mMemberName, true);
            auto componentID = locate(c.mValuePath, g);

            { // component tag getter
                const auto& tagType = getTagType(c.mName, scratch);

                auto outputGetContent = [&]() {
                    INDENT();
                    if (c.isVector()) {
                        OSS << "return {g." << member << "};\n";
                    } else {
                        OSS << "return {g, &" << name << "::Vertex::" << member << "Iter};\n";
                    }
                };

                OSS << "inline typename boost::property_map<" << name << ", " << name << "::" << tagType
                    << ">::const_type\n";
                OSS << "get(" << name << "::" << tagType << " /*tag*/, const " << name << "& g";
                oss << ") noexcept {\n";
                {
                    outputGetContent();
                }
                OSS << "}\n";

                oss << "\n";
                OSS << "inline typename boost::property_map<" << name << ", " << name << "::" << tagType
                    << ">::type\n";
                OSS << "get(" << name << "::" << tagType << " /*tag*/, " << name << "& g";
                oss << ") noexcept {\n";
                {
                    outputGetContent();
                }
                OSS << "}\n";
            }

            const auto& traits = get(g.traits, g, componentID);
            if (!traits.mImport && g.isComposition(componentID)) {
                oss << "\n";
                oss << "// Vertex ComponentMember\n";
                OSS << "template <class T>\n";
                OSS << "inline typename boost::property_map<" << name << ", T " << vertexComponent
                    << "::*>::const_type\n";
                OSS << "get(T " << vertexComponent << "::*memberPointer, const " << name << "& g";
                oss << ") noexcept {\n";
                {
                    INDENT();
                    if (c.isVector()) {
                        OSS << "return {g." << member << ", memberPointer};\n";
                    } else {
                        OSS << "return {g, &" << name << "::Vertex::" << member << "Iter, memberPointer};\n";
                    }
                }
                OSS << "}\n";

                oss << "\n";
                OSS << "template <class T>\n";
                OSS << "inline typename boost::property_map<" << name << ", T " << vertexComponent
                    << "::*>::type\n";
                OSS << "get(T " << vertexComponent << "::*memberPointer, " << name << "& g";
                oss << ") noexcept {\n";
                {
                    INDENT();
                    if (c.isVector()) {
                        OSS << "return {g." << member << ", memberPointer};\n";
                    } else {
                        OSS << "return {g, &" << name << "::Vertex::" << member << "Iter, memberPointer};\n";
                    }
                }
                OSS << "}\n";
            }

            if (s.mNamed && s.mNamedConcept.mComponent && s.mNamedConcept.mComponentName == c.mName) {
                oss << "\n";
                oss << "// Vertex Name\n";
                OSS << "inline boost::property_map<" << name << ", boost::vertex_name_t>::const_type\n";
                OSS << "get(boost::vertex_name_t /*tag*/, const " << name << "& g";
                oss << ") noexcept {\n";
                {
                    INDENT();
                    if (s.mNamedConcept.mComponentMemberName.empty()) {
                        if (c.isVector()) {
                            OSS << "return {g." << member << "};\n";
                        } else {
                            OSS << "return {g, &" << name << "::Vertex::" << member << "Iter};\n";
                        }
                    } else {
                        if (c.isVector()) {
                            OSS << "return {g." << member << ", &"
                                << vertexComponent << "::"
                                << s.mNamedConcept.mComponentMemberName << "};\n";
                        } else {
                            OSS << "return {g, &" << name << "::Vertex::" << member << "Iter, &"
                                << vertexComponent << "::"
                                << s.mNamedConcept.mComponentMemberName << "};\n";
                        }
                    }
                }
                OSS << "}\n";

                if (!traits.mImport && g.isComposition(componentID)) {
                    oss << "\n";
                    OSS << "inline boost::property_map<" << name << ", boost::vertex_name_t>::type\n";
                    OSS << "get(boost::vertex_name_t /*tag*/, " << name << "& g";
                    oss << ") noexcept {\n";
                    {
                        INDENT();
                        if (s.mNamedConcept.mComponentMemberName.empty()) {
                            if (c.isVector()) {
                                OSS << "return {g." << member << "};\n";
                            } else {
                                OSS << "return {g, &" << name << "::Vertex::" << member << "Iter};\n";
                            }
                        } else {
                            if (c.isVector()) {
                                OSS << "return {g." << member << ", &"
                                    << vertexComponent << "::"
                                    << s.mNamedConcept.mComponentMemberName << "};\n";
                            } else {
                                OSS << "return {g, &" << name << "::Vertex::" << member << "Iter, &"
                                    << vertexComponent << "::"
                                    << s.mNamedConcept.mComponentMemberName << "};\n";
                            }
                        }
                    }
                    OSS << "}\n";
                }
            }
        }
    }

    if (s.mAddressable && s.mAddressableConcept.mPathPropertyMap) {
        oss << "\n";
        OSS << "inline boost::property_map<" << name << ", impl::path_t>::const_type\n";
        OSS << "get(impl::path_t /*tag*/, const " << name << "& g";
        oss << ") noexcept {\n";
        {
            INDENT();
            if (s.isVector()) {
                if (s.isAliasGraph()) {
                    OSS << "return {g.vertices};\n";
                } else {
                    OSS << "return {g.objects};\n";
                }
            }
        }
        OSS << "}\n";
    }

    if (s.isPolymorphic()) {
        oss << "\n";
        OSS << "// PolymorphicGraph\n";
        if (holds_alternative<Vector_>(s.mVertexListType)) {
            OSS << gNoDiscard << "inline " << name << "::vertices_size_type\n";
            OSS << "id(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept {\n";
            {
                INDENT();
                OSS << "using vertex_descriptor = " << name << "::vertex_descriptor;\n";
                OSS << "return cc::visit(\n";
                {
                    INDENT();
                    OSS << "overload(\n";
                    {
                        INDENT();
                        int count = 0;
                        for (const auto& c : s.mPolymorphic.mConcepts) {
                            if (count++) {
                                oss << ",\n";
                            }
                            if (c.mMemberName.empty()) {
                                OSS << "[](const " << handleElemType(c, cn, true) << "&) {\n";
                                OSS << "    return " << name << "::null_vertex();\n";
                                OSS << "}";
                            } else {
                                if (c.isVector()) {
                                    OSS << "[](const " << handleElemType(c, cn, true) << "& h) {\n";
                                    OSS << "    return h.value;\n";
                                    OSS << "}";
                                } else {
                                    OSS << "[](const " << handleElemType(c, cn, true) << "& h) {\n";
                                    OSS << "    return " << name << "::null_vertex();\n";
                                    OSS << "}";
                                }
                            }
                        }
                    }
                    if (s.isVector()) {
                        oss << "),\n";
                        OSS << "g." << g.getMemberName("mVertices", true) << "[u].handle);\n";
                    } else {
                        oss << "),\n";
                        OSS << "static_cast<" << name << "::Vertex*>(u)->handle);\n";
                    }
                }
            }
            OSS << "}\n";
            oss << "\n";
        }

        OSS << gNoDiscard << "inline " << name << "::VertexTag\n";
        OSS << "tag(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept {\n";
        {
            INDENT();
            OSS << "using vertex_descriptor = " << name << "::vertex_descriptor;\n";
            OSS << "return cc::visit(\n";
            {
                INDENT();
                OSS << "overload(\n";
                {
                    INDENT();
                    int count = 0;
                    for (const auto& c : s.mPolymorphic.mConcepts) {
                        if (count++) {
                            oss << ",\n";
                        }
                        OSS << "[](const " << handleElemType(c, cn, true) << "&) {\n";
                        OSS << "    return " << name << "::VertexTag{"
                            << cpp.getDependentName(c.mTag) << "{}};\n";
                        OSS << "}";
                    }
                }
                if (s.isVector()) {
                    oss << "),\n";
                    OSS << "g.vertices[u].handle);\n";
                } else {
                    oss << "),\n";
                    OSS << "static_cast<" << name << "::Vertex*>(u)->handle);\n";
                }
            }
        }
        OSS << "}\n";

        auto generateGetValue = [&](bool bConst) {
            std::string valueType = bConst ? "VertexConstValue" : "VertexValue";
            oss << "\n";
            OSS << gNoDiscard << "inline " << name << "::" << valueType << "\n";
            OSS << "value(" << name << "::vertex_descriptor u, ";
            if (bConst)
                oss << "const ";
            oss << name << "& g) noexcept {\n";
            {
                INDENT();
                OSS << "using vertex_descriptor = " << name << "::vertex_descriptor;\n";
                OSS << "return cc::visit(\n";
                {
                    INDENT();
                    OSS << "overload(\n";
                    {
                        INDENT();
                        int count = 0;
                        for (const auto& c : s.mPolymorphic.mConcepts) {
                            if (count) {
                                oss << ",\n";
                            }
                            OSS << "[&](";
                            if (c.isIntrusive()) {
                                if (bConst) {
                                    oss << "const ";
                                }
                            } else {
                                oss << "const ";
                            }
                            oss << handleElemType(c, cn, true) << "& h) {\n";
                            {
                                INDENT();
                                if (c.isIntrusive()) {
                                    OSS << "return " << name << "::" << valueType << "{";
                                    oss << "&h.value";
                                    oss << "};\n";
                                } else {
                                    if (c.isVector()) {
                                        OSS << "return " << name << "::" << valueType << "{&g."
                                            << g.getMemberName(c.mMemberName, true) << "[h.value]};\n";
                                    } else {
                                        OSS << "return " << name << "::" << valueType << "{&*h.value};\n";
                                    }
                                }
                            }
                            OSS << "}";
                            ++count;
                        }
                    }
                    if (s.isVector()) {
                        oss << "),\n";
                        OSS << "g.vertices[u].handle);\n";
                    } else {
                        oss << "),\n";
                        OSS << "static_cast<" << name << "::Vertex*>(u)->handle);\n";
                    }
                }
            }
            OSS << "}\n";
        };
        generateGetValue(false);
        generateGetValue(true);

        auto generateHold = [&](bool bTag) {
            oss << "\n";
            if (bTag) {
                OSS << "template <class Tag>\n";
            } else {
                OSS << "template <class ValueT>\n";
            }
            OSS << gNoDiscard << "inline bool\n";
            if (bTag) {
                OSS << "holds(" << name << "::vertex_descriptor v, ";
                oss << "const " << name << "& g) noexcept {\n";
            } else {
                OSS << "holds_alternative(" << name << "::vertex_descriptor v, ";
                oss << "const " << name << "& g) noexcept { // NOLINT\n";
            }
            {
                INDENT();
                OSS << "using vertex_descriptor = " << name << "::vertex_descriptor;\n";
                OSS;
                int count = 0;
                for (const auto& c : s.mPolymorphic.mConcepts) {
                    if (count++) {
                        oss << " else ";
                    }
                    if (bTag) {
                        oss << "if constexpr (std::is_same_v<cc::remove_cvref_t<Tag>, "
                            << cpp.getDependentName(c.mTag) << ">) {\n";
                    } else {
                        oss << "if constexpr (std::is_same_v<cc::remove_cvref_t<ValueT>, "
                            << cpp.getDependentName(c.mValue) << ">) {\n";
                    }
                    {
                        INDENT();
                        OSS << "return holds_alternative<" << handleElemType(c, cn, true) << ">(g.vertices[v].handle);\n";
                    }
                    OSS << "}";
                }
                oss << " else {\n";
                OSS << "    []<bool flag = false>() {\n";
                if (bTag) {
                    OSS << "        static_assert(flag, \"Tag type is not in PolymorphicGraph\");\n";
                } else {
                    OSS << "        static_assert(flag, \"Value type is not in PolymorphicGraph\");\n";
                }
                OSS << "    }\n";
                OSS << "    ();\n";
                OSS << "}";
                oss << "\n";
                OSS << "return false;\n";
            }
            OSS << "}\n";
        };
        auto generateHold2 = [&](bool bTag) {
            oss << "\n";
            if (bTag) {
                OSS << "template <class Tag>\n";
            } else {
                OSS << "template <class ValueT>\n";
            }
            OSS << gNoDiscard << "inline bool\n";
            if (bTag) {
                OSS << "holds(" << name << "::vertex_descriptor v, ";
                oss << "const " << name << "& g) noexcept;\n";
            } else {
                OSS << "holds_alternative(" << name << "::vertex_descriptor v, ";
                oss << "const " << name << "& g) noexcept; // NOLINT\n";
            }
            if (false) {
                oss << "const " << name << "& g) noexcept { // NOLINT\n";
                {
                    INDENT();
                    if (bTag) {
                        OSS << "static_assert(false, \"Tag type is not in PolymorphicGraph\");\n";
                    } else {
                        OSS << "static_assert(false, \"Value type is not in PolymorphicGraph\");\n";
                    }
                    OSS << "return false;\n";
                }
                OSS << "}\n"; 
            }
            for (const auto& c : s.mPolymorphic.mConcepts) {
                oss << "\n";
                OSS << "template <>\n";
                OSS << gNoDiscard << "inline bool\n";
                if (bTag) {
                    OSS << "holds<" << cpp.getDependentName(c.mTag) << ">("
                        << name << "::vertex_descriptor v, ";
                    oss << "const " << name << "& g) noexcept {\n";
                } else {
                    OSS << "holds_alternative<" << cpp.getDependentName(c.mValue) << ">("
                        << name << "::vertex_descriptor v, ";
                    oss << "const " << name << "& g) noexcept { // NOLINT\n";
                }
                {
                    INDENT();
                    OSS << "return boost::variant2::holds_alternative<\n";
                    OSS << "    " << handleElemType(c, cn, false) << ">(\n";
                    OSS << "    g.vertices[v].handle);\n";
                }
                OSS << "}\n";
            }
        };

        if (true) {
            generateHold2(true);
            generateHold2(false);
        } else {
            generateHold(true);
            generateHold(false);
        }

        auto generateGet = [&](bool bConst, bool bTag) {
            std::string valueType = bTag ? "Tag" : "ValueT";
            oss << "\n";
            if (bTag) {
                OSS << "template <class Tag>\n";
                if (bConst) {
                    OSS << gNoDiscard << "inline const auto&\n";
                } else {
                    OSS << gNoDiscard << "inline auto&\n";
                }
            } else {
                OSS << "template <class ValueT>\n";
                if (bConst) {
                    OSS << gNoDiscard << "inline const ValueT&\n";
                } else {
                    OSS << gNoDiscard << "inline ValueT&\n";
                }
            }

            OSS << "get";
            if (bTag)
                oss << "_by_tag";
            oss << "(" << name << "::vertex_descriptor v, ";
            if (bConst)
                oss << "const ";
            oss << name << "& g) noexcept { // NOLINT\n";
            {
                INDENT();
                OSS << "using vertex_descriptor = " << name << "::vertex_descriptor;\n";
                OSS;
                int count = 0;
                for (const auto& c : s.mPolymorphic.mConcepts) {
                    if (count++) {
                        oss << " else ";
                    }
                    oss << "if constexpr (std::is_same_v<cc::remove_cvref_t<" << valueType << ">, ";
                    if (bTag) {
                        oss << cpp.getDependentName(c.mTag);
                    } else {
                        oss << cpp.getDependentName(c.mValue);
                    }
                    oss << ">) {\n";
                    {
                        INDENT();
                        if (s.isVector()) {
                            OSS;
                            if (bConst)
                                oss << "const ";
                            oss << "auto& handle = std::get<" << handleElemType(c, cn, true)
                                << ">(g.vertices[v].handle);\n";
                        } else {
                            OSS;
                            if (bConst)
                                oss << "const ";
                            oss << "auto& handle = std::get<" << handleElemType(c, cn, true)
                                << ">(static_cast<" << name << "::Vertex*>(v)->handle);\n";
                        }

                        if (c.mMemberName.empty()) {
                            OSS << "return handle.value;\n";
                        } else {
                            if (c.isVector()) {
                                OSS << "return g." << g.getMemberName(c.mMemberName, true) << "[handle.value];\n";
                            } else {
                                OSS << "return *handle.value;\n";
                            }
                        }
                    }
                    OSS << "}";
                }
                oss << " else {\n";
                OSS << "    []<bool flag = false>() {\n";
                OSS << "        static_assert(flag, \"Value type is not in PolymorphicGraph\");\n";
                OSS << "    }\n";
                OSS << "    ();\n";
                OSS << "}\n";
            }
            OSS << "}\n";
        };
        
        auto generateGet2 = [&](bool bConst, bool bTag) {
            std::string valueType = bTag ? "Tag" : "ValueT";
            if (bTag) {
                if (false) {
                    oss << "\n";
                    OSS << "template <class Tag>\n";
                    OSS << "inline void get(Tag, " << name << "::vertex_descriptor /*v*/, ";
                    if (bConst)
                        oss << "const ";
                    oss << name << "& /*g*/) {\n";
                    OSS << "    static_assert(false, \"Tag type is not in PolymorphicGraph\");\n";
                    OSS << "}\n";
                }
            } else {
                oss << "\n";
                OSS << "template <class ValueT>\n";
                if (bConst) {
                    OSS << gNoDiscard << "inline const ValueT&\n";
                } else {
                    OSS << gNoDiscard << "inline ValueT&\n";
                }
                OSS << "get(" << name << "::vertex_descriptor /*v*/, ";

                if (bConst)
                    oss << "const ";
                if (true) {
                    oss << name << "& /*g*/);\n";
                } else {
                    oss << name << "& /*g*/) {\n";
                    OSS << "    static_assert(false, \"Value type is not in PolymorphicGraph\");\n";
                    OSS << "}\n";
                }
            }

            for (const auto& c : s.mPolymorphic.mConcepts) {
                oss << "\n";
                if (!bTag) {
                    OSS << "template <>\n";
                }
                if (bConst) {
                    OSS << gNoDiscard << "inline const " << cpp.getDependentName(c.mValue) << "&\n";
                } else {
                    OSS << gNoDiscard << "inline " << cpp.getDependentName(c.mValue) << "&\n";
                }
                if (bTag) {
                    OSS << "get(";
                    oss << cpp.getDependentName(c.mTag) << " /*tag*/, ";
                } else {
                    OSS << "get<";
                    oss << cpp.getDependentName(c.mValue);
                    oss << ">(";
                }
                oss << name << "::vertex_descriptor v, ";
                if (bConst)
                    oss << "const ";
                oss << name << "& g) {\n";
                {
                    INDENT();
                    if (s.isVector()) {
                        OSS;
                        if (bConst)
                            oss << "const ";
                        oss << "auto& handle = boost::variant2::get<\n";
                        OSS << "    " << handleElemType(c, cn, false) << ">(\n";
                        OSS << "    g.vertices[v].handle);\n";
                    } else {
                        OSS;
                        if (bConst)
                            oss << "const ";
                        oss << "auto& handle = boost::variant2::get<\n";
                        OSS << "    " << handleElemType(c, cn, false) << ">(\n";
                        OSS << "    static_cast<" << name << "::Vertex*>(v)->handle);\n";
                    }
                    if (c.mMemberName.empty()) {
                        OSS << "return handle.value;\n";
                    } else {
                        if (c.isVector()) {
                            OSS << "return g." << g.getMemberName(c.mMemberName, true) << "[handle.value];\n";
                        } else {
                            OSS << "return *handle.value;\n";
                        }
                    }
                }
                OSS << "}\n";
            }
        };

        auto generateGetIf = [&](bool bConst) {
            oss << "\n";
            OSS << "template <class ValueT>\n";
            if (bConst) {
                OSS << gNoDiscard << "inline const ValueT*\n";
            } else {
                OSS << gNoDiscard << "inline ValueT*\n";
            }
            OSS << "get_if(" << name << "::vertex_descriptor v, ";
            if (bConst)
                oss << "const ";
            oss << name << "* pGraph) noexcept { // NOLINT\n";
            {
                INDENT();
                OSS << "using vertex_descriptor = " << name << "::vertex_descriptor;\n";
                if (bConst) {
                    OSS << "const ValueT* ptr       = nullptr;\n";
                } else {
                    OSS << "ValueT* ptr             = nullptr;\n";
                }
                oss << "\n";
                OSS << "if (!pGraph) {\n";
                OSS << "    return ptr;\n";
                OSS << "}\n";
                OSS << "auto& g = *pGraph;\n";
                oss << "\n";
                OSS;
                int count = 0;
                for (const auto& c : s.mPolymorphic.mConcepts) {
                    if (count++) {
                        oss << " else ";
                    }
                    oss << "if constexpr (std::is_same_v<cc::remove_cvref_t<ValueT>, "
                        << cpp.getDependentName(c.mValue) << ">) {\n";
                    {
                        INDENT();
                        if (s.isVector()) {
                            OSS << "auto* pHandle = std::get_if<" << handleElemType(c, cn, true)
                                << ">(&g.vertices[v].handle);\n";
                        } else {
                            OSS << "auto* pHandle = std::get_if<" << handleElemType(c, cn, true)
                                << ">(&(static_cast<" << name << "::Vertex*>(v)->handle));\n";
                        }

                        OSS << "if (pHandle) {\n";
                        if (c.mMemberName.empty()) {
                            OSS << "    ptr = &pHandle->value;\n";
                        } else {
                            if (c.isVector()) {
                                OSS << "    ptr = &g." << g.getMemberName(c.mMemberName, true) << "[pHandle->value];\n";
                            } else {
                                OSS << "    ptr = &(*(pHandle->value));\n";
                            }
                        }
                        OSS << "}\n";
                    }
                    OSS << "}";
                }
                oss << " else {\n";
                OSS << "    []<bool flag = false>() {\n";
                OSS << "        static_assert(flag, \"Value type is not in PolymorphicGraph\");\n";
                OSS << "    }\n";
                OSS << "    ();\n";
                OSS << "}";
                oss << "\n";
                OSS << "return ptr;\n";
            }
            OSS << "}\n";
        };

        auto generateGetIf2 = [&](bool bConst) {
            oss << "\n";
            OSS << "template <class ValueT>\n";
            if (bConst) {
                OSS << gNoDiscard << "inline const ValueT*\n";
            } else {
                OSS << gNoDiscard << "inline ValueT*\n";
            }
            OSS << "get_if(" << name << "::vertex_descriptor v, ";
            if (bConst)
                oss << "const ";
            if (true) {
                oss << name << "* pGraph) noexcept; // NOLINT\n";
            } else {
                oss << name << "* pGraph) noexcept { // NOLINT\n";
                OSS << "    static_assert(false, \"Value type is not in PolymorphicGraph\");\n";
                OSS << "}\n";
            }

            for (const auto& c : s.mPolymorphic.mConcepts) {
                auto valueType = cpp.getDependentName(c.mValue);
                oss << "\n";
                OSS << "template <>\n";
                if (bConst) {
                    OSS << gNoDiscard << "inline const " << valueType << "*\n";
                } else {
                    OSS << gNoDiscard << "inline " << valueType << "*\n";
                }
                OSS << "get_if<" << cpp.getDependentName(c.mValue) << ">(" << name << "::vertex_descriptor v, ";
                if (bConst)
                    oss << "const ";
                oss << name << "* pGraph) noexcept { // NOLINT\n";
                {
                    INDENT();
                    if (bConst) {
                        OSS << "const " << valueType << "* ptr = nullptr;\n";
                    } else {
                        OSS << valueType << "* ptr = nullptr;\n";
                    }
                    OSS << "if (!pGraph) {\n";
                    OSS << "    return ptr;\n";
                    OSS << "}\n";
                    OSS;
                    if (bConst)
                        oss << "const ";
                    oss << "auto& g       = *pGraph;\n";
                    if (s.isVector()) {
                        OSS;
                        if (bConst)
                            oss << "const ";
                        oss << "auto* pHandle = boost::variant2::get_if<\n";
                        OSS << "    " << handleElemType(c, cn, false) << ">(\n";
                        OSS << "    &g.vertices[v].handle);\n";
                    } else {
                        OSS;
                        if (bConst)
                            oss << "const ";
                        oss << "auto* pHandle = boost::variant2::get_if<\n";
                        OSS << "    " << handleElemType(c, cn, false) << ">(\n";
                        OSS << "    &(static_cast<" << name << "::Vertex*>(v)->handle));\n";
                    }
                    OSS << "if (pHandle) {\n";
                    if (c.mMemberName.empty()) {
                        OSS << "    ptr = &pHandle->value;\n";
                    } else {
                        if (c.isVector()) {
                            OSS << "    ptr = &g." << g.getMemberName(c.mMemberName, true) << "[pHandle->value];\n";
                        } else {
                            OSS << "    ptr = &(*(pHandle->value));\n";
                        }
                    }
                    OSS << "}\n";
                    OSS << "return ptr;\n";
                }
                OSS << "}\n";
            }
        };

        if (true) {
            generateGet2(false, false);
            generateGet2(true, false);
            generateGet2(false, true);
            generateGet2(true, true);
        } else {
            generateGet(false, false);
            generateGet(true, false);
            generateGet(false, true);
            generateGet(true, true);
        }

        if (true) {
            generateGetIf2(false);
            generateGetIf2(true);
        } else {
            generateGetIf(false);
            generateGetIf(true);
        }
    } // Polymorphic

    bool hasPropertyMap = s.hasVertexProperty() || !s.mComponents.empty();

    if (hasPropertyMap) {
        oss << "\n";
        oss << "// Vertex Constant Getter\n";
        OSS << "template <class Tag>\n";
        OSS << gNoDiscard << "inline decltype(auto)\n";
        OSS << "get(Tag tag, const " << name << "& g, " << name << "::vertex_descriptor v) noexcept {\n";
        OSS << "    return get(get(tag, g), v);\n";
        OSS << "}\n";

        
        if (s.hasVertexProperty() || !s.mComponents.empty()) {
            oss << "\n";
            oss << "// Vertex Mutable Getter\n";
            OSS << "template <class Tag>\n";
            OSS << gNoDiscard << "inline decltype(auto)\n";
            OSS << "get(Tag tag, " << name << "& g, " << name << "::vertex_descriptor v) noexcept {\n";
            OSS << "    return get(get(tag, g), v);\n";
            OSS << "}\n";

            oss << "\n";
            oss << "// Vertex Setter\n";
            OSS << "template <class Tag, class... Args>\n";
            OSS << "inline void put(\n";
            OSS << "    Tag tag, " << name << "& g,\n";
            OSS << "    " << name << "::vertex_descriptor v,\n";
            OSS << "    Args&&... args) {\n";
            OSS << "    put(get(tag, g), v, std::forward<Args>(args)...);\n";
            OSS << "}\n";
        }
    }

    if (s.hasEdgeProperty()) {
        //-----------------------------------------
        // all
        //-----------------------------------------
        oss << "\n";
        oss << "// Edge All\n";
        OSS << "inline boost::property_map<" << name << ", boost::edge_all_t>::const_type\n";
        OSS << "get(boost::edge_all_t /*tag*/, const " << name << "& g) noexcept {\n";
        {
            INDENT();
            OSS << "return {g};\n";
        }
        OSS << "}\n";

        oss << "\n";
        OSS << "inline boost::property_map<" << name << ", boost::edge_all_t>::type\n";
        OSS << "get(boost::edge_all_t /*tag*/, " << name << "& g) noexcept {\n";
        {
            INDENT();
            OSS << "return {g};\n";
        }
        OSS << "}\n";
        //-----------------------------------------
        // bundle
        //-----------------------------------------
        oss << "\n";
        oss << "// Edge Bundle\n";
        OSS << "inline boost::property_map<" << name << ", boost::edge_bundle_t>::const_type\n";
        OSS << "get(boost::edge_bundle_t /*tag*/, const " << name << "& g) noexcept {\n";
        {
            INDENT();
            OSS << "return {g};\n";
        }
        OSS << "}\n";

        oss << "\n";
        OSS << "inline boost::property_map<" << name << ", boost::edge_bundle_t>::type\n";
        OSS << "get(boost::edge_bundle_t /*tag*/, " << name << "& g) noexcept {\n";
        {
            INDENT();
            OSS << "return {g};\n";
        }
        OSS << "}\n";
        //-----------------------------------------
        // bundle member
        //-----------------------------------------
        auto epID = locate(s.mEdgeProperty, g);
        if (g.isComposition(epID)) {
            auto edgeProperty = cpp.getDependentName(s.mEdgeProperty);
            oss << "\n";
            oss << "// Edge BundleMember\n";
            OSS << "template <class T>\n";
            OSS << "inline typename boost::property_map<" << name << ", T " << edgeProperty
                << "::*>::const_type\n";
            OSS << "get(T " << edgeProperty << "::*memberPointer, const " << name << "& g";
            oss << ") noexcept {\n";
            {
                INDENT();
                OSS << "return {g, memberPointer};\n";
            }
            OSS << "}\n";

            oss << "\n";
            OSS << "template <class T>\n";
            OSS << "inline typename boost::property_map<" << name << ", T " << edgeProperty
                << "::*>::type\n";
            OSS << "get(T " << edgeProperty << "::*memberPointer, " << name << "& g";
            oss << ") noexcept {\n";
            {
                INDENT();
                OSS << "return {g, memberPointer};\n";
            }
            OSS << "}\n";
        }

        // put
        oss << "\n";
        oss << "// Edge Constant Getter\n";
        OSS << "template <class Tag>\n";
        OSS << gNoDiscard << "inline decltype(auto)\n";
        OSS << "get(Tag tag, const " << name << "& g, " << name << "::edge_descriptor e) noexcept {\n";
        OSS << "    return get(get(tag, g), e);\n";
        OSS << "}\n";

        oss << "\n";
        oss << "// Edge Mutable Getter\n";
        OSS << "template <class Tag>\n";
        OSS << gNoDiscard << "inline decltype(auto)\n";
        OSS << "get(Tag tag, " << name << "& g, " << name << "::edge_descriptor e) noexcept {\n";
        OSS << "    return get(get(tag, g), e);\n";
        OSS << "}\n";

        oss << "\n";
        oss << "// Edge Setter\n";
        OSS << "template <class Tag, class... Args>\n";
        OSS << "inline void put(\n";
        OSS << "    Tag tag, " << name << "& g,\n";
        OSS << "    " << name << "::edge_descriptor e,\n";
        OSS << "    Args&&... args) {\n";
        OSS << "    put(get(tag, g), e, std::forward<Args>(args)...);\n";
        OSS << "}\n";
    }

    if (!s.mVertexMaps.empty()) {

        for (const auto& map : s.mVertexMaps) {
            const auto& mapMemberName = g.getMemberName(map.mMemberName, true);
            oss << "\n";
            OSS << "// UuidGraph\n";
            OSS << gNoDiscard << "inline " << name << "::vertex_descriptor\n";
            OSS << "vertex(const " << cpp.getDependentName(map.mKeyType)
                << "& key, const " << name << "& g) {\n";
            OSS << "    return g." << mapMemberName << ".at(key);\n";
            OSS << "}\n";

            oss << "\n";
            OSS << "template <class KeyLike>\n";
            OSS << gNoDiscard << "inline " << name << "::vertex_descriptor\n";
            OSS << "vertex(const KeyLike& key, const " << name << "& g) {\n";
            {
                INDENT();
                OSS << "const auto& index = g." << mapMemberName << ";\n";
                OSS << "auto iter = index.find(key);\n";
                OSS << "if (iter == index.end()) {\n";
                OSS << "    throw std::out_of_range(\"at(key, " << name << ") out of range\");\n";
                OSS << "}\n";
                OSS << "return iter->second;\n";
            }
            OSS << "}\n";

            oss << "\n";
            OSS << "template <class KeyLike>\n";
            OSS << gNoDiscard << "inline " << name << "::vertex_descriptor\n";
            OSS << "findVertex(const KeyLike& key, const " << name << "& g) noexcept {\n";
            {
                INDENT();
                OSS << "const auto& index = g." << mapMemberName << ";\n";
                OSS << "auto iter = index.find(key);\n";
                OSS << "if (iter == index.end()) {\n";
                OSS << "    return " << name << "::null_vertex();\n";
                OSS << "}\n";
                OSS << "return iter->second;\n";
            }
            OSS << "}\n";

            oss << "\n";
            OSS << gNoDiscard << "inline bool\n";
            OSS << "contains(const " << cpp.getDependentName(map.mKeyType)
                << "& key, const " << name << "& g) noexcept {\n";
            {
                INDENT();
                if (true) {
                    OSS << "auto iter = g." << mapMemberName << ".find(key);\n";
                    OSS << "return iter != g." << mapMemberName << ".end();\n";
                } else {
                    // c++17
                    OSS << "return g." << mapMemberName << ".contains(key);\n";
                }
            }
            OSS << "}\n";

            oss << "\n";
            OSS << "template <class KeyLike>\n";
            OSS << gNoDiscard << "inline bool\n";
            OSS << "contains(const KeyLike& key, const " << name << "& g) noexcept {\n";
            {
                INDENT();
                if (true) {
                    OSS << "auto iter = g." << mapMemberName << ".find(key);\n";
                    OSS << "return iter != g." << mapMemberName << ".end();\n";
                } else {
                    // c++17
                    OSS << "return g." << mapMemberName << ".contains(key);\n";
                }
            }
            OSS << "}\n";
        }
    }

    if (s.mAddressable) {
        std::pmr::string charName("char",scratch);
        if (s.mAddressableConcept.mUtf8) {
            charName = "char8_t";
        }
        // AddressableGraph
        oss << "\n";
        OSS << "// AddressableGraph\n";
        if (false) {
            OSS << gNoDiscard << "inline std::ptrdiff_t\n";
            OSS << "path_length(" << name << "::vertex_descriptor u, const " << name << "& g) noexcept { // NOLINT\n";
            {
                INDENT();
                OSS << "return impl::pathLength(u, g);\n";
            }
            OSS << "}\n";

            oss << "\n";
            OSS << "template <class Allocator>\n";
            OSS << "inline void\n";
            OSS << "path_composite( // NOLINT\n";
            OSS << "    std::basic_string<" << charName
                << ", std::char_traits<" << charName << ">, Allocator>& str,\n";
            OSS << "    std::ptrdiff_t& sz, " << name << "::vertex_descriptor u,\n";
            OSS << "    const " << name << "& g) noexcept {\n";
            {
                INDENT();
                OSS << "impl::pathComposite(str, sz, u, g);\n";
            }
            OSS << "}\n";
        }
    }

    if (s.mAddressable) {
        auto generatePath = [&](bool general, bool pmr, bool utf8) {
            oss << "\n";
            std::pmr::string charName(scratch);
            std::pmr::string stringName(scratch);
            std::pmr::string viewName(scratch);
            if (utf8) {
                charName = "char8_t";
                viewName = "std::u8string_view";
                if (pmr)
                    stringName = "std::pmr::u8string";
                else
                    stringName = "std::u8string";
            } else {
                charName = "char";
                viewName = "boost::string_view";
                if (pmr)
                    stringName = "ccstd::pmr::string";
                else
                    stringName = "ccstd::string";
            }
            if (general) {
                OSS << "template <class Allocator>\n";
                OSS << "inline const std::basic_string<" << charName << ", std::char_traits<" << charName << ">, Allocator>&\n";
                OSS << "getPath(\n";
                OSS << "    std::basic_string<" << charName << ", std::char_traits<" << charName << ">, Allocator>& output,\n";
            } else {
                OSS << gNoDiscard << "inline " << stringName << "\n";
                OSS << "getPath(\n";
            }
            OSS << "    " << name << "::vertex_descriptor u" << 0;

            oss << ", const " << name << "& g,\n";
            if (!general && pmr) {
                OSS << "    boost::container::pmr::memory_resource* mr, ";
            } else {
                OSS << "    ";
            }
            oss << viewName << " prefix = {}, " << name << "::vertex_descriptor parent = "
                << name << "::null_vertex()) {\n";
            {
                INDENT();
                if (general) {
                    OSS << "output.clear();\n";
                } else {
                    if (pmr) {
                        OSS << stringName << " output(mr);\n";
                    } else {
                        OSS << stringName << " output;\n";
                    }
                }

                if (general) {
                    auto graphName = camelToVariable(name, scratch);
                    OSS << "const auto sz0 = static_cast<std::ptrdiff_t>(prefix.size());\n";
                    OSS << "auto       sz  = sz0;\n";
                    oss << "\n";
                    OSS << "const auto& " << graphName << " = g;\n";
                    OSS << "sz += impl::pathLength(u" << 0 << ", " << graphName << ", parent);\n";
                    oss << "\n";
                    OSS << "output.resize(sz);\n";

                    oss << "\n";
                    OSS << "impl::pathComposite(output, sz, u" << 0 << ", " << graphName << ", parent);\n";
                    OSS << "CC_ENSURES(sz >= sz0);\n";

                    oss << "\n";
                    OSS << "CC_ENSURES(sz == sz0);\n";
                    OSS << "std::copy(prefix.begin(), prefix.end(), output.begin());\n";
                    oss << "\n";
                    OSS << "return output;\n";
                } else {
                    OSS << "getPath(output, u0, g, prefix, parent);\n";
                    OSS << "return output;\n";
                }
            }
            OSS << "}\n";
        };

        generatePath(true, false, s.mAddressableConcept.mUtf8);
        generatePath(false, false, s.mAddressableConcept.mUtf8);
        if (true) {
            generatePath(false, true, s.mAddressableConcept.mUtf8);
        }

        auto generateParentPath = [&](bool general, bool pmr, bool utf8) {
            oss << "\n";
            std::string charName;
            std::string stringName;
            std::string viewName;
            if (utf8) {
                charName = "char8_t";
                viewName = "std::u8string_view";
                if (pmr)
                    stringName = "std::pmr::u8string";
                else
                    stringName = "std::u8string";
            } else {
                charName = "char";
                viewName = "boost::string_view";
                if (pmr)
                    stringName = "ccstd::pmr::string";
                else
                    stringName = "ccstd::string";
            }
            if (general) {
                OSS << "template <class Allocator>\n";
                OSS << "inline const std::basic_string<" << charName << ", std::char_traits<" << charName << ">, Allocator>&\n";
                OSS << "getPath(\n";
                OSS << "    std::basic_string<" << charName << ", std::char_traits<" << charName << ">, Allocator>& output,\n";
                OSS << "    ";
            } else {
                OSS << gNoDiscard << "inline " << stringName << "\n";
                OSS << "getPath(";
            }
            oss << name << "::vertex_descriptor parent, ";
            oss << viewName << " name, const " << name << "& g";

            if (!general) {
                if (pmr) {
                    oss << ", boost::container::pmr::memory_resource* mr";
                }
            }

            oss << ") {\n";
            {
                INDENT();
                if (general) {
                    OSS << "output.clear();\n";
                } else {
                    if (pmr) {
                        OSS << stringName << " output(mr);\n";
                    } else {
                        OSS << stringName << " output;\n";
                    }
                }

                if (general) {
                    OSS << "auto sz = impl::pathLength(parent, g);\n";
                    OSS << "output.resize(sz + name.size() + 1);\n";
                    OSS << "output[sz] = '/';\n";
                    OSS << "std::copy(name.begin(), name.end(), output.begin() + sz + 1);\n";
                    OSS << "impl::pathComposite(output, sz, parent, g);\n";
                    OSS << "CC_ENSURES(sz == 0);\n";
                    OSS << "return output;\n";
                } else {
                    OSS << "getPath(output, parent, name, g);\n";
                    OSS << "return output;\n";
                }
            }
            OSS << "}\n";
        };

        generateParentPath(true, false, s.mAddressableConcept.mUtf8);
        generateParentPath(false, false, s.mAddressableConcept.mUtf8);
        if (true) {
            generateParentPath(false, true, s.mAddressableConcept.mUtf8);
        }
    }

    if (s.mAddressable) {
        std::pmr::string stringName(scratch);
        std::pmr::string viewName(scratch);
        if (s.mAddressableConcept.mUtf8) {
            stringName = "std::pmr::u8string";
            viewName = "std::u8string_view";
        } else {
            stringName = "std::pmr::string";
            viewName = "boost::string_view";
        }
        oss << "\n";
        OSS << gNoDiscard << "inline " << name << "::vertex_descriptor\n";
        OSS << "locate(" << viewName << " absolute, const " << name << "& g) noexcept {\n";
        {
            INDENT();
            visit(
                overload(
                    [&](Direct_) {
                        Expects(false);
                    },
                    [&](Map_) {
                        OSS << "auto iter = g." << g.getMemberName(s.mAddressableConcept.mMemberName, true) << ".find(absolute);\n";
                        OSS << "if (iter != g." << g.getMemberName(s.mAddressableConcept.mMemberName, true) << ".end()) {\n";
                        OSS << "    return iter->second;\n";
                        OSS << "}\n";
                        OSS << "return " << name << "::null_vertex();\n";
                    }),
                s.mAddressableConcept.mType);
        }
        OSS << "};\n";

        auto generateLocate = [&](bool pmr) {
            oss << "\n";
            OSS << gNoDiscard << "inline " << name << "::vertex_descriptor\n";
            OSS << "locate(" << name << "::vertex_descriptor u, " << viewName << " relative, const " << name << "& g";
            if (pmr) {
                oss << ",\n";
                OSS << "    std::pmr::memory_resource* mr";
            }
            oss << ") {\n";
            {
                INDENT();
                visit(
                    overload(
                        [&](Direct_) {
                            Expects(false);
                        },
                        [&](Map_) {
                            OSS << "CC_EXPECTS(!relative.starts_with('/'));\n";
                            OSS << "CC_EXPECTS(!relative.ends_with('/'));\n";
                            if (pmr) {
                                OSS << "auto key = getPath(u, relative, g, mr);\n";
                            } else {
                                OSS << "auto key = getPath(u, relative, g);\n";
                            }
                            OSS << "impl::cleanPath(key);\n";
                            OSS << "return locate(key, g);\n";
                        }),
                    s.mAddressableConcept.mType);
            }
            OSS << "};\n";
        };

        generateLocate(false);
        if (false) {
            generateLocate(true);
        }

        oss << "\n";
        OSS << gNoDiscard << "inline bool\n";
        OSS << "contains(" << viewName << " absolute, const " << name << "& g) noexcept {\n";
        OSS << "    return locate(absolute, g) != " << name << "::null_vertex();\n";
        OSS << "}\n";

        if (s.isPolymorphic()) {
            oss << "\n";
            OSS << "template <class ValueT>\n";
            OSS << gNoDiscard << "inline ValueT&\n";
            if (s.mAddressableConcept.mUtf8) {
                OSS << "get(std::u8string_view pt, " << name << "& g) {\n";
            } else {
                OSS << "get(boost::string_view pt, " << name << "& g) {\n";
            }
            OSS << "    auto v = locate(pt, g);\n";
            OSS << "    if (v == " << name << "::null_vertex()) {\n";
            OSS << "        throw std::out_of_range(\"at " << name << "\");\n";
            OSS << "    }\n";
            OSS << "    return get<ValueT>(v, g);\n";
            OSS << "}\n";

            oss << "\n";
            OSS << "template <class ValueT>\n";
            OSS << gNoDiscard << "inline const ValueT&\n";
            if (s.mAddressableConcept.mUtf8) {
                OSS << "get(std::u8string_view pt, const " << name << "& g) {\n";
            } else {
                OSS << "get(boost::string_view pt, const " << name << "& g) {\n";
            }
            OSS << "    auto v = locate(pt, g);\n";
            OSS << "    if (v == " << name << "::null_vertex()) {\n";
            OSS << "        throw std::out_of_range(\"at " << name << "\");\n";
            OSS << "    }\n";
            OSS << "    return get<ValueT>(v, g);\n";
            OSS << "}\n";

            oss << "\n";
            OSS << "template <class ValueT>\n";
            OSS << gNoDiscard << "inline ValueT*\n";
            if (s.mAddressableConcept.mUtf8) {
                OSS << "get_if(std::u8string_view pt, " << name << "* pGraph) noexcept { // NOLINT\n";
            } else {
                OSS << "get_if(boost::string_view pt, " << name << "* pGraph) noexcept { // NOLINT\n";
            }
            OSS << "    if (pGraph) {\n";
            OSS << "        auto v = locate(pt, *pGraph);\n";
            OSS << "        if (v != " << name << "::null_vertex()) {\n";
            OSS << "            return get_if<ValueT>(v, pGraph);\n";
            OSS << "        }\n";
            OSS << "    }\n";
            OSS << "    return nullptr;\n";
            OSS << "}\n";

            oss << "\n";
            OSS << "template <class ValueT>\n";
            OSS << gNoDiscard << "inline const ValueT*\n";
            if (s.mAddressableConcept.mUtf8) {
                OSS << "get_if(std::u8string_view pt, const " << name << "* pGraph) noexcept { // NOLINT\n";
            } else {
                OSS << "get_if(boost::string_view pt, const " << name << "* pGraph) noexcept { // NOLINT\n";
            }
            OSS << "    if (pGraph) {\n";
            OSS << "        auto v = locate(pt, *pGraph);\n";
            OSS << "        if (v != " << name << "::null_vertex()) {\n";
            OSS << "            return get_if<ValueT>(v, pGraph);\n";
            OSS << "        }\n";
            OSS << "    }\n";
            OSS << "    return nullptr;\n";
            OSS << "}\n";
        }
    }

    if (s.mVertexList && s.mMutableGraphVertex && s.mAddressable) {
        oss << "\n";
        OSS << "// MutableGraph(Vertex)\n";
        OSS << "inline void addPathImpl(" << name << "::vertex_descriptor u, "
            << name << "::vertex_descriptor v, " << name << "& g) { // NOLINT\n";
        {
            INDENT();
            OSS << "// add to parent\n";
            OSS << "if (u != " << name << "::null_vertex()) {\n";
            {
                INDENT();
                copyString(oss, space, generateAddEdge(false, false, false, true, false));
            }
            OSS << "}\n";
            if (s.isVector()) {
                visit(
                    overload(
                        [&](Direct_) {
                        },
                        [&](Map_) {
                            oss << "\n";
                            OSS << "// add to external path index\n";
                            if (g.isPathPmr(s)) {
                                OSS << "auto pathName = getPath(v, g, g."
                                    << g.getMemberName(s.mAddressableConcept.mMemberName, true)
                                    << ".get_allocator().resource());\n";
                            } else {
                                OSS << "auto pathName = getPath(v, g);\n";
                            }
                            OSS << "auto res      = g." << g.getMemberName(s.mAddressableConcept.mMemberName, true)
                                << ".emplace(std::move(pathName), v);\n";
                            OSS << "CC_ENSURES(res.second);\n";
                            if (s.mAddressableConcept.mPathPropertyMap) {
                                if (s.isAliasGraph()) {
                                    OSS << "g.vertices[u].mPathIterator = res.first;\n";
                                } else {
                                    OSS << "g.objects[u].mPathIterator = res.first;\n";
                                }
                            }
                        }),
                    s.mAddressableConcept.mType);
            }
        }
        OSS << "}\n";

        if (holds_alternative<Map_>(s.mAddressableConcept.mType)) {
            oss << "\n";
            OSS << "inline void removePathImpl(" << name << "::vertex_descriptor u, " << name << "& g) noexcept {\n";
            {
                INDENT();
                const auto& member = g.getMemberName(s.mAddressableConcept.mMemberName, true);
                if (s.mAddressableConcept.mPathPropertyMap) {
                    visit(
                        overload(
                            [&](Direct_) {
                                Expects(false);
                            },
                            [&](Map_) {
                                if (s.isVector()) {
                                    if (s.isAliasGraph()) {
                                        OSS << "g.pathIndex.erase(g.vertices[u].mPathIterator);\n";
                                    } else {
                                        OSS << "g.pathIndex.erase(g.objects[u].mPathIterator);\n";
                                    }
                                } else {
                                    if (s.isAliasGraph()) {
                                        OSS << "g.pathIndex.erase(static_cast<const " << name << "::Vertex*>(u)->mPathIterator);\n";
                                    } else {
                                        OSS << "g.pathIndex.erase(static_cast<const " << name << "::object_type*>(u)->mPathIterator);\n";
                                    }
                                }
                            }),
                        s.mAddressableConcept.mType);
                } else {
                    if (bPmr) {
                        OSS << "// notice: here we use ccstd::string, not std::pmr::string\n";
                        OSS << "// we do not want to increase the memory of g\n";
                        OSS << "auto pathName = getPath(u, g);\n";
                    } else {
                        OSS << "auto pathName = getPath(u, g);\n";
                    }
                    if (s.mAddressableConcept.mUtf8) {
                        OSS << "auto iter     = g." << member << ".find(std::u8string_view(pathName));\n";
                    } else {
                        OSS << "auto iter     = g." << member << ".find(boost::string_view(pathName));\n";
                    }
                    OSS << "CC_EXPECTS(iter != g." << member << ".end());\n";
                    OSS << "g." << member << ".erase(iter);\n";
                }
                if (s.isVector()) {
                    OSS << "for (auto&& nvp : g." << member << ") {\n";
                    {
                        INDENT();
                        OSS << "auto& v = nvp.second;\n";
                        OSS << "if (v > u) {\n";
                        OSS << "    --v;\n";
                        OSS << "}\n";
                    }
                    OSS << "}\n";
                }
            }
            OSS << "}\n";
        }

        oss << "\n";
        copyString(oss, space, clearVertex(s, name, scratch));

        if (s.isPolymorphic()) {
            oss << "\n";
            copyString(oss, space, removePolymorphicType(g, *this, s, name, scratch));
        }

        oss << "\n";
        copyString(oss, space, removeVertex(*this, s, name, scratch));
    }

    if (s.mVertexList && s.mMutableGraphVertex && s.mReferenceGraph
        && (s.hasVertexProperty() || !s.mComponents.empty() || s.isPolymorphic())) {
        oss << "\n";
        OSS << "// MutablePropertyGraph(Vertex)\n";
        copyString(oss, space, addVertex(true, false, true));
        oss << "\n";
        copyString(oss, space, addVertex(true, true, true));
    }

    if (s.mVertexList) {
        auto addDefaultVertex = [&](bool str, bool cstr) {
            if (s.isPolymorphic()) {
                OSS << "template <class Tag>\n";
            }
            OSS << "inline " << name << "::vertex_descriptor\n";
            OSS << "add_vertex(";
            oss << name << "& g";

            if (s.isPolymorphic()) {
                oss << ", Tag t";
            }

            if (s.hasVertexProperty()) {
                oss << ", " << name << "::vertex_property_type vp";
            }

            auto generateNamed = [&]() {
                std::string strName;
                std::string u8Name;
                if (cstr) {
                    strName = ", const char* name";
                    u8Name = ", const char8_t* name";
                } else if (str) {
                    if (g.isPathPmr(s)) {
                        // TODO: cocos workaround
                        strName = ", ccstd::pmr::string&& name";
                        u8Name = ", std::pmr::u8string&& name";
                    } else {
                        strName = ", ccstd::string&& name";
                        u8Name = ", std::u8string&& name";
                    }
                } else {
                    strName = ", boost::string_view name";
                    u8Name = ", std::u8string_view name";
                }
                if (!s.mNamedConcept.mComponent) {
                    Expects(vpID != g.null_vertex());
                    if (s.mNamedConcept.mComponentMemberName.empty()) {
                        Expects(g.isString(vpID));
                        if (g.isUtf8(vpID)) {
                            oss << u8Name;
                        } else {
                            oss << strName;
                        }
                    } else {
                        Expects(g.hasString(vpID));
                        auto stringID = g.getMemberType(vpID, s.mNamedConcept.mComponentMemberName);
                        Expects(g.isString(stringID));
                        if (g.isUtf8(stringID)) {
                            oss << u8Name;
                        } else {
                            oss << strName;
                        }
                    }
                } else {
                    const auto& c = s.getComponent(s.mNamedConcept.mComponentName);
                    auto comID = locate(c.mValuePath, g);
                    if (s.mNamedConcept.mComponentMemberName.empty()) {
                        Expects(g.isString(comID));
                        if (g.isUtf8(comID)) {
                            oss << u8Name;
                        } else {
                            oss << strName;
                        }
                    } else {
                        Expects(g.hasString(comID));
                        auto stringID = g.getMemberType(comID, s.mNamedConcept.mComponentMemberName);
                        Expects(g.isString(stringID));
                        if (g.isUtf8(stringID)) {
                            oss << u8Name;
                        } else {
                            oss << strName;
                        }
                    }
                }
            };

            auto generateUuid = [&]() {
                int count = 0;
                for (const auto& map : s.mVertexMaps) {
                    if (s.mNamed && s.mNamedConcept.mComponentName == map.mComponentName) {
                        continue;
                    }
                    oss << ", const " << cpp.getDependentName(map.mKeyType) << "& key";
                    if (count++)
                        oss << count;
                }
            };

            if (s.mNamed && !s.mVertexMaps.empty()) {
                generateNamed();
                generateUuid();
            } else if (s.mNamed) {
                generateNamed();
            } else if (!s.mVertexMaps.empty()) {
                generateUuid();
            }

            if (s.mAddressable) {
                oss << ", " << name << "::vertex_descriptor parentID = " << name << "::null_vertex()";
            }
            oss << ") { // NOLINT\n";
            {
                INDENT();
                int numSpaces = 0;
                OSS << "return addVertex(\n";
                {
                    INDENT();
                    if (s.isPolymorphic()) {
                        OSS << "t,\n";
                    } else {
                        OSS << "std::piecewise_construct,\n";
                    }
                    if (s.hasVertexProperty()) {
                        OSS << "std::forward_as_tuple(std::move(vp)),\n";
                    }
                    for (const auto& c : s.mComponents) {
                        if (s.mNamed && s.mNamedConcept.mComponentName == c.mName) {
                            if (cstr || !str) {
                                OSS << "std::forward_as_tuple(name), // " << g.getMemberName(c.mMemberName, true) << "\n";
                                numSpaces = 4;
                            } else {
                                OSS << "std::forward_as_tuple(std::move(name)), // " << g.getMemberName(c.mMemberName, true) << "\n";
                                numSpaces = 15;
                            }
                        } else {
                            bool isKey = false;
                            for (const auto& map : s.mVertexMaps) {
                                if (map.mComponentName == c.mName) {
                                    OSS << "std::forward_as_tuple(key)," << std::string(numSpaces, ' ') << " // " << g.getMemberName(c.mMemberName, true) << "\n";
                                    isKey = true;
                                    break;
                                }
                            }
                            if (!isKey) {
                                OSS << "std::forward_as_tuple()," << std::string(numSpaces, ' ') << " // " << g.getMemberName(c.mMemberName, true) << "\n";
                            }
                        }
                    }
                    if (s.isPolymorphic()) {
                        OSS << "std::forward_as_tuple()," << std::string(numSpaces, ' ') << " // PolymorphicType\n";
                    }
                    OSS << "g";
                    if (s.mAddressable)
                        oss << ", parentID";
                    oss << ");\n";
                }
            }
            OSS << "}\n";
        };

        oss << "\n";
        OSS << "// MutableGraph(Vertex)\n";
        if (s.mComponents.empty() && !s.isPolymorphic() && !s.mAddressable && !s.mNamed && s.mVertexMaps.empty()) {
            copyString(oss, space, addVertex(false, false, true));
        } else {
            if (s.mNamed) {
                //oss << "\n";
                addDefaultVertex(true, false);
                oss << "\n";
                addDefaultVertex(false, true);
            } else {
                // TODO: cocos workaround
                addDefaultVertex(false, false);
            }
        }
    }

    return oss.str();
}

}
