#pragma once
#include <cstdint>
#include <format>

#include "gui_node_pin_data.h"
#include "gui_node_pin.h"


struct ImVec2;
struct Pin;

class NodeEditorUIManager
{
	uint32_t node_width;
	float pin_radius;
public:
	explicit NodeEditorUIManager(const uint32_t node_width) {
		this->node_width = node_width;
		pin_radius = static_cast<float>(node_width * 0.03625);
	}

	template<std::invocable<Pin&> Func>
	void draw_pin_popup(Pin& pin, const bool hit_pin, Func&& func) {
		if (hit_pin) {
			ImGui::OpenPopup(std::format("PinPopup##{}", pin.id.Get()).c_str());
		}

		if (ImGui::BeginPopup(std::format("PinPopup##{}", pin.id.Get()).c_str())) {
			FWD(func)(pin);
			ImGui::EndPopup();
		}
	}

	void draw_node_pin(const ImVec2& center, const PinVariant& pin_data) const;
};
