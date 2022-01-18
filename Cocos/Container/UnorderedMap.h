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
#include <unordered_map>
#include <string>

namespace Cocos {

template <class Char>
struct StringHash {
    using is_transparent = void;
    using string_view_type = std::basic_string_view<Char>;

    size_t operator()(string_view_type str) const noexcept {
        return std::hash<string_view_type>{}(str);
    }
    size_t operator()(const Char* str) const noexcept {
        return std::hash<string_view_type>{}(str);
    }
    template <class Alloc>
    size_t operator()(const std::basic_string<Char, std::char_traits<Char>, Alloc>& str) const noexcept {
        return std::hash<string_view_type>{}(str);
    }
};

template <class Key, class Value>
using PmrUnorderedStringMap = std::pmr::unordered_map<Key, Value,
    StringHash<typename Key::value_type>, std::equal_to<>>;

template <class Key, class Value, class Hash, class Equal, class Allocator, class KeyLike>
inline typename std::unordered_map<Key, Value, Hash, Equal, Allocator>::mapped_type&
at(std::unordered_map<Key, Value, Hash, Equal, Allocator>& m, const KeyLike& key) {
    auto iter = m.find(key);
    if (iter == m.end()) {
        throw std::out_of_range("at(std::unordered_map) out of range");
    }
    return iter->second;
}

template <class Key, class Value, class Hash, class Equal, class Allocator, class KeyLike>
inline const typename std::unordered_map<Key, Value, Hash, Equal, Allocator>::mapped_type&
at(const std::unordered_map<Key, Value, Hash, Equal, Allocator>& m, const KeyLike& key) {
    auto iter = m.find(key);
    if (iter == m.end()) {
        throw std::out_of_range("at(std::unordered_map) out of range");
    }
    return iter->second;
}

template <class Key, class Value, class Hash, class Equal, class Allocator>
void reserve(std::unordered_map<Key, Value, Hash, Equal, Allocator>& m) noexcept {
    // do nothing
}

}
