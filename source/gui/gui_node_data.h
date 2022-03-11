#pragma once

#include "../vk_image.h"
#include "../vk_buffer.h"
#include "../vk_pipeline.h"
#include "../vk_initializers.h"
#include "../vk_engine.h"
#include "gui_node_texture_manager.h"

#include "imgui_color_gradient.h"
#include <imgui_impl_vulkan.h>


constexpr static inline uint32_t preview_image_size = 128;

namespace engine {
	class Shader;
}

template<size_t N>
struct StringLiteral {
	constexpr StringLiteral(const char(&str)[N]) {
		std::copy_n(str, N, value);
	}

	char value[N];
};

template <typename T, typename FieldType>
struct count_field_type {
	static inline constexpr size_t value = []() {
		size_t field_count = 0;
		for (size_t i = 0; i < Reflect::class_t<T>::TotalFields; i++) {
			Reflect::class_t<T>::FieldAt(i, [&](auto& field) {
				using CurrFieldType = typename std::remove_reference_t<decltype(field)>::Type;
				if constexpr (std::is_same_v<FieldType, CurrFieldType>) {
					++field_count;
				}
				});
		}
		return field_count;
	}();
};
template <typename T, typename FieldType>
static constexpr size_t count_field_type_v = count_field_type<T, FieldType>::value;

template <typename T, typename FieldType>
static constexpr bool has_field_type_v = (count_field_type_v<T, FieldType>) > 0;

//Define all kinds of NodeData, which serves as UBO member type or subtype of PinVaraint 
struct NodeData {};

struct IntData : public NodeData {
	using value_t = int32_t;
	value_t value = 0;
};

struct FloatData : public NodeData {
	using value_t = float;
	value_t value = 0.0f;
};

struct Float4Data : public NodeData {
	using value_t = float[4];
	value_t value = { 0.0f };
};

struct Color4Data : public NodeData {
	using value_t = float[4];
	value_t value = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct BoolData : public NodeData {
	using value_t = bool;
	value_t value;
};

struct EnumData : public NodeData {
	using value_t = uint32_t;
	value_t value;
};

struct TextureIdData : public NodeData {
	using value_t = int32_t;
	value_t value = -1;
};

struct RampTexture {
	VulkanEngine* engine;
	VkImage image;
	VkImageView image_view;
	VkSampler sampler;
	VkDeviceMemory memory;
	engine::Buffer staging_buffer;
	VkCommandBuffer command_buffer;
	uint32_t color_ramp_texture_id;

	RampTexture(VulkanEngine* engine);
	~RampTexture();
	void create_command_buffer();

};

struct ColorRampData : public NodeData {
	using value_t = int32_t;
	value_t value = -1;
	std::unique_ptr<ImGradient> ui_value;
	std::unique_ptr<RampTexture> ubo_value;

	inline ColorRampData() {}
	inline ColorRampData(VulkanEngine* engine) {
		ubo_value = std::make_unique<RampTexture>(engine);
		value = ubo_value->color_ramp_texture_id;
		void* data;
		vkMapMemory(engine->device, ubo_value->staging_buffer.memory, 0, ubo_value->staging_buffer.size, 0, &data);
		ui_value = std::make_unique<ImGradient>(static_cast<float*>(data));
	}
	//inline ColorRampData(ColorRampData&&ramp_data) noexcept :ui_value(std::move(ramp_data.ui_value)), ubo_value(std::move(ramp_data.ubo_value)){}
	//inline ColorRampData& operator=(ColorRampData&& ramp_data) noexcept {
	//	ui_value = std::move(ramp_data.ui_value);
	//	ubo_value = std::move(ramp_data.ubo_value);
	//	return *this;
	//}
};

inline void to_json(json& j, const ColorRampData& p) {
	j = *p.ui_value;
}

namespace nlohmann {
	template <typename T> requires std::derived_from<T, NodeData>
	struct adl_serializer<T> {
		static void to_json(json& j, const T& opt) {
			if constexpr (std::same_as<T, ColorRampData>) {
				j = *(opt.ui_value);
			}
			else {
				j = opt.value;
			}

		}
	};

	template <typename ...Args>
	struct adl_serializer<std::variant<Args...>> {
		static void to_json(json& j, std::variant<Args...> const& var) {
			std::visit([&](auto&& value) {
				j = FWD(value);
				}, var);
		}
	};
}

using PinVariant = std::variant<
	TextureIdData,
	FloatData,
	IntData,
	BoolData,
	Color4Data,
	EnumData,
	ColorRampData>;

template<typename UniformBufferType, StringLiteral ...Shaders>
struct ImageData : public NodeData {
	using UboType = UniformBufferType;

