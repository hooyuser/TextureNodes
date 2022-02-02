#pragma once
#include <vector>
#include <string>
#include <array>
#include <variant>

#include "../vk_image.h"
#include "../vk_buffer.h"
#include "../vk_pipeline.h"
#include "../vk_initializers.h"
#include "../vk_engine.h"

class VulkanEngine;

namespace engine {
	class Shader;
}

namespace ed = ax::NodeEditor;

template<size_t N>
struct StringLiteral {
	constexpr StringLiteral(const char(&str)[N]) {
		std::copy_n(str, N, value);
	}

	char value[N];
};

struct NodeData {};

template<typename UboType, StringLiteral ...Shaders>
struct ImageData : public NodeData {
	TexturePtr texture;
	BufferPtr uniform_buffer;
	inline static VkDescriptorSetLayout descriptor_set_layout = nullptr;
	inline static VkPipelineLayout image_pocessing_pipeline_layout = nullptr;
	inline static VkPipeline image_pocessing_pipeline = nullptr;
	VkDescriptorSet descriptor_set;
	UboType ubo;
	//ImageData(const ImageData& data) {
	//	texture = data.texture;
	//	uniform_buffer = data.uniform_buffer;
	//	descriptor_set = data.descriptor_set;
	//}
	//ImageData& operator=(const ImageData& data){
	//	texture = data.texture;
	//	uniform_buffer = data.uniform_buffer;
	//	descriptor_set = data.descriptor_set;
	//	return *this;
	//}
	//ImageData(){};
	ImageData(VulkanEngine* engine) {
		texture = engine::Texture::create_2D_render_target(engine,
			1024,
			1024,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_ASPECT_COLOR_BIT);

		uniform_buffer = engine::Buffer::createBuffer(engine,
			sizeof(UboType),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			SWAPCHAIN_INDEPENDENT_BIT);

		if (descriptor_set_layout == nullptr) {
			auto ubo_layout_binding = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
			std::array layout_bindings = { ubo_layout_binding };
			engine->create_descriptor_set_layout(layout_bindings, descriptor_set_layout);
		}

		VkDescriptorSetAllocateInfo descriptor_alloc_info{};
		descriptor_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptor_alloc_info.descriptorPool = engine->descriptorPool;
		descriptor_alloc_info.descriptorSetCount = 1;
		descriptor_alloc_info.pSetLayouts = &descriptor_set_layout;
		if (vkAllocateDescriptorSets(engine->device, &descriptor_alloc_info, &descriptor_set) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = uniform_buffer->buffer;
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(UboType);

		engine::PipelineBuilder pipeline_builder(engine);

		if (image_pocessing_pipeline_layout == nullptr) {
			std::array descriptor_set_layouts = { descriptor_set_layout };

			VkPipelineLayoutCreateInfo image_pocessing_pipeline_info = vkinit::pipelineLayoutCreateInfo(descriptor_set_layouts);

			if (vkCreatePipelineLayout(engine->device, &image_pocessing_pipeline_info, nullptr, &image_pocessing_pipeline_layout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create pipeline layout!");
			}

			engine->swapChainDeletionQueue.push_function([=]() {
				vkDestroyPipelineLayout(engine->device, image_pocessing_pipeline_layout, nullptr);
				});
		}

		std::array<std::string, sizeof...(Shaders)> shader_files;;
		size_t i = 0;
		((shader_files[i++] = Shaders.value), ...);
		auto shaders = engine::Shader::createFromSpv(engine, shader_files);

		for (auto shader_module : shaders->shaderModules) {
			VkPipelineShaderStageCreateInfo shader_info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
			shader_info.pNext = nullptr;
			shader_info.stage = shader_module.stage;
			shader_info.module = shader_module.shader;
			shader_info.pName = "main";
			pipeline_builder.shaderStages.emplace_back(std::move(shader_info));
		}
		pipeline_builder.buildPipeline(engine->device, VK_NULL_HANDLE, image_pocessing_pipeline_layout, image_pocessing_pipeline);
	}
};

template<typename UboType, StringLiteral ...Shaders>
using ImageDataPtr = std::shared_ptr<ImageData<UboType, Shaders...>>;

struct FloatData : public NodeData {
	float value = 0.0f;
	FloatData(VulkanEngine* engine = nullptr) {};
};

struct Float4Data : public NodeData {
	float value[4] = { 0.0f };
	Float4Data(VulkanEngine* engine = nullptr) {};
};

