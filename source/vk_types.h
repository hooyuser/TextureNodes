#pragma once
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <optional>

#include "util/flag_bit_enum.h"
#include "util/class_field_counter.h"

inline constexpr uint64_t VULKAN_WAIT_TIMEOUT = 3000000000;

template <class R, class T>
concept Matrix = std::convertible_to<std::ranges::range_reference_t<std::ranges::range_reference_t<R>>, T>;

typedef enum CreateResourceFlagBits {
	TEMP_BIT = 0x00000000,
	SWAPCHAIN_INDEPENDENT_BIT = 0x00000001,
	SWAPCHAIN_DEPENDENT_BIT = 0x00000003,  //resource should be recreated if the swapchain is recreated
} CreateResourceFlagBits;

//typedef enum TextureSetFlagBits {  //deprecated
//None = 0x00000000,
//	BASE_COLOR = 0x00000001,
//} TextureSetFlagBits;
//MAKE_ENUM_FLAGS(TextureSetFlagBits)

typedef enum ShaderFlagBits {
	FLAT = 0x00000000,
	PBR = 0x00000001,
} ShaderFlagBits;
MAKE_ENUM_FLAGS(ShaderFlagBits)

namespace engine {
	struct Texture;
}

using TexturePtr = std::shared_ptr<engine::Texture>;
struct PbrMaterialTextureSet {
	uint32_t base_color_id;
	uint32_t metalness_id;
	uint32_t roughness_id;
	uint32_t normal_id;
};
constexpr static inline auto PbrMaterialTextureNum = count_member_v<PbrMaterialTextureSet>;

struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	bool is_complete() const {
		return graphics_family.has_value() && present_family.has_value();
	}
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec3 pos;
};

struct AllocatedImage {
	VkImage _image;
	VkDeviceMemory _imageMemory;
};


