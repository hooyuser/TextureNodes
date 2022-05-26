#include "vk_image.h"
#include "vk_util.h"
#include "vk_buffer.h"

#include <stdexcept>
#include <filesystem>

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
	VkSamplerCreateInfo sampler_create_info(VkPhysicalDevice physicalDevice, VkFilter filters, uint32_t mipLevels,
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
	Image::Image(VulkanEngine* engine, uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits sample_count_flag,
		VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, PreferredMemoryType preferred_memory_type,
		VkImageAspectFlags aspect_flags, uint32_t layer_count, VkImageCreateFlags image_flag, VkComponentMapping components) :
		engine(engine), width(width), height(height), format(format), mip_levels(mip_levels), layer_count(layer_count) {

		const VkImageCreateInfo image_info{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags = image_flag,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = format,
			.extent = {
				.width = width,
				.height = height,
				.depth = 1,
			},
			.mipLevels = mip_levels,
			.arrayLayers = layer_count,
			.samples = sample_count_flag,
			.tiling = tiling,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		VmaAllocationInfo allocation_info;
		if (vmaCreateImage(
			engine->vma_allocator,
			&image_info,
			&memory_type_vma_alloc_map.at(preferred_memory_type),
			&image,
			&allocation,
			&allocation_info) != VK_SUCCESS
			) {
			throw std::runtime_error("vma failed to create image!");
		}

		const VkImageViewCreateInfo view_info{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = layer_count == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_CUBE,
			.format = format,
			.components = components,
			.subresourceRange = {
				.aspectMask = aspect_flags,
				.baseMipLevel = 0,
				.levelCount = mip_levels,
				.baseArrayLayer = 0,
				.layerCount = layer_count,
			}
		};

		if (vkCreateImageView(engine->device, &view_info, nullptr, &image_view) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
	}

	Image::~Image() {
		if (image_view != VK_NULL_HANDLE) {
			vkDestroyImageView(engine->device, image_view, nullptr);
			image_view = VK_NULL_HANDLE;
		}
		if (image != VK_NULL_HANDLE) {
			vmaDestroyImage(engine->vma_allocator, image, allocation);
			image = VK_NULL_HANDLE;
		}
	}

	/*ImagePtr Image::create_image(VulkanEngine* engine, uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits sample_count_flag,
		VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, PreferredMemoryType preferred_memory_type, VkImageAspectFlags aspect_flags,
		CreateResourceFlagBits image_description) {
		auto image = std::make_shared<Image>(
			engine,
			width,
			height,
			mip_levels,
			sample_count_flag,
			format,
			tiling,
			usage,
			preferred_memory_type,
			aspect_flags);

		if (image_description & 0x00000001) {
			((image_description == SWAPCHAIN_DEPENDENT_BIT) ? engine->swap_chain_deletion_queue : engine->main_deletion_queue).push_function([=] {
				vkDestroyImageView(engine->device, image->image_view, nullptr);
				vkDestroyImage(engine->device, image->image, nullptr);
				vmaDestroyImage(engine->vma_allocator, image->image, image->allocation);
				image->image = VK_NULL_HANDLE;
				image->image_view = VK_NULL_HANDLE;
				});
		}
		return image;
	}*/

	void Image::insert_memory_barrier(
		VkCommandBuffer            command_buffer,
		VkPipelineStageFlags2      src_stage_mask,
		VkAccessFlags2             src_access_mask,
		VkPipelineStageFlags2      dst_stage_mask,
		VkAccessFlags2             dst_access_mask,
		VkImageLayout              old_layout,
		VkImageLayout              new_layout,
		uint32_t                   src_queue_family_index /*= VK_QUEUE_FAMILY_IGNORED*/,
		uint32_t                   dst_queue_family_index /*= VK_QUEUE_FAMILY_IGNORED*/
	) const {
		insert_image_memory_barrier(
			command_buffer,
			image,
			src_stage_mask,
			src_access_mask,
			dst_stage_mask,
			dst_access_mask,
			old_layout,
			new_layout,
			src_queue_family_index,
			dst_queue_family_index,
			mip_levels,
			layer_count
		);
	}

	void Image::transition_image_layout(VkImageLayout old_layout, VkImageLayout new_layout, QueueFamilyCategory queue_family_category) {
		auto [queue, command_pool] = queue_family_category == QueueFamilyCategory::GRAPHICS ?
			std::tuple{ engine->graphics_queue, engine->graphic_command_pool } :
			std::tuple{ engine->compute_queue,engine->compute_command_pool };

		immediate_submit(engine, queue, command_pool, [&](VkCommandBuffer commandBuffer) {
			VkPipelineStageFlags2 source_stage;
			VkPipelineStageFlags2 destination_stage;
			VkAccessFlags2 src_access_mask;
			VkAccessFlags2 dst_access_mask;

			if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
				source_stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
				src_access_mask = VK_ACCESS_2_NONE;
				destination_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				dst_access_mask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			}
			else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				source_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				src_access_mask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
				destination_stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
				dst_access_mask = VK_ACCESS_2_SHADER_READ_BIT;
			}
			else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				source_stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
				src_access_mask = VK_ACCESS_2_NONE;
				destination_stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
				dst_access_mask = VK_ACCESS_2_NONE;
			}
			else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_GENERAL) {
				source_stage = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
				src_access_mask = VK_ACCESS_2_NONE;
				destination_stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
				dst_access_mask = VK_ACCESS_2_NONE;
			}
			else {
				throw std::invalid_argument("unsupported layout transition!");
			}

			insert_memory_barrier(commandBuffer,
				source_stage,
				src_access_mask,
				destination_stage,
				dst_access_mask,
				old_layout,
				new_layout
			);
			});
	}

	void Image::copy_from_buffer(VkBuffer buffer, uint32_t mip_level) {
		immediate_submit(engine, [&](VkCommandBuffer command_buffer) {
			const VkBufferImageCopy region{
				.bufferOffset = 0,
				.bufferRowLength = 0,
				.bufferImageHeight = 0,
				.imageSubresource {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = mip_level,
					.baseArrayLayer = 0,
					.layerCount = layer_count,
				},
				.imageOffset = { 0, 0, 0 },
				.imageExtent = {
					width >> mip_level,
					height >> mip_level,
					1,
				},
			};

			vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			});
	}

	void Image::generate_mipmaps() const {
		// Check if image format supports linear blitting
		VkFormatProperties format_properties;
		vkGetPhysicalDeviceFormatProperties(engine->physical_device, format, &format_properties);

		if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			throw std::runtime_error("texture image format does not support linear blitting!");
		}

		immediate_submit(engine, [=](VkCommandBuffer command_buffer) {

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = layer_count;
			barrier.subresourceRange.levelCount = 1;

			int32_t mipWidth = width;
			int32_t mipHeight = height;

			for (uint32_t i = 1; i < mip_levels; i++) {
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				vkCmdPipelineBarrier(command_buffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				VkImageBlit blit{
					.srcSubresource = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel = i - 1,
						.baseArrayLayer = 0,
						.layerCount = layer_count,
					},
					.srcOffsets = {
						{0, 0, 0},
						{mipWidth, mipHeight, 1}
					},
					.dstSubresource = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel = i,
						.baseArrayLayer = 0,
						.layerCount = layer_count,
					},
					.dstOffsets = {
						{ 0, 0, 0 },
						{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 }
					}
				};

				vkCmdBlitImage(command_buffer,
					image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &blit,
					VK_FILTER_LINEAR);

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(command_buffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;
			}

			barrier.subresourceRange.baseMipLevel = mip_levels - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(command_buffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
			});
	}

	Texture::Texture(VulkanEngine* engine, uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits sample_count_flag,
		VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, PreferredMemoryType preferred_memory_type,
		VkImageAspectFlags aspect_flags, VkFilter filter, uint32_t layer_count, VkImageCreateFlags image_flag,
		VkComponentMapping components) :
		Image(engine, width, height, mip_levels, sample_count_flag, format, tiling,
			usage, preferred_memory_type, aspect_flags, layer_count, image_flag, components) {

		const VkSamplerCreateInfo sampler_info = vk_init::sampler_create_info(engine->physical_device, filter, mip_levels);

		if (vkCreateSampler(engine->device, &sampler_info, nullptr, &sampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	Texture::~Texture() {
		if (sampler != VK_NULL_HANDLE) {
			vkDestroySampler(engine->device, sampler, nullptr);
			sampler = VK_NULL_HANDLE;
		}
	}

	void Texture::add_resource_release_callback(CreateResourceFlagBits image_description) {
		if (image_description & 0x00000001) {
			((image_description == SWAPCHAIN_DEPENDENT_BIT) ? engine->swap_chain_deletion_queue : engine->main_deletion_queue).push_function([=] {
				vkDestroySampler(engine->device, sampler, nullptr);
				vkDestroyImageView(engine->device, image_view, nullptr);
				vmaDestroyImage(engine->vma_allocator, image, allocation);
				image = VK_NULL_HANDLE;
				image_view = VK_NULL_HANDLE;
				sampler = VK_NULL_HANDLE;
				});
		}
	}

	//TexturePtr Texture::create_texture(VulkanEngine* engine, uint32_t width, uint32_t height, uint32_t mip_levels,
	//	VkSampleCountFlagBits sample_count_flag, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
	//	PreferredMemoryType preferred_memory_type, VkImageAspectFlags aspect_flags, VkFilter filter,
	//	CreateResourceFlagBits image_description) {
	//	auto texture = std::make_shared<Texture>(engine, width, height, mip_levels, sample_count_flag, format, tiling, usage, preferred_memory_type, aspect_flags, filter);
	//	texture->add_resource_release_callback(image_description);
	//	return texture;
	//}

	TexturePtr Texture::create_2d_texture(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, CreateResourceFlagBits image_description) {
		auto mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		auto texture = std::make_shared<Texture>(engine,
			width,
			height,
			mip_levels,
			VK_SAMPLE_COUNT_1_BIT,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			PreferredMemoryType::VRAM_UNMAPPABLE,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_FILTER_LINEAR);
		texture->add_resource_release_callback(image_description);
		return texture;
	}

	TexturePtr Texture::create_2d_texture(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, CreateResourceFlagBits image_description, const uint32_t mip_levels) {
		auto texture = std::make_shared<Texture>(engine,
			width,
			height,
			mip_levels,
			VK_SAMPLE_COUNT_1_BIT,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			PreferredMemoryType::VRAM_UNMAPPABLE,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_FILTER_LINEAR);
		texture->add_resource_release_callback(image_description);
		return texture;
	}

	TexturePtr Texture::create_2D_render_target(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, VkImageAspectFlags aspect_flags, CreateResourceFlagBits image_description, VkSampleCountFlagBits sample_count_flag) {
		const VkImageUsageFlagBits usage_flag = (aspect_flags == VK_IMAGE_ASPECT_COLOR_BIT) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		auto texture = std::make_shared<Texture>(
			/*engine*/                 engine,
			/*width*/	               width,
			/*height*/                 height,
			/*mip_levels*/             1,
			/*sample_count_flag*/      sample_count_flag,
			/*format*/                 format,
			/*tiling*/                 VK_IMAGE_TILING_OPTIMAL,
			/*usage*/                  usage_flag | VK_IMAGE_USAGE_SAMPLED_BIT,
			/*preferred_memory_type*/  PreferredMemoryType::VRAM_UNMAPPABLE,
			/*aspect_flags*/           aspect_flags,
			/*filter*/                 VK_FILTER_LINEAR);
		texture->add_resource_release_callback(image_description);
		return texture;
	}

	TexturePtr Texture::create_device_texture(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, VkImageAspectFlags aspectFlags, VkImageUsageFlags usage_flag, CreateResourceFlagBits image_description, bool greyscale) {
		TexturePtr texture;
		if (greyscale) {
			texture = std::make_shared<Texture>(
				/*engine*/                engine,
				/*width*/	              width,
				/*height*/                height,
				/*mip_levels*/            1,
				/*sample_count_flag*/     VK_SAMPLE_COUNT_1_BIT,
				/*format*/                format,
				/*tiling*/                VK_IMAGE_TILING_OPTIMAL,
				/*usage*/                 usage_flag,
				/*preferred_memory_type*/ PreferredMemoryType::VRAM_UNMAPPABLE,
				/*aspect_flags*/          aspectFlags,
				/*filter*/                VK_FILTER_LINEAR,
				/*layer_count*/           1,
				/*image_flag*/            0,
				/*components*/            VkComponentMapping{
											VK_COMPONENT_SWIZZLE_R,
											VK_COMPONENT_SWIZZLE_R,
											VK_COMPONENT_SWIZZLE_R,
											VK_COMPONENT_SWIZZLE_ONE
				});
		}
		else {
			texture = std::make_shared<Texture>(engine,
				width,
				height,
				1,
				VK_SAMPLE_COUNT_1_BIT,
				format,
				VK_IMAGE_TILING_OPTIMAL,
				usage_flag,
				PreferredMemoryType::VRAM_UNMAPPABLE,
				aspectFlags,
				VK_FILTER_LINEAR);
		}
		texture->add_resource_release_callback(image_description);
		return texture;
	}

	TexturePtr Texture::create_cubemap_texture(VulkanEngine* engine, uint32_t width, VkFormat format, CreateResourceFlagBits image_description) {
		auto mip_levels = static_cast<uint32_t>(std::floor(std::log2(width))) + 1;
		auto texture = std::make_shared<Texture>(engine,
			/*width*/ width,
			/*height*/ width,
			mip_levels,
			VK_SAMPLE_COUNT_1_BIT,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			PreferredMemoryType::VRAM_UNMAPPABLE,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_FILTER_LINEAR,
			6,
			VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
		texture->add_resource_release_callback(image_description);
		return texture;
	}

	TexturePtr Texture::create_cubemap_texture(VulkanEngine* engine, uint32_t width, VkFormat format, CreateResourceFlagBits image_description, const uint32_t mip_levels) {
		auto texture = std::make_shared<Texture>(engine,
			width,
			width,
			mip_levels,
			VK_SAMPLE_COUNT_1_BIT,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			PreferredMemoryType::VRAM_UNMAPPABLE,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_FILTER_LINEAR,
			6,
			VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
		texture->add_resource_release_callback(image_description);
		return texture;
	}

	TexturePtr Texture::load_2d_texture_from_host(VulkanEngine* engine, void const* host_pixels, int tex_width, int texHeight, int texChannels, bool enableMipmap/*= true*/, VkFormat format/*= VK_FORMAT_R8G8B8A8_SRGB*/) {

		const VkDeviceSize image_size = static_cast<uint64_t>(tex_width) * texHeight * texChannels;

		auto const staging_buffer = engine::Buffer::create_buffer(engine,
			image_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			PreferredMemoryType::RAM_FOR_UPLOAD,
			TEMP_BIT);
		staging_buffer->copy_from_host(host_pixels);

		engine::TexturePtr texture;

		if (enableMipmap) {
			texture = engine::Texture::create_2d_texture(engine, tex_width, texHeight, format, SWAPCHAIN_INDEPENDENT_BIT);
		}
		else {
			texture = engine::Texture::create_2d_texture(engine, tex_width, texHeight, format, SWAPCHAIN_INDEPENDENT_BIT, 1);
		}

		texture->transition_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		texture->copy_from_buffer(staging_buffer->buffer);

		//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

		texture->generate_mipmaps();

		return texture;
	}

	TexturePtr Texture::load_2d_texture(VulkanEngine* engine, const std::string_view file_path, const bool enable_mipmap/*= true*/, const VkFormat format/* = VK_FORMAT_R8G8B8A8_SRGB*/) {
		int tex_width, tex_height, tex_channels;
		stbi_uc* pixels = stbi_load(file_path.data(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
		//VkDeviceSize imageSize = static_cast<uint64_t>(texWidth) * texHeight * 4;
		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}
		return load_2d_texture_from_host(engine, pixels, tex_width, tex_height, tex_channels, enable_mipmap, format);
	}

	TexturePtr Texture::load_cubemap_texture(VulkanEngine* engine, const std::span<std::string const> file_paths) {
		int texWidth, texHeight, texChannels;
		float* pixels[6];
		for (int8_t i = 0; i < 6; i++) {
			auto const extension_name = std::filesystem::path(file_paths[i]).extension();
			if (extension_name == ".hdr") { // hdr layout: r8g8b8e8(32-bit)
				pixels[i] = stbi_loadf(file_paths[i].c_str(), &texWidth, &texHeight, &texChannels, 4);
				if (!pixels[i]) {
					throw std::runtime_error("Error occurs when loading hdr!");
				}
			}
			else if (extension_name == ".exr") {
				const char* err;
				const int ret = LoadEXR(&pixels[i], &texWidth, &texHeight, file_paths[i].c_str(), &err); // LoadEXR returned layout: r32g32b32a32

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

		const VkDeviceSize layer_size = static_cast<uint64_t>(texWidth) * texHeight * 4 * sizeof(float);
		const VkDeviceSize image_size = layer_size * 6;

		auto const staging_buffer = engine::Buffer::create_buffer(engine,
			image_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			PreferredMemoryType::RAM_FOR_UPLOAD,
			TEMP_BIT);

		for (int8_t i = 0; i < 6; i++) {
			memcpy(static_cast<char*>(staging_buffer->mapped_buffer) + (layer_size * i), pixels[i], layer_size);
			stbi_image_free(pixels[i]);
		}

		auto texture = engine::Texture::create_cubemap_texture(engine, texWidth, VK_FORMAT_R32G32B32A32_SFLOAT, SWAPCHAIN_INDEPENDENT_BIT);

		texture->transition_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		texture->copy_from_buffer(staging_buffer->buffer);

		//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

		texture->generate_mipmaps();

		return texture;
	}

	TexturePtr Texture::load_prefiltered_map_texture(VulkanEngine* engine, const std::span<std::vector<std::string> const> file_path_layers) {

		TexturePtr texture;

		for (uint32_t mip_level = 0; mip_level < file_path_layers.size(); mip_level++) {
			auto& file_paths = file_path_layers[mip_level];
			int tex_width, tex_height, tex_channels;
			float* pixels[6];
			for (int8_t i = 0; i < 6; i++) {
				auto const extension_name = std::filesystem::path(file_paths[i]).extension();
				if (extension_name == ".hdr") { // hdr layout: r8g8b8e8(32-bit)
					pixels[i] = stbi_loadf(file_paths[i].c_str(), &tex_width, &tex_height, &tex_channels, 4);
					if (!pixels[i]) {
						throw std::runtime_error("Error occurs when loading hdr!");
					}
				}
				else if (extension_name == ".exr") {
					const char* err;
					const int ret = LoadEXR(&pixels[i], &tex_width, &tex_height, file_paths[i].c_str(), &err); // LoadEXR returned layout: r32g32b32a32

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

			const VkDeviceSize layer_size = static_cast<uint64_t>(tex_width) * tex_height * 4 * sizeof(float);
			const VkDeviceSize image_size = layer_size * 6;

			auto const staging_buffer = engine::Buffer::create_buffer(engine,
				image_size,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				PreferredMemoryType::RAM_FOR_UPLOAD,
				TEMP_BIT);

			for (int8_t i = 0; i < 6; i++) {
				memcpy(static_cast<char*>(staging_buffer->mapped_buffer) + (layer_size * i), pixels[i], layer_size);
				stbi_image_free(pixels[i]);
			}

			if (mip_level == 0) {
				texture = engine::Texture::create_cubemap_texture(
					engine,
					tex_width,
					VK_FORMAT_R32G32B32A32_SFLOAT,
					SWAPCHAIN_INDEPENDENT_BIT,
					static_cast<uint32_t>(file_path_layers.size()));
			}

			texture->transition_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			texture->copy_from_buffer(staging_buffer->buffer, mip_level);
		}

		//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
		immediate_submit(engine, [=](VkCommandBuffer command_buffer) {
			texture->insert_memory_barrier(
				command_buffer,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			});
		return texture;
	}
}