#include "vk_buffer.h"
#include "vk_util.h"

#include <stdexcept>


namespace vk_base {
	Buffer::Buffer(VkDevice device, VkPhysicalDevice physical_device, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties, VkDeviceSize size) : device(device), size(size) {
		const VkBufferCreateInfo buffer_info{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = buffer_usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};

		if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memory_requirements;
		vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

		const VkMemoryAllocateInfo allocate_info{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memory_requirements.size,
			.memoryTypeIndex = find_memory_type(physical_device, memory_requirements.memoryTypeBits, memory_properties),
		};

		if (vkAllocateMemory(device, &allocate_info, nullptr, &memory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, memory, 0);

		if (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & memory_properties) {
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

	BufferPtr Buffer::create_buffer(VulkanEngine* engine, VkDeviceSize device_size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties, CreateResourceFlagBits buffer_description) {
		auto p_buffer = std::make_shared<Buffer>(engine->device, engine->physical_device, buffer_usage, memory_properties, device_size);
		if (buffer_description & 0x00000001) {
			((buffer_description == SWAPCHAIN_DEPENDENT_BIT) ? engine->swap_chain_deletion_queue : engine->main_deletion_queue).push_function([=]() {
				vkDestroyBuffer(p_buffer->device, p_buffer->buffer, nullptr);
				vkFreeMemory(p_buffer->device, p_buffer->memory, nullptr);
				p_buffer->buffer = VK_NULL_HANDLE;
				p_buffer->memory = VK_NULL_HANDLE;
				});
		}
		return p_buffer;
	}

	void Buffer::copy_from_host(void const* host_data) const {
		memcpy(mapped_buffer, host_data, size);
	}

	void Buffer::copy_from_host(void const* host_data, size_t data_size, size_t offset) const {
		memcpy(static_cast<char*>(mapped_buffer) + offset, host_data, data_size);
	}

	void Buffer::copy_from_buffer(VulkanEngine* engine, VkBuffer src_buffer) const {
		immediate_submit(engine, [&](VkCommandBuffer command_buffer) {
			const VkBufferCopy copy_region{
				.size = size
			};
			vkCmdCopyBuffer(command_buffer, src_buffer, buffer, 1, &copy_region);
			});
	}
}