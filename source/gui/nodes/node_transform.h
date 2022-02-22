#pragma once
#include "../gui_node_base.h"

struct NodeTransform : NodeTypeImageBase {

	struct UBO {
		NOTE(shift_x, NumberInputWidgetInfo{ .min = 0, .max = 2, .speed = 0.005, .enable_slider = true })
			FloatData shift_x {
			.value = 0.75f
		};

		NOTE(shift_y, NumberInputWidgetInfo{ .min = 0, .max = 360, .speed = 1.0, .enable_slider = true })
			FloatData shift_y {
			.value = 0.0f
		};

		NOTE(rotation, NumberInputWidgetInfo{ .min = 0, .max = 360, .speed = 1.0, .enable_slider = true })
			FloatData rotation {
			.value = 0.0f
		};

		NOTE(scale_x, NumberInputWidgetInfo{ .min = 0, .max = 2, .speed = 0.005, .enable_slider = true })
			FloatData scale_x {
			.value = 0.75f
		};

		NOTE(scale_y, NumberInputWidgetInfo{ .min = 0, .max = 360, .speed = 1.0, .enable_slider = true })
			FloatData scale_y {
			.value = 0.0f
		};

		BoolData clamp{
			.value = false
		};

		REFLECT(UBO,
			shift_x,
			shift_y,
			rotation,
			scale_x,
			scale_y,
			clamp
		)
	};

	using data_type = std::shared_ptr<ImageData<UBO,
		"assets/shaders/node_polygon.vert.spv",
		"assets/shaders/node_polygon.frag.spv"
		>>;

	constexpr auto static name() { return "Transform"; }
};