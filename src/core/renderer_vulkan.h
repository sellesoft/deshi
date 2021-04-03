#pragma once
#ifndef DESHI_RENDERER_VULKAN_H
#define DESHI_RENDERER_VULKAN_H

#include "../utils/defines.h"
#include "../utils/Debug.h"
#include "../utils/optional.h"

#if defined(_MSC_VER)
#pragma comment(lib,"vulkan-1.lib")
#pragma comment(lib,"glfw3.lib")
#endif
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>

struct Window;
struct Time;
struct Mesh;
struct Texture;
struct Scene;
struct Console;
struct Matrix4;
struct Triangle;
struct Vector3;
struct deshiImGui;
typedef u8 stbi_uc;

struct RenderSettings{
	//loaded from file
	bool vsync;
	bool reduceBuffering;
	f32  brightness;
	f32  gamma;
	u32  presetLevel;
	u32  anisotropicFiltering;
	u32  antiAliasing;
	u32  shadowQuality;
	u32  modelQuality;
	u32  textureQuality;
	u32  reflectionQuality;
	u32  lightQuality;
	u32  shaderQuality;
	
	//debug
	bool wireframe     = false;
	bool wireframeOnly = false;
	bool globalAxis    = false;
};

struct RenderStats{
	uint32 totalTriangles;
	uint32 totalVertices;
	uint32 totalIndices;
	uint32 drawnTriangles;
	uint32 drawnIndices;
};

////////////////////////////////
//// vulkan support structs ////
////////////////////////////////

struct QueueFamilyIndices {
	Optional<u32> graphicsFamily;
	Optional<u32> presentFamily;
	bool isComplete() { return graphicsFamily.test() && presentFamily.test(); }
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct VertexVk{
	glm::vec3 pos;
	glm::vec2 texCoord;
	glm::vec3 color; //between 0 and 1
	glm::vec3 normal;
	
	bool operator==(const VertexVk& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
	}
};

struct TextureVk {
	char filename[64];
	u32 id = 0xFFFFFFFF;
	int width, height, channels;
	stbi_uc* pixels;
	u32 mipLevels;
	u32 type;
	
	VkImage        image;
	VkDeviceMemory imageMemory;
	VkDeviceSize   imageSize;
	
	VkImageView   view;
	VkSampler     sampler;
	VkImageLayout layout;
	VkDescriptorImageInfo imageInfo;
};

//a primitive contains the information for one draw call (a batch)
struct PrimitiveVk{
	u32 firstIndex    = 0xFFFFFFFF;
	u32 indexCount    = 0xFFFFFFFF;
	u32 materialIndex = 0xFFFFFFFF;
};

struct MeshVk{
	u32  id = 0xFFFFFFFF;
	bool visible = true;
	glm::mat4 modelMatrix = glm::mat4(1.f);
	std::vector<PrimitiveVk> primitives;
};

struct MaterialVk{
	u32 id     = 0xFFFFFFFF;
	u32 shader = 0;
	u32 albedoTextureIndex   = 0;
	u32 normalTextureIndex   = 2;
	u32 specularTextureIndex = 2;
	u32 lightTextureIndex    = 2;
	
	VkDescriptorSet descriptorSet;
	VkPipeline      pipeline = 0;
};

struct FrameVk{
	VkImage         image         = VK_NULL_HANDLE;
	VkImageView     imageView     = VK_NULL_HANDLE;
	VkFramebuffer   framebuffer   = VK_NULL_HANDLE;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
};

struct SemaphoresVk{
	VkSemaphore imageAcquired  = VK_NULL_HANDLE;
	VkSemaphore renderComplete = VK_NULL_HANDLE;
};

struct FramebufferAttachmentsVk {
	VkImage        colorImage       = VK_NULL_HANDLE;
	VkDeviceMemory colorImageMemory = VK_NULL_HANDLE;
	VkImageView    colorImageView   = VK_NULL_HANDLE;
	VkImage        depthImage       = VK_NULL_HANDLE;
	VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
	VkImageView    depthImageView   = VK_NULL_HANDLE;
};

struct StagingBufferVk {
	VkBuffer buffer;
	VkDeviceMemory memory;
};

struct Renderer{
	////////////////////////////
	//// renderer variables ////
	////////////////////////////
	Time*          time;
	Console*       console;
	GLFWwindow*    window;
	RenderSettings settings; //TODO(delle,Re) load render settings from a file
	RenderStats    stats{};
	Scene* scene;
	
	//////////////////////////
	//// render interface ////
	//////////////////////////
	
