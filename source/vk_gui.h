#pragma once
#include "vk_types.h"
#include <vector>

class VulkanEngine;

namespace engine {
	class GUI {
	public:
		VulkanEngine* engine;

		VkRenderPass renderPass;
		VkCommandPool commandPool;
		std::vector<VkCommandBuffer> command_buffers;
		std::vector<VkFramebuffer> framebuffers;

		void init(VulkanEngine* engine);
		void create_framebuffers();
		void begin_render();
		void end_render(VulkanEngine* engine, const uint32_t imageIndex);

	protected:
		void init_command_pool();
		void init_render_pass();
		void init_command_buffers();
	};
}