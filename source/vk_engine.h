#pragma once

#include "vk_types.h"
#include "vk_camera.h"
#include "vk_buffer.h"
#include "vk_material.h"
#include "vk_gui.h"
#include "gui/gui_node_editor.h"


// #define GLFW_INCLUDE_VULKAN

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <deque>

#include <set>
#include <span>
#include <unordered_map>


struct QueueFamilyIndices;
struct SwapChainSupportDetails;


struct FrameData {
	VkFence inFlightFence;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
};

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) {
		deletors.emplace_back(function);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call the function
		}

		deletors.clear();
	}
};

namespace engine {
	class Mesh;
}

using MeshPtr = std::shared_ptr<engine::Mesh>;


struct ViewportUI {
	float width = 0.0;
	float height = 0.0;
	VkSampleCountFlagBits msaa_count = VK_SAMPLE_COUNT_1_BIT;
	VkRenderPass render_pass;
	std::vector<VkFramebuffer> framebuffers;
	std::vector<TexturePtr> color_textures;
	std::vector<TexturePtr> depth_textures;
	std::vector<void*> gui_textures;
	std::vector<VkCommandBuffer> cmd_buffers;
};



struct RenderObject {
	MeshPtr mesh;
	glm::mat4 transformMatrix;
};

class VulkanEngine {
public:
	bool isInitialized{ false };

	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

	uint32_t screen_width, screen_height;

	struct GLFWwindow* window{ nullptr };

	int windowWidth, windowHeight;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	QueueFamilyIndices queueFamilyIndices;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;

	VkRenderPass renderPass;
	VkDescriptorSetLayout sceneSetLayout;
	VkDescriptorSetLayout texSetLayout;
	VkPipelineLayout meshPipelineLayout;
	VkPipelineLayout envPipelineLayout;
	VkPipeline envPipeline;

	ImagePtr pColorImage;
	ImagePtr pDepthImage;

	std::vector<RenderObject> renderables;
	std::unordered_map<std::string, engine::MaterialPtrV> materials;
	std::vector<engine::PbrMaterialPtr> loadedMaterials;
	std::vector<MeshPtr> loadedMeshes;
	std::vector<TexturePtr> loadedTexture2Ds;
	std::vector<TexturePtr> loadedTextureCubemaps;

	std::vector<BufferPtr> pUniformBuffers;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> sceneDescriptorSets;

	VkCommandPool commandPool;
	

	std::vector<VkFence> imagesInFlight;
	std::vector<FrameData> frameData;
	size_t currentFrame = 0;
	bool framebufferResized = false;

	DeletionQueue main_deletion_queue;
	DeletionQueue swapChainDeletionQueue;

	static Camera camera;
	static glm::vec2 mouse_previous_pos;
	static glm::vec2 mouse_delta_pos;
	static bool mouse_hover_viewport;

	std::unique_ptr<engine::GUI> gui;

	std::unique_ptr<engine::NodeEditor> node_editor;

	ViewportUI viewport3D;

	uint32_t swapchain_image_count;

	void initWindow();

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	void initVulkan();

	void mainLoop();

	void cleanup();

	void recreateSwapChain();

	void createInstance();

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	void setupDebugMessenger();

	void createSurface();

	void pickPhysicalDevice();

	void createLogicalDevice();

	void create_viewport_attachments();

	void create_viewport_render_pass();

	void create_viewport_framebuffers();

	void create_viewport_cmd_buffers();

	void create_swap_chain();

	void create_swap_chain_image_views();

	void parseMaterialInfo();

	void createRenderPass();

	void createDescriptorSetLayouts();

	void createGraphicsPipeline();

	void createMeshPipeline();

	void createEnvLightPipeline();

	void createCommandPool();

	void record_viewport_cmd_buffer(const int commandBufferIndex);

	void create_window_attachments();

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	VkFormat findDepthFormat();

	bool hasStencilComponent(VkFormat format);

	VkSampleCountFlagBits getMaxUsableSampleCount();

	VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, CreateResourceFlagBits imageViewDescription);

	void create_uniform_buffers();

	void create_descriptor_pool();

	void create_descriptor_sets();

	void create_command_buffers();

	void create_sync_objects();

	void init_imgui();

	void update_uniform_buffer(uint32_t currentImage);

	void draw_frame();

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	bool isDeviceSuitable(VkPhysicalDevice device);

	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	std::vector<const char*> getRequiredExtensions();

	bool checkValidationLayerSupport();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

	static void mouseCursorCallback(GLFWwindow* window, double xpos, double ypos);

	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

	static void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void set_camera();

	void create_descriptor_set_layout(std::span<VkDescriptorSetLayoutBinding>&& descriptorSetLayoutBindings, VkDescriptorSetLayout& descriptorSetLayout);
};