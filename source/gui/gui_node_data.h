#pragma once

#include "../vk_pipeline.h"
#include "../vk_initializers.h"
#include "../vk_engine.h"
#include "../vk_util.h"
#include "gui_node_pin_data.h"
#include <imgui_impl_vulkan.h>
#include <unordered_map>

constexpr static inline uint32_t PREVIEW_IMAGE_SIZE = 128;
constexpr static inline uint32_t TEXTURE_IMAGE_SIZE = 1024;

namespace engine {
	class Shader;
}

template<typename UniformBufferType, typename ResultT>
struct ValueData : NodeData {
	using UboType = UniformBufferType;
	using ResultType = ResultT;

	inline constexpr static auto calculate = UniformBufferType::operation;
};

template<typename UniformBufferType>
struct UboMixin {
	using UboType = UniformBufferType;

	BufferPtr uniform_buffer;

	explicit UboMixin(VulkanEngine* engine, BufferPtr buffer = nullptr) {
		if(buffer) {
			uniform_buffer = buffer;
		}
		else {
			uniform_buffer = engine::Buffer::create_buffer(engine,
			sizeof(UboType),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			SWAPCHAIN_INDEPENDENT_BIT);
		}
	}

	void update_ubo(const PinVariant& value, size_t index) {  //use value to update the pin at the index  
		if constexpr (has_field_type_v<UboType, ColorRampData>) {
			typename UboType::value_t::Class::FieldAt(index, [&](auto& field_v) {
				using PinValueT = typename std::decay_t<decltype(field_v)>::Type;
				UboType::Class::FieldAt(index, [&](auto& field) {
					using PinT = typename std::decay_t<decltype(field)>::Type;
					uniform_buffer->copy_from_host(reinterpret_cast<const char*>(std::get_if<PinT>(&value)), sizeof(PinValueT), field_v.getOffset());
					});
				});
		}
		else {
			UboType::Class::FieldAt(index, [&](auto& field) {
				using PinT = typename std::decay_t<decltype(field)>::Type;
				if constexpr (std::same_as<PinT, FloatTextureIdData>) {
					std::visit([&](auto&& v) {
						using StartPinT = std::decay_t<decltype(v)>;
						if (std::same_as<StartPinT, FloatData>) {
							uniform_buffer->copy_from_host(reinterpret_cast<const char*>(&v), sizeof(FloatData), field.getOffset());
						}
						else if (std::same_as<StartPinT, TextureIdData>) {
							uniform_buffer->copy_from_host(reinterpret_cast<const char*>(&v), sizeof(TextureIdData), field.getOffset() + sizeof(FloatData));
						}
						else if (std::same_as<StartPinT, FloatTextureIdData>) {
							uniform_buffer->copy_from_host(reinterpret_cast<const char*>(&v), sizeof(FloatTextureIdData), field.getOffset());
						}
						else {
							assert((false, "Error occurs when updating ubo. Pin type is FloatTextureIdData"));
						}
						}, value);
				}
				else if constexpr (std::same_as<PinT, Color4TextureIdData>) {
					std::visit([&](auto&& v) {
						using StartPinT = std::decay_t<decltype(v)>;
						if (std::same_as<StartPinT, Color4Data>) {
							uniform_buffer->copy_from_host(reinterpret_cast<const char*>(&v), sizeof(Color4Data), field.getOffset());
						}
						else if (std::same_as<StartPinT, TextureIdData>) {
							uniform_buffer->copy_from_host(reinterpret_cast<const char*>(&v), sizeof(TextureIdData), field.getOffset() + sizeof(Color4Data));
						}
						else if (std::same_as<StartPinT, Color4TextureIdData>) {
							uniform_buffer->copy_from_host(reinterpret_cast<const char*>(&v), sizeof(Color4TextureIdData), field.getOffset());
						}
						else {
							assert((false, "Error occurs when updating ubo. Pin type is Color4TextureIdData"));
						}
						}, value);
				}
				else {
					uniform_buffer->copy_from_host(reinterpret_cast<const char*>(std::get_if<PinT>(&value)), sizeof(PinT), field.getOffset());
				}
				});
		}
	}

	template<PinDataConcept StartPinT>
	void update_ubo_by_value(const StartPinT& value, size_t index) {
		UboType::Class::FieldAt(index, [&](auto& field) {
			using PinT = typename std::decay_t<decltype(field)>::Type;
			if constexpr (std::same_as<PinT, FloatTextureIdData>) {
				std::visit([&](auto&& v) {
					using StartPinT = std::decay_t<decltype(v)>;
					if (std::same_as<StartPinT, FloatData>) {
						uniform_buffer->copy_from_host(reinterpret_cast<const char*>(&v), sizeof(FloatData), field.getOffset());
					}
					else if (std::same_as<StartPinT, TextureIdData>) {
						uniform_buffer->copy_from_host(reinterpret_cast<const char*>(&v), sizeof(TextureIdData), field.getOffset() + sizeof(FloatData));
					}
					else if (std::same_as<StartPinT, FloatTextureIdData>) {
						uniform_buffer->copy_from_host(reinterpret_cast<const char*>(&v), sizeof(FloatTextureIdData), field.getOffset());
					}
					else {
						assert((false, "Error occurs when updating ubo. Pin type is FloatTextureIdData"));
					}
					}, value);
			}
			else if constexpr (std::same_as<PinT, StartPinT>) {
				uniform_buffer->copy_from_host(reinterpret_cast<const char*>(std::get_if<PinT>(&value)), sizeof(PinT), field.getOffset());
			}
			});
	}
};

template<typename UniformBufferType>
struct ShaderData : NodeData, UboMixin<UniformBufferType> {
	using UboType = UniformBufferType;

	explicit ShaderData(VulkanEngine* engine) : UboMixin<UniformBufferType>(engine, engine->material_preview_ubo) {}
};

struct SubmitInfoMembers{
	VkSemaphoreSubmitInfo wait_semaphore_submit_info;
	VkCommandBufferSubmitInfo cmd_buffer_submit_info;
	VkSemaphoreSubmitInfo signal_semaphore_submit_info;
};

