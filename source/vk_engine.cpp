#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_shader.h"
#include "vk_mesh.h"
#include "vk_util.h"
#include "vk_pipeline.h"


#include <span>
#include <filesystem>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#include <glm/gtc/type_ptr.hpp>

#include <json.hpp>
using json = nlohmann::json;

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

using namespace engine;

constexpr uint32_t WIDTH = 1200;
constexpr uint32_t HEIGHT = 900;
constexpr auto SCALE_X = 0.5;
constexpr auto SCALE_Y = 0.5;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;
constexpr double maxFPS = 300.0;
constexpr double maxPeriod = 1.0 / maxFPS;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

constexpr std::array<const char*, 0> instance_extensions = {

};

constexpr std::array deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
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


void VulkanEngine::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	mouse_previous_pos.x = xpos;
	mouse_previous_pos.y = ypos;
	glfwSetCursorPosCallback(window, mouseCursorCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetScrollCallback(window, mouseScrollCallback);
}

void VulkanEngine::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}


void VulkanEngine::initVulkan() {
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();

	create_swap_chain();//recreateSwapChain
	create_swap_chain_image_views();//recreateSwapChain

	createCommandPool();
	parseMaterialInfo();
	createDescriptorSetLayouts();


	create_viewport_attachments();
	create_viewport_render_pass();
	create_viewport_framebuffers();
	create_viewport_cmd_buffers();

	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createGraphicsPipeline();

	createSyncObjects();

	setCamera();

	initImgui();

	isInitialized = true;
}



void VulkanEngine::mainLoop() {
	bool running = true;
	double lastTime = 0.0;

	while (!glfwWindowShouldClose(window)) {
		double time = glfwGetTime();
		double deltaTime = time - lastTime;
		if (deltaTime >= maxPeriod) {
			lastTime = time;
			glfwPollEvents();
			drawFrame();
		}

	}

	vkDeviceWaitIdle(device);
}

void VulkanEngine::cleanup() {

	if (isInitialized) {
		vkDeviceWaitIdle(device);

		swapChainDeletionQueue.flush();
		main_deletion_queue.flush();

		vkDestroyDevice(device, nullptr);

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}
}

void VulkanEngine::recreateSwapChain() {

	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
	while (windowWidth == 0 || windowHeight == 0) {
		glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	swapChainDeletionQueue.flush();

	create_swap_chain();
	create_swap_chain_image_views();

	//createRenderPass();
	//create_window_attachments();
	//createFramebuffers();
	//createUniformBuffers();
	//createDescriptorPool();
	//createDescriptorSets();
	//createGraphicsPipeline();
	//createCommandBuffers();
	setCamera();

	imagesInFlight.resize(swapchain_image_count, VK_NULL_HANDLE);

	gui->create_framebuffers();
	ImGui_ImplVulkan_SetMinImageCount(swapchain_image_count);
}

void VulkanEngine::createInstance() {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
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
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

void VulkanEngine::setupDebugMessenger() {
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}

	main_deletion_queue.push_function([=]() {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		});
}

void VulkanEngine::createSurface() {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void VulkanEngine::pickPhysicalDevice() {
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

void VulkanEngine::createLogicalDevice() {
	// queueFamilyIndices = findQueueFamilies(physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.emplace_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
	descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	// Enable non-uniform indexing
	descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
	descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
	descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
	createInfo.pNext = &descriptorIndexingFeatures;

	if (enableValidationLayers) {
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
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	swapchain_image_count = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && swapchain_image_count > swapChainSupport.capabilities.maxImageCount) {
		swapchain_image_count = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = swapchain_image_count;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


	uint32_t queueFamilyIndexArray[] = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };

	if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndexArray;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, swapChain, &swapchain_image_count, nullptr);
	swapChainImages.resize(swapchain_image_count);
	vkGetSwapchainImagesKHR(device, swapChain, &swapchain_image_count, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	swapChainDeletionQueue.push_function([=]() {
		vkDestroySwapchainKHR(device, swapChain, nullptr);
		});
}

void VulkanEngine::create_swap_chain_image_views() {
	swapChainImageViews.resize(swapChainImages.size());

	for (uint32_t i = 0; i < swapChainImages.size(); i++) {
		swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, AFTER_SWAPCHAIN_BIT);
	}
}

void VulkanEngine::createRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = msaaSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = msaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = swapChainImageFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}

	swapChainDeletionQueue.push_function([=]() {
		vkDestroyRenderPass(device, renderPass, nullptr);
		});
}


