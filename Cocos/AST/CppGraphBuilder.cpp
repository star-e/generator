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

void CppGraphBuilder::prepareNamespace(std::string_view& ns) const {
    if (ns == ".")
        ns = mStruct.mCurrentPath;
}

std::pmr::string CppGraphBuilder::nullVertexValue() const {
    const auto& s = *mGraph;
    if (s.isVector()) {
        return std::pmr::string("std::numeric_limits<vertex_descriptor>::max()",
            get_allocator());
    } else {
        return std::pmr::string("nullptr", get_allocator());
    }
}

std::pmr::string CppGraphBuilder::vertexDescType() const {
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

std::pmr::string CppGraphBuilder::edgeDescType() const {
    const auto& s = *mGraph;
    std::pmr::string type(get_allocator());

    if (s.mEdgeList) {
        if (s.isDirectedOnly()) {
            if (s.hasEdgeProperty()) {
                type = "impl::EdgeDescriptorWithProperty<directed_category, vertex_descriptor>";
            } else {
                type = "impl::EdgeDescriptor<directed_category, vertex_descriptor>";
            }
        } else if (s.mUndirected) {
            type = "impl::EdgeDescriptorWithProperty<directed_category, vertex_descriptor>";
        } else if (s.mBidirectional) {
            if (s.hasEdgeProperty()) {
                type = "impl::EdgeDescriptorWithProperty<directed_category, vertex_descriptor>";
            } else {
                type = "impl::EdgeDescriptor<directed_category, vertex_descriptor>";
            }
        }
    } else {
        type = "impl::EdgeDescriptor<directed_category, vertex_descriptor>";
    }
    return type;
}

CppGraphBuilder::CppGraphBuilder(const SyntaxGraph* syntaxGraph,
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

std::pmr::string CppGraphBuilder::graphType(std::string_view ns) const {
    prepareNamespace(ns);
    const auto& g = *mStruct.mSyntaxGraph;
    const auto vertID = mStruct.mCurrentVertex;
    auto scratch = get_allocator().resource();
    auto name = g.getDependentName(ns, vertID, scratch, scratch);
    return getCppPath(name, scratch);
}

std::pmr::string CppGraphBuilder::vertexDesc(std::string_view ns) const {
    prepareNamespace(ns);
    const auto& g = *mStruct.mSyntaxGraph;
    const auto vertID = mStruct.mCurrentVertex;
    auto scratch = get_allocator().resource();
    auto name = g.getDependentName(ns, vertID, scratch, scratch);
    if (name.empty()) {
        name.append("vertex_descriptor");
    } else {
        name.append("::vertex_descriptor");
    }
    return getCppPath(name, scratch);
}

std::pmr::string CppGraphBuilder::edgeDesc(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());

    return oss.str();
}

std::pmr::string CppGraphBuilder::vertexType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    auto scratch = get_allocator().resource();
    auto name = getCppPath(g.getDependentName(
        ns, mStruct.mCurrentVertex, scratch, scratch), scratch);
    if (name.empty()) {
        oss << "Vertex";
    } else {
        oss << name << "::Vertex";
    }
    return oss.str();
}

std::pmr::string CppGraphBuilder::edgeType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    if (s.hasEdgeProperty()) {
        auto epID = locate(s.mEdgeProperty, g);
        if (g.isPmr(epID)) {
            oss << "impl::PmrListEdge<vertex_descriptor, " << edgePropertyType(ns) << ">";
        } else {
            oss << "impl::ListEdge<vertex_descriptor, " << edgePropertyType(ns) << ">";
        }
    } else {
        oss << "impl::ListEdge<vertex_descriptor>";
    }
    return oss.str();
}

std::pmr::string CppGraphBuilder::vertexPropertyType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto vpID = locate(s.mVertexProperty, g);

    return getCppPath(g.getDependentName(ns, vpID, scratch, scratch), scratch);
}

std::pmr::string CppGraphBuilder::edgePropertyType(std::string_view ns) const {
    prepareNamespace(ns);
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto epID = locate(s.mEdgeProperty, g);
    return getCppPath(g.getDependentName(ns, epID, scratch, scratch), scratch);
}

std::pmr::string CppGraphBuilder::objectListType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    auto vecID = locate(s.mVertexListPath, g);
    oss << getCppPath(g.getDependentName(ns, vecID, scratch, scratch), scratch) << "<Object>";
    return oss.str();
}

std::pmr::string CppGraphBuilder::vertexListType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    auto vecID = locate(s.mVertexListPath, g);
    oss << getCppPath(g.getDependentName(ns, vecID, scratch, scratch), scratch) << "<Vertex>";
    return oss.str();
}

std::pmr::string CppGraphBuilder::edgeListType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto listID = locate(s.mEdgeListPath, g);
    oss << getCppPath(g.getDependentName(ns, listID, scratch, scratch), scratch);
    oss << "<";

    const auto vertID = mStruct.mCurrentVertex;
    auto name = g.getDependentName(ns, vertID, scratch, scratch);
    if (name.empty()) {
        name.append("edge_type");
    } else {
        name.append("::edge_type");
    }
    oss << name;
    oss << ">";
    return oss.str();
}

