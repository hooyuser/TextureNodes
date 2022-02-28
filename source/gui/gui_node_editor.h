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

using PinVariant = std::variant<
	TexturePtr,
	FloatData,
	IntData,
	BoolData,
	Color4Data,
	TextureIdData
>;

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

template <typename T>
consteval bool is_image_data() {
	using tuple_type = ImageNodeDataTypeTuple;
	return std::invoke([] <std::size_t... I> (std::index_sequence<I...>) {
		return any_of<T, std::tuple_element_t<I, tuple_type>...>;
	}, std::make_index_sequence<std::tuple_size<tuple_type>::value>{});
}

////////////////////////////////////////////////////////////////////////////////////////////

struct Node;
struct Pin {
	ed::PinId id;
	::Node* node = nullptr;
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

	ImRect display();
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
			using ref_type = std::decay_t<decltype(*std::declval<T::data_type>())>;
			data = std::make_shared<ref_type>(engine);
		}
		else {
			data = NodeDataVariant(std::in_place_type<T::data_type>);
		}
	}
};

struct Link {
	ed::LinkId id;

	ed::PinId start_pin_id;
	ed::PinId end_pin_id;

	//ImColor Color;

	Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId) :
		id(id), start_pin_id(startPinId), end_pin_id(endPinId) {}

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

namespace engine {
	class NodeEditor {
	private:
		VulkanEngine* engine;
		ed::EditorContext* context = nullptr;
		std::vector<Node> nodes;
		std::unordered_set<Link> links;
		int next_id = 1;
		constexpr static inline int image_size = 128;
		constexpr static inline uint32_t max_bindless_node_textures = 300;

		constexpr int get_next_id();

		void update_from(Node* node);

		void build_node(Node* node);

		template<typename NodeType>
		Node* create_node() {
			nodes.emplace_back(get_next_id(), NodeType::name(), NodeType{}, engine);
			auto& node = nodes.back();

			if constexpr (std::derived_from<NodeType, NodeTypeImageBase>) {
				auto& ubo = std::get<NodeType::data_type>(node.data)->ubo;
				std::decay_t<decltype(ubo)>::Class::ForEachField(ubo, [&](auto& field, auto& value) {
					using PinType = std::decay_t<decltype(value)>;
					node.inputs.emplace_back(get_next_id(), first_letter_to_upper(field.name), std::in_place_type<PinType>);
					//value = std::get<PinType>(node.inputs.back().default_value);
					});
				node.outputs.emplace_back(get_next_id(), "Result", std::in_place_type<TexturePtr>);
			}
			else if constexpr (std::same_as<NodeType, NodeAdd>) {
				node.inputs.emplace_back(get_next_id(), "Value", FloatData{});
				node.inputs.emplace_back(get_next_id(), "Value", FloatData{});
				node.outputs.emplace_back(get_next_id(), "Result", FloatData{});
			}

			build_node(&node);

			return &node;
		}

		void create_node_descriptor_pool();

		void create_node_descriptor_set_layouts();

		void create_node_texture_descriptor_set();

	public:
		NodeEditor(VulkanEngine* engine);

		void draw();
	};
};

