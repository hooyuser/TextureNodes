#pragma once

#include "gui_node_data.h"

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
