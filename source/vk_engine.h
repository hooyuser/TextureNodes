#pragma once
#include "vk_types.h"
#include "util/util.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include <deque>
#include <variant>
#include <span>
#include <ranges>
#include <unordered_map>

constexpr inline uint32_t TEXTURE_WIDTH = 1024;
constexpr inline uint32_t TEXTURE_HEIGHT = 1024;

struct FrameData {
	VkFence in_flight_fence;
	VkSemaphore image_available_semaphore;
	VkSemaphore render_finished_semaphore;
};

struct DeletionQueue {
	std::deque<std::function<void()>> deletors;

	void push_function(auto&& function) noexcept {
		deletors.emplace_back(FWD(function));
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto const& func : deletors | std::views::reverse) {
			func();
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
	std::vector<TexturePtr> color_resolve_textures;
	std::vector<void*> gui_textures;
	std::vector<VkCommandBuffer> cmd_buffers;
};
//End Forward Declaration


struct RenderObject {
	MeshPtr mesh;
	glm::mat4 transform_matrix;
};

struct PbrMaterialTextureSet {
	TexturePtr base_color;
	TexturePtr metalness;
	TexturePtr roughness;
	TexturePtr normal;
};

struct MaterialPreviewUBO {
	glm::vec4 base_color{ 0.8f, 0.8f , 0.8f ,1.0f };
	int base_color_texture_id = -1;
	float metallic = 0.0f;
	int metallic_texture_id = -1;
	float roughness = 0.4f;
	int roughness_texture_id = -1;
	int normal_texture_id = -1;
	int irradiance_map_id = -1;
	int brdf_LUT_id = -1;
	int prefiltered_map_id = -1;
};

class VulkanEngine {
public:
	~VulkanEngine();

	bool is_initialized{ false };

	void run() {
		init_window();
		init_vulkan();
		main_loop();
		cleanup();
	}

	uint32_t screen_width, screen_height;

	struct GLFWwindow* window{ nullptr };

	int window_width, window_height;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkSurfaceKHR surface;

	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkSampleCountFlagBits msaa_samples = VK_SAMPLE_COUNT_8_BIT;
	VkDevice device;

	VkQueue graphics_queue;
	VkQueue present_queue;
	QueueFamilyIndices queue_family_indices;

	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchain_images;
	VkFormat swapchain_image_format;
	VkExtent2D swapchain_extent;
	std::vector<VkImageView> swapchain_image_views;

	VkRenderPass render_pass;
	VkDescriptorSetLayout scene_set_layout;
	VkPipelineLayout mesh_pipeline_layout;
	VkPipelineLayout env_pipeline_layout;
	VkPipeline env_pipeline;

	ImagePtr color_image;
	ImagePtr depth_image;

	std::vector<RenderObject> renderables;
	std::unordered_map<std::string, engine::MaterialPtrV> materials;
	std::vector<engine::PbrMaterialPtr> loaded_materials;
	std::vector<MeshPtr> loaded_meshes;
	std::vector<TexturePtr> loaded_2d_textures;
	std::vector<TexturePtr> loaded_cubemap_textures;

	std::vector<BufferPtr> uniform_buffers;

	VkDescriptorPool descriptor_pool;
	std::vector<VkDescriptorSet> scene_descriptor_sets;

	VkDescriptorSet material_preview_descriptor_set;

	PbrMaterialTextureSet pbr_material_texture_set;
	VkPipelineLayout material_preview_pipeline_layout;
	BufferPtr material_preview_ubo;
	MaterialPreviewUBO init_material_preview_ubo;

	VkCommandPool command_pool;

	std::vector<VkFence> images_in_flight;
	std::vector<FrameData> frame_data;
	size_t current_frame = 0;
	bool framebuffer_resized = false;

	DeletionQueue main_deletion_queue;
	DeletionQueue swap_chain_deletion_queue;

	static Camera camera;
	static glm::vec2 mouse_previous_pos;
	static glm::vec2 mouse_delta_pos;
	static bool mouse_hover_viewport;

	std::shared_ptr<engine::GUI> gui;
	std::shared_ptr<engine::NodeEditor> node_editor;

	ViewportUI viewport_3d;

	uint32_t swapchain_image_count;

	constexpr static inline uint32_t max_bindless_node_2d_textures = 300;
	constexpr static inline uint32_t max_bindless_node_1d_textures = 50;
	VkDescriptorPool node_descriptor_pool;
	std::shared_ptr<NodeTextureManager> node_texture_2d_manager;
	std::shared_ptr<NodeTextureManager> node_texture_1d_manager;

	VkFence immediate_submit_fence;

	void init_window();

	static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);

	void init_vulkan();

	void main_loop();

	void cleanup();

	void recreate_swap_chain();

	void create_instance();

	static void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);

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

	void create_descriptor_set_layouts();

	void create_graphics_pipeline();

	void create_mesh_pipeline();

	void create_env_light_pipeline();

	void create_command_pool();

	void record_viewport_cmd_buffer(int command_buffer_index);

	void create_window_attachments();

	void load_gltf();

	void load_obj();

	VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

	VkFormat find_depth_format() const;

	static bool has_stencil_component(VkFormat format);

	VkSampleCountFlagBits get_max_usable_sample_count() const;

	VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, CreateResourceFlagBits imageViewDescription);

	void create_uniform_buffers();

	void create_descriptor_pool();

	void create_descriptor_sets();

	void create_command_buffers();

	void create_sync_objects();

	void init_imgui();

	void update_uniform_buffer(uint32_t currentImage);

	void draw_frame();

	void imgui_render(uint32_t image_index);

	static VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	static VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes);

	VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

	SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device) const;

	bool is_device_suitable(VkPhysicalDevice device);

	static bool check_device_extension_support(VkPhysicalDevice device);

	QueueFamilyIndices find_queue_families(VkPhysicalDevice device) const;

	static std::vector<const char*> get_required_extensions();

	static bool check_validation_layer_support();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

	static void mouse_cursor_callback(GLFWwindow* window, double x_pos, double y_pos);

	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

	static void mouse_scroll_callback(GLFWwindow* window, double x_offset, double y_offset);

	void set_camera() const;

	void create_descriptor_set_layout(std::span<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings, VkDescriptorSetLayout& descriptorSetLayout);
};