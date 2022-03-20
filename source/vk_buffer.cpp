#include "vk_buffer.h"
#include "vk_util.h"
#include "vk_engine.h"

#include <stdexcept>


namespace vk_base {
	Buffer::Buffer(VkDevice device, VkPhysicalDevice physicalDevice, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, VkDeviceSize size) : device(device), size(size) {
		VkBufferCreateInfo bufferInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = bufferUsage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = find_memory_type(physicalDevice, memRequirements.memoryTypeBits, memoryProperties),
		};

		if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, memory, 0);

		if (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & memoryProperties) {
			vkMapMemory(device, memory, 0, size, 0, &mapped_buffer);
		}
		
	}

	Buffer::~Buffer() {
		if (memory != VK_NULL_HANDLE) {
			if (mapped_buffer) {
				vkUnmapMemory(device, memory);
				mapped_buffer = nullptr;
			}
			vkFreeMemory(device, memory, nullptr);
			memory = VK_NULL_HANDLE;
		}
		if (buffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(device, buffer, nullptr);
			buffer = VK_NULL_HANDLE;
		}
	}
}

namespace engine {
	using BufferPtr = std::shared_ptr<Buffer>;

	BufferPtr Buffer::createBuffer(VulkanEngine* engine, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, CreateResourceFlagBits bufferDescription) {
		auto pBuffer = std::make_shared<Buffer>(engine->device, engine->physicalDevice, bufferUsage, memoryProperties, size);
		if (bufferDescription & 0x00000001) {
			((bufferDescription == SWAPCHAIN_DEPENDENT_BIT) ? engine->swapChainDeletionQueue : engine->main_deletion_queue).push_function([=]() {
				vkDestroyBuffer(pBuffer->device, pBuffer->buffer, nullptr);
				vkFreeMemory(pBuffer->device, pBuffer->memory, nullptr);
				pBuffer->buffer = VK_NULL_HANDLE;
				pBuffer->memory = VK_NULL_HANDLE;
				});
		}
		return pBuffer;
	}

	void Buffer::copyFromHost(void* hostData) {
		memcpy(mapped_buffer, hostData, (size_t)size);
	}

	void Buffer::copyFromHost(const void* host_data, size_t data_size, size_t offset) {
		memcpy(static_cast<char*>(mapped_buffer) + offset, host_data, data_size);
	}

	void Buffer::copyFromBuffer(VulkanEngine* engine, VkBuffer srcBuffer) {
		immediate_submit(engine, [=](VkCommandBuffer commandBuffer) {
			VkBufferCopy copyRegion{};
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, srcBuffer, buffer, 1, &copyRegion);
			});
	}
}