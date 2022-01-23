#pragma once

#include <type_traits>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

#define MAKE_ENUM_FLAGS(TEnum)                                                      \
    inline TEnum operator~(TEnum a) {                                               \
        using TUnder = typename std::underlying_type_t<TEnum>;                      \
        return static_cast<TEnum>(~static_cast<TUnder>(a));                         \
    }                                                                               \
    inline TEnum operator|(TEnum a, TEnum b) {                                      \
        using TUnder = typename std::underlying_type_t<TEnum>;                      \
        return static_cast<TEnum>(static_cast<TUnder>(a) | static_cast<TUnder>(b)); \
    }                                                                               \
    inline TEnum operator&(TEnum a, TEnum b) {                                      \
        using TUnder = typename std::underlying_type_t<TEnum>;                      \
        return static_cast<TEnum>(static_cast<TUnder>(a) & static_cast<TUnder>(b)); \
    }                                                                               \
    inline TEnum operator^(TEnum a, TEnum b) {                                      \
        using TUnder = typename std::underlying_type_t<TEnum>;                      \
        return static_cast<TEnum>(static_cast<TUnder>(a) ^ static_cast<TUnder>(b)); \
    }                                                                               \
    inline TEnum& operator|=(TEnum& a, TEnum b) {                                   \
        using TUnder = typename std::underlying_type_t<TEnum>;                      \
        a = static_cast<TEnum>(static_cast<TUnder>(a) | static_cast<TUnder>(b));    \
        return a;                                                                   \
    }                                                                               \
    inline TEnum& operator&=(TEnum& a, TEnum b) {                                   \
        using TUnder = typename std::underlying_type_t<TEnum>;                      \
        a = static_cast<TEnum>(static_cast<TUnder>(a) & static_cast<TUnder>(b));    \
        return a;                                                                   \
    }                                                                               \
    inline TEnum& operator^=(TEnum& a, TEnum b) {                                   \
        using TUnder = typename std::underlying_type_t<TEnum>;                      \
        a = static_cast<TEnum>(static_cast<TUnder>(a) ^ static_cast<TUnder>(b));    \
        return a;                                                                   \
    }


template<class...>
using void_t = void;

struct AnyType {
	template <typename T>
	operator T();
};

template <typename T, typename = void, typename ...Ts>
struct CountMember {
	constexpr static size_t value = sizeof...(Ts) - 1;
};

template <typename T, typename ...Ts>
struct CountMember < T, void_t<decltype(T{ Ts{}... }) > , Ts... > {
	constexpr static size_t value = CountMember<T, void, Ts..., AnyType>::value;
};