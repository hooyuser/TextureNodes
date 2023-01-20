#pragma once
#include "../gui_node_base.h"

struct NodeNoise : NodeTypeImageBase {

	struct Info {
		NOTE(format, format_str_array, FormatEnum::True)
		EnumData format {
			.value = static_cast<EnumData::value_t>(str_format_map.index_of(VK_FORMAT_R16_UNORM)),
		};

		NOTE(dimension, std::array{ "1D", "2D", "3D" })
			EnumData dimension {
			.value = 1
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

		NOTE(detail, NumberInputWidgetInfo{ .min = 0, .max = 500, .speed = 0.0025f, .enable_slider = false })
			FloatData detail {
			.value = 2.0f
		};

		NOTE(roughness, NumberInputWidgetInfo{ .min = 0, .max = 1, .speed = 0.0025f, .enable_slider = true })
			FloatData roughness {
			.value = 0.5f
		};

		NOTE(distortion, NumberInputWidgetInfo{ .min = 0, .max = 500, .speed = 0.0025f, .enable_slider = false })
			FloatData distortion {
			.value = 0.0f
		};

		REFLECT(Info,
			format,
			dimension,
			x,
			y,
			z,
			scale,
			detail,
			roughness,
			distortion
		)

		constexpr static std::array shader_file_paths{
			"assets/shaders/node_shared_out_uv.vert.spv",
			"assets/shaders/node_noise.frag.spv"
		};

		constexpr auto static default_format = VK_FORMAT_R16_UNORM;
	};

	using data_type = std::shared_ptr<ImageData<ComponentGraphicPipeline<Info>>>;

	constexpr auto static name() { return "Noise"; }
};