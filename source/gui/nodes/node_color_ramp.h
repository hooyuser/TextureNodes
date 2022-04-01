#pragma once
#include "../gui_node_base.h"

struct NodeColorRamp : NodeTypeImageBase {
	struct UboValue {
		TextureIdData::value_t texture_v;
		ColorRampData::value_t color_ramp_v;
		REFLECT(UboValue,
			texture_v,
			color_ramp_v
		)
	};

	struct UBO {
		using value_t = UboValue;
		TextureIdData texture{
			.value = -1
		};

		ColorRampData color_ramp;

		constexpr auto static inline format = VK_FORMAT_R8G8B8A8_SRGB; 

		REFLECT(UBO,
			texture,
			color_ramp
		)
	};

	using data_type = std::shared_ptr<ImageData<UBO,
		"assets/shaders/node_shared_out_uv.vert.spv",
		"assets/shaders/node_color_ramp.frag.spv"
		>>;

	constexpr auto static name() { return "Color Ramp"; }
};