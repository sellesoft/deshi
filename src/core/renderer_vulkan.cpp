/*
Useful or Reading List Links:
https://renderdoc.org/vkspec_chunked/index.html
https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer#page_Using-a-staging-buffer:~:text=You%20may
https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer#page_Conclusion:~:text=It%20should
https://vulkan-tutorial.com/en/Vertex_buffers/Index_buffer#page_Using-an-index-buffer:~:text=The%20previous
https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_layout_and_buffer#page_Updating-uniform-data:~:text=Using%20a%20UBO
https://vulkan-tutorial.com/en/Texture_mapping/Combined_image_sampler#page_Updating-the-descriptors:~:text=determined.-,It%20is
https://vulkan-tutorial.com/en/Generating_Mipmaps#page_Generating-Mipmaps:~:text=Beware%20if%20you
https://vulkan-tutorial.com/en/Generating_Mipmaps#page_Linear-filtering-support:~:text=There%20are%20two
https://vulkan-tutorial.com/en/Multisampling#page_Conclusion:~:text=features%2C-,like
https://vkguide.dev/docs/extra-chapter/abstracting_descriptors/
https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
https://github.com/zeux/volk
http://www.ludicon.com/castano/blog/2009/02/optimal-grid-rendering/
http://gameangst.com/?p=9
*/

#include "renderer.h"
#include "assets.h"
#include "imgui.h"
#include "input.h"
#include "time.h"
#include "window.h"
#include "../scene/Scene.h"
#include "../math/Math.h"
#include "../utils/utils.h"
#include "../utils/optional.h"
#include "../utils/tuple.h"
#include "../utils/Color.h"
#include "../utils/debug.h"

#include "../external/imgui/imgui_impl_glfw.h"
#include "../external/imgui/imgui_impl_vulkan.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb/stb_image.h"

#if defined(_MSC_VER)
#pragma comment(lib,"shaderc_combined.lib")
#endif
#include <shaderc/shaderc.h>

#include <vector>
#include <array>
#include <set>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <fstream>


//-------------------------------------------------------------------------------------------------
// INTERFACE VARIABLES


static_internal RenderSettings settings;
static_internal ConfigMap configMap = {
	{"#render settings config file",0,0},
	{"\n#    //// REQUIRES RESTART ////",  ConfigValueType_PADSECTION,(void*)10},
	{"debugging", ConfigValueType_B32, &settings.debugging},
	{"printf",    ConfigValueType_B32, &settings.printf},
	{"recompile_all_shaders",        ConfigValueType_B32, &settings.recompileAllShaders},
	{"find_mesh_triangle_neighbors", ConfigValueType_B32, &settings.findMeshTriangleNeighbors},
	{"\n#    //// RUNTIME VARIABLES ////", ConfigValueType_PADSECTION,(void*)15},
	{"logging_level",  ConfigValueType_U32, &settings.loggingLevel},
	{"crash_on_error", ConfigValueType_B32, &settings.crashOnError},
	{"vsync_type",     ConfigValueType_U32, &settings.vsync},
	{"msaa_samples",   ConfigValueType_U32, &settings.msaaSamples},
	{"\n#shaders",                         ConfigValueType_PADSECTION,(void*)17},
	{"optimize_shaders", ConfigValueType_B32, &settings.optimizeShaders},
	{"\n#shadows",                         ConfigValueType_PADSECTION,(void*)20},
	{"shadow_pcf",          ConfigValueType_B32, &settings.shadowPCF},
	{"shadow_resolution",   ConfigValueType_U32, &settings.shadowResolution},
	{"shadow_nearz",        ConfigValueType_F32, &settings.shadowNearZ},
	{"shadow_farz",         ConfigValueType_F32, &settings.shadowFarZ},
	{"depth_bias_constant", ConfigValueType_F32, &settings.depthBiasConstant},
	{"depth_bias_slope",    ConfigValueType_F32, &settings.depthBiasSlope},
	{"show_shadow_map",     ConfigValueType_B32, &settings.showShadowMap},
	{"\n#colors",                          ConfigValueType_PADSECTION,(void*)15},
	{"clear_color",    ConfigValueType_FV4, &settings.clearColor},
	{"selected_color", ConfigValueType_FV4, &settings.selectedColor},
	{"collider_color", ConfigValueType_FV4, &settings.colliderColor},
	{"\n#filters",                         ConfigValueType_PADSECTION,(void*)15},
	{"wireframe_only", ConfigValueType_B32, &settings.wireframeOnly},
	{"\n#overlays",                        ConfigValueType_PADSECTION,(void*)16},
	{"mesh_wireframes", ConfigValueType_B32, &settings.meshWireframes},
	{"mesh_normals",    ConfigValueType_B32, &settings.meshNormals},
	{"light_frustrums", ConfigValueType_B32, &settings.lightFrustrums},
};

static_internal RenderStats   stats{};
static_internal RendererStage rendererStage = RENDERERSTAGE_NONE;

//-------------------------------------------------------------------------------------------------
// VULKAN STRUCTS

struct QueueFamilyIndices{
    Optional<u32> graphicsFamily;
    Optional<u32> presentFamily;
    bool isComplete(){ return graphicsFamily.test() && presentFamily.test(); }
};

struct SwapChainSupportDetails{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct FrameVk{
    VkImage         image         = VK_NULL_HANDLE;
    VkImageView     imageView     = VK_NULL_HANDLE;
    VkFramebuffer   framebuffer   = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
};

struct FramebufferAttachmentsVk{
    VkImage        colorImage       = VK_NULL_HANDLE;
    VkDeviceMemory colorImageMemory = VK_NULL_HANDLE;
    VkImageView    colorImageView   = VK_NULL_HANDLE;
    VkImage        depthImage       = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView    depthImageView   = VK_NULL_HANDLE;
};

struct StagingBufferVk{
    VkBuffer buffer;
    VkDeviceMemory memory;
};


//-------------------------------------------------------------------------------------------------
// VULKAN VARIABLES

//TODO(delle,ReOp) use container manager for arrays that remove elements
std::vector<VertexVk>    vertexBuffer = std::vector<VertexVk>(0);
std::vector<u32>         indexBuffer  = std::vector<u32>(0);
std::vector<TextureVk>   textures     = std::vector<TextureVk>(0);
std::vector<MeshVk>      meshes       = std::vector<MeshVk>(0);
std::vector<MaterialVk>  materials    = std::vector<MaterialVk>(0);
std::vector<MeshBrushVk> meshBrushes  = std::vector<MeshBrushVk>(0);
std::vector<u32>         selected     = std::vector<u32>(0);

vec4 lights[10]{ vec4(0,0,0,-1) };

static_internal std::vector<const char*> validationLayers = { 
	"VK_LAYER_KHRONOS_validation" 
};
static_internal std::vector<const char*> deviceExtensions = { 
	VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
	VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME 
};
static_internal std::vector<VkValidationFeatureEnableEXT> validationFeaturesEnabled = {};

//this is temporary
//TODO(sushi, Re) implement SSBOs so we can have a dynamically sized light array
//and other various dynamically sized things for things and such
static_internal bool generatingWorldGrid = false; //this area is my random var test area now :)

static_internal const int MAX_FRAMES = 2;
static_internal VkAllocationCallbacks* allocator = 0;

static_internal bool initialized     = false;
static_internal bool remakeWindow    = false;
static_internal bool remakePipelines = false;
static_internal bool _remakeOffscreen = false;

VkSampleCountFlagBits msaaSamples{};

VkDebugUtilsMessageSeverityFlagsEXT callbackSeverities = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
VkDebugUtilsMessageTypeFlagsEXT     callbackTypes      = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

//// initialization ////
static_internal VkInstance               instance = VK_NULL_HANDLE;
static_internal VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
static_internal VkSurfaceKHR             surface;
static_internal VkPhysicalDevice         physicalDevice = VK_NULL_HANDLE;
static_internal VkSampleCountFlagBits    maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
static_internal VkPhysicalDeviceFeatures deviceFeatures{};
static_internal VkPhysicalDeviceFeatures enabledFeatures{};
static_internal QueueFamilyIndices       physicalQueueFamilies;
static_internal VkDevice                 device        = VK_NULL_HANDLE;
static_internal VkQueue                  graphicsQueue = VK_NULL_HANDLE;
static_internal VkQueue                  presentQueue  = VK_NULL_HANDLE; 
static_internal VkDeviceSize             bufferMemoryAlignment = 256;

//// swapchain ////
static_internal s32                     width  = 0;
static_internal s32                     height = 0;
static_internal VkSwapchainKHR          swapchain = VK_NULL_HANDLE;
static_internal SwapChainSupportDetails supportDetails{};
static_internal VkSurfaceFormatKHR      surfaceFormat{};
static_internal VkPresentModeKHR        presentMode;
static_internal VkExtent2D              extent;
static_internal s32                     minImageCount = 0;

//// renderpass ////
static_internal VkRenderPass renderPass = VK_NULL_HANDLE;

//// frames ////
static_internal u32 imageCount = 0;
static_internal u32 frameIndex = 0;
static_internal std::vector<FrameVk> frames;
static_internal FramebufferAttachmentsVk attachments{};
static_internal VkSemaphore   imageAcquiredSemaphore  = VK_NULL_HANDLE;
static_internal VkSemaphore   renderCompleteSemaphore = VK_NULL_HANDLE;
static_internal VkCommandPool commandPool = VK_NULL_HANDLE;

//// buffers ////
static_internal struct{ //uniform buffer for the vertex shaders
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
static_internal struct{ //uniform buffer for the geometry shaders
	VkBuffer               buffer;
	VkDeviceMemory         bufferMemory;
	VkDeviceSize           bufferSize;
	VkDescriptorBufferInfo bufferDescriptor;
	
	struct{
		mat4 view; //camera view matrix
		mat4 proj; //camera projection matrix
	} values;
} uboGS{};
static_internal struct{
	VkBuffer               buffer;
	VkDeviceMemory         bufferMemory;
	VkDeviceSize           bufferSize;
	VkDescriptorBufferInfo bufferDescriptor;
	
	struct{
		mat4 lightVP;
	} values;
} uboVSoffscreen{};
static_internal struct{ //vertices buffer
	VkBuffer       buffer;
	VkDeviceMemory bufferMemory;
	VkDeviceSize   bufferSize;
} vertices{};
static_internal struct{ //indices buffer
	VkBuffer       buffer;
	VkDeviceMemory bufferMemory;
	VkDeviceSize   bufferSize;
} indices{};

//// offscreen rendering //// 
static_internal struct{ //TODO(delle,Vu) distribute these variables around
	s32 width, height;
	VkImage               depthImage;
	VkDeviceMemory        depthImageMemory;
	VkImageView           depthImageView;
	VkSampler             depthSampler;
	VkDescriptorImageInfo depthDescriptor;
	VkRenderPass          renderpass;
	VkFramebuffer         framebuffer;
} offscreen{};

//// pipelines ////
static_internal struct{ //descriptor set layouts for pipelines
	VkDescriptorSetLayout ubos       = VK_NULL_HANDLE;
	VkDescriptorSetLayout textures   = VK_NULL_HANDLE;
	VkDescriptorSetLayout instances  = VK_NULL_HANDLE;
} descriptorSetLayouts; //TODO(delle,Vu) rename descriptorSetLayouts.ubo to something more general

static_internal VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
static_internal struct{
	VkDescriptorSet scene;
	VkDescriptorSet offscreen;
	VkDescriptorSet shadowMap_debug;
} descriptorSets;

static_internal VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
static_internal VkPipelineCache  pipelineCache  = VK_NULL_HANDLE;
static_internal VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
static_internal VkPipelineRasterizationStateCreateInfo rasterizationState{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
static_internal VkPipelineColorBlendAttachmentState    colorBlendAttachmentState{};
static_internal VkPipelineColorBlendStateCreateInfo    colorBlendState{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
static_internal VkPipelineDepthStencilStateCreateInfo  depthStencilState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
static_internal VkPipelineViewportStateCreateInfo      viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
static_internal VkPipelineMultisampleStateCreateInfo   multisampleState{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
static_internal VkPipelineVertexInputStateCreateInfo   vertexInputState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
static_internal VkPipelineDynamicStateCreateInfo       dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
static_internal VkGraphicsPipelineCreateInfo           pipelineCreateInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
static_internal std::vector<VkDynamicState>                    dynamicStates;
static_internal std::vector<VkVertexInputBindingDescription>   vertexInputBindings;
static_internal std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;

static_internal struct{ //pipelines
	union{
		VkPipeline array[15];
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

static_internal std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages;
static_internal std::vector<pair<std::string, VkShaderModule>> shaderModules;


//-------------------------------------------------------------------------------------------------
// VULKAN FUNCTIONS

////////////////////
//// @utilities ////
////////////////////

#define AssertVk(result, ...) Assert((result) == VK_SUCCESS)
#define AssertRS(stages, ...) Assert((rendererStage & (stages)) == (stages))

template<typename... Args>
static_internal inline void
PrintVk(u32 level, Args... args){
	if(settings.loggingLevel >= level){
		LOG("[Vulkan] ", args...);
	}
}

PFN_vkCmdBeginDebugUtilsLabelEXT func_vkCmdBeginDebugUtilsLabelEXT;
static_internal inline void 
DebugBeginLabelVk(VkCommandBuffer command_buffer, const char* label_name, Vector4 color){
#ifdef DESHI_INTERNAL
	VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
	label.pLabelName = label_name;
	label.color[0]   = color.r;
	label.color[1]   = color.g;
	label.color[2]   = color.b;
	label.color[3]   = color.a;
	func_vkCmdBeginDebugUtilsLabelEXT(command_buffer, &label);
#endif //DESHI_INTERNAL
}

PFN_vkCmdEndDebugUtilsLabelEXT func_vkCmdEndDebugUtilsLabelEXT;
static_internal inline void 
DebugEndLabelVk(VkCommandBuffer command_buffer){
#ifdef DESHI_INTERNAL
	func_vkCmdEndDebugUtilsLabelEXT(command_buffer);
#endif //DESHI_INTERNAL
}

PFN_vkCmdInsertDebugUtilsLabelEXT func_vkCmdInsertDebugUtilsLabelEXT;
static_internal inline  void 
DebugInsertLabelVk(VkCommandBuffer command_buffer, const char* label_name, Vector4 color){
#ifdef DESHI_INTERNAL
	VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
	label.pLabelName = label_name;
	label.color[0]   = color.r;
	label.color[1]   = color.g;
	label.color[2]   = color.b;
	label.color[3]   = color.a;
	func_vkCmdInsertDebugUtilsLabelEXT(command_buffer, &label);
#endif //DESHI_INTERNAL
}

PFN_vkSetDebugUtilsObjectNameEXT func_vkSetDebugUtilsObjectNameEXT;
static_internal inline void 
DebugSetObjectNameVk(VkDevice device, VkObjectType object_type, u64 object_handle, const char *object_name){
#ifdef DESHI_INTERNAL
	if(!object_handle) return;
	VkDebugUtilsObjectNameInfoEXT nameInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
	nameInfo.objectType   = object_type;
	nameInfo.objectHandle = object_handle;
	nameInfo.pObjectName  = object_name;
	func_vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
#endif //DESHI_INTERNAL
}

//returns a command buffer that will only execute once
static_internal VkCommandBuffer
BeginSingleTimeCommands(){
	VkCommandBuffer commandBuffer;
	
	VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool        = commandPool;
	allocInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
	
	VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	
	return commandBuffer;
}

//ends a command buffer and frees that buffers memory
static_internal void
EndSingleTimeCommands(VkCommandBuffer commandBuffer){
	//TODO(delle,ReOpVu) maybe add a fence to ensure the buffer has finished executing
	//instead of waiting for queue to be idle, see: sascha/VulkanDevice.cpp:508
	vkEndCommandBuffer(commandBuffer);
	
	VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &commandBuffer;
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	
	vkQueueWaitIdle(graphicsQueue);
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

static_internal VKAPI_ATTR VkBool32 VKAPI_CALL 
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData){
	switch(messageSeverity){
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:{
			ERROR("[Vulkan] ", pCallbackData->pMessage); 
			PRINTLN(pCallbackData->pMessage << "\n");
			if(settings.crashOnError) Assert(!"crashing because of error in vulkan");
		}break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:{
			WARNING("[Vulkan] ", pCallbackData->pMessage); 
		}break;
		default:{
			PrintVk(4, pCallbackData->pMessage);
		}break;
	}
	return VK_FALSE;
}


////////////////////////
//// @memory/images ////
////////////////////////


//finds which memory types the graphics card offers
static_internal u32
FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties){
	PrintVk(4, "      Finding Memory Types");
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	
	for(u32 i = 0; i < memProperties.memoryTypeCount; i++){
		if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties){
			return i;
		}
	}
	
	Assert(!"failed to find suitable memory type");
	return 0;
}

//creates an image view specifying how to use an image
static_internal VkImageView
CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 mipLevels){
	PrintVk(4, "      Creating Image View");
	VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	viewInfo.image    = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format   = format;
	viewInfo.subresourceRange.aspectMask     = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel   = 0;
	viewInfo.subresourceRange.levelCount     = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount     = 1;
	
	VkImageView imageView{};
	AssertVk(vkCreateImageView(device, &viewInfo, allocator, &imageView), "failed to create texture image view");
	return imageView;
}

//creates and binds a vulkan image to the GPU
static_internal void 
CreateImage(u32 width, u32 height, u32 mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory){
	PrintVk(4, "      Creating Image");
	VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	imageInfo.imageType     = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width  = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth  = 1;
	imageInfo.mipLevels     = mipLevels;
	imageInfo.arrayLayers   = 1;
	imageInfo.format        = format;
	imageInfo.tiling        = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage         = usage;
	imageInfo.samples       = numSamples;
	imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	AssertVk(vkCreateImage(device, &imageInfo, allocator, &image), "failed to create image");
	
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);
	
	VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
	AssertVk(vkAllocateMemory(device, &allocInfo, allocator, &imageMemory), "failed to allocate image memory");
	
	vkBindImageMemory(device, image, imageMemory, 0);
}

//converts a VkImage from one layout to another using an image memory barrier
static_internal void 
TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels){
	PrintVk(4, "      Transitioning Image Layout");
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
	
	VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	barrier.oldLayout           = oldLayout;
	barrier.newLayout           = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = image;
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;
	
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	
	if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		
		sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}else{
		Assert(!"unsupported layout transition");
	}
	
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	EndSingleTimeCommands(commandBuffer);
}

//scans an image for max possible mipmaps and generates them
static_internal void 
GenerateMipmaps(VkImage image, VkFormat imageFormat, s32 texWidth, s32 texHeight, u32 mipLevels){
	PrintVk(4, "      Creating Image Mipmaps");
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
	if(!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)){
		Assert(!"texture image format does not support linear blitting");
	}
	
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
	
	VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = image;
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;
	barrier.subresourceRange.levelCount     = 1;
	
	s32 mipWidth  = texWidth;
	s32 mipHeight = texHeight;
	for(u32 i = 1; i < mipLevels; i++){
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.subresourceRange.baseMipLevel = i - 1;
		
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		
		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel       = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount     = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel       = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount     = 1;
		
		vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
		
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		
		if(mipWidth > 1)  mipWidth /= 2;
		if(mipHeight > 1) mipHeight /= 2;
	}
	
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	
	EndSingleTimeCommands(commandBuffer);
}

//creates a buffer of defined usage and size on the device
static_internal void 
CreateOrResizeBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize& bufferSize, size_t newSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){
	PrintVk(4, "      Creating or Resizing Buffer");
	//delete old buffer
	if(buffer != VK_NULL_HANDLE) vkDestroyBuffer(device, buffer, allocator); 
	if(bufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, bufferMemory, allocator); 
	
	VkDeviceSize alignedBufferSize = ((newSize-1) / bufferMemoryAlignment + 1) * bufferMemoryAlignment;
	VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.size        = alignedBufferSize;
	bufferInfo.usage       = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	AssertVk(vkCreateBuffer(device, &bufferInfo, allocator, &buffer), "failed to create buffer");
	
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(device, buffer, &req);
	bufferMemoryAlignment = (bufferMemoryAlignment > req.alignment) ? bufferMemoryAlignment : req.alignment;
	
	VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.allocationSize  = req.size;
	allocInfo.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, properties);
	
	AssertVk(vkAllocateMemory(device, &allocInfo, allocator, &bufferMemory), "failed to allocate buffer memory");
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
	bufferSize = newSize;
}

//creates a buffer and maps provided data to it
static_internal void 
CreateAndMapBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize& bufferSize, size_t newSize, void* data, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){
	PrintVk(4, "      Creating and Mapping Buffer");
	//delete old buffer
	if(buffer != VK_NULL_HANDLE) vkDestroyBuffer(device, buffer, allocator); 
	if(bufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, bufferMemory, allocator); 
	
	//create buffer
	VkDeviceSize alignedBufferSize = ((newSize-1) / bufferMemoryAlignment + 1) * bufferMemoryAlignment;
	VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.size        = alignedBufferSize;
	bufferInfo.usage       = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	AssertVk(vkCreateBuffer(device, &bufferInfo, allocator, &buffer), "failed to create buffer");
	
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(device, buffer, &req);
	bufferMemoryAlignment = (bufferMemoryAlignment > req.alignment) ? bufferMemoryAlignment : req.alignment;
	
	//allocate buffer
	VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.allocationSize  = req.size;
	allocInfo.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, properties);
	
	AssertVk(vkAllocateMemory(device, &allocInfo, allocator, &bufferMemory), "failed to allocate buffer memory");
	
	//if data pointer, map buffer and copy data
	if(data != nullptr){
		void* mapped = 0;
		AssertVk(vkMapMemory(device, bufferMemory, 0, newSize, 0, &mapped), "couldnt map memory");{
			memcpy(mapped, data, newSize);
			// If host coherency hasn't been requested, do a manual flush to make writes visible
			if((properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0){
				VkMappedMemoryRange mappedRange{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
				mappedRange.memory = bufferMemory;
				mappedRange.offset = 0;
				mappedRange.size   = newSize;
				vkFlushMappedMemoryRanges(device, 1, &mappedRange);
			}
		}vkUnmapMemory(device, bufferMemory);
	}
	
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
	bufferSize = newSize;
}

//uses commands to copy a buffer to an image
static_internal void 
CopyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height){
	PrintVk(4, "      Copying Buffer To Image");
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();{
		VkBufferImageCopy region{};
		region.bufferOffset      = 0;
		region.bufferRowLength   = 0;
		region.bufferImageHeight = 0;
		region.imageOffset       = {0, 0, 0};
		region.imageExtent       = {width, height, 1};
		region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel       = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount     = 1;
		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}EndSingleTimeCommands(commandBuffer);
}

//copies a buffer, we use this to copy from CPU to GPU
static_internal void 
CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size){
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();{
		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	}EndSingleTimeCommands(commandBuffer);
}


///////////////////
//// @instance ////
///////////////////


static_internal void 
CreateInstance(){
	PrintVk(2, "  Creating Vulkan Instance");
	Assert(rendererStage == RENDERERSTAGE_NONE, "renderer stage was not NONE at CreateInstance");
	rendererStage |= RSVK_INSTANCE;
	
	//check for validation layer support
	if(settings.debugging){
		PrintVk(3, "    Checking Validation Layer Support");
		b32 has_support = true;
		
		u32 layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		
		for(const char* layerName : validationLayers){
			bool layerFound = false;
			for(const auto& layerProperties : availableLayers){
				if(strcmp(layerName, layerProperties.layerName) == 0){
					layerFound = true;
					break;
				}
			}
			if(!layerFound) Assert(!"validation layer requested, but not available");
		}
	}
	
	//set instance's application info
	VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	appInfo.pApplicationName   = "deshi";
	appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
	appInfo.pEngineName        = "deshi";
	appInfo.engineVersion      = VK_MAKE_VERSION(1,0,0);
	appInfo.apiVersion         = VK_API_VERSION_1_0;
	
	VkValidationFeaturesEXT validationFeatures{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
	validationFeatures.disabledValidationFeatureCount = 0;
	validationFeatures.pDisabledValidationFeatures    = nullptr;
	validationFeatures.enabledValidationFeatureCount  = validationFeaturesEnabled.size();
	validationFeatures.pEnabledValidationFeatures     = validationFeaturesEnabled.data();
	
	//get required extensions
	PrintVk(3, "    Getting Required Extensions");
	u32 glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if(settings.debugging) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	
	//setup instance debug messenger
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	debugCreateInfo.messageSeverity = callbackSeverities;
	debugCreateInfo.messageType     = callbackTypes;
	debugCreateInfo.pfnUserCallback = DebugCallback;
	
	//create the instance
	VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	createInfo.pApplicationInfo        = &appInfo;
	createInfo.enabledExtensionCount   = (u32)extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
	if(settings.debugging){
		createInfo.enabledLayerCount   = (u32)validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
		debugCreateInfo.pNext          = &validationFeatures;
		createInfo.pNext               = &debugCreateInfo;
	} else {
		createInfo.enabledLayerCount   = 0;
		createInfo.pNext               = nullptr;
	}
	AssertVk(vkCreateInstance(&createInfo, allocator, &instance), "failed to create instance");
}


//////////////////////////
//// @debug messenger ////
//////////////////////////


static_internal void 
SetupDebugMessenger(){
	PrintVk(2, "  Setting Up Debug Messenger");
	AssertRS(RSVK_INSTANCE, "SetupDebugMessenger was called before CreateInstance");
	
	if(!settings.debugging) return;
	
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	debugCreateInfo.messageSeverity = callbackSeverities;
	debugCreateInfo.messageType     = callbackTypes;
	debugCreateInfo.pfnUserCallback = DebugCallback;
	
	VkResult err = VK_ERROR_EXTENSION_NOT_PRESENT;
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if(func != nullptr){
		err = func(instance, &debugCreateInfo, allocator, &debugMessenger);
	}
	AssertVk(err, "failed to setup debug messenger");
}


//////////////////
//// @surface ////
//////////////////


static_internal void 
CreateSurface(){
	PrintVk(2, "  Creating GLFW-Vulkan Surface");
	AssertRS(RSVK_INSTANCE, "CreateSurface called before CreateInstance");
	rendererStage |= RSVK_SURFACE;
	
	AssertVk(glfwCreateWindowSurface(instance, DengWindow->window, allocator, &surface), "failed to create window surface");
}


/////////////////  physical device = actual GPU
//// @device ////  logical  device = interface to the GPU
/////////////////


static_internal void 
PickPhysicalDevice(){
	PrintVk(2, "  Picking Physical Device");
	AssertRS(RSVK_SURFACE, "PickPhysicalDevice called before CreateSurface");
	rendererStage |= RSVK_PHYSICALDEVICE;
	
	u32 deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	
	//test all available GPUs
	for(VkPhysicalDevice device : devices){ 
		{//find device's queue families
			physicalQueueFamilies.graphicsFamily.reset();
			physicalQueueFamilies.presentFamily.reset();

			u32 queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

			for(u32 family_idx = 0; family_idx < queueFamilyCount; ++family_idx){
				if(queueFamilies[family_idx].queueFlags & VK_QUEUE_GRAPHICS_BIT) physicalQueueFamilies.graphicsFamily = family_idx;

				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, family_idx, surface, &presentSupport);
				if(presentSupport) physicalQueueFamilies.presentFamily = family_idx;

				if(physicalQueueFamilies.isComplete()) break;
			}

			if(!physicalQueueFamilies.isComplete()) continue;
		}

		{//check if device supports enabled/required extensions
			u32 extensionCount;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		
			std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
			for(VkExtensionProperties extension : availableExtensions){
				requiredExtensions.erase(extension.extensionName);
			}
			if(!requiredExtensions.empty()) continue;
		}

		{//check if the device's swapchain is valid
			u32 formatCount;
			u32 presentModeCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

			if(formatCount == 0 || presentModeCount == 0) continue;
		}

		physicalDevice = device; 
		break; 
	}
	Assert(physicalDevice != VK_NULL_HANDLE, "failed to find a suitable GPU that supports Vulkan");
	
	//get device's max msaa samples
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if     (counts & VK_SAMPLE_COUNT_64_BIT){ maxMsaaSamples = VK_SAMPLE_COUNT_64_BIT; }
	else if(counts & VK_SAMPLE_COUNT_32_BIT){ maxMsaaSamples = VK_SAMPLE_COUNT_32_BIT; }
	else if(counts & VK_SAMPLE_COUNT_16_BIT){ maxMsaaSamples = VK_SAMPLE_COUNT_16_BIT; }
	else if(counts & VK_SAMPLE_COUNT_8_BIT)  { maxMsaaSamples = VK_SAMPLE_COUNT_8_BIT;  }
	else if(counts & VK_SAMPLE_COUNT_4_BIT)  { maxMsaaSamples = VK_SAMPLE_COUNT_4_BIT;  }
	else if(counts & VK_SAMPLE_COUNT_2_BIT)  { maxMsaaSamples = VK_SAMPLE_COUNT_2_BIT;  }
	else                                     { maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;  }

	//get physical device capabilities
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
}

static_internal void 
CreateLogicalDevice(){
	PrintVk(2, "  Creating Logical Device");
	AssertRS(RSVK_PHYSICALDEVICE, "CreateLogicalDevice called before PickPhysicalDevice");
	rendererStage |= RSVK_LOGICALDEVICE;
	
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<u32> uniqueQueueFamilies = {
		physicalQueueFamilies.graphicsFamily.value, physicalQueueFamilies.presentFamily.value
	};
	
	float queuePriority = 1.0f;
	//queueCreateInfos.reserve(uniqueQueueFamilies.size());
	for(int queueFamily : uniqueQueueFamilies){
		VkDeviceQueueCreateInfo queueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
		queueCreateInfo.queueFamilyIndex = physicalQueueFamilies.graphicsFamily.value;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	//TODO(delle,ReVu) add render settings here
	//enable possible features
	if(deviceFeatures.samplerAnisotropy){
		enabledFeatures.samplerAnisotropy = VK_TRUE; //anistropic filtering
		enabledFeatures.sampleRateShading = VK_TRUE; //sample shading
	}
	if(deviceFeatures.fillModeNonSolid){
		enabledFeatures.fillModeNonSolid = VK_TRUE; //wireframe
		if(deviceFeatures.wideLines){
			enabledFeatures.wideLines = VK_TRUE; //wide lines (anime/toon style)
		}
	}

	//enable debugging features
	if(settings.debugging){
		if(deviceFeatures.geometryShader){
			enabledFeatures.geometryShader = VK_TRUE;
		}
	}
	
	VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	createInfo.pQueueCreateInfos       = queueCreateInfos.data();
	createInfo.queueCreateInfoCount    = static_cast<u32>(queueCreateInfos.size());
	createInfo.pEnabledFeatures        = &enabledFeatures;
	createInfo.enabledExtensionCount   = static_cast<u32>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	if(settings.debugging){
		createInfo.enabledLayerCount   = static_cast<u32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}else{
		createInfo.enabledLayerCount   = 0;
	}
	
	AssertVk(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), "failed to create logical device");
	
	vkGetDeviceQueue(device, physicalQueueFamilies.graphicsFamily.value, 0, &graphicsQueue);
	vkGetDeviceQueue(device, physicalQueueFamilies.presentFamily.value, 0, &presentQueue);
}


/////////////////////
//// @swap chain ////
/////////////////////


//destroy old swap chain and in-flight frames, create a new swap chain with new dimensions
static_internal void 
CreateSwapChain(){
	PrintVk(2, "  Creating Swapchain");
	AssertRS(RSVK_LOGICALDEVICE, "CreateSwapChain called before CreateLogicalDevice");
	rendererStage |= RSVK_SWAPCHAIN;
	
	VkSwapchainKHR oldSwapChain = swapchain;
	swapchain = VK_NULL_HANDLE;
	vkDeviceWaitIdle(device);
	
	//update width and height
	glfwGetFramebufferSize(DengWindow->window, &width, &height);
	
	{//check GPU's features/capabilities for the new swapchain
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &supportDetails.capabilities);

		u32 formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
		if(formatCount != 0){
			supportDetails.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, supportDetails.formats.data());
		}

		u32 presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
		if(presentModeCount != 0){
			supportDetails.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, supportDetails.presentModes.data());
		}
	}

	{//choose swapchain's surface format
		surfaceFormat = supportDetails.formats[0];
		for(VkSurfaceFormatKHR availableFormat : supportDetails.formats){
			if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
				surfaceFormat = availableFormat;
				break;
			}
		}
	}

	{//choose the swapchain's present mode
		//TODO(delle,ReVu) add render settings here (vsync)
		b32 immediate    = false;
		b32 fifo_relaxed = false;
		b32 mailbox      = false;

		for(VkPresentModeKHR availablePresentMode : supportDetails.presentModes){
			if(availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)    immediate    = true;
			if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)      mailbox      = true;
			if(availablePresentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR) fifo_relaxed = true;
		}

		//NOTE immediate is forced false b/c ImGui requires minImageCount to be at least 2
		if      (immediate && false){
			settings.vsync = VSyncType_Immediate;
			presentMode    = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}else if(mailbox){
			settings.vsync = VSyncType_Mailbox;
			presentMode    = VK_PRESENT_MODE_MAILBOX_KHR;
		}else if(fifo_relaxed){
			settings.vsync = VSyncType_FifoRelaxed;
			presentMode    = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
		}else{
			settings.vsync = VSyncType_Fifo;
			presentMode    = VK_PRESENT_MODE_FIFO_KHR;
		}
	}

	//find the actual extent of the swapchain
	if(supportDetails.capabilities.currentExtent.width != UINT32_MAX){
		extent = supportDetails.capabilities.currentExtent;
	}else{
		extent = { (u32)width, (u32)height };
		extent.width  = Max(supportDetails.capabilities.minImageExtent.width,  
							Min(supportDetails.capabilities.maxImageExtent.width,  extent.width));
		extent.height = Max(supportDetails.capabilities.minImageExtent.height, 
							Min(supportDetails.capabilities.maxImageExtent.height, extent.height));
	}
	
	//get min image count if not specified
	if(minImageCount == 0){ //TODO(delle,ReVu) add render settings here (extra buffering)
		switch(presentMode){
			case VK_PRESENT_MODE_MAILBOX_KHR:     { minImageCount = 2;  }break;
			case VK_PRESENT_MODE_FIFO_KHR:        { minImageCount = 2;  }break;
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR:{ minImageCount = 2;  }break;
			case VK_PRESENT_MODE_IMMEDIATE_KHR:   { minImageCount = 1;  }break;
			default:                              { minImageCount = -1; }break;
		}
	}
	
	u32 queueFamilyIndices[2] = {
		physicalQueueFamilies.graphicsFamily.value, physicalQueueFamilies.presentFamily.value
	};
	
	//create swapchain and swap chain images, set width and height
	VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	info.surface                   = surface;
	info.imageFormat               = surfaceFormat.format;
	info.imageColorSpace           = surfaceFormat.colorSpace;
	info.imageArrayLayers          = 1;
	info.imageUsage                = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if(physicalQueueFamilies.graphicsFamily.value != physicalQueueFamilies.presentFamily.value){
		info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices   = queueFamilyIndices;
	}else{
		info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
		info.queueFamilyIndexCount = 0;
		info.pQueueFamilyIndices   = 0;
	}
	info.preTransform              = supportDetails.capabilities.currentTransform;
	info.compositeAlpha            = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode               = presentMode;
	info.clipped                   = VK_TRUE;
	info.oldSwapchain              = oldSwapChain;
	info.minImageCount             = minImageCount;
	if(supportDetails.capabilities.maxImageCount != 0 && info.minImageCount > supportDetails.capabilities.maxImageCount){
		info.minImageCount         = supportDetails.capabilities.maxImageCount;
	}
	if(extent.width == 0xffffffff){
		info.imageExtent.width  = width;
		info.imageExtent.height = height;
	} else {
		info.imageExtent.width  = width  = extent.width;
		info.imageExtent.height = height = extent.height;
	}
	AssertVk(vkCreateSwapchainKHR(device, &info, allocator, &swapchain), "failed to create swap chain");
	
	//delete old swap chain
	if(oldSwapChain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, oldSwapChain, allocator);
}


/////////////////////
//// @renderpass ////
/////////////////////


static_internal VkFormat
findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features){
	PrintVk(4, "      Finding supported image formats");
	for(VkFormat format : candidates){
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
		
		if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features){
			return format;
		}else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features){
			return format;
		}
	}
	
	Assert(!"failed to find supported format");
	return VK_FORMAT_UNDEFINED;
}

