#pragma once
#include "../gui_node_base.h"

struct NodeColorRamp : NodeTypeImageBase {
	struct UboValue {
		EnumData::value_t format_v;
		TextureIdData::value_t texture_v;
		ColorRampData::value_t color_ramp_v;
		REFLECT(UboValue,
			format_v,
			texture_v,
			color_ramp_v
		)
	};

	struct UBO {
		using value_t = UboValue;

		NOTE(format, format_str_array, FormatEnum::True)
		EnumData format {
			.value = 0
		};

		TextureIdData texture{
			.value = -1
		};

		ColorRampData color_ramp;

		REFLECT(UBO,
			format,
			texture,
			color_ramp
		)

		constexpr static std::array shader_file_paths{
			"assets/shaders/node_shared_out_uv.vert.spv",
			"assets/shaders/node_color_ramp.frag.spv"
		};

		constexpr auto static default_format = VK_FORMAT_R8G8B8A8_SRGB; 
	};

	using data_type = std::shared_ptr<ImageData<UBO>>;

	constexpr auto static name() { return "Color Ramp"; }
};