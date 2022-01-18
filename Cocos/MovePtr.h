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

namespace Cocos {

template<typename T>
class move_ptr {
    T* mPointer;
public:
    move_ptr(T* ptr = nullptr) noexcept
        : mPointer(ptr)
    {}
    move_ptr(move_ptr<T>&& other) noexcept
        : mPointer(other.mPointer)
    {
        other.mPointer = nullptr;
    }
    move_ptr(move_ptr<T> const& other) = delete;

    move_ptr<T>& operator=(move_ptr<T>&& other) noexcept {
        mPointer = other.mPointer;
        other.mPointer = nullptr;
        return *this;
    }
    move_ptr<T>& operator=(move_ptr<T> const& other) = delete;

    move_ptr<T>& operator=(T* ptr) noexcept {
        mPointer = ptr;
        return *this;
    }

    bool operator==(T* ptr) const noexcept {
        return mPointer == ptr;
    }

    bool operator!=(T* ptr) const noexcept {
        return mPointer != ptr;
    }

    T* operator->() const noexcept {
        return mPointer;
    }
    T& operator*() const noexcept {
        return *mPointer;
    }
    T* get() const noexcept {
        return mPointer;
    }
    explicit operator bool() const noexcept {
        return mPointer;
    }
};

}
