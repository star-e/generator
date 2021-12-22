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
