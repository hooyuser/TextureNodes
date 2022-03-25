#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_shader.h"
#include "vk_mesh.h"
#include "vk_pipeline.h"
#include "vk_image.h"
#include "vk_buffer.h"
#include "vk_camera.h"
#include "vk_gui.h"
#include "gui/gui_node_editor.h"
#include "gui/ImGuiFileDialog.h"
#include "util/class_field_type_list.h"

#include <cstring>
#include <array>
#include <set>
#include <filesystem>
#include <chrono>
#include <algorithm>

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
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
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

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	mouse_previous_pos.x = xpos;
	mouse_previous_pos.y = ypos;
	glfwSetCursorPosCallback(window, mouseCursorCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetScrollCallback(window, mouseScrollCallback);
}

void VulkanEngine::framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
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
	parse_material_info();
	create_descriptor_set_layouts();

	create_viewport_attachments();
	create_viewport_render_pass();
	create_viewport_framebuffers();
	create_viewport_cmd_buffers();

	create_uniform_buffers();
	create_descriptor_pool();
	create_descriptor_sets();
	create_graphics_pipeline();

	load_obj();

	set_camera();

	init_imgui();

	is_initialized = true;
}



void VulkanEngine::main_loop() {
	//bool running = true;
	double lastTime = 0.0;

	while (!glfwWindowShouldClose(window)) {
		double time = glfwGetTime();
		double deltaTime = time - lastTime;
		if (deltaTime >= MAX_PERIOD) {
			lastTime = time;
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

	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
	while (windowWidth == 0 || windowHeight == 0) {
		glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	swap_chain_deletion_queue.flush();

	create_swap_chain();
	create_swap_chain_image_views();

	set_camera();

	imagesInFlight.resize(swapchain_image_count, VK_NULL_HANDLE);

	gui->create_framebuffers();
	ImGui_ImplVulkan_SetMinImageCount(swapchain_image_count);
}

void VulkanEngine::create_instance() {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3,
	};

	auto extensions = getRequiredExtensions();
	VkInstanceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
	};

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if constexpr (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

void VulkanEngine::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

void VulkanEngine::setup_debug_messenger() {
	if constexpr (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}

	main_deletion_queue.push_function([=]() {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		});
}

void VulkanEngine::create_surface() {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void VulkanEngine::pick_physical_device() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		queueFamilyIndices = findQueueFamilies(device);
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			if (msaaSamples != VK_SAMPLE_COUNT_1_BIT) {
				msaaSamples = getMaxUsableSampleCount();
			}
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

void VulkanEngine::create_logical_device() {
	// queueFamilyIndices = findQueueFamilies(physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		queueCreateInfos.emplace_back(
			VkDeviceQueueCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = queueFamily,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority,
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
		.samplerAnisotropy = VK_TRUE,
	};

	VkDeviceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &vk12_features,
		.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures = &device_features,
	};

	if constexpr (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
}

void VulkanEngine::create_swap_chain() {
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
	VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	swapChainExtent = chooseSwapExtent(swapChainSupport.capabilities);

	swapchain_image_count = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && swapchain_image_count > swapChainSupport.capabilities.maxImageCount) {
		swapchain_image_count = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR create_info{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = swapchain_image_count,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = swapChainExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	};

	const std::array queue_family_index_array{
		queueFamilyIndices.graphicsFamily.value(),
		queueFamilyIndices.presentFamily.value()
	};

	if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = queue_family_index_array.size();
		create_info.pQueueFamilyIndices = queue_family_index_array.data();
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	create_info.preTransform = swapChainSupport.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = presentMode;
	create_info.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, swapChain, &swapchain_image_count, nullptr);
	swapChainImages.resize(swapchain_image_count);
	vkGetSwapchainImagesKHR(device, swapChain, &swapchain_image_count, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;

	swap_chain_deletion_queue.push_function([=]() {
		vkDestroySwapchainKHR(device, swapChain, nullptr);
		});
}

void VulkanEngine::create_swap_chain_image_views() {
	swapChainImageViews.resize(swapChainImages.size());

	for (uint32_t i = 0; i < swapChainImages.size(); i++) {
		swapChainImageViews[i] = create_image_view(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, SWAPCHAIN_DEPENDENT_BIT);
	}
}

//void VulkanEngine::create_render_pass() {
//	VkAttachmentDescription colorAttachment{
//		.format = swapChainImageFormat,
//		.samples = msaaSamples,
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
//	depthAttachment.samples = msaaSamples;
//	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//	VkAttachmentDescription colorAttachmentResolve{};
//	colorAttachmentResolve.format = swapChainImageFormat;
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
//	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
//		throw std::runtime_error("failed to create render pass!");
//	}
//
//	swap_chain_deletion_queue.push_function([=]() {
//		vkDestroyRenderPass(device, renderPass, nullptr);
//		});
//}

void VulkanEngine::load_gltf() {
	const std::string filename = "assets/gltf_models/dragon.gltf";
	tinygltf::Model glTFModel;
	tinygltf::TinyGLTF gltfContext;
	std::string error, warning;

	const bool file_loaded = gltfContext.LoadASCIIFromFile(&glTFModel, &error, &warning, filename);

	if (file_loaded) {
		for (size_t i = 0; i < glTFModel.images.size(); i++) {
			tinygltf::Image& glTFImage = glTFModel.images[i];
			loaded_2d_textures.emplace_back(engine::Texture::load2DTextureFromHost(this, glTFImage.image.data(), glTFImage.width, glTFImage.height, glTFImage.component));
		}

		std::array<std::string, 2> shaderFilePaths{
			"assets/shaders/pbr.vert.spv",
			"assets/shaders/pbr.frag.spv"
		};

		auto pbrShader = engine::Shader::createFromSpv(this, std::move(shaderFilePaths));

		for (auto& glTFMaterial : glTFModel.materials) {
			auto material = std::make_shared<PbrMaterial>();
			material->pShaders = pbrShader;
			auto& paras = material->paras;

			paras.baseColorRed = glTFMaterial.pbrMetallicRoughness.baseColorFactor[0];
			paras.baseColorGreen = glTFMaterial.pbrMetallicRoughness.baseColorFactor[1];
			paras.baseColorBlue = glTFMaterial.pbrMetallicRoughness.baseColorFactor[2];
			paras.baseColorTextureID = glTFMaterial.pbrMetallicRoughness.baseColorTexture.index;

			paras.metalnessFactor = glTFMaterial.pbrMetallicRoughness.metallicFactor;
			paras.roughnessFactor = glTFMaterial.pbrMetallicRoughness.roughnessFactor;
			paras.metallicRoughnessTextureId = glTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;

			loaded_materials.emplace_back(material);
			materials.try_emplace(glTFMaterial.name, std::move(material));
		}

		for (auto& glTFMesh : glTFModel.meshes) {
			loaded_meshes.emplace_back(engine::Mesh::load_from_gltf(this, glTFModel, glTFMesh));
		}

		// Only support one scene
		for (auto const node_index : glTFModel.scenes[0].nodes) {
			const tinygltf::Node& node = glTFModel.nodes[node_index];

			if (node.mesh > -1) {
				RenderObject render_object;
				render_object.mesh = loaded_meshes[node.mesh];
				if (node.translation.size() == 3) {
					render_object.transformMatrix = glm::translate(render_object.transformMatrix, glm::vec3(glm::make_vec3(node.translation.data())));
				}
				if (node.rotation.size() == 4) {
					glm::quat q = glm::make_quat(node.rotation.data());
					render_object.transformMatrix *= glm::mat4(q);
				}
				if (node.scale.size() == 3) {
					render_object.transformMatrix = glm::scale(render_object.transformMatrix, glm::vec3(glm::make_vec3(node.scale.data())));
				}
				if (node.matrix.size() == 16) {
					render_object.transformMatrix = glm::make_mat4x4(node.matrix.data());
				};
				renderables.emplace_back(std::move(render_object));
			}
		}
	}
}



void VulkanEngine::parse_material_info() {

	load_gltf();

	auto envMaterialInfoJson = R"(
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


	if (envMaterialInfoJson["type"].get<std::string>() == "cubemap") {
		materials.emplace("env_light", std::make_shared<HDRiMaterial>());
		auto envMat = std::get<HDRiMaterialPtr>(materials["env_light"]);
		std::array<std::string, 2> spvFilePaths = {
			"assets/shaders/env_cubemap.vert.spv",
			"assets/shaders/env_cubemap.frag.spv"
		};
		envMat->pShaders = engine::Shader::createFromSpv(this, std::move(spvFilePaths));
		envMat->textureArrayIndex.emplace("cubemap", loaded_cubemap_textures.size());
		envMat->paras.baseColorTextureID = loaded_cubemap_textures.size();
		loaded_cubemap_textures.emplace_back(engine::Texture::loadCubemapTexture(this, envMaterialInfoJson["filePaths"].get<std::vector<std::string>>()));
		for (auto const& mat : loaded_materials) {
			mat->paras.irradianceMapId = loaded_cubemap_textures.size();
		}
		loaded_cubemap_textures.emplace_back(engine::Texture::loadCubemapTexture(this, envMaterialInfoJson["irradianceMapPaths"].get<std::vector<std::string>>()));

		for (auto const& mat : loaded_materials) {
			mat->paras.prefilteredMapId = loaded_cubemap_textures.size();
		}
		loaded_cubemap_textures.emplace_back(engine::Texture::loadPrefilteredMapTexture(this, envMaterialInfoJson["prefilteredMapPaths"].get<std::vector<std::vector<std::string>>>()));

		for (auto const& mat : loaded_materials) {
			mat->paras.brdfLUTId = loaded_2d_textures.size();
		}
		loaded_2d_textures.emplace_back(engine::Texture::load2DTexture(this, envMaterialInfoJson["BRDF_2D_LUT"].get<std::string>(), false));
	}

	for (auto const& mat : loaded_materials) {
		mat->paras.texture2DArraySize = loaded_2d_textures.size();
	}

	for (auto& mat : materials | std::views::values) {
		std::visit([&](auto& obj) {
			if constexpr (!std::same_as<MaterialPtr, std::decay_t<decltype(obj)>>) {
				obj->paras.textureCubemapArraySize = loaded_cubemap_textures.size();
			}
			}, mat);
	}

	RenderObject skyBoxObject;
	skyBoxObject.mesh = Mesh::load_from_obj(this, "assets/obj_models/skybox.obj");
	skyBoxObject.mesh->pMaterial = std::get<HDRiMaterialPtr>(materials["env_light"]);
	skyBoxObject.transformMatrix = glm::mat4{ 1.0f };
	renderables.emplace_back(std::move(skyBoxObject));
}

void VulkanEngine::create_descriptor_set_layouts() {

	auto const cam_ubo_layout_binding = vkinit::descriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0);

	auto const texture_2d_array_layout_binding = vkinit::descriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		1,
		loaded_2d_textures.size());

	auto const cubemap_array_layout_binding = vkinit::descriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		2,
		loaded_cubemap_textures.size());

	auto const material_preview_layout_binding = vkinit::descriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		3);

	std::array scene_bindings{
		cam_ubo_layout_binding,
		texture_2d_array_layout_binding,
		cubemap_array_layout_binding,
		material_preview_layout_binding
	};

	create_descriptor_set_layout(scene_bindings, scene_set_layout);
}

