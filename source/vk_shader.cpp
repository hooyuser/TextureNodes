#include "vk_shader.h"

#include <fstream>


namespace engine {
	using ShaderPtr = std::shared_ptr<Shader>;

	ShaderModule::ShaderModule(){}
	ShaderModule::ShaderModule(VkShaderStageFlagBits stage, VkShaderModule shader) :stage(stage), shader(shader) {}
	ShaderModule::ShaderModule(const ShaderModule& shaderModule): stage(shaderModule.stage), shader(shaderModule.shader) {}
	ShaderModule::ShaderModule(ShaderModule&& shaderModule) noexcept: stage(shaderModule.stage), shader(shaderModule.shader) {}


	std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		auto const file_size = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(file_size);

		file.seekg(0);
		file.read(buffer.data(), file_size);

		file.close();

		return buffer;
	}

	Shader::Shader(VkDevice device, std::vector<ShaderModule>&& shaderModules): device(device), shader_modules(shaderModules){}

}