	//runs the vulkan functions necessary to start rendering
	void Init(Time* time, Window* window, deshiImGui* imgui, Console* console);
	//acquires the next image from vulkan, resets the command buffers, 
	//updates uploaded information, begins the command buffers, begins the render pass, 
	//runs the different shader draw methods, ends the render pass
	void Render();
	void Present();
	//saves the pipeline cache to disk
	void Cleanup();
	
	//adds a triangle to the 2d shader's vertex and index buffers
	//returns the ID of the triangle
	u32 AddTriangle(Triangle* triangle);
	//removes the triangle with triangleID from the 2d shader's vertex buffer
	void RemoveTriangle(u32 triangleID);
	void UpdateTriangleColor(u32 triangleID, Color color);
	void UpdateTrianglePosition(u32 triangleID, Vector3 position);
	void TranslateTriangle(u32 triangleID, Vector3 translation);
	//adds an array of triangles to the 2d shader's vertex and index buffers
	//returns an array of the triangle's IDs
	std::vector<u32> AddTriangles(std::vector<Triangle*> triangles);
	//removes the triangles with triangleID from the 2d shader's vertex buffer
	void RemoveTriangles(std::vector<u32> triangleIDs);
	void UpdateTrianglesColor(std::vector<u32> triangleIDs, Color color);
	void TranslateTriangles(std::vector<u32> triangleIDs, Vector3 translation);
	
	//loads a mesh to the different shaders specified in its batches
	//returns the ID of the mesh
	u32  LoadMesh(Mesh* mesh);
	u32  DuplicateMesh(u32 meshID, Matrix4 matrix);
	void UnloadMesh(u32 meshID);
	Matrix4 GetMeshMatrix(u32 meshID);
	//updates a mesh's model matrix: translation, rotation, scale
	void UpdateMeshMatrix(u32 meshID, Matrix4 matrix);
	void TransformMeshMatrix(u32 meshID, Matrix4 transform);
	void UpdateMeshBatchMaterial(u32 meshID, u32 batchIndex, u32 matID);
	void UpdateMeshVisibility(u32 meshID, bool visible);
	
	u32  MakeInstance(u32 meshID, Matrix4 matrix);
	void RemoveInstance(u32 instanceID);
	//updates an instance's model matrix: translation, rotation, scale
	void UpdateInstanceMatrix(u32 instanceID, Matrix4 matrix);
	void TransformInstanceMatrix(u32 instanceID, Matrix4 transform);
	void UpdateInstanceVisibility(u32 instanceID, bool visible);
	
	//loads a texture onto the GPU
	//returns the texture's id
	u32 LoadTexture(Texture texure);
	//unloads a texture from the GPU
	//NOTE the previously used texture ID will not be used again
	void UnloadTexture(u32 textureID);
	std::string ListTextures();
	
	u32 CreateMaterial(u32 shader, u32 albedoTextureID = 0, u32 normalTextureID = 2, u32 specTextureID = 2, u32 lightTextureID = 2);
	void UpdateMaterialTexture(u32 matID, u32 textureSlot, u32 textureID);
	void UpdateMaterialShader(u32 matID, u32 shader);
	std::vector<u32> GetMaterialIDs(u32 MeshID);

	void LoadDefaultAssets();
	//loads a new scene to the GPU
	//NOTE this should not be done often in gameplay
	void LoadScene(Scene* scene);
	
	void UpdateCameraPosition(Vector3 position);
	void UpdateCameraViewMatrix(Matrix4 m);
	void UpdateCameraProjectionMatrix(Matrix4 m);
	
	//signals vulkan to remake the pipelines
	void ReloadShader(u32 shaderID);
	void ReloadAllShaders();
	void UpdateDebugOptions(bool wireframe, bool globalAxis);
	
	//scene //TODO(delle,ReOp) use container manager for arrays that remove elements
	std::vector<VertexVk>   vertexBuffer = std::vector<VertexVk>(0);
	std::vector<u32>        indexBuffer  = std::vector<u32>(0);
	std::vector<TextureVk>  textures     = std::vector<TextureVk>(0);
	std::vector<MeshVk>     meshes       = std::vector<MeshVk>(0);
	std::vector<MaterialVk> materials    = std::vector<MaterialVk>(0);
	
	//////////////////////////////
	//// vulkan api variables ////
	//////////////////////////////
	const int MAX_FRAMES = 2;
	
	//vulkan instance
	VkAllocationCallbacks*   allocator = 0;
	VkInstance               instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
	VkSurfaceKHR             surface;
	
