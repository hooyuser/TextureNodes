#pragma once

#include <unordered_set>
#include <concepts>
#include <format>
#include <functional>
#define IMGUI_DEFINE_MATH_OPERATORS


#include <imgui_impl_vulkan.h>

#include "all_node_headers.h"
#include "gui_node_editor_ui.h"
#include "gui_pin.h"
#include "../util/class_field_type_list.h"
#include "../util/cpp_type.h"
#include "../util/hash_str.h"


static std::string first_letter_to_upper(std::string_view str);

template <typename T>
concept std_variant = requires (T t) {
	[] <typename... Ts> (std::variant<Ts...>) {}(t);
};

template <typename T>
using to_data_type = std::invoke_result_t<
	decltype([] <template<typename...> typename TypeList, typename... Ts>
		(TypeList<Ts...>) consteval -> TypeList<typename Ts::data_type...> {}),
	T > ;



////////////////////////////////////////////////////////////////////////////////////////////
//Node Menus
using NodeMenuGenerator = TypeList<
	NodeUniformColor,
	NodePolygon,
	NodeNoise,
	NodeVoronoi
>;

using NodeMenuFilter = TypeList<
	NodeTransform,
	NodeBlend,
	NodeColorRamp,
	NodeBlur,
	NodeSlopeBlur,
	NodeUdf,
	NodeNormal
>;

using NodeMenuNumerical = TypeList<
	NodeAdd
>;

using NodeMenuShader = TypeList<
	NodePbrShader
>;

using NodeMenu = TypeList<
	NodeMenuGenerator,
	NodeMenuFilter,
	NodeMenuNumerical,
	NodeMenuShader
>;

//All Node Types
using NodeTypeList = concat_t<
	NodeMenuGenerator,
	NodeMenuFilter,
	NodeMenuNumerical,
	NodeMenuShader
>;

inline static constexpr auto NODE_TYPE_NAMES = [] <typename ...NodeT>  (TypeList<NodeT...>) consteval {
	return std::array{
		cpp_type_name<NodeT>...
	};
}(NodeTypeList{});

inline static constexpr auto NODE_TYPE_HASH_VALUES = [&] <std::size_t... I> (std::index_sequence<I...>) consteval {
	return std::array{
		hash_str(NODE_TYPE_NAMES[I])...
	};
}(std::make_index_sequence<NodeTypeList::size>{});

template<typename T>
constexpr std::string_view type_alias() {
	using Type = std::decay_t<T>;
	if constexpr (std::same_as<NodeMenuGenerator, Type>) {
		return "Generator";
	}
	else if constexpr (std::same_as<NodeMenuFilter, Type>) {
		return "Filter";
	}
	else if constexpr (std::same_as<NodeMenuNumerical, Type>) {
		return "Numerical";
	}
	else if constexpr (std::same_as<NodeMenuShader, Type>) {
		return "Shader";
	}
	else {
		return "Unknown Type";
	}
}

template<typename T>
constexpr inline static auto menu_name = type_alias<T>();

//Cast node types to their data types
using NodeDataTypeList = to_data_type<NodeTypeList>;

template <typename T>
concept NodeDataConcept = NodeDataTypeList::has_type<T>;

//Generate aliases for std::variant
using NodeVariant = NodeTypeList::cast_to<std::variant>;
using NodeDataVariant = NodeDataTypeList::cast_to<std::variant>;

//Predicate to check whether a class `T` is derived from the class `Base`
template <typename Base>
constexpr static inline auto BaseClassPredicate = []<typename T> (T) consteval {
	return std::derived_from<T, Base>;
};

//Subset of Node Types
template <typename Base>
using NodeTypeSubList = NodeTypeList::filtered_by<BaseClassPredicate<Base>>;

//Subset of Node Data Types
template <typename Base>
using NodeDataTypeSubList = to_data_type<NodeTypeSubList<Base>>;

//Check node data of specific types 
template <typename T, typename Base>
concept is_node_data_of = NodeDataTypeSubList<Base>::template has_type<T>;

//All Image Node Types
template <typename T>
concept image_data = is_node_data_of<T, NodeTypeImageBase>;

