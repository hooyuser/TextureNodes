#pragma once

#include <unordered_set>
#include <concepts>
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
	decltype([] <template<typename ...> typename TypeList, typename ...Ts>
		(TypeList<Ts...>) consteval -> TypeList<typename Ts::data_type...> {}),
	T > ;

enum class PinInOut {
	INPUT,
	OUTPUT
};

////////////////////////////////////////////////////////////////////////////////////////////
//All Node Types
using NodeTypeList = TypeList<
	NodeUniformColor,
	NodePolygon,
	NodeTransform,
	NodeBlend,
	NodeColorRamp,
	NodeAdd
>;

//All Image Node Types
template <typename T>
struct ImageBasePredicate : std::bool_constant<std::derived_from<T, NodeTypeImageBase>> {};

using ImageNodeTypeList = NodeTypeList::filtered_by<ImageBasePredicate>;

//Cast node types to their data types
using NodeDataTypeList = to_data_type<NodeTypeList>;

using ImageNodeDataTypeList = to_data_type<ImageNodeTypeList>;

//Generate aliases for std::variant
using NodeVariant = NodeTypeList::cast_to<std::variant>;

using NodeDataVariant = NodeDataTypeList::cast_to<std::variant>;

//template <typename T>
//constexpr bool is_image_data() {
//	return [] <typename... Ts> (std::tuple<Ts...>) {
//		return any_of<T, Ts...>;
//	}(ImageNodeDataTypeTuple{});
//}

template <typename T>
static constexpr bool is_image_data = ImageNodeDataTypeList::has_type<T>;

template <typename T>
concept ImageDataConcept = is_image_data<T>;

template <typename T>
concept NonImageDataConcept = NodeDataTypeList::has_type<T> && !is_image_data<T>;

template <ImageDataConcept T>
auto get_ubo(T)->typename ref_t<T>::UboType;

template <NonImageDataConcept T>
auto get_ubo(T)->typename T::UboType;

template <typename NodeDataT>
using UboOf = std::decay_t<decltype(get_ubo(std::declval<NodeDataT>()))>;

template <typename UboT>
using FieldTypeList = typename TypeList<>::from<FieldTypeTuple<UboT>>::remove_ref;

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
		else {
			data = NodeDataVariant(std::in_place_type<typename T::data_type>);
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

//using NodePtr = std::unique_ptr<Node>;

namespace engine {
	class NodeEditor {
	private:
		VulkanEngine* engine;
		ed::EditorContext* context = nullptr;
		std::vector<Node> nodes;
		std::unordered_set<Link> links;
		VkFence fence;
		//VkFence fence;
		int next_id = 1;

		std::optional<size_t> color_pin_index; //implies whether colorpicker should be open
		std::optional<size_t> color_ramp_pin_index; //implies whether color ramp should be open
		std::optional<size_t> enum_pin_index;  //implies whether enum menu should be open

		ed::NodeId display_node_id = ed::NodeId::Invalid;
		void* gui_display_texture_handle = nullptr;
		//uint64_t semophore_counter = 0;

		VkSemaphoreWaitInfo preview_semaphore_wait_info{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
		};

		constexpr static inline uint32_t preview_image_size = 128;
		//constexpr static inline uint32_t max_bindless_node_2d_textures = 300;

		constexpr int get_next_id();

		void update_from(uint32_t node_index);

		void build_node(uint32_t node_index);

		template<typename NodeType>
		uint32_t create_node() {
			auto const node_index = nodes.size();
			nodes.emplace_back(get_next_id(), NodeType::name(), NodeType{}, engine);
			auto& node = nodes.back();

			using NodeDataType = typename NodeType::data_type;
			using UboT = UboOf<NodeDataType>;
			UboT ubo{};

			node.inputs.reserve(UboT::Class::TotalFields);
			for (size_t index = 0; index < UboT::Class::TotalFields; ++index) {
				UboT::Class::FieldAt(ubo, index, [&](auto& field, auto& value) {
					using PinType = typename std::decay_t<decltype(field)>::Type;
					node.inputs.emplace_back(get_next_id(), node_index, first_letter_to_upper(field.name), std::in_place_type<PinType>);
					auto& pin_value = node.inputs[index].default_value;

					if constexpr (std::same_as<PinType, ColorRampData>) {
						pin_value = std::move(ColorRampData(engine));

						vkWaitForFences(engine->device, 1, &fence, VK_TRUE, VULKAN_WAIT_TIMEOUT);
						const VkSubmitInfo submit_info{
							.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
							.commandBufferCount = 1,
							.pCommandBuffers = &(std::get<ColorRampData>(pin_value).ubo_value->command_buffer),
						};
						vkResetFences(engine->device, 1, &fence);
						vkQueueSubmit(engine->graphicsQueue, 1, &submit_info, fence);
						vkWaitForFences(engine->device, 1, &fence, VK_TRUE, VULKAN_WAIT_TIMEOUT);
					}
					else {
						pin_value = value;
					}

					if constexpr (ImageDataConcept<NodeDataType> && has_field_type_v<UboT, ColorRampData>) {
						std::get<NodeDataType>(node.data)->update_ubo(pin_value, index);
					}
					});
			}

			if constexpr (ImageDataConcept<NodeDataType>) {
				node.outputs.emplace_back(get_next_id(), node_index, "Result", std::in_place_type<TextureIdData>);
				auto& node_data = std::get<NodeDataType>(node.data);
				node.outputs[0].default_value = TextureIdData{ .value = node_data->node_texture_id };
				if constexpr (!has_field_type_v<UboT, ColorRampData>) {
					node_data->uniform_buffer->copy_from_host(&ubo);
				}
			}
			else {
				NodeDataType::ResultType::Class::ForEachField([&](auto& field) {
					using PinType = typename std::decay_t<decltype(field)>::Type;
					node.outputs.emplace_back(get_next_id(), node_index, first_letter_to_upper(field.name), std::in_place_type<PinType>);
					});
			}
			
			build_node(node_index);

			return node_index;
		}
		void create_fence();

		bool is_pin_connection_valid(const PinVariant& pin1, const PinVariant& pin2);

	public:
		NodeEditor(VulkanEngine* engine);

		void* get_gui_display_texture_handle() const noexcept {
			return gui_display_texture_handle;
		}

		void draw();

		void serialize(std::string_view file_path);

		void deserialize(std::string_view file_path);

		void clear();

		template<std::invocable<uint32_t> Func>
		void for_each_connected_image_node(Func&& func, uint32_t node_index) {  //node_index is the index of current node
			for (auto& pin : nodes[node_index].outputs) {
				for (auto connected_pin : pin.connected_pins) {
					std::visit([&](auto&& connected_node_data) {
						using NodeDataT = std::decay_t<decltype(connected_node_data)>;
						if constexpr (is_image_data<NodeDataT>) {
							std::invoke(FWD(func), (connected_pin->node_index));
						}
						}, nodes[connected_pin->node_index].data);
				}
			}
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

		static size_t get_input_pin_index(const Node& node, const Pin& pin) {
			auto const ubo_index = std::ranges::find(node.inputs, pin);
			assert(("get_input_pin_index() error: vector access violation!", ubo_index != node.inputs.end()));
			return ubo_index - node.inputs.begin();
		}

		static size_t get_output_pin_index(const Node& node, const Pin& pin) {
			auto const ubo_index = std::ranges::find(node.outputs, pin);
			assert(("get_output_pin_index() error: vector access violation!", ubo_index != node.outputs.end()));
			return ubo_index - node.outputs.begin();
		}
	};
};

