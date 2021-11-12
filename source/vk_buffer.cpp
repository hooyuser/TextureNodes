#include "vk_buffer.h"
#include "vk_util.h"
#include "vk_engine.h"

#include <stdexcept>


namespace vk_base {
	Buffer::Buffer(VkDevice device, VkPhysicalDevice physicalDevice, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, VkDeviceSize size) : device(device), size(size) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = bufferUsage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, memoryProperties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, memory, 0);
	}

	Buffer::~Buffer() {
		if (buffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(device, buffer, nullptr);
			buffer = VK_NULL_HANDLE;
		}
		if (memory != VK_NULL_HANDLE) {
			vkFreeMemory(device, memory, nullptr);
			memory = VK_NULL_HANDLE;
		}
	}
}

namespace engine {
	using BufferPtr = std::shared_ptr<Buffer>;

	BufferPtr Buffer::createBuffer(VulkanEngine* engine, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, CreateResourceFlagBits bufferDescription) {
		auto pBuffer = std::make_shared<Buffer>(engine->device, engine->physicalDevice, bufferUsage, memoryProperties, size);
		if (bufferDescription & 0x00000001) {
			((bufferDescription == AFTER_SWAPCHAIN_BIT) ? engine->swapChainDeletionQueue : engine->mainDeletionQueue).push_function([=]() {
				vkDestroyBuffer(pBuffer->device, pBuffer->buffer, nullptr);
				vkFreeMemory(pBuffer->device, pBuffer->memory, nullptr);
				pBuffer->buffer = VK_NULL_HANDLE;
				pBuffer->memory = VK_NULL_HANDLE;
				});

		}
		return pBuffer;
	}

	void Buffer::copyFromHost(void* hostData) {
		void* data;
		vkMapMemory(device, memory, 0, size, 0, &data);
		memcpy(data, hostData, (size_t)size);
		vkUnmapMemory(device, memory);
	}

	void Buffer::copyFromBuffer(VulkanEngine* engine, VkBuffer srcBuffer) {
		immediateSubmit(engine, [=](VkCommandBuffer commandBuffer) {
			VkBufferCopy copyRegion{};
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, srcBuffer, buffer, 1, &copyRegion);
			});
	}

}