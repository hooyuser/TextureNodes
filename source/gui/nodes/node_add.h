#pragma once
#include "../gui_node_base.h"

struct NodeAdd : NodeTypeValueBase {
	struct UBO {
		NOTE(value1, NumberInputWidgetInfo{ .min = 0, .max = 0, .speed = 0.02f, .enable_slider = false });
		FloatData value1{
			.value = 0.0f
		};

		NOTE(value2, NumberInputWidgetInfo{ .min = 0, .max = 0, .speed = 0.02f, .enable_slider = false });
		FloatData value2{
			.value = 0.0f
		};

		REFLECT(UBO,
			value1,
			value2
		)
	};

	struct ResultData {
		FloatData result;
		//std::tuple calc { [&](FloatData value1, FloatData value2)-> FloatData {return FloatData{.value=value1.value+ value2.value} };
		REFLECT(ResultData,
			result
		)
	};

	using data_type = NonImageData < UBO, ResultData,
		[](FloatData value1, FloatData value2) -> FloatData {
		return { .value = value1.value + value2.value };
	} > ;

	//constexpr inline static auto name = "Add";
	constexpr auto static name() { return "Add"; }
};
