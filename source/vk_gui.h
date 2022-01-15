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
		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkFramebuffer> framebuffers;

		void init(VulkanEngine* engine);
		void create_framebuffers();
		void beginRender();
		void endRender(VulkanEngine* engine, const uint32_t imageIndex);

	protected:
		void initCommandPool();
		void initRenderPass();
		void initCommandBuffers();
	};
}