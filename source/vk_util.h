#pragma once
#include "vk_types.h"
#include <functional>

class VulkanEngine;

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void immediateSubmit(VkDevice device, VkCommandPool commandPool, VkQueue queue, std::function<void(VkCommandBuffer commandBuffer)>&& function);

namespace engine {
	void immediateSubmit(VulkanEngine* engine, std::function<void(VkCommandBuffer commandBuffer)>&& function);
}