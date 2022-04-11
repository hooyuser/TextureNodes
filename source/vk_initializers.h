#pragma once

#include "vk_types.h"
#include <vector>
#include <span>

class PbrParameters;

namespace vkinit {
	VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);

	VkRenderPassBeginInfo renderPassBeginInfo(VkRenderPass renderPass, VkExtent2D windowExtent, VkFramebuffer framebuffer);

	VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo(const std::vector<VkVertexInputBindingDescription>& bindingDescriptions, const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions);

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo(VkPrimitiveTopology topology);

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo(const VkViewport* pViewport, const VkRect2D* pScissor);

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo(VkPolygonMode polygonMode, VkCullModeFlagBits cullMode = VK_CULL_MODE_NONE);

	VkPipelineMultisampleStateCreateInfo multisampling_state_create_info(VkSampleCountFlagBits msaa_samples);

	constexpr VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo(VkCompareOp compareOp) noexcept {
		return {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = compareOp,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
		};
	}

	VkPipelineColorBlendAttachmentState color_blend_attachment_state();

	VkPipelineColorBlendStateCreateInfo colorBlendAttachmentCreateInfo(VkPipelineColorBlendAttachmentState& colorBlendAttachment, uint32_t attachment_count = 1);

	VkPipelineLayoutCreateInfo pipeline_layout_create_info(std::span<VkDescriptorSetLayout> descriptorSetLayout);

	VkFramebufferCreateInfo framebufferCreateInfo(VkRenderPass renderPass, VkExtent2D extent, std::span<VkImageView> attachments);

	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding, uint32_t descriptorCount = 1);

	constexpr VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0) noexcept {
		return {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = flags,
		};
	}

	constexpr VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags /*= 0*/) noexcept {
		return {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = flags,
		};
	}
}