template<typename UniformBufferType>
struct ComponentUdf : UboMixin<UniformBufferType> {
	using UboT = UniformBufferType;
	inline constexpr static auto shader_num = UboT::shader_file_paths.size();
	inline static std::array<VkDescriptorSetLayout, shader_num> ubo_descriptor_set_layouts{ nullptr };
	inline static std::array<VkPipelineLayout, shader_num> image_processing_pipeline_layouts{ nullptr };
	inline static std::array<VkPipeline, shader_num> image_processing_compute_pipelines{ nullptr };

	TexturePtr texture;
	TexturePtr preview_texture;
	std::array<VkDescriptorSet, UboT::shader_file_paths.size()> ubo_descriptor_sets;
	std::array<TexturePtr, 2> ping_pong_images;
	VkImageView result_image_view;
	std::function<void(int)> record_image_processing_cmd_buffer_func;
	std::function<void(int)> record_preview_cmd_buffer_func;

	VkCommandBuffer ownership_release_cmd_buffer = nullptr;
	VkCommandBuffer image_processing_cmd_buffer = nullptr;
	VkCommandBuffer generate_preview_cmd_buffer = nullptr;

	uint32_t width = TEXTURE_IMAGE_SIZE;
	uint32_t height = TEXTURE_IMAGE_SIZE;

	VkSemaphore semaphore;

	std::vector<VkSemaphoreSubmitInfo> wait_semaphore_submit_info_0;
	VkCommandBufferSubmitInfo cmd_buffer_submit_info_0;
	VkSemaphoreSubmitInfo signal_semaphore_submit_info_0;

	std::array<SubmitInfoMembers, 2> submit_info_members;

	std::array<VkSubmitInfo2, 3> submit_info;

	explicit ComponentUdf(VulkanEngine* engine) :UboMixin<UniformBufferType>(engine) {}

	void create_image_processing_pipeline_resource(VulkanEngine* engine, VkFormat format) {
		//update_ubo_descriptor_sets(engine);
		create_image_processing_compute_pipelines(engine);
		create_image_processing_compute_command_buffer_func(engine);
	}

