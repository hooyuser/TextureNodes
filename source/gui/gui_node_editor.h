#pragma once
#include <vector>
#include <string>
#include <unordered_set>
#include <variant>
#include <imgui_node_editor.h>
#include <imgui_internal.h>

#include "../vk_image.h"
#include "../vk_buffer.h"

class VulkanEngine;

namespace ed = ax::NodeEditor;

struct NodeResult {};

struct ImageData : public NodeResult {
	TexturePtr texture;
	BufferPtr uniform_buffer;
	inline static VkDescriptorSetLayout descriptor_set_layout = nullptr;
	VkDescriptorSet descriptor_set;

	ImageData(VulkanEngine* engine);
};

using ImageDataPtr = std::shared_ptr<ImageData>;

struct FloatData : public NodeResult {
	float value = 0.0f;
	FloatData(VulkanEngine* engine) {};
};

struct Float3Data : public NodeResult {
	float value[4] = { 0.0f };
	Float3Data(VulkanEngine* engine = nullptr) {};
};

enum class PinInOut {
	INPUT,
	OUTPUT
};

struct PinBase {};

struct PinColor : PinBase {
	using data_type = Float3Data;
};

struct PinFloat : PinBase {
	using data_type = float;
};

struct NodeBase {};

struct NodeUniformColor: NodeBase {
	using result_type = ImageData;
	constexpr auto static name() { return "Uniform Color"; }
};

struct NodeAdd: NodeBase {
	using result_type = FloatData;
	constexpr auto static name() { return "Add"; }
};

struct Node;
struct Pin {
	ed::PinId id;
	::Node* node = nullptr;
	std::unordered_set<Pin*> connected_pins;
	std::string name;
	PinInOut flow_direction = PinInOut::INPUT;
	std::variant<float, Float3Data> default_value;

	template<typename T>
	Pin(ed::PinId id, std::string name, const T&) :id(id), name(name) {
		using pin_data_t = T::data_type;
		default_value = pin_data_t{};
	}

	ImRect display();
};

struct Node {
	ed::NodeId id;
	std::string name;
	std::vector<Pin> inputs;
	std::vector<Pin> outputs;
	//ImColor Color ;
	std::string type_name;
	ImVec2 size = { 0, 0 };
	std::shared_ptr<NodeResult> result;

	template<typename T>
	Node(int id, std::string name, const T&, VulkanEngine* engine) : id(id), name(name) {
		type_name = T::name();
		auto result_data = std::make_shared<T::result_type>(engine);
		result = std::static_pointer_cast<NodeResult>(result_data);
	}
};

struct Link {
	ed::LinkId id;

	ed::PinId start_pin_id;
	ed::PinId end_pin_id;

	//ImColor Color;

	Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId) :
		id(id), start_pin_id(startPinId), end_pin_id(endPinId) {}

	bool operator==(const Link& link) const
	{
		return (this->id == link.id);
	}
};

namespace std {
	template <> struct hash<Link> {
		size_t operator()(const Link& link) const
		{
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

		constexpr int get_next_id();

		void build_node(Node* node);

		Node* create_node_add(std::string name);

		Node* create_node_uniform_color(std::string name);

	public:
		NodeEditor(VulkanEngine* engine);

		void draw();
	};
};

