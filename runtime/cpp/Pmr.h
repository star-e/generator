#pragma once

#include <tuple>
#include <utility>
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

// Construct in the destination column to propagate its allocator to the new value.
template<class Column, class Tuple, class... EmptyArgs>
void emplacePmrBack(Column& column, Tuple&& args, EmptyArgs&&... emptyArgs) {
    std::apply([&](auto&&... values) {
        if constexpr (sizeof...(values) == 0) {
            column.emplace_back(std::forward<EmptyArgs>(emptyArgs)...);
        } else {
            column.emplace_back(std::forward<decltype(values)>(values)...);
        }
    }, std::forward<Tuple>(args));
}

} // namespace cc::detail
