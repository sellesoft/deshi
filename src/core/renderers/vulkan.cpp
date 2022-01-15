//-------------------------------------------------------------------------------------------------
// @VULKAN STRUCTS
struct MeshVk{
	Mesh* base;
	u32 vtxOffset;
	u32 vtxCount;
	u32 idxOffset;
	u32 idxCount;
};

struct TextureVk {
	Texture*       base;
	VkImage        image;
	VkDeviceMemory memory;
	VkDeviceSize   size;
	VkImageView    view;
	VkSampler      sampler;
	VkDescriptorImageInfo descriptor;
	VkDescriptorSet descriptorSet;
};

struct MaterialVk{
	Material*       base;
	VkDescriptorSet descriptorSet;
	VkPipeline      pipeline;
};

struct ModelCmdVk{
	u32   vertexOffset;
	u32   indexOffset;
	u32   indexCount;
	u32   material;
	char* name;
	mat4  matrix;
};

struct Push2DVk{
	vec2 scale;
	vec2 translate;
	s32 font_offset;
};

struct TwodCmdVk{
	VkDescriptorSet descriptorSet;
	u16  indexOffset;
	u16  indexCount;
	vec2 scissorOffset;
	vec2 scissorExtent;
	b32  textured;
};

struct QueueFamilyIndices{
	optional<u32> graphicsFamily;
	optional<u32> presentFamily;
	bool isComplete(){ return graphicsFamily.test() && presentFamily.test(); }
};

struct SwapChainSupportDetails{
	VkSurfaceCapabilitiesKHR  capabilities;
	array<VkSurfaceFormatKHR> formats;
	array<VkPresentModeKHR>   presentModes;
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

struct BufferVk{
	VkBuffer               buffer;
	VkDeviceMemory         memory;
	VkDeviceSize           size; //size of data, not allocation
	VkDescriptorBufferInfo descriptor;
};


//-------------------------------------------------------------------------------------------------
// @INTERFACE VARIABLES
local RenderSettings settings;
local ConfigMap configMap = {
	{"#render settings config file",0,0},
	
	{"\n#    //// REQUIRES RESTART ////",  ConfigValueType_PADSECTION,(void*)21},
	{"debugging",            ConfigValueType_Bool, &settings.debugging},
	{"printf",               ConfigValueType_Bool, &settings.printf},
	{"texture_filtering",    ConfigValueType_Bool, &settings.textureFiltering},
	{"anistropic_filtering", ConfigValueType_Bool, &settings.anistropicFiltering},
	{"msaa_level",           ConfigValueType_U32,  &settings.msaaSamples},
	{"recompile_all_shaders",        ConfigValueType_Bool, &settings.recompileAllShaders},
	
	{"\n#    //// RUNTIME VARIABLES ////", ConfigValueType_PADSECTION,(void*)15},
	{"logging_level",  ConfigValueType_U32,  &settings.loggingLevel},
	{"crash_on_error", ConfigValueType_Bool, &settings.crashOnError},
	{"vsync_type",     ConfigValueType_U32,  &settings.vsync},
	
	{"\n#shaders",                         ConfigValueType_PADSECTION,(void*)17},
	{"optimize_shaders", ConfigValueType_Bool, &settings.optimizeShaders},
	
	{"\n#shadows",                         ConfigValueType_PADSECTION,(void*)20},
	{"shadow_pcf",          ConfigValueType_Bool, &settings.shadowPCF},
	{"shadow_resolution",   ConfigValueType_U32,  &settings.shadowResolution},
	{"shadow_nearz",        ConfigValueType_F32,  &settings.shadowNearZ},
	{"shadow_farz",         ConfigValueType_F32,  &settings.shadowFarZ},
	{"depth_bias_constant", ConfigValueType_F32,  &settings.depthBiasConstant},
	{"depth_bias_slope",    ConfigValueType_F32,  &settings.depthBiasSlope},
	{"show_shadow_map",     ConfigValueType_Bool, &settings.showShadowMap},
	
	{"\n#colors",                          ConfigValueType_PADSECTION,(void*)15},
	{"clear_color",    ConfigValueType_FV4, &settings.clearColor},
	{"selected_color", ConfigValueType_FV4, &settings.selectedColor},
	{"collider_color", ConfigValueType_FV4, &settings.colliderColor},
	
	{"\n#filters",                         ConfigValueType_PADSECTION,(void*)15},
	{"wireframe_only", ConfigValueType_Bool, &settings.wireframeOnly},
	
	{"\n#overlays",                        ConfigValueType_PADSECTION,(void*)17},
	{"mesh_wireframes",  ConfigValueType_Bool, &settings.meshWireframes},
	{"mesh_normals",     ConfigValueType_Bool, &settings.meshNormals},
	{"light_frustrums",  ConfigValueType_Bool, &settings.lightFrustrums},
	{"temp_mesh_on_top", ConfigValueType_Bool, &settings.tempMeshOnTop},
};

local RenderStats   stats{};
local RendererStage rendererStage = RENDERERSTAGE_NONE;


//-------------------------------------------------------------------------------------------------
// @VULKAN VARIABLES
#define INDEX_TYPE_VK_UI   VK_INDEX_TYPE_UINT32
#define INDEX_TYPE_VK_TEMP VK_INDEX_TYPE_UINT16
#define INDEX_TYPE_VK_MESH VK_INDEX_TYPE_UINT32

//arbitray limits, change if needed
#define MAX_TWOD_VERTICES  0xFFFFF //max u16: 65535
#define MAX_TWOD_INDICES   3*MAX_TWOD_VERTICES
#define MAX_TWOD_CMDS      1000
#define TWOD_LAYERS        11
typedef u32 TwodIndexVk; //if you change this make sure to change whats passed in the vkCmdBindIndexBuffer as well
local TwodIndexVk twodVertexCount = 0;
local TwodIndexVk twodIndexCount  = 0;
local Vertex2     twodVertexArray[MAX_TWOD_VERTICES];
local TwodIndexVk twodIndexArray [MAX_TWOD_INDICES];
local TwodIndexVk twodCmdCounts[TWOD_LAYERS]; //start with 1
local TwodCmdVk   twodCmdArrays[TWOD_LAYERS][MAX_TWOD_CMDS]; //different UI cmd per texture

#define MAX_TEMP_VERTICES 0xFFFF //max u16: 65535
#define MAX_TEMP_INDICES 3*MAX_TEMP_VERTICES
typedef u16 TempIndexVk;
local TempIndexVk  tempWireframeVertexCount = 0;
local TempIndexVk  tempWireframeIndexCount  = 0;
local Mesh::Vertex tempWireframeVertexArray[MAX_TEMP_VERTICES];
local TempIndexVk  tempWireframeIndexArray [MAX_TEMP_INDICES];
local TempIndexVk  tempFilledVertexCount    = 0;
local TempIndexVk  tempFilledIndexCount     = 0;
local Mesh::Vertex tempFilledVertexArray   [MAX_TEMP_VERTICES];
local TempIndexVk  tempFilledIndexArray    [MAX_TEMP_INDICES];
local TempIndexVk  debugWireframeVertexCount = 0;
local TempIndexVk  debugWireframeIndexCount  = 0;
local Mesh::Vertex debugWireframeVertexArray[MAX_TEMP_VERTICES];
local TempIndexVk  debugWireframeIndexArray [MAX_TEMP_INDICES];
local TempIndexVk  debugFilledVertexCount    = 0;
local TempIndexVk  debugFilledIndexCount     = 0;
local Mesh::Vertex debugFilledVertexArray   [MAX_TEMP_VERTICES];
local TempIndexVk  debugFilledIndexArray    [MAX_TEMP_INDICES];

#define MAX_MODEL_CMDS 10000 
typedef u32 ModelIndexVk;
local ModelIndexVk modelCmdCount = 0;
local ModelCmdVk   modelCmdArray[MAX_MODEL_CMDS];

local array<MeshVk>      vkMeshes(deshi_allocator);
local array<TextureVk>   textures(deshi_allocator);
local array<MaterialVk>  vkMaterials(deshi_allocator);
local vec4 vkLights[10]{ vec4(0,0,0,-1) };

local const char* validationLayers[] = {
	"VK_LAYER_KHRONOS_validation"
};
local std::vector<const char*> deviceExtensions = { 
	VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
	VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME 
};
local array<VkValidationFeatureEnableEXT> validationFeaturesEnabled;

local bool initialized      = false;
local bool remakeWindow     = false;
local bool remakePipelines  = false;
local bool _remakeOffscreen = false;

local VkSampleCountFlagBits msaaSamples{};

local VkDebugUtilsMessageSeverityFlagsEXT callbackSeverities = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
local VkDebugUtilsMessageTypeFlagsEXT     callbackTypes      = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

/////////////////////////
//// @initialization ////
/////////////////////////
local VkAllocationCallbacks    allocator_            = {};
local VkAllocationCallbacks*   allocator             = &allocator_;
local VkAllocationCallbacks    temp_allocator_       = {};
local VkAllocationCallbacks*   temp_allocator        = &temp_allocator_;
local VkInstance               instance              = VK_NULL_HANDLE;
local VkDebugUtilsMessengerEXT debugMessenger        = VK_NULL_HANDLE;
local VkSurfaceKHR             surface;
local VkSurfaceKHR             surface2;
local VkPhysicalDevice         physicalDevice        = VK_NULL_HANDLE;
local VkPhysicalDeviceProperties physicalDeviceProperties{};
local VkSampleCountFlagBits    maxMsaaSamples        = VK_SAMPLE_COUNT_1_BIT;
local VkPhysicalDeviceFeatures deviceFeatures        = {};
local VkPhysicalDeviceFeatures enabledFeatures       = {};
local QueueFamilyIndices       physicalQueueFamilies = {};
local VkDevice                 device                = VK_NULL_HANDLE;
local VkQueue                  graphicsQueue         = VK_NULL_HANDLE;
local VkQueue                  presentQueue          = VK_NULL_HANDLE; 
local VkDeviceSize             bufferMemoryAlignment = 256;

////////////////////
//// @swapchain ////
////////////////////
local s32                     width          = 0;
local s32                     height         = 0;
local VkSwapchainKHR          swapchain      = VK_NULL_HANDLE;
local SwapChainSupportDetails supportDetails = {};
local VkSurfaceFormatKHR      surfaceFormat  = {};
local VkPresentModeKHR        presentMode    = VK_PRESENT_MODE_FIFO_KHR;
local VkExtent2D              extent         = {};
local s32                     minImageCount  = 0;

/////////////////////
//// @renderpass ////
/////////////////////
local VkRenderPass baseRenderPass = VK_NULL_HANDLE;
local VkRenderPass msaaRenderPass = VK_NULL_HANDLE;
local VkRenderPass renderPass = VK_NULL_HANDLE;

/////////////////
//// @frames ////
/////////////////
local u32 imageCount = 0;
local u32 frameIndex = 0;
local std::vector<FrameVk> frames;
local FramebufferAttachmentsVk attachments{};
local VkSemaphore   imageAcquiredSemaphore  = VK_NULL_HANDLE;
local VkSemaphore   renderCompleteSemaphore = VK_NULL_HANDLE;
local VkCommandPool commandPool = VK_NULL_HANDLE;

//////////////////
//// @buffers ////
//////////////////
local struct{ //uniform buffer for the vertex shaders
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
		mat4 lightVP;     //first light's view projection matrix
		bool enablePCF;   //whether to blur shadow edges //TODOf(delle,ReVu) convert to specialization constant
	} values;
} uboVS{};

local struct{ //uniform buffer for the geometry shaders
	VkBuffer               buffer;
	VkDeviceMemory         bufferMemory;
	VkDeviceSize           bufferSize;
	VkDescriptorBufferInfo bufferDescriptor;
	
	struct{
		mat4 view; //camera view matrix
		mat4 proj; //camera projection matrix
	} values;
} uboGS{};

local struct{
	VkBuffer               buffer;
	VkDeviceMemory         bufferMemory;
	VkDeviceSize           bufferSize;
	VkDescriptorBufferInfo bufferDescriptor;
	
	struct{
		mat4 lightVP;
	} values;
} uboVSoffscreen{};

local BufferVk meshVertexBuffer{};
local BufferVk meshIndexBuffer{};
local BufferVk uiVertexBuffer{};
local BufferVk uiIndexBuffer{};
local BufferVk tempVertexBuffer{};
local BufferVk tempIndexBuffer{};
local BufferVk debugVertexBuffer{};
local BufferVk debugIndexBuffer{};

////////////////////
//// @pipelines ////
////////////////////
local struct{
	VkDescriptorSetLayout base;
	VkDescriptorSetLayout textures;
	VkDescriptorSetLayout instances;
	VkDescriptorSetLayout twod;
	VkDescriptorSetLayout geometry;
} descriptorSetLayouts{};

local VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
local struct{
	VkDescriptorSet base;
	VkDescriptorSet offscreen;
	VkDescriptorSet ui;
	VkDescriptorSet geometry;
	VkDescriptorSet shadowMap_debug;
} descriptorSets{};

local struct{
	VkPipelineLayout base;
	VkPipelineLayout twod;
	VkPipelineLayout geometry;
} pipelineLayouts{};

