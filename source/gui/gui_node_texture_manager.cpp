#include "gui_node_texture_manager.h"
#include "../vk_engine.h"
#include <array>
#include <ranges>
#include <stdexcept>

TextureManager::TextureManager(VulkanEngine* engine, uint32_t max_textures) {
	
	create_texture_array_descriptor_set_layouts(engine);
	create_texture_array_descriptor_set(engine);

	for (auto i : std::views::iota(0u, max_textures)) {
		unused_id.emplace(i);
	}
}

void TextureManager::create_texture_array_descriptor_set_layouts(VulkanEngine* engine) {
	std::array node_descriptor_set_layout_bindings{
		VkDescriptorSetLayoutBinding{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = engine->max_bindless_textures,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
		},
	};

	constexpr auto binding_num = node_descriptor_set_layout_bindings.size();

	constexpr std::array<VkDescriptorBindingFlags, binding_num> descriptor_binding_flags{
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | 
		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
	};

	const VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_create_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount = descriptor_binding_flags.size(),
		.pBindingFlags = descriptor_binding_flags.data(),
	};

	const VkDescriptorSetLayoutCreateInfo node_descriptor_layout_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &binding_flags_create_info,
		.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
		.bindingCount = node_descriptor_set_layout_bindings.size(),
		.pBindings = node_descriptor_set_layout_bindings.data(),
	};

	if (vkCreateDescriptorSetLayout(engine->device, &node_descriptor_layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	engine->main_deletion_queue.push_function([=] {
		vkDestroyDescriptorSetLayout(engine->device, descriptor_set_layout, nullptr);
		});
}

void TextureManager::create_texture_array_descriptor_set(VulkanEngine* engine) {
	VkDescriptorSetVariableDescriptorCountAllocateInfo variabl_descriptor_count_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
		.descriptorSetCount = 1,
		.pDescriptorCounts = &engine->max_bindless_textures,
	};

	const VkDescriptorSetAllocateInfo descriptor_alloc_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = &variabl_descriptor_count_info,
		.descriptorPool = engine->dynamic_descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &descriptor_set_layout,
	};

	if (vkAllocateDescriptorSets(engine->device, &descriptor_alloc_info, &descriptor_set) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
}

//void TextureManager::create_image_array_descriptor_set_layouts(VulkanEngine* engine) {
//	std::array node_descriptor_set_layout_bindings{
//		VkDescriptorSetLayoutBinding{
//			.binding = 0,
//			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//			.descriptorCount = engine->max_bindless_textures,
//			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
//		},
//	};
//
//	constexpr auto binding_num = node_descriptor_set_layout_bindings.size();
//
//	constexpr std::array<VkDescriptorBindingFlags, binding_num> descriptor_binding_flags{
//		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
//		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | 
//		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
//	};
//
//	const VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_create_info{
//		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
//		.bindingCount = descriptor_binding_flags.size(),
//		.pBindingFlags = descriptor_binding_flags.data(),
//	};
//
//	const VkDescriptorSetLayoutCreateInfo node_descriptor_layout_info = {
//		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
//		.pNext = &binding_flags_create_info,
//		.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
//		.bindingCount = node_descriptor_set_layout_bindings.size(),
//		.pBindings = node_descriptor_set_layout_bindings.data(),
//	};
//
//	if (vkCreateDescriptorSetLayout(engine->device, &node_descriptor_layout_info, nullptr, &image_descriptor_set_layout) != VK_SUCCESS) {
//		throw std::runtime_error("failed to create descriptor set layout!");
//	}
//
//	engine->main_deletion_queue.push_function([device = engine->device, layout = image_descriptor_set_layout] {
//		vkDestroyDescriptorSetLayout(device, layout, nullptr);
//		});
//}
//
//void TextureManager::create_image_array_descriptor_set(VulkanEngine* engine) {
//	VkDescriptorSetVariableDescriptorCountAllocateInfo variabl_descriptor_count_info{
//		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
//		.descriptorSetCount = 1,
//		.pDescriptorCounts = &engine->max_bindless_textures,
//	};
//
//	const VkDescriptorSetAllocateInfo descriptor_alloc_info{
//		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
//		.pNext = &variabl_descriptor_count_info,
//		.descriptorPool = engine->dynamic_descriptor_pool,
//		.descriptorSetCount = 1,
//		.pSetLayouts = &image_descriptor_set_layout,
//	};
//
//	if (vkAllocateDescriptorSets(engine->device, &descriptor_alloc_info, &image_descriptor_set) != VK_SUCCESS) {
//		throw std::runtime_error("failed to allocate descriptor sets!");
//	}
//}