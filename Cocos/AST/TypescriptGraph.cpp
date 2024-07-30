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
#include "TypescriptGraph.h"
#include "BuilderMacros.h"
#include "BuilderTypes.h"
#include "SyntaxUtils.h"
#include "SyntaxGraphs.h"

namespace Cocos::Meta {

namespace {

constexpr bool gThrow = true;

std::pmr::string generatePushIndicenceList(const Graph& g,
    std::string_view el, std::string_view outEdgeType,
    std::string_view v, std::string_view edge,
    std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios_base::out, scratch);
    std::pmr::string space(scratch);

    if (edge.empty()) {
        OSS << el << ".push(new " << outEdgeType << "(" << v << "));\n";
    } else {
        OSS << el << ".push(new " << outEdgeType << "(" << v << ", " << edge << "));\n";
    }

    return oss.str();
}

std::string generateAddEdge(const Graph* ptr, std::string_view name,
    std::string_view edgeType, std::string_view edgeDescType, std::string_view outEdgeType,
    bool property, bool addressable,
    std::pmr::memory_resource* scratch) {
    const auto& g = *ptr;
    const auto& s = *ptr;
    std::ostringstream oss;
    std::string space;

    bool directed = true;
    bool bidirectional = true;
    bool edgeProperty = !ptr->mEdgeProperty.empty();
    bool mutablePropertyGraph = true;

    if (addressable) {
        directed = true;
        bidirectional = true;
        edgeProperty = false;
        mutablePropertyGraph = false;
    }

    bool hasProperty = mutablePropertyGraph && edgeProperty && property;
    bool isDirectedOnly = directed && !bidirectional;
    bool isBidirectionalOnly = directed && bidirectional;
    bool isEdgeListNotNeeded = isDirectedOnly || (isBidirectionalOnly && !edgeProperty);
    bool needEdgeList = !isEdgeListNotNeeded;

    std::string_view edge;
    // precondition, add edge
    if (needEdgeList) {
        edge = "edge";

        OSS << "// insert new edge\n";
        OSS << "const edge = new " << edgeType << "(u, v, p);\n";
        OSS << "this._edges.add(edge);\n";
    }

    auto outEdgeList = g.getTypescriptVertexDereference("u", scratch)
                           .append(".")
                           .append(g.getTypescriptOutEdgeList(addressable));

    auto inEdgeList = g.getTypescriptVertexDereference("v", scratch)
                          .append(".")
                          .append(g.getTypescriptInEdgeList(addressable));

    OSS << "// update in/out edge list\n";
    copyString(oss, space, generatePushIndicenceList(g, outEdgeList, outEdgeType, "v", edge, scratch));
    copyString(oss, space, generatePushIndicenceList(g, inEdgeList, outEdgeType, "u", edge, scratch));

    OSS << "return new " << edgeDescType << "(u, v";
    if (needEdgeList) {
        oss << ", edge";
    }
    oss << ");\n";

    return oss.str();
}

void outputRemoveEdge(std::ostream& oss, std::pmr::string& space, const Graph& s,
    std::string_view vertexDescType,
    std::string_view edgeType,
    bool bReferenceGraph,
    std::pmr::memory_resource* scratch) {
    OSS << "const u = e.source as " << vertexDescType << ";\n";
    OSS << "const v = e.target as " << vertexDescType << ";\n";

    auto outEdgeList = s.getTypescriptOutEdgeList(bReferenceGraph);
    auto inEdgeList = s.getTypescriptInEdgeList(bReferenceGraph);

    {
        OSS << "const source = " << s.getTypescriptVertexDereference("u", scratch) << ";\n";
        if (s.needEdgeList() && !bReferenceGraph) {
            OSS << "for (let i = 0; i !== source." << outEdgeList << ".length;) {\n";
            OSS << "    if (source." << outEdgeList << "[i].edge === e.edge) {\n";
            OSS << "        source." << outEdgeList << ".splice(i, 1);\n";
            OSS << "        break; // remove one edge\n";
            OSS << "    } else {\n";
            OSS << "        ++i;\n";
            OSS << "    }\n";
            OSS << "}\n";
        } else {
            OSS << "for (let i = 0; i !== source." << outEdgeList << ".length;) {\n";
            OSS << "    if (source." << outEdgeList << "[i].target === v) {\n";
            OSS << "        source." << outEdgeList << ".splice(i, 1);\n";
            OSS << "        break; // remove one edge\n";
            OSS << "    } else {\n";
            OSS << "        ++i;\n";
            OSS << "    }\n";
            OSS << "}\n";
        }
    }

    {
        OSS << "const target = " << s.getTypescriptVertexDereference("v", scratch) << ";\n";
        if (s.needEdgeList() && !bReferenceGraph) {
            OSS << "for (let i = 0; i !== target." << inEdgeList << ".length;) {\n";
            OSS << "    if (target." << inEdgeList << "[i].edge === e.edge) {\n";
            OSS << "        target." << inEdgeList << ".splice(i, 1);\n";
            OSS << "        break; // remove one edge\n";
            OSS << "    } else {\n";
            OSS << "        ++i;\n";
            OSS << "    }\n";
            OSS << "}\n";
        } else {
            OSS << "for (let i = 0; i !== target." << inEdgeList << ".length;) {\n";
            OSS << "    if (target." << inEdgeList << "[i].target === u) {\n";
            OSS << "        target." << inEdgeList << ".splice(i, 1);\n";
            OSS << "        break; // remove one edge\n";
            OSS << "    } else {\n";
            OSS << "        ++i;\n";
            OSS << "    }\n";
            OSS << "}\n";
        }
    }

    if (s.needEdgeList() && !bReferenceGraph) {
        OSS << "// remove edge\n";
        OSS << "this._edges.delete(e.edge as " << edgeType << ");\n";
    }
}

void outputRemoveOutEdge(std::ostream& oss, std::pmr::string& space,
    std::string_view vert,
    std::string_view outEdgeList,
    std::string_view target, std::string_view vertDesc, bool bOne) {

    OSS << "for (let i = 0; i !== " << vert << "." << outEdgeList << ".length;) {";
    if (bOne) {
        oss << " // remove one edge\n";
    } else {
        oss << " // remove all edges\n";
    }
    OSS << "    if (" << vert << "." << outEdgeList << "[i]." << target << " === " << vertDesc << ") {\n";
    OSS << "        " << vert << "." << outEdgeList << ".splice(i, 1);\n";
    if (bOne) {
        OSS << "        break; // remove only one edge\n";
    }
    OSS << "    } else {\n";
    OSS << "        ++i;\n";
    OSS << "    }\n";
    OSS << "}\n";
}

void outputRemoveEdges(std::ostream& oss, std::pmr::string& space, const Graph& s,
    bool bReferenceGraph,
    std::pmr::set<std::pmr::string>& imports,
    std::pmr::memory_resource* scratch) {
    {
        auto source = s.getTypescriptVertexDereference("u", scratch);
        OSS << "const source = " << source << ";\n";
        if (s.needEdgeList() && !bReferenceGraph) {
            OSS << "// remove all edges from edge list\n";
            if (gImpl) {
                OSS << "impl.removeAllEdgesFromList(this._edges, source.";
            } else {
                OSS << "removeAllEdgesFromList(this._edges, source.";
                imports.emplace("removeAllEdgesFromList");
            }
            oss << s.getTypescriptOutEdgeList(bReferenceGraph) << ", v);\n";
        }
        OSS << "// remove out edges of u\n";
        if (true) {
            outputRemoveOutEdge(oss, space, "source", s.getTypescriptOutEdgeList(bReferenceGraph), "target", "v", false);
        } else {
            OSS << "source." << s.getTypescriptOutEdgeList(bReferenceGraph) << " = "
                << "source." << s.getTypescriptOutEdgeList(bReferenceGraph) << ".filter(oe => oe.target !== v);\n";
        }

        auto target = s.getTypescriptVertexDereference("v", scratch);
        OSS << "// remove in edges of v\n";
        OSS << "const target = " << target << ";\n";
        if (true) {
            outputRemoveOutEdge(oss, space, "target", s.getTypescriptInEdgeList(bReferenceGraph), "target", "u", false);
        } else {
            OSS << "target." << s.getTypescriptInEdgeList(bReferenceGraph) << " = "
                << "target." << s.getTypescriptInEdgeList(bReferenceGraph) << ".filter(ie => ie.target !== u);\n";
        }
    }
}

void outputNullVertex(std::ostream& oss, std::pmr::string& space,
    const Graph& s, std::string_view vertexDescType) {
    if (s.isVector()) {
        OSS << "nullVertex (): " << vertexDescType << " { ";
    } else {
        OSS << "nullVertex (): null { ";
    }
    visit(
        overload(
            [&](Vector_) {
                oss << "return 0xFFFFFFFF;";
            },
            [&](List_) {
                oss << "return null;";
            }),
        s.mVertexListType);
    oss << " }\n";
}

void outputGraphPolymorphics(std::ostream& oss, std::pmr::string& space, std::string_view name,
    const SyntaxGraph& g, const Graph& s, std::pmr::memory_resource* scratch) {
    OSS << "export const enum " << name << "Value {\n";
    {
        INDENT();
        for (const auto& c : s.mPolymorphic.mConcepts) {
            auto name = getTypescriptTagType(extractName(c.mTag), scratch);
            OSS << name << ",\n";
        }
    }
    OSS << "}\n";

    oss << "\n";
    OSS << "export function get" << name << "ValueName (e: " << name << "Value): string {\n";
    {
        INDENT();
        OSS << "switch (e) {\n";
        for (const auto& c : s.mPolymorphic.mConcepts) {
            auto eName = getTypescriptTagType(extractName(c.mTag), scratch);
            OSS << "case " << name << "Value." << eName << ": return '" << eName << "';\n";
        }
        OSS << "default: return '';\n";
        OSS << "}\n";
    }
    OSS << "}\n";

    oss << "\n";
    OSS << "export interface " << name << "ValueType {\n";
    {
        INDENT();
        for (const auto& c : s.mPolymorphic.mConcepts) {
            auto enumName = getTypescriptTagType(extractName(c.mTag), scratch);
            auto typeName = g.getTypescriptTypename(c.mValue, scratch, scratch);
            OSS << "[" << name << "Value." << enumName << "]: " << typeName << "\n";
        }
    }
    OSS << "}\n";

    oss << "\n";
    OSS << "export interface " << name << "Visitor {\n";
    {
        INDENT();
        for (const auto& c : s.mPolymorphic.mConcepts) {
            auto name = getTypescriptTagType(extractName(c.mTag), scratch);
            auto typeName = g.getTypescriptTypename(c.mValue, scratch, scratch);

            OSS << getVariableName(name, scratch) << "(value: "
                << typeName << "): unknown;\n";
        }
    }
    OSS << "}\n";

    oss << "\n";
    if (s.mPolymorphic.mConcepts.size() > 4) {
        OSS << "export type " << name << "Object = ";
        for (uint32_t count = 0; const auto& c : s.mPolymorphic.mConcepts) {
            if (count++) {
                oss << "\n";
                oss << "| ";
            }
            auto typeName = g.getTypescriptTypename(c.mValue, scratch, scratch);
            oss << typeName;
        }
        oss << ";\n";
    } else {
        OSS << "export type " << name << "Object = ";
        for (uint32_t count = 0; const auto& c : s.mPolymorphic.mConcepts) {
            if (count++) {
                oss << " | ";
            }
            auto typeName = g.getTypescriptTypename(c.mValue, scratch, scratch);
            oss << typeName;
        }
        oss << ";\n";
    }
}

void outputGraphComponents(std::ostream& oss, std::pmr::string& space, std::string_view name,
    const SyntaxGraph& g, const Graph& s, std::pmr::memory_resource* scratch) {
    Expects(!s.mComponents.empty());

    OSS << "export const enum " << name << "Component {\n";
    {
        INDENT();
        for (const auto& c : s.mComponents) {
            OSS << getTypescriptTagType(c.mName, scratch) << ",\n";
        }
    }
    OSS << "}\n";

    oss << "\n";
    OSS << "export interface " << name << "ComponentType {\n";
    {
        INDENT();
        for (const auto& c : s.mComponents) {
            auto typeName = g.getTypescriptTypename(c.mValuePath, scratch, scratch);
            OSS << "[" << name << "Component." << getTypescriptTagType(c.mName, scratch) << "]: " << typeName << ";\n";
        }
    }
    OSS << "}\n";

    if (!gReduceCode) {
        oss << "\n";
        OSS << "export interface " << name << "ComponentPropertyMap {\n";
        {
            INDENT();
            for (const auto& c : s.mComponents) {
                auto typeName = g.getTypescriptTypename(c.mValuePath, scratch, scratch);
                OSS << "[" << name << "Component." << getTypescriptTagType(c.mName, scratch) << "]: " << name << convertTag(c.mName) << "Map;\n";
            }
        }
        OSS << "}\n";
    }
}

void outputGraphVertex(std::ostream& oss, std::pmr::string& space,
    const ModuleBuilder& builder,
    const Graph& s, std::string_view name,
    std::string_view vertexDescType,
    std::string_view vertexName,
    std::string_view outEdgeType,
    std::string_view outRefType,
    bool bVectorVertexDescriptor,
    std::pmr::set<std::pmr::string>& imports,
    std::pmr::memory_resource* scratch) {
    const auto& g = builder.mSyntaxGraph;

    OSS << "export class " << vertexName;
    if (!bVectorVertexDescriptor) {
        if (gImpl) {
            oss << " implements impl.Vertex";
        } else {
            imports.emplace("Vertex");
            oss << " implements Vertex";
        }
    }
    oss << " {\n";
    {
        INDENT();
        if (!bVectorVertexDescriptor) {
            outputNullVertex(oss, space, s, vertexDescType);
        }
        if (s.hasProperties()) {
            if (s.isPolymorphic() || !s.mVertexProperty.empty() || s.isComponentInVertex()) {
                int count = 0;
                OSS << "constructor (";
                {
                    INDENT();
                    if (s.isPolymorphic()) {
                        if (count++) {
                            oss << ",\n";
                        } else {
                            oss << "\n";
                        }
                        OSS << "readonly id: " << name << "Value,\n";
                        OSS << "readonly object: " << name << "Object";
                    }
                    if (false && s.mNamed) {
                        if (count++) {
                            oss << ",\n";
                        } else {
                            oss << "\n";
                        }
                        OSS << "readonly name: string";
                    }
                    if (!s.mVertexProperty.empty()) {
                        if (count++) {
                            oss << ",\n";
                        } else {
                            oss << "\n";
                        }
                        OSS << "readonly property: " << s.getTypescriptVertexPropertyType(g, scratch, scratch);
                    }
                    if (s.isComponentInVertex()) {
                        for (const auto& c : s.mComponents) {
                            if (count++) {
                                oss << ",\n";
                            } else {
                                oss << "\n";
                            }
                            OSS << "readonly " << g.getMemberName(c.mMemberName, true);
                            oss << ": " << c.getTypescriptComponentType(g, scratch, scratch);
                        }
                    }
                }
                if (count) {
                    oss << ",\n";
                    OSS << ") {\n";
                } else {
                    oss << ") {\n";
                }
                {
                    INDENT();
                    if (s.isPolymorphic()) {
                        OSS << "this._id = id;\n";
                        OSS << "this._object = object;\n";
                    }
                    if (false && s.mNamed) {
                        OSS << "this._name = name;\n";
                    }
                    if (!s.mVertexProperty.empty()) {
                        OSS << "this._property = property;\n";
                    }
                    if (s.isComponentInVertex()) {
                        for (const auto& c : s.mComponents) {
                            OSS << "this." << g.getMemberName(c.mMemberName, false)
                                << " = " << g.getMemberName(c.mMemberName, true) << ";\n";
                        }
                    }
                }
                OSS << "}\n";
            }
            if (true) { // IncidenceGraph
                OSS << "readonly _outEdges: " << outEdgeType << "[] = [];\n";
            }
            if (true) { // BidirectionalGraph
                OSS << "readonly _inEdges: " << outEdgeType << "[] = [];\n";
            }
            if (s.needReferenceEdges()) {
                OSS << "readonly _children: " << outRefType << "[] = [];\n";
                OSS << "readonly _parents: " << outRefType << "[] = [];\n";
            }
            if (s.isPolymorphic()) {
                OSS << "readonly _id: " << name << "Value;\n";
                OSS << "_object: "<< name << "Object;\n";
            }
            if (false && s.mNamed)
                OSS << "readonly _name: string;\n";
            if (!s.mVertexProperty.empty()) {
                OSS << "readonly _property: " << s.getTypescriptVertexPropertyType(g, scratch, scratch) << ";\n";
            }
            if (s.isComponentInVertex()) {
                for (const auto& c : s.mComponents) {
                    OSS << "readonly " << g.getMemberName(c.mMemberName, false);
                    oss << ": " << c.getTypescriptComponentType(g, scratch, scratch) << ";\n";
                }
            }
        }
    }
    OSS << "}\n";
}

void outputGraphEdge(std::ostream& oss, std::pmr::string& space,
    const Graph& s, std::string_view vertexDescType,
    std::string_view edgeType, std::string_view edgeProperty,
    std::pmr::set<std::pmr::string>& imports) {
    Expects(!edgeProperty.empty());

    OSS << "class " << edgeType;
    if (gImpl) {
        oss << " implements impl.Edge";
    } else {
        imports.emplace("Edge");
        oss << " implements Edge";
    }
    oss << " {\n";
    {
        INDENT();
        OSS << "constructor (readonly u: " << vertexDescType << ", readonly v: "
            << vertexDescType << ", readonly property: " << edgeProperty << ") {\n";
        OSS << "    this.source = u;\n";
        OSS << "    this.target = v;\n";
        OSS << "    this._property = property;\n";
        OSS << "}\n";
        OSS << "getProperty (): " << edgeProperty << " {\n";
        {
            INDENT();
            OSS << "return this._property;\n";
        }
        OSS << "}\n";
        OSS << "source: " << vertexDescType << ";\n";
        OSS << "target: " << vertexDescType << ";\n";
        OSS << "readonly _property: " << edgeProperty << ";\n";
    }
    OSS << "}\n";
}

}

