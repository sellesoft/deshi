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
typedef uint8 stbi_uc;

enum struct RenderAPI{
	VULKAN
};

enum struct RenderAlphaMode{
	OPAQUE, BLEND//, CLIP, HASHED
};

struct RenderSettings{
	bool   vsync;
	bool   reduceBuffering;
	float  brightness;
	float  gamma;
	uint32 presetLevel;
	uint32 anisotropicFiltering;
	uint32 antiAliasing;
	uint32 shadowQuality;
	uint32 modelQuality;
	uint32 textureQuality;
	uint32 reflectionQuality;
	uint32 lightQuality;
	uint32 shaderQuality;
};

struct Renderer{
	Time* time;
	GLFWwindow*    window;
	RenderSettings settings; //TODO(r,delle) load render settings from a file
	
	virtual void Init(Window* window, deshiImGui* imgui) = 0;
	virtual void Render() = 0;
	virtual void Present() = 0;
	virtual void Cleanup() = 0;
	
	//2d interface
	virtual uint32 AddTriangle(Triangle* triangle) = 0;
	virtual void   RemoveTriangle(uint32 triangleID) = 0;
	virtual void   UpdateTriangleColor(uint32 triangleID, Color color) = 0;
	virtual void   UpdateTrianglePosition(uint32 triangleID, Vector3 position) = 0;
	virtual void   TranslateTriangle(uint32 triangleID, Vector3 translation) = 0;
	virtual std::vector<uint32> AddTriangles(std::vector<Triangle*> triangles) = 0;
	virtual void   RemoveTriangles(std::vector<uint32> triangleIDs) = 0;
	virtual void   UpdateTrianglesColor(std::vector<uint32> triangleIDs, Color color) = 0;
	virtual void   TranslateTriangles(std::vector<uint32> triangleIDs, Vector3 translation) = 0;
	
	//mesh interface
	virtual uint32 LoadMesh(Mesh* mesh) = 0;
	virtual void   UnloadMesh(uint32 meshID) = 0;
	virtual void   ApplyTextureToMesh(uint32 textureID, uint32 meshID) = 0;
	virtual void   RemoveTextureFromMesh(uint32 textureID, uint32 meshID) = 0;
	virtual void   UpdateMeshMatrix(uint32 meshID, Matrix4 matrix) = 0;
	virtual void   TransformMeshMatrix(uint32 meshID, Matrix4 transform) = 0;
	virtual void   UpdateMeshBatchShader(uint32 meshID, uint32 batchIndex, uint32 shader) = 0;
	
	//texture interface
	virtual uint32 LoadTexture(Texture texure) = 0;
	virtual void   UnloadTexture(uint32 textureID) = 0;
	
	//scene interface
	virtual void   LoadDefaultAssets() = 0;
	virtual void   LoadScene(Scene* scene) = 0;
	
	//camera interface
	virtual void   UpdateCameraPosition(Vector3 position) = 0;
	virtual void   UpdateCameraRotation(Vector3 rotation) = 0;
	virtual void   UpdateCameraViewMatrix(Matrix4 m) = 0;
	virtual void   UpdateCameraProjectionMatrix(Matrix4 m) = 0;
	virtual void   UpdateCameraProjectionProperties(float fovX, float nearZ, float farZ, bool precalcMatrices) = 0;
	
	//other
	virtual void   ReloadShaders() = 0;
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
	boost::optional<uint32> graphicsFamily;
	boost::optional<uint32> presentFamily;
	bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct CameraVk{
	float fovX, nearZ, farZ;
	glm::vec3 position;
	glm::vec3 rotation;
	bool precalcMatrices; 
	uint32 mode = 0;
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
	uint32 id;
	int width, height, channels;
	stbi_uc* pixels;
	uint32 mipLevels;
	uint32 type;
	
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
	uint32 firstIndex;
	uint32 indexCount;
	uint32 materialIndex;
};

struct MeshVk{
	uint32    id;
	bool      visible;
	glm::mat4 modelMatrix;
	std::vector<PrimitiveVk> primitives;
	//maybe add draw function here?
};

struct MaterialVk{
	uint32 id;
	uint32 albedoTextureIndex   = 0;
	uint32 normalTextureIndex   = 2;
	uint32 specularTextureIndex = 2;
	uint32 lightTextureIndex    = 2;
	uint32 shader = 0;
	
	float           alphaThreshold; //A pixel is rendered only if its alpha value is above this threshold
	RenderAlphaMode alphaMode; //Blend Mode for Transparent Faces
	VkDescriptorSet descriptorSet;
	VkPipeline      pipeline = 0;
};

struct SceneVk{
	std::vector<VertexVk>   vertexBuffer = std::vector<VertexVk>(0);
	std::vector<uint32>     indexBuffer  = std::vector<uint32>(0);
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
	
	//renders all of the scene's meshes
	void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
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
	
	bool remakeWindow    = false;
	bool remakePipelines = false;
	
	//swapchain specifics
	int32                    width = 0;
	int32                    height = 0;
	uint32                   imageCount = 0;
	VkSwapchainKHR           swapchain = VK_NULL_HANDLE;
	SwapChainSupportDetails  supportDetails;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkPhysicalDeviceFeatures enabledFeatures{};
	VkSurfaceFormatKHR       surfaceFormat;
	VkPresentModeKHR         presentMode;
	VkExtent2D               extent;
	int32                    minImageCount = 0;
	VkRenderPass             renderPass = VK_NULL_HANDLE;
	
	uint32                    frameIndex = 0;
	std::vector<FrameVk>      frames;
	std::vector<VkFence>      fencesInFlight;
	std::vector<VkFence>      imagesInFlight;
	std::vector<SemaphoresVk> semaphores;
	FramebufferAttachmentsVk  attachments = {};
	
