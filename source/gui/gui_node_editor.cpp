#include "gui_node_editor.h"
#include <imgui_internal.h>

#include "../vk_shader.h"

#include <IconsFontAwesome5.h>
#include <json.hpp>
#include <ranges>
#include <fstream>

using json = nlohmann::json;

constexpr bool SHOW_IMGUI_DEMO = false;
constexpr float NODE_WIDTH = 160.0f;


template<typename NodeDataT>
constexpr static bool is_component_graphic = std::derived_from<ref_t<NodeDataT>, ComponentGraphicPipeline<typename ref_t<NodeDataT>::UboType>>;

template<typename NodeDataT>
constexpr static bool is_component_udf = std::derived_from<ref_t<NodeDataT>, ComponentUdf<typename ref_t<NodeDataT>::UboType>>;

template <typename T, typename ArrayElementT>
concept std_array = requires (std::remove_cvref_t<T> t) {
	[] <size_t I> (std::array<ArrayElementT, I>) {}(t);
};

void to_json(json& j, const ImVec2& p) {
	j = std::array{ p.x, p.y };
}

static ImRect imgui_get_item_rect() {
	return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

std::string first_letter_to_upper(std::string_view str) {
	auto split = str
		| std::views::split('_')
		| std::views::transform([](auto&& str_v) {return std::string_view(&*str_v.begin(), std::ranges::distance(str_v)); });

	std::string upper_str = "";
	std::string_view separator = "";
	for (auto&& word : split) {
		upper_str += std::exchange(separator, " ").data() + (static_cast<char>(std::toupper(word[0])) + std::string(word.substr(1, word.size() - 1)));
	}
	return upper_str;
}

namespace engine {
	uint64_t NodeEditor::get_next_id() noexcept {
		return next_id++;
	}

	void NodeEditor::update_node_ubo(NodeDataVariant& node_data, const PinVariant& value, size_t index) {
		std::visit([&](auto&& _node_data) {
			using NodeDataT = std::remove_reference_t<decltype(_node_data)>;
			if constexpr (image_data<NodeDataT>) {
				_node_data->update_ubo(value, index);
			}
			else if constexpr (shader_data<NodeDataT>) {
				_node_data.update_ubo(value, index);
			}
			}, node_data);
	}

	void NodeEditor::topological_sort(const uint32_t index, std::vector<char>& visited_nodes, std::vector<uint32_t>& sorted_nodes) const {
		std::stack<int64_t> topo_sort_stack;
		int64_t idx;

		visited_nodes[index] = 1;
		topo_sort_stack.push(index);

		while (!topo_sort_stack.empty()) { //topological sorting
			idx = topo_sort_stack.top();
			topo_sort_stack.pop();

			if (idx < 0) {
				sorted_nodes.emplace_back(~idx);
			}
			else {
				topo_sort_stack.push(~idx);
				for (auto& pin : nodes[idx].outputs) {
					for (const Pin* connected_pin : pin.connected_pins) {
						auto const connected_node_idx = connected_pin->node_index;
						if (visited_nodes[connected_node_idx] == 0) {
							visited_nodes[connected_node_idx] = 1;
							topo_sort_stack.push(connected_node_idx);
						}
					}
				}
			}
		}
	}

	void NodeEditor::execute_graph(const std::vector<uint32_t>& sorted_nodes) {
		std::vector<VkSubmitInfo2> graphic_submits;
		graphic_submits.reserve(sorted_nodes.size() * 2 + 2);
		std::vector<VkSubmitInfo2> compute_submits;
		compute_submits.reserve(sorted_nodes.size() * 2);
		std::vector<CopyImageSubmitInfo> copy_image_submit_infos;
		copy_image_submit_infos.reserve(PbrMaterialTextureNum);

		for (auto const i : sorted_nodes) {
			std::visit([&](auto&& node_data) {
				using NodeDataT = std::remove_reference_t<decltype(node_data)>;
				if constexpr (image_data<NodeDataT>) {
					node_data->wait_semaphore_submit_info_0.clear();
				}
				}, nodes[i].data);
		}

		for (auto i : sorted_nodes | std::views::reverse) {

			std::visit([&](auto&& node_data) {
				using NodeDataT = std::decay_t<decltype(node_data)>;
				if constexpr (image_data<NodeDataT>) {
					uint64_t counter;
					vkGetSemaphoreCounterValue(engine->device, node_data->semaphore, &counter);
					node_data->signal_semaphore_submit_info_0.value = counter + 1;
					const uint64_t duration = ((node_data->submit_info.size() + 1) >> 1) << 1;
					uint64_t last_signal_counter = counter + duration;

					if constexpr (is_component_graphic<NodeDataT>) {
						node_data->wait_semaphore_submit_info_1.value = counter + 1;
						node_data->signal_semaphore_submit_info_1.value = last_signal_counter;
						update_wait_semaphores(i, node_data, copy_image_submit_infos, last_signal_counter);
						graphic_submits.push_back(node_data->submit_info[0]);
						graphic_submits.push_back(node_data->submit_info[1]);
					}
					else if constexpr (is_component_udf<NodeDataT>) {
						if (node_data->submit_info[0].pCommandBufferInfos->commandBuffer) {
							node_data->submit_info_members[0].wait_semaphore_submit_info.value = counter + 1;
							node_data->submit_info_members[0].signal_semaphore_submit_info.value = counter + 2;
							node_data->submit_info_members[1].wait_semaphore_submit_info.value = counter + 2;
							node_data->submit_info_members[1].signal_semaphore_submit_info.value = last_signal_counter;
							update_wait_semaphores(i, node_data, copy_image_submit_infos, last_signal_counter);
							graphic_submits.push_back(node_data->submit_info[0]);
							compute_submits.push_back(node_data->submit_info[1]);
							graphic_submits.push_back(node_data->submit_info[2]);
						}
					}
				}
				else if constexpr (value_data<NodeDataT>) {
					recalculate_node(i);
					for (auto& output : nodes[i].outputs) {
						for (Pin* connected_pin : output.connected_pins) {
							auto& connected_node = nodes[connected_pin->node_index];
							update_node_ubo(connected_node.data, output.default_value, get_input_pin_index(*connected_pin));
						}
					}
				}
				}, nodes[i].data);

		}

		for (auto& [wait_semaphore_submit_info, cmd_buffer_submit_info] : copy_image_submit_infos) {
			graphic_submits.emplace_back(
				VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
				nullptr,
				0,
				1,
				&wait_semaphore_submit_info,
				1,
				&cmd_buffer_submit_info
			);
		}

		const std::array fences{ graphic_fence, compute_fence };
		vkResetFences(engine->device, fences.size(), fences.data());

		if (vkQueueSubmit2(engine->graphics_queue, graphic_submits.size(), graphic_submits.data(), graphic_fence) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer to graphic queue!");
		}

		if (vkQueueSubmit2(engine->compute_queue, compute_submits.size(), compute_submits.data(), compute_fence) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer to compute queue!");
		}
	}

	void NodeEditor::update_from(uint32_t updated_node_index) {
		if (vkGetFenceStatus(engine->device, graphic_fence) != VK_SUCCESS ||
			vkGetFenceStatus(engine->device, compute_fence) != VK_SUCCESS) {
			return;
		}

		static std::vector<char> visited_nodes; //check if a node has been visited
		visited_nodes.resize(nodes.size());
		visited_nodes.assign(visited_nodes.size(), 0);

		std::vector<uint32_t> sorted_nodes;
		sorted_nodes.reserve(nodes.size());

		topological_sort(updated_node_index, visited_nodes, sorted_nodes);
		execute_graph(sorted_nodes);
	}

	void NodeEditor::update_all_nodes() {
		wait_node_execute_fences();

		static std::vector<char> visited_nodes; //check if a node has been visited
		visited_nodes.resize(nodes.size());
		visited_nodes.assign(visited_nodes.size(), 0);

		std::vector<uint32_t> sorted_nodes;
		sorted_nodes.reserve(nodes.size());

		for (auto i : std::views::iota(0u, nodes.size())) {
			if (visited_nodes[i] == 0) {
				topological_sort(i, visited_nodes, sorted_nodes);
			}
		}

		execute_graph(sorted_nodes);
		wait_node_execute_fences();
	}

	void NodeEditor::build_node(uint32_t node_index) {
		for (auto&& input : nodes[node_index].inputs) {
			input.flow_direction = PinInOut::INPUT;
		}

		for (auto&& output : nodes[node_index].outputs) {
			output.flow_direction = PinInOut::OUTPUT;
		}
	}

	bool NodeEditor::is_pin_connection_valid(const PinVariant& output_pin, const PinVariant& input_pin) {
		return output_pin.index() == input_pin.index()
			|| std::holds_alternative<FloatData>(output_pin) && std::holds_alternative<FloatTextureIdData>(input_pin)
			|| std::holds_alternative<Color4Data>(output_pin) && std::holds_alternative<Color4TextureIdData>(input_pin)
			|| std::holds_alternative<TextureIdData>(output_pin) &&
			(std::holds_alternative<FloatTextureIdData>(input_pin) ||
				std::holds_alternative<Color4TextureIdData>(input_pin));
	}

	void NodeEditor::draw() {
		//static bool first = true;
		auto const& io = ImGui::GetIO();

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("Add")) {
				node_menu<NodeMenu>();
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		if constexpr (SHOW_IMGUI_DEMO) {
			ImGui::ShowDemoWindow();
		}

		ed::SetCurrentEditor(context);
		ed::Begin("My Editor", ImVec2(0.0f, 0.0f));

		ed::Suspend();
		if (ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_Tab])) {
			ImGui::OpenPopup("Add New Node");
		}
		if (ImGui::BeginPopup("Add New Node")) {
			node_menu<NodeMenu, SetNodePositionTag>();
			ImGui::EndPopup();
		}
		ed::Resume();

		//Set display node
		if (auto const node_id = ed::GetDoubleClickedNode(); node_id != ed::NodeId::Invalid) {
			display_node_id = node_id;
		}

		static std::optional<size_t> color_node_index;
		bool hit_color_pin = false;  //implies whether enum pin has been hit

		static std::optional<size_t> color_ramp_node_index;
		bool hit_color_ramp_pin = false;  //implies whether color ramp pin has been hit

		static std::optional<size_t> enum_node_index;
		bool hit_enum_pin = false;  //implies whether enum pin has been hit



		// Start drawing nodes.
		for (std::size_t node_index = 0; node_index < nodes.size(); ++node_index) {
			auto& node = nodes[node_index];
			if (!node.display_panel) {
				ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4{ 8,8,8,-2 });
			}
			ed::BeginNode(node.id);
			auto yy = ImGui::GetCursorPosY();
			auto const draw_list = ImGui::GetWindowDrawList();
			bool display_panel_cache = node.display_panel;
			node.display_panel ^= ImGui::InvisibleButton(std::format("##{}", node.id.Get()).c_str(), ImGui::CalcTextSize(".O") * ImVec2 { 1.5f, 1 });
			auto dummy_rect = imgui_get_item_rect();
			//ImGui::SetCursorPosX(dummy_rect.Max.x);
			//ImGui::SetCursorPosY(dummy_rect.Max.y);
			float button_width = dummy_rect.GetWidth();
			ImVec2 arrow_right_corner = ImVec2{ dummy_rect.Min.x + 0.55f * button_width,dummy_rect.GetCenter().y };
			constexpr float line_thickness = 2;
			draw_list->AddLine(
				arrow_right_corner + ImVec2{ -button_width,button_width }*0.25f,
				arrow_right_corner,
				ImColor(255, 255, 255, 255),
				line_thickness);
			draw_list->AddLine(
				arrow_right_corner + ImVec2{ -button_width,-button_width }*0.25f,
				arrow_right_corner + ImVec2{ line_thickness,line_thickness } *0.35355339f,
				ImColor(255, 255, 255, 255),
				line_thickness);
			ImGui::SameLine();
			ImGui::SetCursorPosX(dummy_rect.Min.x + ImGui::CalcTextSize("OO").x);
			ImGui::Text(node.name.c_str());
			ImGui::Dummy(ImVec2(NODE_WIDTH, 3.0f));

			//ImGui::BeginVertical("delegates", ImVec2(0, 28));

			constexpr float radius = 5.8f;
			auto node_rect = imgui_get_item_rect();

			if (display_panel_cache) {
				ImGui::GetWindowDrawList()->AddRectFilled(node_rect.GetTL(), node_rect.GetBR(), ImColor(68, 129, 196, 160));
				//loop over output pins
				for (auto& pin : node.outputs) {
					//ed::PushStyleVar(ed::StyleVar_PinCorners, 15);
					ed::BeginPin(pin.id, ed::PinKind::Output);
					//auto rect = imgui_get_item_rect();

					auto const str = pin.name.c_str();
					ImGui::SetCursorPosX(node_rect.Max.x - ImGui::CalcTextSize(str).x);
					ImGui::Text(str);

					auto rect = imgui_get_item_rect();

					auto const pin_center = ImVec2(node_rect.Max.x + 8.0f, rect.GetCenter().y);

					draw_list->AddCircleFilled(pin_center, radius, ImColor(68, 129, 196, 160), 24);
					draw_list->AddCircle(ImVec2(node_rect.Max.x + 8.0f, rect.GetCenter().y), radius, ImColor(68, 129, 196, 255), 24, 1.8);

					ed::PinPivotRect(pin_center, pin_center);
					ed::PinRect(ImVec2(pin_center.x - radius, pin_center.y - radius), ImVec2(pin_center.x + radius, pin_center.y + radius));

					ed::EndPin();
					//ed::PopStyleVar(1);
				}

				//Loop over input pins
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
								bool response_flag = false;
								std::visit([&](auto&& node_data) {
									using NodeDataT = std::decay_t<decltype(node_data)>;
									UboOf<NodeDataT>::Class::FieldAt(i, [&](auto& field) {
										auto widget_info = field.template getAnnotation<NumberInputWidgetInfo>();
										ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
										ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 3.0f, 1.0f });
										if constexpr (std::is_same_v<PinT, FloatData>) {
											if (widget_info.enable_slider) {
												response_flag |= ImGui::SliderFloat(std::format("##{}", pin->id.Get()).c_str(),
													&std::get_if<PinT>(&pin->default_value)->value,
													widget_info.min,
													widget_info.max,
													(pin->name + " : %.3f").c_str());
											}
											else {
												response_flag |= ImGui::DragFloat(std::format("##{}", pin->id.Get()).c_str(),
													&std::get_if<PinT>(&pin->default_value)->value,
													widget_info.speed,
													widget_info.min,
													widget_info.max,
													(pin->name + " : %.3f").c_str());
											}
										}
										else if constexpr (std::is_same_v<PinT, IntData>) {
											response_flag |= ImGui::DragInt(std::format("##{}", pin->id.Get()).c_str(),
												&std::get_if<PinT>(&pin->default_value)->value,
												widget_info.speed,
												static_cast<int>(widget_info.min),
												static_cast<int>(widget_info.max),
												(pin->name + " : %d").c_str());
										}
										ImGui::PopStyleVar(2);
										if (response_flag) {
											if constexpr (image_data<NodeDataT>) {
												node_data->update_ubo(pin->default_value, i);
												update_from(node_index);
											}
											else if constexpr (value_data<NodeDataT>) {
												update_from(node_index);
											}
											else if constexpr (shader_data<NodeDataT>) {
												node_data.update_ubo(pin->default_value, i);
											}
										}
										});
									}, node.data);

								ImGui::PopItemWidth();
							}
							else {
								ImGui::Text(pin->name.c_str());
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
								ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
								if (ImGui::ColorButton(std::format("ColorButton##{}", pin->id.Get()).c_str(), std::bit_cast<ImVec4>(default_value), ImGuiColorEditFlags_NoTooltip, ImVec2{ 50, 25 })) {
									color_node_index = node_index;
									color_pin_index = i;
									hit_color_pin = true;
								}
								ImGui::PopStyleVar(1);
							}
						}
						else if constexpr (std::is_same_v<PinT, Color4TextureIdData>) {
							ImGui::Text(pin->name.c_str());
							rect = imgui_get_item_rect();
							if (pin->connected_pins.empty()) {
								ImGui::PushItemWidth(100.f);
								ImGui::SameLine();
								ImGui::PopItemWidth();
								ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
								if (ImGui::ColorButton(std::format("ColorButton##{}", pin->id.Get()).c_str(), *reinterpret_cast<ImVec4*>(&default_value), ImGuiColorEditFlags_NoTooltip, ImVec2{ 50, 25 })) {
									color_node_index = node_index;
									color_pin_index = i;
									hit_color_pin = true;
								}
								ImGui::PopStyleVar(1);
							}
						}
						else if constexpr (std::is_same_v<PinT, TextureIdData>) {
							ImGui::Text(pin->name.c_str());
							rect = imgui_get_item_rect();
						}
						else if constexpr (std::is_same_v<PinT, FloatTextureIdData>) {
							if (pin->connected_pins.empty()) {
								ImGui::PushItemWidth(150.f);
								//ImGui::SameLine();
								bool response_flag = false;
								std::visit([&](auto&& node_data) {
									using NodeDataT = std::decay_t<decltype(node_data)>;
									UboOf<NodeDataT>::Class::FieldAt(i, [&](auto& field) {
										auto widget_info = field.template getAnnotation<NumberInputWidgetInfo>();
										ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
										if (widget_info.enable_slider) {
											response_flag = ImGui::SliderFloat(std::format("##{}", pin->id.Get()).c_str(),
												&(std::get_if<PinT>(&pin->default_value)->value.number),
												widget_info.min,
												widget_info.max,
												(pin->name + " : %.3f").c_str());
										}
										else {
											response_flag = ImGui::DragFloat(std::format("##{}", pin->id.Get()).c_str(),
												&(std::get_if<PinT>(&pin->default_value)->value.number),
												widget_info.speed,
												widget_info.min,
												widget_info.max,
												(pin->name + " : %.3f").c_str());
										}
										ImGui::PopStyleVar(1);
										if (response_flag) {
											if constexpr (image_data<NodeDataT>) {
												node_data->update_ubo(pin->default_value, i);
												update_from(node_index);
											}
											else if constexpr (shader_data<NodeDataT>) {
												node_data.update_ubo(pin->default_value, i);
												update_from(node_index);
											}
											else {
												assert(!"Error occurs during processing FloatTextureIdData");
											}
										}
										});
									}, node.data);

								ImGui::PopItemWidth();
							}
							else {
								ImGui::Text(pin->name.c_str());
							}
							rect = imgui_get_item_rect();
						}
						else if constexpr (std::is_same_v<PinT, EnumData>) {
							ImGui::Text(pin->name.c_str());
							rect = imgui_get_item_rect();
							ImGui::SameLine();
							std::visit([&](auto&& node_data) {
								using NodeDataT = std::decay_t<decltype(node_data)>;
								UboOf<NodeDataT>::Class::FieldAt(i, [&](auto& field) {
									field.forEachAnnotation([&](auto& items) {
										using T = std::remove_cvref_t<decltype(items)>;
										if constexpr (std_array<T, const char*>) {
											ImGui::PushItemWidth(50.f);
											ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
											ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{ 0.0f,0.0f });
											auto const pin_name_width = ImGui::CalcTextSize(pin->name.c_str()).x;
											if (ImGui::Button(std::format("{}##{}", items[default_value.value], pin->id.Get()).c_str(), ImVec2{ NODE_WIDTH - pin_name_width - 10,28 })) {
												enum_node_index = node_index;
												enum_pin_index = i;
												hit_enum_pin = true;
											}
											ImGui::PopStyleVar(2);
											ImGui::PopItemWidth();
										}
										});
									});
								}, node.data);
						}
						else if constexpr (std::is_same_v<PinT, BoolData>) {
							BoolData* bool_data = std::get_if<BoolData>(&node.inputs[i].default_value);
							if (ImGui::Checkbox(std::format("{}##{}", pin->name, pin->id.Get()).c_str(), &bool_data->value)) {
								std::visit([&](auto&& node_data) {
									using NodeT = std::decay_t<decltype(node_data)>;
									if constexpr (image_data<NodeT>) {
										node_data->update_ubo(pin->default_value, i);
										update_from(node_index);
									}
									}, node.data);
							}
							//ImGui::Text(pin->name.c_str());
							rect = imgui_get_item_rect();
						}
						else if constexpr (std::is_same_v<PinT, ColorRampData>) {
							ImGui::Dummy(ImVec2(2.0f, 25.0f));
							rect = imgui_get_item_rect();
							ImGui::SameLine();
							auto& color_ramp_data = *(std::get_if<ColorRampData>(&node.inputs[i].default_value)->ui_value);
							if (ImGui::GradientButton(std::format("GradientBar##{}", pin->id.Get()).c_str(), &color_ramp_data, 140.0f)) {
								color_ramp_node_index = node_index;
								color_ramp_pin_index = i;
								hit_color_ramp_pin = true;
							}
						}
						}, pin->default_value);


					auto const drawList = ImGui::GetWindowDrawList();
					ImVec2 pin_center = ImVec2(rect.Min.x - 8.0f, rect.GetCenter().y);
					drawList->AddCircleFilled(pin_center, radius, ImColor(68, 129, 196, 160), 24);
					drawList->AddCircle(pin_center, radius, ImColor(68, 129, 196, 255), 24, 1.8);

					ed::PinPivotRect(pin_center, pin_center);
					ed::PinRect(ImVec2(pin_center.x - radius, pin_center.y - radius), ImVec2(pin_center.x + radius, pin_center.y + radius));

					ed::EndPin();
					ed::PopStyleVar(1);
				}
				ed::EndNode();
			}
			else {
				float pin_center_y = dummy_rect.GetCenter().y;
				auto pin_center = ImVec2(node_rect.Max.x + 8.0f, pin_center_y);
				draw_list->AddCircleFilled(
					pin_center,
					radius,
					ImColor(68, 129, 196, 160),
					24);
				draw_list->AddCircle(
					ImVec2(node_rect.Max.x + 8.0f, pin_center_y),
					radius,
					ImColor(68, 129, 196, 255),
					24,
					1.8);

				ed::PushStyleVar(ed::StyleVar_GroupBorderWidth, 0.0f);
				for (auto& pin : node.outputs) {
					//ed::PushStyleVar(ed::StyleVar_PinCorners, 15);
					ed::BeginPin(pin.id, ed::PinKind::Output);
					ed::PinPivotRect(pin_center, pin_center);
					ed::PinRect(
						ImVec2(pin_center.x - radius, pin_center_y - radius),
						ImVec2(pin_center.x + radius, pin_center_y + radius)
					);
					ed::EndPin();
					//ed::PopStyleVar(1);
				}

				pin_center = ImVec2(node_rect.Min.x - 8.0f, pin_center_y);
				draw_list->AddCircleFilled(
					pin_center,
					radius,
					ImColor(68, 129, 196, 160),
					24);
				draw_list->AddCircle(
					pin_center,
					radius,
					ImColor(68, 129, 196, 255),
					24,
					1.8);

				for (auto& pin : node.inputs) {
					ed::BeginPin(pin.id, ed::PinKind::Input);
					ed::PinPivotRect(pin_center, pin_center);
					ed::PinRect(
						ImVec2(pin_center.x - radius, pin_center_y - radius),
						ImVec2(pin_center.x + radius, pin_center_y + radius)
					);
					ed::EndPin();
				}
				ed::PopStyleVar(1);
				ed::EndNode();
				ed::PopStyleVar(1);
			}

			//Draw display icon
			if (node.id == display_node_id) {
				ImGui::SetCursorPosX(node_rect.GetTR().x + 10);
				ImGui::SetCursorPosY(yy);
				ImGui::TextColored(ImVec4(1.0f, 0.324f, 0.0f, 1.0f), "  " ICON_FA_EYE);
				std::visit([&](auto&& node_data) {
					using T = std::decay_t<decltype(node_data)>;
					if constexpr (image_data<T>) {
						gui_display_texture_handle = node_data->gui_texture;
					}
					else {
						gui_display_texture_handle = nullptr;
					}
					}, node.data);
			}

			//Draw preview image
			std::visit([&](auto&& node_data) {
				using T = std::decay_t<decltype(node_data)>;
				if constexpr (image_data<T>) {
					ImGui::SetCursorPosX(node_rect.GetCenter().x - preview_image_size * 0.5);
					ImGui::SetCursorPosY(yy - preview_image_size - 15);
					uint64_t counter;
					vkGetSemaphoreCounterValue(engine->device, node_data->semaphore, &counter);
					if (counter & 1) { //counter is odd implies the preview texture is being written
						const uint64_t wait_value = counter + 1;
						preview_semaphore_wait_info.pSemaphores = &node_data->semaphore;
						preview_semaphore_wait_info.pValues = &wait_value;
						vkWaitSemaphores(engine->device, &preview_semaphore_wait_info, VULKAN_WAIT_TIMEOUT);
					}
					ImGui::Image(static_cast<ImTextureID>(node_data->gui_preview_texture), ImVec2{ preview_image_size, preview_image_size }, ImVec2{ 0, 0 }, ImVec2{ 1, 1 });
					ImGui::SetCursorPosX(node_rect.GetCenter().x - preview_image_size * 0.5);
					ImGui::SetCursorPosY(yy - preview_image_size - 40);

					ImGui::Text(str_format_map.at(node_data->texture->format));
				}
				}, node.data);
		}

		for (auto& link : links) {
			ed::Link(link.id, link.start_pin->id, link.end_pin->id, ImColor(123, 174, 111, 245), 2.5f);
		}

		//Processing color pin popup
		if (color_node_index.has_value() && color_pin_index.has_value()) {
			ed::Suspend();
			auto& color_node = nodes[*color_node_index];
			auto& color_pin = color_node.inputs[*color_pin_index];
			if (hit_color_pin) {
				ImGui::OpenPopup(std::format("ColorPopup##{}", color_pin.id.Get()).c_str());
			}

			if (ImGui::BeginPopup(std::format("ColorPopup##{}", color_pin.id.Get()).c_str())) {
				if (ImGui::ColorPicker4(std::format("##ColorPicker{}", color_pin.id.Get()).c_str(), reinterpret_cast<float*>(&color_pin.default_value), ImGuiColorEditFlags_None, nullptr)) {
					std::visit([&](auto&& node_data) {
						using NodeDataT = std::decay_t<decltype(node_data)>;
						if constexpr (image_data<NodeDataT>) {
							node_data->update_ubo(color_pin.default_value, *color_pin_index);
							update_from(*color_node_index);
						}
						else if constexpr (shader_data<NodeDataT>) {
							node_data.update_ubo(color_pin.default_value, *color_pin_index);
						}
						}, color_node.data);
				}
				ImGui::EndPopup();
			}
			ed::Resume();
		}

		//Processing color ramp pin popup
		if (color_ramp_node_index.has_value() && color_ramp_pin_index.has_value()) {
			ed::Suspend();
			auto& color_ramp_node = nodes[*color_ramp_node_index];
			auto& color_ramp_pin = color_ramp_node.inputs[*color_ramp_pin_index];
			if (hit_color_ramp_pin) {
				ImGui::OpenPopup(std::format("ColorRampPopup##{}", color_ramp_pin.id.Get()).c_str());
			}

			if (ImGui::BeginPopup(std::format("ColorRampPopup##{}", color_ramp_pin.id.Get()).c_str())) {
				auto& color_ramp_data = *std::get_if<ColorRampData>(&color_ramp_pin.default_value);
				if (ImGui::GradientEditor(std::format("ColorRampEditor##{}", color_ramp_pin.id.Get()).c_str(), color_ramp_data.ui_value.get(), color_ramp_data.dragging_mark, color_ramp_data.selected_mark)) {
					std::visit([&](auto&& node_data) {
						using NodeDataT = std::decay_t<decltype(node_data)>;
						if constexpr (image_data<NodeDataT>) {
							node_data->update_ubo(color_ramp_pin.default_value, *color_ramp_pin_index);
							if (vkGetFenceStatus(engine->device, graphic_fence) == VK_SUCCESS) {
								const VkSubmitInfo submit_info{
									.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
									.commandBufferCount = 1,
									.pCommandBuffers = &color_ramp_data.ubo_value->command_buffer,
								};
								vkResetFences(engine->device, 1, &graphic_fence);
								vkQueueSubmit(engine->graphics_queue, 1, &submit_info, graphic_fence);
								wait_node_execute_fences();
								update_from(*color_ramp_node_index);
							}
						}
						}, color_ramp_node.data);
				}
				ImGui::EndPopup();
			}
			ed::Resume();
		}

		//Processing enum pin popup
		if (enum_node_index.has_value() && enum_pin_index.has_value()) {
			ed::Suspend();
			auto& enum_node = nodes[*enum_node_index];
			auto& enum_pin = enum_node.inputs[*enum_pin_index];

			if (hit_enum_pin) {
				ImGui::OpenPopup(std::format("EnumPopup##{}", enum_pin.id.Get()).c_str());
			}

			if (ImGui::BeginPopup(std::format("EnumPopup##{}", enum_pin.id.Get()).c_str())) {
				std::visit([&](auto&& node_data) {
					using NodeDataT = std::decay_t<decltype(node_data)>;
					if constexpr (image_data<NodeDataT>) {
						UboOf<NodeDataT>::Class::FieldAt(*enum_pin_index, [&](auto& field) {
							field.forEachAnnotation([&](auto& items) {
								using AnnotationT = std::remove_cvref_t<decltype(items)>;
								if constexpr (std_array<AnnotationT, const char*>) {
									for (size_t i = 0; i < items.size(); ++i) {
										if (ImGui::MenuItem(items[i])) {
											std::get_if<EnumData>(&enum_pin.default_value)->value = i;
											if constexpr (image_data<NodeDataT>) {
												if (field.template getAnnotation<FormatEnum>() == FormatEnum::True) {
													wait_node_execute_fences();
													node_data->recreate_texture_resource(str_format_map.get_key(i));
												}
												else {
													node_data->update_ubo(enum_pin.default_value, *enum_pin_index);
												}
											}
											update_from(*enum_node_index);
											ImGui::CloseCurrentPopup();
											enum_node_index.reset();
											enum_pin_index.reset();
										}
									}
								}
								});
							});
					}
					}, enum_node.data);
				ImGui::EndPopup();
			}
			ed::Resume();
		}


		//Create new link
		if (ed::BeginCreate()) {
			ed::PinId start_pin_id, end_pin_id;
			if (ed::QueryNewLink(&start_pin_id, &end_pin_id)) {
				if (start_pin_id && end_pin_id) {
					// ed::AcceptNewItem() return true when user release mouse button.
					Pin* start_pin = nullptr;
					Pin* end_pin = nullptr;
					int start_pin_index = -1;
					int end_pin_index = -1;

					for (auto& node : nodes) {
						for (size_t i = 0; auto & output: node.outputs) {
							if (output.id == start_pin_id) {
								start_pin = &output;
								start_pin_index = i;
							}
							if (output.id == end_pin_id) {
								end_pin = &output;
								end_pin_index = i;
							}
							++i;
						}
						for (size_t i = 0; auto & input: node.inputs) {
							if (input.id == start_pin_id) {
								start_pin = &input;
								start_pin_index = i;
							}
							if (input.id == end_pin_id) {
								end_pin = &input;
								end_pin_index = i;
							}
							++i;
						}
						if (start_pin_index >= 0 && end_pin_index >= 0) {
							break;
						}
					}
					assert(start_pin_index >= 0 && end_pin_index >= 0);

					auto show_label = [](const char* label, const ImColor color) {
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
						auto const size = ImGui::CalcTextSize(label);

						auto const padding = ImGui::GetStyle().FramePadding;
						auto const spacing = ImGui::GetStyle().ItemSpacing;

						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

						auto const rect_min = ImGui::GetCursorScreenPos() - padding;
						auto const rect_max = ImGui::GetCursorScreenPos() + size + padding;

						ImGui::GetWindowDrawList()->AddRectFilled(rect_min, rect_max, color, size.y * 0.15f);
						ImGui::TextUnformatted(label);
					};

					if (start_pin == end_pin) {
						ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
					}
					else if (start_pin->flow_direction == end_pin->flow_direction) {
						auto const hint = (start_pin->flow_direction == PinInOut::INPUT) ?
							"Input Can Only Be Connected To Output" : "Output Can Only Be Connected To Input";
						show_label(hint, ImColor(45, 32, 32, 180));
						ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
					}
					else if (start_pin->flow_direction == PinInOut::INPUT && !is_pin_connection_valid(end_pin->default_value, start_pin->default_value)
						|| start_pin->flow_direction == PinInOut::OUTPUT && !is_pin_connection_valid(start_pin->default_value, end_pin->default_value)) {

						show_label("Incompatible Pin Type", ImColor(45, 32, 32, 180));
						ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
					}
					else if (ed::AcceptNewItem()) {
						// Since we accepted new link, lets add one to our list of links.

						if (start_pin->flow_direction == PinInOut::INPUT) {
							std::swap(start_pin, end_pin);
							std::swap(start_pin_index, end_pin_index);
						}

						start_pin->connected_pins.emplace(end_pin);
						if (!end_pin->connected_pins.empty()) {
							auto const deleted_link = std::ranges::find_if(links, [=](auto& link) {
								return link.end_pin->id == end_pin->id;
								});
							if (deleted_link != links.end()) {
								links.erase(deleted_link);
							}
							for (Pin* pin : end_pin->connected_pins) {
								pin->connected_pins.erase(end_pin);
							}
							end_pin->connected_pins.clear();
						}
						end_pin->connected_pins.emplace(start_pin);

						links.emplace(ed::LinkId(get_next_id()), start_pin, end_pin);

						std::visit([&](auto&& end_node_data) {
							using EndNodeT = std::decay_t<decltype(end_node_data)>;
							if constexpr (image_data<EndNodeT>) {

								UboOf<EndNodeT>::Class::FieldAt(end_pin_index, [&](auto& field) {
									if (field.template getAnnotation<AutoFormat>() == AutoFormat::True) {
										std::visit([&](auto&& start_node_data) {
											using StartNodeT = std::decay_t<decltype(start_node_data)>;
											if constexpr (image_data<StartNodeT>) {
												auto format = start_node_data->texture->format;
												if (format != end_node_data->texture->format) {
													wait_node_execute_fences();
													end_node_data->recreate_texture_resource(format);
												}
											}
											}, nodes[start_pin->node_index].data);
									}
									if constexpr (requires(EndNodeT node_data) { node_data->record_image_processing_cmd_buffer_func(0); }) {
										std::visit([&](auto&& start_node_data) {
											using StartNodeT = std::decay_t<decltype(start_node_data)>;
											if constexpr (image_data<StartNodeT>) {
												end_node_data->record_image_processing_cmd_buffer_func(start_node_data->node_texture_id);
												end_node_data->record_preview_cmd_buffer_func(start_node_data->node_texture_id);
												end_node_data->update_command_buffer_submit_info();
											}
											}, nodes[start_pin->node_index].data);
									}
									});
								if (std::holds_alternative<FloatData>(start_pin->default_value) &&
									std::holds_alternative<FloatTextureIdData>(end_pin->default_value)) {
									std::get_if<FloatTextureIdData>(&end_pin->default_value)->value.id = -1;
								}
								end_node_data->update_ubo(start_pin->default_value, end_pin_index);
							}
							else if constexpr (shader_data<EndNodeT>) {
								wait_node_execute_fences();
								if (std::holds_alternative<FloatData>(start_pin->default_value) &&
									std::holds_alternative<FloatTextureIdData>(end_pin->default_value)) {
									std::get_if<FloatTextureIdData>(&end_pin->default_value)->value.id = -1;
								}
								else if (std::holds_alternative<Color4Data>(start_pin->default_value) &&
									std::holds_alternative<Color4TextureIdData>(end_pin->default_value)) {
									std::get_if<Color4TextureIdData>(&end_pin->default_value)->value.id = -1;
								}
								else {
									std::visit([&](auto&& start_node_data) {
										using StartNodeT = std::decay_t<decltype(start_node_data)>;
										if constexpr (image_data<StartNodeT>) {
											field_at(engine->pbr_material_texture_set, end_pin_index, [&](auto pbr_texture_id) {
												auto input_format = start_node_data->texture->format;
												auto& pbr_texture = engine->texture_manager->textures[pbr_texture_id];
												if (pbr_texture == nullptr || input_format != pbr_texture->format) {
													pbr_texture = Texture::create_device_texture(engine,
														TEXTURE_WIDTH,
														TEXTURE_HEIGHT,
														input_format,
														VK_IMAGE_ASPECT_COLOR_BIT,
														VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
													pbr_texture->transition_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
													engine->update_image_descriptor(pbr_texture, pbr_texture_id);
													start_node_data->record_copy_image_cmd_buffers(end_pin_index);
												}
												});

										}
										}, nodes[start_pin->node_index].data);
								}
								end_node_data.update_ubo(start_pin->default_value, end_pin_index);

								VkSubmitInfo submit_info{
									.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
									.commandBufferCount = 1,
								};

								std::visit([&](auto&& start_node_data) {
									using StartNodeT = std::decay_t<decltype(start_node_data)>;
									if constexpr (image_data<StartNodeT>) {
										submit_info.pCommandBuffers = &start_node_data->copy_image_cmd_buffers[end_pin_index];
										vkResetFences(engine->device, 1, &graphic_fence);
										vkQueueSubmit(engine->graphics_queue, 1, &submit_info, graphic_fence);
										wait_node_execute_fences();
									}
									}, nodes[start_pin->node_index].data);
							}
							}, nodes[end_pin->node_index].data);

						update_from(end_pin->node_index);
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
					auto deleted_link = std::ranges::find_if(links, [=](auto& link) {
						return link.id == deleted_link_id;
						});

					if (deleted_link != links.end()) {
						auto link_start_pin = deleted_link->start_pin;
						auto link_end_pin = deleted_link->end_pin;

						link_start_pin->connected_pins.erase(link_end_pin);
						link_end_pin->connected_pins.erase(link_start_pin);

						links.erase(deleted_link);

						std::visit([&](auto&& end_node_data) {
							using EndNodeT = std::decay_t<decltype(end_node_data)>;
							if constexpr (image_data<EndNodeT> || shader_data<EndNodeT>) {
								std::visit([&](auto&& end_pin_value) {
									using PinT = std::decay_t<decltype(end_pin_value)>;
									if constexpr (std::same_as<PinT, TextureIdData>) {
										end_pin_value.value = -1;
									}
									else if constexpr (std::same_as<PinT, FloatTextureIdData> || std::same_as<PinT, Color4TextureIdData>) {
										end_pin_value.value.id = -1;
									}
									}, link_end_pin->default_value);
								if constexpr (image_data<EndNodeT>) {
									end_node_data->update_ubo(link_end_pin->default_value, get_input_pin_index(*link_end_pin));
								}
								else if constexpr (shader_data<EndNodeT>) {
									end_node_data.update_ubo(link_end_pin->default_value, get_input_pin_index(*link_end_pin));
								}
							}
							}, nodes[link_end_pin->node_index].data);
						wait_node_execute_fences();
						update_from(link_end_pin->node_index);
					}
				}
			}

			ed::NodeId deleted_node_id;
			while (ed::QueryDeletedNode(&deleted_node_id)) {
				if (ed::AcceptDeletedItem()) {
					auto deleted_node = std::ranges::find_if(nodes, [=](auto& node) {
						return node.id == deleted_node_id;
						});

					if (deleted_node != nodes.end()) {
						color_pin_index.reset();
						color_ramp_pin_index.reset();
						enum_pin_index.reset();

						wait_node_execute_fences();
						for (auto iter = deleted_node + 1; iter != nodes.end(); ++iter) {
							for (auto& input : iter->inputs) {
								--input.node_index;
							}
							for (auto& output : iter->outputs) {
								--output.node_index;
							}
						}
						nodes.erase(deleted_node);
					}
				}
			}
		}
		ed::EndDelete(); // Wrap up deletion action

		//shortcut
		if (ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_Space])) {
			ed::NavigateToContent();
		}
		if (ImGui::IsKeyPressed('F')) {
			ed::NavigateToSelection();
		}

		ed::End();
		ed::SetCurrentEditor(nullptr);
	}

	void NodeEditor::create_fence() {
		auto const fence_info = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
		if (vkCreateFence(engine->device, &fence_info, nullptr, &graphic_fence) != VK_SUCCESS) {
			throw std::runtime_error("failed to create fence!");
		}

		if (vkCreateFence(engine->device, &fence_info, nullptr, &compute_fence) != VK_SUCCESS) {
			throw std::runtime_error("failed to create fence!");
		}
	}

	NodeEditor::NodeEditor(VulkanEngine* engine) :engine(engine) {
		ed::Config config;
		//disable writing json
		config.SaveSettings = [](const char*, size_t, ed::SaveReasonFlags, void*) {
			return false;
		};

		config.SaveNodeSettings = [](ed::NodeId, const char*, size_t, ed::SaveReasonFlags, void*) {
			return false;
		};
		context = ed::CreateEditor(&config);

		create_fence();

		engine->main_deletion_queue.push_function([&nodes = nodes, device = engine->device, c_fence = compute_fence, g_fence = graphic_fence, context = context]{
			nodes.clear();
			vkDestroyFence(device, c_fence, nullptr);
			vkDestroyFence(device, g_fence, nullptr);
			ed::DestroyEditor(context);
			});
	}

	void NodeEditor::serialize(const std::string_view file_path) {
		json json_file;
		ed::SetCurrentEditor(context);
		for (auto& node : nodes) {
			json_file["nodes"].emplace_back(json{
				{"type", NODE_TYPE_NAMES[node.data.index()]},
				{"type_hash", NODE_TYPE_HASH_VALUES[node.data.index()]},
				{"pins", node.inputs},
				{"pos", GetNodePosition(node.id)},
				});
		}
		for (auto& link : links) {
			auto start_node_index = link.start_pin->node_index;
			auto start_pin_index = get_output_pin_index(*link.start_pin);
			auto end_node_index = link.end_pin->node_index;
			auto end_pin_index = get_input_pin_index(*link.end_pin);

			json_file["links"].emplace_back(json{
				{"start_node_index", start_node_index},
				{"start_pin_index", start_pin_index},
				{"end_node_index", end_node_index},
				{"end_pin_index", end_pin_index},
				});
		}

		json_file["view"] = {
			{
				"origin",{
					ed::GetCurrentViewOrigin().x,
					ed::GetCurrentViewOrigin().y,
				}
			},
			{
				"scale", ed::GetCurrentViewScale(),
			}
		};
		std::ofstream o_file(file_path.data());
		o_file << std::setw(4) << json_file << std::endl;
		ed::SetCurrentEditor(nullptr);
	}

	void NodeEditor::deserialize(const std::string_view file_path) {
		std::ifstream i_file(file_path.data());
		json json_file;
		i_file >> json_file;
		ed::SetCurrentEditor(context);

		for (size_t node_index = 0; node_index < json_file["nodes"].size(); ++node_index) {
			auto& json_node = json_file["nodes"][node_index];
			UNROLL<NodeTypeList::size>([&] <std::size_t type_index>() {
				if (json_node["type_hash"] == NODE_TYPE_HASH_VALUES[type_index]) {
					using NodeType = NodeTypeList::at<type_index>;
					create_node<NodeType>();

					using NodeDataT = typename NodeType::data_type;
					using UboT = UboOf<NodeDataT>;
					using FieldTypes = FieldTypeList<UboT>;

					UNROLL<FieldTypes::size>([&] <size_t pin_index>() {
						using PinType = typename FieldTypes::template at<pin_index>;
						auto& node = nodes[node_index];
						auto& pin_value = node.inputs[pin_index].default_value;

						if constexpr (std::same_as<PinType, ColorRampData>) {
							auto const& ramp_ui_value = std::get_if<ColorRampData>(&pin_value)->ui_value;
							ramp_ui_value->clear_marks();
							for (auto& json_mark : json_node["pins"][pin_index]) {
								ramp_ui_value->insert_mark(json_mark["position"].get<float>(), json_mark["color"].get<ImColor>());
							}
							ramp_ui_value->refreshCache();
							wait_node_execute_fences();
							const VkSubmitInfo submit_info{
								.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
								.commandBufferCount = 1,
								.pCommandBuffers = &(std::get_if<ColorRampData>(&pin_value)->ubo_value->command_buffer),
							};
							vkResetFences(engine->device, 1, &graphic_fence);
							vkQueueSubmit(engine->graphics_queue, 1, &submit_info, graphic_fence);
							wait_node_execute_fences();
						}
						else if constexpr (!std::same_as<PinType, TextureIdData>) {
							pin_value = json_node["pins"][pin_index].get<PinType>();
							if constexpr (image_data<NodeDataT>) {
								(*std::get_if<NodeDataT>(&node.data))->update_ubo(pin_value, pin_index);
							}
							else if constexpr (shader_data<NodeDataT>) {
								std::get_if<NodeDataT>(&node.data)->update_ubo(pin_value, pin_index);
							}
						}
					});
				}
			});

			ed::SetNodePosition(nodes[node_index].id, ImVec2{ json_node["pos"][0], json_node["pos"][1] });
		}

		for (auto& json_link : json_file["links"]) {
			auto const start_node_index = json_link["start_node_index"].get<int>();
			auto const start_pin_index = json_link["start_pin_index"].get<int>();
			auto const end_node_index = json_link["end_node_index"].get<int>();
			auto const end_pin_index = json_link["end_pin_index"].get<int>();

			auto& start_pin = nodes[start_node_index].outputs[start_pin_index];
			auto& end_pin = nodes[end_node_index].inputs[end_pin_index];
			start_pin.connected_pins.emplace(&end_pin);
			end_pin.connected_pins.emplace(&start_pin);
			links.emplace(ed::LinkId(get_next_id()), &start_pin, &end_pin);

			std::visit([&](auto&& end_node_data) {
				using EndNodeDataT = std::decay_t<decltype(end_node_data)>;
				if constexpr (image_data<EndNodeDataT>) {
					end_node_data->update_ubo(start_pin.default_value, end_pin_index);
				}
				if constexpr (shader_data<EndNodeDataT>) {
					end_node_data.update_ubo(start_pin.default_value, end_pin_index);
					std::visit([&](auto&& start_node_data) {
						using StartNodeT = std::decay_t<decltype(start_node_data)>;
						if constexpr (image_data<StartNodeT>) {
							field_at(engine->pbr_material_texture_set, end_pin_index, [&](auto pbr_texture_id) {
								auto input_format = start_node_data->texture->format;
								auto& pbr_texture = engine->texture_manager->textures[pbr_texture_id];
								if (pbr_texture == nullptr || input_format != pbr_texture->format) {
									pbr_texture = Texture::create_device_texture(engine,
										TEXTURE_WIDTH,
										TEXTURE_HEIGHT,
										input_format,
										VK_IMAGE_ASPECT_COLOR_BIT,
										VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
									pbr_texture->transition_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
									engine->update_image_descriptor(pbr_texture, pbr_texture_id);
									start_node_data->record_copy_image_cmd_buffers(end_pin_index);
								}
								});
						}
						}, nodes[start_node_index].data);
				}
				}, nodes[end_node_index].data);
		}

		update_all_nodes();

		ed::SetCurrentView(
			ImVec2{
				json_file["view"]["origin"][0].get<float>(),
				json_file["view"]["origin"][1].get<float>()
			},
			json_file["view"]["scale"].get<float>()
		);

		ed::SetCurrentEditor(nullptr);
	}

	void NodeEditor::recalculate_node(const size_t index) {
		std::visit([=](auto&& node_data) {
			using NodeDataT = std::decay_t<decltype(node_data)>;
			if constexpr (value_data<NodeDataT>) {
				using UboT = UboOf<NodeDataT>;
				using FieldTypes = FieldTypeList<UboT>;
				nodes[index].outputs[0].default_value = [&] <std::size_t... I> (std::index_sequence<I...>) {
					return NodeDataT::calculate(
						*std::get_if<FieldTypes::template at<I>>(&nodes[index].evaluate_input(I))...);
				}(std::make_index_sequence<UboT::Class::TotalFields>{});
			}
			}, nodes[index].data);
	}

	void NodeEditor::clear() {
		wait_node_execute_fences();
		color_pin_index.reset();
		color_ramp_pin_index.reset();
		enum_pin_index.reset();
		links.clear();
		nodes.clear();
		next_id = 1;
	}
};
