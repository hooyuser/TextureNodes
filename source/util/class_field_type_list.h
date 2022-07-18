#pragma once
#include "class_field_counter.h"
#include "util.h"

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
#define MACRO_BUILD80(x) MACRO_BUILD69(x), x(80)
#define MACRO_BUILD81(x) MACRO_BUILD80(x), x(81)
#define MACRO_BUILD82(x) MACRO_BUILD81(x), x(82)
#define MACRO_BUILD83(x) MACRO_BUILD82(x), x(83)
#define MACRO_BUILD84(x) MACRO_BUILD83(x), x(84)
#define MACRO_BUILD85(x) MACRO_BUILD84(x), x(85)
#define MACRO_BUILD86(x) MACRO_BUILD85(x), x(86)
#define MACRO_BUILD87(x) MACRO_BUILD86(x), x(87)
#define MACRO_BUILD88(x) MACRO_BUILD87(x), x(88)
#define MACRO_BUILD89(x) MACRO_BUILD88(x), x(89)
#define MACRO_BUILD90(x) MACRO_BUILD89(x), x(90)
#define MACRO_BUILD91(x) MACRO_BUILD90(x), x(91)
#define MACRO_BUILD92(x) MACRO_BUILD91(x), x(92)
#define MACRO_BUILD93(x) MACRO_BUILD92(x), x(93)
#define MACRO_BUILD94(x) MACRO_BUILD93(x), x(94)
#define MACRO_BUILD95(x) MACRO_BUILD94(x), x(95)
#define MACRO_BUILD96(x) MACRO_BUILD95(x), x(96)
#define MACRO_BUILD97(x) MACRO_BUILD96(x), x(97)
#define MACRO_BUILD98(x) MACRO_BUILD97(x), x(98)
#define MACRO_BUILD99(x) MACRO_BUILD98(x), x(99)

#define MACRO_BUILD(x, i) MACRO_BUILD##i(x)

#define MACRO_FIELD(n) MACRO_FIELD_##n
#define MACRO_ACC_FIELD(N) MACRO_BUILD(MACRO_FIELD, N)

#define MACRO_STRUCTURE_BINDING(...) __VA_OPT__(auto&& [__VA_ARGS__] = s;)
#define MACRO_TIE(...) return std::forward_as_tuple(__VA_ARGS__);

#define MACRO_IF_FIELD_COUNT(N) if constexpr (count == N) { \
    MACRO_STRUCTURE_BINDING(MACRO_ACC_FIELD(N)) \
    MACRO_TIE(MACRO_ACC_FIELD(N))}

#define MACRO_IF_FIELD_COUNT10(m, i) \
    MACRO_IF_FIELD_COUNT(i##0) \
    MACRO_IF_FIELD_COUNT(i##1) \
    MACRO_IF_FIELD_COUNT(i##2) \
    MACRO_IF_FIELD_COUNT(i##3) \
    MACRO_IF_FIELD_COUNT(i##4) \
    MACRO_IF_FIELD_COUNT(i##5) \
    MACRO_IF_FIELD_COUNT(i##6) \
    MACRO_IF_FIELD_COUNT(i##7) \
    MACRO_IF_FIELD_COUNT(i##8) \
    MACRO_IF_FIELD_COUNT(i##9)

template <aggregate T>
constexpr auto class_field_to_tuple(T&& s) noexcept {
	constexpr auto count = count_member_v<T>;
	MACRO_IF_FIELD_COUNT10(m, )
	MACRO_IF_FIELD_COUNT10(m, 1)
	MACRO_IF_FIELD_COUNT10(m, 2)
	MACRO_IF_FIELD_COUNT10(m, 3)
	MACRO_IF_FIELD_COUNT10(m, 4)
	MACRO_IF_FIELD_COUNT10(m, 5)
	MACRO_IF_FIELD_COUNT10(m, 6)
	MACRO_IF_FIELD_COUNT10(m, 7)
	MACRO_IF_FIELD_COUNT10(m, 8)
	MACRO_IF_FIELD_COUNT10(m, 9)
}

template<aggregate Class>
using FieldTypeTuple = std::decay_t<decltype(class_field_to_tuple(std::declval<Class>()))>;

template <typename T, typename Func>
concept apply_func_to_field_tuple = aggregate<T> && std::invoke(
	[] <std::size_t... I> (std::index_sequence<I...>) {
	return (std::invocable<Func, std::tuple_element_t<I, FieldTypeTuple<T>>, uint32_t> && ...);
}, std::make_index_sequence<count_member_v<T>>{});

template <typename T, typename Func> //requires apply_func_to_field_tuple<T, Func>
constexpr decltype(auto) for_each_field(T&& s, Func&& func) {
	uint32_t i = 0;
	return std::apply(
		[&](auto&&... args) {
			((FWD(func)(args, i++)), ...);
		}, class_field_to_tuple(FWD(s)));
}

template <typename T, typename Func>
constexpr decltype(auto) field_at(T&& s, size_t field_index, Func&& func) {
	return UNROLL<count_member_v<T>>([field_index, func = FWD(func), s = FWD(s)]<size_t I>() {
		if (field_index == I) {
			func(std::get<I>(class_field_to_tuple(s)));
		}
	});
}
