#pragma once

#include "vk_types.h"
#include "vk_memory.h"

#include <span>

class VulkanEngine;

enum class QueueFamilyCategory {
	GRAPHICS,
	COMPUTE,
	TRANSFER,
	PRESENT,
};

namespace engine {
	class Image {
		using ImagePtr = std::shared_ptr<Image>;
	public:
		VulkanEngine* engine;
		VkImage image = VK_NULL_HANDLE;
		VmaAllocation allocation = nullptr;
		VkImageView image_view = VK_NULL_HANDLE;

		uint32_t width;
		uint32_t height;
		VkFormat format;
		uint32_t mip_levels = 0;
		uint32_t layer_count;

		Image(VulkanEngine* engine, uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits sample_count_flag,
			VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, PreferredMemoryType preferred_memory_type,
			VkImageAspectFlags aspect_flags, uint32_t layer_count = 1, VkImageCreateFlags image_flag = 0,
			VkComponentMapping components = { VK_COMPONENT_SWIZZLE_IDENTITY });

		~Image();

		void insert_memory_barrier(
			VkCommandBuffer            command_buffer,
			VkPipelineStageFlags2      src_stage_mask,
			VkAccessFlags2             src_access_mask,
			VkPipelineStageFlags2      dst_stage_mask,
			VkAccessFlags2             dst_access_mask,
			VkImageLayout              old_layout,
			VkImageLayout              new_layout,
			uint32_t                   src_queue_family_index = VK_QUEUE_FAMILY_IGNORED,
			uint32_t                   dst_queue_family_index = VK_QUEUE_FAMILY_IGNORED
		) const;

		void transition_image_layout(VkImageLayout old_layout, VkImageLayout new_layout, QueueFamilyCategory = QueueFamilyCategory::GRAPHICS);

		void copy_from_buffer(VkBuffer buffer, uint32_t mip_level = 0);

		void generate_mipmaps() const;
	};


	struct Texture : public Image {
		using TexturePtr = std::shared_ptr<Texture>;
	public:
		VkSampler sampler = VK_NULL_HANDLE;

		Texture(VulkanEngine* engine, uint32_t tex_width, uint32_t tex_height, uint32_t mip_levels, VkSampleCountFlagBits sample_count_flag,
			VkFormat img_format, VkImageTiling img_tiling, VkImageUsageFlags img_usage, PreferredMemoryType preferred_memory_type,
			VkImageAspectFlags aspect_flags, VkFilter filter, uint32_t layer_count = 1, VkImageCreateFlags image_flag = 0,
			VkComponentMapping components = { VK_COMPONENT_SWIZZLE_IDENTITY });

		~Texture();

		void add_resource_release_callback(CreateResourceFlagBits image_description);

		static TexturePtr create_2d_texture(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, CreateResourceFlagBits image_description);

		static TexturePtr create_2d_texture(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, CreateResourceFlagBits image_description, uint32_t mip_levels);

		static TexturePtr create_2D_render_target(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, VkImageAspectFlags aspect_flags, CreateResourceFlagBits image_description = SWAPCHAIN_INDEPENDENT_BIT, VkSampleCountFlagBits sample_count_flag = VK_SAMPLE_COUNT_1_BIT);

		static TexturePtr create_device_texture(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, VkImageAspectFlags aspectFlags, VkImageUsageFlags usage_flag, CreateResourceFlagBits image_description = SWAPCHAIN_INDEPENDENT_BIT, bool greyscale = false);

		static TexturePtr create_cubemap_texture(VulkanEngine* engine, uint32_t width, VkFormat format, CreateResourceFlagBits image_description);

		static TexturePtr create_cubemap_texture(VulkanEngine* engine, uint32_t width, VkFormat format, CreateResourceFlagBits image_description, uint32_t mip_levels);

		static TexturePtr load_2d_texture_from_host(VulkanEngine* engine, void const* host_pixels, int tex_width, int texHeight, int texChannels, bool enableMipmap = true, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);

		static TexturePtr load_2d_texture(VulkanEngine* engine, std::string_view file_path, bool enable_mipmap = true, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);

		static TexturePtr load_cubemap_texture(VulkanEngine* engine, std::span<std::string const> file_paths);

		static TexturePtr load_prefiltered_map_texture(VulkanEngine* engine, std::span<std::vector<std::string> const> file_path_layers);
	};
	using TexturePtr = std::shared_ptr<engine::Texture>;
}


using ImagePtr = std::shared_ptr<engine::Image>;
using TexturePtr = std::shared_ptr<engine::Texture>;