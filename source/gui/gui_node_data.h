#pragma once

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

template <typename T, typename FieldType>
struct has_field_type {
	static inline constexpr bool value = []() {
		bool hadField = false;
		for (size_t i = 0; i < Reflect::class_t<T>::TotalFields; i++) {
			Reflect::class_t<T>::FieldAt(i, [&](auto& field) {
				using CurrFieldType = typename std::remove_reference_t<decltype(field)>::Type;
				if constexpr (std::is_same_v<FieldType, CurrFieldType>) {
					hadField = true;
				}
				});
		}
		return hadField;
	}();
};
template <typename T, typename FieldType>
static constexpr bool has_field_type_v = has_field_type<T, FieldType>::value;

struct NodeData {};

struct IntData : public NodeData {
	int32_t value = 0;
	//IntData(VulkanEngine* engine = nullptr) {};
};

struct FloatData : public NodeData {
	float value = 0.0f;
	//FloatData(VulkanEngine* engine = nullptr) {};
};

struct Float4Data : public NodeData {
	float value[4] = { 0.0f };
	//Float4Data(VulkanEngine* engine = nullptr) {};
};

struct Color4Data : public NodeData {
	float value[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	//Color4Data(VulkanEngine* engine = nullptr) {};
};

struct BoolData : public NodeData {
	bool value;
};

struct TextureIdData : public NodeData {
	int value = -1;
};

using PinVariant = std::variant<
	TextureIdData,
	FloatData,
	IntData,
	BoolData,
	Color4Data
>;

template<typename UniformBufferType, StringLiteral ...Shaders>
struct ImageData : public NodeData {
	using UboType = UniformBufferType;

	VulkanEngine* engine;
	BufferPtr uniform_buffer;
	TexturePtr texture;
	void* gui_texture;

	VkDescriptorSet ubo_descriptor_set;
	VkFramebuffer image_pocessing_framebuffer;
	VkCommandBuffer node_cmd_buffer;
	VkFence fence;

	int node_texture_id = -1;
	uint32_t width = 1024;
	uint32_t height = 1024;

