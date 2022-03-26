#pragma once
#include "vk_types.h"
#include "vk_engine.h"
#include "util/util.h"

class VulkanEngine;

uint32_t find_memory_type(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void immediate_submit(VkDevice device, VkCommandPool command_pool, VkQueue queue, VkFence fence, std::invocable<VkCommandBuffer> auto&& function) {
	const VkCommandBufferAllocateInfo alloc_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

	constexpr VkCommandBufferBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vkBeginCommandBuffer(command_buffer, &begin_info);
	std::invoke(FWD(function), command_buffer);
	vkEndCommandBuffer(command_buffer);

	const VkSubmitInfo submit_info{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffer,
	};

	vkResetFences(device, 1, &fence);
	vkQueueSubmit(queue, 1, &submit_info, fence);
	vkWaitForFences(device, 1, &fence, VK_TRUE, VULKAN_WAIT_TIMEOUT);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

namespace engine {
	void immediate_submit(VulkanEngine* engine, std::invocable<VkCommandBuffer> auto&& function) {
		::immediate_submit(engine->device, engine->command_pool, engine->graphics_queue, engine->immediate_submit_fence, FWD(function));
	}
}