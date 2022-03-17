#pragma once

#include <unordered_set>
#include <concepts>
#include <functional>
#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui_node_editor.h>
#include <imgui_internal.h>
#include <imgui_impl_vulkan.h>

#include "all_node_headers.h"
#include "../util/class_field_type_list.h"

#if NDEBUG
#else
#include "../util/debug_type.h"
#endif

namespace ed = ax::NodeEditor;

//using namespace std::literals;
static std::string first_letter_to_upper(std::string_view str);


template <typename T>
concept std_variant = requires (T t) {
	[] <typename... Ts> (std::variant<Ts...>) {}(t);
};

template <template<typename ...> typename TypeList, typename ...Ts>
consteval auto to_data_type(TypeList<Ts...>)->TypeList<typename Ts::data_type...>;

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
struct PredImageBase : std::bool_constant<std::derived_from<T, NodeTypeImageBase>> {};

using ImageNodeTypeList = NodeTypeList::filtered_by<PredImageBase>;

//Cast node types to their data types
using NodeDataTypeList = decltype(to_data_type(std::declval<NodeTypeList>()));

using ImageNodeDataTypeList = decltype(to_data_type(std::declval<ImageNodeTypeList>()));

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
auto get_ubo(T)->ref_t<T>::UboType;

template <NonImageDataConcept T>
auto get_ubo(T)->T::UboType;

template <typename NodeDataT>
using UboOf = std::decay_t<decltype(get_ubo(std::declval<NodeDataT>()))>;

template <typename UboT>
using FieldTypeList = TypeList<>::from<decltype(class_field_to_tuple(std::declval<UboT>()))>::remove_ref;

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
	Pin(ed::PinId id, std::string name, const T&) :
		id(id), name(name), default_value(T()) {
	}

	inline bool operator==(const Pin& pin) const {
		return (this->id == pin.id);
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
			data = std::make_shared<ref_t<T::data_type>>(engine);
		}
		else {
			data = NodeDataVariant(std::in_place_type<T::data_type>);
		}
	}

	inline PinVariant& evaluate_input(uint32_t i) {
		auto& connect_pins = inputs[i].connected_pins;
		return (connect_pins.empty()) ? inputs[i].default_value : (*(inputs[i].connected_pins.begin()))->default_value;
	}
};

struct Link {
	ed::LinkId id;
	Pin* start_pin;
	Pin* end_pin;
	//ed::PinId start_pin_id;
	//ed::PinId end_pin_id;

	//ImColor Color;

	Link(ed::LinkId id, Pin* start_pin, Pin* end_pin) :
		id(id), start_pin(start_pin), end_pin(end_pin) {}

	bool operator==(const Link& link) const {
		return (this->id == link.id);
	}
};

namespace std {
	template <> struct hash<Link> {
		size_t operator()(const Link& link) const {
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
			nodes.emplace_back(get_next_id(), NodeType::name(), NodeType{}, engine);
			auto& node = nodes.back();

			using NodeDataType = NodeType::data_type;
			using UboT = UboOf<NodeDataType>;
			UboT ubo{};

			node.inputs.reserve(UboT::Class::TotalFields);
			for (size_t index = 0; index < UboT::Class::TotalFields; ++index) {
				UboT::Class::FieldAt(ubo, index, [&](auto& field, auto& value) {
					using PinType = std::decay_t<decltype(field)>::Type;
					node.inputs.emplace_back(get_next_id(), first_letter_to_upper(field.name), std::in_place_type<PinType>);
					auto& pin_value = node.inputs[index].default_value;
					
					if constexpr (std::same_as<PinType, ColorRampData>) {
						pin_value = std::move(ColorRampData(engine));
						
						vkWaitForFences(engine->device, 1, &fence, VK_TRUE, VULKAN_WAIT_TIMEOUT);
						VkSubmitInfo submitInfo{
							.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
							.commandBufferCount = 1,
							.pCommandBuffers = &(std::get<ColorRampData>(pin_value).ubo_value->command_buffer),
						};
						vkResetFences(engine->device, 1, &fence);
						vkQueueSubmit(engine->graphicsQueue, 1, &submitInfo, fence);
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
				node.outputs.emplace_back(get_next_id(), "Result", std::in_place_type<TextureIdData>);
				auto& node_data = std::get<NodeDataType>(node.data);
				node.outputs[0].default_value = TextureIdData{ .value = node_data->node_texture_id };
				if constexpr (!has_field_type_v<UboT, ColorRampData>) {
					node_data->uniform_buffer->copyFromHost(&ubo);
				}
			}
			else {
				NodeDataType::ResultType::Class::ForEachField([&](auto& field) {
					using PinType = std::decay_t<decltype(field)>::Type;
					node.outputs.emplace_back(get_next_id(), first_letter_to_upper(field.name), std::in_place_type<PinType>);
					});
			}

			uint32_t node_index = nodes.size() - 1;

			build_node(node_index);

			return node_index;
		}
		void create_fence();

	public:
		NodeEditor(VulkanEngine* engine);

		void* get_gui_display_texture_handle() {
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

		void topological_sort(uint32_t index, std::vector<char>& visited_nodes, std::vector<uint32_t>& sorted_nodes);

		void excute_graph(const std::vector<uint32_t>& sorted_nodes);

		inline size_t get_input_pin_index(const Node& node, const Pin& pin) {
			auto ubo_index = std::find(node.inputs.begin(), node.inputs.end(), pin);
			assert(("get_input_pin_index() error: vector access violation!", ubo_index != node.inputs.end()));
			return ubo_index - node.inputs.begin();
		}

		inline size_t get_output_pin_index(const Node& node, const Pin& pin) {
			auto ubo_index = std::find(node.outputs.begin(), node.outputs.end(), pin);
			assert(("get_output_pin_index() error: vector access violation!", ubo_index != node.outputs.end()));
			return ubo_index - node.outputs.begin();
		}
	};
};

