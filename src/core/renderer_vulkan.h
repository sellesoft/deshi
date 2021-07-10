#pragma once
#ifndef DESHI_RENDERER_VULKAN_H
#define DESHI_RENDERER_VULKAN_H

#include "../defines.h"
#include "../math/VectorMatrix.h"
#include "../utils/optional.h"
#include "../utils/tuple.h"
#include "../utils/Color.h"

#if defined(_MSC_VER)
#pragma comment(lib,"vulkan-1.lib")
#pragma comment(lib,"glfw3.lib")
#endif
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <array>
#include <vector>
#include <string>

struct DearImGui;
struct Scene; struct Mesh; struct Texture;
typedef u8 stbi_uc;

enum VSyncTypeBits : u32{
    VSyncType_Immediate, VSyncType_FifoRelaxed, VSyncType_Mailbox, VSyncType_Fifo
}; typedef u32 VSyncType;

struct RenderSettings{ //loaded from file
	//// requires restart ////
    b32 debugging = true;
    b32 printf    = false;
	b32 recompileAllShaders = true;
    b32 findMeshTriangleNeighbors = true; //TODO(delle,Cl) move this to a better location
	
	//// runtime changeable ////
	u32 loggingLevel = 1; //if printf is true in the config file, this will be set to 4
	b32 crashOnError = false;
	VSyncType vsync  = VSyncType_Immediate;
	u32 msaaSamples  = 0;
	
	//shaders
	b32 optimizeShaders     = false;
	
	//shadows
	b32 shadowPCF         = false;
	u32 shadowResolution  = 2048;
	f32 shadowNearZ       = 1.f;
	f32 shadowFarZ        = 70.f;
	f32 depthBiasConstant = 1.25f;
	f32 depthBiasSlope    = 1.75f;
    b32 showShadowMap     = false;
	
    //colors
	Vector4 clearColor   {0.02f,0.02f,0.02f,1.00f};
	Vector4 selectedColor{0.80f,0.49f,0.16f,1.00f};
	Vector4 colliderColor{0.46f,0.71f,0.26f,1.00f};
	
	//filters
    b32 wireframeOnly = false;
	
    //overlays
	b32 meshWireframes  = false;
    b32 meshNormals     = false;
    b32 lightFrustrums  = false;
};

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

enum RendererStageBits : u32 { 
    RENDERERSTAGE_NONE  = 0, 
    RSVK_INSTANCE       = 1 << 0,
    RSVK_SURFACE        = 1 << 1,
    RSVK_PHYSICALDEVICE = 1 << 2,
    RSVK_LOGICALDEVICE  = 1 << 3,
    RSVK_SWAPCHAIN      = 1 << 4,
    RSVK_RENDERPASS     = 1 << 5,
    RSVK_COMMANDPOOL    = 1 << 6,
    RSVK_FRAMES         = 1 << 7,
    RSVK_SYNCOBJECTS    = 1 << 8,
    RSVK_UNIFORMBUFFER  = 1 << 9,
    RSVK_LAYOUTS        = 1 << 10,
	RSVK_DESCRIPTORPOOL = 1 << 11,
    RSVK_DESCRIPTORSETS = 1 << 12,
    RSVK_PIPELINESETUP  = 1 << 13,
    RSVK_PIPELINECREATE = 1 << 14,
    RSVK_RENDER      = 0xFFFFFFFF,
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
	vec4 pos;
	vec4 uv;
	vec4 color;
	vec4 normal;
    
    bool operator==(const VertexVk& other) const {
        return pos == other.pos && color == other.color && uv == other.uv && normal == other.normal;
    }
};

struct TextureVk {
    char filename[DESHI_NAME_SIZE];
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
    VkDescriptorImageInfo imageInfo; //just a combo of the previous three vars
};

//a primitive contains the information for one draw call (a batch)
struct PrimitiveVk{
    u32 firstIndex    = 0;
    u32 indexCount    = 0;
    u32 materialIndex = 0;
};