void VulkanEngine::parseMaterialInfo() {
	const std::string filename = "assets/gltf_models/dragon.gltf";
	tinygltf::Model glTFModel;
	tinygltf::TinyGLTF gltfContext;
	std::string error, warning;

	bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFModel, &error, &warning, filename);

	if (fileLoaded) {

		for (size_t i = 0; i < glTFModel.images.size(); i++) {
			tinygltf::Image& glTFImage = glTFModel.images[i];
			loadedTexture2Ds.emplace_back(engine::Texture::load2DTextureFromHost(this, glTFImage.image.data(), glTFImage.width, glTFImage.height, glTFImage.component));
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

			loadedMaterials.emplace_back(material);
			materials.try_emplace(glTFMaterial.name, std::move(material));
		}

		for (auto& glTFMesh : glTFModel.meshes) {
			loadedMeshes.emplace_back(engine::Mesh::loadFromGLTF(this, glTFModel, glTFMesh));
		}

		// Only support one scene
		for (auto node_index : glTFModel.scenes[0].nodes) {
			const tinygltf::Node& node = glTFModel.nodes[node_index];

			if (node.mesh > -1) {
				RenderObject renderobject;
				renderobject.mesh = loadedMeshes[node.mesh];
				if (node.translation.size() == 3) {
					renderobject.transformMatrix = glm::translate(renderobject.transformMatrix, glm::vec3(glm::make_vec3(node.translation.data())));
				}
				if (node.rotation.size() == 4) {
					glm::quat q = glm::make_quat(node.rotation.data());
					renderobject.transformMatrix *= glm::mat4(q);
				}
				if (node.scale.size() == 3) {
					renderobject.transformMatrix = glm::scale(renderobject.transformMatrix, glm::vec3(glm::make_vec3(node.scale.data())));
				}
				if (node.matrix.size() == 16) {
					renderobject.transformMatrix = glm::make_mat4x4(node.matrix.data());
				};
				renderables.emplace_back(std::move(renderobject));
			}
		}
	}

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
		envMat->textureArrayIndex.emplace("cubemap", loadedTextureCubemaps.size());
		envMat->paras.baseColorTextureID = loadedTextureCubemaps.size();
		loadedTextureCubemaps.emplace_back(engine::Texture::loadCubemapTexture(this, envMaterialInfoJson["filePaths"].get<std::vector<std::string>>()));
		for (auto& mat : loadedMaterials) {
			mat->paras.irradianceMapId = loadedTextureCubemaps.size();
		}
		loadedTextureCubemaps.emplace_back(engine::Texture::loadCubemapTexture(this, envMaterialInfoJson["irradianceMapPaths"].get<std::vector<std::string>>()));

		for (auto& mat : loadedMaterials) {
			mat->paras.prefilteredMapId = loadedTextureCubemaps.size();
		}
		loadedTextureCubemaps.emplace_back(engine::Texture::loadPrefilteredMapTexture(this, envMaterialInfoJson["prefilteredMapPaths"].get<std::vector<std::vector<std::string>>>()));

		for (auto& mat : loadedMaterials) {
			mat->paras.brdfLUTId = loadedTexture2Ds.size();
		}
		loadedTexture2Ds.emplace_back(engine::Texture::load2DTexture(this, envMaterialInfoJson["BRDF_2D_LUT"].get<std::string>(), false));
	}

	for (auto& mat : loadedMaterials) {
		mat->paras.texture2DArraySize = loadedTexture2Ds.size();
	}

	for (auto& [name, mat] : materials) {
		std::visit([this](auto& obj) {
			obj->paras.textureCubemapArraySize = loadedTextureCubemaps.size();
			}, mat);
	}

	RenderObject skyBoxObject;
	skyBoxObject.mesh = Mesh::loadFromObj(this, "assets/obj_models/skybox.obj");
	skyBoxObject.mesh->pMaterial = std::get<HDRiMaterialPtr>(materials["env_light"]);
	skyBoxObject.transformMatrix = glm::mat4{ 1.0f };
	renderables.emplace_back(std::move(skyBoxObject));
}

