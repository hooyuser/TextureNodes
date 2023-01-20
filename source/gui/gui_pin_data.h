#pragma once

#include "../vk_buffer.h"
#include "../util/type_list.h"
#include "gui_node_texture_manager.h"
#include "imgui_color_gradient.h"
#include <json.hpp>
#include <variant>

using json = nlohmann::json;

class VulkanEngine;

template <typename T, typename FieldType>
struct count_field_type {
	static inline constexpr size_t value = [] {
		size_t field_count = 0;
		for (size_t i = 0; i < Reflect::class_t<T>::TotalFields; i++) {
			Reflect::class_t<T>::FieldAt(i, [&](auto& field) {
				using CurrentFieldType = typename std::remove_reference_t<decltype(field)>::Type;
				if constexpr (std::is_same_v<CurrentFieldType, FieldType>) {
					++field_count;
				}
				});
		}
		return field_count;
	}();
};

template <typename T, typename FieldType>
static constexpr size_t count_field_type_v = count_field_type<T, FieldType>::value;

template <typename T, typename FieldType>
static constexpr bool has_field_type_v = (count_field_type_v<T, FieldType>) > 0;

//Define all kinds of NodeData, which serves as UBO member type or subtype of PinVaraint 
struct PinData {};

struct IntData : PinData {
	using value_t = int32_t;
	value_t value = 0;
};

struct FloatData : PinData {
	using value_t = float;
	value_t value = 0.0f;
};

struct Float4Data : PinData {
	using value_t = float[4];
	value_t value = { 0.0f };
};

struct Color4Data : PinData {
	using value_t = float[4];
	value_t value = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct BoolData : PinData {
	using value_t = bool;
	value_t value;
};

struct EnumData : PinData {
	using value_t = uint32_t;
	value_t value;
};

struct TextureIdData : PinData {
	using value_t = int32_t;
	value_t value = -1;
};

struct FloatTextureIdData : PinData {
	using value_t = struct Value {
		float number = 0.0f;
		int32_t id = -1;
	};
	value_t value = Value{};
};

struct Color4TextureIdData : PinData {
	using value_t = struct Value {
		float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		int32_t id = -1;
	};
	value_t value = Value{};
};

struct RampTexture {
	VulkanEngine* engine;
	VkImage image;
	VkImageView image_view;
	VkSampler sampler;
	VkDeviceMemory memory;
	engine::Buffer staging_buffer;
	VkCommandBuffer command_buffer;
	uint32_t color_ramp_texture_id;

	RampTexture(VulkanEngine* engine);
	~RampTexture();
	void create_command_buffer();

};

struct ColorRampData : PinData {
	using value_t = int32_t;
	value_t value = -1;
	std::unique_ptr<ImGradient> ui_value;
	std::unique_ptr<RampTexture> ubo_value;
	ImGradientMark* dragging_mark = nullptr;
	ImGradientMark* selected_mark = nullptr;

	constexpr ColorRampData() {
		ui_value = nullptr;
		ubo_value = nullptr;
	}
	explicit ColorRampData(VulkanEngine* engine);

	//ColorRampData(const ColorRampData& ramp_data) =default;

	/*ColorRampData(ColorRampData&&ramp_data) noexcept :
		value(ramp_data.value),
		ui_value(std::move(ramp_data.ui_value)),
		ubo_value(std::move(ramp_data.ubo_value)),
		draggingMark(ramp_data.draggingMark),
		selectedMark(ramp_data.selectedMark)
	{}

	ColorRampData& operator=(ColorRampData&& ramp_data) noexcept {
		ui_value = std::move(ramp_data.ui_value);
		ubo_value = std::move(ramp_data.ubo_value);
		value = ramp_data.value;
		draggingMark = ramp_data.draggingMark;
		selectedMark = ramp_data.selectedMark;
		return *this;
	}
	ColorRampData& operator=(const ColorRampData& ramp_data) = default;*/

	//~ColorRampData() {}
};

inline void to_json(json& j, const ColorRampData& p) {
	j = *p.ui_value;
}

namespace nlohmann {
	template <typename T> requires (std::derived_from<T, PinData> && !std::same_as<T, ColorRampData>)
		struct adl_serializer<T> {
		static void to_json(json& j, const T& data) {
			j = data.value;
		}

		static void from_json(const json& j, T& data) {
			j.get_to(data.value);
		}
	};

	template <>
	struct adl_serializer<ColorRampData> {
		static void to_json(json& j, const ColorRampData& data) {
			j = *(data.ui_value);
		}
	};

	template <>
	struct adl_serializer<FloatTextureIdData> {
		static void to_json(json& j, const FloatTextureIdData& data) {
			j = data.value.number;
		}

		static void from_json(const json& j, FloatTextureIdData& data) {
			data = FloatTextureIdData{
				.value = {
					.number = j.get<float>()
				}
			};
		}
	};

	template <>
	struct adl_serializer<Color4TextureIdData> {
		static void to_json(json& j, const Color4TextureIdData& data) {
			j = data.value.color;
		}

		static void from_json(const json& j, Color4TextureIdData& data) {
			data = Color4TextureIdData{
				.value {
					.color {
						j[0].get<float>(),
						j[1].get<float>(),
						j[2].get<float>(),
						j[3].get<float>(),
					},
				}
			};
		}
	};

	template <typename ...Args>
	struct adl_serializer<std::variant<Args...>> {
		static void to_json(json& j, std::variant<Args...> const& var) {
			std::visit([&](auto&& value) {
				j = FWD(value);
				}, var);
		}
	};
}

using PinTypeList = TypeList<
	TextureIdData,
	FloatData,
	IntData,
	BoolData,
	Color4Data,
	EnumData,
	ColorRampData,
	FloatTextureIdData,
	Color4TextureIdData
>;

using PinVariant = PinTypeList::cast_to<std::variant>;

template <typename T>
concept PinDataConcept = PinTypeList::has_type<T>;

template <typename InfoT>
constexpr static inline bool has_texture_field = has_field_type_v<InfoT, ColorRampData> ||
has_field_type_v<InfoT, TextureIdData> ||
has_field_type_v<InfoT, Color4TextureIdData> ||
has_field_type_v<InfoT, FloatTextureIdData>;
