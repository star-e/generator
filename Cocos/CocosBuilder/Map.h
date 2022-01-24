#pragma once
#include <boost/container/map.hpp>
#include <boost/container/pmr/map.hpp>

// for std::less<> the transparent comparator
// see https://stackoverflow.com/questions/20317413/what-are-transparent-comparators

namespace cc {

template<class Key, class Value>
using Map = boost::container::map<Key, Value, std::less<>>;

template<class Key, class Value>
using PmrMap = boost::container::pmr::map<Key, Value, std::less<>>;

template<class Key, class Value>
using PmrMultimap = boost::container::pmr::multimap<Key, Value, std::less<>>;

// variant
struct VariantIndexLess;

template<class K, class T>
using VariantIndexMap = boost::container::map<K, T, VariantIndexLess>;

template<class K, class T>
using PmrVariantIndexMap = boost::container::pmr::map<K, T, VariantIndexLess>;

// help functions
template<class Key, class Value, class KeyLike, class Allocator>
inline bool exists(const boost::container::map<Key, Value, std::less<>, Allocator>& m, const KeyLike& key) noexcept {
    auto iter = m.find(key);
    return iter != m.end();
}

template<class Key, class Value, class Allocator, class KeyLike>
inline typename boost::container::map<Key, Value, std::less<>, Allocator>::mapped_type&
at(boost::container::map<Key, Value, std::less<>, Allocator>& m, const KeyLike& key) {
    auto iter = m.find(key);
    if (iter == m.end()) {
        throw std::out_of_range("at(boost::container::map) out of range");
    }
    return iter->second;
}

template<class Key, class Value, class Allocator, class KeyLike>
inline typename boost::container::map<Key, Value, std::less<>, Allocator>::mapped_type const&
at(const boost::container::map<Key, Value, std::less<>, Allocator>& m, const KeyLike& key) {
    auto iter = m.find(key);
    if (iter == m.end()) {
        throw std::out_of_range("at(boost::container::map) out of range");
    }
    return iter->second;
}

template <class Key, class Value, class Pred, class Allocator>
void reserve(boost::container::map<Key, Value, Pred, Allocator>& m, size_t sz) noexcept {
    // do nothing
}

struct NameLess {
    template<class T>
    bool operator()(const T& lhs, const T& rhs) const noexcept {
        return lhs.mName < rhs.mName;
    }
};

}
