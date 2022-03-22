#pragma once
#include "vk_types.h"
//#include "gui/gui_node_texture_manager.h"

// #define GLFW_INCLUDE_VULKAN

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include <deque>
#include <variant>
#include <span>
#include <unordered_map>
#include <unordered_set>

struct FrameData {
	VkFence inFlightFence;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
};

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	inline void push_function(std::function<void()>&& function) {
		deletors.emplace_back(function);
	}

	inline void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call the function
		}

		deletors.clear();
	}
};

//Forward Declaration
struct SwapChainSupportDetails;
class Camera;
struct Pbr;
struct HDRi;
struct NodeTextureManager;

namespace engine {
	class Mesh;
	class Buffer;
	class Image;
	class Texture;
	class GUI;
	class NodeEditor;

	struct Empty_Type;
	template <typename ParaT> class Material;
	
	using PbrMaterial = Material<Pbr>;
	using HDRiMaterial = Material<HDRi>;
	using PbrMaterialPtr = std::shared_ptr<PbrMaterial>;
	using HDRiMaterialPtr = std::shared_ptr<HDRiMaterial>;
	using MaterialPtr = std::shared_ptr<Material<Empty_Type>>;
	using MaterialPtrV = std::variant<MaterialPtr, PbrMaterialPtr, HDRiMaterialPtr>;
}

using MeshPtr = std::shared_ptr<engine::Mesh>;
using BufferPtr = std::shared_ptr<engine::Buffer>;
using ImagePtr = std::shared_ptr<engine::Image>;
using TexturePtr = std::shared_ptr<engine::Texture>;

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
//End Forward Declaration


struct RenderObject {
	MeshPtr mesh;
	glm::mat4 transformMatrix;
};

class VulkanEngine {
public:
	~VulkanEngine();

	bool isInitialized{ false };

	void run() {
		init_window();
		init_vulkan();
		main_loop();
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
	VkDescriptorSetLayout material_preview_set_layout;
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

	std::shared_ptr<engine::GUI> gui;
	std::shared_ptr<engine::NodeEditor> node_editor;

	ViewportUI viewport3D;

	uint32_t swapchain_image_count;

	constexpr static inline uint32_t max_bindless_node_2d_textures = 300;
	constexpr static inline uint32_t max_bindless_node_1d_textures = 50;
	VkDescriptorPool node_descriptor_pool;
	std::shared_ptr<NodeTextureManager> node_texture_2d_manager;
	std::shared_ptr<NodeTextureManager> node_texture_1d_manager;

	VkFence immediate_submit_fence;

	void init_window();

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	void init_vulkan();

	void main_loop();

	void cleanup();

	void recreate_swap_chain();

	void create_instance();

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	void setup_debug_messenger();

	void create_surface();

	void pick_physical_device();

	void create_logical_device();

	void create_viewport_attachments();

	void create_viewport_render_pass();

	void create_viewport_framebuffers();

	void create_viewport_cmd_buffers();

	void create_swap_chain();

	void create_swap_chain_image_views();

	void parse_material_info();

	//void create_render_pass();

	void create_descriptor_set_layouts();

	void create_graphics_pipeline();

	void create_mesh_pipeline();

	void create_env_light_pipeline();

	void create_command_pool();

	void record_viewport_cmd_buffer(const int commandBufferIndex);

	void create_window_attachments();

	void load_gltf();

	void load_obj();

	VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

	VkFormat find_depth_format();

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

	VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);

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

	void create_descriptor_set_layout(std::span<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings, VkDescriptorSetLayout& descriptorSetLayout);
};