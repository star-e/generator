#pragma once

#include <vector>

namespace cc::detail {

// The MSVC debug STL allocates an iterator proxy even for an empty container.
// Its allocator-only vector and string constructors are unconditionally noexcept.
template<class T, class Alloc, class Empty>
T makeEmptyPmr(const Alloc& alloc, const Empty& empty)
#if defined(_MSVC_STL_VERSION) && _ITERATOR_DEBUG_LEVEL != 0
    noexcept(noexcept(T(empty, typename T::allocator_type(alloc)))) {
    return T(empty, typename T::allocator_type(alloc));
#else
    noexcept(noexcept(T(alloc))) {
    (void)empty;
    return T(alloc);
#endif
}

} // namespace cc::detail
