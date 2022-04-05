#pragma once
#include "../gui_node_base.h"

struct NodeUniformColor : NodeTypeImageBase {

	struct UBO {
		Color4Data color;
		
		REFLECT(UBO,
			color
		)

		constexpr static std::array shader_file_paths{
			"assets/shaders/node_uniform_color.vert.spv",
			"assets/shaders/node_uniform_color.frag.spv"
		};

		constexpr auto static default_format = VK_FORMAT_R8G8B8A8_SRGB;
	};

	using data_type = std::shared_ptr<ImageData<UBO>>;

	constexpr auto static name() { return "Uniform Color"; }
};