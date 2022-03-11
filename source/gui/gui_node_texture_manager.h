#pragma once
#include <unordered_set>
#include <vulkan/vulkan.h>

class VulkanEngine;


struct NodeTextureManager {
	std::unordered_set<uint32_t> used_id;
	std::unordered_set<uint32_t> unused_id;
	
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorSet descriptor_set;

	void create_node_descriptor_set_layouts(VulkanEngine* engine);

	void create_node_texture_descriptor_set(VulkanEngine* engine);

	NodeTextureManager(VulkanEngine* engine, uint32_t max_textures);

	inline const auto get_id() {
		auto id = *unused_id.begin();
		used_id.emplace(id);
		unused_id.erase(id);
		return id;
	}

	inline void delete_id(uint32_t id) {
		unused_id.emplace(id);
		used_id.erase(id);
	}
};