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
#include <memory_resource>
#include <string>
#include <sstream>

namespace Cocos {

template<class String>
class Indent {
public:
    Indent(String& indent)
        : mIndent(&indent) {
        mIndent->append("    ");
    }
    ~Indent() {
        reset();
    }
    Indent(Indent&& rhs) noexcept
        : mIndent(rhs.mIndent) {
        rhs.mIndent = nullptr;
    }
    Indent& operator=(Indent&& rhs) noexcept {
        mIndent = rhs.mIndent;
        rhs.mIndent = nullptr;
        return *this;
    }
    void reset() {
        if (mIndent) {
            mIndent->erase(mIndent->size() - 4);
            mIndent = nullptr;
        }
    }
    String* mIndent = nullptr;
};

inline Indent<std::string> indent(std::string& str) {
    return Indent<std::string>(str);
}

inline Indent<std::pmr::string> indent(std::pmr::string& str) {
    return Indent<std::pmr::string>(str);
}

inline void copyString(std::ostream& os, const std::string& space, std::string_view str, bool append = false) {
    std::istringstream iss{ std::string(str) };
    std::string line;
    int count = 0;
    while (std::getline(iss, line)) {
        if (line.empty())
            os << '\n';
        else if (line[0] == '#') {
            os << line << '\n';
        } else {
            if (append) {
                if (count == 0) {
                    os << line;
                } else {
                    os << '\n' << space << line;
                }
            } else {
                os << space << line << '\n';
            }
        }
        ++count;
    }
}

inline void copyString(std::ostream& os, const std::pmr::string& space, std::string_view str, bool append = false) {
    std::istringstream iss{ std::string(str) };
    std::string line;
    int count = 0;
    while (std::getline(iss, line)) {
        if (line.empty())
            os << '\n';
        else if (line[0] == '#') {
            os << line << '\n';
        } else {
            if (append) {
                if (count == 0) {
                    os << line;
                } else {
                    os << '\n' << space << line;
                }
            } else {
                os << space << line << '\n';
            }
        }
        ++count;
    }
}

inline void try_copyString(std::ostream& os, const std::string& space, const std::string& str) {
    if (str.empty())
        return;
    copyString(os, space, str);
}

inline void copyString(std::ostream& os, const std::string& str) {
    std::istringstream iss(str);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty())
            os << '\n';
        else if (line[0] == '#') {
            os << line << '\n';
        } else {
            os << line << '\n';
        }
    }
}

inline void copyString(std::ostream& os, const std::string& str, std::string_view prefix, std::string_view suffix) {
    std::string str2 = boost::algorithm::replace_all_copy(str, "\r", "");
    std::istringstream iss(str2);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty())
            continue;
        os << prefix << line << suffix;
    }
}

inline void copyString(std::ostream& os, const std::pmr::string& str) {
    std::basic_istringstream<char, std::char_traits<char>,
        std::pmr::polymorphic_allocator<char>> iss(str);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty())
            os << '\n';
        else if (line[0] == '#') {
            os << line << '\n';
        } else {
            os << line << '\n';
        }
    }
}

inline void try_copyString(std::ostream& os, const std::string& str, bool addNewLine = true) {
    if (str.empty())
        return;
    if (addNewLine)
        os << "\n";
    copyString(os, str);
}

inline void copyCppString(std::ostream& os, std::string_view space, std::string_view str, bool append = false) {
    std::istringstream iss{ std::string(str) };
    std::string line;
    int count = 0;
    while (std::getline(iss, line)) {
        if (line.empty()) {
            os << '\n';
        } else if (line[0] == '#') {
            os << line << '\n';
        } else if (*line.rbegin() == ':') {
            os << space.substr(0, std::max(size_t(4), space.size()) - 4) << line << "\n";
        } else {
            if (append) {
                if (count == 0) {
                    os << line;
                } else {
                    os << '\n' << space << line;
                }
            } else {
                os << space << line << '\n';
            }
        }
        ++count;
    }
}

}