//All Value Node Types
template <typename T>
concept value_data = is_node_data_of<T, NodeTypeValueBase>;

//All Shader Node Types
template <typename T>
concept shader_data = is_node_data_of<T, NodeTypeShaderBase>;

template <image_data T>
constexpr auto get_info(T)->typename ref_t<T>::InfoT;

template <typename T> requires value_data<T> || shader_data<T>
constexpr auto get_info(T)->typename T::InfoT;

template <typename NodeDataT>
using MetaInfo = std::decay_t<decltype(get_info(std::declval<NodeDataT>()))>;

template <typename T>
using FieldTypeList = typename to_type_list<FieldTypeTuple<T>>::remove_ref;

////////////////////////////////////////////////////////////////////////////////////////////

inline void to_json(json& j, const Pin& p) {
	j = p.default_value;
}

struct Node {
	ed::NodeId id;
	std::string name;
	std::vector<Pin> inputs;
	std::vector<Pin> outputs;
	bool display_panel = true;
	ImVec2 size = { 0, 0 };
	NodeDataVariant data;

	template<std::derived_from<NodeTypeBase> T>
	Node(int id, std::string name, const T&, VulkanEngine* engine) :
		id(id), name(name) {
		if constexpr (std::derived_from<T, NodeTypeImageBase>) {
			data = std::make_shared<ref_t<typename T::data_type>>(engine);
		}
		else if constexpr (std::derived_from<T, NodeTypeValueBase>) {
			data = NodeDataVariant(std::in_place_type<typename T::data_type>);
		}
		else if constexpr (std::derived_from<T, NodeTypeShaderBase>) {
			data = NodeDataVariant(typename T::data_type(engine));
		}
	}

	PinVariant& evaluate_input(const uint32_t i) {
		auto const& connect_pins = inputs[i].connected_pins;
		return connect_pins.empty() ? inputs[i].default_value : inputs[i].connected_pin()->default_value;
	}
};

struct Link {
	ed::LinkId id;
	Pin* start_pin;
	Pin* end_pin;
	//ed::PinId start_pin_id;
	//ed::PinId end_pin_id;

	//ImColor Color;

	Link(const ed::LinkId id, Pin* start_pin, Pin* end_pin) :
		id(id), start_pin(start_pin), end_pin(end_pin) {}

	bool operator==(const Link& link) const noexcept {
		return this->id == link.id;
	}
};

namespace std {
	template<> struct hash<Link> {
		size_t operator()(const Link& link) const noexcept {
			return reinterpret_cast<size_t>(link.id.AsPointer());
		}
	};
}

struct CopyImageSubmitInfo {
	VkSemaphoreSubmitInfo wait_semaphore_submit_info;
	VkCommandBufferSubmitInfo cmd_buffer_submit_info;
};

struct SetNodePositionTag {};

template <typename T> 
struct GarbagePingPongBuffer {
	std::unique_ptr<std::vector<T>> garbage_in;
	std::unique_ptr<std::vector<T>> garbage_out;

	GarbagePingPongBuffer() {
		garbage_in = std::make_unique_for_overwrite<std::vector<T>>();
		garbage_out = std::make_unique_for_overwrite<std::vector<T>>();
	}

	template <typename ...Args> 
	constexpr void emplace_back(Args&&... args) {
		garbage_in->emplace_back(FWD(args)...);
	}

	void clear() {
		garbage_out->clear();
		std::swap(garbage_in, garbage_out);
	}

	void clear_all() {
		garbage_out->clear();
		garbage_in->clear();
	}
};

namespace engine {
	class NodeEditor {
	private:
		VulkanEngine* engine;
		ed::EditorContext* context = nullptr;
		std::vector<Node> nodes;
		std::unordered_set<Link> links;

		const uint32_t node_width;
		const float node_left_padding;
		const float node_right_padding;
		NodeEditorUIManager ui;
		
		VkFence graphic_fence;
		VkFence compute_fence;
		uint32_t next_id = 1;

		uint32_t internal_clock = 1;

		GarbagePingPongBuffer<NodeDataVariant> garbage_nodes = GarbagePingPongBuffer<NodeDataVariant>();
		constexpr inline static uint32_t garbage_collection_delay = 256;

