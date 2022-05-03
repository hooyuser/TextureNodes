#include "vk_buffer.h"
#include "vk_util.h"
#include <stdexcept>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace vk_base {
	Buffer::Buffer(VmaAllocator vma_allocator, VkBufferUsageFlags buffer_usage, PreferredMemoryType preferred_memory_type, VkDeviceSize size) : vma_allocator(vma_allocator), size(size) {
		const VkBufferCreateInfo buffer_info{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = buffer_usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};

		VmaAllocationCreateInfo alloc_create_info;
		switch (preferred_memory_type) {
			using enum PreferredMemoryType;
		case VRAM_UNMAPPABLE:
			alloc_create_info = VmaAllocationCreateInfo{
				.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
			};
			break;
		case VRAM_MAPPABLE:
			alloc_create_info = VmaAllocationCreateInfo{
				.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
				.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
			};
			break;
		case RAM_FOR_UPLOAD:
			alloc_create_info = VmaAllocationCreateInfo{
				.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
				.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
			};
			break;
		case RAM_FOR_DOWNLOAD:
			alloc_create_info = VmaAllocationCreateInfo{
				.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
				.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
			};
		}
		VmaAllocationInfo allocation_info;
		if (vmaCreateBuffer(
			vma_allocator,
			&buffer_info,
			&alloc_create_info,
			&buffer,
			&allocation,
			&allocation_info) != VK_SUCCESS
			) {
			throw std::runtime_error("vma failed to create buffer!");
		}
		mapped_buffer = allocation_info.pMappedData;
	}

	Buffer::~Buffer() {
		if (buffer != VK_NULL_HANDLE) {
			vmaDestroyBuffer(vma_allocator, buffer, allocation);
			buffer = VK_NULL_HANDLE;
		}
	}
}

namespace engine {
	using BufferPtr = std::shared_ptr<Buffer>;

	BufferPtr Buffer::create_buffer(VulkanEngine* engine, VkDeviceSize device_size, VkBufferUsageFlags buffer_usage, PreferredMemoryType preferred_memory_type, CreateResourceFlagBits buffer_description) {
		auto p_buffer = std::make_shared<Buffer>(
			engine->vma_allocator,
			buffer_usage,
			preferred_memory_type,
			device_size);
		if (buffer_description & 0x00000001) {
			((buffer_description == SWAPCHAIN_DEPENDENT_BIT) ? engine->swap_chain_deletion_queue : engine->main_deletion_queue).push_function([=] {
				if (p_buffer->buffer != VK_NULL_HANDLE) {
					vmaDestroyBuffer(engine->vma_allocator, p_buffer->buffer, p_buffer->allocation);
					p_buffer->buffer = VK_NULL_HANDLE;
				}
				});
		}
		return p_buffer;
	}

	void Buffer::copy_from_host(void const* host_data) const {
		memcpy(mapped_buffer, host_data, size);
	}

	void Buffer::copy_from_host(void const* host_data, const size_t data_size, const size_t offset) const {
		memcpy(static_cast<int8_t*>(mapped_buffer) + offset, host_data, data_size);
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