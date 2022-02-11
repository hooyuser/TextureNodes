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

#include "gui_node_data.h"

namespace ed = ax::NodeEditor;

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
	using uniform_buffer_type = Float4Data;
};

using PinVariant = std::variant<float, Float4Data, TexturePtr>;

////////////////////////////////////////////////////////////////////////////////////////////

struct NodeTypeBase {};

struct NodeTypeImageBase : NodeTypeBase {};

struct NodeUniformColor : NodeTypeImageBase {
	using data_type = ImageData<Float4Data,
		"assets/shaders/node_uniform_color.vert.spv",
		"assets/shaders/node_uniform_color.frag.spv"
	>;
	constexpr auto static name() { return "Uniform Color"; }
};

struct NodeAdd : NodeTypeBase {
	using data_type = FloatData;
	constexpr auto static name() { return "Add"; }
};

//using NodeDataVariant = std::variant<NodeUniformColor::data_type, NodeAdd::data_type>;

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
	std::shared_ptr<NodeData> data;
	void* gui_texture = nullptr;
	VkFence fence = nullptr;

	template<std::derived_from<NodeTypeBase> T>
	Node(int id, std::string name, const T&, VulkanEngine* engine) :
		id(id), name(name), type_name(T::name()),
		data(std::static_pointer_cast<NodeData>(std::make_shared<T::data_type>(engine))) {
		if constexpr (std::is_base_of_v<NodeTypeImageBase, T>) {
			auto image_data = std::static_pointer_cast<T::data_type>(data);
			auto fenceInfo = vkinit::fenceCreateInfo();
			if (vkCreateFence(engine->device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
				throw std::runtime_error("failed to create fence!");
			}
			engine->main_deletion_queue.push_function([=]() {
				vkDestroyFence(engine->device, fence, nullptr);
				});
			gui_texture = ImGui_ImplVulkan_AddTexture(image_data->texture->sampler, image_data->texture->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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

		template<typename T>
		void update_from(Node* node);

		void build_node(Node* node);

		Node* create_node_add(std::string name);

		Node* create_node_uniform_color(std::string name);

	public:
		NodeEditor(VulkanEngine* engine);

		void draw();
	};
};

