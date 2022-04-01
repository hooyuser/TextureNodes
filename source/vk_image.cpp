#include "vk_image.h"
#include "vk_util.h"
#include "vk_buffer.h"

#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#define TINYEXR_USE_THREAD 1
#define TINYEXR_USE_OPENMP 1
#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

namespace vk_init {
	VkSamplerCreateInfo samplerCreateInfo(VkPhysicalDevice physicalDevice, VkFilter filters, uint32_t mipLevels,
		VkSamplerAddressMode samplerAdressMode /*= VK_SAMPLER_ADDRESS_MODE_REPEAT*/) {

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		return VkSamplerCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.magFilter = filters,
			.minFilter = filters,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = samplerAdressMode,
			.addressModeV = samplerAdressMode,
			.addressModeW = samplerAdressMode,
			.mipLodBias = 0.0f,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.minLod = 0.0f,
			.maxLod = static_cast<float>(mipLevels),
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.unnormalizedCoordinates = VK_FALSE,
		};
	}
}


namespace engine {
	Image::Image(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels,
		VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		VkImageAspectFlags aspectFlags, VkComponentMapping components) : device(device), width(width), height(height), format(format), mipLevels(mipLevels) {

		VkImageCreateInfo imageInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = format,
			.extent = {
				.width = width,
				.height = height,
				.depth = 1,
			},
			.mipLevels = mipLevels,
			.arrayLayers = 1,
			.samples = numSamples,
			.tiling = tiling,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = find_memory_type(physicalDevice, memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(device, image, memory, 0);

		const VkImageViewCreateInfo view_info{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.components = components,
			.subresourceRange={
				.aspectMask = aspectFlags,
				.baseMipLevel = 0,
				.levelCount = mipLevels,
				.baseArrayLayer = 0,
				.layerCount = 1,
			}
		};
	
		if (vkCreateImageView(device, &view_info, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
	}

	Image::Image(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels,
		VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		VkImageAspectFlags aspectFlags, VkMemoryPropertyFlags imageFlag, uint32_t layerCount) : device(device), width(width), height(height),
		format(format), mipLevels(mipLevels), layerCount(layerCount) {

		const VkImageCreateInfo image_info{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags = imageFlag,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = format,
			.extent = {
				.width = width,
				.height = width,
				.depth = 1,
			},
			.mipLevels = mipLevels,
			.arrayLayers = layerCount,
			.samples = numSamples,
			.tiling = tiling,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
	

		if (vkCreateImage(device, &image_info, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = find_memory_type(physicalDevice, memRequirements.memoryTypeBits, properties),
		};

		if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(device, image, memory, 0);

		VkImageViewCreateInfo viewInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_CUBE,
			.format = format,
			.subresourceRange = {
				.aspectMask = aspectFlags,
				.baseMipLevel = 0,
				.levelCount = mipLevels,
				.baseArrayLayer = 0,
				.layerCount = layerCount,

			},
		};

		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
	}

	Image::~Image() {
		if (imageView != VK_NULL_HANDLE) {
			vkDestroyImageView(device, imageView, nullptr);
			imageView = VK_NULL_HANDLE;
		}
		if (image != VK_NULL_HANDLE) {
			vkDestroyImage(device, image, nullptr);
			image = VK_NULL_HANDLE;
		}
		if (memory != VK_NULL_HANDLE) {
			vkFreeMemory(device, memory, nullptr);
			memory = VK_NULL_HANDLE;
		}
	}

	ImagePtr Image::createImage(VulkanEngine* engine, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags, CreateResourceFlagBits imageDescription) {
		auto pImage = std::make_shared<Image>(engine->device, engine->physical_device, width, height, mipLevels, numSamples, format, tiling, usage, properties, aspectFlags);
		if (imageDescription & 0x00000001) {
			((imageDescription == SWAPCHAIN_DEPENDENT_BIT) ? engine->swap_chain_deletion_queue : engine->main_deletion_queue).push_function([=]() {
				vkDestroyImageView(engine->device, pImage->imageView, nullptr);
				vkDestroyImage(engine->device, pImage->image, nullptr);
				vkFreeMemory(engine->device, pImage->memory, nullptr);
				pImage->image = VK_NULL_HANDLE;
				pImage->imageView = VK_NULL_HANDLE;
				pImage->memory = VK_NULL_HANDLE;
				});
		}
		return pImage;
	}

	void Image::transitionImageLayout(VulkanEngine* engine, VkImageLayout oldLayout, VkImageLayout newLayout) {
		immediate_submit(engine, [&](VkCommandBuffer commandBuffer) {
			VkImageMemoryBarrier barrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.oldLayout = oldLayout,
				.newLayout = newLayout,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = mipLevels,
					.baseArrayLayer = 0,
					.layerCount = layerCount,
				}
			};

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = 0;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			}
			else {
				throw std::invalid_argument("unsupported layout transition!");
			}

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
			});
	}

	void Image::copyFromBuffer(VulkanEngine* engine, VkBuffer buffer, uint32_t mipLevel) {
		immediate_submit(engine, [=](VkCommandBuffer commandBuffer) {
			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = mipLevel;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = layerCount;
			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = {
				width >> mipLevel,
				height >> mipLevel,
				1
			};

			vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			});
	}

	void Image::generateMipmaps(VulkanEngine* engine) const{
		// Check if image format supports linear blitting
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(engine->physical_device, format, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			throw std::runtime_error("texture image format does not support linear blitting!");
		}

		immediate_submit(engine, [=](VkCommandBuffer commandBuffer) {

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = layerCount;
			barrier.subresourceRange.levelCount = 1;

			int32_t mipWidth = width;
			int32_t mipHeight = height;

			for (uint32_t i = 1; i < mipLevels; i++) {
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				vkCmdPipelineBarrier(commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				VkImageBlit blit{};
				blit.srcOffsets[0] = { 0, 0, 0 };
				blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = 0;
				blit.srcSubresource.layerCount = layerCount;
				blit.dstOffsets[0] = { 0, 0, 0 };
				blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = 0;
				blit.dstSubresource.layerCount = layerCount;

				vkCmdBlitImage(commandBuffer,
					image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &blit,
					VK_FILTER_LINEAR);

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;
			}

			barrier.subresourceRange.baseMipLevel = mipLevels - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
			});
	}

	Texture::Texture(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels,
		VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		VkImageAspectFlags aspectFlags, VkFilter filters, VkComponentMapping components) :
		Image(device, physicalDevice, width, height, mipLevels, numSamples, format, tiling, usage, properties, aspectFlags, components) {

		const VkSamplerCreateInfo samplerInfo = vk_init::samplerCreateInfo(physicalDevice, filters, mipLevels);

		if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	Texture::Texture(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels,
		VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		VkImageAspectFlags aspectFlags, VkFilter filters, VkImageCreateFlagBits imageFlag, uint32_t layerCount) :
		Image(device, physicalDevice, width, height, mipLevels, numSamples, format, tiling, usage, properties, aspectFlags, imageFlag, layerCount) {

		VkSamplerCreateInfo samplerInfo = vk_init::samplerCreateInfo(physicalDevice, filters, mipLevels);

		if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	Texture::~Texture() {
		if (sampler != VK_NULL_HANDLE) {
			vkDestroySampler(device, sampler, nullptr);
			sampler = VK_NULL_HANDLE;
		}
	}

	TexturePtr Texture::createTexture(VulkanEngine* engine, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags, VkFilter filters, CreateResourceFlagBits imageDescription) {
		auto pTexture = std::make_shared<Texture>(engine->device, engine->physical_device, width, height, mipLevels, numSamples, format, tiling, usage, properties, aspectFlags, filters);
		if (imageDescription & 0x00000001) {
			((imageDescription == SWAPCHAIN_DEPENDENT_BIT) ? engine->swap_chain_deletion_queue : engine->main_deletion_queue).push_function([=]() {
				vkDestroySampler(engine->device, pTexture->sampler, nullptr);
				vkDestroyImageView(engine->device, pTexture->imageView, nullptr);
				vkDestroyImage(engine->device, pTexture->image, nullptr);
				vkFreeMemory(engine->device, pTexture->memory, nullptr);
				pTexture->image = VK_NULL_HANDLE;
				pTexture->imageView = VK_NULL_HANDLE;
				pTexture->memory = VK_NULL_HANDLE;
				pTexture->sampler = VK_NULL_HANDLE;
				});
		}
		return pTexture;
	}

	TexturePtr Texture::create2DTexture(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, CreateResourceFlagBits imageDescription) {
		auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		auto pTexture = std::make_shared<Texture>(engine->device,
			engine->physical_device,
			width,
			height,
			mipLevels,
			VK_SAMPLE_COUNT_1_BIT,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_FILTER_LINEAR);
		if (imageDescription & 0x00000001) {
			((imageDescription == SWAPCHAIN_DEPENDENT_BIT) ? engine->swap_chain_deletion_queue : engine->main_deletion_queue).push_function([=]() {
				vkDestroySampler(engine->device, pTexture->sampler, nullptr);
				vkDestroyImageView(engine->device, pTexture->imageView, nullptr);
				vkDestroyImage(engine->device, pTexture->image, nullptr);
				vkFreeMemory(engine->device, pTexture->memory, nullptr);
				pTexture->image = VK_NULL_HANDLE;
				pTexture->imageView = VK_NULL_HANDLE;
				pTexture->memory = VK_NULL_HANDLE;
				pTexture->sampler = VK_NULL_HANDLE;
				});
		}
		return pTexture;
	}

	TexturePtr Texture::create2DTexture(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, CreateResourceFlagBits imageDescription, const uint32_t mipLevels) {
		auto pTexture = std::make_shared<Texture>(engine->device,
			engine->physical_device,
			width,
			height,
			mipLevels,
			VK_SAMPLE_COUNT_1_BIT,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_FILTER_LINEAR);
		if (imageDescription & 0x00000001) {
			((imageDescription == SWAPCHAIN_DEPENDENT_BIT) ? engine->swap_chain_deletion_queue : engine->main_deletion_queue).push_function([=]() {
				vkDestroySampler(engine->device, pTexture->sampler, nullptr);
				vkDestroyImageView(engine->device, pTexture->imageView, nullptr);
				vkDestroyImage(engine->device, pTexture->image, nullptr);
				vkFreeMemory(engine->device, pTexture->memory, nullptr);
				pTexture->image = VK_NULL_HANDLE;
				pTexture->imageView = VK_NULL_HANDLE;
				pTexture->memory = VK_NULL_HANDLE;
				pTexture->sampler = VK_NULL_HANDLE;
				});
		}
		return pTexture;
	}

	TexturePtr Texture::create_2D_render_target(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, VkImageAspectFlags aspectFlags, CreateResourceFlagBits imageDescription, VkSampleCountFlagBits sample_count_flag) {
		//auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		const VkImageUsageFlagBits usage_flag = (aspectFlags == VK_IMAGE_ASPECT_COLOR_BIT) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		auto pTexture = std::make_shared<Texture>(engine->device,
			engine->physical_device,
			width,
			height,
			1,
			sample_count_flag,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			usage_flag | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			aspectFlags,
			VK_FILTER_LINEAR);
		if (imageDescription & 0x00000001) {
			((imageDescription == SWAPCHAIN_DEPENDENT_BIT) ? engine->swap_chain_deletion_queue : engine->main_deletion_queue).push_function([=]() {
				vkDestroySampler(engine->device, pTexture->sampler, nullptr);
				vkDestroyImageView(engine->device, pTexture->imageView, nullptr);
				vkDestroyImage(engine->device, pTexture->image, nullptr);
				vkFreeMemory(engine->device, pTexture->memory, nullptr);
				pTexture->image = VK_NULL_HANDLE;
				pTexture->imageView = VK_NULL_HANDLE;
				pTexture->memory = VK_NULL_HANDLE;
				pTexture->sampler = VK_NULL_HANDLE;
				});
		}
		return pTexture;
	}

	TexturePtr Texture::create_device_texture(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, VkImageAspectFlags aspectFlags, VkImageUsageFlags usage_flag, CreateResourceFlagBits imageDescription, bool greyscale) {
		TexturePtr pTexture;
		if(greyscale) {
			pTexture = std::make_shared<Texture>(engine->device,
				engine->physical_device,
				width,
				height,
				1,
				VK_SAMPLE_COUNT_1_BIT,
				format,
				VK_IMAGE_TILING_OPTIMAL,
				usage_flag,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				aspectFlags,
				VK_FILTER_LINEAR,
				VkComponentMapping{
					VK_COMPONENT_SWIZZLE_R,
					VK_COMPONENT_SWIZZLE_R,
					VK_COMPONENT_SWIZZLE_R,
					VK_COMPONENT_SWIZZLE_ONE});
		}
		else {
			pTexture = std::make_shared<Texture>(engine->device,
				engine->physical_device,
				width,
				height,
				1,
				VK_SAMPLE_COUNT_1_BIT,
				format,
				VK_IMAGE_TILING_OPTIMAL,
				usage_flag,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				aspectFlags,
				VK_FILTER_LINEAR);
		}
		
		if (imageDescription & 0x00000001) {
			((imageDescription == SWAPCHAIN_DEPENDENT_BIT) ? engine->swap_chain_deletion_queue : engine->main_deletion_queue).push_function([=]() {
				vkDestroySampler(engine->device, pTexture->sampler, nullptr);
				vkDestroyImageView(engine->device, pTexture->imageView, nullptr);
				vkDestroyImage(engine->device, pTexture->image, nullptr);
				vkFreeMemory(engine->device, pTexture->memory, nullptr);
				pTexture->image = VK_NULL_HANDLE;
				pTexture->imageView = VK_NULL_HANDLE;
				pTexture->memory = VK_NULL_HANDLE;
				pTexture->sampler = VK_NULL_HANDLE;
				});
		}
		return pTexture;
	}

	TexturePtr Texture::create_cubemap_texture(VulkanEngine* engine, uint32_t width, VkFormat format, CreateResourceFlagBits imageDescription) {
		auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(width))) + 1;
		auto pTexture = std::make_shared<Texture>(engine->device,
			engine->physical_device,
			width,
			width,
			mipLevels,
			VK_SAMPLE_COUNT_1_BIT,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_FILTER_LINEAR,
			VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
			6);
		if (imageDescription & 0x00000001) {
			((imageDescription == SWAPCHAIN_DEPENDENT_BIT) ? engine->swap_chain_deletion_queue : engine->main_deletion_queue).push_function([=]() {
				vkDestroySampler(engine->device, pTexture->sampler, nullptr);
				vkDestroyImageView(engine->device, pTexture->imageView, nullptr);
				vkDestroyImage(engine->device, pTexture->image, nullptr);
				vkFreeMemory(engine->device, pTexture->memory, nullptr);
				pTexture->image = VK_NULL_HANDLE;
				pTexture->imageView = VK_NULL_HANDLE;
				pTexture->memory = VK_NULL_HANDLE;
				pTexture->sampler = VK_NULL_HANDLE;
				});
		}
		return pTexture;
	}

	TexturePtr Texture::create_cubemap_texture(VulkanEngine* engine, uint32_t width, VkFormat format, CreateResourceFlagBits imageDescription, const uint32_t mipLevels) {
		auto pTexture = std::make_shared<Texture>(engine->device,
			engine->physical_device,
			width,
			width,
			mipLevels,
			VK_SAMPLE_COUNT_1_BIT,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_FILTER_LINEAR,
			VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
			6);
		if (imageDescription & 0x00000001) {
			((imageDescription == SWAPCHAIN_DEPENDENT_BIT) ? engine->swap_chain_deletion_queue : engine->main_deletion_queue).push_function([=]() {
				vkDestroySampler(engine->device, pTexture->sampler, nullptr);
				vkDestroyImageView(engine->device, pTexture->imageView, nullptr);
				vkDestroyImage(engine->device, pTexture->image, nullptr);
				vkFreeMemory(engine->device, pTexture->memory, nullptr);
				pTexture->image = VK_NULL_HANDLE;
				pTexture->imageView = VK_NULL_HANDLE;
				pTexture->memory = VK_NULL_HANDLE;
				pTexture->sampler = VK_NULL_HANDLE;
				});
		}
		return pTexture;
	}

	TexturePtr Texture::load_2d_texture_from_host(VulkanEngine* engine, void const* host_pixels, int tex_width, int texHeight, int texChannels, bool enableMipmap/*= true*/, VkFormat format/*= VK_FORMAT_R8G8B8A8_SRGB*/) {

		VkDeviceSize imageSize = static_cast<uint64_t>(tex_width) * texHeight * texChannels;

		auto pStagingBuffer = engine::Buffer::create_buffer(engine,
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			TEMP_BIT);
		pStagingBuffer->copy_from_host(host_pixels);

		engine::TexturePtr pTexture;

		if (enableMipmap) {
			pTexture = engine::Texture::create2DTexture(engine, tex_width, texHeight, format, SWAPCHAIN_INDEPENDENT_BIT);
		}
		else {
			pTexture = engine::Texture::create2DTexture(engine, tex_width, texHeight, format, SWAPCHAIN_INDEPENDENT_BIT, 1);
		}

		pTexture->transitionImageLayout(engine, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		pTexture->copyFromBuffer(engine, pStagingBuffer->buffer);

		//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps


		pTexture->generateMipmaps(engine);


		return pTexture;
	}

	TexturePtr Texture::load_2d_texture(VulkanEngine* engine, const char* filePath, bool enableMipmap/*= true*/, VkFormat format/* = VK_FORMAT_R8G8B8A8_SRGB*/) {
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		//VkDeviceSize imageSize = static_cast<uint64_t>(texWidth) * texHeight * 4;
		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}
		return load_2d_texture_from_host(engine, pixels, texWidth, texHeight, texChannels, enableMipmap, format);
	}

	TexturePtr Texture::load_2d_texture(VulkanEngine* engine, const std::string& filePath, bool enableMipmap/*= true*/, VkFormat format/* = VK_FORMAT_R8G8B8A8_SRGB*/) {
		return load_2d_texture(engine, filePath.c_str(), enableMipmap, format);
	}

	TexturePtr Texture::load_cubemap_texture(VulkanEngine* engine, const std::vector<std::string>& filePaths) {
		int texWidth, texHeight, texChannels;
		float* pixels[6];
		for (int i = 0; i < 6; i++) {
			std::string extensionName = filePaths[i].substr(filePaths[i].find_last_of(".") + 1);
			if (extensionName == "hdr") { // hdr layout: r8g8b8e8(32-bit)
				pixels[i] = stbi_loadf(filePaths[i].c_str(), &texWidth, &texHeight, &texChannels, 4);
				if (!pixels[i]) {
					throw std::runtime_error("Error occurs when loading hdr!");
				}
			}
			else if (extensionName == "exr") {
				const char* err;
				int ret = LoadEXR(&pixels[i], &texWidth, &texHeight, filePaths[i].c_str(), &err); // LoadEXR returned layout: r32g32b32a32

				if (ret != TINYEXR_SUCCESS) {
					if (err) {
						printf("err: %s\n", err);
						FreeEXRErrorMessage(err);
						throw std::runtime_error("Error occurs when loading exr!");
					}
				}
			}
			else {
				throw std::runtime_error("Failed to load texture image! Unsupported image format.");
			}
		}

		const VkDeviceSize layerSize = static_cast<uint64_t>(texWidth) * texHeight * 4 * sizeof(float);
		VkDeviceSize imageSize = layerSize * 6;

		auto pStagingBuffer = engine::Buffer::create_buffer(engine,
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			TEMP_BIT);

		for (int i = 0; i < 6; i++) {
			memcpy(static_cast<char*>(pStagingBuffer->mapped_buffer) + (layerSize * i), pixels[i], static_cast<size_t>(layerSize));
			stbi_image_free(pixels[i]);
		}

		auto pTexture = engine::Texture::create_cubemap_texture(engine, texWidth, VK_FORMAT_R32G32B32A32_SFLOAT, SWAPCHAIN_INDEPENDENT_BIT);

		pTexture->transitionImageLayout(engine, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		pTexture->copyFromBuffer(engine, pStagingBuffer->buffer);

		//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

		pTexture->generateMipmaps(engine);

		return pTexture;
	}

	TexturePtr Texture::load_prefiltered_map_texture(VulkanEngine* engine, const std::vector<std::vector<std::string>>& filePathLayers) {

		auto pTexture = std::make_shared<engine::Texture>();

		for (uint32_t mipLevel = 0; mipLevel < filePathLayers.size(); mipLevel++) {
			auto& filePaths = filePathLayers[mipLevel];
			int texWidth, texHeight, texChannels;
			float* pixels[6];
			for (int i = 0; i < 6; i++) {
				std::string extensionName = filePaths[i].substr(filePaths[i].find_last_of(".") + 1);
				if (extensionName == "hdr") { // hdr layout: r8g8b8e8(32-bit)
					pixels[i] = stbi_loadf(filePaths[i].c_str(), &texWidth, &texHeight, &texChannels, 4);
					if (!pixels[i]) {
						throw std::runtime_error("Error occurs when loading hdr!");
					}
				}
				else if (extensionName == "exr") {
					const char* err;
					int ret = LoadEXR(&pixels[i], &texWidth, &texHeight, filePaths[i].c_str(), &err); // LoadEXR returned layout: r32g32b32a32

					if (ret != TINYEXR_SUCCESS) {
						if (err) {
							printf("err: %s\n", err);
							FreeEXRErrorMessage(err);
							throw std::runtime_error("Error occurs when loading exr!");
						}
					}
				}
				else {
					throw std::runtime_error("Failed to load texture image! Unsupported image format.");
				}
			}

			const VkDeviceSize layerSize = static_cast<uint64_t>(texWidth) * texHeight * 4 * sizeof(float);
			VkDeviceSize imageSize = layerSize * 6;

			auto pStagingBuffer = engine::Buffer::create_buffer(engine,
				imageSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				TEMP_BIT);

			for (int i = 0; i < 6; i++) {
				memcpy(static_cast<char*>(pStagingBuffer->mapped_buffer) + (layerSize * i), pixels[i], static_cast<size_t>(layerSize));
				stbi_image_free(pixels[i]);
			}

			if (mipLevel == 0) {
				pTexture = engine::Texture::create_cubemap_texture(engine, texWidth, VK_FORMAT_R32G32B32A32_SFLOAT, SWAPCHAIN_INDEPENDENT_BIT, filePathLayers.size());
			}

			pTexture->transitionImageLayout(engine, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			pTexture->copyFromBuffer(engine, pStagingBuffer->buffer, mipLevel);
		}

		immediate_submit(engine, [=](VkCommandBuffer commandBuffer) {
			//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = pTexture->image;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = pTexture->layerCount;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = filePathLayers.size();
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
			});
		return pTexture;
	}
}