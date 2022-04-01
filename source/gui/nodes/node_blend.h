#pragma once
#include "../gui_node_base.h"

struct NodeBlend : NodeTypeImageBase {

	struct UBO {
		NOTE(mode, std::array{"Normal", "Add", "Substract", "Multiply", "Divide"})
		EnumData mode {
			.value = 0
		};

		NOTE(factor, NumberInputWidgetInfo{ .min = 0, .max = 1, .speed = 0.005f, .enable_slider = true })
		FloatTextureIdData factor {
			.value = {
				.number = 0.5f,
				.id = -1 
			}
		};

		alignas(16) Color4TextureIdData texture1{
			.value = {
				.color = {1.0f, 1.0f, 1.0f, 1.0f},
				.id = -1 
			}
		};

		alignas(16) Color4TextureIdData texture2{
			.value = {
				.color = {1.0f, 1.0f, 1.0f, 1.0f},
				.id = -1 
			}
		};

		constexpr auto static inline format = VK_FORMAT_R8G8B8A8_SRGB;

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