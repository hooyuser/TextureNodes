#pragma once
#include <type_traits>
#include <utility>
#include <memory>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

#define AS_LAMBDA(func) [&](auto&&... args) -> decltype(func(FWD(args)...)) { return func(FWD(args)...); }

template <bool cond_v, typename Then, typename OrElse>
constexpr decltype(auto) constexpr_if(Then&& then, OrElse&& or_else) {
    if constexpr (cond_v) {
        return FWD(then);
    }
    else {
        return FWD(or_else);
    }
}

template <int N, typename F>
constexpr decltype(auto) UNROLL(F&& func) {
    return [func = FWD(func)] <std::size_t... I> (std::index_sequence<I...>) {
        (func.template operator() < I > (), ...);
    }(std::make_index_sequence<N>{});
}

template <typename T> struct Pointer;

template <typename T> struct Pointer<T*>
{
    typedef T Type;
};

template <typename T> struct Pointer<std::shared_ptr<T>>
{
    typedef T Type;
};

template <typename T> struct Pointer<std::unique_ptr<T>>
{
    typedef T Type;
};


template <class T>
using ref_t = typename Pointer<std::remove_reference_t<T>>::Type;