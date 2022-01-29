#pragma once
#include <vector>
#include <string>
#include <variant>
#include <imgui_node_editor.h>

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
	float value;
	FloatData(VulkanEngine* engine) {};
};

enum class PinType {
	IMAGE,
	FLOAT
};

enum class PinInOut {
	INPUT,
	OUTPUT
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
	std::vector<Node*> connect_nodes;
	std::string name;
	PinType type;
	PinInOut flow_direction = PinInOut::INPUT;
	std::variant<float> default_value;

	Pin(ed::PinId id, std::string name, PinType type) :
		id(id), name(name), type(type) {}

	void display();
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

struct Link
{
	ed::LinkId id;

	ed::PinId start_pin_id;
	ed::PinId end_pin_id;

	//ImColor Color;

	Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId) :
		id(id), start_pin_id(startPinId), end_pin_id(endPinId) {}
};

namespace engine {
	class NodeEditor {
	private:
		VulkanEngine* engine;
		ed::EditorContext* context = nullptr;
		std::vector<Node> nodes;
		std::vector<Link> links;
		int next_id = 1;

		constexpr int get_next_id();

		void build_node(Node* node);

		Node* create_node_add(std::string name, ImVec2 pos);

	public:
		NodeEditor(VulkanEngine* engine);

		void draw();
	};
};

