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

	VkPipelineMultisampleStateCreateInfo multisamplingStateCreateInfo(VkSampleCountFlagBits msaaSamples);

	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo(VkCompareOp compareOp);

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState();

	VkPipelineColorBlendStateCreateInfo colorBlendAttachmentCreateInfo(VkPipelineColorBlendAttachmentState& colorBlendAttachment, uint32_t attachment_count = 1);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(std::span<VkDescriptorSetLayout> descriptorSetLayout);

	VkFramebufferCreateInfo framebufferCreateInfo(VkRenderPass renderPass, VkExtent2D extent, std::span<VkImageView> attachments);

	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding, uint32_t descriptorCount = 1);

	VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

	VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);
}