std::pmr::string CppGraphBuilder::componentContainerType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    const auto& g = *mStruct.mSyntaxGraph;
    auto bPmr = g.isPmr(mStruct.mCurrentVertex);
    if (s.isVector()) {
        if (bPmr) {
            oss << "/ccstd/pmr/vector";
        } else {
            oss << "/ccstd/vector";
        }
    } else {
        if (bPmr) {
            oss << "/boost/container/pmr/list";
        } else {
            oss << "/boost/container/list";
        }
    }
    return oss.str();
}

std::pmr::string CppGraphBuilder::outEdgeListType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto listID = locate(s.mOutEdgeListPath, g);
    oss << getCppPath(g.getDependentName(ns, listID, scratch, scratch), scratch);
    oss << "<OutEdge>";
    return oss.str();
}

std::pmr::string CppGraphBuilder::outEdgeType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto name = graphType(ns);

    if (s.hasEdgeProperty()) {
        auto edgePropType = edgePropertyType(ns);
        if (s.isDirectedOnly()) {
            oss << "impl::StoredEdgeWithProperty<\n";
            oss << "    " << vertexDesc(ns) << ",\n";
            oss << "    " << edgeListType(ns) << ">";
        } else {
            if (s.isEdgeListVector()) {
                oss << "impl::StoredEdgeWithRandomAccessEdgeIter<\n";
                oss << "    " << vertexDesc(ns) << ",\n";
                oss << "    " << edgeListType(ns) << ",\n";
                oss << "    " << edgePropertyType(ns) << ">";
            } else {
                oss << "impl::StoredEdgeWithEdgeIter<\n";
                oss << "    " << vertexDesc(ns) << ",\n";
                oss << "    " << edgeListType(ns) << "::iterator,\n";
                oss << "    " << edgePropertyType(ns) << ">";
            }
        }
    } else {
        if (s.needEdgeList()) {
            if (s.isEdgeListVector()) {
                oss << "impl::StoredEdgeWithRandomAccessEdgeIter<\n";
                oss << "    " << vertexDesc(ns) << ",\n";
                oss << "    " << edgeListType(ns) << ">";
            } else {
                oss << "impl::StoredEdgeWithEdgeIter<\n";
                oss << "    " << vertexDesc(ns) << ",\n";
                oss << "    " << edgeListType(ns) << "::iterator>";
            }
        } else {
            oss << "impl::StoredEdge<" << vertexDesc(ns) << ">";
        }
    }

    return oss.str();
}

std::pmr::string CppGraphBuilder::outIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (s.hasEdgeProperty() || s.mUndirected) {
        oss << "impl::OutPropertyEdgeIter<\n";
    } else {
        oss << "impl::OutEdgeIter<\n";
    }
    oss << "    " << outEdgeListType() << "::iterator,\n";
    oss << "    vertex_descriptor, edge_descriptor, "
        << s.mDifferenceType << ">";
    return oss.str();
}

std::pmr::string CppGraphBuilder::inEdgeListType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto listID = locate(s.mOutEdgeListPath, g);
    oss << getCppPath(g.getDependentName(ns, listID, scratch, scratch), scratch);
    oss << "<InEdge>";
    return oss.str();
}

std::pmr::string CppGraphBuilder::inEdgeType(std::string_view ns) const {
    prepareNamespace(ns);
    return outEdgeType(ns);
}

std::pmr::string CppGraphBuilder::inIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;

    if (s.hasEdgeProperty() || s.mUndirected) {
        oss << "impl::InPropertyEdgeIter<\n";
    } else {
        oss << "impl::InEdgeIter<\n";
    }
    oss << "    " << inEdgeListType() << "::iterator,\n";
    oss << "    vertex_descriptor, edge_descriptor, "
        << s.mDifferenceType << ">";

    return oss.str();
}

std::pmr::string CppGraphBuilder::edgeIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());

    return oss.str();
}

std::pmr::string CppGraphBuilder::childListType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto listID = locate(s.mOutEdgeListPath, g);
    oss << getCppPath(g.getDependentName(ns, listID, scratch, scratch), scratch);
    if (s.mAliasGraph) {
        oss << "<OutEdge>";
    } else {
        oss << "<ChildEdge>";
    }
    return oss.str();
}

std::pmr::string CppGraphBuilder::childEdgeType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (s.mAliasGraph) {
        oss << "OutEdge";
    } else {
        oss << "impl::StoredEdge<vertex_descriptor>";
    }
    return oss.str();
}

std::pmr::string CppGraphBuilder::childIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;

    oss << "impl::OutEdgeIter<\n";
    oss << "    " << childListType() << "::iterator,\n";
    oss << "    vertex_descriptor, ownership_descriptor, " << s.mDifferenceType << ">";

    return oss.str();
}

std::pmr::string CppGraphBuilder::parentEdgeType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (s.mAliasGraph) {
        oss << "InEdge";
    } else {
        oss << "impl::StoredEdge<vertex_descriptor>";
    }
    return oss.str();
}

std::pmr::string CppGraphBuilder::parentListType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto listID = locate(s.mOutEdgeListPath, g);
    oss << getCppPath(g.getDependentName(ns, listID, scratch, scratch), scratch);
    if (s.mAliasGraph) {
        oss << "<InEdge>";
    } else {
        oss << "<ParentEdge>";
    }
    return oss.str();
}

