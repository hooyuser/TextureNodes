#include "gui_node_editor.h"
#include "../vk_engine.h"
#include "../vk_initializers.h"
#include "../vk_shader.h"

static const std::unordered_set<std::string> image_node_type_name{
	"Uniform Color"
};

static ImRect imgui_get_item_rect() {
	return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

namespace engine {
	constexpr int NodeEditor::get_next_id() {
		return next_id++;
	}

	void NodeEditor::update_from(Node* node) {
		//if (node->updated == NodeState::Updated) {
			std::visit([&](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, NodeUniformColor::data_type>) {
					std::vector<VkCommandBuffer> submitCommandBuffers;
					submitCommandBuffers.emplace_back(arg->get_node_cmd_buffer());

					vkResetFences(engine->device, 1, &arg->fence);

					VkSubmitInfo submitInfo{
						.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
						.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size()),
						.pCommandBuffers = submitCommandBuffers.data(),
					};

					auto result = vkQueueSubmit(engine->graphicsQueue, 1, &submitInfo, arg->fence);
					if (result != VK_SUCCESS) {
						throw std::runtime_error("failed to submit draw command buffer!");
					}

				}
				else if constexpr (std::is_same_v<T, Color4Data>) {
					int xxxxxxxx = 0;
				}
				}, node->data);
			//node->updated = NodeState::Processing;
			//}
	}

	Node* NodeEditor::create_node_add(std::string name) {
		nodes.emplace_back(get_next_id(), name, NodeAdd{}, engine);
		auto& node = nodes.back();
		node.inputs.emplace_back(get_next_id(), "Value", PinFloat{});
		node.inputs.emplace_back(get_next_id(), "Value", PinFloat{});
		node.outputs.emplace_back(get_next_id(), "Result", PinFloat{});

		build_node(&node);

		//ed::SetNodePosition(node.id, pos);

		return &node;
	}

	Node* NodeEditor::create_node_uniform_color(std::string name) {
		using NodeType = NodeUniformColor;
		nodes.emplace_back(get_next_id(), name, NodeType{}, engine);
		auto& node = nodes.back();
		
		using PinType1 = PinColor;
		node.inputs.emplace_back(get_next_id(), "Color", PinType1{});
		std::get<NodeUniformColor::data_type>(node.data)->ubo.color = std::get<PinType1::data_type>(node.inputs.back().default_value);
		
		node.outputs.emplace_back(get_next_id(), "Result", PinImage{});

		build_node(&node);
		//node.updated = NodeState::Updated;
		//update_from(&node);
		//update_from(&node);

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
					create_node_add("Add");
				}
				if (ImGui::MenuItem("Uniform Color")) {
					create_node_uniform_color("Uniform Color");
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
			auto yy = ImGui::GetCursorPosY();
			ImGui::Text(node.type_name.c_str());
			ImGui::Dummy(ImVec2(160.0f, 3.0f));
			auto node_rect = imgui_get_item_rect();
			ImGui::GetWindowDrawList()->AddRectFilled(node_rect.GetTL(), node_rect.GetBR(), ImColor(68, 129, 196, 160));
			//ImGui::BeginVertical("delegates", ImVec2(0, 28));

			const float radius = 5.8f;
			for (auto& pin : node.outputs) {
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

			Pin* color_pin = nullptr;

			for (std::size_t i = 0; i < node.inputs.size(); ++i) {
				auto pin = &node.inputs[i];
				ed::PushStyleVar(ed::StyleVar_PinCorners, 15);
				ed::BeginPin(pin->id, ed::PinKind::Input);
				//ed::PinPivotRect(rect.GetCenter(), rect.GetCenter());
				//ed::PinRect(rect.GetTL(), rect.GetBR());

				ImRect rect;
				std::visit([&](auto&& arg) {
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, float>) {
						ImGui::Text(pin->name.c_str());
						rect = imgui_get_item_rect();
						if (pin->connected_pins.empty()) {
							ImGui::PushItemWidth(100.f);
							ImGui::SameLine();
							ImGui::DragFloat(("##" + std::to_string(pin->id.Get())).c_str(), std::get_if<float>(&(pin->default_value)), 0.005f);
							ImGui::PopItemWidth();
						}
					}
					else if constexpr (std::is_same_v<T, Color4Data>) {

						ImGui::Text(pin->name.c_str());
						rect = imgui_get_item_rect();
						if (pin->connected_pins.empty()) {
							ImGui::PushItemWidth(100.f);
							ImGui::SameLine();
							auto color = ImVec4{ arg.value[0], arg.value[1], arg.value[2], arg.value[3] };
							ImGui::PopItemWidth();
							if (ImGui::ColorButton(("ColorButton##" + std::to_string(pin->id.Get())).c_str(), color, ImGuiColorEditFlags_NoTooltip, ImVec2{ 40, 20 })) {
								ed::Suspend();
								ImGui::OpenPopup(("ColorPopup##" + std::to_string(pin->id.Get())).c_str());
								ed::Resume();
							}

							color_pin = pin;
						}
					}
					}, pin->default_value);

				auto drawList = ImGui::GetWindowDrawList();
				ImVec2 pin_center = ImVec2(rect.Min.x - 8.0f, rect.GetCenter().y);
				drawList->AddCircleFilled(pin_center, radius, ImColor(68, 129, 196, 160), 24);
				drawList->AddCircle(pin_center, radius, ImColor(68, 129, 196, 255), 24, 1.8);

				ed::PinPivotRect(pin_center, pin_center);
				ed::PinRect(ImVec2(pin_center.x - radius, pin_center.y - radius), ImVec2(pin_center.x + radius, pin_center.y + radius));

				ed::EndPin();
				ed::PopStyleVar(1);
			}

			ed::EndNode();

			std::visit([&](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, NodeUniformColor::data_type>) {
					ImGui::SetCursorPosX(node_rect.GetCenter().x - image_size * 0.5);
					ImGui::SetCursorPosY(yy - image_size - 15);
					//if (node.updated == NodeState::Processing) {
						vkWaitForFences(engine->device, 1, &arg->fence, VK_TRUE, UINT64_MAX);
						//node.updated = NodeState::Normal;
					//}					
					ImGui::Image(static_cast<ImTextureID>(arg->gui_texture), ImVec2{ image_size, image_size }, ImVec2{ 0, 0 }, ImVec2{ 1, 1 });
				}
				else if constexpr (std::is_same_v<T, Color4Data>) {

				}
				}, node.data);

		
			if (color_pin) {
				ed::Suspend();
				if (ImGui::BeginPopup(("ColorPopup##" + std::to_string(color_pin->id.Get())).c_str())) {
					if (ImGui::ColorPicker4(("##ColorPicker" + std::to_string(color_pin->id.Get())).c_str(), reinterpret_cast<float*>(std::get_if<Color4Data>(&(color_pin->default_value))), ImGuiColorEditFlags_None, NULL)) {
						int index = 0;
						for (const auto& input_pin : node.inputs) {
							if (input_pin.id == color_pin->id) {
								break;
							}
							index++;
						}
						auto& ubo = std::get<NodeUniformColor::data_type>(node.data)->ubo;
						std::decay_t<decltype(ubo)>::Class::FieldAt(ubo, index, [&](auto& field, auto& value) {
							value = std::get<Color4Data>(node.inputs.back().default_value);
							});

						std::visit([&](auto&& arg) {
							using T = std::decay_t<decltype(arg)>;
							if constexpr (std::is_same_v<T, NodeUniformColor::data_type>) {
								arg->uniform_buffer->copyFromHost(&ubo);
								//if (node.updated == NodeState::Normal) {
								//	node.updated = NodeState::Updated;
								//}
								update_from(&node);
							}
							}, node.data);
					}
					ImGui::EndPopup();
				}
				ed::Resume();
			}

			for (auto& link : links) {
				ed::Link(link.id, link.start_pin_id, link.end_pin_id, ImColor(123, 174, 111, 245), 2.5f);
			}

			if (ed::BeginCreate()) {
				ed::PinId start_pin_id, end_pin_id;
				if (ed::QueryNewLink(&start_pin_id, &end_pin_id)) {
					if (start_pin_id && end_pin_id) {
						// ed::AcceptNewItem() return true when user release mouse button.
						Pin* pins[2];
						char i = 0;
						while (i < 2) {
							for (auto& node : nodes) {
								for (auto& pin : node.outputs) {
									if (pin.id == start_pin_id || pin.id == end_pin_id) {
										pins[i++] = &pin;

									}
								}
								for (auto& pin : node.inputs) {
									if (pin.id == start_pin_id || pin.id == end_pin_id) {
										pins[i++] = &pin;
									}
								}
							}
						}
						assert(i == 2);
						auto showLabel = [](const char* label, ImColor color)
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
							auto size = ImGui::CalcTextSize(label);

							auto padding = ImGui::GetStyle().FramePadding;
							auto spacing = ImGui::GetStyle().ItemSpacing;

							ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

							auto rectMin = ImGui::GetCursorScreenPos() - padding;
							auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

							auto drawList = ImGui::GetWindowDrawList();
							drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
							ImGui::TextUnformatted(label);
						};
						if (pins[0] == pins[1]) {
							ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
						}
						else if (pins[0]->flow_direction == pins[1]->flow_direction) {
							auto hint = (pins[0]->flow_direction == PinInOut::INPUT) ?
								"Input Can Only Be Connected To Output" : "Output Can Only Be Connected To Input";
							showLabel(hint, ImColor(45, 32, 32, 180));
							ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
						}
						else if (pins[0]->default_value.index() != pins[1]->default_value.index()) {
							showLabel("Incompatible Pin Type", ImColor(45, 32, 32, 180));
							ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
						}
						else if (ed::AcceptNewItem()) {
							// Since we accepted new link, lets add one to our list of links.
							links.emplace(ed::LinkId(get_next_id()), start_pin_id, end_pin_id);


							pins[0]->connected_pins.emplace(pins[1]);
							pins[1]->connected_pins.emplace(pins[0]);
						}
					}
				}

			}
			ed::EndCreate(); // Wraps up object creation action handling.

			if (ed::BeginDelete()) {
				// There may be many links marked for deletion, let's loop over them.
				ed::LinkId deleted_link_id;
				while (ed::QueryDeletedLink(&deleted_link_id)) {
					// If you agree that link can be deleted, accept deletion.
					if (ed::AcceptDeletedItem()) {
						// Then remove link from your data.
						auto deleted_link = std::find_if(links.begin(), links.end(), [=](auto& link) {
							return link.id == deleted_link_id;
							});

						auto x = deleted_link != links.end();
						//connect_nodes
						if (deleted_link != links.end()) {
							auto start_pin_id = deleted_link->start_pin_id;
							auto end_pin_id = deleted_link->end_pin_id;
							std::invoke([&] {
								for (auto& node : nodes) {
									for (auto& pin : node.outputs) {
										if (pin.id == start_pin_id) {
											for (auto connected_pin : pin.connected_pins) {
												if (connected_pin->id == end_pin_id) {
													pin.connected_pins.erase(connected_pin);
													connected_pin->connected_pins.erase(&pin);
													return;
												}
											}
										}
									}
								}
								});
							links.erase(deleted_link);
						}
					}
				}

				ed::NodeId deleted_node_id;
				while (ed::QueryDeletedNode(&deleted_node_id))
				{
					if (ed::AcceptDeletedItem())
					{
						auto deleted_node = std::find_if(nodes.begin(), nodes.end(), [=](auto& node) {
							return node.id == deleted_node_id;
							});
						//for (auto& pin : node.outputs) {
						//	for (auto connected_pin : pin.connected_pins) {
						//		connected_pin->connected_pins.erase(&pin);
						//	}
						//}
						//for (auto& pin : node.inputs) {
						//	for (auto connected_pin : pin.connected_pins) {
						//		connected_pin->connected_pins.erase(&pin);
						//	}
						//}
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

		//VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(engine->commandPool, 1);

		/*if (vkAllocateCommandBuffers(engine->device, &cmdAllocInfo, &command_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}*/

		engine->main_deletion_queue.push_function([=]() {
			//vkFreeCommandBuffers(engine->device, engine->commandPool, 1, &command_buffer);
			ed::DestroyEditor(context);
			});
	}
};