	VulkanEngine* engine;
	BufferPtr uniform_buffer;
	TexturePtr texture;
	TexturePtr preview_texture;
	void* gui_texture;
	void* gui_preview_texture;

	VkDescriptorSet ubo_descriptor_set;
	VkFramebuffer image_pocessing_framebuffer;
	VkCommandBuffer image_pocessing_cmd_buffer;
	VkCommandBuffer generate_preview_cmd_buffer;

	VkSemaphore semaphore;

	std::vector<VkSemaphoreSubmitInfo> wait_semaphore_submit_info1;
	VkCommandBufferSubmitInfo cmd_buffer_submit_info1;
	VkSemaphoreSubmitInfo signal_semaphore_submit_info1;

	VkSemaphoreSubmitInfo wait_semaphore_submit_info2;
	VkCommandBufferSubmitInfo cmd_buffer_submit_info2;
	VkSemaphoreSubmitInfo signal_semaphore_submit_info2;

	std::array<VkSubmitInfo2, 2> submit_info;

	VkImageMemoryBarrier2 image_memory_barrier;
	VkDependencyInfo dependency_info;

	int node_texture_id = -1;

	uint32_t width = 1024;
	uint32_t height = 1024;

	inline static VkDescriptorSetLayout ubo_descriptor_set_layout = nullptr;
	inline static VkPipelineLayout image_pocessing_pipeline_layout = nullptr;
	inline static VkPipeline image_pocessing_pipeline = nullptr;
	inline static VkRenderPass image_pocessing_render_pass = nullptr;

	ImageData(VulkanEngine* engine) :engine(engine) {

		create_semaphore();

		create_texture();

		uniform_buffer = engine::Buffer::createBuffer(engine,
			sizeof(UboType),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			SWAPCHAIN_INDEPENDENT_BIT);

		if (ubo_descriptor_set_layout == nullptr) {
			create_ubo_descriptor_set_layout();
		}

		create_ubo_descriptor_set();

		update_descriptor_sets();

		if (image_pocessing_render_pass == nullptr) {
			create_image_pocessing_render_pass();
			create_image_pocessing_pipeline_layout();
			create_image_pocessing_pipeline();
		}

		create_framebuffer();

		create_image_pocessing_command_buffer();

		create_preview_command_buffer();

		create_cmd_buffer_submit_info();
	}

	~ImageData() {
		engine->node_texture_2d_manager->delete_id(node_texture_id);
		std::array cmd_buffers{ image_pocessing_cmd_buffer, generate_preview_cmd_buffer };
		vkFreeCommandBuffers(engine->device, engine->commandPool, cmd_buffers.size(), cmd_buffers.data());
		vkDestroyFramebuffer(engine->device, image_pocessing_framebuffer, nullptr);
		vkFreeDescriptorSets(engine->device, engine->node_descriptor_pool, 1, &ubo_descriptor_set);
		vkDestroySemaphore(engine->device, semaphore, nullptr);
	}

	void create_semaphore() {
		VkSemaphoreTypeCreateInfo timeline_semaphore_create_info{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
			.pNext = NULL,
			.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
			.initialValue = 0,
		};

		VkSemaphoreCreateInfo semaphore_create_info{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = &timeline_semaphore_create_info,
			.flags = 0,
		};

		vkCreateSemaphore(engine->device, &semaphore_create_info, NULL, &semaphore);
	}