void VulkanEngine::create_descriptor_set_layout(std::span<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings, VkDescriptorSetLayout& descriptorSetLayout) {
	const VkDescriptorSetLayoutCreateInfo layout_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size()),
		.pBindings = descriptorSetLayoutBindings.data(),
	};

	if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	main_deletion_queue.push_function([=]() {
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		});
}

void VulkanEngine::create_mesh_pipeline() {

	PipelineBuilder pipeline_builder(this);

	std::array mesh_descriptor_set_layouts = { scene_set_layout };

	const VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkinit::pipelineLayoutCreateInfo(mesh_descriptor_set_layouts);

	if (vkCreatePipelineLayout(device, &mesh_pipeline_layout_info, nullptr, &meshPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	swap_chain_deletion_queue.push_function([=]() {
		vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
		});

	for (auto const& material : loaded_materials) {
		pipeline_builder.setShaderStages(material);

		pipeline_builder.depthStencil = vkinit::depthStencilCreateInfo(VK_COMPARE_OP_LESS);

		pipeline_builder.buildPipeline(device, viewport_3d.render_pass, meshPipelineLayout, material->pipeline);

		main_deletion_queue.push_function([=]() {
			vkDestroyPipeline(device, material->pipeline, nullptr);
			});

		material->pipelineLayout = meshPipelineLayout;
	}
}

void VulkanEngine::create_env_light_pipeline() {
	PipelineBuilder pipeline_builder(this);

	pipeline_builder.setShaderStages(std::get<HDRiMaterialPtr>(materials["env_light"]));

	std::array env_descriptor_set_layouts = { scene_set_layout };

	const VkPipelineLayoutCreateInfo env_pipeline_layout_info = vkinit::pipelineLayoutCreateInfo(env_descriptor_set_layouts);

	if (vkCreatePipelineLayout(device, &env_pipeline_layout_info, nullptr, &envPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	pipeline_builder.depthStencil = vkinit::depthStencilCreateInfo(VK_COMPARE_OP_LESS_OR_EQUAL);

	pipeline_builder.buildPipeline(device, viewport_3d.render_pass, envPipelineLayout, envPipeline);

	std::get<HDRiMaterialPtr>(materials["env_light"])->pipelineLayout = envPipelineLayout;
	std::get<HDRiMaterialPtr>(materials["env_light"])->pipeline = envPipeline;

	main_deletion_queue.push_function([=]() {
		vkDestroyPipeline(device, envPipeline, nullptr);
		vkDestroyPipelineLayout(device, envPipelineLayout, nullptr);
		});
}

void VulkanEngine::create_graphics_pipeline() {
	create_mesh_pipeline();
	create_env_light_pipeline();
}

void VulkanEngine::create_command_pool() {

	const uint32_t queue_family_index = queueFamilyIndices.graphicsFamily.value();
	const VkCommandPoolCreateInfo command_pool_info = vkinit::commandPoolCreateInfo(queue_family_index, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	if (vkCreateCommandPool(device, &command_pool_info, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics command pool!");
	}

	main_deletion_queue.push_function([=]() {
		vkDestroyCommandPool(device, commandPool, nullptr);
		});
}

void VulkanEngine::create_window_attachments() {
	pColorImage = engine::Image::createImage(this,
		swapChainExtent.width,
		swapChainExtent.height,
		1,
		msaaSamples,
		swapChainImageFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		SWAPCHAIN_DEPENDENT_BIT);

	pDepthImage = engine::Image::createImage(this,
		swapChainExtent.width,
		swapChainExtent.height,
		1,
		msaaSamples,
		find_depth_format(),
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		SWAPCHAIN_DEPENDENT_BIT);
}

void VulkanEngine::create_viewport_attachments() {
	auto const mode = glfwGetVideoMode(glfwGetPrimaryMonitor()); //get monitor resolution
	screen_width = mode->width;
	screen_height = mode->height;

	for (size_t i = 0; i < swapchain_image_count; i++) {
		viewport_3d.color_textures.emplace_back(engine::Texture::create_2D_render_target(this,
			screen_width,
			screen_height,
			swapChainImageFormat,  //color format
			VK_IMAGE_ASPECT_COLOR_BIT,
			SWAPCHAIN_INDEPENDENT_BIT));

		viewport_3d.depth_textures.emplace_back(engine::Texture::create_2D_render_target(this,
			screen_width,
			screen_height,
			find_depth_format(),  //depth format
			VK_IMAGE_ASPECT_DEPTH_BIT,
			SWAPCHAIN_INDEPENDENT_BIT));
	}
}

void VulkanEngine::create_viewport_render_pass() {
	const VkAttachmentDescription color_attachment{
		.format = swapChainImageFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	constexpr VkAttachmentReference color_attachment_ref{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	const VkAttachmentDescription depth_attachment{
		.format = find_depth_format(),
		.samples = VK_SAMPLE_COUNT_1_BIT,
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

	const VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref,
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

	std::array attachments = { color_attachment, depth_attachment };

	VkRenderPassCreateInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = static_cast<uint32_t>(dependencies.size()),
		.pDependencies = dependencies.data(),
	};

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &viewport_3d.render_pass) != VK_SUCCESS) {
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
		};

		VkFramebufferCreateInfo framebufferInfo = vkinit::framebufferCreateInfo(viewport_3d.render_pass, VkExtent2D{ screen_width , screen_height }, attachments);

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &viewport_3d.framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}

		main_deletion_queue.push_function([=]() {
			vkDestroyFramebuffer(device, viewport_3d.framebuffers[i], nullptr);
			});
	}
}

void VulkanEngine::create_viewport_cmd_buffers() {
	viewport_3d.cmd_buffers.resize(swapchain_image_count);
	auto const cmd_alloc_info = vkinit::commandBufferAllocateInfo(commandPool, static_cast<uint32_t>(viewport_3d.cmd_buffers.size()));

	if (vkAllocateCommandBuffers(device, &cmd_alloc_info, viewport_3d.cmd_buffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	main_deletion_queue.push_function([&]() {
		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(viewport_3d.cmd_buffers.size()), viewport_3d.cmd_buffers.data());
		});
}

VkFormat VulkanEngine::find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const {
	for (const VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat VulkanEngine::find_depth_format() {
	return find_supported_format(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool VulkanEngine::hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkSampleCountFlagBits VulkanEngine::getMaxUsableSampleCount() {
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

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
	VkImageViewCreateInfo viewInfo{
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

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	if (imageViewDescription == SWAPCHAIN_DEPENDENT_BIT) {
		swap_chain_deletion_queue.push_function([=]() {
			vkDestroyImageView(device, imageView, nullptr);
			});
	}
	else if (imageViewDescription == SWAPCHAIN_INDEPENDENT_BIT) {
		main_deletion_queue.push_function([=]() {
			vkDestroyImageView(device, imageView, nullptr);
			});
	}
	else {
		throw std::runtime_error("illegal imageViewDescription value");
	}

	return imageView;
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
}

void VulkanEngine::create_descriptor_pool() {
	const uint32_t descriptorSize = swapchain_image_count * 200;
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorSize },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorSize }
	};

	VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = 0,
		.maxSets = descriptorSize,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data(),
	};

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	main_deletion_queue.push_function([=]() {
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		});

	constexpr uint32_t descriptor_size = 2000;
	std::array pool_sizes = {
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptor_size },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, max_bindless_node_2d_textures + max_bindless_node_1d_textures },
	};

	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = descriptor_size + max_bindless_node_2d_textures + max_bindless_node_1d_textures,
		.poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
		.pPoolSizes = pool_sizes.data(),
	};

	if (vkCreateDescriptorPool(device, &pool_info, nullptr, &node_descriptor_pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	main_deletion_queue.push_function([=]() {
		vkDestroyDescriptorPool(device, node_descriptor_pool, nullptr);
		});
}

void VulkanEngine::create_descriptor_sets() {
	const VkDescriptorSetAllocateInfo alloc_scene_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &scene_set_layout,
	};

	scene_descriptor_sets.resize(swapchain_image_count);

	for (size_t i = 0; i < swapchain_image_count; i++) {

		if (vkAllocateDescriptorSets(device, &alloc_scene_info, &scene_descriptor_sets[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
		VkDescriptorBufferInfo uniformBufferInfo{
			.buffer = uniform_buffers[i]->buffer,
			.offset = 0,
			.range = sizeof(UniformBufferObject),
		};

		std::vector<VkDescriptorImageInfo> textureDescriptors(loaded_2d_textures.size());
		for (size_t j = 0; j < loaded_2d_textures.size(); j++) {
			textureDescriptors[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			textureDescriptors[j].sampler = loaded_2d_textures[j]->sampler;;
			textureDescriptors[j].imageView = loaded_2d_textures[j]->imageView;
		}

		std::vector<VkDescriptorImageInfo> cubemapDescriptors(loaded_cubemap_textures.size());
		for (size_t j = 0; j < loaded_cubemap_textures.size(); j++) {
			cubemapDescriptors[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			cubemapDescriptors[j].sampler = loaded_cubemap_textures[j]->sampler;;
			cubemapDescriptors[j].imageView = loaded_cubemap_textures[j]->imageView;
		}

		std::array descriptorWrites{
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
				.descriptorCount = static_cast<uint32_t>(textureDescriptors.size()),
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = textureDescriptors.data(),
			},
			VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = scene_descriptor_sets[i],
				.dstBinding = 2,
				.dstArrayElement = 0,
				.descriptorCount = static_cast<uint32_t>(cubemapDescriptors.size()),
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = cubemapDescriptors.data(),
			},
		};

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void VulkanEngine::create_command_buffers() {
	viewport_3d.cmd_buffers.resize(swapchain_image_count);
	const VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::commandBufferAllocateInfo(commandPool, (uint32_t)viewport_3d.cmd_buffers.size());

	if (vkAllocateCommandBuffers(device, &cmd_alloc_info, viewport_3d.cmd_buffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	swap_chain_deletion_queue.push_function([=]() {
		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(viewport_3d.cmd_buffers.size()), viewport_3d.cmd_buffers.data());
		});
}

void VulkanEngine::record_viewport_cmd_buffer(const int command_buffer_index) {
	auto const i = command_buffer_index;

	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};

	if (vkBeginCommandBuffer(viewport_3d.cmd_buffers[i], &beginInfo) != VK_SUCCESS) {
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

	auto cmd = viewport_3d.cmd_buffers[i];

	for (int k = 0; k < renderables.size(); k++)
	{
		auto mesh = renderables[k].mesh;

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, envPipelineLayout, 0, 1, &scene_descriptor_sets[i], 0, nullptr);

		//only bind the pipeline if it doesn't match with the already bound one

		//only bind the mesh if its a different one from last bind
		if (mesh != last_mesh) {
			if (mesh->pMaterial != last_material) {
				std::visit([&](auto p_material) {
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p_material->pipeline);
					}, mesh->pMaterial);
				last_material = mesh->pMaterial;
			}
			//bind the mesh vertex buffer with offset 0
			VkBuffer vertexBuffers{ mesh->pVertexBuffer->buffer };
			VkDeviceSize offsets{ 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffers, &offsets);

			vkCmdBindIndexBuffer(cmd, mesh->pIndexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
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
	using namespace std::string_view_literals;

	loaded_meshes.emplace_back(engine::Mesh::load_from_obj(this, "assets/obj_models/rounded_cube.obj"));


	auto& mesh = loaded_meshes.back();
	auto material = std::make_shared<Material<>>();
	auto const spv_file_paths = std::array{
		"assets/shaders/pbr_texture.vert.spv"sv,
		"assets/shaders/pbr_texture.frag.spv"sv
	};
	material->pShaders = engine::Shader::createFromSpv(this, spv_file_paths);

	std::apply([this](auto&&... args) {
		((args = Texture::create_device_texture(this,
			TEXTURE_WIDTH,
			TEXTURE_HEIGHT,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)), ...);
		}, class_field_to_tuple(pbr_material_texture_set));
	bool oo = 0;

	//std::apply([&](auto&... x) { (ar(x), ...); }, class_field_to_tuple(pbr_material_texture_set));
	//pbr_material_texture_set = PbrMaterialTextureSet{
	//	.base_color = Texture::create_device_texture(this,
	//		TEXTURE_WIDTH,
	//		TEXTURE_HEIGHT,
	//		VK_FORMAT_R8G8B8A8_UNORM,
	//		VK_IMAGE_ASPECT_COLOR_BIT,
	//		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
	//	
	//};
}

void VulkanEngine::create_sync_objects() {
	frame_data.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(swapchain_image_count, VK_NULL_HANDLE);

	const VkSemaphoreCreateInfo semaphore_info = vkinit::semaphoreCreateInfo();
	const VkFenceCreateInfo fence_info = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device, &semaphore_info, nullptr, &frame_data[i].imageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphore_info, nullptr, &frame_data[i].renderFinishedSemaphore) != VK_SUCCESS ||
			vkCreateFence(device, &fence_info, nullptr, &frame_data[i].inFlightFence) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}

		main_deletion_queue.push_function([=]() {
			vkDestroyFence(device, frame_data[i].inFlightFence, nullptr);
			vkDestroySemaphore(device, frame_data[i].renderFinishedSemaphore, nullptr);
			vkDestroySemaphore(device, frame_data[i].imageAvailableSemaphore, nullptr);
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
	node_texture_1d_manager = std::make_shared<NodeTextureManager>(this, max_bindless_node_1d_textures);
	node_texture_2d_manager = std::make_shared<NodeTextureManager>(this, max_bindless_node_2d_textures);
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

void VulkanEngine::draw_frame() {
	//drawFrame will first acquire the index of the available swapchain image, then render into this image, and finally request to prensent this image

	vkWaitForFences(device, 1, &frame_data[current_frame].inFlightFence, VK_TRUE, VULKAN_WAIT_TIMEOUT); // begin draw i+2 frame if we've complete rendering at frame i

	uint32_t image_index;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, frame_data[current_frame].imageAvailableSemaphore, VK_NULL_HANDLE, &image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_swap_chain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	const ImGuiIO& io = ImGui::GetIO();

	// IMGUI RENDERING
	gui->begin_render();

	{
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

		ImGui::Image(static_cast<ImTextureID>(viewport_3d.gui_textures[image_index]), viewport_panel_size, ImVec2{ 0, 0 }, uv);
		mouse_hover_viewport = ImGui::IsItemHovered() ? true : false;
		ImGui::End();

		ImGui::PopStyleVar(3);
	}

	gui->end_render(image_index);

	record_viewport_cmd_buffer(image_index);

	std::array submit_command_buffers = { viewport_3d.cmd_buffers[image_index], gui->command_buffers[image_index] };

	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	const VkSubmitInfo submit_info{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame_data[current_frame].imageAvailableSemaphore,
		.pWaitDstStageMask = wait_stages,

		.commandBufferCount = static_cast<uint32_t>(submit_command_buffers.size()),
		.pCommandBuffers = submit_command_buffers.data(),

		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &frame_data[current_frame].renderFinishedSemaphore,
	};

	if (imagesInFlight[image_index] != VK_NULL_HANDLE) {
		vkWaitForFences(device, 1, &imagesInFlight[image_index], VK_TRUE, VULKAN_WAIT_TIMEOUT);  //start to render into this image if we've complete the last rendering of this image
	}
	imagesInFlight[image_index] = frame_data[current_frame].inFlightFence;  //update the fence of this image

	vkResetFences(device, 1, &frame_data[current_frame].inFlightFence);

	if (vkQueueSubmit(graphicsQueue, 1, &submit_info, frame_data[current_frame].inFlightFence) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	const VkPresentInfoKHR present_info{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame_data[current_frame].renderFinishedSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &swapChain,
		.pImageIndices = &image_index,
	};

	result = vkQueuePresentKHR(presentQueue, &present_info);

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

VkPresentModeKHR VulkanEngine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanEngine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	}
	else {
		glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(windowWidth),
			static_cast<uint32_t>(windowHeight)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

SwapChainSupportDetails VulkanEngine::querySwapChainSupport(VkPhysicalDevice device) {
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
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

bool VulkanEngine::isDeviceSuitable(VkPhysicalDevice device) {


	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return queueFamilyIndices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool VulkanEngine::checkDeviceExtensionSupport(VkPhysicalDevice device) {
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

QueueFamilyIndices VulkanEngine::findQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

std::vector<const char*> VulkanEngine::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if constexpr (enableValidationLayers) {
		extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	extensions.insert(extensions.end(), instance_extensions.begin(), instance_extensions.end());

	return extensions;
}

bool VulkanEngine::checkValidationLayerSupport() {
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

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanEngine::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	}

	return VK_FALSE;
}

void VulkanEngine::mouseCursorCallback(GLFWwindow* window, double xpos, double ypos) {

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
		return;
	}
	auto mouseCurrentPos = glm::vec2(xpos, ypos);
	mouse_delta_pos = mouseCurrentPos - mouse_previous_pos;

	if (mouse_hover_viewport) {
		camera.rotate(-mouse_delta_pos.x, -mouse_delta_pos.y, 0.005f);
	}

	mouse_previous_pos = mouseCurrentPos;
}

void VulkanEngine::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		mouse_previous_pos.x = xpos;
		mouse_previous_pos.y = ypos;
	}
}

void VulkanEngine::mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	if (mouse_hover_viewport) {
		camera.zoom(static_cast<float>(yoffset), 0.3f);
	}
}

void VulkanEngine::set_camera() {
	camera.set_aspect_ratio(viewport_3d.width / viewport_3d.height);
}

VulkanEngine::~VulkanEngine() {}