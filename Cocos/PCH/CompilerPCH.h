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
