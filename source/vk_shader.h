#pragma once
#include "vk_types.h"
#include "vk_engine.h"
#include <filesystem>
#include <string>


namespace engine {
	std::vector<char> read_file(std::string_view filename);

	struct ShaderModule {
		VkShaderStageFlagBits stage;
		VkShaderModule shader = VK_NULL_HANDLE;
		ShaderModule();
		ShaderModule(VkShaderStageFlagBits stage, VkShaderModule shader);
		ShaderModule(const ShaderModule& shaderModule);
		ShaderModule(ShaderModule&& shaderModule) noexcept;
	};

	class Shader {
		using ShaderPtr = std::shared_ptr<Shader>;
	public:

		VkDevice device = VK_NULL_HANDLE;
		std::vector<ShaderModule> shader_modules;

		Shader(VkDevice device, std::vector<ShaderModule>&& shaderModules);

		static ShaderPtr createFromSpv(VulkanEngine* engine, const std::span<const char* const> spv_file_paths) {
			std::vector<ShaderModule> shaderModuleVector;
			for (auto const& spv_file_path : spv_file_paths) {
				auto spvCode = read_file(spv_file_path);
				VkShaderModuleCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				createInfo.codeSize = spvCode.size();
				createInfo.pCode = reinterpret_cast<const uint32_t*>(spvCode.data());

				ShaderModule shaderModule;
				auto extension = std::filesystem::path(spv_file_path).stem().extension().string();
				if (extension == ".vert") {
					shaderModule.stage = VK_SHADER_STAGE_VERTEX_BIT;
				}
				else if (extension == ".frag") {
					shaderModule.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				}
				else if (extension == ".comp") {
					shaderModule.stage = VK_SHADER_STAGE_COMPUTE_BIT;
				}
				else {
					throw std::runtime_error("Illegal shader name! Please specify the stage!");
				}

				if (vkCreateShaderModule(engine->device, &createInfo, nullptr, &shaderModule.shader) != VK_SUCCESS) {
					throw std::runtime_error("failed to create shader module!");
				}

				shaderModuleVector.emplace_back(std::move(shaderModule));

				engine->main_deletion_queue.push_function([=] {
					vkDestroyShaderModule(engine->device, shaderModule.shader, nullptr);
					});
			}
			return std::make_shared<Shader>(engine->device, std::move(shaderModuleVector));
		}
	};
}

using ShaderPtr = std::shared_ptr<engine::Shader>;