	//device
	VkPhysicalDevice physicalDevice   = VK_NULL_HANDLE;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	QueueFamilyIndices physicalQueueFamilies;
	VkDevice     device        = VK_NULL_HANDLE;
	VkQueue      graphicsQueue = VK_NULL_HANDLE;
	VkQueue      presentQueue  = VK_NULL_HANDLE;
	VkDeviceSize bufferMemoryAlignment = 256;
	
	//swapchain
	
	
	//pipeline setup
	VkCommandPool    commandPool    = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkPipelineCache  pipelineCache  = VK_NULL_HANDLE;
	bool clearEnable;
    std::array<VkClearValue, 2> clearValues;
	
	bool initialized     = false;
	bool remakeWindow    = false;
	bool remakePipelines = false;
	
	//swapchain specifics
	i32                    width = 0;
	i32                    height = 0;
	u32                    imageCount = 0;
	VkSwapchainKHR           swapchain = VK_NULL_HANDLE;
	SwapChainSupportDetails  supportDetails;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkPhysicalDeviceFeatures enabledFeatures{};
	VkSurfaceFormatKHR       surfaceFormat;
	VkPresentModeKHR         presentMode;
	VkExtent2D               extent;
	i32                    minImageCount = 0;
	VkRenderPass             renderPass = VK_NULL_HANDLE;
	
	u32                    frameIndex = 0;
	std::vector<FrameVk>      frames;
	std::vector<VkFence>      fencesInFlight;
	std::vector<VkFence>      imagesInFlight;
	std::vector<SemaphoresVk> semaphores;
	FramebufferAttachmentsVk  attachments = {};
	
	struct ShaderData{ //uniform buffer for the shaders
		VkBuffer       uniformBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
		VkDeviceSize   uniformBufferSize   = VK_NULL_HANDLE;
		
		struct Values{ //TODO(delle,ReOp) size this so its a multiple of 16bytes
			glm::mat4 view;     //camera view matrix
			glm::mat4 proj;     //camera projection matrix
			glm::vec4 lightPos; //main light pos
			glm::vec4 viewPos;  //camera pos
			glm::f32 time;
			glm::f32 swidth;
			glm::f32 sheight;
		} values;
	} shaderData;
	VkDescriptorSet sceneDescriptorSet;
	
	VkBuffer        instanceBuffer       = VK_NULL_HANDLE;
	VkDeviceMemory  instanceBufferMemory = VK_NULL_HANDLE;
	VkDeviceSize    instanceBufferSize   = VK_NULL_HANDLE;
	VkDescriptorSet instanceDescriptorSet;
	
	struct{
		VkBuffer       buffer;
		VkDeviceMemory bufferMemory;
		VkDeviceSize   bufferSize;
	} vertices{};
	
	struct{
		VkBuffer       buffer;
		VkDeviceMemory bufferMemory;
		VkDeviceSize   bufferSize;
	} indices{};
	
	struct { //descriptor set layouts for pipelines
		VkDescriptorSetLayout matrices;
		VkDescriptorSetLayout textures;
		VkDescriptorSetLayout instances;
	} descriptorSetLayouts;
	
	//pipelines
	struct {
		VkPipeline FLAT      = VK_NULL_HANDLE;
		VkPipeline PHONG     = VK_NULL_HANDLE;
		VkPipeline TWOD      = VK_NULL_HANDLE;
		VkPipeline PBR       = VK_NULL_HANDLE;
		VkPipeline WIREFRAME = VK_NULL_HANDLE;
		VkPipeline LAVALAMP  = VK_NULL_HANDLE;
		VkPipeline TESTING0  = VK_NULL_HANDLE;
		VkPipeline TESTING1  = VK_NULL_HANDLE;
	} pipelines;
	
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
	VkPipelineRasterizationStateCreateInfo rasterizationState{};
	VkPipelineColorBlendAttachmentState    colorBlendAttachmentState{};
	VkPipelineColorBlendStateCreateInfo    colorBlendState{};
	VkPipelineDepthStencilStateCreateInfo  depthStencilState{};
	VkPipelineViewportStateCreateInfo      viewportState{};
	VkPipelineMultisampleStateCreateInfo   multisampleState{};
	VkPipelineVertexInputStateCreateInfo   vertexInputState{};
	VkPipelineDynamicStateCreateInfo       dynamicState{};
	std::vector<VkDynamicState>                    dynamicStates;
	std::vector<VkVertexInputBindingDescription>   vertexInputBindings;
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
	std::vector<std::pair<char*, VkShaderModule>>  shaderModules;
	
