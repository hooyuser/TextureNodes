#pragma once
#include "vk_types.h"
#include "vk_engine.h"
#include "util/util.h"

class VulkanEngine;

uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties);

void immediate_submit(const VkDevice device, const VkCommandPool command_pool, const VkQueue queue, const VkFence fence, std::invocable<VkCommandBuffer> auto&& function) {
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
		::immediate_submit(engine->device, engine->graphic_command_pool, engine->graphics_queue, engine->immediate_submit_fence, FWD(function));
	}

	void immediate_submit(VulkanEngine* engine, VkQueue queue, VkCommandPool command_pool, std::invocable<VkCommandBuffer> auto&& function) {
		::immediate_submit(engine->device, command_pool, queue, engine->immediate_submit_fence, FWD(function));
	}
}

void insert_image_memory_barrier(
	VkCommandBuffer            command_buffer,
	VkImage                    image,
	VkPipelineStageFlags2      src_stage_mask,
	VkAccessFlags2             src_access_mask,
	VkPipelineStageFlags2      dst_stage_mask,
	VkAccessFlags2             dst_access_mask,
	VkImageLayout              old_layout,
	VkImageLayout              new_layout,
	uint32_t                   src_queue_family_index = VK_QUEUE_FAMILY_IGNORED,
	uint32_t                   dst_queue_family_index = VK_QUEUE_FAMILY_IGNORED,
	uint32_t                   level_count = 1,
	uint32_t                   layer_count = 1
);