	static void create_ubo_descriptor_set_layout(VulkanEngine* engine) {
		std::array preprocess_layout_bindings{
			vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0),
			vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1),
		};
		engine->create_descriptor_set_layout(
			preprocess_layout_bindings,
			ubo_descriptor_set_layouts[0]);

		std::array layout_bindings{
			vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0),
			vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1),
			vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2),
			vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 3),
		};
		engine->create_descriptor_set_layout(layout_bindings,
			ubo_descriptor_set_layouts[1]);
	}

	void create_ubo_descriptor_sets(VulkanEngine* engine) {
	
		VkDescriptorSetAllocateInfo descriptor_set_allocate_infos{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = engine->dynamic_descriptor_pool,
			.descriptorSetCount = ubo_descriptor_set_layouts.size(),
			.pSetLayouts = ubo_descriptor_set_layouts.data(),
		};
	

		if (vkAllocateDescriptorSets(engine->device, &descriptor_set_allocate_infos, ubo_descriptor_sets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
	}

	static void create_image_processing_pipeline_layouts(VulkanEngine* engine) {
		if (image_processing_pipeline_layouts[0]) {
			return;
		}
		UNROLL<shader_num>([&]<size_t i> {
			auto descriptor_set_layouts = [&] {
				if constexpr (i == 0) {
					return std::array{
						ubo_descriptor_set_layouts[i],
						engine->texture_manager->descriptor_set_layout,
					};
				}
				else {
					return std::array{ ubo_descriptor_set_layouts[i] };
				}
			}();
			auto image_processing_pipeline_info = vkinit::pipeline_layout_create_info(descriptor_set_layouts);
			VkPushConstantRange push_constant_range;
			if constexpr (i == 1) {
				push_constant_range = {
					.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
					.offset = 0,
					.size = 4
				};
				image_processing_pipeline_info.pushConstantRangeCount = 1;
				image_processing_pipeline_info.pPushConstantRanges = &push_constant_range;
			}

			if (vkCreatePipelineLayout(engine->device, &image_processing_pipeline_info, nullptr, &image_processing_pipeline_layouts[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create pipeline layout!");
			}

			engine->main_deletion_queue.push_function([device = engine->device, pipeline_layout = image_processing_pipeline_layouts[i]]{
				vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
				});
		});
	}

	void create_textures(VulkanEngine* engine, const VkFormat format) {
		const bool is_gray_scale = (format == VK_FORMAT_R16_UNORM || format == VK_FORMAT_R16_SFLOAT);

		texture = engine::Texture::create_device_texture(engine,
			this->width,
			this->height,
			format,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | (is_gray_scale ? VK_IMAGE_USAGE_STORAGE_BIT : 0),
			SWAPCHAIN_INDEPENDENT_BIT,
			is_gray_scale);

		texture->transition_image_layout(engine, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		for (size_t i = 0; i < 2; ++i) {
			ping_pong_images[i] = engine::Texture::create_device_texture(engine,
				width,
				height,
				VK_FORMAT_R16G16_UINT,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_USAGE_STORAGE_BIT,
				SWAPCHAIN_INDEPENDENT_BIT);
			ping_pong_images[i]->transition_image_layout(engine, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, QueueFamilyCategory::COMPUTE);
		}
	}

	void update_ubo_descriptor_sets(VulkanEngine* engine) {
		const VkImageViewCreateInfo result_image_view_create_info{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = texture->image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = texture->format,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			}
		};

		if (vkCreateImageView(engine->device, &result_image_view_create_info, nullptr, &result_image_view) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}

		const VkDescriptorImageInfo result_image_info{
			.imageView = result_image_view,
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
		};

		const std::array ping_pong_image_infos{
			VkDescriptorImageInfo{
				.imageView = ping_pong_images[0]->image_view,
				.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
			},
			VkDescriptorImageInfo{
				.imageView = ping_pong_images[1]->image_view,
				.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
			}
		};

		const VkDescriptorBufferInfo uniform_buffer_info{
			.buffer = this->uniform_buffer->buffer,
			.offset = 0,
			.range = sizeof(UboT)
		};

		auto const descriptor_writes = std::array{
			//Pass 1
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = ubo_descriptor_sets[0],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &uniform_buffer_info,
			},
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = ubo_descriptor_sets[0],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.pImageInfo = &ping_pong_image_infos[0],
			},
			//Pass 2
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = ubo_descriptor_sets[1],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &uniform_buffer_info,
			},
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = ubo_descriptor_sets[1],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.pImageInfo = &ping_pong_image_infos[0],
			},
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = ubo_descriptor_sets[1],
				.dstBinding = 2,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.pImageInfo = &ping_pong_image_infos[1],
			},
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = ubo_descriptor_sets[1],
				.dstBinding = 3,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.pImageInfo = &result_image_info,
			},
		};

		vkUpdateDescriptorSets(
			engine->device,
			descriptor_writes.size(),
			descriptor_writes.data(),
			0,
			nullptr);
	}

	void create_image_processing_compute_pipelines(VulkanEngine* engine) {

		auto shader = engine::Shader::createFromSpv(engine, UboT::shader_file_paths);

		for (size_t i = 0; i < shader->shader_modules.size(); ++i) {
			const VkComputePipelineCreateInfo compute_pipeline_create_info{
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.stage = {
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage = shader->shader_modules[i].stage,
					.module = shader->shader_modules[i].shader,
					.pName = "main",
				},
				.layout = image_processing_pipeline_layouts[i],
			};

			if (vkCreateComputePipelines(
				engine->device,
				VK_NULL_HANDLE,
				1,
				&compute_pipeline_create_info,
				nullptr,
				&image_processing_compute_pipelines[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create graphics pipeline!");
			}

			engine->main_deletion_queue.push_function([device = engine->device, pipeline = image_processing_compute_pipelines[i]]{
				vkDestroyPipeline(device, pipeline, nullptr);
				});
		}
	}

	void create_image_processing_compute_command_buffer_func(const VulkanEngine* engine) {
		record_image_processing_cmd_buffer_func = [=](int input_image_idx) {
			
			VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::commandBufferAllocateInfo(engine->graphic_command_pool, 1);
			if (vkAllocateCommandBuffers(engine->device, &cmd_alloc_info, &ownership_release_cmd_buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate command buffers!");
			}
			const VkCommandBufferBeginInfo begin_info{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
			};

			if (vkBeginCommandBuffer(ownership_release_cmd_buffer, &begin_info) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			//insert_image_memory_barrier(
			//	ownership_release_cmd_buffer,
			//	engine->texture_manager->textures[input_image_idx]->image,
			//	VK_PIPELINE_STAGE_2_NONE,
			//	VK_ACCESS_2_NONE,
			//	VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			//	VK_ACCESS_2_SHADER_READ_BIT,
			//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			//	engine->queue_family_indices.graphics_family.value(),
			//	engine->queue_family_indices.compute_family.value()
			//);

			//insert_image_memory_barrier(
			//	ownership_release_cmd_buffer,
			//	texture->image,
			//	VK_PIPELINE_STAGE_2_NONE,
			//	VK_ACCESS_2_NONE,
			//	VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			//	VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
			//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			//	VK_IMAGE_LAYOUT_GENERAL,
			//	engine->queue_family_indices.graphics_family.value(),
			//	engine->queue_family_indices.compute_family.value()
			//);

			if (vkEndCommandBuffer(ownership_release_cmd_buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}

			//record image_processing_cmd_buffer
			cmd_alloc_info = vkinit::commandBufferAllocateInfo(engine->compute_command_pool, 1);
			if (vkAllocateCommandBuffers(engine->device, &cmd_alloc_info, &image_processing_cmd_buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate command buffers!");
			}

			if (vkBeginCommandBuffer(image_processing_cmd_buffer, &begin_info) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			/*insert_image_memory_barrier(
				image_processing_cmd_buffer,
				engine->texture_manager->textures[input_image_idx]->image,
				VK_PIPELINE_STAGE_2_NONE,
				VK_ACCESS_2_NONE,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				engine->queue_family_indices.graphics_family.value(),
				engine->queue_family_indices.compute_family.value()
			);*/

			

			vkCmdBindPipeline(image_processing_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, image_processing_compute_pipelines[0]);

			std::array descriptor_sets{
				ubo_descriptor_sets[0],
				engine->texture_manager->descriptor_set,
			};
			vkCmdBindDescriptorSets(
				image_processing_cmd_buffer,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				image_processing_pipeline_layouts[0],
				0, descriptor_sets.size(), descriptor_sets.data(),
				0, nullptr);

			vkCmdDispatch(image_processing_cmd_buffer, texture->width / 16, texture->height / 16, 1);

			//insert_image_memory_barrier(
			//	image_processing_cmd_buffer,
			//	engine->texture_manager->textures[input_image_idx]->image,
			//	VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			//	VK_ACCESS_2_NONE,
			//	VK_PIPELINE_STAGE_2_NONE,
			//	VK_ACCESS_2_NONE,
			//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			//	engine->queue_family_indices.compute_family.value(),
			//	engine->queue_family_indices.graphics_family.value()
			//);

			insert_image_memory_barrier(
				image_processing_cmd_buffer,
				ping_pong_images[0]->image,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_GENERAL
			);

			insert_image_memory_barrier(
				image_processing_cmd_buffer,
				texture->image,
				VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
				VK_ACCESS_2_NONE,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_GENERAL//,
				//engine->queue_family_indices.graphics_family.value(),
				//engine->queue_family_indices.compute_family.value()
			);

			vkCmdBindPipeline(image_processing_cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, image_processing_compute_pipelines[1]);

			vkCmdBindDescriptorSets(
				image_processing_cmd_buffer,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				image_processing_pipeline_layouts[1],
				0, 1, &ubo_descriptor_sets[1],
				0, nullptr);

			int idx = 1;
			const int size = texture->width;
			while (size >> idx) {
				vkCmdPushConstants(image_processing_cmd_buffer, image_processing_pipeline_layouts[1], VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &idx);
				vkCmdDispatch(image_processing_cmd_buffer, texture->width / 16, texture->height / 16, 1);
				insert_image_memory_barrier(
					image_processing_cmd_buffer,
					ping_pong_images[idx%2]->image,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_IMAGE_LAYOUT_GENERAL
				);
				++idx;
			}
			vkCmdDispatch(image_processing_cmd_buffer, texture->width / 16, texture->height / 16, 1);
			insert_image_memory_barrier(
				image_processing_cmd_buffer,
				ping_pong_images[idx%2]->image,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_GENERAL
			);

			idx = -1;
			vkCmdPushConstants(image_processing_cmd_buffer, image_processing_pipeline_layouts[1], VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &idx);
			vkCmdDispatch(image_processing_cmd_buffer, texture->width / 16, texture->height / 16, 1);

			//insert_image_memory_barrier(
			//	image_processing_cmd_buffer,
			//	texture->image,
			//	VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			//	VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
			//	VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			//	VK_ACCESS_2_NONE,
			//	VK_IMAGE_LAYOUT_GENERAL,
			//	VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			//	engine->queue_family_indices.compute_family.value(),
			//	engine->queue_family_indices.graphics_family.value()
			//);

			if (vkEndCommandBuffer(image_processing_cmd_buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		};
	}

	void update_command_buffer_submit_info() {
		cmd_buffer_submit_info_0.commandBuffer = ownership_release_cmd_buffer;
		submit_info_members[0].cmd_buffer_submit_info.commandBuffer = image_processing_cmd_buffer;
		submit_info_members[1].cmd_buffer_submit_info.commandBuffer = generate_preview_cmd_buffer;
	}

	void create_cmd_buffer_submit_info() {
		//std::vector<VkSemaphoreSubmitInfo> wait_semaphore_submit_info1;
		//wait_semaphore_submit_info1.reserve(count_field_type_v<UboType, TextureIdData>);

		cmd_buffer_submit_info_0 = VkCommandBufferSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = ownership_release_cmd_buffer,
		};

		signal_semaphore_submit_info_0 = VkSemaphoreSubmitInfo{
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
			.pCommandBufferInfos = &cmd_buffer_submit_info_0,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &signal_semaphore_submit_info_0,
		};

		submit_info_members = {
			SubmitInfoMembers{
				.wait_semaphore_submit_info = {
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
					.pNext = nullptr,
					.semaphore = semaphore,
					.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				},
				.cmd_buffer_submit_info = {
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
					.commandBuffer = image_processing_cmd_buffer,
				},
				.signal_semaphore_submit_info = {
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
					.pNext = nullptr,
					.semaphore = semaphore,
					.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				}
			},
			SubmitInfoMembers{
				.wait_semaphore_submit_info = {
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
					.pNext = nullptr,
					.semaphore = semaphore,
					.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				},
				.cmd_buffer_submit_info = {
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
					.commandBuffer = generate_preview_cmd_buffer,
				},
				.signal_semaphore_submit_info = {
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
					.pNext = nullptr,
					.semaphore = semaphore,
					.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				}
			}
		};

		for(size_t i=1; i<=submit_info_members.size(); ++i) {
			submit_info[i] = VkSubmitInfo2{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
				.waitSemaphoreInfoCount = 1,
				.pWaitSemaphoreInfos = &submit_info_members[i-1].wait_semaphore_submit_info,
				.commandBufferInfoCount = 1,
				.pCommandBufferInfos = &submit_info_members[i-1].cmd_buffer_submit_info,
				.signalSemaphoreInfoCount = 1,
				.pSignalSemaphoreInfos = &submit_info_members[i-1].signal_semaphore_submit_info,
			};
		}
	}

	void create_preview_command_buffer(VulkanEngine* engine) {
		record_preview_cmd_buffer_func = [=](int input_image_idx) {
			const VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::commandBufferAllocateInfo(engine->graphic_command_pool, 1);

			if (vkAllocateCommandBuffers(engine->device, &cmd_alloc_info, &this->generate_preview_cmd_buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate command buffers!");
			}

			const VkCommandBufferBeginInfo begin_info{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			};

			if (vkBeginCommandBuffer(this->generate_preview_cmd_buffer, &begin_info) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			//insert_image_memory_barrier(
			//	this->generate_preview_cmd_buffer,
			//	engine->texture_manager->textures[input_image_idx]->image,
			//	VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			//	VK_ACCESS_2_NONE,
			//	VK_PIPELINE_STAGE_2_NONE,
			//	VK_ACCESS_2_NONE,
			//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			//	engine->queue_family_indices.compute_family.value(),
			//	engine->queue_family_indices.graphics_family.value()
			//);

			insert_image_memory_barrier(
				this->generate_preview_cmd_buffer,
				texture->image,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				VK_PIPELINE_STAGE_2_BLIT_BIT,
				VK_ACCESS_2_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			);

			insert_image_memory_barrier(
				this->generate_preview_cmd_buffer,
				preview_texture->image,
				VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				VK_ACCESS_2_NONE,
				VK_PIPELINE_STAGE_2_BLIT_BIT,
				VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			);

			const VkImageBlit image_blit{
				.srcSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = 0,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
				.srcOffsets = {
					{0, 0, 0},
					{static_cast<int32_t>(this->texture->width), static_cast<int32_t>(this->texture->height), 1},
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

			vkCmdBlitImage(
				this->generate_preview_cmd_buffer,
				this->texture->image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				preview_texture->image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&image_blit,
				VK_FILTER_LINEAR
			);

			insert_image_memory_barrier(
				this->generate_preview_cmd_buffer,
				preview_texture->image,
				VK_PIPELINE_STAGE_2_BLIT_BIT,
				VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				VK_ACCESS_2_NONE,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);

			insert_image_memory_barrier(
				this->generate_preview_cmd_buffer,
				texture->image,
				VK_PIPELINE_STAGE_2_BLIT_BIT,
				VK_ACCESS_2_TRANSFER_READ_BIT,
				VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				VK_ACCESS_2_NONE,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);

			if (vkEndCommandBuffer(this->generate_preview_cmd_buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		};
	}

	void clear(VulkanEngine* engine) const {
		vkFreeCommandBuffers(engine->device, engine->compute_command_pool, 1, &image_processing_cmd_buffer);
	}
};

template<typename UniformBufferType>
struct ComponentGraphicPipeline : UboMixin<UniformBufferType> {
	using UboT = UniformBufferType;

	inline static VkDescriptorSetLayout ubo_descriptor_set_layout = nullptr;
	inline static VkPipelineLayout image_processing_pipeline_layout = nullptr;
	inline static std::unordered_map<VkFormat, VkPipeline> image_processing_pipelines;
	inline static std::unordered_map<VkFormat, VkRenderPass> image_processing_render_passes;

	TexturePtr texture;
	TexturePtr preview_texture;
	VkImageView render_target_image_view;
	VkDescriptorSet ubo_descriptor_set;
	VkFramebuffer image_processing_framebuffer;
	VkCommandBuffer image_processing_cmd_buffer = nullptr;
	VkCommandBuffer generate_preview_cmd_buffer = nullptr;

	VkSemaphore semaphore;

	std::vector<VkSemaphoreSubmitInfo> wait_semaphore_submit_info_0;
	VkCommandBufferSubmitInfo cmd_buffer_submit_info_0;
	VkSemaphoreSubmitInfo signal_semaphore_submit_info_0;

	VkSemaphoreSubmitInfo wait_semaphore_submit_info1;
	VkCommandBufferSubmitInfo cmd_buffer_submit_info1;
	VkSemaphoreSubmitInfo signal_semaphore_submit_info1;

	std::array<VkSubmitInfo2, 2> submit_info;

	uint32_t width = TEXTURE_IMAGE_SIZE;
	uint32_t height = TEXTURE_IMAGE_SIZE;

	explicit ComponentGraphicPipeline(VulkanEngine* engine) : UboMixin<UniformBufferType>(engine) {}

	void create_image_processing_pipeline_resource(VulkanEngine* engine, VkFormat format) {
		create_image_processing_render_pass(engine, format);
		create_image_processing_pipeline(engine, format);
		create_framebuffer(engine, format);
		create_image_processing_command_buffer(engine, format);
	}

	static void create_ubo_descriptor_set_layout(VulkanEngine* engine) {
		std::array layout_bindings = {
			vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
		};
		engine->create_descriptor_set_layout(layout_bindings, ubo_descriptor_set_layout);
	}

	void create_ubo_descriptor_sets(VulkanEngine* engine) {
		const VkDescriptorSetAllocateInfo ubo_descriptor_alloc_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = engine->dynamic_descriptor_pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &ubo_descriptor_set_layout,
		};

		if (vkAllocateDescriptorSets(engine->device, &ubo_descriptor_alloc_info, &ubo_descriptor_set) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
	}

	void update_ubo_descriptor_sets(VulkanEngine* engine) {
		const VkDescriptorBufferInfo uniform_buffer_info{
			.buffer = this->uniform_buffer->buffer,
			.offset = 0,
			.range = sizeof(UboT)
		};

		const std::array descriptor_writes{
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = ubo_descriptor_set,
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &uniform_buffer_info,
			},
		};

		vkUpdateDescriptorSets(engine->device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
	}

	void create_image_processing_pipeline_layouts(VulkanEngine* engine) {
		if (image_processing_pipeline_layout) {
			return;
		}

		auto descriptor_set_layouts = [&] {
			if constexpr (has_texture_field<UboT>) {
				return std::array{
					ubo_descriptor_set_layout,
					engine->texture_manager->descriptor_set_layout,
				};
			}
			else {
				return std::array{ ubo_descriptor_set_layout };
			}
		}();

		auto image_processing_pipeline_info = vkinit::pipeline_layout_create_info(descriptor_set_layouts);

		if (vkCreatePipelineLayout(engine->device, &image_processing_pipeline_info, nullptr, &image_processing_pipeline_layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		engine->main_deletion_queue.push_function([device = engine->device, pipeline_layout = image_processing_pipeline_layout]{
			vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
			});
	}

	void create_textures(VulkanEngine* engine, const VkFormat format) {
		const bool is_gray_scale = (format == VK_FORMAT_R16_UNORM || format == VK_FORMAT_R16_SFLOAT);

		texture = engine::Texture::create_device_texture(engine,
			this->width,
			this->height,
			format,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			SWAPCHAIN_INDEPENDENT_BIT,
			is_gray_scale);

		texture->transition_image_layout(engine, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	static void create_image_processing_render_pass(VulkanEngine* engine, const VkFormat format) {
		if (image_processing_render_passes.contains(format)) {
			return;
		}
		const VkAttachmentDescription color_attachment{
			.format = format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		};

		constexpr VkAttachmentReference color_attachment_ref{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		constexpr VkSubpassDescription subpass{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment_ref,
		};

		const std::array attachments{ color_attachment };

		constexpr VkRenderPassCreateInfo render_pass_info{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 0,
			.pDependencies = nullptr,
		};

		if (vkCreateRenderPass(engine->device, &render_pass_info, nullptr, &image_processing_render_passes[format]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}

		engine->main_deletion_queue.push_function([device = engine->device, render_pass = image_processing_render_passes[format]]{
			vkDestroyRenderPass(device, render_pass, nullptr);
			});
	}

	void create_image_processing_pipeline(VulkanEngine* engine, const VkFormat format) {
		if (image_processing_pipelines.contains(format)) {
			return;
		}
		engine::PipelineBuilder pipeline_builder(engine, engine::ENABLE_DYNAMIC_VIEWPORT, engine::DISABLE_VERTEX_INPUT);

		auto shaders = engine::Shader::createFromSpv(engine, UboT::shader_file_paths);

		for (auto shader_module : shaders->shader_modules) {
			VkPipelineShaderStageCreateInfo shader_info{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = shader_module.stage,
				.module = shader_module.shader,
				.pName = "main"
			};
			pipeline_builder.shaderStages.emplace_back(std::move(shader_info));
		}

		pipeline_builder.build_pipeline(engine->device, image_processing_render_passes[format], image_processing_pipeline_layout, image_processing_pipelines[format]);

		engine->main_deletion_queue.push_function([device = engine->device, pipeline = image_processing_pipelines[format]]{
			vkDestroyPipeline(device, pipeline, nullptr);
			});
	}

	void create_framebuffer(VulkanEngine* engine, const VkFormat format) {
		VkFramebufferAttachmentImageInfo framebuffer_attachment_image_info{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
			.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			.width = width,
			.height = height,
			.layerCount = 1,
			.viewFormatCount = 1,
			.pViewFormats = &texture->format,
		};

		VkFramebufferAttachmentsCreateInfo framebuffer_attachments_create_info{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO,
			.attachmentImageInfoCount = 1,
			.pAttachmentImageInfos = &framebuffer_attachment_image_info,
		};

		const VkFramebufferCreateInfo framebuffer_info{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = &framebuffer_attachments_create_info,
			.flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT,
			.renderPass = image_processing_render_passes[format],
			.attachmentCount = 1,
			.width = width,
			.height = height,
			.layers = 1,
		};

		if (vkCreateFramebuffer(engine->device, &framebuffer_info, nullptr, &image_processing_framebuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}

	void create_image_processing_command_buffer(VulkanEngine* engine, const VkFormat format) {
		const VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::commandBufferAllocateInfo(engine->graphic_command_pool, 1);

		if (vkAllocateCommandBuffers(engine->device, &cmd_alloc_info, &image_processing_cmd_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		const VkCommandBufferBeginInfo begin_info{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
		};

		if (vkBeginCommandBuffer(image_processing_cmd_buffer, &begin_info) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkExtent2D image_extent{ width, height };

		const VkImageViewCreateInfo render_target_view_info{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = texture->image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			}
		};

		if (vkCreateImageView(engine->device, &render_target_view_info, nullptr, &render_target_image_view) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}

		const VkRenderPassAttachmentBeginInfo attachment_begin_info{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO,
			.attachmentCount = 1,
			.pAttachments = &render_target_image_view,
		};

		VkRenderPassBeginInfo render_pass_info = vkinit::renderPassBeginInfo(image_processing_render_passes[format], image_extent, image_processing_framebuffer);
		render_pass_info.pNext = &attachment_begin_info;

		vkCmdBeginRenderPass(image_processing_cmd_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		const VkViewport viewport{
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(width),
			.height = static_cast<float>(height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};

		const VkRect2D scissor{
			.offset = { 0, 0 },
			.extent = image_extent,
		};

		auto descriptor_sets = [&] {
			if constexpr (has_texture_field<UboT>) {
				return std::array{
					ubo_descriptor_set,
					engine->texture_manager->descriptor_set,
				};
			}
			else {
				return std::array{ ubo_descriptor_set };
			}
		}();

		vkCmdSetViewport(image_processing_cmd_buffer, 0, 1, &viewport);
		vkCmdSetScissor(image_processing_cmd_buffer, 0, 1, &scissor);
		vkCmdBindDescriptorSets(image_processing_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, image_processing_pipeline_layout, 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
		vkCmdBindPipeline(image_processing_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, image_processing_pipelines[format]);
		vkCmdDraw(image_processing_cmd_buffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(image_processing_cmd_buffer);

		if (vkEndCommandBuffer(image_processing_cmd_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	void update_command_buffer_submit_info() {
		cmd_buffer_submit_info_0.commandBuffer = image_processing_cmd_buffer;
		cmd_buffer_submit_info1.commandBuffer = generate_preview_cmd_buffer;
	}

	void create_cmd_buffer_submit_info() {
		//std::vector<VkSemaphoreSubmitInfo> wait_semaphore_submit_info1;
		//wait_semaphore_submit_info1.reserve(count_field_type_v<UboType, TextureIdData>);

		cmd_buffer_submit_info_0 = VkCommandBufferSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = this->image_processing_cmd_buffer,
		};

		signal_semaphore_submit_info_0 = VkSemaphoreSubmitInfo{
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
			.pCommandBufferInfos = &cmd_buffer_submit_info_0,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &signal_semaphore_submit_info_0,
		};

		wait_semaphore_submit_info1 = VkSemaphoreSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.pNext = nullptr,
			.semaphore = semaphore,
			.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		};

		cmd_buffer_submit_info1 = VkCommandBufferSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = generate_preview_cmd_buffer,
		};

		signal_semaphore_submit_info1 = VkSemaphoreSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.pNext = nullptr,
			.semaphore = semaphore,
			.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		};

		submit_info[1] = VkSubmitInfo2{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.pNext = nullptr,
			.waitSemaphoreInfoCount = 1,
			.pWaitSemaphoreInfos = &wait_semaphore_submit_info1,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cmd_buffer_submit_info1,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &signal_semaphore_submit_info1,
		};
	}

	void create_preview_command_buffer(VulkanEngine* engine) {
		const VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::commandBufferAllocateInfo(engine->graphic_command_pool, 1);

		if (vkAllocateCommandBuffers(engine->device, &cmd_alloc_info, &this->generate_preview_cmd_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		const VkCommandBufferBeginInfo begin_info{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		};

		if (vkBeginCommandBuffer(this->generate_preview_cmd_buffer, &begin_info) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		insert_image_memory_barrier(
			this->generate_preview_cmd_buffer,
			preview_texture->image,
			VK_PIPELINE_STAGE_2_NONE,
			VK_ACCESS_2_NONE,
			VK_PIPELINE_STAGE_2_BLIT_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		const VkImageBlit image_blit{
			.srcSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.srcOffsets = {
				{0, 0, 0},
				{static_cast<int32_t>(this->texture->width), static_cast<int32_t>(this->texture->height), 1},
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

		vkCmdBlitImage(
			this->generate_preview_cmd_buffer,
			this->texture->image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			preview_texture->image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&image_blit,
			VK_FILTER_LINEAR
		);

		insert_image_memory_barrier(
			this->generate_preview_cmd_buffer,
			preview_texture->image,
			VK_PIPELINE_STAGE_2_BLIT_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_NONE,
			VK_ACCESS_2_NONE,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		insert_image_memory_barrier(
			this->generate_preview_cmd_buffer,
			this->texture->image,
			VK_PIPELINE_STAGE_2_BLIT_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_NONE,
			VK_ACCESS_2_NONE,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		if (vkEndCommandBuffer(this->generate_preview_cmd_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}


	void clear(VulkanEngine* engine) const {
		vkDestroyImageView(engine->device, render_target_image_view, nullptr);
		vkFreeCommandBuffers(engine->device, engine->graphic_command_pool, 1, &image_processing_cmd_buffer);
		vkDestroyFramebuffer(engine->device, image_processing_framebuffer, nullptr);
	}
};

template<typename Component>
struct ImageData : NodeData, Component {
	using UboType = typename Component::UboT;

	VulkanEngine* engine;

	int node_texture_id = -1;

	void* gui_texture;
	void* gui_preview_texture;

	std::array<VkCommandBuffer, PbrMaterialTextureNum> copy_image_cmd_buffers;

	explicit ImageData(VulkanEngine* engine) :Component(engine), engine(engine) {

		create_semaphore();

		Component::create_ubo_descriptor_set_layout(engine);

		Component::create_ubo_descriptor_sets(engine);

		Component::create_image_processing_pipeline_layouts(engine);

		create_texture_resource(UboType::default_format);

		Component::create_cmd_buffer_submit_info();
	}

	void create_texture_resource(VkFormat format) {
		Component::create_textures(engine, format);
		create_preview_texture(format);
		update_image_descriptor_sets();
		Component::update_ubo_descriptor_sets(engine);
		Component::create_image_processing_pipeline_resource(engine, format);

		Component::create_preview_command_buffer(engine);
		create_copy_image_cmd_buffers();
	}

	void recreate_texture_resource(VkFormat format) {
		Component::clear(engine);
		vkFreeCommandBuffers(engine->device, engine->graphic_command_pool, 1, &this->generate_preview_cmd_buffer);
		create_texture_resource(format);
		Component::update_command_buffer_submit_info();
	}

	~ImageData() {
		engine->texture_manager->delete_id(node_texture_id);
		Component::clear(engine);
		vkFreeCommandBuffers(engine->device, engine->graphic_command_pool, 1, &this->generate_preview_cmd_buffer);
		if constexpr (std::same_as<Component, ComponentUdf<UboType>>) {
			vkFreeDescriptorSets(engine->device, engine->dynamic_descriptor_pool, this->ubo_descriptor_sets.size(), this->ubo_descriptor_sets.data());
			vkDestroyImageView(engine->device, this->result_image_view, nullptr);
		}
		else {
			vkFreeDescriptorSets(engine->device, engine->dynamic_descriptor_pool, 1, &this->ubo_descriptor_set);
		}
		vkDestroySemaphore(engine->device, this->semaphore, nullptr);
	}

	void create_semaphore() {
		constexpr VkSemaphoreTypeCreateInfo timeline_semaphore_create_info{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
			.pNext = nullptr,
			.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
			.initialValue = 0,
		};

		constexpr VkSemaphoreCreateInfo semaphore_create_info{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = &timeline_semaphore_create_info,
			.flags = 0,
		};

		vkCreateSemaphore(engine->device, &semaphore_create_info, nullptr, &this->semaphore);
	}

	void update_image_descriptor_sets() const {
		const VkDescriptorImageInfo image_info{
			.sampler = this->texture->sampler,
			.imageView = this->texture->image_view,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		const std::array descriptor_writes{
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = engine->texture_manager->descriptor_set,
				.dstBinding = 0,
				.dstArrayElement = static_cast<uint32_t>(node_texture_id),
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &image_info,
			},
		};

		vkUpdateDescriptorSets(engine->device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
	}

	
	

	void create_preview_texture(const VkFormat format) {
		const bool is_gray_scale = (format == VK_FORMAT_R16_UNORM || format == VK_FORMAT_R16_SFLOAT);
		this->preview_texture = engine::Texture::create_device_texture(engine,
			PREVIEW_IMAGE_SIZE,
			PREVIEW_IMAGE_SIZE,
			format,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			SWAPCHAIN_INDEPENDENT_BIT,
			is_gray_scale);

		this->preview_texture->transition_image_layout(engine, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		gui_texture = ImGui_ImplVulkan_AddTexture(this->texture->sampler, this->texture->image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		gui_preview_texture = ImGui_ImplVulkan_AddTexture(this->preview_texture->sampler, this->preview_texture->image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		if (node_texture_id == -1) {
			node_texture_id = engine->texture_manager->add_texture(this->texture);
		}
		else {
			engine->texture_manager->textures[node_texture_id] = this->texture;
		}
	}

	void create_copy_image_cmd_buffers() {
		const VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::commandBufferAllocateInfo(engine->graphic_command_pool, copy_image_cmd_buffers.size());

		if (vkAllocateCommandBuffers(engine->device, &cmd_alloc_info, copy_image_cmd_buffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	void record_copy_image_cmd_buffers(size_t index) {
		field_at(engine->pbr_material_texture_set, index, [&](auto dst_texture_id) {
			auto& copy_image_cmd_buffer = copy_image_cmd_buffers[index];
			auto& dst_texture = engine->texture_manager->textures[dst_texture_id];
			const VkCommandBufferBeginInfo begin_info{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			};

			if (vkBeginCommandBuffer(copy_image_cmd_buffer, &begin_info) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			for (auto const& [texture, new_layout] : {
				std::tuple{this->texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
				std::tuple{dst_texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL} }) {

				insert_image_memory_barrier(
					copy_image_cmd_buffer,
					texture->image,
					VK_PIPELINE_STAGE_2_NONE,
					VK_ACCESS_2_NONE,
					VK_PIPELINE_STAGE_2_COPY_BIT,
					VK_ACCESS_2_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					new_layout
				);
			}

			VkImageCopy2 image_copy_region{
				.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
				.pNext = nullptr,
				.srcSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = 0,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
				.srcOffset = {0,0,0},
				.dstSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = 0,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
				.dstOffset = {0,0,0},
				.extent = {
					this->texture->width,
					this->texture->height,
					1,
				},
			};

			const VkCopyImageInfo2 copy_image_info{
				.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
				.pNext = nullptr,
				.srcImage = this->texture->image,
				.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.dstImage = dst_texture->image,
				.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.regionCount = 1,
				.pRegions = &image_copy_region,
			};

			vkCmdCopyImage2(copy_image_cmd_buffer, &copy_image_info);

			for (auto const& [texture, old_layout] : {
				std::tuple{this->texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
				std::tuple{dst_texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL} }) {

				insert_image_memory_barrier(
					copy_image_cmd_buffer,
					texture->image,
					VK_PIPELINE_STAGE_2_COPY_BIT,
					VK_ACCESS_2_TRANSFER_WRITE_BIT,
					VK_PIPELINE_STAGE_2_NONE,
					VK_ACCESS_2_NONE,
					old_layout,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				);
			}

			if (vkEndCommandBuffer(copy_image_cmd_buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
			});
	}

	//void create_blit_image_cmd_buffers() {
	//	const VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::commandBufferAllocateInfo(engine->command_pool, copy_image_cmd_buffers.size());

	//	if (vkAllocateCommandBuffers(engine->device, &cmd_alloc_info, copy_image_cmd_buffers.data()) != VK_SUCCESS) {
	//		throw std::runtime_error("failed to allocate command buffers!");
	//	}

	//	for_each_field(engine->pbr_material_texture_set, [&](auto& dst_texture, auto index) {
	//		auto& copy_image_cmd_buffer = copy_image_cmd_buffers[index];
	//		const VkCommandBufferBeginInfo begin_info{
	//			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	//		};

	//		if (vkBeginCommandBuffer(copy_image_cmd_buffer, &begin_info) != VK_SUCCESS) {
	//			throw std::runtime_error("failed to begin recording command buffer!");
	//		}

	//		for (auto const& [texture, layout] : {
	//			std::tuple{texture,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
	//			std::tuple{dst_texture,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL} }) {

	//			auto image_memory_barrier = VkImageMemoryBarrier2{
	//				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	//				.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
	//				.srcAccessMask = VK_ACCESS_2_NONE,
	//				.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
	//				.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
	//				.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	//				.newLayout = layout,
	//				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	//				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	//				.image = texture->image,
	//				.subresourceRange = {
	//					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	//					.levelCount = 1,
	//					.layerCount = 1,
	//				},
	//			};

	//			const auto dependency_info = VkDependencyInfo{
	//				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	//				.imageMemoryBarrierCount = 1,
	//				.pImageMemoryBarriers = &image_memory_barrier,
	//			};

	//			vkCmdPipelineBarrier2(copy_image_cmd_buffer, &dependency_info);
	//		}
	//
	//		VkImageBlit2 image_blit_region{
	//			.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
	//			.srcSubresource = {
	//				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	//				.mipLevel = 0,
	//				.baseArrayLayer = 0,
	//				.layerCount = 1,
	//			},
	//			.srcOffsets = {
	//				{0, 0, 0},
	//				{static_cast<int32_t>(texture->width), static_cast<int32_t>(texture->height), 1},
	//			},
	//			.dstSubresource = {
	//				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	//				.mipLevel = 0,
	//				.baseArrayLayer = 0,
	//				.layerCount = 1,
	//			},
	//			.dstOffsets = {
	//				{0, 0, 0},
	//				{static_cast<int32_t>(dst_texture->width), static_cast<int32_t>(dst_texture->height), 1},
	//			},
	//		};

	//		VkBlitImageInfo2 blit_image_info{
	//			.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
	//			.srcImage = texture->image,
	//			.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	//			.dstImage = dst_texture->image,
	//			.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	//			.regionCount = 1,
	//			.pRegions = &image_blit_region,
	//			.filter =VK_FILTER_LINEAR,
	//		};

	//		vkCmdBlitImage2(copy_image_cmd_buffer, &blit_image_info);

	//		for (auto const& [texture, layout] : {
	//			std::tuple{texture,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
	//			std::tuple{dst_texture,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL} }) {

	//			auto image_memory_barrier = VkImageMemoryBarrier2{
	//				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	//				.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
	//				.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
	//				.dstStageMask = VK_PIPELINE_STAGE_2_NONE,
	//				.dstAccessMask = VK_ACCESS_2_NONE,
	//				.oldLayout = layout,
	//				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	//				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	//				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	//				.image = texture->image,
	//				.subresourceRange = {
	//					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	//					.levelCount = 1,
	//					.layerCount = 1,
	//				},
	//			};

	//			auto dependency_info = VkDependencyInfo{
	//				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	//				.imageMemoryBarrierCount = 1,
	//				.pImageMemoryBarriers = &image_memory_barrier,
	//			};

	//			vkCmdPipelineBarrier2(copy_image_cmd_buffer, &dependency_info);
	//		}

	//		if (vkEndCommandBuffer(copy_image_cmd_buffer) != VK_SUCCESS) {
	//			throw std::runtime_error("failed to record command buffer!");
	//		}
	//	});
	//}
};

template<typename UboType>
using ImageDataPtr = std::shared_ptr<ImageData<UboType>>;
