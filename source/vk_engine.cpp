#include "vk_engine.h"
#include "vk_shader.h"
#include "vk_mesh.h"
#include "vk_camera.h"
#include "vk_gui.h"
#include "gui/gui_node_editor.h"
#include "gui/ImGuiFileDialog.h"

#include <cstring>
#include <array>
#include <set>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <iostream>

#include <IconsFontAwesome5.h>

#include <GLFW/glfw3.h>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#include <glm/gtc/type_ptr.hpp>

#include <json.hpp>
using json = nlohmann::json;

#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

using namespace std::string_view_literals;
using namespace engine;

constexpr uint32_t WIDTH = 1200;
constexpr uint32_t HEIGHT = 900;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;
constexpr double MAX_FPS = 200.0;
constexpr double MAX_PERIOD = 1.0 / MAX_FPS;



const std::vector validationLayers{
	"VK_LAYER_KHRONOS_validation",
	"VK_LAYER_KHRONOS_synchronization2",
};

constexpr std::array<const char*, 0> instance_extensions{

};

constexpr std::array deviceExtensions{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	//VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto const func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto const func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};


SphericalCoord camSphericalCoord{ glm::radians(0.0f) ,glm::radians(90.0f), 5.46f, glm::vec3(0.0f) };


Camera VulkanEngine::camera = Camera(camSphericalCoord, glm::radians(45.0f), 1.0f, 0.01f, 1000.0f);
glm::vec2 VulkanEngine::mouse_previous_pos = glm::vec2(0.0);
glm::vec2 VulkanEngine::mouse_delta_pos = glm::vec2(0.0);
bool VulkanEngine::mouse_hover_viewport = false;


void VulkanEngine::init_window() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

	double x_pos, y_pos;
	glfwGetCursorPos(window, &x_pos, &y_pos);
	mouse_previous_pos.x = x_pos;
	mouse_previous_pos.y = y_pos;
	glfwSetCursorPosCallback(window, mouse_cursor_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, mouse_scroll_callback);
}

void VulkanEngine::framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
	auto const app = static_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
	app->framebuffer_resized = true;
}

void VulkanEngine::init_vulkan() {
	create_instance();
	setup_debug_messenger();
	create_surface();
	pick_physical_device();
	create_logical_device();

	create_swap_chain();//recreateSwapChain
	create_swap_chain_image_views();//recreateSwapChain

	create_sync_objects();

	create_command_pool();

	create_descriptor_pool();
	create_texture_manager();
	parse_material_info();
	create_descriptor_set_layouts();

	create_viewport_attachments();
	create_viewport_render_pass();
	create_viewport_framebuffers();
	create_viewport_cmd_buffers();

	create_uniform_buffers();
	load_obj();
	create_descriptor_sets();
	create_graphics_pipeline();

	set_camera();

	init_imgui();

	is_initialized = true;
}



void VulkanEngine::main_loop() {
	//bool running = true;
	double last_time = 0.0;

	while (!glfwWindowShouldClose(window)) {
		const double time = glfwGetTime();
		const double delta_time = time - last_time;
		if (delta_time >= MAX_PERIOD) {
			last_time = time;
			glfwPollEvents();
			draw_frame();
		}
	}

	vkDeviceWaitIdle(device);
}

void VulkanEngine::cleanup() {

	if (is_initialized) {
		vkDeviceWaitIdle(device);

		swap_chain_deletion_queue.flush();
		main_deletion_queue.flush();

		vkDestroyDevice(device, nullptr);

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}
}

void VulkanEngine::recreate_swap_chain() {

	glfwGetFramebufferSize(window, &window_width, &window_height);
	while (window_width == 0 || window_height == 0) {
		glfwGetFramebufferSize(window, &window_width, &window_height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	swap_chain_deletion_queue.flush();

	create_swap_chain();
	create_swap_chain_image_views();

	set_camera();

	images_in_flight.resize(swapchain_image_count, VK_NULL_HANDLE);

	gui->create_framebuffers();
	ImGui_ImplVulkan_SetMinImageCount(swapchain_image_count);
}

void VulkanEngine::create_instance() {
	if (enableValidationLayers && !check_validation_layer_support()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo app_info{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3,
	};

	auto extensions = get_required_extensions();
	VkInstanceCreateInfo create_info{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
	};

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
	if constexpr (enableValidationLayers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		create_info.ppEnabledLayerNames = validationLayers.data();

		populate_debug_messenger_create_info(debug_create_info);
		create_info.pNext = &debug_create_info;
	}
	else {
		create_info.enabledLayerCount = 0;
		create_info.pNext = nullptr;
	}

	if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

void VulkanEngine::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
	create_info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = debug_callback,
	};
}

void VulkanEngine::setup_debug_messenger() {
	if constexpr (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT create_info;
	populate_debug_messenger_create_info(create_info);

	if (CreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}

	main_deletion_queue.push_function([=]() {
		DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
		});
}

void VulkanEngine::create_surface() {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void VulkanEngine::pick_physical_device() {
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

	if (device_count == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

	for (const auto& device : devices) {
		queue_family_indices = find_queue_families(device);
		if (is_device_suitable(device)) {
			physical_device = device;
			if (msaa_samples != VK_SAMPLE_COUNT_1_BIT) {
				msaa_samples = get_max_usable_sample_count();
			}
			break;
		}
	}

	if (physical_device == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

void VulkanEngine::create_logical_device() {
	// queue_family_indices = find_queue_families(physical_device);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> unique_queue_families = {
		queue_family_indices.graphics_family.value(),
		queue_family_indices.present_family.value()
	};

	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families) {
		queueCreateInfos.emplace_back(
			VkDeviceQueueCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = queue_family,
				.queueCount = 1,
				.pQueuePriorities = &queue_priority,
			});
	}

	VkPhysicalDeviceVulkan13Features vk13_features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = nullptr,
		.synchronization2 = VK_TRUE,
	};

	VkPhysicalDeviceVulkan12Features vk12_features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &vk13_features,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
		.descriptorBindingPartiallyBound = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
		.timelineSemaphore = VK_TRUE,
	};

	VkPhysicalDeviceFeatures device_features{
		.sampleRateShading = VK_TRUE,
		.samplerAnisotropy = VK_TRUE,
	};

	VkDeviceCreateInfo device_create_info{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &vk12_features,
		.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures = &device_features,
	};

	if constexpr (enableValidationLayers) {
		device_create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		device_create_info.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		device_create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device, queue_family_indices.graphics_family.value(), 0, &graphics_queue);
	vkGetDeviceQueue(device, queue_family_indices.present_family.value(), 0, &present_queue);
}

void VulkanEngine::create_swap_chain() {
	auto const [swap_chain_capabilities, swap_chain_formats, swap_chain_presentModes] = query_swap_chain_support(physical_device);
	auto const [surface_format, surface_colorSpace] = choose_swap_surface_format(swap_chain_formats);
	const VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_presentModes);
	swapchain_extent = choose_swap_extent(swap_chain_capabilities);

	swapchain_image_count = swap_chain_capabilities.minImageCount + 1;
	if (swap_chain_capabilities.maxImageCount > 0 && swapchain_image_count > swap_chain_capabilities.maxImageCount) {
		swapchain_image_count = swap_chain_capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR create_info{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = swapchain_image_count,
		.imageFormat = surface_format,
		.imageColorSpace = surface_colorSpace,
		.imageExtent = swapchain_extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	};

	const std::array queue_family_index_array{
		queue_family_indices.graphics_family.value(),
		queue_family_indices.present_family.value()
	};

	if (queue_family_indices.graphics_family != queue_family_indices.present_family) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = queue_family_index_array.size();
		create_info.pQueueFamilyIndices = queue_family_index_array.data();
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	create_info.preTransform = swap_chain_capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);
	swapchain_images.resize(swapchain_image_count);
	vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());

	swapchain_image_format = surface_format;

	swap_chain_deletion_queue.push_function([=]() {
		vkDestroySwapchainKHR(device, swapchain, nullptr);
		});
}

