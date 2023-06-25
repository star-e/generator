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

#pragma once
#include <Cocos/GraphImpl.h>
#include <Cocos/AST/SyntaxTypes.h>

namespace Cocos {

namespace Meta {

// IncidenceGraph
inline SyntaxGraph::vertex_descriptor
source(const SyntaxGraph::edge_descriptor& e, const SyntaxGraph& g) noexcept {
    return e.m_source;
}

inline SyntaxGraph::vertex_descriptor
target(const SyntaxGraph::edge_descriptor& e, const SyntaxGraph& g) noexcept {
    return e.m_target;
}

inline std::pair<SyntaxGraph::out_edge_iterator, SyntaxGraph::out_edge_iterator>
out_edges(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    return std::make_pair(
        SyntaxGraph::out_edge_iterator(const_cast<SyntaxGraph&>(g).out_edge_list(u).begin(), u),
        SyntaxGraph::out_edge_iterator(const_cast<SyntaxGraph&>(g).out_edge_list(u).end(), u));
}

inline SyntaxGraph::degree_size_type
out_degree(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    return gsl::narrow_cast<SyntaxGraph::degree_size_type>(g.out_edge_list(u).size());
}

inline std::pair<SyntaxGraph::edge_descriptor, bool>
edge(SyntaxGraph::vertex_descriptor u, SyntaxGraph::vertex_descriptor v, const SyntaxGraph& g) noexcept {
    auto& outEdgeList = g.out_edge_list(u);
    auto iter = std::find(outEdgeList.begin(), outEdgeList.end(), SyntaxGraph::out_edge_type(v));
    bool hasEdge = (iter != outEdgeList.end());
    return std::pair{ SyntaxGraph::edge_descriptor(u, v), hasEdge };
}

// BidirectionalGraph(Directed)
inline std::pair<SyntaxGraph::in_edge_iterator, SyntaxGraph::in_edge_iterator>
in_edges(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    return std::make_pair(
        SyntaxGraph::in_edge_iterator(const_cast<SyntaxGraph&>(g).in_edge_list(u).begin(), u),
        SyntaxGraph::in_edge_iterator(const_cast<SyntaxGraph&>(g).in_edge_list(u).end(), u));
}

inline SyntaxGraph::degree_size_type
in_degree(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    return gsl::narrow_cast<SyntaxGraph::degree_size_type>(g.in_edge_list(u).size());
}

inline SyntaxGraph::degree_size_type
degree(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    return in_degree(u, g) + out_degree(u, g);
}

// AdjacencyGraph
inline std::pair<SyntaxGraph::adjacency_iterator, SyntaxGraph::adjacency_iterator>
adjacent_vertices(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    auto edges = out_edges(u, g);
    return std::make_pair(SyntaxGraph::adjacency_iterator(edges.first, &g), SyntaxGraph::adjacency_iterator(edges.second, &g));
}

// VertexListGraph
inline std::pair<SyntaxGraph::vertex_iterator, SyntaxGraph::vertex_iterator>
vertices(const SyntaxGraph& g) noexcept {
    return std::make_pair(const_cast<SyntaxGraph&>(g).vertex_set().begin(), const_cast<SyntaxGraph&>(g).vertex_set().end());
}

inline SyntaxGraph::vertices_size_type
num_vertices(const SyntaxGraph& g) noexcept {
    return gsl::narrow_cast<SyntaxGraph::vertices_size_type>(g.vertex_set().size());
}

// EdgeListGraph
inline std::pair<SyntaxGraph::edge_iterator, SyntaxGraph::edge_iterator>
edges(const SyntaxGraph& g0) noexcept {
    auto& g = const_cast<SyntaxGraph&>(g0);
    return std::make_pair(
        SyntaxGraph::edge_iterator(g.vertex_set().begin(), g.vertex_set().begin(), g.vertex_set().end(), g),
        SyntaxGraph::edge_iterator(g.vertex_set().begin(), g.vertex_set().end(), g.vertex_set().end(), g));
}

inline SyntaxGraph::edges_size_type
num_edges(const SyntaxGraph& g) noexcept {
    SyntaxGraph::edges_size_type numEdges = 0;
    auto range = vertices(g);
    for (auto iter = range.first; iter != range.second; ++iter) {
        numEdges += out_degree(*iter, g);
    }
    return numEdges;
}

// MutableGraph(Edge)
inline std::pair<SyntaxGraph::edge_descriptor, bool>
add_edge(
    SyntaxGraph::vertex_descriptor u,
    SyntaxGraph::vertex_descriptor v, SyntaxGraph& g) {
    auto& outEdgeList = g.out_edge_list(u);
    outEdgeList.emplace_back(v);

    auto& inEdgeList = g.in_edge_list(v);
    inEdgeList.emplace_back(u);

    return std::make_pair(SyntaxGraph::edge_descriptor(u, v), true);
}

inline void remove_edge(SyntaxGraph::vertex_descriptor u, SyntaxGraph::vertex_descriptor v, SyntaxGraph& g) noexcept {
    // remove out-edges
    auto& outEdgeList = g.out_edge_list(u);
    // eraseFromIncidenceList
    Impl::sequenceEraseIf(outEdgeList, [v](const auto& e) {
        return e.get_target() == v;
    });

    // remove reciprocal (bidirectional) in-edges
    auto& inEdgeList = g.in_edge_list(v);
    // eraseFromIncidenceList
    Impl::sequenceEraseIf(inEdgeList, [u](const auto& e) {
        return e.get_target() == u;
    });
}

inline void remove_edge(SyntaxGraph::edge_descriptor e, SyntaxGraph& g) noexcept {
    // remove_edge need rewrite
    auto& outEdgeList = g.out_edge_list(source(e, g));
    Impl::removeIncidenceEdge(e, outEdgeList);
    auto& inEdgeList = g.in_edge_list(target(e, g));
    Impl::removeIncidenceEdge(e, inEdgeList);
}

inline void remove_edge(SyntaxGraph::out_edge_iterator iter, SyntaxGraph& g) noexcept {
    auto e = *iter;
    auto& outEdgeList = g.out_edge_list(source(e, g));
    auto& inEdgeList = g.in_edge_list(target(e, g));
    Impl::removeIncidenceEdge(e, inEdgeList);
    outEdgeList.erase(iter.base());
}

template <class Predicate>
inline void remove_out_edge_if(SyntaxGraph::vertex_descriptor u, Predicate&& pred, SyntaxGraph& g) noexcept {
    for (auto [out_i, out_end] = out_edges(u, g); out_i != out_end; ++out_i) {
        if (pred(*out_i)) {
            auto& inEdgeList = g.in_edge_list(target(*out_i, g));
            auto e = *out_i;
            Impl::removeIncidenceEdge(e, inEdgeList);
        }
    }
    auto [first, last] = out_edges(u, g);
    auto& outEdgeList = g.out_edge_list(u);
    Impl::sequenceRemoveIncidenceEdgeIf(first, last, outEdgeList, std::forward<Predicate>(pred));
}

template <class Predicate>
inline void remove_in_edge_if(SyntaxGraph::vertex_descriptor v, Predicate&& pred, SyntaxGraph& g) noexcept {
    for (auto [in_i, in_end] = in_edges(v, g); in_i != in_end; ++in_i) {
        if (pred(*in_i)) {
            auto& outEdgeList = g.out_edge_list(source(*in_i, g));
            auto e = *in_i;
            Impl::removeIncidenceEdge(e, outEdgeList);
        }
    }
    auto [first, last] = in_edges(v, g);
    auto& inEdgeList = g.in_edge_list(v);
    Impl::sequenceRemoveIncidenceEdgeIf(first, last, inEdgeList, std::forward<Predicate>(pred));
}

template <class Predicate>
inline void remove_edge_if(Predicate&& pred, SyntaxGraph& g) noexcept {
    auto [ei, ei_end] = edges(g);
    for (auto next = ei; ei != ei_end; ei = next) {
        ++next;
        if (pred(*ei))
            remove_edge(*ei, g);
    }
}

// AddressableGraph
inline SyntaxGraph::vertex_descriptor
parent(const SyntaxGraph::ownership_descriptor& e, const SyntaxGraph& g) noexcept {
    return e.m_source;
}

inline SyntaxGraph::vertex_descriptor
child(const SyntaxGraph::ownership_descriptor& e, const SyntaxGraph& g) noexcept {
    return e.m_target;
}

inline std::pair<SyntaxGraph::children_iterator, SyntaxGraph::children_iterator>
children(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    return std::make_pair(
        SyntaxGraph::children_iterator(const_cast<SyntaxGraph&>(g).children_list(u).begin(), u),
        SyntaxGraph::children_iterator(const_cast<SyntaxGraph&>(g).children_list(u).end(), u));
}

inline SyntaxGraph::children_size_type
num_children(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    return gsl::narrow_cast<SyntaxGraph::children_size_type>(g.children_list(u).size());
}

inline std::pair<SyntaxGraph::ownership_descriptor, bool>
ownership(SyntaxGraph::vertex_descriptor u, SyntaxGraph::vertex_descriptor v, SyntaxGraph& g) noexcept {
    auto& outEdgeList = g.children_list(u);
    auto iter = std::find(outEdgeList.begin(), outEdgeList.end(), SyntaxGraph::out_edge_type(v));
    bool hasEdge = (iter != outEdgeList.end());
    return std::pair{ SyntaxGraph::ownership_descriptor(u, v), hasEdge };
}

inline std::pair<SyntaxGraph::parent_iterator, SyntaxGraph::parent_iterator>
parents(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    return std::make_pair(
        SyntaxGraph::parent_iterator(const_cast<SyntaxGraph&>(g).parents_list(u).begin(), u),
        SyntaxGraph::parent_iterator(const_cast<SyntaxGraph&>(g).parents_list(u).end(), u));
}

inline SyntaxGraph::children_size_type
num_parents(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    return gsl::narrow_cast<SyntaxGraph::children_size_type>(g.parents_list(u).size());
}

inline SyntaxGraph::vertex_descriptor
parent(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    auto r = parents(u, g);
    if (r.first == r.second) {
        return SyntaxGraph::null_vertex();
    } else {
        return parent(*r.first, g);
    }
}

inline bool
ancestor(SyntaxGraph::vertex_descriptor u, SyntaxGraph::vertex_descriptor v, const SyntaxGraph& g) noexcept {
    Expects(u != v);
    bool isAncestor = false;
    auto r = parents(v, g);
    while (r.first != r.second) {
        v = parent(*r.first, g);
        if (u == v) {
            isAncestor = true;
            break;
        }
        r = parents(v, g);
    }
    return isAncestor;
}

inline std::pair<SyntaxGraph::ownership_iterator, SyntaxGraph::ownership_iterator>
ownerships(const SyntaxGraph& g0) noexcept {
    auto& g = const_cast<SyntaxGraph&>(g0);
    return std::make_pair(
        SyntaxGraph::ownership_iterator(g.vertex_set().begin(), g.vertex_set().begin(), g.vertex_set().end(), g),
        SyntaxGraph::ownership_iterator(g.vertex_set().begin(), g.vertex_set().end(), g.vertex_set().end(), g));
}

inline SyntaxGraph::ownerships_size_type
num_ownerships(const SyntaxGraph& g) noexcept {
    SyntaxGraph::ownerships_size_type numEdges = 0;
    auto range = vertices(g);
    for (auto iter = range.first; iter != range.second; ++iter) {
        numEdges += num_children(*iter, g);
    }
    return numEdges;
}

// IncidenceGraph
inline ModuleGraph::vertex_descriptor
source(const ModuleGraph::edge_descriptor& e, const ModuleGraph& g) noexcept {
    return e.m_source;
}

inline ModuleGraph::vertex_descriptor
target(const ModuleGraph::edge_descriptor& e, const ModuleGraph& g) noexcept {
    return e.m_target;
}

inline std::pair<ModuleGraph::out_edge_iterator, ModuleGraph::out_edge_iterator>
out_edges(ModuleGraph::vertex_descriptor u, const ModuleGraph& g) noexcept {
    return std::make_pair(
        ModuleGraph::out_edge_iterator(const_cast<ModuleGraph&>(g).out_edge_list(u).begin(), u),
        ModuleGraph::out_edge_iterator(const_cast<ModuleGraph&>(g).out_edge_list(u).end(), u));
}

inline ModuleGraph::degree_size_type
out_degree(ModuleGraph::vertex_descriptor u, const ModuleGraph& g) noexcept {
    return gsl::narrow_cast<ModuleGraph::degree_size_type>(g.out_edge_list(u).size());
}

inline std::pair<ModuleGraph::edge_descriptor, bool>
edge(ModuleGraph::vertex_descriptor u, ModuleGraph::vertex_descriptor v, const ModuleGraph& g) noexcept {
    auto& outEdgeList = g.out_edge_list(u);
    auto iter = std::find(outEdgeList.begin(), outEdgeList.end(), ModuleGraph::out_edge_type(v));
    bool hasEdge = (iter != outEdgeList.end());
    return std::pair{ ModuleGraph::edge_descriptor(u, v), hasEdge };
}

// BidirectionalGraph(Directed)
inline std::pair<ModuleGraph::in_edge_iterator, ModuleGraph::in_edge_iterator>
in_edges(ModuleGraph::vertex_descriptor u, const ModuleGraph& g) noexcept {
    return std::make_pair(
        ModuleGraph::in_edge_iterator(const_cast<ModuleGraph&>(g).in_edge_list(u).begin(), u),
        ModuleGraph::in_edge_iterator(const_cast<ModuleGraph&>(g).in_edge_list(u).end(), u));
}

inline ModuleGraph::degree_size_type
in_degree(ModuleGraph::vertex_descriptor u, const ModuleGraph& g) noexcept {
    return gsl::narrow_cast<ModuleGraph::degree_size_type>(g.in_edge_list(u).size());
}

inline ModuleGraph::degree_size_type
degree(ModuleGraph::vertex_descriptor u, const ModuleGraph& g) noexcept {
    return in_degree(u, g) + out_degree(u, g);
}

// AdjacencyGraph
inline std::pair<ModuleGraph::adjacency_iterator, ModuleGraph::adjacency_iterator>
adjacent_vertices(ModuleGraph::vertex_descriptor u, const ModuleGraph& g) noexcept {
    auto edges = out_edges(u, g);
    return std::make_pair(ModuleGraph::adjacency_iterator(edges.first, &g), ModuleGraph::adjacency_iterator(edges.second, &g));
}

// VertexListGraph
inline std::pair<ModuleGraph::vertex_iterator, ModuleGraph::vertex_iterator>
vertices(const ModuleGraph& g) noexcept {
    return std::make_pair(const_cast<ModuleGraph&>(g).vertex_set().begin(), const_cast<ModuleGraph&>(g).vertex_set().end());
}

inline ModuleGraph::vertices_size_type
num_vertices(const ModuleGraph& g) noexcept {
    return gsl::narrow_cast<ModuleGraph::vertices_size_type>(g.vertex_set().size());
}

// EdgeListGraph
inline std::pair<ModuleGraph::edge_iterator, ModuleGraph::edge_iterator>
edges(const ModuleGraph& g0) noexcept {
    auto& g = const_cast<ModuleGraph&>(g0);
    return std::make_pair(
        ModuleGraph::edge_iterator(g.vertex_set().begin(), g.vertex_set().begin(), g.vertex_set().end(), g),
        ModuleGraph::edge_iterator(g.vertex_set().begin(), g.vertex_set().end(), g.vertex_set().end(), g));
}

inline ModuleGraph::edges_size_type
num_edges(const ModuleGraph& g) noexcept {
    ModuleGraph::edges_size_type numEdges = 0;
    auto range = vertices(g);
    for (auto iter = range.first; iter != range.second; ++iter) {
        numEdges += out_degree(*iter, g);
    }
    return numEdges;
}

// MutableGraph(Edge)
inline std::pair<ModuleGraph::edge_descriptor, bool>
add_edge(
    ModuleGraph::vertex_descriptor u,
    ModuleGraph::vertex_descriptor v, ModuleGraph& g) {
    auto& outEdgeList = g.out_edge_list(u);
    outEdgeList.emplace_back(v);

    auto& inEdgeList = g.in_edge_list(v);
    inEdgeList.emplace_back(u);

    return std::make_pair(ModuleGraph::edge_descriptor(u, v), true);
}

inline void remove_edge(ModuleGraph::vertex_descriptor u, ModuleGraph::vertex_descriptor v, ModuleGraph& g) noexcept {
    // remove out-edges
    auto& outEdgeList = g.out_edge_list(u);
    // eraseFromIncidenceList
    Impl::sequenceEraseIf(outEdgeList, [v](const auto& e) {
        return e.get_target() == v;
    });

    // remove reciprocal (bidirectional) in-edges
    auto& inEdgeList = g.in_edge_list(v);
    // eraseFromIncidenceList
    Impl::sequenceEraseIf(inEdgeList, [u](const auto& e) {
        return e.get_target() == u;
    });
}

inline void remove_edge(ModuleGraph::edge_descriptor e, ModuleGraph& g) noexcept {
    // remove_edge need rewrite
    auto& outEdgeList = g.out_edge_list(source(e, g));
    Impl::removeIncidenceEdge(e, outEdgeList);
    auto& inEdgeList = g.in_edge_list(target(e, g));
    Impl::removeIncidenceEdge(e, inEdgeList);
}

inline void remove_edge(ModuleGraph::out_edge_iterator iter, ModuleGraph& g) noexcept {
    auto e = *iter;
    auto& outEdgeList = g.out_edge_list(source(e, g));
    auto& inEdgeList = g.in_edge_list(target(e, g));
    Impl::removeIncidenceEdge(e, inEdgeList);
    outEdgeList.erase(iter.base());
}

template <class Predicate>
inline void remove_out_edge_if(ModuleGraph::vertex_descriptor u, Predicate&& pred, ModuleGraph& g) noexcept {
    for (auto [out_i, out_end] = out_edges(u, g); out_i != out_end; ++out_i) {
        if (pred(*out_i)) {
            auto& inEdgeList = g.in_edge_list(target(*out_i, g));
            auto e = *out_i;
            Impl::removeIncidenceEdge(e, inEdgeList);
        }
    }
    auto [first, last] = out_edges(u, g);
    auto& outEdgeList = g.out_edge_list(u);
    Impl::sequenceRemoveIncidenceEdgeIf(first, last, outEdgeList, std::forward<Predicate>(pred));
}

template <class Predicate>
inline void remove_in_edge_if(ModuleGraph::vertex_descriptor v, Predicate&& pred, ModuleGraph& g) noexcept {
    for (auto [in_i, in_end] = in_edges(v, g); in_i != in_end; ++in_i) {
        if (pred(*in_i)) {
            auto& outEdgeList = g.out_edge_list(source(*in_i, g));
            auto e = *in_i;
            Impl::removeIncidenceEdge(e, outEdgeList);
        }
    }
    auto [first, last] = in_edges(v, g);
    auto& inEdgeList = g.in_edge_list(v);
    Impl::sequenceRemoveIncidenceEdgeIf(first, last, inEdgeList, std::forward<Predicate>(pred));
}

template <class Predicate>
inline void remove_edge_if(Predicate&& pred, ModuleGraph& g) noexcept {
    auto [ei, ei_end] = edges(g);
    for (auto next = ei; ei != ei_end; ei = next) {
        ++next;
        if (pred(*ei))
            remove_edge(*ei, g);
    }
}

// AddressableGraph
inline ModuleGraph::vertex_descriptor
parent(const ModuleGraph::ownership_descriptor& e, const ModuleGraph& g) noexcept {
    return e.m_source;
}

inline ModuleGraph::vertex_descriptor
child(const ModuleGraph::ownership_descriptor& e, const ModuleGraph& g) noexcept {
    return e.m_target;
}

inline std::pair<ModuleGraph::children_iterator, ModuleGraph::children_iterator>
children(ModuleGraph::vertex_descriptor u, const ModuleGraph& g) noexcept {
    return std::make_pair(
        ModuleGraph::children_iterator(const_cast<ModuleGraph&>(g).children_list(u).begin(), u),
        ModuleGraph::children_iterator(const_cast<ModuleGraph&>(g).children_list(u).end(), u));
}

inline ModuleGraph::children_size_type
num_children(ModuleGraph::vertex_descriptor u, const ModuleGraph& g) noexcept {
    return gsl::narrow_cast<ModuleGraph::children_size_type>(g.children_list(u).size());
}

inline std::pair<ModuleGraph::ownership_descriptor, bool>
ownership(ModuleGraph::vertex_descriptor u, ModuleGraph::vertex_descriptor v, ModuleGraph& g) noexcept {
    auto& outEdgeList = g.children_list(u);
    auto iter = std::find(outEdgeList.begin(), outEdgeList.end(), ModuleGraph::out_edge_type(v));
    bool hasEdge = (iter != outEdgeList.end());
    return std::pair{ ModuleGraph::ownership_descriptor(u, v), hasEdge };
}

inline std::pair<ModuleGraph::parent_iterator, ModuleGraph::parent_iterator>
parents(ModuleGraph::vertex_descriptor u, const ModuleGraph& g) noexcept {
    return std::make_pair(
        ModuleGraph::parent_iterator(const_cast<ModuleGraph&>(g).parents_list(u).begin(), u),
        ModuleGraph::parent_iterator(const_cast<ModuleGraph&>(g).parents_list(u).end(), u));
}

inline ModuleGraph::children_size_type
num_parents(ModuleGraph::vertex_descriptor u, const ModuleGraph& g) noexcept {
    return gsl::narrow_cast<ModuleGraph::children_size_type>(g.parents_list(u).size());
}

inline ModuleGraph::vertex_descriptor
parent(ModuleGraph::vertex_descriptor u, const ModuleGraph& g) noexcept {
    auto r = parents(u, g);
    if (r.first == r.second) {
        return ModuleGraph::null_vertex();
    } else {
        return parent(*r.first, g);
    }
}

inline bool
ancestor(ModuleGraph::vertex_descriptor u, ModuleGraph::vertex_descriptor v, const ModuleGraph& g) noexcept {
    Expects(u != v);
    bool isAncestor = false;
    auto r = parents(v, g);
    while (r.first != r.second) {
        v = parent(*r.first, g);
        if (u == v) {
            isAncestor = true;
            break;
        }
        r = parents(v, g);
    }
    return isAncestor;
}

inline std::pair<ModuleGraph::ownership_iterator, ModuleGraph::ownership_iterator>
ownerships(const ModuleGraph& g0) noexcept {
    auto& g = const_cast<ModuleGraph&>(g0);
    return std::make_pair(
        ModuleGraph::ownership_iterator(g.vertex_set().begin(), g.vertex_set().begin(), g.vertex_set().end(), g),
        ModuleGraph::ownership_iterator(g.vertex_set().begin(), g.vertex_set().end(), g.vertex_set().end(), g));
}

inline ModuleGraph::ownerships_size_type
num_ownerships(const ModuleGraph& g) noexcept {
    ModuleGraph::ownerships_size_type numEdges = 0;
    auto range = vertices(g);
    for (auto iter = range.first; iter != range.second; ++iter) {
        numEdges += num_children(*iter, g);
    }
    return numEdges;
}

} // namespace Meta

} // namespace Cocos