		std::optional<size_t> color_pin_index; //implies whether colorpicker should be open
		std::optional<size_t> color_ramp_pin_index; //implies whether color ramp should be open
		std::optional<size_t> enum_pin_index;  //implies whether enum menu should be open

		ed::NodeId display_node_id = ed::NodeId::Invalid;
		void* gui_display_texture_handle = nullptr;

		VkSemaphoreWaitInfo preview_semaphore_wait_info{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
		};

		float preview_image_size;

		

		uint64_t get_next_id() noexcept;

		void update_from(uint32_t node_index);

		void build_node(uint32_t node_index);

		template<typename NodeType>
		uint32_t create_node() {
			auto const node_index = nodes.size();
			nodes.emplace_back(get_next_id(), NodeType::name(), NodeType{}, engine);
			auto& node = nodes.back();

			using NodeDataType = typename NodeType::data_type;
			using InfoT = typename NodeType::Info;
			InfoT ubo{};

			node.inputs.reserve(InfoT::Class::TotalFields);
			for (size_t index = 0; index < InfoT::Class::TotalFields; ++index) {
				InfoT::Class::FieldAt(ubo, index, [&](auto& field, auto& value) {
					using PinType = typename std::decay_t<decltype(field)>::Type;
					node.inputs.emplace_back(get_next_id(), node_index, first_letter_to_upper(field.name), std::in_place_type<PinType>);
					auto& pin_value = node.inputs[index].default_value;

					if constexpr (std::same_as<PinType, ColorRampData>) {
						pin_value = std::move(ColorRampData(engine));

						vkWaitForFences(engine->device, 1, &graphic_fence, VK_TRUE, VULKAN_WAIT_TIMEOUT);
						const VkSubmitInfo submit_info{
							.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
							.commandBufferCount = 1,
							.pCommandBuffers = &std::get_if<ColorRampData>(&pin_value)->ubo_value->command_buffer,
						};
						vkResetFences(engine->device, 1, &graphic_fence);
						vkQueueSubmit(engine->graphics_queue, 1, &submit_info, graphic_fence);
						vkWaitForFences(engine->device, 1, &graphic_fence, VK_TRUE, VULKAN_WAIT_TIMEOUT);
					}
					else {
						pin_value = value;
					}

					if constexpr (image_data<NodeDataType> && has_field_type_v<InfoT, ColorRampData>) {
						(*std::get_if<NodeDataType>(&node.data))->update_ubo(pin_value, index);
					}
					});
			}

			if constexpr (image_data<NodeDataType>) {
				node.outputs.emplace_back(get_next_id(), node_index, "Result", std::in_place_type<TextureIdData>);
				auto& node_data = *std::get_if<NodeDataType>(&node.data);
				node.outputs[0].default_value = TextureIdData{ .value = node_data->node_texture_id };
				if constexpr (!has_field_type_v<InfoT, ColorRampData>) {
					node_data->uniform_buffer->copy_from_host(&ubo);
				}
			}
			else if constexpr (value_data<NodeDataType>) {
				NodeDataType::ResultType::Class::ForEachField([&](auto& field) {
					using PinType = typename std::decay_t<decltype(field)>::Type;
					node.outputs.emplace_back(get_next_id(), node_index, first_letter_to_upper(field.name), std::in_place_type<PinType>);
					});
			}

			build_node(node_index);

			return node_index;
		}
		void create_fence();

		static bool is_pin_connection_valid(const PinVariant& pin1, const PinVariant& pin2);

	public:
		explicit NodeEditor(VulkanEngine* engine);

		void* get_gui_display_texture_handle() const noexcept {
			return gui_display_texture_handle;
		}

		void draw();

		void create_new_link();

		void delete_node_or_link();

		void garbage_collection();