struct MeshVk{
    u32 id      = -1;
    b32 visible = true;
    b32 base    = false;
    Mesh* ptr   = nullptr;
    char name[DESHI_NAME_SIZE];
    mat4 modelMatrix = mat4::IDENTITY;
    std::vector<PrimitiveVk> primitives;
    std::vector<u32> children;
};

struct MeshBrushVk{
    u32  id = -1;
    char name[DESHI_NAME_SIZE];
    b32 visible = true;
    mat4 modelMatrix = mat4::IDENTITY;
    std::vector<VertexVk> vertices;
    std::vector<u32>      indices;
    VkBuffer       vertexBuffer       = 0;
    VkDeviceMemory vertexBufferMemory = 0;
    VkDeviceSize   vertexBufferSize   = 0;
    VkBuffer       indexBuffer        = 0;
    VkDeviceMemory indexBufferMemory  = 0;
    VkDeviceSize   indexBufferSize    = 0;
};

struct MaterialVk{
    u32 id         = -1;
    u32 shader     = 0;
    u32 albedoID   = 0;
    u32 normalID   = 2;
    u32 specularID = 2;
    u32 lightID    = 2;
    char name[DESHI_NAME_SIZE];
    
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

namespace Render{
	
	RenderSettings* GetSettings();
	
}; //namespace Render

struct Renderer{
    ////////////////////////////
    //// renderer variables ////
    ////////////////////////////
    RenderStats    stats{};
    RendererStage  rendererStage{};
    
    //this is temporary
    //TODO(sushi, Re) implement SSBOs so we can have a dynamically sized light array
    //and other various dynamically sized things for things and such
    vec4 lights[10]{ vec4(0,0,0,-1) };
    bool generatingWorldGrid = false; //this area is my random var test area now :)
    
    //////////////////////////
    //// render interface ////
    //////////////////////////
    void SaveSettings();
	void LoadSettings();
    //runs the vulkan functions necessary to start rendering
    void Init(DearImGui* imgui);
    //acquires the next image from vulkan, resets the command buffers, 
    //updates uploaded information, begins the command buffers, begins the render pass, 
    //runs the different shader draw methods, ends the render pass
    void Render();
    //clears the scene
    void Reset();
    //saves the pipeline cache to disk
    void Cleanup();
    
    //returns a mesh brush ID
    u32 CreateDebugLine(Vector3 start, Vector3 end, Color color, bool visible = false);
    void UpdateDebugLine(u32 id, Vector3 start, Vector3 end, Color color);
    //returns a mesh brush ID
    u32 CreateDebugTriangle(Vector3 v1, Vector3 v2, Vector3 v3, Color color, bool visible = false);
    //creates a mesh with editable vertices that requires getting its buffers updated
    u32 CreateMeshBrush(Mesh* m, Matrix4 matrix, b32 log_creation = false);
    void UpdateMeshBrushMatrix(u32 meshID, Matrix4 transform);
    void UpdateMeshBrushBuffers(u32 meshBrushIdx);
    void RemoveMeshBrush(u32 meshBrushIdx);
	
    //2d primitives where points are defined from -1 to 1, where -1 is the bottom/left and 1 is the top/right of the screen
    u32 CreateTwod(std::vector<Vector2> points);
    u32 CreateTwod(std::vector<Vector2> points, Vector2 position, Vector2 scale);
    u32 CreateTwod(std::vector<Vector2> points, Vector2 position, Vector2 scale, Color color, float rotation);
    //position: center of the image
    u32 CreateImage(Texture texture, Vector2 position);
    //removes the twod with twodID from the 2d shader's vertex buffer
    void RemoveTwod(u32 twodID);
    void UpdateTwodColor(u32 twodID, Color color);
    void UpdateTwodPosition(u32 twodID, Vector2 position);
    void UpdateTwodRotation(u32 twodID, float rotation);
    void UpdateTwodVisibility(u32 twodID, b32 visible);
    
