#include "gui_node_pin_data.h"
#include "../vk_engine.h"
#include "../vk_util.h"
#include "../vk_image.h"
#include "../vk_initializers.h"


constexpr uint32_t RAMP_TEXTURE_SIZE = 256;

ColorRampData::ColorRampData(VulkanEngine* engine) {
	ubo_value = std::make_unique<RampTexture>(engine);
	value = ubo_value->color_ramp_texture_id;
	ui_value = std::make_unique<ImGradient>(static_cast<float*>(ubo_value->staging_buffer.mapped_buffer));
}

RampTexture::RampTexture(VulkanEngine* engine) :
	engine(engine),
	staging_buffer(engine::Buffer(
		engine->vma_allocator,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		PreferredMemoryType::RAM_FOR_UPLOAD,
		RAMP_TEXTURE_SIZE * 4 * sizeof(float))) {

	const VkImageCreateInfo imageInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_1D,
		.format = VK_FORMAT_R32G32B32A32_SFLOAT,//VK_FORMAT_R8G8B8A8_UNORM, //VK_FORMAT_R32G32B32A32_SFLOAT,
		.extent = {
			.width = RAMP_TEXTURE_SIZE,
			.height = 1,
			.depth = 1,
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	if (vkCreateImage(engine->device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(engine->device, image, &memory_requirements);

	const VkMemoryAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = find_memory_type(engine->physical_device, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	if (vkAllocateMemory(engine->device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(engine->device, image, memory, 0);

	const VkImageViewCreateInfo viewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_1D,
		.format = VK_FORMAT_R32G32B32A32_SFLOAT,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	if (vkCreateImageView(engine->device, &viewInfo, nullptr, &image_view) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	const VkSamplerCreateInfo sampler_info = vkinit::sampler_create_info(
		engine->physical_device,
		VK_FILTER_LINEAR,
		1,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	if (vkCreateSampler(engine->device, &sampler_info, nullptr, &sampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}

	engine::immediate_submit(engine, [=](VkCommandBuffer cmd) {
		const VkImageMemoryBarrier2 image_memory_barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.srcAccessMask = VK_ACCESS_2_NONE,
			.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.dstAccessMask = VK_ACCESS_2_NONE,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		};

		const VkDependencyInfo dependency_info{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &image_memory_barrier,
		};

		vkCmdPipelineBarrier2(cmd, &dependency_info);
		});

	VkDescriptorImageInfo image_info{
		.sampler = sampler,
		.imageView = image_view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	color_ramp_texture_id = engine->texture_manager->get_id();

	const std::array descriptor_writes{
		VkWriteDescriptorSet {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = engine->texture_manager->descriptor_set,
			.dstBinding = 0,
			.dstArrayElement = color_ramp_texture_id,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &image_info,
		},
	};

	vkUpdateDescriptorSets(engine->device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

	create_command_buffer();
}

RampTexture::~RampTexture() {
	engine->texture_manager->delete_id(color_ramp_texture_id);
	vkDestroySampler(engine->device, sampler, nullptr);
	vkDestroyImageView(engine->device, image_view, nullptr);
	vkDestroyImage(engine->device, image, nullptr);
	vkFreeMemory(engine->device, memory, nullptr);
}

void RampTexture::create_command_buffer() {
	const VkCommandBufferAllocateInfo cmd_allocate_info = vkinit::commandBufferAllocateInfo(engine->graphic_command_pool, 1);

	if (vkAllocateCommandBuffers(engine->device, &cmd_allocate_info, &command_buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	const VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};

	if (vkBeginCommandBuffer(command_buffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	{
		const VkImageMemoryBarrier2 image_memory_barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
			.srcAccessMask = VK_ACCESS_2_NONE,
			.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
			.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		};

		const VkDependencyInfo dependency_info{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &image_memory_barrier,
		};

		vkCmdPipelineBarrier2(command_buffer, &dependency_info);
	}

	const VkBufferImageCopy region{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageOffset = { 0, 0, 0 },
		.imageExtent = {
			RAMP_TEXTURE_SIZE,
			1,
			1
		}
	};
	vkCmdCopyBufferToImage(command_buffer, staging_buffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	{
		const VkImageMemoryBarrier2 image_memory_barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
			.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_NONE,
			.dstAccessMask = VK_ACCESS_2_NONE,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		};

		const VkDependencyInfo dependency_info{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &image_memory_barrier,
		};

		vkCmdPipelineBarrier2(command_buffer, &dependency_info);
	}

	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}