void VulkanEngine::createDescriptorSetLayouts() {

	auto camUboLayoutBinding = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	auto texture2DArrayLayoutBinding = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, loadedTexture2Ds.size());
	auto cubemapArrayLayoutBinding = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, loadedTextureCubemaps.size());

	std::array<VkDescriptorSetLayoutBinding, 3> scenebindings = { camUboLayoutBinding, texture2DArrayLayoutBinding, cubemapArrayLayoutBinding };
	createDescriptorSetLayout(scenebindings, sceneSetLayout);
}

void VulkanEngine::createDescriptorSetLayout(std::span<VkDescriptorSetLayoutBinding>&& descriptorSetLayoutBindings, VkDescriptorSetLayout& descriptorSetLayout) {
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
	layoutInfo.pBindings = descriptorSetLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	main_deletion_queue.push_function([=]() {
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		});
}

void VulkanEngine::createMeshPipeline() {

	PipelineBuilder pipelineBuilder(this);

	std::array<VkDescriptorSetLayout, 1> meshDescriptorSetLayouts = { sceneSetLayout };

	VkPipelineLayoutCreateInfo meshPipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(meshDescriptorSetLayouts);

	if (vkCreatePipelineLayout(device, &meshPipelineLayoutInfo, nullptr, &meshPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	swapChainDeletionQueue.push_function([=]() {
		vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
		});

	for (auto& material : loadedMaterials) {
		pipelineBuilder.setShaderStages(material);

		pipelineBuilder.depthStencil = vkinit::depthStencilCreateInfo(VK_COMPARE_OP_LESS);


		pipelineBuilder.buildPipeline(device, viewport3D.render_pass, meshPipelineLayout, material->pipeline);

		main_deletion_queue.push_function([=]() {
			vkDestroyPipeline(device, material->pipeline, nullptr);
			});

		material->pipelineLayout = meshPipelineLayout;
	}
}

void VulkanEngine::createEnvLightPipeline() {
	PipelineBuilder pipelineBuilder(this);

	pipelineBuilder.setShaderStages(std::get<HDRiMaterialPtr>(materials["env_light"]));

	std::array<VkDescriptorSetLayout, 1> envDescriptorSetLayouts = { sceneSetLayout };

	VkPipelineLayoutCreateInfo envPipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(envDescriptorSetLayouts);

	if (vkCreatePipelineLayout(device, &envPipelineLayoutInfo, nullptr, &envPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	pipelineBuilder.depthStencil = vkinit::depthStencilCreateInfo(VK_COMPARE_OP_LESS_OR_EQUAL);

	pipelineBuilder.buildPipeline(device, viewport3D.render_pass, envPipelineLayout, envPipeline);

	std::get<HDRiMaterialPtr>(materials["env_light"])->pipelineLayout = envPipelineLayout;
	std::get<HDRiMaterialPtr>(materials["env_light"])->pipeline = envPipeline;

	main_deletion_queue.push_function([=]() {
		vkDestroyPipeline(device, envPipeline, nullptr);
		vkDestroyPipelineLayout(device, envPipelineLayout, nullptr);
		});
}

void VulkanEngine::createGraphicsPipeline() {
	createMeshPipeline();
	createEnvLightPipeline();
}

//void VulkanEngine::createFramebuffers() {
//	swapChainFramebuffers.resize(swapChainImageViews.size());
//
//	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
//		std::array<VkImageView, 3> attachments = {
//			pColorImage->imageView,
//			pDepthImage->imageView,
//			swapChainImageViews[i]
//		};
//
//		VkFramebufferCreateInfo framebufferInfo = vkinit::framebufferCreateInfo(renderPass, swapChainExtent, attachments);
//
//		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create framebuffer!");
//		}
//
//		swapChainDeletionQueue.push_function([=]() {
//			vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
//			});
//	}
//}

void VulkanEngine::createCommandPool() {

	uint32_t queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::commandPoolCreateInfo(queueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics command pool!");
	}

	main_deletion_queue.push_function([=]() {
		vkDestroyCommandPool(device, commandPool, nullptr);
		});
}

void VulkanEngine::create_window_attachments() {
	VkFormat colorFormat = swapChainImageFormat;
	VkFormat depthFormat = findDepthFormat();

	pColorImage = engine::Image::createImage(this,
		swapChainExtent.width,
		swapChainExtent.height,
		1,
		msaaSamples,
		colorFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		AFTER_SWAPCHAIN_BIT);

	pDepthImage = engine::Image::createImage(this,
		swapChainExtent.width,
		swapChainExtent.height,
		1,
		msaaSamples,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		AFTER_SWAPCHAIN_BIT);
}

void VulkanEngine::create_viewport_attachments() {
	VkFormat colorFormat = swapChainImageFormat;
	VkFormat depthFormat = findDepthFormat();

	auto mode = glfwGetVideoMode(glfwGetPrimaryMonitor()); //get monitor resolution
	screen_width = mode->width;
	screen_height = mode->height;

	for (size_t i = 0; i < swapchain_image_count; i++) {
		viewport3D.color_textures.emplace_back(engine::Texture::create_2D_render_target(this,
			screen_width,
			screen_height,
			colorFormat,
			VK_IMAGE_ASPECT_COLOR_BIT,
			BEFORE_SWAPCHAIN_BIT));

		viewport3D.depth_textures.emplace_back(engine::Texture::create_2D_render_target(this,
			screen_width,
			screen_height,
			depthFormat,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			BEFORE_SWAPCHAIN_BIT));
	}
}

void VulkanEngine::create_viewport_render_pass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = 0;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
	dependencies[1].dependencyFlags = 0;

	std::array attachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();
	renderPassInfo.pNext = nullptr;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &viewport3D.render_pass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}

	main_deletion_queue.push_function([=]() {
		vkDestroyRenderPass(device, viewport3D.render_pass, nullptr);
		});
}


void VulkanEngine::create_viewport_framebuffers() {
	viewport3D.framebuffers.resize(swapchain_image_count);

	for (size_t i = 0; i < swapchain_image_count; i++) {
		std::array attachments = {
			viewport3D.color_textures[i]->imageView,
			viewport3D.depth_textures[i]->imageView,
		};

		VkFramebufferCreateInfo framebufferInfo = vkinit::framebufferCreateInfo(viewport3D.render_pass, VkExtent2D{ screen_width , screen_height }, attachments);

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &viewport3D.framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}

		main_deletion_queue.push_function([=]() {
			vkDestroyFramebuffer(device, viewport3D.framebuffers[i], nullptr);
			});
	}
}

void VulkanEngine::create_viewport_cmd_buffers() {
	viewport3D.cmd_buffers.resize(swapchain_image_count);
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(commandPool, (uint32_t)viewport3D.cmd_buffers.size());

	if (vkAllocateCommandBuffers(device, &cmdAllocInfo, viewport3D.cmd_buffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	main_deletion_queue.push_function([=]() {
		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(viewport3D.cmd_buffers.size()), viewport3D.cmd_buffers.data());
		});
}

VkFormat VulkanEngine::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
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

VkFormat VulkanEngine::findDepthFormat() {
	return findSupportedFormat(
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

VkImageView VulkanEngine::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, CreateResourceFlagBits imageViewDescription) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	if (imageViewDescription == AFTER_SWAPCHAIN_BIT) {
		swapChainDeletionQueue.push_function([=]() {
			vkDestroyImageView(device, imageView, nullptr);
			});
	}
	else if (imageViewDescription == BEFORE_SWAPCHAIN_BIT) {
		main_deletion_queue.push_function([=]() {
			vkDestroyImageView(device, imageView, nullptr);
			});
	}
	else {
		throw std::runtime_error("illegal imageViewDescription value");
	}

	return imageView;
}

void VulkanEngine::createUniformBuffers() {
	pUniformBuffers.resize(swapchain_image_count);

	for (size_t i = 0; i < swapchain_image_count; i++) {
		pUniformBuffers[i] = engine::Buffer::createBuffer(this,
			sizeof(UniformBufferObject),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			BEFORE_SWAPCHAIN_BIT);
	}
}

void VulkanEngine::createDescriptorPool() {
	const uint32_t descriptorSize = swapchain_image_count * 100;
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorSize },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorSize }
	};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = 0;
	poolInfo.maxSets = descriptorSize;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	main_deletion_queue.push_function([=]() {
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		});
}

