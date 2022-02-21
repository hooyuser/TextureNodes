#pragma once
#include <tuple>
#include <concepts>

template<typename... Ts>
struct TypeList {
	template<typename Func>
	static void for_each(Func&& func) {
		(func.template operator() < Ts > (), ...);
	}

	template<template<typename...> class U>
	using cast_to = U<Ts...>;

	using to_tuple = cast_to<std::tuple>;
};

template <template<typename ...> typename TypeSetFrom, template<typename ...> typename TypeSetTo, typename ...Types>
constexpr auto convert_type_list_to(TypeSetFrom<Types...>, TypeSetTo<>)->TypeSetTo<Types...>;

template <template<typename ...> typename TypeSetFrom, typename ...Types>
constexpr auto convert_type_list_to_tuple(TypeSetFrom<Types...>)->std::tuple<Types...>;

template <template<typename ...> typename TypeSet, typename ...Ts>
constexpr auto concat(TypeSet<Ts...>)->TypeSet<Ts...>;

template <template<typename ...> typename TypeSet, typename ...Ts1, typename ...Ts2>
constexpr auto concat(TypeSet<Ts1...>, TypeSet<Ts2...>)->TypeSet<Ts1..., Ts2...>;

template<template<typename ...> typename TypeSet, typename ...Ts1, typename ...Ts2, typename... Rest>
constexpr auto concat(TypeSet<Ts1...>, TypeSet<Ts2...>, Rest...) -> decltype(concat(std::declval<TypeSet<Ts1..., Ts2...>>(), std::declval<Rest>()...));

template <template<typename, typename ...> typename TypeSet, typename T, typename ...Rest >
constexpr auto make_unique_typeset(TypeSet<T, Rest...>) {
	if constexpr ((std::same_as<T, Rest> || ...)) {
		return make_unique_typeset(TypeSet<Rest... >{});
	}
	else if constexpr (sizeof...(Rest) > 0) {
		return concat(TypeSet<T>{}, make_unique_typeset(TypeSet<Rest...>{}));
	}
	else {
		return TypeSet<T>{};
	}
}

template <typename, template <typename> class>
struct filter;

template <typename ... Ts, template <typename> class Pred>
struct filter<std::tuple<Ts...>, Pred> {
	using type = decltype(std::tuple_cat(std::declval<
		std::conditional_t<Pred<Ts>::value,
		std::tuple<Ts>,
		std::tuple<>>>()...));
};

template <typename Tpl, template <typename> class Pred>
using filter_t = typename filter<Tpl, Pred>::type;