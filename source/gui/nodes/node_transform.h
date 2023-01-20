#pragma once
#include "../gui_node_base.h"

struct NodeTransform : NodeTypeImageBase {

	struct Info {

		NOTE(texture, AutoFormat::True)
		TextureIdData texture{
			.value = -1
		};

		NOTE(shift_x, NumberInputWidgetInfo{ .min = -1, .max = 1, .speed = 0.005f, .enable_slider = true })
			FloatData shift_x {
			.value = 0.0f
		};

		NOTE(shift_y, NumberInputWidgetInfo{ .min = -1, .max = 1, .speed = 0.005f, .enable_slider = true })
			FloatData shift_y {
			.value = 0.0f 
		};

		NOTE(rotation, NumberInputWidgetInfo{ .min = 0, .max = 360, .speed = 1.0f, .enable_slider = true })
			FloatData rotation {
			.value = 0.0f
		};

		NOTE(scale_x, NumberInputWidgetInfo{ .min = -10, .max = 10, .speed = 0.005f, .enable_slider = false })
			FloatData scale_x {
			.value = 1.0f
		};

		NOTE(scale_y, NumberInputWidgetInfo{ .min = -10, .max = 10, .speed = 0.005f, .enable_slider = false })
			FloatData scale_y {
			.value = 1.0f
		};

		BoolData clamp{
			.value = false
		};

		REFLECT(Info,
			texture,
			shift_x,
			shift_y,
			rotation,
			scale_x,
			scale_y,
			clamp
		)

		constexpr static std::array shader_file_paths{
			"assets/shaders/node_shared_out_uv.vert.spv",
			"assets/shaders/node_transform.frag.spv"
		};

		constexpr auto static default_format = VK_FORMAT_R16_UNORM; 
	};

	using data_type = std::shared_ptr<ImageData<ComponentGraphicPipeline<Info>>>;

	constexpr auto static name() { return "Transform"; }
};