#pragma once

#include "vk_material.h"

class VulkanEngine;

namespace engine {
	enum DynamicViewportFlagBits {
		ENABLE_DYNAMIC_VIEWPORT = 0x00000001,
		DISABLE_DYNAMIC_VIEWPORT = 0x00000002,
	};

	enum VertexInputFlagBits {
		ENABLE_VERTEX_INPUT = 0x00000001,
		DISABLE_VERTEX_INPUT = 0x00000002,
	};

	class PipelineBuilder {
	public:
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		VkPipelineVertexInputStateCreateInfo vertexInput;
		VkPipelineInputAssemblyStateCreateInfo inputAssembly;
		VkViewport viewport;
		VkRect2D scissor;
		VkPipelineViewportStateCreateInfo viewportState;
		VkPipelineRasterizationStateCreateInfo rasterizer;
		VkPipelineColorBlendStateCreateInfo colorBlend;
		VkPipelineMultisampleStateCreateInfo multisampling;
		VkPipelineDepthStencilStateCreateInfo depthStencil;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		std::vector<VkSpecializationMapEntry> specializationMapEntries;
		VkSpecializationInfo specializationInfo;
		VkPipelineDynamicStateCreateInfo dynamicState;
		void* p_next = VK_NULL_HANDLE;

		const std::array<VkDynamicState, 2> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		explicit PipelineBuilder(VulkanEngine* engine, DynamicViewportFlagBits dynamic_viewport = ENABLE_DYNAMIC_VIEWPORT, VertexInputFlagBits enable_vertex_input = ENABLE_VERTEX_INPUT);

		void set_viewport(float width, float height);

		void set_msaa(VkSampleCountFlagBits msaaSamples);

		void build_pipeline(const VkDevice& device, const VkRenderPass& pass, const VkPipelineLayout& pipelineLayout, VkPipeline& pipeline);

		template<typename ParaT>
		void set_shader_stages(const std::shared_ptr<Material<ParaT>>& pMaterial) {
			shaderStages.clear();
			for (auto shader_module : pMaterial->shaders->shader_modules) {
				if (shader_module.stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
					constexpr auto para_num = ParaT::Class::TotalFields;
					specializationMapEntries.resize(para_num);

					for (uint32_t i = 0; i < para_num; i++) {
						ParaT::Class::FieldAt(pMaterial->paras, i, [&](auto& field, auto& value) {
							specializationMapEntries[i].constantID = i;
							specializationMapEntries[i].offset = static_cast<uint32_t>(field.getOffset());
							specializationMapEntries[i].size = sizeof(value);
							});
					}

					specializationInfo.mapEntryCount = para_num;
					specializationInfo.pMapEntries = specializationMapEntries.data();
					specializationInfo.dataSize = sizeof(ParaT);
					specializationInfo.pData = &pMaterial->paras;

					shaderStages.emplace_back(VkPipelineShaderStageCreateInfo {
						.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
						.pNext = nullptr,
						.stage = shader_module.stage,
						.module = shader_module.shader,
						.pName = "main",
						.pSpecializationInfo = &specializationInfo,
					});
				}
				else {
					shaderStages.emplace_back(VkPipelineShaderStageCreateInfo {
						.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
						.pNext = nullptr,
						.stage = shader_module.stage,
						.module = shader_module.shader,
						.pName = "main",
					});
				}
			}
		}
	};

}