local VkPipelineCache  pipelineCache  = VK_NULL_HANDLE;
local VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
local VkPipelineRasterizationStateCreateInfo rasterizationState{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
local VkPipelineColorBlendAttachmentState    colorBlendAttachmentState{};
local VkPipelineColorBlendStateCreateInfo    colorBlendState{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
local VkPipelineDepthStencilStateCreateInfo  depthStencilState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
local VkPipelineViewportStateCreateInfo      viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
local VkPipelineMultisampleStateCreateInfo   multisampleState{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
local VkPipelineVertexInputStateCreateInfo   vertexInputState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
local VkPipelineVertexInputStateCreateInfo   twodVertexInputState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
local VkPipelineDynamicStateCreateInfo       dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
local VkGraphicsPipelineCreateInfo           pipelineCreateInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
local std::vector<VkDynamicState>                    dynamicStates;
local std::vector<VkVertexInputBindingDescription>   vertexInputBindings;
local std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
local std::vector<VkVertexInputBindingDescription>   twodVertexInputBindings;
local std::vector<VkVertexInputAttributeDescription> twodVertexInputAttributes;

local struct{ //pipelines
	union{
		VkPipeline array[16];
		struct{
			//game shaders
			VkPipeline null;
			VkPipeline flat;
			VkPipeline phong;
			VkPipeline pbr;
			VkPipeline lavalamp;
			VkPipeline twod;
			VkPipeline ui;
			
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

//////////////////
//// @shaders ////
//////////////////
local VkPipelineShaderStageCreateInfo shaderStages[6];
local std::vector<pair<std::string, VkShaderModule>> shaderModules;

/////////////////
//// @other  //// 
/////////////////
local struct{ //TODO(delle,Vu) distribute these variables around
	s32 width, height;
	VkImage               depthImage;
	VkDeviceMemory        depthImageMemory;
	VkImageView           depthImageView;
	VkSampler             depthSampler;
	VkDescriptorImageInfo depthDescriptor;
	VkRenderPass          renderpass;
	VkFramebuffer         framebuffer;
} offscreen{};


//-------------------------------------------------------------------------------------------------
// @VULKAN FUNCTIONS
////////////////////
//// @utilities ////
////////////////////
#define AssertVk(result, ...) Assert((result) == VK_SUCCESS)
#define AssertRS(stages, ...) Assert((rendererStage & (stages)) == (stages))

template<typename... Args>
local inline void
PrintVk(u32 level, Args... args){
	if(settings.loggingLevel >= level){
		Log("vulkan", args...);
	}
}

PFN_vkCmdBeginDebugUtilsLabelEXT func_vkCmdBeginDebugUtilsLabelEXT;
local inline void 
DebugBeginLabelVk(VkCommandBuffer command_buffer, const char* label_name, vec4 color){
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
local inline void 
DebugEndLabelVk(VkCommandBuffer command_buffer){
#ifdef DESHI_INTERNAL
	func_vkCmdEndDebugUtilsLabelEXT(command_buffer);
#endif //DESHI_INTERNAL
}

PFN_vkCmdInsertDebugUtilsLabelEXT func_vkCmdInsertDebugUtilsLabelEXT;
local inline void 
DebugInsertLabelVk(VkCommandBuffer command_buffer, const char* label_name, vec4 color){
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
local inline void 
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
local VkCommandBuffer
BeginSingleTimeCommands(){
	AssertRS(RSVK_COMMANDPOOL, "BeginSingleTimeCommands called before CreateCommandPool");
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
local void
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

local VKAPI_ATTR VkBool32 VKAPI_CALL 
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
			  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData){
	switch(messageSeverity){
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:{
			LogE("vulkan",pCallbackData->pMessage); 
			if(settings.crashOnError) Assert(!"crashing because of error in vulkan");
		}break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:{
			LogW("vulkan",pCallbackData->pMessage); 
		}break;
		default:{
			PrintVk(5, pCallbackData->pMessage);
		}break;
	}
	return VK_FALSE;
}


////////////////////////
//// @memory/images ////
////////////////////////
//finds which memory types the graphics card offers
local u32
FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties){
	PrintVk(4,"Finding memory types");
	AssertRS(RSVK_PHYSICALDEVICE, "FindMemoryType called before PickPhysicalDevice");
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
local VkImageView
CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 mipLevels){
	PrintVk(4, "Creating image view");
	AssertRS(RSVK_LOGICALDEVICE, "CreateImageView called before CreateLogicalDevice");
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
local void 
CreateImage(u32 width, u32 height, u32 mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory){
	PrintVk(4,"Creating image");
	AssertRS(RSVK_LOGICALDEVICE, "CreateImage called before CreateLogicalDevice");
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
local void 
TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels){
	PrintVk(4,"Transitioning image layout");
	AssertRS(RSVK_LOGICALDEVICE, "TransitionImageLayout called before CreateLogicalDevice");
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
local void 
GenerateMipmaps(VkImage image, VkFormat imageFormat, s32 texWidth, s32 texHeight, u32 mipLevels){
	PrintVk(4,"Creating image mipmaps");
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
		blit.dstOffsets[1] = { (mipWidth > 1) ? mipWidth / 2 : 1, (mipHeight > 1) ? mipHeight / 2 : 1, 1 };
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
		
		if(mipWidth  > 1) mipWidth  /= 2;
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
local void 
CreateOrResizeBuffer(VkBuffer& buffer, VkDeviceMemory& buffer_memory, VkDeviceSize& buffer_size, size_t new_size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){
	VkBuffer old_buffer = buffer; buffer = VK_NULL_HANDLE;
	VkDeviceMemory old_buffer_memory = buffer_memory; buffer_memory = VK_NULL_HANDLE;
	
	VkDeviceSize aligned_buffer_size = RoundUpTo(new_size, bufferMemoryAlignment);
	VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.size        = aligned_buffer_size;
	bufferInfo.usage       = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	AssertVk(vkCreateBuffer(device, &bufferInfo, allocator, &buffer));
	
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(device, buffer, &req);
	bufferMemoryAlignment = (bufferMemoryAlignment > req.alignment) ? bufferMemoryAlignment : req.alignment;
	VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.allocationSize  = req.size;
	allocInfo.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, properties);
	AssertVk(vkAllocateMemory(device, &allocInfo, allocator, &buffer_memory));
	AssertVk(vkBindBufferMemory(device, buffer, buffer_memory, 0));
	
	if(buffer_size){
		void* old_buffer_data; void* new_buffer_data;
		AssertVk(vkMapMemory(device, old_buffer_memory, 0, buffer_size, 0, &old_buffer_data));
		AssertVk(vkMapMemory(device, buffer_memory,     0, new_size,    0, &new_buffer_data));
		
		memcpy(new_buffer_data, old_buffer_data, buffer_size);
		
		VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
		range.memory = buffer_memory;
		range.offset = 0;
		range.size   = VK_WHOLE_SIZE;
		AssertVk(vkFlushMappedMemoryRanges(device, 1, &range));
		
		vkUnmapMemory(device, old_buffer_memory);
		vkUnmapMemory(device, buffer_memory);
	}
	
	//delete old buffer
	if(old_buffer        != VK_NULL_HANDLE) vkDestroyBuffer(device, old_buffer, allocator); 
	if(old_buffer_memory != VK_NULL_HANDLE) vkFreeMemory(device, old_buffer_memory, allocator); 
	
	buffer_size = new_size;
}

local void 
CreateOrResizeBuffer(BufferVk* buffer, size_t new_size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){
	CreateOrResizeBuffer(buffer->buffer, buffer->memory, buffer->size, new_size, usage, properties);
}

//creates a buffer and maps provided data to it
local void 
CreateAndMapBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize& bufferSize, size_t newSize, void* data, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){
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
local void 
CopyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height){
	PrintVk(4,"Copying buffer to image");
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
local void 
CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size){
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();{
		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	}EndSingleTimeCommands(commandBuffer);
}


/////////////////////////
//// @initialization ////
/////////////////////////
local void 
SetupAllocator(){
	//!ref: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkAllocationCallbacks.html
	PrintVk(2,"Setting up vulkan allocator");
	Assert(rendererStage == RENDERERSTAGE_NONE, "renderer stage was not NONE at SetupAllocator");
	
	//regular allocator
	auto deshi_vulkan_allocation_func = [](void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope){
		void* result = memalloc(RoundUpTo(size,alignment));
		Assert((size_t)result % alignment == 0, "The alignment of the pointer is invalid");
		return result;
	};
	
	auto deshi_vulkan_reallocation_func = [](void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope){
		Assert((size_t)pOriginal % alignment == 0, "The previous allocation does not match the requested alignment");
		void* result = memrealloc(pOriginal, RoundUpTo(size,alignment));
		Assert((size_t)result % alignment == 0, "The alignment of the pointer is invalid");
		return result;
	};
	
	auto deshi_vulkan_free_func = [](void* pUserData, void* pMemory){
		memzfree(pMemory);
	};
	
	allocator_.pfnAllocation = deshi_vulkan_allocation_func;
	allocator_.pfnReallocation = deshi_vulkan_reallocation_func;
	allocator_.pfnFree = deshi_vulkan_free_func;
	
	//temporary allocator
	auto deshi_vulkan_temp_allocation_func = [](void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope){
		Assert(allocationScope != VK_SYSTEM_ALLOCATION_SCOPE_DEVICE && allocationScope != VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE,
			   "Vulkan device and instance creation can not use the temporary allocator");
		void* result = memtalloc(RoundUpTo(size,alignment));
		Assert((size_t)result % alignment == 0, "The alignment of the pointer is invalid");
		return result;
	};
	
	auto deshi_vulkan_temp_reallocation_func = [](void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope){
		Assert((size_t)pOriginal % alignment == 0, "The previous allocation does not match the requested alignment");
		void* result = memtrealloc(pOriginal, RoundUpTo(size,alignment));
		Assert((size_t)result % alignment == 0, "The alignment of the pointer is invalid");
		return result;
	};
	
	auto deshi_vulkan_temp_free_func = [](void* pUserData, void* pMemory){};
	
	temp_allocator_.pfnAllocation = deshi_vulkan_temp_allocation_func;
	temp_allocator_.pfnReallocation = deshi_vulkan_temp_reallocation_func;
	temp_allocator_.pfnFree = deshi_vulkan_temp_free_func;
}

local void 
CreateInstance(){
	PrintVk(2,"Creating vulkan instance");
	Assert(rendererStage == RENDERERSTAGE_NONE, "renderer stage was not NONE at CreateInstance");
	rendererStage |= RSVK_INSTANCE;
	
	//check for validation layer support
	if(settings.debugging){
		PrintVk(3,"Checking validation layer support");
		bool has_support = true;
		
		u32 layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		
		forI(ArrayCount(validationLayers)){
			const char* layerName = validationLayers[i];
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
	appInfo.pApplicationName   = DeshWindow->name;
	appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
	appInfo.pEngineName        = "deshi";
	appInfo.engineVersion      = VK_MAKE_VERSION(1,0,0);
	appInfo.apiVersion         = VK_API_VERSION_1_0;
	
	VkValidationFeaturesEXT validationFeatures{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
	validationFeatures.disabledValidationFeatureCount = 0;
	validationFeatures.pDisabledValidationFeatures    = nullptr;
	validationFeatures.enabledValidationFeatureCount  = validationFeaturesEnabled.count;
	validationFeatures.pEnabledValidationFeatures     = validationFeaturesEnabled.data;
	
	u32 pcount = 0;
	vkEnumerateInstanceExtensionProperties(0, &pcount, 0);
	VkExtensionProperties* eprops = (VkExtensionProperties*)memalloc(pcount*sizeof(VkExtensionProperties));
	vkEnumerateInstanceExtensionProperties(0, &pcount, eprops);
	forI(pcount) {
		PRINTLN(eprops[i].extensionName);
	}

	//get required extensions
	PrintVk(3, "Getting required extensions");
	
#if DESHI_WINDOWS
	u32 extensionCount = 2;
	array<const char*> extensions{ VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	if (settings.debugging) extensions.add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#elif DESHI_LINUX
	u32 extensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	array<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (settings.debugging) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#elif DESHI_MAC
	u32 extensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	array<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (settings.debugging) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	
	//setup instance debug messenger
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	debugCreateInfo.messageSeverity = callbackSeverities;
	debugCreateInfo.messageType     = callbackTypes;
	debugCreateInfo.pfnUserCallback = DebugCallback;
	
	//create the instance
	VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	createInfo.pApplicationInfo        = &appInfo;
	createInfo.enabledExtensionCount   = (u32)extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data;
	if(settings.debugging){
		createInfo.enabledLayerCount   = (u32)ArrayCount(validationLayers);
		createInfo.ppEnabledLayerNames = validationLayers;
		debugCreateInfo.pNext          = &validationFeatures;
		createInfo.pNext               = &debugCreateInfo;
	} else {
		createInfo.enabledLayerCount   = 0;
		createInfo.pNext               = nullptr;
	}
	AssertVk(vkCreateInstance(&createInfo, allocator, &instance), "failed to create instance");
}

local void 
SetupDebugMessenger(){
	PrintVk(2, "Setting up debug messenger");
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

local void 
CreateSurface(){
	AssertRS(RSVK_INSTANCE, "CreateSurface called before CreateInstance");
	rendererStage |= RSVK_SURFACE;
	
	//NOTE(sushi) not sure if its necessary for this to be in any interface file
#ifdef DESHI_WINDOWS
	PrintVk(2, "Creating Win32-Vulkan surface");
	VkWin32SurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
	info.hwnd = (HWND)DeshWindow->handle;
	info.hinstance = (HINSTANCE)DeshWindow->instance;
	AssertVk(vkCreateWin32SurfaceKHR(instance, &info, 0, &surface), "failed to create win32 surface");
#elif DESHI_LINUX
	PrintVk(2, "Creating glfw-Vulkan surface");
	AssertVk(glfwCreateWindowSurface(instance, DeshWindow->window, allocator, &surface), "failed to create window surface");
#elif DESHI_MAC
	PrintVk(2, "Creating glfw-Vulkan surface");
	AssertVk(glfwCreateWindowSurface(instance, DeshWindow->window, allocator, &surface), "failed to create window surface");
#endif
}

local void 
PickPhysicalDevice(){
	PrintVk(2, "Picking physical device");
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

local void 
CreateLogicalDevice(){
	PrintVk(2, "Creating logical device");
	AssertRS(RSVK_PHYSICALDEVICE, "CreateLogicalDevice called before PickPhysicalDevice");
	rendererStage |= RSVK_LOGICALDEVICE;
	
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<u32> uniqueQueueFamilies = {
		physicalQueueFamilies.graphicsFamily.value, physicalQueueFamilies.presentFamily.value
	};
	
	f32 queuePriority = 1.0f;
	//queueCreateInfos.reserve(uniqueQueueFamilies.size());
	for(u32 queueFamily : uniqueQueueFamilies){
		VkDeviceQueueCreateInfo queueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
		queueCreateInfo.queueFamilyIndex = physicalQueueFamilies.graphicsFamily.value;
		queueCreateInfo.queueCount       = 1;
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
	createInfo.queueCreateInfoCount    = (u32)queueCreateInfos.size();
	createInfo.pEnabledFeatures        = &enabledFeatures;
	createInfo.enabledExtensionCount   = (u32)deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	if(settings.debugging){
		createInfo.enabledLayerCount   = (u32)ArrayCount(validationLayers);
		createInfo.ppEnabledLayerNames = validationLayers;
	}else{
		createInfo.enabledLayerCount   = 0;
	}
	
	AssertVk(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), "failed to create logical device");
	
	vkGetDeviceQueue(device, physicalQueueFamilies.graphicsFamily.value, 0, &graphicsQueue);
	vkGetDeviceQueue(device, physicalQueueFamilies.presentFamily.value, 0, &presentQueue);
}


////////////////////
//// @swapchain ////
////////////////////
//destroy old swap chain and in-flight frames, create a new swap chain with new dimensions
local void 
CreateSwapChain(){
	PrintVk(2, "Creating swapchain");
	AssertRS(RSVK_LOGICALDEVICE, "CreateSwapChain called before CreateLogicalDevice");
	rendererStage |= RSVK_SWAPCHAIN;
	
	VkSwapchainKHR oldSwapChain = swapchain;
	swapchain = VK_NULL_HANDLE;
	vkDeviceWaitIdle(device);
	
	//update width and height
	DeshWindow->GetScreenSize(width, height);
	
	{//check GPU's features/capabilities for the new swapchain
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &supportDetails.capabilities);
		
		u32 formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
		if(formatCount != 0){
			supportDetails.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, supportDetails.formats.data);
		}
		
		u32 presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
		if(presentModeCount != 0){
			supportDetails.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, supportDetails.presentModes.data);
		}
	}
	
	{//choose swapchain's surface format
		surfaceFormat = supportDetails.formats[0];
		for(VkSurfaceFormatKHR availableFormat : supportDetails.formats){
			if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT){
				surfaceFormat = availableFormat;
				break;
			}
		}
	}
	
	{//choose the swapchain's present mode
		//TODO(delle,ReVu) add render settings here (vsync)
		bool immediate    = false;
		bool fifo_relaxed = false;
		bool mailbox      = false;
		
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
			case VK_PRESENT_MODE_MAILBOX_KHR:     { minImageCount =  2; }break;
			case VK_PRESENT_MODE_FIFO_KHR:        { minImageCount =  2; }break;
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR:{ minImageCount =  2; }break;
			case VK_PRESENT_MODE_IMMEDIATE_KHR:   { minImageCount =  1; }break;
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
local VkFormat
findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features){
	PrintVk(4, "Finding supported image formats");
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

local VkFormat
findDepthFormat(){
	return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

local void 
CreateRenderpasses(){
	PrintVk(2, "Creating render pass");
	AssertRS(RSVK_LOGICALDEVICE, "CreateRenderPasses called before CreateLogicalDevice");
	rendererStage |= RSVK_RENDERPASS;
	
	if(baseRenderPass) vkDestroyRenderPass(device, baseRenderPass, allocator);
	if(msaaRenderPass) vkDestroyRenderPass(device, msaaRenderPass, allocator);
	
	VkAttachmentDescription attachments[3]{};
	//attachment 0: color 
	attachments[0].format         = surfaceFormat.format;
	attachments[0].samples        = msaaSamples;
	attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	//attachment 1: depth
	attachments[1].format         = findDepthFormat();
	attachments[1].samples        = msaaSamples;
	attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	//attachment 2: color resolve
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
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments    = attachments;
	renderPassInfo.subpassCount    = 1;
	renderPassInfo.pSubpasses      = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies   = dependencies;
	
	//TODO(delle) fix this scuffed renderpass switch
	if(msaaSamples != VK_SAMPLE_COUNT_1_BIT){
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		subpass.pResolveAttachments = &resolveAttachmentRef;
		renderPassInfo.attachmentCount = 3;
		
		AssertVk(vkCreateRenderPass(device, &renderPassInfo, allocator, &msaaRenderPass));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_RENDER_PASS, (u64)msaaRenderPass, "MSAA render pass");
		
		renderPass = msaaRenderPass;
	}else{
		AssertVk(vkCreateRenderPass(device, &renderPassInfo, allocator, &baseRenderPass));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_RENDER_PASS, (u64)baseRenderPass, "Base render pass");
		
		renderPass = baseRenderPass;
	}
}


/////////////////
//// @frames ////
/////////////////
local void 
CreateCommandPool(){
	PrintVk(2, "Creating command pool");
	AssertRS(RSVK_LOGICALDEVICE, "CreateCommandPool called before CreateLogicalDevice");
	rendererStage |= RSVK_COMMANDPOOL;
	
	VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = physicalQueueFamilies.graphicsFamily.value;
	
	AssertVk(vkCreateCommandPool(device, &poolInfo, allocator, &commandPool), "failed to create command pool");
}


//creates image views, color/depth resources, framebuffers, commandbuffers
local void 
CreateFrames(){
	PrintVk(2, "Creating frames");
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
			vkDestroyImageView(device, attachments.colorImageView, allocator);
			vkDestroyImage(device, attachments.colorImage, allocator);
			vkFreeMemory(device, attachments.colorImageMemory, allocator);
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
			vkDestroyImageView(device, attachments.depthImageView, allocator);
			vkDestroyImage(device, attachments.depthImage, allocator);
			vkFreeMemory(device, attachments.depthImageMemory, allocator);
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
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE, (u64)frames[i].image, TOSTDSTRING("Frame image ", i).c_str());
		
		//create the image views
		if(frames[i].imageView) vkDestroyImageView(device, frames[i].imageView, allocator);
		frames[i].imageView = CreateImageView(frames[i].image, surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)frames[i].imageView, TOSTDSTRING("Frame imageview ", i).c_str());
		
		//create the framebuffers
		if(frames[i].framebuffer) vkDestroyFramebuffer(device, frames[i].framebuffer, allocator);
		
		std::vector<VkImageView> frameBufferAttachments; //TODO(delle) fix scuffed msaa hack
		if(msaaSamples != VK_SAMPLE_COUNT_1_BIT){
			frameBufferAttachments = { attachments.colorImageView, attachments.depthImageView, frames[i].imageView };
		}else{
			frameBufferAttachments = { frames[i].imageView, attachments.depthImageView, };
		}
		
		VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		info.renderPass      = renderPass;
		info.attachmentCount = frameBufferAttachments.size();
		info.pAttachments    = frameBufferAttachments.data();
		info.width           = width;
		info.height          = height;
		info.layers          = 1;
		AssertVk(vkCreateFramebuffer(device, &info, allocator, &frames[i].framebuffer), "failed to create framebuffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_FRAMEBUFFER, (u64)frames[i].framebuffer, TOSTDSTRING("Frame framebuffer ", i).c_str());
		
		//allocate command buffers
		if(frames[i].commandBuffer) vkFreeCommandBuffers(device, commandPool, 1, &frames[i].commandBuffer);
		VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
		allocInfo.commandPool        = commandPool;
		allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		AssertVk(vkAllocateCommandBuffers(device, &allocInfo, &frames[i].commandBuffer), "failed to allocate command buffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (u64)frames[i].commandBuffer, TOSTDSTRING("Frame command buffer ", i).c_str());
	}
}

//creates semaphores indicating: image acquired, rendering complete
//semaphores (GPU-GPU) coordinate operations across command buffers so that they execute in a specified order
//fences (CPU-GPU) are similar but are waited for in the code itself rather than threads
local void 
CreateSyncObjects(){
	PrintVk(2, "Creating sync objects");
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


//////////////////
//// @buffers ////
//////////////////
//TODO(delle,ReOpVu) maybe only do one mapping at buffer creation, see: gltfscenerendering.cpp, line:600
local void 
UpdateUniformBuffers(){
	AssertRS(RSVK_UNIFORMBUFFER, "UpdateUniformBuffer called before CreateUniformBuffer");
	//PrintVk(2, "  Updating Uniform Buffer");
	
	{//update offscreen vertex shader ubo
		//calculate light ViewProjection for shadow map based on first light
		uboVSoffscreen.values.lightVP = 
			Math::LookAtMatrix(vkLights[0].toVec3(), vec3::ZERO).Inverse() * 
			Math::PerspectiveProjectionMatrix((f32)settings.shadowResolution, (f32)settings.shadowResolution, 90.0f, settings.shadowNearZ, settings.shadowFarZ);
		
		void* data;
		vkMapMemory(device, uboVSoffscreen.bufferMemory, 0, sizeof(uboVSoffscreen.values), 0, &data);{
			memcpy(data, &uboVSoffscreen.values, sizeof(uboVSoffscreen.values));
		}vkUnmapMemory(device, uboVSoffscreen.bufferMemory);
	}
	
	{//update scene vertex shader ubo
		uboVS.values.time = DeshTime->totalTime;
		std::copy(vkLights, vkLights+10, uboVS.values.lights);
		uboVS.values.screen = vec2((f32)extent.width, (f32)extent.height);
		uboVS.values.mousepos = vec2(DeshInput->mousePos.x, DeshInput->mousePos.y);
		if(initialized) uboVS.values.mouseWorld = Math::ScreenToWorld(DeshInput->mousePos, uboVS.values.proj, uboVS.values.view, DeshWindow->dimensions);
		uboVS.values.enablePCF = settings.shadowPCF;
		uboVS.values.lightVP = uboVSoffscreen.values.lightVP;
		
		void* data;
		vkMapMemory(device, uboVS.bufferMemory, 0, sizeof(uboVS.values), 0, &data);{
			memcpy(data, &uboVS.values, sizeof(uboVS.values));
		}vkUnmapMemory(device, uboVS.bufferMemory);
	}
	
	//update normals geometry shader ubo
	if(enabledFeatures.geometryShader){
		uboGS.values.view = uboVS.values.view;
		uboGS.values.proj = uboVS.values.proj;
		
		void* data;
		vkMapMemory(device, uboGS.bufferMemory, 0, sizeof(uboGS.values), 0, &data);{
			memcpy(data, &uboGS.values, sizeof(uboGS.values));
		}vkUnmapMemory(device, uboGS.bufferMemory);
	}
}

local void 
CreateUniformBuffers(){
	PrintVk(2, "Creating uniform buffers");
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
	if(enabledFeatures.geometryShader){
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

////////////////
//// @other ////
////////////////
local void 
SetupOffscreenRendering(){
	PrintVk(2, "Creating offscreen rendering stuffs");
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
		sampler.magFilter     = (settings.textureFiltering) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		sampler.minFilter     = (settings.textureFiltering) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		sampler.mipmapMode    = (settings.textureFiltering) ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
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
local void 
CreateLayouts(){
	PrintVk(2, "Creating layouts");
	AssertRS(RSVK_LOGICALDEVICE, "CreateLayouts called before CreateLogicalDevice");
	rendererStage |= RSVK_LAYOUTS;
	
	VkDescriptorSetLayoutBinding setLayoutBindings[4]{};
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	descriptorSetLayoutCI.pBindings    = setLayoutBindings;
	descriptorSetLayoutCI.bindingCount = 0;
	
	{//create base descriptor set layout
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
		AssertVk(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, allocator, &descriptorSetLayouts.base));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)descriptorSetLayouts.base, 
							 "Base descriptor set layout");
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
		AssertVk(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, allocator, &descriptorSetLayouts.textures));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)descriptorSetLayouts.textures, 
							 "Textures descriptor set layout");
	}
	
	{//create instances descriptor set layout
		
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)descriptorSetLayouts.instances, 
							 "Instances descriptor set layout");
	}
	
	{//create twod descriptor set layout
		//binding 1: fragment shader font image sampler
		setLayoutBindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBindings[0].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[0].binding         = 0;
		setLayoutBindings[0].descriptorCount = 1;
		
		descriptorSetLayoutCI.bindingCount = 1;
		AssertVk(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.twod));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)descriptorSetLayouts.twod, 
							 "2D descriptor set layout");
	}
	
	//create geometry descriptor set layout
	if(enabledFeatures.geometryShader){
		//binding 0: vertex shader scene UBO
		setLayoutBindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setLayoutBindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
		setLayoutBindings[0].binding         = 0;
		setLayoutBindings[0].descriptorCount = 1;
		//binding 1: geometry shader UBO
		setLayoutBindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setLayoutBindings[1].stageFlags      = VK_SHADER_STAGE_GEOMETRY_BIT;
		setLayoutBindings[1].binding         = 1;
		setLayoutBindings[1].descriptorCount = 1;
		
		descriptorSetLayoutCI.bindingCount = 2;
		AssertVk(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, allocator, &descriptorSetLayouts.geometry));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)descriptorSetLayouts.geometry, 
							 "Geometry descriptor set layout");
	}
	
	{//create base pipeline layout
		//setup push constants for passing model matrix
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset     = 0;
		pushConstantRange.size       = sizeof(mat4);
		
		VkDescriptorSetLayout setLayouts[] = { 
			descriptorSetLayouts.base, descriptorSetLayouts.textures
		};
		
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		pipelineLayoutInfo.setLayoutCount         = ArrayCount(setLayouts);
		pipelineLayoutInfo.pSetLayouts            = setLayouts;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;
		AssertVk(vkCreatePipelineLayout(device, &pipelineLayoutInfo, allocator, &pipelineLayouts.base));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)pipelineLayouts.base, 
							 "Base pipeline layout");
	}
	
	{//create twod pipeline layout
		//setup push constants for passing scale and translate
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset     = 0;
		pushConstantRange.size       = sizeof(Push2DVk);
		
		VkPipelineLayoutCreateInfo createInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		createInfo.setLayoutCount         = 1;
		createInfo.pSetLayouts            = &descriptorSetLayouts.twod;
		createInfo.pushConstantRangeCount = 1;
		createInfo.pPushConstantRanges    = &pushConstantRange;
		AssertVk(vkCreatePipelineLayout(device, &createInfo, allocator, &pipelineLayouts.twod));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)pipelineLayouts.twod, 
							 "2D pipeline layout");
	}
	
	{//create geometry shader pipeline layout
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
		pushConstantRange.offset     = 0;
		pushConstantRange.size       = sizeof(mat4);
		
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		pipelineLayoutInfo.setLayoutCount         = 1;
		pipelineLayoutInfo.pSetLayouts            = &descriptorSetLayouts.geometry;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;
		AssertVk(vkCreatePipelineLayout(device, &pipelineLayoutInfo, allocator, &pipelineLayouts.geometry));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)pipelineLayouts.geometry, 
							 "Geometry pipeline layout");
	}
}

