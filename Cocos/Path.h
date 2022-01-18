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
#include <gsl/gsl_assert>
#include <gsl/gsl_util>

namespace Cocos {

class PathIter {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::u8string_view;
    using reference = std::u8string_view;
    using pointer = std::u8string_view*;
    using difference_type = std::ptrdiff_t;
    using distance_type = size_t;

    PathIter() = default;
    
    PathIter(const std::u8string_view& str) noexcept
        : mView(&str)
    {
        if (str.empty()) {
            return;
        }

        bool absolute = (str[0] == '/');
        mBegin = absolute ? 1 : 0;

        auto pos = str.find('/', mBegin);
        if (pos == std::u8string_view::npos) {
            mEnd = gsl::narrow_cast<uint32_t>(str.size());
        } else {
            mEnd = gsl::narrow_cast<uint32_t>(pos);
        }
    }

    PathIter(const std::u8string_view& str, bool bEnd) noexcept
        : mView(&str)
        , mBegin(gsl::narrow_cast<uint32_t>(str.size()))
        , mEnd(gsl::narrow_cast<uint32_t>(str.size()))
    {}

    PathIter& operator++() noexcept {
        if (mEnd < mView->size()) {
            mBegin = mEnd + 1;
            auto pos = mView->find('/', mBegin);
            if (pos == std::u8string_view::npos) {
                mEnd = gsl::narrow_cast<uint32_t>(mView->size());
            } else {
                mEnd = gsl::narrow_cast<uint32_t>(pos);
            }
        } else {
            mBegin = mEnd;
        }
        return *this;
    }
    PathIter operator++(int) noexcept {
        PathIter tmp = *this;
        ++(*this);
        return tmp;
    }
    value_type operator*() const noexcept {
        return mView->substr(mBegin, mEnd);
    }
    bool operator==(const PathIter& rhs) const noexcept {
        return mBegin == rhs.mBegin && mEnd == rhs.mEnd;
    }
    bool operator!=(const PathIter& rhs) const noexcept {
        return !(this->operator==(rhs));
    }
protected:
    const std::u8string_view* mView = nullptr;
    uint32_t mBegin = {};
    uint32_t mEnd = {};
};

struct Path;

struct PathView {
    PathView() = default;

    PathView(const char8_t* name) noexcept : mName(name) {}
    PathView(const char8_t* name, size_t sz) noexcept : mName(name, sz) {}
    PathView(std::u8string_view name) noexcept : mName(name) {}
    PathView(const std::u8string& name) noexcept : mName(name) {}
    PathView(const std::pmr::u8string& name) noexcept : mName(name) {}
    PathView(const Path& name) noexcept;

    using iterator = PathIter;
    using const_iterator = PathIter;

    bool empty() const noexcept {
        return mName.empty();
    }

    size_t size() const noexcept {
        return mName.size();
    }

    bool is_absolute() const noexcept {
        Expects(!empty());
        return mName[0] == '/';
    }

    bool is_relative() const noexcept {
        return !this->is_absolute();
    }

    PathView relative() const noexcept {
        return PathView(mName.substr(this->is_absolute() ? 1 : 0));
    }

    std::u8string_view extension() const noexcept {
        auto pos = mName.find_last_of('.');
        if (pos == mName.npos || pos == 0) {
            return {};
        } else {
            return mName.substr(pos);
        }
    }

    std::u8string_view stem() const noexcept {
        Expects(is_absolute());

        auto pos = mName.find_last_of('.');
        if (pos == std::u8string_view::npos || pos == 0) {
            return mName;
        }
        return mName.substr(0, pos);
    }

    std::u8string_view name() const noexcept {
        auto pos = mName.find_last_of('/');
        if (pos == mName.npos) {
            return mName;
        } else {
            return mName.substr(pos + 1);
        }
    }

    std::u8string_view basename() const noexcept {
        auto base = stem();
        auto pos = base.find_last_of('/');
        if (pos == base.npos) {
            return base;
        } else {
            return base.substr(pos + 1);
        }
    }

    std::u8string_view parent() const noexcept {
        auto pos = mName.find_last_of('/');
        if (pos == mName.npos) {
            return {};
        } else {
            return mName.substr(0, pos);
        }
    }

    const std::u8string_view& sv() const noexcept {
        return mName;
    }

    iterator begin() const noexcept {
        return PathIter(mName);
    }

    iterator end() const noexcept {
        return PathIter(mName, true);
    }

    const_iterator cbegin() const noexcept {
        return PathIter(mName);
    }

    const_iterator cend() const noexcept {
        return PathIter(mName, true);
    }

    operator const std::u8string_view& () const noexcept {
        return mName;
    }

    inline bool operator<(const PathView& rhs) const noexcept {
        return mName < rhs.mName;
    }