static_internal VkFormat
findDepthFormat(){
	return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

static_internal void 
CreateRenderpass(){
	PrintVk(2, "  Creating Render Pass");
	AssertRS(RSVK_LOGICALDEVICE, "CreateRenderPass called before CreateLogicalDevice");
	rendererStage |= RSVK_RENDERPASS;
	
	if(renderPass) vkDestroyRenderPass(device, renderPass, allocator);
	
	VkAttachmentDescription attachments[3]{};
	//attachment 0: color 
	attachments[0].format         = surfaceFormat.format;
	attachments[0].samples        = msaaSamples;
	attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//attachment 0: depth
	attachments[1].format         = findDepthFormat();
	attachments[1].samples        = msaaSamples;
	attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	//attachment 0: color resolve
	attachments[2].format         = surfaceFormat.format;
	attachments[2].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[2].loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[2].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[2].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[2].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment   = 0;
	colorAttachmentRef.layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment   = 1;
	depthAttachmentRef.layout       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkAttachmentReference resolveAttachmentRef{};
	resolveAttachmentRef.attachment = 2;
	resolveAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount    = 1;
	subpass.pColorAttachments       = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments     = &resolveAttachmentRef;
	
	VkSubpassDependency dependencies[2]{};
	dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass      = 0;
	dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	dependencies[1].srcSubpass      = 0;
	dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	
	VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
	renderPassInfo.attachmentCount = 3;
	renderPassInfo.pAttachments    = attachments;
	renderPassInfo.subpassCount    = 1;
	renderPassInfo.pSubpasses      = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies   = dependencies;
	
	AssertVk(vkCreateRenderPass(device, &renderPassInfo, allocator, &renderPass), "failed to create render pass");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_RENDER_PASS, (u64)renderPass, "Base render pass");
}


/////////////////
//// @frames ////
/////////////////


static_internal void 
CreateCommandPool(){
	PrintVk(2, "  Creating Command Pool");
	AssertRS(RSVK_LOGICALDEVICE, "CreateCommandPool called before CreateLogicalDevice");
	rendererStage |= RSVK_COMMANDPOOL;
	
	VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = physicalQueueFamilies.graphicsFamily.value;
	
	AssertVk(vkCreateCommandPool(device, &poolInfo, allocator, &commandPool), "failed to create command pool");
}

//creates image views, color/depth resources, framebuffers, commandbuffers
static_internal void 
CreateFrames(){
	PrintVk(2, "  Creating Frames");
	AssertRS(RSVK_COMMANDPOOL, "CreateFrames called before CreateCommandPool");
	rendererStage |= RSVK_FRAMES;
	
	//get swap chain images
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr); //gets the image count
	Assert(imageCount >= minImageCount, "the window should always have at least the min image count");
	Assert(imageCount < 16, "the window should have less than 16 images, around 2-3 is ideal");
	VkImage images[16] = {};
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images); //assigns to images
	
	{//color framebuffer attachment
		if(attachments.colorImage){
			vkDestroyImageView(device, attachments.colorImageView, nullptr);
			vkDestroyImage(device, attachments.colorImage, nullptr);
			vkFreeMemory(device, attachments.colorImageMemory, nullptr);
		}
		VkFormat colorFormat = surfaceFormat.format;
		CreateImage(width, height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, 
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachments.colorImage, attachments.colorImageMemory);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE, (u64)attachments.colorImage, "Framebuffer color image");
		attachments.colorImageView = CreateImageView(attachments.colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)attachments.colorImageView, "Framebuffer color imageview");
	}

	{//depth framebuffer attachment
		if(attachments.depthImage){
			vkDestroyImageView(device, attachments.depthImageView, nullptr);
			vkDestroyImage(device, attachments.depthImage, nullptr);
			vkFreeMemory(device, attachments.depthImageMemory, nullptr);
		}
		VkFormat depthFormat = findDepthFormat();
		CreateImage(width, height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, 
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachments.depthImage, attachments.depthImageMemory);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE, (u64)attachments.depthImage, "Framebuffer depth image");
		attachments.depthImageView = CreateImageView(attachments.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)attachments.depthImageView, "Framebuffer depth imageview");
	}
	
	frames.resize(imageCount);
	for(u32 i = 0; i < imageCount; ++i){
		//set the frame images to the swap chain images
		//NOTE the previous image and its memory gets freed when the swapchain gets destroyed
		frames[i].image = images[i];
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE, (u64)frames[i].image, TOSTRING("Frame image ", i).c_str());
		
		//create the image views
		if(frames[i].imageView) vkDestroyImageView(device, frames[i].imageView, nullptr);
		frames[i].imageView = CreateImageView(frames[i].image, surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)frames[i].imageView, TOSTRING("Frame imageview ", i).c_str());
		
		//create the framebuffers
		if(frames[i].framebuffer) vkDestroyFramebuffer(device, frames[i].framebuffer, nullptr);
		VkImageView frameBufferAttachments[] = { 
			attachments.colorImageView, attachments.depthImageView, frames[i].imageView 
		};
		VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		info.renderPass      = renderPass;
		info.attachmentCount = ArrayCount(frameBufferAttachments);
		info.pAttachments    = frameBufferAttachments;
		info.width           = width;
		info.height          = height;
		info.layers          = 1;
		AssertVk(vkCreateFramebuffer(device, &info, allocator, &frames[i].framebuffer), "failed to create framebuffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_FRAMEBUFFER, (u64)frames[i].framebuffer, TOSTRING("Frame framebuffer ", i).c_str());
		
		//allocate command buffers
		if(frames[i].commandBuffer) vkFreeCommandBuffers(device, commandPool, 1, &frames[i].commandBuffer);
		VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
		allocInfo.commandPool        = commandPool;
		allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		AssertVk(vkAllocateCommandBuffers(device, &allocInfo, &frames[i].commandBuffer), "failed to allocate command buffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (u64)frames[i].commandBuffer, TOSTRING("Frame command buffer ", i).c_str());
	}
}

//creates semaphores indicating: image acquired, rendering complete
//semaphores (GPU-GPU) coordinate operations across command buffers so that they execute in a specified order
//fences (CPU-GPU) are similar but are waited for in the code itself rather than threads
static_internal void 
CreateSyncObjects(){
	PrintVk(2, "  Creating Sync Objects");
	AssertRS(RSVK_FRAMES, "CreateSyncObjects called before CreateFrames");
	rendererStage |= RSVK_SYNCOBJECTS;
	
	VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	
	VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	
	if(vkCreateSemaphore(device, &semaphoreInfo, allocator, &imageAcquiredSemaphore) ||
	   vkCreateSemaphore(device, &semaphoreInfo, allocator, &renderCompleteSemaphore)){
		Assert(!"failed to create sync objects");
	}
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SEMAPHORE, (u64)imageAcquiredSemaphore, "Semaphore image acquired");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SEMAPHORE, (u64)renderCompleteSemaphore, "Semaphore render complete");
}


////////////////////////
//// @global buffers ////
////////////////////////


//TODO(delle,ReOpVu) maybe only do one mapping at buffer creation, see: gltfscenerendering.cpp, line:600
static_internal void 
UpdateUniformBuffers(){
	AssertRS(RSVK_UNIFORMBUFFER, "UpdateUniformBuffer called before CreateUniformBuffer");
	//PrintVk(2, "  Updating Uniform Buffer");
	
	{//update offscreen vertex shader ubo
		//calculate light ViewProjection for shadow map based on first light
		uboVSoffscreen.values.lightVP = 
			Math::LookAtMatrix(lights[0].ToVector3(), Vector3::ZERO).Inverse() * 
			Math::PerspectiveProjectionMatrix(settings.shadowResolution, settings.shadowResolution, 90.0f, settings.shadowNearZ, settings.shadowFarZ);
		
		void* data;
		vkMapMemory(device, uboVSoffscreen.bufferMemory, 0, sizeof(uboVSoffscreen.values), 0, &data);{
			memcpy(data, &uboVSoffscreen.values, sizeof(uboVSoffscreen.values));
		}vkUnmapMemory(device, uboVSoffscreen.bufferMemory);
	}

	{//update scene vertex shader ubo
		uboVS.values.time = DengTime->totalTime;
		std::copy(lights, lights+10, uboVS.values.lights);
		uboVS.values.screen = vec2(extent.width, extent.height);
		uboVS.values.mousepos = vec2(DengInput->mousePos.x, DengInput->mousePos.y);
		if(initialized) uboVS.values.mouseWorld = Math::ScreenToWorld(DengInput->mousePos, uboVS.values.proj, uboVS.values.view, DengWindow->dimensions);
		uboVS.values.enablePCF = settings.shadowPCF;
		uboVS.values.lightVP = uboVSoffscreen.values.lightVP;
		
		void* data;
		vkMapMemory(device, uboVS.bufferMemory, 0, sizeof(uboVS.values), 0, &data);{
			memcpy(data, &uboVS.values, sizeof(uboVS.values));
		}vkUnmapMemory(device, uboVS.bufferMemory);
	}

	//update normals geometry shader ubo
	if(settings.debugging && enabledFeatures.geometryShader){
		uboGS.values.view = uboVS.values.view;
		uboGS.values.proj = uboVS.values.proj;
		
		void* data;
		vkMapMemory(device, uboGS.bufferMemory, 0, sizeof(uboGS.values), 0, &data);{
			memcpy(data, &uboGS.values, sizeof(uboGS.values));
		}vkUnmapMemory(device, uboGS.bufferMemory);
	}
}

static_internal void 
CreateUniformBuffers(){
	PrintVk(2, "  Creating uniform buffers");
	AssertRS(RSVK_LOGICALDEVICE, "CreateUniformBuffer called before CreateLogicalDevice");
	rendererStage |= RSVK_UNIFORMBUFFER;
	
	{//create scene vertex shader ubo
		CreateOrResizeBuffer(uboVS.buffer, uboVS.bufferMemory, uboVS.bufferSize, 
							 sizeof(uboVS.values), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		uboVS.bufferDescriptor.buffer = uboVS.buffer;
		uboVS.bufferDescriptor.offset = 0;
		uboVS.bufferDescriptor.range  = sizeof(uboVS.values);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)uboVS.buffer, "Scene vertex shader UBO");
	}

	//create normals geometry shader ubo
	if(settings.debugging && enabledFeatures.geometryShader){
		CreateOrResizeBuffer(uboGS.buffer, uboGS.bufferMemory, uboGS.bufferSize, 
							 sizeof(uboGS.values), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		uboGS.bufferDescriptor.buffer = uboGS.buffer;
		uboGS.bufferDescriptor.offset = 0;
		uboGS.bufferDescriptor.range  = sizeof(uboGS.values);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)uboGS.buffer, "Geometry shader UBO");
	}

	{//create offscreen vertex shader ubo
		CreateOrResizeBuffer(uboVSoffscreen.buffer, uboVSoffscreen.bufferMemory, uboVSoffscreen.bufferSize,
							 sizeof(uboVSoffscreen.values), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		uboVSoffscreen.bufferDescriptor.buffer = uboVSoffscreen.buffer;
		uboVSoffscreen.bufferDescriptor.offset = 0;
		uboVSoffscreen.bufferDescriptor.range  = sizeof(uboVSoffscreen.values);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)uboVSoffscreen.buffer, "Offscreen vertex shader UBO");
	}

	UpdateUniformBuffers();
}

static_internal void 
CreateSceneMeshBuffers(){
	PrintVk(3, "    Creating Scene Buffers");
	StagingBufferVk vertexStaging{}, indexStaging{};
	size_t vertexBufferSize = vertexBuffer.size() * sizeof(VertexVk);
	size_t indexBufferSize  = indexBuffer.size() * sizeof(u32);
	if(vertexBufferSize == 0 || indexBufferSize == 0) return; //early out if empty buffers
	
	//create host visible vertex and index buffers (CPU/RAM)
	CreateAndMapBuffer(vertexStaging.buffer, vertexStaging.memory, vertices.bufferSize, vertexBufferSize, vertexBuffer.data(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	CreateAndMapBuffer(indexStaging.buffer, indexStaging.memory, indices.bufferSize, indexBufferSize, indexBuffer.data(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	//create device local buffers (GPU)
	CreateAndMapBuffer(vertices.buffer, vertices.bufferMemory, vertices.bufferSize, vertexBufferSize, nullptr, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	CreateAndMapBuffer(indices.buffer, indices.bufferMemory, indices.bufferSize, indexBufferSize, nullptr, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	//copy data from staging buffers to device local buffers
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();{
		VkBufferCopy copyRegion{};
		
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(commandBuffer, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);
		
		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(commandBuffer, indexStaging.buffer, indices.buffer, 1, &copyRegion);
	}EndSingleTimeCommands(commandBuffer);
	
	//free staging resources
	vkDestroyBuffer(device, vertexStaging.buffer, nullptr);
	vkFreeMemory(device, vertexStaging.memory, nullptr);
	vkDestroyBuffer(device, indexStaging.buffer, nullptr);
	vkFreeMemory(device, indexStaging.memory, nullptr);
	
	//name buffers for debugging
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)vertices.buffer, "Global vertex buffer");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)indices.buffer, "Global index buffer");
}


/////////////////////////////
//// @offscreen rendering ////
/////////////////////////////


static_internal void 
SetupOffscreenRendering(){
	PrintVk(2, "  Creating offscreen rendering stuffs");
	AssertRS(RSVK_LOGICALDEVICE, "SetupOffscreenRendering called before CreateLogicalDevice");
	rendererStage |= RSVK_RENDERPASS;
	
	//cleanup previous offscreen stuff
	if(offscreen.framebuffer){
		vkDestroyImageView(  device, offscreen.depthImageView,   allocator);
		vkDestroyImage(      device, offscreen.depthImage,       allocator);
		vkFreeMemory(        device, offscreen.depthImageMemory, allocator);
		vkDestroySampler(    device, offscreen.depthSampler,     allocator);
		vkDestroyRenderPass( device, offscreen.renderpass,       allocator);
		vkDestroyFramebuffer(device, offscreen.framebuffer,      allocator);
	}
	
	offscreen.width  = settings.shadowResolution;
	offscreen.height = settings.shadowResolution;
	VkFormat depthFormat = VK_FORMAT_D16_UNORM; //16bits might be enough for a small scene
	
	{//create the depth image and image view to be used in a sampler
		CreateImage(offscreen.width, offscreen.height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					offscreen.depthImage, offscreen.depthImageMemory);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE, (u64)offscreen.depthImage, "Offscreen shadowmap depth image");
		
		offscreen.depthImageView = CreateImageView(offscreen.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)offscreen.depthImageView, "Offscreen shadowmap depth image view");
	}

	{//create the sampler for the depth attachment used in frag shader for shadow mapping
		VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		sampler.magFilter     = VK_FILTER_LINEAR;
		sampler.minFilter     = VK_FILTER_LINEAR;
		sampler.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.mipLodBias    = 0.0f;
		sampler.maxAnisotropy = 1.0f;
		sampler.minLod        = 0.0f;
		sampler.maxLod        = 1.0f;
		sampler.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		AssertVk(vkCreateSampler(device, &sampler, nullptr, &offscreen.depthSampler), "failed to create offscreen depth attachment sampler");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SAMPLER, (u64)offscreen.depthSampler, "Offscreen shadowmap sampler");
	}

	{//create image descriptor for depth attachment
		offscreen.depthDescriptor.sampler = offscreen.depthSampler;
		offscreen.depthDescriptor.imageView = offscreen.depthImageView;
		offscreen.depthDescriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}

	{//create the render pass
		VkAttachmentDescription attachments[1]{};
		attachments[0].format         = depthFormat;
		attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR; //clear depth at beginning of pass
		attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE; //store results so it can be read later
		attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED; //don't care about initial layout
		attachments[0].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; //transition to shader read after pass
		
		VkAttachmentReference depthReference{};
		depthReference.attachment = 0;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; //attachment will be used as depth/stencil during pass
		
		VkSubpassDescription subpasses[1]{};
		subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[0].colorAttachmentCount = 0;
		subpasses[0].pDepthStencilAttachment = &depthReference;
		
		//use subpass dependencies for layout transitions
		VkSubpassDependency dependencies[2]{};
		dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass      = 0;
		dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask   = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		dependencies[1].srcSubpass      = 0;
		dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		
		VkRenderPassCreateInfo createInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
		createInfo.attachmentCount = 1;
		createInfo.pAttachments    = attachments;
		createInfo.subpassCount    = 1;
		createInfo.pSubpasses      = subpasses;
		createInfo.dependencyCount = 2;
		createInfo.pDependencies   = dependencies;
		AssertVk(vkCreateRenderPass(device, &createInfo, allocator, &offscreen.renderpass), "failed to create offscreen render pass");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_RENDER_PASS, (u64)offscreen.renderpass, "Offscreen render pass");
	}

	{//create the framebuffer
		VkFramebufferCreateInfo createInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		createInfo.renderPass      = offscreen.renderpass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments    = &offscreen.depthImageView;
		createInfo.width           = offscreen.width;
		createInfo.height          = offscreen.height;
		createInfo.layers          = 1;
		AssertVk(vkCreateFramebuffer(device, &createInfo, allocator, &offscreen.framebuffer), "failed to create offscreen framebuffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_FRAMEBUFFER, (u64)offscreen.framebuffer, "Offscreen framebuffer");
	}
}


////////////////////////// descriptor pool, layouts, pipeline cache
//// @pipelines setup //// pipeline create info structs
////////////////////////// (rasterizer, depth test, etc)


