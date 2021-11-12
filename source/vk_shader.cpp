#include "vk_shader.h"

#include <fstream>


namespace engine {
	using ShaderPtr = std::shared_ptr<Shader>;

	ShaderModule::ShaderModule(){}
	ShaderModule::ShaderModule(VkShaderStageFlagBits stage, VkShaderModule shader) :stage(stage), shader(shader) {}
	ShaderModule::ShaderModule(const ShaderModule& shaderModule): stage(shaderModule.stage), shader(shaderModule.shader) {}
	ShaderModule::ShaderModule(ShaderModule&& shaderModule) : stage(shaderModule.stage), shader(shaderModule.shader) {}


	std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	Shader::Shader(VkDevice device, std::vector<ShaderModule>&& shaderModules): device(device), shaderModules(shaderModules){}

}