#pragma once
#include "Reflect.h"
#include "gui_node_data.h"

using namespace Reflect;

struct NodeTypeBase {};

struct NodeTypeImageBase : NodeTypeBase {};

struct NumberInputWidgetInfo {
	float min;
	float max;
	float speed;
	bool enable_slider;
};