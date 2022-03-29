#pragma once
#include <tuple>
#include "util.h"

template <typename T>
concept aggregate = std::is_aggregate_v<std::remove_cvref_t<T>>;

template <size_t I>
struct any_type {
	template <class T>
	constexpr operator T() const noexcept;
};
template <aggregate S, size_t... Is>
constexpr size_t detect_fields_count(std::index_sequence<Is...>) noexcept {
	if constexpr (!requires { S{ std::declval<any_type<Is>>()... }; }) {
		return sizeof...(Is) - 1;
	}
	else {
		return detect_fields_count<S>(std::make_index_sequence<sizeof...(Is) + 1>{});
	}
}

template <aggregate S>
constexpr size_t counter_member_v = detect_fields_count<std::decay_t<S>>(std::make_index_sequence<1>{});

#define MACRO_BUILD0(x)
#define MACRO_BUILD1(x)  MACRO_BUILD0(x)   x(1)
#define MACRO_BUILD2(x)  MACRO_BUILD1(x) , x(2)
#define MACRO_BUILD3(x)  MACRO_BUILD2(x) , x(3)
#define MACRO_BUILD4(x)  MACRO_BUILD3(x) , x(4)
#define MACRO_BUILD5(x)  MACRO_BUILD4(x) , x(5)
#define MACRO_BUILD6(x)  MACRO_BUILD5(x) , x(6)
#define MACRO_BUILD7(x)  MACRO_BUILD6(x) , x(7)
#define MACRO_BUILD8(x)  MACRO_BUILD7(x) , x(8)
#define MACRO_BUILD9(x)  MACRO_BUILD8(x) , x(9)
#define MACRO_BUILD10(x) MACRO_BUILD9(x) , x(10)
#define MACRO_BUILD11(x) MACRO_BUILD10(x), x(11)
#define MACRO_BUILD12(x) MACRO_BUILD11(x), x(12)
#define MACRO_BUILD13(x) MACRO_BUILD12(x), x(13)
#define MACRO_BUILD14(x) MACRO_BUILD13(x), x(14)
#define MACRO_BUILD15(x) MACRO_BUILD14(x), x(15)
#define MACRO_BUILD16(x) MACRO_BUILD15(x), x(16)
#define MACRO_BUILD17(x) MACRO_BUILD16(x), x(17)
#define MACRO_BUILD18(x) MACRO_BUILD17(x), x(18)
#define MACRO_BUILD19(x) MACRO_BUILD18(x), x(19)
#define MACRO_BUILD20(x) MACRO_BUILD19(x), x(20)
#define MACRO_BUILD21(x) MACRO_BUILD20(x), x(21)
#define MACRO_BUILD22(x) MACRO_BUILD21(x), x(22)
#define MACRO_BUILD23(x) MACRO_BUILD22(x), x(23)
#define MACRO_BUILD24(x) MACRO_BUILD23(x), x(24)
#define MACRO_BUILD25(x) MACRO_BUILD24(x), x(25)
#define MACRO_BUILD26(x) MACRO_BUILD25(x), x(26)
#define MACRO_BUILD27(x) MACRO_BUILD26(x), x(27)
#define MACRO_BUILD28(x) MACRO_BUILD27(x), x(28)
#define MACRO_BUILD29(x) MACRO_BUILD28(x), x(29)
#define MACRO_BUILD30(x) MACRO_BUILD29(x), x(30)
#define MACRO_BUILD31(x) MACRO_BUILD30(x), x(31)
#define MACRO_BUILD32(x) MACRO_BUILD31(x), x(32)
#define MACRO_BUILD33(x) MACRO_BUILD32(x), x(33)
#define MACRO_BUILD34(x) MACRO_BUILD33(x), x(34)
#define MACRO_BUILD35(x) MACRO_BUILD34(x), x(35)
#define MACRO_BUILD36(x) MACRO_BUILD35(x), x(36)
#define MACRO_BUILD37(x) MACRO_BUILD36(x), x(37)
#define MACRO_BUILD38(x) MACRO_BUILD37(x), x(38)
#define MACRO_BUILD39(x) MACRO_BUILD38(x), x(39)
#define MACRO_BUILD40(x) MACRO_BUILD39(x), x(40)
#define MACRO_BUILD41(x) MACRO_BUILD40(x), x(41)
#define MACRO_BUILD42(x) MACRO_BUILD41(x), x(42)
#define MACRO_BUILD43(x) MACRO_BUILD42(x), x(43)
#define MACRO_BUILD44(x) MACRO_BUILD43(x), x(44)
#define MACRO_BUILD45(x) MACRO_BUILD44(x), x(45)
#define MACRO_BUILD46(x) MACRO_BUILD45(x), x(46)
#define MACRO_BUILD47(x) MACRO_BUILD46(x), x(47)
#define MACRO_BUILD48(x) MACRO_BUILD47(x), x(48)
#define MACRO_BUILD49(x) MACRO_BUILD48(x), x(49)
#define MACRO_BUILD50(x) MACRO_BUILD49(x), x(50)
#define MACRO_BUILD51(x) MACRO_BUILD50(x), x(51)
#define MACRO_BUILD52(x) MACRO_BUILD51(x), x(52)
#define MACRO_BUILD53(x) MACRO_BUILD52(x), x(53)
#define MACRO_BUILD54(x) MACRO_BUILD53(x), x(54)
#define MACRO_BUILD55(x) MACRO_BUILD54(x), x(55)
#define MACRO_BUILD56(x) MACRO_BUILD55(x), x(56)
#define MACRO_BUILD57(x) MACRO_BUILD56(x), x(57)
#define MACRO_BUILD58(x) MACRO_BUILD57(x), x(58)
#define MACRO_BUILD59(x) MACRO_BUILD58(x), x(59)
#define MACRO_BUILD60(x) MACRO_BUILD59(x), x(60)
#define MACRO_BUILD61(x) MACRO_BUILD60(x), x(61)
#define MACRO_BUILD62(x) MACRO_BUILD61(x), x(62)
#define MACRO_BUILD63(x) MACRO_BUILD62(x), x(63)
#define MACRO_BUILD64(x) MACRO_BUILD63(x), x(64)
#define MACRO_BUILD65(x) MACRO_BUILD64(x), x(65)
#define MACRO_BUILD66(x) MACRO_BUILD65(x), x(66)
#define MACRO_BUILD67(x) MACRO_BUILD66(x), x(67)
#define MACRO_BUILD68(x) MACRO_BUILD67(x), x(68)
#define MACRO_BUILD69(x) MACRO_BUILD68(x), x(69)
#define MACRO_BUILD70(x) MACRO_BUILD69(x), x(70)
#define MACRO_BUILD71(x) MACRO_BUILD70(x), x(71)
#define MACRO_BUILD72(x) MACRO_BUILD71(x), x(72)
#define MACRO_BUILD73(x) MACRO_BUILD72(x), x(73)
#define MACRO_BUILD74(x) MACRO_BUILD73(x), x(74)
#define MACRO_BUILD75(x) MACRO_BUILD74(x), x(75)
#define MACRO_BUILD76(x) MACRO_BUILD75(x), x(76)
#define MACRO_BUILD77(x) MACRO_BUILD76(x), x(77)
#define MACRO_BUILD78(x) MACRO_BUILD77(x), x(78)
#define MACRO_BUILD79(x) MACRO_BUILD78(x), x(79)
#define MACRO_BUILD(x, i) MACRO_BUILD##i(x)

