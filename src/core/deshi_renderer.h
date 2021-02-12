#pragma once
#include "../utils/defines.h"
#include "../utils/Debug.h"

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

enum struct RenderAPI{
	VULKAN
};

struct Renderer{
	virtual void Init(Window* window) = 0;
	virtual void Draw() = 0;
	virtual void Render() = 0;
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

struct FramebufferAttachmentsVk {
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
};

struct WindowVk{
	int32                    width;
	int32                    height;
	VkSwapchainKHR           swapchain;
	VkSurfaceKHR             surface;
	SwapChainSupportDetails  supportDetails;
	VkSurfaceFormatKHR       surfaceFormat;
	VkPresentModeKHR         presentMode;
	VkExtent2D               extent;
	VkRenderPass             renderPass;
	VkPipeline               pipeline;
	uint32                   imageCount;
	bool                     clearEnable;
    VkClearValue*            clearValues;
	uint32                   frameIndex;
	uint32                   semaphoreIndex;
	FrameVk*                 frames;
	FrameSemaphoreVk*        frameSephamores;
	FramebufferAttachmentsVk attachments;
};

//////////////////////////////
//// vulkan delcarations  ////
//////////////////////////////

struct Renderer_Vulkan : public Renderer{
	///////////////////////////////
	//// user config variables ////
	///////////////////////////////
	
	//TODO(r,delle) INSERT VIDEO SETTINGS HERE
	
	
	//////////////////////////////
	//// vulkan api variables ////
	//////////////////////////////
	VkAllocationCallbacks* allocator = 0;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	GLFWwindow* glfwWindow;
	
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	QueueFamilyIndices physicalQueueFamilies;
	
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	
	VkDescriptorPool descriptorPool;
	VkPipelineCache graphicsPipelineCache = VK_NULL_HANDLE;

	WindowVk window = {0};

	int32 minImageCount = 0;
	bool framebufferResized = false;
	
	//////////////////////////
	//// vulkan functions ////
	//////////////////////////
	
	virtual void Init(Window* window) override;
	
	//grabs an image from swap chain, submits the command buffer to the command queue, adds the image to the presentation queue
	//https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation
	virtual void Draw() override;

	virtual void Render() override;
	
	virtual void Cleanup() override;

	void createInstance();
	
	void setupDebugMessenger();
	
	//https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Window_surface
	void createSurface();
	
	//https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families
	void pickPhysicalDevice();
	
	//creates an interface between the actual GPU device and a virtual device for interaction
	//https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Logical_device_and_queues
	void createLogicalDevice();

	//creates a pool of descriptors for a buffer (per image) to be sent to shaders
	//https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_pool_and_sets
	void createDescriptorPool();

	void CreateOrResizeWindow(int w, int h);

	//destroy old swap chain and in-flight frames, create a new swap chain with desired dimensions
	void CreateWindowSwapChain(int w, int h);

	void CreateWindowCommandBuffers();

	///////////////////////////
	//// utility functions ////
	///////////////////////////

	void DestroyFrame(FrameVk* frame);

	void DestroyFrameSemaphore(FrameSemaphoreVk* sema);

	int GetMinImageCountFromPresentMode(VkPresentModeKHR mode);
	
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
