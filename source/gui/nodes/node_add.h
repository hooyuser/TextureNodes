#pragma once
#include "../gui_node_base.h"

struct NodeAdd : NodeTypeValueBase {
	struct Info {
		NOTE(value1, NumberInputWidgetInfo{ .min = 0, .max = 0, .speed = 0.02f, .enable_slider = false });
		FloatData value1{
			.value = 0.0f
		};

		NOTE(value2, NumberInputWidgetInfo{ .min = 0, .max = 0, .speed = 0.02f, .enable_slider = false });
		FloatData value2{
			.value = 0.0f
		};

		inline constexpr auto static operation = [](const FloatData value1, const FloatData value2) -> FloatData {
			return { .value = value1.value + value2.value };
		};

		REFLECT(Info,
			value1,
			value2
		)
	};

	struct ResultData {
		FloatData result;

		REFLECT(ResultData,
			result
		)
	};

	using data_type = ValueData<Info, ResultData>;

	//constexpr inline static auto name = "Add";
	constexpr auto static name() { return "Add"; }
};
