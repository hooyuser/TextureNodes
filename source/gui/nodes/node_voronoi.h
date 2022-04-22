#pragma once
#include "../gui_node_base.h"

struct NodeVoronoi : NodeTypeImageBase {

	struct UBO {
		NOTE(dimension, std::array{ "2D", "3D" })
			EnumData dimension {
			.value = 0
		};

		NOTE(method, std::array{ "F1", "Smooth F1", "F2", "Distance to Edge", "Sphere Radius" })
			EnumData method {
			.value = 0
		};

		NOTE(metric, std::array{ "Euclidean", "Manhattan", "Chebychev", "Minkowski" })
			EnumData metric {
			.value = 0
		};

		NOTE(x, NumberInputWidgetInfo{ .min = 0, .max = 0, .speed = 0.0025f, .enable_slider = false })
			FloatData x {
			.value = 0.0f
		};

		NOTE(y, NumberInputWidgetInfo{ .min = 0, .max = 0, .speed = 0.0025f, .enable_slider = false })
			FloatData y {
			.value = 0.0f
		};

		NOTE(z, NumberInputWidgetInfo{ .min = 0, .max = 0, .speed = 0.0025f, .enable_slider = false })
			FloatData z {
			.value = 0.0f
		};

		NOTE(scale, NumberInputWidgetInfo{ .min = 0, .max = 0, .speed = 0.0025f, .enable_slider = false })
			FloatData scale {
			.value = 1.0f
		};

		NOTE(randomness, NumberInputWidgetInfo{ .min = 0, .max = 1, .speed = 0.0025f, .enable_slider = true })
			FloatData randomness {
			.value = 1.0f
		};

		NOTE(smoothness, NumberInputWidgetInfo{ .min = 0, .max = 1, .speed = 0.0025f, .enable_slider = true })
			FloatData smoothness {
			.value = 1.0f
		};

		NOTE(exponent, NumberInputWidgetInfo{ .min = 0, .max = 64.0f, .speed = 0.0025f, .enable_slider = false })
			FloatData exponent {
			.value = 1.0f
		};

		REFLECT(UBO,
			dimension,
			method,
			metric,
			x,
			y,
			z,
			scale,
			randomness,
			smoothness,
			exponent
		)

		constexpr static std::array shader_file_paths{
			"assets/shaders/node_shared_out_uv.vert.spv",
			"assets/shaders/node_voronoi.frag.spv"
		};

		constexpr auto static default_format = VK_FORMAT_R16_UNORM;
	};

	using data_type = std::shared_ptr<ImageData<ComponentGraphicPipeline<UBO>>>;

	constexpr auto static name() { return "Voronoi"; }
};