	CameraVk camera;
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
		VkPipeline DEFAULT   = VK_NULL_HANDLE;
		VkPipeline TWOD      = VK_NULL_HANDLE;
		VkPipeline PBR       = VK_NULL_HANDLE;
		VkPipeline WIREFRAME = VK_NULL_HANDLE;
	} pipelines;
	
	//list of shader modules created
	std::vector<std::pair<char*, VkShaderModule>> shaderModules;
	
	//////////////////////////
	//// render interface ////
	//////////////////////////
	
	//runs the vulkan functions necessary to start rendering
	virtual void Init(Window* window, deshiImGui* imgui) override;
	
	//acquires the next image from vulkan, resets the command buffers, 
	//updates uploaded information, begins the command buffers, begins the render pass, 
	//runs the different shader draw methods, ends the render pass
	virtual void Render() override;
	
	//places the swapchain in the presentation queue, iterates the frame index
	virtual void Present() override;
	
	//cleans up memory created in Init
	virtual void Cleanup() override;
	
	//adds a triangle to the 2d shader's vertex and index buffers
	//returns the ID of the triangle
	virtual uint32 AddTriangle(Triangle* triangle) override;
	
	//removes the triangle with triangleID from the 2d shader's vertex buffer
	virtual void RemoveTriangle(uint32 triangleID) override;
	
	virtual void UpdateTriangleColor(uint32 triangleID, Color color) override;
	
	virtual void UpdateTrianglePosition(uint32 triangleID, Vector3 position) override;
	
	virtual void TranslateTriangle(uint32 triangleID, Vector3 translation) override;
	
	//adds an array of triangles to the 2d shader's vertex and index buffers
	//returns an array of the triangle's IDs
	virtual std::vector<uint32> AddTriangles(std::vector<Triangle*> triangles) override;
	
	//removes the triangles with triangleID from the 2d shader's vertex buffer
	virtual void RemoveTriangles(std::vector<uint32> triangleIDs) override;
	
	virtual void UpdateTrianglesColor(std::vector<uint32> triangleIDs, Color color) override;
	
	virtual void TranslateTriangles(std::vector<uint32> triangleIDs, Vector3 translation) override;
	
	//loads a mesh to the different shaders specified in its batches
	//returns the ID of the mesh
	virtual uint32 LoadMesh(Mesh* mesh) override;
	
	virtual void UnloadMesh(uint32 meshID) override;
	
	//loads a texture onto the GPU
	//returns the texture's id
	virtual uint32 LoadTexture(Texture texure) override;
	
	//unloads a texture from the GPU
	//NOTE the previously used texture ID will not be used again
	virtual void UnloadTexture(uint32 textureID) override;
	
	//attempts to apply the texture to the mesh, 
	//replaces the previous texture of the same type
	//NOTE does not unload the previous texture from the GPU
	virtual void ApplyTextureToMesh(uint32 textureID, uint32 meshID) override;
	
	//removes the texture from the mesh
	//NOTE if no textures remain on the mesh, it will use the null texture
	virtual void RemoveTextureFromMesh(uint32 textureID, uint32 meshID) override;
	
	//updates a mesh's model matrix: translation, rotation, scale
	virtual void UpdateMeshMatrix(uint32 meshID, Matrix4 matrix) override;
	virtual void TransformMeshMatrix(uint32 meshID, Matrix4 transform) override;
	
	virtual void UpdateMeshBatchShader(uint32 meshID, uint32 batchIndex, uint32 shader) override;
	
	virtual void LoadDefaultAssets() override;
	
	//loads a new scene to the GPU
	//NOTE this should not be done often in gameplay
	virtual void LoadScene(Scene* scene) override;
	
	virtual void UpdateCameraPosition(Vector3 position) override;
	virtual void UpdateCameraRotation(Vector3 rotation) override;
	virtual void UpdateCameraViewMatrix(Matrix4 m) override;
	virtual void UpdateCameraProjectionMatrix(Matrix4 m) override;
	virtual void UpdateCameraProjectionProperties(float fovX, float nearZ, float farZ, bool precalcMatrices) override;
	
	//signals vulkan to remake the pipelines
	virtual void ReloadShaders() override;
	
	//////////////////////////////////
	//// initialization functions //// (called once)
	//////////////////////////////////
	
	void CreateInstance();
	
	void SetupDebugMessenger();
	
	//https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Window_surface
	void CreateSurface();
	
	//https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families
	void PickPhysicalDevice();
	
	//creates an interface between the actual GPU device and a virtual device for interaction
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
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32 mipLevels);
	
	//searches the device a format matching the arguments
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	
	//tries to find a viable depth format
	VkFormat findDepthFormat();
	
	//creates a pipeline shader stage from the shader bytecode
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage) ;
	
	//finds which memory types the graphics card offers
	uint32 findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties);
	
	//creates and binds a vulkan image to the GPU
	void createImage(uint32 width, uint32 height, uint32 mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, 
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
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32 mipLevels);
	
	//uses commands to copy a buffer to an image
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32 width, uint32 height);
	
	//scans an image for max possible mipmaps and generates them
	//https://vulkan-tutorial.com/en/Generating_Mipmaps
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32 texWidth, int32 texHeight, uint32 mipLevels);
	
	//copies a buffer, we use this to copy from CPU to GPU
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	
	VkPipeline GetPipelineFromShader(uint32 shader);
	
	//compiles the shaders in the shader folder using shaderc by Google from the VulkanSDK
	void CompileShaders(bool optimize);
	
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	
	
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};
