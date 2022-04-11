#pragma once
#include "../gui_node_base.h"

struct NodeUdf : NodeTypeImageBase {

	struct UBO {

		TextureIdData texture{
			.value = -1
		};

		NOTE(max_distance, NumberInputWidgetInfo{ .min = 1, .max = 120000, .speed = 3.0f, .enable_slider = true })
			FloatData max_distance {
			.value = 800.0f
		};

		REFLECT(UBO,
			texture,
			max_distance
		)

		constexpr static std::array shader_file_paths{
			"assets/shaders/node_udf_preprocess.comp.spv"
			"assets/shaders/node_udf_process.comp.spv"
		};

		constexpr auto static default_format = VK_FORMAT_R16_UNORM; 
	};

	using data_type = std::shared_ptr<ImageData<ComponentUdf<UBO>>>;

	constexpr auto static name() { return "Udf"; }
};