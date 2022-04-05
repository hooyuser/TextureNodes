#pragma once
#include <unordered_set>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "../vk_image.h"

class VulkanEngine;

struct TextureManager {
	std::unordered_map<uint32_t, TexturePtr> textures;
	std::unordered_set<uint32_t> unused_id;

	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorSet descriptor_set;

	void create_texture_array_descriptor_set_layouts(VulkanEngine* engine);

	void create_texture_array_descriptor_set(VulkanEngine* engine);

	TextureManager(VulkanEngine* engine, uint32_t max_textures);

	auto get_id() {
		auto id = *unused_id.begin();
		textures.emplace(id, nullptr);
		unused_id.erase(id);
		return id;
	}

	auto add_texture(const TexturePtr& texture) {
		auto id = *unused_id.begin();
		textures.emplace(id, texture);
		unused_id.erase(id);
		return id;
	}

	auto add_texture(TexturePtr&& texture) noexcept {
		auto id = *unused_id.begin();
		textures.emplace(id, std::move(texture));
		unused_id.erase(id);
		return id;
	}

	void delete_id(uint32_t id) {
		unused_id.emplace(id);
		textures.erase(id);
	}
};