#pragma once

#include "gui_node_data.h"

using namespace std::literals::string_view_literals;

struct NodeTypeBase {};

struct NodeTypeImageBase : NodeTypeBase {};

struct NodeTypeValueBase : NodeTypeBase {};

struct NodeTypeShaderBase : NodeTypeBase {};

struct NumberInputWidgetInfo {
	float min;
	float max;
	float speed;
	bool enable_slider;
};

enum class AutoFormat {
	False = 0,
	True = 1
};

enum class FormatEnum {
	False = 0,
	True = 1
};

template <typename Key, typename Value, std::size_t Size>
struct StaticMap {
	std::array<std::pair<Key, Value>, Size> data;

	[[nodiscard]] constexpr Value at(const Key& key) const {
		const auto itr = std::ranges::find_if(data, [&key](const auto& entry) {
			return entry.first == key;
			});

		if (itr != end(data)) {
			return itr->second;
		}
		else {
			throw std::range_error("Not Found Key");
		}
	}

	[[nodiscard]] constexpr Key at(const Value& value) const {
		const auto itr = std::ranges::find_if(data, [&value](const auto& entry) {
			return entry.second == value;
			});
		if (itr != end(data)) {
			return itr->first;
		}
		else {
			throw std::range_error("Not Found Value");
		}
	}

	consteval auto keys() const {
		return[=] <size_t ...I> (std::index_sequence<I...>) {
			return std::array{ data[I].first... };
		} (std::make_index_sequence<Size>());
	}
};

template<class Key, class Value, typename ...Ts>
StaticMap(std::pair<Key, Value>, Ts...)->StaticMap<Key, Value, 1 + sizeof...(Ts)>;

static constexpr inline auto str_format_map = StaticMap{
	std::pair{"C8 SRGB", VK_FORMAT_R8G8B8A8_SRGB},
	std::pair{"R16 UNORM", VK_FORMAT_R16_UNORM}
};

static constexpr inline auto format_str_array = str_format_map.keys();