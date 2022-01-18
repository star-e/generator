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
#include <string>

namespace Cocos {

inline std::pmr::string makePath(
    std::initializer_list<std::string_view> names,
    std::pmr::memory_resource* mr
) {
    std::pmr::string output(mr); // temp string
    { // reserve capacity
        size_t sz = 0;
        for (const auto& name : names) {
            sz += name.size() + 1;
        }
        output.reserve(sz);
    }
    for (const auto& name : names) {
        output.append("/");
        output.append(name);
    }

    // pmr allocator cannot use move/copy cntr because of POCCA/POCMA
    // NRVO is also not guaranteed
    // must use guaranteed RVO, invoke move cntr with alloc
    return std::pmr::string(std::move(output), mr);
}

inline std::pmr::string makePath(std::string_view name,
    std::pmr::memory_resource* mr
) {
    std::pmr::string output(mr); // temp string
    output.reserve(name.size() + 1);
    output.append("/");
    output.append(name);

    // pmr allocator cannot use move/copy cntr because of POCCA/POCMA
    // NRVO is also not guaranteed
    // must use guaranteed RVO, invoke move cntr with alloc
    return std::pmr::string(std::move(output), mr);
}

template<class CharT, class Allocator>
inline void cleanPath(std::basic_string<CharT, std::char_traits<CharT>, Allocator>& str) noexcept {
    using string_t = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;
    constexpr CharT slash[] = { '/', '\0' };
    constexpr CharT doubleSlash[] = { '/', '/', '\0' };

    Expects(!str.empty());
    Expects(str.starts_with(slash));
    Expects(str.find(doubleSlash) == string_t::npos);
    Expects([&]() {
        bool valid = true;
        for (uint8_t c : str) {
            if (c < uint8_t('.'))
                valid = false;
        }
        return valid;
    }());

    { // remove all /./
        constexpr CharT current[] = { '/', '.', '/', '\0' };

        auto pos = str.rfind(current);
        while (pos != string_t::npos) {
            str.erase(pos, 2);
            pos = str.rfind(current);
        }
        // remove tailing /.
        constexpr CharT ending[] = { '/', '.', '\0' };
        if (str.ends_with(ending)) {
            str.resize(str.size() - 2);
        }
    }

    // try remove /..
    constexpr std::array previous{ CharT('/'), CharT('.'), CharT('.'), CharT('\0') };
    auto pos = str.find(previous.data());
    while (pos != string_t::npos) {
        if (pos == 0) {
            // root element has not parent path
            str = {}; // slash;
            return;
        }
        auto beg = str.rfind(slash, pos - 1);
        Expects(beg != string_t::npos);
        str.erase(beg, pos - beg + previous.size() - 1);
    }

    // if tailing path is /.., the str will be empty. check and make it pseudo-root
    //if (str.empty()) {
    //    str = slash;
    //}
    //Ensures(str.starts_with(slash));
}

}
