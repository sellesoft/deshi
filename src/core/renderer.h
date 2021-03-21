#pragma once
#ifndef DESHI_RENDERER_H
#define DESHI_RENDERER_H

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
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <boost/optional.hpp>

#include <array>

struct Window;
struct Time;
struct Mesh;
struct Texture;
struct Scene;
struct Matrix4;
struct Triangle;
struct Vector3;
struct deshiImGui;
typedef u8 stbi_uc;

enum struct RenderAPI{
	VULKAN
};

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
	bool wireframe  = false;
	bool globalAxis = false;
};

struct RenderStats{
	uint32 totalTriangles;
	uint32 totalVertices;
	uint32 totalIndices;
	uint32 drawnTriangles;
	uint32 drawnIndices;
};

struct Renderer{
	Time*          time;
	GLFWwindow*    window;
	RenderSettings settings; //TODO(r,delle) load render settings from a file
	RenderStats    stats{};
	
	virtual void Init(Window* window, deshiImGui* imgui) = 0;
	virtual void Render() = 0;
	virtual void Present() = 0;
	virtual void Cleanup() = 0;
	
	//2d interface
	virtual u32  AddTriangle(Triangle* triangle) = 0;
	virtual void RemoveTriangle(u32 triangleID) = 0;
	virtual void UpdateTriangleColor(u32 triangleID, Color color) = 0;
	virtual void UpdateTrianglePosition(u32 triangleID, Vector3 position) = 0;
	virtual void TranslateTriangle(u32 triangleID, Vector3 translation) = 0;
	virtual std::vector<u32> AddTriangles(std::vector<Triangle*> triangles) = 0;
	virtual void RemoveTriangles(std::vector<u32> triangleIDs) = 0;
	virtual void UpdateTrianglesColor(std::vector<u32> triangleIDs, Color color) = 0;
	virtual void TranslateTriangles(std::vector<u32> triangleIDs, Vector3 translation) = 0;
	
	//mesh interface
	virtual u32  LoadMesh(Mesh* mesh) = 0;
	virtual void UnloadMesh(u32 meshID) = 0;
	virtual void UpdateMeshMatrix(u32 meshID, Matrix4 matrix) = 0;
	virtual void TransformMeshMatrix(u32 meshID, Matrix4 transform) = 0;
	virtual void UpdateMeshBatchMaterial(u32 meshID, u32 batchIndex, u32 matID) = 0;
	virtual void UpdateMeshVisibility(u32 meshID, bool visible) = 0;
	
	//texture interface
	virtual u32  LoadTexture(Texture texure) = 0;
	virtual void UnloadTexture(u32 textureID) = 0;
	virtual std::string ListTextures() = 0;
	
	//material interface
	virtual u32  CreateMaterial(u32 shader, u32 albedoTextureID, u32 normalTextureID, u32 specTextureID, u32 lightTextureID) = 0;
	virtual void UpdateMaterialTexture(u32 matID, u32 textureType, u32 textureID) = 0;
	virtual void UpdateMaterialShader(u32 matID, u32 shader) = 0;
	
	//scene interface
	virtual void LoadDefaultAssets() = 0;
	virtual void LoadScene(Scene* scene) = 0;
	
	//camera interface
	virtual void UpdateCameraPosition(Vector3 position) = 0;
	virtual void UpdateCameraViewMatrix(Matrix4 m) = 0;
	virtual void UpdateCameraProjectionMatrix(Matrix4 m) = 0;
	
	//other
	virtual void ReloadShaders() = 0;
	virtual void UpdateDebugOptions(bool wireframe, bool globalAxis) = 0;
};

////////////////////////////////
//// vulkan support structs ////
////////////////////////////////
//Vulkan Spec [https://renderdoc.org/vkspec_chunked/index.html]
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

//TODO(cr,delle) remove boost stuff and do it manually 
struct QueueFamilyIndices {
	boost::optional<u32> graphicsFamily;
	boost::optional<u32> presentFamily;
	bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
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
	
	static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
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
	u32       id      = 0xFFFFFFFF;
	bool      visible = true;
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

struct SceneVk{
	std::vector<VertexVk>   vertexBuffer = std::vector<VertexVk>(0);
	std::vector<u32>        indexBuffer  = std::vector<u32>(0);
	std::vector<TextureVk>  textures     = std::vector<TextureVk>(0);
	std::vector<MeshVk>     meshes       = std::vector<MeshVk>(0);
	std::vector<MaterialVk> materials    = std::vector<MaterialVk>(0);
	
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
	
