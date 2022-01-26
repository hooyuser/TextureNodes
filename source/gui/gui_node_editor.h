#pragma once
#include <vector>
#include <string>
#include <variant>
#include <imgui_node_editor.h>
#include "../vk_image.h"

class VulkanEngine;

namespace ed = ax::NodeEditor;

enum class PinType {
	IMAGE,
	FLOAT
};

enum class PinInOut {
	INPUT,
	OUTPUT
};

enum class NodeType {
	OUTPUT,
	ADD,
	UNIFORM_COLOR
};

struct Node;
struct Pin {
	ed::PinId id;
	::Node* node = nullptr;
	std::string name;
	PinType type;
	PinInOut flow_direction = PinInOut::INPUT;

	Pin(ed::PinId id, std::string name, PinType type) :
		id(id), name(name), type(type) {}
};



struct Node {
	ed::NodeId id;
	std::string name;
	std::vector<Pin> inputs;
	std::vector<Pin> outputs;
	//ImColor Color ;
	NodeType type;
	std::string type_name;
	ImVec2 size = { 0, 0 };
	std::variant<TexturePtr, float> result;

	Node(int id, std::string name, NodeType type, VulkanEngine* engine);
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