    //loads a mesh to the different shaders specified in its batches
    //returns the ID of the mesh
    u32 LoadBaseMesh(Mesh* m, bool visible = false);
    u32 GetBaseMeshID(const char* name);
    u32 CreateMesh(Scene* scene, const char* filename, b32 new_material = false);
    u32 CreateMesh(Mesh* mesh, Matrix4 matrix, b32 new_material = false);
    u32 CreateMesh(u32 meshID, Matrix4 matrix, b32 new_material = false);
    void UnloadBaseMesh(u32 meshID);
    void RemoveMesh(u32 meshID);
    Matrix4 GetMeshMatrix(u32 meshID);
    Mesh* GetMeshPtr(u32 meshID);
    //updates a mesh's model matrix: translation, rotation, scale
    void UpdateMeshMatrix(u32 meshID, Matrix4 matrix);
    void TransformMeshMatrix(u32 meshID, Matrix4 transform);
    void UpdateMeshBatchMaterial(u32 meshID, u32 batchIndex, u32 matID);
    void UpdateMeshVisibility(u32 meshID, bool visible);
    void UpdateMeshBrushVisibility(u32 meshID, bool visible);
    void AddSelectedMesh(u32 meshID);
    //passing -1 will remove all
    void RemoveSelectedMesh(u32 meshID);
    
    u32  MakeInstance(u32 meshID, Matrix4 matrix);
    void RemoveInstance(u32 instanceID);
    //updates an instance's model matrix: translation, rotation, scale
    void UpdateInstanceMatrix(u32 instanceID, Matrix4 matrix);
    void TransformInstanceMatrix(u32 instanceID, Matrix4 transform);
    void UpdateInstanceVisibility(u32 instanceID, bool visible);
    
    //loads a texture onto the GPU
    //returns the texture's id
    u32 LoadTexture(const char* filename, u32 type);
    u32 LoadTexture(Texture texure);
    //unloads a texture from the GPU
    //NOTE the previously used texture ID will not be used again
    void UnloadTexture(u32 textureID);
    std::string ListTextures();
    
    u32 CreateMaterial(const char* name, u32 shader, u32 albedoTextureID = 0, u32 normalTextureID = 2, u32 specTextureID = 2, u32 lightTextureID = 2);
    u32  CopyMaterial(u32 materialID);
    void UpdateMaterialTexture(u32 matID, u32 textureType, u32 textureID);
    void UpdateMaterialShader(u32 matID, u32 shader);
    std::vector<u32> GetMaterialIDs(u32 MeshID);
    void RemoveMaterial(u32 materialID);
    
    void LoadDefaultAssets();
    //loads a new scene to the GPU
    //NOTE this should not be done often in gameplay
    void LoadScene(Scene* scene);
    
    void UpdateCameraPosition(Vector3 position);
    void UpdateCameraViewMatrix(Matrix4 m);
    void UpdateCameraProjectionMatrix(Matrix4 m);
	
    pair<Vector3, Vector3> SceneBoundingBox(); 
    
    //signals vulkan to remake the pipelines
    void ReloadShader(u32 shaderID);
    void ReloadAllShaders();
    void UpdateRenderSettings(RenderSettings settings);
    
    //scene //TODO(delle,ReOp) use container manager for arrays that remove elements
    std::vector<VertexVk>    vertexBuffer = std::vector<VertexVk>(0);
    std::vector<u32>         indexBuffer  = std::vector<u32>(0);
    std::vector<TextureVk>   textures     = std::vector<TextureVk>(0);
    std::vector<MeshVk>      meshes       = std::vector<MeshVk>(0);
    std::vector<MaterialVk>  materials    = std::vector<MaterialVk>(0);
    std::vector<MeshBrushVk> meshBrushes  = std::vector<MeshBrushVk>(0);
    std::vector<u32>         selected     = std::vector<u32>(0);
    
    //////////////////////////////
    //// vulkan api variables ////
    //////////////////////////////
    const int MAX_FRAMES = 2;
    VkAllocationCallbacks* allocator = 0;
    
