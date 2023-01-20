#pragma once
#include "../gui_node_base.h"

struct NodeSlopeBlur : NodeTypeImageBase {

	struct Info {

		NOTE(texture, AutoFormat::True)
		TextureIdData texture{
			.value = -1
		};

		TextureIdData slope{
			.value = -1
		};

		NOTE(intensity, NumberInputWidgetInfo{ .min = 0, .max = 10, .speed = 0.005f, .enable_slider = false })
			FloatTextureIdData intensity {
			.value = {
				.number = 1.0f,
				.id = -1
			}
		};

		NOTE(quality, NumberInputWidgetInfo{ .min = 0, .max = 20, .speed = 0.05f, .enable_slider = false })
			FloatData quality {
			.value = 5
		};

		REFLECT(Info,
			texture,
			slope,
			intensity,
			quality
		)

		constexpr static std::array shader_file_paths{
			"assets/shaders/node_shared_out_uv.vert.spv",
			"assets/shaders/node_slope_blur.frag.spv"
		};

		constexpr auto static default_format = VK_FORMAT_R8G8B8A8_SRGB;
	};

	using data_type = std::shared_ptr<ImageData<ComponentGraphicPipeline<Info>>>;

	constexpr auto static name() { return "Slope Blur"; }
};