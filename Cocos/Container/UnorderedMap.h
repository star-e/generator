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