void VulkanEngine::create_swap_chain_image_views() {
	swapchain_image_views.resize(swapchain_images.size());

	for (uint32_t i = 0; i < swapchain_images.size(); i++) {
		swapchain_image_views[i] = create_image_view(swapchain_images[i], swapchain_image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1, SWAPCHAIN_DEPENDENT_BIT);
	}
}

//void VulkanEngine::create_render_pass() {
//	VkAttachmentDescription colorAttachment{
//		.format = swapchain_image_format,
//		.samples = msaa_samples,
//		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
//		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
//		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
//		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
//		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
//		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
//	};
//
//	VkAttachmentDescription depthAttachment{};
//	depthAttachment.format = find_depth_format();
//	depthAttachment.samples = msaa_samples;
//	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//	VkAttachmentDescription colorAttachmentResolve{};
//	colorAttachmentResolve.format = swapchain_image_format;
//	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
//	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//	VkAttachmentReference colorAttachmentRef{};
//	colorAttachmentRef.attachment = 0;
//	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//	VkAttachmentReference depthAttachmentRef{};
//	depthAttachmentRef.attachment = 1;
//	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//	VkAttachmentReference colorAttachmentResolveRef{};
//	colorAttachmentResolveRef.attachment = 2;
//	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//	VkSubpassDescription subpass{};
//	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//	subpass.colorAttachmentCount = 1;
//	subpass.pColorAttachments = &colorAttachmentRef;
//	subpass.pDepthStencilAttachment = &depthAttachmentRef;
//	subpass.pResolveAttachments = &colorAttachmentResolveRef;
//
//	VkSubpassDependency dependency{};
//	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
//	dependency.dstSubpass = 0;
//	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//	dependency.srcAccessMask = 0;
//	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//
//	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
//	VkRenderPassCreateInfo renderPassInfo{};
//	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
//	renderPassInfo.pAttachments = attachments.data();
//	renderPassInfo.subpassCount = 1;
//	renderPassInfo.pSubpasses = &subpass;
//	renderPassInfo.dependencyCount = 1;
//	renderPassInfo.pDependencies = &dependency;
//
//	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &render_pass) != VK_SUCCESS) {
//		throw std::runtime_error("failed to create render pass!");
//	}
//
//	swap_chain_deletion_queue.push_function([=]() {
//		vkDestroyRenderPass(device, render_pass, nullptr);
//		});
//}

//void VulkanEngine::load_gltf() {
//	const std::string filename = "assets/gltf_models/dragon.gltf";
//	tinygltf::Model gltf_model;
//	tinygltf::TinyGLTF gltf_context;
//	std::string error, warning;
//
//	const bool file_loaded = gltf_context.LoadASCIIFromFile(&gltf_model, &error, &warning, filename);
//
//	if (file_loaded) {
//		for (size_t i = 0; i < gltf_model.images.size(); i++) {
//			tinygltf::Image& gltf_image = gltf_model.images[i];
//			loaded_textures.emplace_back(engine::Texture::load_2d_texture_from_host(this, gltf_image.image.data(), gltf_image.width, gltf_image.height, gltf_image.component));
//		}
//
//		const std::array shader_file_paths{
//			"assets/shaders/pbr.vert.spv"sv,
//			"assets/shaders/pbr.frag.spv"sv
//		};
//
//		auto const pbr_shader = engine::Shader::createFromSpv(this, shader_file_paths);
//
//		for (auto& gltf_material : gltf_model.materials) {
//			auto material = std::make_shared<PbrMaterial>();
//			material->pShaders = pbr_shader;
//			auto& paras = material->paras;
//
//			paras.baseColorRed = gltf_material.pbrMetallicRoughness.baseColorFactor[0];
//			paras.baseColorGreen = gltf_material.pbrMetallicRoughness.baseColorFactor[1];
//			paras.baseColorBlue = gltf_material.pbrMetallicRoughness.baseColorFactor[2];
//			paras.baseColorTextureID = gltf_material.pbrMetallicRoughness.baseColorTexture.index;
//
//			paras.metalnessFactor = gltf_material.pbrMetallicRoughness.metallicFactor;
//			paras.roughnessFactor = gltf_material.pbrMetallicRoughness.roughnessFactor;
//			paras.metallicRoughnessTextureId = gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index;
//
//			loaded_materials.emplace_back(material);
//			materials.try_emplace(gltf_material.name, std::move(material));
//		}
//
//		for (auto& gltf_mesh : gltf_model.meshes) {
//			loaded_meshes.emplace_back(engine::Mesh::load_from_gltf(this, gltf_model, gltf_mesh));
//		}
//
//		// Only support one scene
//		for (auto const node_index : gltf_model.scenes[0].nodes) {
//			const tinygltf::Node& node = gltf_model.nodes[node_index];
//
//			if (node.mesh > -1) {
//				RenderObject render_object;
//				render_object.mesh = loaded_meshes[node.mesh];
//				if (node.translation.size() == 3) {
//					render_object.transform_matrix = glm::translate(render_object.transform_matrix, glm::vec3(glm::make_vec3(node.translation.data())));
//				}
//				if (node.rotation.size() == 4) {
//					glm::quat q = glm::make_quat(node.rotation.data());
//					render_object.transform_matrix *= glm::mat4(q);
//				}
//				if (node.scale.size() == 3) {
//					render_object.transform_matrix = glm::scale(render_object.transform_matrix, glm::vec3(glm::make_vec3(node.scale.data())));
//				}
//				if (node.matrix.size() == 16) {
//					render_object.transform_matrix = glm::make_mat4x4(node.matrix.data());
//				};
//				renderables.emplace_back(std::move(render_object));
//			}
//		}
//	}
//}



