#pragma once
#include <tuple>

template <typename T>
concept aggregate = std::is_aggregate_v<std::remove_cvref_t<T>>;

namespace field_counter {
	template <size_t I>
	struct any_type {
		template <class T>
		constexpr operator T() const noexcept;
	};
}

template <aggregate S, size_t... Is>
constexpr size_t detect_fields_count(std::index_sequence<Is...>) noexcept {
	if constexpr (!requires { S{ std::declval<field_counter::any_type<Is>>()... }; }) {
		return sizeof...(Is) - 1;
	}
	else {
		return detect_fields_count<S>(std::make_index_sequence<sizeof...(Is) + 1>{});
	}
}

template <aggregate S>
constexpr size_t count_member_v = detect_fields_count<std::decay_t<S>>(std::make_index_sequence<1>{});

//template<class...>
//using void_t = void;
//
//struct AnyType {
//	template <typename T>
//	operator T();
//};
//
//template <typename T, typename = void, typename ...Ts>
//struct CountMember {
//	constexpr static size_t value = sizeof...(Ts) - 1;
//};
//
//template <typename T, typename ...Ts>
//struct CountMember < T, void_t<decltype(T{ Ts{}... }) > , Ts... > {
//	constexpr static size_t value = CountMember<T, void, Ts..., AnyType>::value;
//};