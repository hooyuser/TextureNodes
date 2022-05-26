#include "vk_initializers.h"
#include "vk_mesh.h"
#include <array>


VkCommandPoolCreateInfo vkinit::commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags /*= 0*/) {
	VkCommandPoolCreateInfo commandPoolInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags,
		.queueFamilyIndex = queueFamilyIndex,
	};

	return commandPoolInfo;
}

VkRenderPassBeginInfo vkinit::renderPassBeginInfo(VkRenderPass renderPass, VkExtent2D extent, VkFramebuffer framebuffer) {

	static const std::array clearValues{
		VkClearValue{
			.color = { {1.0f, 0.0f, 0.0f, 1.0f} },
		},
		VkClearValue{
			.depthStencil = { 1.0f, 0 },
		},
	};

	VkRenderPassBeginInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = framebuffer,
		.renderArea = {
			.offset = {0, 0},
			.extent = extent,
		},
		.clearValueCount = static_cast<uint32_t>(clearValues.size()),
		.pClearValues = clearValues.data(),
	};

	return renderPassInfo;
}

VkCommandBufferAllocateInfo vkinit::commandBufferAllocateInfo(VkCommandPool pool, uint32_t count /*= 1*/, VkCommandBufferLevel level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/) {
	VkCommandBufferAllocateInfo info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO ,
		.pNext = nullptr,
		.commandPool = pool,
		.level = level,
		.commandBufferCount = count,
	};

	return info;
}

VkPipelineShaderStageCreateInfo vkinit::pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule) {

	VkPipelineShaderStageCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };

	info.pNext = nullptr;
	//shader stage
	info.stage = stage;
	//module containing the code for this shader stage
	info.module = shaderModule;
	//the entry point of the shader
	info.pName = "main";
	return info;
}


VkPipelineVertexInputStateCreateInfo vkinit::vertexInputStateCreateInfo(const std::vector<VkVertexInputBindingDescription>& bindingDescriptions, const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions) {
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	vertexInputInfo.pNext = nullptr;

	//no vertex bindings or attributes
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	return vertexInputInfo;
}

VkPipelineInputAssemblyStateCreateInfo vkinit::inputAssemblyCreateInfo(VkPrimitiveTopology topology) {
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

	inputAssemblyInfo.pNext = nullptr;
	inputAssemblyInfo.topology = topology;
	//we are not going to use primitive restart on the entire tutorial so leave it on false
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
	return inputAssemblyInfo;
}

VkPipelineViewportStateCreateInfo vkinit::viewportStateCreateInfo(const VkViewport* pViewport, const VkRect2D* pScissor) {
	VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };

	viewportState.viewportCount = 1;
	viewportState.pViewports = pViewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = pScissor;
	return viewportState;
}

VkPipelineRasterizationStateCreateInfo vkinit::rasterizationStateCreateInfo(VkPolygonMode polygonMode, VkCullModeFlagBits cullMode/*= VK_CULL_MODE_NONE*/) {
	VkPipelineRasterizationStateCreateInfo rasterizerInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

	rasterizerInfo.pNext = nullptr;
	rasterizerInfo.depthClampEnable = VK_FALSE;
	//rasterizer discard allows objects with holes, default to no
	rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;

	rasterizerInfo.polygonMode = polygonMode;
	rasterizerInfo.lineWidth = 1.0f;
	rasterizerInfo.cullMode = cullMode;
	rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	//no depth bias
	rasterizerInfo.depthBiasEnable = VK_FALSE;
	rasterizerInfo.depthBiasConstantFactor = 0.0f;
	rasterizerInfo.depthBiasClamp = 0.0f;
	rasterizerInfo.depthBiasSlopeFactor = 0.0f;

	return rasterizerInfo;
}

VkPipelineMultisampleStateCreateInfo vkinit::multisampling_state_create_info(VkSampleCountFlagBits msaa_samples) {
	return {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO ,
		.pNext = nullptr,
		.rasterizationSamples = msaa_samples,
		.sampleShadingEnable = VK_TRUE,
		.minSampleShading = 0.2f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};
}

VkPipelineColorBlendAttachmentState vkinit::color_blend_attachment_state() {
	return {
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};
}

VkPipelineColorBlendStateCreateInfo vkinit::colorBlendAttachmentCreateInfo(VkPipelineColorBlendAttachmentState& colorBlendAttachment, uint32_t attachment_count) {
	return {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = attachment_count,
		.pAttachments = &colorBlendAttachment,
		.blendConstants{ 0.0f, 0.0f, 0.0f, 0.0f },
	};
}

VkPipelineLayoutCreateInfo vkinit::pipeline_layout_create_info(std::span<VkDescriptorSetLayout> descriptorSetLayouts) {
	return {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
		.pSetLayouts = descriptorSetLayouts.data(),
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};
}

VkFramebufferCreateInfo vkinit::framebufferCreateInfo(VkRenderPass renderPass, VkExtent2D extent, std::span<VkImageView> attachments) {
	VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };

	framebufferInfo.pNext = nullptr;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = extent.width;
	framebufferInfo.height = extent.height;
	framebufferInfo.layers = 1;

	return framebufferInfo;
}

VkDescriptorSetLayoutBinding vkinit::descriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding, uint32_t descriptorCount /*= 1*/) {
	return {
		.binding = binding,
		.descriptorType = type,
		.descriptorCount = descriptorCount,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr,
	};
}

VkSamplerCreateInfo vkinit::sampler_create_info(VkPhysicalDevice physical_device, VkFilter filters, uint32_t mip_levels,
	VkSamplerAddressMode sampler_address_mode /*= VK_SAMPLER_ADDRESS_MODE_REPEAT*/) {

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physical_device, &properties);

	return VkSamplerCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.magFilter = filters,
		.minFilter = filters,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = sampler_address_mode,
		.addressModeV = sampler_address_mode,
		.addressModeW = sampler_address_mode,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = static_cast<float>(mip_levels),
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
	};
}