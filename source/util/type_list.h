#pragma once
#include <tuple>
#include <concepts>
#include "util.h"

template <typename T>
concept std_tuple = requires (T t) {
	[] <typename... Ts> (std::tuple<Ts...>) {}(t);
};

template<typename UnknownType, typename ...ReferenceTypes>
concept any_of = (std::same_as<std::decay_t<UnknownType>, std::decay_t<ReferenceTypes>> || ...);

//check if `T` is a subtype of `Tuple`, where `Tuple` is a std::tuple
template <typename T, typename Tuple>
concept any_of_tuple = std_tuple<Tuple> && std::invoke([] <std::size_t... I> (std::index_sequence<I...>) {
		return any_of<T, std::tuple_element_t<I, Tuple>...>;
	}, std::make_index_sequence<std::tuple_size_v<Tuple>>{});

/**
 * filter_by_template_predicate /////////////////////////////////////////////////
 */
template <std_tuple, template <typename> class>
struct filter_by_template_predicate;

template <typename ... Ts, template <typename> typename Predicate>
	requires (std::same_as<std::decay_t<decltype(Predicate<Ts>::value)>, bool> && ...)
struct filter_by_template_predicate<std::tuple<Ts...>, Predicate> {
	using type = decltype(std::tuple_cat(std::declval<
		std::conditional_t<Predicate<Ts>::value,
		std::tuple<Ts>,
		std::tuple<>>>()...));
};

template <std_tuple Tuple, template <typename> typename Predicate>
using filter_by_template_predicate_t = typename filter_by_template_predicate<Tuple, Predicate>::type;

/**
 * filter_by_functor_type_predicate /////////////////////////////////////////////////
 */

template <std_tuple Tuple, typename FuncPredicate>
using filter_by_functor_type_predicate = std::invoke_result_t<decltype(
	[] <typename... Ts> (std::tuple<Ts...>) requires (std::predicate<FuncPredicate, Ts> && ...) {
	return std::tuple_cat(
		std::conditional_t < FuncPredicate{}(Ts{}),
		std::tuple<Ts>,
		std::tuple<>
		> {}...);
}
), Tuple > ;

/**
 * filter_by_func_predicate /////////////////////////////////////////////////
 */

template <std_tuple Tuple, auto FuncPredicate>
using filter_by_func_predicate = std::invoke_result_t<decltype(
	[] <typename... Ts> (std::tuple<Ts...>) requires (std::predicate<decltype(FuncPredicate), Ts> && ...) {
	return std::tuple_cat(
		std::conditional_t < FuncPredicate(Ts{}),
		std::tuple<Ts>,
		std::tuple<>
		> {}...);
}
), Tuple > ;

template <template<typename ...> typename TypeSetFrom, template<typename ...> typename TypeSetTo, typename ...Types>
consteval auto convert_type_list_to(TypeSetFrom<Types...>, TypeSetTo<>)->TypeSetTo<Types...>;

template<typename... Ts>
struct TypeList {
	//Member constants
	static constexpr size_t size = sizeof...(Ts);

	template<typename T>
	static constexpr bool has_type = any_of_tuple<T, std::tuple<Ts...>>;

	//Member types
	template <typename TypeSetFrom>
	using from = decltype(convert_type_list_to(std::declval<std::decay_t<TypeSetFrom>>(), std::declval<TypeList<>>()));

	template<template<typename...> class TypeSetTo>
	using cast_to = TypeSetTo<Ts...>;

	using to_tuple = cast_to<std::tuple>;

	using remove_ref = TypeList<std::remove_reference_t<Ts>...>;

	template<size_t I>
	using at = std::tuple_element_t<I, std::tuple<Ts...>>;

	template<auto Predicate>
	using filtered_by = from<filter_by_func_predicate<std::tuple<Ts...>, Predicate>>;

	template<typename Predicate>
	using filtered_by_functor_predicate = from<filter_by_functor_type_predicate<std::tuple<Ts...>, Predicate>>;

	template<template<typename...> typename Predicate>
	using filtered_by_template_predicate = from<filter_by_template_predicate_t<std::tuple<Ts...>, Predicate>>;

	//Member functions
	template<typename Func>
	static void for_each(Func&& func) {  //loop over types
		(FWD(func).template operator() < Ts > (), ...);
	}
};

template <typename TypeSetFrom>
using to_type_list = TypeList<>::from<TypeSetFrom>;

template <template<typename ...> typename TypeSet, typename ...Ts>
consteval auto concat(TypeSet<Ts...>)->TypeSet<Ts...>;

template <template<typename ...> typename TypeSet, typename ...Ts1, typename ...Ts2>
consteval auto concat(TypeSet<Ts1...>, TypeSet<Ts2...>)->TypeSet<Ts1..., Ts2...>;

template<template<typename ...> typename TypeSet, typename ...Ts1, typename ...Ts2, typename... Rest>
consteval auto concat(TypeSet<Ts1...>, TypeSet<Ts2...>, Rest...) -> decltype(concat(std::declval<TypeSet<Ts1..., Ts2...>>(), std::declval<Rest>()...));

template <typename ...TypeSetFrom>
using concat_t = std::decay_t<decltype(concat(std::declval<TypeSetFrom>()...))>;

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

template <typename T>
concept is_type_list = requires (T t) {
	[] <typename... Ts> (TypeList<Ts...>) {}(t);
};