	//////////////////////////
	//// Vulkan Functions ////
	//////////////////////////
	
	void ResizeWindow(int w, int h);
	//destroy old swap chain and in-flight frames, create a new swap chain with new dimensions
	void CreateSwapChain();
	void CreateFrames();
	void CreateRenderPass();
	//updates the uniform buffers sent to shaders
	//https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_layout_and_buffer
	void UpdateUniformBuffer();
	//we define a call order to command buffers so they can be executed by Render()
	//dynamically sets the viewport and scissor, binds the descriptor set,
	//calls scene's draw, then calls imgui's draw
	//NOTE ref: gltfscenerendering.cpp:310
	void BuildCommandBuffers();
	//renders all of the scene's meshes
	void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
	//this controls color formats
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	//this controls vsync/triple buffering
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	//returns the drawable dimensions of the window
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	//creates an image view specifying how to use an image
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 mipLevels);
	//searches the device a format matching the arguments
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	//tries to find a viable depth format
	VkFormat findDepthFormat();
	
	//finds which memory types the graphics card offers
	u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
	//creates and binds a vulkan image to the GPU
	void createImage(u32 width, u32 height, u32 mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, 
					 VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	//creates a buffer of defined usage and size on the device
	void CreateOrResizeBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize& bufferSize, size_t newSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	//creates a buffer and maps provided data to it
	void CreateAndMapBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize& bufferSize, size_t newSize, void* data, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	//returns a command buffer that will only execute once
	VkCommandBuffer beginSingleTimeCommands();
	//ends a command buffer and frees that buffers memory
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	//converts a VkImage from one layout to another using an image memory barrier
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels);
	//uses commands to copy a buffer to an image
	void copyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height);
	//scans an image for max possible mipmaps and generates them
	//https://vulkan-tutorial.com/en/Generating_Mipmaps
	void generateMipmaps(VkImage image, VkFormat imageFormat, i32 texWidth, i32 texHeight, u32 mipLevels);
	//copies a buffer, we use this to copy from CPU to GPU
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	int GetMinImageCountFromPresentMode(VkPresentModeKHR mode);
	
	//creates the vertex, index, and instance buffers on the GPU
	void CreateSceneBuffers();
	void UpdateVertexBuffer();
	void UpdateIndexBuffer();
	
	//// instance ////
	
	void CreateInstance();
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
	
	//// debug messenger ////
	
	void SetupDebugMessenger();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
														VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	
	//// surface ////
	
	//https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Window_surface
	void CreateSurface();
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	
	//// device ////
	
	void PickPhysicalDevice();
	//checks whether the graphics card supports swapchains
	bool isDeviceSuitable(VkPhysicalDevice device);
	VkSampleCountFlagBits getMaxUsableSampleCount();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	//creates an interface between the actual GPU device and a  device for interaction
	void CreateLogicalDevice();
	
	//// frames ////
	
	void DestroyFrame(FrameVk* frame);
	
	//// pipelines setup ////
	
	//initializes the color and depth used to clearing a frame
	void CreateClearValues();
	//creates a pool for command
	void CreateCommandPool();
	//creates semaphores and fences indicating: images are ready, rendering is finished
	//[GPU-GPU sync] semaphores coordinate operations across command buffers so that they execute in a specified order	(pause threads)
	//[CPU-GPU sync] fences are similar but are waited for in the code itself rather than threads						(pause code)
	void CreateSyncObjects();
	//creates a pool of descriptors of different types to be sent to shaders
	void CreateDescriptorPool();
	void CreatePipelineCache();
	//creates and allocates the uniform buffer on ShaderData
	void CreateUniformBuffer();
	//create descriptor set layouts and a push constant for shaders,
	//create pipeline layout, allocate and write to descriptor sets
	void CreateLayouts();
	
	//// pipelines ////
	
	void SetupPipelineCreation();
	void CreatePipelines();
	VkPipeline GetPipelineFromShader(u32 shader);
	void UpdateMaterialPipelines();
	void RemakePipeline(VkPipeline pipeline);
	//creates a pipeline shader stage from the shader bytecode
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);
	VkPipelineShaderStageCreateInfo CompileAndLoadShader(std::string filename, VkShaderStageFlagBits stage, bool optimize = false);
	
	//// shader compiling //// (using shaderc by Google from the VulkanSDK)
	std::vector<std::string> GetUncompiledShaders();
	void CompileAllShaders(bool optimize = false);
	void CompileShader(std::string& filename, bool optimize = false);
	
};

#endif //DESHI_RENDERER_VULKAN_H