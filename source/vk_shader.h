#pragma once
#include "vk_types.h"
#include "vk_engine.h"
#include <filesystem>
#include <string>


namespace engine {
	std::vector<char> readFile(const std::string& filename);

	struct ShaderModule {
		VkShaderStageFlagBits stage;
		VkShaderModule shader = VK_NULL_HANDLE;
		ShaderModule();
		ShaderModule(VkShaderStageFlagBits stage, VkShaderModule shader);
		ShaderModule(const ShaderModule& shaderModule);
		ShaderModule(ShaderModule&& shaderModule);
	};

	class Shader {
		using ShaderPtr = std::shared_ptr<Shader>;
	public:

		VkDevice device = VK_NULL_HANDLE;
		std::vector<ShaderModule> shaderModules;

		Shader(VkDevice device, std::vector<ShaderModule>&& shaderModules);

		static ShaderPtr createFromSpv(VulkanEngine* engine, Matrix<char> auto const& spvFilePaths) {
			std::vector<ShaderModule> shaderModuleVector;
			for (auto const& spvFilePath : spvFilePaths) {
				auto spvCode = readFile(std::string{ spvFilePath });
				VkShaderModuleCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				createInfo.codeSize = spvCode.size();
				createInfo.pCode = reinterpret_cast<const uint32_t*>(spvCode.data());

				ShaderModule shaderModule;
				auto extention = std::filesystem::path(spvFilePath).stem().extension().string();
				if (extention == ".vert") {
					shaderModule.stage = VK_SHADER_STAGE_VERTEX_BIT;
				}
				else if (extention == ".frag") {
					shaderModule.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				}
				else {
					throw std::runtime_error("Illegal shader name! Please specify the stage!");
				}

				if (vkCreateShaderModule(engine->device, &createInfo, nullptr, &shaderModule.shader) != VK_SUCCESS) {
					throw std::runtime_error("failed to create shader module!");
				}

				shaderModuleVector.emplace_back(std::move(shaderModule));

				engine->main_deletion_queue.push_function([=]() {
					vkDestroyShaderModule(engine->device, shaderModule.shader, nullptr);
					});
			}
			return std::make_shared<Shader>(engine->device, std::move(shaderModuleVector));
		}
	};
}

using ShaderPtr = std::shared_ptr<engine::Shader>;