#include "vk_util.h"

#include <stdexcept>

uint32_t find_memory_type(
	const VkPhysicalDevice physical_device,
	const uint32_t type_filter,
	const VkMemoryPropertyFlags properties
) {
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
		if ((type_filter & (1 << i)) &&
			(memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void insert_image_memory_barrier(
	const VkCommandBuffer            command_buffer,
	const VkImage                    image,
	const VkPipelineStageFlags2      src_stage_mask,
	const VkAccessFlags2             src_access_mask,
	const VkPipelineStageFlags2      dst_stage_mask,
	const VkAccessFlags2             dst_access_mask,
	const VkImageLayout              old_layout,
	const VkImageLayout              new_layout,
	const uint32_t                   src_queue_family_index,
	const uint32_t                   dst_queue_family_index,
	const uint32_t                   level_count,
	const uint32_t                   layer_count
) {
	const VkImageMemoryBarrier2 image_memory_barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = src_stage_mask,
		.srcAccessMask = src_access_mask,
		.dstStageMask = dst_stage_mask,
		.dstAccessMask = dst_access_mask,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = src_queue_family_index,
		.dstQueueFamilyIndex = dst_queue_family_index,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = level_count,
			.baseArrayLayer = 0,
			.layerCount = layer_count,
		},
	};

	const VkDependencyInfo dependency_info{
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &image_memory_barrier,
	};

	vkCmdPipelineBarrier2(command_buffer, &dependency_info);
}