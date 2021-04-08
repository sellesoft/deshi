#pragma once
#ifndef DESHI_RENDERER_VULKAN_H
#define DESHI_RENDERER_VULKAN_H

#include "../utils/defines.h"
#include "../utils/debug.h"
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
	u32  antiAliasing = 0;
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

//TODO(delle,Re) add timer here (cycles and time)
struct RenderStats{
	u32 totalTriangles;
	u32 totalVertices;
	u32 totalIndices;
	u32 drawnTriangles;
	u32 drawnIndices;
	f32 renderTimeMS;
};

////////////////////////////////
//// vulkan support structs ////
////////////////////////////////

//TODO(delle,Re) make this so its bit/flag based
enum RendererStageBits : u32 { 
	RENDERERSTAGE_NONE  = 0, 
	RSVK_INSTANCE       = 1,
	RSVK_SURFACE        = 2,
	RSVK_PHYSICALDEVICE = 4,
	RSVK_LOGICALDEVICE  = 8,
	RSVK_SWAPCHAIN      = 16,
	RSVK_RENDERPASS     = 32,
	RSVK_COMMANDPOOL    = 64,
	RSVK_FRAMES         = 128,
	RSVK_SYNCOBJECTS    = 256,
	RSVK_UNIFORMBUFFER  = 512,
	RSVK_DESCRIPTORPOOL = 1024,
	RSVK_LAYOUTS        = 2048,
	RSVK_PIPELINESETUP  = 4096,
	RSVK_PIPELINECREATE = 8192,
	RSVK_RENDER   = 0xFFFFFFFF,
}; typedef u32 RendererStage;

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
	Mesh* ptr    = nullptr;
	u32  id      = 0xFFFFFFFF;
	std::string name = "";
	bool visible = true;
	bool base    = false;
	glm::mat4 modelMatrix = glm::mat4(1.f);
	std::vector<PrimitiveVk> primitives;
	std::vector<u32> children;
};

struct MaterialVk{
	u32 id     = 0xFFFFFFFF;
	u32 shader = 0;
	u32 albedoID   = 0;
	u32 normalID   = 2;
	u32 specularID = 2;
	u32 lightID    = 2;
	
	VkDescriptorSet descriptorSet;
	VkPipeline      pipeline = 0;
};

struct FrameVk{
	VkImage         image         = VK_NULL_HANDLE;
	VkImageView     imageView     = VK_NULL_HANDLE;
	VkFramebuffer   framebuffer   = VK_NULL_HANDLE;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
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
	GLFWwindow*    window;
	RenderSettings settings;
	RenderStats    stats{};
	RendererStage  rendererStage{};
	
	//////////////////////////
	//// render interface ////
	//////////////////////////
	
	//TODO(delle,Re) load render settings from a file
	void LoadRenderSettings();
	
	//runs the vulkan functions necessary to start rendering
	void Init(Time* time, Window* window, deshiImGui* imgui);
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
	u32  LoadBaseMesh(Mesh* mesh);
	u32  CreateMesh(u32 meshID, Matrix4 matrix);
	void UnloadBaseMesh(u32 meshID);
	void RemoveMesh(u32 meshID);
	Matrix4 GetMeshMatrix(u32 meshID);
	Mesh* GetMeshPtr(u32 meshID);
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
	void UpdateDebugOptions(bool wireframe, bool globalAxis, bool wireframeOnly);
	
	//scene //TODO(delle,ReOp) use container manager for arrays that remove elements
	std::vector<VertexVk>   vertexBuffer = std::vector<VertexVk>(0);
	std::vector<u32>        indexBuffer  = std::vector<u32>(0);
	std::vector<TextureVk>  textures     = std::vector<TextureVk>(0);
	std::vector<MeshVk>     meshes       = std::vector<MeshVk>(0);
	std::vector<MeshVk>     basemeshes   = std::vector<MeshVk>(0);
	std::vector<MaterialVk> materials    = std::vector<MaterialVk>(0);
	
	//////////////////////////////
	//// vulkan api variables ////
	//////////////////////////////
	const int MAX_FRAMES = 2;
	VkAllocationCallbacks* allocator = 0;
	
	bool initialized     = false;
	bool remakeWindow    = false;
	bool remakePipelines = false;
	
	//render settings
	VkSampleCountFlagBits msaaSamples{};
	
	//////////////////////////
	//// vulkan functions ////
	//////////////////////////
	//// instance ////
	
	
	VkInstance instance = VK_NULL_HANDLE; //ptr
	
