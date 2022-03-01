#include "gui_node_editor.h"
#include "../vk_engine.h"
#include "../vk_initializers.h"
#include "../vk_shader.h"
#include <ranges>

static ImRect imgui_get_item_rect() {
	return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

std::string first_letter_to_upper(std::string_view str) {
	auto split = str
		| std::ranges::views::split('_')
		| std::ranges::views::transform([](auto&& str) {return std::string_view(&*str.begin(), std::ranges::distance(str)); });

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
		std::visit([&](auto&& node_data) {
			using NodeDataT = std::decay_t<decltype(node_data)>;
			if constexpr (is_image_data<NodeDataT>) {
				std::vector<VkCommandBuffer> submitCommandBuffers;
				submitCommandBuffers.emplace_back(node_data->node_cmd_buffer);

				vkResetFences(engine->device, 1, &node_data->fence);

				VkSubmitInfo submitInfo{
					.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
					.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size()),
					.pCommandBuffers = submitCommandBuffers.data(),
				};

				auto result = vkQueueSubmit(engine->graphicsQueue, 1, &submitInfo, node_data->fence);
				if (result != VK_SUCCESS) {
					throw std::runtime_error("failed to submit draw command buffer!");
				}
			}
			else if constexpr (std::is_same_v<NodeDataT, Color4Data>) {

			}
			}, node->data);
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

			//loop over output pins
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

			//loop over input pins
			std::optional<size_t> color_pin_index;

			for (std::size_t i = 0; i < node.inputs.size(); ++i) {
				auto pin = &node.inputs[i];
				ed::PushStyleVar(ed::StyleVar_PinCorners, 15);
				ed::BeginPin(pin->id, ed::PinKind::Input);
				//ed::PinPivotRect(rect.GetCenter(), rect.GetCenter());
				//ed::PinRect(rect.GetTL(), rect.GetBR());

				ImRect rect;
				std::visit([&](auto&& default_value) {
					using PinT = std::decay_t<decltype(default_value)>;
					if constexpr (std::is_same_v<PinT, FloatData> || std::is_same_v<PinT, IntData>) {
						//ImGui::Text(pin->name.c_str());

						if (pin->connected_pins.empty()) {
							ImGui::PushItemWidth(150.f);
							//ImGui::SameLine();
							bool response_flag = false;
							std::visit([&](auto&& node_data) {
								using NodeDataT = std::remove_reference_t<decltype(node_data)>;
								if constexpr (is_image_data<NodeDataT>) {
									using UboT = typename ref_t<NodeDataT>::UboType;
									UboT::Class::FieldAt(i, [&](auto& field) {
										auto widget_info = field.template getAnnotation<NumberInputWidgetInfo>();
										if constexpr (std::is_same_v<PinT, FloatData>) {
											if (widget_info.enable_slider) {
												response_flag |= ImGui::SliderFloat(("##" + std::to_string(pin->id.Get())).c_str(),
													&std::get<PinT>(pin->default_value).value,
													widget_info.min,
													widget_info.max,
													(pin->name + " : %.3f").c_str());
											}
											else {
												response_flag |= ImGui::DragFloat(("##" + std::to_string(pin->id.Get())).c_str(),
													&std::get<PinT>(pin->default_value).value,
													widget_info.speed,
													widget_info.min,
													widget_info.max,
													(pin->name + " : %.3f").c_str());
											}
										}
										else if constexpr (std::is_same_v<PinT, IntData>) {
											response_flag |= ImGui::DragInt(("##" + std::to_string(pin->id.Get())).c_str(),
												&std::get<PinT>(pin->default_value).value,
												widget_info.speed,
												static_cast<int>(widget_info.min),
												static_cast<int>(widget_info.max),
												(pin->name + " : %d").c_str());
										}
										if (response_flag) {
											node_data->update_ubo(pin->default_value, i);
											update_from(&node);
										}
										});
								}
								}, node.data);

							ImGui::PopItemWidth();
						}
						rect = imgui_get_item_rect();
					}
					else if constexpr (std::is_same_v<PinT, Color4Data>) {
						ImGui::Text(pin->name.c_str());
						rect = imgui_get_item_rect();
						if (pin->connected_pins.empty()) {
							ImGui::PushItemWidth(100.f);
							ImGui::SameLine();
							ImGui::PopItemWidth();
							if (ImGui::ColorButton(("ColorButton##" + std::to_string(pin->id.Get())).c_str(), std::bit_cast<ImVec4>(default_value), ImGuiColorEditFlags_NoTooltip, ImVec2{ 40, 20 })) {
								ed::Suspend();
								ImGui::OpenPopup(("ColorPopup##" + std::to_string(pin->id.Get())).c_str());
								ed::Resume();
							}
							color_pin_index = i;
						}
					}
					else if constexpr (std::is_same_v<PinT, TextureIdData>) {
						ImGui::Text(pin->name.c_str());
						rect = imgui_get_item_rect();
					}
					else if constexpr (std::is_same_v<PinT, BoolData>) {

						auto& bool_data = std::get<BoolData>(node.inputs[i].default_value);
						if (ImGui::Checkbox(pin->name.c_str(), &bool_data.value)) {
							std::visit([&](auto&& node_data) {
								using NodeT = std::decay_t<decltype(node_data)>;
								if constexpr (is_image_data<NodeT>) {
									node_data->update_ubo(pin->default_value, i);
									update_from(&node);
								}
								}, node.data);
						}
						//ImGui::Text(pin->name.c_str());
						rect = imgui_get_item_rect();
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
				if constexpr (is_image_data<T>) {
					ImGui::SetCursorPosX(node_rect.GetCenter().x - image_size * 0.5);
					ImGui::SetCursorPosY(yy - image_size - 15);
					vkWaitForFences(engine->device, 1, &arg->fence, VK_TRUE, UINT64_MAX);
					ImGui::Image(static_cast<ImTextureID>(arg->gui_texture), ImVec2{ image_size, image_size }, ImVec2{ 0, 0 }, ImVec2{ 1, 1 });
				}
				else if constexpr (std::is_same_v<T, NodeAdd::data_type>) {

				}
				}, node.data);


			if (color_pin_index) {
				auto& color_pin = node.inputs[*color_pin_index];
				ed::Suspend();
				if (ImGui::BeginPopup(("ColorPopup##" + std::to_string(color_pin.id.Get())).c_str())) {
					if (ImGui::ColorPicker4(("##ColorPicker" + std::to_string(color_pin.id.Get())).c_str(), reinterpret_cast<float*>(std::get_if<Color4Data>(&(color_pin.default_value))), ImGuiColorEditFlags_None, NULL)) {
						std::visit([&](auto&& node_data) {
							using NodeDataT = std::decay_t<decltype(node_data)>;
							if constexpr (is_image_data<NodeDataT>) {
								node_data->update_ubo(color_pin.default_value, *color_pin_index);  
								update_from(&node);
							}
							}, node.data);
					}
					ImGui::EndPopup();
				}
				ed::Resume();
			}
		}

		for (auto& link : links) {
			ed::Link(link.id, link.start_pin_id, link.end_pin_id, ImColor(123, 174, 111, 245), 2.5f);
		}

		if (ed::BeginCreate()) {
			ed::PinId start_pin_id, end_pin_id;
			if (ed::QueryNewLink(&start_pin_id, &end_pin_id)) {
				if (start_pin_id && end_pin_id) {
					// ed::AcceptNewItem() return true when user release mouse button.
					Pin* start_pin;
					Pin* end_pin;
					int start_pin_index = -1;
					int end_pin_index = -1;
					while (start_pin_index < 0 || end_pin_index<0) {
						for (auto& node : nodes) {
							for (size_t i = 0; i < node.outputs.size(); ++i) {
								if (node.outputs[i].id == start_pin_id) {
									start_pin = &node.outputs[i];
									start_pin_index = i;
								} else if(node.outputs[i].id == end_pin_id) {
									end_pin = &node.outputs[i];
									end_pin_index = i;
								}
							}
							for (size_t i = 0; i < node.inputs.size(); ++i) {
								if (node.inputs[i].id == start_pin_id) {
									start_pin = &node.inputs[i];
									start_pin_index = i;
								}
								else if (node.inputs[i].id == end_pin_id) {
									end_pin = &node.inputs[i];
									end_pin_index = i;
								}
							}
						}
					}
					auto showLabel = [](const char* label, ImColor color) {
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
					if (start_pin == end_pin) {
						ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
					}
					else if (start_pin->flow_direction == end_pin->flow_direction) {
						auto hint = (start_pin->flow_direction == PinInOut::INPUT) ?
							"Input Can Only Be Connected To Output" : "Output Can Only Be Connected To Input";
						showLabel(hint, ImColor(45, 32, 32, 180));
						ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
					}
					else if (start_pin->default_value.index() != end_pin->default_value.index()) {
						showLabel("Incompatible Pin Type", ImColor(45, 32, 32, 180));
						ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
					}
					else if (ed::AcceptNewItem()) {
						// Since we accepted new link, lets add one to our list of links.
						
						if (start_pin->flow_direction == PinInOut::INPUT) {
							std::swap(start_pin, end_pin);
							std::swap(start_pin_index, end_pin_index);
						}
						links.emplace(ed::LinkId(get_next_id()), start_pin_id, end_pin_id);
						start_pin->connected_pins.emplace(end_pin);
						end_pin->connected_pins.emplace(start_pin);

						std::visit([&](auto&& node_data) {
							using NodeT = std::decay_t<decltype(node_data)>;
							if constexpr (is_image_data<NodeT>) {
								node_data->update_ubo(start_pin->default_value, end_pin_index); 
							}
							}, end_pin->node->data);
						
						update_from(end_pin->node);
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


		ed::End();
		ed::SetCurrentEditor(nullptr);

	}

	void NodeEditor::create_node_descriptor_pool() {

		constexpr uint32_t descriptor_size = 2000;
		std::array pool_sizes = {
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptor_size },
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptor_size }
		};

		VkDescriptorPoolCreateInfo pool_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			.maxSets = descriptor_size * 2,
			.poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
			.pPoolSizes = pool_sizes.data(),
		};

		if (vkCreateDescriptorPool(engine->device, &pool_info, nullptr, &engine->node_texture_manager.descriptor_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}

		engine->main_deletion_queue.push_function([=]() {
			vkDestroyDescriptorPool(engine->device, engine->node_texture_manager.descriptor_pool, nullptr);
			});
	}

	void NodeEditor::create_node_descriptor_set_layouts() {
		std::array node_descriptor_set_layout_bindings{
			VkDescriptorSetLayoutBinding{
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = max_bindless_node_textures,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			}
		};

		constexpr auto binding_num = node_descriptor_set_layout_bindings.size();

		std::array<VkDescriptorBindingFlags, binding_num> descriptor_binding_flags;
		descriptor_binding_flags[0] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

		VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_create_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			.bindingCount = descriptor_binding_flags.size(),
			.pBindingFlags = descriptor_binding_flags.data(),
		};

		VkDescriptorSetLayoutCreateInfo node_descriptor_layout_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = &binding_flags_create_info,
			.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
			.bindingCount = node_descriptor_set_layout_bindings.size(),
			.pBindings = node_descriptor_set_layout_bindings.data(),
		};

		if (vkCreateDescriptorSetLayout(engine->device, &node_descriptor_layout_info, nullptr, &engine->node_texture_manager.descriptor_set_layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		engine->main_deletion_queue.push_function([=]() {
			vkDestroyDescriptorSetLayout(engine->device, engine->node_texture_manager.descriptor_set_layout, nullptr);
			});
	}

	void NodeEditor::create_node_texture_descriptor_set() {
		VkDescriptorSetVariableDescriptorCountAllocateInfo variabl_descriptor_count_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
			.descriptorSetCount = 1,
			.pDescriptorCounts = &max_bindless_node_textures,
		};

		VkDescriptorSetAllocateInfo descriptor_alloc_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = &variabl_descriptor_count_info,
			.descriptorPool = engine->node_texture_manager.descriptor_pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &engine->node_texture_manager.descriptor_set_layout,
		};

		if (vkAllocateDescriptorSets(engine->device, &descriptor_alloc_info, &engine->node_texture_manager.descriptor_set) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
	}

	NodeEditor::NodeEditor(VulkanEngine* engine) :engine(engine) {
		ed::Config config;
		config.SettingsFile = "Simple.json";
		context = ed::CreateEditor(&config);

		create_node_descriptor_pool();
		create_node_descriptor_set_layouts();
		create_node_texture_descriptor_set();

		for (auto i : std::views::iota(0u, max_bindless_node_textures)) {
			engine->node_texture_manager.unused_id.emplace(i);
		}

		engine->main_deletion_queue.push_function([=]() {
			nodes.clear();
			ed::DestroyEditor(context);
			});
	}
};
