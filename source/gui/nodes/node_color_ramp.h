#pragma once
#include "../gui_node_base.h"

struct NodeColorRamp : NodeTypeImageBase {

	struct UBO {

		TextureIdData texture{
			.value = -1
		};

		REFLECT(UBO,
			texture
		)
	};

	using data_type = std::shared_ptr<ImageData<UBO,
		"assets/shaders/node_shared_out_uv.vert.spv",
		"assets/shaders/node_color_ramp.frag.spv"
		>>;

	constexpr auto static name() { return "Color Ramp"; }
};