	VkDescriptorSet  descriptorSet;
	
	inline VkDescriptorImageInfo getTextureDescriptorInfo(size_t index);
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

struct Renderer_Vulkan : public Renderer{
	//////////////////////////////
	//// vulkan api variables ////
	//////////////////////////////
	const int MAX_FRAMES = 2;
	
	VkAllocationCallbacks*   allocator = 0;
	VkInstance               instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
	VkSurfaceKHR             surface;
	
	VkPhysicalDevice physicalDevice   = VK_NULL_HANDLE;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	QueueFamilyIndices physicalQueueFamilies;
	
	VkDevice     device        = VK_NULL_HANDLE;
	VkQueue      graphicsQueue = VK_NULL_HANDLE;
	VkQueue      presentQueue  = VK_NULL_HANDLE;
	VkDeviceSize bufferMemoryAlignment = 256;
	
	VkCommandPool    commandPool    = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkPipelineCache  pipelineCache  = VK_NULL_HANDLE;
	
	bool clearEnable;
    std::array<VkClearValue, 2> clearValues;
	
	SceneVk scene;
	
	bool initialized     = false;
	bool remakeWindow    = false;
	bool remakePipelines = false;
	
	//swapchain specifics
	i32                    width = 0;
	i32                    height = 0;
	u32                   imageCount = 0;
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
		
		struct Values{
			glm::mat4 view;     //camera view matrix
			glm::mat4 proj;     //camera projection matrix
			glm::vec4 lightPos; //main light pos
			glm::vec4 viewPos;  //camera pos
			glm::f32 time;
			glm::f32 swidth;
			glm::f32 sheight;
		} values;
	} shaderData;
	
	struct { //descriptor set layouts for pipelines
		VkDescriptorSetLayout matrices;
		VkDescriptorSetLayout textures;
	} descriptorSetLayouts;
	
	struct { //pipelines for the different shaders
		VkPipeline FLAT      = VK_NULL_HANDLE;
		VkPipeline PHONG     = VK_NULL_HANDLE;
		VkPipeline TWOD      = VK_NULL_HANDLE;
		VkPipeline PBR       = VK_NULL_HANDLE;
		VkPipeline WIREFRAME = VK_NULL_HANDLE;
		VkPipeline LAVALAMP  = VK_NULL_HANDLE;
		//NOTE(delle) testing shaders should be removed on release
		VkPipeline TESTING0  = VK_NULL_HANDLE;
		VkPipeline TESTING1  = VK_NULL_HANDLE;
		VkPipeline TESTING2  = VK_NULL_HANDLE;
		VkPipeline TESTING3  = VK_NULL_HANDLE;
	} pipelines;
	
	//list of shader modules created
	std::vector<std::pair<char*, VkShaderModule>> shaderModules;
	
	//////////////////////////
	//// render interface ////
	//////////////////////////
	
	//runs the vulkan functions necessary to start rendering
	void Init(Window* window, deshiImGui* imgui) override;
	//acquires the next image from vulkan, resets the command buffers, 
	//updates uploaded information, begins the command buffers, begins the render pass, 
	//runs the different shader draw methods, ends the render pass
	void Render() override;
	void Present() override;
	//saves the pipeline cache to disk
	void Cleanup() override;
	
	//adds a triangle to the 2d shader's vertex and index buffers
	//returns the ID of the triangle
	u32 AddTriangle(Triangle* triangle) override;
	//removes the triangle with triangleID from the 2d shader's vertex buffer
	void RemoveTriangle(u32 triangleID) override;
	void UpdateTriangleColor(u32 triangleID, Color color) override;
	void UpdateTrianglePosition(u32 triangleID, Vector3 position) override;
	void TranslateTriangle(u32 triangleID, Vector3 translation) override;
	//adds an array of triangles to the 2d shader's vertex and index buffers
	//returns an array of the triangle's IDs
	std::vector<u32> AddTriangles(std::vector<Triangle*> triangles) override;
	//removes the triangles with triangleID from the 2d shader's vertex buffer
	void RemoveTriangles(std::vector<u32> triangleIDs) override;
	void UpdateTrianglesColor(std::vector<u32> triangleIDs, Color color) override;
	void TranslateTriangles(std::vector<u32> triangleIDs, Vector3 translation) override;
	
