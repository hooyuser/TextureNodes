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

		Buffer(VkDevice device, VkPhysicalDevice physicalDevice, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, VkDeviceSize size);

		~Buffer();
	private:
		Buffer() {}
	};
}

namespace engine {
	class Buffer : public vk_base::Buffer {
		using Base = vk_base::Buffer;
		using Base::Base;
		using BufferPtr = std::shared_ptr<Buffer>;
	public:
		static BufferPtr createBuffer(VulkanEngine* engine, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, CreateResourceFlagBits bufferDescription = SWAPCHAIN_INDEPENDENT_BIT);
		void copyFromHost(void* hostData);
		void copyFromHost(const void* host_data, size_t data_size, size_t offset = 0);
		void copyFromBuffer(VulkanEngine* engine, VkBuffer srcBuffer);
	};
}

using BufferPtr = std::shared_ptr<engine::Buffer>;

