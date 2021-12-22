#pragma once
#include <set>

namespace Cocos {

template<class Value>
using Set = std::set<Value, std::less<>>;

template<class Value>
using StdSet = std::set<Value, std::less<>>;

template<class Value>
using PmrSet = std::set<Value, std::less<>,
    std::pmr::polymorphic_allocator<Value>
>;

template<class Value, class KeyLike, class Allocator>
inline bool exists(const std::set<Value, std::less<>, Allocator>& m, const KeyLike& key) noexcept {
    auto iter = m.find(key);
    return iter != m.end();
}

template<class Value, class Allocator, class KeyLike>
inline typename std::set<Value, std::less<>, Allocator>::value_type&
at(std::set<Value, std::less<>, Allocator>& m, const KeyLike& key) {
    auto iter = m.find(key);
    if (iter == m.end()) {
        throw std::out_of_range("at(std::set) out of range");
    }
    return *iter;
}

template<class Value, class Allocator, class KeyLike>
inline typename std::set<Value, std::less<>, Allocator>::value_type const&
at(const std::set<Value, std::less<>, Allocator>& m, const KeyLike& key) {
    auto iter = m.find(key);
    if (iter == m.end()) {
        throw std::out_of_range("at(std::set) out of range");
    }
    return *iter;
}

template <class Value, class Pred, class Allocator>
void reserve(std::set<Value, Pred, Allocator>& m, size_t sz) noexcept {
    // do nothing
}

}