std::pmr::string CppGraphBuilder::parentIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    oss << "impl::InEdgeIter<\n";
    oss << "    " << parentListType() << "::iterator,\n";
    oss << "    vertex_descriptor, ownership_descriptor, " << s.mDifferenceType << ">";

    return oss.str();
}

std::pmr::string CppGraphBuilder::referenceIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;

    if (s.mAliasGraph) {
        oss << "impl::DirectedEdgeIterator<vertex_iterator, children_iterator, "
            << graphType(mStruct.mCurrentNamespace) << ">";
    } else {
        oss << "impl::OwnershipIterator<vertex_iterator, children_iterator, "
            << graphType(mStruct.mCurrentNamespace) << ">";
    }
    return oss.str();
}

std::pmr::string CppGraphBuilder::vertexIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (s.isVector()) {
        oss << "boost::integer_range<vertex_descriptor>::iterator";
    } else {
        oss << "impl::VertexIter<" << vertexListType()
            << "::iterator, vertex_descriptor, "
            << s.mDifferenceType << ">";
    }
    return oss.str();
}

std::pmr::string CppGraphBuilder::adjIterType() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    std::pmr::string space(get_allocator());

    return oss.str();
}

std::pmr::string CppGraphBuilder::tagType(std::string_view ns) const {
    const auto& cpp = mStruct;
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    if (mStruct.mBoost) {
        oss << "ccstd::variant<";
    } else {
        oss << "std::variant<";
    }
    int count = 0;
    for (const auto& c : s.mPolymorphic.mConcepts) {
        if (count++)
            oss << ", ";
        auto conceptID = locate(c.mTag, g);
        oss << cpp.getDependentName(conceptID);
    }
    oss << ">";
    return oss.str();
}

std::pmr::string CppGraphBuilder::valueType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    if (mStruct.mBoost) {
        oss << "ccstd::variant<";
    } else {
        oss << "std::variant<";
    }
    int count = 0;
    for (const auto& c : s.mPolymorphic.mConcepts) {
        if (count++)
            oss << ", ";
        auto conceptID = locate(c.mValue, g);
        oss << getCppPath(g.getDependentName(ns, conceptID, scratch, scratch), scratch);
        oss << "*";
    }
    oss << ">";
    return oss.str();
}

std::pmr::string CppGraphBuilder::constValueType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    if (mStruct.mBoost) {
        oss << "ccstd::variant<";
    } else {
        oss << "std::variant<";
    }
    int count = 0;
    for (const auto& c : s.mPolymorphic.mConcepts) {
        if (count++)
            oss << ", ";
        oss << "const ";
        auto conceptID = locate(c.mValue, g);
        oss << getCppPath(g.getDependentName(ns, conceptID, scratch, scratch), scratch);
        oss << "*";
    }
    oss << ">";
    return oss.str();
}

std::pmr::string CppGraphBuilder::handleElemType(const PolymorphicPair& pair,
    std::string_view ns, bool bSkipName) const {
    const auto& cpp = mStruct;
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    auto tagID = locate(pair.mTag, g);
    auto valueID = locate(pair.mValue, g);
    
    auto tagName = cpp.getDependentName(tagID);
    auto valueName = getCppPath(g.getDependentName(ns, valueID, scratch, scratch), scratch);

    oss << "impl::ValueHandle<" << tagName << ", ";
    if (pair.mMemberName.empty()) {
        oss << valueName;
    } else {
        if (pair.isVector()) {
            auto name = graphType(ns);
            if (bSkipName || name.empty()) {
                oss << "vertex_descriptor";
            } else {
                oss << name << "::vertex_descriptor";
            }
        } else {
            auto vecID = locate(pair.mContainerPath, g);
            auto vecType = getCppPath(g.getDependentName(ns, vecID, scratch, scratch), scratch);
            oss << vecType << "<" << valueName << ">::iterator";
        }
    }
    oss << ">";
    return oss.str();
}

std::pmr::string CppGraphBuilder::handleType(std::string_view ns) const {
    prepareNamespace(ns);
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    if (mStruct.mBoost) {
        oss << "ccstd::variant<\n";
    } else {
        oss << "std::variant<\n";
    }
    {
        INDENT();
        int count = 0;
        for (const auto& c : s.mPolymorphic.mConcepts) {
            if (count++) {
                oss << ",\n";
            }
            auto conceptID = locate(c.mValue, g);
            oss << handleElemType(c, ns);
        }
    }
    oss << ">\n";
    return oss.str();
}