	void CreateInstance();
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
	
	
	//// debug messenger ////
	
	
	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE; //ptr
	
	void SetupDebugMessenger();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
														VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	
	
	//// surface ////
	
	
	VkSurfaceKHR surface; //ptr or struct; platform dependent
	
	void CreateSurface();
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	
	
	//// device ////
	
	
	VkPhysicalDevice         physicalDevice = VK_NULL_HANDLE; //ptr
	VkSampleCountFlagBits    maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;  //flags
	VkPhysicalDeviceFeatures deviceFeatures{}; //struct
	VkPhysicalDeviceFeatures enabledFeatures{}; //struct
	QueueFamilyIndices       physicalQueueFamilies; //struct
	VkDevice                 device        = VK_NULL_HANDLE; //ptr
	VkQueue                  graphicsQueue = VK_NULL_HANDLE; //ptr
	VkQueue                  presentQueue  = VK_NULL_HANDLE; //ptr
	VkDeviceSize             bufferMemoryAlignment = 256; //u64
	
	void PickPhysicalDevice();
	//checks whether the graphics card supports swapchains
	bool isDeviceSuitable(VkPhysicalDevice device);
	VkSampleCountFlagBits getMaxUsableSampleCount();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	//creates an interface between the actual GPU device and a  device for interaction
	void CreateLogicalDevice();
	
	
	//// swap chain ////
	
	i32 width  = 0;
	i32 height = 0;
	VkSwapchainKHR          swapchain = VK_NULL_HANDLE; //ptr
	SwapChainSupportDetails supportDetails{}; //struct
	VkSurfaceFormatKHR      surfaceFormat{}; //struct
	VkPresentModeKHR        presentMode; //flags
	VkExtent2D              extent; //struct
	i32                     minImageCount = 0;
	
	//destroy old swap chain and in-flight frames, create a new swap chain with new dimensions
	void CreateSwapChain();
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	//this controls color formats
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	//this controls vsync/triple buffering
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	//returns the drawable dimensions of the window
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	int GetMinImageCountFromPresentMode(VkPresentModeKHR mode);
	
	
	//// render pass ////
	
	
	VkRenderPass renderPass = VK_NULL_HANDLE;
	
	void CreateRenderPass();
	void CreateRenderPass2();
	
	
	//// frames ////
	
	
	u32           imageCount = 0;
	u32           frameIndex = 0;
	std::vector<FrameVk> frames;
	FramebufferAttachmentsVk attachments{};
	VkSemaphore   imageAcquiredSemaphore  = VK_NULL_HANDLE; //ptr
	VkSemaphore   renderCompleteSemaphore = VK_NULL_HANDLE; //ptr
	VkCommandPool commandPool = VK_NULL_HANDLE; //ptr
	
	//creates a pool for commands
	void CreateCommandPool();
	//creates image views, color/depth resources, framebuffers, commandbuffers
	void CreateFrames();
	void CreateFrames2();
	//creates semaphores indicating: image acquired, rendering complete
	void CreateSyncObjects();
	
	
	//// global buffers ////
	
	
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
		
