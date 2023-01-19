#pragma once
#include <cstdint>
#include "gui_node_pin_data.h"

struct ImVec2;


class NodeEditorUIManager
{
	uint32_t node_width;
	float pin_radius;
public:
	explicit NodeEditorUIManager(const uint32_t node_width) {
		this->node_width = node_width;
		pin_radius = static_cast<float>(node_width * 0.03625);
	}
	void draw_node_pin(const ImVec2& center, const PinVariant& pin_data);
};
