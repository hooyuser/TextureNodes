#include "gui_node_editor.h"
#include "../vk_engine.h"
#include "imgui_internal.h"
#include "imgui.h"

namespace node_ed {
	static ed::EditorContext* context = nullptr;

	static std::vector<Node> nodes;
	static std::vector<Link> links;
	static int nextId = 1;
}


constexpr static int get_next_id() {
	return node_ed::nextId++;
}

static ImRect imgui_get_item_rect() {
	return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}


void create_nodes() {
	create_node_add();
}

static Node* create_node_add() {
	node_ed::nodes.emplace_back(get_next_id(), "Add", NodeType::ADD);
	auto& node = node_ed::nodes.back();
	node.inputs.emplace_back(get_next_id(), "value 1", PinType::FLOAT);
	node.inputs.emplace_back(get_next_id(), "value 2", PinType::FLOAT);
	node.outputs.emplace_back(get_next_id(), "result", PinType::FLOAT);

	build_node(&node);
	//ed::SetNodePosition(node.id, ImVec2(500, 70));

	return &node;
}

static void build_node(Node* node) {
	for (auto&& input : node->inputs) {
		input.node = node;
		input.flow_direction = PinInOut::INPUT;
	}

	for (auto&& output : node->outputs) {
		output.node = node;
		output.flow_direction = PinInOut::OUTPUT;
	}
}

void draw_node_editer() {
	ed::SetCurrentEditor(node_ed::context);
	ed::Begin("My Editor", ImVec2(0.0f, 0.0f));
	// Start drawing nodes.
	for (auto& node : node_ed::nodes) {
		ed::BeginNode(node.id);
		
		ImGui::TextUnformatted(node.name.c_str());
		//auto rect = imgui_get_item_rect();
		//ImGui::GetWindowDrawList()->AddRectFilled(rect.GetTL(), rect.GetBR(), ImGui::ColorConvertFloat4ToU32(ImVec4(1, .15, .15, 1)));
		//ImGui::BeginVertical("delegates", ImVec2(0, 28));
		for (auto& pin : node.outputs) {
			ed::BeginPin(get_next_id(), ed::PinKind::Output);
			ImGui::Text(pin.name.c_str());
			ed::EndPin();
		}
		
		for (auto& pin : node.inputs) {
			ed::BeginPin(get_next_id(), ed::PinKind::Input);
			ImGui::Text(pin.name.c_str());
			ed::EndPin();
		}
		//ImGui::SameLine();
		ed::EndNode();
		
	}

	ed::End();
	ed::SetCurrentEditor(nullptr);
}

void init_node_editor(VulkanEngine* engine) {
	ed::Config config;
	config.SettingsFile = "Simple.json";
	node_ed::context = ed::CreateEditor(&config);
	engine->main_deletion_queue.push_function([=]() {
		ed::DestroyEditor(node_ed::context);
		});
}