		template<typename NodeTypeList, typename SetPos = void>
		void node_menu() {
			NodeTypeList::for_each([&]<typename T> () {
				if constexpr (is_type_list<T>) {
					if (ImGui::BeginMenu(std::format(" {}", menu_name<T>).c_str())) {
						node_menu<T, SetPos>();
						ImGui::EndMenu();
					}
				}
				else {
					if (ImGui::MenuItem(std::format(" {}", T::name()).c_str())) {
						auto node_i = create_node<T>();
						update_from(node_i);
						if constexpr (std::same_as<SetPos, SetNodePositionTag>) {
							ed::SetNodePosition(nodes[node_i].id, ed::ScreenToCanvas(ImGui::GetMousePos()));
						}
					}
				}
			});
		}

		void serialize(std::string_view file_path);

		void deserialize(std::string_view file_path);

		void clear();

		static bool hold_image_data(const NodeDataVariant& node_data) {
			return std::visit([&](const NodeDataVariant& data) {
				using NodeDataT = std::decay_t<decltype(data)>;
				if constexpr (image_data<NodeDataT>) {
					return true;
				}
				else {
					return false;
				}
				}, node_data);
		}

		template<std::invocable<uint32_t> Func>
		void for_each_connected_image_node(Func&& func, uint32_t node_index) {  //node_index is the index of current node
			for (auto& pin : nodes[node_index].outputs) {
				for (const Pin* connected_pin : pin.connected_pins) {
					if (hold_image_data(nodes[connected_pin->node_index].data)) {
						std::invoke(FWD(func), connected_pin->node_index);
					}
				}
			}
		}

		static void update_node_ubo(NodeDataVariant& node_data, const PinVariant& value, size_t index);

		template<NodeDataConcept NodeDataT>
		void update_wait_semaphores(const uint32_t i, const NodeDataT& node_data, std::vector<CopyImageSubmitInfo>& copy_image_submit_infos, uint64_t last_signal_counter) {
			for (auto& pin : nodes[i].outputs) {
				for (auto const connected_pin : pin.connected_pins) {
					std::visit([&](auto&& connected_node_data) {
						using NodeDataT = std::decay_t<decltype(connected_node_data)>;
						if constexpr (image_data<NodeDataT>) {
							connected_node_data->wait_semaphore_submit_info_0.emplace_back(
								VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
								nullptr,
								node_data->semaphore,
								last_signal_counter,
								VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
						}
						else if constexpr (shader_data<NodeDataT>) {
							copy_image_submit_infos.emplace_back(
								VkSemaphoreSubmitInfo{
									.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
									.pNext = nullptr,
									.semaphore = node_data->semaphore,
									.value = last_signal_counter,
									.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
								},
								VkCommandBufferSubmitInfo{
									.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
									.commandBuffer = node_data->copy_image_cmd_buffers[get_input_pin_index(*connected_pin)],
								}
							);
						}
						}, nodes[connected_pin->node_index].data);
				}
			}
			node_data->submit_info[0].waitSemaphoreInfoCount = static_cast<uint32_t>(node_data->wait_semaphore_submit_info_0.size());
			node_data->submit_info[0].pWaitSemaphoreInfos = (node_data->submit_info[0].waitSemaphoreInfoCount > 0) ? node_data->wait_semaphore_submit_info_0.data() : nullptr;
		}

		void recalculate_node(size_t index);

		void update_all_nodes();

		void topological_sort(uint32_t index, std::vector<char>& visited_nodes, std::vector<uint32_t>& sorted_nodes) const;

		void execute_graph(const std::vector<uint32_t>& sorted_nodes);

		size_t get_input_pin_index(const Pin& pin) const {
			const Node& node = nodes[pin.node_index];
			auto const ubo_index = std::ranges::find(node.inputs, pin);
			assert(("get_input_pin_index() error: vector access violation!", ubo_index != node.inputs.end()));
			return ubo_index - node.inputs.begin();
		}

		size_t get_output_pin_index(const Pin& pin) const {
			const Node& node = nodes[pin.node_index];
			auto const ubo_index = std::ranges::find(node.outputs, pin);
			assert(("get_output_pin_index() error: vector access violation!", ubo_index != node.outputs.end()));
			return ubo_index - node.outputs.begin();
		}

		void wait_node_execute_fences() const {
			const std::array fences{ graphic_fence, compute_fence };
			vkWaitForFences(engine->device, fences.size(), fences.data(), VK_TRUE, VULKAN_WAIT_TIMEOUT);
		}
	};
};