namespace boost {

// Vertex Index
template <>
struct property_map<Cocos::Meta::SyntaxGraph, vertex_index_t> {
    using const_type = identity_property_map;
    using type = identity_property_map;
};

// Vertex Component
template <>
struct property_map<Cocos::Meta::SyntaxGraph, Cocos::Meta::SyntaxGraph::names_> {
    using const_type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::SyntaxGraph,
        const std::pmr::vector<std::pmr::string>,
        std::pmr::string,
        const std::pmr::string&
    >;
    using type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::SyntaxGraph,
        std::pmr::vector<std::pmr::string>,
        std::pmr::string,
        std::pmr::string&
    >;
};

// Vertex Name
template <>
struct property_map<Cocos::Meta::SyntaxGraph, vertex_name_t> {
    using const_type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::SyntaxGraph,
        const std::pmr::vector<std::pmr::string>,
        std::pmr::string,
        const std::pmr::string&
    >;
    using type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::SyntaxGraph,
        std::pmr::vector<std::pmr::string>,
        std::pmr::string,
        std::pmr::string&
    >;
};

// Vertex Component
template <>
struct property_map<Cocos::Meta::SyntaxGraph, Cocos::Meta::SyntaxGraph::traits_> {
    using const_type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::SyntaxGraph,
        const std::pmr::vector<Cocos::Meta::Traits>,
        Cocos::Meta::Traits,
        const Cocos::Meta::Traits&
    >;
    using type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::SyntaxGraph,
        std::pmr::vector<Cocos::Meta::Traits>,
        Cocos::Meta::Traits,
        Cocos::Meta::Traits&
    >;
};

// Vertex ComponentMember
template <class T>
struct property_map<Cocos::Meta::SyntaxGraph, T Cocos::Meta::Traits::*> {
    using const_type = Cocos::Impl::VectorVertexComponentMemberPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::SyntaxGraph,
        const std::pmr::vector<Cocos::Meta::Traits>,
        T,
        const T&,
        T Cocos::Meta::Traits::*
    >;
    using type = Cocos::Impl::VectorVertexComponentMemberPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::SyntaxGraph,
        std::pmr::vector<Cocos::Meta::Traits>,
        T,
        T&,
        T Cocos::Meta::Traits::*
    >;
};

// Vertex Component
template <>
struct property_map<Cocos::Meta::SyntaxGraph, Cocos::Meta::SyntaxGraph::constraints_> {
    using const_type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::SyntaxGraph,
        const std::pmr::vector<Cocos::Meta::Constraints>,
        Cocos::Meta::Constraints,
        const Cocos::Meta::Constraints&
    >;
    using type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::SyntaxGraph,
        std::pmr::vector<Cocos::Meta::Constraints>,
        Cocos::Meta::Constraints,
        Cocos::Meta::Constraints&
    >;
};

// Vertex ComponentMember
template <class T>
struct property_map<Cocos::Meta::SyntaxGraph, T Cocos::Meta::Constraints::*> {
    using const_type = Cocos::Impl::VectorVertexComponentMemberPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::SyntaxGraph,
        const std::pmr::vector<Cocos::Meta::Constraints>,
        T,
        const T&,
        T Cocos::Meta::Constraints::*
    >;
    using type = Cocos::Impl::VectorVertexComponentMemberPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::SyntaxGraph,
        std::pmr::vector<Cocos::Meta::Constraints>,
        T,
        T&,
        T Cocos::Meta::Constraints::*
    >;
};

// Vertex Component
template <>
struct property_map<Cocos::Meta::SyntaxGraph, Cocos::Meta::SyntaxGraph::inherits_> {
    using const_type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::SyntaxGraph,
        const std::pmr::vector<Cocos::Meta::Inherits>,
        Cocos::Meta::Inherits,
        const Cocos::Meta::Inherits&
    >;
    using type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::SyntaxGraph,
        std::pmr::vector<Cocos::Meta::Inherits>,
        Cocos::Meta::Inherits,
        Cocos::Meta::Inherits&
    >;
};

// Vertex ComponentMember
template <class T>
struct property_map<Cocos::Meta::SyntaxGraph, T Cocos::Meta::Inherits::*> {
    using const_type = Cocos::Impl::VectorVertexComponentMemberPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::SyntaxGraph,
        const std::pmr::vector<Cocos::Meta::Inherits>,
        T,
        const T&,
        T Cocos::Meta::Inherits::*
    >;
    using type = Cocos::Impl::VectorVertexComponentMemberPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::SyntaxGraph,
        std::pmr::vector<Cocos::Meta::Inherits>,
        T,
        T&,
        T Cocos::Meta::Inherits::*
    >;
};

// Vertex Component
template <>
struct property_map<Cocos::Meta::SyntaxGraph, Cocos::Meta::SyntaxGraph::modulePaths_> {
    using const_type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::SyntaxGraph,
        const std::pmr::vector<std::pmr::string>,
        std::pmr::string,
        const std::pmr::string&
    >;
    using type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::SyntaxGraph,
        std::pmr::vector<std::pmr::string>,
        std::pmr::string,
        std::pmr::string&
    >;
};

// Vertex Component
template <>
struct property_map<Cocos::Meta::SyntaxGraph, Cocos::Meta::SyntaxGraph::typescripts_> {
    using const_type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::SyntaxGraph,
        const std::pmr::vector<Cocos::Meta::Typescript>,
        Cocos::Meta::Typescript,
        const Cocos::Meta::Typescript&
    >;
    using type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::SyntaxGraph,
        std::pmr::vector<Cocos::Meta::Typescript>,
        Cocos::Meta::Typescript,
        Cocos::Meta::Typescript&
    >;
};

// Vertex ComponentMember
template <class T>
struct property_map<Cocos::Meta::SyntaxGraph, T Cocos::Meta::Typescript::*> {
    using const_type = Cocos::Impl::VectorVertexComponentMemberPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::SyntaxGraph,
        const std::pmr::vector<Cocos::Meta::Typescript>,
        T,
        const T&,
        T Cocos::Meta::Typescript::*
    >;
    using type = Cocos::Impl::VectorVertexComponentMemberPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::SyntaxGraph,
        std::pmr::vector<Cocos::Meta::Typescript>,
        T,
        T&,
        T Cocos::Meta::Typescript::*
    >;
};

// Vertex ComponentMember(String)
template <>
struct property_map<Cocos::Meta::SyntaxGraph, std::pmr::string Cocos::Meta::Typescript::*> {
    using const_type = Cocos::Impl::VectorVertexComponentMemberPropertyMap<
        read_write_property_map_tag,
        const Cocos::Meta::SyntaxGraph,
        const std::pmr::vector<Cocos::Meta::Typescript>,
        std::string_view,
        const std::pmr::string&,
        const std::pmr::string Cocos::Meta::Typescript::*
    >;
    using type = Cocos::Impl::VectorVertexComponentMemberPropertyMap<
        read_write_property_map_tag,
        Cocos::Meta::SyntaxGraph,
        std::pmr::vector<Cocos::Meta::Typescript>,
        std::string_view,
        std::pmr::string&,
        std::pmr::string Cocos::Meta::Typescript::*
    >;
};

// Vertex Component
template <>
struct property_map<Cocos::Meta::SyntaxGraph, Cocos::Meta::SyntaxGraph::comments_> {
    using const_type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::SyntaxGraph,
        const std::pmr::vector<std::pmr::string>,
        std::pmr::string,
        const std::pmr::string&
    >;
    using type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::SyntaxGraph,
        std::pmr::vector<std::pmr::string>,
        std::pmr::string,
        std::pmr::string&
    >;
};

// Vertex Index
template <>
struct property_map<Cocos::Meta::ModuleGraph, vertex_index_t> {
    using const_type = identity_property_map;
    using type = identity_property_map;
};

// Vertex Component
template <>
struct property_map<Cocos::Meta::ModuleGraph, Cocos::Meta::ModuleGraph::names_> {
    using const_type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::ModuleGraph,
        const std::pmr::vector<std::pmr::string>,
        std::pmr::string,
        const std::pmr::string&
    >;
    using type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::ModuleGraph,
        std::pmr::vector<std::pmr::string>,
        std::pmr::string,
        std::pmr::string&
    >;
};

// Vertex Name
template <>
struct property_map<Cocos::Meta::ModuleGraph, vertex_name_t> {
    using const_type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::ModuleGraph,
        const std::pmr::vector<std::pmr::string>,
        std::pmr::string,
        const std::pmr::string&
    >;
    using type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::ModuleGraph,
        std::pmr::vector<std::pmr::string>,
        std::pmr::string,
        std::pmr::string&
    >;
};

// Vertex Component
template <>
struct property_map<Cocos::Meta::ModuleGraph, Cocos::Meta::ModuleGraph::modules_> {
    using const_type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::ModuleGraph,
        const std::pmr::vector<Cocos::Meta::ModuleInfo>,
        Cocos::Meta::ModuleInfo,
        const Cocos::Meta::ModuleInfo&
    >;
    using type = Cocos::Impl::VectorVertexComponentPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::ModuleGraph,
        std::pmr::vector<Cocos::Meta::ModuleInfo>,
        Cocos::Meta::ModuleInfo,
        Cocos::Meta::ModuleInfo&
    >;
};

// Vertex ComponentMember
template <class T>
struct property_map<Cocos::Meta::ModuleGraph, T Cocos::Meta::ModuleInfo::*> {
    using const_type = Cocos::Impl::VectorVertexComponentMemberPropertyMap<
        lvalue_property_map_tag,
        const Cocos::Meta::ModuleGraph,
        const std::pmr::vector<Cocos::Meta::ModuleInfo>,
        T,
        const T&,
        T Cocos::Meta::ModuleInfo::*
    >;
    using type = Cocos::Impl::VectorVertexComponentMemberPropertyMap<
        lvalue_property_map_tag,
        Cocos::Meta::ModuleGraph,
        std::pmr::vector<Cocos::Meta::ModuleInfo>,
        T,
        T&,
        T Cocos::Meta::ModuleInfo::*
    >;
};

// Vertex ComponentMember(String)
template <>
struct property_map<Cocos::Meta::ModuleGraph, std::pmr::string Cocos::Meta::ModuleInfo::*> {
    using const_type = Cocos::Impl::VectorVertexComponentMemberPropertyMap<
        read_write_property_map_tag,
        const Cocos::Meta::ModuleGraph,
        const std::pmr::vector<Cocos::Meta::ModuleInfo>,
        std::string_view,
        const std::pmr::string&,
        const std::pmr::string Cocos::Meta::ModuleInfo::*
    >;
    using type = Cocos::Impl::VectorVertexComponentMemberPropertyMap<
        read_write_property_map_tag,
        Cocos::Meta::ModuleGraph,
        std::pmr::vector<Cocos::Meta::ModuleInfo>,
        std::string_view,
        std::pmr::string&,
        std::pmr::string Cocos::Meta::ModuleInfo::*
    >;
};

} // namespace boost

