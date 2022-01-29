#include "gui_node_editor.h"
#include "../vk_engine.h"
#include "../vk_initializers.h"
#include <imgui_internal.h>


static ImRect imgui_get_item_rect() {
	return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

ImageData::ImageData(VulkanEngine* engine) {
	texture = engine::Texture::create_2D_render_target(engine,
		1024,
		1024,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_ASPECT_COLOR_BIT);

	uniform_buffer = engine::Buffer::createBuffer(engine,
		sizeof(UniformBufferObject),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		SWAPCHAIN_INDEPENDENT_BIT);

	if (descriptor_set_layout == nullptr) {
		auto UboLayoutBinding = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
		std::array scenebindings = { UboLayoutBinding };
		engine->create_descriptor_set_layout(scenebindings, descriptor_set_layout);
	}

	VkDescriptorSetAllocateInfo descriptor_alloc_info{};
	descriptor_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_alloc_info.descriptorPool = engine->descriptorPool;
	descriptor_alloc_info.descriptorSetCount = 1;
	descriptor_alloc_info.pSetLayouts = &descriptor_set_layout;
	if (vkAllocateDescriptorSets(engine->device, &descriptor_alloc_info, &descriptor_set) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	VkDescriptorBufferInfo uniformBufferInfo{};
	uniformBufferInfo.buffer = uniform_buffer->buffer;
	uniformBufferInfo.offset = 0;
	uniformBufferInfo.range = sizeof(UniformBufferObject);

}

void Pin::display() {
	if (default_value.index() == 0) {

	}
}

namespace engine {
	constexpr int NodeEditor::get_next_id() {
		return next_id++;
	}

	Node* NodeEditor::create_node_add(std::string name, ImVec2 pos) {
		nodes.emplace_back(get_next_id(), name, NodeAdd(), engine);
		auto& node = nodes.back();
		node.inputs.emplace_back(get_next_id(), "Value", PinType::FLOAT);
		node.inputs.emplace_back(get_next_id(), "Value", PinType::FLOAT);
		node.outputs.emplace_back(get_next_id(), "Result", PinType::FLOAT);

		build_node(&node);

		//ed::SetNodePosition(node.id, pos);

		return &node;
	}

	void NodeEditor::build_node(Node* node) {
		for (auto&& input : node->inputs) {
			input.node = node;
			input.flow_direction = PinInOut::INPUT;
		}

		for (auto&& output : node->outputs) {
			output.node = node;
			output.flow_direction = PinInOut::OUTPUT;
		}
	}

	void NodeEditor::draw() {
		//static bool add_node_add = false;

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("Add")) {
				if (ImGui::MenuItem("Add")) {
					create_node_add("Add", ImVec2(0, 0));
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::ShowDemoWindow();
		ed::SetCurrentEditor(context);
		ed::Begin("My Editor", ImVec2(0.0f, 0.0f));

		// Start drawing nodes.

		//if (add_node_add) {
			//create_node_add("Add", ImVec2(0, 0));
			//add_node_add = false;
		//}

		for (auto& node : nodes) {
			ed::BeginNode(node.id);
			ImGui::Text(node.type_name.c_str());
			ImGui::Dummy(ImVec2(160.0f, 3.0f));
			auto node_rect = imgui_get_item_rect();
			ImGui::GetWindowDrawList()->AddRectFilled(node_rect.GetTL(), node_rect.GetBR(), ImColor(68, 129, 196, 160));
			//ImGui::BeginVertical("delegates", ImVec2(0, 28));

			//if (ImGui::BeginTable("table1", 1)) {
			const float radius = 5.8f;
			for (auto& pin : node.outputs) {
				//ImGui::TableNextColumn();
				//ed::PushStyleVar(ed::StyleVar_PinCorners, 15);
				ed::BeginPin(pin.id, ed::PinKind::Output);
				//auto rect = imgui_get_item_rect();

				auto str = pin.name.c_str();
				ImGui::SetCursorPosX(node_rect.Max.x - ImGui::CalcTextSize(str).x);
				ImGui::Text(str);

				auto rect = imgui_get_item_rect();
				auto draw_list = ImGui::GetWindowDrawList();
				ImVec2 pin_center = ImVec2(node_rect.Max.x + 8.0f, rect.GetCenter().y);

				draw_list->AddCircleFilled(pin_center, radius, ImColor(68, 129, 196, 160), 24);
				draw_list->AddCircle(ImVec2(node_rect.Max.x + 8.0f, rect.GetCenter().y), radius, ImColor(68, 129, 196, 255), 24, 1.8);

				ed::PinPivotRect(pin_center, pin_center);
				ed::PinRect(ImVec2(pin_center.x - radius, pin_center.y - radius), ImVec2(pin_center.x + radius, pin_center.y + radius));

				ed::EndPin();
				//ed::PopStyleVar(1);
			}

			
			for (std::size_t i = 0; i < node.inputs.size(); ++i) {
				//ImGui::TableNextColumn();
				auto& pin = node.inputs[i];
				ed::PushStyleVar(ed::StyleVar_PinCorners, 15);
				ed::BeginPin(pin.id, ed::PinKind::Input);
				//ed::PinPivotRect(rect.GetCenter(), rect.GetCenter());
				//ed::PinRect(rect.GetTL(), rect.GetBR());
				
				ImGui::Text(pin.name.c_str());
				auto rect = imgui_get_item_rect();
				
				if (pin.connect_nodes.empty()) {
					ImGui::PushItemWidth(100.f);
					ImGui::SameLine();
					ImGui::DragFloat(("##" + std::to_string(pin.id.Get())).c_str(), std::get_if<float>(&pin.default_value), 0.005f);
				}

				auto drawList = ImGui::GetWindowDrawList();
				ImVec2 pin_center = ImVec2(rect.Min.x - 8.0f, rect.GetCenter().y);
				drawList->AddCircleFilled(pin_center, radius, ImColor(68, 129, 196, 160), 24);
				drawList->AddCircle(pin_center, radius, ImColor(68, 129, 196, 255), 24, 1.8);

				ed::PinPivotRect(pin_center, pin_center);
				ed::PinRect(ImVec2(pin_center.x - radius, pin_center.y - radius), ImVec2(pin_center.x + radius, pin_center.y + radius));

				ed::EndPin();
				ed::PopStyleVar(1);
			}
			//ImGui::EndTable();
		//}
		//ImGui::SameLine();
			ed::EndNode();

			for (auto& link : links) {
				ed::Link(link.id, link.start_pin_id, link.end_pin_id, ImColor(123, 174, 111, 245), 2.5f);
			}

			if (ed::BeginCreate()) {
				ed::PinId inputPinId, outputPinId;
				if (ed::QueryNewLink(&inputPinId, &outputPinId)) {
					if (inputPinId && outputPinId) {
						// ed::AcceptNewItem() return true when user release mouse button.
						if (ed::AcceptNewItem()) {
							// Since we accepted new link, lets add one to our list of links.
							links.emplace_back(ed::LinkId(get_next_id()), inputPinId, outputPinId);
							Node* input_node;
							bool found_input = false;
							Pin* output_pin;
							bool found_output = false;
							for (auto& node : nodes) {
								for (auto& pin : node.outputs) {
									if (pin.id == inputPinId) {
										input_node = &node;
										found_output = true;
									}
								}
								for (auto& pin : node.inputs) {
									if (pin.id == outputPinId) {
										output_pin = &pin;
										found_input = true;
									}
								}
								if (found_input && found_output) {
									break;
								}
							}
							output_pin->connect_nodes.emplace_back(input_node);
						}
					}
				}

			}
			ed::EndCreate(); // Wraps up object creation action handling.

			if (ed::BeginDelete())
			{
				// There may be many links marked for deletion, let's loop over them.
				ed::LinkId deleted_link_id;
				while (ed::QueryDeletedLink(&deleted_link_id))
				{
					// If you agree that link can be deleted, accept deletion.
					if (ed::AcceptDeletedItem())
					{
						// Then remove link from your data.
						auto deleted_link = std::find_if(links.begin(), links.end(), [=](auto& link) {
							return link.id == deleted_link_id;
							});
						if (deleted_link != links.end()) {
							links.erase(deleted_link);
						}
					}
				}

				ed::NodeId deleted_node_id;
				while (ed::QueryDeletedNode(&deleted_node_id))
				{
					// If you agree that link can be deleted, accept deletion.
					if (ed::AcceptDeletedItem())
					{
						// Then remove link from your data.
						auto deleted_node = std::find_if(nodes.begin(), nodes.end(), [=](auto& node) {
							return node.id == deleted_node_id;
							});
						if (deleted_node != nodes.end()) {
							nodes.erase(deleted_node);
						}
					}
				}
			}
			ed::EndDelete(); // Wrap up deletion action
		}

		ed::End();
		ed::SetCurrentEditor(nullptr);
	}

	NodeEditor::NodeEditor(VulkanEngine* engine) :engine(engine) {
		ed::Config config;
		config.SettingsFile = "Simple.json";
		context = ed::CreateEditor(&config);
		engine->main_deletion_queue.push_function([=]() {
			ed::DestroyEditor(context);
			});
	}
};