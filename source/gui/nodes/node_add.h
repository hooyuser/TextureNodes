#pragma once
#include "../gui_node_base.h"

struct NodeAdd : NodeTypeBase {
	struct UBO {
		NOTE(value1, NumberInputWidgetInfo{ .min = 0, .max = 0, .speed = 0.02, .enable_slider = false });
		FloatData value1{
			.value = 0.0f
		};

		NOTE(value2, NumberInputWidgetInfo{ .min = 0, .max = 0, .speed = 0.02, .enable_slider = false });
		FloatData value2{
			.value = 0.0f
		};

		REFLECT(UBO,
			value1,
			value2
		)
	};
	using data_type = NonImageData<UBO, FloatData>;

	//constexpr inline static auto name = "Add";
	constexpr auto static name() { return "Add"; }
};