    bool initialized     = false;
    bool remakeWindow    = false;
    bool remakePipelines = false;
    bool remakeOffscreen = false;
    
    //render settings
    VkSampleCountFlagBits msaaSamples{};
    
    //////////////////////////
    //// vulkan functions ////
    //////////////////////////
    //// instance ////
    
    
    VkInstance instance = VK_NULL_HANDLE;
    
    void CreateInstance();
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    
    
    //// debug messenger ////
    
    
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    
    void SetupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    
    
    //// surface ////
    
    
    VkSurfaceKHR surface;
    
    void CreateSurface();
    
    
    //// device ////
    
    
    VkPhysicalDevice         physicalDevice = VK_NULL_HANDLE;
    VkSampleCountFlagBits    maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPhysicalDeviceFeatures deviceFeatures{};
    VkPhysicalDeviceFeatures enabledFeatures{};
    QueueFamilyIndices       physicalQueueFamilies;
    VkDevice                 device        = VK_NULL_HANDLE;
    VkQueue                  graphicsQueue = VK_NULL_HANDLE;
    VkQueue                  presentQueue  = VK_NULL_HANDLE; 
    VkDeviceSize             bufferMemoryAlignment = 256;
    
    void PickPhysicalDevice();
    //checks whether the graphics card supports swapchains
    bool isDeviceSuitable(VkPhysicalDevice device);
    VkSampleCountFlagBits getMaxUsableSampleCount();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    //creates an interface between the actual GPU device and a  device for interaction
    void CreateLogicalDevice();
    
    
    //// swap chain ////
    
    s32 width  = 0;
    s32 height = 0;
    VkSwapchainKHR          swapchain = VK_NULL_HANDLE;
    SwapChainSupportDetails supportDetails{};
    VkSurfaceFormatKHR      surfaceFormat{};
    VkPresentModeKHR        presentMode;
    VkExtent2D              extent;
    s32                     minImageCount = 0;
    
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
	
	void CreateRenderpass();
	
    
    //// frames ////
    
    
    u32 imageCount = 0;
    u32 frameIndex = 0;
    std::vector<FrameVk> frames;
    FramebufferAttachmentsVk attachments{};
    VkSemaphore   imageAcquiredSemaphore  = VK_NULL_HANDLE;
    VkSemaphore   renderCompleteSemaphore = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    
    //creates a pool for commands
    void CreateCommandPool();
    //creates image views, color/depth resources, framebuffers, commandbuffers
    void CreateFrames();
    //creates semaphores indicating: image acquired, rendering complete
    void CreateSyncObjects();
	
    //// global buffers ////
    
    
    struct{ //uniform buffer for the vertex shaders
        VkBuffer               buffer;
        VkDeviceMemory         bufferMemory;
        VkDeviceSize           bufferSize;
        VkDescriptorBufferInfo bufferDescriptor;
        
        struct{ //size: 101*4=404 bytes
            mat4 view;        //camera view matrix
            mat4 proj;        //camera projection matrix
            vec4 lights[10];  //lights
            vec4 viewPos;     //camera pos
            vec2 screen;      //screen dimensions
            vec2 mousepos;    //mouse screen pos
            vec3 mouseWorld;  //point casted out from mouse 
			f32  time;        //total time
			mat4 lightVP;     //first light's view projection matrix //TODO(delle,ReVu) redo how lights are stored
			b32  enablePCF;   //whether to blur shadow edges //TODO(delle,ReVu) convert to specialization constant
        } values;
    } uboVS{};
    struct{ //uniform buffer for the geometry shaders
        VkBuffer               buffer;
        VkDeviceMemory         bufferMemory;
        VkDeviceSize           bufferSize;
        VkDescriptorBufferInfo bufferDescriptor;
		
        struct{
            mat4 view; //camera view matrix
            mat4 proj; //camera projection matrix
        } values;
    } uboGS{};
	struct{
		VkBuffer               buffer;
        VkDeviceMemory         bufferMemory;
        VkDeviceSize           bufferSize;
        VkDescriptorBufferInfo bufferDescriptor;
		
