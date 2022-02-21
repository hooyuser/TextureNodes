#pragma once

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