	//loads a mesh to the different shaders specified in its batches
	//returns the ID of the mesh
	u32 LoadMesh(Mesh* mesh) override;
	void UnloadMesh(u32 meshID) override;
	//updates a mesh's model matrix: translation, rotation, scale
	void UpdateMeshMatrix(u32 meshID, Matrix4 matrix) override;
	void TransformMeshMatrix(u32 meshID, Matrix4 transform) override;
	void UpdateMeshBatchMaterial(u32 meshID, u32 batchIndex, u32 matID) override;
	void UpdateMeshVisibility(u32 meshID, bool visible) override;
	
	//loads a texture onto the GPU
	//returns the texture's id
	u32 LoadTexture(Texture texure) override;
	//unloads a texture from the GPU
	//NOTE the previously used texture ID will not be used again
	void UnloadTexture(u32 textureID) override;
	std::string ListTextures() override;
	
	u32 CreateMaterial(u32 shader, u32 albedoTextureID = 0, u32 normalTextureID = 2, u32 specTextureID = 2, u32 lightTextureID = 2) override;
	void UpdateMaterialTexture(u32 matID, u32 textureSlot, u32 textureID) override;
	void UpdateMaterialShader(u32 matID, u32 shader) override;
	
	void LoadDefaultAssets() override;
	//loads a new scene to the GPU
	//NOTE this should not be done often in gameplay
	void LoadScene(Scene* scene) override;
	
	void UpdateCameraPosition(Vector3 position) override;
	void UpdateCameraViewMatrix(Matrix4 m) override;
	void UpdateCameraProjectionMatrix(Matrix4 m) override;
	
	//signals vulkan to remake the pipelines
	void ReloadShaders() override;
	void UpdateDebugOptions(bool wireframe, bool globalAxis) override;
	
	//////////////////////////////////
	//// initialization functions //// (called once)
	//////////////////////////////////
	
	void CreateInstance();
	void SetupDebugMessenger();
	//https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Window_surface
	void CreateSurface();
	//https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families
	void PickPhysicalDevice();
	//creates an interface between the actual GPU device and a  device for interaction
	//https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Logical_device_and_queues
	void CreateLogicalDevice();
	//initializes the color and depth used to clearing a frame
	void CreateClearValues();
	//creates a pool for command
	//https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Command_buffers
	void CreateCommandPool();
	//creates semaphores and fences indicating: images are ready, rendering is finished
	//[GPU-GPU sync] semaphores coordinate operations across command buffers so that they execute in a specified order	(pause threads)
	//[CPU-GPU sync] fences are similar but are waited for in the code itself rather than threads						(pause code)
	//https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation
	void CreateSyncObjects();
	//creates a pool of descriptors of different types to be sent to shaders
	//https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_pool_and_sets
	void CreateDescriptorPool();
	void CreatePipelineCache();
	//creates and allocates the uniform buffer on ShaderData
	//https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_layout_and_buffer
	void CreateUniformBuffer();
	//create descriptor set layouts and a push constant for shaders,
	//create pipeline layout, allocate and write to descriptor sets
	void CreateLayouts();
	void CreatePipelines();
	
	//////////////////////////////////
	//// window resized functions //// (called whenever reconstruction is necessary)
	//////////////////////////////////
	
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
	
	///////////////////////////
	//// utility functions ////
	///////////////////////////
	
	void DestroyFrame(FrameVk* frame);
	int GetMinImageCountFromPresentMode(VkPresentModeKHR mode);
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
														VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	//checks whether the graphics card supports swapchains
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
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 mipLevels);
	//searches the device a format matching the arguments
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	//tries to find a viable depth format
	VkFormat findDepthFormat();
	//creates a pipeline shader stage from the shader bytecode
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage) ;
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
	VkPipeline GetPipelineFromShader(u32 shader);
	//compiles the shaders in the shader folder using shaderc by Google from the VulkanSDK
	void CompileShaders(bool optimize);
	//creates the vertex and index buffers on the GPU
	void CreateSceneBuffers();
	void UpdateMaterialPipelines();
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};

#endif //DESHI_RENDERER_H