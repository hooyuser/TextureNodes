#pragma once
#include "../gui_node_base.h"

struct NodeAdd : NodeTypeBase {
	struct InputWidget {
		NOTE(value1, NumberInputWidgetInfo{ .min = 0, .max = 0, .speed = 0.005, .enable_slider = false });
		FloatData value1{
			.value = 1.0f
		};


		NOTE(value2, NumberInputWidgetInfo{ .min = 0, .max = 0, .speed = 0.005, .enable_slider = false });
		FloatData value2{
			.value = 1.0f
		};

		REFLECT(InputWidget,
			value1,
			value2
		)
	};
	using data_type = FloatData;
	constexpr auto static name() { return "Add"; }
};
