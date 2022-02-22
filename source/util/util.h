#pragma once
#include <type_traits>
#include <utility>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

#define AS_LAMBDA(func) [&](auto&&... args) -> decltype(func(FWD(args)...)) { return func(FWD(args)...); }

template <bool cond_v, typename Then, typename OrElse>
decltype(auto) constexpr_if(Then&& then, OrElse&& or_else) {
    if constexpr (cond_v) {
        return FWD(then);
    }
    else {
        return FWD(or_else);
    }
}