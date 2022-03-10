#pragma once
#include "../gui_node_base.h"

struct NodeBlend : NodeTypeImageBase {

	struct UBO {
		NOTE(mode, std::array{"Normal", "Add", "Substract", "Multiply", "Divide"})
		EnumData mode {
			.value = 0
		};

		NOTE(factor, NumberInputWidgetInfo{ .min = 0, .max = 1, .speed = 0.005, .enable_slider = true })
		FloatData factor {
			.value = 0.5f
		};

		TextureIdData texture1{
			.value = -1
		};

		TextureIdData texture2{
			.value = -1
		};

		REFLECT(UBO,
			mode,
			factor,
			texture1,
			texture2
		)
	};

	using data_type = std::shared_ptr<ImageData<UBO,
		"assets/shaders/node_shared_out_uv.vert.spv",
		"assets/shaders/node_blend.frag.spv"
		>>;

	constexpr auto static name() { return "Blend"; }
};