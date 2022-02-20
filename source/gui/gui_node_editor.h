#pragma once
#include <vector>
#include <string>
#include <unordered_set>
#include <variant>
#include <concepts>
#include <stdexcept>
#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui_node_editor.h>
#include <imgui_internal.h>
#include <imgui_impl_vulkan.h>
#include "Reflect.h"
#include "gui_node_data.h"

namespace ed = ax::NodeEditor;
using namespace Reflect;

enum class PinInOut {
	INPUT,
	OUTPUT
};

struct PinTypeBase {};

struct PinColor : PinTypeBase {
	using data_type = Color4Data;
};

struct PinFloat : PinTypeBase {
	using data_type = float;
};

struct PinImage : PinTypeBase {
	using data_type = TexturePtr;
	using uniform_buffer_type = Color4Data;
};

using PinVariant = std::variant<float, Color4Data, TexturePtr>;

////////////////////////////////////////////////////////////////////////////////////////////

struct NodeTypeBase {};

struct NodeTypeImageBase : NodeTypeBase {};

struct NodeUniformColor : NodeTypeImageBase {

	struct UBO {
		Color4Data color;
		REFLECT(UBO,
			color)
	};

	using data_type = std::shared_ptr<ImageData<UBO,
		"assets/shaders/node_uniform_color.vert.spv",
		"assets/shaders/node_uniform_color.frag.spv"
		>>;

	constexpr auto static name() { return "Uniform Color"; }
};

struct NodeAdd : NodeTypeBase {
	using data_type = FloatData;
	constexpr auto static name() { return "Add"; }
};

using NodeDataVariant = std::variant<NodeUniformColor::data_type, NodeAdd::data_type>;

////////////////////////////////////////////////////////////////////////////////////////////

struct Node;
struct Pin {
	ed::PinId id;
	::Node* node = nullptr;
	std::unordered_set<Pin*> connected_pins;
	std::string name;
	PinInOut flow_direction = PinInOut::INPUT;
	PinVariant default_value;
	
	template<std::derived_from<PinTypeBase> T>
	Pin(ed::PinId id, std::string name, const T&) :
		id(id), name(name), default_value(std::in_place_type<T::data_type>) {
	}

	bool operator==(const Pin& pin) const {
		return (this->id == pin.id);
	}

	ImRect display();
};

enum class NodeState {
	Normal = 0,
	Updated = 1,
	Processing = 2
};

struct Node {
	ed::NodeId id;
	std::string name;
	std::vector<Pin> inputs;
	std::vector<Pin> outputs;
	//ImColor Color ;
	std::string type_name;
	ImVec2 size = { 0, 0 };
	NodeDataVariant data;
	NodeState updated = NodeState::Normal;

	template<std::derived_from<NodeTypeBase> T>
	Node(int id, std::string name, const T&, VulkanEngine* engine) :
		id(id), name(name), type_name(T::name()) {
		if constexpr (std::derived_from<T, NodeTypeImageBase>) {
			using ref_type = std::decay_t<decltype(*std::declval<T::data_type>())>;
			data = std::make_shared<ref_type>(engine);
			auto p = std::holds_alternative<T::data_type>(data);
			auto debug = 0;
		}
	}
};

struct Link {
	ed::LinkId id;

	ed::PinId start_pin_id;
	ed::PinId end_pin_id;

	//ImColor Color;

	Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId) :
		id(id), start_pin_id(startPinId), end_pin_id(endPinId) {}

	bool operator==(const Link& link) const {
		return (this->id == link.id);
	}
};

namespace std {
	template <> struct hash<Link> {
		size_t operator()(const Link& link) const {
			return reinterpret_cast<size_t>(link.id.AsPointer());
		}
	};
}

namespace engine {
	class NodeEditor {
	private:
		VulkanEngine* engine;
		ed::EditorContext* context = nullptr;
		std::vector<Node> nodes;
		std::unordered_set<Link> links;
		int next_id = 1;
		constexpr static inline int image_size = 128;
		//VkCommandBuffer command_buffer = nullptr;

		constexpr int get_next_id();

		void update_from(Node* node);

		void build_node(Node* node);

		Node* create_node_add(std::string name);

		Node* create_node_uniform_color(std::string name);

	public:
		NodeEditor(VulkanEngine* engine);

		void draw();
	};
};

