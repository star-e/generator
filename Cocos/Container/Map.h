#pragma once
#include <map>

// for std::less<> the transparent comparator
// see https://stackoverflow.com/questions/20317413/what-are-transparent-comparators

namespace Cocos {

template<class Key, class Value>
using Map = std::map<Key, Value, std::less<>>;

template<class Key, class Value>
using PmrMap = std::pmr::map<Key, Value, std::less<>>;

template<class Key, class Value>
using PmrMultimap = std::pmr::multimap<Key, Value, std::less<>>;

// variant
struct VariantIndexLess;

template<class K, class T>
using VariantIndexMap = std::map<K, T, VariantIndexLess>;

template<class K, class T>
using PmrVariantIndexMap = std::pmr::map<K, T, VariantIndexLess>;

// help functions
template<class Key, class Value, class KeyLike, class Allocator>
inline bool exists(const std::map<Key, Value, std::less<>, Allocator>& m, const KeyLike& key) noexcept {
    auto iter = m.find(key);
    return iter != m.end();
}

template<class Key, class Value, class Allocator, class KeyLike>
inline typename std::map<Key, Value, std::less<>, Allocator>::mapped_type&
at(std::map<Key, Value, std::less<>, Allocator>& m, const KeyLike& key) {
    auto iter = m.find(key);
    if (iter == m.end()) {
        throw std::out_of_range("at(std::map) out of range");
    }
    return iter->second;
}

template<class Key, class Value, class Allocator, class KeyLike>
inline typename std::map<Key, Value, std::less<>, Allocator>::mapped_type const&
at(const std::map<Key, Value, std::less<>, Allocator>& m, const KeyLike& key) {
    auto iter = m.find(key);
    if (iter == m.end()) {
        throw std::out_of_range("at(std::map) out of range");
    }
    return iter->second;
}

template <class Key, class Value, class Pred, class Allocator>
void reserve(std::map<Key, Value, Pred, Allocator>& m, size_t sz) noexcept {
    // do nothing
}

struct NameLess {
    template<class T>
    bool operator()(const T& lhs, const T& rhs) const noexcept {
        return lhs.mName < rhs.mName;
    }
};

}
