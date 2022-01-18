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
