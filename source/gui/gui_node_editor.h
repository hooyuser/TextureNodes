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
	//ImageData(const ImageData& data) {
	//	texture = data.texture;
	//	uniform_buffer = data.uniform_buffer;
	//	descriptor_set = data.descriptor_set;
	//}
	//ImageData& operator=(const ImageData& data){
	//	texture = data.texture;
	//	uniform_buffer = data.uniform_buffer;
	//	descriptor_set = data.descriptor_set;
	//	return *this;
	//}
	//ImageData(){};
	ImageData(VulkanEngine* engine);
};

using ImageDataPtr = std::shared_ptr<ImageData>;

struct FloatData : public NodeResult {
	float value = 0.0f;
	FloatData(VulkanEngine* engine = nullptr) {};
};

struct Float4Data : public NodeResult {
	float value[4] = { 0.0f };
	Float4Data(VulkanEngine* engine = nullptr) {};
};

enum class PinInOut {
	INPUT,
	OUTPUT
};

struct PinTypeBase {};

struct PinColor : PinTypeBase {
	using data_type = Float4Data;
};

struct PinFloat : PinTypeBase {
	using data_type = float;
};

struct PinImage : PinTypeBase {
	using data_type = TexturePtr;
};

using PinVariant = std::variant<float, Float4Data, TexturePtr>;

struct NodeTypeBase {};

struct NodeUniformColor : NodeTypeBase {
	using result_type = ImageData;
	constexpr auto static name() { return "Uniform Color"; }
};

struct NodeAdd : NodeTypeBase {
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
	PinVariant default_value;

	template<std::derived_from<PinTypeBase> T>
	Pin(ed::PinId id, std::string name, const T&) :id(id), name(name), default_value(std::in_place_type<T::data_type>) {
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

	template<std::derived_from<NodeTypeBase> T>
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

	bool operator==(const Link& link) const {
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

