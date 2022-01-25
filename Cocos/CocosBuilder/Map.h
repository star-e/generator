#pragma once
#include <map>
#include <unordered_map>
#include <boost/container/pmr/map.hpp>

// for std::less<> the transparent comparator
// see https://stackoverflow.com/questions/20317413/what-are-transparent-comparators

namespace cc {

template<class Key, class Value>
using TransparentMap = std::map<Key, Value, std::less<>>;

template<class Key, class Value>
using TransparentMultiMap = std::multimap<Key, Value, std::less<>>;

template<class Key, class Value>
using PmrTransparentMap = boost::container::pmr::map<Key, Value, std::less<>>;

template<class Key, class Value>
using PmrTransparentMultimap = boost::container::pmr::multimap<Key, Value, std::less<>>;

}
