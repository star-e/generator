#include "BuilderTypes.h"
#include "SyntaxGraphs.h"
#include "SyntaxUtils.h"

namespace Cocos::Meta {

std::pmr::string PolymorphicPair::getMaskName() const {
    pmr_ostringstream oss(std::ios::out, get_allocator().resource());
    oss << "m" << extractName(std::string_view(mTag).substr(0, mTag.size() - 1)) << "Mask";
    return oss.str();
}

std::pmr::string PolymorphicPair::getBitsName() const {
    pmr_ostringstream oss(std::ios::out, get_allocator().resource());
    oss << "mNum" << extractName(std::string_view(mTag).substr(0, mTag.size() - 1)) << "Bits";
    return oss.str();
}

void ObjectGraphBuilder::prepareNamespace(std::string_view& ns) const {
    if (ns == ".")
        ns = mStruct.mCurrentPath;
}

std::pmr::string ObjectGraphBuilder::nullVertexValue() const {
    const auto& s = *mGraph;
    if (s.isVector()) {
        return std::pmr::string("std::numeric_limits<Vertex>::max()",
            get_allocator());
    } else {
        return std::pmr::string("nullptr", get_allocator());
    }
}

std::pmr::string ObjectGraphBuilder::vertexDescType() const {
    const auto& s = *mGraph;
    std::pmr::string type(get_allocator());
    if (s.mVertexDescriptor.empty()) {
        if (s.isVector()) {
            type = "uint32_t";
        } else {
            type = "void*";
        }
    } else {
        type = s.mVertexDescriptor;
    }
    return type;
}

std::pmr::string ObjectGraphBuilder::edgeDescType() const {
    const auto& s = *mGraph;
    std::pmr::string type(get_allocator());

    if (s.needEdgeList()) {
        if (s.hasEdgeProperty()) {
            type = "PropertyEdgeDescriptor<Vertex>";
        } else {
            // not supported
            Expects(false);
        }
    } else {
        type = "EdgeDescriptor<Vertex>";
    }

    return type;
}

std::pmr::string ObjectGraphBuilder::linkDescType() const {
    const auto& s = *mGraph;
    std::pmr::string type(get_allocator());

    if (s.mAliasGraph) {
        type = "Edge";
    } else {
        type = "EdgeDescriptor<Vertex>";
    }

    return type;
}

ObjectGraphBuilder::ObjectGraphBuilder(const SyntaxGraph* syntaxGraph,
    const ModuleGraph* moduleGraph,
    uint32_t currentVertex, uint32_t currentModule,
    std::string_view currentNamespace,
    bool bDLL, std::string_view projectName, const allocator_type& alloc)
    : mStruct(syntaxGraph, moduleGraph, currentVertex, currentModule,
          currentNamespace, projectName, alloc)
    , mGraph(&get_by_tag<Graph_>(currentVertex, *syntaxGraph))
    , mDLL(bDLL)
    , mVertexType(alloc) {
}

std::pmr::string ObjectGraphBuilder::graphType(std::string_view ns) const {
    prepareNamespace(ns);
    const auto& g = *mStruct.mSyntaxGraph;
    const auto vertID = mStruct.mCurrentVertex;
    auto scratch = get_allocator().resource();
    auto name = g.getDependentName(ns, vertID);
    return getCppPath(name, scratch);
}

std::pmr::string ObjectGraphBuilder::vertexDesc(std::string_view ns) const {
    prepareNamespace(ns);
    const auto& g = *mStruct.mSyntaxGraph;
    const auto vertID = mStruct.mCurrentVertex;
    const auto& s = get<Graph>(vertID, g);
    auto scratch = get_allocator().resource();
    auto name = g.getDependentName(ns, vertID);
    if (name.empty()) {
        name.append("Vertex");
    } else {
        name.append("::Vertex");
    }
    return getCppPath(name, scratch);
}

std::pmr::string ObjectGraphBuilder::edgeDesc(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::vertexType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    auto scratch = get_allocator().resource();
    auto name = getCppPath(
        g.getDependentName(ns, mStruct.mCurrentVertex),
        scratch);
    if (name.empty()) {
        oss << "VertexType";
    } else {
        oss << name << "::VertexType";
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::edgeType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    if (s.hasEdgeProperty()) {
        auto epID = locate(s.mEdgeProperty, g);
        if (g.isPmr(epID)) {
            oss << "PmrPropertyEdge<Vertex, " << edgePropertyType(ns) << ">";
        } else {
            oss << "PropertyEdge<Vertex, " << edgePropertyType(ns) << ">";
        }
    } else {
        oss << "Edge<Vertex>";
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::vertexPropertyType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto vpID = locate(s.mVertexProperty, g);

    return getCppPath(g.getDependentName(ns, vpID), scratch);
}

std::pmr::string ObjectGraphBuilder::edgePropertyType(std::string_view ns) const {
    prepareNamespace(ns);
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto epID = locate(s.mEdgeProperty, g);
    return getCppPath(g.getDependentName(ns, epID), scratch);
}

std::pmr::string ObjectGraphBuilder::objectListType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    auto vecID = locate(s.mVertexListPath, g);
    oss << getCppPath(g.getDependentName(ns, vecID), scratch) << "<object_type>";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::vertexListType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    auto vecID = locate(s.mVertexListPath, g);
    oss << getCppPath(g.getDependentName(ns, vecID), scratch) << "<VertexType>";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::edgeListType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto listID = locate(s.mEdgeListPath, g);
    oss << getCppPath(g.getDependentName(ns, listID), scratch);
    oss << "<";

    const auto vertID = mStruct.mCurrentVertex;
    auto name = g.getDependentName(ns, vertID);
    if (name.empty()) {
        name.append("EdgeType");
    } else {
        name.append("::EdgeType");
    }
    oss << name;
    oss << ">";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::componentContainerType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    const auto& g = *mStruct.mSyntaxGraph;
    auto bPmr = g.isPmr(mStruct.mCurrentVertex);
    if (s.isVector()) {
        if (bPmr) {
            oss << "/Star/PmrVector";
        } else {
            oss << "/std/vector";
        }
    } else {
        if (bPmr) {
            oss << "/Star/PmrList";
        } else {
            oss << "/std/list";
        }
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::outEdgeListName() const {
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    return std::pmr::string("outEdgeList", scratch);
}

std::pmr::string ObjectGraphBuilder::outEdgeListMember() const {
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    return std::pmr::string("mOutEdges", scratch);
}

std::pmr::string ObjectGraphBuilder::outEdgeListType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto listID = locate(s.mOutEdgeListPath, g);
    oss << getCppPath(g.getDependentName(ns, listID), scratch);
    oss << "<" << outEdgeType(ns) << ">";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::outEdgeType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto name = graphType(ns);

    if (s.needEdgeList()) {
        if (s.isDirectedOnly()) {
            Expects(false);
        } else {
            auto edgePropType = edgePropertyType(ns);
            oss << "IncidencePropertyEdge<"
                << vertexDesc(ns) << ", "
                << edgeListType(ns) << ">";
        }
    } else {
        oss << vertexDesc(ns);
    }

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::outIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (s.needEdgeList()) {
        if (s.hasEdgeProperty()) {
            oss << "OutPropertyEdgeIterator<Edge, "
                << outEdgeListType() << "::const_iterator>";
        } else {
            Expects(false);
        }
    } else {
        oss << "OutEdgeIterator<Edge, "
            << outEdgeListType() << "::const_iterator>";
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::inEdgeListName() const {
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    return std::pmr::string("inEdgeList", scratch);
}

std::pmr::string ObjectGraphBuilder::inEdgeListMember() const {
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    return std::pmr::string("mInEdges", scratch);
}

std::pmr::string ObjectGraphBuilder::inEdgeListType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto listID = locate(s.mOutEdgeListPath, g);
    oss << getCppPath(g.getDependentName(ns, listID), scratch);
    oss << "<" << inEdgeType(ns) << ">";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::inEdgeType(std::string_view ns) const {
    prepareNamespace(ns);
    return outEdgeType(ns);
}

std::pmr::string ObjectGraphBuilder::inIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (s.needEdgeList()) {
        if (s.hasEdgeProperty()) {
            oss << "InPropertyEdgeIterator<Edge, "
                << inEdgeListType() << "::const_iterator>";
        } else {
            Expects(false);
        }
    } else {
        oss << "InEdgeIterator<Edge, "
            << inEdgeListType() << "::const_iterator>";
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::edgeIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (s.needEdgeList()) {
        if (s.hasEdgeProperty()) {
            oss << "PropertyEdgeIterator<"
                << edgeListType() << "::const_iterator, Edge>";
        } else {
            Expects(false);
        }
    } else {
        auto name = mStruct.getDependentName(mStruct.mCurrentPath);

        oss << "EdgeIterator<VertexIter, OutEdgeIter, "
            << name << ">";
        if (false) {
            oss << "using EdgeIter = EdgeIterator<VertexIter,\n";
            oss << "    " << vertexListType() << "::const_iterator,\n";
            oss << "    OutEdgeIter, " << name << ">;\n";
        }
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::childListName() const {
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    return std::pmr::string("childList", scratch);
}

std::pmr::string ObjectGraphBuilder::childListMember() const {
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    return std::pmr::string("mChildren", scratch);
}

std::pmr::string ObjectGraphBuilder::childListType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto listID = locate(s.mOutEdgeListPath, g);
    oss << getCppPath(g.getDependentName(ns, listID), scratch);
    oss << "<" << childEdgeType(ns) << ">";

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::childEdgeType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (s.mAliasGraph) {
        oss << outEdgeType(ns);
    } else {
        oss << vertexDesc(ns);
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::childIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;

    if (s.mAliasGraph) {
        oss << outIterType();
    } else {
        oss << "OutEdgeIterator<Link, "
            << outEdgeListType() << "::const_iterator>";
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::parentListName() const {
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    return std::pmr::string("parentList", scratch);
}

std::pmr::string ObjectGraphBuilder::parentListMember() const {
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    return std::pmr::string("mParents", scratch);
}

std::pmr::string ObjectGraphBuilder::parentEdgeType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (s.mAliasGraph) {
        oss << inEdgeType(ns);
    } else {
        oss << vertexDesc(ns);
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::parentListType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto listID = locate(s.mOutEdgeListPath, g);
    oss << getCppPath(g.getDependentName(ns, listID), scratch);
    oss << "<" << parentEdgeType(ns) << ">";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::parentIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    if (s.mAliasGraph) {
        oss << inIterType();
    } else {
        oss << "InEdgeIterator<Link, "
            << inEdgeListType() << "::const_iterator>";
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::linkIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;

    if (s.mAliasGraph) {
        oss << edgeIterType();
    } else {
        auto name = mStruct.getDependentName(mStruct.mCurrentPath);
        oss << "LinkIterator<VertexIter, ChildLinkIter, " << name << ">";
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::vertexIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (s.isVector()) {
        oss << "std::ranges::iota_view<Vertex, Vertex>::iterator";
    } else {
        oss << "Impl::VertexIter<" << vertexListType()
            << "::iterator, Vertex, "
            << s.mDifferenceType << ">";
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::adjIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    std::pmr::string space(get_allocator());

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::stringType(std::string_view ns) const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios::out, scratch);
    const auto& s = *mGraph;
    const auto& g = *mStruct.mSyntaxGraph;
    Expects(s.mNamed);

    auto typeID = g.null_vertex();
    if (s.mNamedConcept.mComponent) {
        const auto& c = s.getComponent(s.mNamedConcept.mComponentName);
        typeID = locate(c.mValuePath, g);
    } else {
        typeID = locate(s.mVertexProperty, g);
    }

    if (s.mNamedConcept.mComponentMemberName.empty()) {
        return g.getDependentCppName(ns, typeID);
    } else {
        return visit_vertex(
            typeID, g,
            [&](const Composition_ auto& s1) {
                for (const Member& m : s1.mMembers) {
                    if (m.mMemberName != s.mNamedConcept.mComponentMemberName)
                        continue;
                    auto stringID = locate(m.mTypePath, g);
                    return g.getDependentCppName(ns, stringID);
                }
                Expects(false);
                return std::pmr::string(scratch);
            },
            [&](const auto&) {
                Expects(false);
                return std::pmr::string(scratch);
            });
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::stringViewType() const {
    auto scratch = get_allocator().resource();
    auto type = stringType();
    if (type == "std::pmr::string" || type == "std::string") {
        return std::pmr::string("std::string_view", scratch);
    } else if (type == "std::pmr::u8string" || type == "std::u8string") {
        return std::pmr::string("std::u8string_view", scratch);
    } else if (type == "Star::Token") {
        return std::pmr::string("std::string_view", scratch);
    } else if (type == "Star::PmrString") {
        return std::pmr::string("std::string_view", scratch);
    }
    Expects(false);
    return std::pmr::string(scratch);
}

std::pmr::string ObjectGraphBuilder::tagType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    oss << "std::variant<";
    int count = 0;
    for (const auto& c : s.mPolymorphic.mConcepts) {
        if (count++)
            oss << ", ";
        auto conceptID = locate(c.mTag, g);
        oss << getCppPath(g.getDependentName(ns, conceptID), scratch);
    }
    oss << ">";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::valueType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    oss << "std::variant<";
    int count = 0;
    for (const auto& c : s.mPolymorphic.mConcepts) {
        if (count++)
            oss << ", ";
        auto conceptID = locate(c.mValue, g);
        oss << getCppPath(g.getDependentName(ns, conceptID), scratch);
        oss << "*";
    }
    oss << ">";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::constValueType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    oss << "std::variant<";
    int count = 0;
    for (const auto& c : s.mPolymorphic.mConcepts) {
        if (count++)
            oss << ", ";
        oss << "const ";
        auto conceptID = locate(c.mValue, g);
        oss << getCppPath(g.getDependentName(ns, conceptID), scratch);
        oss << "*";
    }
    oss << ">";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::handleElemType(const PolymorphicPair& pair,
    std::string_view ns, bool bSkipName) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto tagID = locate(pair.mTag, g);
    auto valueID = locate(pair.mValue, g);

    auto tagName = getCppPath(g.getDependentName(ns, tagID), scratch);
    auto valueName = getCppPath(g.getDependentName(ns, valueID), scratch);
    if (pair.mConst) {
        valueName.insert(0, "const ");
    }
    if (pair.mPointer) {
        valueName.append("*");
    }
    oss << "TaggedValue<" << tagName << ", ";
    if (pair.isIntrusive()) {
        oss << valueName;
    } else {
        if (s.isVector()) {
            if (pair.isVector()) {
                auto name = graphType(ns);
                oss << s.mVertexSizeType;
            } else {
                auto vecID = locate(pair.mContainerPath, g);
                auto vecType = getCppPath(g.getDependentName(ns, vecID), scratch);
                oss << vecType << "<" << valueName << ">::iterator";
            }
        } else {
            Expects(g.isPmr(valueID));
            oss << "Unique<" << valueName << ">";
        }
    }
    oss << ">";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::handleType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    oss << "std::variant<\n";
    int count = 0;
    for (const auto& c : s.mPolymorphic.mConcepts) {
        if (count++) {
            oss << ",\n";
        }
        auto conceptID = locate(c.mValue, g);
        oss << handleElemType(c, ns);
    }
    oss << ">\n";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::layerHandleType(const Layer& layer,
    std::string_view ns, bool bSkipName) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());

    auto graphName = getCppPath(g.getDependentName(ns, cpp.mCurrentVertex), scratch);
    oss << "Impl::ValueHandle<" << cpp.getDependentName(layer.mTagPath) << ", ";
    if (layer.isIntrusive()) {
        oss << graphName;
    } else {
        visit(
            overload(
                [&](Vector_) {
                    if (bSkipName || graphName.empty()) {
                        oss << "layer_descriptor";
                    } else {
                        oss << graphName << "::layer_descriptor";
                    }
                },
                [&](List_) {
                    oss << cpp.getDependentName(layer.mContainer) << "<" << graphName << ">::iterator";
                }),
            layer.mContainerType);
    }

    oss << ">";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::layerHandleVariantType(std::string_view ns) const {
    prepareNamespace(ns);
    auto scratch = get_allocator().resource();
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());

    OSS << "std::variant<\n";
    {
        INDENT();
        int count = 0;
        for (const auto& l : s.mStack.mLayers) {
            if (count++) {
                oss << ",\n";
            }
            OSS << layerHandleType(l, ns);
        }
        oss << "\n";
    }
    OSS << ">\n";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::layerTagVariantType(std::string_view ns) const {
    prepareNamespace(ns);
    auto scratch = get_allocator().resource();
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);

    OSS << "std::variant<";
    int count = 0;
    for (const auto& l : s.mStack.mLayers) {
        if (count++)
            oss << ", ";
        oss << cpp.getDependentName(l.mTagPath);
    }
    oss << ">";

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::layerValueVariantType(bool bConst, std::string_view ns) const {
    prepareNamespace(ns);
    auto scratch = get_allocator().resource();
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);

    OSS << "std::variant<";
    int count = 0;
    for (const auto& l : s.mStack.mLayers) {
        if (count++)
            oss << ", ";

        if (bConst) {
            oss << "const ";
        }
        oss << cpp.getDependentName(l.mGraphPath);
        oss << "*";
    }
    oss << ">";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::vertexPropertyMapName(bool bConst) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    auto cn = mStruct.mCurrentNamespace;

    if (cn.empty() || !cn.starts_with("/" + mStruct.mProjectName)) {
        OSS << mStruct.mProjectName << "::";
    }
    if (s.isVector()) {
        oss << "Impl::VectorVertexBundlePropertyMap<\n";
    } else {
        oss << "Impl::PointerVertexBundlePropertyMap<\n";
    }
    {
        INDENT();
        bool isString = false;
        bool isU8String = false;
        if (s.mVertexProperty == "/std/string"
            || s.mVertexProperty == "/std/pmr/string"
            || s.mVertexProperty == "/Star/PmrString") {
            isString = true;
        } else if (s.mVertexProperty == "/std/u8string"
            || s.mVertexProperty == "/std/pmr/u8string") {
            isString = true;
            isU8String = true;
        }
        if (isString) {
            OSS << "read_write_property_map_tag,\n";
        } else {
            OSS << "lvalue_property_map_tag,\n";
        }
        OSS << (bConst ? "const " : "") << cpp.getDependentName(mStruct.mCurrentPath) << ",\n";
        if (isString) {
            if (isU8String) {
                OSS << "std::string_view,\n";
            } else {
                OSS << "std::u8string_view,\n";
            }
        } else {
            OSS << "" << cpp.getDependentName(s.mVertexProperty) << ",\n";
        }
        OSS << (bConst ? "const " : "") << cpp.getDependentName(s.mVertexProperty) << "&";
    }
    oss << ">";

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::vertexPropertyMapMemberName(bool bConst) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    auto cn = mStruct.mCurrentNamespace;

    if (cn.empty() || !cn.starts_with(mStruct.mProjectName + "::")) {
        OSS << mStruct.mProjectName << "::";
    }

    if (s.isVector()) {
        oss << "Impl::VectorVertexBundleMemberPropertyMap<\n";
    } else {
        oss << "Impl::PointerVertexBundleMemberPropertyMap<\n";
    }

    {
        INDENT();
        bool isString = false;
        bool isU8String = false;
        if (s.mVertexProperty == "/std/string"
            || s.mVertexProperty == "/std/pmr/string"
            || s.mVertexProperty == "/Star/PmrString") {
            isString = true;
        } else if (s.mVertexProperty == "/std/u8string"
            || s.mVertexProperty == "/std/pmr/u8string") {
            isString = true;
            isU8String = true;
        }

        if (!isString) {
            OSS << "lvalue_property_map_tag,\n";
        } else {
            OSS << "read_write_property_map_tag,\n";
        }
        OSS << (bConst ? "const " : "") << cpp.getDependentName(mStruct.mCurrentPath) << ",\n";
        if (!isString) {
            OSS << "T,\n";
        } else {
            if (isU8String) {
                OSS << "std::u8string_view,\n";
            } else {
                OSS << "std::string_view,\n";
            }
        }
        if (!isString) {
            OSS << (bConst ? "const " : "") << "T&,\n";
            OSS << "T " << cpp.getDependentName(s.mVertexProperty) << "::*";
        } else {
            // implementation is inappropriate
            Expects(false);
            if (isU8String) {
                OSS << (bConst ? "const " : "") << "std::pmr::u8string&,\n";
                OSS << (bConst ? "const " : "") << "std::pmr::u8string " << cpp.getDependentName(s.mVertexProperty) << "::*";
            } else {
                OSS << (bConst ? "const " : "") << "std::pmr::string&,\n";
                OSS << (bConst ? "const " : "") << "std::pmr::string " << cpp.getDependentName(s.mVertexProperty) << "::*";
            }
        }
    }
    oss << ">";

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::edgePropertyMapName(bool bConst) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    auto cn = mStruct.mCurrentNamespace;

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (cn.empty() || cn.substr(0, 6) != mStruct.mProjectName + "::") {
        OSS << mStruct.mProjectName << "::";
    }
    oss << "Impl::EdgeBundlePropertyMap<\n";
    {
        INDENT();
        auto epID = locate(s.mEdgeProperty, g);
        bool isString = g.isString(epID);
        if (isString) {
            OSS << "read_write_property_map_tag,\n";
        } else {
            OSS << "lvalue_property_map_tag,\n";
        }
        OSS << (bConst ? "const " : "") << name << ",\n";
        if (isString) {
            if (g.isUtf8(epID)) {
                OSS << "std::u8string_view,\n";
            } else {
                OSS << "std::string_view,\n";
            }
        } else {
            OSS << "" << cpp.getDependentName(s.mEdgeProperty) << ",\n";
        }
        OSS << (bConst ? "const " : "") << cpp.getDependentName(s.mEdgeProperty) << "&";
    }
    oss << ">";

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::edgePropertyMapMemberName(bool bConst, std::string_view stringPath) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    auto cn = mStruct.mCurrentNamespace;

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (cn.empty() || cn.substr(0, 6) != mStruct.mProjectName + "::") {
        OSS << mStruct.mProjectName << "::";
    }
    oss << "Impl::EdgeBundleMemberPropertyMap<\n";
    {
        INDENT();
        bool isString = false;
        bool isU8String = false;
        bool bPmr = false;

        if (stringPath == "std::string"
            || stringPath == "std::pmr::string") {
            isString = true;
            if (stringPath == "std::pmr::string")
                bPmr = true;
        } else if (stringPath == "std::u8string"
            || stringPath == "std::pmr::u8string") {
            isString = true;
            isU8String = true;
            if (stringPath == "std::pmr::u8string")
                bPmr = true;
        }

        if (!isString) {
            OSS << "lvalue_property_map_tag,\n";
        } else {
            OSS << "read_write_property_map_tag,\n";
        }
        OSS << (bConst ? "const " : "") << name << ",\n";
        if (!isString) {
            OSS << "T,\n";
        } else {
            if (isU8String) {
                OSS << "std::u8string_view,\n";
            } else {
                OSS << "std::string_view,\n";
            }
        }
        if (!isString) {
            OSS << (bConst ? "const " : "") << "T&,\n";
            OSS << "T " << cpp.getDependentName(s.mEdgeProperty) << "::*";
        } else {
            OSS << (bConst ? "const " : "") << stringPath << "&,\n";
            OSS << stringPath << " " << cpp.getDependentName(s.mEdgeProperty) << "::*";
        }
    }
    oss << ">";

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::vertexComponentMapName(const Component& c, bool bConst) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    auto cn = mStruct.mCurrentNamespace;

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (cn.empty() || cn.substr(0, 6) != cpp.mProjectName + "::") {
        OSS << cpp.mProjectName << "::";
    }
    if (s.isVector()) {
        if (c.isVector()) {
            oss << "Impl::VectorVertexComponentPropertyMap<\n";
        } else {
            oss << "Impl::VectorVertexIteratorComponentPropertyMap<\n";
        }
    } else {
        Expects(false);
    }

    bool isString = false;
    bool isU8String = false;
    bool bPmr = false;
    if (c.mValuePath == "/std/string"
        || c.mValuePath == "/std/pmr/string"
        || c.mValuePath == "/Star/PmrString") {
        isString = true;
        if (c.mValuePath == "/std/pmr/string" || c.mValuePath == "/Star/PmrString")
            bPmr = true;
    } else if (c.mValuePath == "/std/u8string"
        || c.mValuePath == "/std/pmr/u8string") {
        isString = true;
        isU8String = true;
        if (c.mValuePath == "/std/pmr/u8string")
            bPmr = true;
    }

    {
        INDENT();
        // category
        if (!isString) {
            OSS << "lvalue_property_map_tag,\n";
        } else {
            OSS << "read_write_property_map_tag,\n";
        }

        // graph
        OSS << (bConst ? "const " : "") << name << ",\n";

        auto container = cpp.getDependentName(c.mContainerPath);
        auto component = cpp.getDependentName(c.mValuePath);
        if (s.isVector()) {
            if (c.isVector()) {
                OSS << (bConst ? "const " : "") << container << "<" << component << ">,\n";
            } else {
                OSS << container << "<" << component << ">::iterator " << name << "::VertexType::*,\n";
            }
        }

        // value
        if (isString) {
            if (isU8String) {
                OSS << "std::u8string_view,\n";
            } else {
                OSS << "std::string_view,\n";
            }
        } else {
            OSS << component << ",\n";
        }

        // reference
        if (isString) {
            if (isU8String) {
                if (bPmr) {
                    OSS << (bConst ? "const " : "") << "std::pmr::u8string&";
                } else {
                    OSS << (bConst ? "const " : "") << "std::u8string&";
                }
            } else {
                if (bPmr) {
                    OSS << (bConst ? "const " : "") << "std::pmr::string&";
                } else {
                    OSS << (bConst ? "const " : "") << "std::string&";
                }
            }
        } else {
            OSS << (bConst ? "const " : "") << component << "&";
        }
    }
    oss << ">";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::vertexComponentMapMemberName(
    const Component& c, bool bConst, std::string_view stringPath) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    auto cn = mStruct.mCurrentNamespace;

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (cn.empty() || cn.substr(0, 6) != cpp.mProjectName + "::") {
        OSS << cpp.mProjectName << "::";
    }
    if (s.isVector()) {
        if (c.isVector()) {
            oss << "Impl::VectorVertexComponentMemberPropertyMap<\n";
        } else {
            oss << "Impl::VectorVertexIteratorComponentMemberPropertyMap<\n";
        }
    } else {
        Expects(false);
    }

    bool isString = false;
    bool isU8String = false;
    bool bPmr = false;

    if (stringPath == "std::string"
        || stringPath == "std::pmr::string") {
        isString = true;
        if (stringPath == "std::pmr::string")
            bPmr = true;
    } else if (stringPath == "std::u8string"
        || stringPath == "std::pmr::u8string") {
        isString = true;
        isU8String = true;
        if (stringPath == "std::pmr::u8string")
            bPmr = true;
    }

    {
        INDENT();
        // tag
        if (!isString) {
            OSS << "lvalue_property_map_tag,\n";
        } else {
            OSS << "read_write_property_map_tag,\n";
        }
        // graph
        OSS << (bConst ? "const " : "") << name << ",\n";

        auto container = cpp.getDependentName(c.mContainerPath);
        auto component = cpp.getDependentName(c.mValuePath);
        // container / component pointer
        if (s.isVector()) {
            if (c.isVector()) {
                OSS << (bConst ? "const " : "") << container << "<" << component << ">,\n";
            } else {
                OSS << container << "<" << component << ">::iterator " << name << "::VertexType::*,\n";
            }
        }

        // value
        if (!isString) {
            OSS << "T,\n";
        } else {
            if (isU8String) {
                OSS << "std::u8string_view,\n";
            } else {
                OSS << "std::string_view,\n";
            }
        }
        // reference & member
        if (!isString) {
            OSS << (bConst ? "const " : "") << "T&,\n";
            OSS << "T " << component << "::*";
        } else {
            if (isU8String) {
                if (bPmr) {
                    OSS << (bConst ? "const " : "") << "std::pmr::u8string&,\n";
                    OSS << (bConst ? "const " : "") << "std::pmr::u8string " << component << "::*";
                } else {
                    OSS << (bConst ? "const " : "") << "std::u8string&,\n";
                    OSS << (bConst ? "const " : "") << "std::u8string " << component << "::*";
                }
            } else {
                if (bPmr) {
                    OSS << (bConst ? "const " : "") << "std::pmr::string&,\n";
                    OSS << (bConst ? "const " : "") << "std::pmr::string " << component << "::*";
                } else {
                    OSS << (bConst ? "const " : "") << "std::string&,\n";
                    OSS << (bConst ? "const " : "") << "std::string " << component << "::*";
                }
            }
        }
    }
    oss << ">";

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateVertexType(std::string_view name, bool layer) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateAllocator_h() {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& cpp = mStruct;
    const auto vertID = mStruct.mCurrentVertex;
    const auto& api = mStruct.mAPI;

    if (g.isPmr(vertID)) {
        if (mCount++)
            oss << "\n";
        OSS << "using allocator_type = PmrAllocator<std::byte>;\n";
        if (mDLL) {
            OSS << api << "_API " << cpp.generateGetAllocatorSignature(true) << ";\n";
        } else {
            OSS << cpp.generateGetAllocatorSignature(true) << " {\n";
            {
                INDENT();
                copyString(oss, space, cpp.generateGetAllocatorBody());
            }
            OSS << "}\n";
        }
    }

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateConstructors_h() {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& cpp = mStruct;
    const auto vertID = mStruct.mCurrentVertex;
    const auto& api = mStruct.mAPI;

    auto cntrs = cpp.generateHeaderConstructors();
    copyString(oss, space, cntrs);
    if (!cntrs.empty())
        ++mCount;

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateGraph_h() {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& cpp = mStruct;
    const auto vertID = mStruct.mCurrentVertex;
    const auto& api = mStruct.mAPI;
    const auto& s = *mGraph;
    std::string_view ns = cpp.mCurrentNamespace;
    auto name = cpp.getDependentName(cpp.mCurrentPath);
    auto scratch = get_allocator().resource();

    oss << "// Graph\n";
    OSS << "struct VertexType;\n";
    OSS << "using Vertex = " << name << "Vertex;\n";
    if (s.needEdgeList()) {
        auto edgeID = locate(s.mEdgeProperty, g);
        if (g.isPmr(edgeID)) {
            auto edgeType = g.getDependentCppName(ns, edgeID);
            OSS << "using EdgeType = PmrPropertyEdge<Vertex, " << edgeType << ">;\n";
        } else {
            auto edgeType = g.getDependentCppName(ns, edgeID);
            OSS << "using EdgeType = PropertyEdge<Vertex, " << edgeType << ">;\n";
        }
    }
    OSS << "using Edge = " << edgeDescType() << ";\n";
    if (s.mIncidence) {
        OSS << "// IncidenceGraph\n";
        OSS << "using OutEdgeIter = " << outIterType() << ";\n";
        if (s.mBidirectional) {
            OSS << "// BidirectionalGraph\n";
            OSS << "using InEdgeIter = " << inIterType() << ";\n";
        }
    }

    if (s.mReferenceGraph) {
        OSS << "// HierarchyGraph";
        if (s.mAliasGraph) {
            oss << " (Alias)";
        } else {
            oss << " (Separated)";
        }
        oss << "\n";
        OSS << "using Link = " << linkDescType() << ";\n";
        OSS << "using ChildLinkIter = " << childIterType() << ";\n";
        OSS << "using ParentLinkIter = " << parentIterType() << ";\n";
    }
    if (s.mIncidence && s.mAdjacency) {
        OSS << "// AdjacencyGraph\n";
        if (s.needEdgeList()) {
            OSS << "using AdjIter = AdjacencyPropertyIterator<Vertex, OutEdgeIter>;\n";
        } else {
            OSS << "using AdjIter = AdjacencyIterator<Vertex, OutEdgeIter>;\n";
        }
    }
    if (s.mAdjacency && s.mReferenceGraph) {
        if (s.mAliasGraph) {
            if (s.needEdgeList()) {
                OSS << "using ChildIter = AdjacencyPropertyIterator<Vertex, OutEdgeIter>;\n";
            } else {
                OSS << "using ChildIter = AdjacencyIterator<Vertex, OutEdgeIter>;\n";
            }
        } else {
            OSS << "using ChildIter = AdjacencyIterator<Vertex, ChildLinkIter>;\n";
        }
    }
    if (s.mVertexList) {
        OSS << "// VertexListGraph\n";
        if (s.isVector()) {
            if (s.mRecycle) {
                bool bPmr = g.isPmr(vertID);
                if (bPmr) {
                    OSS << "using VertexIter = PmrFilteredVertexIterator<Vertex>;\n";
                } else {
                    OSS << "using VertexIter = FilteredVertexIterator<Vertex>;\n";
                }
            } else {
                OSS << "using VertexIter = IntegerIterator<Vertex>;\n";
            }
        } else {
            OSS << "using VertexIter = VertexIterator<" << vertexListType() << "::const_iterator>;\n";
        }
    }
    if (s.mIncidence && s.mEdgeList) {
        OSS << "// EdgeListGraph\n";
        OSS << "using EdgeIter = " << edgeIterType() << ";\n";
    }
    if (s.mEdgeList && s.mReferenceGraph) {
        OSS << "using LinkIter = " << linkIterType() << ";\n";
    }
    if (s.mReferenceGraph && s.mNamed) {
        OSS << "// NamedGraph && HierarchyGraph\n";
        OSS << "using NamedChildIter = NameFilteredAdjacencyIterator<"
            << name << ", " << stringType(ns) << ", ChildIter>;\n";
    }

    OSS << "// Graph\n";
    OSS << "constexpr static Vertex null_vertex() noexcept {\n";
    OSS << "    return " << nullVertexValue() << ";\n";
    OSS << "}\n";

    if (s.mGarbageCollection) {
        OSS << "// GarbageCollection\n";
        OSS << "void root_vertex(Vertex v) noexcept {\n";
        {
            INDENT();
            OSS << "Expects(mRootBitset[v] == false);\n";
            OSS << "mRootBitset[v] = true;\n";
        }
        OSS << "}\n";
        if (s.isPolymorphic()) {
            for (const auto& c : s.mPolymorphic.mConcepts) {
                const auto& tagID = locate(c.mTag, g);
                const auto& typeID = locate(c.mValue, g);
                auto tagName = g.getDependentCppName(ns, tagID);
                auto typeName = g.getDependentCppName(ns, typeID);

                std::pmr::string handleName(tagName, scratch);
                handleName.pop_back();
                handleName.append("Handle");

                std::pmr::string handleType(scratch);
                handleType.append("GraphImpl::UniqueHandle<");
                handleType.append(name);
                handleType.append(", ");
                handleType.append(tagName);
                handleType.append(", ");
                if (c.mPointer) {
                    if (c.mConst) {
                        handleType.append("const ");
                    }
                    handleType.append(typeName);
                } else {
                    Expects(g.isInstantiation(typeID));
                    auto paramID = g.getFirstTemplateParameter(typeID);
                    auto typeName = g.getDependentCppName(ns, paramID);
                    handleType.append(typeName);
                }
                handleType.append(">");
                OSS << "using " << handleName << " = " << handleType << ";\n";
            }
            oss << "\n";
            for (const auto& c : s.mPolymorphic.mConcepts) {
                const auto& tagID = locate(c.mTag, g);
                const auto& typeID = locate(c.mValue, g);
                auto tagName = g.getDependentCppName(ns, tagID);
                auto typeName = g.getDependentCppName(ns, typeID);

                std::pmr::string viewName(tagName, scratch);
                viewName.pop_back();
                viewName.append("View");

                std::pmr::string viewType(scratch);
                viewType.append("GraphImpl::ViewHandle<");
                viewType.append(name);
                viewType.append(", ");
                viewType.append(tagName);
                viewType.append(", ");
                if (c.mPointer) {
                    if (c.mConst) {
                        viewType.append("const ");
                    }
                    viewType.append(typeName);
                } else {
                    Expects(g.isInstantiation(typeID));
                    auto paramID = g.getFirstTemplateParameter(typeID);
                    auto typeName = g.getDependentCppName(ns, paramID);
                    viewType.append(typeName);
                }
                viewType.append(">");

                OSS << "using " << viewName << " = " << viewType << ";\n";
            }
            oss << "\n";
        } else {
            OSS << "GraphImpl::VectorHandle<" << name << "> persist_vertex(Vertex v) {\n";
            {
                INDENT();
                OSS << "root_vertex(v);\n";
                OSS << "return { this, v };\n";
            }
            OSS << "}\n";
            OSS << "GraphImpl::VectorView<" << name << "> view_vertex(Vertex v) {\n";
            {
                INDENT();
                OSS << "Expects(mRootBitset[v] == true);\n";
                OSS << "return { this, v };\n";
            }
            OSS << "}\n";
        }
        OSS << "void release_vertex(Vertex v) noexcept;\n";
        OSS << "void mark_and_sweep(PmrMemoryResource* scratch) noexcept;\n";
    }

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateIncidenceGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (!s.mIncidence)
        return oss.str();

    const auto& cpp = mStruct;
    auto name = cpp.getDependentName(cpp.mCurrentPath);

    auto scratch = get_allocator().resource();
    std::pmr::string api(cpp.mAPI, scratch);
    if (!cpp.mAPI.empty()) {
        api.append("_API ");
    }

    std::pmr::string space(get_allocator());

    if (!mImpl) {
        oss << "\n";
        OSS << "// IncidenceGraph\n";
        OSS << api << "std::pair<Edge, bool> edge(Vertex u, Vertex v) const noexcept;\n";
        OSS << "Vertex source(Edge e) const noexcept;\n";
        OSS << "Vertex target(Edge e) const noexcept;\n";
        OSS << "IteratorPair<OutEdgeIter> out_edges(Vertex u) const noexcept;\n";
        OSS << s.mVertexSizeType << " out_degree(Vertex u) const noexcept;\n";
    } else {
        OSS << "inline " << name << "::Vertex " << name << "::source(Edge e) const noexcept {\n";
        OSS << "    return e.source;\n";
        OSS << "}\n";
        OSS << "inline " << name << "::Vertex " << name << "::target(Edge e) const noexcept {\n";
        OSS << "    return e.target;\n";
        OSS << "}\n";
        OSS << "inline IteratorPair<" << name << "::OutEdgeIter> " << name << "::out_edges(Vertex u) const noexcept {\n";
        {
            INDENT();
            if (s.isVector()) {
                OSS << "const auto& vert = mVertices[u];\n";
            } else {
                OSS << "const auto& vert = *static_cast<const VertexType*>(u);\n";
            }
            OSS << "return {\n";
            OSS << "    { u, vert.mOutEdges.begin() },\n";
            OSS << "    { u, vert.mOutEdges.end() }\n";
            OSS << "};\n";
        }
        OSS << "}\n";
        OSS << "inline " << s.mVertexSizeType << " " << name << "::out_degree(Vertex u) const noexcept {\n";
        if (s.isVector()) {
            OSS << "    return static_cast<" << s.mVertexSizeType << ">(std::size(mVertices[u].mOutEdges));\n";
        } else {
            OSS << "    return static_cast<" << s.mVertexSizeType << ">(std::size(static_cast<const VertexType*>(u)->mOutEdges));\n";
        }
        OSS << "}\n";
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateBidirectionalGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (!s.mIncidence || !s.mBidirectional)
        return oss.str();

    const auto& cpp = mStruct;
    auto name = cpp.getDependentName(cpp.mCurrentPath);

    std::pmr::string space(get_allocator());
    if (!mImpl) {
        oss << "\n";
        OSS << "// BidirectionalGraph\n";
        OSS << "IteratorPair<InEdgeIter> in_edges(Vertex v) const noexcept;\n";
        OSS << s.mVertexSizeType << " in_degree(Vertex v) const noexcept;\n";
        OSS << s.mVertexSizeType << " degree(Vertex v) const noexcept;\n";
    } else {
        OSS << "inline IteratorPair<" << name << "::InEdgeIter> " << name << "::in_edges(Vertex v) const noexcept {\n";
        {
            INDENT();
            if (s.isVector()) {
                OSS << "const auto& vert = mVertices[v];\n";
            } else {
                OSS << "const auto& vert = *static_cast<const VertexType*>(v);\n";
            }
            OSS << "return {\n";
            OSS << "    { v, vert.mInEdges.begin() },\n";
            OSS << "    { v, vert.mInEdges.end() }\n";
            OSS << "};\n";
        }
        OSS << "}\n";
        OSS << "inline " << s.mVertexSizeType << " " << name << "::in_degree(Vertex v) const noexcept {\n";
        if (s.isVector()) {
            OSS << "    return static_cast<" << s.mVertexSizeType << ">(std::size(mVertices[v].mInEdges));\n";
        } else {
            OSS << "    return static_cast<" << s.mVertexSizeType << ">(std::size(static_cast<const VertexType*>(v)->mInEdges));\n";
        }
        OSS << "}\n";
        OSS << "inline " << s.mVertexSizeType << " " << name << "::degree(Vertex v) const noexcept {\n";
        OSS << "    return out_degree(v) + in_degree(v);\n";
        OSS << "}\n";
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateAdjacencyGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (!s.mIncidence || !s.mAdjacency)
        return oss.str();

    std::pmr::string space(get_allocator());

    const auto& cpp = mStruct;
    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (!mImpl) {
        oss << "\n";
        OSS << "// AdjacencyGraph\n";
        OSS << "IteratorPair<AdjIter> adjacent_vertices(Vertex u) const noexcept;\n";
        if (s.mReferenceGraph) {
            OSS << "IteratorPair<ChildIter> children(Vertex u) const noexcept;\n";
        }
        if (s.mReferenceGraph && (!s.mAliasGraph || s.isBidirectionalOnly())) {
            OSS << "Vertex parent(Vertex v) const noexcept;\n";
        }
    } else {
        OSS << "inline IteratorPair<" << name << "::AdjIter> " << name << "::adjacent_vertices(Vertex u) const noexcept {\n";
        {
            INDENT();
            OSS << "return { out_edges(u) };\n";
        }
        OSS << "}\n";

        if (s.mReferenceGraph) {
            OSS << "inline IteratorPair<" << name << "::ChildIter> " << name << "::children(Vertex u) const noexcept {\n";
            {
                INDENT();
                OSS << "return { child_links(u) };\n";
            }
            OSS << "}\n";
            if (!s.mAliasGraph || s.isBidirectionalOnly()) {
                OSS << "inline " << name << "::Vertex " << name << "::parent(Vertex v) const noexcept {\n";
                {
                    INDENT();
                    if (false) {
                        if (s.isVector()) {
                            OSS << "const auto& vert = mVertices[u];\n";
                        } else {
                            OSS << "const auto& vert = *u;\n";
                        }
                        OSS << "if (vert.mParents.empty()) {\n";
                        OSS << "    return null_vertex();\n";
                        OSS << "}\n";
                        OSS << "return parent(*beg);\n";
                    } else {
                        OSS << "auto [beg, end] = parent_links(v);\n";
                        OSS << "if (beg == end) {\n";
                        OSS << "    return null_vertex();\n";
                        OSS << "}\n";
                        OSS << "return parent(*beg);\n";
                    }
                }
                OSS << "}\n";
            }
        }
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateVertexListGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (!s.mVertexList)
        return oss.str();

    std::pmr::string space(get_allocator());

    const auto& cpp = mStruct;
    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (!mImpl) {
        oss << "\n";
        OSS << "// VertexListGraph\n";
        OSS << "IteratorPair<VertexIter> vertices() const noexcept;\n";
        OSS << s.mVertexSizeType << " num_vertices() const noexcept;\n";
        if (false && s.mColorMap) {
            oss << "\n";
            OSS << "[[nodiscard]] inline PmrVector<uint8_t> colors(PmrMemoryResource* mr) const;\n";
        }
    } else {
        if (s.isVector()) {
            if (s.mRecycle) {
                OSS << "inline IteratorPair<" << name << "::VertexIter> " << name << "::vertices() const noexcept {\n";
                OSS << "    return {\n";
                OSS << "        VertexIter(0, num_vertices(), mEmptyBitset.cbegin()),\n";
                OSS << "        VertexIter(num_vertices(), num_vertices(), mEmptyBitset.cend())\n";
                OSS << "    };\n";
                OSS << "}\n";
            } else {
                OSS << "inline IteratorPair<" << name << "::VertexIter> " << name << "::vertices() const noexcept {\n";
                OSS << "    return { VertexIter(0), VertexIter(num_vertices()) };\n";
                OSS << "}\n";
            }
            OSS << "inline " << s.mVertexSizeType << " " << name << "::num_vertices() const noexcept {\n";
            if (s.mIncidence) {
                OSS << "    return static_cast<" << s.mVertexSizeType << ">(std::size(mVertices));\n";
            } else if (s.isPolymorphic()) {
                OSS << "    return static_cast<" << s.mVertexSizeType << ">(std::size(mVariants));\n";
            } else if (!s.mComponents.empty()) {
                const auto& c = s.mComponents.front();
                OSS << "    return static_cast<" << s.mVertexSizeType << ">(std::size(" << c.mMemberName << "));\n";
            } else {
                Expects(false);
            }
            OSS << "}\n";
        } else {
            OSS << "inline IteratorPair<" << name << "::VertexIter> " << name << "::vertices() const noexcept {\n";
            OSS << "    return { VertexIter(mVertices.begin()), VertexIter(mVertices.end()) };\n";
            OSS << "}\n";
            OSS << s.mVertexSizeType << " " << name << "::num_vertices() const noexcept {\n";
            OSS << "    return static_cast<" << s.mVertexSizeType << ">(std::size(mVertices));\n";
            OSS << "}\n";
        }
        if (false && s.mColorMap) {
            oss << "\n";
            OSS << "[[nodiscard]] inline PmrVector<uint8_t> " << name << "::colors(PmrMemoryResource* mr) const {\n";
            OSS << "    return PmrVector<uint8_t>{ mVertices.size(), mr };\n";
            OSS << "}\n";
        }
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateEdgeListGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (!s.mIncidence || !s.mEdgeList)
        return oss.str();

    std::pmr::string space(get_allocator());

    const auto& cpp = mStruct;
    auto name = cpp.getDependentName(cpp.mCurrentPath);

    auto scratch = get_allocator().resource();
    std::pmr::string api(mStruct.mAPI, scratch);
    if (!mStruct.mAPI.empty()) {
        api.append("_API ");
    }

    if (!mImpl) {
        oss << "\n";
        OSS << "// EdgeListGraph\n";
        OSS << api << "IteratorPair<EdgeIter> edges() const noexcept;\n";
        OSS << api << s.mEdgeSizeType << " num_edges() const noexcept;\n";
        if (s.mReferenceGraph) {
            OSS << api << "IteratorPair<LinkIter> links() const noexcept;\n";
            OSS << api << s.mEdgeSizeType << " num_links() const noexcept;\n";
        }
        if (!s.mIncidence) {
            OSS << "Vertex source(Edge e) const noexcept;\n";
            OSS << "Vertex target(Edge e) const noexcept;\n";
            if (s.mReferenceGraph) {
                OSS << api << "Vertex parent(Link e) const noexcept;\n";
                OSS << api << "Vertex child(Link e) const noexcept;\n";
            }
        }
    } else {
        if (!s.mIncidence) {
            OSS << "inline " << name << "::Vertex " << name << "::source(Edge e) const noexcept {\n";
            OSS << "    return e.source;\n";
            OSS << "}\n";
            OSS << "inline " << name << "::Vertex " << name << "::target(Edge e) const noexcept {\n";
            OSS << "    return e.target;\n";
            OSS << "}\n";
        }
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateMutableGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& cpp = mStruct;
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    std::string_view ns = cpp.mCurrentNamespace;

    std::pmr::string api(mStruct.mAPI, scratch);
    if (!mStruct.mAPI.empty()) {
        api.append("_API ");
    }

    std::pmr::string space(get_allocator());

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (!mImpl) {
        oss << "\n";
        OSS << "// MutableGraph\n";
        copyString(oss, space, addVertexTemplate(true, false));
        OSS << "Vertex " << addVertexFunction(true, false, true) << ";\n";
        if (s.hasProperties()) {
            copyString(oss, space, addVertexTemplate(true, true));
            OSS << "Vertex " << addVertexFunction(true, true, true) << ";\n";
        }
        if (!s.mComponents.empty() && s.isPolymorphic()) {
            const bool bPrecise = false;
            if (bPrecise) {
                OSS << "template <class Type";
                for (const auto& c : s.mComponents) {
                    auto tagID = locate(c.mTagPath, g);
                    auto tagName = g.getDependentCppName(ns, tagID);
                    oss << ", class " << std::string_view{ tagName }.substr(0, tagName.size() - 1);
                }
                oss << ">\n";
                OSS << "Vertex add_vertex(std::tuple<Type&&";
                for (const auto& c : s.mComponents) {
                    auto tagID = locate(c.mTagPath, g);
                    auto tagName = g.getDependentCppName(ns, tagID);
                    oss << ", " << std::string_view{ tagName }.substr(0, tagName.size() - 1) << "&&";
                }
                oss << "> t, Vertex u = null_vertex()) {\n";
                {
                    INDENT();
                    OSS << "return std::apply(\n";
                    OSS << "    [this, u](auto&&... args) {\n";
                    OSS << "        return add_vertex(std::forward<decltype(args)>(args)..., u);\n";
                    OSS << "    },\n";
                    OSS << "    std::move(t));\n";
                }
                OSS << "}\n";
            } else {
                OSS << "template <class Tuple>\n";
                OSS << "Vertex add_vertex_from_tuple(Tuple&& t, Vertex u = null_vertex()) {\n";
                {
                    INDENT();
                    OSS << "return std::apply(\n";
                    OSS << "    [this, u](auto&&... args) {\n";
                    OSS << "        return add_vertex(std::forward<decltype(args)>(args)..., u);\n";
                    OSS << "    },\n";
                    OSS << "    std::forward<Tuple>(t));\n";
                }
                OSS << "}\n";
            }
        }

        if (s.mIncidence) {
            if (s.hasEdgeProperty()) {
                OSS << "template <class... Args>\n";
            }
            OSS << "std::pair<Edge, bool> add_edge(Vertex u, Vertex v";
            if (s.hasEdgeProperty()) {
                oss << ", Args&&... args";
            }
            oss << ");\n";

            if (s.mReferenceGraph) {
                if (s.mAliasGraph && s.hasEdgeProperty()) {
                    OSS << "template <class... Args>\n";
                }
                OSS << "std::pair<Link, bool> add_link(Vertex u, Vertex v";
                if (s.mAliasGraph && s.hasEdgeProperty()) {
                    oss << ", Args&&... args";
                }
                oss << ") noexcept;\n";
            }

            if (s.mReferenceGraph && s.mNamed && s.mUniqueName) {
                OSS << api << "bool detachable(Vertex v) const noexcept;\n";
            }

            OSS << api << "void remove_edge(const OutEdgeIter& iter) noexcept;\n";
            OSS << api << "void remove_edge(Edge e) noexcept;\n";
            OSS << api << "void remove_edges(Vertex u, Vertex v) noexcept;\n";

            if (s.mReferenceGraph) {
                OSS << api << "void remove_link(const ChildLinkIter& iter) noexcept;\n";
                OSS << api << "void remove_link(Link l) noexcept;\n";
                OSS << api << "void remove_links(Vertex u, Vertex v) noexcept;\n";
            }

            if (!(s.isUniqueAddressAliasGraph())) {
                OSS << api << "void clear_out_edges(Vertex u) noexcept;\n";
            }

            if (s.mBidirectional) {
                OSS << api << "void clear_in_edges(Vertex v) noexcept;\n";
            }

            if (!(s.isUniqueAddressAliasGraph() || s.isAliasDirected())) {
                OSS << api << "void clear_edges(Vertex v) noexcept;\n";
            }

            if (s.mReferenceGraph) {
                if (s.mAliasGraph) {
                    Expects(!s.mUndirected);
                    if (!s.mNamed && s.mUniqueName) {
                        OSS << api << "void clear_children(Vertex u) noexcept;\n";
                    }
                    if (s.isBidirectionalOnly()) {
                        OSS << api << "void clear_parents(Vertex v) noexcept;\n";
                    }
                } else {
                    if (!s.mNamed && s.mUniqueName) {
                        OSS << api << "void clear_children(Vertex u) noexcept;\n";
                    }
                    OSS << api << "void clear_parents(Vertex v) noexcept;\n";
                }
                if (!s.mNamed && s.mUniqueName) {
                    OSS << api << "void clear_links(Vertex v) noexcept;\n";
                }
            }

            if (!(s.isUniqueAddressGraph() || s.isAliasDirected())) {
                OSS << api << "void clear_vertex(Vertex v) noexcept;\n";
            }
        }

        OSS << api << "void remove_vertex(Vertex u) noexcept;\n";

        if (s.hasVertexProperty()) {
            auto vpID = locate(s.mVertexProperty, g);
            auto vp = g.getDependentCppName(cpp.mCurrentNamespace, vpID);
            oss << "\n";
            OSS << "// PropertyMap (Vertex)\n";
            OSS << api << "const " << vp << "& operator[](Vertex v) const noexcept;\n";
            OSS << api << vp << "& operator[](Vertex v) noexcept;\n";

            if (false) {
                OSS << "const " << vp << "& operator[](Vertex v) const noexcept;\n";
                OSS << vp << "& operator[](Vertex v) noexcept;\n";
            }
        }

        if (s.hasEdgeProperty()) {
            auto epID = locate(s.mEdgeProperty, g);
            auto ep = g.getDependentCppName(cpp.mCurrentNamespace, epID);
            oss << "\n";
            OSS << "// PropertyMap (Edge)\n";
            OSS << "const " << ep << "& operator[](Edge e) const noexcept;\n";
            OSS << ep << "& operator[](Edge e) noexcept;\n";
        }
    } else {
        copyString(oss, space, addVertexTemplate(true, false));
        OSS << "inline " << name << "::Vertex " << name << "::" << addVertexFunction(true, false, false) << " {\n";
        {
            INDENT();
            copyString(oss, space, addVertexVectorImpl(true, false));
        }
        OSS << "}\n";

        if (s.hasProperties()) {
            copyString(oss, space, addVertexTemplate(true, true));
            OSS << "inline " << name << "::Vertex " << name << "::" << addVertexFunction(true, true, false) << " {\n";
            {
                INDENT();
                copyString(oss, space, addVertexVectorImpl(true, true));
            }
            OSS << "}\n";
        }

        if (s.mIncidence) {
            if (s.hasEdgeProperty()) {
                OSS << "template <class... Args>\n";
            }
            OSS << "inline std::pair<" << name << "::Edge, bool> " << name << "::add_edge(Vertex u, Vertex v";
            if (s.hasEdgeProperty()) {
                oss << ", Args&&... args";
            }
            oss << ") {\n";
            {
                INDENT();
                if (s.needEdgeList()) {
                    Expects(s.hasEdgeProperty());
                    OSS << "auto edgeIter = mEdges.emplace(mEdges.end(), u, v";
                    if (s.hasEdgeProperty()) {
                        oss << ", std::forward<Args>(args)...";
                    }
                    oss << ");\n";
                    if (s.isDirectedOnly()) {
                        Expects(false);
                    } else {
                        if (s.isVector()) {
                            OSS << "auto& s = mVertices[u];\n";
                            OSS << "auto& t = mVertices[v];\n";
                        } else {
                            OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                            OSS << "auto& t = *static_cast<VertexType*>(v);\n";
                        }
                        if (s.isBidirectionalOnly()) {
                            OSS << "s.mOutEdges.emplace_back(v, edgeIter);\n";
                            OSS << "t.mInEdges.emplace_back(u, edgeIter);\n";
                        } else {
                            OSS << "s.mOutEdges.emplace_back(v, edgeIter);\n";
                            OSS << "t.mOutEdges.emplace_back(u, edgeIter);\n";
                        }
                    }
                    OSS << "return { { u, v, &edgeIter->property }, true };\n";
                } else {

                    if (s.isDirectedOnly()) {
                        if (s.isVector()) {
                            OSS << "auto& s = mVertices[u];\n";
                        } else {
                            OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                        }
                        OSS << "s.mOutEdges.emplace_back(v);\n";
                    } else {
                        if (s.isVector()) {
                            OSS << "auto& s = mVertices[u];\n";
                            OSS << "auto& t = mVertices[v];\n";
                        } else {
                            OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                            OSS << "auto& t = *static_cast<VertexType*>(v);\n";
                        }
                        if (s.isBidirectionalOnly()) {
                            OSS << "s.mOutEdges.emplace_back(v);\n";
                            OSS << "t.mInEdges.emplace_back(u);\n";
                        } else {
                            OSS << "s.mOutEdges.emplace_back(v);\n";
                            OSS << "t.mOutEdges.emplace_back(u);\n";
                        }
                    }
                    OSS << "return { { u, v }, true };\n";
                }
            }
            OSS << "}\n";

            if (s.mReferenceGraph) {
                if (s.mAliasGraph && s.hasEdgeProperty()) {
                    OSS << "template <class... Args>\n";
                }
                OSS << "inline std::pair<" << name << "::Link, bool> " << name << "::add_link(Vertex u, Vertex v";
                if (s.mAliasGraph && s.hasEdgeProperty()) {
                    oss << ", Args&&... args";
                }
                oss << ") noexcept {\n";
                {
                    INDENT();
                    if (s.mAliasGraph) {
                        OSS << "return add_edge(u, v";
                        if (s.hasEdgeProperty()) {
                            oss << ", std::forward<Args>(args)...";
                        }
                        oss << ");\n";
                    } else {
                        if (s.isVector()) {
                            OSS << "mVertices[u].mChildren.emplace_back(v);\n";
                            OSS << "mVertices[v].mParents.emplace_back(u);\n";
                        } else {
                            OSS << "static_cast<VertexType*>(u)->mChildren.emplace_back(v);\n";
                            OSS << "static_cast<VertexType*>(v)->mParents.emplace_back(u);\n";
                        }
                        OSS << "return { { u, v }, true };\n";
                    }
                }
                OSS << "}\n";
            }
        }
        if (s.hasVertexProperty()) {
            auto vpID = locate(s.mVertexProperty, g);
            auto vp = g.getDependentCppName(cpp.mCurrentNamespace, vpID);
            if (false) {
                OSS << "inline const " << vp << "& " << name << "::operator[](Vertex v) const noexcept {\n";
                {
                    INDENT();
                    if (s.isVector()) {
                        OSS << "return mVertices[v].mProperty;\n";
                    } else {
                        OSS << "Expects(v);\n";
                        OSS << "auto* vert = static_cast<const VertexType*>(v);\n";
                        OSS << "return vert->mProperty;\n";
                    }
                }
                OSS << "}\n";
                OSS << "inline " << vp << "& " << name << "::operator[](Vertex v) noexcept {\n";
                {
                    INDENT();
                    if (s.isVector()) {
                        OSS << "return mVertices[v].mProperty;\n";
                    } else {
                        OSS << "Expects(v);\n";
                        OSS << "auto* vert = static_cast<VertexType*>(v);\n";
                        OSS << "return vert->mProperty;\n";
                    }
                }
                OSS << "}\n";
            }
        }
        if (s.hasEdgeProperty()) {
            auto epID = locate(s.mEdgeProperty, g);
            auto ep = g.getDependentCppName(cpp.mCurrentNamespace, epID);
            OSS << "inline const " << ep << "& " << name << "::operator[](Edge e) const noexcept {\n";
            {
                INDENT();
                OSS << "return *static_cast<const " << ep << "*>(e.property);\n";
            }
            OSS << "}\n";
            OSS << "inline " << ep << "& " << name << "::operator[](Edge e) noexcept {\n";
            {
                INDENT();
                OSS << "return *static_cast<" << ep << "*>(e.property);\n";
            }
            OSS << "}\n";
        }
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateReferenceGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    std::pmr::string space(get_allocator());

    if (!s.mReferenceGraph)
        return oss.str();

    const auto& cpp = mStruct;
    auto name = cpp.getDependentName(cpp.mCurrentPath);

    auto scratch = get_allocator().resource();
    std::pmr::string api(mStruct.mAPI, scratch);
    if (!mStruct.mAPI.empty()) {
        api.append("_API ");
    }

    if (!mImpl) {
        oss << "\n";
        if (s.mAliasGraph) {
            OSS << "// HierarchyGraph (Alias)\n";
        } else {
            OSS << "// HierarchyGraph (Separated)\n";
        }
        OSS << api << "std::pair<Link, bool> linked(Vertex u, Vertex v) const noexcept;\n";
        if (!s.mAliasGraph || s.isBidirectionalOnly()) {
            OSS << api << "bool descendant(Vertex ancestor, Vertex v) const noexcept;\n";
        }
        OSS << "Vertex parent(Link e) const noexcept;\n";
        OSS << "Vertex child(Link e) const noexcept;\n";
        OSS << "IteratorPair<ChildLinkIter> child_links(Vertex u) const noexcept;\n";
        OSS << s.mVertexSizeType << " num_children(Vertex u) const noexcept;\n";
        if (!s.mAliasGraph || s.isBidirectionalOnly()) {
            OSS << "IteratorPair<ParentLinkIter> parent_links(Vertex v) const noexcept;\n";
            OSS << s.mVertexSizeType << " num_parents(Vertex v) const noexcept;\n";
            OSS << s.mVertexSizeType << " link_degree(Vertex v) const noexcept;\n";
        }
        if (s.mReferenceGraph && s.mNamed && s.mUniqueName) {
            OSS << api << "bool uniqueAddress() const noexcept;\n";
        }
    } else {
        OSS << "inline " << name << "::Vertex " << name << "::parent(Link e) const noexcept {\n";
        OSS << "    return e.source;\n";
        OSS << "}\n";
        OSS << "inline " << name << "::Vertex " << name << "::child(Link e) const noexcept {\n";
        OSS << "    return e.target;\n";
        OSS << "}\n";
        OSS << "inline IteratorPair<" << name << "::ChildLinkIter> " << name << "::child_links(Vertex u) const noexcept {\n";
        {
            INDENT();
            if (s.mAliasGraph) {
                OSS << "return out_edges(u);\n";
            } else {
                if (s.isVector()) {
                    OSS << "const auto& vert = mVertices[u];\n";
                } else {
                    OSS << "const auto& vert = *static_cast<const VertexType*>(u);\n";
                }
                OSS << "return {\n";
                OSS << "    { u, vert.mChildren.begin() },\n";
                OSS << "    { u, vert.mChildren.end() }\n";
                OSS << "};\n";
            }
        }
        OSS << "}\n";
        OSS << "inline " << s.mVertexSizeType << " " << name << "::num_children(Vertex u) const noexcept {\n";
        {
            INDENT();
            if (s.mAliasGraph) {
                OSS << "return out_degree(u);\n";
            } else {
                if (s.isVector()) {
                    OSS << "return static_cast<" << s.mVertexSizeType << ">(std::size(mVertices[u].mChildren));\n";
                } else {
                    OSS << "return static_cast<" << s.mVertexSizeType << ">(std::size(static_cast<const VertexType*>(u)->mChildren));\n";
                }
            }
        }
        OSS << "}\n";

        if (!s.mAliasGraph || s.isBidirectionalOnly()) {
            OSS << "inline IteratorPair<" << name << "::ParentLinkIter> " << name << "::parent_links(Vertex v) const noexcept {\n";
            {
                INDENT();
                if (s.mAliasGraph) {
                    OSS << "return in_edges(v);\n";
                } else {
                    if (s.isVector()) {
                        OSS << "const auto& vert = mVertices[v];\n";
                    } else {
                        OSS << "const auto& vert = *static_cast<const VertexType*>(v);\n";
                    }
                    OSS << "return {\n";
                    OSS << "    { v, vert.mParents.begin() },\n";
                    OSS << "    { v, vert.mParents.end() }\n";
                    OSS << "};\n";
                }
            }
            OSS << "}\n";
            OSS << "inline " << s.mVertexSizeType << " " << name << "::num_parents(Vertex v) const noexcept {\n";
            {
                INDENT();
                if (s.mAliasGraph) {
                    OSS << "return in_degree(v);\n";
                } else {
                    if (s.isVector()) {
                        OSS << "return static_cast<" << s.mVertexSizeType << ">(std::size(mVertices[v].mParents));\n";
                    } else {
                        OSS << "return static_cast<" << s.mVertexSizeType << ">(std::size(static_cast<const VertexType*>(v)->mParents));\n";
                    }
                }
            }
            OSS << "}\n";
            OSS << "inline " << s.mVertexSizeType << " " << name << "::link_degree(Vertex v) const noexcept {\n";
            OSS << "    return num_children(v) + num_parents(v);\n";
            OSS << "}\n";
        }
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generatePropertyGraph_h() const {
    auto scratch = get_allocator().resource();
    const auto& cpp = mStruct;
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    std::string_view ns = cpp.mCurrentNamespace;
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());

    if (!s.isVector())
        return oss.str();

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (!mImpl) {
        if (!s.mComponents.empty()) {
            oss << "\n";
            oss << "// PropertyMap\n";
        }
        for (const auto& c : s.mComponents) {
            const auto& tagID = locate(c.mTagPath, g);
            auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
            OSS << "auto get(" << tagName << ") const;\n";
            OSS << "auto get(" << tagName << ");\n";
        }
    } else {
        if (!s.mComponents.empty()) {
            oss << "\n";
            oss << "// PropertyMap\n";
        }
        for (const auto& c : s.mComponents) {
            const auto& tagID = locate(c.mTagPath, g);
            auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
            OSS << "inline auto " << name << "::get(" << tagName << ") const {\n";
            OSS << "    return GraphImpl::makeGetterSetter(" << c.mMemberName << ");\n";
            OSS << "}\n";
            OSS << "inline auto " << name << "::get(" << tagName << ") {\n";
            OSS << "    return GraphImpl::makeGetterSetter(" << c.mMemberName << ");\n";
            OSS << "}\n";
        }
    }

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateComponentGraph_h() const {
    auto scratch = get_allocator().resource();
    const auto& cpp = mStruct;
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    std::string_view ns = cpp.mCurrentNamespace;
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (s.mComponents.empty()) {
        return oss.str();
    }

    if (!mImpl) {
        oss << "\n";
        OSS << "// ComponentGraph\n";
        for (const auto& c : s.mComponents) {
            Expects(!c.mTagPath.empty());
            auto tagID = locate(c.mTagPath, g);
            auto componentID = locate(c.mValuePath, g);
            auto tagName = g.getDependentCppName(ns, tagID);
            auto typeName = g.getDependentCppName(ns, componentID);
            if (c.mCounterName.empty()) {
                OSS << "const " << typeName << "& get(" << tagName << ", Vertex u) const;\n";
                if (!c.mReadOnly) {
                    OSS << typeName << "& get(" << tagName << ", Vertex u);\n";
                    if (false) {
                        if (needView(typeName)) {
                            OSS << "void set(" << tagName << ", Vertex u, " << getViewOrMove(typeName, scratch) << " rhs);\n";
                        } else {
                            OSS << "void set(" << tagName << ", Vertex u, " << getViewOrMove(typeName, scratch) << " rhs);\n";
                            OSS << "void set(" << tagName << ", Vertex u, " << getViewOrConstRef(typeName, scratch) << " rhs);\n";
                        }
                    }
                }
                continue;
            }
            OSS << "std::span<const " << typeName << "> get(" << tagName << ", Vertex u) const;\n";
            if (!c.mReadOnly) {
                OSS << "std::span<" << typeName << "> get(" << tagName << ", Vertex u);\n";
            }
            OSS << "void resizeChunk(" << tagName << ", uint32_t chunkSize);\n";
        }
    } else {
        for (const auto& c : s.mComponents) {
            Expects(!c.mTagPath.empty());
            auto tagID = locate(c.mTagPath, g);
            auto componentID = locate(c.mValuePath, g);
            auto tagName = g.getDependentCppName(ns, tagID);
            auto typeName = g.getDependentCppName(ns, componentID);
            if (c.mCounterName.empty()) {
                OSS << "inline const " << typeName << "& " << name << "::get(" << tagName << ", Vertex u) const {\n";
                {
                    INDENT();
                    if (s.isVector()) {
                        OSS << "Expects(u < " << c.mMemberName << ".size());\n";
                        OSS << "return " << c.mMemberName << "[u];\n";
                    } else {
                        OSS << "return static_cast<const VertexType*>(u)->" << c.mMemberName << ";\n";
                    }
                }
                OSS << "}\n";
                if (!c.mReadOnly) {
                    OSS << "inline " << typeName << "& " << name << "::get(" << tagName << ", Vertex u) {\n";
                    {
                        INDENT();
                        if (s.isVector()) {
                            OSS << "Expects(u < " << c.mMemberName << ".size());\n";
                            OSS << "return " << c.mMemberName << "[u];\n";
                        } else {
                            OSS << "return static_cast<VertexType*>(u)->" << c.mMemberName << ";\n";
                        }
                    }
                    OSS << "}\n";
                }
                continue;
            }
            OSS << "inline std::span<const " << typeName << "> " << name << "::get(" << tagName << ", Vertex u) const {\n";
            {
                INDENT();
                Expects(s.isVector());
                OSS << "Expects(u < " << c.mMemberName << ".size());\n";
                OSS << "return { &" << c.mMemberName << "[static_cast<size_t>(u) * " << c.mCounterName << "], "
                    << c.mCounterName << " };\n";
            }
            OSS << "}\n";
            if (!c.mReadOnly) {
                OSS << "inline std::span<" << typeName << "> " << name << "::get(" << tagName << ", Vertex u) {\n";
                {
                    INDENT();
                    Expects(s.isVector());
                    OSS << "Expects(static_cast<size_t>(u) * " << c.mCounterName << " < " << c.mMemberName << ".size());\n";
                    OSS << "return { &" << c.mMemberName << "[static_cast<size_t>(u) * " << c.mCounterName << "], "
                        << c.mCounterName << " };\n";
                }
                OSS << "}\n";
            }
            OSS << "inline void " << name << "::resizeChunk(" << tagName << ", uint32_t chunkSize) {\n";
            {
                INDENT();
                OSS << "Expects(num_vertices() == 0);\n";
                OSS << "Expects(chunkSize);\n";
                OSS << c.mCounterName << " = chunkSize;\n";
            }
            OSS << "}\n";
        }
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateNamedGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& s = *mGraph;

    const auto& cpp = mStruct;
    auto name = cpp.getDependentName(cpp.mCurrentPath);
    if (!s.mNamed) {
        return oss.str();
    }
    if (!mImpl) {
        oss << "\n";
        OSS << "// NamedGraph\n";
        OSS << stringViewType() << " name(Vertex u) const noexcept;\n";
    } else {
        OSS << "inline " << stringViewType() << " " << name << "::name(Vertex u) const noexcept {\n";
        {
            INDENT();
            if (s.mNamedConcept.mComponent) {
                const auto& c = s.getComponent(s.mNamedConcept.mComponentName);
                if (s.mNamedConcept.mComponentMemberName.empty()) {
                    if (s.isVector()) {
                        OSS << "return " << c.mMemberName << "[u];\n";
                    } else {
                        OSS << "return static_cast<const VertexType*>(u)->"
                            << c.mMemberName << ";\n";
                    }
                } else {
                    if (s.isVector()) {
                        OSS << "return " << c.mMemberName << "[u]."
                            << s.mNamedConcept.mComponentMemberName << ";\n";
                    } else {
                        OSS << "return static_cast<const VertexType*>(u)->"
                            << c.mMemberName << "."
                            << s.mNamedConcept.mComponentMemberName << ";\n";
                    }
                }
            } else {
                Expects(false);
            }
        }
        OSS << "}\n";
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateParentGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    std::pmr::string space(get_allocator());
    auto scratch = get_allocator().resource();
    std::pmr::string api(mStruct.mAPI, scratch);
    if (!mStruct.mAPI.empty()) {
        api.append("_API ");
    }

    const auto& cpp = mStruct;
    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (s.mNamed && s.mReferenceGraph) {
        if (!mImpl) {
            oss << "\n";
            oss << "// NamedGraph && HierarchyGraph\n";
            OSS << api << "Vertex find_child(Vertex u, " << stringViewType() << " name0) const noexcept;\n";
        }
    }

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateAddressableGraph_h() const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios::out, scratch);
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& cpp = mStruct;
    std::string_view ns = cpp.mCurrentNamespace;
    const auto& s = *mGraph;
    std::pmr::string space(get_allocator());

    std::pmr::string api(mStruct.mAPI, scratch);
    if (!mStruct.mAPI.empty()) {
        api.append("_API ");
    }

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (!mImpl) {
        if (s.mAddressable) {
            Expects(s.mNamed && s.mReferenceGraph);
            oss << "\n";
            oss << "// AddressableGraph\n";
            OSS << api << "bool exists(" << stringViewType() << " absPath) const noexcept;\n";
            OSS << api << "Vertex locate(" << stringViewType() << " absPath) const noexcept;\n";
            OSS << api << "Vertex locate_relative(Vertex u, " << stringViewType() << " relPath) const noexcept;\n";

            auto strType = stringType();
            std::pmr::string strPath(strType, scratch);
            convertTypename(strPath);
            auto strID = g.lookupType(ns, strPath);
            bool bPmrString = g.isPmr(strID);

            OSS << api << stringType(ns) << " get_path(Vertex u";
            if (bPmrString) {
                oss << ", PmrMemoryResource* mr";
            }
            oss << ") const;\n";

            OSS << api << stringType(ns) << " get_relative_path(Vertex u";
            if (bPmrString) {
                oss << ", PmrMemoryResource* mr";
            }
            oss << ") const;\n";

            OSS << api << stringType(ns) << " get_relative_path(Vertex v, Vertex ancestor";
            if (bPmrString) {
                oss << ", PmrMemoryResource* mr";
            }
            oss << ") const;\n";
        }
    } else {
        // noop
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateUuidGraph_h() const {
    auto scratch = get_allocator().resource();
    const auto& cpp = mStruct;
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());

    std::pmr::string api(mStruct.mAPI, scratch);
    if (!mStruct.mAPI.empty()) {
        api.append("_API ");
    }

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (!mImpl) {
        if (!s.mVertexMaps.empty()) {
            oss << "\n";
            oss << "// UuidGraph\n";
            for (const auto& m : s.mVertexMaps) {
                const auto& c = s.getComponent(m.mComponentName);
                const auto& keyID = locate(m.mKeyType, g);
                auto typeName = g.getDependentCppName(cpp.mCurrentNamespace, keyID);
                OSS << api << "bool contains("
                    << getViewOrConstRef(typeName, scratch)
                    << " key) const noexcept;\n";

                OSS << api << "Vertex at("
                    << getViewOrConstRef(typeName, scratch)
                    << " key) const;\n";

                OSS << api << "Vertex find("
                    << getViewOrConstRef(typeName, scratch)
                    << " key) const noexcept;\n";
            }
        }
    }

    return oss.str();
}

namespace {

void outputPolymorphicGet(
    std::ostream& oss, std::pmr::string& space,
    const ObjectGraphBuilder& builder,
    const CppStructBuilder& cpp, const SyntaxGraph& g,
    std::string_view name, const Graph& s, bool bInline) {
    std::string_view ns = cpp.mCurrentNamespace;
    for (const auto& c : s.mPolymorphic.mConcepts) {
        const auto& tagID = locate(c.mTag, g);
        const auto& typeID = locate(c.mValue, g);
        auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
        auto typeName = g.getDependentCppName(cpp.mCurrentNamespace, typeID);
        std::string_view constName = c.mConst ? " const" : "";
        std::string_view pointerName = c.mPointer ? "*" : "";
        std::string_view inlineName = bInline ? "inline " : "";
        if (!bInline) {
            oss << "\n";
        }
        OSS << inlineName << typeName << constName << pointerName << " const& " << name << "::get(" << tagName << ", Vertex u) const {\n";
        {
            INDENT();
            if (s.isVector()) {
                OSS << "const auto& t = std::get<" << builder.handleElemType(c, ns, true) << ">(mVariants[u]);\n";
            } else {
                OSS << "const auto& t = std::get<" << builder.handleElemType(c, ns, true) << ">(\n";
                OSS << "    static_cast<const VertexType*>(u)->mVariant);\n";
            }
            if (c.isIntrusive()) {
                OSS << "return t.mValue;\n";
            } else {
                if (s.isVector()) {
                    OSS << "return " << c.mMemberName << "[t.mValue];\n";
                } else {
                    OSS << "return *t.mValue;\n";
                }
            }
        }
        OSS << "}\n";
        if (!bInline) {
            oss << "\n";
        }
        OSS << inlineName << typeName << constName << pointerName << "& " << name << "::get(" << tagName << ", Vertex u) {\n";
        {
            INDENT();
            if (s.isVector()) {
                OSS << "auto& t = std::get<" << builder.handleElemType(c, ns, true) << ">(mVariants[u]);\n";
            } else {
                OSS << "auto& t = std::get<" << builder.handleElemType(c, ns, true) << ">(\n";
                OSS << "    static_cast<VertexType*>(u)->mVariant);\n";
            }
            if (c.isIntrusive()) {
                OSS << "return t.mValue;\n";
            } else {
                if (s.isVector()) {
                    OSS << "return " << c.mMemberName << "[t.mValue];\n";
                } else {
                    OSS << "return *t.mValue;\n";
                }
            }
        }
        OSS << "}\n";
    }
}

} // namespace

std::pmr::string ObjectGraphBuilder::generatePolymorphicGraph_h() const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios::out, scratch);
    const auto& cpp = mStruct;
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    std::string_view ns = cpp.mCurrentNamespace;

    std::pmr::string space(get_allocator());
    if (!s.isPolymorphic())
        return oss.str();

    std::pmr::string api(get_allocator());
    api = mStruct.mAPI;
    if (!api.empty()) {
        api.append("_API ");
    }

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (!mImpl) {
        oss << "\n";
        OSS << "// PolymorphicGraph\n";
        OSS << "size_t index(Vertex v) const noexcept;\n";
        if (s.isVector()) {
            OSS << s.mVertexSizeType << " object_id(Vertex v) const noexcept;\n";
        }
        OSS << "template <class Tag>\n";
        OSS << "bool holds_tag(Vertex v) const noexcept;\n";
        OSS << "template <class Type>\n";
        OSS << "const Type& get(Vertex v) const noexcept;\n";
        OSS << "template <class Type>\n";
        OSS << "Type& get(Vertex v) noexcept;\n";
        OSS << "template <class... Ts>\n";
        OSS << "auto visit_vertex(Vertex v, Ts&&... args) const;\n";
        OSS << "template <class... Ts>\n";
        OSS << "auto visit_vertex(Vertex v, Ts&&... args);\n";
        OSS << "template <class... Ts>\n";
        OSS << "auto visit_vertex_tag(Vertex v, Ts&&... args) const;\n";
        OSS << "template <class... Ts>\n";
        OSS << "auto visit_vertex_tag(Vertex v, Ts&&... args);\n";
        for (const auto& c : s.mPolymorphic.mConcepts) {
            const auto& tagID = locate(c.mTag, g);
            const auto& typeID = locate(c.mValue, g);
            auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
            auto typeName = g.getDependentCppName(cpp.mCurrentNamespace, typeID);
            std::string_view constName = c.mConst ? " const" : "";
            std::string_view pointerName = c.mPointer ? "*" : "";
            OSS << typeName << constName << pointerName << " const& get(" << tagName << ", Vertex u) const;\n";
            OSS << typeName << constName << pointerName << "& get(" << tagName << ", Vertex u);\n";
        }
        if (s.mGarbageCollection) {
            auto name = cpp.getDependentName(cpp.mCurrentPath);
            for (const auto& c : s.mPolymorphic.mConcepts) {
                const auto& tagID = locate(c.mTag, g);
                const auto& typeID = locate(c.mValue, g);
                auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
                auto typeName = g.getDependentCppName(cpp.mCurrentNamespace, typeID);
                std::pmr::string name(tagName, scratch);
                name.pop_back();
                name.append("Handle");
                OSS << name << " persist_vertex(" << tagName << ", Vertex v);\n";

                std::pmr::string view(tagName, scratch);
                view.pop_back();
                view.append("View");
                OSS << view << " view_vertex(" << tagName << ", Vertex v);\n";
            }
        }
    } else {
        OSS << "inline size_t " << name << "::index(Vertex v) const noexcept {\n";
        {
            INDENT();
            if (s.isVector()) {
                OSS << "return mVariants[v].index();\n";
            } else {
                OSS << "return static_cast<const VertexType*>(v)->mVariant.index();\n";
            }
        }
        OSS << "}\n";
        if (s.isVector()) {
            OSS << "inline " << s.mVertexSizeType << " " << name << "::object_id(Vertex v) const noexcept {\n";
            {
                INDENT();
                OSS << "return visit(\n";
                {
                    INDENT();
                    OSS << "overloaded{\n";
                    {
                        INDENT();
                        int count = 0;
                        for (const auto& c : s.mPolymorphic.mConcepts) {
                            if (count++)
                                oss << ",\n";
                            OSS << "[&](const " << handleElemType(c, ns, true) << "&";
                            if (c.isIntrusive()) {
                                oss << ") {\n";
                            } else {
                                oss << " t) {\n";
                            }
                            {
                                INDENT();
                                if (c.isIntrusive()) {
                                    OSS << "return std::numeric_limits<"
                                        << s.mVertexSizeType << ">::max();\n";
                                } else {
                                    OSS << "return t.mValue;\n";
                                }
                            }
                            OSS << "}";
                        }
                        oss << " },\n";
                    }
                    if (s.isVector()) {
                        OSS << "mVariants[v]);\n";
                    } else {
                        OSS << "static_cast<const VertexType*>(v)->mVariant);\n";
                    }
                }
            }
            OSS << "}\n";
        }
        OSS << "template <class Tag>\n";
        OSS << "bool " << name << "::holds_tag(Vertex v) const noexcept {\n";
        {
            INDENT();
            OSS;
            int count = 0;
            for (const auto& c : s.mPolymorphic.mConcepts) {
                if (count++) {
                    oss << " else ";
                }
                oss << "if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<Tag>>, "
                    << cpp.getDependentName(c.mTag) << ">) {\n";
                {
                    INDENT();
                    if (s.isVector()) {
                        OSS << "return std::holds_alternative<" << handleElemType(c, ns, true) << ">(mVariants[v]);\n";
                    } else {
                        OSS << "return std::holds_alternative<" << handleElemType(c, ns, true) << ">(static_cast<const VertexType*>(v)->mVariant);\n";
                    }
                }
                OSS << "}";
            }
            oss << " else {\n";
            OSS << "    static_assert(!sizeof(Tag), \"Tag not in Graph\");\n";
            OSS << "}\n";
        }
        OSS << "}\n";

        outputPolymorphicGet(oss, space, *this, cpp, g, name, s, true);

        auto generateGet = [&](bool bConst) {
            std::string valueType = "Type";
            OSS;
            int count = 0;
            for (const auto& c : s.mPolymorphic.mConcepts) {
                if (count++) {
                    oss << " else ";
                }
                oss << "if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<" << valueType << ">>, ";
                oss << cpp.getDependentName(c.mValue);
                oss << ">) {\n";
                {
                    INDENT();
                    if (!c.isIntrusive()) {
                        OSS << "const ";
                    } else {
                        OSS;
                        if (bConst) {
                            oss << "const ";
                        }
                    }
                    if (s.isVector()) {
                        oss << "auto& t = std::get<" << handleElemType(c, ns, true)
                            << ">(mVariants[v]);\n";
                    } else {
                        oss << "auto& t = std::get<" << handleElemType(c, ns, true)
                            << ">(static_cast<VertexType*>(v)->mVariant);\n";
                    }

                    if (c.isIntrusive()) {
                        OSS << "return t.mValue;\n";
                    } else {
                        if (s.isVector()) {
                            OSS << "return " << c.mMemberName << "[t.mValue];\n";
                        } else {
                            OSS << "return *t.mValue;\n";
                        }
                    }
                }
                OSS << "}";
            }
            oss << " else {\n";
            OSS << "    static_assert(!sizeof(Type), \"Type not in Graph\");\n";
            OSS << "}\n";
        };

        OSS << "template <class Type>\n";
        OSS << "const Type& " << name << "::get(Vertex v) const noexcept {\n";
        {
            INDENT();
            generateGet(true);
        }
        OSS << "}\n";

        OSS << "template <class Type>\n";
        OSS << "Type& " << name << "::get(Vertex v) noexcept {\n";
        {
            INDENT();
            generateGet(false);
        }
        OSS << "}\n";

        auto outputVisit = [&](bool bConst) {
            INDENT();
            OSS << "auto visitor = overloaded{ std::forward<Ts>(args)... };\n";
            OSS << "return visit(\n";
            {
                INDENT();
                OSS << "overloaded{\n";
                {
                    INDENT();
                    int count = 0;
                    for (const auto& c : s.mPolymorphic.mConcepts) {
                        if (count++)
                            oss << ",\n";
                        OSS << "[&](const " << handleElemType(c, ns, true) << "& t) {\n";
                        {
                            INDENT();
                            if (c.isIntrusive()) {
                                OSS << "return visitor(t.mValue);\n";
                            } else {
                                if (s.isVector()) {
                                    OSS << "return visitor(" << c.mMemberName << "[t.mValue]);\n";
                                } else {
                                    OSS << "return visitor(*t.mValue);\n";
                                }
                            }
                        }
                        OSS << "}";
                    }
                    oss << " },\n";
                }
                if (s.isVector()) {
                    OSS << "mVariants[v]);\n";
                } else {
                    if (bConst) {
                        OSS << "static_cast<const VertexType*>(v)->mVariant);\n";
                    } else {
                        OSS << "static_cast<VertexType*>(v)->mVariant);\n";
                    }
                }
            }
        };

        auto outputVisit2 = [&](bool bConst) {
            INDENT();
            OSS << "auto visitor = overloaded{ std::forward<Ts>(args)... };\n";
            if (bConst) {
                OSS << "const auto& var = mVariants[v];\n";
            } else {
                OSS << "auto& var = mVariants[v];\n";
            }
            OSS << "switch (var.index()) {\n";
            {
                int count = 0;
                for (const auto& c : s.mPolymorphic.mConcepts) {
                    OSS << "case " << count << ":\n";
                    if (c.isIntrusive()) {
                        OSS << "    return visitor("
                            << "std::get<"
                            << handleElemType(c, ns, true) << ">(var)"
                            << ".mValue);\n";
                    } else {
                        if (s.isVector()) {
                            OSS << "    return visitor(" << c.mMemberName << "["
                                << "std::get<"
                                << handleElemType(c, ns, true) << ">(var)"
                                << ".mValue]);\n";
                        } else {
                            OSS << "    return visitor(*";
                            oss << "std::get<"
                                << handleElemType(c, ns, true) << ">(var)";
                            oss << ".mValue);\n";
                        }
                    }
                    ++count;
                }
                // OSS << "default: std::runtime_error(\"invalid vertex polymorphic type\");\n";
                OSS << "default:\n";
                OSS << "    std::terminate();\n";
            }
            OSS << "}\n";
        };

        OSS << "template <class... Ts>\n";
        OSS << "auto " << name << "::visit_vertex(Vertex v, Ts&&... args) const {\n";
        outputVisit2(true);
        OSS << "}\n";
        OSS << "template <class... Ts>\n";
        OSS << "auto " << name << "::visit_vertex(Vertex v, Ts&&... args) {\n";
        outputVisit2(false);
        OSS << "}\n";

        auto outputVisitTag = [&](bool bConst) {
            INDENT();
            OSS << "auto visitor = overloaded{ std::forward<Ts>(args)... };\n";
            OSS << "return visit(\n";
            {
                INDENT();
                OSS << "overloaded{\n";
                {
                    INDENT();
                    int count = 0;
                    for (const auto& c : s.mPolymorphic.mConcepts) {
                        if (count++)
                            oss << ",\n";
                        OSS << "[&](const " << handleElemType(c, ns, true) << "& /*t*/) {\n";
                        OSS << "    return visitor(" << cpp.getDependentName(c.mTag) << "{});\n";
                        OSS << "}";
                    }
                    oss << " },\n";
                }
                if (s.isVector()) {
                    OSS << "mVariants[v]);\n";
                } else {
                    if (bConst) {
                        OSS << "static_cast<const VertexType*>(v)->mVariant);\n";
                    } else {
                        OSS << "static_cast<VertexType*>(v)->mVariant);\n";
                    }
                }
            }
        };

        auto outputVisitTag2 = [&](bool bConst) {
            INDENT();
            OSS << "auto visitor = overloaded{ std::forward<Ts>(args)... };\n";

            if (bConst) {
                if (s.isVector()) {
                    OSS << "const auto& var = mVariants[v];\n";
                } else {
                    OSS << "const auto& var = static_cast<const VertexType*>(v)->mVariant);\n";
                }
            } else {
                if (s.isVector()) {
                    OSS << "auto& var = mVariants[v];\n";
                } else {
                    OSS << "auto& var = static_cast<VertexType*>(v)->mVariant);\n";
                }
            }

            OSS << "switch (var.index()) {\n";
            {
                int count = 0;
                for (const auto& c : s.mPolymorphic.mConcepts) {
                    OSS << "case " << count << ":\n";
                    OSS << "    return visitor(" << cpp.getDependentName(c.mTag) << "{});\n";
                    ++count;
                }
                // OSS << "default: std::runtime_error(\"invalid vertex polymorphic type\");\n";
                OSS << "default:\n";
                OSS << "    std::terminate();\n";
            }
            OSS << "}\n";
        };

        OSS << "template <class... Ts>\n";
        OSS << "auto " << name << "::visit_vertex_tag(Vertex v, Ts&&... args) const {\n";
        outputVisitTag2(true);
        OSS << "}\n";
        OSS << "template <class... Ts>\n";
        OSS << "auto " << name << "::visit_vertex_tag(Vertex v, Ts&&... args) {\n";
        outputVisitTag2(false);
        OSS << "}\n";

        if (s.mGarbageCollection) {
            auto name = cpp.getDependentName(cpp.mCurrentPath);
            for (const auto& c : s.mPolymorphic.mConcepts) {
                const auto& tagID = locate(c.mTag, g);
                const auto& typeID = locate(c.mValue, g);
                auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
                auto typeName = g.getDependentCppName(cpp.mCurrentNamespace, typeID);
                std::pmr::string name(tagName, scratch);
                name.pop_back();
                name.append("Handle");
                OSS << name << " " << name << "::persist_vertex(" << tagName << ", Vertex v) {\n";
                {
                    INDENT();
                    OSS << "root_vertex(v);\n";
                    OSS << "return { this, v };\n";
                }
                OSS << "}\n";

                std::pmr::string view(tagName, scratch);
                view.pop_back();
                view.append("View");
                OSS << view << " " << name << "::view_vertex(" << tagName << ", Vertex v) {\n";
                {
                    INDENT();
                    OSS << "Expects(mRootBitset[v] == true);\n";
                    OSS << "return { this, v };\n";
                }
                OSS << "}\n";
            }
        }
    }
    return oss.str();
}

namespace {

enum class TypeSpecifier {
    None,
    Const,
    Rvalue,
};

} // namespace

std::pmr::string ObjectGraphBuilder::generateVertexProperties_h() const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);
    const auto& cpp = mStruct;
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    std::string_view ns = cpp.mCurrentNamespace;

    if (!s.isPolymorphic() || s.mComponents.empty()) {
        return oss.str();
    }

    const bool bChangeLine = (s.mComponents.size() + s.isPolymorphic()) > 3;
    for (const auto type : { TypeSpecifier::None, TypeSpecifier::Const, TypeSpecifier::Rvalue }) {
        OSS << "template <class Tag>\n";
        // function name
        OSS << "auto get_vertex_tuple(Tag, Vertex v)";
        switch (type) {
        case TypeSpecifier::Const:
            oss << " const&";
            break;
        case TypeSpecifier::Rvalue:
            oss << " &&";
            break;
        case TypeSpecifier::None:
            oss << " &";
            break;
        }
        oss << " -> std::tuple<";
        {
            INDENT();
            if (bChangeLine) {
                oss << "\n";
            }
            { // return type
                uint32_t count = 0;
                auto outputSep = [&]() {
                    if (count++) {
                        if (bChangeLine) {
                            oss << ",\n";
                            OSS;
                        } else {
                            oss << ", ";
                        }
                    } else {
                        if (bChangeLine) {
                            OSS;
                        }
                    }
                };
                outputSep();
                if (type == TypeSpecifier::Rvalue) {
                    oss << "decltype(std::move(get(Tag{}, v)))";
                } else {
                    oss << "decltype(get(Tag{}, v))";
                }
                for (const auto& c : s.mComponents) {
                    outputSep();
                    auto componentID = locate(c.mValuePath, g);
                    auto typeName = g.getDependentCppName(ns, componentID);
                    if (c.mCounterName.empty()) {
                        if (type == TypeSpecifier::Const || c.mReadOnly) {
                            oss << "const ";
                            oss << typeName << "&";
                        } else if (type == TypeSpecifier::Rvalue) {
                            if (g.isTriviallyCopyable(componentID)) {
                                oss << typeName;
                            } else {
                                oss << typeName << "&&";
                            }
                        } else {
                            oss << typeName << "&";
                        }
                    } else {
                        oss << "std::span<";
                        if (type == TypeSpecifier::Const || c.mReadOnly) {
                            oss << "const ";
                        }
                        oss << typeName;
                        oss << ">";
                    }
                }
            }
            oss << ">";
        }

        oss << " {\n";
        { // function body
            INDENT();
            OSS << "return {";
            if (bChangeLine) {
                oss << "\n";
            } else {
                oss << " ";
            }
            { // members
                uint32_t count = 0;
                INDENT();
                if (bChangeLine) {
                    OSS;
                } else if (count++) {
                    oss << ", ";
                }
                if (type == TypeSpecifier::Rvalue) {
                    oss << "std::move(get(Tag{}, v))";
                } else {
                    oss << "get(Tag{}, v)";
                }
                if (bChangeLine) {
                    oss << ",\n";
                }
                for (const auto& c : s.mComponents) {
                    auto tagID = locate(c.mTagPath, g);
                    auto componentID = locate(c.mValuePath, g);
                    auto tagName = g.getDependentCppName(ns, tagID);
                    if (bChangeLine) {
                        OSS;
                    } else if (count++) {
                        oss << ", ";
                    }
                    if (type == TypeSpecifier::Rvalue) {
                        if (g.isTriviallyCopyable(componentID) || c.mReadOnly) {
                            oss << "get(" << tagName << "{}, v)";
                        } else {
                            oss << "std::move(get(" << tagName << "{}, v))";
                        }
                    } else {
                        oss << "get(" << tagName << "{}, v)";
                    }
                    if (bChangeLine) {
                        oss << ",\n";
                    }
                }
            }
            if (bChangeLine) {
                OSS << "};\n";
            } else {
                oss << " };\n";
            }
        }
        OSS << "}\n";
    }

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateGraphPropertyGraph_h() const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios::out, scratch);
    const auto& cpp = mStruct;
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    std::string_view ns = cpp.mCurrentNamespace;
    const auto& pg = s.mPropertyGraph;

    std::pmr::string space(get_allocator());
    if (pg.mProperties.empty())
        return oss.str();

    std::pmr::string api(get_allocator());
    api = mStruct.mAPI;
    if (!api.empty()) {
        api.append("_API ");
    }

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    auto getPropertyType = [&](const PolymorphicPair& c) {
        auto valueID = locate(c.mValue, g);
        if (g.isInstantiation(valueID)) {
            valueID = g.getFirstTemplateParameter(valueID);
        }
        return g.getDependentName(cpp.mCurrentNamespace, valueID) + "T";
    };

    if (!mImpl) {
        oss << "\n";
        OSS << "// PropertyGraph\n";
        for (auto propertyId = 0U; propertyId != pg.mProperties.size(); ++propertyId) {
            const auto& properties = pg.mProperties[propertyId];
            const auto& propertyIndex = pg.mPropertyMaps[propertyId];
            const auto& c0 = properties.mConcepts.at(0);
            const auto& tagID = locate(c0.mTag, g);
            auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
            std::pmr::string name(tagName, scratch);
            name.pop_back();

            if (properties.mConcepts.size() == 1) {
                OSS << "template <class... Args>\n";
                if (!propertyIndex.mMapType.empty()) {
                    OSS << "std::pair<Index, bool> add_property(" << tagName;
                } else {
                    OSS << "Index add_property(" << tagName;
                }
                if (!propertyIndex.mMapType.empty()) {
                    const auto keyId = locate(propertyIndex.mKeyType, g);
                    auto keyName = g.getDependentCppName(cpp.mCurrentNamespace, keyId);
                    oss << ", const " << keyName << "& key";
                }
                const auto& c = properties.mConcepts.front();
                if (c.mMemberName.empty()) {
                    oss << ");\n";
                } else {
                    oss << ", Args&&... value);\n";
                }
            } else {
                OSS << "template <";
                for (uint32_t count = 0; const auto& c : properties.mConcepts) {
                    if (count) {
                        oss << ", ";
                    }
                    oss << "class " << getPropertyType(c) << count;
                    ++count;
                }
                oss << ">\n";
                if (!propertyIndex.mMapType.empty()) {
                    OSS << "std::pair<Index, bool> add_property(" << tagName;
                } else {
                    OSS << "Index add_property(" << tagName;
                }
                if (!propertyIndex.mMapType.empty()) {
                    const auto keyId = locate(propertyIndex.mKeyType, g);
                    auto keyName = g.getDependentCppName(cpp.mCurrentNamespace, keyId);
                    oss << ", const " << keyName << "& key";
                }
                for (uint32_t count = 0; const auto& c : properties.mConcepts) {
                    auto varName = camelToVariable(getPropertyType(c), scratch);
                    oss << ", " << getPropertyType(c) << count << "&& " << varName << count;
                    ++count;
                }
                oss << ");\n";
            }
        }
        for (const auto& properties : pg.mProperties) {
            if (properties.mConcepts.size() == 1) {
                const auto& c = properties.mConcepts[0];
                if (c.mMemberName.empty()) {
                    continue; // no member name, no getter
                }
                const auto& tagID = locate(c.mTag, g);
                const auto& typeID = locate(c.mValue, g);
                auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
                auto typeName = g.getDependentCppName(cpp.mCurrentNamespace, typeID);
                std::pmr::string name(tagName, scratch);
                name.pop_back();

                OSS << typeName << "& get_property(" << tagName << ", uint32_t id);\n";
                OSS << typeName << " const& get_property(" << tagName << ", uint32_t id) const;\n";
            } else {
                // TODO(hyde): add tuple
            }
        }
        for (auto propertyId = 0U; propertyId != pg.mProperties.size(); ++propertyId) {
            const auto& properties = pg.mProperties[propertyId];
            const auto& propertyIndex = pg.mPropertyMaps[propertyId];
            const auto& c0 = properties.mConcepts.at(0);
            const auto& tagID = locate(c0.mTag, g);
            auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
            if (propertyIndex.mMapType.empty()) {
                OSS << api << "void remove_property(" << tagName << ", uint32_t id, uint32_t count = 1) noexcept;\n";
            } else {
                const auto keyId = locate(propertyIndex.mKeyType, g);
                auto keyName = g.getDependentCppName(cpp.mCurrentNamespace, keyId);
                OSS << api << "bool remove_property(" << tagName << ", const " << keyName << "& key) noexcept;\n";
            }
        }
        if (s.mDirtyMask) {
            for (const auto& properties : pg.mProperties) {
                const auto& c0 = properties.mConcepts.at(0);
                const auto& tagID = locate(c0.mTag, g);
                auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
                OSS << api << "bool property_invalidated(" << tagName << ", uint32_t id) const noexcept;\n";
            }
            for (const auto& properties : pg.mProperties) {
                const auto& c0 = properties.mConcepts.at(0);
                const auto& tagID = locate(c0.mTag, g);
                auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
                OSS << api << "bool property_dependency_invalidated(" << tagName << ", uint32_t id) const noexcept;\n";
            }
            for (const auto& properties : pg.mProperties) {
                const auto& c0 = properties.mConcepts.at(0);
                const auto& tagID = locate(c0.mTag, g);
                auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
                OSS << api << "void invalidate_properties(" << tagName << ") noexcept;\n";
            }
            OSS << api << "void invalidate_graph_properties() noexcept; // Implemented by user\n";
            OSS << api << "void reset_properties() noexcept;\n";
        }
        OSS << api << "void validate_properties() const;\n";
        OSS << api << "bool property_empty() const noexcept;\n";
        OSS << api << "void validate_property_empty() const;\n";
        if (s.mDirtyMask) {
            OSS << api << "PropertyCounter count_invalidated_property() const;\n";
        }
    } else {
        for (auto propertyId = 0U; propertyId != pg.mProperties.size(); ++propertyId) {
            const auto& properties = pg.mProperties[propertyId];
            const auto& propertyIndex = pg.mPropertyMaps[propertyId];
            const auto& c0 = properties.mConcepts.at(0);
            const auto& tagID = locate(c0.mTag, g);
            auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
            auto outputPropertyIndexFind = [&]() {
                if (propertyIndex.mMapType.empty()) {
                    return;
                }
                OSS << "auto iter = " << propertyIndex.mMemberName << ".find(key);\n";
                OSS << "if (iter != " << propertyIndex.mMemberName << ".end()) {\n";
                {
                    INDENT();
                    if (propertyIndex.mRefCounted) {
                        OSS << "++iter->second.mRefCount;\n";
                        OSS << "return { iter->second.mIndex, false };\n";
                    } else {
                        OSS << "return { iter->second, false };\n";
                    }
                }
                OSS << "}\n";
            };
            auto outputPropertyIndexInsert = [&](std::string_view propMemberName) {
                if (propertyIndex.mMapType.empty()) {
                    return;
                }
                OSS << "bool inserted;\n";
                if (propertyIndex.mRefCounted) {
                    OSS << "std::tie(iter, inserted) = " << propertyIndex.mMemberName
                        << ".emplace(key, IndexReference{ Index{ id }, 1 });\n";
                    OSS << "Ensures(inserted);\n";
                } else {
                    OSS << "std::tie(iter, inserted) = " << propertyIndex.mMemberName
                        << ".emplace(key, Index{ id });\n";
                    OSS << "Ensures(inserted);\n";
                }
                OSS << "Ensures(" << propertyIndex.mMemberName << ".size() == "
                    << propMemberName << ".size());\n";
            };
            if (properties.mConcepts.size() == 1) {
                const auto& c = properties.mConcepts.front();
                OSS << "template <class... Args>\n";
                if (!propertyIndex.mMapType.empty()) {
                    OSS << "std::pair<Index, bool> ";
                } else {
                    OSS << "Index ";
                }
                oss << name << "::add_property(" << tagName;
                if (!propertyIndex.mMapType.empty()) {
                    const auto keyId = locate(propertyIndex.mKeyType, g);
                    auto keyName = g.getDependentCppName(cpp.mCurrentNamespace, keyId);
                    oss << ", const " << keyName << "& key";
                }
                if (c.mMemberName.empty()) {
                    oss << ") {\n";
                } else {
                    oss << ", Args&&... value) {\n";
                }
                {
                    INDENT();
                    outputPropertyIndexFind();
                    if (c.mMemberName.empty()) {
                        if (s.mDirtyMask) {
                            OSS << "const auto id = Index{ static_cast<uint32_t>(" << c.getMaskName() << ".size()) };\n";
                        }
                    } else {
                        OSS << "const auto id = Index{ static_cast<uint32_t>(" << c.mMemberName << ".size()) };\n";
                    }
                    if (s.mDirtyMask) {
                        OSS << c.getMaskName() << ".push_back(true);\n";
                    }
                    if (!c.mMemberName.empty()) {
                        OSS << c.mMemberName << ".emplace_back(std::forward<Args>(value)...);\n";
                    }
                    if (s.mDirtyMask && !c.mMemberName.empty()) {
                        OSS << "Ensures(" << c.getMaskName() << ".size() == " << c.mMemberName << ".size());\n";
                    }
                    outputPropertyIndexInsert(c.mMemberName);
                    if (propertyIndex.mMapType.empty()) {
                        OSS << "return id;\n";
                    } else {
                        OSS << "return { id, true };\n";
                    }
                }
                OSS << "}\n";
            } else {
                OSS << "template <";
                for (uint32_t count = 0; const auto& c : properties.mConcepts) {
                    if (count) {
                        oss << ", ";
                    }
                    oss << "class " << getPropertyType(c) << count;
                    ++count;
                }
                oss << ">\n";
                if (!propertyIndex.mMapType.empty()) {
                    OSS << "std::pair<Index, bool> ";
                } else {
                    OSS << "Index ";
                }
                oss << name << "::add_property(" << tagName;
                if (!propertyIndex.mMapType.empty()) {
                    const auto keyId = locate(propertyIndex.mKeyType, g);
                    auto keyName = g.getDependentCppName(cpp.mCurrentNamespace, keyId);
                    oss << ", const " << keyName << "& key";
                }
                for (uint32_t count = 0; const auto& c : properties.mConcepts) {
                    const auto varName = camelToVariable(getPropertyType(c), scratch);
                    oss << ", " << getPropertyType(c) << count << "&& " << varName << count;
                    ++count;
                }
                oss << ") {\n";
                {
                    INDENT();
                    outputPropertyIndexFind();
                    const auto& c0 = properties.mConcepts.at(0);
                    if (s.mDirtyMask) {
                        OSS << c0.getMaskName() << ".push_back(true);\n";
                    }
                    for (uint32_t count = 0; const auto& c : properties.mConcepts) {
                        if (count == 0) {
                            OSS << "const auto id = Index{ static_cast<uint32_t>(" << c.mMemberName << ".size()) };\n";
                        }
                        const auto varName = camelToVariable(getPropertyType(c), scratch);
                        OSS << c.mMemberName << ".emplace_back(std::forward<"
                            << getPropertyType(c) << count << ">(" << varName << count << "));\n";
                        if (count) {
                            OSS << "Ensures(" << c.mMemberName << ".size() == " << c0.mMemberName << ".size());\n";
                        }
                        ++count;
                    }
                    if (s.mDirtyMask) {
                        OSS << "Ensures(" << c0.getMaskName() << ".size() == " << c0.mMemberName << ".size());\n";
                    }
                    outputPropertyIndexInsert(c0.mMemberName);
                    if (propertyIndex.mMapType.empty()) {
                        OSS << "return id;\n";
                    } else {
                        OSS << "return { id, true };\n";
                    }
                }
                OSS << "}\n";
            }
        }
        for (const auto& properties : pg.mProperties) {
            if (properties.mConcepts.size() == 1) {
                const auto& c = properties.mConcepts.front();
                if (c.mMemberName.empty()) {
                    continue; // no member, no getter
                }
                const auto& tagID = locate(c.mTag, g);
                const auto& typeID = locate(c.mValue, g);
                auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
                auto typeName = g.getDependentCppName(cpp.mCurrentNamespace, typeID);
                OSS << "inline " << typeName << "& " << name << "::get_property(" << tagName << ", uint32_t id) {\n";
                {
                    INDENT();
                    OSS << "Expects(id < " << c.mMemberName << ".size());\n";
                    OSS << "return " << c.mMemberName << "[id];\n";
                }
                OSS << "}\n";
                OSS << "inline " << typeName << " const& " << name << "::get_property(" << tagName << ", uint32_t id) const {\n";
                {
                    INDENT();
                    OSS << "Expects(id < " << c.mMemberName << ".size());\n";
                    OSS << "return " << c.mMemberName << "[id];\n";
                }
                OSS << "}\n";
            } else {
                // TODO(hyde): add tuple
            }
        }
    }
    return oss.str();
}

namespace {

void outputResetChildrenPropertyIndex(
    std::ostream& oss, std::pmr::string& space,
    std::string_view ns,
    const SyntaxGraph& g,
    SyntaxGraph::vertex_descriptor typeID,
    std::string_view var,
    SyntaxGraph::vertex_descriptor tagID,
    uint32_t depth,
    bool bDirtyMask,
    bool bCount,
    bool bNoCheck,
    std::pmr::memory_resource* scratch) {
    visit_vertex(
        typeID, g,
        [&](const Composition_ auto& s) {
            const auto& traits = get(g.traits, g, typeID);
            if (traits.mFlags & PROPERTY_GRAPH_INDEX) {
                std::string_view functionName = bNoCheck ? "resetIndexWithoutRangeCheck" : "resetIndex";
                if (bDirtyMask) {
                    OSS << "dirty |= Impl::" << functionName << "(" << var << ", id);\n";
                } else {
                    if (bCount) {
                        OSS << "Impl::" << functionName << "(" << var << ", id, count);\n";
                    } else {
                        OSS << "Impl::" << functionName << "(" << var << ", id);\n";
                    }
                }
                return;
            }
            for (const auto& m : s.mMembers) {
                const auto memberID = locate(m.mTypePath, g);
                std::pmr::string memberName(var, scratch);
                memberName.append(".");
                memberName.append(m.mMemberName);
                if (m.mRange) {
                    memberName.append(".mIndex");
                }
                if (m.mWeakRef) {
                    bNoCheck |= true;
                }
                std::string_view functionName = bNoCheck ? "resetIndexWithoutRangeCheck" : "resetIndex";
                // member has index
                if (g.hasGraphPropertyTag(memberID, tagID, false)) {
                    outputResetChildrenPropertyIndex(oss, space, ns, g,
                        memberID, memberName, tagID, depth, bDirtyMask, bCount, bNoCheck, scratch);
                }
                // member is index
                if (locate(m.mPropertyGraphTag, g) != tagID) {
                    continue;
                }
                if (holds_tag<Instance_>(memberID, g)) {
                    outputResetChildrenPropertyIndex(oss, space, ns, g,
                        memberID, memberName, tagID, depth, bDirtyMask, bCount, bNoCheck, scratch);
                } else {
                    if (m.mOptional) {
                        OSS << "if (!!" << memberName << ") {\n";
                        {
                            INDENT();
                            if (bDirtyMask) {
                                OSS << "dirty |= Impl::" << functionName << "(" << memberName << ", id);\n";
                            } else {
                                if (bCount) {
                                    OSS << "Impl::" << functionName << "(" << memberName << ", id, count);\n";
                                } else {
                                    OSS << "Impl::" << functionName << "(" << memberName << ", id);\n";
                                }
                            }
                        }
                        OSS << "}\n";
                    } else {
                        if (bDirtyMask) {
                            OSS << "dirty |= Impl::" << functionName << "(" << memberName << ", id);\n";
                        } else {
                            if (bCount) {
                                OSS << "Impl::" << functionName << "(" << memberName << ", id, count);\n";
                            } else {
                                OSS << "Impl::" << functionName << "(" << memberName << ", id);\n";
                            }
                        }
                    }
                }
            }
        },
        [&](const Instance& inst) {
            bool bPointer = (inst.mTemplate == "/std/unique_ptr"
                || inst.mTemplate == "/std/shared_ptr"
                || inst.mTemplate == "/Star/Intrusive");
            if (bPointer) {
                OSS << "if (" << var << ") {\n";
                INDENT();
                OSS << "auto& prop" << depth << " = *" << var << ";\n";
            } else {
                OSS << "for (auto& prop" << depth << " : " << var << ") {\n";
            }
            {
                INDENT();
                std::pmr::string memberName("prop", scratch);
                memberName.append(std::to_string(depth));
                if (g.isMap(typeID)) {
                    const auto valueID = locate(inst.mParameters.at(1).mTypePath, g);
                    outputResetChildrenPropertyIndex(oss, space, ns, g,
                        valueID, "prop" + std::to_string(depth) + ".second",
                        tagID, depth + 1, bDirtyMask, bCount, bNoCheck, scratch);
                } else {
                    const auto valueID = g.getFirstTemplateParameter(typeID);
                    outputResetChildrenPropertyIndex(oss, space, ns, g,
                        valueID, "prop" + std::to_string(depth),
                        tagID, depth + 1, bDirtyMask, bCount, bNoCheck, scratch);
                }
            }
            OSS << "}\n";
        },
        [&](const auto&) {

        });
}

void outputInvalidatedChildrenPropertyIndex(
    const CppStructBuilder& cpp,
    std::ostream& oss, std::pmr::string& space,
    std::string_view ns,
    const SyntaxGraph& g,
    SyntaxGraph::vertex_descriptor typeID,
    std::string_view var,
    SyntaxGraph::vertex_descriptor tagID,
    uint32_t depth,
    bool bOptional,
    bool bRange,
    std::pmr::memory_resource* scratch) {

    visit_vertex(
        typeID, g,
        [&](const Composition_ auto& s) {
            const auto& traits = get(g.traits, g, typeID);
            if (tagID != SyntaxGraph::null_vertex() && traits.mFlags & PROPERTY_GRAPH_INDEX) {
                auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
                auto outputRange = [&]() {
                    OSS << "for (const auto& i" << depth << " : " << var << ") {\n";
                    {
                        INDENT();
                        OSS << "if (property_invalidated(" << tagName << "{}, i" << depth << ")) {\n";
                        OSS << "    return true;\n";
                        OSS << "}\n";
                    }
                    OSS << "}\n";
                };
                if (bOptional) {
                    OSS << "if (!!" << var << ") {\n";
                    {
                        INDENT();
                        if (bRange) {
                            outputRange();
                        } else {
                            OSS << "if (property_invalidated(" << tagName << "{}, " << var << ")) {\n";
                            OSS << "    return true;\n";
                            OSS << "}\n";
                        }
                    }
                    OSS << "}\n";
                } else {
                    if (bRange) {
                        outputRange();
                    } else {
                        OSS << "if (property_invalidated(" << tagName << "{}, " << var << ")) {\n";
                        OSS << "    return true;\n";
                        OSS << "}\n";
                    }
                }
            }
            for (const auto& m : s.mMembers) {
                const auto memberID = locate(m.mTypePath, g);

                std::pmr::string memberName(var, scratch);
                memberName.append(".");
                memberName.append(m.mMemberName);

                if (m.mPropertyGraphTag.empty()) {
                    if (g.hasGraphPropertyTag(memberID, g.null_vertex(), false)) {
                        outputInvalidatedChildrenPropertyIndex(cpp, oss, space, ns, g,
                            memberID, memberName, tagID, depth, m.mOptional, m.mRange, scratch);
                    }
                    continue;
                }

                const auto tagID = locate(m.mPropertyGraphTag, g);
                outputInvalidatedChildrenPropertyIndex(cpp, oss, space, ns, g,
                    memberID, memberName, tagID, depth, m.mOptional, m.mRange, scratch);
            }
        },
        [&](const Instance& inst) {
            OSS << "// NOLINTNEXTLINE(readability-use-anyofallof)\n";
            if (inst.mTemplate == "/std/unique_ptr"
                || inst.mTemplate == "/std/shared_ptr"
                || inst.mTemplate == "/Star/Intrusive") {
                OSS << "if (" << var << ") {\n";
                INDENT();
                OSS << "const auto& prop" << depth << " = *" << var << ";\n";
            } else {
                OSS << "for (const auto& prop" << depth << " : " << var << ") {\n";
            }
            {
                INDENT();
                std::pmr::string memberName("prop", scratch);
                memberName.append(std::to_string(depth));
                if (g.isMap(typeID)) {
                    const auto valueID = locate(inst.mParameters.at(1).mTypePath, g);
                    outputInvalidatedChildrenPropertyIndex(cpp, oss, space, ns, g,
                        valueID, "prop" + std::to_string(depth) + ".second",
                        tagID, depth + 1, false, false, scratch);
                } else {
                    const auto valueID = g.getFirstTemplateParameter(typeID);
                    outputInvalidatedChildrenPropertyIndex(cpp, oss, space, ns, g,
                        valueID, "prop" + std::to_string(depth),
                        tagID, depth + 1, false, false, scratch);
                }
            }
            OSS << "}\n";
        },
        [&](const auto&) {

        });
}

std::pmr::string getPropertyMemberName(const Graph& g0,
    std::string_view tagPath,
    std::pmr::memory_resource* scratch) {
    for (const auto& c : g0.mComponents) {
        if (c.mTagPath == tagPath) {
            return { c.mMemberName, scratch };
        }
    }
    for (const auto& properties : g0.mPropertyGraph.mProperties) {
        const auto& c0 = properties.mConcepts.at(0);
        if (c0.mTag == tagPath) {
            const auto& c = properties.mConcepts.at(0);
            return { c.mMemberName, scratch };
        }
    }
    return std::pmr::string{ scratch };
}

void outputValidateChildrenPropertyIndex(
    const Graph& g0,
    std::ostream& oss, std::pmr::string& space,
    std::string_view ns,
    const SyntaxGraph& g,
    SyntaxGraph::vertex_descriptor typeID,
    std::string_view var,
    uint32_t depth,
    uint32_t parentTagID,
    bool bOptional,
    std::pmr::memory_resource* scratch) {
    visit_vertex(
        typeID, g,
        [&](const Composition_ auto& s) {
            const auto& traits = get(g.traits, g, typeID);
            if (traits.mFlags & PROPERTY_GRAPH_INDEX) {
                // will only reach here, when index is in container
                Expects(parentTagID != g.null_vertex());
                auto name = g.getDependentCppName(ns, typeID);
                auto tagPath = get_path(parentTagID, g, scratch);
                auto memberName0 = getPropertyMemberName(g0, tagPath, scratch);
                if (!memberName0.empty()) {
                    OSS << "Ensures("
                        << var << " < "
                        << memberName0 << ".size());\n";
                } else {
                    OSS << "std::ignore = " << var << ";\n";
                }
            }
            for (const auto& m : s.mMembers) {
                const auto memberID = locate(m.mTypePath, g);
                std::pmr::string memberName(var, scratch);
                memberName.append(".");
                memberName.append(m.mMemberName);

                SyntaxGraph::vertex_descriptor tagID = g.null_vertex();
                if (!m.mPropertyGraphTag.empty()) {
                    tagID = locate(m.mPropertyGraphTag, g);
                }

                // member has index
                if (g.hasGraphPropertyTag(memberID, g.null_vertex(), false)) {
                    outputValidateChildrenPropertyIndex(g0, oss, space, ns, g,
                        memberID, memberName, depth, tagID,
                        bOptional || m.mOptional, scratch);
                }
                if (m.mPropertyGraphTag.empty()) {
                    continue;
                }
                // member is index
                if (holds_tag<Instance_>(memberID, g)) {
                    outputValidateChildrenPropertyIndex(g0, oss, space, ns, g,
                        memberID, memberName, depth, tagID,
                        bOptional || m.mOptional, scratch);
                } else {
                    auto memberType = g.getDependentCppName(ns, memberID);
                    auto memberName0 = getPropertyMemberName(g0, m.mPropertyGraphTag, scratch);
                    if (!memberName0.empty()) {
                        OSS << "Ensures(";
                        if (m.mOptional || bOptional) {
                            oss << memberName << " == " << memberType << "{} || ";
                        }
                        if (m.mRange) {
                            oss << memberName << ".end_point() <= " << memberName0 << ".size());\n";
                        } else {
                            oss << memberName << " < " << memberName0 << ".size());\n";
                        }
                    } else {
                        OSS << "std::ignore = " << memberName << ";\n";
                    }
                }
            }
        },
        [&](const Instance& inst) {
            if (inst.mTemplate == "/std/unique_ptr"
                || inst.mTemplate == "/std/shared_ptr"
                || inst.mTemplate == "/Star/Intrusive") {
                OSS << "if (" << var << ") {\n";
                INDENT();
                OSS << "const auto& prop" << depth << " = *" << var << ";\n";
            } else {
                OSS << "for (const auto& prop" << depth << " : " << var << ") {\n";
            }
            {
                INDENT();
                std::pmr::string memberName("prop", scratch);
                memberName.append(std::to_string(depth));
                if (g.isMap(typeID)) {
                    const auto valueID = locate(inst.mParameters.at(1).mTypePath, g);
                    outputValidateChildrenPropertyIndex(g0, oss, space, ns, g,
                        valueID, "prop" + std::to_string(depth) + ".second",
                        depth + 1, parentTagID,
                        bOptional, scratch);
                } else {
                    const auto valueID = g.getFirstTemplateParameter(typeID);
                    outputValidateChildrenPropertyIndex(g0, oss, space, ns, g,
                        valueID, "prop" + std::to_string(depth),
                        depth + 1, parentTagID,
                        bOptional, scratch);
                }
            }
            OSS << "}\n";
        },
        [&](const auto&) {

        });
}
}

std::pmr::string ObjectGraphBuilder::generateGraphPropertyGraph_cpp() const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios::out, scratch);
    const auto& cpp = mStruct;
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    std::string_view ns = cpp.mCurrentNamespace;
    const auto& pg = s.mPropertyGraph;
    auto graphName = cpp.getDependentName(cpp.mCurrentPath);

    std::pmr::string space(get_allocator());
    if (pg.mProperties.empty())
        return oss.str();

    std::pmr::string api(get_allocator());
    api = mStruct.mAPI;
    if (!api.empty()) {
        api.append("_API ");
    }

    oss << "\n";
    OSS << "// PropertyGraph\n";
    int count = 0;
    for (auto propertyId = 0U; propertyId != pg.mProperties.size(); ++propertyId) {
        const auto& properties = pg.mProperties[propertyId];
        const auto& propertyIndex = pg.mPropertyMaps[propertyId];
        const auto& c0 = properties.mConcepts.at(0);
        const auto& tagID = locate(c0.mTag, g);
        auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
        if (count++) {
            oss << "\n";
        }
        bool bHasIndex = !propertyIndex.mMapType.empty();
        OSS << "// NOLINTNEXTLINE(readability-function-cognitive-complexity)\n";
        if (propertyIndex.mMapType.empty()) {
            OSS << "void " << graphName << "::remove_property("
                << tagName << ", uint32_t id, uint32_t count) noexcept {\n";
        } else {
            const auto keyId = locate(propertyIndex.mKeyType, g);
            auto keyName = g.getDependentCppName(cpp.mCurrentNamespace, keyId);
            OSS << "bool " << graphName
                << "::remove_property(" << tagName << ", const " << keyName << "& key) noexcept {\n";
        }
        {
            INDENT();
            if (!propertyIndex.mMapType.empty()) {
                OSS << "auto iter = " << propertyIndex.mMemberName << ".find(key);\n";
                OSS << "if (iter == " << propertyIndex.mMemberName << ".end()) {\n";
                {
                    INDENT();
                    OSS << "Expects(false);\n";
                    OSS << "return false;\n";
                }
                OSS << "}\n";
                if (propertyIndex.mRefCounted) {
                    OSS << "auto& ref = iter->second;\n";
                    OSS << "--ref.mRefCount;\n";
                    OSS << "if (ref.mRefCount > 0) {\n";
                    OSS << "    return false;\n";
                    OSS << "}\n";
                    OSS << "const auto id = ref.mIndex;\n";
                } else {
                    OSS << "const auto id = iter->second;\n";
                }
            }

            auto reindex = [&](SyntaxGraph::vertex_descriptor typeID,
                               std::string_view memberName, std::string_view maskName,
                               bool bNoCheck,
                               std::string_view comment,
                               uint32_t& count) {
                if (!g.hasGraphPropertyTag(typeID, tagID, false)) {
                    return;
                }
                if (count++ == 0) {
                    copyString(oss, space, comment);
                }
                const bool bDirtyMask = false; // s.mDirtyMask && !maskName.empty();
                if (bDirtyMask) {
                    OSS << "for (uint32_t i = 0; auto& prop : " << memberName << ") {\n";
                    OSS << "    bool dirty = false;\n";
                } else {
                    OSS << "for (auto& prop : " << memberName << ") {\n";
                }
                {
                    INDENT();
                    outputResetChildrenPropertyIndex(oss, space, ns, g,
                        typeID, "prop", tagID, 0, bDirtyMask, !bHasIndex, bNoCheck, scratch);
                    if (bDirtyMask) {
                        OSS << "if (dirty) {\n";
                        {
                            INDENT();
                            OSS << maskName << ".set(i, true);\n";
                        }
                        OSS << "}\n";
                        OSS << "++i;\n";
                    }
                }
                OSS << "}\n";
            };
            for (uint32_t count = 0; const auto& src : s.mComponents) {
                const auto& typeID = locate(src.mValuePath, g);
                const bool bNoCheck = (src.mTagPath == c0.mTag);
                reindex(typeID, src.mMemberName, "", bNoCheck,
                    "// Reindex component references\n", count);
            }
            for (auto propertyId = 0U, count = 0U; propertyId != pg.mProperties.size(); ++propertyId) {
                const auto& properties = pg.mProperties[propertyId];
                const auto& propertyIndex = pg.mPropertyMaps[propertyId];
                for (const auto& c : properties.mConcepts) {
                    const auto& typeID = locate(c.mValue, g);
                    const bool bNoCheck = (c.mTag == c0.mTag);
                    reindex(typeID, c.mMemberName, c.getMaskName(), bNoCheck,
                        "// Reindex referencing properties\n", count);
                }
            }
            if (properties.mConcepts.size() > 1) {
                OSS << "// Remove properties\n";
            } else {
                OSS << "// Remove property\n";
            }

            bool hasProperty = false;
            for (uint32_t count1 = 0; const auto& c : properties.mConcepts) {
                if (c.mMemberName.empty()) {
                    OSS << "// No member, skipping remove.\n";
                    continue; // no member, no remove
                }
                hasProperty = true;
                if (bHasIndex) {
                    OSS << c.mMemberName << ".erase(" << c.mMemberName << ".begin() + id);\n";
                } else {
                    OSS << c.mMemberName << ".erase("
                        << c.mMemberName << ".begin() + id, "
                        << c.mMemberName << ".begin() + id + count);\n";
                }
                if (count1++) {
                    OSS << "Ensures(" << c.mMemberName << ".size() == " << c0.mMemberName << ".size());\n";
                }
            }
            if (s.mDirtyMask) {
                const auto& c = properties.mConcepts.at(0);
                OSS << "// Remove dirty mask\n";
                if (bHasIndex) {
                    OSS << "Impl::eraseFromBitset(" << c.getMaskName() << ", id, 1U);\n";
                } else {
                    OSS << "Impl::eraseFromBitset(" << c.getMaskName() << ", id, count);\n";
                }
                if (hasProperty) {
                    OSS << "Ensures(" << c.getMaskName() << ".size() == " << c.mMemberName << ".size());\n";
                }
            }
            if (!propertyIndex.mMapType.empty()) {
                OSS << "// Remove property index\n";
                OSS << propertyIndex.mMemberName << ".erase(iter);\n";
                if (propertyIndex.mRefCounted) {
                    OSS << "for (auto&& [key1, ref1] : " << propertyIndex.mMemberName << ") {\n";
                    {
                        INDENT();
                        OSS << "if (ref1.mIndex > id) {\n";
                        OSS << "    --ref1.mIndex;\n";
                        OSS << "}\n";
                    }
                    OSS << "}\n";
                } else {
                    OSS << "for (auto&& [key1, id1] : " << propertyIndex.mMemberName << ") {\n";
                    {
                        INDENT();
                        OSS << "if (id1 > id) {\n";
                        OSS << "    --id1;\n";
                        OSS << "}\n";
                    }
                    OSS << "}\n";
                }
                OSS << "Ensures(" << propertyIndex.mMemberName << ".size() == " << c0.mMemberName << ".size());\n";
                OSS << "return true;\n";
            }
        }
        OSS << "}\n";
    }

    if (s.mDirtyMask) {
        for (const auto& properties : pg.mProperties) {
            const auto& c0 = properties.mConcepts.at(0);
            const auto& tagID = locate(c0.mTag, g);
            auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
            std::pmr::string name(tagName, scratch);
            name.pop_back();
            if (count++) {
                oss << "\n";
            }
            OSS << "bool " << graphName << "::property_invalidated("
                << tagName << ", uint32_t id) const noexcept {\n";
            {
                INDENT();
                const auto& c = properties.mConcepts.at(0);
                OSS << "Expects(id < " << c.getMaskName() << ".size());\n";
                OSS << "return " << c.getMaskName() << "[id];\n";
            }
            OSS << "}\n";
        }

        for (const auto& properties : pg.mProperties) {
            const auto& c0 = properties.mConcepts.at(0);
            const auto& tagID = locate(c0.mTag, g);
            auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
            std::pmr::string name(tagName, scratch);
            name.pop_back();
            const auto& c = properties.mConcepts.at(0);
            if (count++) {
                oss << "\n";
            }
            OSS << "// NOLINTNEXTLINE(readability-function-cognitive-complexity)\n";
            OSS << "bool " << graphName << "::property_dependency_invalidated("
                << tagName << ", uint32_t id) const noexcept {\n";
            {
                INDENT();
                OSS << "Expects(id < " << c.getMaskName() << ".size());\n";
                for (const auto& c : properties.mConcepts) {
                    const auto& typeID = locate(c.mValue, g);
                    auto typeName = g.getDependentCppName(cpp.mCurrentNamespace, typeID);
                    if (g.hasGraphPropertyTag(typeID, g.null_vertex(), false)) {
                        if (properties.mConcepts.size() > 1) {
                            OSS << "{\n";
                            space += "    ";
                        }
                        {
                            OSS << "Expects(id < " << c.mMemberName << ".size());\n";
                            OSS << "const auto& prop = " << c.mMemberName << "[id];\n";
                            outputInvalidatedChildrenPropertyIndex(
                                cpp, oss, space, ns, g,
                                typeID, "prop", tagID, 0, false, false,
                                scratch);
                        }
                        if (properties.mConcepts.size() > 1) {
                            space.resize(space.size() - 4);
                            OSS << "}\n";
                        }
                    }
                }
                OSS << "return false;\n";
            }
            OSS << "}\n";
        }

        for (const auto& properties : pg.mProperties) {
            const auto& c0 = properties.mConcepts.at(0);
            const auto& tagID = locate(c0.mTag, g);
            auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
            std::pmr::string name(tagName, scratch);
            name.pop_back();
            const auto& c = properties.mConcepts.at(0);
            const auto& maskName = c.getMaskName();
            OSS << "void " << graphName << "::invalidate_properties(" << tagName << ") noexcept {\n";
            {
                INDENT();
                OSS << "for (uint32_t i = 0; i != " << maskName << ".size(); ++i) {\n";
                {
                    INDENT();
                    OSS << "if (" << maskName << ".test(i)) {\n";
                    OSS << "    continue;\n";
                    OSS << "}\n";
                    OSS << "const bool invaliated = property_dependency_invalidated(" << tagName << "{}, i);\n";
                    OSS << c.getMaskName() << ".set(i, invaliated);\n";
                }
                OSS << "}\n";
            }
            OSS << "}\n";
        }

        oss << "\n";
        OSS << "void " << graphName << "::reset_properties() noexcept {\n";
        {
            INDENT();
            for (const auto& properties : pg.mProperties) {
                if (properties.mSkipReset) {
                    continue;
                }
                const auto& c0 = properties.mConcepts.at(0);
                const auto& tagID = locate(c0.mTag, g);
                auto tagName = g.getDependentCppName(cpp.mCurrentNamespace, tagID);
                std::pmr::string name(tagName, scratch);
                name.pop_back();
                const auto& c = properties.mConcepts.at(0);
                const auto& maskName = c.getMaskName();
                OSS << maskName << ".reset();\n";
            }
        }
        OSS << "}\n";
    }

    oss << "\n";
    OSS << "// NOLINTNEXTLINE(readability-function-cognitive-complexity)\n";
    OSS << "void " << graphName << "::validate_properties() const {\n";
    {
        INDENT();
        auto validate = [&](SyntaxGraph::vertex_descriptor typeID,
                            std::string_view memberName,
                            std::string_view tagPath) {
            auto tagID = locate(tagPath, g);
            if (!g.hasGraphPropertyTag(typeID, g.null_vertex(), false)) {
                return;
            }
            OSS << "for (const auto& prop : " << memberName << ") {\n";
            {
                INDENT();
                outputValidateChildrenPropertyIndex(s, oss, space, ns, g,
                    typeID, "prop", 0, tagID, false, scratch);
            }
            OSS << "}\n";
        };
        for (const auto& src : s.mComponents) {
            const auto& typeID = locate(src.mValuePath, g);
            validate(typeID, src.mMemberName, src.mTagPath);
        }
        for (const auto& properties : pg.mProperties) {
            for (const auto& c : properties.mConcepts) {
                const auto& typeID = locate(c.mValue, g);
                validate(typeID, c.mMemberName, c.mTag);
            }
        }
    }
    OSS << "}\n";

    oss << "\n";
    OSS << "bool " << graphName << "::property_empty() const noexcept {\n";
    {
        INDENT();
        OSS << "return ";
        int count = 0;
        for (const auto& properties : pg.mProperties) {
            const auto& c0 = properties.mConcepts.at(0);
            if (c0.mMemberName.empty()) {
                continue; // no member, no check
            }
            if (count++) {
                oss << "\n";
                OSS << "    && ";
            }
            oss << c0.mMemberName << ".empty()";
        }
        oss << ";\n";
    }
    OSS << "}\n";

    oss << "\n";
    OSS << "// NOLINTNEXTLINE(readability-function-cognitive-complexity)\n";
    OSS << "void " << graphName << "::validate_property_empty() const {\n";
    {
        INDENT();
        for (const auto& properties : pg.mProperties) {
            if (s.mDirtyMask) {
                OSS << "Expects(" << properties.mConcepts.at(0).getMaskName() << ".empty());\n";
            }
            for (const auto& c : properties.mConcepts) {
                if (c.mMemberName.empty()) {
                    continue; // no member, no check
                }
                OSS << "Expects(" << c.mMemberName << ".empty());\n";
            }
        }
        OSS << "// Validate property index\n";
        for (const auto& map : pg.mPropertyMaps) {
            if (map.mMemberName.empty()) {
                continue;
            }
            OSS << "Expects(" << map.mMemberName << ".empty());\n";
        }
    }
    OSS << "}\n";

    if (s.mDirtyMask) {
        oss << "\n";
        OSS << "// NOLINTNEXTLINE(readability-function-cognitive-complexity)\n";
        OSS << graphName << "::PropertyCounter "
            << graphName << "::count_invalidated_property() const {\n";
        {
            INDENT();
            OSS << "return PropertyCounter{\n";
            {
                INDENT();
                for (const auto& properties : pg.mProperties) {
                    const auto& c0 = properties.mConcepts.at(0);
                    OSS << "." << c0.getBitsName() << " = static_cast<uint32_t>("
                        << c0.getMaskName() << ".count()),\n";
                }
            }
            OSS << "};\n";
        }
        OSS << "}\n";
    }

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateStackGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());

    const auto& s = *mGraph;
    const auto& cpp = mStruct;

    if (!s.hasStack())
        return oss.str();

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    oss << "\n";
    OSS << "// GraphStack\n";
    OSS << "using layer_descriptor = uint16_t;\n";
    OSS << "using layers_size_type = uint16_t;\n";
    OSS << "using layer_iterator = boost::integer_range<layer_descriptor>::iterator;\n";
    OSS << "using layer_tag_type = " << layerTagVariantType() << ";\n";
    OSS << "using layer_value_type = " << layerValueVariantType(false) << ";\n";
    OSS << "using layer_const_value_type = " << layerValueVariantType(true) << ";\n";
    OSS << "using layer_handle_type = ";
    copyString(oss, space, layerHandleVariantType(), true);
    oss << ";\n";

    oss << "\n";
    OSS << "// GraphStack help functions\n";
    OSS << "static layer_descriptor null_layer() noexcept {\n";
    OSS << "    return std::numeric_limits<layer_descriptor>::max();\n";
    OSS << "}\n";
    oss << "\n";
    OSS << "inline boost::integer_range<layer_descriptor> layer_set() const noexcept {\n";
    OSS << "    return boost::integer_range<layer_descriptor>(0, gsl::narrow_cast<layers_size_type>(mLayers.size()));\n";
    OSS << "}\n";

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateVisitors_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    std::pmr::string space(get_allocator());

    const auto& cpp = mStruct;
    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (!mImpl) {
        oss << "\n";
        OSS << "// Graph Visitors\n";
        auto outputDFS = [&](std::string_view func, std::string_view method, bool bVertex) {
            OSS << "template <class Visitor>\n";
            OSS << "void " << func << "(Visitor&& vis";
            if (bVertex) {
                oss << ", Vertex startVertex";
            }
            oss << ", PmrMemoryResource* scratch) const;\n";
        };
        outputDFS("dfs", "depthFirstSearch", true);
        outputDFS("dfs", "depthFirstSearch", false);
    } else {
        auto outputDFS = [&](std::string_view func, std::string_view method, bool bVertex) {
            OSS << "template <class Visitor>\n";
            OSS << "void " << name << "::" << func << "(Visitor&& vis";
            if (bVertex) {
                oss << ", Vertex startVertex";
            }
            oss << ", PmrMemoryResource* scratch) const {\n";
            {
                INDENT();
                if (s.isVector()) {
                    OSS << "PmrVector<GraphColor> colors(num_vertices(), scratch);\n";
                } else {
                    OSS << "std::pmr::unordered_map<Vertex, GraphColor> colors(scratch);\n";
                    OSS << "colors.reserve(num_vertices());\n";
                }
                OSS << "auto colorMap = GraphImpl::makeGetterSetter(colors);\n";
                if (bVertex) {
                    OSS << method << "(*this, std::forward<Visitor>(vis), colorMap, startVertex, scratch);\n";
                } else {
                    OSS << method << "(*this, std::forward<Visitor>(vis), colorMap, scratch);\n";
                }
            }
            OSS << "}\n";
        };
        outputDFS("dfs", "depthFirstSearch", true);
        outputDFS("dfs", "depthFirstSearch", false);
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateMemberFunctions_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    std::pmr::string api(get_allocator());
    api = mStruct.mAPI;
    if (!api.empty()) {
        api.append("_API ");
    }

    std::pmr::string space(get_allocator());

    if (!s.mMemberFunctions.empty()) {
        oss << "\n";
        OSS << "// Member Functions\n";
    }
    for (const auto& func : s.mMemberFunctions) {
        auto functions = boost::algorithm::replace_all_copy(
            func, "[[dll]] ", api);
        functions = boost::algorithm::replace_all_copy(
            functions, "[[sender]] ", api);
        copyCppString(oss, space, functions);
    }
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateReserve_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;

    auto hasReserve = [&]() {
        if (s.isVector())
            return true;
        return false;
    };

    if (!hasReserve())
        return oss.str();

    std::pmr::string space(get_allocator());

    const auto& cpp = mStruct;
    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (!mImpl) {
        oss << "\n";
        OSS << "// ContinousContainer\n";
        if (!mStruct.mAPI.empty()) {
            OSS << mStruct.mAPI << "_API ";
        } else {
            OSS;
        }

        oss << "void reserve(" << s.mVertexSizeType << " sz);\n";

        if (s.mClear) {
            oss << "void clear() noexcept;\n";
        }
    }

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateTags_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (s.mComponents.empty()) {
        return oss.str();
    }

    std::pmr::string space(get_allocator());
    auto scratch = get_allocator().resource();
    oss << "\n";
    int count = 0;
    for (const auto& c : s.mComponents) {
        auto tagName = getTagName(c.mName, scratch);
        OSS << "struct " << c.mName << " {\n";
        OSS << "} static constexpr " << tagName << " = {};\n";
        ++count;
    }
    oss << "\n";
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateMembers_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    copyString(oss, space, mStruct.generateMembers());
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateGraph_cpp() const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios::out, scratch);
    const auto& cpp = mStruct;
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    std::string_view ns = cpp.mCurrentNamespace;

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    std::pmr::string space(get_allocator());

    if (s.mIncidence) {
        oss << "\n";
        OSS << "std::pair<" << name << "::Edge, bool> " << name << "::edge(Vertex u, Vertex v) const noexcept {\n";
        {
            INDENT();
            OSS << "for (auto e : out_edges(u)) {\n";
            {
                INDENT();
                OSS << "if (target(e) == v) {\n";
                OSS << "    return { e, true };\n";
                OSS << "}\n";
            }
            OSS << "}\n";
            OSS << "return { Edge(null_vertex(), null_vertex()), false };\n";
        }
        OSS << "}\n";

        if (s.mReferenceGraph) {
            oss << "\n";
            OSS << "std::pair<" << name << "::Link, bool> " << name << "::linked(Vertex u, Vertex v) const noexcept {\n";
            {
                INDENT();
                if (s.mAliasGraph) {
                    OSS << "return edge(u, v);\n";
                } else {
                    OSS << "for (auto l : child_links(u)) {\n";
                    {
                        INDENT();
                        OSS << "if (child(l) == v) {\n";
                        OSS << "    return { l, true };\n";
                        OSS << "}\n";
                    }
                    OSS << "}\n";
                    OSS << "return { Link(null_vertex(), null_vertex()), false };\n";
                }
            }
            OSS << "}\n";

            if (!s.mAliasGraph || s.isBidirectionalOnly()) {
                oss << "\n";
                OSS << "bool " << name << "::descendant(Vertex ancestor, Vertex v) const noexcept {\n";
                {
                    INDENT();
                    copyString(oss, space, R"(if (v == null_vertex()) {
    return false;
}
if (ancestor == v) {
    return false;
}
if (ancestor == null_vertex()) {
    return true;
}
for (auto [beg, end] = parent_links(v); beg != end;) {
)");
                    if (s.isVector()) {
                        OSS << "    auto p = parent(*beg);";
                    } else {
                        OSS << "    auto* p = parent(*beg);";
                    }
                    copyString(oss, space, R"(
    if (ancestor == p) {
        return true;
    }
    std::tie(beg, end) = parent_links(p);
}
return false;
)");
                }
                OSS << "}\n";
            }
        }
    }

    if (s.mReferenceGraph && s.mNamed && s.mUniqueName) {
        oss << "\n";
        OSS << "bool " << name << "::uniqueAddress() const noexcept {\n";
        if (true) {
            INDENT();
            OSS << "PmrSet<PmrString> names;\n";
            OSS << "for (auto v : vertices()) {\n";
            {
                INDENT();
                OSS << "if (num_parents(v) != 0) {\n";
                OSS << "    continue;\n";
                OSS << "}\n";
                OSS << "auto res = names.emplace(name(v));\n";
                OSS << "if (!res.second) {\n";
                OSS << "    return false;\n";
                OSS << "}\n";
            }
            OSS << "}\n";
            OSS << "return true;\n";
        } else {
            OSS << "return true;\n";
        }
        OSS << "}\n";
    }

    if (s.mIncidence && s.mEdgeList) {
        oss << "\n";
        OSS << "IteratorPair<" << name << "::EdgeIter> " << name << "::edges() const noexcept {\n";
        {
            INDENT();
            if (s.needEdgeList()) {
                copyString(oss, space, R"(return {
    EdgeIter{ mEdges.begin() },
    EdgeIter{ mEdges.end() }
};
)");
            } else {
                OSS << "auto [beg, end] = vertices();\n";
                OSS << "return {\n";
                OSS << "    { beg, beg, end, *this },\n";
                OSS << "    { beg, end, end, *this }\n";
                OSS << "};\n";
            }
        }
        OSS << "}\n";

        if (s.needEdgeList()) {
            oss << "\n";
            OSS << s.mEdgeSizeType << " " << name << "::num_edges() const noexcept {\n";
            OSS << "    return static_cast<" << s.mEdgeSizeType << ">(std::size(mEdges));\n";
            OSS << "}\n";
        } else {
            oss << "\n";
            OSS << s.mEdgeSizeType << " " << name << "::num_edges() const noexcept {\n";
            {
                INDENT();
                OSS << s.mEdgeSizeType << " numEdges = 0;\n";
                if (s.isVector()) {
                    OSS << "for (auto u : vertices()) {\n";
                } else {
                    OSS << "for (auto* u : vertices()) {\n";
                }
                OSS << "    numEdges += out_degree(u);\n";
                OSS << "}\n";
                OSS << "return numEdges;\n";
            }
            OSS << "}\n";
        }
        if (s.mReferenceGraph) {
            oss << "\n";
            OSS << "IteratorPair<" << name << "::LinkIter> " << name << "::links() const noexcept {\n";
            {
                INDENT();
                if (s.mAliasGraph) {
                    OSS << "return edges();\n";
                } else {
                    OSS << "auto [beg, end] = vertices();\n";
                    OSS << "return {\n";
                    OSS << "    { beg, beg, end, *this },\n";
                    OSS << "    { beg, end, end, *this }\n";
                    OSS << "};\n";
                }
            }
            OSS << "}\n";

            oss << "\n";
            OSS << s.mEdgeSizeType << " " << name << "::num_links() const noexcept {\n";
            {
                INDENT();
                if (s.mAliasGraph) {
                    OSS << "return num_edges();\n";
                } else {
                    OSS << s.mEdgeSizeType << " numLinks = 0;\n";
                    OSS << "for (auto u : vertices()) {\n";
                    OSS << "    numLinks += num_children(u);\n";
                    OSS << "}\n";
                    OSS << "return numLinks;\n";
                }
            }
            OSS << "}\n";
        }
    }

    if (s.mMutableGraphVertex) {
        if (s.mReferenceGraph && s.mNamed && s.mUniqueName) {
            oss << "\n";
            OSS << "bool " << name << "::detachable(Vertex v) const noexcept {\n";
            {
                INDENT();
                OSS << "return find_child(null_vertex(), name(v)) == null_vertex();\n";
            }
            OSS << "}\n";
        }

        if (s.mIncidence) {
            oss << "\n";
            OSS << "void " << name << "::remove_edge(const OutEdgeIter& outIter) noexcept {";
            if (!s.isVector()) {
                oss << " // NOLINT(readability-make-member-function-const)";
            }
            oss << "\n";
            {
                INDENT();
                OSS << "auto e = *outIter;\n";
                bool bDefineV
                    = !s.isDirectedOnly()
                    || (s.mReferenceGraph && s.mAliasGraph && s.mNamed && s.mUniqueName);

                if (s.isVector()) {
                    OSS << "const auto u = source(e);\n";
                    if (bDefineV) {
                        OSS << "const auto v = target(e);\n";
                    }
                } else {
                    OSS << "auto* u = source(e);\n";
                    if (bDefineV) {
                        OSS << "auto* v = target(e);\n";
                    }
                }
                if (s.mReferenceGraph && s.mAliasGraph && s.mNamed && s.mUniqueName) {
                    OSS << "Expects(detachable(v));\n";
                }
                if (s.isVector()) {
                    OSS << "auto& s = mVertices[u];\n";
                    if (!s.isDirectedOnly()) {
                        OSS << "auto& t = mVertices[v];\n";
                    }
                } else {
                    OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                    if (!s.isDirectedOnly()) {
                        OSS << "auto& t = *static_cast<VertexType*>(v);\n";
                    }
                }
                if (s.needEdgeList()) {
                    OSS << "auto edgeIter = outIter.iterator->iterator;\n";
                    if (s.isDirectedOnly()) {
                        Expects(false);
                    } else if (s.isBidirectionalOnly()) {
                        OSS << inEdgeType() << " ie(u, edgeIter);\n";
                        OSS << "auto inIter = std::find(t.mInEdges.begin(), t.mInEdges.end(), ie);\n";
                        OSS << "Expects(inIter != t.mInEdges.end());\n";
                        OSS << "t.mInEdges.erase(inIter);\n";
                    } else {
                        OSS << outEdgeType() << " oe(u, edgeIter);\n";
                        OSS << "auto outIter = std::find_if(t.mOutEdges.begin(), t.mOutEdges.end(), oe);\n";
                        OSS << "Expects(outIter != t.mOutEdges.end());\n";
                        OSS << "t.mOutEdges.erase(outIter);\n";
                    }
                    OSS << "mEdges.erase(edgeIter);\n";
                } else {
                    if (s.isDirectedOnly()) {
                        // noop
                    } else if (s.isBidirectionalOnly()) {
                        OSS << "auto inIter = std::find(t.mInEdges.begin(), t.mInEdges.end(), u);\n";
                        OSS << "Expects(inIter != t.mInEdges.end());\n";
                        OSS << "t.mInEdges.erase(inIter);\n";
                    } else {
                        OSS << "auto outIter = std::find_if(t.mOutEdges.begin(), t.mOutEdges.end(), u);\n";
                        OSS << "Expects(outIter != t.mOutEdges.end());\n";
                        OSS << "t.mOutEdges.erase(outIter);\n";
                    }
                }
                OSS << "s.mOutEdges.erase(outIter.iterator);\n";
            }
            OSS << "}\n";

            oss << "\n";
            OSS << "void " << name << "::remove_edge(Edge e) noexcept {\n";
            {
                INDENT();
                if (s.needEdgeList()) {
                    if (s.isDirectedOnly()) {
                        Expects(false);
                    } else {
                        if (s.isVector()) {
                            OSS << "const auto u = source(e);\n";
                            // OSS << "const auto v = target(e);\n";
                        } else {
                            OSS << "auto* u = source(e);\n";
                            // OSS << "auto* v = target(e);\n";
                        }
                        OSS << "auto* const p = e.property;\n";
                        if (s.isVector()) {
                            OSS << "auto& s = mVertices[u];\n";
                        } else {
                            OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                        }
                        // find edgeIter
                        OSS << "auto outIter = std::find_if(s.mOutEdges.begin(), s.mOutEdges.end(), [p](const auto& oe) {\n";
                        OSS << "    return &oe.property() == p;\n";
                        OSS << "});\n";
                        OSS << "Expects(outIter != s.mOutEdges.end());\n";
                    }
                } else {
                    if (s.isVector()) {
                        OSS << "const auto u = source(e);\n";
                        OSS << "const auto v = target(e);\n";
                    } else {
                        OSS << "auto* u = source(e);\n";
                        OSS << "auto* v = target(e);\n";
                    }
                    if (s.isVector()) {
                        OSS << "auto& s = mVertices[u];\n";
                    } else {
                        OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                    }
                    OSS << "auto outIter = std::find(s.mOutEdges.begin(), s.mOutEdges.end(), v);\n";
                    OSS << "Expects(outIter != s.mOutEdges.end());\n";
                }
                OSS << "remove_edge(OutEdgeIter(u, outIter));\n";
            }
            OSS << "}\n";

            oss << "\n";
            OSS << "void " << name << "::remove_edges(Vertex u, Vertex v) noexcept {\n";
            {
                INDENT();
                if (s.mReferenceGraph && s.mAliasGraph && s.mNamed && s.mUniqueName) {
                    OSS << "Expects(detachable(v));\n";
                }
                if (s.needEdgeList()) {
                    if (s.isDirectedOnly()) {
                        Expects(false);
                    } else {
                        if (s.isVector()) {
                            OSS << "auto& s = mVertices[u];\n";
                            OSS << "auto& t = mVertices[v];\n";
                        } else {
                            OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                            OSS << "auto& t = *static_cast<VertexType*>(v);\n";
                        }
                        // remove edges
                        oss << "\n";
                        OSS << "Impl::removeEdges(*this, s.mOutEdges, v);\n";
                        // remove incidence edges
                        oss << "\n";
                        OSS << "s.mOutEdges.erase(\n";
                        {
                            INDENT();
                            OSS << "std::remove_if(s.mOutEdges.begin(), s.mOutEdges.end(),\n";
                            {
                                INDENT();
                                OSS << "[v](const " << outEdgeType() << "& oe) {\n";
                                OSS << "    return oe.target == v;\n";
                                OSS << "}),\n";
                            }
                            OSS << "s.mOutEdges.end());\n";
                        }
                        if (s.isBidirectionalOnly()) {
                            OSS << "t.mInEdges.erase(\n";
                            {
                                INDENT();
                                OSS << "std::remove_if(t.mInEdges.begin(), t.mInEdges.end(),\n";
                                {
                                    INDENT();
                                    OSS << "[u](const " << inEdgeType() << "& ie) {\n";
                                    OSS << "    return ie.target == u;\n";
                                    OSS << "}),\n";
                                }
                                OSS << "t.mInEdges.end());\n";
                            }
                        } else {
                            OSS << "t.mOutEdges.erase(\n";
                            {
                                INDENT();
                                OSS << "std::remove_if(t.mOutEdges.begin(), t.mOutEdges.end(),\n";
                                {
                                    INDENT();
                                    OSS << "[u](const " << outEdgeType() << "& oe) {\n";
                                    OSS << "    return oe.target == u;\n";
                                    OSS << "}),\n";
                                }
                                OSS << "t.mOutEdges.end());\n";
                            }
                        }
                    }
                } else {
                    if (s.isDirectedOnly()) {
                        if (s.isVector()) {
                            OSS << "auto& s = mVertices[u];\n";
                        } else {
                            OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                        }
                        OSS << "s.mOutEdges.erase(std::remove(s.mOutEdges.begin(), s.mOutEdges.end(), v), s.mOutEdges.end());\n";
                    } else {
                        if (s.isVector()) {
                            OSS << "auto& s = mVertices[u];\n";
                            OSS << "auto& t = mVertices[v];\n";
                        } else {
                            OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                            OSS << "auto& t = *static_cast<VertexType*>(v);\n";
                        }
                        OSS << "s.mOutEdges.erase(std::remove(s.mOutEdges.begin(), s.mOutEdges.end(), v), s.mOutEdges.end());\n";
                        if (s.isBidirectionalOnly()) {
                            OSS << "t.mInEdges.erase(std::remove(t.mInEdges.begin(), t.mInEdges.end(), u), t.mInEdges.end());\n";
                        } else {
                            OSS << "t.mOutEdges.erase(std::remove_if(t.mOutEdges.begin(), t.mOutEdges.end(), u), t.mOutEdges.end());\n";
                        }
                    }
                }
            }
            OSS << "}\n";

            if (s.mReferenceGraph) {
                oss << "\n";
                OSS << "void " << name << "::remove_link(const ChildLinkIter& iter) noexcept {\n";
                {
                    INDENT();
                    if (s.mAliasGraph) {
                        OSS << "remove_edge(iter);\n";
                    } else {
                        OSS << "auto l = *iter;\n";
                        OSS << "const auto u = parent(l);\n";
                        OSS << "const auto v = child(l);\n";
                        if (s.mNamed && s.mUniqueName) {
                            OSS << "Expects(detachable(v));\n";
                        }
                        if (s.isVector()) {
                            OSS << "auto& s = mVertices[u];\n";
                            OSS << "auto& t = mVertices[v];\n";
                        } else {
                            OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                            OSS << "auto& t = *static_cast<VertexType*>(v);\n";
                        }
                        OSS << "auto parentIter = std::find(t.mParents.begin(), t.mParents.end(), u);\n";
                        OSS << "Expects(parentIter != t.mParents.end());\n";
                        OSS << "t.mParents.erase(parentIter);\n";
                        OSS << "s.mChildren.erase(iter.iterator);\n";
                    }
                }
                OSS << "}\n";

                oss << "\n";
                OSS << "void " << name << "::remove_link(Link l) noexcept {\n";
                {
                    INDENT();
                    if (s.mAliasGraph) {
                        OSS << "remove_edge(l);\n";
                    } else {
                        OSS << "const auto u = parent(l);\n";
                        OSS << "const auto v = child(l);\n";
                        if (s.isVector()) {
                            OSS << "auto& s = mVertices[u];\n";
                            // OSS << "auto& t = mVertices[v];\n";
                        } else {
                            OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                            // OSS << "auto& t = *static_cast<VertexType*>(v);\n";
                        }
                        OSS << "auto childIter = std::find(s.mChildren.begin(), s.mChildren.end(), v);\n";
                        OSS << "Expects(childIter != s.mChildren.end());\n";
                        OSS << "remove_link(ChildLinkIter(u, childIter));\n";
                    }
                }
                OSS << "}\n";

                oss << "\n";
                OSS << "void " << name << "::remove_links(Vertex u, Vertex v) noexcept {\n";
                {
                    INDENT();
                    if (s.mAliasGraph) {
                        OSS << "remove_edges(u, v);\n";
                    } else {
                        if (s.mNamed && s.mUniqueName) {
                            OSS << "Expects(detachable(v));\n";
                        }
                        if (s.isVector()) {
                            OSS << "auto& s = mVertices[u];\n";
                            OSS << "auto& t = mVertices[v];\n";
                        } else {
                            OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                            OSS << "auto& t = *static_cast<VertexType*>(v);\n";
                        }
                        OSS << "s.mChildren.erase(std::remove(s.mChildren.begin(), s.mChildren.end(), v), s.mChildren.end());\n";
                        OSS << "t.mParents.erase(std::remove(t.mParents.begin(), t.mParents.end(), u), t.mParents.end());\n";
                    }
                }
                OSS << "}\n";
            }

            if (!(s.mReferenceGraph && s.mAliasGraph && s.mNamed && s.mUniqueName)) {
                oss << "\n";
                OSS << "void " << name << "::clear_out_edges(Vertex u) noexcept {\n";
                {
                    INDENT();
                    if (s.isVector()) {
                        OSS << "auto& s = mVertices[u];\n";
                    } else {
                        OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                    }
                    if (s.needEdgeList()) {
                        OSS << "for (const auto& oe : s.mOutEdges) {\n";
                        {
                            INDENT();
                            OSS << "mEdges.erase(oe.iterator);\n";
                            if (s.isDirectedOnly()) {
                                Expects(false);
                            } else {
                                OSS << "auto& t = mVertices[oe.target];\n";
                                if (s.mBidirectional) {
                                    OSS << "t.mInEdges.erase(\n";
                                    {
                                        INDENT();
                                        OSS << "std::remove_if(t.mInEdges.begin(), t.mInEdges.end(),\n";
                                        {
                                            INDENT();
                                            OSS << "[u](const " << inEdgeType() << "& ie) {\n";
                                            OSS << "    return ie.target == u;\n";
                                            OSS << "}),\n";
                                        }
                                        OSS << "t.mInEdges.end());\n";
                                    }
                                } else {
                                    OSS << "t.mOutEdges.erase(\n";
                                    {
                                        INDENT();
                                        OSS << "std::remove_if(t.mOutEdges.begin(), t.mOutEdges.end(),\n";
                                        {
                                            INDENT();
                                            OSS << "[u](const " << outEdgeType() << "& oe) {\n";
                                            OSS << "    return oe.target == u;\n";
                                            OSS << "}),\n";
                                        }
                                        OSS << "t.mOutEdges.end());\n";
                                    }
                                }
                            }
                        }
                        OSS << "}\n";
                    } else if (!s.isDirectedOnly()) {
                        OSS << "for (const auto& v : s.mOutEdges) {\n";
                        {
                            INDENT();
                            if (s.isVector()) {
                                OSS << "auto& t = mVertices[v];\n";
                            } else {
                                OSS << "auto& t = *static_cast<VertexType*>(v);\n";
                            }
                            Expects(!s.isDirectedOnly());
                            if (s.mBidirectional) {
                                OSS << "t.mInEdges.erase(std::remove(t.mInEdges.begin(), t.mInEdges.end(), u), t.mInEdges.end());\n";
                            } else {
                                OSS << "t.mOutEdges.erase(std::remove(t.mOutEdges.begin(), t.mOutEdges.end(), u), t.mOutEdges.end());\n";
                            }
                        }
                        OSS << "}\n";
                    }
                    OSS << "s.mOutEdges.clear();\n";
                }
                OSS << "}\n";
            }

            if (s.mBidirectional) {
                oss << "\n";
                OSS << "void " << name << "::clear_in_edges(Vertex v) noexcept {\n";
                {
                    INDENT();
                    if (s.isVector()) {
                        OSS << "auto& t = mVertices[v];\n";
                    } else {
                        OSS << "auto& t = *static_cast<VertexType*>(v);\n";
                    }
                    if (s.needEdgeList()) {
                        OSS << "for (const auto& ie : t.mInEdges) {\n";
                        {
                            INDENT();
                            OSS << "mEdges.erase(ie.iterator);\n";
                            OSS << "auto& s = mVertices[ie.target];\n";
                            OSS << "s.mOutEdges.erase(\n";
                            {
                                INDENT();
                                OSS << "std::remove_if(s.mOutEdges.begin(), s.mOutEdges.end(),\n";
                                {
                                    INDENT();
                                    OSS << "[v](const " << outEdgeType() << "& oe) {\n";
                                    OSS << "    return oe.target == v;\n";
                                    OSS << "}),\n";
                                }
                                OSS << "s.mOutEdges.end());\n";
                            }
                        }
                        OSS << "}\n";
                    } else {
                        OSS << "for (const auto& u : t.mInEdges) {\n";
                        {
                            INDENT();
                            if (s.isVector()) {
                                OSS << "auto& s = mVertices[u];\n";
                            } else {
                                OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                            }
                            OSS << "s.mOutEdges.erase(std::remove(s.mOutEdges.begin(), s.mOutEdges.end(), v), s.mOutEdges.end());\n";
                        }
                        OSS << "}\n";
                    }
                    OSS << "t.mInEdges.clear();\n";
                }
                OSS << "}\n";
            }

            if (!(s.isUniqueAddressAliasGraph() || s.isAliasDirected())) {
                oss << "\n";
                OSS << "void " << name << "::clear_edges(Vertex v) noexcept {\n";
                OSS << "    clear_out_edges(v);\n";
                if (s.mBidirectional) {
                    OSS << "    clear_in_edges(v);\n";
                }
                OSS << "}\n";
            }

            if (s.mReferenceGraph) {
                if (s.mAliasGraph) {
                    Expects(!s.mUndirected);
                    if (!(s.mNamed && s.mUniqueName)) {
                        oss << "\n";
                        OSS << "void " << name << "::clear_children(Vertex u) noexcept {\n";
                        OSS << "    clear_out_edges(u);\n";
                        OSS << "}\n";
                    }
                    if (s.isBidirectionalOnly()) {
                        oss << "\n";
                        OSS << "void " << name << "::clear_parents(Vertex v) noexcept {\n";
                        OSS << "    clear_in_edges(v);\n";
                        OSS << "}\n";
                    }
                } else {
                    if (!s.mNamed && s.mUniqueName) {
                        oss << "\n";
                        OSS << "void " << name << "::clear_children(Vertex u) noexcept {\n";
                        {
                            INDENT();
                            if (s.isVector()) {
                                OSS << "auto& s = mVertices[u];\n";
                            } else {
                                OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                            }
                            OSS << "for (const auto& v : s.mChildren) {\n";
                            {
                                INDENT();
                                if (s.isVector()) {
                                    OSS << "auto& t = mVertices[v];\n";
                                } else {
                                    OSS << "auto& t = *static_cast<VertexType*>(v);\n";
                                }
                                OSS << "t.mParents.erase(std::remove(t.mParents.begin(), t.mParents.end(), u), t.mParents.end());\n";
                            }
                            OSS << "}\n";
                            OSS << "s.mChildren.clear();\n";
                        }
                        OSS << "}\n";
                    }
                    oss << "\n";
                    OSS << "void " << name << "::clear_parents(Vertex v) noexcept {\n";
                    {
                        INDENT();
                        if (s.isVector()) {
                            OSS << "auto& t = mVertices[v];\n";
                        } else {
                            OSS << "auto& t = *static_cast<VertexType*>(v);\n";
                        }
                        OSS << "for (const auto& u : t.mParents) {\n";
                        {
                            INDENT();
                            if (s.isVector()) {
                                OSS << "auto& s = mVertices[u];\n";
                            } else {
                                OSS << "auto& s = *static_cast<VertexType*>(u);\n";
                            }
                            OSS << "s.mChildren.erase(std::remove(s.mChildren.begin(), s.mChildren.end(), v), s.mChildren.end());\n";
                        }
                        OSS << "}\n";
                        OSS << "t.mParents.clear();\n";
                    }
                    OSS << "}\n";
                }
                if (!s.mNamed && s.mUniqueName) {
                    oss << "\n";
                    OSS << "void " << name << "::clear_links(Vertex v) noexcept {\n";
                    {
                        INDENT();
                        OSS << "clear_children(v);\n";
                        if (!s.mAliasGraph || s.isBidirectionalOnly()) {
                            OSS << "clear_parents(v);\n";
                        }
                    }
                    OSS << "}\n";
                }
            }

            if (!(s.isUniqueAddressGraph() || s.isAliasDirected())) {
                oss << "\n";
                OSS << "void " << name << "::clear_vertex(Vertex v) noexcept {\n";
                {
                    INDENT();
                    OSS << "clear_edges(v);\n";
                    if (s.mReferenceGraph) {
                        OSS << "clear_links(v);\n";
                    }
                }
                OSS << "}\n";
            }
        }

        oss << "\n";
        if (s.isPolymorphic() && s.mPolymorphic.mConcepts.size() > 6) {
            OSS << "// NOLINTNEXTLINE(readability-function-cognitive-complexity)\n";
        }
        OSS << "void " << name << "::remove_vertex(Vertex u) noexcept {\n";
        {
            INDENT();
            if (s.mIncidence) {
                OSS << "Expects(out_degree(u) == 0);\n";
                if (s.mBidirectional) {
                    OSS << "Expects(in_degree(u) == 0);\n";
                }
            }
            if (s.mReferenceGraph && !s.mAliasGraph) {
                OSS << "Expects(num_children(u) == 0);\n";
                OSS << "Expects(num_parents(u) == 0);\n";
            }

            // remove map
            if (!s.mVertexMaps.empty()) {
                oss << "\n";
                OSS << "// UuidGraph\n";
            }
            for (const auto& map : s.mVertexMaps) {

                OSS << "{";
                if (map.mOptional) {
                    oss << " // Optional";
                }
                oss << "\n";
                {
                    INDENT();
                    const auto& c = s.getComponent(map.mComponentName);
                    const auto& member = c.mMemberName;
                    if (s.isVector()) {
                        OSS << "const auto& key = " << member << "[u];\n";
                    } else {
                        OSS << "const auto& key = static_cast<const VertexType*>(u)->"
                            << member << ";\n";
                    }
                    if (map.mOptional) {
                        OSS << "if (!key.empty()) {\n";
                        {
                            INDENT();
                            OSS << "auto num = " << map.mMemberName << ".erase(key);\n";
                            OSS << "Ensures(num == 1);\n";
                        }
                        OSS << "}\n";
                    } else {
                        OSS << "auto num = " << map.mMemberName << ".erase(key);\n";
                        OSS << "Ensures(num == 1);\n";
                    }
                    if (s.isVector()) {
                        if (!s.mRecycle) {
                            OSS << "for (auto&& [_, v] : " << map.mMemberName << ") {\n";
                            {
                                INDENT();
                                OSS << "if (v > u) {\n";
                                OSS << "    --v;\n";
                                OSS << "}\n";
                            }
                            OSS << "}\n";
                        }
                    }
                }
                OSS << "}\n";
            }

            // remove polymorphic
            if (s.isVector() && s.isPolymorphic()) {
                oss << "\n";
                OSS << "// PolymorphicGraph\n";
                if (false) {
                    OSS << "try {\n";
                }
                {
                    // INDENT();
                    {
                        OSS << "visit(\n";
                        INDENT();
                        OSS << "overloaded{\n";
                        {
                            INDENT();
                            int count = 0;
                            for (const auto& c : s.mPolymorphic.mConcepts) {
                                if (count++)
                                    oss << ",\n";
                                OSS << "[&](";
                                if (s.isVector() && !s.mRecycle) {
                                    oss << "const ";
                                }
                                bool useTaggedValue
                                    = !c.isIntrusive()
                                    || (s.isVector() && s.mRecycle);
                                if (useTaggedValue) {
                                    oss << handleElemType(c, ns, true) << "& t) {";
                                } else {
                                    oss << handleElemType(c, ns, true) << "&) {";
                                }
                                if (!c.isIntrusive()) {
                                    INDENT();
                                    oss << "\n";
                                    OSS << "const auto iter = " << c.mMemberName << ".erase(" << c.mMemberName << ".begin() + t.mValue);\n";
                                    OSS << "if (iter == " << c.mMemberName << ".end()) {\n";
                                    OSS << "    return;\n";
                                    OSS << "}\n";
                                    OSS << "Impl::reindexVariant<"
                                        << cpp.getDependentName(c.mTag)
                                        << ">(mVariants, t.mValue);\n";
                                    UNINDENT();
                                    OSS;
                                } else {
                                    if (s.isVector() && s.mRecycle) {
                                        INDENT();
                                        oss << "\n";
                                        const auto& tagID = locate(c.mTag, g);
                                        const auto& typeID = locate(c.mValue, g);
                                        auto tagName = g.getDependentCppName(ns, tagID);
                                        auto typeName = g.getDependentCppName(ns, typeID);
                                        OSS << "t = {};\n";
                                        UNINDENT();
                                        OSS;
                                    }
                                }
                                oss << "}";
                            }
                            oss << " },\n";
                        }
                        if (s.isVector()) {
                            OSS << "mVariants[u]);\n";
                        } else {
                            OSS << "static_cast<VertexType*>(u)->mVariant);\n";
                        }
                    }
                }
                if (false) {
                    OSS << "} catch (...) {\n";
                    OSS << "    // noop\n";
                    OSS << "}\n";
                }
            } // remove polymorphic

            // reindex edges
            if (s.mIncidence && s.mEdgeList && s.needEdgeList() && s.isVector() && !s.mRecycle) {
                oss << "\n";
                OSS << "// EdgeListGraph\n";
                OSS << "Impl::reindexEdges(mEdges, u);\n";
            }

            // remove vertex
            bool bPmr = g.isPmr(cpp.mCurrentVertex);
            if (s.mVertexList) {
                oss << "\n";
                OSS << "// VertexListGraph\n";
                if (s.isVector()) {
                    if (s.mRecycle) {
                        if (bPmr) {
                            OSS << "mVertices[u] = VertexType(get_allocator());\n";
                        } else {
                            OSS << "mVertices[u] = VertexType{};\n";
                        }
                        OSS << "Expects(mEmptyBitset[u] == false);\n";
                        OSS << "mEmptyBitset[u] = true;\n";
                        if (s.mGarbageCollection) {
                            OSS << "Expects(mRootBitset[u] == false);\n";
                        }
                    } else {
                        if (s.mIncidence) {
                            OSS << "mVertices.erase(mVertices.begin() + u);\n";
                        }
                        if (s.isPolymorphic()) {
                            OSS << "mVariants.erase(mVariants.begin() + u);\n";
                        }
                        if (s.mGarbageCollection) {
                            OSS << "mRootBitset.erase(mRootBitset.begin() + u);\n";
                        }
                    }
                } else {
                    Expects(!s.mGarbageCollection);
                    OSS << "auto iter = std::find_if(mVertices.begin(), mVertices.end(),\n";
                    {
                        INDENT();
                        OSS << "[&](const VertexType& vert) {\n";
                        OSS << "    return &vert == u;\n";
                        OSS << "});\n";
                    }
                    OSS << "mVertices.erase(iter);\n";
                }
                if (s.isVector() && s.mIncidence && !s.mRecycle) {
                    oss << "\n";
                    OSS << "// IncidenceGraph\n";
                    OSS << "for (auto& vert : mVertices) {\n";
                    {
                        INDENT();
                        if (s.isDirectedOnly()) {
                            if (s.needEdgeList()) {
                                Expects(false);
                            } else {
                                OSS << "Impl::reindexIncidenceEdges(vert.mOutEdges, u);\n";
                            }
                        } else if (s.isBidirectionalOnly()) {
                            if (s.needEdgeList()) {
                                OSS << "Impl::reindexIncidencePropertyEdges(vert.mOutEdges, u);\n";
                                OSS << "Impl::reindexIncidencePropertyEdges(vert.mInEdges, u);\n";
                            } else {
                                OSS << "Impl::reindexIncidenceEdges(vert.mOutEdges, u);\n";
                                OSS << "Impl::reindexIncidenceEdges(vert.mInEdges, u);\n";
                            }
                        } else {
                            if (s.needEdgeList()) {
                                OSS << "Impl::reindexIncidencePropertyEdges(vert.mOutEdges, u);\n";
                            } else {
                                OSS << "Impl::reindexIncidenceEdges(vert.mOutEdges, u);\n";
                            }
                        }
                        if (s.isReference() && !s.mAliasGraph) {
                            OSS << "Impl::reindexIncidenceEdges(vert.mChildren, u);\n";
                            OSS << "Impl::reindexIncidenceEdges(vert.mParents, u);\n";
                        }
                    }
                    OSS << "}\n";
                }
            }
            // remove component
            if (s.isVector() && !s.mComponents.empty()) {
                oss << "\n";
                OSS << "// ComponentGraph\n";
                if (s.mRecycle) {
                    for (const auto& c : s.mComponents) {
                        auto cID = locate(c.mValuePath, g);
                        auto cName = g.getDependentCppName(ns, cID);
                        const auto& member = c.mMemberName;
                        auto bPmr = g.isPmr(cID);
                        if (bPmr) {
                            OSS << member << "[u] = " << cName << "(get_allocator());\n";
                        } else {
                            OSS << member << "[u] = " << cName << "{};\n";
                        }
                    }
                } else {
                    for (const auto& c : s.mComponents) {
                        const auto& member = c.mMemberName;
                        if (c.mCounterName.empty()) {
                            OSS << member << ".erase(" << member << ".begin() + u);\n";
                        } else {
                            OSS << member << ".erase("
                                << member << ".begin() + static_cast<std::ptrdiff_t>(u) * "
                                << c.mCounterName << ",\n";
                            OSS << "    " << member << ".begin() + static_cast<std::ptrdiff_t>(u) * ("
                                << c.mCounterName << " + 1)" << ");\n";
                        }
                    }
                }
            }

            if (s.hasStack()) {
                Expects(false);
            }

            if (false && s.mReferenceGraph && s.mNamed && s.mUniqueName) {
                oss << "\n";
                OSS << "Ensures(uniqueAddress());\n";
            }
        }
        OSS << "}\n";
    } // MutableGraph

    if (s.mNamed && s.mReferenceGraph) {
        oss << "\n";
        OSS << name << "::Vertex " << name << "::find_child(Vertex u, " << stringViewType() << " name0) const noexcept {\n";
        {
            INDENT();
            copyString(oss, space, R"(if (u == null_vertex()) {
    for (auto v : vertices()) {
        if (num_parents(v) == 0 && name(v) == name0) {
            return v;
        }
    }
    return null_vertex();
})");
            OSS << "for (auto v : children(u)) {\n";
            {
                INDENT();
                OSS << "if (name(v) == name0) {\n";
                OSS << "    return v;\n";
                OSS << "}\n";
            }
            OSS << "}\n";
            OSS << "return null_vertex();\n";
        }
        OSS << "}\n";
    }

    if (s.mAddressable) {
        Expects(s.mNamed && s.mReferenceGraph);
        oss << "\n";
        OSS << "bool " << name << "::exists(" << stringViewType() << " absPath) const noexcept {\n";
        {
            INDENT();
            OSS << "return Impl::findRelative(*this, null_vertex(), absPath) != null_vertex();\n";
        }
        OSS << "}\n";
        oss << "\n";
        OSS << name << "::Vertex " << name << "::locate(" << stringViewType() << " absPath) const noexcept {\n";
        {
            INDENT();
            OSS << "return Impl::findRelative(*this, null_vertex(), absPath);\n";
        }
        OSS << "}\n";
        oss << "\n";
        OSS << name << "::Vertex " << name << "::locate_relative(Vertex u, " << stringViewType() << " relPath) const noexcept {\n";
        {
            INDENT();
            OSS << "Expects(relPath.empty() || relPath.front() != '/');\n";
            OSS << "return Impl::findRelative(*this, u, relPath);\n";
        }
        OSS << "}\n";

        auto strType = stringType(ns);
        std::pmr::string strPath(strType, scratch);
        convertTypename(strPath);
        auto strID = g.lookupType(ns, strPath);
        bool bPmrString = g.isPmr(strID);

        oss << "\n";
        OSS << stringType(ns) << " " << name << "::get_path(Vertex u";
        if (bPmrString) {
            oss << ", PmrMemoryResource* mr";
        }
        oss << ") const {\n";
        {
            INDENT();
            if (bPmrString) {
                OSS << strType << " path(mr);\n";
            } else {
                OSS << strType << " path;\n";
            }
            OSS << "size_t sz = Impl::pathLength(*this, u, null_vertex());\n";
            OSS << "Impl::pathComposite(path, sz, u, *this, null_vertex());\n";
            OSS << "Ensures(sz == 0);\n";
            OSS << "return path;\n";
        }
        OSS << "}\n";

        oss << "\n";
        OSS << stringType(ns) << " " << name << "::get_relative_path(Vertex u";
        if (bPmrString) {
            oss << ", PmrMemoryResource* mr";
        }
        oss << ") const {\n";
        {
            INDENT();
            if (bPmrString) {
                OSS << strType << " path(mr);\n";
            } else {
                OSS << strType << " path;\n";
            }
            OSS << "size_t sz = Impl::relativePathLength(*this, u, null_vertex());\n";
            OSS << "Impl::relativePathComposite(path, sz, u, *this, null_vertex());\n";
            OSS << "Ensures(sz == 0);\n";
            OSS << "return path;\n";
        }
        OSS << "}\n";

        oss << "\n";
        OSS << stringType(ns) << " " << name << "::get_relative_path(Vertex v, Vertex ancestor";
        if (bPmrString) {
            oss << ", PmrMemoryResource* mr";
        }
        oss << ") const {\n";
        {
            INDENT();
            if (bPmrString) {
                OSS << strType << " path(mr);\n";
            } else {
                OSS << strType << " path;\n";
            }
            OSS << "size_t sz = Impl::pathLength(*this, v, ancestor);\n";
            OSS << "Impl::pathComposite(path, sz, v, *this, ancestor);\n";
            OSS << "Ensures(sz == 0);\n";
            OSS << "return path;\n";
        }
        OSS << "}\n";
    }

    if (!s.mVertexMaps.empty()) {
        for (const auto& m : s.mVertexMaps) {
            const auto& c = s.getComponent(m.mComponentName);
            const auto& keyID = locate(m.mKeyType, g);
            auto typeName = g.getDependentCppName(cpp.mCurrentNamespace, keyID);
            oss << "\n";
            OSS << "bool " << name << "::contains("
                << getViewOrConstRef(typeName, scratch)
                << " key) const noexcept {\n";
            {
                INDENT();
                OSS << "return " << m.mMemberName << ".find(key) != " << m.mMemberName << ".end();\n";
            }
            OSS << "}\n";

            oss << "\n";
            OSS << name << "::Vertex " << name << "::at("
                << getViewOrConstRef(typeName, scratch)
                << " key) const {\n";
            {
                INDENT();
                OSS << "auto iter = " << m.mMemberName << ".find(key);\n";
                OSS << "if (iter == " << m.mMemberName << ".end()) {\n";
                OSS << "    throw std::out_of_range(\"resource not found\");\n";
                OSS << "}\n";
                OSS << "return iter->second;\n";
            }
            OSS << "}\n";

            oss << "\n";
            OSS << name << "::Vertex " << name << "::find("
                << getViewOrConstRef(typeName, scratch)
                << " key) const noexcept {\n";
            {
                INDENT();
                OSS << "auto iter = " << m.mMemberName << ".find(key);\n";
                OSS << "if (iter != " << m.mMemberName << ".end()) {\n";
                OSS << "    return iter->second;\n";
                OSS << "}\n";
                OSS << "return null_vertex();\n";
            }
            OSS << "}\n";
        }
    }

    if (s.isVector()) {
        oss << "\n";
        OSS << "void " << name << "::reserve(" << s.mVertexSizeType << " sz) {\n";
        {
            INDENT();
            if (s.isVector()) {
                if (s.mRecycle) {
                    OSS << "mEmptyBitset.reserve(sz);\n";
                }
                if (s.mGarbageCollection) {
                    OSS << "mRootBitset.reserve(sz);\n";
                }
                if (s.mIncidence) {
                    OSS << "::Star::reserve(mVertices, sz);\n";
                }
                for (const auto& c : s.mComponents) {
                    if (c.mCounterName.empty()) {
                        OSS << "::Star::reserve(" << c.mMemberName << ", sz);\n";
                    } else {
                        OSS << "::Star::reserve(" << c.mMemberName << ", static_cast<size_t>(sz) * " << c.mCounterName << ");\n";
                    }
                }
                if (s.isPolymorphic()) {
                    OSS << "mVariants.reserve(sz);\n";
                }
            }
            if (s.hasStack()) {
                int count = 0;
                oss << "\n";
                OSS << "// GraphStack\n";
                for (const auto& layer : s.mStack.mLayers) {
                    if (count++)
                        oss << "\n";
                    OSS << "for (auto& layer : " << layer.mMemberName << ") {\n";
                    OSS << "    layer.reserve(sz);\n";
                    OSS << "}\n";
                }
            }
        }
        OSS << "}\n";
    }

    if (s.mClear) {
        oss << "\n";
        OSS << "void " << name << "::clear() noexcept {\n";
        {
            INDENT();
            if (s.isVector()) {
                if (s.mRecycle) {
                    OSS << "mEmptyBitset.clear();\n";
                }
                if (s.mGarbageCollection) {
                    OSS << "mRootBitset.clear();\n";
                }
                OSS << "mVertices.clear();\n";
                for (const auto& c : s.mComponents) {
                    OSS << c.mMemberName << ".clear();\n";
                }
                if (s.isPolymorphic()) {
                    OSS << "mVariants.clear();\n";
                }
            }
            if (s.hasStack()) {
                int count = 0;
                oss << "\n";
                OSS << "// GraphStack\n";
                for (const auto& layer : s.mStack.mLayers) {
                    if (count++)
                        oss << "\n";
                    OSS << "for (auto& layer : " << layer.mMemberName << ") {\n";
                    OSS << "    layer.clear();\n";
                    OSS << "}\n";
                }
            }
            if (s.needEdgeList()) {
                oss << "\n";
                OSS << "mEdges.clear();\n";
            }
            if (!s.mVertexMaps.empty()) {
                oss << "\n";
                for (const auto& map : s.mVertexMaps) {
                    OSS << map.mMemberName << ".clear();\n";
                }
            }
        }
        OSS << "}\n";
    }

    if (!s.mPropertyGraph.mProperties.empty()) {
        copyString(oss, space, generateGraphPropertyGraph_cpp());
    }

    if (s.mGarbageCollection) {
        Expects(s.isVector());
        Expects(s.mMutableGraphVertex);

        oss << "\n";
        OSS << "void " << name << "::release_vertex(Vertex v) noexcept {\n";
        {
            INDENT();
            OSS << "if (v == null_vertex()) {\n";
            OSS << "    return;\n";
            OSS << "}\n";
            OSS << "Expects(mRootBitset[v] == true);\n";
            OSS << "mRootBitset[v] = false;\n";
            OSS << "if (in_degree(v) == 0) {\n";
            {
                INDENT();
                OSS << "clear_edges(v);\n";
                OSS << "remove_vertex(v);\n";
            }
            OSS << "}\n";
        }
        OSS << "}\n";

        oss << "\n";
        OSS << "void " << name << "::mark_and_sweep(PmrMemoryResource* scratch) noexcept {\n";
        {
            INDENT();
            copyString(oss, space, R"(mColors.clear();
mColors.resize(num_vertices(), GraphColor::White);
auto colorMap = GraphImpl::makeGetterSetter(mColors);
// mark
for (const auto v : vertices()) {
    if (!mRootBitset[v]) {
        continue;
    }
    if (colorMap.get(v) == GraphColor::White) {
        depthFirstVisit(*this, v, DfsVisitor{}, colorMap, scratch);
    }
}
// sweep
for (const auto v : vertices()) {
    if (colorMap.get(v) == GraphColor::White) {
        clear_vertex(v);
        remove_vertex(v);
    }
}
)");
        }
        OSS << "}\n";
    }

    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateReserve_cpp() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    return oss.str();
}

std::pmr::string ObjectGraphBuilder::generateGraphSerialization_h(bool nvp) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    auto scratch = get_allocator().resource();

    auto cn = mStruct.mCurrentNamespace;
    auto name = cpp.getDependentName(cpp.mCurrentPath);

    bool bListVertexList = false;
    bool bContinuousOutEdgeList = false;
    if (!s.isVector()) {
        bListVertexList = true;
    }

    visit(
        overload(
            [&](Vector_) {
                bContinuousOutEdgeList = true;
            },
            [&](auto) {

            }),
        s.mOutEdgeListType);

    OSS << "\n";

    OSS << "STAR_CLASS_IMPLEMENTATION(" << name << ", object_serializable);\n";
    OSS << "STAR_CLASS_TRACKING(" << name << ", track_never);\n";

    OSS << "template <class Archive>\n";
    OSS << "void save(Archive& ar, const " << name << "& g, const uint32_t) {\n";
    {
        INDENT();
        OSS << "using Graph = " << name << ";\n";
        OSS << "using SizeType = uint32_t;\n";
        if (!bListVertexList) {
            OSS << "static_assert(std::is_same_v<Graph::Vertex, SizeType>);\n";
        }
        oss << "\n";
        OSS << "auto numVertices = g.num_vertices();\n";
        OSS << "auto numEdges = g.num_edges();\n";
        if (nvp) {
            OSS << "ar << make_nvp(\"numV\", numVertices);\n";
            OSS << "ar << make_nvp(\"numE\", numEdges);\n";
        } else {
            OSS << "ar << numVertices;\n";
            OSS << "ar << numEdges;\n";
        }

        if (s.isPolymorphic() && s.isVector()) {
            int count = 0;
            for (const auto& c : s.mPolymorphic.mConcepts) {
                if (c.isIntrusive())
                    continue;

                auto conceptID = locate(c.mValue, g);
                Expects(!c.mMemberName.empty());
                auto conceptName = std::string_view(c.mMemberName).substr(1, c.mMemberName.size() - 1);
                if (count++ == 0) {
                    oss << "\n";
                    OSS << "// PolymorphicGraph, ContinuousContainer\n";
                    OSS << "{\n";
                }
                INDENT();
                if (nvp) {
                    std::pmr::string varName("num", scratch);
                    varName.append(conceptName);
                    OSS << "auto " << varName << " = gsl::narrow_cast<SizeType>(g." << c.mMemberName << ".size()); \n";
                    OSS << "ar << make_nvp(\"" << varName << "\", " << varName << ");\n";
                } else {
                    OSS << "ar << gsl::narrow_cast<SizeType>(g." << c.mMemberName << ".size());\n";
                }
            }
            if (count) {
                OSS << "}\n";
            }
        }

        oss << "\n";
        if (bListVertexList) {
            OSS << "container::flat_map<Graph::Vertex, SizeType> indices;\n";
            OSS << "indices.reserve(numVertices);\n";
            OSS << "SizeType count = 0;\n";
        }

        if (bListVertexList || s.hasVertexProperty() || bContinuousOutEdgeList) {
            OSS << "{\n";
            {
                INDENT();

                OSS << "for (auto [iter, end] = g.vertices(); iter != end; ++iter) {\n";
                {
                    INDENT();
                    OSS << "const auto& v = *iter;\n";
                    if (bListVertexList) {
                        OSS << "indices[v] = count++;\n";
                    }
                    if (s.hasVertexProperty()) {
                        if (nvp) {
                            OSS << "ar << make_nvp(\"vp\", get(vertex_all, g, v));\n";
                        } else {
                            OSS << "ar << get(vertex_all, g, v);\n";
                        }
                    }

                    if (!s.mComponents.empty()) {
                        oss << "\n";
                        OSS << "// Vertex Components\n";
                    }
                    int id = 0;
                    for (const auto& c : s.mComponents) {
                        if (c.isVector()) {
                            if (nvp) {
                                OSS << "ar << make_nvp(\"vc" << id << "\", g." << c.mMemberName << "[v]);\n";
                            } else {
                                OSS << "ar << g." << c.mMemberName << "[v];\n";
                            }
                        } else {
                            if (nvp) {
                                OSS << "ar << make_nvp(\"vc" << id << "\", *g.mVertices[v]." << c.mMemberName << "Iter);\n";
                            } else {
                                OSS << "ar << *g.mVertices[v]." << c.mMemberName << "Iter;\n";
                            }
                        }
                        ++id;
                    }

                    if (s.mAddressable) {
                        oss << "\n";
                        OSS << "// AddressableGraph\n";
                        OSS << "auto range = g.parent_links(v);\n";
                        OSS << "if (range.first != range.second) {\n";
                        {
                            INDENT();
                            if (bListVertexList) {
                                if (nvp) {
                                    OSS << "ar << make_nvp(\"u\", indices.at(g.parent(*range.first)));\n";
                                } else {
                                    OSS << "ar << indices.at(g.parent(*range.first));\n";
                                }
                            } else {
                                if (nvp) {
                                    OSS << "auto u = g.parent(*range.first);\n";
                                    OSS << "ar << make_nvp(\"u\", u);\n";
                                } else {
                                    OSS << "ar << g.parent(*range.first);\n";
                                }
                            }
                        }
                        OSS << "} else {\n";
                        if (nvp) {
                            OSS << "    auto u = std::numeric_limits<SizeType>::max();\n";
                            OSS << "    ar << make_nvp(\"u\", u);\n";
                        } else {
                            OSS << "    ar << std::numeric_limits<SizeType>::max();\n";
                        }
                        OSS << "}\n";
                    }

                    if (s.isPolymorphic()) {
                        oss << "\n";
                        OSS << "// PolymorphicGraph\n";
                        if (nvp) {
                            OSS << "ar << make_nvp(\"t\", gsl::narrow_cast<uint32_t>(g.index(v)));\n";
                        } else {
                            OSS << "ar << gsl::narrow_cast<uint32_t>(g.index(v));\n";
                        }

                        OSS << "g.visit_vertex(v, [&](const auto& t) {\n";
                        {
                            INDENT();
                            if (nvp) {
                                OSS << "ar << make_nvp(\"val\", t);\n";
                            } else {
                                OSS << "ar << t;\n";
                            }
                        }
                        OSS << "});\n";
                    }

                    if (bContinuousOutEdgeList) {
                        oss << "\n";
                        OSS << "// continuous in/out edge list\n";
                        if (nvp) {
                            OSS << "auto outD = g.out_degree(v);\n";
                            OSS << "ar << make_nvp(\"outD\", outD);\n";
                        } else {
                            OSS << "ar << g.out_degree(v);\n";
                        }
                        if (s.mBidirectional) {
                            if (nvp) {
                                OSS << "auto inD = g.in_degree(v);\n";
                                OSS << "ar << make_nvp(\"inD\", inD);\n";
                            } else {
                                OSS << "ar << g.in_degree(v);\n";
                            }
                        }
                    }
                }
                OSS << "}\n";
            }
            OSS << "}\n";
        }

        if (s.mEdgeList && s.mMutableGraphEdge) {
            oss << "\n";
            OSS << "for (auto [iter, end] = g.edges(); iter != end; ++iter) {\n";
            {
                INDENT();
                OSS << "auto e = *iter;\n";
                if (bListVertexList) {
                    if (nvp) {
                        OSS << "ar << make_nvp(\"u\", indices.at(g.source(e)));\n";
                        OSS << "ar << make_nvp(\"v\", indices.at(g.target(e)));\n";
                    } else {
                        OSS << "ar << indices.at(g.source(e));\n";
                        OSS << "ar << indices.at(g.target(e));\n";
                    }
                } else {
                    if (nvp) {
                        OSS << "auto u = g.source(e);\n";
                        OSS << "auto v = g.target(e);\n";
                        OSS << "ar << make_nvp(\"u\", u);\n";
                        OSS << "ar << make_nvp(\"v\", v);\n";
                    } else {
                        OSS << "ar << g.source(e);\n";
                        OSS << "ar << g.target(e);\n";
                    }
                }
                if (s.hasEdgeProperty()) {
                    if (nvp) {
                        OSS << "ar << make_nvp(\"ep\", g.get(edge_all, e));\n";
                    } else {
                        OSS << "ar << g.get(edge_all, e);\n";
                    }
                }
            }
            OSS << "}\n";
        }
        for (int count = 0; const auto& m : s.mMembers) {
            const auto& member = m.mMemberName;
            auto memberID = locate(m.mTypePath, g);
            if (!g.isSerializable(memberID, m))
                continue;

            if (count++ == 0)
                oss << "\n";

            if (nvp) {
                OSS << "ar << make_nvp(\"" << getMemberName(member, scratch)
                    << "\", g." << member << ");\n";
            } else {
                OSS << "ar << g." << member << ";\n";
            }
        }
    }
    OSS << "}\n";

    oss << "\n";
    OSS << "template <class Archive>\n";
    OSS << "void load(Archive& ar, " << name << "& g, const uint32_t) {\n";
    {
        INDENT();
        OSS << "using Graph = " << name << ";\n";
        OSS << "using SizeType = uint32_t;\n";
        OSS << "using EdgeSizeType = uint32_t;\n";
        if (!bListVertexList) {
            OSS << "static_assert(std::is_same_v<Graph::Vertex, SizeType>);\n";
        }
        oss << "\n";
        OSS << "SizeType numVertices = 0;\n";
        OSS << "EdgeSizeType numEdges = 0;\n";
        if (nvp) {
            OSS << "ar >> make_nvp(\"numV\", numVertices);\n";
            OSS << "ar >> make_nvp(\"numE\", numEdges);\n";
        } else {
            OSS << "ar >> numVertices;\n";
            OSS << "ar >> numEdges;\n";
        }

        if (s.hasReserve()) {
            oss << "\n";
            OSS << "g.reserve(numVertices);\n";
        }

        if (s.isPolymorphic() && s.isContinuousContainer()) {
            int count = 0;
            for (const auto& c : s.mPolymorphic.mConcepts) {
                if (c.isIntrusive())
                    continue;

                if (count++ == 0) {
                    oss << "\n";
                    OSS << "// PolymorphicGraph, ContinuousContainer\n";
                    OSS << "{\n";
                }

                INDENT();
                std::pmr::string varName("num", scratch);
                varName.append(std::string_view(c.mMemberName).substr(1));
                OSS << "SizeType " << varName << ";\n";
                if (nvp) {
                    OSS << "ar >> make_nvp(\"" << varName << "\", " << varName << ");\n";
                } else {
                    OSS << "ar >> " << varName << ";\n";
                }
                OSS << "g." << c.mMemberName << ".reserve(" << varName << ");\n";
            }
            if (count) {
                OSS << "}\n";
            }
        }

        oss << "\n";
        if (bListVertexList) {
            OSS << "std::vector<Graph::Vertex> verts(numVertices, Graph::null_vertex());\n";
            OSS << "SizeType i = 0;\n";
        }
        OSS << "while (numVertices-- > 0) {\n";
        {
            INDENT();
            if (s.hasVertexProperty()) {
                auto vpID = locate(s.mVertexProperty, g);
                OSS << "Graph::vertex_property_type vp{";
                if (g.isPmr(vpID)) {
                    oss << "g.get_allocator().resource()";
                }
                oss << "};\n";
                if (nvp) {
                    OSS << "ar >> make_nvp(\"vp\", vp);\n";
                } else {
                    OSS << "ar >> vp;\n";
                }
            }

            if (!s.mComponents.empty()) {
                Expects(s.isVector());

                int id = 0;
                for (const auto& c : s.mComponents) {
                    const auto componentID = locate(c.mValuePath, g);
                    auto componentName = cpp.getDependentName(c.mValuePath);
                    OSS << componentName << " vc" << id << "{";
                    if (g.isPmr(componentID)) {
                        oss << " ";
                        if (g.isPair(componentID)) {
                            Expects(holds_tag<Instance_>(componentID, g));
                            const auto& inst = get<Instance>(componentID, g);
                            Expects(inst.mParameters.size() == 2);
                            auto firstID = locate(inst.mParameters[0].mTypePath, g);
                            auto secondID = locate(inst.mParameters[1].mTypePath, g);
                            oss << "std::piecewise_construct";
                            if (g.isPmr(firstID)) {
                                oss << ", std::forward_as_tuple(g.get_allocator().resource())";
                            } else {
                                oss << ", std::forward_as_tuple()";
                            }
                            if (g.isPmr(secondID)) {
                                oss << ", std::forward_as_tuple(g.get_allocator().resource())";
                            } else {
                                oss << ", std::forward_as_tuple()";
                            }
                        } else {
                            oss << "g.get_allocator().resource()";
                        }
                        oss << " ";
                    }
                    oss << "};\n";
                    if (nvp) {
                        OSS << "ar >> make_nvp(\"vc" << id << "\", vc" << id << ");\n";
                    } else {
                        OSS << "ar >> vc" << id << ";\n";
                    }
                    ++id;
                }
            }

            if (s.mAddressable) {
                oss << "\n";
                OSS << "// AddressableGraph\n";
                OSS << "SizeType u0{};\n";
                if (nvp) {
                    OSS << "ar >> boost::make_nvp(\"u\", u0);\n";
                } else {
                    OSS << "ar >> u0;\n";
                }
                OSS << "Graph::Vertex u = (u0 == std::numeric_limits<SizeType>::max() ? Graph::null_vertex() : ";
                if (bListVertexList) {
                    oss << "verts.at(u0)";
                } else {
                    oss << "u0";
                }
                oss << ");\n";
            }

            auto addVertexImpl = [&](const PolymorphicPair* c) {
                std::ostringstream oss;
                std::string space;
                {
                    int count = 0;
                    int id = 0;
                    if (s.isPolymorphic()) {
                        OSS << "v = g.add_vertex(";
                    } else {
                        OSS << "auto v = g.add_vertex(";
                    }
                    if (s.isPolymorphic()) {
                        oss << (count++ == 0 ? "" : ", ") << "std::move(val)";
                    }
                    if (s.hasVertexProperty()) {
                        oss << (count++ == 0 ? "" : ", ") << "std::move(vp)";
                    }
                    for (const auto& c : s.mComponents) {
                        oss << (count++ == 0 ? "" : ", ") << "std::move(vc" << id << ")";
                        ++id;
                    }
                    if (s.mAddressable) {
                        oss << ", u";
                    }
                    oss << ");\n";
                }
                {
                    if (s.isPolymorphic()) {
                        if (c->isIntrusive()) {
                            OSS << "ar.reset_object_address(&std::get<\n";
                            OSS << "    " << cpp.mProjectName << "::"
                                << handleElemType(*c, cn) << "\n";
                            OSS << ">(g.mVertices[v].mHandle).mValue, &val);\n";
                        } else {
                            OSS << "ar.reset_object_address(&*g."
                                << c->mMemberName << ".rbegin(), &val);\n";
                        }
                    }
                }
                if (bListVertexList) {
                    OSS << "verts[i++] = v;\n";
                }

                return oss.str();
            };

            if (s.isPolymorphic()) {
                oss << "\n";
                OSS << "// PolymorphicGraph\n";
                OSS << "uint32_t t = 0xFFFFFFFF;\n";
                if (nvp) {
                    OSS << "ar >> boost::make_nvp(\"t\", t);\n";
                } else {
                    OSS << "ar >> t;\n";
                }
                oss << "\n";
                OSS << "Graph::Vertex v = Graph::null_vertex();\n";
                OSS << "switch (t) {\n";
                {
                    for (uint32_t id = 0; const auto& c : s.mPolymorphic.mConcepts) {
                        OSS << "case " << id++ << ": {\n";
                        {
                            INDENT();
                            auto valueID = locate(c.mValue, g);
                            OSS << cpp.getDependentName(c.mValue) << " val{";
                            if (g.isPmr(valueID)) {
                                oss << " g." << c.mMemberName << ".get_allocator().resource() ";
                            }
                            oss << "};\n";
                            if (nvp) {
                                OSS << "ar >> boost::make_nvp(\"val\", val);\n";
                            } else {
                                OSS << "ar >> val;\n";
                            }
                            copyString(oss, space, addVertexImpl(&c));
                        }
                        OSS << "} break;\n";
                    }
                }
                OSS << "default:\n";
                {
                    INDENT();
                    OSS << "throw std::runtime_error(\"invalid variant index\");\n";
                }
                OSS << "}\n";
            } else {
                copyString(oss, space, addVertexImpl(nullptr));
            }

            if (s.hasVertexProperty()) {
                oss << "\n";
                OSS << "ar.reset_object_address(&g[v], &vp);\n";
            }

            if (!s.mComponents.empty()) {
                oss << "\n";
                int id = 0;
                for (const auto& c : s.mComponents) {
                    const auto& member = c.mMemberName;
                    if (c.isVector()) {
                        OSS << "ar.reset_object_address(&g." << member << "[v], &vc" << id << ");\n";
                    } else {
                        OSS << "ar.reset_object_address(&*g.mVertices[v]." << member << "Iter, &vc" << id << ");\n";
                    }
                    ++id;
                }
            }

            if (bContinuousOutEdgeList) {
                oss << "\n";
                OSS << "SizeType numOutDegree = 0;\n";
                if (nvp) {
                    OSS << "ar >> make_nvp(\"outD\", numOutDegree);\n";
                } else {
                    OSS << "ar >> numOutDegree;\n";
                }
                OSS << "g.mVertices[v].mOutEdges.reserve(numOutDegree);\n";
                if (s.mBidirectional) {
                    oss << "\n";
                    OSS << "SizeType numInDegree = 0;\n";
                    if (nvp) {
                        OSS << "ar >> make_nvp(\"inD\", numInDegree);\n";
                    } else {
                        OSS << "ar >> numInDegree;\n";
                    }
                    OSS << "g.mVertices[v].mInEdges.reserve(numInDegree);\n";
                }
            }
        }
        OSS << "}\n";

        if (s.mEdgeList && s.mMutableGraphEdge) {
            bool skip = false;
            if (s.mAddressable) {
                if (s.isAliasGraph()) {
                    skip = true;
                }
            }
            oss << "\n";
            OSS << "while (numEdges-- > 0) {\n";
            {
                INDENT();
                if (bListVertexList) {
                    OSS << "SizeType u = 0;\n";
                    OSS << "SizeType v = 0;\n";
                } else {
                    OSS << "Graph::Vertex u = Graph::null_vertex();\n";
                    OSS << "Graph::Vertex v = Graph::null_vertex();\n";
                }
                if (nvp) {
                    OSS << "ar >> make_nvp(\"u\", u);\n";
                    OSS << "ar >> make_nvp(\"v\", v);\n";
                } else {
                    OSS << "ar >> u;\n";
                    OSS << "ar >> v;\n";
                }

                if (skip) {
                    if (bListVertexList) {
                        OSS << "Ensures(g.edge(verts[u], verts[v]).second);\n";
                    } else {
                        OSS << "Ensures(g.edge(u, v).second);\n";
                    }
                } else {
                    if (bListVertexList) {
                        OSS << "auto [e, inserted] = g.add_edge(verts[u], verts[v]);\n";
                    } else {
                        OSS << "auto [e, inserted] = g.add_edge(u, v);\n";
                    }
                    OSS << "Ensures(inserted);\n";
                }

                if (s.hasEdgeProperty()) {
                    if (nvp) {
                        OSS << "ar >> make_nvp(\"ep\", g.get(edge_all, e));\n";
                    } else {
                        OSS << "ar >> g.get(edge_all, e);\n";
                    }
                }
            }
            OSS << "}\n";
        }

        if (!s.mMembers.empty()) {
            for (int count = 0; const auto& m : s.mMembers) {
                const auto& member = m.mMemberName;
                auto memberID = locate(m.mTypePath, g);
                if (!g.isSerializable(memberID, m))
                    continue;
                if (count++ == 0)
                    oss << "\n";
                if (nvp) {
                    OSS << "ar >> make_nvp(\"" << getMemberName(member, scratch) << "\", g." << member << ");\n";
                } else {
                    OSS << "ar >> g." << member << ";\n";
                }
            }
        }
    }
    OSS << "}\n";

    oss << "\n";
    OSS << "template <class Archive>\n";
    OSS << "void serialize(Archive& ar, " << name << "& g, const uint32_t version) {\n";
    OSS << "    boost::serialization::split_free(ar, g, version);\n";
    OSS << "}\n";

    return oss.str();
}

}