	void create_texture() {
		texture = engine::Texture::create_device_texture(engine,
			width,
			height,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

		texture->transitionImageLayout(engine, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		preview_texture = engine::Texture::create_device_texture(engine,
			preview_image_size,
			preview_image_size,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

		preview_texture->transitionImageLayout(engine, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


		gui_texture = ImGui_ImplVulkan_AddTexture(texture->sampler, texture->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		gui_preview_texture = ImGui_ImplVulkan_AddTexture(preview_texture->sampler, preview_texture->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		node_texture_id = engine->node_texture_2d_manager->get_id();
	}

	void create_ubo_descriptor_set_layout() {
		auto ubo_layout_binding = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
		std::array layout_bindings = { ubo_layout_binding };
		engine->create_descriptor_set_layout(layout_bindings, ubo_descriptor_set_layout);
	}

	void create_ubo_descriptor_set() {
		VkDescriptorSetAllocateInfo ubo_descriptor_alloc_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = engine->node_descriptor_pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &ubo_descriptor_set_layout,
		};

		if (vkAllocateDescriptorSets(engine->device, &ubo_descriptor_alloc_info, &ubo_descriptor_set) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
	}

	void update_descriptor_sets() {
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
				.dstSet = engine->node_texture_2d_manager->descriptor_set,
				.dstBinding = 0,
				.dstArrayElement = static_cast<uint32_t>(node_texture_id),
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &image_info,
			},
		};

		vkUpdateDescriptorSets(engine->device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

	}

	void create_image_pocessing_render_pass() {
		VkAttachmentDescription colorAttachment{
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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
				.srcStageMask = 0,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dependencyFlags = 0,
			},
			VkSubpassDependency {
				.srcSubpass = 0,
				.dstSubpass = VK_SUBPASS_EXTERNAL,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = 0,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = 0,
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

	void create_image_pocessing_pipeline_layout() {

		auto descriptor_set_layouts = [&]() {
			if constexpr (has_field_type_v<UboType, ColorRampData> && has_field_type_v<UboType, TextureIdData>) {
				return std::array{
					ubo_descriptor_set_layout,
					engine->node_texture_2d_manager->descriptor_set_layout,
					engine->node_texture_1d_manager->descriptor_set_layout
				};
			}
			else if constexpr (has_field_type_v<UboType, TextureIdData>) {
				return std::array{
					ubo_descriptor_set_layout,
					engine->node_texture_2d_manager->descriptor_set_layout
				};
			}
			else {
				return std::array{ ubo_descriptor_set_layout };
			}
		}();

		VkPipelineLayoutCreateInfo image_pocessing_pipeline_info = vkinit::pipelineLayoutCreateInfo(descriptor_set_layouts);

		if (vkCreatePipelineLayout(engine->device, &image_pocessing_pipeline_info, nullptr, &image_pocessing_pipeline_layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		engine->main_deletion_queue.push_function([=]() {
			vkDestroyPipelineLayout(engine->device, image_pocessing_pipeline_layout, nullptr);
			});
	}

	void create_image_pocessing_pipeline() {
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
			if (image_pocessing_pipeline) {
				vkDestroyPipeline(engine->device, image_pocessing_pipeline, nullptr);
				image_pocessing_pipeline = nullptr;
			}
			});
	}

	void create_framebuffer() {
		std::array framebuffer_attachments = { texture->imageView };

		VkFramebufferCreateInfo framebufferInfo = vkinit::framebufferCreateInfo(image_pocessing_render_pass, VkExtent2D{ width , height }, framebuffer_attachments);

		if (vkCreateFramebuffer(engine->device, &framebufferInfo, nullptr, &image_pocessing_framebuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}

	void create_image_pocessing_command_buffer() {
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(engine->commandPool, 1);

		if (vkAllocateCommandBuffers(engine->device, &cmdAllocInfo, &image_pocessing_cmd_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
		};

		if (vkBeginCommandBuffer(image_pocessing_cmd_buffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkExtent2D image_extent{ width, height };

		VkRenderPassBeginInfo renderPassInfo = vkinit::renderPassBeginInfo(image_pocessing_render_pass, image_extent, image_pocessing_framebuffer);

		vkCmdBeginRenderPass(image_pocessing_cmd_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(width),
			.height = static_cast<float>(height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};

		VkRect2D scissor{
			.offset = { 0, 0 },
			.extent = image_extent,
		};

		auto descriptor_sets = [&]() {
			if constexpr (has_field_type_v<UboType, ColorRampData> && has_field_type_v<UboType, TextureIdData>) {
				return std::array{
					ubo_descriptor_set,
					engine->node_texture_2d_manager->descriptor_set,
					engine->node_texture_1d_manager->descriptor_set,
				};
			}
			else if constexpr (has_field_type_v<UboType, TextureIdData>) {
				return std::array{
					ubo_descriptor_set,
					engine->node_texture_2d_manager->descriptor_set 
				};
			}
			else {
				return std::array{ ubo_descriptor_set };
			}
		}();

		vkCmdSetViewport(image_pocessing_cmd_buffer, 0, 1, &viewport);
		vkCmdSetScissor(image_pocessing_cmd_buffer, 0, 1, &scissor);
		vkCmdBindDescriptorSets(image_pocessing_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, image_pocessing_pipeline_layout, 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
		vkCmdBindPipeline(image_pocessing_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, image_pocessing_pipeline);
		vkCmdDraw(image_pocessing_cmd_buffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(image_pocessing_cmd_buffer);

		if (vkEndCommandBuffer(image_pocessing_cmd_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	void create_preview_command_buffer() {
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(engine->commandPool, 1);

		if (vkAllocateCommandBuffers(engine->device, &cmdAllocInfo, &generate_preview_cmd_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		};

		if (vkBeginCommandBuffer(generate_preview_cmd_buffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}
		{
			auto image_memory_barrier = VkImageMemoryBarrier2{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
				.srcAccessMask = VK_ACCESS_2_NONE,
				.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
				.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = preview_texture->image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1,
				},
			};

			auto dependency_info = VkDependencyInfo{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &image_memory_barrier,
			};

			vkCmdPipelineBarrier2(generate_preview_cmd_buffer, &dependency_info);
		}

		VkImageBlit image_blit{
			.srcSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.srcOffsets = {
				{0, 0, 0},
				{static_cast<int32_t>(texture->width), static_cast<int32_t>(texture->height), 1},
			},
			.dstSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.dstOffsets = {
				{0, 0, 0},
				{static_cast<int32_t>(preview_texture->width), static_cast<int32_t>(preview_texture->height), 1},
			},
		};

		vkCmdBlitImage(generate_preview_cmd_buffer,
			texture->image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			preview_texture->image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&image_blit,
			VK_FILTER_LINEAR);

		{
			auto image_memory_barrier = VkImageMemoryBarrier2{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
				.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_NONE,
				.dstAccessMask = VK_ACCESS_2_NONE,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = preview_texture->image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1,
				},
			};

			auto dependency_info = VkDependencyInfo{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &image_memory_barrier,
			};

			vkCmdPipelineBarrier2(generate_preview_cmd_buffer, &dependency_info);
		}

		{
			auto image_memory_barrier = VkImageMemoryBarrier2{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
				.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_NONE,
				.dstAccessMask = VK_ACCESS_2_NONE,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = texture->image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1,
				},
			};

			auto dependency_info = VkDependencyInfo{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &image_memory_barrier,
			};

			vkCmdPipelineBarrier2(generate_preview_cmd_buffer, &dependency_info);
		}

		if (vkEndCommandBuffer(generate_preview_cmd_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

	}

	void create_cmd_buffer_submit_info() {
		std::vector<VkSemaphoreSubmitInfo> wait_semaphore_submit_info1;
		wait_semaphore_submit_info1.reserve(count_field_type_v<UboType, TextureIdData>);

		cmd_buffer_submit_info1 = VkCommandBufferSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = image_pocessing_cmd_buffer,
		};

		signal_semaphore_submit_info1 = VkSemaphoreSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.pNext = nullptr,
			.semaphore = semaphore,
			.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		};

		submit_info[0] = VkSubmitInfo2{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.pNext = nullptr,
			//.waitSemaphoreInfoCount = static_cast<uint32_t>(wait_semaphore_submit_info1.size()),
			//.pWaitSemaphoreInfos = wait_semaphore_submit_info1.data(),
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cmd_buffer_submit_info1,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &signal_semaphore_submit_info1,
		};

		wait_semaphore_submit_info2 = VkSemaphoreSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.pNext = nullptr,
			.semaphore = semaphore,
			.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		};

		cmd_buffer_submit_info2 = VkCommandBufferSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = generate_preview_cmd_buffer,
		};

		signal_semaphore_submit_info2 = VkSemaphoreSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.pNext = nullptr,
			.semaphore = semaphore,
			.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		};

		submit_info[1] = VkSubmitInfo2{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.pNext = nullptr,
			.waitSemaphoreInfoCount = 1,
			.pWaitSemaphoreInfos = &wait_semaphore_submit_info2,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cmd_buffer_submit_info2,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &signal_semaphore_submit_info2,
		};
	}

	void update_ubo(const PinVariant& value, size_t index) {
		if constexpr (has_field_type_v<UboType, ColorRampData>) {
			UboType::value_t::Class::FieldAt(index, [&](auto& field_v) {
				using PinValueT = std::decay_t<decltype(field_v)>::Type;
				UboType::Class::FieldAt(index, [&](auto& field) {
					using PinT = std::decay_t<decltype(field)>::Type;
					uniform_buffer->copyFromHost(reinterpret_cast<const char*>(&std::get<PinT>(value)), sizeof(PinValueT), field_v.getOffset());
					});
				});
		}
		else {
			UboType::Class::FieldAt(index, [&](auto& field) {
				using PinT = std::decay_t<decltype(field)>::Type;
				uniform_buffer->copyFromHost(reinterpret_cast<const char*>(&std::get<PinT>(value)), sizeof(PinT), field.getOffset());
				});
		}
	}
};

template<typename UboType, StringLiteral ...Shaders>
using ImageDataPtr = std::shared_ptr<ImageData<UboType, Shaders...>>;
