#include "gui_node_editor.h"
#include "../vk_engine.h"
#include "imgui_internal.h"

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
	node.inputs.emplace_back(get_next_id(), "Value", PinType::FLOAT);
	node.inputs.emplace_back(get_next_id(), "Value", PinType::FLOAT);
	node.outputs.emplace_back(get_next_id(), "Result", PinType::FLOAT);

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
	ImGui::ShowDemoWindow();
	ed::SetCurrentEditor(node_ed::context);
	ed::Begin("My Editor", ImVec2(0.0f, 0.0f));
	// Start drawing nodes.
	for (auto& node : node_ed::nodes) {
		ed::BeginNode(node.id);
		ImGui::TextUnformatted(node.name.c_str());
		ImGui::Dummy(ImVec2(140.0f, 3.0f));
		auto node_rect = imgui_get_item_rect();
		ImGui::GetWindowDrawList()->AddRectFilled(node_rect.GetTL(), node_rect.GetBR(), ImColor(68, 129, 196, 160));
		//ImGui::BeginVertical("delegates", ImVec2(0, 28));

		if (ImGui::BeginTable("table1", 1)) {
			for (auto& pin : node.outputs) {
				ImGui::TableNextColumn();
				ed::PushStyleVar(ed::StyleVar_PinCorners, 15);
				ed::BeginPin(get_next_id(), ed::PinKind::Output);
				auto rect = imgui_get_item_rect();
				ed::PinPivotRect(rect.GetCenter(), rect.GetCenter());
				ed::PinRect(rect.GetTL(), rect.GetBR());

				auto str = pin.name.c_str();
				ImGui::SetCursorPosX(node_rect.Max.x - ImGui::CalcTextSize(str).x);

				ImGui::Text(str);
				rect = imgui_get_item_rect();

				auto drawList = ImGui::GetWindowDrawList();
				drawList->AddCircleFilled(ImVec2(node_rect.Max.x + 8.0f, rect.GetCenter().y), 5.8f, ImColor(68, 129, 196, 160), 24);
				drawList->AddCircle(ImVec2(node_rect.Max.x + 8.0f, rect.GetCenter().y), 5.8f, ImColor(68, 129, 196, 255), 24, 1.8);
				ed::EndPin();
				ed::PopStyleVar(1);
			}

			for (auto& pin : node.inputs) {
				ImGui::TableNextColumn();
				ed::PushStyleVar(ed::StyleVar_PinCorners, 15);
				ed::BeginPin(get_next_id(), ed::PinKind::Input);
				auto rect = imgui_get_item_rect();
				ed::PinPivotRect(rect.GetCenter(), rect.GetCenter());
				ed::PinRect(rect.GetTL(), rect.GetBR());
				ImGui::Text(pin.name.c_str());
				rect = imgui_get_item_rect();

				auto drawList = ImGui::GetWindowDrawList();
				drawList->AddCircleFilled(ImVec2(rect.Min.x - 8.0f, rect.GetCenter().y), 5.8f, ImColor(68, 129, 196, 160), 24);
				drawList->AddCircle(ImVec2(rect.Min.x - 8.0f, rect.GetCenter().y), 5.8f, ImColor(68, 129, 196, 255), 24, 1.8);
				ed::EndPin();
				ed::PopStyleVar(1);
			}
			ImGui::EndTable();
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