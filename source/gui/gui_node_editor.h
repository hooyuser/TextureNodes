#pragma once
#include <vector>
#include <string>
#include <imgui_node_editor.h>

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
	ADD
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

	std::string state;
	std::string saved_state;

	Node(int id, std::string name, NodeType type);
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



void draw_node_editer();

void init_node_editor(VulkanEngine* engine);

void create_nodes();

static void build_node(Node* node);

static Node* create_node_add(std::string name, ImVec2 pos);