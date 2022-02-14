#pragma once
#include <type_traits>
#include <tuple>
#include <Star/SOverload.h>

namespace Star {

template <class... Ts>
struct vertex_overloaded : overloaded<std::remove_cvref_t<Ts>...> {
    template <class T>
    auto operator()(T* ptr) {
        return this->overloaded<std::remove_cvref_t<Ts>...>::operator()(*ptr);
    }
};

template <class GraphT, class... Ts>
auto visit_vertex(typename GraphT::vertex_descriptor v, GraphT& g, Ts&&... args) {
    return visit(
        vertex_overloaded<std::remove_cvref_t<Ts>...>{
            overloaded<std::remove_cvref_t<Ts>...>{ std::forward<Ts>(args)... } },
        value(v, g));
}

template <class T, class GraphT>
std::tuple<std::remove_cvref_t<T>&, typename GraphT::vertex_descriptor> get_parent(
    typename GraphT::vertex_descriptor vertID, GraphT& g) {
    Expects(vertID != GraphT::null_vertex());
    auto id = parent(vertID, g);
    Expects(id != GraphT::null_vertex());
    return { get<std::remove_cvref_t<T>>(id, g), id };
}

template <class T, class GraphT>
std::tuple<const std::remove_cvref_t<T>&, typename GraphT::vertex_descriptor> get_parent(
    typename GraphT::vertex_descriptor vertID, const GraphT& g) {
    Expects(vertID != GraphT::null_vertex());
    auto id = parent(vertID, g);
    Expects(id != GraphT::null_vertex());
    return { get<std::remove_cvref_t<T>>(id, g), id };
}

template <class T, class GraphT>
std::tuple<std::remove_cvref_t<T>&, typename GraphT::vertex_descriptor> get_ancestor(
    typename GraphT::vertex_descriptor vertID, GraphT& g) {
    Expects(vertID != GraphT::null_vertex());
    for (auto id = parent(vertID, g); id != GraphT::null_vertex(); id = parent(id, g)) {
        auto ptr = get_if<std::remove_cvref_t<T>>(id, &g);
        if (ptr) {
            return { *ptr, vertID };
        }
    }
    throw std::out_of_range("parent not found in graph");
}

template <class T, class GraphT>
std::tuple<const std::remove_cvref_t<T>&, typename GraphT::vertex_descriptor> get_ancestor(
    typename GraphT::vertex_descriptor vertID, const GraphT& g) {
    Expects(vertID != GraphT::null_vertex());
    for (auto id = parent(vertID, g); id != GraphT::null_vertex(); id = parent(id, g)) {
        auto ptr = get_if<std::remove_cvref_t<T>>(id, &g);
        if (ptr) {
            return { *ptr, vertID };
        }
    }
    throw std::out_of_range("parent not found in graph");
}

}