//creates descriptor set layouts, push constants for shaders, and the pipeline layout
static_internal void 
CreateLayouts(){
	PrintVk(2, "  Creating Layouts");
	AssertRS(RSVK_LOGICALDEVICE, "CreateLayouts called before CreateLogicalDevice");
	rendererStage |= RSVK_LAYOUTS;
	
	VkDescriptorSetLayoutBinding setLayoutBindings[4]{};
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	descriptorSetLayoutCI.pBindings    = setLayoutBindings;
	descriptorSetLayoutCI.bindingCount = 0;
	
	{//create generic descriptor set layout
		//binding 0: vertex shader scene UBO
		setLayoutBindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setLayoutBindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
		setLayoutBindings[0].binding         = 0;
		setLayoutBindings[0].descriptorCount = 1;
		//binding 1: fragment shader shadow map image sampler
		setLayoutBindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[1].binding         = 1;
		setLayoutBindings[1].descriptorCount = 1;
		
		descriptorSetLayoutCI.bindingCount = 2;
		
		//binding 2: geometry shader UBO
		if(settings.debugging && enabledFeatures.geometryShader){
			setLayoutBindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			setLayoutBindings[2].stageFlags      = VK_SHADER_STAGE_GEOMETRY_BIT;
			setLayoutBindings[2].binding         = 2;
			setLayoutBindings[2].descriptorCount = 1;
			
			descriptorSetLayoutCI.bindingCount = 3;
		}
		
		AssertVk(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.ubos), "failed to create ubos descriptor set layout");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)descriptorSetLayouts.ubos, "UBOs descriptor set layout");
	}
	
	{//create textures descriptor set layout
		//binding 0: fragment shader color/albedo map
		setLayoutBindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBindings[0].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[0].binding         = 0;
		setLayoutBindings[0].descriptorCount = 1;
		
		//binding 1: fragment shader normal map
		setLayoutBindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[1].binding         = 1;
		setLayoutBindings[1].descriptorCount = 1;
		
		//binding 2: fragment shader specular/reflective map
		setLayoutBindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBindings[2].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[2].binding         = 2;
		setLayoutBindings[2].descriptorCount = 1;
		
		//binding 3: fragment shader light/emissive map
		setLayoutBindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBindings[3].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[3].binding         = 3;
		setLayoutBindings[3].descriptorCount = 1;
		
		descriptorSetLayoutCI.bindingCount = 4;
		AssertVk(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.textures), "failed to create textures descriptor set layout");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)descriptorSetLayouts.textures, "Textures descriptor set layout");
	}
	
	{//create instances descriptor set layout
		
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)descriptorSetLayouts.instances, "Instances descriptor set layout");
	}
	
	{//create pipeline layout
		//setup push constants for passing model matrix
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(mat4);
		
		VkDescriptorSetLayout setLayouts[] = { 
			descriptorSetLayouts.ubos, descriptorSetLayouts.textures
		};
		
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		pipelineLayoutInfo.setLayoutCount         = ArrayCount(setLayouts);
		pipelineLayoutInfo.pSetLayouts            = setLayouts;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;
		AssertVk(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout), "failed to create pipelineLayout");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)pipelineLayout, "Base pipeline layout");
	}
}

//creates a pool of descriptors of different types to be sent to shaders
//TODO(delle,ReVu) find a better/more accurate way to do this, see gltfloading.cpp, line:592
static_internal void 
CreateDescriptorPool(){
	PrintVk(2, "  Creating Descriptor Pool");
	AssertRS(RSVK_LOGICALDEVICE, "CreateDescriptorPool called before CreateLogicalDevice");
	rendererStage |= RSVK_DESCRIPTORPOOL;
	
	const int types = 11;
	VkDescriptorPoolSize poolSizes[types] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER,                1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000 }
	};
	
	VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets       = 1000 * types;
	poolInfo.poolSizeCount = types;
	poolInfo.pPoolSizes    = poolSizes;
	AssertVk(vkCreateDescriptorPool(device, &poolInfo, allocator, &descriptorPool), "failed to create descriptor pool");
}

//allocates in the descriptor pool and creates the descriptor sets
static_internal void 
CreateDescriptorSets(){
	AssertRS(RSVK_DESCRIPTORPOOL | RSVK_UNIFORMBUFFER, "CreateLayouts called before CreateDescriptorPool or CreateUniformBuffer");
	rendererStage |= RSVK_DESCRIPTORSETS;
	
	VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	allocInfo.descriptorPool     = descriptorPool;
	allocInfo.pSetLayouts        = &descriptorSetLayouts.ubos;
	allocInfo.descriptorSetCount = 1;
	
	VkWriteDescriptorSet writeDescriptorSets[3]{};
	
	{//scene descriptor sets
		AssertVk(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.scene), "failed to allocate scene descriptor sets");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)descriptorSets.scene, "Scene descriptor set");
		
		//binding 0: vertex shader ubo
		writeDescriptorSets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].dstSet          = descriptorSets.scene;
		writeDescriptorSets[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSets[0].dstBinding      = 0;
		writeDescriptorSets[0].pBufferInfo     = &uboVS.bufferDescriptor;
		writeDescriptorSets[0].descriptorCount = 1;
		//binding 1: fragment shader shadow sampler
		writeDescriptorSets[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[1].dstSet          = descriptorSets.scene;
		writeDescriptorSets[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSets[1].dstBinding      = 1;
		writeDescriptorSets[1].pImageInfo      = &offscreen.depthDescriptor;
		writeDescriptorSets[1].descriptorCount = 1;
		
		if(settings.debugging && enabledFeatures.geometryShader){
			//binding 2: geometry shader ubo
			writeDescriptorSets[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[2].dstSet          = descriptorSets.scene;
			writeDescriptorSets[2].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[2].dstBinding      = 2;
			writeDescriptorSets[2].pBufferInfo     = &uboGS.bufferDescriptor;
			writeDescriptorSets[2].descriptorCount = 1;
			
			vkUpdateDescriptorSets(device, 3, writeDescriptorSets, 0, nullptr);
		}else{
			vkUpdateDescriptorSets(device, 2, writeDescriptorSets, 0, nullptr);
		}
	}
	
	{//offscreen shadow map generation descriptor set
		AssertVk(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.offscreen), "failed to allocate scene descriptor sets");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)descriptorSets.offscreen, "Offscreen descriptor set");
		
		//binding 0: vertex shader ubo
		writeDescriptorSets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].dstSet          = descriptorSets.offscreen;
		writeDescriptorSets[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSets[0].dstBinding      = 0;
		writeDescriptorSets[0].pBufferInfo     = &uboVSoffscreen.bufferDescriptor;
		writeDescriptorSets[0].descriptorCount = 1;
		
		vkUpdateDescriptorSets(device, 1, writeDescriptorSets, 0, nullptr);
	}
	
	{//DEBUG show shadow map descriptor set
		AssertVk(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.shadowMap_debug), "failed to allocate shadowMap descriptor sets");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)descriptorSets.shadowMap_debug, "DEBUG Shadowmap descriptor set");
		
		//binding 1: fragment shader shadow sampler
		writeDescriptorSets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].dstSet          = descriptorSets.shadowMap_debug;
		writeDescriptorSets[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSets[0].dstBinding      = 1;
		writeDescriptorSets[0].pImageInfo      = &offscreen.depthDescriptor;
		writeDescriptorSets[0].descriptorCount = 1;
		
		vkUpdateDescriptorSets(device, 1, writeDescriptorSets, 0, nullptr);
	}
}

static_internal void 
CreatePipelineCache(){
	PrintVk(2, "  Creating Pipeline Cache");
	AssertRS(RSVK_LOGICALDEVICE, "CreatePipelineCache called before CreateLogicalDevice");
	
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
	
	//try to read pipeline cache file if exists
	std::string path = Assets::assetPath("pipelines.cache", AssetType_Data, false);
	std::vector<char> data;
	if(path != ""){
		data = Assets::readFileBinary(path);
		pipelineCacheCreateInfo.initialDataSize = data.size();
		pipelineCacheCreateInfo.pInitialData    = data.data();
	}
	
	AssertVk(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache), "failed to create pipeline cache");
}

static_internal void 
SetupPipelineCreation(){
	PrintVk(2, "  Setting up pipeline creation");
	AssertRS(RSVK_LAYOUTS | RSVK_RENDERPASS, "SetupPipelineCreation called before CreateLayouts or CreateRenderPass");
	rendererStage |= RSVK_PIPELINESETUP;
	
	//vertex input flow control
	//https://renderdoc.org/vkspec_chunked/chap23.html#VkPipelineVertexInputStateCreateInfo
	
	//binding:u32, stride:u32, inputRate:VkVertexInputRate
	vertexInputBindings = {
		{0, sizeof(VertexVk), VK_VERTEX_INPUT_RATE_VERTEX},
	};
	
	//location:u32, binding:u32, format:VkFormat, offset:u32
	vertexInputAttributes = {
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexVk, pos)},
		{1, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(VertexVk, uv)},
		{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexVk, color)},
		{3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexVk, normal)},
	};
	
	vertexInputState.pNext                           = 0;
	vertexInputState.flags                           = 0;
	vertexInputState.vertexBindingDescriptionCount   = (u32)vertexInputBindings.size();
	vertexInputState.pVertexBindingDescriptions      = vertexInputBindings.data();
	vertexInputState.vertexAttributeDescriptionCount = (u32)vertexInputAttributes.size();
	vertexInputState.pVertexAttributeDescriptions    = vertexInputAttributes.data();
	
	//determines how to group vertices together
	//https://renderdoc.org/vkspec_chunked/chap22.html#VkPipelineInputAssemblyStateCreateInfo
	inputAssemblyState.pNext                  = 0;
	inputAssemblyState.flags                  = 0;
	inputAssemblyState.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; 
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;
	
	//container for viewports and scissors
	//https://renderdoc.org/vkspec_chunked/chap27.html#VkPipelineViewportStateCreateInfo
	viewportState.pNext         = 0;
	viewportState.flags         = 0;
	viewportState.viewportCount = 1;
	viewportState.pViewports    = 0;
	viewportState.scissorCount  = 1;
	viewportState.pScissors     = 0;
	
	//how to draw/cull/depth things
	//https://renderdoc.org/vkspec_chunked/chap28.html#VkPipelineRasterizationStateCreateInfo
	rasterizationState.pNext                   = 0;
	rasterizationState.flags                   = 0;
	rasterizationState.depthClampEnable        = VK_FALSE; //look into for shadowmapping
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.polygonMode             = VK_POLYGON_MODE_FILL; //draw mode: fill, wireframe, vertices
	rasterizationState.cullMode                = VK_CULL_MODE_BACK_BIT;
	rasterizationState.frontFace               = VK_FRONT_FACE_CLOCKWISE; //VK_FRONT_FACE_COUNTER_CLOCKWISE
	rasterizationState.depthBiasEnable         = VK_FALSE;
	rasterizationState.depthBiasConstantFactor = 0.0f;
	rasterizationState.depthBiasClamp          = 0.0f;
	rasterizationState.depthBiasSlopeFactor    = 0.0f;
	rasterizationState.lineWidth               = 1.0f;
	
	//useful for multisample anti-aliasing (MSAA)
	//https://renderdoc.org/vkspec_chunked/chap28.html#VkPipelineMultisampleStateCreateInfo
	multisampleState.pNext                 = 0;
	multisampleState.flags                 = 0;
	multisampleState.rasterizationSamples  = msaaSamples; //VK_SAMPLE_COUNT_1_BIT to disable anti-aliasing
	multisampleState.sampleShadingEnable   = VK_TRUE; //enable sample shading in the pipeline, VK_FALSE to disable
	multisampleState.minSampleShading      = .2f; //min fraction for sample shading; closer to one is smoother
	multisampleState.pSampleMask           = 0;
	multisampleState.alphaToCoverageEnable = VK_FALSE;
	multisampleState.alphaToOneEnable      = VK_FALSE;
	
	//depth testing and discarding
	//https://renderdoc.org/vkspec_chunked/chap29.html#VkPipelineDepthStencilStateCreateInfo
	depthStencilState.pNext                 = 0;
	depthStencilState.flags                 = 0;
	depthStencilState.depthTestEnable       = VK_TRUE;
	depthStencilState.depthWriteEnable      = VK_TRUE;
	depthStencilState.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable     = VK_FALSE;
	depthStencilState.minDepthBounds        = 0.0f;
	depthStencilState.maxDepthBounds        = 1.0f;
	depthStencilState.stencilTestEnable     = VK_FALSE;
	depthStencilState.front                 = {};
	depthStencilState.back.compareOp        = VK_COMPARE_OP_ALWAYS;
	
	//how to combine colors; alpha: options to allow alpha blending
	//https://renderdoc.org/vkspec_chunked/chap30.html#VkPipelineColorBlendAttachmentState
	colorBlendAttachmentState.blendEnable         = VK_FALSE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.colorBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.colorWriteMask      = 0xF; //RGBA
	
	//container struct for color blend attachments with overall blending constants
	//https://renderdoc.org/vkspec_chunked/chap30.html#VkPipelineColorBlendStateCreateInfo
	colorBlendState.pNext             = 0;
	colorBlendState.flags             = 0;
	colorBlendState.logicOpEnable     = VK_FALSE;
	colorBlendState.logicOp           = VK_LOGIC_OP_COPY; //TODO(delle) maybe VK_LOGIC_OP_CLEAR?
	colorBlendState.attachmentCount   = 1;
	colorBlendState.pAttachments      = &colorBlendAttachmentState;
	colorBlendState.blendConstants[0] = 0.0f;
	colorBlendState.blendConstants[1] = 0.0f;
	colorBlendState.blendConstants[2] = 0.0f;
	colorBlendState.blendConstants[3] = 0.0f;
	
	//dynamic states that can vary in the command buffer
	//https://renderdoc.org/vkspec_chunked/chap11.html#VkPipelineDynamicStateCreateInfo
	dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT, 
		VK_DYNAMIC_STATE_SCISSOR, 
	};
	dynamicState.pNext             = 0;
	dynamicState.flags             = 0;
	dynamicState.dynamicStateCount = (u32)dynamicStates.size();
	dynamicState.pDynamicStates    = dynamicStates.data();
	
	//base pipeline info and options
	pipelineCreateInfo.pNext               = 0;
	pipelineCreateInfo.flags               = 0;
	pipelineCreateInfo.stageCount          = 0;
	pipelineCreateInfo.pStages             = shaderStages.data();
	pipelineCreateInfo.pVertexInputState   = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pTessellationState  = 0;
	pipelineCreateInfo.pViewportState      = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pMultisampleState   = &multisampleState;
	pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
	pipelineCreateInfo.pColorBlendState    = &colorBlendState;
	pipelineCreateInfo.pDynamicState       = &dynamicState;
	pipelineCreateInfo.layout              = pipelineLayout;
	pipelineCreateInfo.renderPass          = renderPass;
	pipelineCreateInfo.subpass             = 0;
	pipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex   = -1;
} //SetupPipelineCreation


////////////////////////////
//// pipelines creation ////
////////////////////////////


//TODO(delle,ReCl) clean this up
static_internal std::vector<std::string> 
GetUncompiledShaders(){
	std::vector<std::string> compiled;
	for(auto& entry : std::filesystem::directory_iterator(Assets::dirShaders())){
		if(entry.path().extension() == ".spv"){
			compiled.push_back(entry.path().stem().string());
		}
	}
	
	std::vector<std::string> files;
	for(auto& entry : std::filesystem::directory_iterator(Assets::dirShaders())){
		if(entry.path().extension() == ".vert" ||
		   entry.path().extension() == ".frag" ||
		   entry.path().extension() == ".geom"){
			bool good = true;
			for(auto& s : compiled){
				if(entry.path().filename().string().compare(s) == 0){ good = false; break; }
			}
			if(good) files.push_back(entry.path().filename().string());
		}
	}
	return files;
}

//creates a pipeline shader stage from the shader bytecode
static_internal VkPipelineShaderStageCreateInfo 
loadShader(std::string filename, VkShaderStageFlagBits stage){
	PrintVk(3, "    Loading shader: ", filename);
	//setup shader stage create info
	VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	shaderStage.stage = stage;
	shaderStage.pName = "main";
	
	//check if shader has already been created
	for(auto& module : shaderModules){
		if(filename == module.first){
			shaderStage.module = module.second;
			break;
		}
	}
	
	//create shader module
	std::vector<char> code = Assets::readFileBinary(Assets::dirShaders() + filename);
	Assert(code.size(), "Unable to read shader file");
	
	VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	moduleInfo.codeSize = code.size();
	moduleInfo.pCode    = (u32*)code.data();
	
	VkShaderModule shaderModule{};
	AssertVk(vkCreateShaderModule(device, &moduleInfo, allocator, &shaderModule), "failed to create shader module");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SHADER_MODULE, (u64)shaderModule, TOSTRING("Shader ", filename).c_str());
	
	shaderStage.module = shaderModule;
	shaderModules.push_back(pair<std::string,VkShaderModule>(filename, shaderStage.module));
	return shaderStage;
}

//TODO(delle,Re) maybe dont crash on failed shader compile?
static_internal VkPipelineShaderStageCreateInfo 
CompileAndLoadShader(std::string filename, VkShaderStageFlagBits stage, bool optimize = false){
	PrintVk(3, "    Compiling and loading shader: ", filename);
	//check if file exists
	std::filesystem::path entry(Assets::dirShaders() + filename);
	if(std::filesystem::exists(entry)){
		std::string ext      = entry.extension().string();
		std::string filename = entry.filename().string();
		
		//setup shader compiler
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		if(optimize) shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
		
		std::vector<char> code = Assets::readFileBinary(Assets::dirShaders() + filename); //read shader code
		Assert(code.size(), "Unable to read shader file");
		
		//try compile from GLSL to SPIR-V binary
		shaderc_compilation_result_t result = 0;
		if(ext.compare(".vert") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_vertex_shader, 
											  filename.c_str(), "main", options);
		}else if(ext.compare(".frag") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_fragment_shader, 
											  filename.c_str(), "main", options);
		}else if(ext.compare(".geom") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_geometry_shader, 
											  filename.c_str(), "main", options);
		}else{ Assert(!"unsupported shader"); }
		defer{ shaderc_result_release(result); };
		
		//check for errors
		if(!result){ 
			PRINTLN("[ERROR]"<< filename <<": Shader compiler returned a null result"); 
			Assert(!"crashing on shader compile error"); 
		}
		if(shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success){
			PRINTLN("[ERROR] "<< shaderc_result_get_error_message(result));
			Assert(!"crashing on shader compile error"); 
		}
		
		//create shader module
		VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
		moduleInfo.codeSize = shaderc_result_get_length(result);
		moduleInfo.pCode    = (u32*)shaderc_result_get_bytes(result);
		
		VkShaderModule shaderModule{};
		AssertVk(vkCreateShaderModule(device, &moduleInfo, allocator, &shaderModule), "failed to create shader module");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SHADER_MODULE, (u64)shaderModule, TOSTRING("Shader ", filename).c_str());
		
		//setup shader stage create info
		VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
		shaderStage.stage  = stage;
		shaderStage.pName  = "main";
		shaderStage.module = shaderModule;
		
		return shaderStage;
	}
	Assert(!"failed to load shader module b/c file does not exist");
	return VkPipelineShaderStageCreateInfo();
}