//creates a pool of descriptors of different types to be sent to shaders
//TODO(delle,ReVu) find a better/more accurate way to do this, see gltfloading.cpp, line:592
local void 
CreateDescriptorPool(){
	PrintVk(2, "Creating descriptor pool");
	AssertRS(RSVK_LOGICALDEVICE, "CreateDescriptorPool called before CreateLogicalDevice");
	rendererStage |= RSVK_DESCRIPTORPOOL;
	
	const s32 types = 11;
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
local void 
CreateDescriptorSets(){
	AssertRS(RSVK_DESCRIPTORPOOL | RSVK_UNIFORMBUFFER, "CreateLayouts called before CreateDescriptorPool or CreateUniformBuffer");
	rendererStage |= RSVK_DESCRIPTORSETS;
	
	VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	allocInfo.descriptorPool     = descriptorPool;
	allocInfo.pSetLayouts        = &descriptorSetLayouts.base;
	allocInfo.descriptorSetCount = 1;
	
	VkWriteDescriptorSet writeDescriptorSets[2]{};
	
	{//base descriptor sets
		AssertVk(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.base));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)descriptorSets.base, "Base descriptor set");
		
		//binding 0: vertex shader ubo
		writeDescriptorSets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].dstSet          = descriptorSets.base;
		writeDescriptorSets[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSets[0].dstBinding      = 0;
		writeDescriptorSets[0].pBufferInfo     = &uboVS.bufferDescriptor;
		writeDescriptorSets[0].descriptorCount = 1;
		//binding 1: fragment shader shadow sampler
		writeDescriptorSets[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[1].dstSet          = descriptorSets.base;
		writeDescriptorSets[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSets[1].dstBinding      = 1;
		writeDescriptorSets[1].pImageInfo      = &offscreen.depthDescriptor;
		writeDescriptorSets[1].descriptorCount = 1;
		
		vkUpdateDescriptorSets(device, 2, writeDescriptorSets, 0, nullptr);
	}
	
	{//offscreen shadow map generation descriptor set
		AssertVk(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.offscreen));
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
	
	//geometry descriptor sets
	if(enabledFeatures.geometryShader){
		allocInfo.pSetLayouts = &descriptorSetLayouts.geometry;
		AssertVk(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.geometry));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)descriptorSets.geometry, "Geometry descriptor set");
		
		//binding 0: vertex shader ubo
		writeDescriptorSets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].dstSet          = descriptorSets.geometry;
		writeDescriptorSets[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSets[0].dstBinding      = 0;
		writeDescriptorSets[0].pBufferInfo     = &uboVS.bufferDescriptor;
		writeDescriptorSets[0].descriptorCount = 1;
		//binding 1: geometry shader ubo
		writeDescriptorSets[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[1].dstSet          = descriptorSets.geometry;
		writeDescriptorSets[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSets[1].dstBinding      = 1;
		writeDescriptorSets[1].pBufferInfo     = &uboGS.bufferDescriptor;
		writeDescriptorSets[1].descriptorCount = 1;
		
		vkUpdateDescriptorSets(device, 2, writeDescriptorSets, 0, nullptr);
		allocInfo.pSetLayouts = &descriptorSetLayouts.base;
	}
	
	{//DEBUG show shadow map descriptor set
		AssertVk(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.shadowMap_debug));
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

local void 
CreatePipelineCache(){
	PrintVk(2, "Creating pipeline cache");
	AssertRS(RSVK_LOGICALDEVICE, "CreatePipelineCache called before CreateLogicalDevice");
	
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
	
	//try to read pipeline cache file if exists
	std::vector<char> data = Assets::readFileBinary(Assets::dirData()+"pipelines.cache", 0, false);
	if(data.size()){
		pipelineCacheCreateInfo.initialDataSize = data.size();
		pipelineCacheCreateInfo.pInitialData    = data.data();
		AssertVk(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache), "failed to create pipeline cache");
	}
}

local void 
SetupPipelineCreation(){
	PrintVk(2, "Setting up pipeline creation");
	AssertRS(RSVK_LAYOUTS | RSVK_RENDERPASS, "SetupPipelineCreation called before CreateLayouts or CreateRenderPasses");
	rendererStage |= RSVK_PIPELINESETUP;
	
	//vertex input flow control
	//https://renderdoc.org/vkspec_chunked/chap23.html#VkPipelineVertexInputStateCreateInfo
	vertexInputBindings = { //binding:u32, stride:u32, inputRate:VkVertexInputRate
		{0, sizeof(Mesh::Vertex), VK_VERTEX_INPUT_RATE_VERTEX},
	};
	vertexInputAttributes = { //location:u32, binding:u32, format:VkFormat, offset:u32
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Mesh::Vertex, pos)},
		{1, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Mesh::Vertex, uv)},
		{2, 0, VK_FORMAT_R8G8B8A8_UNORM,   offsetof(Mesh::Vertex, color)},
		{3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Mesh::Vertex, normal)},
	};
	vertexInputState.vertexBindingDescriptionCount   = (u32)vertexInputBindings.size();
	vertexInputState.pVertexBindingDescriptions      = vertexInputBindings.data();
	vertexInputState.vertexAttributeDescriptionCount = (u32)vertexInputAttributes.size();
	vertexInputState.pVertexAttributeDescriptions    = vertexInputAttributes.data();
	
	twodVertexInputBindings = { //binding:u32, stride:u32, inputRate:VkVertexInputRate
		{0, sizeof(Vertex2), VK_VERTEX_INPUT_RATE_VERTEX},
	};
	twodVertexInputAttributes = { //location:u32, binding:u32, format:VkFormat, offset:u32
		{0, 0, VK_FORMAT_R32G32_SFLOAT,  offsetof(Vertex2, pos)},
		{1, 0, VK_FORMAT_R32G32_SFLOAT,  offsetof(Vertex2, uv)},
		{2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(Vertex2, color)},
	};
	twodVertexInputState.vertexBindingDescriptionCount   = (u32)twodVertexInputBindings.size();
	twodVertexInputState.pVertexBindingDescriptions      = twodVertexInputBindings.data();
	twodVertexInputState.vertexAttributeDescriptionCount = (u32)twodVertexInputAttributes.size();
	twodVertexInputState.pVertexAttributeDescriptions    = twodVertexInputAttributes.data();
	
	//determines how to group vertices together
	//https://renderdoc.org/vkspec_chunked/chap22.html#VkPipelineInputAssemblyStateCreateInfo
	inputAssemblyState.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; 
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;
	
	//container for viewports and scissors
	//https://renderdoc.org/vkspec_chunked/chap27.html#VkPipelineViewportStateCreateInfo
	viewportState.viewportCount = 1;
	viewportState.pViewports    = 0;
	viewportState.scissorCount  = 1;
	viewportState.pScissors     = 0;
	
	//how to draw/cull/depth things
	//https://renderdoc.org/vkspec_chunked/chap28.html#VkPipelineRasterizationStateCreateInfo
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
	multisampleState.rasterizationSamples  = msaaSamples;
	multisampleState.sampleShadingEnable   = (msaaSamples != VK_SAMPLE_COUNT_1_BIT) ? VK_TRUE : VK_FALSE;
	multisampleState.minSampleShading      = .2f; //min fraction for sample shading; closer to one is smoother
	multisampleState.pSampleMask           = 0;
	multisampleState.alphaToCoverageEnable = VK_FALSE;
	multisampleState.alphaToOneEnable      = VK_FALSE;
	
	//depth testing and discarding
	//https://renderdoc.org/vkspec_chunked/chap29.html#VkPipelineDepthStencilStateCreateInfo
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
	colorBlendAttachmentState.blendEnable         = VK_TRUE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.colorBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.colorWriteMask      = 0xF; //RGBA
	
	//container struct for color blend attachments with overall blending constants; global_ settings
	//https://renderdoc.org/vkspec_chunked/chap30.html#VkPipelineColorBlendStateCreateInfo
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
	dynamicState.dynamicStateCount = (u32)dynamicStates.size();
	dynamicState.pDynamicStates    = dynamicStates.data();
	
	//base pipeline info and options
	pipelineCreateInfo.stageCount          = 0;
	pipelineCreateInfo.pStages             = shaderStages;
	pipelineCreateInfo.pVertexInputState   = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pTessellationState  = 0;
	pipelineCreateInfo.pViewportState      = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pMultisampleState   = &multisampleState;
	pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
	pipelineCreateInfo.pColorBlendState    = &colorBlendState;
	pipelineCreateInfo.pDynamicState       = &dynamicState;
	pipelineCreateInfo.layout              = pipelineLayouts.base;
	pipelineCreateInfo.renderPass          = renderPass;
	pipelineCreateInfo.subpass             = 0;
	pipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex   = -1;
} //SetupPipelineCreation


//////////////////
//// @shaders ////
//////////////////
//TODO(delle,ReCl) clean this up
local std::vector<std::string> 
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
local VkPipelineShaderStageCreateInfo 
loadShader(std::string filename, VkShaderStageFlagBits stage){
	PrintVk(3, "Loading shader: ", filename);
	//setup shader stage create info
	VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	shaderStage.stage = stage;
	shaderStage.pName = "main";
	
	//check if shader has already been created
	for(auto& module : shaderModules){ //!FixMe this loop doesnt actually prevent recreation
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
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SHADER_MODULE, (u64)shaderModule, TOSTDSTRING("Shader ", filename).c_str());
	
	shaderStage.module = shaderModule;
	shaderModules.push_back(pair<std::string,VkShaderModule>(filename, shaderStage.module));
	return shaderStage;
}