void VulkanEngine::createDescriptorSets() {
	VkDescriptorSetAllocateInfo allocSceneInfo{};
	allocSceneInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocSceneInfo.descriptorPool = descriptorPool;
	allocSceneInfo.descriptorSetCount = 1;
	allocSceneInfo.pSetLayouts = &sceneSetLayout;
	sceneDescriptorSets.resize(swapchain_image_count);

	for (size_t i = 0; i < swapchain_image_count; i++) {

		if (vkAllocateDescriptorSets(device, &allocSceneInfo, &sceneDescriptorSets[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = pUniformBuffers[i]->buffer;
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(UniformBufferObject);

		std::vector<VkDescriptorImageInfo> textureDescriptors(loadedTexture2Ds.size());
		for (size_t i = 0; i < loadedTexture2Ds.size(); i++) {
			textureDescriptors[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			textureDescriptors[i].sampler = loadedTexture2Ds[i]->sampler;;
			textureDescriptors[i].imageView = loadedTexture2Ds[i]->imageView;
		}

		std::vector<VkDescriptorImageInfo> cubemapDescriptors(loadedTextureCubemaps.size());
		for (size_t i = 0; i < loadedTextureCubemaps.size(); i++) {
			cubemapDescriptors[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			cubemapDescriptors[i].sampler = loadedTextureCubemaps[i]->sampler;;
			cubemapDescriptors[i].imageView = loadedTextureCubemaps[i]->imageView;
		}

		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = sceneDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = sceneDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = static_cast<uint32_t>(textureDescriptors.size());
		descriptorWrites[1].pImageInfo = textureDescriptors.data();

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = sceneDescriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2].descriptorCount = static_cast<uint32_t>(cubemapDescriptors.size());
		descriptorWrites[2].pImageInfo = cubemapDescriptors.data();

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

VkCommandBuffer VulkanEngine::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanEngine::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}


void VulkanEngine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

uint32_t VulkanEngine::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void VulkanEngine::createCommandBuffers() {
	viewport3D.cmd_buffers.resize(swapchain_image_count);
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(commandPool, (uint32_t)viewport3D.cmd_buffers.size());

	if (vkAllocateCommandBuffers(device, &cmdAllocInfo, viewport3D.cmd_buffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}



	swapChainDeletionQueue.push_function([=]() {
		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(viewport3D.cmd_buffers.size()), viewport3D.cmd_buffers.data());
		});
}

void VulkanEngine::record_viewport_cmd_buffer(const int commandBufferIndex) {
	auto const i = commandBufferIndex;

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(viewport3D.cmd_buffers[i], &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo = vkinit::renderPassBeginInfo(viewport3D.render_pass, swapChainExtent, viewport3D.framebuffers[i]);

	vkCmdBeginRenderPass(viewport3D.cmd_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {};
	//if (static_cast<float>(windowWidth) / windowHeight >= viewport3D.width / viewport3D.height) {
	//	viewport.width = viewport3D.height * static_cast<float>(windowWidth) / windowHeight;
	//	viewport.height = -viewport3D.height;
	//	viewport.x = viewport3D.x - 0.5 * (viewport.width - viewport3D.width);
	//	viewport.y = viewport3D.y + viewport3D.height;
	//} 
	//else {
	//	viewport.width = viewport3D.width;
	//	viewport.height = -viewport3D.width * static_cast<float>(windowHeight) / windowWidth ;
	//	viewport.x = viewport3D.x - 0.5 * (viewport.width - viewport3D.width);
	//	viewport.y = viewport3D.y - viewport.height;
	//}
	viewport.x = 0.0f;
	viewport.y = viewport3D.height;
	viewport.width = viewport3D.width;
	viewport.height = -viewport3D.height;    // flip y axis
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.extent.width = viewport3D.width;
	scissor.extent.height = viewport3D.height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;

	vkCmdSetViewport(viewport3D.cmd_buffers[i], 0, 1, &viewport);
	vkCmdSetScissor(viewport3D.cmd_buffers[i], 0, 1, &scissor);

	MeshPtr lastMesh = nullptr;
	MaterialPtrV lastMaterial;

	auto cmd = viewport3D.cmd_buffers[i];

	for (int k = 0; k < renderables.size(); k++)
	{
		RenderObject& object = renderables[k];

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, envPipelineLayout, 0, 1, &sceneDescriptorSets[i], 0, nullptr);

		//only bind the pipeline if it doesnt match with the already bound one

		//only bind the mesh if its a different one from last bind
		if (object.mesh != lastMesh) {
			if (object.mesh->pMaterial != lastMaterial) {
				if (std::holds_alternative<PbrMaterialPtr>(object.mesh->pMaterial)) {
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, std::get<PbrMaterialPtr>(object.mesh->pMaterial)->pipeline);
				}
				else if (std::holds_alternative<HDRiMaterialPtr>(object.mesh->pMaterial)) {
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, std::get<HDRiMaterialPtr>(object.mesh->pMaterial)->pipeline);
				}
				lastMaterial = object.mesh->pMaterial;

			}
			//bind the mesh vertex buffer with offset 0
			VkBuffer vertexBuffers[] = { object.mesh->pVertexBuffer->buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(cmd, object.mesh->pIndexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
			lastMesh = object.mesh;
		}
		//we can now draw
		vkCmdDrawIndexed(cmd, static_cast<uint32_t>(object.mesh->_indices.size()), 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(viewport3D.cmd_buffers[i]);



	if (vkEndCommandBuffer(viewport3D.cmd_buffers[i]) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void VulkanEngine::createSyncObjects() {
	frameData.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(swapchain_image_count, VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = vkinit::semaphoreCreateInfo();
	VkFenceCreateInfo fenceInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameData[i].imageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameData[i].renderFinishedSemaphore) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &frameData[i].inFlightFence) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}

		main_deletion_queue.push_function([=]() {
			vkDestroyFence(device, frameData[i].inFlightFence, nullptr);
			vkDestroySemaphore(device, frameData[i].renderFinishedSemaphore, nullptr);
			vkDestroySemaphore(device, frameData[i].imageAvailableSemaphore, nullptr);
			});
	}
}

void VulkanEngine::initImgui() {
	gui = std::make_unique<engine::GUI>();
	gui->init(this);
}

void VulkanEngine::updateUniformBuffer(uint32_t currentImage) {
	static auto start_time = std::chrono::high_resolution_clock::now();

	auto current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	camera.set_aspect_ratio(viewport3D.width / viewport3D.height);
	ubo.view = camera.view_matrix();
	//ubo.view = glm::mat4(glm::mat3(camera.viewMatrix()));
	ubo.proj = camera.proj_matrix();
	ubo.pos = camera.get_position();
	//ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
	//ubo.proj[1][1] *= -1;

	pUniformBuffers[currentImage]->copyFromHost(&ubo);
}

void VulkanEngine::drawFrame() {
	//drawFrame will first acquire the index of the available swapchain image, then render into this image, and finally request to prensent this image

	vkWaitForFences(device, 1, &frameData[currentFrame].inFlightFence, VK_TRUE, UINT64_MAX); // begin draw i+2 frame if we've complete rendering at frame i

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, frameData[currentFrame].imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}



	// IMGUI RENDERING
	gui->beginRender();

	{
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;


		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", nullptr, window_flags);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();

		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

		static auto first_time = true;

		static ImGuiID dock_id_right_p, dock_id_down, dock_id_left;
		ImGuiID dock_id_right = ImGui::GetID("Right");;

		if (first_time)
		{
			first_time = false;

			ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
			ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
			//auto dockspace_id_Node = ImGui::DockBuilderGetNode(dockspace_id);

			// split the dockspace into 2 nodes -- DockBuilderSplitNode takes in the following args in the following order
			//   window ID to split, direction, fraction (between 0 and 1), the final two setting let's us choose which id we want (which ever one we DON'T set as NULL, will be returned by the function)
			//                                                              out_id_at_dir is the id of the node in the direction we specified earlier, out_id_at_opposite_dir is in the opposite direction

			dock_id_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.5f, nullptr, &dockspace_id);
			auto up_node = ImGui::DockBuilderGetNode(dockspace_id);
			up_node->SetLocalFlags(up_node->LocalFlags | ImGuiDockNodeFlags_PassthruCentralNode);
			//auto dock_id_down_Node = ImGui::DockBuilderGetNode(dock_id_down);

			dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.5f, nullptr, &dock_id_right_p);
			auto dock_id_right_p_Node = ImGui::DockBuilderGetNode(dock_id_right_p);

			//dock_id_right_p_Node->SetLocalFlags(dock_id_right_Node->LocalFlags | ImGuiDockNodeFlags_PassthruCentralNode);
			//dock_id_right_p_Node->UpdateMergedFlags();

			auto size = dock_id_right_p_Node->Size;
			auto pos = dock_id_right_p_Node->Pos;
			ImGui::DockBuilderAddNode(dock_id_right, dockspace_flags);
			ImGui::DockBuilderSetNodeSize(dock_id_right, size);
			ImGui::DockBuilderSetNodePos(dock_id_right, pos);
			auto right_node = ImGui::DockBuilderGetNode(dock_id_right);
			up_node->ChildNodes[0] = ImGui::DockBuilderGetNode(dock_id_left);
			up_node->ChildNodes[1] = right_node;
			right_node->ParentNode = up_node;

			//dock_id_right_Node->LocalFlags |= ImGuiDockNodeFlags_PassthruCentralNode;
			//dock_id_right_Node->LocalFlagsInWindows |= ImGuiDockNodeFlags_PassthruCentralNode;
			//dock_id_right_Node->UpdateMergedFlags();

			// we now dock our windows into the docking node we made above
			ImGui::DockBuilderDockWindow("Down", dock_id_down);
			ImGui::DockBuilderDockWindow("Left", dock_id_left);
			ImGui::DockBuilderDockWindow("Right", dock_id_right);
			ImGui::DockBuilderFinish(dockspace_id);
		}

		//auto viewport3DNode = ImGui::DockBuilderGetNode(dock_id_right);
		//if (viewport3D.width != viewport3DNode->Size.x) {
		//	viewport3D.width = viewport3DNode->Size.x;
		//	viewport3D.toBeChangedNum = commandBuffers.size();
		//}
		//if (viewport3D.height != viewport3DNode->Size.y) {
		//	viewport3D.height = viewport3DNode->Size.y;
		//	viewport3D.toBeChangedNum = commandBuffers.size();
		//}
		//if (viewport3D.x != viewport3DNode->Pos.x) {
		//	viewport3D.x = viewport3DNode->Pos.x;
		//	viewport3D.toBeChangedNum = commandBuffers.size();
		//}
		//if (viewport3D.y = viewport3DNode->Pos.y) {
		//	viewport3D.y = viewport3DNode->Pos.y;
		//	viewport3D.toBeChangedNum = commandBuffers.size();
		//}
		ImGui::End();

		auto viewport_3D_window_flags = ImGuiWindowFlags_NoBackground;

		ImGui::Begin("Left");

		{
			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			ImVec2 viewportPanelPos = ImGui::GetWindowContentRegionMin();
			ImGui::Text("Pos = (%f, %f)", viewportPanelPos.x, viewportPanelPos.y);
			ImGui::Text("Size = (%f, %f)", viewportPanelSize.x, viewportPanelSize.y);
		}
		ImGui::End();

		ImGui::Begin("Down");
		{
			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			ImVec2 viewportPanelPos = ImGui::GetWindowContentRegionMin();
			ImGui::Text("Pos = (%f, %f)", viewportPanelPos.x, viewportPanelPos.y);
			ImGui::Text("Size = (%f, %f)", viewportPanelSize.x, viewportPanelSize.y);
		}
		//node = ImGui::DockBuilderGetNode(dock_id_down);
		//ImGui::Text("Pos = (%f, %f)", node->Pos.x, node->Pos.y);
		//ImGui::Text("Size = (%f, %f)", node->Size.x, node->Size.y);
		ImGui::End();

		ImGui::SetNextWindowBgAlpha(0.0f);
		//ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Set window background to red
		//ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Set window background to red
		ImGui::Begin("Right", nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		ImVec2 viewportPanelPos = ImGui::GetWindowContentRegionMin();
		viewport3D.width = viewportPanelSize.x;
		viewport3D.height = viewportPanelSize.y;
		viewport3D.x = viewportPanelPos.x;
		viewport3D.y = viewportPanelPos.y;
		//::PopStyleColor(2);
		//node = ImGui::DockBuilderGetNode(dock_id_right);
		//ImGui::Text("Pos = (%f, %f)", node->Pos.x, node->Pos.y);
		//ImGui::Text("Size = (%f, %f)", node->Size.x, node->Size.y);
		{
			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			ImVec2 viewportPanelPos = ImGui::GetWindowContentRegionMin();
			ImGui::Text("Pos = (%f, %f)", viewportPanelPos.x, viewportPanelPos.y);
			ImGui::Text("Size = (%f, %f)", viewportPanelSize.x, viewportPanelSize.y);
		}

		ImGui::End();

	}

	gui->endRender(this, imageIndex);


	//if (viewport3D.toBeChangedNum > 0) {
	//	record_viewport_cmd_buffer(imageIndex);
	//	--viewport3D.toBeChangedNum;
	//}

	ImGuiIO& io = ImGui::GetIO();

	if (!io.WantCaptureMouse) {
		updateUniformBuffer(imageIndex);
	}
	record_viewport_cmd_buffer(imageIndex);

	std::array submitCommandBuffers = { viewport3D.cmd_buffers[imageIndex], gui->commandBuffers[imageIndex] };
	//std::array submitCommandBuffers = { gui->commandBuffers[imageIndex] };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &frameData[currentFrame].imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &frameData[currentFrame].renderFinishedSemaphore;

	submitInfo.commandBufferCount = submitCommandBuffers.size();
	submitInfo.pCommandBuffers = submitCommandBuffers.data();

	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);  //start to render into this image if we've complete the last rendering of this image
	}
	imagesInFlight[imageIndex] = frameData[currentFrame].inFlightFence;  //update the fence of this image

	vkResetFences(device, 1, &frameData[currentFrame].inFlightFence);

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameData[currentFrame].inFlightFence) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &frameData[currentFrame].renderFinishedSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

VkSurfaceFormatKHR VulkanEngine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
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

	if (enableValidationLayers) {
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

void VulkanEngine::mouseCursorCallback(GLFWwindow* window, double xpos, double ypos)
{

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
	{
		return;
	}
	auto mouseCurrentPos = glm::vec2(xpos, ypos);
	mouse_delta_pos = mouseCurrentPos - mouse_previous_pos;

	if (!ImGui::GetIO().WantCaptureMouse) {
		camera.rotate(-mouse_delta_pos.x, -mouse_delta_pos.y, 0.005f);
	}

	mouse_previous_pos = mouseCurrentPos;
}

void VulkanEngine::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		mouse_previous_pos.x = xpos;
		mouse_previous_pos.y = ypos;
	}
}

void VulkanEngine::mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	if (!ImGui::GetIO().WantCaptureMouse) {
		camera.zoom((float)yoffset, 0.3f);
	}
}

void VulkanEngine::setCamera() {
	camera.set_aspect_ratio(swapChainExtent.width / (float)swapChainExtent.height);
}
