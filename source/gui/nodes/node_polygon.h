#pragma once
#include "../gui_node_base.h"

struct NodePolygon : NodeTypeImageBase {

	struct Info {
		NOTE(radius, NumberInputWidgetInfo{ .min = 0, .max = 2, .speed = 0.005f, .enable_slider = true })
			FloatData radius {
			.value = 0.75f
		};

		NOTE(angle, NumberInputWidgetInfo{ .min = 0, .max = 360, .speed = 1.0, .enable_slider = true })
			FloatData angle {
			.value = 0.0f
		};

		NOTE(sides, NumberInputWidgetInfo{ .min = 0, .max = 128, .speed = 0.1f, .enable_slider = false })
			IntData sides {
			.value = 4
		};

		NOTE(gradient, NumberInputWidgetInfo{ .min = 0, .max = 1, .speed = 0.0025f, .enable_slider = true })
			FloatData gradient {
			.value = 0.0f
		};

		REFLECT(Info,
			radius,
			angle,
			sides,
			gradient
		)

		constexpr static std::array shader_file_paths{
			"assets/shaders/node_shared_out_uv.vert.spv",
			"assets/shaders/node_polygon.frag.spv"
		};

		constexpr auto static default_format = VK_FORMAT_R16_UNORM; 
	};

	using data_type = std::shared_ptr<ImageData<ComponentGraphicPipeline<Info>>>;

	constexpr auto static name() { return "Polygon"; }
};