void VulkanEngine::parse_material_info() {

	//load_gltf();

	auto env_material_info_json = R"(
	{
		"type": "cubemap",
		"filePaths": [
			"assets/textures/HDRi/output_pos_x.hdr",
			"assets/textures/HDRi/output_neg_x.hdr",
			"assets/textures/HDRi/output_pos_y.hdr",
			"assets/textures/HDRi/output_neg_y.hdr",
			"assets/textures/HDRi/output_pos_z.hdr",
			"assets/textures/HDRi/output_neg_z.hdr"
		],
		"irradianceMapPaths": [
			"assets/textures/HDRi/irradiance_map/irradiance_map_pos_x.exr",
			"assets/textures/HDRi/irradiance_map/irradiance_map_neg_x.exr",
			"assets/textures/HDRi/irradiance_map/irradiance_map_pos_y.exr",
			"assets/textures/HDRi/irradiance_map/irradiance_map_neg_y.exr",
			"assets/textures/HDRi/irradiance_map/irradiance_map_pos_z.exr",
			"assets/textures/HDRi/irradiance_map/irradiance_map_neg_z.exr"
		],
		"prefilteredMapPaths": [
			[
				"assets/textures/HDRi/output_pos_x.hdr",
				"assets/textures/HDRi/output_neg_x.hdr",
				"assets/textures/HDRi/output_pos_y.hdr",
				"assets/textures/HDRi/output_neg_y.hdr",
				"assets/textures/HDRi/output_pos_z.hdr",
				"assets/textures/HDRi/output_neg_z.hdr"
			], [	
				"assets/textures/HDRi/prefiltered_map/512@0.2/pre-filtered_map_pos_x.exr",
				"assets/textures/HDRi/prefiltered_map/512@0.2/pre-filtered_map_neg_x.exr",
				"assets/textures/HDRi/prefiltered_map/512@0.2/pre-filtered_map_pos_y.exr",
				"assets/textures/HDRi/prefiltered_map/512@0.2/pre-filtered_map_neg_y.exr",
				"assets/textures/HDRi/prefiltered_map/512@0.2/pre-filtered_map_pos_z.exr",
				"assets/textures/HDRi/prefiltered_map/512@0.2/pre-filtered_map_neg_z.exr"
			], [
				"assets/textures/HDRi/prefiltered_map/256@0.4/pre-filtered_map_pos_x.exr",
				"assets/textures/HDRi/prefiltered_map/256@0.4/pre-filtered_map_neg_x.exr",
				"assets/textures/HDRi/prefiltered_map/256@0.4/pre-filtered_map_pos_y.exr",
				"assets/textures/HDRi/prefiltered_map/256@0.4/pre-filtered_map_neg_y.exr",
				"assets/textures/HDRi/prefiltered_map/256@0.4/pre-filtered_map_pos_z.exr",
				"assets/textures/HDRi/prefiltered_map/256@0.4/pre-filtered_map_neg_z.exr"
			], [
				"assets/textures/HDRi/prefiltered_map/128@0.6/pre-filtered_map_pos_x.exr",
				"assets/textures/HDRi/prefiltered_map/128@0.6/pre-filtered_map_neg_x.exr",
				"assets/textures/HDRi/prefiltered_map/128@0.6/pre-filtered_map_pos_y.exr",
				"assets/textures/HDRi/prefiltered_map/128@0.6/pre-filtered_map_neg_y.exr",
				"assets/textures/HDRi/prefiltered_map/128@0.6/pre-filtered_map_pos_z.exr",
				"assets/textures/HDRi/prefiltered_map/128@0.6/pre-filtered_map_neg_z.exr"
			], [
				"assets/textures/HDRi/prefiltered_map/64@0.8/pre-filtered_map_pos_x.exr",
				"assets/textures/HDRi/prefiltered_map/64@0.8/pre-filtered_map_neg_x.exr",
				"assets/textures/HDRi/prefiltered_map/64@0.8/pre-filtered_map_pos_y.exr",
				"assets/textures/HDRi/prefiltered_map/64@0.8/pre-filtered_map_neg_y.exr",
				"assets/textures/HDRi/prefiltered_map/64@0.8/pre-filtered_map_pos_z.exr",
				"assets/textures/HDRi/prefiltered_map/64@0.8/pre-filtered_map_neg_z.exr"
			], [
				"assets/textures/HDRi/prefiltered_map/32@1.0/pre-filtered_map_pos_x.exr",
				"assets/textures/HDRi/prefiltered_map/32@1.0/pre-filtered_map_neg_x.exr",
				"assets/textures/HDRi/prefiltered_map/32@1.0/pre-filtered_map_pos_y.exr",
				"assets/textures/HDRi/prefiltered_map/32@1.0/pre-filtered_map_neg_y.exr",
				"assets/textures/HDRi/prefiltered_map/32@1.0/pre-filtered_map_pos_z.exr",
				"assets/textures/HDRi/prefiltered_map/32@1.0/pre-filtered_map_neg_z.exr"
			]
		],
		"BRDF_2D_LUT": "assets/textures/HDRi/BRDF_2D_LUT/BRDF_2D_LUT.png"
	}
	)"_json;


	if (env_material_info_json["type"].get<std::string>() == "cubemap") {
		materials.emplace("env_light", std::make_shared<HDRiMaterial>());
		auto const env_mat = std::get<HDRiMaterialPtr>(materials["env_light"]);
		std::array<std::string, 2> spvFilePaths = {
			"assets/shaders/env_cubemap.vert.spv",
			"assets/shaders/env_cubemap.frag.spv"
		};
		env_mat->shaders = engine::Shader::createFromSpv(this, std::move(spvFilePaths));
		//envMat->textureArrayIndex.emplace("cubemap", loaded_textures.size());
		//envMat->paras.baseColorTextureID = loaded_textures.size();
		env_mat->paras.base_color_texture_id = texture_manager->add_texture(
			engine::Texture::load_cubemap_texture(this,
				env_material_info_json["filePaths"].get<std::vector<std::string>>()));
		env_mat->textureArrayIndex.emplace("cubemap", env_mat->paras.base_color_texture_id);

		/*for (auto const& mat : loaded_materials) {
			mat->paras.irradianceMapId = loaded_textures.size();
		}*/
		init_material_preview_ubo.irradiance_map_id = texture_manager->add_texture(
			engine::Texture::load_cubemap_texture(this,
				env_material_info_json["irradianceMapPaths"].get<std::vector<std::string>>()));

		/*for (auto const& mat : loaded_materials) {
			mat->paras.prefilteredMapId = loaded_textures.size();
		}*/
		init_material_preview_ubo.prefiltered_map_id = texture_manager->add_texture(
			engine::Texture::load_prefiltered_map_texture(this,
				env_material_info_json["prefilteredMapPaths"].get<std::vector<std::vector<std::string>>>()));

		/*for (auto const& mat : loaded_materials) {
			mat->paras.brdfLUTId = loaded_textures.size();
		}*/
		init_material_preview_ubo.brdf_LUT_id = texture_manager->add_texture(
			engine::Texture::load_2d_texture(this,
				env_material_info_json["BRDF_2D_LUT"].get<std::string>(),
				false));
	}

	//for (auto const& mat : loaded_materials) {
	//	mat->paras.texture2DArraySize = loaded_textures.size();
	//}

	//for (auto& mat : materials | std::views::values) {
	//	std::visit([&](auto& obj) {
	//		if constexpr (!std::same_as<MaterialPtr, std::decay_t<decltype(obj)>>) {
	//			obj->paras.textureCubemapArraySize = loaded_textures.size();
	//		}
	//		}, mat);
	//}

	RenderObject skyBoxObject;
	skyBoxObject.mesh = Mesh::load_from_obj(this, "assets/obj_models/skybox.obj");
	skyBoxObject.mesh->material = std::get<HDRiMaterialPtr>(materials["env_light"]);
	skyBoxObject.transform_matrix = glm::mat4{ 1.0f };
	renderables.emplace_back(std::move(skyBoxObject));
}

