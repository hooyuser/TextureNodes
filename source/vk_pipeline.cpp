#include "vk_pipeline.h"
#include "vk_mesh.h"
#include "vk_initializers.h"
#include "vk_engine.h"

namespace engine {
	PipelineBuilder::PipelineBuilder(VulkanEngine* engine, DynamicViewportFlagBits dynamic_viewport, VertexInputFlagBits enable_vertex_input) {
		if (enable_vertex_input == ENABLE_VERTEX_INPUT) {
			bindingDescriptions = Vertex::getBindingDescriptions();
			attributeDescriptions = Vertex::getAttributeDescriptions();
			vertexInput = vkinit::vertexInputStateCreateInfo(bindingDescriptions, attributeDescriptions);
		}
		else if (enable_vertex_input == DISABLE_VERTEX_INPUT) {
			VkPipelineVertexInputStateCreateInfo vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
			vertexInput = vertexInputInfo;
		}
		inputAssembly = vkinit::inputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		if (dynamic_viewport == ENABLE_DYNAMIC_VIEWPORT) {
			viewportState = vkinit::viewportStateCreateInfo(VK_NULL_HANDLE, VK_NULL_HANDLE);
		}
		else if (dynamic_viewport == DISABLE_DYNAMIC_VIEWPORT) {
			viewport.x = 0.0f;
			viewport.y = engine->viewport3D.height;
			viewport.width = engine->viewport3D.width;
			viewport.height = -engine->viewport3D.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			scissor.offset = { 0, 0 };
			scissor.extent = { static_cast<uint32_t>(engine->viewport3D.width), static_cast<uint32_t>(engine->viewport3D.height) };

			viewportState = vkinit::viewportStateCreateInfo(&viewport, &scissor);
		}
		else {
			throw std::runtime_error("failed to set a valid enum value for the parameter dynamic_viewport!");
		}

		if (enable_vertex_input == ENABLE_VERTEX_INPUT) {
			rasterizer = vkinit::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
		}
		else {
			rasterizer = vkinit::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT);
		}

		multisampling = vkinit::multisamplingStateCreateInfo(engine->msaaSamples);

		depthStencil = vkinit::depthStencilCreateInfo(VK_COMPARE_OP_LESS);

		colorBlendAttachment = vkinit::colorBlendAttachmentState();

		colorBlend = vkinit::colorBlendAttachmentCreateInfo(colorBlendAttachment);

		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.flags = 0;
		dynamicState.pNext = nullptr;
	}

	void PipelineBuilder::buildPipeline(const VkDevice& device, const VkRenderPass& renderPass, const VkPipelineLayout& pipelineLayout, VkPipeline& graphicsPipeline) {
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInput;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlend;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}
	}
}