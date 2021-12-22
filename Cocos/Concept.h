#pragma once

#if (__cplusplus > 201703L)
#include <concepts>

namespace Star {

template<class T>
concept AllocatorUserClass_ = requires(T t) {
    typename T::allocator_type;
    t.get_allocator();
};

template<class T>
concept PmrAllocatorUserClass_ = AllocatorUserClass_<T> && requires(T t) {
    t.get_allocator().resource();
};

}

#endif
