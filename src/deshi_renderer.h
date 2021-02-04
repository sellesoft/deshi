#pragma once
#include "deshi_defines.h"
#include "deshi_glfw.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <boost/optional.hpp>

enum RenderAPI{
	VULKAN
};

struct Renderer{
	virtual void Init(Window* window) = 0;
	virtual void Draw() = 0;
	virtual void Cleanup() = 0;
};

struct Renderer_Vulkan : public Renderer{
	//instance
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	GLFWwindow* window;
	VkSurfaceKHR surface;
	
	//device
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	
	//image buffers
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	
	//render pipeline
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	
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
	
	std::vector<Vertex> vertices;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	std::vector<uint32> indices;
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
	
	virtual void Init(Window* window) override;
	virtual void Draw() override;
	virtual void Cleanup() override;
	
	
};
