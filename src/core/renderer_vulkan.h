#pragma once
#ifndef DESHI_RENDERER_VULKAN_H
#define DESHI_RENDERER_VULKAN_H

#include "../defines.h"
#include "../utils/optional.h"
#include "../utils/tuple.h"

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
#include <vector>
#include <string>

struct deshiImGui;
struct Scene; struct Mesh; struct Texture;
struct Matrix4; struct Matrix3;
struct Vector4; struct Vector3; struct Vector2; struct Color;
typedef u8 stbi_uc;

enum VSyncTypeBits : u32{
    VSyncType_Immediate, VSyncType_FifoRelaxed, VSyncType_Mailbox, VSyncType_Fifo
}; typedef u32 VSyncType;

struct RenderSettings{ //loaded from file
	VSyncType vsync  = VSyncType_Immediate;
	u32 msaa_samples = 0;
    
    //debug settings that require restart
    b32 debugging = true;
    b32 printf    = true;
	
    //debug settings that can change at runtime
    u8  selectedR = 204, selectedG = 125, selectedB = 41, selectedA = 255;
    u8  colliderR = 116, colliderG = 186, colliderB = 67, colliderA = 255;
    b32 wireframe     = false;
    b32 wireframeOnly = false;
    b32 globalAxis    = false;
    b32 normals       = false;
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
    RSVK_DESCRIPTORPOOL = 1 << 10,
    RSVK_LAYOUTS        = 1 << 11,
    RSVK_PIPELINESETUP  = 1 << 12,
    RSVK_PIPELINECREATE = 1 << 13,
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
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 color; //between 0 and 1
    glm::vec3 normal;
    
    bool operator==(const VertexVk& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
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
    glm::mat4 modelMatrix = glm::mat4(1.f);
    std::vector<PrimitiveVk> primitives;
    std::vector<u32> children;
};

struct MeshBrushVk{
    u32  id = -1;
    char name[DESHI_NAME_SIZE];
    b32 visible = true;
    glm::mat4 matrix = glm::mat4(1.f);
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

struct Renderer{
    ////////////////////////////
    //// renderer variables ////
    ////////////////////////////
    RenderSettings settings;
    RenderStats    stats{};
    RendererStage  rendererStage{};
    
    //this is temporary
    //TODO(sushi, Re) implement SSBOs so we can have a dynamically sized light array
    //and other various dynamically sized things for things and such
    glm::vec4 lights[10]{ glm::vec4(0,0,0,-1) };
    bool generatingWorldGrid = false; //this area is my random var test area now :)
    
    //////////////////////////
    //// render interface ////
    //////////////////////////
    void SaveSettings();
	void LoadSettings();
    //runs the vulkan functions necessary to start rendering
    void Init(deshiImGui* imgui);
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
    void UpdateDebugOptions(bool wireframe, bool globalAxis, bool wireframeOnly);
    
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
    
    s32 width  = 0;
    s32 height = 0;
    VkSwapchainKHR          swapchain = VK_NULL_HANDLE; //ptr
    SwapChainSupportDetails supportDetails{}; //struct
    VkSurfaceFormatKHR      surfaceFormat{}; //struct
    VkPresentModeKHR        presentMode; //flags
    VkExtent2D              extent; //struct
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
    
    void CreateRenderPass();
    
    
    //// frames ////
    
    
    u32 imageCount = 0;
    u32 frameIndex = 0;
    std::vector<FrameVk> frames;
    FramebufferAttachmentsVk attachments{};
    VkSemaphore   imageAcquiredSemaphore  = VK_NULL_HANDLE; //ptr
    VkSemaphore   renderCompleteSemaphore = VK_NULL_HANDLE; //ptr
    VkCommandPool commandPool = VK_NULL_HANDLE; //ptr
    
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
        
        struct{ //TODO(delle,ReOp) size this so its a multiple of 16bytes
            glm::mat4 view;        //camera view matrix
            glm::mat4 proj;        //camera projection matrix
            glm::vec4 lights[10];  //lights
            glm::vec4 viewPos;     //camera pos
            glm::f32  time;         //total time
            glm::f32  width;		   //screen width
            glm::f32  height;	   //screen height
            glm::vec2 mousepos;    //mouse screen pos
            glm::vec3 mouseWorld;  //point casted out from mouse 
        } values;
    } uboVS{};
    struct{ //uniform buffer for the geometry shaders
        VkBuffer               buffer;
        VkDeviceMemory         bufferMemory;
        VkDeviceSize           bufferSize;
        VkDescriptorBufferInfo bufferDescriptor;
		
        struct{
            glm::mat4 view; //camera view matrix
            glm::mat4 proj; //camera projection matrix
        } values;
    } uboGS{};
	
    VkDescriptorSet uboDescriptorSet;
	
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
    void CreateUniformBuffer();
    //updates the uniform buffers sent to shaders
    void UpdateUniformBuffer();
    //creates the vertex and index buffers on the GPU
    void CreateSceneBuffers();
    void UpdateVertexBuffer();
    void UpdateIndexBuffer();
    
	
    //// pipelines setup ////
    
    
    std::array<VkClearValue, 3> clearValues{}; //struct
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
        VkDescriptorSetLayout ubos       = VK_NULL_HANDLE;
        VkDescriptorSetLayout textures   = VK_NULL_HANDLE;
        VkDescriptorSetLayout instances  = VK_NULL_HANDLE;
    } descriptorSetLayouts;
	
    //initializes the color and depth used to clearing a frame
    void CreateClearValues();
    //creates a pool of descriptors of different types to be sent to shaders
    void CreateDescriptorPool();
    //create descriptor set layouts and a push constant for shaders,
    //create pipeline layout, allocate and write to descriptor sets
    void CreateLayouts();
    void CreatePipelineCache();
    void SetupPipelineCreation();
    
    
    //// pipelines creation ////
    
    
    struct { //pipelines
        VkPipeline base;
        VkPipeline flat;
        VkPipeline phong;
        VkPipeline twod;
        VkPipeline pbr;
        VkPipeline lavalamp;
        VkPipeline wireframe;
        VkPipeline wireframe_depth;
        VkPipeline selected;
        VkPipeline collider;
        VkPipeline normals;
        VkPipeline testing0;
        VkPipeline testing1;
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
    
	//give names to objects for debugging
	void DebugNameObjects();
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