		struct{
			mat4 lightVP;
		}values;
	} uboVSoffscreen;
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
    
    //creates and allocates the uniform buffer on VertexUBO
    void CreateUniformBuffers();
    //updates the uniform buffers sent to shaders
    void UpdateUniformBuffers();
    //creates the vertex and index buffers on the GPU
    void CreateSceneMeshBuffers();
	
	
	//// offscreen rendering ////
	
	
	//TODO(delle,Vu) distribute these variables around
	struct{
		s32 width, height;
		VkImage               depthImage;
		VkDeviceMemory        depthImageMemory;
		VkImageView           depthImageView;
		VkSampler             depthSampler;
		VkDescriptorImageInfo depthDescriptor;
		VkRenderPass          renderpass;
		VkFramebuffer         framebuffer;
	} offscreen{};
	
	void SetupOffscreenRendering();
    
	
    //// pipelines setup ////
    
    
	struct{ //descriptor set layouts for pipelines
        VkDescriptorSetLayout ubos       = VK_NULL_HANDLE;
        VkDescriptorSetLayout textures   = VK_NULL_HANDLE;
        VkDescriptorSetLayout instances  = VK_NULL_HANDLE;
    } descriptorSetLayouts;
	
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	struct{
		VkDescriptorSet scene;
		VkDescriptorSet offscreen;
		VkDescriptorSet shadowMap_debug;
	} descriptorSets;
	
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipelineCache  pipelineCache  = VK_NULL_HANDLE;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    VkPipelineRasterizationStateCreateInfo rasterizationState{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    VkPipelineColorBlendAttachmentState    colorBlendAttachmentState{};
    VkPipelineColorBlendStateCreateInfo    colorBlendState{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    VkPipelineDepthStencilStateCreateInfo  depthStencilState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    VkPipelineViewportStateCreateInfo      viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    VkPipelineMultisampleStateCreateInfo   multisampleState{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	VkPipelineVertexInputStateCreateInfo   vertexInputState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
	VkPipelineDynamicStateCreateInfo       dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	VkGraphicsPipelineCreateInfo           pipelineCreateInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    std::vector<VkDynamicState>                    dynamicStates;
    std::vector<VkVertexInputBindingDescription>   vertexInputBindings;
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
	//TODO(delle,Vu) rename descriptorSetLayouts.ubo to something more general
	
	
    //creates descriptor set layouts, push constants for shaders, and the pipeline layout
    void CreateLayouts();
	//creates a pool of descriptors of different types to be sent to shaders
    void CreateDescriptorPool();
	//allocates in the descriptor pool and creates the descriptor sets
	void CreateDescriptorSets();
	void CreatePipelineCache();
    void SetupPipelineCreation();
    
    
    //// pipelines creation ////
    
    
    struct { //pipelines
		union{
			VkPipeline array[14];
			struct{
				//game shaders
				VkPipeline flat;
				VkPipeline phong;
				VkPipeline twod;
				VkPipeline pbr;
				VkPipeline lavalamp;
				
				//development shaders
				VkPipeline base;
				VkPipeline wireframe;
				VkPipeline wireframe_depth;
				VkPipeline selected;
				VkPipeline collider;
				VkPipeline testing0;
				VkPipeline testing1;
				VkPipeline offscreen;
				
				//debug shaders
				VkPipeline normals_debug;
				VkPipeline shadowmap_debug;
			};
		};
    } pipelines{};
    
    std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages;
    std::vector<pair<std::string, VkShaderModule>>  shaderModules;
    
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
    void generateMipmaps(VkImage image, VkFormat imageFormat, s32 texWidth, s32 texHeight, u32 mipLevels);
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
    //dynamically sets the viewport and scissor, binds the descriptor set
    void BuildCommandBuffers();
    
};

//global renderer pointer
extern Renderer* g_renderer;
#define DengRenderer g_renderer

#endif //DESHI_RENDERER_VULKAN_H