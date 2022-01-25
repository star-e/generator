#pragma once
#include <type_traits>
#include <utility>

namespace cc {

// https://stackoverflow.com/questions/50510122/stdvariant-with-overloaded-lambdas-alternative-with-msvc

template <class... Ts>
struct overloaded_t {};

template <class T0>
struct overloaded_t<T0> : T0 {
    using T0::operator();
    overloaded_t(T0 t0)
        : T0(std::move(t0)) {}
};

template <class T0, class T1, class... Ts>
struct overloaded_t<T0, T1, Ts...> : T0
    , overloaded_t<T1, Ts...> {
    using T0::operator();
    using overloaded_t<T1, Ts...>::operator();
    overloaded_t(T0 t0, T1 t1, Ts... ts)
        : T0(std::move(t0))
        , overloaded_t<T1, Ts...>(std::move(t1), std::move(ts)...) {}
};

template <class... Ts>
overloaded_t<Ts...> overload(Ts... ts) {
    return { std::move(ts)... };
}

}
