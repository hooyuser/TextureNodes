#pragma once
#include "vk_types.h"

class VulkanEngine;

namespace vk_base {
	class Buffer {
	public:
		VkDevice device = VK_NULL_HANDLE;

		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;

		VkDeviceSize size = 0;
		void* mapped_buffer = nullptr;

		Buffer(VkDevice device, VkPhysicalDevice physical_device, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties, VkDeviceSize size);

		~Buffer();
	private:
		Buffer() = default;
	};
}

namespace engine {
	class Buffer : public vk_base::Buffer {
		using Base = vk_base::Buffer;
		using Base::Base;
		using BufferPtr = std::shared_ptr<Buffer>;
	public:
		static BufferPtr create_buffer(VulkanEngine* engine, VkDeviceSize device_size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties, CreateResourceFlagBits buffer_description = SWAPCHAIN_INDEPENDENT_BIT);
		void copy_from_host(void const* host_data) const;
		void copy_from_host(void const* host_data, size_t data_size, size_t offset = 0) const;
		void copy_from_buffer(VulkanEngine* engine, VkBuffer src_buffer) const;
	};
}

using BufferPtr = std::shared_ptr<engine::Buffer>;