		bool freeze = false;
	} shaderData;
	VkDescriptorSet uboDescriptorSet;
	struct{ //instances buffer
		VkBuffer        buffer;
		VkDeviceMemory  bufferMemory;
		VkDeviceSize    bufferSize;
		VkDescriptorSet descriptorSet;
	} instances{};
	struct{ //vertices buffer
		VkBuffer       buffer;
		VkDeviceMemory bufferMemory;
		VkDeviceSize   bufferSize;
	} vertices{};
	struct{ //indices buffer
		VkBuffer       buffer;
		VkDeviceMemory bufferMemory;
		VkDeviceSize   bufferSize;
	} indices{};
	
	//creates and allocates the uniform buffer on ShaderData
	void CreateUniformBuffer();
	//updates the uniform buffers sent to shaders
	void UpdateUniformBuffer();
	//creates the vertex and index buffers on the GPU
	void CreateSceneBuffers();
	void UpdateVertexBuffer();
	void UpdateIndexBuffer();
	
	//// pipelines setup ////
	
	
	std::array<VkClearValue, 3> clearValues; //struct
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE; //ptr
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE; //ptr
	VkPipelineCache  pipelineCache  = VK_NULL_HANDLE; //ptr
	VkGraphicsPipelineCreateInfo           pipelineCreateInfo{}; //struct
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{}; //struct
	VkPipelineRasterizationStateCreateInfo rasterizationState{}; //struct
	VkPipelineColorBlendAttachmentState    colorBlendAttachmentState{}; //struct
	VkPipelineColorBlendStateCreateInfo    colorBlendState{}; //struct
	VkPipelineDepthStencilStateCreateInfo  depthStencilState{}; //struct
	VkPipelineViewportStateCreateInfo      viewportState{}; //struct
	VkPipelineMultisampleStateCreateInfo   multisampleState{}; //struct
	VkPipelineVertexInputStateCreateInfo   vertexInputState{}; //struct
	VkPipelineDynamicStateCreateInfo       dynamicState{}; //struct
	std::vector<VkDynamicState>                    dynamicStates; //struct
	std::vector<VkVertexInputBindingDescription>   vertexInputBindings; //struct
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes; //struct
	struct { //descriptor set layouts for pipelines
		VkDescriptorSetLayout matrices;
		VkDescriptorSetLayout textures;
		VkDescriptorSetLayout iar; //input attachment read
		VkDescriptorSetLayout instances;
	} descriptorSetLayouts;
	
	VkDescriptorSet iarDescriptorSet;
	
	//initializes the color and depth used to clearing a frame
	void CreateClearValues();
	void CreateClearValues2();
	//creates a pool of descriptors of different types to be sent to shaders
	void CreateDescriptorPool();
	//create descriptor set layouts and a push constant for shaders,
	//create pipeline layout, allocate and write to descriptor sets
	void CreateLayouts();
	void CreateLayouts2();
	void CreatePipelineCache();
	void SetupPipelineCreation();
	
	
	//// pipelines creation ////
	
	
	struct { //pipelines
		VkPipeline FLAT      = VK_NULL_HANDLE;
		VkPipeline PHONG     = VK_NULL_HANDLE;
		VkPipeline TWOD      = VK_NULL_HANDLE;
		VkPipeline PBR       = VK_NULL_HANDLE;
		VkPipeline WIREFRAME = VK_NULL_HANDLE;
		VkPipeline LAVALAMP  = VK_NULL_HANDLE;
		VkPipeline TESTING0  = VK_NULL_HANDLE;
		VkPipeline TESTING1  = VK_NULL_HANDLE;
	} pipelines;
	
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
	std::vector<std::pair<char*, VkShaderModule>>  shaderModules;
	
	void CreatePipelines();
	void RemakePipeline(VkPipeline pipeline);
	VkPipeline GetPipelineFromShader(u32 shader);
	void UpdateMaterialPipelines();
	std::vector<std::string> GetUncompiledShaders();
	//creates a pipeline shader stage from the shader bytecode
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);
	VkPipelineShaderStageCreateInfo CompileAndLoadShader(std::string filename, VkShaderStageFlagBits stage, bool optimize = false);
	void CompileAllShaders(bool optimize = false);
	void CompileShader(std::string& filename, bool optimize = false);
	
	
	//// memory/images ////
	
	
	//finds which memory types the graphics card offers
	u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
	//tries to find a viable depth format
	VkFormat findDepthFormat();
	//searches the device a format matching the arguments
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	//creates an image view specifying how to use an image
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 mipLevels);
	//creates and binds a vulkan image to the GPU
	void createImage(u32 width, u32 height, u32 mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, 
					 VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	//converts a VkImage from one layout to another using an image memory barrier
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels);
	//scans an image for max possible mipmaps and generates them
	void generateMipmaps(VkImage image, VkFormat imageFormat, i32 texWidth, i32 texHeight, u32 mipLevels);
	//creates a buffer of defined usage and size on the device
	void CreateOrResizeBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize& bufferSize, size_t newSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	//creates a buffer and maps provided data to it
	void CreateAndMapBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize& bufferSize, size_t newSize, void* data, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	//uses commands to copy a buffer to an image
	void copyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height);
	//copies a buffer, we use this to copy from CPU to GPU
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	
	
	//// other ////
	
	//recreates the swapchain and frames
	void ResizeWindow();
	//returns a command buffer that will only execute once
	VkCommandBuffer beginSingleTimeCommands();
	//ends a command buffer and frees that buffers memory
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	//we define a call order to command buffers so they can be executed by Render()
	//dynamically sets the viewport and scissor, binds the descriptor set,
	void BuildCommandBuffers();
	
};

#endif //DESHI_RENDERER_VULKAN_H