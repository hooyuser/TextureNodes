#pragma once
#include <vk_mem_alloc.h>
#include "util/static_map.h"

enum class PreferredMemoryType {
	VRAM_UNMAPPABLE = 0,
	VRAM_MAPPABLE,
	RAM_FOR_UPLOAD,
	RAM_FOR_DOWNLOAD,
};

static constexpr inline auto memory_type_vma_alloc_map = StaticMap{
	std::pair{
		PreferredMemoryType::VRAM_UNMAPPABLE,
		VmaAllocationCreateInfo{
			.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		}
	},
	std::pair{
		PreferredMemoryType::VRAM_MAPPABLE,
		VmaAllocationCreateInfo{
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		}
	},
	std::pair{
		PreferredMemoryType::RAM_FOR_UPLOAD,
		VmaAllocationCreateInfo{
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
		}
	},
	std::pair{
		PreferredMemoryType::RAM_FOR_DOWNLOAD,
		VmaAllocationCreateInfo{
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
		}
	},
};
