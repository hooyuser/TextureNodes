#pragma once
#include <imgui_node_editor.h>
#include <unordered_set>

namespace ed = ax::NodeEditor;

enum class PinInOut {
	INPUT,
	OUTPUT
};

struct Pin {
	ed::PinId id;
	uint32_t node_index;
	std::unordered_set<Pin*> connected_pins;
	std::string name;
	PinInOut flow_direction = PinInOut::INPUT;
	PinVariant default_value;

	template<typename T> requires std::constructible_from<PinVariant, T>
	Pin(const ed::PinId id, const uint32_t node_index, std::string name, const T&) :
		id(id), node_index(node_index), name(name), default_value(T()) {
	}

	bool operator==(const Pin& pin) const noexcept {
		return (this->id == pin.id);
	}

	Pin* connected_pin() const noexcept {
		return *(connected_pins.begin());
	}
};