std::pmr::string generateGraph(const ModuleBuilder& builder,
    const ModuleInfo& moduleInfo,
    const Graph& s, SyntaxGraph::vertex_descriptor vertID, std::string_view name,
    std::pmr::set<std::pmr::string>& imports,
    bool bPublicFormat,
    std::pmr::memory_resource* scratch) {
    const auto& g = builder.mSyntaxGraph;

    pmr_ostringstream oss(std::ios_base::out, scratch);
    std::pmr::string space(scratch);

    auto epID = locate(s.mEdgeProperty, g);
    auto edgeProperty = g.getTypescriptTypename(epID, scratch, scratch);
    auto vpID = locate(s.mVertexProperty, g);
    auto vertexProperty = g.getTypescriptTypename(vpID, scratch, scratch);

    auto vertexDescType = s.getTypescriptVertexDescriptorType(name, scratch);
    auto nullableVertexDescType = vertexDescType;
    if (!s.isVector()) {
        nullableVertexDescType += " | null";
    }
    auto edgeDescType = s.getTypescriptEdgeDescriptorType();
    auto refDescType = s.getTypescriptReferenceDescriptorType();

    std::string_view outEdgeType;
    std::string_view outEdgeIter;
    std::string_view inEdgeIter;

    if (s.mEdgeProperty.empty()) {
        if (gImpl) {
            outEdgeType = "impl.OutE";
            outEdgeIter = "impl.OutEI";
            inEdgeIter = "impl.InEI";
        } else {
            outEdgeType = "OutE";
            outEdgeIter = "OutEI";
            inEdgeIter = "InEI";
            imports.emplace("OutE");
            imports.emplace("OutEI");
            imports.emplace("InEI");
        }
    } else {
        if (gImpl) {
            outEdgeType = "impl.OutEP";
            outEdgeIter = "impl.OutEPI";
            inEdgeIter = "impl.InEPI";
        } else {
            outEdgeType = "OutEP";
            outEdgeIter = "OutEPI";
            inEdgeIter = "InEPI";
            imports.emplace("OutEP");
            imports.emplace("OutEPI");
            imports.emplace("InEPI");
        }
    }

    std::string_view outRefType;
    std::string_view outRefIter;
    std::string_view inRefIter;
    if (gImpl) {
        outRefType = "impl.OutE";
        outRefIter = "impl.OutEI";
        inRefIter = "impl.InEI";
    } else {
        outRefType = "OutE";
        outRefIter = "OutEI";
        inRefIter = "InEI";
    }

    if (s.isAliasGraph() && !s.mEdgeProperty.empty()) {
        if (gImpl) {
            outRefType = "impl.OutEP";
            outRefIter = "impl.OutEPI";
            inRefIter = "impl.InEPI";
        } else {
            outRefType = "OutEP";
            outRefIter = "OutEPI";
            inRefIter = "InEPI";
        }
    }

    if (!gImpl) {
        imports.emplace(outRefType);
        imports.emplace(outRefIter);
        imports.emplace(inRefIter);
    }

    const bool bVectorVertexDescriptor = holds_alternative<Vector_>(s.mVertexListType);

    OSS << "//=================================================================\n";
    OSS << "// " << name << "\n";
    OSS << "//=================================================================\n";

    if (s.isPolymorphic()) {
        OSS << "// PolymorphicGraph Concept\n";
        outputGraphPolymorphics(oss, space, name, g, s, scratch);
        oss << "\n";
        OSS << "//-----------------------------------------------------------------\n";
    }

    OSS << "// Graph Concept\n";

    std::pmr::string vertexName(name, scratch);
    vertexName.append("Vertex");

    if (true) { // Vertex
        outputGraphVertex(oss, space, builder, s, name, vertexDescType,
            vertexName, outEdgeType, outRefType,
            bVectorVertexDescriptor, imports, scratch);
    }

    std::pmr::string edgeType(name, scratch);
    edgeType.append("Edge");

    if (s.needEdgeList()) { // Edge
        oss << "\n";
        outputGraphEdge(oss, space, s, vertexDescType, edgeType, edgeProperty, imports);
    }

    if (s.hasProperties()) { // PropertyMap
        auto outputPropertyMap = [&](SyntaxGraph::vertex_descriptor vertID,
            const Component* c,
            std::string_view mapName, bool bMember,
            std::string_view container, std::string_view component,
            std::string_view value, std::string_view type) {
            std::string_view arg = (!value.empty() && value.front() == '_')
                ? value.substr(1)
                : value;
            Expects(mapName.back() != '_');

            OSS << "export class " << name << mapName << "Map";
            if (gImpl) {
                oss << " implements impl.PropertyMap {\n";
            } else {
                oss << " implements PropertyMap {\n";
                imports.emplace("PropertyMap");
            }
            {
                INDENT();
                if (!container.empty() && s.isVector()) {
                    OSS << "constructor (readonly " << container << ": " << component << "[]) {\n";
                    {
                        INDENT();
                        OSS << "this._" << container << " = " << container << ";\n";
                    }
                    OSS << "}\n";
                }
                OSS << "get (v: " << vertexDescType << "): " << type << " {\n";
                {
                    INDENT();
                    if (container.empty()) {
                        OSS << "return null;\n";
                    } else {
                        if (s.isVector()) {
                            if (bMember) {
                                OSS << "return this._" << container << "[v]." << value << ";\n";
                            } else {
                                OSS << "return this._" << container << "[v]"
                                    << ";\n";
                            }
                        } else {
                            OSS << "return v." << value << ";\n";
                        }
                    }
                }
                OSS << "}\n";

                if (vertID != g.null_vertex() && g.isTypescriptValueType(vertID)) {
                    if (c && !(s.mNamedConcept.mComponent && s.mNamedConcept.mComponentName == c->mName && s.mAddressable)) {
                        OSS << "set (v: " << vertexDescType << ", " << arg << ": " << type << "): void {\n";
                        {
                            INDENT();
                            if (!container.empty()) {
                                if (s.isVector()) {
                                    if (bMember) {
                                        OSS << "this._" << container << "[v]." << value << " = " << arg << ";\n";
                                    } else {
                                        OSS << "this._" << container << "[v]"
                                            << " = " << arg << ";\n";
                                    }
                                } else {
                                    OSS << "v." << value << " = " << arg << ";\n";
                                }
                            }
                        }
                        OSS << "}\n";
                    } else {
                        OSS << "// skip set, name is constant in AddressableGraph\n";
                    }
                }
                if (s.isVector()) {
                    OSS << "readonly _" << container << ": " << component << "[];\n";
                }
            }
            OSS << "}\n";
        };

        int count = 0;
        if (!gReduceCode) {
            oss << "\n";
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// PropertyGraph Concept\n";
            if (false && s.mNamed) {
                if (count++)
                    oss << "\n";
                outputPropertyMap(SyntaxGraph::null_vertex(), nullptr, "Name", true, "vertices", vertexName, "_name", "string");
            }

            if (!s.mVertexProperty.empty()) {
                if (count++)
                    oss << "\n";
                auto vertID = locate(s.mVertexProperty, g);
                auto tsType = g.getTypescriptTypename(vertID, scratch, scratch);
                auto member = std::pmr::string(get(g.names, g, vertID), scratch);
                member.front() = tolower(member.front());
                outputPropertyMap(vertID, nullptr, "VertexProperty", true, "vertices", vertexName, "_property", tsType);
            }

            if (!s.mComponents.empty()) {
                for (const auto& c : s.mComponents) {
                    if (count++)
                        oss << "\n";
                    auto vertID = locate(c.mValuePath, g);
                    auto componentType = g.getTypescriptTypename(vertID, scratch, scratch);
                    Expects(!c.mMemberName.empty());
                    auto member = g.getMemberName(c.mMemberName, true);
                    outputPropertyMap(vertID, &c, convertTag(c.mName), false, member, componentType,
                        g.getMemberName(c.mMemberName, false), componentType);
                }
            }
        }
        if (!s.mComponents.empty()) {
            if (count++)
                oss << "\n";
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// ComponentGraph Concept\n";
            outputGraphComponents(oss, space, name, g, s, scratch);
        }
    }

    oss << "\n";
    OSS << "//-----------------------------------------------------------------\n";
    OSS << "// " << name << " Implementation\n";
    const auto& traits = get(g.traits, g, vertID);
    if (traits.mFlags & DECORATOR) {
        OSS << "@ccclass('cc." << name << "')\n";
    }
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
    if (constraints.mConcepts.empty()) {
        if (gImpl) {
            oss << " implements impl.BidirectionalGraph\n";
        } else {
            oss << " implements BidirectionalGraph\n";
            imports.emplace("BidirectionalGraph");
        }
    } else {
        oss << "\n";
        INDENT();
        if (gImpl) {
            oss << "implements impl.BidirectionalGraph\n";
        } else {
            oss << "implements BidirectionalGraph\n";
            imports.emplace("BidirectionalGraph");
        }
    }
    {
        if (!constraints.mConcepts.empty()) {
            space.append("    ");
        }
        if (gImpl) {
            OSS << ", impl.AdjacencyGraph\n";
            OSS << ", impl.VertexListGraph\n";
            OSS << ", impl.MutableGraph";
        } else {
            OSS << ", AdjacencyGraph\n";
            OSS << ", VertexListGraph\n";
            OSS << ", MutableGraph";
            imports.emplace("AdjacencyGraph");
            imports.emplace("VertexListGraph");
            imports.emplace("MutableGraph");
        }

        if (s.hasProperties()) {
            oss << "\n";
            if (gImpl) {
                OSS << ", impl.PropertyGraph";
            } else {
                OSS << ", PropertyGraph";
                imports.emplace("PropertyGraph");
            }
        }
        if (s.mNamed) {
            oss << "\n";
            if (gImpl) {
                OSS << ", impl.NamedGraph";
            } else {
                OSS << ", NamedGraph";
                imports.emplace("NamedGraph");
            }
        }
        if (!s.mComponents.empty()) {
            oss << "\n";
            if (gImpl) {
                OSS << ", impl.ComponentGraph";
            } else {
                OSS << ", ComponentGraph";
                imports.emplace("ComponentGraph");
            }
        }
        if (s.isPolymorphic()) {
            oss << "\n";
            if (gImpl) {
                OSS << ", impl.PolymorphicGraph";
            } else {
                OSS << ", PolymorphicGraph";
                imports.emplace("PolymorphicGraph");
            }
        }
        if (s.isReference()) {
            oss << "\n";
            if (gImpl) {
                OSS << ", impl.ReferenceGraph";
            } else {
                OSS << ", ReferenceGraph";
                imports.emplace("ReferenceGraph");
            }
        }
        if (s.isMutableReference()) {
            oss << "\n";
            if (gImpl) {
                OSS << ", impl.MutableReferenceGraph";
            } else {
                OSS << ", MutableReferenceGraph";
                imports.emplace("MutableReferenceGraph");
            }
        }
        if (s.isAddressable()) {
            oss << "\n";
            if (gImpl) {
                OSS << ", impl.AddressableGraph";
            } else {
                OSS << ", AddressableGraph";
                imports.emplace("AddressableGraph");
            }
        }
        if (!s.mVertexMaps.empty()) {
            Expects(s.mVertexMaps.size() == 1);
            const auto& map = s.mVertexMaps.front();
            const auto keyID = locate(map.mKeyType, g);
            const auto keyType = g.getTypescriptTypename(keyID, scratch, scratch);
            oss << "\n";
            if (gImpl) {
                OSS << ", impl.UuidGraph<" << keyType << ">";
            } else {
                OSS << ", UuidGraph<" << keyType << ">";
                imports.emplace("UuidGraph");
            }
        }
        if (!constraints.mConcepts.empty()) {
            space.resize(space.size() - 4);
        }
    }

    oss << " {\n";
    {
        INDENT();
        if (true) { // Graph Basics
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// Graph\n";
            OSS << "// type vertex_descriptor = " << vertexDescType << ";\n";
            outputNullVertex(oss, space, s, vertexDescType);

            // Edge
            if (gImpl) {
                if (s.mEdgeProperty.empty()) {
                    OSS << "// type edge_descriptor = impl.ED;\n";
                } else {
                    OSS << "// type edge_descriptor = impl.EPD;\n";
                }
                // Directional
                OSS << "readonly directed_category: impl.directional = impl.directional.bidirectional;\n";

                // Edge parallel
                OSS << "readonly edge_parallel_category: impl.parallel = impl.parallel.allow;\n";

                // Traversal
                OSS << "readonly traversal_category: impl.traversal = impl.traversal.incidence\n";
                OSS << "    | impl.traversal.bidirectional\n";
                OSS << "    | impl.traversal.adjacency\n";
                OSS << "    | impl.traversal.vertex_list;\n";
            } else {
                if (s.mEdgeProperty.empty()) {
                    OSS << "// type edge_descriptor = ED;\n";
                    imports.emplace("ED");
                } else {
                    OSS << "// type edge_descriptor = EPD;\n";
                    imports.emplace("EPD");
                }
                // Directional
                OSS << "readonly directed_category: directional = directional.bidirectional;\n";
                imports.emplace("directional");

                // Edge parallel
                OSS << "readonly edge_parallel_category: parallel = parallel.allow;\n";
                imports.emplace("parallel");

                // Traversal
                OSS << "readonly traversal_category: traversal = traversal.incidence\n";
                OSS << "    | traversal.bidirectional\n";
                OSS << "    | traversal.adjacency\n";
                OSS << "    | traversal.vertex_list;\n";
                imports.emplace("traversal");
            }
        } // Graph Basics

        if (true) { // IncidenceGraph
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// IncidenceGraph\n";
            OSS << "// type out_edge_iterator = " << outEdgeIter << ";\n";
            OSS << "// type degree_size_type = number;\n";

            OSS << "edge (u: " << vertexDescType << ", v: "
                << vertexDescType << "): boolean {\n";
            {
                INDENT();
                if (bVectorVertexDescriptor) {
                    OSS << "for (const oe of this._vertices[u]._outEdges) {\n";
                    {
                        INDENT();
                        OSS << "if (v === oe.target as " << vertexDescType << ") {\n";
                        OSS << "    return true;\n";
                        OSS << "}\n";
                    }
                    OSS << "}\n";
                    OSS << "return false;\n";
                } else {
                    OSS << "for (const oe of u._outEdges) {\n";
                    {
                        INDENT();
                        OSS << "if (v === oe.target as " << vertexDescType << ") {\n";
                        OSS << "    return true;\n";
                        OSS << "}\n";
                    }
                    OSS << "}\n";
                    OSS << "return false;\n";
                }
            }
            OSS << "}\n";

            OSS << "source (e: " << edgeDescType << "): " << vertexDescType << " {\n";
            {
                INDENT();
                OSS << "return e.source as " << vertexDescType << ";\n";
            }
            OSS << "}\n";

            OSS << "target (e: " << edgeDescType << "): " << vertexDescType << " {\n";
            {
                INDENT();
                OSS << "return e.target as " << vertexDescType << ";\n";
            }
            OSS << "}\n";

            OSS << "outEdges (v: " << vertexDescType << "): " << outEdgeIter << " {\n";
            {
                INDENT();
                if (bVectorVertexDescriptor) {
                    OSS << "return new " << outEdgeIter
                        << "(this._vertices[v]._outEdges.values(), v);\n";
                } else {
                    OSS << "return new " << outEdgeIter
                        << "(v._outEdges.values(), v);\n";
                }
            }
            OSS << "}\n";

            OSS << "outDegree (v: " << vertexDescType << "): number {\n";
            {
                INDENT();
                if (bVectorVertexDescriptor) {
                    OSS << "return this._vertices[v]._outEdges.length;\n";
                } else {
                    OSS << "return v._outEdges.length;\n";
                }
            }
            OSS << "}\n";
        }

        if (true) { // Bidirectional
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// BidirectionalGraph\n";
            OSS << "// type in_edge_iterator = " << inEdgeIter << ";\n";
            OSS << "inEdges (v: " << vertexDescType << "): " << inEdgeIter << " {\n";
            {
                INDENT();
                if (bVectorVertexDescriptor) {
                    OSS << "return new " << inEdgeIter
                        << "(this._vertices[v]._inEdges.values(), v);\n";
                } else {
                    OSS << "return new " << inEdgeIter
                        << "(v._inEdges.values(), v);\n";
                }
            }
            OSS << "}\n";

            OSS << "inDegree (v: " << vertexDescType << "): number {\n";
            {
                INDENT();
                if (bVectorVertexDescriptor) {
                    OSS << "return this._vertices[v]._inEdges.length;\n";
                } else {
                    OSS << "return v._inEdges.length;\n";
                }
            }
            OSS << "}\n";

            OSS << "degree (v: " << vertexDescType << "): number {\n";
            {
                INDENT();
                OSS << "return this.outDegree(v) + this.inDegree(v);\n";
            }
            OSS << "}\n";
        }

        std::string_view adjIter;
        if (true) { // AdjacencyGraph
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// AdjacencyGraph\n";
            if (s.mEdgeProperty.empty()) {
                if (gImpl) {
                    adjIter = "impl.AdjI";
                } else {
                    adjIter = "AdjI";
                    imports.emplace("AdjI");
                }
            } else {
                if (gImpl) {
                    adjIter = "impl.AdjPI";
                } else {
                    adjIter = "AdjPI";
                    imports.emplace("AdjPI");
                }
            }
            OSS << "// type adjacency_iterator = " << adjIter << ";\n";

            OSS << "adjacentVertices (v: " << vertexDescType << "): " << adjIter << " {\n";
            {
                INDENT();
                OSS << "return new " << adjIter << "(this, this.outEdges(v));\n";
            }
            OSS << "}\n";
        }

        if (true) { // VertexList Graph
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// VertexListGraph\n";
            OSS << "vertices (): IterableIterator<" << vertexDescType << "> {\n";
            {
                INDENT();
                if (bVectorVertexDescriptor) {
                    OSS << "return this._vertices.keys();\n";
                } else {
                    OSS << "return this._vertices.values();\n";
                }
            }
            OSS << "}\n";
            OSS << "numVertices (): number {\n";
            {
                INDENT();
                if (bVectorVertexDescriptor) {
                    OSS << "return this._vertices.length;\n";
                } else {
                    OSS << "return this._vertices.size;\n";
                }
            }
            OSS << "}\n";
        }

        if (true) {
            if (s.needEdgeList()) {
                Expects(false);
            } else {
                OSS << "//-----------------------------------------------------------------\n";
                OSS << "// EdgeListGraph\n";
                OSS << "numEdges (): number {\n";
                {
                    INDENT();
                    OSS << "let numEdges = 0;\n";
                    OSS << "for (const v of this.vertices()) {\n";
                    {
                        INDENT();
                        OSS << "numEdges += this.outDegree(v);\n";
                    }
                    OSS << "}\n";
                    OSS << "return numEdges;\n";
                }
                OSS << "}\n";
            }
        }

        if (true) { // MutableGraph
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// MutableGraph\n";
            std::pmr::string enumType(name, scratch);
            enumType += "Value";

            std::pmr::string enumTypeMap(name, scratch);
            enumTypeMap += "ValueType";

            // clear all
            OSS << "clear (): void {\n";
            {
                INDENT();

                // Members
                int count = 0;
                int i = 0;
                int numCleared = 0;
                for (const auto& m : s.mMembers) {
                    if (m.mFlags & IMPL_DETAIL) {
                        ++i;
                        continue;
                    }
                    bool bSkip = false;
                    for (const auto& cntr : s.mConstructors) {
                        Expects(s.mConstructors.size() == 1);
                        for (const auto& index : cntr.mIndices) {
                            if (index == i) {
                                bSkip = true;
                                break;
                            }
                        }
                    }
                    if (count++ == 0) {
                        OSS << "// Members\n";
                    }

                    const auto memberID = locate(m.mTypePath, g);
                    auto memberName = g.getMemberName(m.mMemberName, true);

                    if (bSkip) {
                        OSS << "// skip this." << memberName << "\n";
                        ++numCleared;
                        ++i;
                        continue;
                    }

                    if (!m.mDefaultValue.empty()) {
                        OSS << "this." << memberName << " = "
                            << g.getTypescriptInitialValue(memberID, m, scratch, scratch) << ";\n";
                    } else if (g.isTypescriptArray(memberID, scratch)) {
                        OSS << "this." << memberName << ".length = 0;\n";
                    } else if (g.isTypescriptPointer(memberID)) {
                        OSS << "this." << memberName << " = null;\n";
                    } else if (g.isTypescriptBoolean(memberID)) {
                        OSS << "this." << memberName << " = false;\n";
                    } else if (g.isTypescriptNumber(memberID)) {
                        OSS << "this." << memberName << " = 0;\n";
                    } else if (g.isTypescriptString(memberID)) {
                        OSS << "this." << memberName << " = '';\n";
                    } else if (g.isTypescriptSet(memberID)) {
                        OSS << "this." << memberName << ".clear();\n";
                    } else if (g.isTypescriptMap(memberID)) {
                        OSS << "this." << memberName << ".clear();\n";
                    } else {
                        OSS << "this." << memberName << ".clear();\n";
                    }
                    ++numCleared;
                    ++i;
                }

                // UuidGraph
                if (!s.mVertexMaps.empty()) {
                    Expects(s.mVertexMaps.size() == 1);
                    OSS << "// UuidGraph\n";
                    for (const auto& map : s.mVertexMaps) {
                        for (const auto& c : s.mComponents) {
                            if (c.mName != map.mComponentName)
                                continue;
                            const auto& componentVar = getTagVariableName(c.mName, scratch);
                            OSS << "this." << g.getMemberName(map.mMemberName, false)
                                << ".clear();\n";
                            ++numCleared;
                            break;
                        }
                    }
                }

                // AddressableGraph
                if (s.isAddressable() && s.hasAddressIndex()) {
                    ++numCleared;
                }

                // Graph Edges
                if (s.needEdgeList()) {
                    OSS << "// Graph Edges\n";
                    OSS << "this._edges.clear();\n";
                    ++numCleared;
                }

                // ComponentGraph
                if (s.isVector()) {
                    OSS << "// ComponentGraph\n";
                    for (const auto& c : s.mComponents) {
                        OSS << "this." << g.getMemberName(c.mMemberName, false) << ".length = 0;\n";
                        ++numCleared;
                    }
                }

                // PolymorphicGraph
                if (s.isVector()) {
                    numCleared += gsl::narrow_cast<int>(s.mPolymorphic.mConcepts.size());
                }
                // ReferenceGraph
                if (s.isReference() && !s.isAliasGraph()) {
                    ++numCleared; // mObjects
                }
                // Graph Vertices
                OSS << "// Graph Vertices\n";
                if (s.isVector()) {
                    OSS << "this._vertices.length = 0;\n";
                    ++numCleared;
                } else {
                    OSS << "this._vertices.clear();\n";
                    ++numCleared;
                }
                Ensures(numCleared == s.mMembers.size());
            }
            OSS << "}\n";

            // add vertex
            OSS << "addVertex";
            {
                {
                    INDENT();
                    int count = 0;
                    if (s.isPolymorphic()) {
                        oss << "<T extends " << enumType << "> (\n";
                        OSS << "id: " << enumType << ",\n";
                        OSS << "object: " << enumTypeMap << "[T],\n";
                        ++count;
                    } else {
                        oss << " (";
                    }
                    if (false && s.mNamed) {
                        if (count++ == 0)
                            oss << '\n';
                        OSS << "name: string,\n";
                    }
                    if (!s.mVertexProperty.empty()) {
                        if (count++ == 0)
                            oss << '\n';
                        OSS << "vertex: " << vertexProperty << ",\n";
                    }
                    for (const auto& c : s.mComponents) {
                        if (count++ == 0)
                            oss << '\n';
                        auto componentType = c.getTypescriptComponentType(g, scratch, scratch);
                        OSS << getTagVariableName(c.mName, scratch) << ": " << componentType << ",\n";
                    }
                    if (s.isReference()) {
                        if (count++ == 0)
                            oss << '\n';
                        if (s.isVector()) {
                            OSS << "u = " << s.getTypescriptNullVertex() << ",\n";
                        } else {
                            OSS << "u: " << nullableVertexDescType << " = " << s.getTypescriptNullVertex() << ",\n";
                        }
                        if (s.isAliasGraph() && !s.mEdgeProperty.empty()) {
                            OSS << "ep: " << edgeProperty << " = null,\n";
                        }
                    }
                }
                OSS << "): " << vertexDescType << " {\n";
                INDENT();
                if (s.isVector()) {
                    OSS << "const vert = new " << vertexName << "(";
                } else {
                    OSS << "const v = new " << vertexName << "(";
                }
                {
                    int count = 0;
                    if (s.isPolymorphic()) {
                        oss << "id, ";
                        oss << "object";
                        ++count;
                    }
                    if (false && s.mNamed) {
                        if (count++)
                            oss << ", ";
                        oss << "name";
                    }
                    if (!s.mVertexProperty.empty()) {
                        if (count++)
                            oss << ", ";
                        oss << "vertex";
                    }
                    if (!s.isVector()) {
                        for (const auto& c : s.mComponents) {
                            if (count++)
                                oss << ", ";
                            oss << getTagVariableName(c.mName, scratch);
                        }
                    }
                }
                oss << ");\n";

                // add vertex
                if (s.isVector()) {
                    OSS << "const v = this._vertices.length;\n";
                    OSS << "this._vertices.push(vert);\n";
                    for (const auto& c : s.mComponents) {
                        OSS << "this." << g.getMemberName(c.mMemberName, false)
                            << ".push(" << getTagVariableName(c.mName, scratch) << ");\n";
                    }
                } else {
                    OSS << "this._vertices.add(v);\n";
                }

                // UuidGraph
                if (!s.mVertexMaps.empty()) {
                    Expects(s.mVertexMaps.size() == 1);
                    OSS << "// UuidGraph\n";
                    for (const auto& map : s.mVertexMaps) {
                        for (const auto& c : s.mComponents) {
                            if (c.mName != map.mComponentName)
                                continue;
                            const auto& componentVar = getTagVariableName(c.mName, scratch);
                            OSS << "this." << g.getMemberName(map.mMemberName, false)
                                << ".set(" << componentVar << ", v);\n";
                            break;
                        }
                    }
                }

                // connect references
                if (s.isReference()) {
                    oss << "\n";
                    OSS << "// ReferenceGraph\n";
                    OSS << "if (u !== " << s.getTypescriptNullVertex() << ") {\n";
                    {
                        INDENT();
                        if (s.isAliasGraph()) {
                            if (s.mEdgeProperty.empty()) {
                                OSS << "this.addEdge(u, v);\n";
                            } else {
                                OSS << "this.addEdge(u, v, ep);\n";
                            }
                        } else {
                            if (s.isVector()) {
                                OSS << "this._vertices[u]._children.push(new " << outRefType << "(v));\n";
                                OSS << "vert._parents.push(new " << outRefType << "(u));\n";
                            } else {
                                OSS << "u._children.push(new " << outRefType << "(v));\n";
                                OSS << "v._parents.push(new " << outRefType << "(u));\n";
                            }
                        }
                    }
                    OSS << "}\n";
                    oss << "\n";
                }

                OSS << "return v;\n";
            }
            OSS << "}\n";

            // clear_vertex
            if (!gReduceCode) {
                OSS << "clearVertex (v: " << vertexDescType << "): void {\n";
                {
                    INDENT();
                    if (s.isReference()) {
                        OSS << "// ReferenceGraph";
                        if (s.isAliasGraph()) {
                            oss << "(Alias)";
                        } else {
                            oss << "(Separated)";
                        }
                        oss << "\n";
                    }
                    OSS << "const vert = " << s.getTypescriptVertexDereference("v", scratch) << ";\n";

                    auto outputClearEdges = [&](bool bOutputSeperated) {
                        auto outEdgeList = s.getTypescriptOutEdgeList(bOutputSeperated);
                        auto inEdgeList = s.getTypescriptInEdgeList(bOutputSeperated);

                        if (bOutputSeperated) {
                            OSS << "// clear child edges\n";
                        } else {
                            OSS << "// clear out edges\n";
                        }
                        OSS << "for (const oe of vert." << outEdgeList << ") {\n";
                        {
                            INDENT();
                            std::pmr::string key("oe.target as ", scratch);
                            if (s.isVector()) {
                                key.append("number");
                            } else {
                                key.append(vertexDescType);
                            }
                            OSS << "const target = "
                                << s.getTypescriptVertexDereference(key, scratch) << ";\n";

                            if (true) {
                                outputRemoveOutEdge(oss, space, "target", inEdgeList, "target", "v", false);
                            } else {
                                OSS << "target." << inEdgeList << " = target." << inEdgeList
                                    << ".filter(ie => ie.target !== v);\n";
                            }
                            if (!bOutputSeperated && s.needEdgeList()) {
                                OSS << "// remove edge from edge list\n";
                                OSS << "this._edges.delete(oe.edge as " << edgeType << ");\n";
                            }
                        }
                        OSS << "}\n";
                        OSS << "vert." << outEdgeList << ".length = 0;\n";

                        oss << "\n";
                        if (bOutputSeperated) {
                            OSS << "// clear parent edges\n";
                        } else {
                            OSS << "// clear in edges\n";
                        }
                        OSS << "for (const ie of vert." << inEdgeList << ") {\n";
                        {
                            INDENT();
                            std::pmr::string key("ie.target as ", scratch);
                            if (s.isVector()) {
                                key.append("number");
                            } else {
                                key.append(vertexDescType);
                            }
                            OSS << "const source = "
                                << s.getTypescriptVertexDereference(key, scratch) << ";\n";
                            if (true) {
                                outputRemoveOutEdge(oss, space, "source", outEdgeList, "target", "v", false);
                            } else {
                                OSS << "source." << outEdgeList << " = source."
                                    << outEdgeList << ".filter(ie => ie.target !== v);\n";
                            }
                            if (!bOutputSeperated && s.needEdgeList()) {
                                OSS << "// remove edge from edge list\n";
                                OSS << "this._edges.delete(ie.edge as " << edgeType << ");\n";
                            }
                        }
                        OSS << "}\n";
                        OSS << "vert." << inEdgeList << ".length = 0;\n";
                    };
                    outputClearEdges(false);
                    if (s.isReference() && !s.isAliasGraph()) {
                        oss << "\n";
                        outputClearEdges(true);
                    }
                }
                OSS << "}\n";

                // remove_vertex
                OSS << "removeVertex (u: " << vertexDescType << "): void {\n";
                {
                    INDENT();
                    if (s.isVector()) {
                        Expects(s.mVertexMaps.empty() || s.mVertexMaps.size() == 1);
                        for (const auto& map : s.mVertexMaps) {
                            OSS << "{ // UuidGraph\n";
                            {
                                INDENT();
                                for (const auto& c : s.mComponents) {
                                    if (c.mName != map.mComponentName)
                                        continue;

                                    const auto& component = g.getMemberName(c.mMemberName, false);
                                    OSS << "const key = this." << component << "[u];\n";
                                    OSS << "this." << g.getMemberName(map.mMemberName, false) << ".delete(key);\n";
                                    OSS << "this." << g.getMemberName(map.mMemberName, false) << ".forEach((v): void => {\n";
                                    {
                                        INDENT();
                                        OSS << "if (v > u) { --v; }\n";
                                    }
                                    OSS << "});\n";
                                    break;
                                }
                            }
                            OSS << "}\n";
                        }

                        OSS << "this._vertices.splice(u, 1);\n";

                        for (const auto& c : s.mComponents) {
                            Expects(!c.mMemberName.empty());
                            OSS << "this." << g.getMemberName(c.mMemberName, false)
                                << ".splice(u, 1);\n";
                        }

                        oss << "\n";
                        OSS << "const sz = this._vertices.length;\n";
                        OSS << "if (u === sz) {\n";
                        OSS << "    return;\n";
                        OSS << "}\n";
                        oss << "\n";
                        OSS << "for (let v = 0; v !== sz; ++v) {\n";
                        {
                            INDENT();
                            OSS << "const vert = " << s.getTypescriptVertexDereference("v", scratch) << ";\n";
                            if (gImpl) {
                                OSS << "impl.reindexEdgeList(vert." << s.getTypescriptOutEdgeList(false) << ", u);\n";
                                OSS << "impl.reindexEdgeList(vert." << s.getTypescriptInEdgeList(false) << ", u);\n";
                                if (s.isReference() && !s.isAliasGraph()) {
                                    OSS << "// ReferenceGraph (Separated)\n";
                                    OSS << "impl.reindexEdgeList(vert." << s.getTypescriptOutEdgeList(true) << ", u);\n";
                                    OSS << "impl.reindexEdgeList(vert." << s.getTypescriptInEdgeList(true) << ", u);\n";
                                }
                            } else {
                                OSS << "reindexEdgeList(vert." << s.getTypescriptOutEdgeList(false) << ", u);\n";
                                OSS << "reindexEdgeList(vert." << s.getTypescriptInEdgeList(false) << ", u);\n";
                                if (s.isReference() && !s.isAliasGraph()) {
                                    OSS << "// ReferenceGraph (Separated)\n";
                                    OSS << "reindexEdgeList(vert." << s.getTypescriptOutEdgeList(true) << ", u);\n";
                                    OSS << "reindexEdgeList(vert." << s.getTypescriptInEdgeList(true) << ", u);\n";
                                }
                                imports.emplace("reindexEdgeList");
                            }
                        }
                        OSS << "}\n";

                        if (s.needEdgeList()) {
                            oss << "\n";
                            OSS << "for (const e of this._edges) {\n";
                            {
                                INDENT();
                                OSS << "if (e.source > u) {\n";
                                OSS << "    --e.source;\n";
                                OSS << "}\n";
                                OSS << "if (e.target > u) {\n";
                                OSS << "    --e.target;\n";
                                OSS << "}\n";
                            }
                            OSS << "}\n";
                        }
                    } else {
                        for (const auto& map : s.mVertexMaps) {
                            OSS << "{ // UuidGraph\n";
                            {
                                INDENT();
                                for (const auto& c : s.mComponents) {
                                    if (c.mName != map.mComponentName)
                                        continue;

                                    const auto& component = g.getMemberName(c.mMemberName, false);
                                    OSS << "const key = this." << component << "[u];\n";
                                    OSS << "this." << g.getMemberName(map.mMemberName, false) << ".delete(key);\n";
                                    break;
                                }
                            }
                            OSS << "}\n";
                        }
                        OSS << "this._vertices.delete(u);\n";
                    }
                }
                OSS << "}\n";
            }

            // add_edge
            OSS << "addEdge (u: " << vertexDescType << ", v: " << vertexDescType;
            if (!edgeProperty.empty()) {
                oss << ", p: " << edgeProperty;
            }
            oss << "): " << edgeDescType << " | null {\n";
            {
                INDENT();
                copyString(oss, space, generateAddEdge(&s, name, edgeType, edgeDescType, outEdgeType, true, false, scratch));
            }
            OSS << "}\n";

            if (!gReduceCode) {
                OSS << "removeEdges (u: " << vertexDescType << ", v: " << vertexDescType << "): void {\n";
                {
                    INDENT();
                    outputRemoveEdges(oss, space, s, false, imports, scratch);
                }
                OSS << "}\n";

                OSS << "removeEdge (e: " << edgeDescType << "): void {\n";
                {
                    INDENT();
                    outputRemoveEdge(oss, space, s, vertexDescType, edgeType, false, scratch);
                }
                OSS << "}\n";
            }
        }

        if (s.mNamed) { // NamedGraph
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// NamedGraph\n";
            OSS << "vertexName (v: " << vertexDescType << "): string {\n";
            {
                INDENT();
                Expects(s.mNamedConcept.mComponent);
                for (const auto& c : s.mComponents) {
                    if (c.mName != s.mNamedConcept.mComponentName)
                        continue;

                    OSS << "return " << builder.getTypescriptVertexName(vertID, "v") << ";\n";
                }
            }
            OSS << "}\n";
            if (!gReduceCode) {
                OSS << "vertexNameMap (): " << name << "NameMap {\n";
                {
                    INDENT();
                    if (s.isVector()) {
                        for (const auto& c : s.mComponents) {
                            if (c.mName != s.mNamedConcept.mComponentName)
                                continue;

                            OSS << "return new " << name << "NameMap(this."
                                << g.getMemberName(c.mMemberName, false) << ");\n";
                        }
                    } else {
                        OSS << "return new " << name << "NameMap();\n";
                    }
                }
                OSS << "}\n";
            }
        }

        if (s.hasProperties()) { // PropertyGraph
            if (!gReduceCode) {
                OSS << "//-----------------------------------------------------------------\n";
                OSS << "// PropertyGraph\n";
                if (!s.mVertexProperty.empty()) {
                    OSS << "vertexProperty (v: " << vertexDescType << "): "
                        << s.getTypescriptVertexPropertyType(g, scratch, scratch) << " {\n";
                    {
                        INDENT();
                        if (s.isVector()) {
                            OSS << "return this._vertices[v]._property;\n";
                        } else {
                            OSS << "return v._property;\n";
                        }
                    }
                    OSS << "}\n";
                    OSS << "vertexPropertyMap (): " << name << "VertexPropertyMap {\n";
                    {
                        INDENT();
                        if (s.isVector()) {
                            OSS << "return new " << name << "VertexPropertyMap(this._vertices);\n";
                        } else {
                            OSS << "return new " << name << "VertexPropertyMap();\n";
                        }
                    }
                    OSS << "}\n";
                }

                OSS << "get (tag: string): ";
                int count = 0;
                if (false && s.mNamed) {
                    if (count++)
                        oss << " | ";
                    oss << name << "NameMap";
                }
                if (!s.mVertexProperty.empty()) {
                    if (count++)
                        oss << " | ";
                    oss << name << "VertexPropertyMap";
                }
                for (const auto& c : s.mComponents) {
                    if (count++)
                        oss << " | ";
                    oss << name << convertTag(c.mName) << "Map";
                }
                oss << " {\n";
                {
                    INDENT();
                    OSS << "switch (tag) {\n";
                    if (false && s.mNamed) {
                        OSS << "// NamedGraph\n";
                        OSS << "case '"
                            << "name"
                            << "':\n";
                        INDENT();
                        if (s.isVector()) {
                            OSS << "return new " << name << "NameMap(this._vertices);\n";
                        } else {
                            OSS << "return new " << name << "NameMap();\n";
                        }
                    }
                    if (!s.mVertexProperty.empty()) {
                        OSS << "// VertexProperty\n";
                        OSS << "case '"
                            << "vertex"
                            << "':\n";
                        INDENT();
                        if (s.isVector()) {
                            OSS << "return new " << name << "VertexPropertyMap(this._vertices);\n";
                        } else {
                            OSS << "return new " << name << "VertexPropertyMap();\n";
                        }
                    }
                    if (!s.mComponents.empty()) {
                        OSS << "// Components\n";
                    }
                    for (const auto& c : s.mComponents) {
                        auto componentType = c.getTypescriptComponentType(g, scratch, scratch);
                        auto member = g.getMemberName(c.mMemberName, false);
                        OSS << "case '" << getTypescriptTagType(c.mName, scratch) << "':\n";
                        Expects(c.isValid());
                        if (s.isVector()) {
                            OSS << "    return new " << name << convertTag(c.mName) << "Map("
                                << "this." << member << ");\n";
                        } else {
                            OSS << "    return new " << name << convertTag(c.mName) << "Map();\n";
                        }
                    }
                    OSS << "default:\n";
                    if (gThrow) {
                        OSS << "    throw Error('property map not found');\n";
                    } else {
                        OSS << "    return undefined;\n";
                    }
                    OSS << "}\n";
                }
                OSS << "}\n";
            }
        }

        if (!s.mComponents.empty()) { // ComponentGraph
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// ComponentGraph\n";
            OSS << "component<T extends " << name << "Component> (id: T, v: "
                << vertexDescType << "): " << name << "ComponentType[T] {\n";
            {
                INDENT();
                OSS << "switch (id) {\n";
                for (const auto& c : s.mComponents) {
                    auto componentType = c.getTypescriptComponentType(g, scratch, scratch);
                    OSS << "case " << name << "Component."
                        << getTypescriptTagType(c.mName, scratch) << ":\n";
                    INDENT();
                    if (s.isVector()) {
                        OSS << "return this."
                            << g.getMemberName(c.mMemberName, false)
                            << "[v] as " << name << "ComponentType[T];\n";
                    } else {
                        OSS << "return v."
                            << g.getMemberName(c.mMemberName, false)
                            << " as " << name << "ComponentType[T];\n";
                    }
                }
                OSS << "default:\n";
                if (gThrow) {
                    OSS << "    throw Error('component not found');\n";
                } else {
                    OSS << "    return undefined;\n";
                }
                OSS << "}\n";
            }
            OSS << "}\n";

            if (!gReduceCode) {
                OSS << "componentMap<T extends " << name << "Component> (id: T): " << name << "ComponentPropertyMap[T] {\n";
                {
                    INDENT();
                    OSS << "switch (id) {\n";
                    for (const auto& c : s.mComponents) {
                        auto componentType = c.getTypescriptComponentType(g, scratch, scratch);
                        auto member = g.getMemberName(c.mMemberName, false);

                        OSS << "case " << name << "Component."
                            << getTypescriptTagType(c.mName, scratch) << ":\n";
                        INDENT();
                        if (s.isVector()) {
                            OSS << "return new " << name << convertTag(c.mName) << "Map("
                                << "this." << member << ") as "
                                << name << "ComponentPropertyMap[T];\n";
                        } else {
                            OSS << "return new " << name << convertTag(c.mName) << "Map() as "
                                << name << "ComponentPropertyMap[T];\n";
                        }
                    }
                    OSS << "default:\n";
                    if (gThrow) {
                        OSS << "    throw Error('component map not found');\n";
                    } else {
                        OSS << "    return undefined;\n";
                    }
                    OSS << "}\n";
                }
                OSS << "}\n";
            }

            if (true) {
                for (const auto& c : s.mComponents) {
                    const auto componentID = locate(c.mValuePath, g);
                    auto componentType = c.getTypescriptComponentType(g, scratch, scratch);
                    auto member = g.getMemberName(c.mMemberName, false);
                    auto bNeedNameSetter = !(s.mNamedConcept.mComponent && s.mNamedConcept.mComponentName == c.mName && s.mAddressable);
                    if (!bNeedNameSetter) {
                        OSS << "// skip set" << convertTag(c.mName) << ", " << convertTag(c.mName) << " is constant in AddressableGraph\n";
                    }
                    OSS << "get" << convertTag(c.mName) << " (v: " << vertexDescType << "): "
                        << componentType << " {\n";
                    {
                        INDENT();
                        if (c.isValid()) {
                            if (s.isVector()) {
                                OSS << "return this." << member << "[v];\n";
                            } else {
                                OSS << "return v." << member << ";\n";
                            }
                        } else {
                            OSS << "return null;\n";
                        }
                    }
                    OSS << "}\n";

                    if (g.isTypescriptValueType(componentID)) {
                        if (bNeedNameSetter) {
                            OSS << "set" << convertTag(c.mName) << " (v: " << vertexDescType
                                << ", value: " << componentType << "): void {\n";
                            {
                                INDENT();
                                Expects(c.isValid());
                                if (s.isVector()) {
                                    OSS << "this." << member << "[v] = value;\n";
                                } else {
                                    OSS << "v." << member << " = value;\n";
                                }
                            }
                            OSS << "}\n";
                        }
                    }
                }
            }
        }

        if (s.isPolymorphic()) {
            std::pmr::string enumType(name, scratch);
            enumType += "Value";
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// PolymorphicGraph\n";
            OSS << "holds (id: " << enumType << ", v: " << vertexDescType << "): boolean {\n";
            {
                INDENT();
                if (s.isVector()) {
                    OSS << "return this._vertices[v]._id === id;\n";
                } else {
                    OSS << "return v._id === id;\n";
                }
            }
            OSS << "}\n";

            OSS << "id (v: " << vertexDescType << "): " << enumType << " {\n";
            {
                INDENT();
                if (s.isVector()) {
                    OSS << "return this._vertices[v]._id;\n";
                } else {
                    OSS << "return v._id;\n";
                }
            }
            OSS << "}\n";

            OSS << "object (v: " << vertexDescType << "): " << name << "Object {\n";
            {
                INDENT();
                if (s.isVector()) {
                    OSS << "return this._vertices[v]._object;\n";
                } else {
                    OSS << "return v._object;\n";
                }
            }
            OSS << "}\n";

            auto generateValue = [&](bool bTry) {
                if (bTry) {
                    OSS << "tryValue";
                } else {
                    OSS << "value";
                }
                oss << "<T extends " << name << "Value> (id: T, v: " << vertexDescType << "): "
                    << name << "ValueType[T]";
                if (bTry)
                    oss << " | null";
                oss << " {\n";
                {
                    INDENT();
                    if (s.isVector()) {
                        OSS << "if (this._vertices[v]._id === id) {\n";
                        OSS << "    return this._vertices[v]._object as " << name << "ValueType[T];\n";
                        OSS << "} else {\n";
                        if (bTry) {
                            OSS << "    return null;\n";
                        } else {
                            if (gThrow) {
                                OSS << "    throw Error('value id not match');\n";
                            } else {
                                OSS << "    return undefined;\n";
                            }
                        }
                        OSS << "}\n";
                    } else {
                        OSS << "if (v._id === id) {\n";
                        OSS << "    return v._object as " << name << "ValueType[T];\n";
                        OSS << "} else {\n";
                        if (bTry) {
                            OSS << "    return null;\n";
                        } else {
                            if (gThrow) {
                                OSS << "    throw Error('value id not match');\n";
                            } else {
                                OSS << "    return undefined;\n";
                            }
                        }
                        OSS << "}\n";
                    }
                }
                OSS << "}\n";
            };

            generateValue(false);
            generateValue(true);

            OSS << "visitVertex (visitor: " << name << "Visitor, v: "
                << vertexDescType << "): unknown {\n";
            {
                INDENT();
                if (s.isVector()) {
                    OSS << "const vert = this._vertices[v];\n";
                } else {
                    OSS << "const vert = v;\n";
                }
                OSS << "switch (vert._id) {\n";

                for (const auto& c : s.mPolymorphic.mConcepts) {
                    auto typeName = g.getTypescriptTypename(c.mValue, scratch, scratch);
                    const auto& tagName = getTypescriptTagType(extractName(c.mTag), scratch);

                    OSS << "case " << name << "Value." << getTypescriptTagType(extractName(c.mTag), scratch) << ":\n";
                    INDENT();
                    OSS << "return visitor." << getVariableName(tagName, scratch);
                    oss << "(vert._object as " << typeName << ");\n";
                }
                OSS << "default:\n";
                if (gThrow) {
                    OSS << "    throw Error('polymorphic type not found');\n";
                } else {
                    OSS << "    return undefined;\n";
                }
                OSS << "}\n";
            }
            OSS << "}\n";

            auto generatePolymorphicGetters = [&](bool bTry) {
                for (const auto& c : s.mPolymorphic.mConcepts) {
                    auto typeName = g.getTypescriptTypename(c.mValue, scratch, scratch);
                    auto tagName = getTypescriptTagType(extractName(c.mTag), scratch);
                    if (bTry) {
                        OSS << "tryGet";
                    } else {
                        OSS << "get";
                    }
                    oss << getTypescriptTagType(extractName(c.mTag)) << " (v: " << vertexDescType << "): " << typeName;
                    if (bTry) {
                        oss << " | null";
                    }
                    oss << " {\n";
                    {
                        INDENT();
                        if (s.isVector()) {
                            OSS << "if (this._vertices[v]._id === "
                                << enumType << "." << tagName << ") {\n";
                            OSS << "    return this._vertices[v]._object as " << typeName << ";\n";
                            OSS << "} else {\n";
                            if (bTry) {
                                OSS << "    return null;\n";
                            } else {
                                if (gThrow) {
                                    OSS << "    throw Error('value id not match');\n";
                                } else {
                                    OSS << "    return undefined;\n";
                                }
                            }
                            OSS << "}\n";
                        } else {
                            OSS << "if (v._id === "
                                << enumType << "." << tagName << ") {\n";
                            OSS << "    return v._object as " << typeName << ";\n";
                            OSS << "} else {\n";
                            if (bTry) {
                                OSS << "    return null;\n";
                            } else {
                                if (gThrow) {
                                    OSS << "    throw Error('value id not match');\n";
                                } else {
                                    OSS << "    return undefined;\n";
                                }
                            }
                            OSS << "}\n";
                        }
                    }
                    OSS << "}\n";
                }
            };

            if (true) {
                generatePolymorphicGetters(false);
                generatePolymorphicGetters(true);
            }
        }

        if (s.isReference()) {
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// ReferenceGraph\n";
            OSS << "// type reference_descriptor = " << refDescType << ";\n";
            OSS << "// type child_iterator = " << outRefIter << ";\n";
            OSS << "// type parent_iterator = " << inRefIter << ";\n";

            OSS << "reference (u: " << vertexDescType << ", v: "
                << vertexDescType << "): boolean {\n";
            {
                INDENT();
                if (bVectorVertexDescriptor) {
                    OSS << "for (const oe of this._vertices[u]." << s.getTypescriptOutEdgeList(true) << ") {\n";
                    {
                        INDENT();
                        OSS << "if (v === oe.target as " << vertexDescType << ") {\n";
                        OSS << "    return true;\n";
                        OSS << "}\n";
                    }
                    OSS << "}\n";
                    OSS << "return false;\n";
                } else {
                    OSS << "for (const oe of u." << s.getTypescriptOutEdgeList(true) << ") {\n";
                    {
                        INDENT();
                        OSS << "if (v === oe.target as " << vertexDescType << ") {\n";
                        OSS << "    return true;\n";
                        OSS << "}\n";
                    }
                    OSS << "}\n";
                    OSS << "return false;\n";
                }
            }
            OSS << "}\n";

            OSS << "parent (e: " << refDescType << "): " << vertexDescType << " {\n";
            {
                INDENT();
                OSS << "return e.source as " << vertexDescType << ";\n";
            }
            OSS << "}\n";
            OSS << "child (e: " << refDescType << "): " << vertexDescType << " {\n";
            {
                INDENT();
                OSS << "return e.target as " << vertexDescType << ";\n";
            }
            OSS << "}\n";
            OSS << "parents (v: " << vertexDescType << "): " << inRefIter << " {\n";
            {
                INDENT();
                if (s.isAliasGraph()) {
                    if (bVectorVertexDescriptor) {
                        OSS << "return new " << inRefIter
                            << "(this._vertices[v]._inEdges.values(), v);\n";
                    } else {
                        OSS << "return new " << inRefIter
                            << "(v._inEdges.values(), v);\n";
                    }
                } else {
                    if (bVectorVertexDescriptor) {
                        OSS << "return new " << inRefIter
                            << "(this._vertices[v]._parents.values(), v);\n";
                    } else {
                        OSS << "return new " << inRefIter
                            << "(v._parents.values(), v);\n";
                    }
                }
            }
            OSS << "}\n";
            OSS << "children (v: " << vertexDescType << "): " << outRefIter << " {\n";
            {
                INDENT();
                if (s.isAliasGraph()) {
                    if (bVectorVertexDescriptor) {
                        OSS << "return new " << outRefIter
                            << "(this._vertices[v]._outEdges.values(), v);\n";
                    } else {
                        OSS << "return new " << outRefIter
                            << "(v._outEdges.values(), v);\n";
                    }
                } else {
                    if (bVectorVertexDescriptor) {
                        OSS << "return new " << outRefIter
                            << "(this._vertices[v]._children.values(), v);\n";
                    } else {
                        OSS << "return new " << outRefIter
                            << "(v._children.values(), v);\n";
                    }
                }
            }
            OSS << "}\n";
            OSS << "numParents (v: " << vertexDescType << "): number {\n";
            {
                INDENT();
                OSS << "return ";
                if (s.isVector()) {
                    oss << "this._vertices[v].";
                } else {
                    oss << "v.";
                }
                if (s.isAliasGraph()) {
                    oss << "_inEdges.length";
                } else {
                    oss << "_parents.length";
                }
                oss << ";\n";
            }
            OSS << "}\n";
            OSS << "numChildren (v: " << vertexDescType << "): number {\n";
            {
                INDENT();
                OSS << "return ";
                if (s.isVector()) {
                    oss << "this._vertices[v].";
                } else {
                    oss << "v.";
                }
                if (s.isAliasGraph()) {
                    oss << "_outEdges.length";
                } else {
                    oss << "_children.length";
                }
                oss << ";\n";
            }
            OSS << "}\n";
            OSS << "getParent (v: " << vertexDescType << "): " << nullableVertexDescType << " {\n";
            {
                INDENT();
                OSS << "if (v === " << s.getTypescriptNullVertex() << ") {\n";
                OSS << "    return " << s.getTypescriptNullVertex() << ";\n";
                OSS << "}\n";
                if (s.isVector()) {
                    OSS << "const list = this._vertices[v].";
                } else {
                    OSS << "const list = v.";
                }
                if (s.isAliasGraph()) {
                    oss << "_inEdges";
                } else {
                    oss << "_parents";
                }
                oss << ";\n";
                OSS << "if (list.length === 0) {\n";
                OSS << "    return " << s.getTypescriptNullVertex() << ";\n";
                OSS << "} else {\n";
                OSS << "    return list[0].target as " << vertexDescType << ";\n";
                OSS << "}\n";
            }
            OSS << "}\n";

            OSS << "isAncestor (ancestor: " << vertexDescType
                << ", descendent: " << vertexDescType << "): boolean {\n";
            {
                INDENT();
                OSS << "const pseudo = " << s.getTypescriptNullVertex() << ";\n";
                OSS << "if (ancestor === descendent) {\n";
                OSS << "    // when ancestor === descendent, is_ancestor is defined as false\n";
                OSS << "    return false;\n";
                OSS << "}\n";
                OSS << "if (ancestor === pseudo) {\n";
                OSS << "    // special case: pseudo root is always ancestor\n";
                OSS << "    return true;\n";
                OSS << "}\n";
                OSS << "if (descendent === pseudo) {\n";
                OSS << "    // special case: pseudo root is never descendent\n";
                OSS << "    return false;\n";
                OSS << "}\n";
                OSS << "for (let parent = this.getParent(descendent); parent !== pseudo;) {\n";
                {
                    INDENT();
                    OSS << "if (ancestor === parent) {\n";
                    OSS << "    return true;\n";
                    OSS << "}\n";
                    OSS << "parent = this.getParent(parent);\n";
                }
                OSS << "}\n";
                OSS << "return false;\n";
            }
            OSS << "}\n";
        }

        if (s.isMutableReference()) {
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// MutableReferenceGraph\n";
            OSS << "addReference (u: " << vertexDescType
                << ", v: " << vertexDescType;
            if (s.isAliasGraph() && !s.mEdgeProperty.empty()) {
                oss << ", p: " << edgeProperty;
            }
            oss << "): " << refDescType << " | null {\n";
            {
                INDENT();
                if (s.isAliasGraph()) {
                    OSS << "return this.addEdge(u, v";
                    if (!s.mEdgeProperty.empty()) {
                        oss << ", p";
                    }
                    oss << ");\n";
                } else {
                    copyString(oss, space, generateAddEdge(&s, name, edgeType, refDescType, outRefType, false, true, scratch));
                }
            }
            OSS << "}\n";
            if (!gReduceCode) {
                OSS << "removeReference (e: " << refDescType << "): void {\n";
                {
                    INDENT();
                    if (s.isAliasGraph()) {
                        OSS << "return this.removeEdge(e);\n";
                    } else {
                        outputRemoveEdge(oss, space, s, vertexDescType, edgeType, true, scratch);
                    }
                }
                OSS << "}\n";

                OSS << "removeReferences (u: " << vertexDescType
                    << ", v: " << vertexDescType << "): void {\n";
                {
                    INDENT();
                    if (s.isAliasGraph()) {
                        OSS << "return this.removeEdges(u, v);\n";
                    } else {
                        outputRemoveEdges(oss, space, s, true, imports, scratch);
                    }
                }
                OSS << "}\n";
            }
        }

        if (s.isAddressable()) {
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// ParentGraph\n";
            OSS << "locateChild (u: " << nullableVertexDescType << ", name: string): " << nullableVertexDescType << " {\n";
            {
                INDENT();
                OSS << "if (u === " << s.getTypescriptNullVertex() << ") {\n";
                {
                    INDENT();
                    if (s.isVector()) {
                        OSS << "for (const v of this._vertices.keys()) {\n";
                        {
                            INDENT();
                            OSS << "const vert = this._vertices[v];\n";
                            if (s.isAliasGraph()) {
                                OSS << "if (vert._inEdges.length === 0 && "
                                    << builder.getTypescriptVertexName(vertID, "v") << " === name) {\n";
                            } else {
                                OSS << "if (vert._parents.length === 0 && "
                                    << builder.getTypescriptVertexName(vertID, "v") << " === name) {\n";
                            }
                            OSS << "    return v;\n";
                            OSS << "}\n";
                        }
                        OSS << "}\n";
                    } else {
                        OSS << "for (const v of this._vertices) {\n";
                        {
                            INDENT();
                            if (s.isAliasGraph()) {
                                OSS << "if (v._inEdges.length === 0 && "
                                    << builder.getTypescriptVertexName(vertID, "v") << " === name) {\n";
                            } else {
                                OSS << "if (v._parents.length === 0 && "
                                    << builder.getTypescriptVertexName(vertID, "v") << " === name) {\n";
                            }
                            OSS << "    return v;\n";
                            OSS << "}\n";
                        }
                        OSS << "}\n";
                    }
                    OSS << "return " << s.getTypescriptNullVertex() << ";\n";
                }
                OSS << "}\n";

                OSS << "for (const oe of ";
                if (s.isVector()) {
                    oss << "this._vertices[u].";
                } else {
                    oss << "u.";
                }
                if (s.isAliasGraph()) {
                    oss << "_outEdges";
                } else {
                    oss << "_children";
                }
                oss << ") {\n";
                {
                    INDENT();
                    OSS << "const child = oe.target as " << vertexDescType << ";\n";
                    if (s.isVector()) {
                        OSS << "if (name === " << builder.getTypescriptVertexName(vertID, "child") << ") {\n";
                        OSS << "    return child;\n";
                    } else {
                        OSS << "if (name === " << builder.getTypescriptVertexName(vertID, "child") << ") {\n";
                        OSS << "    return child;\n";
                    }
                    OSS << "}\n";
                }
                OSS << "}\n";
                OSS << "return " << s.getTypescriptNullVertex() << ";\n";
            }
            OSS << "}\n";
            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// AddressableGraph\n";
            OSS << "addressable (absPath: string): boolean {\n";
            {
                INDENT();
                if (gImpl) {
                    OSS << "return impl.findRelative(this, ";
                } else {
                    imports.emplace("findRelative");
                    OSS << "return findRelative(this, ";
                }
                oss << s.getTypescriptNullVertex() << ", absPath) as "
                    << vertexDescType << " !== " << s.getTypescriptNullVertex() << ";\n";
            }
            OSS << "}\n";

            OSS << "locate (absPath: string): " << vertexDescType << " {\n";
            {
                INDENT();
                if (gImpl) {
                    OSS << "return impl.findRelative(this, ";
                } else {
                    imports.emplace("findRelative");
                    OSS << "return findRelative(this, ";
                }
                oss << s.getTypescriptNullVertex() << ", absPath) as "
                    << vertexDescType << ";\n";
            }
            OSS << "}\n";

            if (s.isVector()) {
                OSS << "locateRelative (path: string, start = "
                    << s.getTypescriptNullVertex() << "): " << vertexDescType << " {\n";
            } else {
                OSS << "locateRelative (path: string, start: " << nullableVertexDescType << " = "
                    << s.getTypescriptNullVertex() << "): " << nullableVertexDescType << " {\n";
            }
            {
                INDENT();
                if (gImpl) {
                    OSS << "return impl.findRelative(this, start, path) as ";
                } else {
                    imports.emplace("findRelative");
                    OSS << "return findRelative(this, start, path) as ";
                }
                oss << nullableVertexDescType << ";\n";
            }
            OSS << "}\n";

            OSS << "path (v: " << vertexDescType << "): string {\n";
            {
                INDENT();
                if (gImpl) {
                    OSS << "return impl.getPath(this, v);\n";
                } else {
                    imports.emplace("getPath");
                    OSS << "return getPath(this, v);\n";
                }
            }
            OSS << "}\n";
        }

        if (!s.mVertexMaps.empty()) {
            Expects(s.mVertexMaps.size() == 1);
            const auto& map = s.mVertexMaps.front();
            const auto keyID = locate(map.mKeyType, g);
            const auto keyType = g.getTypescriptTypename(keyID, scratch, scratch);

            OSS << "//-----------------------------------------------------------------\n";
            OSS << "// UuidGraph\n";
            OSS << "contains (key: " << keyType << "): boolean {\n";
            {
                INDENT();
                OSS << "return this." << g.getMemberName(map.mMemberName, false) << ".has(key);\n";
            }
            OSS << "}\n";
            OSS << "vertex (key: " << keyType << "): " << vertexDescType << " {\n";
            {
                INDENT();
                OSS << "return this." << g.getMemberName(map.mMemberName, false) << ".get(key)!;\n";
            }
            OSS << "}\n";
            OSS << "find (key: " << keyType << "): " << nullableVertexDescType << " {\n";
            {
                INDENT();
                OSS << "const v = this." << g.getMemberName(map.mMemberName, false) << ".get(key);\n";
                OSS << "if (v === undefined) return " << s.getTypescriptNullVertex() << ";\n";
                OSS << "return v;\n";
            }
            OSS << "}\n";
        }
        // Members
        oss << "\n";
        if (!s.mComponents.empty()) {
            OSS << "readonly components: string[] = [";
            int count = 0;
            for (const auto& c : s.mComponents) {
                if (count++)
                    oss << ", ";
                oss << "'" << getTypescriptTagType(c.mName, scratch) << "'";
            }
            oss << "];\n";
        }
        if (true) { // VertexList Graph
            if (bVectorVertexDescriptor) {
                OSS << "readonly _vertices: " << vertexName << "[] = [];\n";
            } else {
                OSS << "readonly _vertices: Set<" << vertexName << "> = new Set<" << vertexName << ">();\n";
            }
        }

        if (true) { // EdgeList Graph
            if (s.needEdgeList()) {
                OSS << "readonly _edges: Set<" << edgeType << "> = new Set<" << edgeType << ">();\n";
            }
        }
        if (!s.mComponents.empty()) { // ComponentGraph
            if (s.isVector()) {
                for (const auto& c : s.mComponents) {
                    Expects(!c.mMemberName.empty());
                    auto vertID = locate(c.mValuePath, g);
                    OSS << "readonly " << g.getMemberName(c.mMemberName, false)
                        << ": " << c.getTypescriptComponentType(g, scratch, scratch)
                        << "[] = [];\n";
                }
            }
        }

        if (!s.mVertexMaps.empty()) {
            Expects(s.mVertexMaps.size() == 1);
            const auto& map = s.mVertexMaps.front();
            const auto keyID = locate(map.mKeyType, g);
            const auto keyType = g.getTypescriptTypename(keyID, scratch, scratch);
            auto componentID = locate(map.mComponentName, g);
            const auto member = g.getMemberName(map.mMemberName, false);
            std::pmr::string mapType(scratch);
            {
                const auto& keyName = map.mKeyType.substr(1);
                const auto& mapName = map.mMapType.substr(1);
                mapType.append(mapName).append("<").append(keyName).append(",vertex_descriptor>");
            }
            auto ns = g.getScope(vertID, scratch);
            auto mapID = g.lookupType(ns, mapType, scratch);
            Expects(mapID != g.null_vertex());
            OSS << "readonly " << member << ": " << g.getTypescriptTypename(mapID, scratch, scratch)
                << " = new " << g.getTypescriptTypename(mapID, scratch, scratch) << "();\n";
        }

        // GraphMembers
        const auto& constraints = get(g.constraints, g, vertID);
        outputMembers(oss, space, builder, moduleInfo,
            g, vertID,
            constraints.mConcepts,
            s.mMembers, s.mTypescriptFunctions,
            s.mConstructors, s.mMethods,
            bPublicFormat, scratch);
    }
    OSS << "}\n";

    if (gImpl) {
        Ensures(imports.empty());
    }

    return oss.str();
}

}
