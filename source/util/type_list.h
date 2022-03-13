#pragma once
#include <tuple>
#include <concepts>
#include "util.h"

template<typename UnknownType, typename ...ReferenceTypes>
concept any_of = (std::same_as<std::decay_t<UnknownType>, ReferenceTypes> || ...);

template <typename T, typename Tuple>
struct any_of_tuple {
	static constexpr bool value = std::invoke([] <std::size_t... I> (std::index_sequence<I...>) {
		return any_of<T, std::tuple_element_t<I, Tuple>...>;
	}, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
};
template <typename T, typename Tuple>
static constexpr bool any_of_tuple_v = any_of_tuple<T, Tuple>::value; //chech if T is a subtype of Tuple, Tuple is std::tuple

template <typename, template <typename> class>
struct filter;

template <typename ... Ts, template <typename> typename Pred>
	requires (std::same_as<std::decay_t<decltype(Pred<Ts>::value)>, bool> && ...)
struct filter<std::tuple<Ts...>, Pred> {
	using type = decltype(std::tuple_cat(std::declval<
		std::conditional_t<Pred<Ts>::value,
		std::tuple<Ts>,
		std::tuple<>>>()...));
};

template <typename Tpl, template <typename> typename Pred>
using filter_t = typename filter<Tpl, Pred>::type;

template <template<typename ...> typename TypeSetFrom, template<typename ...> typename TypeSetTo, typename ...Types>
consteval auto convert_type_list_to(TypeSetFrom<Types...>, TypeSetTo<>)->TypeSetTo<Types...>;

template<typename... Ts>
struct TypeList {
	template<typename Func>
	static void for_each(Func&& func) {
		(FWD(func).template operator() < Ts > (), ...);
	}

	template<template<typename...> class U>
	using cast_to = U<Ts...>;

	using to_tuple = cast_to<std::tuple>;

	template <typename FromT>
	using from = decltype(convert_type_list_to(std::declval<FromT>(), std::declval<TypeList<>>()));

	template<size_t I>
	using element_t = std::tuple_element_t<I, std::tuple<Ts...>>;

	template<template<typename...> class Pred>
	using filtered_by = from<filter_t<std::tuple<Ts...>, Pred>>;

	template<typename T>
	static constexpr bool has_type = any_of_tuple_v<T, std::tuple<Ts...>>;
};



template <template<typename ...> typename TypeSet, typename ...Ts>
consteval auto concat(TypeSet<Ts...>)->TypeSet<Ts...>;

template <template<typename ...> typename TypeSet, typename ...Ts1, typename ...Ts2>
consteval auto concat(TypeSet<Ts1...>, TypeSet<Ts2...>)->TypeSet<Ts1..., Ts2...>;

template<template<typename ...> typename TypeSet, typename ...Ts1, typename ...Ts2, typename... Rest>
consteval auto concat(TypeSet<Ts1...>, TypeSet<Ts2...>, Rest...) -> decltype(concat(std::declval<TypeSet<Ts1..., Ts2...>>(), std::declval<Rest>()...));

template <template<typename, typename ...> typename TypeSet, typename T, typename ...Rest >
consteval auto make_unique_typeset(TypeSet<T, Rest...>) {
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

