#pragma once
#include "deshi_defines.h"

#if defined(_MSC_VER)
#pragma comment(lib,"vulkan-1.lib")
#pragma comment(lib,"glfw3.lib")
#endif
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <boost/optional.hpp>

struct Window;

enum RenderAPI{
	VULKAN
};

struct Renderer{
	virtual void Init(Window* window) = 0;
	virtual void Draw() = 0;
	virtual void Cleanup() = 0;
};

////////////////////////////////
//// vulkan support structs ////
////////////////////////////////
/*TODO(r,delle) update vulkan rendering
In real implementation: 
https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer#page_Using-a-staging-buffer:~:text=You%20may
https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer#page_Conclusion:~:text=It%20should
https://vulkan-tutorial.com/en/Vertex_buffers/Index_buffer#page_Using-an-index-buffer:~:text=The%20previous
https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_layout_and_buffer#page_Updating-uniform-data:~:text=Using%20a%20UBO
https://vulkan-tutorial.com/en/Texture_mapping/Combined_image_sampler#page_Updating-the-descriptors:~:text=determined.-,It%20is
https://vulkan-tutorial.com/en/Generating_Mipmaps#page_Generating-Mipmaps:~:text=Beware%20if%20you
https://vulkan-tutorial.com/en/Generating_Mipmaps#page_Linear-filtering-support:~:text=There%20are%20two
https://vulkan-tutorial.com/en/Multisampling#page_Conclusion:~:text=features%2C-,like
*/

