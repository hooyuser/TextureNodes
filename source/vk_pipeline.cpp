#include "vk_pipeline.h"
#include "vk_mesh.h"
#include "vk_initializers.h"
#include "vk_engine.h"
#include <stdexcept>

namespace engine {
	PipelineBuilder::PipelineBuilder(VulkanEngine* engine, DynamicViewportFlagBits dynamic_viewport, VertexInputFlagBits enable_vertex_input) {
		if (enable_vertex_input == ENABLE_VERTEX_INPUT) {
			bindingDescriptions = Vertex::get_binding_descriptions();
			attributeDescriptions = Vertex::get_attribute_descriptions();
			vertexInput = vkinit::vertexInputStateCreateInfo(bindingDescriptions, attributeDescriptions);
		}
		else if (enable_vertex_input == DISABLE_VERTEX_INPUT) {
			const VkPipelineVertexInputStateCreateInfo vertex_input_info = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				.vertexBindingDescriptionCount = 0,
				.pVertexBindingDescriptions = nullptr,
				.vertexAttributeDescriptionCount = 0,
				.pVertexAttributeDescriptions = nullptr,
			};
			vertexInput = vertex_input_info;
		}
		inputAssembly = vkinit::inputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		if (dynamic_viewport == ENABLE_DYNAMIC_VIEWPORT) {
			viewportState = vkinit::viewportStateCreateInfo(VK_NULL_HANDLE, VK_NULL_HANDLE);
		}
		else if (dynamic_viewport == DISABLE_DYNAMIC_VIEWPORT) {
			/*viewport.x = 0.0f;
			viewport.y = engine->viewport_3d.height;
			viewport.width = engine->viewport_3d.width;
			viewport.height = -engine->viewport_3d.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			scissor.offset = { 0, 0 };
			scissor.extent = { static_cast<uint32_t>(engine->viewport_3d.width), static_cast<uint32_t>(engine->viewport_3d.height) };

			viewportState = vkinit::viewportStateCreateInfo(&viewport, &scissor);*/
		}
		else {
			throw std::runtime_error("failed to set a valid enum value for the parameter dynamic_viewport!");
		}

		if (enable_vertex_input == ENABLE_VERTEX_INPUT) {
			rasterizer = vkinit::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL); // possibly set culling mode
		}
		else {
			rasterizer = vkinit::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
		}

		multisampling = vkinit::multisampling_state_create_info(VK_SAMPLE_COUNT_1_BIT);

		depthStencil = vkinit::depthStencilCreateInfo(VK_COMPARE_OP_LESS);

		colorBlendAttachment = vkinit::color_blend_attachment_state();

		colorBlend = vkinit::colorBlendAttachmentCreateInfo(colorBlendAttachment);

		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.flags = 0;
		dynamicState.pNext = nullptr;
	}

	void PipelineBuilder::set_viewport(float width, float height) {
		viewport.x = 0.0f;
		viewport.y = height;
		viewport.width = width;
		viewport.height = -height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		scissor.offset = { 0, 0 };
		scissor.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		viewportState = vkinit::viewportStateCreateInfo(&viewport, &scissor);
	}

	void PipelineBuilder::set_msaa(VkSampleCountFlagBits msaaSamples) {
		multisampling = vkinit::multisampling_state_create_info(msaaSamples);
	}

	void PipelineBuilder::build_pipeline(const VkDevice& device, const VkRenderPass& renderPass, const VkPipelineLayout& pipelineLayout, VkPipeline& graphicsPipeline) {
		const VkGraphicsPipelineCreateInfo pipeline_info{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = p_next,
			.stageCount = 2,
			.pStages = shaderStages.data(),
			.pVertexInputState = &vertexInput,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = &depthStencil,
			.pColorBlendState = &colorBlend,
			.pDynamicState = &dynamicState,
			.layout = pipelineLayout,
			.renderPass = renderPass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
		};
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}
	}
}