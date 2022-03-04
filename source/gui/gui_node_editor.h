#pragma once

#include <unordered_set>
#include <concepts>
#include <functional>
#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui_node_editor.h>
#include <imgui_internal.h>
#include <imgui_impl_vulkan.h>

#include "../util/type_list.h"
#include "all_node_headers.h"

#if NDEBUG
#else
#include "../util/debug_type.h"
#endif

namespace ed = ax::NodeEditor;

//using namespace std::literals;
static std::string first_letter_to_upper(std::string_view str);

template<typename UnknownType, typename ...ReferenceTypes>
concept any_of = (std::same_as<std::decay_t<UnknownType>, ReferenceTypes> || ...);

template <template<typename ...> typename TypeList, typename ...Ts>
consteval auto to_data_type(TypeList<Ts...>)->TypeList<typename Ts::data_type...>;

enum class PinInOut {
	INPUT,
	OUTPUT
};

////////////////////////////////////////////////////////////////////////////////////////////

using NodeTypeList = TypeList<
	NodeUniformColor,
	NodePolygon,
	NodeTransform,
	NodeAdd
>;

using NodeVariant = NodeTypeList::cast_to<std::variant>;

using NodeDataVariant = decltype(make_unique_typeset(to_data_type(std::declval<NodeVariant>())));

template <typename T>
struct PredImageBase {
	static inline constexpr bool value = std::is_base_of<NodeTypeImageBase, T>::value;
};

using ImageNodeTypeTuple = filter_t<NodeTypeList::to_tuple, PredImageBase>;

using ImageNodeDataTypeTuple = decltype(make_unique_typeset(to_data_type(std::declval<ImageNodeTypeTuple>())));

//template <typename T>
//constexpr bool is_image_data() {
//	return [] <typename... Ts> (std::tuple<Ts...>) {
//		return any_of<T, Ts...>;
//	}(ImageNodeDataTypeTuple{});
//}


template <typename T, typename Tuple>
struct any_of_tuple {
	static constexpr bool value = std::invoke([] <std::size_t... I> (std::index_sequence<I...>) {
		return any_of<T, std::tuple_element_t<I, Tuple>...>;
	}, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
};
template <typename T, typename Tuple>
static constexpr bool any_of_tuple_v = any_of_tuple<T, Tuple>::value;

template <typename T>
static constexpr bool is_image_data = any_of_tuple_v<T, ImageNodeDataTypeTuple>;

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

	bool operator==(const Pin& pin) const {
		return (this->id == pin.id);
	}
};

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
	std::string type_name;
	ImVec2 size = { 0, 0 };
	NodeDataVariant data;

	template<std::derived_from<NodeTypeBase> T>
	Node(int id, std::string name, const T&, VulkanEngine* engine) :
		id(id), name(name), type_name(T::name()) {
		if constexpr (std::derived_from<T, NodeTypeImageBase>) {
			data = std::make_shared<ref_t<T::data_type>>(engine);
		}
		else {
			data = NodeDataVariant(std::in_place_type<T::data_type>);
		}
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
		int next_id = 1;
		//uint64_t semophore_counter = 0;

		std::vector<VkSubmitInfo2> submits;
		std::vector<char> visited_nodes;
		VkSemaphoreWaitInfo preview_semaphore_wait_info{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
		};
		
		constexpr static inline uint32_t preview_image_size = 128;
		constexpr static inline uint32_t max_bindless_node_textures = 300;

		constexpr int get_next_id();

		void update_from(uint32_t node_index);

		void build_node(uint32_t node_index);

		template<typename NodeType>
		uint32_t create_node() {
			nodes.emplace_back(get_next_id(), NodeType::name(), NodeType{}, engine);
			auto& node = nodes.back();

			if constexpr (std::derived_from<NodeType, NodeTypeImageBase>) {
				using NodeDataType = NodeType::data_type;
				using UboT = typename ref_t<NodeDataType>::UboType;
				UboT ubo{};
				for (size_t index = 0; index < UboT::Class::TotalFields; ++index) {
					UboT::Class::FieldAt(ubo, index, [&](auto& field, auto& value) {
						using PinType = std::decay_t<decltype(field)>::Type;
						node.inputs.reserve(UboT::Class::TotalFields);
						node.inputs.emplace_back(get_next_id(), first_letter_to_upper(field.name), std::in_place_type<PinType>);
						node.inputs[index].default_value = value;
						});
				}
				node.outputs.emplace_back(get_next_id(), "Result", std::in_place_type<TextureIdData>);
				auto& node_data = std::get<NodeDataType>(node.data);
				node.outputs[0].default_value = TextureIdData{ .value = node_data->node_texture_id };
				node_data->uniform_buffer->copyFromHost(&ubo);
			}
			else if constexpr (std::same_as<NodeType, NodeAdd>) {
				node.inputs.reserve(2);
				node.inputs.emplace_back(get_next_id(), "Value", FloatData{});
				node.inputs.emplace_back(get_next_id(), "Value", FloatData{});
				node.outputs.emplace_back(get_next_id(), "Result", FloatData{});
			}

			uint32_t node_index = nodes.size() - 1;

			build_node(node_index);

			return node_index;
		}
		void create_fence();

		void create_node_descriptor_pool();

		void create_node_descriptor_set_layouts();

		void create_node_texture_descriptor_set();

	public:
		NodeEditor(VulkanEngine* engine);

		void draw();

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
	};
};