static_internal void 
CompileAllShaders(bool optimize = false){
	//setup shader compiler
	shaderc_compiler_t compiler       = shaderc_compiler_initialize();
	shaderc_compile_options_t options = shaderc_compile_options_initialize();
	if(optimize) shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
	
	//loop thru all files in the shaders dir, compile the shaders, write them to .spv files
	for(auto& entry : std::filesystem::directory_iterator(Assets::dirShaders())){
		std::string ext      = entry.path().extension().string();
		std::string filename = entry.path().filename().string();
		
		if(ext.compare(".spv") == 0) continue; //early out if .spv
		std::vector<char> code = Assets::readFileBinary(entry.path().string()); //read shader code
		Assert(code.size(), "Unable to read shader file");
		PrintVk(4, "      Compiling shader: ", filename);
		
		//try compile from GLSL to SPIR-V binary
		shaderc_compilation_result_t result;
		if(ext.compare(".vert") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_vertex_shader, 
											  filename.c_str(), "main", options);
		}else if(ext.compare(".frag") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_fragment_shader, 
											  filename.c_str(), "main", options);
		}else if(ext.compare(".geom") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_geometry_shader, 
											  filename.c_str(), "main", options);
		}else{ continue; }
		defer{ shaderc_result_release(result); };
		
		//check for errors
		if(!result){ 
			PRINTLN("[ERROR]"<< filename <<": Shader compiler returned a null result");
			ERROR_LOC(filename,": Shader compiler returned a null result"); continue; 
		}
		if(shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success){
			PRINTLN("[ERROR] "<< shaderc_result_get_error_message(result));
			ERROR(shaderc_result_get_error_message(result)); 
			continue;
		}
		
		//create or overwrite .spv files
		std::ofstream outFile(entry.path().string() + ".spv", std::ios::out | std::ios::binary | std::ios::trunc);
		Assert(outFile.is_open(), "failed to open file");
		defer{ outFile.close(); };
		outFile.write(shaderc_result_get_bytes(result), shaderc_result_get_length(result));
	}
	
	//cleanup shader compiler and options
	shaderc_compile_options_release(options);
	shaderc_compiler_release(compiler);
}

static_internal void
CompileShader(std::string& filename, bool optimize){
	PrintVk(3, "    Compiling shader: ", filename);
	std::filesystem::path entry(Assets::dirShaders() + filename);
	if(std::filesystem::exists(entry)){
		std::string ext      = entry.extension().string();
		std::string filename = entry.filename().string();
		
		//setup shader compiler
		shaderc_compiler_t compiler       = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		if(optimize) shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
		
		std::vector<char> code = Assets::readFileBinary(Assets::dirShaders() + filename); //read shader code
		Assert(code.size(), "Unable to read shader file");
		
		//try compile from GLSL to SPIR-V binary
		shaderc_compilation_result_t result;
		if(ext.compare(".vert") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_vertex_shader, 
											  filename.c_str(), "main", options);
		}else if(ext.compare(".frag") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_fragment_shader, 
											  filename.c_str(), "main", options);
		}else if(ext.compare(".geom") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_geometry_shader, 
											  filename.c_str(), "main", options);
		}else{ return; }
		defer{ shaderc_result_release(result); };
		
		//check for errors
		if(!result){ 
			PRINTLN("[ERROR]"<< filename <<": Shader compiler returned a null result");
			ERROR_LOC(filename,": Shader compiler returned a null result");
		}
		if(shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success){
			PRINTLN("[ERROR] "<< shaderc_result_get_error_message(result));
			ERROR(shaderc_result_get_error_message(result)); 
		}
		
		//create or overwrite .spv files
		std::ofstream outFile(entry.string() + ".spv", std::ios::out | std::ios::binary | std::ios::trunc);
		Assert(outFile.is_open(), "failed to open file");
		defer{ outFile.close(); };
		outFile.write(shaderc_result_get_bytes(result), shaderc_result_get_length(result));
	}else{
		ERROR_LOC("[ERROR] failed to open file: ", filename);
	}
}

static_internal void 
CreatePipelines(){
	PrintVk(2, "  Creating Pipelines");
	AssertRS(RSVK_PIPELINESETUP, "CreatePipelines called before SetupPipelineCreation");
	rendererStage |= RSVK_PIPELINECREATE;
	
	//destroy previous pipelines
	forI(ArrayCount(pipelines.array)){
		if(pipelines.array[i]) vkDestroyPipeline(device, pipelines.array[i], nullptr);
	}
	
	//destroy previous shader modules
	size_t oldCount = shaderModules.size();
	for(auto& pair : shaderModules){
		vkDestroyShaderModule(device, pair.second, allocator);
	}
	shaderModules.clear(); shaderModules.reserve(oldCount);
	
	//compile uncompiled shaders
	PrintVk(3, "    Compiling shaders");
	TIMER_START(t_s);
	if(settings.recompileAllShaders){
		CompileAllShaders(settings.optimizeShaders);
	}else{
		for(auto& s : GetUncompiledShaders()){ CompileShader(s, settings.optimizeShaders); }
	}
	PrintVk(3, "    Finished compiling shaders in ", TIMER_END(t_s), "ms");
	
	//setup specialization constants
	/*
VkSpecializationMapEntry entryShadowPCF{};
	entryShadowPCF.constantID = 0;
	entryShadowPCF.offset = 0;
	entryShadowPCF.size = sizeof(b32);
	*/
	
	VkSpecializationInfo specializationInfo{};
	/*
specializationInfo.mapEntryCount = 1;
	specializationInfo.pMapEntries = &entryShadowPCF;
	specializationInfo.dataSize = sizeof(b32);
	specializationInfo.pData = &settings.shadowPCF;
*/
	
	{//base pipeline
		//flag that this pipeline will be used as a base
		pipelineCreateInfo.flags              = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCreateInfo.basePipelineIndex  = -1;
		
		shaderStages[0] = loadShader("base.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("base.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.base), "failed to create base graphics pipeline");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.base, "Base pipeline");
		
		//flag that all other pipelines are derivatives
		pipelineCreateInfo.flags              = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
		pipelineCreateInfo.basePipelineHandle = pipelines.base;
		pipelineCreateInfo.basePipelineIndex  = -1; //can either use handle or index, not both (section 9.5 of vulkan spec)
	}
	
	{//flat pipeline
		shaderStages[0] = loadShader("flat.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("flat.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.flat), "failed to create flat graphics pipeline");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.flat, "Flat pipeline");
	}
	
	{//phong
		shaderStages[0] = loadShader("phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.phong), "failed to create phong graphics pipeline");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.phong, "Phong pipeline");
	}
	
	{//2d
		shaderStages[0] = loadShader("twod.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("twod.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.twod), "failed to create twod graphics pipeline");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.twod, "TwoD pipeline");
	}
	
	{//pbr
		colorBlendAttachmentState.blendEnable = VK_TRUE;
		colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		
		shaderStages[0] = loadShader("pbr.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("pbr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.pbr), "failed to create pbr graphics pipeline");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.pbr, "PBR pipeline");
		
		colorBlendAttachmentState.blendEnable = VK_FALSE;
		colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	}
	
	//wireframe
	if(deviceFeatures.fillModeNonSolid){
		rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		depthStencilState.depthTestEnable = VK_FALSE;
		
		shaderStages[0] = loadShader("wireframe.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("wireframe.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.wireframe), "failed to create wireframe graphics pipeline");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.wireframe, "Wireframe pipeline");
		
		{//wireframe with depth test
			depthStencilState.depthTestEnable = VK_TRUE;
			
			AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.wireframe_depth), "failed to create wireframe-depth graphics pipeline");
			DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.wireframe_depth, "Wireframe Depth pipeline");
			
			depthStencilState.depthTestEnable = VK_FALSE;
		}
		
		{ //selected entity and collider gets a specific colored wireframe
			colorBlendAttachmentState.blendEnable = VK_TRUE;
			colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
			colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
			colorBlendState.blendConstants[0] = (f32)settings.selectedColor.r;
			colorBlendState.blendConstants[1] = (f32)settings.selectedColor.g;
			colorBlendState.blendConstants[2] = (f32)settings.selectedColor.b;
			colorBlendState.blendConstants[3] = (f32)settings.selectedColor.a;
			
			AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.selected), "failed to create selected entity graphics pipeline");
			DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.selected, "Selected pipeline");
			
			colorBlendState.blendConstants[0] = (f32)settings.colliderColor.r;
			colorBlendState.blendConstants[1] = (f32)settings.colliderColor.g;
			colorBlendState.blendConstants[2] = (f32)settings.colliderColor.b;
			colorBlendState.blendConstants[3] = (f32)settings.colliderColor.a;
			
			AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.collider), "failed to create collider graphics pipeline");
			DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.collider, "Collider pipeline");
			
			colorBlendAttachmentState.blendEnable = VK_FALSE;
			colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendState.blendConstants[0] = 0.f;
			colorBlendState.blendConstants[1] = 0.f;
			colorBlendState.blendConstants[2] = 0.f;
			colorBlendState.blendConstants[3] = 1.0f;
		}
		
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		depthStencilState.depthTestEnable = VK_TRUE;
	}
	
	{//lavalamp
		shaderStages[0] = loadShader("lavalamp.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("lavalamp.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.lavalamp), "failed to create lavalamp graphics pipeline");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.lavalamp, "Lavalamp pipeline");
	}
	
	{//offscreen
		colorBlendState.attachmentCount = 0; //no color attachments used
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; //cull front faces
		rasterizationState.depthBiasEnable = VK_TRUE; //enable depth bias
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.sampleShadingEnable  = VK_FALSE;
		dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS, };
		dynamicState.dynamicStateCount = (u32)dynamicStates.size(); //add depth bias to dynamic state so
		dynamicState.pDynamicStates    = dynamicStates.data();         //it can be changed at runtime
		pipelineCreateInfo.renderPass = offscreen.renderpass;
		
		shaderStages[0] = loadShader("offscreen.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		pipelineCreateInfo.stageCount = 1;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.offscreen), "failed to create offscreen graphics pipeline");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.offscreen, "Offscreen pipeline");
		
		colorBlendState.attachmentCount = 1;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		rasterizationState.depthBiasEnable = VK_FALSE;
		multisampleState.rasterizationSamples = msaaSamples;
		multisampleState.sampleShadingEnable  = VK_TRUE;
		dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, };
		dynamicState.dynamicStateCount = (u32)dynamicStates.size();
		dynamicState.pDynamicStates    = dynamicStates.data();
		pipelineCreateInfo.renderPass = renderPass;
	}
	
	//NOTE(delle) testing/debug shaders should be removed on release
	{//testing0
		shaderStages[0] = loadShader("testing0.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("testing0.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.testing0), "failed to create testing0 graphics pipeline");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.testing0, "Testing0 pipeline");
	}
	
	{//testing1
		colorBlendAttachmentState.blendEnable = VK_TRUE;
		colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		
		shaderStages[0] = loadShader("testing1.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("testing1.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.testing1), "failed to create testing1 graphics pipeline");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.testing1, "Testing1 pipeline");
		
		colorBlendAttachmentState.blendEnable = VK_FALSE;
		colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; 
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	}
	
	//DEBUG mesh normals
	if(settings.debugging){
		shaderStages[0] = loadShader("base.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("base.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[2] = loadShader("normaldebug.geom.spv", VK_SHADER_STAGE_GEOMETRY_BIT);
		pipelineCreateInfo.stageCount = 3;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.normals_debug), "failed to create normals_debug graphics pipeline");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.normals_debug, "DEBUG Mesh normals pipeline");
	}
	
	{//DEBUG shadow map
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		VkPipelineVertexInputStateCreateInfo emptyVertexInputState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
		pipelineCreateInfo.pVertexInputState = &emptyVertexInputState;
		
		shaderStages[0] = loadShader("shadowmapDEBUG.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("shadowmapDEBUG.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.shadowmap_debug), "failed to create DEBUG shadowmap graphics pipeline");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.shadowmap_debug, "DEBUG Shadowmap pipeline");
		
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
	}
} //CreatePipelines

static_internal VkPipeline 
GetPipelineFromShader(u32 shader){
	switch(shader){
		case(Shader_Flat):default: { return pipelines.flat;      };
		case(Shader_Phong):        { return pipelines.phong;     };
		case(Shader_Twod):         { return pipelines.twod;      };
		case(Shader_PBR):          { return pipelines.pbr;       };
		case(Shader_Wireframe):    { return pipelines.wireframe; };
		case(Shader_Lavalamp):     { return pipelines.lavalamp;  };
		case(Shader_Testing0):     { return pipelines.testing0;  };
		case(Shader_Testing1):     { return pipelines.testing1;  };
	}
}

static_internal void 
UpdateMaterialPipelines(){
	PrintVk(4, "      Updating material pipelines");
	for(auto& mat : materials){
		mat.pipeline = GetPipelineFromShader(mat.shader);
	}
}


///////////////
//// other ////
///////////////


