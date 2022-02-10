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
	inline static VkRenderPass image_pocessing_render_pass = nullptr;
	inline static VkFramebuffer image_pocessing_framebuffer = nullptr;
	inline static VkCommandBuffer command_buffer = nullptr;
	VkDescriptorSet descriptor_set;
	UboType ubo;
	uint32_t width = 1024;
	uint32_t height = 1024;
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
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT);

		texture->transitionImageLayout(engine, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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

		VkDescriptorSetAllocateInfo descriptor_alloc_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = engine->descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &descriptor_set_layout
		};

		if (vkAllocateDescriptorSets(engine->device, &descriptor_alloc_info, &descriptor_set) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		VkDescriptorBufferInfo uniformBufferInfo{
			.buffer = uniform_buffer->buffer,
			.offset = 0,
			.range = sizeof(UboType)
		};

		VkWriteDescriptorSet descriptor_write{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptor_set,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &uniformBufferInfo,
		};

		vkUpdateDescriptorSets(engine->device, 1, &descriptor_write, 0, nullptr);

		//create renderpass
		if (image_pocessing_render_pass == nullptr) {
			VkAttachmentDescription colorAttachment{
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			VkAttachmentReference colorAttachmentRef{
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};

			VkSubpassDescription subpass{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &colorAttachmentRef,
			};

			std::array<VkSubpassDependency, 2> dependencies;

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependencies[0].srcAccessMask = 0;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = 0;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
			dependencies[1].dependencyFlags = 0;

			std::array attachments = { colorAttachment };

			VkRenderPassCreateInfo renderPassInfo{
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
				.pNext = nullptr,
				.attachmentCount = static_cast<uint32_t>(attachments.size()),
				.pAttachments = attachments.data(),
				.subpassCount = 1,
				.pSubpasses = &subpass,
				.dependencyCount = static_cast<uint32_t>(dependencies.size()),
				.pDependencies = dependencies.data(),
			};

			if (vkCreateRenderPass(engine->device, &renderPassInfo, nullptr, &image_pocessing_render_pass) != VK_SUCCESS) {
				throw std::runtime_error("failed to create render pass!");
			}

			engine->main_deletion_queue.push_function([=]() {
				vkDestroyRenderPass(engine->device, image_pocessing_render_pass, nullptr);
				});
		}

		if (image_pocessing_pipeline_layout == nullptr) {
			std::array descriptor_set_layouts = { descriptor_set_layout };

			VkPipelineLayoutCreateInfo image_pocessing_pipeline_info = vkinit::pipelineLayoutCreateInfo(descriptor_set_layouts);

			if (vkCreatePipelineLayout(engine->device, &image_pocessing_pipeline_info, nullptr, &image_pocessing_pipeline_layout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create pipeline layout!");
			}

			engine->main_deletion_queue.push_function([=]() {
				vkDestroyPipelineLayout(engine->device, image_pocessing_pipeline_layout, nullptr);
				});
		}

		if (image_pocessing_pipeline == nullptr) {
			engine::PipelineBuilder pipeline_builder(engine, engine::ENABLE_DYNAMIC_VIEWPORT, engine::DISABLE_VERTEX_INPUT);
			std::array<std::string, sizeof...(Shaders)> shader_files;;
			size_t i = 0;
			((shader_files[i++] = Shaders.value), ...);
			auto shaders = engine::Shader::createFromSpv(engine, shader_files);

			for (auto shader_module : shaders->shaderModules) {
				VkPipelineShaderStageCreateInfo shader_info{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.pNext = nullptr,
					.stage = shader_module.stage,
					.module = shader_module.shader,
					.pName = "main"
				};
				pipeline_builder.shaderStages.emplace_back(std::move(shader_info));
			}
			pipeline_builder.buildPipeline(engine->device, image_pocessing_render_pass, image_pocessing_pipeline_layout, image_pocessing_pipeline);
			engine->main_deletion_queue.push_function([=]() {
				vkDestroyPipeline(engine->device, image_pocessing_pipeline, nullptr);
				});
		}

		if (image_pocessing_framebuffer == nullptr) {
			std::array framebuffer_attachments = { texture->imageView };

			VkFramebufferCreateInfo framebufferInfo = vkinit::framebufferCreateInfo(image_pocessing_render_pass, VkExtent2D{ width , height }, framebuffer_attachments);

			if (vkCreateFramebuffer(engine->device, &framebufferInfo, nullptr, &image_pocessing_framebuffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}

			engine->main_deletion_queue.push_function([=]() {
				vkDestroyFramebuffer(engine->device, image_pocessing_framebuffer, nullptr);
				});
		}

		if (command_buffer == nullptr) {
			VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(engine->commandPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY);

			if (vkAllocateCommandBuffers(engine->device, &cmdAllocInfo, &command_buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate command buffers!");
			}

			engine->main_deletion_queue.push_function([=]() {
				vkFreeCommandBuffers(engine->device, engine->commandPool, 1, &command_buffer);
				});

			VkCommandBufferBeginInfo beginInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
			};

			if (vkBeginCommandBuffer(command_buffer, &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			VkExtent2D image_extent{ width, height };

			VkRenderPassBeginInfo renderPassInfo = vkinit::renderPassBeginInfo(image_pocessing_render_pass, image_extent, image_pocessing_framebuffer);

			vkCmdBeginRenderPass(command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport{
				.x = 0.0f,
				.y = static_cast<float>(height),
				.width = static_cast<float>(width),
				.height = -static_cast<float>(height),    // flip y axis
				.minDepth = 0.0f,
				.maxDepth = 1.0f,
			};

			VkRect2D scissor{
				.offset = { 0, 0 },
				.extent = image_extent,
			};

			vkCmdSetViewport(command_buffer, 0, 1, &viewport);
			vkCmdSetScissor(command_buffer, 0, 1, &scissor);
			vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, image_pocessing_pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
			vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, image_pocessing_pipeline);
			vkCmdDraw(command_buffer, 3, 1, 0, 0);
			vkCmdEndRenderPass(command_buffer);

			if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}

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