    std::u8string_view mName;
};
//
//inline bool operator<(const PathView& lhs, std::u8string_view rhs) noexcept {
//    return lhs.mName < rhs;
//}
//
//inline bool operator<(std::u8string_view lhs, const PathView& rhs) noexcept {
//    return lhs < rhs.mName;
//}
//
//inline bool operator<(const PathView& lhs, const std::pmr::u8string& rhs) noexcept {
//    return lhs.mName < std::u8string_view{ rhs };
//}
//
//inline bool operator<(const std::pmr::u8string& lhs, const PathView& rhs) noexcept {
//    return std::u8string_view{ lhs } < rhs.mName;
//}
//
//inline bool operator<(const PathView& lhs, const std::u8string& rhs) noexcept {
//    return lhs.mName < std::u8string_view{ rhs };
//}
//
//inline bool operator<(const std::u8string& lhs, const PathView& rhs) noexcept {
//    return std::u8string_view{ lhs } < rhs.mName;
//}

struct Path {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

    allocator_type get_allocator() const noexcept {
        return allocator_type(mName.get_allocator().resource());
    }

    Path(const allocator_type& alloc) : mName(alloc) {}
    Path(Path&& rhs, const allocator_type& alloc) : mName(std::move(rhs.mName), alloc) {}
    Path(const Path& rhs, const allocator_type& alloc) : mName(rhs.mName, alloc) {}

    Path(const char8_t* name, const allocator_type& alloc) : mName(name, alloc) {}
    Path(std::pmr::u8string&& name, const allocator_type& alloc) : mName(std::move(name), alloc) {}
    Path(std::u8string_view name, const allocator_type& alloc) : mName(name, alloc) {}
    Path(PathView view, const allocator_type& alloc) : mName(view.sv(), alloc) {}

    Path(Path&&) noexcept = default;
    Path(Path const&) = delete;
    Path& operator=(Path&&) = default;
    Path& operator=(Path const&) = default;

    Path& append(PathView view0) {
        auto view = view0.relative();

        mName.reserve(mName.size() + view.size() + 1);
        mName.push_back(u8'/');
        mName.append(view.sv());

        return *this;
    }

    Path& append(std::u8string_view name) {
        this->append(PathView(name));
        return *this;
    }

    Path& append(const Path& name) {
        this->append(PathView(name.sv()));
        return *this;
    }

    [[nodiscard]] Path make(std::initializer_list<std::u8string_view> names, std::pmr::memory_resource* mr) {
        std::pmr::u8string output(mr); // temp string
        { // reserve capacity
            size_t sz = 0;
            for (const auto& name : names) {
                sz += name.size() + 1;
            }
            output.reserve(sz);
        }
        for (const auto& name : names) {
            output.push_back(u8'/');
            output.append(name);
        }
        // pmr allocator cannot use move/copy cntr because of POCCA/POCMA
        // NRVO is also not guaranteed
        // must use guaranteed RVO, invoke move cntr with alloc
        return Path(std::move(output), mr);
    }

    [[nodiscard]] Path make(std::u8string_view name, std::pmr::memory_resource* mr) {
        return make({ name }, mr);
    }


    using iterator = PathIter;
    using const_iterator = PathIter;

    bool empty() const noexcept {
        return mName.empty();
    }

    size_t size() const noexcept {
        return mName.size();
    }

    bool is_absolute() const noexcept {
        Expects(!empty());
        return mName[0] == '/';
    }

    bool is_relative() const noexcept {
        return !this->is_absolute();
    }

    PathView relative() const noexcept {
        return PathView(this->sv()).relative();
    }

    std::u8string_view name() const noexcept {
        return PathView{ *this }.name();
    }

    std::u8string_view parent() const noexcept {
        return PathView{ *this }.parent();
    }

    PathView view() const noexcept {
        return PathView(this->sv());
    }

    const std::pmr::u8string& string() const noexcept {
        return mName;
    }

    std::u8string_view sv() const noexcept {
        return { mName };
    }

    const char8_t* c_str() const noexcept {
        return mName.c_str();
    }

    iterator begin() const noexcept {
        return PathIter(mName);
    }

    iterator end() const noexcept {
        return PathIter(mName, true);
    }

    const_iterator cbegin() const noexcept {
        return PathIter(mName);
    }

    const_iterator cend() const noexcept {
        return PathIter(mName, true);
    }

    operator const std::pmr::u8string& () const noexcept {
        return mName;
    }

    operator std::u8string_view() const noexcept {
        return { mName };
    }

    inline bool operator<(const Path& rhs) const noexcept {
        return mName < rhs.mName;
    }

    std::pmr::u8string mName;
};

inline PathView::PathView(const Path& name) noexcept : mName(name.sv()) {}

inline bool operator<(const Path& lhs, std::u8string_view rhs) noexcept {
    return lhs.mName < rhs;
}

inline bool operator<(std::u8string_view lhs, const Path& rhs) noexcept {
    return lhs < rhs.mName;
}

}
