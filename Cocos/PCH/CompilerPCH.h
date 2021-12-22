/*
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2021 Xiamen Yaji Software Co., Ltd.

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
//------------------------------------------------------------
// C
//------------------------------------------------------------
#include <cstdint>
#include <cstddef>
#define _USE_MATH_DEFINES
#include <cmath>

//------------------------------------------------------------
// C++
//------------------------------------------------------------
#include <type_traits>
#include <array>
#include <tuple>
#include <optional>
#include <variant>

#include <memory>
#include <memory_resource>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <functional>
#include <algorithm>

#include <complex>

#if (__cplusplus > 201703L)
#include <numbers>
#include <concepts>
#include <ranges>
#include <span>
#include <bit>
#include <coroutine>
#include <numbers>
//#include <format>
#endif

//------------------------------------------------------------
// boost
//------------------------------------------------------------
// preprocessor
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/comma_if.hpp>
#include <boost/preprocessor/control/expr_if.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>

// oop
#include <boost/cast.hpp>

// memory
#include <boost/align.hpp>

// algorithm
#include <boost/algorithm/clamp.hpp>

// iterator
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_adaptor.hpp>

// range
#include <boost/range/iterator_range.hpp>
#include <boost/range/irange.hpp>

// container
#include <boost/container/static_vector.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

// uuid
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_hash.hpp>

// graph
#include <boost/graph/detail/edge.hpp>
#include <boost/graph/detail/adj_list_edge_iterator.hpp>
#include <boost/graph/adjacency_iterator.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/property_map/property_map.hpp>

//------------------------------------------------------------
// gsl
//------------------------------------------------------------
#include <gsl/pointers>
#include <gsl/gsl_util>
#include <gsl/gsl_assert>

//------------------------------------------------------------
// Cocos
//------------------------------------------------------------
#include <Cocos/Container/Map.h>
#include <Cocos/Container/Set.h>
#include <Cocos/Container/UnorderedMap.h>
#include <Cocos/GraphImpl.h>
#include <Cocos/MovePtr.h>
