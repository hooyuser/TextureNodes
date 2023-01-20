#pragma once
#include "../gui_node_base.h"

struct NodeNormal : NodeTypeImageBase {

	struct Info {

		NOTE(texture, AutoFormat::False)
		TextureIdData texture {
			.value = -1
		};

		NOTE(strength, NumberInputWidgetInfo{ .min = -5, .max = 5, .speed = 0.005f, .enable_slider = false })
		FloatData strength {
			.value = 1.0f,
		};

		NOTE(max_range, NumberInputWidgetInfo{ .min = 0.0001f, .max = 128, .speed = 0.02f, .enable_slider = false })
		FloatData max_range {
			.value = 1.0f,
		};

		BoolData match_size {
			.value = true
		};

		REFLECT(Info,
			texture,
			strength,
			max_range,
			match_size
		)

		constexpr static std::array shader_file_paths{
			"assets/shaders/node_shared_out_uv.vert.spv",
			"assets/shaders/node_normal.frag.spv"
		};

		constexpr auto static default_format = VK_FORMAT_R8G8B8A8_UNORM;
	};

	using data_type = std::shared_ptr<ImageData<ComponentGraphicPipeline<Info>>>;

	constexpr auto static name() { return "Normal"; }
};