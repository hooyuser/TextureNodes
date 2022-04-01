#pragma once
#include "vk_types.h"

class VulkanEngine;

namespace vk_init {
	VkSamplerCreateInfo samplerCreateInfo(VkPhysicalDevice physicalDevice, VkFilter filters, uint32_t mipLevels, VkSamplerAddressMode samplerAdressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

namespace engine {
	class Image {
		using ImagePtr = std::shared_ptr<Image>;
	public:
		VkDevice device = VK_NULL_HANDLE;

		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;

		uint32_t width;
		uint32_t height;
		VkFormat format;
		uint32_t mipLevels = 0;
		uint32_t layerCount = 1;

		Image() {}

		Image(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels,
			VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags, VkComponentMapping components = {VK_COMPONENT_SWIZZLE_IDENTITY});

		Image(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels,
			VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags, VkImageCreateFlags imageFlag, uint32_t layerCount);

		~Image();

		static ImagePtr createImage(VulkanEngine* engine, uint32_t width, uint32_t height, uint32_t mipLevels,
			VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags, CreateResourceFlagBits imageDescription);

		void transitionImageLayout(VulkanEngine* engine, VkImageLayout oldLayout, VkImageLayout newLayout);

		void copyFromBuffer(VulkanEngine* engine, VkBuffer buffer, uint32_t mipLevel = 0);

		void generateMipmaps(VulkanEngine* engine) const;
	};


	class Texture : public Image {
		using TexturePtr = std::shared_ptr<Texture>;
	public:
		VkSampler sampler = VK_NULL_HANDLE;

		Texture() {}

		Texture(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels,
			VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags, VkFilter filters, VkComponentMapping components = {VK_COMPONENT_SWIZZLE_IDENTITY});

		Texture(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels,
			VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags, VkFilter filters, VkImageCreateFlagBits imageFlag,
			uint32_t layerCount);

		~Texture();

		static TexturePtr createTexture(VulkanEngine* engine, uint32_t width, uint32_t height, uint32_t mipLevels,
			VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags, VkFilter filters, CreateResourceFlagBits imageDescription);

		static TexturePtr create2DTexture(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, CreateResourceFlagBits imageDescription);

		static TexturePtr create2DTexture(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, CreateResourceFlagBits imageDescription, const uint32_t mipLevels);

		static TexturePtr create_2D_render_target(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, VkImageAspectFlags aspectFlags, CreateResourceFlagBits imageDescription = SWAPCHAIN_INDEPENDENT_BIT, VkSampleCountFlagBits sample_count_flag = VK_SAMPLE_COUNT_1_BIT);

		static TexturePtr create_device_texture(VulkanEngine* engine, uint32_t width, uint32_t height, VkFormat format, VkImageAspectFlags aspectFlags, VkImageUsageFlags usage_flag, CreateResourceFlagBits imageDescription = SWAPCHAIN_INDEPENDENT_BIT, bool greyscale = false);

		static TexturePtr create_cubemap_texture(VulkanEngine* engine, uint32_t width, VkFormat format, CreateResourceFlagBits imageDescription);

		static TexturePtr create_cubemap_texture(VulkanEngine* engine, uint32_t width, VkFormat format, CreateResourceFlagBits imageDescription, const uint32_t mipLevels);

		static TexturePtr load_2d_texture_from_host(VulkanEngine* engine, void const* host_pixels, int tex_width, int texHeight, int texChannels, bool enableMipmap = true, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);

		static TexturePtr load_2d_texture(VulkanEngine* engine, const char* filePath, bool enableMipmap = true, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);

		static TexturePtr load_2d_texture(VulkanEngine* engine, const std::string& filePath, bool enableMipmap = true, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);

		static TexturePtr load_cubemap_texture(VulkanEngine* engine, const std::vector<std::string>& filePaths);

		static TexturePtr load_prefiltered_map_texture(VulkanEngine* engine, const std::vector<std::vector<std::string>>& filePathLayers);
	};
	using TexturePtr = std::shared_ptr<engine::Texture>;
}


using ImagePtr = std::shared_ptr<engine::Image>;
using TexturePtr = std::shared_ptr<engine::Texture>;