struct VertexVk{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	
	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
	bool operator==(const VertexVk& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std {
	template<> struct hash<VertexVk> {
		size_t operator()(VertexVk const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
};

struct UniformBufferObject{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct QueueFamilyIndices {
	boost::optional<uint32> graphicsFamily;
	boost::optional<uint32> presentFamily;
	bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct FrameVk{
	VkCommandPool   commandPool;
	VkCommandBuffer commandBuffer;
	VkFence         fence;
	VkImage         image;
	VkImageView     imageView;
	VkFramebuffer   framebuffer;
};

struct FrameSemaphoreVk{
	VkSemaphore imageAcquiredSemaphore;
	VkSemaphore renderCompleteSemaphore;
};

struct WindowVk{
	int32              width;
	int32              height;
	VkSwapchainKHR     swapchain;
	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR   presentMode;
	VkRenderPass       renderPass;
	VkPipeline         pipeline;
	uint32             frameIndex;
	uint32             imageCount;
	uint32             semaphoreIndex;
	FrameVk*           frames;
	FrameSemaphoreVk*  frameSephamores;
};

//////////////////////////////
//// vulkan delcarations  ////
//////////////////////////////

struct Renderer_Vulkan : public Renderer{
	///////////////////////////////
	//// user config variables ////
	///////////////////////////////
	
	//TODO(r,delle) INSERT VIDEO SETTINGS HERE
	int32 windowWidth;
	int32 windowHeight;
	int32 minImageCount = 2;
	int32 imageCount = minImageCount;
	
	//////////////////////////////
	//// vulkan api variables ////
	//////////////////////////////
	VkAllocationCallbacks* allocator = 0;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	GLFWwindow* window;
	VkSurfaceKHR surface;
	
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	QueueFamilyIndices physicalQueueFamilies;
	
	VkDevice device;
	QueueFamilyIndices deviceQueueFamilies;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	VkPipelineCache graphicsPipelineCache = VK_NULL_HANDLE;
	
	VkCommandPool commandPool;
	
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;
	
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	
	uint32 mipLevels;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	
	std::vector<VertexVk> vertices = {
		{{-0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
	};
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	std::vector<uint32> indices = {
		0, 1, 2, 2, 3, 0
	};
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	
	std::vector<VkCommandBuffer> commandBuffers;
	
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	size_t currentFrame = 0;
	
	bool framebufferResized = false;
	
	//////////////////////////
	//// vulkan functions ////
	//////////////////////////
	
	virtual void Init(Window* window) override;
	
	//grabs an image from swap chain, submits the command buffer to the command queue, adds the image to the presentation queue
	//https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation
	virtual void Draw() override;
	
	virtual void Cleanup() override;
	
	void cleanupSwapChain();
	
	//cleans up prev swap chain, recreates it and things that depend on it
	//https://vulkan-tutorial.com/en/Drawing_a_triangle/Swap_chain_recreation
	void recreateSwapChain();
	
	//TODO(r,delle) INSERT VIDEO SETTINGS HERE
	void createInstance();
	
	void setupDebugMessenger();
	
	//https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Window_surface
	void createSurface();
	
	//https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families
	void pickPhysicalDevice();
	
	//creates an interface between the actual GPU device and a virtual device for interaction
	//https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Logical_device_and_queues
	void createLogicalDevice();
	
	//creates a set of images that can be drawn to and presented to the window
	//https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain
	void createSwapChain();
	
	//creates an image view for each swap chain image to describe how to use/write to that image
	//https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Image_views
	void createImageViews();
	
	//TODO(r,delle) INSERT VIDEO SETTINGS HERE
	//how many color/depth buffers, their sample amounts, how they should be rendered
	//https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
	void createRenderPass();
	
	//creates (storage) buffers or images to be used by shaders
	//https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_layout_and_buffer
	void createDescriptorSetLayout();
	
	//TODO(r,delle) INSERT VIDEO SETTINGS HERE
	//almost all the parameters for doing graphics/rendering
	//https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Introduction
	void createGraphicsPipeline();
	
	//creates a pool for commands to be executed on a device
	//https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Command_buffers
	void createCommandPool();
	
	//creates colorImage with selected msaaSamples
	//https://vulkan-tutorial.com/en/Multisampling
	void createColorResources();
	
	//creates depthImage, a depth buffer image after finding optimal settings
	//https://vulkan-tutorial.com/en/Depth_buffering
	void createDepthResources();
	
	//creates a framebuffer per image view using the renderpass
	//https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Framebuffers
	void createFramebuffers();
	
	//creates staging image on CPU, copies texture pixels to it, creates vulkan image on GPU from the staging image
	//https://vulkan-tutorial.com/en/Texture_mapping/Images
	void createTextureImage();
	
	//creates textureImageView that specifies the attributes of the textureImage already created
	//https://vulkan-tutorial.com/en/Texture_mapping/Image_view_and_sampler
	void createTextureImageView();
	
	//TODO(r,delle) INSERT VIDEO SETTINGS HERE
	//used for filtering/anti-aliasing, this uses Repeat sampling
	//https://vulkan-tutorial.com/en/Texture_mapping/Image_view_and_sampler#page_Samplers
	void createTextureSampler();
	
	//creates a vertex buffer on CPU and GPU, fills a CPU staging buffer, copies it to the GPU
	//https://vulkan-tutorial.com/en/Vertex_buffers/Vertex_buffer_creation
	void createVertexBuffer();
	
	//creates an index buffer on CPU and GPU, fills a CPU staging buffer, copies it to the GPU
	//https://vulkan-tutorial.com/en/Vertex_buffers/Index_buffer
	void createIndexBuffer();
	
	//creates and allocates a uniform buffer per swap chain image
	//https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_layout_and_buffer
	void createUniformBuffers();
	
	//creates a pool of descriptors for a buffer (per image) to be sent to shaders
	//https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_pool_and_sets
	void createDescriptorPool();
	
	//creates the actual sets of info to be sent to the GPU
	//https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_pool_and_sets
	void createDescriptorSets();
	
	//TODO(r,delle) INSERT VIDEO SETTINGS HERE
	//creates a command buffer per frame buffer
	//https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Command_buffers
	void createCommandBuffers();
	
	//creates semaphores and fences indicating: images are ready, rendering is finished
	//[GPU-GPU sync] semaphores coordinate operations across command buffers so that they execute in a specified order	(pause threads)
	//[CPU-GPU sync] fences are similar but are waited for in the code itself rather than threads						(pause code)
	//https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation
	void createSyncObjects();
	
	//TODO(r,delle) INSERT VIDEO SETTINGS HERE
	//updates the uniform buffers used by shaders
	//https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_layout_and_buffer
	void updateUniformBuffer(uint32 currentImage);
	
	///////////////////////////
	//// utility functions ////
	///////////////////////////
	
	bool checkValidationLayerSupport();
	
	std::vector<const char*> getRequiredExtensions();
	
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
														VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	
	//device requires: anistrophy
	bool isDeviceSuitable(VkPhysicalDevice device);
	
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	
	//https://vulkan-tutorial.com/en/Multisampling
	VkSampleCountFlagBits getMaxUsableSampleCount();
	
	//this controls color formats
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	
	//this controls vsync/triple buffering
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	
	//returns the drawable dimensions of the window
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	
	//creates an image view specifying how to use an image
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32 mipLevels);
	
	//searches the device a format matching the arguments
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	
	//tries to find a viable depth format
	VkFormat findDepthFormat();
	//creates a shader module from the shader bytecode
	VkShaderModule createShaderModule(const std::vector<char>& code);
	
	//finds which memory types the graphics card offers
	uint32 findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties);
	
	//creates and binds a vulkan image to the GPU
	void createImage(uint32 width, uint32 height, uint32 mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, 
					 VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	
	//creates a buffer of defined usage and size on the device
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	
	//returns a command buffer that will only execute once
	VkCommandBuffer beginSingleTimeCommands();
	
	//ends a command buffer and frees that buffers memory
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	
	//converts a VkImage from one layout to another using an image memory barrier
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32 mipLevels);
	
	//uses commands to copy a buffer to an image
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32 width, uint32 height);
	
	//scans an image for max possible mipmaps and generates them
	//https://vulkan-tutorial.com/en/Generating_Mipmaps
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32 texWidth, int32 texHeight, uint32 mipLevels);
	
	//copies a buffer, we use this to copy from CPU to GPU
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	
	
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};