void VulkanEngine::create_descriptor_set_layouts() {

	auto const cam_ubo_layout_binding = vkinit::descriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0);

	auto const material_preview_layout_binding = vkinit::descriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		1);

	std::array scene_bindings{
		cam_ubo_layout_binding,
		material_preview_layout_binding,
	};

	create_descriptor_set_layout(scene_bindings, scene_set_layout);
}


void VulkanEngine::create_descriptor_set_layout(std::span<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings, VkDescriptorSetLayout& descriptor_set_layout) {

	const VkDescriptorSetLayoutCreateInfo layout_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(descriptor_set_layout_bindings.size()),
		.pBindings = descriptor_set_layout_bindings.data(),
	};

	if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	main_deletion_queue.push_function([=]() {
		vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
		});
}

void VulkanEngine::create_mesh_pipeline() {

	PipelineBuilder pipeline_builder(this);
	pipeline_builder.set_msaa(msaa_samples);

	std::array mesh_descriptor_set_layouts = { scene_set_layout, texture_manager->descriptor_set_layout };

	const VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkinit::pipelineLayoutCreateInfo(mesh_descriptor_set_layouts);

	if (vkCreatePipelineLayout(device, &mesh_pipeline_layout_info, nullptr, &mesh_pipeline_layout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	swap_chain_deletion_queue.push_function([=]() {
		vkDestroyPipelineLayout(device, mesh_pipeline_layout, nullptr);
		});

	for (auto const& material : loaded_materials) {
		pipeline_builder.set_shader_stages(material);

		pipeline_builder.depthStencil = vkinit::depthStencilCreateInfo(VK_COMPARE_OP_LESS);

		pipeline_builder.build_pipeline(device, viewport_3d.render_pass, mesh_pipeline_layout, material->pipeline);

		main_deletion_queue.push_function([=]() {
			vkDestroyPipeline(device, material->pipeline, nullptr);
			});

		material->pipelineLayout = mesh_pipeline_layout;
	}
}

void VulkanEngine::create_env_light_pipeline() {
	PipelineBuilder pipeline_builder(this);
	pipeline_builder.set_msaa(msaa_samples);

	pipeline_builder.set_shader_stages(std::get<HDRiMaterialPtr>(materials["env_light"]));

	std::array env_descriptor_set_layouts = { scene_set_layout, texture_manager->descriptor_set_layout };

	const VkPipelineLayoutCreateInfo env_pipeline_layout_info = vkinit::pipelineLayoutCreateInfo(env_descriptor_set_layouts);

	if (vkCreatePipelineLayout(device, &env_pipeline_layout_info, nullptr, &env_pipeline_layout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	pipeline_builder.depthStencil = vkinit::depthStencilCreateInfo(VK_COMPARE_OP_LESS_OR_EQUAL);

	pipeline_builder.build_pipeline(device, viewport_3d.render_pass, env_pipeline_layout, env_pipeline);

	std::get<HDRiMaterialPtr>(materials["env_light"])->pipelineLayout = env_pipeline_layout;
	std::get<HDRiMaterialPtr>(materials["env_light"])->pipeline = env_pipeline;

	main_deletion_queue.push_function([=]() {
		vkDestroyPipeline(device, env_pipeline, nullptr);
		vkDestroyPipelineLayout(device, env_pipeline_layout, nullptr);
		});
}

void VulkanEngine::create_graphics_pipeline() {
	create_mesh_pipeline();
	create_env_light_pipeline();
}

void VulkanEngine::create_command_pool() {

	const uint32_t queue_family_index = queue_family_indices.graphics_family.value();
	const VkCommandPoolCreateInfo command_pool_info = vkinit::commandPoolCreateInfo(queue_family_index, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	if (vkCreateCommandPool(device, &command_pool_info, nullptr, &command_pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics command pool!");
	}

	main_deletion_queue.push_function([=]() {
		vkDestroyCommandPool(device, command_pool, nullptr);
		});
}

void VulkanEngine::create_viewport_attachments() {
	auto const mode = glfwGetVideoMode(glfwGetPrimaryMonitor()); //get monitor resolution
	screen_width = mode->width;
	screen_height = mode->height;

	for (size_t i = 0; i < swapchain_image_count; i++) {
		viewport_3d.color_textures.emplace_back(engine::Texture::create_2D_render_target(this,
			screen_width,
			screen_height,
			swapchain_image_format,  //color format
			VK_IMAGE_ASPECT_COLOR_BIT,
			SWAPCHAIN_INDEPENDENT_BIT,
			msaa_samples));

		viewport_3d.depth_textures.emplace_back(engine::Texture::create_2D_render_target(this,
			screen_width,
			screen_height,
			find_depth_format(),  //depth format
			VK_IMAGE_ASPECT_DEPTH_BIT,
			SWAPCHAIN_INDEPENDENT_BIT,
			msaa_samples));

		viewport_3d.color_resolve_textures.emplace_back(engine::Texture::create_2D_render_target(this,
			screen_width,
			screen_height,
			swapchain_image_format,  //color resolve format
			VK_IMAGE_ASPECT_COLOR_BIT,
			SWAPCHAIN_INDEPENDENT_BIT));
	}
}

void VulkanEngine::create_viewport_render_pass() {
	const VkAttachmentDescription color_attachment{
		.format = swapchain_image_format,
		.samples = msaa_samples,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	constexpr VkAttachmentReference color_attachment_ref{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	const VkAttachmentDescription depth_attachment{
		.format = find_depth_format(),
		.samples = msaa_samples,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	constexpr VkAttachmentReference depth_attachment_ref{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	const VkAttachmentDescription color_attachment_resolve{
		.format = swapchain_image_format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	constexpr VkAttachmentReference color_attachment_resolve_ref{
		.attachment = 2,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	const VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref,
		.pResolveAttachments = &color_attachment_resolve_ref,
		.pDepthStencilAttachment = &depth_attachment_ref,
	};

	std::array dependencies{
		VkSubpassDependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
							VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
							 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
							 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0,
		},
		VkSubpassDependency{
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT,
			.dependencyFlags = 0,
		},
	};

	std::array attachments = {
		color_attachment,
		depth_attachment,
		color_attachment_resolve,
	};

	const VkRenderPassCreateInfo render_pass_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = static_cast<uint32_t>(dependencies.size()),
		.pDependencies = dependencies.data(),
	};

	if (vkCreateRenderPass(device, &render_pass_info, nullptr, &viewport_3d.render_pass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}

	main_deletion_queue.push_function([=]() {
		vkDestroyRenderPass(device, viewport_3d.render_pass, nullptr);
		});
}


void VulkanEngine::create_viewport_framebuffers() {
	viewport_3d.framebuffers.resize(swapchain_image_count);

	for (size_t i = 0; i < swapchain_image_count; i++) {
		std::array attachments = {
			viewport_3d.color_textures[i]->imageView,
			viewport_3d.depth_textures[i]->imageView,
			viewport_3d.color_resolve_textures[i]->imageView,
		};

		VkFramebufferCreateInfo framebuffer_info = vkinit::framebufferCreateInfo(viewport_3d.render_pass, VkExtent2D{ screen_width , screen_height }, attachments);

		if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &viewport_3d.framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}

		main_deletion_queue.push_function([=]() {
			vkDestroyFramebuffer(device, viewport_3d.framebuffers[i], nullptr);
			});
	}
}

void VulkanEngine::create_viewport_cmd_buffers() {
	viewport_3d.cmd_buffers.resize(swapchain_image_count);
	auto const cmd_alloc_info = vkinit::commandBufferAllocateInfo(command_pool, static_cast<uint32_t>(viewport_3d.cmd_buffers.size()));

	if (vkAllocateCommandBuffers(device, &cmd_alloc_info, viewport_3d.cmd_buffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	main_deletion_queue.push_function([&]() {
		vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(viewport_3d.cmd_buffers.size()), viewport_3d.cmd_buffers.data());
		});
}

VkFormat VulkanEngine::find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const {
	for (const VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat VulkanEngine::find_depth_format() const {
	return find_supported_format(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool VulkanEngine::has_stencil_component(const VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkSampleCountFlagBits VulkanEngine::get_max_usable_sample_count() const {
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physical_device, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

VkImageView VulkanEngine::create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, CreateResourceFlagBits imageViewDescription) {
	const VkImageViewCreateInfo image_view_create_info{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange{
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	VkImageView image_view;
	if (vkCreateImageView(device, &image_view_create_info, nullptr, &image_view) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	if (imageViewDescription == SWAPCHAIN_DEPENDENT_BIT) {
		swap_chain_deletion_queue.push_function([=]() {
			vkDestroyImageView(device, image_view, nullptr);
			});
	}
	else if (imageViewDescription == SWAPCHAIN_INDEPENDENT_BIT) {
		main_deletion_queue.push_function([=]() {
			vkDestroyImageView(device, image_view, nullptr);
			});
	}
	else {
		throw std::runtime_error("illegal imageViewDescription value");
	}

	return image_view;
}

void VulkanEngine::create_uniform_buffers() {
	uniform_buffers.resize(swapchain_image_count);

	for (size_t i = 0; i < swapchain_image_count; i++) {
		uniform_buffers[i] = engine::Buffer::create_buffer(this,
			sizeof(UniformBufferObject),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			SWAPCHAIN_INDEPENDENT_BIT);
	}

	material_preview_ubo = Buffer::create_buffer(this,
		sizeof(UniformBufferObject),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		SWAPCHAIN_INDEPENDENT_BIT);
}

void VulkanEngine::create_texture_manager() {
	texture_manager = std::make_shared<TextureManager>(this, max_bindless_textures);
}

void VulkanEngine::create_descriptor_pool() {
	const uint32_t descriptorSize = swapchain_image_count * 300;
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorSize },
		//{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorSize }
	};

	const VkDescriptorPoolCreateInfo pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = 0,
		.maxSets = descriptorSize,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data(),
	};

	if (vkCreateDescriptorPool(device, &pool_create_info, nullptr, &static_descriptor_pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	main_deletion_queue.push_function([=]() {
		vkDestroyDescriptorPool(device, static_descriptor_pool, nullptr);
		});

	constexpr uint32_t descriptor_size = 2000;
	std::array pool_sizes = {
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptor_size },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, max_bindless_textures },
	};

	const VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = descriptor_size + max_bindless_textures,
		.poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
		.pPoolSizes = pool_sizes.data(),
	};

	if (vkCreateDescriptorPool(device, &pool_info, nullptr, &dynamic_descriptor_pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	main_deletion_queue.push_function([=]() {
		vkDestroyDescriptorPool(device, dynamic_descriptor_pool, nullptr);
		});
}

void VulkanEngine::create_descriptor_sets() {
	const VkDescriptorSetAllocateInfo alloc_scene_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = static_descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &scene_set_layout,
	};

	scene_descriptor_sets.resize(swapchain_image_count);

	VkDescriptorBufferInfo material_preview_uniform_buffer_info{
		.buffer = material_preview_ubo->buffer,
		.offset = 0,
		.range = sizeof(MaterialPreviewUBO),
	};

	for (size_t i = 0; i < swapchain_image_count; i++) {

		if (vkAllocateDescriptorSets(device, &alloc_scene_info, &scene_descriptor_sets[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
		VkDescriptorBufferInfo uniformBufferInfo{
			.buffer = uniform_buffers[i]->buffer,
			.offset = 0,
			.range = sizeof(UniformBufferObject),
		};

		std::array descriptor_writes{
			VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = scene_descriptor_sets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &uniformBufferInfo,
			},
			VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = scene_descriptor_sets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &material_preview_uniform_buffer_info,
			},
		};

		vkUpdateDescriptorSets(device,
			static_cast<uint32_t>(descriptor_writes.size()),
			descriptor_writes.data(),
			0,
			nullptr);
	}

	for (auto const& [index, texture] : texture_manager->textures) {
		VkDescriptorImageInfo descriptor_image_info{
			.sampler = texture->sampler,
			.imageView = texture->imageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		const std::array texture_array_writes{
			VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = texture_manager->descriptor_set,
				.dstBinding = 0,
				.dstArrayElement = index,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &descriptor_image_info,
			},
		};

		vkUpdateDescriptorSets(device,
			texture_array_writes.size(),
			texture_array_writes.data(),
			0,
			nullptr);
	}
}

void VulkanEngine::create_command_buffers() {
	viewport_3d.cmd_buffers.resize(swapchain_image_count);
	const VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::commandBufferAllocateInfo(command_pool, static_cast<uint32_t>(viewport_3d.cmd_buffers.size()));

	if (vkAllocateCommandBuffers(device, &cmd_alloc_info, viewport_3d.cmd_buffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	swap_chain_deletion_queue.push_function([=]() {
		vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(viewport_3d.cmd_buffers.size()), viewport_3d.cmd_buffers.data());
		});
}

void VulkanEngine::record_viewport_cmd_buffer(const int command_buffer_index) {
	auto const i = command_buffer_index;

	constexpr VkCommandBufferBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};

	if (vkBeginCommandBuffer(viewport_3d.cmd_buffers[i], &begin_info) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	const VkExtent2D viewport_extent{ static_cast<uint32_t>(viewport_3d.width), static_cast<uint32_t>(viewport_3d.height) };

	const VkRenderPassBeginInfo render_pass_info = vkinit::renderPassBeginInfo(viewport_3d.render_pass, viewport_extent, viewport_3d.framebuffers[i]);

	vkCmdBeginRenderPass(viewport_3d.cmd_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	const VkViewport viewport{
		.x = 0.0f,
		.y = viewport_3d.height,
		.width = viewport_3d.width,
		.height = -viewport_3d.height,    // flip y axis
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	const VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = viewport_extent,
	};

	vkCmdSetViewport(viewport_3d.cmd_buffers[i], 0, 1, &viewport);
	vkCmdSetScissor(viewport_3d.cmd_buffers[i], 0, 1, &scissor);

	MeshPtr last_mesh = nullptr;
	MaterialPtrV last_material;

	auto const cmd = viewport_3d.cmd_buffers[i];

	for (int k = 0; k < renderables.size(); k++)
	{
		auto mesh = renderables[k].mesh;

		std::array descriptor_sets{
			scene_descriptor_sets[i],
			texture_manager->descriptor_set
		};

		vkCmdBindDescriptorSets(cmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			env_pipeline_layout,
			0,
			descriptor_sets.size(), descriptor_sets.data(),
			0, nullptr);

		//only bind the pipeline if it doesn't match with the already bound one

		//only bind the mesh if its a different one from last bind
		if (mesh != last_mesh) {
			if (mesh->material != last_material) {
				std::visit([&](auto p_material) {
					vkCmdBindPipeline(cmd,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						p_material->pipeline);
					}, mesh->material);
				last_material = mesh->material;
			}
			//bind the mesh vertex buffer with offset 0
			VkBuffer vertexBuffers{ mesh->vertex_buffer->buffer };
			VkDeviceSize offsets{ 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffers, &offsets);

			vkCmdBindIndexBuffer(cmd, mesh->index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);
			last_mesh = mesh;
		}
		//we can now draw
		vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh->_indices.size()), 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(viewport_3d.cmd_buffers[i]);

	if (vkEndCommandBuffer(viewport_3d.cmd_buffers[i]) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void VulkanEngine::load_obj() {
	loaded_meshes.emplace_back(engine::Mesh::load_from_obj(this, "assets/obj_models/rounded_cube.obj"));

	auto material = std::make_shared<PbrTextureMaterial>();
	auto const spv_file_paths = std::array{
		"assets/shaders/pbr_texture.vert.spv"sv,
		"assets/shaders/pbr_texture.frag.spv"sv
	};
	material->shaders = engine::Shader::createFromSpv(this, spv_file_paths);

	for_each_field(pbr_material_texture_set,[&](auto& texture, auto index) {
		texture = Texture::create_device_texture(this,
			TEXTURE_WIDTH,
			TEXTURE_HEIGHT,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		texture->transitionImageLayout(this, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		PbrTexture::Class::FieldAt(material->paras, index, [&](auto& field, auto& value) {
			value = texture_manager->add_texture(texture);
		});
	});

	PipelineBuilder pipeline_builder(this);
	pipeline_builder.set_msaa(msaa_samples);
	pipeline_builder.set_shader_stages(material);

	std::array env_descriptor_set_layouts = { scene_set_layout, texture_manager->descriptor_set_layout };

	const VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkinit::pipelineLayoutCreateInfo(env_descriptor_set_layouts);

	if (vkCreatePipelineLayout(device, &mesh_pipeline_layout_info, nullptr, &material_preview_pipeline_layout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	swap_chain_deletion_queue.push_function([=]() {
		vkDestroyPipelineLayout(device, material_preview_pipeline_layout, nullptr);
		});

	pipeline_builder.depthStencil = vkinit::depthStencilCreateInfo(VK_COMPARE_OP_LESS);

	pipeline_builder.build_pipeline(device, viewport_3d.render_pass, material_preview_pipeline_layout, material->pipeline);

	main_deletion_queue.push_function([=]() {
		vkDestroyPipeline(device, material->pipeline, nullptr);
		});

	material->pipelineLayout = material_preview_pipeline_layout;

	material_preview_ubo->copy_from_host(&init_material_preview_ubo);
	auto const& mesh = loaded_meshes.back();
	mesh->material = std::move(material);
	renderables.emplace_back(mesh, glm::mat4{ 1.0f });
}

void VulkanEngine::create_sync_objects() {
	frame_data.resize(MAX_FRAMES_IN_FLIGHT);
	images_in_flight.resize(swapchain_image_count, VK_NULL_HANDLE);

	const VkSemaphoreCreateInfo semaphore_info = vkinit::semaphoreCreateInfo();
	const VkFenceCreateInfo fence_info = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device, &semaphore_info, nullptr, &frame_data[i].image_available_semaphore) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphore_info, nullptr, &frame_data[i].render_finished_semaphore) != VK_SUCCESS ||
			vkCreateFence(device, &fence_info, nullptr, &frame_data[i].in_flight_fence) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}

		main_deletion_queue.push_function([=]() {
			vkDestroyFence(device, frame_data[i].in_flight_fence, nullptr);
			vkDestroySemaphore(device, frame_data[i].render_finished_semaphore, nullptr);
			vkDestroySemaphore(device, frame_data[i].image_available_semaphore, nullptr);
			});
	}

	if (vkCreateFence(device, &fence_info, nullptr, &immediate_submit_fence) != VK_SUCCESS) {
		throw std::runtime_error("failed to create synchronization objects for a frame!");
	}
	main_deletion_queue.push_function([=]() {
		vkDestroyFence(device, immediate_submit_fence, nullptr);
		});
}

void VulkanEngine::init_imgui() {
	gui = std::make_shared<engine::GUI>();
	gui->init(this);
	
	node_editor = std::make_shared<engine::NodeEditor>(this);
}

void VulkanEngine::update_uniform_buffer(uint32_t currentImage) {
	/*static auto start_time = std::chrono::high_resolution_clock::now();
	auto const current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();*/

	set_camera();
	UniformBufferObject ubo{
		.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		.view = camera.view_matrix(),
		//.view = glm::mat4(glm::mat3(camera.viewMatrix())),
		.proj = camera.proj_matrix(),
		.pos = camera.get_position(),
	};

	uniform_buffers[currentImage]->copy_from_host(&ubo);
}

void VulkanEngine::imgui_render(uint32_t image_index) {
	const ImGuiIO& io = ImGui::GetIO();

	constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	// DockSpace
	ImGui::Begin("DockSpace", nullptr, window_flags);
	ImGuiID dock_space_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dock_space_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	static bool first_time = true;
	if (first_time) {
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByTypeDir, "", ImVec4(0.5f, 1.0f, 0.9f, 0.9f), ICON_FA_FOLDER);
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".txg", ImVec4(1.0f, 1.0f, 0.0f, 0.9f), ICON_FA_CODE_BRANCH);
	}

	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem(" " ICON_FA_FOLDER_PLUS " New")) {
				node_editor->clear();
			}
			if (ImGui::MenuItem(" " ICON_FA_FOLDER_OPEN " Open")) {
				ImGuiFileDialog::Instance()->OpenDialog("OpenFileDlgKey", "Open File", ".txg", ".", 1, nullptr);
			}
			if (ImGui::MenuItem(" " ICON_FA_SAVE " Save")) {
				ImGuiFileDialog::Instance()->OpenDialog("SaveFileDlgKey", "Save File", ".txg", ".", 1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem(" Document")) {

			}
			ImGui::EndMenu();
		}
		ImGui::SameLine(ImGui::GetWindowWidth() - 110);
		ImGui::Text("FPS: %.f (%.fms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
		ImGui::EndMenuBar();
	}

	constexpr auto file_dialog_min_x = 720;
	constexpr auto file_dialog_min_y = 495;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
	ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
	if (ImGuiFileDialog::Instance()->Display("OpenFileDlgKey", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, ImVec2{ file_dialog_min_x, file_dialog_min_y })) {

		// action if OK
		if (ImGuiFileDialog::Instance()->IsOk()) {
			node_editor->clear();
			node_editor->deserialize(ImGuiFileDialog::Instance()->GetFilePathName());
			// action
		}
		// close
		ImGuiFileDialog::Instance()->Close();
	}

	if (ImGuiFileDialog::Instance()->Display("SaveFileDlgKey", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, ImVec2{ file_dialog_min_x, file_dialog_min_y })) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			node_editor->serialize(ImGuiFileDialog::Instance()->GetFilePathName());
		}
		ImGuiFileDialog::Instance()->Close();
	}
	ImGui::PopStyleVar(5);
	ImGui::PopStyleColor();

	if (first_time) {
		first_time = false;
		ImGuiID dock_id_right_p;
		const ImGuiID dock_id_right = ImGui::GetID("3D Viewport");

		ImGui::DockBuilderRemoveNode(dock_space_id); // clear any previous layout
		ImGui::DockBuilderAddNode(dock_space_id, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dock_space_id, viewport->Size);
		//auto dockspace_id_Node = ImGui::DockBuilderGetNode(dockspace_id);

		// split the dockspace into 2 nodes -- DockBuilderSplitNode takes in the following args in the following order
		//   window ID to split, direction, fraction (between 0 and 1), the final two setting let's us choose which id we want (which ever one we DON'T set as NULL, will be returned by the function)
		//                                                              out_id_at_dir is the id of the node in the direction we specified earlier, out_id_at_opposite_dir is in the opposite direction

		const ImGuiID dock_id_down = ImGui::DockBuilderSplitNode(dock_space_id, ImGuiDir_Down, 0.5f, nullptr, &dock_space_id);
		auto const up_node = ImGui::DockBuilderGetNode(dock_space_id);


		const ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_space_id, ImGuiDir_Left, 0.5f, nullptr, &dock_id_right_p);
		auto const dock_id_right_p_node = ImGui::DockBuilderGetNode(dock_id_right_p);

		auto const size = dock_id_right_p_node->Size;
		auto const pos = dock_id_right_p_node->Pos;
		ImGui::DockBuilderAddNode(dock_id_right, ImGuiDockNodeFlags_PassthruCentralNode);
		ImGui::DockBuilderSetNodeSize(dock_id_right, size);
		ImGui::DockBuilderSetNodePos(dock_id_right, pos);
		auto const right_node = ImGui::DockBuilderGetNode(dock_id_right);
		up_node->ChildNodes[0] = ImGui::DockBuilderGetNode(dock_id_left);
		up_node->ChildNodes[1] = right_node;
		right_node->ParentNode = up_node;

		// we now dock our windows into the docking node we made above
		ImGui::DockBuilderDockWindow("Node Editor", dock_id_down);
		ImGui::DockBuilderDockWindow("Texture Viewer", dock_id_left);
		ImGui::DockBuilderDockWindow("3D Viewport", dock_id_right);
		ImGui::DockBuilderFinish(dock_space_id);
	}
	ImGui::End();

	ImGui::Begin("Node Editor", nullptr, ImGuiWindowFlags_MenuBar);
	//ImGui::Begin("Node Editor");
	{
		node_editor->draw();
	}
	ImGui::End();

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.227115f, 0.227115f, 0.227115f, 1.0f));
	ImGui::Begin("Texture Viewer");
	ImGui::PopStyleColor();
	{
		if (auto const handle = static_cast<ImTextureID>(node_editor->get_gui_display_texture_handle())) {
			const ImVec2 window_size = ImGui::GetWindowSize();  //include menu height
			const ImVec2 viewer_size = ImGui::GetContentRegionAvail();
			constexpr static float scale_factor = 0.975;
			const float image_width = std::min(viewer_size.x, viewer_size.y) * scale_factor;
			const ImVec2 image_size{ image_width, image_width };
			ImGui::SetCursorPos((viewer_size - image_size) * 0.5f + ImVec2{ 0, window_size.y - viewer_size.y });
			ImGui::Image(handle, image_size, ImVec2{ 0, 0 }, ImVec2{ 1, 1 });
		}
		/*ImVec2 viewportPanelPos = ImGui::GetWindowContentRegionMin();
		ImGui::Text("Pos = (%f, %f)", viewportPanelPos.x, viewportPanelPos.y);
		ImGui::Text("Size = (%f, %f)", viewportPanelSize.x, viewportPanelSize.y);*/
	}
	ImGui::End();

	ImGui::SetNextWindowBgAlpha(0.0f);
	//ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Set window background to red
	//ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Set window background to red

	ImGui::Begin("3D Viewport", nullptr, ImGuiDockNodeFlags_PassthruCentralNode & ~ImGuiWindowFlags_NoInputs & ~ImGuiWindowFlags_NoBringToFrontOnFocus);

	const ImVec2 viewport_panel_size = ImGui::GetContentRegionAvail();

	//::PopStyleColor(2);
	//ImGui::Text("io.WantCaptureMouse = %d", ImGui::IsItemHovered());

	viewport_3d.width = viewport_panel_size.x;
	viewport_3d.height = viewport_panel_size.y;
	update_uniform_buffer(image_index);
	const ImVec2 uv{ viewport_panel_size.x / screen_width , viewport_panel_size.y / screen_height };

	ImGui::Image(viewport_3d.gui_textures[image_index], viewport_panel_size, ImVec2{ 0, 0 }, uv);
	mouse_hover_viewport = ImGui::IsItemHovered() ? true : false;
	ImGui::End();

	ImGui::PopStyleVar(3);
}

void VulkanEngine::draw_frame() {
	//drawFrame will first acquire the index of the available swapchain image, then render into this image, and finally request to prensent this image

	vkWaitForFences(device, 1, &frame_data[current_frame].in_flight_fence, VK_TRUE, VULKAN_WAIT_TIMEOUT); // begin draw i+2 frame if we've complete rendering at frame i

	uint32_t image_index;
	VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, frame_data[current_frame].image_available_semaphore, VK_NULL_HANDLE, &image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_swap_chain();
		return;
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	// IMGUI RENDERING
	gui->begin_render();

	imgui_render(image_index);

	gui->end_render(image_index);

	record_viewport_cmd_buffer(image_index);

	std::array submit_command_buffers = { viewport_3d.cmd_buffers[image_index], gui->command_buffers[image_index] };

	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	const VkSubmitInfo submit_info{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame_data[current_frame].image_available_semaphore,
		.pWaitDstStageMask = wait_stages,

		.commandBufferCount = static_cast<uint32_t>(submit_command_buffers.size()),
		.pCommandBuffers = submit_command_buffers.data(),

		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &frame_data[current_frame].render_finished_semaphore,
	};

	if (images_in_flight[image_index] != VK_NULL_HANDLE) {
		vkWaitForFences(device, 1, &images_in_flight[image_index], VK_TRUE, VULKAN_WAIT_TIMEOUT);  //start to render into this image if we've complete the last rendering of this image
	}
	images_in_flight[image_index] = frame_data[current_frame].in_flight_fence;  //update the fence of this image

	vkResetFences(device, 1, &frame_data[current_frame].in_flight_fence);

	if (vkQueueSubmit(graphics_queue, 1, &submit_info, frame_data[current_frame].in_flight_fence) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	const VkPresentInfoKHR present_info{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame_data[current_frame].render_finished_semaphore,
		.swapchainCount = 1,
		.pSwapchains = &swapchain,
		.pImageIndices = &image_index,
	};

	result = vkQueuePresentKHR(present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
		framebuffer_resized = false;
		recreate_swap_chain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

VkSurfaceFormatKHR VulkanEngine::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VulkanEngine::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) {
	for (const auto& available_present_mode : available_present_modes) {
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return available_present_mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanEngine::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	}
	else {
		glfwGetFramebufferSize(window, &window_width, &window_height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(window_width),
			static_cast<uint32_t>(window_height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

SwapChainSupportDetails VulkanEngine::query_swap_chain_support(VkPhysicalDevice device) const {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.present_modes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.present_modes.data());
	}

	return details;
}

bool VulkanEngine::is_device_suitable(VkPhysicalDevice device) {

	const bool extensions_supported = check_device_extension_support(device);

	bool swap_chain_adequate = false;
	if (extensions_supported) {
		auto const [_, formats, present_modes] = query_swap_chain_support(device);
		swap_chain_adequate = !formats.empty() && !present_modes.empty();
	}

	VkPhysicalDeviceFeatures supported_features;
	vkGetPhysicalDeviceFeatures(device, &supported_features);

	return queue_family_indices.is_complete() && extensions_supported && swap_chain_adequate && supported_features.samplerAnisotropy;
}

bool VulkanEngine::check_device_extension_support(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

QueueFamilyIndices VulkanEngine::find_queue_families(VkPhysicalDevice device) const {
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	int i = 0;
	for (const auto& queueFamily : queue_families) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family = i;
		}

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

		if (present_support) {
			indices.present_family = i;
		}

		if (indices.is_complete()) {
			break;
		}

		i++;
	}

	return indices;
}

std::vector<const char*> VulkanEngine::get_required_extensions() {
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if constexpr (enableValidationLayers) {
		extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	extensions.insert(extensions.end(), instance_extensions.begin(), instance_extensions.end());

	return extensions;
}

bool VulkanEngine::check_validation_layer_support() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanEngine::debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	}

	return VK_FALSE;
}

void VulkanEngine::mouse_cursor_callback(GLFWwindow* window, const double x_pos, const double y_pos) {

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
		return;
	}
	auto const mouse_current_pos = glm::vec2(x_pos, y_pos);
	mouse_delta_pos = mouse_current_pos - mouse_previous_pos;

	if (mouse_hover_viewport) {
		camera.rotate(-mouse_delta_pos.x, -mouse_delta_pos.y, 0.005f);
	}

	mouse_previous_pos = mouse_current_pos;
}

void VulkanEngine::mouse_button_callback(GLFWwindow* window, const int button, const int action, const int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double x_pos, y_pos;
		glfwGetCursorPos(window, &x_pos, &y_pos);
		mouse_previous_pos.x = x_pos;
		mouse_previous_pos.y = y_pos;
	}
}

void VulkanEngine::mouse_scroll_callback(GLFWwindow* window, const double x_offset, const double y_offset) {
	if (mouse_hover_viewport) {
		camera.zoom(static_cast<float>(y_offset), 0.3f);
	}
}

void VulkanEngine::set_camera() const {
	camera.set_aspect_ratio(viewport_3d.width / viewport_3d.height);
}

VulkanEngine::~VulkanEngine() {}