//we define a call order to command buffers so they can be executed by vkSubmitQueue()
static_internal void 
BuildCommandBuffers(){
	//PrintVk(2, "  Building Command Buffers");
	AssertRS(RSVK_DESCRIPTORSETS | RSVK_PIPELINECREATE, "BuildCommandBuffers called before CreateDescriptorSets or CreatePipelines");
	
	VkClearValue clearValues[2]{};
	VkCommandBufferBeginInfo cmdBufferInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	VkViewport viewport{};
	VkRect2D scissor{}; //TODO(delle,Re) letterboxing settings here
	
	//TODO(delle,ReVu) figure out why we are doing it for all images
	for(int i = 0; i < imageCount; ++i){
		AssertVk(vkBeginCommandBuffer(frames[i].commandBuffer, &cmdBufferInfo), "failed to begin recording command buffer");
		
		///////////////////////////
		//// first render pass ////
		///////////////////////////
		{//generate shadow map by rendering the scene offscreen
			clearValues[0].depthStencil = {1.0f, 0};
			renderPassInfo.renderPass               = offscreen.renderpass;
			renderPassInfo.framebuffer              = offscreen.framebuffer;
			renderPassInfo.renderArea.offset        = {0, 0};
			renderPassInfo.renderArea.extent.width  = offscreen.width;
			renderPassInfo.renderArea.extent.height = offscreen.height;
			renderPassInfo.clearValueCount          = 1;
			renderPassInfo.pClearValues             = clearValues;
			viewport.width    = (float)offscreen.width;
			viewport.height   = (float)offscreen.height;
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			scissor.offset.x      = 0;
			scissor.offset.y      = 0;
			scissor.extent.width  = offscreen.width;
			scissor.extent.height = offscreen.width;
			
			DebugBeginLabelVk(frames[i].commandBuffer, "Offscreen Render Pass", vec4(0.78f, 0.54f, 0.12f, 1.0f));
			vkCmdBeginRenderPass(frames[i].commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(frames[i].commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(frames[i].commandBuffer, 0, 1, &scissor);
			vkCmdSetDepthBias(frames[i].commandBuffer, settings.depthBiasConstant, 0.0f, settings.depthBiasSlope); //set depth bias (polygon offset) to avoid shadow mapping artifacts
			vkCmdBindPipeline(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
			vkCmdBindDescriptorSets(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.offscreen, 0, nullptr);
			
			VkDeviceSize offsets[1] = { 0 }; //reset vertex buffer offsets
			
			DebugBeginLabelVk(frames[i].commandBuffer, "Meshes", vec4(0.5f, 0.76f, 0.34f, 1.0f));
			vkCmdBindVertexBuffers(frames[i].commandBuffer, 0, 1, &vertices.buffer, offsets);
			vkCmdBindIndexBuffer(frames[i].commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			for(MeshVk& mesh : meshes){
				if(mesh.visible && mesh.primitives.size() > 0){
					vkCmdPushConstants(frames[i].commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(mat4), &mesh.modelMatrix);
					for(PrimitiveVk& primitive : mesh.primitives){
						if(primitive.indexCount > 0){
							DebugInsertLabelVk(frames[i].commandBuffer, mesh.name, vec4(0.4f, 0.61f, 0.27f, 1.0f));
							vkCmdDrawIndexed(frames[i].commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
							stats.drawnIndices += primitive.indexCount;
						}
					}
				}
			}
			DebugEndLabelVk(frames[i].commandBuffer);
			
			vkCmdEndRenderPass(frames[i].commandBuffer);
			DebugEndLabelVk(frames[i].commandBuffer);
		}
		
		//NOTE explicit synchronization is not required because it is done via the subpass dependenies
		
		////////////////////////////
		//// second render pass ////
		////////////////////////////
		{//scene rendering with applied shadow map
			clearValues[0].color        = {settings.clearColor.r, settings.clearColor.g, settings.clearColor.b, settings.clearColor.a};
			clearValues[1].depthStencil = {1.0f, 0};
			renderPassInfo.renderPass        = renderPass;
			renderPassInfo.framebuffer       = frames[i].framebuffer;
			renderPassInfo.clearValueCount   = 2;
			renderPassInfo.pClearValues      = clearValues;
			renderPassInfo.renderArea.offset = {0, 0};
			renderPassInfo.renderArea.extent = extent;
			viewport.x        = 0;
			viewport.y        = 0;
			viewport.width    = (float)width;
			viewport.height   = (float)height;
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			scissor.offset.x      = 0;
			scissor.offset.y      = 0;
			scissor.extent.width  = width;
			scissor.extent.height = height;
			
			
			DebugBeginLabelVk(frames[i].commandBuffer, "Scene Render Pass", vec4(0.78f, 0.54f, 0.12f, 1.0f));
			vkCmdBeginRenderPass(frames[i].commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(frames[i].commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(frames[i].commandBuffer, 0, 1, &scissor);
			vkCmdBindDescriptorSets(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.scene, 0, nullptr);
			VkDeviceSize offsets[1] = { 0 }; //reset vertex buffer offsets
			
			//draw mesh brushes
			if(!generatingWorldGrid){
				DebugBeginLabelVk(frames[i].commandBuffer, "Mesh brushes", vec4(0.5f, 0.76f, 0.34f, 1.0f));
				vkCmdBindPipeline(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe_depth);
				for(MeshBrushVk& mesh : meshBrushes){
					if(mesh.visible){
						vkCmdBindVertexBuffers(frames[i].commandBuffer, 0, 1, &mesh.vertexBuffer, offsets);
						vkCmdBindIndexBuffer(frames[i].commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
						vkCmdPushConstants(frames[i].commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(mat4), &mesh.modelMatrix);
						DebugInsertLabelVk(frames[i].commandBuffer, mesh.name, vec4(0.4f, 0.61f, 0.27f, 1.0f));
						vkCmdDrawIndexed(frames[i].commandBuffer, mesh.indices.size(), 1, 0, 0, 0);
						stats.drawnIndices += mesh.indices.size();
					}
				}
				DebugEndLabelVk(frames[i].commandBuffer);
			}
			
			//draw meshes
			DebugBeginLabelVk(frames[i].commandBuffer, "Meshes", vec4(0.5f, 0.76f, 0.34f, 1.0f));
			vkCmdBindVertexBuffers(frames[i].commandBuffer, 0, 1, &vertices.buffer, offsets);
			vkCmdBindIndexBuffer(frames[i].commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			
			if(settings.wireframeOnly){ //draw all with wireframe shader
				vkCmdBindPipeline(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe);
				for(MeshVk& mesh : meshes){
					if(mesh.visible && mesh.primitives.size() > 0){
						//push the mesh's model matrix to the vertex shader
						vkCmdPushConstants(frames[i].commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(mat4), &mesh.modelMatrix);
						
						for(PrimitiveVk& primitive : mesh.primitives){
							if(primitive.indexCount > 0){
								DebugInsertLabelVk(frames[i].commandBuffer, mesh.name, vec4(0.4f, 0.61f, 0.27f, 1.0f));
								vkCmdDrawIndexed(frames[i].commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
								stats.drawnIndices += primitive.indexCount;
							}
						}
					}
				}
			}else{
				for(MeshVk& mesh : meshes){
					if(mesh.visible && mesh.primitives.size() > 0){
						//push the mesh's model matrix to the vertex shader
						vkCmdPushConstants(frames[i].commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(mat4), &mesh.modelMatrix);
						
						for(PrimitiveVk& primitive : mesh.primitives){
							if(primitive.indexCount > 0){
								MaterialVk& material = materials[primitive.materialIndex];
								// Bind the pipeline for the primitive's material
								vkCmdBindPipeline(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material.pipeline);
								vkCmdBindDescriptorSets(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &material.descriptorSet, 0, nullptr);
								DebugInsertLabelVk(frames[i].commandBuffer, mesh.name, vec4(0.4f, 0.61f, 0.27f, 1.0f));
								vkCmdDrawIndexed(frames[i].commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
								stats.drawnIndices += primitive.indexCount;
								
								if(settings.meshWireframes && material.pipeline != pipelines.wireframe){
									vkCmdBindPipeline(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe);
									DebugInsertLabelVk(frames[i].commandBuffer, mesh.name, vec4(0.4f, 0.61f, 0.27f, 1.0f));
									vkCmdDrawIndexed(frames[i].commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
									stats.drawnIndices += primitive.indexCount;
								}
							}
						}
					}
				}
			}
			DebugEndLabelVk(frames[i].commandBuffer);
			
			//draw selected meshes
			DebugBeginLabelVk(frames[i].commandBuffer, "Selected Meshes", vec4(0.5f, 0.76f, 0.34f, 1.0f));
			for(u32 id : selected){
				MeshVk& mesh = meshes[id];
				if(mesh.primitives.size() > 0){
					//push the mesh's model matrix to the vertex shader
					vkCmdPushConstants(frames[i].commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(mat4), &mesh.modelMatrix);
					
					for(PrimitiveVk& primitive : mesh.primitives){
						if(primitive.indexCount > 0){
							MaterialVk& material = materials[primitive.materialIndex];
							// Bind the pipeline for the primitive's material
							vkCmdBindPipeline(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.selected);
							vkCmdBindDescriptorSets(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &material.descriptorSet, 0, nullptr);
							DebugInsertLabelVk(frames[i].commandBuffer, mesh.name, vec4(0.4f, 0.61f, 0.27f, 1.0f));
							vkCmdDrawIndexed(frames[i].commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
							stats.drawnIndices += primitive.indexCount;
						}
					}
				}
			}
			DebugEndLabelVk(frames[i].commandBuffer);
			
			//DEBUG draw mesh normals
			if(settings.debugging && enabledFeatures.geometryShader && settings.meshNormals){
				DebugBeginLabelVk(frames[i].commandBuffer, "DEBUG Mesh Normals", vec4(0.5f, 0.76f, 0.34f, 1.0f));
				vkCmdBindPipeline(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.normals_debug);
				for(MeshVk& mesh : meshes){
					if(mesh.visible && mesh.primitives.size() > 0){
						//push the mesh's model matrix to the shader
						vkCmdPushConstants(frames[i].commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(mat4), &mesh.modelMatrix);
						
						for(PrimitiveVk& primitive : mesh.primitives){
							if(primitive.indexCount > 0){
								DebugInsertLabelVk(frames[i].commandBuffer, mesh.name, vec4(0.4f, 0.61f, 0.27f, 1.0f));
								vkCmdDrawIndexed(frames[i].commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
								stats.drawnIndices += primitive.indexCount;
							}
						}
					}
				}
				DebugEndLabelVk(frames[i].commandBuffer);
			}
			
			//draw imgui stuff
			if(ImDrawData* imDrawData = ImGui::GetDrawData()){
				DebugBeginLabelVk(frames[i].commandBuffer, "ImGui", vec4(0.5f, 0.76f, 0.34f, 1.0f));
				ImGui_ImplVulkan_RenderDrawData(imDrawData, frames[i].commandBuffer);
				DebugEndLabelVk(frames[i].commandBuffer);
			}
			
			//DEBUG draw shadow map
			if(settings.showShadowMap){
				viewport.x      = (float)(width - 400);
				viewport.y      = (float)(height - 400);
				viewport.width  = 400.f;
				viewport.height = 400.f;
				vkCmdSetViewport(frames[i].commandBuffer, 0, 1, &viewport);
				vkCmdSetScissor(frames[i].commandBuffer, 0, 1, &scissor);
				
				DebugBeginLabelVk(frames[i].commandBuffer, "DEBUG Shadow map quad", vec4(0.5f, 0.76f, 0.34f, 1.0f));
				vkCmdBindDescriptorSets(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.shadowMap_debug, 0, nullptr);
				vkCmdBindPipeline(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.shadowmap_debug);
				vkCmdDraw(frames[i].commandBuffer, 3, 1, 0, 0);
				DebugEndLabelVk(frames[i].commandBuffer);
				
				viewport.x      = 0;
				viewport.y      = 0;
				viewport.width  = (float)width;
				viewport.height = (float)height;
			}
			
			vkCmdEndRenderPass(frames[i].commandBuffer);
			DebugEndLabelVk(frames[i].commandBuffer);
		}
		
		AssertVk(vkEndCommandBuffer(frames[i].commandBuffer), "failed to end recording command buffer");
	}
}


//-------------------------------------------------------------------------------------------------
// IMGUI FUNCTIONS


static_internal void imguiCheckVkResult(VkResult err){
	// https://renderdoc.org/vkspec_chunked/chap4.html#VkResult
	AssertVk(err, "imgui vulkan error");
}

static_internal char iniFilepath[256] = {};
void DeshiImGui::
init(){
	//Setup Dear ImGui context
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	cpystr(iniFilepath, (Assets::dirConfig() + "imgui.ini").c_str(), 256);
	io.IniFilename = iniFilepath;
	
	//Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	
	//Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForVulkan(DengWindow->window, true);
	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.Instance        = instance;
	init_info.PhysicalDevice  = physicalDevice;
	init_info.Device          = device;
	init_info.QueueFamily     = physicalQueueFamilies.graphicsFamily.value;
	init_info.Queue           = graphicsQueue;
	init_info.PipelineCache   = pipelineCache;
	init_info.DescriptorPool  = descriptorPool;
	init_info.Allocator       = allocator;
	init_info.MinImageCount   = minImageCount;
	init_info.ImageCount      = imageCount;
	init_info.CheckVkResultFn = imguiCheckVkResult;
	init_info.MSAASamples     = msaaSamples;
	ImGui_ImplVulkan_Init(&init_info, renderPass);
	
	//Upload Fonts
	VkCommandPool   command_pool   = commandPool;
	VkCommandBuffer command_buffer = frames[frameIndex].commandBuffer;
	
	AssertVk(vkResetCommandPool(device, command_pool, 0));
	
	VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	AssertVk(vkBeginCommandBuffer(command_buffer, &begin_info));
	ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
	
	VkSubmitInfo end_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	end_info.commandBufferCount = 1;
	end_info.pCommandBuffers    = &command_buffer;
	AssertVk(vkEndCommandBuffer(command_buffer));
	
	AssertVk(vkQueueSubmit(graphicsQueue, 1, &end_info, VK_NULL_HANDLE));
	
	AssertVk(vkDeviceWaitIdle(device));
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void DeshiImGui::
cleanup(){
	AssertVk(vkDeviceWaitIdle(device));
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void DeshiImGui::
newFrame(){
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

//-------------------------------------------------------------------------------------------------
// INTERFACE FUNCTIONS

void Render::
Init(){
	PrintVk(1, "Initializing Vulkan");
	TIMER_START(t_v);
	
	//// load RenderSettings ////
	LoadSettings();
	if(settings.debugging && settings.printf){
		validationFeaturesEnabled.push_back(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
	}
	
	TIMER_START(t_temp);
	//// setup Vulkan instance ////
	CreateInstance();
	PrintVk(3, "Finished creating instance in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	SetupDebugMessenger();
	PrintVk(3, "Finished setting up debug messenger in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	
	//// grab Vulkan extension functions ////
#if DESHI_INTERNAL
	func_vkSetDebugUtilsObjectNameEXT  = (PFN_vkSetDebugUtilsObjectNameEXT) vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
	func_vkCmdBeginDebugUtilsLabelEXT  = (PFN_vkCmdBeginDebugUtilsLabelEXT) vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
	func_vkCmdEndDebugUtilsLabelEXT    = (PFN_vkCmdEndDebugUtilsLabelEXT)   vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
	func_vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT");
#endif //DESHI_INTERNAL
	
	//// setup Vulkan-OperatingSystem interactions ////
	CreateSurface();
	PrintVk(3, "Finished creating surface in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	PickPhysicalDevice();
	PrintVk(3, "Finished picking physical device in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	CreateLogicalDevice();
	PrintVk(3, "Finished creating logical device in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	
	//// limit RenderSettings to device capabilties ////
	//NOTE currently disabled b/c of my resolve setup
	//msaaSamples = (VkSampleCountFlagBits)(((1 << settings.msaaSamples) > maxMsaaSamples) ? settings.msaaSamples : 1 << settings.msaaSamples);
	msaaSamples = maxMsaaSamples;
	
	//// setup unchanging Vulkan objects ////
	CreateCommandPool();
	PrintVk(3, "Finished creating command pool in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	CreateUniformBuffers();
	PrintVk(3, "Finished creating uniform buffer in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	CreateLayouts();
	PrintVk(3, "Finished creating layouts in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	CreateDescriptorPool();
	PrintVk(3, "Finished creating descriptor pool in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	SetupOffscreenRendering();
	PrintVk(3, "Finished setting up offscreen rendering in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	
	//// setup window-specific Vulkan objects ////
	CreateSwapChain();
	PrintVk(3, "Finished creating swap chain in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	CreateRenderpass();
	PrintVk(3, "Finished creating render pass in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	CreateFrames();
	PrintVk(3, "Finished creating frames in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	CreateSyncObjects();
	PrintVk(3, "Finished creating sync objects in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	CreateDescriptorSets();
	PrintVk(3, "Finished creating descriptor sets in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	CreatePipelineCache();
	PrintVk(3, "Finished creating pipeline cache in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	SetupPipelineCreation();
	PrintVk(3, "Finished setting up pipeline creation in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	CreatePipelines();
	PrintVk(3, "Finished creating pipelines in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	
	LoadDefaultAssets();
	PrintVk(3, "Finished loading default assets in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	CreateSceneMeshBuffers();
	PrintVk(3, "Finished creating scene mesh buffers in ", TIMER_END(t_temp), "ms");TIMER_RESET(t_temp);
	
	initialized = true;
	PrintVk(3, "Finished initializing Vulkan in ", TIMER_END(t_v), "ms");
}

void Render::
Update(){
	//PrintVk(1, "---------------new frame---------------");
	AssertRS(RSVK_PIPELINECREATE | RSVK_FRAMES | RSVK_SYNCOBJECTS, "Render called before CreatePipelines or CreateFrames or CreateSyncObjects");
	rendererStage = RSVK_RENDER;
	
	if(DengWindow->resized) remakeWindow = true;
	if(remakeWindow){
		int w, h;
		glfwGetFramebufferSize(DengWindow->window, &w, &h);
		if(w <= 0 || h <= 0){ 
			ImGui::EndFrame(); 
			return;  
		}
		
		PrintVk(1, "window resized");
		// Ensure all operations on the device have been finished before destroying resources
		vkDeviceWaitIdle(device);
		CreateSwapChain();
		CreateFrames();
		
		frameIndex = 0;
		remakeWindow = false;
	}
	
	//reset frame stats
	stats = {};
	TIMER_START(t_r);
	
	//get next image from surface
	u32 imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &imageIndex);
	if(result == VK_ERROR_OUT_OF_DATE_KHR){
		remakeWindow = true;
		return;
	}else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
		Assert(!"failed to acquire swap chain image");
	}
	
	//render stuff
	ImGui::Render(); ImDrawData* drawData = ImGui::GetDrawData();
	if(drawData){
		stats.drawnIndices += drawData->TotalIdxCount;
		stats.totalVertices += drawData->TotalVtxCount;
	}
	BuildCommandBuffers();
	
	//update uniform buffers
	UpdateUniformBuffers();
	
	//submit the command buffer to the queue
	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAcquiredSemaphore;
	submitInfo.pWaitDstStageMask = &wait_stage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frames[imageIndex].commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderCompleteSemaphore;
	
	AssertVk(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE), "failed to submit draw command buffer");
	
	if(remakeWindow){ return; }
	
	//present the image
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;
	result = vkQueuePresentKHR(presentQueue, &presentInfo);
	
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || remakeWindow){
		remakeWindow = false;
		
		PrintVk(1, "window resized");
		// Ensure all operations on the device have been finished before destroying resources
		vkDeviceWaitIdle(device);
		CreateSwapChain();
		CreateFrames();
	} else if(result != VK_SUCCESS){
		Assert(!"failed to present swap chain image");
	}
	
	//iterate the frame index
	frameIndex = (frameIndex + 1) % MAX_FRAMES; //loops back to zero after reaching max_frames
	AssertVk(vkQueueWaitIdle(graphicsQueue), "graphics queue failed to wait");
	//update stats
	stats.drawnTriangles += stats.drawnIndices / 3;
	stats.totalVertices += (u32)vertexBuffer.size();
	stats.totalIndices += (u32)indexBuffer.size();
	stats.totalTriangles += stats.totalIndices / 3;
	stats.renderTimeMS = TIMER_END(t_r);
	
	if(remakePipelines){ 
		CreatePipelines(); 
		UpdateMaterialPipelines();
		remakePipelines = false; 
	}
	if(_remakeOffscreen){
		SetupOffscreenRendering();
		_remakeOffscreen = true;
	}
}

void Render::
Reset(){
	SUCCESS("Resetting renderer (Vulkan)");
	vkDeviceWaitIdle(device); //wait before cleanup
	
	//vertex buffer
	vertexBuffer.clear();
	
	//index buffer
	indexBuffer.clear();
	
	//textures
	for(auto& tex : textures){
		vkDestroyImage(device, tex.image, nullptr);
		vkFreeMemory(device, tex.imageMemory, nullptr);
		vkDestroyImageView(device, tex.view, nullptr);
		vkDestroySampler(device, tex.sampler, nullptr);
	}
	textures.clear();
	
	//meshes
	meshes.clear();
	
	//materials
	for(auto& mat : materials){
		vkFreeDescriptorSets(device, descriptorPool, 1, &mat.descriptorSet);
	}
	materials.clear();
	
	//mesh brushes
	for(auto& mesh : meshBrushes){
		vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
		vkFreeMemory(device, mesh.vertexBufferMemory, nullptr);
		vkDestroyBuffer(device, mesh.indexBuffer, nullptr);
		vkFreeMemory(device, mesh.indexBufferMemory, nullptr);
	}
	meshBrushes.clear();
	
	LoadDefaultAssets();
}

//TODO(delle,Vu) maybe cache pipeline creation vars?
void Render::
Cleanup(){
	PrintVk(1, "Initializing Cleanup\n");
	
	Render::SaveSettings();
	
	//save pipeline cache to disk
	if(pipelineCache != VK_NULL_HANDLE){
		// Get size of pipeline cache
		size_t size{};
		AssertVk(vkGetPipelineCacheData(device, pipelineCache, &size, nullptr), "failed to get pipeline cache data size");
		// Get data of pipeline cache
		std::vector<char> data(size);
		AssertVk(vkGetPipelineCacheData(device, pipelineCache, &size, data.data()), "failed to get pipeline cache data");
		// Write pipeline cache data to a file in binary format
		Assets::writeFileBinary(Assets::dirData() + "pipelines.cache", data);
	}
	
	vkDeviceWaitIdle(device);
}

void Render::
SaveSettings(){
	Assets::saveConfig("render.cfg", configMap);
}

void Render::
LoadSettings(){
	Assets::loadConfig("render.cfg", configMap);
	
	//update dependent settings
	if(settings.printf) settings.loggingLevel = 4;
}

RenderSettings* Render::
GetSettings(){
	return &settings;
}

RenderStats* Render::
GetStats(){
	return &stats;
}

RendererStage* Render::
GetStage(){
	return &rendererStage;
}

void Render::
LoadDefaultAssets(){
	PrintVk(2, "  Loading default assets");
	
	//load default textures
	textures.reserve(16);
	LoadTexture("null128.png", 0);
	LoadTexture("default1024.png", 0);
	LoadTexture("black1024.png", 0);
	LoadTexture("white1024.png", 0);
	
	materials.reserve(16);
	//create default materials
	CreateMaterial(ShaderStrings[Shader_Flat], Shader_Flat);
	CreateMaterial(ShaderStrings[Shader_Phong], Shader_Phong);
	CreateMaterial(ShaderStrings[Shader_Twod], Shader_Twod);
}

void Render::
remakeOffscreen(){
	_remakeOffscreen = true;
}

/////////////////////
//// debug stuff ////
/////////////////////


u32 Render::
CreateDebugLine(Vector3 start, Vector3 end, Color color, bool visible){
	Vector3 c = Vector3(color.r, color.g, color.b) / 255.f;
	std::vector<Vertex> vertices = {
		{start, Vector2::ZERO, c, Vector3::ZERO},
		{end  , Vector2::ZERO, c, Vector3::ZERO},
	};
	std::vector<u32> indices = { 0,1,0 };
	Batch batch("line_batch", vertices, indices, {});
	batch.shader = Shader_Wireframe;
	Mesh mesh("debug_line", { batch });
	mesh.vertexCount = 2;
	mesh.indexCount = 3;
	mesh.batchCount = 1;
	u32 id = CreateMeshBrush(&mesh, Matrix4::TransformationMatrix(Vector3::ZERO, Vector3::ZERO, Vector3::ONE));
	//materials[mesh[id].primitives[0].materialIndex].pipeline = pipelines.WIREFRAME_DEPTH;
	//materials[mesh[id].primitives[0].materialIndex].shader = Shader_Wireframe;
	return id;
}

void Render::
UpdateDebugLine(u32 id, Vector3 start, Vector3 end, Color color){
	vec4 c = vec4(vec3(color.r, color.g, color.b) / 255.f, 1.f);
	meshBrushes[id].vertices[0].pos   = vec4(start, 1.f);
	meshBrushes[id].vertices[1].pos   = vec4(end, 1.f);
	meshBrushes[id].vertices[0].color = c;
	meshBrushes[id].vertices[1].color = c;
	UpdateMeshBrushBuffers(id);
	
}

u32 Render::
CreateDebugTriangle(Vector3 v1, Vector3 v2, Vector3 v3, Color color, bool visible){
	Vector3 c = Vector3(color.r, color.g, color.b) / 255.f;
	std::vector<Vertex> vertices = {
		{v1, Vector2::ZERO, c, Vector3::ZERO},
		{v2, Vector2::ZERO, c, Vector3::ZERO},
		{v3, Vector2::ZERO, c, Vector3::ZERO},
	};
	std::vector<u32> indices = { 0,1,2 };
	Batch batch("tri_batch", vertices, indices, {});
	batch.shader = Shader_Wireframe;
	Mesh mesh("debug_tri", { batch });
	mesh.vertexCount = 3;
	mesh.indexCount = 3;
	mesh.batchCount = 1;
	u32 id = LoadBaseMesh(&mesh, visible);
	materials[meshes[id].primitives[0].materialIndex].pipeline = pipelines.wireframe_depth;
	materials[meshes[id].primitives[0].materialIndex].shader = Shader_Wireframe;
	return id;
}


u32 Render::
CreateMeshBrush(Mesh* m, Matrix4 matrix, b32 log_creation){
	if(log_creation) PrintVk(3, "    Creating mesh brush based on: ", m->name);
	
	if(m->vertexCount == 0 || m->indexCount == 0 || m->batchCount == 0){  //early out if empty buffers
		ERROR("CreateMeshBrush: A mesh was passed in with no vertices or indices or batches");
		return -1;
	}
	
	//// mesh brush ////
	MeshBrushVk mesh; mesh.id = meshBrushes.size();
	cpystr(mesh.name, m->name, DESHI_NAME_SIZE);
	mesh.modelMatrix = matrix;
	
	mesh.vertices.reserve(m->vertexCount);
	mesh.indices.reserve(m->indexCount);
	u32 batchVertexStart, batchIndexStart;
	for(Batch& batch : m->batchArray){
		batchVertexStart = (u32)mesh.vertices.size();
		batchIndexStart = (u32)mesh.indices.size();
		
		//vertices
		for(int i=0; i<batch.vertexArray.size(); ++i){ 
			VertexVk vert;
			vert.pos      = vec4(batch.vertexArray[i].pos, 1.0f);
			vert.uv       = vec4(batch.vertexArray[i].uv.x, batch.vertexArray[i].uv.y, 0.0f, 0.0f);
			vert.color    = vec4(batch.vertexArray[i].color, 1.0f);
			vert.normal   = vec4(batch.vertexArray[i].normal, 1.0f);
			mesh.vertices.push_back(vert);
		}
		
		//indices
		for(u32 i : batch.indexArray){
			mesh.indices.push_back(batchVertexStart+i);
		}
	}
	
	{//// vulkan buffers ////
		StagingBufferVk vertexStaging{}, indexStaging{};
		size_t vbSize = mesh.vertices.size() * sizeof(VertexVk);
		size_t ibSize = mesh.indices.size() * sizeof(u32);
		
		//create host visible vertex and index buffers (CPU/RAM)
		CreateAndMapBuffer(vertexStaging.buffer, vertexStaging.memory, mesh.vertexBufferSize, vbSize, mesh.vertices.data(),
						   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		
		CreateAndMapBuffer(indexStaging.buffer, indexStaging.memory, mesh.indexBufferSize, ibSize, mesh.indices.data(),
						   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		
		//create device local buffers (GPU)
		CreateAndMapBuffer(mesh.vertexBuffer, mesh.vertexBufferMemory, mesh.vertexBufferSize, vbSize, nullptr,
						   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
						   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		
		CreateAndMapBuffer(mesh.indexBuffer, mesh.indexBufferMemory, mesh.indexBufferSize, ibSize, nullptr,
						   VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
						   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		
		//copy data from staging buffers to device local buffers
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();{
			VkBufferCopy copyRegion{};
			
			copyRegion.size = vbSize;
			vkCmdCopyBuffer(commandBuffer, vertexStaging.buffer, mesh.vertexBuffer, 1, &copyRegion);
			
			copyRegion.size = ibSize;
			vkCmdCopyBuffer(commandBuffer, indexStaging.buffer, mesh.indexBuffer, 1, &copyRegion);
		}EndSingleTimeCommands(commandBuffer);
		
		//free staging resources
		vkDestroyBuffer(device, vertexStaging.buffer, nullptr);
		vkFreeMemory(device, vertexStaging.memory, nullptr);
		vkDestroyBuffer(device, indexStaging.buffer, nullptr);
		vkFreeMemory(device, indexStaging.memory, nullptr);
		
		//name buffers for debugging
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)mesh.vertexBuffer,
							 TOSTRING("MeshBrush vertex buffer ", mesh.id, ":", mesh.name).c_str());
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)mesh.indexBuffer,
							 TOSTRING("MeshBrush index buffer ", mesh.id, ":", mesh.name).c_str());
	}
	
	//store mesh brush
	meshBrushes.push_back(mesh);
	return mesh.id;
}

void Render::
UpdateMeshBrushMatrix(u32 index, Matrix4 transform){
	if(index >= meshBrushes.size()) return ERROR_LOC("There is no mesh with id: ", index);
	
	meshBrushes[index].modelMatrix = transform;
	
	UpdateMeshBrushBuffers(index);
}

void Render::
UpdateMeshBrushBuffers(u32 meshBrushIdx){
	if(meshBrushIdx >= meshBrushes.size()) return ERROR_LOC("There is no mesh with id: ", meshBrushIdx);
	
	MeshBrushVk& mesh = meshBrushes[meshBrushIdx];
	StagingBufferVk vertexStaging{}, indexStaging{};
	size_t vbSize = mesh.vertices.size() * sizeof(VertexVk);
	size_t ibSize = mesh.indices.size() * sizeof(u32);
	
	//create host visible vertex and index buffers (CPU/RAM)
	CreateAndMapBuffer(vertexStaging.buffer, vertexStaging.memory, mesh.vertexBufferSize, vbSize, mesh.vertices.data(),
					   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	CreateAndMapBuffer(indexStaging.buffer, indexStaging.memory, mesh.indexBufferSize, ibSize, mesh.indices.data(),
					   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	//create device local buffers (GPU)
	CreateAndMapBuffer(mesh.vertexBuffer, mesh.vertexBufferMemory, mesh.vertexBufferSize, vbSize, nullptr,
					   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
					   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	CreateAndMapBuffer(mesh.indexBuffer, mesh.indexBufferMemory, mesh.indexBufferSize, ibSize, nullptr,
					   VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
					   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	//copy data from staging buffers to device local buffers
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();{
		VkBufferCopy copyRegion{};
		
		copyRegion.size = vbSize;
		vkCmdCopyBuffer(commandBuffer, vertexStaging.buffer, mesh.vertexBuffer, 1, &copyRegion);
		
		copyRegion.size = ibSize;
		vkCmdCopyBuffer(commandBuffer, indexStaging.buffer, mesh.indexBuffer, 1, &copyRegion);
	}EndSingleTimeCommands(commandBuffer);
	
	//free staging resources
	vkDestroyBuffer(device, vertexStaging.buffer, nullptr);
	vkFreeMemory(device, vertexStaging.memory, nullptr);
	vkDestroyBuffer(device, indexStaging.buffer, nullptr);
	vkFreeMemory(device, indexStaging.memory, nullptr);
	
	//name buffers for debugging
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)mesh.vertexBuffer,
						 TOSTRING("MeshBrush vertex buffer ", mesh.id, ":", mesh.name).c_str());
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)mesh.indexBuffer,
						 TOSTRING("MeshBrush index buffer ", mesh.id, ":", mesh.name).c_str());
}

void Render::
RemoveMeshBrush(u32 meshBrushIdx){
	if(meshBrushIdx < meshBrushes.size()){
		for(int i=meshBrushIdx; i<meshBrushes.size(); ++i){ --meshBrushes[i].id; } 
		vkDestroyBuffer(device, meshBrushes[meshBrushIdx].vertexBuffer, nullptr);
		vkFreeMemory(device, meshBrushes[meshBrushIdx].vertexBufferMemory, nullptr);
		vkDestroyBuffer(device, meshBrushes[meshBrushIdx].indexBuffer, nullptr);
		vkFreeMemory(device, meshBrushes[meshBrushIdx].indexBufferMemory, nullptr);
		meshBrushes.erase(meshBrushes.begin() + meshBrushIdx);
	}else{ ERROR_LOC("There is no mesh brush with id: ", meshBrushIdx); }
}


//////////////////
//// 2D stuff ////
//////////////////





///////////////////////
//// trimesh stuff ////
///////////////////////


u32 Render::
LoadBaseMesh(Mesh* m, bool visible){
	PrintVk(3, "    Loading base mesh: ", m->name);
	
	MeshVk mesh;  mesh.base = true; 
	mesh.ptr = m; 
	if(!visible) mesh.visible = false;
	mesh.primitives.reserve(m->batchCount);
	cpystr(mesh.name, m->name, DESHI_NAME_SIZE);
	
	//resize scene vectors
	vertexBuffer.reserve(vertexBuffer.size() + m->vertexCount);
	indexBuffer.reserve(indexBuffer.size() + m->indexCount);
	textures.reserve(textures.size() + m->textureCount);
	materials.reserve(materials.size() + m->batchCount);
	
	u32 batchVertexStart, batchIndexStart;
	u32 matID, albedoID, normalID, lightID, specularID;
	for(Batch& batch : m->batchArray){
		batchVertexStart = (u32)vertexBuffer.size();
		batchIndexStart = (u32)indexBuffer.size();
		
		//vertices
		for(int i=0; i<batch.vertexArray.size(); ++i){ 
			VertexVk vert;
			vert.pos    = vec4(batch.vertexArray[i].pos, 1.0f);
			vert.uv     = vec4(batch.vertexArray[i].uv.x, batch.vertexArray[i].uv.y, 0.0f, 0.0f);
			vert.color  = vec4(batch.vertexArray[i].color, 1.0f);
			vert.normal = vec4(batch.vertexArray[i].normal, 1.0f);
			vertexBuffer.push_back(vert);
		}
		
		//indices
		for(u32 i : batch.indexArray){
			indexBuffer.push_back(batchVertexStart+i);
		}
		
		//material and textures
		albedoID = 0, normalID = 2, lightID = 2, specularID = 2;
		for(int i=0; i<batch.textureArray.size(); ++i){ 
			u32 idx = LoadTexture(batch.textureArray[i]);
			switch(textures[idx].type){
				case TextureType_Albedo:  { albedoID   = idx; }break;
				case TextureType_Normal:  { normalID   = idx; }break;
				case TextureType_Light:   { lightID    = idx; }break;
				case TextureType_Specular:{ specularID = idx; }break;
			}
		}
		if(batch.shader == Shader_Flat){
			matID = CreateMaterial(ShaderStrings[Shader_Flat], Shader_Flat, 0, 0, 0, 0);
		}else if(batch.shader == Shader_Phong){
			matID = CreateMaterial(ShaderStrings[Shader_Phong], Shader_Phong, 0, 0, 0, 0);
		}else if(batch.shader == Shader_Twod){
			matID = CreateMaterial(ShaderStrings[Shader_Twod], Shader_Twod, 0, 0, 0, 0);
		}else{
			matID = CreateMaterial(batch.name, batch.shader, albedoID, normalID, specularID, lightID);
		}
		
		//primitive
		PrimitiveVk primitive;
		primitive.firstIndex = batchIndexStart;
		primitive.indexCount = batch.indexArray.size();
		primitive.materialIndex = matID;
		mesh.primitives.push_back(primitive);
	}
	
	//add mesh to scene
	mesh.id = (u32)meshes.size();
	meshes.push_back(mesh);
	if(initialized){ CreateSceneMeshBuffers(); }
	if(visible) mesh.visible = true;
	return mesh.id;
}

u32 Render::
CreateMesh(Scene* scene, const char* filename, b32 new_material){
	//check if Mesh was already created
	for(auto& model : scene->models){ 
		if(strcmp(model.mesh->name, filename) == 0){ 
			return CreateMesh(model.mesh, Matrix4::IDENTITY, new_material);
		} 
	}
	PrintVk(3, "    Creating mesh: ", filename);
	
	scene->models.emplace_back(Mesh::CreateMeshFromOBJ(filename));
	return CreateMesh(scene->models[scene->models.size()-1].mesh, Matrix4::IDENTITY, new_material);
}

u32 Render::
CreateMesh(Mesh* m, Matrix4 matrix, b32 new_material){
	//check if MeshVk was already created
	for(auto& mesh : meshes){ 
		if(strcmp(mesh.name, m->name) == 0){ 
			return CreateMesh(mesh.id, matrix, new_material);
		} 
	}
	
	PrintVk(3, "    Creating mesh: ", m->name);
	return CreateMesh(LoadBaseMesh(m), matrix, new_material);
}

u32 Render::
CreateMesh(u32 meshID, Matrix4 matrix, b32 new_material){
	if(meshID < meshes.size()){
		PrintVk(3, "    Creating Mesh: ", meshes[meshID].ptr->name);
		MeshVk mesh; mesh.base = false; 
		mesh.ptr = meshes[meshID].ptr; mesh.visible = true;
		mesh.primitives = std::vector(meshes[meshID].primitives);
		if(new_material){
			forI(meshes[meshID].primitives.size()){
				mesh.primitives[i].materialIndex = CopyMaterial(meshes[meshID].primitives[i].materialIndex);
			}
		}
		mesh.modelMatrix = matrix;
		cpystr(mesh.name, meshes[meshID].name, DESHI_NAME_SIZE);
		mesh.id = (u32)meshes.size();
		meshes.push_back(mesh);
		meshes[meshID].children.push_back(mesh.id);
		return mesh.id;
	}
	ERROR("There is no mesh with id: ", meshID);
	return 0xFFFFFFFF;
}

void Render::
UnloadBaseMesh(u32 meshID){
	if(meshID < meshes.size()){
		if(meshes[meshID].base){
			//loop thru children and remove them
			//remove verts and indices
			ERROR_LOC("UnloadBaseMesh: Not implemented yet");
		}else{
			ERROR_LOC("UnloadBaseMesh: Only a base mesh can be unloaded");
		}
	}else{
		ERROR_LOC("UnloadBaseMesh: There is no mesh with id: ", meshID);
	}
}


//TODO(delle,ReCl) this causes a "leak" in that it doesnt remove the materials created with CreateMesh
//NOTE temporarily disabled since it causes a crash b/c we don't update mesh comp's ids
void Render::
RemoveMesh(u32 meshID){
	if(meshID < meshes.size()){
		if(!meshes[meshID].base){
			//for(int i=meshID; i<meshes.size(); ++i){ --meshes[i].id;  } 
			//meshes.erase(meshes.begin() + meshID);
			meshes[meshID].visible = false;
			WARNING_LOC("RemoveMesh: Not implemented yet");
		}else{ ERROR_LOC("Only a child/non-base mesh can be removed"); }
	}else{ ERROR_LOC("There is no mesh with id: ", meshID); }
}

Matrix4 Render::
GetMeshMatrix(u32 meshID){
	if(meshID < meshes.size()){
		return meshes[meshID].modelMatrix;
	}
	ERROR_LOC("There is no mesh with id: ", meshID);
	return Matrix4(0.f);
}

Mesh* Render::
GetMeshPtr(u32 meshID){
	if(meshID < meshes.size()){
		return meshes[meshID].ptr;
	}
	ERROR_LOC("There is no mesh with id: ", meshID);
	return nullptr;
}

u32 Render::
GetBaseMeshID(const char* name){
	forI(meshes.size()){
		if(meshes[i].base && strcmp(name, meshes[i].name) == 0) return i;
	}
	return -1;
}

void Render::
UpdateMeshMatrix(u32 meshID, Matrix4 matrix){
	if(meshID < meshes.size()){
		meshes[meshID].modelMatrix = matrix;
	}else{
		ERROR_LOC("There is no mesh with id: ", meshID);
	}
}

void Render::
TransformMeshMatrix(u32 meshID, Matrix4 transform){
	if(meshID < meshes.size()){
		meshes[meshID].modelMatrix = meshes[meshID].modelMatrix * transform;
	}else{
		ERROR_LOC("There is no mesh with id: ", meshID);
	}
}

void Render::
UpdateMeshBatchMaterial(u32 meshID, u32 batchIndex, u32 matID){
	if(meshID < meshes.size()){
		if(batchIndex < meshes[meshID].primitives.size()){
			if(matID < materials.size()){
				meshes[meshID].primitives[batchIndex].materialIndex = matID;
			}else{ ERROR_LOC("There is no material with id: ", matID); } 
		}else{ ERROR_LOC("There is no batch on the mesh with id: ", batchIndex); }
	}else{ ERROR_LOC("There is no mesh with id: ", meshID); }
}

void Render::
UpdateMeshVisibility(u32 meshID, bool visible){
	if(meshID == -1){
		for(auto& mesh : meshes){ mesh.visible = visible; }
	}else if(meshID < meshes.size()){
		meshes[meshID].visible = visible;
	}else{
		ERROR_LOC("There is no mesh with id: ", meshID);
	}
}

void Render::
UpdateMeshBrushVisibility(u32 meshID, bool visible){
	if(meshID == -1){
		for(auto& mesh : meshBrushes){ mesh.visible = visible; }
	}
	else if(meshID < meshBrushes.size()){
		meshBrushes[meshID].visible = visible;
	}
	else {
		ERROR_LOC("There is no mesh with id: ", meshID);
	}
}

void Render::
AddSelectedMesh(u32 meshID){
	if(meshID < meshes.size()){
		selected.push_back(meshID);
	}else{ ERROR_LOC("There is no mesh with id: ", meshID); }
}

void Render::
RemoveSelectedMesh(u32 meshID){
	if(meshID == -1){ 
		selected.clear(); 
		return; 
	}
	if(meshID < meshes.size()){
		forI(selected.size()){
			if(selected[i] == meshID){
				selected.erase(selected.begin()+i);
				return;
			}
		}
	}else{ ERROR_LOC("There is no mesh with id: ", meshID); }
}

u32 Render::
LoadTexture(const char* filename, u32 type){
	for(auto& tex : textures){ if(strcmp(tex.filename, filename) == 0){ return tex.id; } }
	
	PrintVk(3, "    Loading Texture: ", filename);
	TextureVk tex; 
	cpystr(tex.filename, filename, DESHI_NAME_SIZE);
	
	std::string imagePath = Assets::assetPath(filename, AssetType_Texture);
	if(imagePath == ""){ return 0; }
	tex.pixels = stbi_load(imagePath.c_str(), &tex.width, &tex.height, &tex.channels, STBI_rgb_alpha);
	Assert(tex.pixels != 0, "stb failed to load an image");
	
	tex.type = type;
	tex.mipLevels = (u32)std::floor(std::log2(std::max(tex.width, tex.height))) + 1;
	tex.imageSize = tex.width * tex.height * 4;
	
	//copy the memory to a staging buffer
	StagingBufferVk staging{};
	CreateAndMapBuffer(staging.buffer, staging.memory, tex.imageSize, static_cast<size_t>(tex.imageSize), tex.pixels, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	//copy the staging buffer to the image and generate its mipmaps
	CreateImage(tex.width, tex.height, tex.mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex.image, tex.imageMemory);
	TransitionImageLayout(tex.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, tex.mipLevels);
	CopyBufferToImage(staging.buffer, tex.image, (u32)tex.width, (u32)tex.height);
	GenerateMipmaps(tex.image, VK_FORMAT_R8G8B8A8_SRGB, tex.width, tex.height, tex.mipLevels);
	//image layout set to SHADER_READ_ONLY when generating mipmaps
	tex.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	//cleanup staging memory
	vkDestroyBuffer(device, staging.buffer, nullptr);
	vkFreeMemory(device, staging.memory, nullptr);
	stbi_image_free(tex.pixels); tex.pixels = nullptr;
	
	//create sampler
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; //TODO(delle,ReOp) VK_SAMPLER_MIPMAP_MODE_NEAREST for more performance
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = enabledFeatures.samplerAnisotropy;
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	samplerInfo.maxAnisotropy = enabledFeatures.samplerAnisotropy ?  properties.limits.maxSamplerAnisotropy : 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = f32(tex.mipLevels);
	AssertVk(vkCreateSampler(device, &samplerInfo, nullptr, &tex.sampler), "failed to create texture sampler");
	
	//create image view
	tex.view = CreateImageView(tex.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, tex.mipLevels);
	
	//fill descriptor image info
	tex.imageInfo.imageView = tex.view;
	tex.imageInfo.sampler = tex.sampler;
	tex.imageInfo.imageLayout = tex.layout;
	
	//name image, image view, and sampler for debugging
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE,(u64)tex.image, TOSTRING("Texture image ", filename).c_str());
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)tex.view, TOSTRING("Texture imageview ", filename).c_str());
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SAMPLER, (u64)tex.sampler, TOSTRING("Texture sampler ", filename).c_str());
	
	//add the texture to the scene and return its index
	u32 idx = (u32)textures.size();
	tex.id = idx;
	textures.push_back(tex);
	return idx;
}

u32 Render::
LoadTexture(Texture texture){
	for(auto& tex : textures){ if(strcmp(tex.filename, texture.filename) == 0){ return tex.id; } }
	return LoadTexture(texture.filename, texture.type);
}

std::string Render::
ListTextures(){
	std::string out = "[c:yellow]ID  Filename  Width  Height  Depth  Type[c]\n";
	for(auto& tex : textures){
		if(tex.id < 10){
			out += TOSTRING(" ", tex.id, "  ", tex.filename, "  ", tex.width, "  ", tex.height, "  ", tex.channels, "  ", tex.type, "\n");
		}else{
			out += TOSTRING(tex.id, "  ", tex.filename, "  ", tex.width, "  ", tex.height, "  ", tex.channels, "  ", tex.type, "\n");
		}
	}
	return out;
}

//TODO(delle,ReVu) dont create image samplers for non-image materials
u32 Render::
CreateMaterial(const char* name, u32 shader, u32 albedoTextureID, u32 normalTextureID, u32 specTextureID, u32 lightTextureID){
	if(!name){ ERROR("No name passed on material creation"); return 0; }
	for(auto& mat : materials){ //avoid duplicate if not PBR
		if(strcmp(mat.name, name) == 0 && mat.shader == shader && shader != Shader_PBR){ 
			return mat.id; 
		} 
	}
	
	PrintVk(3, "    Creating material: ", name);
	MaterialVk mat; mat.id = (u32)materials.size();
	mat.shader     = shader;          mat.pipeline = GetPipelineFromShader(shader);
	mat.albedoID   = albedoTextureID; mat.normalID = normalTextureID;
	mat.specularID = specTextureID;   mat.lightID  = lightTextureID;
	cpystr(mat.name, name, DESHI_NAME_SIZE);
	
	//allocate and write descriptor set for material
	VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.pSetLayouts = &descriptorSetLayouts.textures;
	allocInfo.descriptorSetCount = 1;
	AssertVk(vkAllocateDescriptorSets(device, &allocInfo, &mat.descriptorSet), "failed to allocate materials descriptor sets");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)mat.descriptorSet,
						 TOSTRING("Material descriptor set ",mat.id,":",mat.name).c_str());
	
	VkWriteDescriptorSet writeDescriptorSets[4]{};
	writeDescriptorSets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].dstSet          = mat.descriptorSet;
	writeDescriptorSets[0].dstArrayElement = 0;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[0].pImageInfo      = &textures[mat.albedoID].imageInfo;
	writeDescriptorSets[0].dstBinding      = 0;
	writeDescriptorSets[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[1].dstSet          = mat.descriptorSet;
	writeDescriptorSets[1].dstArrayElement = 0;
	writeDescriptorSets[1].descriptorCount = 1;
	writeDescriptorSets[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[1].pImageInfo      = &textures[mat.normalID].imageInfo;
	writeDescriptorSets[1].dstBinding      = 1;
	writeDescriptorSets[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[2].dstSet          = mat.descriptorSet;
	writeDescriptorSets[2].dstArrayElement = 0;
	writeDescriptorSets[2].descriptorCount = 1;
	writeDescriptorSets[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[2].pImageInfo      = &textures[mat.specularID].imageInfo;
	writeDescriptorSets[2].dstBinding      = 2;
	writeDescriptorSets[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[3].dstSet          = mat.descriptorSet;
	writeDescriptorSets[3].dstArrayElement = 0;
	writeDescriptorSets[3].descriptorCount = 1;
	writeDescriptorSets[3].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[3].pImageInfo      = &textures[mat.lightID].imageInfo;
	writeDescriptorSets[3].dstBinding      = 3;
	
	vkUpdateDescriptorSets(device, 4, writeDescriptorSets, 0, nullptr);
	
	//add to scene
	materials.push_back(mat);
	return mat.id;
}

//TODO(delle,Vu) https://renderdoc.org/vkspec_chunked/chap15.html#VkCopyDescriptorSet
u32 Render::
CopyMaterial(u32 materialID){
	if(materialID >= materials.size()){ ERROR("Invalid material ID passed to CopyMaterial"); return 0; }
	
	MaterialVk mat = materials[materialID];
	PrintVk(3, "    Copying material: ", mat.name);
	mat.id = (u32)materials.size();
	
	//allocate and write descriptor set for material
	VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	allocInfo.descriptorPool     = descriptorPool;
	allocInfo.pSetLayouts        = &descriptorSetLayouts.textures;
	allocInfo.descriptorSetCount = 1;
	AssertVk(vkAllocateDescriptorSets(device, &allocInfo, &mat.descriptorSet), "failed to allocate materials descriptor sets");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)mat.descriptorSet,
						 TOSTRING("Material descriptor set ",mat.id,":",mat.name).c_str());
	
	VkWriteDescriptorSet writeDescriptorSets[4]{};
	writeDescriptorSets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].dstSet          = mat.descriptorSet;
	writeDescriptorSets[0].dstArrayElement = 0;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[0].pImageInfo      = &textures[mat.albedoID].imageInfo;
	writeDescriptorSets[0].dstBinding      = 0;
	writeDescriptorSets[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[1].dstSet          = mat.descriptorSet;
	writeDescriptorSets[1].dstArrayElement = 0;
	writeDescriptorSets[1].descriptorCount = 1;
	writeDescriptorSets[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[1].pImageInfo      = &textures[mat.normalID].imageInfo;
	writeDescriptorSets[1].dstBinding      = 1;
	writeDescriptorSets[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[2].dstSet          = mat.descriptorSet;
	writeDescriptorSets[2].dstArrayElement = 0;
	writeDescriptorSets[2].descriptorCount = 1;
	writeDescriptorSets[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[2].pImageInfo      = &textures[mat.specularID].imageInfo;
	writeDescriptorSets[2].dstBinding      = 2;
	writeDescriptorSets[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[3].dstSet          = mat.descriptorSet;
	writeDescriptorSets[3].dstArrayElement = 0;
	writeDescriptorSets[3].descriptorCount = 1;
	writeDescriptorSets[3].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[3].pImageInfo      = &textures[mat.lightID].imageInfo;
	writeDescriptorSets[3].dstBinding      = 3;
	
	vkUpdateDescriptorSets(device, 4, writeDescriptorSets, 0, nullptr);
	
	//add to scene
	materials.push_back(mat);
	return mat.id;
}

void Render::
UpdateMaterialTexture(u32 matID, u32 texType, u32 texID){
	if(matID < materials.size() && texID < textures.size()){
		VkWriteDescriptorSet writeDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		writeDescriptorSet.dstSet          = materials[matID].descriptorSet;
		writeDescriptorSet.dstArrayElement = 0;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		
		switch(texType){
			case(TextureType_Albedo):{ 
				materials[matID].albedoID = texID; 
				writeDescriptorSet.pImageInfo = &textures[materials[matID].albedoID].imageInfo;
				writeDescriptorSet.dstBinding = 0;
			} break;
			case(TextureType_Normal):{ 
				materials[matID].normalID = texID; 
				writeDescriptorSet.pImageInfo = &textures[materials[matID].normalID].imageInfo;
				writeDescriptorSet.dstBinding = 1;
			} break;
			case(TextureType_Specular):{ 
				materials[matID].specularID = texID; 
				writeDescriptorSet.pImageInfo = &textures[materials[matID].specularID].imageInfo;
				writeDescriptorSet.dstBinding = 2;
			}  break;
			case(TextureType_Light):{ 
				materials[matID].lightID = texID; 
				writeDescriptorSet.pImageInfo = &textures[materials[matID].lightID].imageInfo;
				writeDescriptorSet.dstBinding = 3;
			}  break;
			default:{ return; }
		}
		
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
	}
}

void Render::
UpdateMaterialShader(u32 matID, u32 shader){
	if(matID == 0xFFFFFFFF){
		for(auto& mat : materials){ mat.pipeline = GetPipelineFromShader(shader); }
	}else if(matID < materials.size()){
		materials[matID].pipeline = GetPipelineFromShader(shader);
		materials[matID].shader = shader;
	}else{
		ERROR_LOC("There is no material with id: ", matID);
	}
}

std::vector<u32> Render::
GetMaterialIDs(u32 meshID){
	if(meshID < meshes.size()){
		MeshVk* m = &meshes[meshID];
		std::vector<u32> out; out.resize(m->primitives.size());
		for(auto& a : m->primitives){
			out.push_back(a.materialIndex);
		}
		return out;
	}
	ERROR_LOC("There is no mesh with id: ", meshID);
	return std::vector<u32>();
}

//TODO(delle,Vu) this leaks in the GPU b/c the descriptor sets are not deallocated, figure that out
void Render::
RemoveMaterial(u32 matID){
	if(matID >= materials.size()) return ERROR_LOC("RemoveMaterial: There is no material with id: ", matID);
	for(MeshVk& mesh : meshes){
		for(PrimitiveVk& prim : mesh.primitives){
			if(prim.materialIndex == matID) prim.materialIndex = 0;
			if(prim.materialIndex > matID) prim.materialIndex -= 1;
		}
	}
	for(int i=matID; i < materials.size(); ++i){
		materials[i].id -= 1;
	}
	materials.erase(materials.begin()+matID);
}

//ref: gltfscenerendering.cpp:350
void Render::
LoadScene(Scene* sc){
	PrintVk(2, "  Loading Scene");
	initialized = false;
	
	//load meshes, materials, and textures
	for(Model& model : sc->models){ LoadBaseMesh(model.mesh); }
	
	CreateSceneMeshBuffers();
	initialized = true;
}

void Render::
UpdateCameraPosition(Vector3 position){
	uboVS.values.viewPos = vec4(position, 1.f);
}

void Render::
UpdateCameraViewMatrix(Matrix4 m){
	uboVS.values.view = m;
}

void Render::
UpdateCameraProjectionMatrix(Matrix4 m){
	uboVS.values.proj = m;
}

pair<Vector3, Vector3> Render::
SceneBoundingBox(){
	float inf = std::numeric_limits<float>::max();
	Vector3 max(-inf, -inf, -inf);
	Vector3 min( inf,  inf,  inf);
	
	Vector3 v;
	for(MeshVk& mesh : meshes){
		for(PrimitiveVk& p : mesh.primitives){
			for(int i = p.firstIndex; i < p.indexCount; i++){
				v = vertexBuffer[indexBuffer[i]].pos.ToVector3() + mesh.modelMatrix.Translation();
				if      (v.x < min.x){ min.x = v.x; }
				else if(v.x > max.x){ max.x = v.x; }
				if      (v.y < min.y){ min.y = v.y; }
				else if(v.y > max.y){ max.y = v.y; }
				if      (v.z < min.z){ min.z = v.z; }
				else if(v.z > max.z){ max.z = v.z; }
			}
		}
	}
	
	return pair<Vector3, Vector3>(max, min);
}

void Render::
ReloadShader(u32 shader){
	switch(shader){
		case(Shader_Flat):{ 
			vkDestroyPipeline(device, pipelines.flat, nullptr);
			shaderStages[0] = CompileAndLoadShader("flat.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("flat.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.flat), "failed to create flat graphics pipeline");
		}break;
		case(Shader_Wireframe):{
			if(deviceFeatures.fillModeNonSolid){
				vkDestroyPipeline(device, pipelines.wireframe, nullptr);
				rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
				rasterizationState.cullMode = VK_CULL_MODE_NONE;
				depthStencilState.depthTestEnable = VK_FALSE;
				shaderStages[0] = CompileAndLoadShader("wireframe.vert", VK_SHADER_STAGE_VERTEX_BIT);
				shaderStages[1] = CompileAndLoadShader("wireframe.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
				pipelineCreateInfo.stageCount = 2;
				AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.wireframe), "failed to create wireframe graphics pipeline");
				rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
				rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
				depthStencilState.depthTestEnable = VK_TRUE;
			}
		}break;
		case(Shader_Phong):{
			vkDestroyPipeline(device, pipelines.phong, nullptr);
			shaderStages[0] = CompileAndLoadShader("phong.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("phong.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.phong), "failed to create phong graphics pipeline");
		}break;
		case(Shader_Twod):{
			vkDestroyPipeline(device, pipelines.twod, nullptr);
			shaderStages[0] = CompileAndLoadShader("twod.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("twod.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.twod), "failed to create twod graphics pipeline");
		}break;
		case(Shader_PBR):{ 
			vkDestroyPipeline(device, pipelines.pbr, nullptr);
			shaderStages[0] = CompileAndLoadShader("pbr.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("pbr.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.pbr), "failed to create pbr graphics pipeline");
		}break;
		case(Shader_Lavalamp):{ 
			vkDestroyPipeline(device, pipelines.lavalamp, nullptr);
			shaderStages[0] = CompileAndLoadShader("lavalamp.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("lavalamp.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.lavalamp), "failed to create lavalamp graphics pipeline");
		}break;
		case(Shader_Testing0):{ 
			vkDestroyPipeline(device, pipelines.testing0, nullptr);
			shaderStages[0] = CompileAndLoadShader("testing0.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("testing0.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.testing0), "failed to create testing0 graphics pipeline");
		}break;
		case(Shader_Testing1):{ 
			vkDestroyPipeline(device, pipelines.testing1, nullptr);
			shaderStages[0] = CompileAndLoadShader("testing1.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("testing1.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.testing1), "failed to create testing1 graphics pipeline");
		}break;
		default:{
			ReloadAllShaders();
		}break;
	}
	UpdateMaterialPipelines();
}

void Render::
ReloadAllShaders(){
	CompileAllShaders();
	remakePipelines = true;
}

void Render::
UpdateRenderSettings(RenderSettings new_settings){
	settings = new_settings;
};

std::vector<VertexVk>* Render::
vertexArray(){
	return &vertexBuffer;
}

std::vector<u32>* Render::
indexArray(){
	return &indexBuffer;
}

std::vector<TextureVk>* Render::
textureArray(){
	return &textures;
}
std::vector<MeshVk>* Render::
meshArray(){
	return &meshes;
}

std::vector<MaterialVk>* Render::
materialArray(){
	return &materials;
}

std::vector<MeshBrushVk>* Render::
meshBrushArray(){
	return &meshBrushes;
}

std::vector<u32>* Render::
selectedArray(){
	return &selected;
}

vec4* Render::
lightArray(){
	return lights;
}

u32 Render::
MeshCount(){
	return meshes.size();
}

b32 Render::
IsBaseMesh(u32 meshIdx){
	return meshes[meshIdx].base;
}

char* Render::
MeshName(u32 meshIdx){
	return meshes[meshIdx].name;
}

void Render::
UpdateLight(u32 lightIdx, Vector4 vec){
	lights[lightIdx] = vec;
}

u32 Render::
TextureCount(){
	return textures.size();
}

u32 Render::
MaterialCount(){
	return materials.size();
}

u32 Render::
MeshBrushCount(){
	return meshBrushes.size();
}

b32 Render::
IsMeshVisible(u32 meshIdx){
	return meshes[meshIdx].visible;
}

char* Render::
MaterialName(u32 matIdx){
	return materials[matIdx].name;
}

char* Render::
TextureName(u32 texIdx){
	return textures[texIdx].filename;
}