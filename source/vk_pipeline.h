#pragma once

#include "vk_types.h"
#include "vk_material.h"

class VulkanEngine;

namespace engine {
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

		PipelineBuilder(VulkanEngine* engine);

		template<typename ParaT>
		void setShaderStages(std::shared_ptr<Material<ParaT>> pMaterial) {
			shaderStages.clear();
			for (auto shaderModule : pMaterial->pShaders->shaderModules) {
				if (shaderModule.stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
					constexpr auto paraNum = ParaT::Class::TotalFields;
					specializationMapEntries.resize(paraNum);

					for (auto i = 0u; i < paraNum; i++) {
						ParaT::Class::FieldAt(pMaterial->paras, i, [&](auto& field, auto& value) {
							specializationMapEntries[i].constantID = i;
							specializationMapEntries[i].offset = field.getOffset();
							specializationMapEntries[i].size = sizeof(value);
							});
					}

					specializationInfo.mapEntryCount = paraNum;
					specializationInfo.pMapEntries = specializationMapEntries.data();
					specializationInfo.dataSize = sizeof(ParaT);
					specializationInfo.pData = &pMaterial->paras;

					VkPipelineShaderStageCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };

					info.pNext = nullptr;
					info.stage = shaderModule.stage;
					info.module = shaderModule.shader;
					info.pName = "main";
					info.pSpecializationInfo = &specializationInfo;

					shaderStages.emplace_back(std::move(info));
				}
				else {
					VkPipelineShaderStageCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };

					info.pNext = nullptr;
					info.stage = shaderModule.stage;
					info.module = shaderModule.shader;
					info.pName = "main";

					shaderStages.emplace_back(std::move(info));
				}
			}
		}

		void buildPipeline(const VkDevice& device, const VkRenderPass& pass, const VkPipelineLayout& pipelineLayout, VkPipeline& pipeline);
	};
}


