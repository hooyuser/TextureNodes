#pragma once
#include "../gui_node_base.h"

struct NodePbrShader : NodeTypeShaderBase {

	struct Info {
		NOTE(base_color, NumberInputWidgetInfo{ .min = 0, .max = 1, .speed = 0.005f, .enable_slider = true })
		Color4TextureIdData base_color {
			.value = {
				.color = {0.8f, 0.8f, 0.8f, 1.0f},
				.id = -1 
			}
		};

		NOTE(metallic, NumberInputWidgetInfo{ .min = 0, .max = 1, .speed = 0.005f, .enable_slider = true })
		FloatTextureIdData metallic {
			.value = {
				.number = 0.0f,
				.id = -1 
			}
		};

		NOTE(roughness, NumberInputWidgetInfo{ .min = 0, .max = 1, .speed = 0.005f, .enable_slider = true })
		FloatTextureIdData roughness {
			.value = {
				.number = 0.4f,
				.id = -1 
			}
		};

		TextureIdData normal{
			.value = -1
		};

		NOTE(specular, NumberInputWidgetInfo{ .min = 0, .max = 1, .speed = 0.005f, .enable_slider = true })
		FloatTextureIdData specular {
			.value = {
				.number = 0.5f,
				.id = -1 
			}
		};

		REFLECT(Info,
			base_color,
			metallic,
			roughness,
			normal,
			specular
		)
	};

	using data_type = ShaderData<Info>;

	constexpr auto static name() { return "Pbr Shader"; }
};