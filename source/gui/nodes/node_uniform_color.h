#pragma once
#include "../gui_node_base.h"

struct NodeUniformColor : NodeTypeImageBase {

	struct UBO {
		Color4Data color;

		constexpr auto static inline default_format = VK_FORMAT_R8G8B8A8_SRGB;

		REFLECT(UBO,
			color
		)
	};

	using data_type = std::shared_ptr<ImageData<UBO,
		"assets/shaders/node_uniform_color.vert.spv",
		"assets/shaders/node_uniform_color.frag.spv"
		>>;

	constexpr auto static name() { return "Uniform Color"; }
};