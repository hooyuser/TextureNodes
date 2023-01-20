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

	struct Info {
		using value_t = UboValue;

		NOTE(format, format_str_array, FormatEnum::True)
		EnumData format {
			.value = static_cast<EnumData::value_t>(str_format_map.index_of(VK_FORMAT_R8G8B8A8_SRGB)),
		};

		TextureIdData texture{
			.value = -1
		};

		ColorRampData color_ramp;

		REFLECT(Info,
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

	using data_type = std::shared_ptr<ImageData<ComponentGraphicPipeline<Info>>>;

	constexpr auto static name() { return "Color Ramp"; }
};