#define MACRO_FIELD(n) MACRO_FIELD_##n
#define MACRO_ACC_FIELD(N) MACRO_BUILD(MACRO_FIELD, N)

#define MACRO_STRUCTURE_BINDING(...) __VA_OPT__(auto&& [__VA_ARGS__] = s;)
#define MACRO_TIE(...) return std::forward_as_tuple(__VA_ARGS__);

#define MACRO_IF_FIELD_COUNT(N) if constexpr (count == N) {\
    MACRO_STRUCTURE_BINDING(MACRO_ACC_FIELD(N))\
    MACRO_TIE(MACRO_ACC_FIELD(N))}

#define MACRO_ELSE_IF_FIELD_COUNT(N) else MACRO_IF_FIELD_COUNT(N)


template <aggregate T>
constexpr auto class_field_to_tuple(T&& s) noexcept {
	constexpr auto count = counter_member_v<T>;
	MACRO_IF_FIELD_COUNT(79)
		MACRO_ELSE_IF_FIELD_COUNT(78)
		MACRO_ELSE_IF_FIELD_COUNT(77)
		MACRO_ELSE_IF_FIELD_COUNT(76)
		MACRO_ELSE_IF_FIELD_COUNT(75)
		MACRO_ELSE_IF_FIELD_COUNT(74)
		MACRO_ELSE_IF_FIELD_COUNT(73)
		MACRO_ELSE_IF_FIELD_COUNT(72)
		MACRO_ELSE_IF_FIELD_COUNT(71)
		MACRO_ELSE_IF_FIELD_COUNT(70)
		MACRO_ELSE_IF_FIELD_COUNT(69)
		MACRO_ELSE_IF_FIELD_COUNT(68)
		MACRO_ELSE_IF_FIELD_COUNT(67)
		MACRO_ELSE_IF_FIELD_COUNT(66)
		MACRO_ELSE_IF_FIELD_COUNT(65)
		MACRO_ELSE_IF_FIELD_COUNT(64)
		MACRO_ELSE_IF_FIELD_COUNT(63)
		MACRO_ELSE_IF_FIELD_COUNT(62)
		MACRO_ELSE_IF_FIELD_COUNT(61)
		MACRO_ELSE_IF_FIELD_COUNT(60)
		MACRO_ELSE_IF_FIELD_COUNT(59)
		MACRO_ELSE_IF_FIELD_COUNT(58)
		MACRO_ELSE_IF_FIELD_COUNT(57)
		MACRO_ELSE_IF_FIELD_COUNT(56)
		MACRO_ELSE_IF_FIELD_COUNT(55)
		MACRO_ELSE_IF_FIELD_COUNT(54)
		MACRO_ELSE_IF_FIELD_COUNT(53)
		MACRO_ELSE_IF_FIELD_COUNT(52)
		MACRO_ELSE_IF_FIELD_COUNT(51)
		MACRO_ELSE_IF_FIELD_COUNT(50)
		MACRO_ELSE_IF_FIELD_COUNT(49)
		MACRO_ELSE_IF_FIELD_COUNT(48)
		MACRO_ELSE_IF_FIELD_COUNT(47)
		MACRO_ELSE_IF_FIELD_COUNT(46)
		MACRO_ELSE_IF_FIELD_COUNT(45)
		MACRO_ELSE_IF_FIELD_COUNT(44)
		MACRO_ELSE_IF_FIELD_COUNT(43)
		MACRO_ELSE_IF_FIELD_COUNT(42)
		MACRO_ELSE_IF_FIELD_COUNT(41)
		MACRO_ELSE_IF_FIELD_COUNT(40)
		MACRO_ELSE_IF_FIELD_COUNT(39)
		MACRO_ELSE_IF_FIELD_COUNT(38)
		MACRO_ELSE_IF_FIELD_COUNT(37)
		MACRO_ELSE_IF_FIELD_COUNT(36)
		MACRO_ELSE_IF_FIELD_COUNT(35)
		MACRO_ELSE_IF_FIELD_COUNT(34)
		MACRO_ELSE_IF_FIELD_COUNT(33)
		MACRO_ELSE_IF_FIELD_COUNT(32)
		MACRO_ELSE_IF_FIELD_COUNT(31)
		MACRO_ELSE_IF_FIELD_COUNT(30)
		MACRO_ELSE_IF_FIELD_COUNT(29)
		MACRO_ELSE_IF_FIELD_COUNT(28)
		MACRO_ELSE_IF_FIELD_COUNT(27)
		MACRO_ELSE_IF_FIELD_COUNT(26)
		MACRO_ELSE_IF_FIELD_COUNT(25)
		MACRO_ELSE_IF_FIELD_COUNT(24)
		MACRO_ELSE_IF_FIELD_COUNT(23)
		MACRO_ELSE_IF_FIELD_COUNT(22)
		MACRO_ELSE_IF_FIELD_COUNT(21)
		MACRO_ELSE_IF_FIELD_COUNT(20)
		MACRO_ELSE_IF_FIELD_COUNT(19)
		MACRO_ELSE_IF_FIELD_COUNT(18)
		MACRO_ELSE_IF_FIELD_COUNT(17)
		MACRO_ELSE_IF_FIELD_COUNT(16)
		MACRO_ELSE_IF_FIELD_COUNT(15)
		MACRO_ELSE_IF_FIELD_COUNT(14)
		MACRO_ELSE_IF_FIELD_COUNT(13)
		MACRO_ELSE_IF_FIELD_COUNT(12)
		MACRO_ELSE_IF_FIELD_COUNT(11)
		MACRO_ELSE_IF_FIELD_COUNT(10)
		MACRO_ELSE_IF_FIELD_COUNT(9)
		MACRO_ELSE_IF_FIELD_COUNT(8)
		MACRO_ELSE_IF_FIELD_COUNT(7)
		MACRO_ELSE_IF_FIELD_COUNT(6)
		MACRO_ELSE_IF_FIELD_COUNT(5)
		MACRO_ELSE_IF_FIELD_COUNT(4)
		MACRO_ELSE_IF_FIELD_COUNT(3)
		MACRO_ELSE_IF_FIELD_COUNT(2)
		MACRO_ELSE_IF_FIELD_COUNT(1)
		MACRO_ELSE_IF_FIELD_COUNT(0)
}

template<aggregate Class>
using FieldTypeTuple = std::decay_t<decltype(class_field_to_tuple(std::declval<Class>()))>;

template <typename T, typename Func>
concept apply_func_to_field_tuple = aggregate<T> && std::invoke(
	[] <std::size_t... I> (std::index_sequence<I...>) {
		return (std::invocable<Func, std::tuple_element_t<I, FieldTypeTuple<T>>, uint32_t> && ...);
	}, std::make_index_sequence<counter_member_v<T>>{});

template <typename T, typename Func> //requires apply_func_to_field_tuple<T, Func>
constexpr decltype(auto) for_each_field(T&& s, Func&& func) {
	uint32_t i = 0;
	return std::apply([&](auto&&... args) {
		((FWD(func)(args, i++)), ...);
	}, class_field_to_tuple(FWD(s)));
}