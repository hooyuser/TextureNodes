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
		static BufferPtr createBuffer(VulkanEngine* engine, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, CreateResourceFlagBits bufferDescription);
		void copyFromHost(void* hostData);
		void copyFromBuffer(VulkanEngine* engine, VkBuffer srcBuffer);
	};
}

using BufferPtr = std::shared_ptr<engine::Buffer>;