std::pmr::string CppGraphBuilder::vertexPropertyMapName(bool bConst) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    auto cn = mStruct.mCurrentNamespace;

    if (cn.empty() || !cn.starts_with("/" + mStruct.mProjectName)) {
        OSS << mStruct.mProjectName << "::render::";
    }
    if (s.isVector()) {
        oss << "impl::VectorVertexBundlePropertyMap<\n";
    } else {
        oss << "impl::PointerVertexBundlePropertyMap<\n";
    }
    {
        INDENT();
        bool isString = false;
        bool isU8String = false;
        if (s.mVertexProperty == "/std/string"
            || s.mVertexProperty == "/std/pmr/string") {
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

std::pmr::string CppGraphBuilder::vertexPropertyMapMemberName(bool bConst) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    auto cn = mStruct.mCurrentNamespace;

    if (cn.empty() || !cn.starts_with(mStruct.mProjectName + "::")) {
        OSS << mStruct.mProjectName << "::render::";
    }

    if (s.isVector()) {
        oss << "impl::VectorVertexBundleMemberPropertyMap<\n";
    } else {
        oss << "impl::PointerVertexBundleMemberPropertyMap<\n";
    }

    {
        INDENT();
        bool isString = false;
        bool isU8String = false;
        if (s.mVertexProperty == "/std/string"
            || s.mVertexProperty == "/std/pmr/string") {
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

std::pmr::string CppGraphBuilder::edgePropertyMapName(bool bConst) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    auto cn = mStruct.mCurrentNamespace;

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (cn.empty() || cn.substr(0, 6) != mStruct.mProjectName + "::") {
        OSS << mStruct.mProjectName << "::render::";
    }
    oss << "impl::EdgeBundlePropertyMap<\n";
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

std::pmr::string CppGraphBuilder::edgePropertyMapMemberName(bool bConst, std::string_view stringPath) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    auto cn = mStruct.mCurrentNamespace;

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (cn.empty() || cn.substr(0, 6) != mStruct.mProjectName + "::") {
        OSS << mStruct.mProjectName << "::render::";
    }
    oss << "impl::EdgeBundleMemberPropertyMap<\n";
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

std::pmr::string CppGraphBuilder::vertexComponentMapName(const Component& c, bool bConst) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    const auto& cpp = mStruct;
    auto scratch = get_allocator().resource();
    std::pmr::string space(get_allocator());
    auto cn = mStruct.mCurrentNamespace;

    auto name = cpp.getDependentName(cpp.mCurrentPath);

    if (cn.empty() || cn.substr(0, 6) != cpp.mProjectName + "::") {
        OSS << cpp.mProjectName << "::render::";
    }
    if (s.isVector()) {
        if (c.isVector()) {
            oss << "impl::VectorVertexComponentPropertyMap<\n";
        } else {
            oss << "impl::VectorVertexIteratorComponentPropertyMap<\n";
        }
    } else {
        Expects(false);
    }

    bool isString = false;
    bool isU8String = false;
    bool bPmr = false;
    if (c.mValuePath == "/ccstd/string"
        || c.mValuePath == "/ccstd/pmr/string") {
        isString = true;
        if (c.mValuePath == "/ccstd/pmr/string")
            bPmr = true;
    } else if (c.mValuePath == "/ccstd/u8string"
        || c.mValuePath == "/ccstd/pmr/u8string") {
        isString = true;
        isU8String = true;
        if (c.mValuePath == "/ccstd/pmr/u8string")
            bPmr = true;
    } else if (c.mValuePath == "/ccstd/pmr/string") {
        isString = true;
        isU8String = false;
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
                OSS << container << "<" << component << ">::iterator " << name << "::Vertex::*,\n";
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
        OSS << (bConst ? "const " : "") << component << "&";
    }
    oss << ">";
    return oss.str();
}

std::pmr::string CppGraphBuilder::vertexComponentMapMemberName(
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
        OSS << cpp.mProjectName << "::render::";
    }
    if (s.isVector()) {
        if (c.isVector()) {
            oss << "impl::VectorVertexComponentMemberPropertyMap<\n";
        } else {
            oss << "impl::VectorVertexIteratorComponentMemberPropertyMap<\n";
        }
    } else {
        Expects(false);
    }

    bool isString = false;
    bool isU8String = false;
    bool bPmr = false;

    if (stringPath == "std::string"
        || stringPath == "std::pmr::string"
        || stringPath == "ccstd::pmr::string") {
        isString = true;
        if (stringPath == "std::pmr::string" || stringPath == "ccstd::pmr::string")
            bPmr = true;
    } else if (stringPath == "std::u8string"
        || stringPath == "std::pmr::u8string"
        || stringPath == "ccstd::u8string"
        || stringPath == "ccstd::pmr::u8string") {
        isString = true;
        isU8String = true;
        if (stringPath == "std::pmr::u8string" || stringPath == "ccstd::pmr::u8string")
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
                OSS << container << "<" << component << ">::iterator " << name << "::Vertex::*,\n";
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
                    OSS << (bConst ? "const " : "") << "ccstd::pmr::u8string&,\n";
                    OSS << (bConst ? "const " : "") << "ccstd::pmr::u8string " << component << "::*";
                } else {
                    OSS << (bConst ? "const " : "") << "std::u8string&,\n";
                    OSS << (bConst ? "const " : "") << "std::u8string " << component << "::*";
                }
            } else {
                if (bPmr) {
                    OSS << (bConst ? "const " : "") << "ccstd::pmr::string&,\n";
                    OSS << (bConst ? "const " : "") << "ccstd::pmr::string " << component << "::*";
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

std::pmr::string CppGraphBuilder::generateVertexType(std::string_view name, bool layer) const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;
    auto scratch = get_allocator().resource();

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateAllocator_h() {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& cpp = mStruct;
    const auto vertID = mStruct.mCurrentVertex;
    const auto& api = mStruct.mAPI;

    if (g.isPmr(vertID)) {
        if (mCount++)
            oss << "\n";
        OSS << "using allocator_type = boost::container::pmr::polymorphic_allocator<char>;\n";
        if (mDLL) {
            OSS << api << "_API " << cpp.generateGetAllocatorSignature(true) << "; // NOLINT\n";
        } else {
            OSS << cpp.generateGetAllocatorSignature(true) << " { // NOLINT\n";
            {
                INDENT();
                copyString(oss, space, cpp.generateGetAllocatorBody());
            }
            OSS << "}\n";
        }
        copyString(oss, space, R"(
boost::container::pmr::memory_resource* resource() const noexcept {
    return get_allocator().resource();
})");
    }

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateConstructors_h() {
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

std::pmr::string CppGraphBuilder::generateGraph_h() {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& cpp = mStruct;
    const auto vertID = mStruct.mCurrentVertex;
    const auto& api = mStruct.mAPI;
    const auto& s = *mGraph;
    if (mCount++) {
        oss << "\n";
    }

    OSS << "// Graph\n";
    if (s.mUndirected) {
        OSS << "using directed_category      = boost::undirected_tag;\n";
    } else {
        if (s.mBidirectional) {
            OSS << "using directed_category      = boost::bidirectional_tag;\n";
        } else {
            OSS << "using directed_category      = boost::directed_tag;\n";
        }
    }

    OSS << "using vertex_descriptor      = " << vertexDescType() << ";\n";
    OSS << "using edge_descriptor        = " << edgeDescType() << ";\n";

    // edge_parallel_category
    OSS << "using edge_parallel_category = boost::allow_parallel_edge_tag;\n";

    // traversal_category
    {
        int count = 0;
        OSS << "struct traversal_category // NOLINT";
        {
            if (s.mIncidence) {
                if (count++) {
                    oss << ",\n";
                    OSS << "  ";
                } else {
                    oss << "\n";
                    OSS << ": ";
                }
                oss << "virtual boost::incidence_graph_tag";
                if (s.mBidirectional) {
                    oss << ",\n";
                    OSS << "  virtual boost::bidirectional_graph_tag";
                }
            }

            if (s.mAdjacency) {
                if (count++) {
                    oss << ",\n";
                    OSS << "  ";
                } else {
                    oss << "\n";
                    OSS << ": ";
                }
                oss << "virtual boost::adjacency_graph_tag";
            }

            if (s.mVertexList) {
                if (count++) {
                    oss << ",\n";
                    OSS << "  ";
                } else {
                    oss << "\n";
                    OSS << ": ";
                }
                oss << "virtual boost::vertex_list_graph_tag";
            }

            if (s.mEdgeList) {
                if (count++) {
                    oss << ",\n";
                    OSS << "  ";
                } else {
                    oss << "\n";
                    OSS << ": ";
                }
                oss << "virtual boost::edge_list_graph_tag";
            }
        }
        oss << " {};\n";
    }

    if (true) {
        oss << "\n";
        OSS << "constexpr static vertex_descriptor null_vertex() noexcept { // NOLINT\n";
        OSS << "    return " << nullVertexValue() << ";\n";
        OSS << "}\n";
    }

    if (s.mVertexList) {
        if (!s.isVector()) {
            OSS << "struct Vertex;\n";
        }
    }
    if (s.needEdgeList()) {
        OSS << "using edge_type = " << edgeType() << ";\n";
    }

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateIncidenceGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (!s.mIncidence)
        return oss.str();

    std::pmr::string space(get_allocator());

    oss << "\n";
    OSS << "// IncidenceGraph\n";
    {
        auto content = outEdgeType();
        if (content.find_first_of('\n') != content.npos) {
            OSS << "using OutEdge = ";
        } else {
            OSS << "using OutEdge     = ";
        }
        copyString(oss, space, content, true);
        oss << ";\n";
    }
    OSS << "using out_edge_iterator = " << outIterType() << ";\n";
    OSS << "using degree_size_type = " << s.mDegreeSizeType << ";\n";
    return oss.str();
}

std::pmr::string CppGraphBuilder::generateBidirectionalGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (!s.mIncidence || !s.mBidirectional)
        return oss.str();

    std::pmr::string space(get_allocator());
    oss << "\n";
    OSS << "// BidirectionalGraph\n";
    {
        auto content = inEdgeType();
        if (content.find_first_of('\n') != content.npos) {
            OSS << "using InEdge = ";
        } else {
            OSS << "using InEdge     = ";
        }
        copyString(oss, space, content, true);
        oss << ";\n";
    }
    OSS << "using in_edge_iterator = " << inIterType() << ";\n";

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateAdjacencyGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (!s.mAdjacency)
        return oss.str();

    std::pmr::string space(get_allocator());
    oss << "\n";
    OSS << "// AdjacencyGraph\n";
    OSS << "using adjacency_iterator = boost::adjacency_iterator_generator<\n";
    OSS << "    " << graphType(mStruct.mCurrentNamespace) << ", vertex_descriptor, out_edge_iterator>::type;\n";

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateVertexListGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (!s.mVertexList)
        return oss.str();

    std::pmr::string space(get_allocator());
    oss << "\n";
    OSS << "// VertexListGraph\n";
    OSS << "using vertex_iterator    = " << vertexIterType() << ";\n";
    OSS << "using vertices_size_type = " << s.mVertexSizeType << ";\n";
    if (s.hasVertexProperty()) {
        oss << "\n";
        OSS << "using vertex_property_type = " << vertexPropertyType() << ";\n";
    }

    if (true) {
        oss << "\n";
        OSS << "// VertexList help functions\n";
        if (s.isVector()) {
            if (s.mIncidence) {
                OSS << outEdgeListType() << "& getOutEdgeList(vertex_descriptor v) noexcept {\n";
                OSS << "    return _vertices[v].outEdges;\n";
                OSS << "}\n";
                OSS << "const " << outEdgeListType() << "& getOutEdgeList(vertex_descriptor v) const noexcept {\n";
                OSS << "    return _vertices[v].outEdges;\n";
                OSS << "}\n";

                if (s.mBidirectional) {
                    oss << "\n";
                    OSS << inEdgeListType() << "& getInEdgeList(vertex_descriptor v) noexcept {\n";
                    OSS << "    return _vertices[v].inEdges;\n";
                    OSS << "}\n";
                    OSS << "const " << inEdgeListType() << "& getInEdgeList(vertex_descriptor v) const noexcept {\n";
                    OSS << "    return _vertices[v].inEdges;\n";
                    OSS << "}\n";
                }
                oss << "\n";
            }
            OSS << "boost::integer_range<vertex_descriptor> getVertexList() const noexcept {\n";
#ifdef CC_USE_GSL
            OSS << "    return {0, gsl::narrow_cast<vertices_size_type>(_vertices.size())};\n";
#else
            OSS << "    return {0, static_cast<vertices_size_type>(_vertices.size())};\n";
#endif
            OSS << "}\n";

            oss << "\n";
            OSS << "vertex_descriptor getCurrentID() const noexcept {\n";
#ifdef CC_USE_GSL
            OSS << "    return gsl::narrow_cast<vertex_descriptor>(_vertices.size());\n";
#else
            OSS << "    return static_cast<vertex_descriptor>(_vertices.size());\n";
#endif
            OSS << "}\n";
            if (s.mColorMap) {
                oss << "\n";
                OSS << "ccstd::pmr::vector<boost::default_color_type> colors(boost::container::pmr::memory_resource* mr) const {\n";
                OSS << "    return ccstd::pmr::vector<boost::default_color_type>(_vertices.size(), mr);\n";
                OSS << "}\n";
            }
        } else {
            if (s.mIncidence) {
                OSS << outEdgeListType() << "& getOutEdgeList(vertex_descriptor v) noexcept {\n";
                OSS << "    auto pVertex = static_cast<Vertex*>(v);\n";
                OSS << "    return pVertex->outEdges;\n";
                OSS << "}\n";
                OSS << "const " << outEdgeListType() << "& getOutEdgeList(vertex_descriptor v) const noexcept {\n";
                OSS << "    auto pVertex = static_cast<const Vertex*>(v);\n";
                OSS << "    return pVertex->outEdges;\n";
                OSS << "}\n";

                if (s.mBidirectional) {
                    oss << "\n";
                    OSS << inEdgeListType() << "& getInEdgeList(vertex_descriptor v) noexcept {\n";
                    OSS << "    auto pVertex = static_cast<Vertex*>(v);\n";
                    OSS << "    return pVertex->inEdges;\n";
                    OSS << "}\n";
                    OSS << "const " << inEdgeListType() << "& getInEdgeList(vertex_descriptor v) const noexcept {\n";
                    OSS << "    auto pVertex = static_cast<Vertex*>(v);\n";
                    OSS << "    return pVertex->inEdges;\n";
                    OSS << "}\n";
                }
                oss << "\n";
            }
            OSS << vertexListType() << "& getVertexList() noexcept {\n";
            OSS << "    return _vertices;\n";
            OSS << "}\n";
            OSS << "const " << vertexListType() << "& getVertexList() const noexcept {\n";
            OSS << "    return _vertices;\n";
            OSS << "}\n";
        }
    }

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateEdgeListGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    if (!s.mEdgeList)
        return oss.str();

    std::pmr::string space(get_allocator());

    oss << "\n";
    OSS << "// EdgeListGraph\n";
    if (s.needEdgeList()) {
        OSS << "using edge_iterator = impl::UndirectedEdgeIter<\n";
        OSS << "    " << edgeListType() << "::iterator,\n";
        OSS << "    edge_descriptor, " << s.mEdgeDifferenceType << ">;\n";
    } else {
        OSS << "using edge_iterator   = impl::DirectedEdgeIterator<vertex_iterator, out_edge_iterator, "
            << graphType(mStruct.mCurrentNamespace) << ">;\n";
    }
    OSS << "using edges_size_type = " << s.mEdgeSizeType << ";\n";
    if (s.hasEdgeProperty()) {
        oss << "\n";
        OSS << "using edge_property_type = " << edgePropertyType() << ";\n";
    }
    return oss.str();
}

std::pmr::string CppGraphBuilder::generateMutableGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    std::pmr::string space(get_allocator());
    if (s.hasVertexProperty()) {
        oss << "\n";
        OSS << "// PropertyGraph (Vertex)\n";
        OSS << "const vertex_property_type& operator[](vertex_descriptor v) const noexcept {\n";
        {
            INDENT();
            if (s.isVector()) {
                OSS << "return _vertices[v].property;\n";
            } else {
                OSS << "Expects(v);\n";
                OSS << "auto* sv = static_cast<const Vertex*>(v);\n";
                OSS << "return sv->property;\n";
            }
        }
        OSS << "}\n";
        OSS << "vertex_property_type& operator[](vertex_descriptor v) noexcept {\n";
        {
            INDENT();
            if (s.isVector()) {
                OSS << "return _vertices[v].property;\n";
            } else {
                OSS << "Expects(v);\n";
                OSS << "auto* sv = static_cast<Vertex*>(v);\n";
                OSS << "return sv->property;\n";
            }
        }
        OSS << "}\n";
    }

    if (s.hasEdgeProperty()) {
        oss << "\n";
        OSS << "// PropertyGraph (Edge)\n";
        OSS << "const edge_property_type& operator[](const edge_descriptor& e) const noexcept {\n";
        {
            INDENT();
            OSS << "return *static_cast<edge_property_type*>(e.get_property());\n";
        }
        OSS << "}\n";
        OSS << "edge_property_type& operator[](const edge_descriptor& e) noexcept {\n";
        {
            INDENT();
            OSS << "return *static_cast<edge_property_type*>(e.get_property());\n";
        }
        OSS << "}\n";
    }

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateReferenceGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    std::pmr::string space(get_allocator());

    if (!s.mReferenceGraph)
        return oss.str();

    oss << "\n";
    if (s.mAliasGraph) {
        OSS << "// AddressableGraph (Alias)\n";
    } else {
        OSS << "// AddressableGraph (Separated)\n";
    }

    OSS << "using ownership_descriptor = impl::EdgeDescriptor<boost::bidirectional_tag, vertex_descriptor>;\n";

    oss << "\n";
    OSS << "using ChildEdge = ";
    copyString(oss, space, childEdgeType(), true);
    oss << ";\n";
    OSS << "using children_iterator  = " << childIterType() << ";\n";
    OSS << "using children_size_type = " << s.mDegreeSizeType << ";\n";

    oss << "\n";
    OSS << "using ParentEdge = ";
    copyString(oss, space, parentEdgeType(), true);
    oss << ";\n";
    OSS << "using parent_iterator  = " << parentIterType() << ";\n";

    oss << "\n";
    OSS << "using ownership_iterator   = " << referenceIterType() << ";\n";
    OSS << "using ownerships_size_type = edges_size_type;\n";

    oss << "\n";
    OSS << "// AddressableGraph help functions\n";
    if (s.mAliasGraph) {
        if (s.isVector()) {
            OSS << childListType() << "& getChildrenList(vertex_descriptor v) noexcept {\n";
            OSS << "    return _vertices[v].outEdges;\n";
            OSS << "}\n";
            OSS << "const " << childListType() << "& getChildrenList(vertex_descriptor v) const noexcept {\n";
            OSS << "    return _vertices[v].outEdges;\n";
            OSS << "}\n";

            if (s.mBidirectional) {
                oss << "\n";
                OSS << parentListType() << "& getParentsList(vertex_descriptor v) noexcept {\n";
                OSS << "    return _vertices[v].inEdges;\n";
                OSS << "}\n";
                OSS << "const " << parentListType() << "& getParentsList(vertex_descriptor v) const noexcept {\n";
                OSS << "    return _vertices[v].inEdges;\n";
                OSS << "}\n";
            }
        } else {
            OSS << childListType() << "& getChildrenList(vertex_descriptor v) noexcept {\n";
            OSS << "    auto pVertex = static_cast<Vertex*>(v);\n";
            OSS << "    return pVertex->outEdges;\n";
            OSS << "}\n";
            OSS << "const " << childListType() << "& getChildrenList(vertex_descriptor v) const noexcept {\n";
            OSS << "    auto pVertex = static_cast<const Vertex*>(v);\n";
            OSS << "    return pVertex->outEdges;\n";
            OSS << "}\n";

            if (s.mBidirectional) {
                oss << "\n";
                OSS << parentListType() << "& getParentsList(vertex_descriptor v) noexcept {\n";
                OSS << "    auto pVertex = static_cast<Vertex*>(v);\n";
                OSS << "    return pVertex->inEdges;\n";
                OSS << "}\n";
                OSS << "const " << parentListType() << "& getParentsList(vertex_descriptor v) const noexcept {\n";
                OSS << "    auto pVertex = static_cast<Vertex*>(v);\n";
                OSS << "    return pVertex->inEdges;\n";
                OSS << "}\n";
            }
        }
    } else {
        if (s.isVector()) {
            OSS << childListType() << "& getChildrenList(vertex_descriptor v) noexcept {\n";
            OSS << "    return objects[v].children;\n";
            OSS << "}\n";
            OSS << "const " << childListType() << "& getChildrenList(vertex_descriptor v) const noexcept {\n";
            OSS << "    return objects[v].children;\n";
            OSS << "}\n";

            if (s.mBidirectional) {
                oss << "\n";
                OSS << parentListType() << "& getParentsList(vertex_descriptor v) noexcept {\n";
                OSS << "    return objects[v].parents;\n";
                OSS << "}\n";
                OSS << "const " << parentListType() << "& getParentsList(vertex_descriptor v) const noexcept {\n";
                OSS << "    return objects[v].parents;\n";
                OSS << "}\n";
            }
        } else {
            OSS << childListType() << "& getChildrenList(vertex_descriptor v) noexcept {\n";
            OSS << "    auto pVertex = static_cast<Vertex*>(v);\n";
            OSS << "    return pVertex->children;\n";
            OSS << "}\n";
            OSS << "const " << childListType() << "& getChildrenList(vertex_descriptor v) const noexcept {\n";
            OSS << "    auto pVertex = static_cast<const Vertex*>(v);\n";
            OSS << "    return pVertex->children;\n";
            OSS << "}\n";

            if (s.mBidirectional) {
                oss << "\n";
                OSS << parentListType() << "& getParentsList(vertex_descriptor v) noexcept {\n";
                OSS << "    auto pVertex = static_cast<Vertex*>(v);\n";
                OSS << "    return pVertex->parents;\n";
                OSS << "}\n";
                OSS << "const " << parentListType() << "& getParentsList(vertex_descriptor v) const noexcept {\n";
                OSS << "    auto pVertex = static_cast<Vertex*>(v);\n";
                OSS << "    return pVertex->parents;\n";
                OSS << "}\n";
            }
        }
    }

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateParentGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    std::pmr::string space(get_allocator());

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateAddressableGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    std::pmr::string space(get_allocator());

    return oss.str();
}

std::pmr::string CppGraphBuilder::generatePolymorphicGraph_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    std::pmr::string space(get_allocator());
    if (!s.isPolymorphic())
        return oss.str();

    oss << "\n";
    OSS << "// PolymorphicGraph\n";
    OSS << "using VertexTag         = " << tagType() << ";\n";
    OSS << "using VertexValue       = " << valueType() << ";\n";
    OSS << "using VertexConstValue = " << constValueType() << ";\n";
    OSS << "using VertexHandle      = ";
    {
        INDENT();
        copyString(oss, space, handleType(), true);
        oss << ";\n";
    }
    return oss.str();
}

std::pmr::string CppGraphBuilder::generateMemberFunctions_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& s = *mGraph;
    std::pmr::string api(get_allocator());
    api = mStruct.mAPI;
    api.append("_API ");

    std::pmr::string space(get_allocator());

    for (const auto& func : s.mMemberFunctions) {
        auto functions = boost::algorithm::replace_all_copy(
            func, "[[dll]] ", api);
        functions = boost::algorithm::replace_all_copy(
            functions, "[[sender]] ", api);
        copyCppString(oss, space, functions);
    }

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateReserve_h() const {
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
    oss << "\n";
    OSS << "// ContinuousContainer\n";
    if (!mStruct.mAPI.empty()) {
        OSS << mStruct.mAPI << "_API ";
    } else {
        OSS;
    }
    oss << "void reserve(vertices_size_type sz);\n";

    return oss.str();
}

std::pmr::string CppGraphBuilder::generateTags_h() const {
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
        if (false) {
            OSS << "struct " << getTagType(c.mName) << " {\n";
            OSS << "} static constexpr " << convertTag(c.mName) << "{}; // NOLINT\n";
        }
        OSS << "struct " << getTagType(c.mName) << " {};\n";
        ++count;
    }
    oss << "\n";
    return oss.str();
}

std::pmr::string CppGraphBuilder::generateMembers_h() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    std::pmr::string space(get_allocator());
    copyString(oss, space, mStruct.generateMembers());
    return oss.str();
}

std::pmr::string CppGraphBuilder::generateReserve_cpp() const {
    pmr_ostringstream oss(std::ios::out, get_allocator());
    const auto& g = *mStruct.mSyntaxGraph;
    const auto& s = *mGraph;

    auto hasReserve = [&]() {
        if (s.isVector())
            return true;
        return false;
    };

    if (!hasReserve())
        return oss.str();

    std::pmr::string space(get_allocator());
    auto structName = mStruct.getImplName();
    oss << "\n";
    OSS << "// ContinuousContainer\n";
    OSS << "void " << structName << "::reserve(vertices_size_type sz) {\n";
    {
        INDENT();
        if (s.isReference() && !s.isAliasGraph()) {
            OSS << "objects.reserve(sz);\n";
        }
        if (s.isVector()) {
            OSS << "_vertices.reserve(sz);\n";
            for (const auto& c : s.mComponents) {
                OSS << g.getMemberName(c.mMemberName, true) << ".reserve(sz);\n";
            }
        }
    }
    OSS << "}\n";

    return oss.str();
}

}
