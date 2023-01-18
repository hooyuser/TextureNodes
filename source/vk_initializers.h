#pragma once

#include "vk_types.h"
#include <vector>
#include <span>

class PbrParameters;

namespace vkinit {
	VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);

	VkRenderPassBeginInfo render_pass_begin_info(VkRenderPass renderPass, VkExtent2D windowExtent, VkFramebuffer framebuffer);

	VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule);

	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info(const std::vector<VkVertexInputBindingDescription>& bindingDescriptions, const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions);

	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(VkPrimitiveTopology topology);

	VkPipelineViewportStateCreateInfo viewport_state_create_info(const VkViewport* pViewport, const VkRect2D* pScissor);

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode, VkCullModeFlagBits cullMode = VK_CULL_MODE_NONE);

	VkPipelineMultisampleStateCreateInfo multisampling_state_create_info(VkSampleCountFlagBits msaa_samples);

	constexpr VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(VkCompareOp compare_op) noexcept {
		return {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = compare_op,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
		};
	}

	VkPipelineColorBlendAttachmentState color_blend_attachment_state();

	VkPipelineColorBlendStateCreateInfo color_blend_attachment_create_info(VkPipelineColorBlendAttachmentState& colorBlendAttachment, uint32_t attachment_count = 1);

	VkPipelineLayoutCreateInfo pipeline_layout_create_info(std::span<VkDescriptorSetLayout> descriptorSetLayout);

	VkFramebufferCreateInfo framebuffer_create_info(VkRenderPass render_pass, VkExtent2D extent, std::span<VkImageView> attachments);

	VkDescriptorSetLayoutBinding descriptor_set_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding, uint32_t descriptorCount = 1);

	constexpr VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0) noexcept {
		return {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = flags,
		};
	}

	constexpr VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags /*= 0*/) noexcept {
		return {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = flags,
		};
	}

	VkSamplerCreateInfo sampler_create_info(VkPhysicalDevice physical_device, VkFilter filters, uint32_t mip_levels, VkSamplerAddressMode sampler_address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
}