	inline static VkDescriptorSetLayout descriptor_set_layout = nullptr;
	inline static VkPipelineLayout image_pocessing_pipeline_layout = nullptr;
	inline static VkPipeline image_pocessing_pipeline = nullptr;
	inline static VkRenderPass image_pocessing_render_pass = nullptr;

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
	ImageData(VulkanEngine* engine) :engine(engine) {
		auto fenceInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		if (vkCreateFence(engine->device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
			throw std::runtime_error("failed to create fence!");
		}

		texture = engine::Texture::create_2D_render_target(engine,
			1024,
			1024,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT);

		texture->transitionImageLayout(engine, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		gui_texture = ImGui_ImplVulkan_AddTexture(texture->sampler, texture->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		node_texture_id = engine->node_texture_manager.get_id();

		uniform_buffer = engine::Buffer::createBuffer(engine,
			sizeof(UboType),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			SWAPCHAIN_INDEPENDENT_BIT);

		if (descriptor_set_layout == nullptr) {
			auto ubo_layout_binding = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
			std::array layout_bindings = { ubo_layout_binding };
			engine->create_descriptor_set_layout(layout_bindings, descriptor_set_layout);
		}

		VkDescriptorSetAllocateInfo descriptor_alloc_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = engine->node_texture_manager.descriptor_pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &descriptor_set_layout,
		};

		if (vkAllocateDescriptorSets(engine->device, &descriptor_alloc_info, &ubo_descriptor_set) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		VkDescriptorBufferInfo uniform_buffer_info{
			.buffer = uniform_buffer->buffer,
			.offset = 0,
			.range = sizeof(UboType)
		};

		VkDescriptorImageInfo image_info{
			.sampler = texture->sampler,
			.imageView = texture->imageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		std::array descriptor_writes{
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = ubo_descriptor_set,
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &uniform_buffer_info,
			},
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = engine->node_texture_manager.descriptor_set,
				.dstBinding = 0,
				.dstArrayElement = static_cast<uint32_t>(node_texture_id),
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &image_info,
			},
		};

		vkUpdateDescriptorSets(engine->device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

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

			std::array dependencies{
				VkSubpassDependency {
					.srcSubpass = VK_SUBPASS_EXTERNAL,
					.dstSubpass = 0,
					.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
									VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
					.srcAccessMask = 0,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
									 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.dependencyFlags = 0,
				},
				VkSubpassDependency {
					.srcSubpass = 0,
					.dstSubpass = VK_SUBPASS_EXTERNAL,
					.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT,
					.dependencyFlags = 0,
				}
			};

			std::array attachments = { colorAttachment };

			VkRenderPassCreateInfo render_pass_info{
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
				.pNext = nullptr,
				.attachmentCount = static_cast<uint32_t>(attachments.size()),
				.pAttachments = attachments.data(),
				.subpassCount = 1,
				.pSubpasses = &subpass,
				.dependencyCount = static_cast<uint32_t>(dependencies.size()),
				.pDependencies = dependencies.data(),
			};

			if (vkCreateRenderPass(engine->device, &render_pass_info, nullptr, &image_pocessing_render_pass) != VK_SUCCESS) {
				throw std::runtime_error("failed to create render pass!");
			}

			engine->main_deletion_queue.push_function([=]() {
				vkDestroyRenderPass(engine->device, image_pocessing_render_pass, nullptr);
				});
		}

		if (image_pocessing_pipeline_layout == nullptr) {

			auto descriptor_set_layouts = constexpr_if<has_field_type_v<UboType, TextureIdData>>(
				std::array{ descriptor_set_layout, engine->node_texture_manager.descriptor_set_layout },
				std::array{ descriptor_set_layout }
			);

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

		//create framebuffer

		std::array framebuffer_attachments = { texture->imageView };

		VkFramebufferCreateInfo framebufferInfo = vkinit::framebufferCreateInfo(image_pocessing_render_pass, VkExtent2D{ width , height }, framebuffer_attachments);

		if (vkCreateFramebuffer(engine->device, &framebufferInfo, nullptr, &image_pocessing_framebuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}

		//create command buffer

		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(engine->commandPool, 1);

		if (vkAllocateCommandBuffers(engine->device, &cmdAllocInfo, &node_cmd_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
		};

		if (vkBeginCommandBuffer(node_cmd_buffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkExtent2D image_extent{ width, height };

		VkRenderPassBeginInfo renderPassInfo = vkinit::renderPassBeginInfo(image_pocessing_render_pass, image_extent, image_pocessing_framebuffer);

		vkCmdBeginRenderPass(node_cmd_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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

		auto descriptor_sets = constexpr_if<has_field_type_v<UboType, TextureIdData>>(
			std::array{ ubo_descriptor_set, engine->node_texture_manager.descriptor_set },
			std::array{ ubo_descriptor_set }
		);

		vkCmdSetViewport(node_cmd_buffer, 0, 1, &viewport);
		vkCmdSetScissor(node_cmd_buffer, 0, 1, &scissor);
		vkCmdBindDescriptorSets(node_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, image_pocessing_pipeline_layout, 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
		vkCmdBindPipeline(node_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, image_pocessing_pipeline);
		vkCmdDraw(node_cmd_buffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(node_cmd_buffer);

		if (vkEndCommandBuffer(node_cmd_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	~ImageData() {
		engine->node_texture_manager.delete_id(node_texture_id);
		vkFreeCommandBuffers(engine->device, engine->commandPool, 1, &node_cmd_buffer);
		vkDestroyFramebuffer(engine->device, image_pocessing_framebuffer, nullptr);
		vkFreeDescriptorSets(engine->device, engine->node_texture_manager.descriptor_pool, 1, &ubo_descriptor_set);
		vkDestroyFence(engine->device, fence, nullptr);
	}

	//template<typename T>
	void update_ubo(const PinVariant& value, size_t index) {
		UboType::Class::FieldAt(index, [&](auto& field) {
			using PinT = std::decay_t<decltype(field)>::Type;
			uniform_buffer->copyFromHost(reinterpret_cast<const char*>(&std::get<PinT>(value)), sizeof(PinT), field.getOffset());
			});
	}
	//VkCommandBuffer get_node_cmd_buffer() {
	//	return node_cmd_buffer;
	//}
};

template<typename UboType, StringLiteral ...Shaders>
using ImageDataPtr = std::shared_ptr<ImageData<UboType, Shaders...>>;



