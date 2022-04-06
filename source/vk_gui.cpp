#include "vk_gui.h"
#include "vk_initializers.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"
#include "vk_util.h"
#include "vk_image.h"

#include <IconsFontAwesome5.h>
#include <filesystem>
#include <array>

namespace engine {
	void GUI::init_render_pass() {
		const VkAttachmentDescription color_attachment{
			.format = engine->swapchain_image_format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		};

		constexpr VkAttachmentReference color_attachment_ref{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		const VkSubpassDescription subpass{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment_ref,
		};

		constexpr VkSubpassDependency dependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,  // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		};

		const VkRenderPassCreateInfo render_pass_info{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &color_attachment,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency,
		};

		if (vkCreateRenderPass(engine->device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}

		engine->main_deletion_queue.push_function([=] {
			vkDestroyRenderPass(engine->device, render_pass, nullptr);
			});
	}

	void GUI::create_framebuffers() {
		framebuffers.resize(engine->swapchain_image_count);

		for (size_t i = 0; i < engine->swapchain_image_count; i++) {
			std::array attachments{
				engine->swapchain_image_views[i]
			};

			VkFramebufferCreateInfo framebuffer_info = vkinit::framebufferCreateInfo(render_pass, engine->swapchain_extent, attachments);

			if (vkCreateFramebuffer(engine->device, &framebuffer_info, nullptr, &framebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}

			engine->swap_chain_deletion_queue.push_function([=] {
				vkDestroyFramebuffer(engine->device, framebuffers[i], nullptr);
				});
		}
	}

	void GUI::init_command_pool() {
		const uint32_t queue_family_index = engine->queue_family_indices.graphics_family.value();
		auto const command_pool_info = vkinit::commandPoolCreateInfo(queue_family_index, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		if (vkCreateCommandPool(engine->device, &command_pool_info, nullptr, &command_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics command pool!");
		}

		engine->main_deletion_queue.push_function([=] {
			vkDestroyCommandPool(engine->device, command_pool, nullptr);
			});
	}

	void GUI::init_command_buffers() {
		command_buffers.resize(engine->swapchain_image_count);
		const VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::commandBufferAllocateInfo(command_pool, static_cast<uint32_t>(command_buffers.size()));

		if (vkAllocateCommandBuffers(engine->device, &cmd_alloc_info, command_buffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		engine->main_deletion_queue.push_function([=] {
			vkFreeCommandBuffers(engine->device, command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());
			});
	}


	void GUI::init(VulkanEngine* vulkan_engine) {
		engine = vulkan_engine;
		init_command_pool();
		init_render_pass();
		create_framebuffers();
		init_command_buffers();

		constexpr size_t descriptor_pool_size = 2000;

		constexpr VkDescriptorPoolSize pool_sizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, descriptor_pool_size },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptor_pool_size },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptor_pool_size },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, descriptor_pool_size },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, descriptor_pool_size },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, descriptor_pool_size },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptor_pool_size },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptor_pool_size },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, descriptor_pool_size },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, descriptor_pool_size },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, descriptor_pool_size }
		};

		const VkDescriptorPoolCreateInfo pool_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			.maxSets = 1000,
			.poolSizeCount = std::size(pool_sizes),
			.pPoolSizes = pool_sizes,
		};

		VkDescriptorPool imgui_descriptor_pool;
		if (vkCreateDescriptorPool(engine->device, &pool_info, nullptr, &imgui_descriptor_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool for imgui!");
		}

		// 2: initialize imgui library

		//this initializes the core structures of imgui
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForVulkan(engine->window, true);

		//this initializes imgui for Vulkan
		ImGui_ImplVulkan_InitInfo init_info{
			.Instance = engine->instance,
			.PhysicalDevice = engine->physical_device,
			.Device = engine->device,
			.Queue = engine->graphics_queue,
			.DescriptorPool = imgui_descriptor_pool,
			.MinImageCount = 3,
			.ImageCount = 3,
			.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		};

		ImGui_ImplVulkan_Init(&init_info, render_pass);

		//execute a gpu command to upload imgui font textures
		static constexpr ImWchar ranges[] = {
			0x0020, 0x00FF, // Basic Latin + Latin Supplement
			0x0104, 0x017C, // Polish characters and more
			0,
		};

		ImFontConfig config{};
		config.OversampleH = 7;
		config.OversampleV = 4;
		config.PixelSnapH = false;
		ImFont* font = io.Fonts->AddFontFromFileTTF((std::filesystem::path(std::getenv("WINDIR")) / "Fonts" / "segoeui.ttf").string().c_str(), 22.0f, &config, ranges);
		assert(font != NULL);

		static constexpr ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig icons_config{};
		icons_config.OversampleH = 8;
		icons_config.OversampleV = 4;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.GlyphMinAdvanceX = 25.0f;
		io.Fonts->AddFontFromFileTTF((std::filesystem::path("assets") / "fonts" / FONT_ICON_FILE_NAME_FAS).string().c_str(), 18.0f, &icons_config, icons_ranges);

		immediate_submit(engine, [&](VkCommandBuffer cmd) {
			ImGui_ImplVulkan_CreateFontsTexture(cmd);
			});

		//clear font textures from cpu data
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		//init engine->viewport_3d.gui_textures
		for (size_t i = 0; i < engine->swapchain_image_count; i++) {
			engine->viewport_3d.gui_textures.emplace_back(ImGui_ImplVulkan_AddTexture(
				engine->viewport_3d.color_resolve_textures[i]->sampler,
				engine->viewport_3d.color_resolve_textures[i]->image_view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			));
		}

		//add the destroy the imgui created structures
		engine->main_deletion_queue.push_function([=] {
			vkDestroyDescriptorPool(engine->device, imgui_descriptor_pool, nullptr);
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			});
	}

	void GUI::begin_render() {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}


	void GUI::end_render(const uint32_t image_index) {
		ImGui::Render();

		const VkCommandBufferBeginInfo command_buffer_begin_info{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
		vkBeginCommandBuffer(command_buffers[image_index], &command_buffer_begin_info);

		constexpr VkClearValue clear_value{
			.color = { {1.0f, 0.0f, 0.0f, 1.0f} }
		};

		const VkRenderPassBeginInfo render_pass_begin_info{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = render_pass,
			.framebuffer = framebuffers[image_index],
			.renderArea = {
				.offset = {0, 0},
				.extent = engine->swapchain_extent,
			},
			.clearValueCount = 1,
			.pClearValues = &clear_value,

		};

		vkCmdBeginRenderPass(command_buffers[image_index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffers[image_index]);
		vkCmdEndRenderPass(command_buffers[image_index]);
		vkEndCommandBuffer(command_buffers[image_index]);
	}

}