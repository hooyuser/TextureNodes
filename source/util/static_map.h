#pragma once
#include <xutility>
#include <stdexcept>

template <typename Key, typename Value, std::size_t Size>
struct StaticMap {
	std::array<std::pair<Key, Value>, Size> data;

	[[nodiscard]] constexpr Value const& at(const Key& key) const {
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

	[[nodiscard]] constexpr auto index_of(const Key& key) const {
		auto iter = std::ranges::find_if(data, [&key](const auto& entry) {
			return entry.first == key;
			});
		return std::distance(data.begin(), iter);
	}

	[[nodiscard]] constexpr auto index_of(const Value& value) const {
		auto iter = std::ranges::find_if(data, [&value](const auto& entry) {
			return entry.second == value;
			});
		return std::distance(data.begin(), iter);
	}

	[[nodiscard]] constexpr auto& get_key(size_t index) const {
		return data[index].first;
	}

	[[nodiscard]] constexpr auto& get_value(size_t index) const {
		return data[index].second;
	}

	[[nodiscard]] consteval auto keys() const {
		return[=] <size_t ...I> (std::index_sequence<I...>) {
			return std::array{ data[I].first... };
		} (std::make_index_sequence<Size>());
	}

	[[nodiscard]] consteval auto values() const {
		return[=] <size_t ...I> (std::index_sequence<I...>) {
			return std::array{ data[I].second... };
		} (std::make_index_sequence<Size>());
	}
};

template<class Key, class Value, typename ...Ts> requires (std::same_as<std::pair<Key, Value>, Ts> && ...)
StaticMap(std::pair<Key, Value>, Ts...)->StaticMap<Key, Value, 1 + sizeof...(Ts)>;


