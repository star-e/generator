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
#include <type_traits>
#include <tuple>
#include <Cocos/Overload.h>

namespace Cocos {

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
