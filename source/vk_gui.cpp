#include "vk_gui.h"
#include "vk_engine.h"
#include "vk_initializers.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"
#include "vk_util.h"

#include <filesystem>

namespace engine {
	void GUI::initRenderPass() {
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = engine->swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;  // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(engine->device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}

		engine->main_deletion_queue.push_function([=]() {
			vkDestroyRenderPass(engine->device, renderPass, nullptr);
			});
	}

	void GUI::create_framebuffers() {
		framebuffers.resize(engine->swapchain_image_count);

		for (size_t i = 0; i < engine->swapchain_image_count; i++) {
			std::array<VkImageView, 1> attachments = {
				engine->swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo = vkinit::framebufferCreateInfo(renderPass, engine->swapChainExtent, attachments);

			if (vkCreateFramebuffer(engine->device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}

			engine->swapChainDeletionQueue.push_function([=]() {
				vkDestroyFramebuffer(engine->device, framebuffers[i], nullptr);
				});
		}
	}

	void GUI::initCommandPool() {
		uint32_t queueFamilyIndex = engine->queueFamilyIndices.graphicsFamily.value();
		VkCommandPoolCreateInfo commandPoolInfo = vkinit::commandPoolCreateInfo(queueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		if (vkCreateCommandPool(engine->device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics command pool!");
		}

		engine->main_deletion_queue.push_function([=]() {
			vkDestroyCommandPool(engine->device, commandPool, nullptr);
			});
	}

	void GUI::initCommandBuffers() {
		commandBuffers.resize(engine->swapchain_image_count);
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(commandPool, (uint32_t)commandBuffers.size());

		if (vkAllocateCommandBuffers(engine->device, &cmdAllocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		engine->main_deletion_queue.push_function([=]() {
			vkFreeCommandBuffers(engine->device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
			});
	}


	void GUI::init(VulkanEngine* vulkanEngine) {
		engine = vulkanEngine;
		initCommandPool();
		initRenderPass();
		create_framebuffers();
		initCommandBuffers();

		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 1000;
		poolInfo.poolSizeCount = std::size(pool_sizes);
		poolInfo.pPoolSizes = pool_sizes;

		VkDescriptorPool imguiDescriptorPool;
		if (vkCreateDescriptorPool(engine->device, &poolInfo, nullptr, &imguiDescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool for imgui!");
		}

		// 2: initialize imgui library

		//this initializes the core structures of imgui
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		//ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForVulkan(engine->window, true);

		//this initializes imgui for Vulkan
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = engine->instance;
		init_info.PhysicalDevice = engine->physicalDevice;
		init_info.Device = engine->device;
		init_info.Queue = engine->graphicsQueue;
		init_info.DescriptorPool = imguiDescriptorPool;
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info, renderPass);

		//execute a gpu command to upload imgui font textures

		ImFont* font = io.Fonts->AddFontFromFileTTF((std::filesystem::path(std::getenv("WINDIR")) / "Fonts" / "segoeui.ttf").string().c_str(), 22.0f);
		assert(font != NULL);

		immediateSubmit(engine, [&](VkCommandBuffer cmd) {
			ImGui_ImplVulkan_CreateFontsTexture(cmd);
			});

		//clear font textures from cpu data
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		//init engine->viewport3D.gui_textures
		for (size_t i = 0; i < engine->swapchain_image_count; i++) {
			engine->viewport3D.gui_textures.emplace_back(ImGui_ImplVulkan_AddTexture(engine->viewport3D.color_textures[i]->sampler, engine->viewport3D.color_textures[i]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		}

		//add the destroy the imgui created structures
		engine->main_deletion_queue.push_function([=]() {
			vkDestroyDescriptorPool(engine->device, imguiDescriptorPool, nullptr);
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			});
	}

	//void GUI::createSwapchainResources() {
	//	initRenderPass();
	//	create_framebuffers();
	//}

	void GUI::beginRender()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}


	void GUI::endRender(VulkanEngine* engine, const uint32_t imageIndex)
	{
		ImGui::Render();

		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffers[imageIndex], &commandBufferBeginInfo);

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = framebuffers[imageIndex];
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = engine->swapChainExtent;

		VkClearValue clearValues{};
		clearValues.color = { 1.0f, 0.0f, 0.0f, 1.0f };

		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = &clearValues;

		vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[imageIndex]);
		vkCmdEndRenderPass(commandBuffers[imageIndex]);
		vkEndCommandBuffer(commandBuffers[imageIndex]);
	}

}