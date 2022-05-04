#pragma once

#include "gui_node_data.h"
#include "../util/static_map.h"

using namespace std::literals::string_view_literals;

struct NodeTypeBase {};

struct NodeTypeImageBase : NodeTypeBase {};

struct NodeTypeValueBase : NodeTypeBase {};

struct NodeTypeShaderBase : NodeTypeBase {};

struct NumberInputWidgetInfo {
	float min;
	float max;
	float speed;
	bool enable_slider;
};

enum class AutoFormat {
	False = 0,
	True = 1
};

enum class FormatEnum {
	False = 0,
	True = 1
};

static constexpr inline StaticMap str_format_map{
	std::pair{"C8 SRGB", VK_FORMAT_R8G8B8A8_SRGB},
	std::pair{"R16 UNORM", VK_FORMAT_R16_UNORM}
};

static constexpr inline auto format_str_array = str_format_map.keys();