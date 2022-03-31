#pragma once

#include "../vk_image.h"
#include "../vk_buffer.h"
#include "../vk_pipeline.h"
#include "../vk_initializers.h"
#include "../vk_engine.h"
#include "../util/type_list.h"
#include "gui_node_texture_manager.h"
#include "imgui_color_gradient.h"

#include <imgui_impl_vulkan.h>


constexpr static inline uint32_t PREVIEW_IMAGE_SIZE = 128;

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
	static inline constexpr size_t value = [] {
		size_t field_count = 0;
		for (size_t i = 0; i < Reflect::class_t<T>::TotalFields; i++) {
			Reflect::class_t<T>::FieldAt(i, [&](auto& field) {
				using CurrentFieldType = typename std::remove_reference_t<decltype(field)>::Type;
				if constexpr (std::is_same_v<CurrentFieldType, FieldType>) {
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

struct IntData : NodeData {
	using value_t = int32_t;
	value_t value = 0;
};

struct FloatData : NodeData {
	using value_t = float;
	value_t value = 0.0f;
};

struct Float4Data : NodeData {
	using value_t = float[4];
	value_t value = { 0.0f };
};

struct Color4Data : NodeData {
	using value_t = float[4];
	value_t value = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct BoolData : NodeData {
	using value_t = bool;
	value_t value;
};

struct EnumData : NodeData {
	using value_t = uint32_t;
	value_t value;
};

struct TextureIdData : NodeData {
	using value_t = int32_t;
	value_t value = -1;
};

struct FloatTextureIdData : NodeData {
	using value_t = struct Value {
		float number = 0.0f;
		int32_t id = -1;
	};
	value_t value = Value{};
};

struct Color4TextureIdData : NodeData {
	using value_t = struct Value {
		float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		int32_t id = -1;
	};
	value_t value = Value{};
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

struct ColorRampData : NodeData {
	using value_t = int32_t;
	value_t value = -1;
	std::unique_ptr<ImGradient> ui_value;
	std::unique_ptr<RampTexture> ubo_value;
	ImGradientMark* draggingMark = nullptr;
	ImGradientMark* selectedMark = nullptr;

	ColorRampData() {
		ui_value = nullptr;
		ubo_value = nullptr;
	}
	ColorRampData(VulkanEngine* engine) {
		ubo_value = std::make_unique<RampTexture>(engine);
		value = ubo_value->color_ramp_texture_id;
		ui_value = std::make_unique<ImGradient>(static_cast<float*>(ubo_value->staging_buffer.mapped_buffer));
	}

	//ColorRampData(const ColorRampData& ramp_data) =default;

	/*ColorRampData(ColorRampData&&ramp_data) noexcept :
		value(ramp_data.value),
		ui_value(std::move(ramp_data.ui_value)),
		ubo_value(std::move(ramp_data.ubo_value)),
		draggingMark(ramp_data.draggingMark),
		selectedMark(ramp_data.selectedMark)
	{}

	ColorRampData& operator=(ColorRampData&& ramp_data) noexcept {
		ui_value = std::move(ramp_data.ui_value);
		ubo_value = std::move(ramp_data.ubo_value);
		value = ramp_data.value;
		draggingMark = ramp_data.draggingMark;
		selectedMark = ramp_data.selectedMark;
		return *this;
	}
	ColorRampData& operator=(const ColorRampData& ramp_data) = default;*/

	//~ColorRampData() {}
};

inline void to_json(json& j, const ColorRampData& p) {
	j = *p.ui_value;
}

namespace nlohmann {
	template <typename T> requires (std::derived_from<T, NodeData> && !std::same_as<T, ColorRampData>)
		struct adl_serializer<T> {
		static void to_json(json& j, const T& data) {
			j = data.value;
		}

		static void from_json(const json& j, T& data) {
			j.get_to(data.value);
		}
	};

	template <>
	struct adl_serializer<ColorRampData> {
		static void to_json(json& j, const ColorRampData& data) {
			j = *(data.ui_value);
		}
	};

	template <>
	struct adl_serializer<FloatTextureIdData> {
		static void to_json(json& j, const FloatTextureIdData& data) {
			j = data.value.number;
		}

		static void from_json(const json& j, FloatTextureIdData& data) {
			data = FloatTextureIdData{
				.value = {
					.number = j.get<float>()
				}
			};
		}
	};

	template <>
	struct adl_serializer<Color4TextureIdData> {
		static void to_json(json& j, const Color4TextureIdData& data) {
			j = data.value.color;
		}

		static void from_json(const json& j, Color4TextureIdData& data) {
			data = Color4TextureIdData{
				.value = {
					.color = j,
				}
			};
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

using PinTypeList = TypeList<
	TextureIdData,
	FloatData,
	IntData,
	BoolData,
	Color4Data,
	EnumData,
	ColorRampData,
	FloatTextureIdData,
	Color4TextureIdData
>;

using PinVariant = PinTypeList::cast_to<std::variant>;

template <typename T>
concept PinDataConcept = PinTypeList::has_type<T>;

template<typename UniformBufferType, typename ResultT, auto function>
struct ValueData : NodeData {
	using UboType = UniformBufferType;
	using ResultType = ResultT;

	inline static decltype(function) calculate = function;
};

template<typename UniformBufferType>
struct UboMixin {
	using UboType = UniformBufferType;

	BufferPtr uniform_buffer;

	explicit UboMixin(const VulkanEngine* engine): uniform_buffer(engine->material_preview_ubo){}

	void update_ubo(const PinVariant& value, size_t index) {  //use value to update the pin at the index  
		if constexpr (has_field_type_v<UboType, ColorRampData>) {
			typename UboType::value_t::Class::FieldAt(index, [&](auto& field_v) {
				using PinValueT = typename std::decay_t<decltype(field_v)>::Type;
				UboType::Class::FieldAt(index, [&](auto& field) {
					using PinT = typename std::decay_t<decltype(field)>::Type;
					uniform_buffer->copy_from_host(reinterpret_cast<const char*>(&std::get<PinT>(value)), sizeof(PinValueT), field_v.getOffset());
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
					uniform_buffer->copy_from_host(reinterpret_cast<const char*>(&std::get<PinT>(value)), sizeof(PinT), field.getOffset());
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
				uniform_buffer->copy_from_host(reinterpret_cast<const char*>(&std::get<PinT>(value)), sizeof(PinT), field.getOffset());
			}
			});
	}
};

template<typename UniformBufferType>
struct ShaderData : NodeData, UboMixin<UniformBufferType>{
	using UboType = UniformBufferType;

	explicit ShaderData(const VulkanEngine* engine): UboMixin<UniformBufferType>(engine){}
};

template<typename UniformBufferType, StringLiteral ...Shaders>
struct ImageData : NodeData, UboMixin<UniformBufferType> {
	using UboType = UniformBufferType;

	VulkanEngine* engine;
	TexturePtr texture;
	TexturePtr preview_texture;
	void* gui_texture;
	void* gui_preview_texture;

	VkDescriptorSet ubo_descriptor_set;
	VkFramebuffer image_processing_framebuffer;
	VkCommandBuffer image_processing_cmd_buffer;
	VkCommandBuffer generate_preview_cmd_buffer;
	std::array<VkCommandBuffer, PbrMaterialTextureNum> copy_image_cmd_buffers;

	VkSemaphore semaphore;

	std::vector<VkSemaphoreSubmitInfo> wait_semaphore_submit_info1;
	VkCommandBufferSubmitInfo cmd_buffer_submit_info1;
	VkSemaphoreSubmitInfo signal_semaphore_submit_info1;

	VkSemaphoreSubmitInfo wait_semaphore_submit_info2;
	VkCommandBufferSubmitInfo cmd_buffer_submit_info2;
	VkSemaphoreSubmitInfo signal_semaphore_submit_info2;

	std::array<VkSubmitInfo2, 2> submit_info;

	//VkImageMemoryBarrier2 image_memory_barrier;
	VkDependencyInfo dependency_info;

	int node_texture_id = -1;

	uint32_t width = 1024;
	uint32_t height = 1024;

	inline static VkDescriptorSetLayout ubo_descriptor_set_layout = nullptr;
	inline static VkPipelineLayout image_processing_pipeline_layout = nullptr;
	inline static VkPipeline image_processing_pipeline = nullptr;
	inline static VkRenderPass image_processing_render_pass = nullptr;

	explicit ImageData(VulkanEngine* engine) :UboMixin<UniformBufferType>(engine), engine(engine) {

		create_semaphore();

		create_texture();

		this->uniform_buffer = engine::Buffer::create_buffer(engine,
			sizeof(UboType),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			SWAPCHAIN_INDEPENDENT_BIT);

		if (ubo_descriptor_set_layout == nullptr) {
			create_ubo_descriptor_set_layout();
		}

		create_ubo_descriptor_set();

		update_descriptor_sets();

		if (image_processing_render_pass == nullptr) {
			create_image_processing_render_pass();
			create_image_processing_pipeline_layout();
			create_image_processing_pipeline();
		}

		create_framebuffer();

		create_image_processing_command_buffer();

		create_preview_command_buffer();

		create_cmd_buffer_submit_info();

		create_copy_image_cmd_buffers();
	}

	~ImageData() {
		engine->texture_manager->delete_id(node_texture_id);
		const std::array cmd_buffers{ image_processing_cmd_buffer, generate_preview_cmd_buffer };
		vkFreeCommandBuffers(engine->device, engine->command_pool, cmd_buffers.size(), cmd_buffers.data());
		vkDestroyFramebuffer(engine->device, image_processing_framebuffer, nullptr);
		vkFreeDescriptorSets(engine->device, engine->dynamic_descriptor_pool, 1, &ubo_descriptor_set);
		vkDestroySemaphore(engine->device, semaphore, nullptr);
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

		vkCreateSemaphore(engine->device, &semaphore_create_info, nullptr, &semaphore);
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
			PREVIEW_IMAGE_SIZE,
			PREVIEW_IMAGE_SIZE,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

		preview_texture->transitionImageLayout(engine, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


		gui_texture = ImGui_ImplVulkan_AddTexture(texture->sampler, texture->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		gui_preview_texture = ImGui_ImplVulkan_AddTexture(preview_texture->sampler, preview_texture->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		node_texture_id = engine->texture_manager->add_texture(texture);
	}

	void create_ubo_descriptor_set_layout() {
		auto const ubo_layout_binding = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
		std::array layout_bindings = { ubo_layout_binding };
		engine->create_descriptor_set_layout(layout_bindings, ubo_descriptor_set_layout);
	}

	void create_ubo_descriptor_set() {
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

	void update_descriptor_sets() {
		const VkDescriptorBufferInfo uniform_buffer_info{
			.buffer = this->uniform_buffer->buffer,
			.offset = 0,
			.range = sizeof(UboType)
		};

		const VkDescriptorImageInfo image_info{
			.sampler = texture->sampler,
			.imageView = texture->imageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
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

	void create_image_processing_render_pass() {
		constexpr VkAttachmentDescription color_attachment{
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		};

		constexpr VkAttachmentReference colorAttachmentRef{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		constexpr VkSubpassDescription subpass{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef,
		};

		//std::array dependencies{
		//	VkSubpassDependency {
		//		.srcSubpass = VK_SUBPASS_EXTERNAL,
		//		.dstSubpass = 0,
		//		.srcStageMask = 0,
		//		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		//		.srcAccessMask = 0,
		//		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		//		.dependencyFlags = 0,
		//	},
		//	VkSubpassDependency {
		//		.srcSubpass = 0,
		//		.dstSubpass = VK_SUBPASS_EXTERNAL,
		//		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		//		.dstStageMask = 0,
		//		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		//		.dstAccessMask = 0,
		//		.dependencyFlags = 0,
		//	}
		//};

		constexpr std::array attachments { color_attachment };

		constexpr VkRenderPassCreateInfo render_pass_info{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = nullptr,
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 0, //.dependencyCount = static_cast<uint32_t>(dependencies.size()),
			.pDependencies = nullptr, //dependencies.data(),
		};

		if (vkCreateRenderPass(engine->device, &render_pass_info, nullptr, &image_processing_render_pass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}

		engine->main_deletion_queue.push_function([device = engine->device, render_pass = image_processing_render_pass]() {
			vkDestroyRenderPass(device, render_pass, nullptr);
		});
	}

	void create_image_processing_pipeline_layout() {

		auto descriptor_set_layouts = [&]() {
			if constexpr (has_field_type_v<UboType, ColorRampData> || has_field_type_v<UboType, TextureIdData>) {
				return std::array{
					ubo_descriptor_set_layout,
					engine->texture_manager->descriptor_set_layout,
				};
			}
			else {
				return std::array{ ubo_descriptor_set_layout };
			}
		}();

		const VkPipelineLayoutCreateInfo image_processing_pipeline_info = vkinit::pipelineLayoutCreateInfo(descriptor_set_layouts);

		if (vkCreatePipelineLayout(engine->device, &image_processing_pipeline_info, nullptr, &image_processing_pipeline_layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		engine->main_deletion_queue.push_function([device = engine->device, pipeline_layout = image_processing_pipeline_layout]() {
			vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
		});
	}

	void create_image_processing_pipeline() {
		engine::PipelineBuilder pipeline_builder(engine, engine::ENABLE_DYNAMIC_VIEWPORT, engine::DISABLE_VERTEX_INPUT);
		std::array<std::string, sizeof...(Shaders)> shader_files;;
		size_t i = 0;
		((shader_files[i++] = Shaders.value), ...);
		auto shaders = engine::Shader::createFromSpv(engine, shader_files);

		for (auto shader_module : shaders->shader_modules) {
			VkPipelineShaderStageCreateInfo shader_info{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.stage = shader_module.stage,
				.module = shader_module.shader,
				.pName = "main"
			};
			pipeline_builder.shaderStages.emplace_back(std::move(shader_info));
		}

		pipeline_builder.build_pipeline(engine->device, image_processing_render_pass, image_processing_pipeline_layout, image_processing_pipeline);

		engine->main_deletion_queue.push_function([device = engine->device, pipeline = image_processing_pipeline]() {
			vkDestroyPipeline(device, pipeline, nullptr);
		});
	}

	void create_framebuffer() {
		std::array framebuffer_attachments = { texture->imageView };

		const VkFramebufferCreateInfo framebuffer_info = vkinit::framebufferCreateInfo(image_processing_render_pass, VkExtent2D{ width , height }, framebuffer_attachments);

		if (vkCreateFramebuffer(engine->device, &framebuffer_info, nullptr, &image_processing_framebuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}

	void create_image_processing_command_buffer() {
		const VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::commandBufferAllocateInfo(engine->command_pool, 1);

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

		const VkRenderPassBeginInfo render_pass_info = vkinit::renderPassBeginInfo(image_processing_render_pass, image_extent, image_processing_framebuffer);

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

		auto descriptor_sets = [&]() {
			if constexpr (has_field_type_v<UboType, ColorRampData> || has_field_type_v<UboType, TextureIdData>) {
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
		vkCmdBindPipeline(image_processing_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, image_processing_pipeline);
		vkCmdDraw(image_processing_cmd_buffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(image_processing_cmd_buffer);

		if (vkEndCommandBuffer(image_processing_cmd_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	void create_preview_command_buffer() {
		VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::commandBufferAllocateInfo(engine->command_pool, 1);

		if (vkAllocateCommandBuffers(engine->device, &cmd_alloc_info, &generate_preview_cmd_buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		VkCommandBufferBeginInfo begin_info{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		};

		if (vkBeginCommandBuffer(generate_preview_cmd_buffer, &begin_info) != VK_SUCCESS) {
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
			.commandBuffer = image_processing_cmd_buffer,
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

	void create_copy_image_cmd_buffers() {
		const VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::commandBufferAllocateInfo(engine->command_pool, copy_image_cmd_buffers.size());

		if (vkAllocateCommandBuffers(engine->device, &cmd_alloc_info, copy_image_cmd_buffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		for_each_field(engine->pbr_material_texture_set, [&](auto& dst_texture, auto index) {
			auto& copy_image_cmd_buffer = copy_image_cmd_buffers[index];
			const VkCommandBufferBeginInfo begin_info{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			};

			if (vkBeginCommandBuffer(copy_image_cmd_buffer, &begin_info) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			for (auto const& [texture, layout] : {
				std::tuple{texture,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
				std::tuple{dst_texture,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL} }) {

				auto image_memory_barrier = VkImageMemoryBarrier2{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
					.srcAccessMask = VK_ACCESS_2_NONE,
					.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
					.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					.newLayout = layout,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = texture->image,
					.subresourceRange = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.levelCount = 1,
						.layerCount = 1,
					},
				};

				const auto dependency_info = VkDependencyInfo{
					.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
					.imageMemoryBarrierCount = 1,
					.pImageMemoryBarriers = &image_memory_barrier,
				};

				vkCmdPipelineBarrier2(copy_image_cmd_buffer, &dependency_info);
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
					texture->width,
					texture->height,
					1,
				},
			};

			VkCopyImageInfo2 copy_image_info{
				.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
				.pNext = nullptr,
				.srcImage = texture->image,
				.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.dstImage = dst_texture->image,
				.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.regionCount = 1,
				.pRegions = &image_copy_region,
			};

			vkCmdCopyImage2(copy_image_cmd_buffer, &copy_image_info);

			for (auto const& [texture, layout] : {
				std::tuple{texture,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
				std::tuple{dst_texture,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL} }) {

				auto image_memory_barrier = VkImageMemoryBarrier2{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
					.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_2_NONE,
					.dstAccessMask = VK_ACCESS_2_NONE,
					.oldLayout = layout,
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

				vkCmdPipelineBarrier2(copy_image_cmd_buffer, &dependency_info);
			}

			if (vkEndCommandBuffer(copy_image_cmd_buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		});
	}
};

template<typename UboType, StringLiteral ...Shaders>
using ImageDataPtr = std::shared_ptr<ImageData<UboType, Shaders...>>;
