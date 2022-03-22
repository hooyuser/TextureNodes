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
	}, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
};
template <typename T, typename Tuple>
static constexpr bool any_of_tuple_v = any_of_tuple<T, Tuple>::value; //check if T is a subtype of Tuple, Tuple is std::tuple

template <typename, template <typename> class>
struct filter;

template <typename ... Ts, template <typename> typename Predicate>
	requires (std::same_as<std::decay_t<decltype(Predicate<Ts>::value)>, bool> && ...)
struct filter<std::tuple<Ts...>, Predicate> {
	using type = decltype(std::tuple_cat(std::declval<
		std::conditional_t<Predicate<Ts>::value,
		std::tuple<Ts>,
		std::tuple<>>>()...));
};

template <typename Tpl, template <typename> typename Predicate>
using filter_t = typename filter<Tpl, Predicate>::type;

template <template<typename ...> typename TypeSetFrom, template<typename ...> typename TypeSetTo, typename ...Types>
consteval auto convert_type_list_to(TypeSetFrom<Types...>, TypeSetTo<>)->TypeSetTo<Types...>;


template<typename... Ts>
struct TypeList {
	//Member constants
	static constexpr size_t size = sizeof...(Ts);

	template<typename T>
	static constexpr bool has_type = any_of_tuple_v<T, std::tuple<Ts...>>;

	//Member types
	template <typename TypeSetFrom>
	using from = decltype(convert_type_list_to(std::declval<std::remove_reference_t<TypeSetFrom>>(), std::declval<TypeList<>>()));

	template<template<typename...> class TypeSetTo>
	using cast_to = TypeSetTo<Ts...>;

	using to_tuple = cast_to<std::tuple>;

	using remove_ref = TypeList<std::remove_reference_t<Ts>...>;

	template<size_t I>
	using at = std::tuple_element_t<I, std::tuple<Ts...>>;

	template<template<typename...> class Predicate>
	using filtered_by = from<filter_t<std::tuple<Ts...>, Predicate>>;

	//Member functions
	template<typename Func>
	static void for_each(Func&& func) {  //loop over types
		(FWD(func).template operator() < Ts > (), ...);
	}
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