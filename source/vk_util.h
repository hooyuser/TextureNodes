#pragma once
#include "vk_types.h"
#include "vk_engine.h"

#include <functional>

class VulkanEngine;

uint32_t find_memory_type(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void immediate_submit(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkFence fence, std::invocable<VkCommandBuffer> auto&& function) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	std::invoke(FWD(function), commandBuffer);
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkResetFences(device, 1, &fence);
	vkQueueSubmit(queue, 1, &submitInfo, fence);
	vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

namespace engine {
	void immediate_submit(VulkanEngine* engine, std::invocable<VkCommandBuffer> auto&& function) {
		::immediate_submit(engine->device, engine->commandPool, engine->graphicsQueue, engine->immediate_submit_fence, FWD(function));
	}
}