namespace Cocos {

namespace Meta {

// Vertex Index
inline boost::property_map<SyntaxGraph, boost::vertex_index_t>::const_type
get(boost::vertex_index_t tag, const SyntaxGraph& g) noexcept {
    return {};
}

inline boost::property_map<SyntaxGraph, boost::vertex_index_t>::type
get(boost::vertex_index_t tag, SyntaxGraph& g) noexcept {
    return {};
}

[[nodiscard]] inline Impl::ColorMap<SyntaxGraph::vertex_descriptor>
get(std::pmr::vector<boost::default_color_type>& colors, const SyntaxGraph& g) noexcept {
    return { colors };
}

// Vertex Component
inline typename boost::property_map<SyntaxGraph, SyntaxGraph::names_>::const_type
get(SyntaxGraph::names_, const SyntaxGraph& g) noexcept {
    return { g.mNames };
}

inline typename boost::property_map<SyntaxGraph, SyntaxGraph::names_>::type
get(SyntaxGraph::names_, SyntaxGraph& g) noexcept {
    return { g.mNames };
}

// Vertex Name
inline boost::property_map<SyntaxGraph, boost::vertex_name_t>::const_type
get(boost::vertex_name_t, const SyntaxGraph& g) noexcept {
    return { g.mNames };
}

// Vertex Component
inline typename boost::property_map<SyntaxGraph, SyntaxGraph::traits_>::const_type
get(SyntaxGraph::traits_, const SyntaxGraph& g) noexcept {
    return { g.mTraits };
}

inline typename boost::property_map<SyntaxGraph, SyntaxGraph::traits_>::type
get(SyntaxGraph::traits_, SyntaxGraph& g) noexcept {
    return { g.mTraits };
}

// Vertex ComponentMember
template <class T>
inline typename boost::property_map<SyntaxGraph, T Traits::*>::const_type
get(T Traits::* memberPointer, const SyntaxGraph& g) noexcept {
    return { g.mTraits, memberPointer };
}

template <class T>
inline typename boost::property_map<SyntaxGraph, T Traits::*>::type
get(T Traits::* memberPointer, SyntaxGraph& g) noexcept {
    return { g.mTraits, memberPointer };
}

// Vertex Component
inline typename boost::property_map<SyntaxGraph, SyntaxGraph::constraints_>::const_type
get(SyntaxGraph::constraints_, const SyntaxGraph& g) noexcept {
    return { g.mConstraints };
}

inline typename boost::property_map<SyntaxGraph, SyntaxGraph::constraints_>::type
get(SyntaxGraph::constraints_, SyntaxGraph& g) noexcept {
    return { g.mConstraints };
}

// Vertex ComponentMember
template <class T>
inline typename boost::property_map<SyntaxGraph, T Constraints::*>::const_type
get(T Constraints::* memberPointer, const SyntaxGraph& g) noexcept {
    return { g.mConstraints, memberPointer };
}

template <class T>
inline typename boost::property_map<SyntaxGraph, T Constraints::*>::type
get(T Constraints::* memberPointer, SyntaxGraph& g) noexcept {
    return { g.mConstraints, memberPointer };
}

// Vertex Component
inline typename boost::property_map<SyntaxGraph, SyntaxGraph::inherits_>::const_type
get(SyntaxGraph::inherits_, const SyntaxGraph& g) noexcept {
    return { g.mInherits };
}

inline typename boost::property_map<SyntaxGraph, SyntaxGraph::inherits_>::type
get(SyntaxGraph::inherits_, SyntaxGraph& g) noexcept {
    return { g.mInherits };
}

// Vertex ComponentMember
template <class T>
inline typename boost::property_map<SyntaxGraph, T Inherits::*>::const_type
get(T Inherits::* memberPointer, const SyntaxGraph& g) noexcept {
    return { g.mInherits, memberPointer };
}

template <class T>
inline typename boost::property_map<SyntaxGraph, T Inherits::*>::type
get(T Inherits::* memberPointer, SyntaxGraph& g) noexcept {
    return { g.mInherits, memberPointer };
}

// Vertex Component
inline typename boost::property_map<SyntaxGraph, SyntaxGraph::modulePaths_>::const_type
get(SyntaxGraph::modulePaths_, const SyntaxGraph& g) noexcept {
    return { g.mModulePaths };
}

inline typename boost::property_map<SyntaxGraph, SyntaxGraph::modulePaths_>::type
get(SyntaxGraph::modulePaths_, SyntaxGraph& g) noexcept {
    return { g.mModulePaths };
}

// Vertex Component
inline typename boost::property_map<SyntaxGraph, SyntaxGraph::typescripts_>::const_type
get(SyntaxGraph::typescripts_, const SyntaxGraph& g) noexcept {
    return { g.mTypescripts };
}

inline typename boost::property_map<SyntaxGraph, SyntaxGraph::typescripts_>::type
get(SyntaxGraph::typescripts_, SyntaxGraph& g) noexcept {
    return { g.mTypescripts };
}

// Vertex ComponentMember
template <class T>
inline typename boost::property_map<SyntaxGraph, T Typescript::*>::const_type
get(T Typescript::* memberPointer, const SyntaxGraph& g) noexcept {
    return { g.mTypescripts, memberPointer };
}

template <class T>
inline typename boost::property_map<SyntaxGraph, T Typescript::*>::type
get(T Typescript::* memberPointer, SyntaxGraph& g) noexcept {
    return { g.mTypescripts, memberPointer };
}

// Vertex Component
inline typename boost::property_map<SyntaxGraph, SyntaxGraph::comments_>::const_type
get(SyntaxGraph::comments_, const SyntaxGraph& g) noexcept {
    return { g.mComments };
}

inline typename boost::property_map<SyntaxGraph, SyntaxGraph::comments_>::type
get(SyntaxGraph::comments_, SyntaxGraph& g) noexcept {
    return { g.mComments };
}

// PolymorphicGraph
[[nodiscard]] inline SyntaxGraph::vertices_size_type
value_id(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    using vertex_descriptor = SyntaxGraph::vertex_descriptor;
    return visit(overload(
        [](const Impl::ValueHandle<Define_, vertex_descriptor>& h) {
            return h.mValue;
        },
        [](const Impl::ValueHandle<Namespace_, Namespace>&) {
            return SyntaxGraph::null_vertex();
        },
        [](const Impl::ValueHandle<Concept_, vertex_descriptor>& h) {
            return h.mValue;
        },
        [](const Impl::ValueHandle<Alias_, vertex_descriptor>& h) {
            return h.mValue;
        },
        [](const Impl::ValueHandle<Value_, Value>&) {
            return SyntaxGraph::null_vertex();
        },
        [](const Impl::ValueHandle<Enum_, vertex_descriptor>& h) {
            return h.mValue;
        },
        [](const Impl::ValueHandle<Tag_, vertex_descriptor>& h) {
            return h.mValue;
        },
        [](const Impl::ValueHandle<Struct_, vertex_descriptor>& h) {
            return h.mValue;
        },
        [](const Impl::ValueHandle<Graph_, vertex_descriptor>& h) {
            return h.mValue;
        },
        [](const Impl::ValueHandle<Variant_, vertex_descriptor>& h) {
            return h.mValue;
        },
        [](const Impl::ValueHandle<Container_, Container>&) {
            return SyntaxGraph::null_vertex();
        },
        [](const Impl::ValueHandle<Map_, Map>&) {
            return SyntaxGraph::null_vertex();
        },
        [](const Impl::ValueHandle<Instance_, vertex_descriptor>& h) {
            return h.mValue;
        }
    ), g.mVertices[u].mHandle);
}

[[nodiscard]] inline SyntaxGraph::vertex_tag_type
tag(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    using vertex_descriptor = SyntaxGraph::vertex_descriptor;
    return visit(overload(
        [](const Impl::ValueHandle<Define_, vertex_descriptor>&) {
            return SyntaxGraph::vertex_tag_type{ Define_{} };
        },
        [](const Impl::ValueHandle<Namespace_, Namespace>&) {
            return SyntaxGraph::vertex_tag_type{ Namespace_{} };
        },
        [](const Impl::ValueHandle<Concept_, vertex_descriptor>&) {
            return SyntaxGraph::vertex_tag_type{ Concept_{} };
        },
        [](const Impl::ValueHandle<Alias_, vertex_descriptor>&) {
            return SyntaxGraph::vertex_tag_type{ Alias_{} };
        },
        [](const Impl::ValueHandle<Value_, Value>&) {
            return SyntaxGraph::vertex_tag_type{ Value_{} };
        },
        [](const Impl::ValueHandle<Enum_, vertex_descriptor>&) {
            return SyntaxGraph::vertex_tag_type{ Enum_{} };
        },
        [](const Impl::ValueHandle<Tag_, vertex_descriptor>&) {
            return SyntaxGraph::vertex_tag_type{ Tag_{} };
        },
        [](const Impl::ValueHandle<Struct_, vertex_descriptor>&) {
            return SyntaxGraph::vertex_tag_type{ Struct_{} };
        },
        [](const Impl::ValueHandle<Graph_, vertex_descriptor>&) {
            return SyntaxGraph::vertex_tag_type{ Graph_{} };
        },
        [](const Impl::ValueHandle<Variant_, vertex_descriptor>&) {
            return SyntaxGraph::vertex_tag_type{ Variant_{} };
        },
        [](const Impl::ValueHandle<Container_, Container>&) {
            return SyntaxGraph::vertex_tag_type{ Container_{} };
        },
        [](const Impl::ValueHandle<Map_, Map>&) {
            return SyntaxGraph::vertex_tag_type{ Map_{} };
        },
        [](const Impl::ValueHandle<Instance_, vertex_descriptor>&) {
            return SyntaxGraph::vertex_tag_type{ Instance_{} };
        }
    ), g.mVertices[u].mHandle);
}

[[nodiscard]] inline SyntaxGraph::vertex_value_type
value(SyntaxGraph::vertex_descriptor u, SyntaxGraph& g) noexcept {
    using vertex_descriptor = SyntaxGraph::vertex_descriptor;
    return visit(overload(
        [&](const Impl::ValueHandle<Define_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_value_type{ &g.mDefines[h.mValue] };
        },
        [&](Impl::ValueHandle<Namespace_, Namespace>& h) {
            return SyntaxGraph::vertex_value_type{ &h.mValue };
        },
        [&](const Impl::ValueHandle<Concept_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_value_type{ &g.mConcepts[h.mValue] };
        },
        [&](const Impl::ValueHandle<Alias_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_value_type{ &g.mAliases[h.mValue] };
        },
        [&](Impl::ValueHandle<Value_, Value>& h) {
            return SyntaxGraph::vertex_value_type{ &h.mValue };
        },
        [&](const Impl::ValueHandle<Enum_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_value_type{ &g.mEnums[h.mValue] };
        },
        [&](const Impl::ValueHandle<Tag_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_value_type{ &g.mTags[h.mValue] };
        },
        [&](const Impl::ValueHandle<Struct_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_value_type{ &g.mStructs[h.mValue] };
        },
        [&](const Impl::ValueHandle<Graph_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_value_type{ &g.mGraphs[h.mValue] };
        },
        [&](const Impl::ValueHandle<Variant_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_value_type{ &g.mVariants[h.mValue] };
        },
        [&](Impl::ValueHandle<Container_, Container>& h) {
            return SyntaxGraph::vertex_value_type{ &h.mValue };
        },
        [&](Impl::ValueHandle<Map_, Map>& h) {
            return SyntaxGraph::vertex_value_type{ &h.mValue };
        },
        [&](const Impl::ValueHandle<Instance_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_value_type{ &g.mInstances[h.mValue] };
        }
    ), g.mVertices[u].mHandle);
}

[[nodiscard]] inline SyntaxGraph::vertex_const_value_type
value(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    using vertex_descriptor = SyntaxGraph::vertex_descriptor;
    return visit(overload(
        [&](const Impl::ValueHandle<Define_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_const_value_type{ &g.mDefines[h.mValue] };
        },
        [&](const Impl::ValueHandle<Namespace_, Namespace>& h) {
            return SyntaxGraph::vertex_const_value_type{ &h.mValue };
        },
        [&](const Impl::ValueHandle<Concept_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_const_value_type{ &g.mConcepts[h.mValue] };
        },
        [&](const Impl::ValueHandle<Alias_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_const_value_type{ &g.mAliases[h.mValue] };
        },
        [&](const Impl::ValueHandle<Value_, Value>& h) {
            return SyntaxGraph::vertex_const_value_type{ &h.mValue };
        },
        [&](const Impl::ValueHandle<Enum_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_const_value_type{ &g.mEnums[h.mValue] };
        },
        [&](const Impl::ValueHandle<Tag_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_const_value_type{ &g.mTags[h.mValue] };
        },
        [&](const Impl::ValueHandle<Struct_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_const_value_type{ &g.mStructs[h.mValue] };
        },
        [&](const Impl::ValueHandle<Graph_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_const_value_type{ &g.mGraphs[h.mValue] };
        },
        [&](const Impl::ValueHandle<Variant_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_const_value_type{ &g.mVariants[h.mValue] };
        },
        [&](const Impl::ValueHandle<Container_, Container>& h) {
            return SyntaxGraph::vertex_const_value_type{ &h.mValue };
        },
        [&](const Impl::ValueHandle<Map_, Map>& h) {
            return SyntaxGraph::vertex_const_value_type{ &h.mValue };
        },
        [&](const Impl::ValueHandle<Instance_, vertex_descriptor>& h) {
            return SyntaxGraph::vertex_const_value_type{ &g.mInstances[h.mValue] };
        }
    ), g.mVertices[u].mHandle);
}

template <class Tag>
[[nodiscard]] inline bool
holds_tag(SyntaxGraph::vertex_descriptor v, const SyntaxGraph& g) noexcept {
    using vertex_descriptor = SyntaxGraph::vertex_descriptor;
    if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Define_>) {
        return std::holds_alternative<Impl::ValueHandle<Define_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Namespace_>) {
        return std::holds_alternative<Impl::ValueHandle<Namespace_, Namespace>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Concept_>) {
        return std::holds_alternative<Impl::ValueHandle<Concept_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Alias_>) {
        return std::holds_alternative<Impl::ValueHandle<Alias_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Value_>) {
        return std::holds_alternative<Impl::ValueHandle<Value_, Value>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Enum_>) {
        return std::holds_alternative<Impl::ValueHandle<Enum_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Tag_>) {
        return std::holds_alternative<Impl::ValueHandle<Tag_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Struct_>) {
        return std::holds_alternative<Impl::ValueHandle<Struct_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Graph_>) {
        return std::holds_alternative<Impl::ValueHandle<Graph_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Variant_>) {
        return std::holds_alternative<Impl::ValueHandle<Variant_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Container_>) {
        return std::holds_alternative<Impl::ValueHandle<Container_, Container>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Map_>) {
        return std::holds_alternative<Impl::ValueHandle<Map_, Map>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Instance_>) {
        return std::holds_alternative<Impl::ValueHandle<Instance_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else {
        [] <bool flag = false>() {
            static_assert(flag, "Tag type is not in PolymorphicGraph");
        }();
    }
    return false;
}

template <class ValueT>
[[nodiscard]] inline bool
holds_alternative(SyntaxGraph::vertex_descriptor v, const SyntaxGraph& g) noexcept {
    using vertex_descriptor = SyntaxGraph::vertex_descriptor;
    if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Define>) {
        return std::holds_alternative<Impl::ValueHandle<Define_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Namespace>) {
        return std::holds_alternative<Impl::ValueHandle<Namespace_, Namespace>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Concept>) {
        return std::holds_alternative<Impl::ValueHandle<Concept_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Alias>) {
        return std::holds_alternative<Impl::ValueHandle<Alias_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Value>) {
        return std::holds_alternative<Impl::ValueHandle<Value_, Value>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Enum>) {
        return std::holds_alternative<Impl::ValueHandle<Enum_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Tag>) {
        return std::holds_alternative<Impl::ValueHandle<Tag_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Struct>) {
        return std::holds_alternative<Impl::ValueHandle<Struct_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Graph>) {
        return std::holds_alternative<Impl::ValueHandle<Graph_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Variant>) {
        return std::holds_alternative<Impl::ValueHandle<Variant_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Container>) {
        return std::holds_alternative<Impl::ValueHandle<Container_, Container>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Map>) {
        return std::holds_alternative<Impl::ValueHandle<Map_, Map>>(g.mVertices[v].mHandle);
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Instance>) {
        return std::holds_alternative<Impl::ValueHandle<Instance_, vertex_descriptor>>(g.mVertices[v].mHandle);
    } else {
        [] <bool flag = false>() {
            static_assert(flag, "Value type is not in PolymorphicGraph");
        }();
    }
    return false;
}

template <class ValueT>
[[nodiscard]] inline ValueT&
get(SyntaxGraph::vertex_descriptor v, SyntaxGraph& g) noexcept {
    using vertex_descriptor = SyntaxGraph::vertex_descriptor;
    if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Define>) {
        auto& handle = std::get<Impl::ValueHandle<Define_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mDefines[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Namespace>) {
        auto& handle = std::get<Impl::ValueHandle<Namespace_, Namespace>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Concept>) {
        auto& handle = std::get<Impl::ValueHandle<Concept_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mConcepts[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Alias>) {
        auto& handle = std::get<Impl::ValueHandle<Alias_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mAliases[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Value>) {
        auto& handle = std::get<Impl::ValueHandle<Value_, Value>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Enum>) {
        auto& handle = std::get<Impl::ValueHandle<Enum_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mEnums[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Tag>) {
        auto& handle = std::get<Impl::ValueHandle<Tag_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mTags[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Struct>) {
        auto& handle = std::get<Impl::ValueHandle<Struct_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mStructs[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Graph>) {
        auto& handle = std::get<Impl::ValueHandle<Graph_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mGraphs[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Variant>) {
        auto& handle = std::get<Impl::ValueHandle<Variant_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mVariants[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Container>) {
        auto& handle = std::get<Impl::ValueHandle<Container_, Container>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Map>) {
        auto& handle = std::get<Impl::ValueHandle<Map_, Map>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Instance>) {
        auto& handle = std::get<Impl::ValueHandle<Instance_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mInstances[handle.mValue];
    } else {
        [] <bool flag = false>() {
            static_assert(flag, "Value type is not in PolymorphicGraph");
        }();
    }
}

template <class ValueT>
[[nodiscard]] inline const ValueT&
get(SyntaxGraph::vertex_descriptor v, const SyntaxGraph& g) noexcept {
    using vertex_descriptor = SyntaxGraph::vertex_descriptor;
    if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Define>) {
        auto& handle = std::get<Impl::ValueHandle<Define_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mDefines[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Namespace>) {
        auto& handle = std::get<Impl::ValueHandle<Namespace_, Namespace>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Concept>) {
        auto& handle = std::get<Impl::ValueHandle<Concept_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mConcepts[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Alias>) {
        auto& handle = std::get<Impl::ValueHandle<Alias_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mAliases[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Value>) {
        auto& handle = std::get<Impl::ValueHandle<Value_, Value>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Enum>) {
        auto& handle = std::get<Impl::ValueHandle<Enum_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mEnums[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Tag>) {
        auto& handle = std::get<Impl::ValueHandle<Tag_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mTags[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Struct>) {
        auto& handle = std::get<Impl::ValueHandle<Struct_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mStructs[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Graph>) {
        auto& handle = std::get<Impl::ValueHandle<Graph_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mGraphs[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Variant>) {
        auto& handle = std::get<Impl::ValueHandle<Variant_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mVariants[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Container>) {
        auto& handle = std::get<Impl::ValueHandle<Container_, Container>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Map>) {
        auto& handle = std::get<Impl::ValueHandle<Map_, Map>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Instance>) {
        auto& handle = std::get<Impl::ValueHandle<Instance_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mInstances[handle.mValue];
    } else {
        [] <bool flag = false>() {
            static_assert(flag, "Value type is not in PolymorphicGraph");
        }();
    }
}

template <class Tag>
[[nodiscard]] inline auto&
get_by_tag(SyntaxGraph::vertex_descriptor v, SyntaxGraph& g) noexcept {
    using vertex_descriptor = SyntaxGraph::vertex_descriptor;
    if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Define_>) {
        auto& handle = std::get<Impl::ValueHandle<Define_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mDefines[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Namespace_>) {
        auto& handle = std::get<Impl::ValueHandle<Namespace_, Namespace>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Concept_>) {
        auto& handle = std::get<Impl::ValueHandle<Concept_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mConcepts[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Alias_>) {
        auto& handle = std::get<Impl::ValueHandle<Alias_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mAliases[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Value_>) {
        auto& handle = std::get<Impl::ValueHandle<Value_, Value>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Enum_>) {
        auto& handle = std::get<Impl::ValueHandle<Enum_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mEnums[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Tag_>) {
        auto& handle = std::get<Impl::ValueHandle<Tag_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mTags[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Struct_>) {
        auto& handle = std::get<Impl::ValueHandle<Struct_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mStructs[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Graph_>) {
        auto& handle = std::get<Impl::ValueHandle<Graph_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mGraphs[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Variant_>) {
        auto& handle = std::get<Impl::ValueHandle<Variant_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mVariants[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Container_>) {
        auto& handle = std::get<Impl::ValueHandle<Container_, Container>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Map_>) {
        auto& handle = std::get<Impl::ValueHandle<Map_, Map>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Instance_>) {
        auto& handle = std::get<Impl::ValueHandle<Instance_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mInstances[handle.mValue];
    } else {
        [] <bool flag = false>() {
            static_assert(flag, "Value type is not in PolymorphicGraph");
        }();
    }
}

template <class Tag>
[[nodiscard]] inline const auto&
get_by_tag(SyntaxGraph::vertex_descriptor v, const SyntaxGraph& g) noexcept {
    using vertex_descriptor = SyntaxGraph::vertex_descriptor;
    if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Define_>) {
        auto& handle = std::get<Impl::ValueHandle<Define_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mDefines[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Namespace_>) {
        auto& handle = std::get<Impl::ValueHandle<Namespace_, Namespace>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Concept_>) {
        auto& handle = std::get<Impl::ValueHandle<Concept_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mConcepts[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Alias_>) {
        auto& handle = std::get<Impl::ValueHandle<Alias_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mAliases[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Value_>) {
        auto& handle = std::get<Impl::ValueHandle<Value_, Value>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Enum_>) {
        auto& handle = std::get<Impl::ValueHandle<Enum_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mEnums[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Tag_>) {
        auto& handle = std::get<Impl::ValueHandle<Tag_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mTags[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Struct_>) {
        auto& handle = std::get<Impl::ValueHandle<Struct_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mStructs[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Graph_>) {
        auto& handle = std::get<Impl::ValueHandle<Graph_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mGraphs[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Variant_>) {
        auto& handle = std::get<Impl::ValueHandle<Variant_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mVariants[handle.mValue];
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Container_>) {
        auto& handle = std::get<Impl::ValueHandle<Container_, Container>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Map_>) {
        auto& handle = std::get<Impl::ValueHandle<Map_, Map>>(g.mVertices[v].mHandle);
        return handle.mValue;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Instance_>) {
        auto& handle = std::get<Impl::ValueHandle<Instance_, vertex_descriptor>>(g.mVertices[v].mHandle);
        return g.mInstances[handle.mValue];
    } else {
        [] <bool flag = false>() {
            static_assert(flag, "Value type is not in PolymorphicGraph");
        }();
    }
}

template <class ValueT>
[[nodiscard]] inline ValueT*
get_if(SyntaxGraph::vertex_descriptor v, SyntaxGraph* pGraph) noexcept {
    using vertex_descriptor = SyntaxGraph::vertex_descriptor;
    ValueT* ptr = nullptr;

    if (!pGraph) {
        return ptr;
    }
    auto& g = *pGraph;

    if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Define>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Define_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mDefines[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Namespace>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Namespace_, Namespace>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &pHandle->mValue;
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Concept>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Concept_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mConcepts[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Alias>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Alias_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mAliases[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Value>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Value_, Value>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &pHandle->mValue;
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Enum>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Enum_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mEnums[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Tag>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Tag_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mTags[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Struct>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Struct_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mStructs[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Graph>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Graph_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mGraphs[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Variant>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Variant_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mVariants[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Container>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Container_, Container>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &pHandle->mValue;
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Map>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Map_, Map>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &pHandle->mValue;
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Instance>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Instance_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mInstances[pHandle->mValue];
        }
    } else {
        [] <bool flag = false>() {
            static_assert(flag, "Value type is not in PolymorphicGraph");
        }();
    }
    return ptr;
}

template <class ValueT>
[[nodiscard]] inline const ValueT*
get_if(SyntaxGraph::vertex_descriptor v, const SyntaxGraph* pGraph) noexcept {
    using vertex_descriptor = SyntaxGraph::vertex_descriptor;
    const ValueT* ptr = nullptr;

    if (!pGraph) {
        return ptr;
    }
    auto& g = *pGraph;

    if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Define>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Define_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mDefines[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Namespace>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Namespace_, Namespace>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &pHandle->mValue;
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Concept>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Concept_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mConcepts[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Alias>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Alias_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mAliases[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Value>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Value_, Value>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &pHandle->mValue;
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Enum>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Enum_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mEnums[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Tag>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Tag_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mTags[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Struct>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Struct_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mStructs[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Graph>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Graph_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mGraphs[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Variant>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Variant_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mVariants[pHandle->mValue];
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Container>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Container_, Container>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &pHandle->mValue;
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Map>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Map_, Map>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &pHandle->mValue;
        }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Instance>) {
        auto* pHandle = std::get_if<Impl::ValueHandle<Instance_, vertex_descriptor>>(&g.mVertices[v].mHandle);
        if (pHandle) {
            ptr = &g.mInstances[pHandle->mValue];
        }
    } else {
        [] <bool flag = false>() {
            static_assert(flag, "Value type is not in PolymorphicGraph");
        }();
    }
    return ptr;
}

// Vertex Constant Getter
template <class Tag>
[[nodiscard]] inline decltype(auto)
get(Tag tag, const SyntaxGraph& g, SyntaxGraph::vertex_descriptor v) noexcept {
    return get(get(tag, g), v);
}

// Vertex Mutable Getter
template <class Tag>
[[nodiscard]] inline decltype(auto)
get(Tag tag, SyntaxGraph& g, SyntaxGraph::vertex_descriptor v) noexcept {
    return get(get(tag, g), v);
}

// Vertex Setter
template <class Tag, class... Args>
inline void put(Tag tag, SyntaxGraph& g,
    SyntaxGraph::vertex_descriptor v,
    Args&&... args) {
    put(get(tag, g), v, std::forward<Args>(args)...);
}

// AddressableGraph
[[nodiscard]] inline std::ptrdiff_t
path_length(SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g) noexcept {
    return Impl::pathLength(u, g);
}

template <class Allocator>
inline void
path_composite(std::basic_string<char, std::char_traits<char>, Allocator>& str,
    std::ptrdiff_t& sz, SyntaxGraph::vertex_descriptor u, const SyntaxGraph& g
) noexcept {
    Impl::pathComposite(str, sz, u, g);
}

template <class Allocator>
inline const std::basic_string<char, std::char_traits<char>, Allocator>&
get_path(std::basic_string<char, std::char_traits<char>, Allocator>& output,
    SyntaxGraph::vertex_descriptor u0, const SyntaxGraph& g,
    std::string_view prefix = {},
    SyntaxGraph::vertex_descriptor parent = SyntaxGraph::null_vertex()) {
    output.clear();
    const auto sz0 = std::ssize(prefix);
    auto sz = sz0;

    const auto& syntaxGraph = g;
    sz += Impl::pathLength(u0, syntaxGraph, parent);

    output.resize(sz);

    Impl::pathComposite(output, sz, u0, syntaxGraph, parent);
    Ensures(sz >= sz0);

    Ensures(sz == sz0);
    std::copy(prefix.begin(), prefix.end(), output.begin());

    return output;
}

[[nodiscard]] inline std::string
get_path(SyntaxGraph::vertex_descriptor u0, const SyntaxGraph& g,
    std::string_view prefix = {},
    SyntaxGraph::vertex_descriptor parent = SyntaxGraph::null_vertex()) {
    std::string output;
    get_path(output, u0, g, prefix, parent);
    return output;
}

[[nodiscard]] inline std::pmr::string
get_path(SyntaxGraph::vertex_descriptor u0, const SyntaxGraph& g,
    std::pmr::memory_resource* mr, std::string_view prefix = {},
    SyntaxGraph::vertex_descriptor parent = SyntaxGraph::null_vertex()) {
    std::pmr::string output(mr);
    get_path(output, u0, g, prefix, parent);
    return output;
}

template <class Allocator>
inline const std::basic_string<char, std::char_traits<char>, Allocator>&
get_path(std::basic_string<char, std::char_traits<char>, Allocator>& output,
    SyntaxGraph::vertex_descriptor parent, std::string_view name, const SyntaxGraph& g) {
    output.clear();
    auto sz = path_length(parent, g);
    output.resize(sz + name.size() + 1);
    output[sz] = '/';
    std::copy(name.begin(), name.end(), output.begin() + sz + 1);
    path_composite(output, sz, parent, g);
    Ensures(sz == 0);
    return output;
}

[[nodiscard]] inline std::string
get_path(SyntaxGraph::vertex_descriptor parent, std::string_view name, const SyntaxGraph& g) {
    std::string output;
    get_path(output, parent, name, g);
    return output;
}

[[nodiscard]] inline std::pmr::string
get_path(SyntaxGraph::vertex_descriptor parent, std::string_view name, const SyntaxGraph& g,
    std::pmr::memory_resource* mr) {
    std::pmr::string output(mr);
    get_path(output, parent, name, g);
    return output;
}

[[nodiscard]] inline SyntaxGraph::vertex_descriptor
locate(std::string_view absolute, const SyntaxGraph& g) noexcept {
    auto iter = g.mPathIndex.find(absolute);
    if (iter != g.mPathIndex.end()) {
         return iter->second;
    } else {
         return SyntaxGraph::null_vertex();
    }
};

[[nodiscard]] inline SyntaxGraph::vertex_descriptor
locate(SyntaxGraph::vertex_descriptor u, std::string_view relative, const SyntaxGraph& g) {
    Expects(!relative.starts_with('/'));
    Expects(!relative.ends_with('/'));
    auto key = get_path(u, relative, g);
    cleanPath(key);
    return locate(key, g);
};

[[nodiscard]] inline SyntaxGraph::vertex_descriptor
locate(SyntaxGraph::vertex_descriptor u, std::string_view relative, const SyntaxGraph& g,
    std::pmr::memory_resource* mr) {
    Expects(!relative.starts_with('/'));
    Expects(!relative.ends_with('/'));
    auto key = get_path(u, relative, g, mr);
    cleanPath(key);
    return locate(key, g);
};

[[nodiscard]] inline bool
contains(std::string_view absolute, const SyntaxGraph& g) noexcept {
    return locate(absolute, g) != SyntaxGraph::null_vertex();
}

template <class ValueT>
[[nodiscard]] inline ValueT&
get(std::string_view pt, SyntaxGraph& g) {
    auto v = locate(pt, g);
    if (v == SyntaxGraph::null_vertex()) {
        throw std::out_of_range("at SyntaxGraph");
    }
    return get<ValueT>(v, g);
}

template <class ValueT>
[[nodiscard]] inline const ValueT&
get(std::string_view pt, const SyntaxGraph& g) {
    auto v = locate(pt, g);
    if (v == SyntaxGraph::null_vertex()) {
        throw std::out_of_range("at SyntaxGraph");
    }
    return get<ValueT>(v, g);
}

template <class ValueT>
[[nodiscard]] inline ValueT*
get_if(std::string_view pt, SyntaxGraph* pGraph) noexcept {
    if (pGraph) {
        auto v = locate(pt, *pGraph);
        if (v != SyntaxGraph::null_vertex()) {
            return get_if<ValueT>(v, pGraph);
        }
    }
    return nullptr;
}

template <class ValueT>
[[nodiscard]] inline const ValueT*
get_if(std::string_view pt, const SyntaxGraph* pGraph) {
    if (pGraph) {
        auto v = locate(pt, *pGraph);
        if (v != SyntaxGraph::null_vertex()) {
            return get_if<ValueT>(v, pGraph);
        }
    }
    return nullptr;
}

// MutableGraph(Vertex)
inline void add_path_impl(SyntaxGraph::vertex_descriptor u, SyntaxGraph::vertex_descriptor v, SyntaxGraph& g) {
    // add to parent
    if (u != SyntaxGraph::null_vertex()) {
        auto& outEdgeList = g.children_list(u);
        outEdgeList.emplace_back(v);

        auto& inEdgeList = g.parents_list(v);
        inEdgeList.emplace_back(u);
    }

    // add to external path index
    auto pathName = get_path(v, g, g.mPathIndex.get_allocator().resource());
    auto res = g.mPathIndex.emplace(std::move(pathName), v);
    Ensures(res.second);
}

inline void remove_path_impl(SyntaxGraph::vertex_descriptor u, SyntaxGraph& g) noexcept {
    // notice: here we use std::string, not std::pmr::string
    // we do not want to increase the memory of g
    auto pathName = get_path(u, g);
    auto iter = g.mPathIndex.find(std::string_view(pathName));
    Expects(iter != g.mPathIndex.end());
    g.mPathIndex.erase(iter);
    for (auto&& [name, v] : g.mPathIndex) {
        if (v > u) {
            --v;
        }
    }
}

inline void clear_out_edges(SyntaxGraph::vertex_descriptor u, SyntaxGraph& g) noexcept {
    // Bidirectional (OutEdges)
    auto& outEdgeList = g.out_edge_list(u);
    auto outEnd = outEdgeList.end();
    for (auto iter = outEdgeList.begin(); iter != outEnd; ++iter) {
        auto& inEdgeList = g.in_edge_list((*iter).get_target());
        // eraseFromIncidenceList
        Impl::sequenceEraseIf(inEdgeList, [u](const auto& e) {
            return e.get_target() == u;
        });
    }
    outEdgeList.clear();
}

inline void clear_in_edges(SyntaxGraph::vertex_descriptor u, SyntaxGraph& g) noexcept {
    // Bidirectional (InEdges)
    auto& inEdgeList = g.in_edge_list(u);
    auto inEnd = inEdgeList.end();
    for (auto iter = inEdgeList.begin(); iter != inEnd; ++iter) {
        auto& outEdgeList = g.out_edge_list((*iter).get_target());
        // eraseFromIncidenceList
        Impl::sequenceEraseIf(outEdgeList, [u](const auto& e) {
            return e.get_target() == u;
        });
    }
    inEdgeList.clear();
}

inline void clear_vertex(SyntaxGraph::vertex_descriptor u, SyntaxGraph& g) noexcept {
    clear_out_edges(u, g);
    clear_in_edges(u, g);
}

inline void remove_vertex_value_impl(const SyntaxGraph::vertex_handle_type& h, SyntaxGraph& g) noexcept {
    using vertex_descriptor = SyntaxGraph::vertex_descriptor;
    visit(overload(
        [&](const Impl::ValueHandle<Define_, vertex_descriptor>& h) {
            g.mDefines.erase(g.mDefines.begin() + h.mValue);
            if (h.mValue == g.mDefines.size()) {
                return;
            }
            Impl::reindexVectorHandle<Define_>(g.mVertices, h.mValue);
        },
        [&](const Impl::ValueHandle<Namespace_, Namespace>& h) {},
        [&](const Impl::ValueHandle<Concept_, vertex_descriptor>& h) {
            g.mConcepts.erase(g.mConcepts.begin() + h.mValue);
            if (h.mValue == g.mConcepts.size()) {
                return;
            }
            Impl::reindexVectorHandle<Concept_>(g.mVertices, h.mValue);
        },
        [&](const Impl::ValueHandle<Alias_, vertex_descriptor>& h) {
            g.mAliases.erase(g.mAliases.begin() + h.mValue);
            if (h.mValue == g.mAliases.size()) {
                return;
            }
            Impl::reindexVectorHandle<Alias_>(g.mVertices, h.mValue);
        },
        [&](const Impl::ValueHandle<Value_, Value>& h) {},
        [&](const Impl::ValueHandle<Enum_, vertex_descriptor>& h) {
            g.mEnums.erase(g.mEnums.begin() + h.mValue);
            if (h.mValue == g.mEnums.size()) {
                return;
            }
            Impl::reindexVectorHandle<Enum_>(g.mVertices, h.mValue);
        },
        [&](const Impl::ValueHandle<Tag_, vertex_descriptor>& h) {
            g.mTags.erase(g.mTags.begin() + h.mValue);
            if (h.mValue == g.mTags.size()) {
                return;
            }
            Impl::reindexVectorHandle<Tag_>(g.mVertices, h.mValue);
        },
        [&](const Impl::ValueHandle<Struct_, vertex_descriptor>& h) {
            g.mStructs.erase(g.mStructs.begin() + h.mValue);
            if (h.mValue == g.mStructs.size()) {
                return;
            }
            Impl::reindexVectorHandle<Struct_>(g.mVertices, h.mValue);
        },
        [&](const Impl::ValueHandle<Graph_, vertex_descriptor>& h) {
            g.mGraphs.erase(g.mGraphs.begin() + h.mValue);
            if (h.mValue == g.mGraphs.size()) {
                return;
            }
            Impl::reindexVectorHandle<Graph_>(g.mVertices, h.mValue);
        },
        [&](const Impl::ValueHandle<Variant_, vertex_descriptor>& h) {
            g.mVariants.erase(g.mVariants.begin() + h.mValue);
            if (h.mValue == g.mVariants.size()) {
                return;
            }
            Impl::reindexVectorHandle<Variant_>(g.mVertices, h.mValue);
        },
        [&](const Impl::ValueHandle<Container_, Container>& h) {},
        [&](const Impl::ValueHandle<Map_, Map>& h) {},
        [&](const Impl::ValueHandle<Instance_, vertex_descriptor>& h) {
            g.mInstances.erase(g.mInstances.begin() + h.mValue);
            if (h.mValue == g.mInstances.size()) {
                return;
            }
            Impl::reindexVectorHandle<Instance_>(g.mVertices, h.mValue);
        }
    ), h);
}

inline void remove_vertex(SyntaxGraph::vertex_descriptor u, SyntaxGraph& g) noexcept {
    { // AddressableGraph (Separated)
        remove_path_impl(u, g);
        Expects(num_children(u, g) == 0);

        // AddressableGraph (Parents)
        auto& parentsList = g.parents_list(u);
        auto inEnd = parentsList.end();
        for (auto iter = parentsList.begin(); iter != inEnd; ++iter) {
            auto& childrenList = g.children_list((*iter).get_target());
            // eraseFromIncidenceList
            Impl::sequenceEraseIf(childrenList, [u](const auto& e) {
                return e.get_target() == u;
            });
        }
        parentsList.clear();

        Impl::removeVectorOwner(g, u);
    }

    // preserve vertex' iterators
    auto& vert = g.mVertices[u];
    remove_vertex_value_impl(vert.mHandle, g);
    Impl::removeVectorVertex(const_cast<SyntaxGraph&>(g), u, SyntaxGraph::directed_category{});

    // remove components
    g.mNames.erase(g.mNames.begin() + u);
    g.mTraits.erase(g.mTraits.begin() + u);
    g.mConstraints.erase(g.mConstraints.begin() + u);
    g.mInherits.erase(g.mInherits.begin() + u);
    g.mModulePaths.erase(g.mModulePaths.begin() + u);
    g.mTypescripts.erase(g.mTypescripts.begin() + u);
    g.mComments.erase(g.mComments.begin() + u);
}

// MutablePropertyGraph(Vertex)
template <class Component0, class Component1, class Component2, class Component3, class Component4, class Component5, class Component6, class ValueT>
inline SyntaxGraph::vertex_descriptor
add_vertex(Component0&& c0, Component1&& c1, Component2&& c2, Component3&& c3, Component4&& c4, Component5&& c5, Component6&& c6, ValueT&& val, SyntaxGraph& g, SyntaxGraph::vertex_descriptor u = SyntaxGraph::null_vertex()) {
    auto v = gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mVertices.size());

    g.mObjects.emplace_back();

    g.mVertices.emplace_back();
    auto& vert = g.mVertices.back();
    g.mNames.emplace_back(std::forward<Component0>(c0));
    g.mTraits.emplace_back(std::forward<Component1>(c1));
    g.mConstraints.emplace_back(std::forward<Component2>(c2));
    g.mInherits.emplace_back(std::forward<Component3>(c3));
    g.mModulePaths.emplace_back(std::forward<Component4>(c4));
    g.mTypescripts.emplace_back(std::forward<Component5>(c5));
    g.mComments.emplace_back(std::forward<Component6>(c6));

    // PolymorphicGraph
    if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Define>) {
        vert.mHandle = Impl::ValueHandle<Define_, SyntaxGraph::vertex_descriptor>{
            gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mDefines.size())
        };
        g.mDefines.emplace_back(std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Namespace>) {
        vert.mHandle = Impl::ValueHandle<Namespace_, Namespace>{ std::forward<ValueT>(val) };
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Concept>) {
        vert.mHandle = Impl::ValueHandle<Concept_, SyntaxGraph::vertex_descriptor>{
            gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mConcepts.size())
        };
        g.mConcepts.emplace_back(std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Alias>) {
        vert.mHandle = Impl::ValueHandle<Alias_, SyntaxGraph::vertex_descriptor>{
            gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mAliases.size())
        };
        g.mAliases.emplace_back(std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Value>) {
        vert.mHandle = Impl::ValueHandle<Value_, Value>{ std::forward<ValueT>(val) };
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Enum>) {
        vert.mHandle = Impl::ValueHandle<Enum_, SyntaxGraph::vertex_descriptor>{
            gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mEnums.size())
        };
        g.mEnums.emplace_back(std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Tag>) {
        vert.mHandle = Impl::ValueHandle<Tag_, SyntaxGraph::vertex_descriptor>{
            gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mTags.size())
        };
        g.mTags.emplace_back(std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Struct>) {
        vert.mHandle = Impl::ValueHandle<Struct_, SyntaxGraph::vertex_descriptor>{
            gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mStructs.size())
        };
        g.mStructs.emplace_back(std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Graph>) {
        vert.mHandle = Impl::ValueHandle<Graph_, SyntaxGraph::vertex_descriptor>{
            gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mGraphs.size())
        };
        g.mGraphs.emplace_back(std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Variant>) {
        vert.mHandle = Impl::ValueHandle<Variant_, SyntaxGraph::vertex_descriptor>{
            gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mVariants.size())
        };
        g.mVariants.emplace_back(std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Container>) {
        vert.mHandle = Impl::ValueHandle<Container_, Container>{ std::forward<ValueT>(val) };
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Map>) {
        vert.mHandle = Impl::ValueHandle<Map_, Map>{ std::forward<ValueT>(val) };
    } else if constexpr (std::is_same_v<std::remove_cvref_t<ValueT>, Instance>) {
        vert.mHandle = Impl::ValueHandle<Instance_, SyntaxGraph::vertex_descriptor>{
            gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mInstances.size())
        };
        g.mInstances.emplace_back(std::forward<ValueT>(val));
    } else {
        [] <bool flag = false>() {
            static_assert(flag, "value not found in graph");
        }();
    }

    // AddressableGraph
    add_path_impl(u, v, g);

    return v;
}

template <class Component0, class Component1, class Component2, class Component3, class Component4, class Component5, class Component6, class Tag, class ValueT>
inline SyntaxGraph::vertex_descriptor
add_vertex(Tag, Component0&& c0, Component1&& c1, Component2&& c2, Component3&& c3, Component4&& c4, Component5&& c5, Component6&& c6, ValueT&& val, SyntaxGraph& g, SyntaxGraph::vertex_descriptor u = SyntaxGraph::null_vertex()) {
    auto v = gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mVertices.size());

    g.mObjects.emplace_back();

    g.mVertices.emplace_back();
    auto& vert = g.mVertices.back();

    std::apply([&]<typename... T>(T&&... args) {
        g.mNames.emplace_back(std::forward<T>(args)...);
    }, std::forward<Component0>(c0));

    std::apply([&]<typename... T>(T&&... args) {
        g.mTraits.emplace_back(std::forward<T>(args)...);
    }, std::forward<Component1>(c1));

    std::apply([&]<typename... T>(T&&... args) {
        g.mConstraints.emplace_back(std::forward<T>(args)...);
    }, std::forward<Component2>(c2));

    std::apply([&]<typename... T>(T&&... args) {
        g.mInherits.emplace_back(std::forward<T>(args)...);
    }, std::forward<Component3>(c3));

    std::apply([&]<typename... T>(T&&... args) {
        g.mModulePaths.emplace_back(std::forward<T>(args)...);
    }, std::forward<Component4>(c4));

    std::apply([&]<typename... T>(T&&... args) {
        g.mTypescripts.emplace_back(std::forward<T>(args)...);
    }, std::forward<Component5>(c5));

    std::apply([&]<typename... T>(T&&... args) {
        g.mComments.emplace_back(std::forward<T>(args)...);
    }, std::forward<Component6>(c6));

    // PolymorphicGraph
    if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Define_>) {
        std::apply([&]<typename... T>(T&&... args) {
            vert.mHandle = Impl::ValueHandle<Define_, SyntaxGraph::vertex_descriptor>{
                gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mDefines.size())
            };
            g.mDefines.emplace_back(std::forward<T>(args)...);
        }, std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Namespace_>) {
        std::apply([&]<typename... T>(T&&... args) {
            vert.mHandle = Impl::ValueHandle<Namespace_, Namespace>{ std::forward<T>(args)... };
        }, std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Concept_>) {
        std::apply([&]<typename... T>(T&&... args) {
            vert.mHandle = Impl::ValueHandle<Concept_, SyntaxGraph::vertex_descriptor>{
                gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mConcepts.size())
            };
            g.mConcepts.emplace_back(std::forward<T>(args)...);
        }, std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Alias_>) {
        std::apply([&]<typename... T>(T&&... args) {
            vert.mHandle = Impl::ValueHandle<Alias_, SyntaxGraph::vertex_descriptor>{
                gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mAliases.size())
            };
            g.mAliases.emplace_back(std::forward<T>(args)...);
        }, std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Value_>) {
        std::apply([&]<typename... T>(T&&... args) {
            vert.mHandle = Impl::ValueHandle<Value_, Value>{ std::forward<T>(args)... };
        }, std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Enum_>) {
        std::apply([&]<typename... T>(T&&... args) {
            vert.mHandle = Impl::ValueHandle<Enum_, SyntaxGraph::vertex_descriptor>{
                gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mEnums.size())
            };
            g.mEnums.emplace_back(std::forward<T>(args)...);
        }, std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Tag_>) {
        std::apply([&]<typename... T>(T&&... args) {
            vert.mHandle = Impl::ValueHandle<Tag_, SyntaxGraph::vertex_descriptor>{
                gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mTags.size())
            };
            g.mTags.emplace_back(std::forward<T>(args)...);
        }, std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Struct_>) {
        std::apply([&]<typename... T>(T&&... args) {
            vert.mHandle = Impl::ValueHandle<Struct_, SyntaxGraph::vertex_descriptor>{
                gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mStructs.size())
            };
            g.mStructs.emplace_back(std::forward<T>(args)...);
        }, std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Graph_>) {
        std::apply([&]<typename... T>(T&&... args) {
            vert.mHandle = Impl::ValueHandle<Graph_, SyntaxGraph::vertex_descriptor>{
                gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mGraphs.size())
            };
            g.mGraphs.emplace_back(std::forward<T>(args)...);
        }, std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Variant_>) {
        std::apply([&]<typename... T>(T&&... args) {
            vert.mHandle = Impl::ValueHandle<Variant_, SyntaxGraph::vertex_descriptor>{
                gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mVariants.size())
            };
            g.mVariants.emplace_back(std::forward<T>(args)...);
        }, std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Container_>) {
        std::apply([&]<typename... T>(T&&... args) {
            vert.mHandle = Impl::ValueHandle<Container_, Container>{ std::forward<T>(args)... };
        }, std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Map_>) {
        std::apply([&]<typename... T>(T&&... args) {
            vert.mHandle = Impl::ValueHandle<Map_, Map>{ std::forward<T>(args)... };
        }, std::forward<ValueT>(val));
    } else if constexpr (std::is_same_v<std::remove_cvref_t<Tag>, Instance_>) {
        std::apply([&]<typename... T>(T&&... args) {
            vert.mHandle = Impl::ValueHandle<Instance_, SyntaxGraph::vertex_descriptor>{
                gsl::narrow_cast<SyntaxGraph::vertex_descriptor>(g.mInstances.size())
            };
            g.mInstances.emplace_back(std::forward<T>(args)...);
        }, std::forward<ValueT>(val));
    } else {
        [] <bool flag = false>() {
            static_assert(flag, "value not found in graph");
        }();
    }

    // AddressableGraph
    add_path_impl(u, v, g);

    return v;
}

// MutableGraph(Vertex)
template <class Tag>
inline SyntaxGraph::vertex_descriptor
add_vertex(SyntaxGraph& g, Tag t, std::string_view name, SyntaxGraph::vertex_descriptor parentID = SyntaxGraph::null_vertex()) {
    return add_vertex(t,
        std::forward_as_tuple(name), // mNames
        std::forward_as_tuple(), // mTraits
        std::forward_as_tuple(), // mConstraints
        std::forward_as_tuple(), // mInherits
        std::forward_as_tuple(), // mModulePaths
        std::forward_as_tuple(), // mTypescripts
        std::forward_as_tuple(), // mComments
        std::forward_as_tuple(), // PolymorphicType
        g, parentID);
}

template <class Tag>
inline SyntaxGraph::vertex_descriptor
add_vertex(SyntaxGraph& g, Tag t, std::pmr::string&& name, SyntaxGraph::vertex_descriptor parentID = SyntaxGraph::null_vertex()) {
    return add_vertex(t,
        std::forward_as_tuple(std::move(name)), // mNames
        std::forward_as_tuple(), // mTraits
        std::forward_as_tuple(), // mConstraints
        std::forward_as_tuple(), // mInherits
        std::forward_as_tuple(), // mModulePaths
        std::forward_as_tuple(), // mTypescripts
        std::forward_as_tuple(), // mComments
        std::forward_as_tuple(), // PolymorphicType
        g, parentID);
}

template <class Tag>
inline SyntaxGraph::vertex_descriptor
add_vertex(SyntaxGraph& g, Tag t, const char* name, SyntaxGraph::vertex_descriptor parentID = SyntaxGraph::null_vertex()) {
    return add_vertex(t,
        std::forward_as_tuple(name), // mNames
        std::forward_as_tuple(), // mTraits
        std::forward_as_tuple(), // mConstraints
        std::forward_as_tuple(), // mInherits
        std::forward_as_tuple(), // mModulePaths
        std::forward_as_tuple(), // mTypescripts
        std::forward_as_tuple(), // mComments
        std::forward_as_tuple(), // PolymorphicType
        g, parentID);
}

// Vertex Index
inline boost::property_map<ModuleGraph, boost::vertex_index_t>::const_type
get(boost::vertex_index_t tag, const ModuleGraph& g) noexcept {
    return {};
}

inline boost::property_map<ModuleGraph, boost::vertex_index_t>::type
get(boost::vertex_index_t tag, ModuleGraph& g) noexcept {
    return {};
}

[[nodiscard]] inline Impl::ColorMap<ModuleGraph::vertex_descriptor>
get(std::pmr::vector<boost::default_color_type>& colors, const ModuleGraph& g) noexcept {
    return { colors };
}

// Vertex Component
inline typename boost::property_map<ModuleGraph, ModuleGraph::names_>::const_type
get(ModuleGraph::names_, const ModuleGraph& g) noexcept {
    return { g.mNames };
}

inline typename boost::property_map<ModuleGraph, ModuleGraph::names_>::type
get(ModuleGraph::names_, ModuleGraph& g) noexcept {
    return { g.mNames };
}

// Vertex Name
inline boost::property_map<ModuleGraph, boost::vertex_name_t>::const_type
get(boost::vertex_name_t, const ModuleGraph& g) noexcept {
    return { g.mNames };
}

// Vertex Component
inline typename boost::property_map<ModuleGraph, ModuleGraph::modules_>::const_type
get(ModuleGraph::modules_, const ModuleGraph& g) noexcept {
    return { g.mModules };
}

inline typename boost::property_map<ModuleGraph, ModuleGraph::modules_>::type
get(ModuleGraph::modules_, ModuleGraph& g) noexcept {
    return { g.mModules };
}

// Vertex ComponentMember
template <class T>
inline typename boost::property_map<ModuleGraph, T ModuleInfo::*>::const_type
get(T ModuleInfo::* memberPointer, const ModuleGraph& g) noexcept {
    return { g.mModules, memberPointer };
}

template <class T>
inline typename boost::property_map<ModuleGraph, T ModuleInfo::*>::type
get(T ModuleInfo::* memberPointer, ModuleGraph& g) noexcept {
    return { g.mModules, memberPointer };
}

// Vertex Constant Getter
template <class Tag>
[[nodiscard]] inline decltype(auto)
get(Tag tag, const ModuleGraph& g, ModuleGraph::vertex_descriptor v) noexcept {
    return get(get(tag, g), v);
}

// Vertex Mutable Getter
template <class Tag>
[[nodiscard]] inline decltype(auto)
get(Tag tag, ModuleGraph& g, ModuleGraph::vertex_descriptor v) noexcept {
    return get(get(tag, g), v);
}

// Vertex Setter
template <class Tag, class... Args>
inline void put(Tag tag, ModuleGraph& g,
    ModuleGraph::vertex_descriptor v,
    Args&&... args) {
    put(get(tag, g), v, std::forward<Args>(args)...);
}

// AddressableGraph
[[nodiscard]] inline std::ptrdiff_t
path_length(ModuleGraph::vertex_descriptor u, const ModuleGraph& g) noexcept {
    return Impl::pathLength(u, g);
}

template <class Allocator>
inline void
path_composite(std::basic_string<char, std::char_traits<char>, Allocator>& str,
    std::ptrdiff_t& sz, ModuleGraph::vertex_descriptor u, const ModuleGraph& g
) noexcept {
    Impl::pathComposite(str, sz, u, g);
}

template <class Allocator>
inline const std::basic_string<char, std::char_traits<char>, Allocator>&
get_path(std::basic_string<char, std::char_traits<char>, Allocator>& output,
    ModuleGraph::vertex_descriptor u0, const ModuleGraph& g,
    std::string_view prefix = {},
    ModuleGraph::vertex_descriptor parent = ModuleGraph::null_vertex()) {
    output.clear();
    const auto sz0 = std::ssize(prefix);
    auto sz = sz0;

    const auto& moduleGraph = g;
    sz += Impl::pathLength(u0, moduleGraph, parent);

    output.resize(sz);

    Impl::pathComposite(output, sz, u0, moduleGraph, parent);
    Ensures(sz >= sz0);

    Ensures(sz == sz0);
    std::copy(prefix.begin(), prefix.end(), output.begin());

    return output;
}

[[nodiscard]] inline std::string
get_path(ModuleGraph::vertex_descriptor u0, const ModuleGraph& g,
    std::string_view prefix = {},
    ModuleGraph::vertex_descriptor parent = ModuleGraph::null_vertex()) {
    std::string output;
    get_path(output, u0, g, prefix, parent);
    return output;
}

[[nodiscard]] inline std::pmr::string
get_path(ModuleGraph::vertex_descriptor u0, const ModuleGraph& g,
    std::pmr::memory_resource* mr, std::string_view prefix = {},
    ModuleGraph::vertex_descriptor parent = ModuleGraph::null_vertex()) {
    std::pmr::string output(mr);
    get_path(output, u0, g, prefix, parent);
    return output;
}

template <class Allocator>
inline const std::basic_string<char, std::char_traits<char>, Allocator>&
get_path(std::basic_string<char, std::char_traits<char>, Allocator>& output,
    ModuleGraph::vertex_descriptor parent, std::string_view name, const ModuleGraph& g) {
    output.clear();
    auto sz = path_length(parent, g);
    output.resize(sz + name.size() + 1);
    output[sz] = '/';
    std::copy(name.begin(), name.end(), output.begin() + sz + 1);
    path_composite(output, sz, parent, g);
    Ensures(sz == 0);
    return output;
}

[[nodiscard]] inline std::string
get_path(ModuleGraph::vertex_descriptor parent, std::string_view name, const ModuleGraph& g) {
    std::string output;
    get_path(output, parent, name, g);
    return output;
}

[[nodiscard]] inline std::pmr::string
get_path(ModuleGraph::vertex_descriptor parent, std::string_view name, const ModuleGraph& g,
    std::pmr::memory_resource* mr) {
    std::pmr::string output(mr);
    get_path(output, parent, name, g);
    return output;
}

[[nodiscard]] inline ModuleGraph::vertex_descriptor
locate(std::string_view absolute, const ModuleGraph& g) noexcept {
    auto iter = g.mPathIndex.find(absolute);
    if (iter != g.mPathIndex.end()) {
         return iter->second;
    } else {
         return ModuleGraph::null_vertex();
    }
};

[[nodiscard]] inline ModuleGraph::vertex_descriptor
locate(ModuleGraph::vertex_descriptor u, std::string_view relative, const ModuleGraph& g) {
    Expects(!relative.starts_with('/'));
    Expects(!relative.ends_with('/'));
    auto key = get_path(u, relative, g);
    cleanPath(key);
    return locate(key, g);
};

[[nodiscard]] inline ModuleGraph::vertex_descriptor
locate(ModuleGraph::vertex_descriptor u, std::string_view relative, const ModuleGraph& g,
    std::pmr::memory_resource* mr) {
    Expects(!relative.starts_with('/'));
    Expects(!relative.ends_with('/'));
    auto key = get_path(u, relative, g, mr);
    cleanPath(key);
    return locate(key, g);
};

[[nodiscard]] inline bool
contains(std::string_view absolute, const ModuleGraph& g) noexcept {
    return locate(absolute, g) != ModuleGraph::null_vertex();
}

// MutableGraph(Vertex)
inline void add_path_impl(ModuleGraph::vertex_descriptor u, ModuleGraph::vertex_descriptor v, ModuleGraph& g) {
    // add to parent
    if (u != ModuleGraph::null_vertex()) {
        auto& outEdgeList = g.children_list(u);
        outEdgeList.emplace_back(v);

        auto& inEdgeList = g.parents_list(v);
        inEdgeList.emplace_back(u);
    }

    // add to external path index
    auto pathName = get_path(v, g, g.mPathIndex.get_allocator().resource());
    auto res = g.mPathIndex.emplace(std::move(pathName), v);
    Ensures(res.second);
}

inline void remove_path_impl(ModuleGraph::vertex_descriptor u, ModuleGraph& g) noexcept {
    // notice: here we use std::string, not std::pmr::string
    // we do not want to increase the memory of g
    auto pathName = get_path(u, g);
    auto iter = g.mPathIndex.find(std::string_view(pathName));
    Expects(iter != g.mPathIndex.end());
    g.mPathIndex.erase(iter);
    for (auto&& [name, v] : g.mPathIndex) {
        if (v > u) {
            --v;
        }
    }
}

inline void clear_out_edges(ModuleGraph::vertex_descriptor u, ModuleGraph& g) noexcept {
    // Bidirectional (OutEdges)
    auto& outEdgeList = g.out_edge_list(u);
    auto outEnd = outEdgeList.end();
    for (auto iter = outEdgeList.begin(); iter != outEnd; ++iter) {
        auto& inEdgeList = g.in_edge_list((*iter).get_target());
        // eraseFromIncidenceList
        Impl::sequenceEraseIf(inEdgeList, [u](const auto& e) {
            return e.get_target() == u;
        });
    }
    outEdgeList.clear();
}

inline void clear_in_edges(ModuleGraph::vertex_descriptor u, ModuleGraph& g) noexcept {
    // Bidirectional (InEdges)
    auto& inEdgeList = g.in_edge_list(u);
    auto inEnd = inEdgeList.end();
    for (auto iter = inEdgeList.begin(); iter != inEnd; ++iter) {
        auto& outEdgeList = g.out_edge_list((*iter).get_target());
        // eraseFromIncidenceList
        Impl::sequenceEraseIf(outEdgeList, [u](const auto& e) {
            return e.get_target() == u;
        });
    }
    inEdgeList.clear();
}

inline void clear_vertex(ModuleGraph::vertex_descriptor u, ModuleGraph& g) noexcept {
    clear_out_edges(u, g);
    clear_in_edges(u, g);
}

inline void remove_vertex(ModuleGraph::vertex_descriptor u, ModuleGraph& g) noexcept {
    { // AddressableGraph (Separated)
        remove_path_impl(u, g);
        Expects(num_children(u, g) == 0);

        // AddressableGraph (Parents)
        auto& parentsList = g.parents_list(u);
        auto inEnd = parentsList.end();
        for (auto iter = parentsList.begin(); iter != inEnd; ++iter) {
            auto& childrenList = g.children_list((*iter).get_target());
            // eraseFromIncidenceList
            Impl::sequenceEraseIf(childrenList, [u](const auto& e) {
                return e.get_target() == u;
            });
        }
        parentsList.clear();

        Impl::removeVectorOwner(g, u);
    }

    Impl::removeVectorVertex(const_cast<ModuleGraph&>(g), u, ModuleGraph::directed_category{});

    // remove components
    g.mNames.erase(g.mNames.begin() + u);
    g.mModules.erase(g.mModules.begin() + u);
}

// MutablePropertyGraph(Vertex)
template <class Component0, class Component1>
inline ModuleGraph::vertex_descriptor
add_vertex(Component0&& c0, Component1&& c1, ModuleGraph& g, ModuleGraph::vertex_descriptor u = ModuleGraph::null_vertex()) {
    auto v = gsl::narrow_cast<ModuleGraph::vertex_descriptor>(g.mVertices.size());

    g.mObjects.emplace_back();

    g.mVertices.emplace_back();
    g.mNames.emplace_back(std::forward<Component0>(c0));
    g.mModules.emplace_back(std::forward<Component1>(c1));

    // AddressableGraph
    add_path_impl(u, v, g);

    return v;
}

template <class Component0, class Component1>
inline ModuleGraph::vertex_descriptor
add_vertex(std::piecewise_construct_t, Component0&& c0, Component1&& c1, ModuleGraph& g, ModuleGraph::vertex_descriptor u = ModuleGraph::null_vertex()) {
    auto v = gsl::narrow_cast<ModuleGraph::vertex_descriptor>(g.mVertices.size());

    g.mObjects.emplace_back();

    g.mVertices.emplace_back();

    std::apply([&]<typename... T>(T&&... args) {
        g.mNames.emplace_back(std::forward<T>(args)...);
    }, std::forward<Component0>(c0));

    std::apply([&]<typename... T>(T&&... args) {
        g.mModules.emplace_back(std::forward<T>(args)...);
    }, std::forward<Component1>(c1));

    // AddressableGraph
    add_path_impl(u, v, g);

    return v;
}

// MutableGraph(Vertex)
inline ModuleGraph::vertex_descriptor
add_vertex(ModuleGraph& g, std::string_view name, ModuleGraph::vertex_descriptor parentID = ModuleGraph::null_vertex()) {
    return add_vertex(std::piecewise_construct,
        std::forward_as_tuple(name), // mNames
        std::forward_as_tuple(), // mModules
        g, parentID);
}

inline ModuleGraph::vertex_descriptor
add_vertex(ModuleGraph& g, std::pmr::string&& name, ModuleGraph::vertex_descriptor parentID = ModuleGraph::null_vertex()) {
    return add_vertex(std::piecewise_construct,
        std::forward_as_tuple(std::move(name)), // mNames
        std::forward_as_tuple(), // mModules
        g, parentID);
}

inline ModuleGraph::vertex_descriptor
add_vertex(ModuleGraph& g, const char* name, ModuleGraph::vertex_descriptor parentID = ModuleGraph::null_vertex()) {
    return add_vertex(std::piecewise_construct,
        std::forward_as_tuple(name), // mNames
        std::forward_as_tuple(), // mModules
        g, parentID);
}

} // namespace Meta

} // namespace Cocos