//TODO(delle,Re) maybe dont crash on failed shader compile?
local VkPipelineShaderStageCreateInfo 
CompileAndLoadShader(std::string filename, VkShaderStageFlagBits stage, bool optimize = false){
	PrintVk(3, "Compiling and loading shader: ", filename);
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
			LogE("vulkan",filename,": Shader compiler returned a null result");
			Assert(!"crashing on shader compile error"); 
		}
		if(shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success){
			LogE("vulkan",shaderc_result_get_error_message(result)); 
			Assert(!"crashing on shader compile error"); 
		}
		
		//create shader module
		VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
		moduleInfo.codeSize = shaderc_result_get_length(result);
		moduleInfo.pCode    = (u32*)shaderc_result_get_bytes(result);
		
		VkShaderModule shaderModule{};
		AssertVk(vkCreateShaderModule(device, &moduleInfo, allocator, &shaderModule), "failed to create shader module");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SHADER_MODULE, (u64)shaderModule, TOSTDSTRING("Shader ", filename).c_str());
		
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


local void 
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
		PrintVk(4, "Compiling shader: ", filename);
		
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
			LogE("vulkan",filename,": Shader compiler returned a null result"); continue; 
		}
		if(shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success){
			LogE("vulkan",shaderc_result_get_error_message(result)); 
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

local void
CompileShader(std::string& filename, bool optimize){
	PrintVk(3, "Compiling shader: ", filename);
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
			LogE("vulkan",filename,": Shader compiler returned a null result");
		}
		if(shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success){
			LogE("vulkan",shaderc_result_get_error_message(result)); 
		}
		
		//create or overwrite .spv files
		std::ofstream outFile(entry.string() + ".spv", std::ios::out | std::ios::binary | std::ios::trunc);
		Assert(outFile.is_open(), "failed to open file");
		defer{ outFile.close(); };
		outFile.write(shaderc_result_get_bytes(result), shaderc_result_get_length(result));
	}else{
		LogE("vulkan","failed to open file: ", filename);
	}
}


