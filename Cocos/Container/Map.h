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
