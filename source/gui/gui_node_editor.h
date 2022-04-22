#pragma once

#include <unordered_set>
#include <concepts>
#include <format>
#include <functional>
#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui_node_editor.h>
#include <imgui_impl_vulkan.h>

#include "all_node_headers.h"
#include "../util/class_field_type_list.h"

namespace ed = ax::NodeEditor;

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

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

enum class PinInOut {
	INPUT,
	OUTPUT
};

////////////////////////////////////////////////////////////////////////////////////////////
//All Node Types


//Node Menus
using NodeMenuGenerator = TypeList<
	NodePolygon,
	NodeNoise,
	NodeVoronoi
>;

using NodeMenuFilter = TypeList<
	NodeUniformColor,
	NodeTransform,
	NodeBlend,
	NodeColorRamp,
	NodeBlur,
	NodeSlopeBlur,
	NodeUdf
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

using NodeTypeList = concat_t<
	NodeMenuGenerator,
	NodeMenuFilter,
	NodeMenuNumerical,
	NodeMenuShader
>;

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
constexpr auto get_ubo(T)->typename ref_t<T>::UboType;

template <value_data T>
constexpr auto get_ubo(T)->typename T::UboType;

template <shader_data T>
constexpr auto get_ubo(T)->typename T::UboType;

template <typename NodeDataT>
using UboOf = std::decay_t<decltype(get_ubo(std::declval<NodeDataT>()))>;

template <typename UboT>
using FieldTypeList = typename to_type_list<FieldTypeTuple<UboT>>::remove_ref;

////////////////////////////////////////////////////////////////////////////////////////////

struct Node;
struct Pin {
	ed::PinId id;
	uint32_t node_index;
	std::unordered_set<Pin*> connected_pins;
	std::string name;
	PinInOut flow_direction = PinInOut::INPUT;
	PinVariant default_value;

	template<typename T> requires std::constructible_from<PinVariant, T>
	Pin(const ed::PinId id, const uint32_t node_index, std::string name, const T&) :
		id(id), node_index(node_index), name(name), default_value(T()) {
	}

	bool operator==(const Pin& pin) const noexcept {
		return (this->id == pin.id);
	}

	Pin* connected_pin() const noexcept {
		return *(connected_pins.begin());
	}
};

inline void to_json(json& j, const Pin& p) {
	j = p.default_value;
}

//enum class NodeState {
//	Normal = 0,
//	Updated = 1,
//	Processing = 2,
//	ADD_NODE = 3,
//};

struct Node {
	ed::NodeId id;
	std::string name;
	std::vector<Pin> inputs;
	std::vector<Pin> outputs;
	bool display_panel = true;
	//ImColor Color ;
	//std::string type_name;
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
	template <> struct hash<Link> {
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

namespace engine {
	class NodeEditor {
	private:
		VulkanEngine* engine;
		ed::EditorContext* context = nullptr;
		std::vector<Node> nodes;
		std::unordered_set<Link> links;
		VkFence graphic_fence;
		VkFence compute_fence;
		int next_id = 1;

		std::optional<size_t> color_pin_index; //implies whether colorpicker should be open
		std::optional<size_t> color_ramp_pin_index; //implies whether color ramp should be open
		std::optional<size_t> enum_pin_index;  //implies whether enum menu should be open

		ed::NodeId display_node_id = ed::NodeId::Invalid;
		void* gui_display_texture_handle = nullptr;

		VkSemaphoreWaitInfo preview_semaphore_wait_info{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
		};

		constexpr static uint32_t preview_image_size = 128;

		int get_next_id() noexcept;

		void update_from(uint32_t node_index);

		void build_node(uint32_t node_index);

		template<typename NodeType>
		uint32_t create_node() {
			auto const node_index = nodes.size();
			nodes.emplace_back(get_next_id(), NodeType::name(), NodeType{}, engine);
			auto& node = nodes.back();

			using NodeDataType = typename NodeType::data_type;
			using UboT = typename NodeType::UBO;
			UboT ubo{};

			node.inputs.reserve(UboT::Class::TotalFields);
			for (size_t index = 0; index < UboT::Class::TotalFields; ++index) {
				UboT::Class::FieldAt(ubo, index, [&](auto& field, auto& value) {
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

					if constexpr (image_data<NodeDataType> && has_field_type_v<UboT, ColorRampData>) {
						(*std::get_if<NodeDataType>(&node.data))->update_ubo(pin_value, index);
					}
					});
			}

			if constexpr (image_data<NodeDataType>) {
				node.outputs.emplace_back(get_next_id(), node_index, "Result", std::in_place_type<TextureIdData>);
				auto& node_data = *std::get_if<NodeDataType>(&node.data);
				node.outputs[0].default_value = TextureIdData{ .value = node_data->node_texture_id };
				if constexpr (!has_field_type_v<UboT, ColorRampData>) {
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
			const std::array fences{ graphic_fence,compute_fence };
			vkWaitForFences(engine->device, fences.size(), fences.data(), VK_TRUE, VULKAN_WAIT_TIMEOUT);
		}
	};
};