/////////////////////////////
//// @pipelines creation ////
/////////////////////////////
local void 
CreatePipelines(){
	PrintVk(2, "Creating pipelines");
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
	if(settings.recompileAllShaders){
		PrintVk(3, "Compiling shaders");
		TIMER_START(t_s);
		CompileAllShaders(settings.optimizeShaders);
		PrintVk(3, "Finished compiling shaders in ", TIMER_END(t_s), "ms");
	}else{
		std::vector<std::string> uncompiled = GetUncompiledShaders();
		if(uncompiled.size()){
			PrintVk(3, "Compiling shaders");
			TIMER_START(t_s);
			for(auto& s : uncompiled){ CompileShader(s, settings.optimizeShaders); }
			PrintVk(3, "Finished compiling shaders in ", TIMER_END(t_s), "ms");
		}
	}
	
	//setup specialization constants
	/*
VkSpecializationMapEntry entryShadowPCF{};
	entryShadowPCF.constantID = 0;
	entryShadowPCF.offset = 0;
	entryShadowPCF.size = sizeof(bool);
	*/
	
	VkSpecializationInfo specializationInfo{};
	/*
specializationInfo.mapEntryCount = 1;
	specializationInfo.pMapEntries = &entryShadowPCF;
	specializationInfo.dataSize = sizeof(bool);
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
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.base));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.base, "Base pipeline");
		
		//flag that all other pipelines are derivatives
		pipelineCreateInfo.flags              = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
		pipelineCreateInfo.basePipelineHandle = pipelines.base;
		pipelineCreateInfo.basePipelineIndex  = -1; //can either use handle or index, not both (section 9.5 of vulkan spec)
	}
	
	{//selected (base with no cull or depth test)
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		depthStencilState.depthTestEnable = VK_FALSE;
		
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.selected));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.selected, "Selected pipeline");
		
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		depthStencilState.depthTestEnable = VK_TRUE;
	}
	
	{//null pipeline
		shaderStages[0] = loadShader("null.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("null.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.null));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.null, "Null pipeline");
	}
	
	{//flat pipeline
		shaderStages[0] = loadShader("flat.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("flat.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.flat));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.flat, "Flat pipeline");
	}
	
	{//phong
		shaderStages[0] = loadShader("phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.phong));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.phong, "Phong pipeline");
	}
	
	{//2d
		pipelineCreateInfo.pVertexInputState = &twodVertexInputState;
		pipelineCreateInfo.layout            = pipelineLayouts.twod;
		rasterizationState.cullMode  = VK_CULL_MODE_NONE;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		depthStencilState.depthTestEnable = VK_FALSE;
		
		shaderStages[0] = loadShader("twod.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("twod.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.twod));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.twod, "2D pipeline");
		
		{//ui
			shaderStages[0] = loadShader("ui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = loadShader("ui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
			shaderStages[1].pSpecializationInfo = &specializationInfo;
			pipelineCreateInfo.stageCount = 2;
			AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.ui));
			DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.ui, "UI pipeline");
		}
		
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
		pipelineCreateInfo.layout            = pipelineLayouts.base;
		rasterizationState.cullMode  = VK_CULL_MODE_BACK_BIT;
		rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
		depthStencilState.depthTestEnable = VK_TRUE;
	}
	
	{//pbr
		shaderStages[0] = loadShader("pbr.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("pbr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.pbr));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.pbr, "PBR pipeline");
	}
	
	//wireframe
	if(deviceFeatures.fillModeNonSolid){
		colorBlendAttachmentState.blendEnable = VK_FALSE;
		rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
		rasterizationState.cullMode    = VK_CULL_MODE_NONE;
		depthStencilState.depthTestEnable = VK_FALSE;
		
		shaderStages[0] = loadShader("wireframe.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("wireframe.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.wireframe));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.wireframe, "Wireframe pipeline");
		
		{//wireframe with depth test
			depthStencilState.depthTestEnable = VK_TRUE;
			
			AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.wireframe_depth));
			DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.wireframe_depth, "Wireframe Depth pipeline");
			
			depthStencilState.depthTestEnable = VK_FALSE;
		}
		
		{ //collider gets a specific colored wireframe
			colorBlendAttachmentState.blendEnable         = VK_TRUE;
			colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
			colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
			
			colorBlendState.blendConstants[0] = (f32)settings.colliderColor.r;
			colorBlendState.blendConstants[1] = (f32)settings.colliderColor.g;
			colorBlendState.blendConstants[2] = (f32)settings.colliderColor.b;
			colorBlendState.blendConstants[3] = (f32)settings.colliderColor.a;
			
			AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.collider));
			DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.collider, "Collider pipeline");
			
			colorBlendAttachmentState.blendEnable         = VK_FALSE;
			colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendState.blendConstants[0] = 0.f;
			colorBlendState.blendConstants[1] = 0.f;
			colorBlendState.blendConstants[2] = 0.f;
			colorBlendState.blendConstants[3] = 1.0f;
		}
		
		colorBlendAttachmentState.blendEnable = VK_TRUE;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.cullMode    = VK_CULL_MODE_BACK_BIT;
		depthStencilState.depthTestEnable = VK_TRUE;
	}
	
	{//lavalamp
		shaderStages[0] = loadShader("lavalamp.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("lavalamp.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.lavalamp));
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
		dynamicState.pDynamicStates    = dynamicStates.data();      //it can be changed at runtime
		pipelineCreateInfo.renderPass = offscreen.renderpass;
		
		shaderStages[0] = loadShader("offscreen.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		pipelineCreateInfo.stageCount = 1;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.offscreen));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.offscreen, "Offscreen pipeline");
		
		colorBlendState.attachmentCount = 1;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		rasterizationState.depthBiasEnable = VK_FALSE;
		multisampleState.rasterizationSamples = msaaSamples;
		multisampleState.sampleShadingEnable  = (msaaSamples != VK_SAMPLE_COUNT_1_BIT) ? VK_TRUE : VK_FALSE;
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
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.testing0));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.testing0, "Testing0 pipeline");
	}
	
	{//testing1
		shaderStages[0] = loadShader("testing1.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("testing1.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.testing1));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.testing1, "Testing1 pipeline");
	}
	
	//DEBUG mesh normals
	if(enabledFeatures.geometryShader){
		pipelineCreateInfo.layout = pipelineLayouts.geometry;
		
		shaderStages[0] = loadShader("nothing.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("nothing.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[2] = loadShader("normaldebug.geom.spv", VK_SHADER_STAGE_GEOMETRY_BIT);
		pipelineCreateInfo.stageCount = 3;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.normals_debug));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.normals_debug, "DEBUG Mesh normals pipeline");
		
		pipelineCreateInfo.layout = pipelineLayouts.base;
	}
	
	{//DEBUG shadow map
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		VkPipelineVertexInputStateCreateInfo emptyVertexInputState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
		pipelineCreateInfo.pVertexInputState = &emptyVertexInputState;
		
		shaderStages[0] = loadShader("shadowmapDEBUG.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("shadowmapDEBUG.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCreateInfo.stageCount = 2;
		AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.shadowmap_debug));
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.shadowmap_debug, "DEBUG Shadowmap pipeline");
		
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
	}
} //CreatePipelines

local VkPipeline 
GetPipelineFromShader(u32 shader){
	switch(shader){
		case(Shader_NULL):default:{ return pipelines.null;      }
		case(Shader_Flat):        { return pipelines.flat;      }
		case(Shader_Phong):       { return pipelines.phong;     }
		case(Shader_PBR):         { return pipelines.pbr;       }
		case(Shader_Wireframe):   { return pipelines.wireframe; }
		case(Shader_Lavalamp):    { return pipelines.lavalamp;  }
		case(Shader_Testing0):    { return pipelines.testing0;  }
		case(Shader_Testing1):    { return pipelines.testing1;  }
	}
}

local void 
UpdateMaterialPipelines(){
	PrintVk(4, "Updating material pipelines");
	for(auto& mat : vkMaterials){
		mat.pipeline = GetPipelineFromShader(mat.base->shader);
	}
}

///////////////////
//// @commands ////
///////////////////
local void
SetupCommands(){
	//create 2D vertex and index buffers
	size_t ui_vb_size = Max(1000*sizeof(Vertex2),   twodVertexCount * sizeof(Vertex2));
	size_t ui_ib_size = Max(3000*sizeof(TwodIndexVk), twodIndexCount  * sizeof(TwodIndexVk));
	if(ui_vb_size && ui_ib_size){
		//create/resize buffers if they are too small
		if(uiVertexBuffer.buffer == VK_NULL_HANDLE || uiVertexBuffer.size < ui_vb_size){
			CreateOrResizeBuffer(uiVertexBuffer.buffer, uiVertexBuffer.memory, uiVertexBuffer.size, ui_vb_size,
								 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		}
		if(uiIndexBuffer.buffer == VK_NULL_HANDLE || uiIndexBuffer.size < ui_ib_size){
			CreateOrResizeBuffer(uiIndexBuffer.buffer, uiIndexBuffer.memory, uiIndexBuffer.size, ui_ib_size,
								 VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		}
		
		//copy memory to the GPU
		void* vb_data; void* ib_data;
		AssertVk(vkMapMemory(device, uiVertexBuffer.memory, 0, ui_vb_size, 0, &vb_data));
		AssertVk(vkMapMemory(device, uiIndexBuffer.memory,  0, ui_ib_size, 0, &ib_data));
		{
			memcpy(vb_data, twodVertexArray, ui_vb_size);
			memcpy(ib_data, twodIndexArray,  ui_ib_size);
			
			VkMappedMemoryRange range[2] = {};
			range[0].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[0].memory = uiVertexBuffer.memory;
			range[0].size   = VK_WHOLE_SIZE;
			range[1].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[1].memory = uiIndexBuffer.memory;
			range[1].size   = VK_WHOLE_SIZE;
			AssertVk(vkFlushMappedMemoryRanges(device, 2, range));
		}
		vkUnmapMemory(device, uiVertexBuffer.memory);
		vkUnmapMemory(device, uiIndexBuffer.memory);
		
		//name buffers for debugging
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)uiVertexBuffer.buffer, "2D vertex buffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)uiIndexBuffer.buffer, "2D index buffer");
	}
	
	//create temp mesh vertex and index buffers
	size_t temp_wire_vb_size = tempWireframeVertexCount*sizeof(Mesh::Vertex);
	size_t temp_fill_vb_size = tempFilledVertexCount*sizeof(Mesh::Vertex);
	size_t temp_wire_ib_size = tempWireframeIndexCount*sizeof(TempIndexVk);
	size_t temp_fill_ib_size = tempFilledIndexCount*sizeof(TempIndexVk);
	size_t temp_vb_size = temp_wire_vb_size+temp_fill_vb_size;
	size_t temp_ib_size = temp_wire_ib_size+temp_fill_ib_size;
	if(tempVertexBuffer.size == 0) temp_vb_size = 1000*sizeof(Mesh::Vertex);
	if(tempIndexBuffer.size == 0)  temp_ib_size = 3000*sizeof(TempIndexVk);
	if(temp_vb_size && temp_ib_size){
		//create/resize buffers if they are too small
		if(tempVertexBuffer.buffer == VK_NULL_HANDLE || tempVertexBuffer.size < temp_vb_size){
			CreateOrResizeBuffer(tempVertexBuffer.buffer, tempVertexBuffer.memory, tempVertexBuffer.size, temp_vb_size,
								 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		}
		if(tempIndexBuffer.buffer == VK_NULL_HANDLE || tempIndexBuffer.size < temp_ib_size){
			CreateOrResizeBuffer(tempIndexBuffer.buffer, tempIndexBuffer.memory, tempIndexBuffer.size, temp_ib_size,
								 VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		}
		
		//copy memory to the GPU
		void* vb_data; void* ib_data;
		AssertVk(vkMapMemory(device, tempVertexBuffer.memory, 0, temp_vb_size, 0, &vb_data));
		AssertVk(vkMapMemory(device, tempIndexBuffer.memory,  0, temp_ib_size, 0, &ib_data));
		{
			memcpy(vb_data, tempWireframeVertexArray, temp_wire_vb_size);
			memcpy(ib_data, tempWireframeIndexArray,  temp_wire_ib_size);
			memcpy((char*)vb_data+temp_wire_vb_size, tempFilledVertexArray, temp_fill_vb_size);
			memcpy((char*)ib_data+temp_wire_ib_size, tempFilledIndexArray,  temp_fill_ib_size);
			
			VkMappedMemoryRange range[2] = {};
			range[0].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[0].memory = tempVertexBuffer.memory;
			range[0].size   = VK_WHOLE_SIZE;
			range[1].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[1].memory = tempIndexBuffer.memory;
			range[1].size   = VK_WHOLE_SIZE;
			AssertVk(vkFlushMappedMemoryRanges(device, 2, range));
		}
		vkUnmapMemory(device, tempVertexBuffer.memory);
		vkUnmapMemory(device, tempIndexBuffer.memory);
		
		//name buffers for debugging
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)tempVertexBuffer.buffer, "Temp vertex buffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)tempIndexBuffer.buffer, "Temp index buffer");
	}
	
	//create debug mesh vertex and index buffers
	size_t debug_wire_vb_size = debugWireframeVertexCount*sizeof(Mesh::Vertex);
	size_t debug_fill_vb_size = debugFilledVertexCount*sizeof(Mesh::Vertex);
	size_t debug_wire_ib_size = debugWireframeIndexCount*sizeof(TempIndexVk);
	size_t debug_fill_ib_size = debugFilledIndexCount*sizeof(TempIndexVk);
	size_t debug_vb_size = debug_wire_vb_size+debug_fill_vb_size;
	size_t debug_ib_size = debug_wire_ib_size+debug_fill_ib_size;
	if(debugVertexBuffer.size == 0) debug_vb_size = 1000*sizeof(Mesh::Vertex);
	if(debugIndexBuffer.size == 0)  debug_ib_size = 3000*sizeof(TempIndexVk);
	if(debug_vb_size && debug_ib_size){
		//create/resize buffers if they are too small
		if(debugVertexBuffer.buffer == VK_NULL_HANDLE || debugVertexBuffer.size < debug_vb_size){
			CreateOrResizeBuffer(debugVertexBuffer.buffer, debugVertexBuffer.memory, debugVertexBuffer.size, debug_vb_size,
								 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		}
		if(debugIndexBuffer.buffer == VK_NULL_HANDLE || debugIndexBuffer.size < debug_ib_size){
			CreateOrResizeBuffer(debugIndexBuffer.buffer, debugIndexBuffer.memory, debugIndexBuffer.size, debug_ib_size,
								 VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		}
		
		//copy memory to the GPU
		void* vb_data; void* ib_data;
		AssertVk(vkMapMemory(device, debugVertexBuffer.memory, 0, debug_vb_size, 0, &vb_data));
		AssertVk(vkMapMemory(device, debugIndexBuffer.memory,  0, debug_ib_size, 0, &ib_data));
		{
			memcpy(vb_data, debugWireframeVertexArray, debug_wire_vb_size);
			memcpy(ib_data, debugWireframeIndexArray,  debug_wire_ib_size);
			memcpy((char*)vb_data+debug_wire_vb_size, debugFilledVertexArray, debug_fill_vb_size);
			memcpy((char*)ib_data+debug_wire_ib_size, debugFilledIndexArray,  debug_fill_ib_size);
			
			VkMappedMemoryRange range[2] = {};
			range[0].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[0].memory = debugVertexBuffer.memory;
			range[0].size   = VK_WHOLE_SIZE;
			range[1].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[1].memory = debugIndexBuffer.memory;
			range[1].size   = VK_WHOLE_SIZE;
			AssertVk(vkFlushMappedMemoryRanges(device, 2, range));
		}
		vkUnmapMemory(device, debugVertexBuffer.memory);
		vkUnmapMemory(device, debugIndexBuffer.memory);
		
		//name buffers for debugging
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)debugVertexBuffer.buffer, "Debug vertex buffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)debugIndexBuffer.buffer, "Debug index buffer");
	}
}

local void
ResetCommands(){
	{//2D commands
		twodVertexCount = 0;
		twodIndexCount  = 0;
		forI(TWOD_LAYERS){
			memset(&twodCmdArrays[i][0], 0, sizeof(TwodCmdVk) * twodCmdCounts[i]);
			twodCmdArrays[i][0].descriptorSet = textures[1].descriptorSet;
			twodCmdCounts[i] = 1;
		}
	}
	
	{//temp commands
		tempWireframeVertexCount = 0;
		tempWireframeIndexCount  = 0;
		tempFilledVertexCount = 0;
		tempFilledIndexCount  = 0;
	}
	
	{//model commands
		modelCmdCount = 0;
	}
}

////////////////
//// @build ////
////////////////
local vec4 render_pass_color = vec4(0.78f, 0.54f, 0.12f, 1.0f);
local vec4 draw_group_color  = vec4(0.50f, 0.76f, 0.34f, 1.0f);
local vec4 draw_cmd_color    = vec4(0.40f, 0.61f, 0.27f, 1.0f);

//we define a call order to command buffers so they can be executed by vkSubmitQueue()
local void 
BuildCommands(){
	//PrintVk(2, "Building Command Buffers");
	AssertRS(RSVK_DESCRIPTORSETS | RSVK_PIPELINECREATE, "BuildCommandBuffers called before CreateDescriptorSets or CreatePipelines");
	
	VkClearValue clearValues[2]{};
	VkCommandBufferBeginInfo cmdBufferInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	VkViewport viewport{}; //scales the image
	VkRect2D   scissor{};  //cuts the scaled image //TODO(delle,Re) letterboxing settings here
	
	forI(imageCount){
		VkCommandBuffer cmdBuffer = frames[i].commandBuffer;
		AssertVk(vkBeginCommandBuffer(cmdBuffer, &cmdBufferInfo), "failed to begin recording command buffer");
		
		////////////////////////////
		//// @first render pass ////
		////////////////////////////
		{//generate shadow map by rendering the scene offscreen
			clearValues[0].depthStencil = {1.0f, 0};
			renderPassInfo.renderPass               = offscreen.renderpass;
			renderPassInfo.framebuffer              = offscreen.framebuffer;
			renderPassInfo.renderArea.offset        = {0, 0};
			renderPassInfo.renderArea.extent.width  = offscreen.width;
			renderPassInfo.renderArea.extent.height = offscreen.height;
			renderPassInfo.clearValueCount          = 1;
			renderPassInfo.pClearValues             = clearValues;
			viewport.width    = (f32)offscreen.width;
			viewport.height   = (f32)offscreen.height;
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			scissor.offset.x      = 0;
			scissor.offset.y      = 0;
			scissor.extent.width  = offscreen.width;
			scissor.extent.height = offscreen.width;
			
			DebugBeginLabelVk(cmdBuffer, "Offscreen Render Pass", render_pass_color);
			vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
			vkCmdSetDepthBias(cmdBuffer, settings.depthBiasConstant, 0.0f, settings.depthBiasSlope);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.base, 0, 1, &descriptorSets.offscreen, 0, nullptr);
			
			DebugBeginLabelVk(cmdBuffer, "Meshes", draw_group_color);
			VkDeviceSize offsets[1] = {0}; //reset vertex buffer offsets
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &meshVertexBuffer.buffer, offsets);
			vkCmdBindIndexBuffer(cmdBuffer, meshIndexBuffer.buffer, 0, INDEX_TYPE_VK_MESH);
			forI(modelCmdCount){
				ModelCmdVk& cmd = modelCmdArray[i];
				MaterialVk& mat = vkMaterials[cmd.material];
				DebugInsertLabelVk(cmdBuffer, cmd.name, draw_cmd_color);
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &cmd.matrix);
				vkCmdDrawIndexed(cmdBuffer, cmd.indexCount, 1, cmd.indexOffset, cmd.vertexOffset, 0);
				stats.drawnIndices += cmd.indexCount;
			}
			DebugEndLabelVk(cmdBuffer);
			
			vkCmdEndRenderPass(cmdBuffer);
			DebugEndLabelVk(cmdBuffer);
		}
		
		//NOTE explicit synchronization is not required because it is done via the subpass dependenies
		
		/////////////////////////////
		//// @second render pass ////
		/////////////////////////////
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
			viewport.width    = (f32)width;
			viewport.height   = (f32)height;
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			scissor.offset.x      = 0;
			scissor.offset.y      = 0;
			scissor.extent.width  = width;
			scissor.extent.height = height;
			
			DebugBeginLabelVk(cmdBuffer, "Scene Render Pass", render_pass_color);
			vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.base, 0, 1, &descriptorSets.base, 0, nullptr);
			
			//draw meshes
			DebugBeginLabelVk(cmdBuffer, "Meshes", draw_group_color);
			VkDeviceSize offsets[1] = {0}; //reset vertex buffer offsets
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &meshVertexBuffer.buffer, offsets);
			vkCmdBindIndexBuffer(cmdBuffer, meshIndexBuffer.buffer, 0, INDEX_TYPE_VK_MESH);
			forI(modelCmdCount){
				ModelCmdVk& cmd = modelCmdArray[i];
				MaterialVk& mat = vkMaterials[cmd.material];
				DebugInsertLabelVk(cmdBuffer, cmd.name, draw_cmd_color);
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &cmd.matrix);
				
				if(settings.wireframeOnly){
					vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe);
				}else{
					vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mat.pipeline);
					vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.base, 1, 1, &mat.descriptorSet, 0, nullptr);
				}
				vkCmdDrawIndexed(cmdBuffer, cmd.indexCount, 1, cmd.indexOffset, cmd.vertexOffset, 0);
				stats.drawnIndices += cmd.indexCount;
				
				//wireframe overlay
				if(settings.meshWireframes){
					vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe);
					vkCmdDrawIndexed(cmdBuffer, cmd.indexCount, 1, cmd.indexOffset, cmd.vertexOffset, 0);
					stats.drawnIndices += cmd.indexCount;
				}
			}
			DebugEndLabelVk(cmdBuffer);
			
			//draw mesh normals overlay
			if(enabledFeatures.geometryShader && settings.meshNormals){
				DebugBeginLabelVk(cmdBuffer, "Debug Normals", draw_group_color);
				forI(modelCmdCount){
					ModelCmdVk& cmd = modelCmdArray[i];
					MaterialVk& mat = vkMaterials[cmd.material];
					DebugInsertLabelVk(cmdBuffer, cmd.name, draw_cmd_color);
					vkCmdPushConstants(cmdBuffer, pipelineLayouts.geometry, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(mat4), &cmd.matrix);
					vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.normals_debug);
					vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.geometry, 0, 1, &descriptorSets.geometry, 0, nullptr);
					vkCmdDrawIndexed(cmdBuffer, cmd.indexCount, 1, cmd.indexOffset, cmd.vertexOffset, 0);
					stats.drawnIndices += cmd.indexCount;
				}
				DebugEndLabelVk(cmdBuffer);
			}
			
			//draw temporary stuff
			if(tempWireframeVertexCount > 0 && tempWireframeIndexCount > 0){
				DebugBeginLabelVk(cmdBuffer, "Temp", draw_group_color);
				VkDeviceSize offsets[1] = {0};
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &tempVertexBuffer.buffer, offsets);
				vkCmdBindIndexBuffer(cmdBuffer, tempIndexBuffer.buffer, 0, INDEX_TYPE_VK_TEMP);
				
				//wireframe
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,(settings.tempMeshOnTop) ? pipelines.wireframe : pipelines.wireframe_depth);
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &mat4::IDENTITY);
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.base, 0, 1, &descriptorSets.base, 0, nullptr);
				vkCmdDrawIndexed(cmdBuffer, tempWireframeIndexCount, 1, 0, 0, 0);
				stats.drawnIndices += tempWireframeIndexCount;
				
				//filled
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.selected);
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &mat4::IDENTITY);
				vkCmdDrawIndexed(cmdBuffer, tempFilledIndexCount, 1, tempWireframeIndexCount, tempWireframeVertexCount, 0);
				stats.drawnIndices += tempFilledIndexCount;
				
				DebugEndLabelVk(cmdBuffer);
			}
			
			//draw debug stuff
			if(debugWireframeVertexCount > 0 && debugWireframeIndexCount > 0){
				DebugBeginLabelVk(cmdBuffer, "Debug", draw_group_color);
				VkDeviceSize offsets[1] = {0};
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &debugVertexBuffer.buffer, offsets);
				vkCmdBindIndexBuffer(cmdBuffer, debugIndexBuffer.buffer, 0, INDEX_TYPE_VK_TEMP);
				
				//wireframe
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,(settings.tempMeshOnTop) ? pipelines.wireframe : pipelines.wireframe_depth);
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &mat4::IDENTITY);
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.base, 0, 1, &descriptorSets.base, 0, nullptr);
				vkCmdDrawIndexed(cmdBuffer, debugWireframeIndexCount, 1, 0, 0, 0);
				stats.drawnIndices += debugWireframeIndexCount;
				
				//filled
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.selected);
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &mat4::IDENTITY);
				vkCmdDrawIndexed(cmdBuffer, debugFilledIndexCount, 1, debugWireframeIndexCount, debugWireframeVertexCount, 0);
				stats.drawnIndices += debugFilledIndexCount;
				
				DebugEndLabelVk(cmdBuffer);
			}
			
			//draw twod stuff
			if(twodVertexCount > 0 && twodIndexCount > 0){
				DebugBeginLabelVk(cmdBuffer, "UI", draw_group_color);
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.ui);
				VkDeviceSize offsets[1] = {0};
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &uiVertexBuffer.buffer, offsets);
				vkCmdBindIndexBuffer(cmdBuffer, uiIndexBuffer.buffer, 0, INDEX_TYPE_VK_UI);
				Push2DVk push{};
				push.scale.x = 2.0f / (f32)width;
				push.scale.y = 2.0f / (f32)height;
				push.translate.x = -1.0f;
				push.translate.y = -1.0f;
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.twod, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push2DVk), &push);
				
				forX(layer, TWOD_LAYERS){
					if(twodCmdCounts[layer] > 1){
						forX(cmd_idx, twodCmdCounts[layer]){
							scissor.offset.x = (u32)twodCmdArrays[layer][cmd_idx].scissorOffset.x;
							scissor.offset.y = (u32)twodCmdArrays[layer][cmd_idx].scissorOffset.y;
							scissor.extent.width = (u32)twodCmdArrays[layer][cmd_idx].scissorExtent.x;
							scissor.extent.height = (u32)twodCmdArrays[layer][cmd_idx].scissorExtent.y;
							vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
							
							if(twodCmdArrays[layer][cmd_idx].descriptorSet){
								vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.twod, 0, 1, &twodCmdArrays[layer][cmd_idx].descriptorSet, 0, nullptr);
								vkCmdDrawIndexed(cmdBuffer, twodCmdArrays[layer][cmd_idx].indexCount, 1, twodCmdArrays[layer][cmd_idx].indexOffset, 0, 0);
							}
						}
					}
				}
				stats.drawnIndices += twodIndexCount;
				
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				scissor.extent.width = width;
				scissor.extent.height = height;
				
				DebugEndLabelVk(cmdBuffer);
			}
			
			//draw imgui stuff
#ifndef DESHI_DISABLE_IMGUI
			if(DeshiModuleLoaded(DS_IMGUI)){
				if(ImDrawData* imDrawData = ImGui::GetDrawData()){
					DebugBeginLabelVk(cmdBuffer, "ImGui", draw_group_color);
					ImGui_ImplVulkan_RenderDrawData(imDrawData, cmdBuffer);
					DebugEndLabelVk(cmdBuffer);
				}
			}
#endif
			
			//DEBUG draw shadow map
			if(settings.showShadowMap){
				viewport.x      = (f32)(width - 400);
				viewport.y      = (f32)(height - 400);
				viewport.width  = 400.f;
				viewport.height = 400.f;
				vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
				vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
				
				DebugBeginLabelVk(cmdBuffer, "DEBUG Shadow map quad", draw_group_color);
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.base, 0, 1, &descriptorSets.shadowMap_debug, 0, nullptr);
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.shadowmap_debug);
				vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
				DebugEndLabelVk(cmdBuffer);
				
				viewport.x      = 0;
				viewport.y      = 0;
				viewport.width  = (f32)width;
				viewport.height = (f32)height;
			}
			
			vkCmdEndRenderPass(cmdBuffer);
			DebugEndLabelVk(cmdBuffer);
		}
		
		AssertVk(vkEndCommandBuffer(cmdBuffer), "failed to end recording command buffer");
	}
}


//-------------------------------------------------------------------------------------------------
// @IMGUI FUNCTIONS


local void imguiCheckVkResult(VkResult err){
	// https://renderdoc.org/vkspec_chunked/chap4.html#VkResult
	AssertVk(err, "imgui vulkan error");
}

local char iniFilepath[256] = {};
void DeshiImGui::
Init(){
	AddFlag(deshiStage, DS_IMGUI);
	TIMER_START(t_s);
	
	//Setup Dear ImGui context
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	cpystr(iniFilepath, (Assets::dirConfig() + "imgui.ini").c_str(), 256);
	io.IniFilename = iniFilepath;
	
	//Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	
	//Setup Platform/Renderer backends
#if DESHI_WINDOWS
	ImGui_ImplWin32_Init((HWND)DeshWindow->handle);
#elif DESHI_LINUX
	ImGui_ImplGlfw_InitForVulkan(DeshWindow->window, true);
#elif DESHI_MAC
	ImGui_ImplGlfw_InitForVulkan(DeshWindow->window, true);
#endif
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
	
	LogS("deshi","Finished imgui initialization in ",TIMER_END(t_s),"ms");
}

void DeshiImGui::
Cleanup(){
	AssertVk(vkDeviceWaitIdle(device));
	ImGui_ImplVulkan_Shutdown();
#if DESHI_WINDOWS
	ImGui_ImplWin32_Shutdown();
#elif DESHI_LINUX
	ImGui_ImplGlfw_Shutdown();
#elif DESHI_MAC
	ImGui_ImplGlfw_Shutdown();
#endif
	ImGui::DestroyContext();
}

void DeshiImGui::
NewFrame(){
	ImGui_ImplVulkan_NewFrame();

#if DESHI_WINDOWS
	ImGui_ImplWin32_NewFrame();
#elif DESHI_LINUX
	ImGui_ImplGlfw_Shutdown();
#elif DESHI_MAC
	ImGui_ImplGlfw_NewFrame();
#endif

	ImGui::NewFrame();
}


//-------------------------------------------------------------------------------------------------
// @2D INTERFACE
//TODO(sushi) find a nicer way to keep track of this
//NOTE im not sure yet if i should be keeping track of this for each primitive or not yet but i dont think i have to
vec2 prevScissorOffset = vec2(0, 0);
vec2 prevScissorExtent = vec2(0, 0);

void Check2DCmdArrays(u32 layer, Texture* tex, b32 textured, vec2 scissorOffset, vec2 scissorExtent){
	if((twodCmdArrays[layer][twodCmdCounts[layer] - 1].textured != textured)
	   || ((tex) ? twodCmdArrays[layer][twodCmdCounts[layer] - 1].descriptorSet != textures[tex->idx].descriptorSet : 0)
	   || (scissorOffset != prevScissorOffset)   //im doing these 2 because we have to know if we're drawing in a new window
	   || (scissorExtent != prevScissorExtent)){ //and you could do text last in one, and text first in another {  
		prevScissorExtent = scissorExtent;
		prevScissorOffset = scissorOffset;         //NOTE null_font is the default texture for 2D items, as its just a white square
		twodCmdArrays[layer][twodCmdCounts[layer]].descriptorSet = textures[(tex ? tex->idx : 1)].descriptorSet;
		twodCmdArrays[layer][twodCmdCounts[layer]].indexOffset = twodIndexCount;
		twodCmdArrays[layer][twodCmdCounts[layer]].textured = textured;
		twodCmdArrays[layer][twodCmdCounts[layer]].scissorOffset = scissorOffset;
		twodCmdArrays[layer][twodCmdCounts[layer]].scissorExtent = scissorExtent;
		twodCmdCounts[layer]++;
	}
	Assert(twodCmdCounts[layer] <= MAX_TWOD_CMDS);
	Assert(twodVertexCount <= MAX_TWOD_VERTICES);
	Assert(twodIndexCount <= MAX_TWOD_INDICES);
	
}

void Render::FillTriangle2D(vec2 p1, vec2 p2, vec2 p3, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0,
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	Check2DCmdArrays(layer, 0, 0, scissorOffset, scissorExtent);
	
	u32       col = color.rgba;
	Vertex2*   vp = twodVertexArray + twodVertexCount;
	TwodIndexVk* ip = twodIndexArray + twodIndexCount;
	
	ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
	vp[0].pos = p1; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = p2; vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = p3; vp[2].uv = { 0,0 }; vp[2].color = col;
	
	twodVertexCount += 3;
	twodIndexCount += 3;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 3;
}

void Render::DrawTriangle2D(vec2 p1, vec2 p2, vec2 p3, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0,
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	
	DrawLine2D(p1, p2, 1, color, layer, scissorOffset, scissorExtent);
	DrawLine2D(p2, p3, 1, color, layer, scissorOffset, scissorExtent);
	DrawLine2D(p3, p1, 1, color, layer, scissorOffset, scissorExtent);
}

void Render::FillRect2D(vec2 pos, vec2 dimensions, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0,
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	Check2DCmdArrays(layer, 0, 0, scissorOffset, scissorExtent);
	
	u32       col = color.rgba;
	Vertex2*   vp = twodVertexArray + twodVertexCount;
	TwodIndexVk* ip = twodIndexArray + twodIndexCount;
	
	ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
	ip[3] = twodVertexCount; ip[4] = twodVertexCount + 2; ip[5] = twodVertexCount + 3;
	vp[0].pos = { pos.x + 0,           pos.y + 0 };            vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = { pos.x + dimensions.w,pos.y + 0 };            vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = { pos.x + dimensions.w,pos.y + dimensions.h }; vp[2].uv = { 0,0 }; vp[2].color = col;
	vp[3].pos = { pos.x + 0,           pos.y + dimensions.h }; vp[3].uv = { 0,0 }; vp[3].color = col;
	
	twodVertexCount += 4;
	twodIndexCount += 6;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
}

//this func is kind of scuffed i think because of the line thickness stuff when trying to draw
//straight lines, see below
void Render::DrawRect2D(vec2 pos, vec2 dimensions, f32 thickness, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0,
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	
	//top left to top right 
	DrawLine2D(pos.xAdd(-thickness), pos.xAdd(thickness/2)+ dimensions.ySet(0), thickness, color, layer, scissorOffset, scissorExtent);
	//top left to bottom left
	DrawLine2D(pos, pos + dimensions.xSet(0), thickness, color, layer, scissorOffset, scissorExtent);
	//bottom right to top right
	DrawLine2D(pos + dimensions, pos + dimensions.ySet(0), thickness, color, layer, scissorOffset, scissorExtent);
	//bottom right to bottom left
	DrawLine2D((pos + dimensions).xAdd(thickness/2), pos + dimensions.xSet(0).xAdd(-thickness), thickness, color, layer, scissorOffset, scissorExtent);
}

void Render::DrawCircle2D(vec2 pos, f32 radius, u32 subdivisions_int, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0,
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	Check2DCmdArrays(layer, 0, 0, scissorOffset, scissorExtent);
	
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (f32(i - 1) * M_2PI) / subdivisions;
		f32 a1 = (f32(i) * M_2PI) / subdivisions;
		f32 x0 = pos.x + radius * cosf(a0); f32 x1 = pos.x + radius * cosf(a1);
		f32 y0 = pos.y + radius * sinf(a0); f32 y1 = pos.y + radius * sinf(a1);
		DrawLine2D(vec2(x0, y0), vec2(x1, y1), 1, color, layer, scissorOffset, scissorExtent);
	}
}

void Render::FillCircle2D(vec2 pos, f32 radius, u32 subdivisions_int, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0,
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	Check2DCmdArrays(layer, 0, 0, scissorOffset, scissorExtent);
	
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (f32(i - 1) * M_2PI) / subdivisions;
		f32 a1 = (f32(i) * M_2PI) / subdivisions;
		f32 x0 = pos.x + radius * cosf(a0); f32 x1 = pos.x + radius * cosf(a1);
		f32 y0 = pos.y + radius * sinf(a0); f32 y1 = pos.y + radius * sinf(a1);
		FillTriangle2D(pos, vec2(x0, y0), vec2(x1, y1), color, layer, scissorOffset, scissorExtent);
	}
}

//TODO(sushi) implement special line drawing for straight lines, since we dont need to do the normal thing
//when drawing them straight
void Render::DrawLine2D(vec2 start, vec2 end, f32 thickness, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0,
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	Check2DCmdArrays(layer, 0, 0, scissorOffset, scissorExtent);
	
	u32       col = color.rgba;
	Vertex2*   vp = twodVertexArray + twodVertexCount;
	TwodIndexVk* ip = twodIndexArray + twodIndexCount;
	
	vec2 ott = end - start;
	vec2 norm = vec2(ott.y, -ott.x).normalized();
	
	ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
	ip[3] = twodVertexCount; ip[4] = twodVertexCount + 2; ip[5] = twodVertexCount + 3;
	vp[0].pos = { start.x,start.y }; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = { end.x,  end.y };   vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = { end.x,  end.y };   vp[2].uv = { 0,0 }; vp[2].color = col;
	vp[3].pos = { start.x,start.y }; vp[3].uv = { 0,0 }; vp[3].color = col;
	
	vp[0].pos += norm * thickness / 2;
	vp[1].pos += norm * thickness / 2;
	vp[2].pos -= norm * thickness / 2;
	vp[3].pos -= norm * thickness / 2;
	
	twodVertexCount += 4;
	twodIndexCount += 6;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
}

//TODO
// this function needs to be made more robust as well as cleaned up. currently, if 2 line segments form a small enough angle,
// the thickness stop being preserved. this funciton also needs to be moved out to suugu and replaced by a more general
// render function that allows you to manipulate the vertex/index arrays

void Render::DrawLines2D(array<vec2>& points, f32 thickness, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0,
		   "Scissor Offset and Extent can't be negative");
	Assert(points.count > 1, "Lines need at least 2 points");
	if(color.a == 0 || thickness == 0) return;
	Check2DCmdArrays(layer, 0, 0, scissorOffset, scissorExtent);
	
	f32 halfthick = thickness / 2;
	
	u32       col = color.rgba;
	Vertex2*   vp = twodVertexArray + twodVertexCount;
	TwodIndexVk* ip = twodIndexArray + twodIndexCount;
	
	{// first point
		
		vec2 ott = points[1] - points[0];
		vec2 norm = vec2(ott.y, -ott.x).normalized();
		
		vp[0].pos = points[0] + norm * halfthick; vp[0].uv = { 0,0 }; vp[0].color = col;
		vp[1].pos = points[0] - norm * halfthick; vp[1].uv = { 0,0 }; vp[1].color = col;
		
		ip[0] = twodVertexCount;
		ip[1] = twodVertexCount + 1;
		ip[3] = twodVertexCount;
		
		twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 3;
		
		twodVertexCount += 2;
		twodIndexCount += 3;
		vp += 2;
	}
	
	//in betweens
	s32 flip = -1;
	for(s32 i = 1; i < points.count - 1; i++, flip *= -1){
		vec2 last, curr, next, norm;
		
		last = points[i - 1];
		curr = points[i];
		next = points[i + 1];
		
		//figure out average norm
		vec2
		p01 = curr - last,
		p12 = next - curr,
		p02 = next - last,
		//norm01 = vec2{ p01.y, -p01.x } * flip, //we flip the normal everytime to keep up the pattern
		//norm12 = vec2{ p12.y, -p12.x } * flip,
		normav;//((norm01 + norm12) / 2).normalized();
		
		f32 a = p01.mag(), b = p12.mag(), c = p02.mag();
		f32 ang = Radians(Math::AngBetweenVectors(-p01, p12));
		
		//this is the critical angle where the thickness of the 2 lines cause them to overlap at small angles
		//if(fabs(ang) < 2 * atanf(thickness / (2 * p02.mag()))){
		//	ang = 2 * atanf(thickness / (2 * p02.mag()));
		//}
		
		normav = p12.normalized();
		normav = Math::vec2RotateByAngle(-Degrees(ang) / 2, normav);
		normav *= (f32)flip;
		
		//this is where we calc how wide the thickness of the inner line is meant to be
		normav = normav.normalized() * thickness / (2 * sinf(ang / 2));
		
		vec2 normavout = normav;
		vec2 normavin = -normav;
		
		normavout.clampMag(0, thickness * 2);//sqrt(2) / 2 * thickness );
		normavin.clampMag(0, thickness * 4);
		
		//set indicies by pattern
		s32 ipidx = 6 * (i - 1) + 2;
		ip[ipidx + 0] =
			ip[ipidx + 2] =
			ip[ipidx + 4] =
			ip[ipidx + 7] =
			twodVertexCount;
		
		ip[ipidx + 3] =
			ip[ipidx + 5] =
			twodVertexCount + 1;
		
		vp[0].pos = curr + normavout; vp[0].uv = { 0,0 }; vp[0].color = col;//PackColorU32(255, 0, 0, 255);
		vp[1].pos = curr + normavin; vp[1].uv = { 0,0 }; vp[1].color = col;//PackColorU32(255, 0, 255, 255);
		
		twodVertexCount += 2;
		twodIndexCount += 6;
		vp += 2;
		
		twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
		
	}
	
	{//last point
		vec2 ott = points[points.count - 1] - points[points.count - 2];
		vec2 norm = vec2(ott.y, -ott.x).normalized() * (f32)flip;
		
		vp[0].pos = points[points.count - 1] + norm * halfthick; vp[0].uv = { 0,0 }; vp[0].color = col;//PackColorU32(255, 50, 255, 255);
		vp[1].pos = points[points.count - 1] - norm * halfthick; vp[1].uv = { 0,0 }; vp[1].color = col;//PackColorU32(255, 50, 100, 255);
		
		//set final indicies by pattern
		s32 ipidx = 6 * (points.count - 2) + 2;
		ip[ipidx + 0] = twodVertexCount;
		ip[ipidx + 2] = twodVertexCount;
		ip[ipidx + 3] = twodVertexCount + 1;
		
		twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 3;
		
		twodVertexCount += 2;
		twodIndexCount += 3;
		vp += 2; ip += 3;
	}
}

void Render::
DrawText2D(Font* font, cstring text, vec2 pos, color color, vec2 scale, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0,
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	Check2DCmdArrays(layer, font->tex, 0, scissorOffset, scissorExtent);
	
	switch (font->type){
		//// BDF (and NULL) font rendering ////
		case FontType_BDF: case FontType_NONE: {
			forI(text.count){
				u32       col = color.rgba;
				Vertex2*   vp = twodVertexArray + twodVertexCount;
				TwodIndexVk* ip = twodIndexArray + twodIndexCount;
				
				f32 w = font->max_width * scale.x;
				f32 h = font->max_height * scale.y;
				f32 dy = 1.f / (f32)font->count;
				
				f32 idx = f32(text[i] - 32);
				f32 topoff = idx * dy;
				f32 botoff = topoff + dy;
				
				ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
				ip[3] = twodVertexCount; ip[4] = twodVertexCount + 2; ip[5] = twodVertexCount + 3;
				vp[0].pos = { pos.x + 0,pos.y + 0 }; vp[0].uv = { 0,topoff + font->uvOffset }; vp[0].color = col;
				vp[1].pos = { pos.x + w,pos.y + 0 }; vp[1].uv = { 1,topoff + font->uvOffset }; vp[1].color = col;
				vp[2].pos = { pos.x + w,pos.y + h }; vp[2].uv = { 1,botoff + font->uvOffset }; vp[2].color = col;
				vp[3].pos = { pos.x + 0,pos.y + h }; vp[3].uv = { 0,botoff + font->uvOffset }; vp[3].color = col;
				
				twodVertexCount += 4;
				twodIndexCount += 6;
				twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
				pos.x += font->max_width * scale.x;
			}
		}break;
		//// TTF font rendering ////
		case FontType_TTF: {
			forI(text.count){
				u32       col = color.rgba;
				Vertex2*   vp = twodVertexArray + twodVertexCount;
				TwodIndexVk* ip = twodIndexArray + twodIndexCount;
				
				aligned_quad q = font->GetPackedQuad(text[i], &pos, scale);
				
				ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
				ip[3] = twodVertexCount; ip[4] = twodVertexCount + 2; ip[5] = twodVertexCount + 3;
				vp[0].pos = { q.x0,q.y0 }; vp[0].uv = { q.s0,q.t0 + font->uvOffset }; vp[0].color = col;
				vp[1].pos = { q.x1,q.y0 }; vp[1].uv = { q.s1,q.t0 + font->uvOffset }; vp[1].color = col;
				vp[2].pos = { q.x1,q.y1 }; vp[2].uv = { q.s1,q.t1 + font->uvOffset }; vp[2].color = col;
				vp[3].pos = { q.x0,q.y1 }; vp[3].uv = { q.s0,q.t1 + font->uvOffset }; vp[3].color = col;
				
				twodVertexCount += 4;
				twodIndexCount += 6;
				twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
			}break;
			default: Assert(!"unhandled font type"); break;
		}
	}
}

void Render::
DrawText2D(Font* font, wcstring text, vec2 pos, color color, vec2 scale, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0,
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	Check2DCmdArrays(layer, font->tex, 0, scissorOffset, scissorExtent);
	
	switch (font->type){
		//// BDF (and NULL) font rendering ////
		case FontType_BDF: case FontType_NONE: {
			forI(text.count){
				u32       col = color.rgba;
				Vertex2*   vp = twodVertexArray + twodVertexCount;
				TwodIndexVk* ip = twodIndexArray + twodIndexCount;
				
				f32 w = font->max_width * scale.x;
				f32 h = font->max_height * scale.y;
				f32 dy = 1.f / (f32)font->count;
				
				f32 idx = f32(text[i] - 32);
				f32 topoff = idx * dy;
				f32 botoff = topoff + dy;
				
				ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
				ip[3] = twodVertexCount; ip[4] = twodVertexCount + 2; ip[5] = twodVertexCount + 3;
				vp[0].pos = { pos.x + 0,pos.y + 0 }; vp[0].uv = { 0,topoff }; vp[0].color = col;
				vp[1].pos = { pos.x + w,pos.y + 0 }; vp[1].uv = { 1,topoff }; vp[1].color = col;
				vp[2].pos = { pos.x + w,pos.y + h }; vp[2].uv = { 1,botoff }; vp[2].color = col;
				vp[3].pos = { pos.x + 0,pos.y + h }; vp[3].uv = { 0,botoff }; vp[3].color = col;
				
				twodVertexCount += 4;
				twodIndexCount += 6;
				twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
				pos.x += font->max_width * scale.x;
			}
		}break;
		//// TTF font rendering ////
		case FontType_TTF: {
			forI(text.count){
				u32       col = color.rgba;
				Vertex2*   vp = twodVertexArray + twodVertexCount;
				TwodIndexVk* ip = twodIndexArray + twodIndexCount;
				
				aligned_quad q = font->GetPackedQuad(text[i], &pos, scale);
				
				ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
				ip[3] = twodVertexCount; ip[4] = twodVertexCount + 2; ip[5] = twodVertexCount + 3;
				vp[0].pos = { q.x0,q.y0 }; vp[0].uv = { q.s0,q.t0 }; vp[0].color = col;
				vp[1].pos = { q.x1,q.y0 }; vp[1].uv = { q.s1,q.t0 }; vp[1].color = col;
				vp[2].pos = { q.x1,q.y1 }; vp[2].uv = { q.s1,q.t1 }; vp[2].color = col;
				vp[3].pos = { q.x0,q.y1 }; vp[3].uv = { q.s0,q.t1 }; vp[3].color = col;
				
				twodVertexCount += 4;
				twodIndexCount += 6;
				twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
			}break;
			default: Assert(!"unhandled font type"); break;
		}
	}
}

void Render::
DrawTexture2D(Texture* texture, vec2 p0, vec2 p1, vec2 p2, vec2 p3, f32 alpha, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0,
		   "Scissor Offset and Extent can't be negative");
	if(alpha == 0) return;
	
	Check2DCmdArrays(layer, texture, 1, scissorOffset, scissorExtent);
	
	u32         col = PackColorU32(255, 255, 255, 255.f * alpha);
	Vertex2*     vp = twodVertexArray + twodVertexCount;
	TwodIndexVk* ip = twodIndexArray + twodIndexCount;
	
	ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
	ip[3] = twodVertexCount; ip[4] = twodVertexCount + 2; ip[5] = twodVertexCount + 3;
	vp[0].pos = p0; vp[0].uv = { 0,1 }; vp[0].color = col;
	vp[1].pos = p1; vp[1].uv = { 1,1 }; vp[1].color = col;
	vp[2].pos = p2; vp[2].uv = { 1,0 }; vp[2].color = col;
	vp[3].pos = p3; vp[3].uv = { 0,0 }; vp[3].color = col;
	
	twodVertexCount += 4;
	twodIndexCount += 6;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
	
}

void Render::
DrawTexture2D(Texture* texture, vec2 pos, vec2 size, f32 rotation, f32 alpha, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	vec2 center = (pos + size) / 2,
	p0 = Math::vec2RotateByAngle(rotation, pos              - center) + center,
	p1 = Math::vec2RotateByAngle(rotation, pos.xAdd(size.x) - center) + center,
	p2 = Math::vec2RotateByAngle(rotation, pos + size       - center) + center,
	p3 = Math::vec2RotateByAngle(rotation, pos.yAdd(size.y) - center) + center;
	
	DrawTexture2D(texture, p0, p1, p2, p3, alpha, layer, scissorOffset, scissorExtent);
}

void Render::
StartNewTwodCmd(u32 layer, Texture* tex, vec2 scissorOffset, vec2 scissorExtent){
	twodCmdArrays[layer][twodCmdCounts[layer]].scissorOffset = scissorOffset;
	twodCmdArrays[layer][twodCmdCounts[layer]].scissorExtent = scissorExtent;
	twodCmdArrays[layer][twodCmdCounts[layer]].descriptorSet = textures[(tex ? tex->idx : 1)].descriptorSet;
	twodCmdArrays[layer][twodCmdCounts[layer]].indexOffset = twodIndexCount;
	twodCmdArrays[layer][twodCmdCounts[layer]].textured = (tex) ? true : false;
	twodCmdCounts[layer]++;
}

void Render::
AddTwodVertices(u32 layer, Vertex2* vertstart, u32 vertcount, u32* indexstart, u32 indexcount){
	Assert(vertcount + twodVertexCount < MAX_TWOD_VERTICES);
	Assert(indexcount + twodIndexCount < MAX_TWOD_INDICES);
	
	Vertex2*     vp = twodVertexArray + twodVertexCount;
	TwodIndexVk* ip = twodIndexArray + twodIndexCount;
	
	memcpy(vp, vertstart, vertcount * sizeof(Vertex2));
	forI(indexcount) ip[i] = twodVertexCount + indexstart[i];
	
	twodVertexCount += vertcount;
	twodIndexCount += indexcount;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += indexcount;
}

///////////////////
//// @settings ////
///////////////////
void Render::
SaveSettings(){
	Assets::saveConfig("render.cfg", configMap);
}

void Render::
LoadSettings(){
	Assets::loadConfig("render.cfg", configMap);
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

///////////////
//// @load ////
///////////////
//TODO(delle) upload extra mesh data to an SSBO
void Render::
LoadMesh(Mesh* mesh){
	AssertRS(RSVK_LOGICALDEVICE, "LoadMesh called before CreateLogicalDevice");
	MeshVk mvk{};
	mvk.base = mesh;
	mvk.vtxCount = mesh->vertexCount;
	mvk.idxCount = mesh->indexCount;
	if(vkMeshes.count){
		mvk.vtxOffset = vkMeshes.last->vtxOffset + vkMeshes.last->vtxCount;
		mvk.idxOffset = vkMeshes.last->idxOffset + vkMeshes.last->idxCount;
	}
	
	u64 mesh_vb_size   = mesh->vertexCount*sizeof(Mesh::Vertex);
	u64 mesh_ib_size   = mesh->indexCount*sizeof(Mesh::Index);
	u64 mesh_vb_offset = meshVertexBuffer.size;
	u64 mesh_ib_offset = meshIndexBuffer.size;
	u64 total_vb_size  = meshVertexBuffer.size + mesh_vb_size;
	u64 total_ib_size  = meshIndexBuffer.size  + mesh_ib_size;
	
	//create/resize buffers
	if(meshVertexBuffer.buffer == VK_NULL_HANDLE || meshVertexBuffer.size < total_vb_size){
		CreateOrResizeBuffer(&meshVertexBuffer, total_vb_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)meshVertexBuffer.buffer, "Mesh vertex buffer");
	}
	if(meshIndexBuffer.buffer == VK_NULL_HANDLE || meshIndexBuffer.size < total_ib_size){
		CreateOrResizeBuffer(&meshIndexBuffer, total_ib_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)meshIndexBuffer.buffer, "Mesh index buffer");
	}
	
	//copy memory to the GPU
	void* vb_data; void* ib_data;
	AssertVk(vkMapMemory(device, meshVertexBuffer.memory, mesh_vb_offset, mesh_vb_size, 0, &vb_data));
	AssertVk(vkMapMemory(device, meshIndexBuffer.memory,  mesh_ib_offset, mesh_ib_size, 0, &ib_data));
	{
		memcpy(vb_data, mesh->vertexArray, mesh_vb_size);
		memcpy(ib_data, mesh->indexArray,  mesh_ib_size);
		
		VkMappedMemoryRange range[2] = {};
		range[0].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[0].memory = meshVertexBuffer.memory;
		range[0].offset = RoundUpTo(mesh_vb_offset, physicalDeviceProperties.limits.nonCoherentAtomSize);
		range[0].size   = VK_WHOLE_SIZE;
		range[1].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[1].memory = meshIndexBuffer.memory;
		range[1].offset = RoundUpTo(mesh_ib_offset, physicalDeviceProperties.limits.nonCoherentAtomSize);
		range[1].size   = VK_WHOLE_SIZE;
		AssertVk(vkFlushMappedMemoryRanges(device, 2, range));
	}
	vkUnmapMemory(device, meshVertexBuffer.memory);
	vkUnmapMemory(device, meshIndexBuffer.memory);
	
	vkMeshes.add(mvk);
}

void Render::
LoadTexture(Texture* texture){
	AssertRS(RSVK_COMMANDPOOL, "LoadTexture called before CreateCommandPool");
	TextureVk tvk{};
	tvk.base = texture;
	tvk.size = texture->width * texture->height * 4;
	
	//determine image format
	VkFormat image_format;
	switch(texture->format){ //TODO(delle) handle non RGBA formats
		case ImageFormat_BW:   image_format = VK_FORMAT_R8G8B8A8_SRGB; break;
		case ImageFormat_BWA:  image_format = VK_FORMAT_R8G8B8A8_SRGB; break;
		case ImageFormat_RGB:  image_format = VK_FORMAT_R8G8B8A8_SRGB; break;
		case ImageFormat_RGBA: image_format = VK_FORMAT_R8G8B8A8_SRGB; break;
		default: PrintVk(0,"Failed to load texture '",texture->name,"' because of unknown texture format"); return;
	}
	
	//determine image type
	VkImageType     image_type;
	VkImageViewType view_type;
	switch(texture->type){ //TODO(delle) handle non 2D image types
		case TextureType_1D:         image_type = VK_IMAGE_TYPE_1D; view_type = VK_IMAGE_VIEW_TYPE_1D;         Assert(!"not implemented yet"); break;
		case TextureType_2D:         image_type = VK_IMAGE_TYPE_2D; view_type = VK_IMAGE_VIEW_TYPE_2D;         break;
		case TextureType_3D:         image_type = VK_IMAGE_TYPE_3D; view_type = VK_IMAGE_VIEW_TYPE_3D;         Assert(!"not implemented yet"); break;
		case TextureType_Cube:       image_type = VK_IMAGE_TYPE_2D; view_type = VK_IMAGE_VIEW_TYPE_CUBE;       Assert(!"not implemented yet"); break;
		case TextureType_Array_1D:   image_type = VK_IMAGE_TYPE_1D; view_type = VK_IMAGE_VIEW_TYPE_1D_ARRAY;   Assert(!"not implemented yet"); break;
		case TextureType_Array_2D:   image_type = VK_IMAGE_TYPE_2D; view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;   Assert(!"not implemented yet"); break;
		case TextureType_Array_Cube: image_type = VK_IMAGE_TYPE_2D; view_type = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY; Assert(!"not implemented yet"); break;
		default: PrintVk(0,"Failed to load texture '",texture->name,"' because of unknown image type"); return;
	}
	
	//copy the image pixels to a staging buffer
	BufferVk staging{};
	CreateAndMapBuffer(staging.buffer, staging.memory, tvk.size, (size_t)tvk.size, texture->pixels, 
					   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	//create the texture image
	VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	imageInfo.imageType     = image_type;
	imageInfo.extent        = {(u32)texture->width, (u32)texture->height, 1};
	imageInfo.mipLevels     = texture->mipmaps;
	imageInfo.arrayLayers   = 1; //NOTE(delle) use image flags here?
	imageInfo.format        = image_format;
	imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT; //NOTE(delle) extra texture samples?
	imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	AssertVk(vkCreateImage(device, &imageInfo, allocator, &tvk.image), "failed to create image");
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, tvk.image, &memRequirements);
	VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	AssertVk(vkAllocateMemory(device, &allocInfo, allocator, &tvk.memory), "failed to allocate image memory");
	vkBindImageMemory(device, tvk.image, tvk.memory, 0);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE,(u64)tvk.image, toStr("Texture image ", texture->name).str);
	
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();{
		//transition image layout to accept memory transfers
		VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		barrier.srcAccessMask       = 0;
		barrier.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image               = tvk.image;
		barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel   = 0;
		barrier.subresourceRange.levelCount     = texture->mipmaps;
		barrier.subresourceRange.baseArrayLayer = 0; //NOTE(delle) use image flags here?
		barrier.subresourceRange.layerCount     = 1; //NOTE(delle) use image flags here?
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
							 0, 0, nullptr, 0, nullptr, 1, &barrier);
		
		//copy the staging buffer to the image
		VkBufferImageCopy region{};
		region.bufferOffset      = 0;
		region.bufferRowLength   = 0;
		region.bufferImageHeight = 0;
		region.imageOffset       = {0, 0, 0};
		region.imageExtent       = {(u32)texture->width, (u32)texture->height, 1};
		region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel       = 0;
		region.imageSubresource.baseArrayLayer = 0; //NOTE(delle) use image flags here?
		region.imageSubresource.layerCount     = 1; //NOTE(delle) use image flags here?
		vkCmdCopyBufferToImage(commandBuffer, staging.buffer, tvk.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}EndSingleTimeCommands(commandBuffer);
	
	//cleanup staging memory
	vkDestroyBuffer(device, staging.buffer, allocator);
	vkFreeMemory(device, staging.memory, allocator);
	
	//generate texture mipmaps (image layout set to SHADER_READ_ONLY in GenerateMipmaps)
	GenerateMipmaps(tvk.image, image_format, texture->width, texture->height, texture->mipmaps);
	
	//create texture imageview
	VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	viewInfo.image    = tvk.image;
	viewInfo.viewType = view_type;
	viewInfo.format   = image_format;
	viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel   = 0;
	viewInfo.subresourceRange.levelCount     = texture->mipmaps;
	viewInfo.subresourceRange.baseArrayLayer = 0; //NOTE(delle) use image flags here?
	viewInfo.subresourceRange.layerCount     = 1; //NOTE(delle) use image flags here?
	AssertVk(vkCreateImageView(device, &viewInfo, allocator, &tvk.view), "failed to create texture image view");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)tvk.view,
						 toStr("Image View ", texture->name).str);
	
	//create texture sampler
	VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	switch(texture->filter){
		case TextureFilter_Nearest:{
			samplerInfo.magFilter    = VK_FILTER_NEAREST;
			samplerInfo.minFilter    = VK_FILTER_NEAREST;
			samplerInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		}break;
		case TextureFilter_Linear:{
			samplerInfo.magFilter    = VK_FILTER_LINEAR;
			samplerInfo.minFilter    = VK_FILTER_LINEAR;
			samplerInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}break;
		case TextureFilter_Cubic:{
			samplerInfo.magFilter    = VK_FILTER_CUBIC_EXT;
			samplerInfo.minFilter    = VK_FILTER_CUBIC_EXT;
			samplerInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}break;
		default: LogE("vulkan", "Unhandled texture filter: ", texture->filter); break;
	}
	switch(texture->uvMode){
		case TextureAddressMode_Repeat:{
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}break;
		case TextureAddressMode_MirroredRepeat:{
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		}break;
		case TextureAddressMode_ClampToEdge:{
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		}break;
		case TextureAddressMode_ClampToWhite:{
			samplerInfo.borderColor  = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		}break;
		case TextureAddressMode_ClampToBlack:{
			samplerInfo.borderColor  = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		}break;
		case TextureAddressMode_ClampToTransparent:{
			samplerInfo.borderColor  = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		}break;
		default: LogE("vulkan", "Unhandled texture address mode: ", texture->uvMode); break;
	}
	samplerInfo.anisotropyEnable = settings.anistropicFiltering;
	samplerInfo.maxAnisotropy    = (settings.anistropicFiltering) ?  physicalDeviceProperties.limits.maxSamplerAnisotropy : 1.0f;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable    = VK_FALSE;
	samplerInfo.compareOp        = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipLodBias       = 0.0f;
	samplerInfo.minLod           = 0.0f;
	samplerInfo.maxLod           = (f32)texture->mipmaps;
	AssertVk(vkCreateSampler(device, &samplerInfo, nullptr, &tvk.sampler), "failed to create texture sampler");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SAMPLER, (u64)tvk.sampler,
						 toStr("Sampler ", texture->name).str);
	
	//fill texture descriptor image info
	tvk.descriptor.imageView   = tvk.view;
	tvk.descriptor.sampler     = tvk.sampler;
	tvk.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	//allocate descriptor set
	VkDescriptorSetAllocateInfo setAllocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	setAllocInfo.descriptorPool = descriptorPool;
	setAllocInfo.pSetLayouts = &descriptorSetLayouts.twod;
	setAllocInfo.descriptorSetCount = 1;
	AssertVk(vkAllocateDescriptorSets(device, &setAllocInfo, &tvk.descriptorSet));
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)tvk.descriptorSet,
						 toStr("Texture descriptor set ", texture->name).str);
	
	VkWriteDescriptorSet set{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	set.dstSet = tvk.descriptorSet;
	set.dstArrayElement = 0;
	set.descriptorCount = 1;
	set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	set.pImageInfo = &tvk.descriptor;
	set.dstBinding = 0;
	
	vkUpdateDescriptorSets(device, 1, &set, 0, nullptr);
	
	textures.add(tvk);
}

//TODO(delle) this currently requires 4 textures, fix that
void Render::
LoadMaterial(Material* material){
	AssertRS(RSVK_DESCRIPTORPOOL, "LoadMaterial called before CreateDescriptorPool");
	MaterialVk mvk{};
	mvk.base     = material;
	mvk.pipeline = GetPipelineFromShader(material->shader);
	
	//allocate descriptor set
	VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	allocInfo.descriptorPool     = descriptorPool;
	allocInfo.pSetLayouts        = &descriptorSetLayouts.textures;
	allocInfo.descriptorSetCount = 1;
	AssertVk(vkAllocateDescriptorSets(device, &allocInfo, &mvk.descriptorSet));
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)mvk.descriptorSet,
						 toStr("Material descriptor set ",material->name).str);
	
	//write descriptor set per texture
	array<VkWriteDescriptorSet> sets;
	for(u32 texIdx : material->textures){
		VkWriteDescriptorSet set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		set.dstSet          = mvk.descriptorSet;
		set.dstArrayElement = 0;
		set.descriptorCount = 1;
		set.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		set.pImageInfo      = &textures[texIdx].descriptor;
		set.dstBinding      = sets.size();
		sets.add(set);
	}
	vkUpdateDescriptorSets(device, sets.size(), sets.data, 0, nullptr);
	
	//HACK to fix materials with no textures
	if(material->textures.size() < 4){
		forI(4 - sets.size()){
			VkWriteDescriptorSet set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
			set.dstSet          = mvk.descriptorSet;
			set.dstArrayElement = 0;
			set.descriptorCount = 1;
			set.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			set.pImageInfo      = &textures[0].descriptor;
			set.dstBinding      = sets.size();
			sets.add(set);
		}
		vkUpdateDescriptorSets(device, sets.size(), sets.data, 0, nullptr);
	}
	
	vkMaterials.add(mvk);
}

/////////////////
//// @update ////
/////////////////
void Render::
UpdateMaterial(Material* material){
	MaterialVk* mvk = &vkMaterials[material->idx];
	mvk->pipeline = GetPipelineFromShader(material->shader);
	
	//update descriptor set per texture
	array<VkWriteDescriptorSet> sets;
	for(u32 texIdx : material->textures){
		VkWriteDescriptorSet set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		set.dstSet          = mvk->descriptorSet;
		set.dstArrayElement = 0;
		set.descriptorCount = 1;
		set.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		set.pImageInfo      = &textures[texIdx].descriptor;
		set.dstBinding      = sets.size();
		sets.add(set);
	}
	vkUpdateDescriptorSets(device, sets.size(), sets.data, 0, nullptr);
	
	//HACK to fix materials with no textures
	if(material->shader == Shader_PBR && material->textures.size() < 4){
		forI(4 - sets.size()){
			VkWriteDescriptorSet set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
			set.dstSet          = mvk->descriptorSet;
			set.dstArrayElement = 0;
			set.descriptorCount = 1;
			set.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			set.pImageInfo      = &textures[0].descriptor;
			set.dstBinding      = sets.size();
			sets.add(set);
		}
		vkUpdateDescriptorSets(device, sets.size(), sets.data, 0, nullptr);
	}
}


/////////////////
//// @unload ////
/////////////////
void Render::
UnloadTexture(Texture* texture){
	Assert(!"not implemented yet"); //!Incomplete
}

void Render::
UnloadMaterial(Material* material){
	Assert(!"not implemented yet"); //!Incomplete
}

void Render::
UnloadMesh(Mesh* mesh){
	Assert(!"not implemented yet"); //!Incomplete
}

///////////////
//// @draw ////
///////////////
//TODO(delle) redo model drawing, i dont really like it, and i kinda want it to be cached if possible
//    things that need to be fixed/better:
//      1. the matrix is duplicated per batch
//      2. most commands will be remade every frame the exact same with just the matrix differing
//      3. this relies on scene mesh indexes matching renderer mesh indexes
void Render:: 
DrawModel(Model* model, mat4 matrix){
	Assert(modelCmdCount + model->batches.size() < MAX_MODEL_CMDS, "attempted to draw more than the global maximum number of batches");
	ModelCmdVk* cmd = modelCmdArray + modelCmdCount;
	
	forI(model->batches.size()){
		if(!model->batches[i].indexCount) continue;
		cmd[i].vertexOffset = vkMeshes[model->mesh->idx].vtxOffset;
		cmd[i].indexOffset  = vkMeshes[model->mesh->idx].idxOffset + model->batches[i].indexOffset;
		cmd[i].indexCount   = model->batches[i].indexCount;
		cmd[i].material     = model->batches[i].material;
		cmd[i].name         = model->name;
		cmd[i].matrix       = matrix;
		modelCmdCount += 1;
	}
}

void Render::
DrawModelWireframe(Model* model, mat4 matrix, color color){
	Assert(!"not implemented yet"); //!Incomplete
}

void Render::
DrawLine(vec3 start, vec3 end, color color){
	if(color.a == 0) return;
	
	u32 col = color.rgba;
	Mesh::Vertex* vp = tempWireframeVertexArray + tempWireframeVertexCount;
	TempIndexVk*  ip = tempWireframeIndexArray + tempWireframeIndexCount;
	
	ip[0] = tempWireframeVertexCount; 
	ip[1] = tempWireframeVertexCount+1; 
	ip[2] = tempWireframeVertexCount;
	vp[0].pos = start; vp[0].color = col;
	vp[1].pos = end;   vp[1].color = col;
	
	tempWireframeVertexCount += 2;
	tempWireframeIndexCount  += 3;
}

void Render::
DrawTriangle(vec3 p0, vec3 p1, vec3 p2, color color){
	if(color.a == 0) return;
	
	u32 col = color.rgba;
	Mesh::Vertex* vp = tempWireframeVertexArray + tempWireframeVertexCount;
	TempIndexVk*   ip = tempWireframeIndexArray + tempWireframeIndexCount;
	
	ip[0] = tempWireframeVertexCount; 
	ip[1] = tempWireframeVertexCount+1; 
	ip[2] = tempWireframeVertexCount+2;
	vp[0].pos = p0; vp[0].color = col;
	vp[1].pos = p1; vp[1].color = col;
	vp[2].pos = p2; vp[2].color = col;
	
	tempWireframeVertexCount += 3;
	tempWireframeIndexCount  += 3;
}

void Render::
DrawTriangleFilled(vec3 p0, vec3 p1, vec3 p2, color color){
	if(color.a == 0) return;
	
	u32 col = color.rgba;
	Mesh::Vertex* vp = tempFilledVertexArray + tempFilledVertexCount;
	TempIndexVk*   ip = tempFilledIndexArray + tempFilledIndexCount;
	
	ip[0] = tempFilledVertexCount; 
	ip[1] = tempFilledVertexCount+1; 
	ip[2] = tempFilledVertexCount+2;
	vp[0].pos = p0; vp[0].color = col;
	vp[1].pos = p1; vp[1].color = col;
	vp[2].pos = p2; vp[2].color = col;
	
	tempFilledVertexCount += 3;
	tempFilledIndexCount  += 3;
}

void Render::
DrawQuad(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color color){
	if(color.a == 0) return;
	DrawLine(p0, p1, color);
	DrawLine(p1, p2, color);
	DrawLine(p2, p3, color);
	DrawLine(p3, p0, color);
}

inline void Render::
DrawQuadFilled(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color color){
	if(color.a == 0) return;
	DrawTriangleFilled(p0, p1, p2, color);
	DrawTriangleFilled(p0, p2, p3, color);
}

void Render::
DrawPoly(array<vec3>& points, color color){
	Assert(points.count > 2);
	if(color.a == 0) return;
	for(s32 i=1; i<points.count-1; ++i) DrawLine(points[i-1], points[i], color);
	DrawLine(points[points.count-2], points[points.count-1], color);
}

void Render::
DrawPolyFilled(array<vec3>& points, color color){
	Assert(points.count > 2);
	if(color.a == 0) return;
	for(s32 i=2; i<points.count-1; ++i) DrawTriangleFilled(points[i-2], points[i-1], points[i], color);
	DrawTriangle(points[points.count-3], points[points.count-2], points[points.count-1], color);
}

void Render::
DrawBox(const mat4& transform, color color){
	if(color.a == 0) return;
	
	vec3 p(0.5f, 0.5f, 0.5f);
	vec3 points[8] = {
		{-p.x, p.y, p.z},{-p.x,-p.y, p.z},{-p.x, p.y,-p.z},{-p.x,-p.y,-p.z},
		{ p.x, p.y, p.z},{ p.x,-p.y, p.z},{ p.x, p.y,-p.z},{ p.x,-p.y,-p.z},
	};
	forI(8){ points[i] = points[i] * transform; }
	DrawLine(points[3], points[1], color); DrawLine(points[3], points[2], color); DrawLine(points[3], points[7], color);
	DrawLine(points[0], points[1], color); DrawLine(points[0], points[2], color); DrawLine(points[0], points[4], color);
	DrawLine(points[5], points[1], color); DrawLine(points[5], points[4], color); DrawLine(points[5], points[7], color);
	DrawLine(points[6], points[2], color); DrawLine(points[6], points[4], color); DrawLine(points[6], points[7], color);
}

void Render::
DrawBoxFilled(const mat4& transform, color color){
	if(color.a == 0) return;
	
	vec3 p(0.5f, 0.5f, 0.5f);
	vec3 points[8] = {
		{-p.x, p.y, p.z},{-p.x,-p.y, p.z},{-p.x, p.y,-p.z},{-p.x,-p.y,-p.z},
		{ p.x, p.y, p.z},{ p.x,-p.y, p.z},{ p.x, p.y,-p.z},{ p.x,-p.y,-p.z},
	};
	forI(8){ points[i] = points[i] * transform; }
	DrawTriangleFilled(points[4], points[2], points[0], color); DrawTriangleFilled(points[4], points[6], points[2], color);
	DrawTriangleFilled(points[2], points[7], points[3], color); DrawTriangleFilled(points[2], points[6], points[7], color);
	DrawTriangleFilled(points[6], points[5], points[7], color); DrawTriangleFilled(points[6], points[4], points[5], color);
	DrawTriangleFilled(points[1], points[7], points[5], color); DrawTriangleFilled(points[1], points[3], points[7], color);
	DrawTriangleFilled(points[0], points[3], points[1], color); DrawTriangleFilled(points[0], points[2], points[3], color);
	DrawTriangleFilled(points[4], points[1], points[5], color); DrawTriangleFilled(points[4], points[0], points[1], color);
}

void Render::
DrawCircle(vec3 position, vec3 rotation, f32 radius, u32 subdivisions_int, color c){
	mat4 transform = mat4::TransformationMatrix(position, rotation, vec3::ONE);
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (f32(i-1)*M_2PI) / subdivisions;
		f32 a1 = (f32(i  )*M_2PI) / subdivisions;
		f32 x0 = radius*cosf(a0); f32 x1 = radius*cosf(a1);
		f32 y0 = radius*sinf(a0); f32 y1 = radius*sinf(a1);
		vec3 xaxis0 = vec3{0, y0, x0} * transform; vec3 xaxis1 = vec3{0, y1, x1} * transform;
		DrawLine(xaxis0, xaxis1, c);
	}
}

void Render::
DrawSphere(vec3 position, vec3 rotation, f32 radius, u32 subdivisions_int, color cx, color cy, color cz){
	mat4 transform = mat4::TransformationMatrix(position, rotation, vec3::ONE);
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (f32(i-1)*M_2PI) / subdivisions;
		f32 a1 = (f32(i  )*M_2PI) / subdivisions;
		f32 x0 = radius*cosf(a0); f32 x1 = radius*cosf(a1);
		f32 y0 = radius*sinf(a0); f32 y1 = radius*sinf(a1);
		vec3 xaxis0 = vec3{0, y0, x0} * transform; vec3 xaxis1 = vec3{0, y1, x1} * transform;
		vec3 yaxis0 = vec3{x0, 0, y0} * transform; vec3 yaxis1 = vec3{x1, 0, y1} * transform;
		vec3 zaxis0 = vec3{x0, y0, 0} * transform; vec3 zaxis1 = vec3{x1, y1, 0} * transform;
		DrawLine(xaxis0, xaxis1, cx); DrawLine(yaxis0, yaxis1, cy); DrawLine(zaxis0, zaxis1, cz);
	}
}

void Render::
DrawFrustrum(vec3 position, vec3 target, f32 aspectRatio, f32 fovx, f32 nearZ, f32 farZ, color color){
	if(color.a == 0) return;
	
	f32 y = tanf(Radians(fovx / 2.0f));
	f32 x = y * aspectRatio;
	f32 nearX = x * nearZ;
	f32 farX  = x * farZ;
	f32 nearY = y * nearZ;
	f32 farY  = y * farZ;
	
	vec4 faces[8] = {
		//near face
		{ nearX,  nearY, nearZ, 1},
		{-nearX,  nearY, nearZ, 1},
		{ nearX, -nearY, nearZ, 1},
		{-nearX, -nearY, nearZ, 1},
		
		//far face
		{ farX,  farY, farZ, 1},
		{-farX,  farY, farZ, 1},
		{ farX, -farY, farZ, 1},
		{-farX, -farY, farZ, 1},
	};
	
	mat4 mat = Math::LookAtMatrix(position, target);
	vec3 v[8];
	forI(8){
		vec4 temp = faces[i] * mat;
		v[i].x = temp.x / temp.w;
		v[i].y = temp.y / temp.w;
		v[i].z = temp.z / temp.w;
	}
	
	DrawLine(v[0], v[1], color);
	DrawLine(v[0], v[2], color);
	DrawLine(v[3], v[1], color);
	DrawLine(v[3], v[2], color);
	DrawLine(v[4], v[5], color);
	DrawLine(v[4], v[6], color);
	DrawLine(v[7], v[5], color);
	DrawLine(v[7], v[6], color);
	DrawLine(v[0], v[4], color);
	DrawLine(v[1], v[5], color);
	DrawLine(v[2], v[6], color);
	DrawLine(v[3], v[7], color);
}


////////////////
//// @debug ////
////////////////
void Render::
ClearDebug(){
	debugWireframeVertexCount = 0;
	debugWireframeIndexCount  = 0;
	debugFilledVertexCount = 0;
	debugFilledIndexCount  = 0;
}

void Render::
DebugLine(vec3 start, vec3 end, color color){
	if(color.a == 0) return;
	
	u32 col = color.rgba;
	Mesh::Vertex* vp = debugWireframeVertexArray + debugWireframeVertexCount;
	TempIndexVk*  ip = debugWireframeIndexArray + debugWireframeIndexCount;
	
	ip[0] = debugWireframeVertexCount; 
	ip[1] = debugWireframeVertexCount+1; 
	ip[2] = debugWireframeVertexCount;
	vp[0].pos = start; vp[0].color = col;
	vp[1].pos = end;   vp[1].color = col;
	
	debugWireframeVertexCount += 2;
	debugWireframeIndexCount  += 3;
}

/////////////
//// @ui ////
/////////////
//TODO(sushi) find a nicer way to keep track of this
//NOTE im not sure yet if i should be keeping track of this for each primitive or not yet but i dont think i have to

/////////////////
//// @camera ////
/////////////////
void Render::
UpdateCameraPosition(vec3 position){
	uboVS.values.viewPos = vec4(position, 1.f);
}

void Render::
UpdateCameraViewMatrix(mat4 m){
	uboVS.values.view = m;
}

void Render::
UpdateCameraProjectionMatrix(mat4 m){
	uboVS.values.proj = m;
}

void Render::
UseDefaultViewProjMatrix(vec3 position, vec3 rotation){
	vec3 forward = (vec3::FORWARD * mat4::RotationMatrix(rotation)).normalized();
	uboVS.values.view = Math::LookAtMatrix(position, position + forward).Inverse();
	uboVS.values.proj = Camera::MakePerspectiveProjectionMatrix((f32)DeshWindow->width, (f32)DeshWindow->height, 90.f, 1000.f, 0.1f);
}

//////////////////
//// @shaders ////
//////////////////
void Render::
ReloadShader(u32 shader){
	switch(shader){
		case(Shader_NULL):{ 
			vkDestroyPipeline(device, pipelines.null, nullptr);
			shaderStages[0] = CompileAndLoadShader("null.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("null.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			AssertVk(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.null));
		}break;
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

////////////////
//// @fixme ////
////////////////
void Render::
UpdateLight(u32 lightIdx, vec4 vec){
	vkLights[lightIdx] = vec;
}

void Render::
remakeOffscreen(){
	_remakeOffscreen = true;
}

///////////////
//// @init ////
///////////////
void Render::
Init(){
	AssertDS(DS_MEMORY, "Attempt to init Vulkan without loading Memory first");
	AssertDS(DS_LOGGER, "Attempt to init Vulkan without loading Logger first");
	AssertDS(DS_WINDOW, "Attempt to init Vulkan without loading Window first");
	deshiStage |= DS_RENDER;
	
	TIMER_START(t_s);
	Log("vulkan","Starting vulkan renderer initialization");
	Logger::PushIndent();
	
	//// load RenderSettings ////
	LoadSettings();
	if(settings.debugging && settings.printf){
		validationFeaturesEnabled.add(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
		settings.loggingLevel = 4;
	}
	
	TIMER_START(t_temp);
	//// setup Vulkan instance ////
	SetupAllocator();
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
	msaaSamples = (VkSampleCountFlagBits)(((1 << settings.msaaSamples) > maxMsaaSamples) ? maxMsaaSamples : 1 << settings.msaaSamples);
	settings.anistropicFiltering = (enabledFeatures.samplerAnisotropy) ? settings.anistropicFiltering : false;
	
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
	CreateRenderpasses();
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
	
	forI(TWOD_LAYERS){ twodCmdCounts[i] = 1; }
	initialized = true;
	
	Logger::PopIndent();
	LogS("deshi","Finished vulkan renderer initialization in ",TIMER_END(t_s),"ms");
}


/////////////////
//// @update ////
/////////////////
void Render::
Update(){
	TIMER_START(t_d);
	AssertRS(RSVK_PIPELINECREATE | RSVK_FRAMES | RSVK_SYNCOBJECTS, "Render called before CreatePipelines or CreateFrames or CreateSyncObjects");
	rendererStage = RSVK_RENDER;
	
	if(DeshWindow->resized) remakeWindow = true;
	if(remakeWindow){
		DeshWindow->GetScreenSize(width, height);
		if(width <= 0 || height <= 0){ ImGui::EndFrame(); return; }
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
	if(settings.lightFrustrums) DrawFrustrum(vkLights[0].toVec3(), vec3::ZERO, 1, 90, settings.shadowNearZ, settings.shadowFarZ);
	if(DeshiModuleLoaded(DS_IMGUI)){
		ImGui::Render();
	}
	UpdateUniformBuffers();
	SetupCommands();
	
	//execute draw commands
	BuildCommands();
	
	//submit the command buffer to the queue
	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submitInfo.waitSemaphoreCount   = 1;
	submitInfo.pWaitSemaphores      = &imageAcquiredSemaphore;
	submitInfo.pWaitDstStageMask    = &wait_stage;
	submitInfo.commandBufferCount   = 1;
	submitInfo.pCommandBuffers      = &frames[imageIndex].commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores    = &renderCompleteSemaphore;
	AssertVk(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE), "failed to submit draw command buffer");
	
	if(remakeWindow){ return; }
	
	//present the image
	VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    = &renderCompleteSemaphore;
	presentInfo.swapchainCount     = 1;
	presentInfo.pSwapchains        = &swapchain;
	presentInfo.pImageIndices      = &imageIndex;
	presentInfo.pResults           = 0;
	result = vkQueuePresentKHR(presentQueue, &presentInfo);
	
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || remakeWindow){  //!Cleanup remakeWindow is already checked
		vkDeviceWaitIdle(device);
		CreateSwapChain();
		CreateFrames();
		remakeWindow = false;
	}else if(result != VK_SUCCESS){
		Assert(!"failed to present swap chain image");
	}
	
	//iterate the frame index
	frameIndex = (frameIndex + 1) % minImageCount; //loops back to zero after reaching minImageCount
	result = vkQueueWaitIdle(graphicsQueue);
	switch(result){
		case VK_ERROR_OUT_OF_HOST_MEMORY:   LogE("vulkan","OUT_OF_HOST_MEMORY");   Assert(!"CPU ran out of memory"); break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: LogE("vulkan","OUT_OF_DEVICE_MEMORY"); Assert(!"GPU ran out of memory"); break;
		case VK_ERROR_DEVICE_LOST:          LogE("vulkan","DEVICE_LOST");          Assert(!"Bad Sync/Overheat/Drive Bug"); break;
		case VK_SUCCESS:default: break;
	}
	
	//update stats
	stats.drawnTriangles += stats.drawnIndices / 3;
	//stats.totalVertices  += (u32)vertexBuffer.size() + twodVertexCount + tempWireframeVertexCount;
	//stats.totalIndices   += (u32)indexBuffer.size()  + twodIndexCount  + tempWireframeIndexCount; //!Incomplete
	stats.totalTriangles += stats.totalIndices / 3;
	stats.renderTimeMS    = TIMER_END(t_r);
	
	ResetCommands();
	
	if(remakePipelines){ 
		CreatePipelines(); 
		UpdateMaterialPipelines();
		remakePipelines = false; 
	}
	if(_remakeOffscreen){
		SetupOffscreenRendering();
		_remakeOffscreen = false;
	}
	DeshTime->renderTime = TIMER_END(t_d);
}


////////////////
//// @reset ////
////////////////
void Render::
Reset(){
	PrintVk(1,"Resetting renderer");
	
	vkDeviceWaitIdle(device); //wait before cleanup
	//TODO(delle) delete things
}


//////////////////
//// @cleanup ////
//////////////////
//TODO(delle,Vu) maybe cache pipeline creation vars?
void Render::
Cleanup(){
	PrintVk(1, "Initializing cleanup\n");
	
	Render::SaveSettings();
	//save pipeline cache to disk
	if(pipelineCache != VK_NULL_HANDLE){
		size_t size{};
		AssertVk(vkGetPipelineCacheData(device, pipelineCache, &size, nullptr), "failed to get pipeline cache data size");
		std::vector<char> data(size);
		AssertVk(vkGetPipelineCacheData(device, pipelineCache, &size, data.data()), "failed to get pipeline cache data");
		Assets::writeFileBinary(Assets::dirData() + "pipelines.cache", data);
	}
	
	vkDeviceWaitIdle(device);
}