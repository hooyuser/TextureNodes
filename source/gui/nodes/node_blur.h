#pragma once
#include "../gui_node_base.h"

struct NodeBlur : NodeTypeImageBase {

	struct Info {

		NOTE(texture, AutoFormat::True)
		TextureIdData texture{
			.value = -1
		};

		NOTE(intensity, NumberInputWidgetInfo{ .min = 0, .max = 10, .speed = 0.005f, .enable_slider = false })
		FloatTextureIdData intensity {
			.value = {
				.number = 1.0f,
				.id = -1
			}
		};

		NOTE(samples, NumberInputWidgetInfo{ .min = 0, .max = 256, .speed = 0.05f, .enable_slider = false })
		IntData samples {
			.value = 50
		};

		REFLECT(Info,
			texture,
			intensity,
			samples
		)

		constexpr static std::array shader_file_paths{
			"assets/shaders/node_shared_out_uv.vert.spv",
			"assets/shaders/node_blur.frag.spv"
		};

		constexpr auto static default_format = VK_FORMAT_R8G8B8A8_SRGB;
	};

	using data_type = std::shared_ptr<ImageData<ComponentGraphicPipeline<Info>>>;

	constexpr auto static name() { return "Blur"; }
};