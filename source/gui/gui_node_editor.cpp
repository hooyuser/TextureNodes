#include "gui_node_editor.h"
#include "../vk_engine.h"
#include "../vk_initializers.h"
#include "../vk_shader.h"
#include <ranges>

//static const std::unordered_set<std::string> image_node_type_name{
//	"Uniform Color"
//};

static ImRect imgui_get_item_rect() {
	return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

std::string first_letter_to_upper(std::string_view str) {
	auto split = str
		| std::ranges::views::split('_')
		| std::ranges::views::transform([](auto&& str) { return std::string_view(&*str.begin(), std::ranges::distance(str)); });

	std::string upper_str = "";
	std::string_view separator = "";
	for (auto&& word : split) {
		upper_str += std::exchange(separator, " ").data() + (static_cast<char>(std::toupper(word[0])) + std::string(word.substr(1, word.size() - 1)));
	}
	return upper_str;
}

namespace engine {
	constexpr int NodeEditor::get_next_id() {
		return next_id++;
	}

	void NodeEditor::update_from(Node* node) {
		std::visit([&](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (is_image_data<T>()) {
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

				arg->uniform_buffer->copyFromHost(&arg->ubo);
			}
			else if constexpr (std::is_same_v<T, Color4Data>) {

			}
			}, node->data);
	}

	//Node* NodeEditor::create_node_add(std::string name) {
	//	nodes.emplace_back(get_next_id(), name, NodeAdd{}, engine);
	//	auto& node = nodes.back();
	//	node.inputs.emplace_back(get_next_id(), "Value", FloatData{});
	//	node.inputs.emplace_back(get_next_id(), "Value", FloatData{});
	//	node.outputs.emplace_back(get_next_id(), "Result", FloatData{});

	//	build_node(&node);

	//	//ed::SetNodePosition(node.id, pos);

	//	return &node;
	//}

	//Node* NodeEditor::create_node_uniform_color(std::string name) {
	//	using NodeType = NodeUniformColor;
	//	nodes.emplace_back(get_next_id(), name, NodeType{}, engine);
	//	auto& node = nodes.back();

	//	if constexpr (std::derived_from<NodeType, NodeTypeImageBase>) {
	//		auto& ubo = std::get<NodeType::data_type>(node.data)->ubo;
	//		std::decay_t<decltype(ubo)>::Class::ForEachField(ubo, [&](auto& field, auto& value) {
	//			using PinType = std::decay_t<decltype(value)>;
	//			node.inputs.emplace_back(get_next_id(), first_letter_to_upper(field.name), std::in_place_type<PinType>);
	//			value = std::get<PinType>(node.inputs.back().default_value);
	//			});
	//		node.outputs.emplace_back(get_next_id(), "Result", std::in_place_type<TexturePtr>);
	//	}

	//	build_node(&node);

	//	return &node;
	//}

	void NodeEditor::build_node(Node* node) {
		for (auto&& input : node->inputs) {
			input.node = node;
			input.flow_direction = PinInOut::INPUT;
		}

		std::visit([&](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			//auto s = cpp_type_name<T>();
			if constexpr (is_image_data<T>()) {
				auto& ubo = arg->ubo;
				for (size_t index = 0; index < node->inputs.size(); ++index) {
					std::decay_t<decltype(ubo)>::Class::FieldAt(ubo, index, [&](auto& field, auto& value) {
						node->inputs[index].default_value = value;
						});
				}
			}
			}, node->data);

		for (auto&& output : node->outputs) {
			output.node = node;
			output.flow_direction = PinInOut::OUTPUT;
		}
	}

	void NodeEditor::draw() {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("Add")) {
				NodeTypeList::for_each([&]<typename T> () {
					if (ImGui::MenuItem(T::name())) {
						update_from(create_node<T>());
					}
				});
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::ShowDemoWindow();
		ed::SetCurrentEditor(context);
		ed::Begin("My Editor", ImVec2(0.0f, 0.0f));

		// Start drawing nodes.
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
					if constexpr (std::is_same_v<T, FloatData> || std::is_same_v<T, IntData>) {
						//ImGui::Text(pin->name.c_str());

						if (pin->connected_pins.empty()) {
							ImGui::PushItemWidth(150.f);
							//ImGui::SameLine();
							bool response_flag = false;
							if constexpr (std::is_same_v<T, FloatData>) {
								response_flag |= ImGui::DragFloat(("##" + std::to_string(pin->id.Get())).c_str(), &std::get<T>(pin->default_value).value, 0.005f, 0.0f, 0.0f, (pin->name + " : %.3f").c_str());
							}
							else if constexpr (std::is_same_v<T, IntData>) {
								response_flag |= ImGui::DragInt(("##" + std::to_string(pin->id.Get())).c_str(), &std::get<T>(pin->default_value).value, 0.1f, 0, 64);
							}
							if (response_flag) {
								std::visit([&](auto&& arg) {
									using T = std::decay_t<decltype(arg)>;
									if constexpr (is_image_data<T>()) {
										auto& ubo = std::get<T>(node.data)->ubo;
										std::decay_t<decltype(ubo)>::Class::FieldAt(ubo, i, [&](auto& field, auto& value) {
											using TV = std::decay_t<decltype(value)>;
											value = std::get<TV>(node.inputs[i].default_value);
											});
									}
									}, node.data);
								update_from(&node);
							}


							ImGui::PopItemWidth();
						}
						rect = imgui_get_item_rect();
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
				if constexpr (is_image_data<T>()) {
					ImGui::SetCursorPosX(node_rect.GetCenter().x - image_size * 0.5);
					ImGui::SetCursorPosY(yy - image_size - 15);
					vkWaitForFences(engine->device, 1, &arg->fence, VK_TRUE, UINT64_MAX);
					ImGui::Image(static_cast<ImTextureID>(arg->gui_texture), ImVec2{ image_size, image_size }, ImVec2{ 0, 0 }, ImVec2{ 1, 1 });
				}
				else if constexpr (std::is_same_v<T, NodeAdd::data_type>) {

				}
				}, node.data);


			if (color_pin) {
				ed::Suspend();
				if (ImGui::BeginPopup(("ColorPopup##" + std::to_string(color_pin->id.Get())).c_str())) {
					if (ImGui::ColorPicker4(("##ColorPicker" + std::to_string(color_pin->id.Get())).c_str(), reinterpret_cast<float*>(std::get_if<Color4Data>(&(color_pin->default_value))), ImGuiColorEditFlags_None, NULL)) {
						int index = 0;
						for (const auto& input_pin : node.inputs) {
							if (input_pin.id == color_pin->id) {
								std::visit([&](auto&& arg) {
									using T = std::decay_t<decltype(arg)>;
									if constexpr (is_image_data<T>()) {
										auto& ubo = std::get<T>(node.data)->ubo;
										std::decay_t<decltype(ubo)>::Class::FieldAt(ubo, index, [&](auto& field, auto& value) {
											using TV = std::decay_t<decltype(value)>;
											value = std::get<TV>(node.inputs[index].default_value);
											});
										update_from(&node);
									}
									}, node.data);
								break;
							}
							index++;
						}
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

		engine->main_deletion_queue.push_function([=]() {
			ed::DestroyEditor(context);
			});
	}
};
