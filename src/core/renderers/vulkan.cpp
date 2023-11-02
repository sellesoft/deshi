/* deshi Vulkan Render Submodule
Index:
@vk_types
@vk_vars
@vk_vars_init
@vk_vars_swapchain
@vk_vars_renderpass
@vk_vars_frames
@vk_vars_buffers
@vk_vars_pipelines
@vk_vars_shaders
@vk_vars_other
@vk_funcs_utils
@vk_funcs_memory
@vk_funcs_images
@vk_funcs_init
@vk_funcs_swapchain
@vk_funcs_renderpass
@vk_funcs_frames
@vk_funcs_buffers
@vk_funcs_other
@vk_funcs_shaders
@vk_funcs_pipelines_setup
@vk_funcs_pipelines_creation
@vk_funcs_commands_setup
@vk_funcs_commands_build
@vk_funcs_imgui
@render_init
@render_update
@render_reset
@render_cleanup
@render_loading
@render_draw_3d
@render_draw_2d
@render_buffer
@render_surface
@render_light
@render_camera
@render_shaders
@render_other
*/


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_types
struct TextureVk{
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

struct Push2DVk{
	vec2 scale;
	vec2 translate;
	s32 font_offset;
};

struct QueueFamilyIndices{
	optional<u32> graphicsFamily;
	optional<u32> presentFamily;
	bool isComplete(){ return graphicsFamily.test() && presentFamily.test(); }
};

struct SwapChainSupportDetails{
	VkSurfaceCapabilitiesKHR  capabilities;
	arrayT<VkSurfaceFormatKHR> formats;
	arrayT<VkPresentModeKHR>   presentModes;
};

struct SwapchainSupportDetailsX {
	VkSurfaceCapabilitiesKHR capabilities;
	VkSurfaceFormatKHR*      formats;
	VkPresentModeKHR*        present_modes;
};

struct FrameVk{
	VkImage         image         = VK_NULL_HANDLE;
	VkImageView     imageView     = VK_NULL_HANDLE;
	VkFramebuffer   framebuffer   = VK_NULL_HANDLE;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
};

struct FramebufferAttachmentsVk{
	VkImage        color_image        = VK_NULL_HANDLE;
	VkDeviceMemory color_image_memory = VK_NULL_HANDLE;
	VkImageView    color_image_view   = VK_NULL_HANDLE;
	VkImage        depth_image        = VK_NULL_HANDLE;
	VkDeviceMemory depth_image_memory = VK_NULL_HANDLE;
	VkImageView    depth_image_view   = VK_NULL_HANDLE;
};

struct BufferVk{
	VkBuffer               buffer;
	VkDeviceMemory         memory;
	VkDeviceSize           size; //size of data, not allocation
	VkDescriptorBufferInfo descriptor;
};


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_vars
#define INDEX_TYPE_VK_UI    VK_INDEX_TYPE_UINT32
#define INDEX_TYPE_VK_TEMP  VK_INDEX_TYPE_UINT32
#define INDEX_TYPE_VK_MESH  VK_INDEX_TYPE_UINT32
StaticAssertAlways(sizeof(RenderTwodIndex)  == 4);
StaticAssertAlways(sizeof(RenderTempIndex)  == 4);
StaticAssertAlways(sizeof(RenderModelIndex) == 4);

// TODO(sushi) replace with pools so we don't have the risk of desync'd indexes
local arrayT<RenderMesh>  vkMeshes(deshi_allocator);
local arrayT<TextureVk>   textures(deshi_allocator);
local arrayT<MaterialVk>  vkMaterials(deshi_allocator);
local vec4 vkLights[10]{ Vec4(0,0,0,-1) };

local bool initialized      = false;
local bool remakeWindow     = false;
local bool remakePipelines  = false;
local bool _remakeOffscreen = false;

local VkSampleCountFlagBits msaaSamples{};
local VkResult resultVk;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_vars_init
local VkAllocationCallbacks    allocator_             = {0};
local VkAllocationCallbacks*   allocator              = &allocator_;
local VkAllocationCallbacks    temp_allocator_        = {0};
local VkAllocationCallbacks*   temp_allocator         = &temp_allocator_;
local VkInstance               instance               = VK_NULL_HANDLE;
local VkDebugUtilsMessengerEXT debugMessenger         = VK_NULL_HANDLE;
local VkSurfaceKHR             surfaces[MAX_SURFACES] = { VK_NULL_HANDLE };
local VkPhysicalDevice         physical_device         = VK_NULL_HANDLE;
local VkPhysicalDeviceProperties physicalDeviceProperties{0};
local VkSampleCountFlagBits    maxMsaaSamples         = VK_SAMPLE_COUNT_1_BIT;
local VkPhysicalDeviceFeatures deviceFeatures         = {0};
local VkPhysicalDeviceFeatures enabledFeatures        = {0};
local QueueFamilyIndices       physicalQueueFamilies  = {};
local VkDevice                 device                 = VK_NULL_HANDLE;
local VkQueue                  graphicsQueue          = VK_NULL_HANDLE;
local VkQueue                  presentQueue           = VK_NULL_HANDLE;
local VkDeviceSize             buffer_memory_alignment  = 256;

// memory_pool
// handles are stored on Windows
local VkSurfaceKHR* surfaces_x;

struct QueueFamilyIndexesX {
	b32 found_graphics_family;
	u32 graphics_family;
	b32 found_present_family;
	u32 present_family;
};

local QueueFamilyIndexesX physical_queue_families = {};

local const char* validation_layers[] = {
	"VK_LAYER_KHRONOS_validation"
};
local char* deviceExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME
};
local arrayT<VkValidationFeatureEnableEXT> validation_features_enabled;
local VkValidationFeatureEnableEXT* validation_features_enabled_x;

local VkDebugUtilsMessageSeverityFlagsEXT callback_severities = 
// VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
local VkDebugUtilsMessageTypeFlagsEXT callback_types = 
VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_vars_swapchain
struct VkSwapchain{
	s32                      width          = 0;
	s32                      height         = 0;
	VkSwapchainKHR           swapchain      = VK_NULL_HANDLE;
	SwapChainSupportDetails  supportDetails = {};
	VkSurfaceFormatKHR       surfaceFormat  = {};
	VkPresentModeKHR         presentMode    = VK_PRESENT_MODE_FIFO_KHR;
	VkExtent2D               extent         = {};
	s32                      minImageCount  = 0;
	Window*                  window         = 0;
	u32                      imageCount     = 0;
	u32                      frameIndex     = 0;
	arrayT<FrameVk>          frames;
	FramebufferAttachmentsVk attachments{};
};

struct VkWindowInfo {
	s32 width;
	s32 height;
	VkSwapchainKHR           swapchain;
	SwapchainSupportDetailsX support_details;
	VkSurfaceKHR             surface;
	VkSurfaceFormatKHR       surface_format;
	VkPresentModeKHR         present_mode;
	VkExtent2D               extent;
	s32                      min_image_count;
	u32                      image_count;
	u32                      frame_index;
	FrameVk*                 frames;
	FramebufferAttachmentsVk attachments;
};

#define WindowFromVkSwapchain(ptr) CastFromMember(Window, render_info, ptr)

local VkSwapchain swapchains[MAX_SURFACES];
//TODO make all of the Create functions take in a swapchain/surface rather than always indexing with a global var :)
#define activeSwapchain swapchains[renderActiveSurface] //TODO replace these with original text eventually or make better getter
#define activeSwapchainKHR swapchains[renderActiveSurface].swapchain
local VkWindowInfo* window_infos;

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_vars_renderpass
local VkRenderPass baseRenderPass = VK_NULL_HANDLE;
local VkRenderPass msaaRenderPass = VK_NULL_HANDLE;
local VkRenderPass renderPass     = VK_NULL_HANDLE;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_vars_frames
local VkSemaphore   imageAcquiredSemaphore  = VK_NULL_HANDLE;
local VkSemaphore   renderCompleteSemaphore = VK_NULL_HANDLE;
local VkCommandPool commandPool             = VK_NULL_HANDLE;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_vars_buffers
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
local u32      externalBufferCount;
local Arena*   externalVertexBuffers{}; //an arena of BufferVk serving external buffers
local Arena*   externalIndexBuffers{}; //an arena of BufferVk serving external buffers
#define MAX_EXTERNAL_BUFFERS 6


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_vars_pipelines
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

local VkPipelineCache pipelineCache  = VK_NULL_HANDLE;
local VkPipelineInputAssemblyStateCreateInfo input_assembly_state{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
local VkPipelineRasterizationStateCreateInfo rasterization_state{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
local VkPipelineColorBlendAttachmentState    color_blend_attachment_state{};
local VkPipelineColorBlendStateCreateInfo    color_blend_state{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
local VkPipelineDepthStencilStateCreateInfo  depth_stencil_state{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
local VkPipelineViewportStateCreateInfo      viewport_state{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
local VkPipelineMultisampleStateCreateInfo   multisample_state{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
local VkPipelineVertexInputStateCreateInfo   vertex_input_state{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
local VkPipelineVertexInputStateCreateInfo   twod_vertex_input_state{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
local VkPipelineDynamicStateCreateInfo       dynamic_state{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
local VkGraphicsPipelineCreateInfo           pipeline_create_info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
local arrayT<VkDynamicState>                    dynamic_states(deshi_allocator);
local arrayT<VkVertexInputBindingDescription>   vertex_input_bindings(deshi_allocator);
local arrayT<VkVertexInputAttributeDescription> vertex_input_attributes(deshi_allocator);
local arrayT<VkVertexInputBindingDescription>   twod_vertex_input_bindings(deshi_allocator);
local arrayT<VkVertexInputAttributeDescription> twod_vertex_input_attributes(deshi_allocator);

local VkVertexInputBindingDescription*  vertex_input_bindings_x;
local VkVertexInputAttributeDescription* vertex_input_attributes_x;
local VkVertexInputBindingDescription*   twod_vertex_input_bindings_x;
local VkVertexInputAttributeDescription* twod_vertex_input_attributes_x;

local struct{ //pipelines
	union{
		VkPipeline array[16];
		struct{
			//game shaders
			VkPipeline null;
			VkPipeline flat;
			VkPipeline phong;
			VkPipeline pbr;
			VkPipeline twod;
			VkPipeline ui;
			
			//development shaders
			VkPipeline base;
			VkPipeline wireframe;
			VkPipeline wireframe_depth;
			VkPipeline selected;
			VkPipeline collider;
			VkPipeline offscreen;
			
			//debug shaders
			VkPipeline normals_debug;
			VkPipeline shadowmap_debug;
		};
	};
} pipelines{};


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_vars_shaders
local VkPipelineShaderStageCreateInfo shader_stages[6];
local shaderc_compiler_t shader_compiler;
local shaderc_compile_options_t shader_compiler_options;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_vars_other
local struct{ //TODO(delle,Vu) distribute these variables around
	s32                   width, height;
	VkImage               depthImage;
	VkDeviceMemory        depthImageMemory;
	VkImageView           depthImageView;
	VkSampler             depthSampler;
	VkDescriptorImageInfo depthDescriptor;
	VkRenderPass          renderpass;
	VkFramebuffer         framebuffer;
} offscreen{};

local struct {
	s32                   width, height;
	VkImage               image;
	VkDeviceMemory        imageMemory;
	VkImageView           imageView;
	VkSampler             sampler;
	VkDescriptorImageInfo descriptor;
	VkRenderPass          renderpass;
	VkFramebuffer         framebuffer;
} topmost{};


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_utils
#define AssertVk(result, ...) Assert((result) == VK_SUCCESS)
#define AssertRS(stages, ...) Assert((renderStage & (stages)) == (stages))
#define PrintVk(level, ...) if(renderSettings.loggingLevel >= level){ logger_push_indent(level); Log("vulkan", __VA_ARGS__); logger_pop_indent(level); }(void)0
#define LogEVk(msg) LogE("render-vulkan", __func__, "(): " msg)
#define LogWVk(msg) LogW("render-vulkan", __func__, "(): " msg)

PFN_vkCmdBeginDebugUtilsLabelEXT func_vkCmdBeginDebugUtilsLabelEXT;
local inline void
DebugBeginLabelVk(VkCommandBuffer command_buffer, const char* label_name, vec4 color){DPZoneScoped;
#ifdef BUILD_INTERNAL
	VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
	label.pLabelName = label_name;
	label.color[0]   = color.r;
	label.color[1]   = color.g;
	label.color[2]   = color.b;
	label.color[3]   = color.a;
	func_vkCmdBeginDebugUtilsLabelEXT(command_buffer, &label);
#endif //BUILD_INTERNAL
}

PFN_vkCmdEndDebugUtilsLabelEXT func_vkCmdEndDebugUtilsLabelEXT;
local inline void
DebugEndLabelVk(VkCommandBuffer command_buffer){DPZoneScoped;
#ifdef BUILD_INTERNAL
	func_vkCmdEndDebugUtilsLabelEXT(command_buffer);
#endif //BUILD_INTERNAL
}

PFN_vkCmdInsertDebugUtilsLabelEXT func_vkCmdInsertDebugUtilsLabelEXT;
local inline void
DebugInsertLabelVk(VkCommandBuffer command_buffer, const char* label_name, vec4 color){DPZoneScoped;
#ifdef BUILD_INTERNAL
	VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
	label.pLabelName = label_name;
	label.color[0]   = color.r;
	label.color[1]   = color.g;
	label.color[2]   = color.b;
	label.color[3]   = color.a;
	func_vkCmdInsertDebugUtilsLabelEXT(command_buffer, &label);
#endif //BUILD_INTERNAL
}

PFN_vkSetDebugUtilsObjectNameEXT func_vkSetDebugUtilsObjectNameEXT;
local inline void
DebugSetObjectNameVk(VkDevice device, VkObjectType object_type, u64 object_handle, const char *object_name){DPZoneScoped;
#ifdef BUILD_INTERNAL
	if(!object_handle) return;
	VkDebugUtilsObjectNameInfoEXT nameInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
	nameInfo.objectType   = object_type;
	nameInfo.objectHandle = object_handle;
	nameInfo.pObjectName  = object_name;
	func_vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
#endif //BUILD_INTERNAL
}

//returns a command buffer that will only execute once
local VkCommandBuffer
BeginSingleTimeCommands(){DPZoneScoped;
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
EndSingleTimeCommands(VkCommandBuffer commandBuffer){DPZoneScoped;
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
			if(renderSettings.crashOnError) Assert(!"crashing because of error in vulkan");
		}break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:{
			LogW("vulkan",pCallbackData->pMessage);
		}break;
		default:{
			PrintVk(6, pCallbackData->pMessage);
		}break;
	}
	return VK_FALSE;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_memory
//finds which memory types the graphics card offers
local u32
FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties){DPZoneScoped;
	PrintVk(6,"Finding memory types");
	AssertRS(RSVK_PHYSICALDEVICE, "FindMemoryType called before PickPhysicalDevice");
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);
	
	for(u32 i = 0; i < memProperties.memoryTypeCount; i++){
		if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties){
			return i;
		}
	}
	
	Assert(!"failed to find suitable memory type");
	return 0;
}

//creates a buffer of defined usage and size on the device
local void
CreateOrResizeBuffer(VkBuffer& buffer, VkDeviceMemory& buffer_memory, VkDeviceSize& buffer_size, size_t new_size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){DPZoneScoped;
	VkBuffer old_buffer = buffer; buffer = VK_NULL_HANDLE;
	VkDeviceMemory old_buffer_memory = buffer_memory; buffer_memory = VK_NULL_HANDLE;
	
	VkDeviceSize aligned_buffer_size = RoundUpTo(new_size, buffer_memory_alignment);
	VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.size        = aligned_buffer_size;
	bufferInfo.usage       = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	resultVk = vkCreateBuffer(device, &bufferInfo, allocator, &buffer); AssertVk(resultVk);
	
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(device, buffer, &req);
	buffer_memory_alignment = (buffer_memory_alignment > req.alignment) ? buffer_memory_alignment : req.alignment;
	VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.allocationSize  = req.size;
	allocInfo.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, properties);
	resultVk = vkAllocateMemory(device, &allocInfo, allocator, &buffer_memory); AssertVk(resultVk);
	resultVk = vkBindBufferMemory(device, buffer, buffer_memory, 0); AssertVk(resultVk);
	
	if(buffer_size){
		void* old_buffer_data; void* new_buffer_data;
		resultVk = vkMapMemory(device, old_buffer_memory, 0, buffer_size,         0, &old_buffer_data); AssertVk(resultVk);
		resultVk = vkMapMemory(device, buffer_memory,     0, aligned_buffer_size, 0, &new_buffer_data); AssertVk(resultVk);
		
		memcpy(new_buffer_data, old_buffer_data, buffer_size);
		
		VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
		range.memory = buffer_memory;
		range.offset = 0;
		range.size   = VK_WHOLE_SIZE;
		resultVk = vkFlushMappedMemoryRanges(device, 1, &range); AssertVk(resultVk);
		
		vkUnmapMemory(device, old_buffer_memory);
		vkUnmapMemory(device, buffer_memory);
	}
	
	//delete old buffer
	if(old_buffer        != VK_NULL_HANDLE) vkDestroyBuffer(device, old_buffer, allocator);
	if(old_buffer_memory != VK_NULL_HANDLE) vkFreeMemory(device, old_buffer_memory, allocator);
	
	buffer_size = aligned_buffer_size;
}

local void
CreateOrResizeBuffer(BufferVk* buffer, size_t new_size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){DPZoneScoped;
	CreateOrResizeBuffer(buffer->buffer, buffer->memory, buffer->size, new_size, usage, properties);
}

//creates a buffer and maps provided data to it
local void
CreateAndMapBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize& bufferSize, size_t newSize, void* data, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){DPZoneScoped;
	//delete old buffer
	if(buffer != VK_NULL_HANDLE) vkDestroyBuffer(device, buffer, allocator);
	if(bufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, bufferMemory, allocator);
	
	//create buffer
	VkDeviceSize alignedBufferSize = ((newSize-1) / buffer_memory_alignment + 1) * buffer_memory_alignment;
	VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.size        = alignedBufferSize;
	bufferInfo.usage       = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	resultVk = vkCreateBuffer(device, &bufferInfo, allocator, &buffer); AssertVk(resultVk, "failed to create buffer");
	
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(device, buffer, &req);
	buffer_memory_alignment = (buffer_memory_alignment > req.alignment) ? buffer_memory_alignment : req.alignment;
	
	//allocate buffer
	VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.allocationSize  = req.size;
	allocInfo.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, properties);
	
	resultVk = vkAllocateMemory(device, &allocInfo, allocator, &bufferMemory); AssertVk(resultVk, "failed to allocate buffer memory");
	
	//if data pointer, map buffer and copy data
	if(data != 0){
		void* mapped = 0;
		resultVk = vkMapMemory(device, bufferMemory, 0, newSize, 0, &mapped); AssertVk(resultVk, "failed to map memory");
		{
			memcpy(mapped, data, newSize);
			// If host coherency hasn't been requested, do a manual flush to make writes visible
			if((properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0){
				VkMappedMemoryRange mappedRange{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
				mappedRange.memory = bufferMemory;
				mappedRange.offset = 0;
				mappedRange.size   = newSize;
				vkFlushMappedMemoryRanges(device, 1, &mappedRange);
			}
		}
		vkUnmapMemory(device, bufferMemory);
	}
	
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
	bufferSize = newSize;
}

//copies a buffer, we use this to copy from a host-visible staging buffer to device-only buffer
local void
CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size){DPZoneScoped;
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();{
		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	}EndSingleTimeCommands(commandBuffer);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_images
//creates an image view specifying how to use an image
local VkImageView
CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 mipLevels){DPZoneScoped;
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
	resultVk = vkCreateImageView(device, &viewInfo, allocator, &imageView); AssertVk(resultVk, "failed to create texture image view");
	return imageView;
}

//creates and binds a vulkan image to the GPU
local void
CreateImage(u32 width, u32 height, u32 mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory){DPZoneScoped;
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
	resultVk = vkCreateImage(device, &imageInfo, allocator, &image); AssertVk(resultVk, "failed to create image");
	
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);
	
	VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
	resultVk = vkAllocateMemory(device, &allocInfo, allocator, &imageMemory); AssertVk(resultVk, "failed to allocate image memory");
	
	vkBindImageMemory(device, image, imageMemory, 0);
}

//converts a VkImage from one layout to another using an image memory barrier
local void
TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels){DPZoneScoped;
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
	
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, 0, 0, 0, 1, &barrier);
	EndSingleTimeCommands(commandBuffer);
}

//scans an image for max possible mipmaps and generates them
local void
GenerateMipmaps(VkImage image, VkFormat imageFormat, s32 texWidth, s32 texHeight, u32 mipLevels){DPZoneScoped;
	PrintVk(4,"Creating image mipmaps");
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physical_device, imageFormat, &formatProperties);
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
		
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, &barrier);
		
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
		
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 0, 0, 1, &barrier);
		
		if(mipWidth  > 1) mipWidth  /= 2;
		if(mipHeight > 1) mipHeight /= 2;
	}
	
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 0, 0, 1, &barrier);
	
	EndSingleTimeCommands(commandBuffer);
}

//uses commands to copy a buffer to an image
local void
CopyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height){DPZoneScoped;
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


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_init
local void
SetupAllocator(){DPZoneScoped;
	//!ref: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkAllocationCallbacks.html
	PrintVk(2,"Setting up vulkan allocator");
	Assert(renderStage == RENDERERSTAGE_NONE, "renderer stage was not NONE at SetupAllocator");
	
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
CreateInstance(){DPZoneScoped;
	PrintVk(2,"Creating vulkan instance");
	Assert(renderStage == RENDERERSTAGE_NONE, "renderer stage was not NONE at CreateInstance");
	renderStage |= RSVK_INSTANCE;
	
	//check for validation layer support
	if(renderSettings.debugging){
		PrintVk(3,"Checking validation layer support");
		bool has_support = true;
		
		u32 layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, 0);
		arrayT<VkLayerProperties> availableLayers(layerCount, deshi_temp_allocator);
		availableLayers.count = layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data);
		
		forI(ArrayCount(validation_layers)){
			bool layerFound = false;
			forE(availableLayers){
				if(strcmp(validation_layers[i], it->layerName) == 0){
					layerFound = true;
					break;
				}
			}
			if(!layerFound) Assert(!"validation layer requested, but not available");
		}
	}
	
	//set instance's application info
	VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	appInfo.pApplicationName   = (const char*)DeshWindow->title.str;
	appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
	appInfo.pEngineName        = "deshi";
	appInfo.engineVersion      = VK_MAKE_VERSION(1,0,0);
	appInfo.apiVersion         = VK_API_VERSION_1_0;
	
	VkValidationFeaturesEXT validationFeatures{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
	validationFeatures.disabledValidationFeatureCount = 0;
	validationFeatures.pDisabledValidationFeatures    = 0;
	validationFeatures.enabledValidationFeatureCount  = validation_features_enabled.count;
	validationFeatures.pEnabledValidationFeatures     = validation_features_enabled.data;
	
	//get required extensions
	PrintVk(3, "Getting required extensions");
#if DESHI_WINDOWS
	const char* extensions[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#  if BUILD_INTERNAL
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#  endif //#if BUILD_INTERNAL
	};
#elif DESHI_LINUX //#if DESHI_WINDOWS
	const char* extensions[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#  if BUILD_INTERNAL
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#  endif //#if BUILD_INTERNAL
	};
#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
#  error "unhandled platform/vulkan interaction"
#endif //#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
	
	//setup instance debug messenger
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	debugCreateInfo.messageSeverity = callback_severities;
	debugCreateInfo.messageType     = callback_types;
	debugCreateInfo.pfnUserCallback = DebugCallback;
	
	//create the instance
	VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	createInfo.pApplicationInfo        = &appInfo;
	createInfo.enabledExtensionCount   = (u32)ArrayCount(extensions);
	createInfo.ppEnabledExtensionNames = extensions;
	if(renderSettings.debugging){
		createInfo.enabledLayerCount   = (u32)ArrayCount(validation_layers);
		createInfo.ppEnabledLayerNames = validation_layers;
		debugCreateInfo.pNext          = &validationFeatures;
		createInfo.pNext               = &debugCreateInfo;
	} else {
		createInfo.enabledLayerCount   = 0;
		createInfo.pNext               = 0;
	}
	resultVk = vkCreateInstance(&createInfo, allocator, &instance); AssertVk(resultVk, "failed to create instance");
}

local void
SetupDebugMessenger(){DPZoneScoped;
	PrintVk(2, "Setting up debug messenger");
	AssertRS(RSVK_INSTANCE, "SetupDebugMessenger was called before CreateInstance");
	
	if(!renderSettings.debugging) return;
	
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	debugCreateInfo.messageSeverity = callback_severities;
	debugCreateInfo.messageType     = callback_types;
	debugCreateInfo.pfnUserCallback = DebugCallback;
	
	VkResult err = VK_ERROR_EXTENSION_NOT_PRESENT;
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if(func != 0){
		err = func(instance, &debugCreateInfo, allocator, &debugMessenger);
	}
	AssertVk(err, "failed to setup debug messenger");
}

local void
CreateSurface(Window* win = DeshWindow, u32 surface_idx = 0){DPZoneScoped;
	AssertRS(RSVK_INSTANCE, "CreateSurface called before CreateInstance");
	Assert(surface_idx < MAX_SURFACES);
	renderStage |= RSVK_SURFACE;
	
	
#if DESHI_WINDOWS
	PrintVk(2, "Creating Win32-Vulkan surface");
	VkWin32SurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
	info.hwnd = (HWND)win->handle;
	info.hinstance = (HINSTANCE)win32_console_instance;
	resultVk = vkCreateWin32SurfaceKHR(instance, &info, 0, &surfaces[surface_idx]); AssertVk(resultVk, "failed to create win32 surface");
#elif DESHI_LINUX
	PrintVk(2, "Creating X11-Vulkan surface");
	VkXlibSurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
	info.window = (X11Window)win->handle;
	info.dpy = linux.x11.display;
	resultVk = vkCreateXlibSurfaceKHR(instance, &info, 0, &surfaces[surface_idx]); AssertVk(resultVk, "failed to create X11 surface");
#elif DESHI_MAC
	PrintVk(2, "Creating glfw-Vulkan surface");
	resultVk = glfwCreateWindowSurface(instance, DeshWindow->window, allocator, &surfaces[renderActiveSurface]); AssertVk(resultVk, "failed to create glfw surface");
#endif
}

local void
PickPhysicalDevice(u32 surface_index = 0){DPZoneScoped;
	PrintVk(2, "Picking physical device");
	AssertRS(RSVK_SURFACE, "PickPhysicalDevice called before CreateSurface");
	renderStage |= RSVK_PHYSICALDEVICE;
	
	u32 deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, 0);
	arrayT<VkPhysicalDevice> devices(deviceCount, deshi_temp_allocator);
	devices.count = deviceCount;
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data);
	
	//test all available GPUs
	for(VkPhysicalDevice device : devices){
		{//find device's queue families
			physicalQueueFamilies.graphicsFamily.reset();
			physicalQueueFamilies.presentFamily.reset();
			
			u32 queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, 0);
			arrayT<VkQueueFamilyProperties> queueFamilies(queueFamilyCount, deshi_temp_allocator);
			queueFamilies.count = queueFamilyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data);
			
			for(u32 family_idx = 0; family_idx < queueFamilyCount; ++family_idx){
				if(queueFamilies[family_idx].queueFlags & VK_QUEUE_GRAPHICS_BIT) physicalQueueFamilies.graphicsFamily = family_idx;
				
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, family_idx, surfaces[surface_index], &presentSupport);
				if(presentSupport) physicalQueueFamilies.presentFamily = family_idx;
				
				if(physicalQueueFamilies.isComplete()) break;
			}
			
			if(!physicalQueueFamilies.isComplete()) continue;
		}
		
		{//check if device supports enabled/required extensions
			u32 extensionCount;
			vkEnumerateDeviceExtensionProperties(device, 0, &extensionCount, 0);
			arrayT<VkExtensionProperties> availableExtensions(extensionCount, deshi_temp_allocator);
			availableExtensions.count = extensionCount;
			vkEnumerateDeviceExtensionProperties(device, 0, &extensionCount, availableExtensions.data);
			
			u32 count = 0;
			for(VkExtensionProperties extension : availableExtensions){
				forI(ArrayCount(deviceExtensions)){
					if(extension.extensionName == deviceExtensions[i]){
						count++;
						break;
					}
				}
				if(count == ArrayCount(deviceExtensions)) break;
			}
			if(count == ArrayCount(deviceExtensions)) continue;
		}
		
		{//check if the device's swapchain is valid
			u32 formatCount;
			u32 presentModeCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surfaces[surface_index], &formatCount, 0);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surfaces[surface_index], &presentModeCount, 0);
			if(formatCount == 0 || presentModeCount == 0) continue;
		}
		
		physical_device = device;
		break;
	}
	Assert(physical_device != VK_NULL_HANDLE, "failed to find a suitable GPU that supports Vulkan");
	
	//get device's max msaa samples
	vkGetPhysicalDeviceProperties(physical_device, &physicalDeviceProperties);
	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if     (counts & VK_SAMPLE_COUNT_64_BIT){ maxMsaaSamples = VK_SAMPLE_COUNT_64_BIT; }
	else if(counts & VK_SAMPLE_COUNT_32_BIT){ maxMsaaSamples = VK_SAMPLE_COUNT_32_BIT; }
	else if(counts & VK_SAMPLE_COUNT_16_BIT){ maxMsaaSamples = VK_SAMPLE_COUNT_16_BIT; }
	else if(counts & VK_SAMPLE_COUNT_8_BIT)  { maxMsaaSamples = VK_SAMPLE_COUNT_8_BIT;  }
	else if(counts & VK_SAMPLE_COUNT_4_BIT)  { maxMsaaSamples = VK_SAMPLE_COUNT_4_BIT;  }
	else if(counts & VK_SAMPLE_COUNT_2_BIT)  { maxMsaaSamples = VK_SAMPLE_COUNT_2_BIT;  }
	else                                     { maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;  }
	
	//get physical device capabilities
	vkGetPhysicalDeviceFeatures(physical_device, &deviceFeatures);
}

local void
CreateLogicalDevice(){DPZoneScoped;
	PrintVk(2, "Creating logical device");
	AssertRS(RSVK_PHYSICALDEVICE, "CreateLogicalDevice called before PickPhysicalDevice");
	renderStage |= RSVK_LOGICALDEVICE;
	
	//setup device queue create infos
	f32 queuePriority = 1.0f;
	arrayT<VkDeviceQueueCreateInfo> queueCreateInfos(deshi_temp_allocator);
	VkDeviceQueueCreateInfo queueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queueCreateInfo.queueFamilyIndex = physicalQueueFamilies.graphicsFamily.value;
	queueCreateInfo.queueCount       = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;
	queueCreateInfos.add(queueCreateInfo);
	if(physicalQueueFamilies.presentFamily.value != physicalQueueFamilies.graphicsFamily.value){
		queueCreateInfo.queueFamilyIndex = physicalQueueFamilies.presentFamily.value;
		queueCreateInfos.add(queueCreateInfo);
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
	if(renderSettings.debugging){
		if(deviceFeatures.geometryShader){
			enabledFeatures.geometryShader = VK_TRUE;
		}
	}
	
	VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	createInfo.pQueueCreateInfos       = queueCreateInfos.data;
	createInfo.queueCreateInfoCount    = (u32)queueCreateInfos.count;
	createInfo.pEnabledFeatures        = &enabledFeatures;
	createInfo.enabledExtensionCount   = (u32)ArrayCount(deviceExtensions);
	createInfo.ppEnabledExtensionNames = deviceExtensions;
	if(renderSettings.debugging){
		createInfo.enabledLayerCount     = (u32)ArrayCount(validation_layers);
		createInfo.ppEnabledLayerNames   = validation_layers;
	}else{
		createInfo.enabledLayerCount     = 0;
	}
	
	resultVk = vkCreateDevice(physical_device, &createInfo, allocator, &device); AssertVk(resultVk, "failed to create logical device");
	
	vkGetDeviceQueue(device, physicalQueueFamilies.graphicsFamily.value, 0, &graphicsQueue);
	vkGetDeviceQueue(device, physicalQueueFamilies.presentFamily.value,  0, &presentQueue);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_swapchain
//destroy old swap chain and in-flight frames, create a new swap chain with new dimensions
local void
CreateSwapchain(Window* win = DeshWindow, u32 swapchain_idx = 0){DPZoneScoped;
	PrintVk(2, "Creating swapchain");
	AssertRS(RSVK_LOGICALDEVICE, "CreateSwapchain called before CreateLogicalDevice");
	Assert(swapchain_idx < MAX_SURFACES);
	renderStage |= RSVK_SWAPCHAIN;
	
	renderActiveSurface = swapchain_idx;
	VkSwapchainKHR oldSwapChain = activeSwapchainKHR;
	activeSwapchainKHR = VK_NULL_HANDLE;
	vkDeviceWaitIdle(device);
	
	//update width and height
	activeSwapchain.width  = win->width;
	activeSwapchain.height = win->height;
	
	{//check GPU's features/capabilities for the new swapchain
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surfaces[renderActiveSurface], &activeSwapchain.supportDetails.capabilities);
		
		u32 formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surfaces[renderActiveSurface], &formatCount, 0);
		if(formatCount != 0){
			activeSwapchain.supportDetails.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surfaces[renderActiveSurface], &formatCount, activeSwapchain.supportDetails.formats.data);
		}
		
		u32 presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surfaces[renderActiveSurface], &presentModeCount, 0);
		if(presentModeCount != 0){
			activeSwapchain.supportDetails.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surfaces[renderActiveSurface], &presentModeCount, activeSwapchain.supportDetails.presentModes.data);
		}
	}
	
	{//choose swapchain's surface format
		activeSwapchain.surfaceFormat = activeSwapchain.supportDetails.formats[0];
		for(VkSurfaceFormatKHR availableFormat : activeSwapchain.supportDetails.formats){
			if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT){
				activeSwapchain.surfaceFormat = availableFormat;
				break;
			}
		}
	}
	
	{//choose the swapchain's present mode
		//TODO(delle,ReVu) add render settings here (vsync)
		bool immediate    = false;
		bool fifo_relaxed = false;
		bool mailbox      = false;
		
		for(VkPresentModeKHR availablePresentMode : activeSwapchain.supportDetails.presentModes){
			if(availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)    immediate    = true;
			if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)      mailbox      = true;
			if(availablePresentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR) fifo_relaxed = true;
		}
		
		//NOTE immediate is forced false b/c ImGui requires minImageCount to be at least 2
		if      (immediate && false){
			renderSettings.vsync = VSyncType_Immediate;
			activeSwapchain.presentMode    = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}else if(mailbox){
			renderSettings.vsync = VSyncType_Mailbox;
			activeSwapchain.presentMode    = VK_PRESENT_MODE_MAILBOX_KHR;
		}else if(fifo_relaxed){
			renderSettings.vsync = VSyncType_FifoRelaxed;
			activeSwapchain.presentMode    = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
		}else{
			renderSettings.vsync = VSyncType_Fifo;
			activeSwapchain.presentMode    = VK_PRESENT_MODE_FIFO_KHR;
		}
	}
	
	//find the actual extent of the swapchain
	if(activeSwapchain.supportDetails.capabilities.currentExtent.width != UINT32_MAX){
		activeSwapchain.extent = activeSwapchain.supportDetails.capabilities.currentExtent;
	}else{
		activeSwapchain.extent = { (u32)activeSwapchain.width, (u32)activeSwapchain.height };
		activeSwapchain.extent.width  = Max(activeSwapchain.supportDetails.capabilities.minImageExtent.width,
											Min(activeSwapchain.supportDetails.capabilities.maxImageExtent.width,  activeSwapchain.extent.width));
		activeSwapchain.extent.height = Max(activeSwapchain.supportDetails.capabilities.minImageExtent.height,
											Min(activeSwapchain.supportDetails.capabilities.maxImageExtent.height, activeSwapchain.extent.height));
	}
	
	//get min image count if not specified
	if(activeSwapchain.minImageCount == 0){ //TODO(delle,ReVu) add render settings here (extra buffering)
		switch(activeSwapchain.presentMode){
			case VK_PRESENT_MODE_MAILBOX_KHR:     { activeSwapchain.minImageCount =  2; }break;
			case VK_PRESENT_MODE_FIFO_KHR:        { activeSwapchain.minImageCount =  2; }break;
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR:{ activeSwapchain.minImageCount =  2; }break;
			case VK_PRESENT_MODE_IMMEDIATE_KHR:   { activeSwapchain.minImageCount =  1; }break;
			default:                              { activeSwapchain.minImageCount = -1; }break;
		}
	}
	
	u32 queueFamilyIndices[2] = {
		physicalQueueFamilies.graphicsFamily.value, physicalQueueFamilies.presentFamily.value
	};
	
	//create swapchain and swap chain images, set width and height
	VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	info.surface                   = surfaces[renderActiveSurface];
	info.imageFormat               = activeSwapchain.surfaceFormat.format;
	info.imageColorSpace           = activeSwapchain.surfaceFormat.colorSpace;
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
	info.preTransform              = activeSwapchain.supportDetails.capabilities.currentTransform;
	info.compositeAlpha            = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode               = activeSwapchain.presentMode;
	info.clipped                   = VK_TRUE;
	info.oldSwapchain              = oldSwapChain;
	info.minImageCount             = activeSwapchain.minImageCount;
	if(activeSwapchain.supportDetails.capabilities.maxImageCount != 0 && info.minImageCount > activeSwapchain.supportDetails.capabilities.maxImageCount){
		info.minImageCount         = activeSwapchain.supportDetails.capabilities.maxImageCount;
	}
	if(activeSwapchain.extent.width == 0xffffffff){
		info.imageExtent.width  = activeSwapchain.width;
		info.imageExtent.height = activeSwapchain.height;
	} else {
		info.imageExtent.width  = activeSwapchain.width  = activeSwapchain.extent.width;
		info.imageExtent.height = activeSwapchain.height = activeSwapchain.extent.height;
	}
	resultVk = vkCreateSwapchainKHR(device, &info, allocator, &activeSwapchainKHR); AssertVk(resultVk, "failed to create swap chain");
	
	activeSwapchain.window = win;
	
	//delete old swap chain
	if(oldSwapChain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, oldSwapChain, allocator);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_renderpass
local VkFormat
findSupportedFormat(VkFormat* formats, u64 format_count, VkImageTiling tiling, VkFormatFeatureFlags features){DPZoneScoped;
	PrintVk(4, "Finding supported image formats");
	forI(format_count){
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physical_device, formats[i], &props);
		if      (tiling == VK_IMAGE_TILING_LINEAR  && (props.linearTilingFeatures  & features) == features){
			return formats[i];
		}else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features){
			return formats[i];
		}
	}
	
	Assert(!"failed to find supported format");
	return VK_FORMAT_UNDEFINED;
}

local VkFormat
findDepthFormat(){DPZoneScoped;
	VkFormat depth_formats[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, };
	return findSupportedFormat(depth_formats, ArrayCount(depth_formats),
							   VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

local void
CreateRenderpasses(){DPZoneScoped;
	PrintVk(2, "Creating render pass");
	AssertRS(RSVK_LOGICALDEVICE, "CreateRenderPasses called before CreateLogicalDevice");
	renderStage |= RSVK_RENDERPASS;
	
	if(baseRenderPass) vkDestroyRenderPass(device, baseRenderPass, allocator);
	if(msaaRenderPass) vkDestroyRenderPass(device, msaaRenderPass, allocator);
	
	VkAttachmentDescription attachments[3]{};
	//attachment 0: color
	attachments[0].format         = activeSwapchain.surfaceFormat.format;
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
	attachments[2].format         = activeSwapchain.surfaceFormat.format;
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
		
		resultVk = vkCreateRenderPass(device, &renderPassInfo, allocator, &msaaRenderPass); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_RENDER_PASS, (u64)msaaRenderPass, "MSAA render pass");
		
		renderPass = msaaRenderPass;
	}else{
		resultVk = vkCreateRenderPass(device, &renderPassInfo, allocator, &baseRenderPass); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_RENDER_PASS, (u64)baseRenderPass, "Base render pass");
		
		renderPass = baseRenderPass;
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_frames
local void
CreateCommandPool(){DPZoneScoped;
	PrintVk(2, "Creating command pool");
	AssertRS(RSVK_LOGICALDEVICE, "CreateCommandPool called before CreateLogicalDevice");
	renderStage |= RSVK_COMMANDPOOL;
	
	VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = physicalQueueFamilies.graphicsFamily.value;
	
	resultVk = vkCreateCommandPool(device, &poolInfo, allocator, &commandPool); AssertVk(resultVk, "failed to create command pool");
}

//creates image views, color/depth resources, framebuffers, commandbuffers
local void
CreateFrames(){DPZoneScoped;
	PrintVk(2, "Creating frames");
	AssertRS(RSVK_COMMANDPOOL, "CreateFrames called before CreateCommandPool");
	renderStage |= RSVK_FRAMES;
	
	//get swap chain images
	vkGetSwapchainImagesKHR(device, activeSwapchainKHR, &activeSwapchain.imageCount, 0); //gets the image count
	Assert(activeSwapchain.imageCount >= activeSwapchain.minImageCount, "the window should always have at least the min image count");
	Assert(activeSwapchain.imageCount < 16, "the window should have less than 16 images, around 2-3 is ideal");
	VkImage images[16] = {};
	vkGetSwapchainImagesKHR(device, activeSwapchainKHR, &activeSwapchain.imageCount, images); //assigns to images
	
	{//color framebuffer attachment
		if(activeSwapchain.attachments.color_image){
			vkDestroyImageView(device, activeSwapchain.attachments.color_image_view, allocator);
			vkDestroyImage(device, activeSwapchain.attachments.color_image, allocator);
			vkFreeMemory(device, activeSwapchain.attachments.color_image_memory, allocator);
		}
		VkFormat colorFormat = activeSwapchain.surfaceFormat.format;
		CreateImage(activeSwapchain.width, activeSwapchain.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, activeSwapchain.attachments.color_image, activeSwapchain.attachments.color_image_memory);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE, (u64)activeSwapchain.attachments.color_image, "Framebuffer color image");
		activeSwapchain.attachments.color_image_view = CreateImageView(activeSwapchain.attachments.color_image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)activeSwapchain.attachments.color_image_view, "Framebuffer color imageview");
	}
	
	{//depth framebuffer attachment
		if(activeSwapchain.attachments.depth_image){
			vkDestroyImageView(device, activeSwapchain.attachments.depth_image_view, allocator);
			vkDestroyImage(device, activeSwapchain.attachments.depth_image, allocator);
			vkFreeMemory(device, activeSwapchain.attachments.depth_image_memory, allocator);
		}
		VkFormat depthFormat = findDepthFormat();
		CreateImage(activeSwapchain.width, activeSwapchain.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, activeSwapchain.attachments.depth_image, activeSwapchain.attachments.depth_image_memory);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE, (u64)activeSwapchain.attachments.depth_image, "Framebuffer depth image");
		activeSwapchain.attachments.depth_image_view = CreateImageView(activeSwapchain.attachments.depth_image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)activeSwapchain.attachments.depth_image_view, "Framebuffer depth imageview");
	}
	
	activeSwapchain.frames.resize(activeSwapchain.imageCount);
	for(u32 i = 0; i < activeSwapchain.imageCount; ++i){
		//set the frame images to the swap chain images
		//NOTE the previous image and its memory gets freed when the swapchain gets destroyed
		activeSwapchain.frames[i].image = images[i];
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE, (u64)activeSwapchain.frames[i].image, (const char*)to_dstr8v(deshi_temp_allocator, "Frame image ", i).str);
		
		//create the image views
		if(activeSwapchain.frames[i].imageView) vkDestroyImageView(device, activeSwapchain.frames[i].imageView, allocator);
		activeSwapchain.frames[i].imageView = CreateImageView(activeSwapchain.frames[i].image, activeSwapchain.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)activeSwapchain.frames[i].imageView, (const char*)to_dstr8v(deshi_temp_allocator, "Frame imageview ", i).str);
		
		//create the framebuffers
		if(activeSwapchain.frames[i].framebuffer) vkDestroyFramebuffer(device, activeSwapchain.frames[i].framebuffer, allocator);
		
		arrayT<VkImageView> frameBufferAttachments(deshi_temp_allocator); //TODO(delle) fix scuffed msaa hack
		if(msaaSamples != VK_SAMPLE_COUNT_1_BIT){
			frameBufferAttachments = { activeSwapchain.attachments.color_image_view, activeSwapchain.attachments.depth_image_view, activeSwapchain.frames[i].imageView };
		}else{
			frameBufferAttachments = { activeSwapchain.frames[i].imageView, activeSwapchain.attachments.depth_image_view, };
		}
		
		VkFramebufferCreateInfo info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		info.renderPass      = renderPass;
		info.attachmentCount = frameBufferAttachments.count;
		info.pAttachments    = frameBufferAttachments.data;
		info.width           = activeSwapchain.width;
		info.height          = activeSwapchain.height;
		info.layers          = 1;
		resultVk = vkCreateFramebuffer(device, &info, allocator, &activeSwapchain.frames[i].framebuffer); AssertVk(resultVk, "failed to create framebuffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_FRAMEBUFFER, (u64)activeSwapchain.frames[i].framebuffer, (const char*)to_dstr8v(deshi_temp_allocator, "Frame framebuffer ", i).str);
		
		//allocate command buffers
		if(activeSwapchain.frames[i].commandBuffer) vkFreeCommandBuffers(device, commandPool, 1, &activeSwapchain.frames[i].commandBuffer);
		VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		resultVk = vkAllocateCommandBuffers(device, &allocInfo, &activeSwapchain.frames[i].commandBuffer); AssertVk(resultVk, "failed to allocate command buffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (u64)activeSwapchain.frames[i].commandBuffer, (const char*)to_dstr8v(deshi_temp_allocator, "Frame command buffer ", i).str);
	}
}

//creates semaphores indicating: image acquired, rendering complete
//semaphores (GPU-GPU) coordinate operations across command buffers so that they execute in a specified order
//fences (CPU-GPU) are similar but are waited for in the code itself rather than threads
local void
CreateSyncObjects(){DPZoneScoped;
	PrintVk(2, "Creating sync objects");
	AssertRS(RSVK_FRAMES, "CreateSyncObjects called before CreateFrames");
	renderStage |= RSVK_SYNCOBJECTS;
	
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


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_buffers
//TODO(delle,ReOpVu) maybe only do one mapping at buffer creation, see: gltfscenerendering.cpp, line:600
local void
UpdateUniformBuffers(){DPZoneScoped;
	AssertRS(RSVK_UNIFORMBUFFER, "UpdateUniformBuffer called before CreateUniformBuffer");
	//PrintVk(2, "  Updating Uniform Buffer");
	
	{//update offscreen vertex shader ubo
		//calculate light ViewProjection for shadow map based on first light
		uboVSoffscreen.values.lightVP =
			Math::LookAtMatrix(vkLights[0].toVec3(), vec3::ZERO).Inverse() *
			Math::PerspectiveProjectionMatrix((f32)renderSettings.shadowResolution, (f32)renderSettings.shadowResolution, 90.0f, renderSettings.shadowNearZ, renderSettings.shadowFarZ);
		
		void* data;
		vkMapMemory(device, uboVSoffscreen.bufferMemory, 0, sizeof(uboVSoffscreen.values), 0, &data);{
			memcpy(data, &uboVSoffscreen.values, sizeof(uboVSoffscreen.values));
		}vkUnmapMemory(device, uboVSoffscreen.bufferMemory);
	}
	
	{//update scene vertex shader ubo
		uboVS.values.time = DeshTime->totalTime;
		CopyMemory(uboVS.values.lights, vkLights, 10*sizeof(vec4));
		uboVS.values.screen = Vec2((f32)activeSwapchain.extent.width, (f32)activeSwapchain.extent.height);
		uboVS.values.mousepos = input_mouse_position();
		if(initialized) uboVS.values.mouseWorld = Math::ScreenToWorld(input_mouse_position(), uboVS.values.proj, uboVS.values.view, Vec2(DeshWindow->width,DeshWindow->height));
		uboVS.values.enablePCF = renderSettings.shadowPCF;
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
CreateUniformBuffers(){DPZoneScoped;
	PrintVk(2, "Creating uniform buffers");
	AssertRS(RSVK_LOGICALDEVICE, "CreateUniformBuffer called before CreateLogicalDevice");
	renderStage |= RSVK_UNIFORMBUFFER;
	
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


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_other
local void
SetupOffscreenRendering(){DPZoneScoped;
	PrintVk(2, "Creating offscreen rendering stuffs");
	AssertRS(RSVK_LOGICALDEVICE, "SetupOffscreenRendering called before CreateLogicalDevice");
	renderStage |= RSVK_RENDERPASS;
	
	//cleanup previous offscreen stuff
	if(offscreen.framebuffer){
		vkDestroyImageView(  device, offscreen.depthImageView,   allocator);
		vkDestroyImage(      device, offscreen.depthImage,       allocator);
		vkFreeMemory(        device, offscreen.depthImageMemory, allocator);
		vkDestroySampler(    device, offscreen.depthSampler,     allocator);
		vkDestroyRenderPass( device, offscreen.renderpass,       allocator);
		vkDestroyFramebuffer(device, offscreen.framebuffer,      allocator);
	}
	
	offscreen.width  = renderSettings.shadowResolution;
	offscreen.height = renderSettings.shadowResolution;
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
		sampler.magFilter     = (renderSettings.textureFiltering) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		sampler.minFilter     = (renderSettings.textureFiltering) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		sampler.mipmapMode    = (renderSettings.textureFiltering) ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
		sampler.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.mipLodBias    = 0.0f;
		sampler.maxAnisotropy = 1.0f;
		sampler.minLod        = 0.0f;
		sampler.maxLod        = 1.0f;
		sampler.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		resultVk = vkCreateSampler(device, &sampler, 0, &offscreen.depthSampler); AssertVk(resultVk, "failed to create offscreen depth attachment sampler");
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
		resultVk = vkCreateRenderPass(device, &createInfo, allocator, &offscreen.renderpass); AssertVk(resultVk, "failed to create offscreen render pass");
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
		resultVk = vkCreateFramebuffer(device, &createInfo, allocator, &offscreen.framebuffer); AssertVk(resultVk, "failed to create offscreen framebuffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_FRAMEBUFFER, (u64)offscreen.framebuffer, "Offscreen framebuffer");
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_shaders
local void
SetupShaderCompiler(){
	shader_compiler         = shaderc_compiler_initialize();
	shader_compiler_options = shaderc_compile_options_initialize();
	if(renderSettings.optimizeShaders){
		shaderc_compile_options_set_optimization_level(shader_compiler_options, shaderc_optimization_level_performance);
	}
}

//TODO(delle) maybe just save the shader module to disk and load that instead of .spv (since openGL can't load vulkan format .spv)?
local VkPipelineShaderStageCreateInfo
load_shader(str8 name, str8 source, VkShaderStageFlagBits stage){DPZoneScoped;
	if(!source) return VkPipelineShaderStageCreateInfo{};
	
	Stopwatch t_s = start_stopwatch();
	PrintVk(4, "Compiling shader: ",name);
	
	//try to compile from GLSL to SPIR-V binary
	shaderc_compilation_result_t compiled;
	if      (stage == VK_SHADER_STAGE_VERTEX_BIT){
		compiled = shaderc_compile_into_spv(shader_compiler, (const char*)source.str, source.count, shaderc_glsl_vertex_shader,
											(const char*)name.str, "main", shader_compiler_options);
	}else if(stage == VK_SHADER_STAGE_FRAGMENT_BIT){
		compiled = shaderc_compile_into_spv(shader_compiler, (const char*)source.str, source.count, shaderc_glsl_fragment_shader,
											(const char*)name.str, "main", shader_compiler_options);
	}else if(stage == VK_SHADER_STAGE_GEOMETRY_BIT){
		compiled = shaderc_compile_into_spv(shader_compiler, (const char*)source.str, source.count, shaderc_glsl_geometry_shader,
											(const char*)name.str, "main", shader_compiler_options);
	}else{
		Assert(!"unhandled shader stage");
		return VkPipelineShaderStageCreateInfo{};
	}
	
	//check for compile errors
	if(!compiled){
		LogE("vulkan",name,": Shader compiler returned a null result");
		return VkPipelineShaderStageCreateInfo{};
	}
	if(shaderc_result_get_compilation_status(compiled) != shaderc_compilation_status_success){
		LogE("vulkan",shaderc_result_get_error_message(compiled));
		return VkPipelineShaderStageCreateInfo{};
	}
	defer{ shaderc_result_release(compiled); };
	
	//create or overwrite .spv files
	file_write_simple(str8_concat3(STR8("data/shaders/"),name,str8_lit(".spv"), deshi_temp_allocator),
					  (void*)shaderc_result_get_bytes(compiled), shaderc_result_get_length(compiled));
	PrintVk(5, "Finished compiling shader '",name,"' in ",peek_stopwatch(t_s),"ms");
	
	//create shader module
	VkShaderModule shaderModule{};
	VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	moduleInfo.codeSize = shaderc_result_get_length(compiled);
	moduleInfo.pCode    = (u32*)shaderc_result_get_bytes(compiled);
	resultVk = vkCreateShaderModule(device, &moduleInfo, allocator, &shaderModule); AssertVk(resultVk, "failed to create shader module");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SHADER_MODULE, (u64)shaderModule, (const char*)to_dstr8v(deshi_temp_allocator, "Shader Module ",name).str);
	
	//setup shader stage create info
	VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	shaderStage.stage  = stage;
	shaderStage.pName  = "main";
	shaderStage.module = shaderModule;
	
	PrintVk(5, "Finished loading shader '",name,"' in ",peek_stopwatch(t_s),"ms");
	return shaderStage;
}

local VkPipelineShaderStageCreateInfo
load_shader_file(str8 name, VkShaderStageFlagBits stage){DPZoneScoped;
	PrintVk(3, "Loading shader: ",name);
	Stopwatch t_s = start_stopwatch();
	str8 dir = STR8("data/shaders/");
	str8 path = str8_concat(dir, name, deshi_temp_allocator);
	
	//load from .spv if previously compiled and create shader module
	if(!renderSettings.recompileAllShaders){
		str8 spv  = str8_concat(path, str8_lit(".spv"), deshi_temp_allocator);
		File* spv_file = file_init_if_exists(spv, FileAccess_Read);
		if(spv_file){
			defer{ file_deinit(spv_file); };
			str8 spv_raw = file_read(spv_file, memory_talloc(spv_file->bytes), spv_file->bytes);
			if(spv_raw){
				VkShaderModule shaderModule;
				VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
				moduleInfo.codeSize = spv_raw.count;
				moduleInfo.pCode    = (u32*)spv_raw.str;
				resultVk = vkCreateShaderModule(device, &moduleInfo, allocator, &shaderModule); AssertVk(resultVk, "failed to create shader module");
				DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SHADER_MODULE, (u64)shaderModule, (const char*)to_dstr8v(deshi_temp_allocator, "Shader Module ",name).str);
				
				VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
				shaderStage.stage  = stage;
				shaderStage.pName  = "main";
				shaderStage.module = shaderModule;
				return shaderStage;
			}
		}
	}
	
	//if not already compiled, compile then load then create shader module
	File shader_file = file_info(path);
	if(!shader_file.creation_time) return VkPipelineShaderStageCreateInfo{};
	
	str8 shader_source = file_read_simple(path, deshi_temp_allocator);
	return load_shader(name, shader_source, stage);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_pipelines_setup (descriptor pool, layouts, pipeline cache, pipeline create info structs (rasterizer, depth test, etc))
//creates descriptor set layouts, push constants for shaders, and the pipeline layout
local void
CreateLayouts(){DPZoneScoped;
	PrintVk(2, "Creating layouts");
	AssertRS(RSVK_LOGICALDEVICE, "CreateLayouts called before CreateLogicalDevice");
	renderStage |= RSVK_LAYOUTS;
	
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
		resultVk = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, allocator, &descriptorSetLayouts.base); AssertVk(resultVk);
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
		resultVk = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, allocator, &descriptorSetLayouts.textures); AssertVk(resultVk);
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
		resultVk = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, 0, &descriptorSetLayouts.twod); AssertVk(resultVk);
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
		resultVk = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, allocator, &descriptorSetLayouts.geometry); AssertVk(resultVk);
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
		resultVk = vkCreatePipelineLayout(device, &pipelineLayoutInfo, allocator, &pipelineLayouts.base); AssertVk(resultVk);
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
		resultVk = vkCreatePipelineLayout(device, &createInfo, allocator, &pipelineLayouts.twod); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)pipelineLayouts.twod,
							 "2D pipeline layout");
	}
	
	if(enabledFeatures.geometryShader){//create geometry shader pipeline layout
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
		pushConstantRange.offset     = 0;
		pushConstantRange.size       = sizeof(mat4);
		
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		pipelineLayoutInfo.setLayoutCount         = 1;
		pipelineLayoutInfo.pSetLayouts            = &descriptorSetLayouts.geometry;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;
		resultVk = vkCreatePipelineLayout(device, &pipelineLayoutInfo, allocator, &pipelineLayouts.geometry); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)pipelineLayouts.geometry,
							 "Geometry pipeline layout");
	}
}

//creates a pool of descriptors of different types to be sent to shaders
//TODO(delle,ReVu) find a better/more accurate way to do this, see gltfloading.cpp, line:592
local void
CreateDescriptorPool(){DPZoneScoped;
	PrintVk(2, "Creating descriptor pool");
	AssertRS(RSVK_LOGICALDEVICE, "CreateDescriptorPool called before CreateLogicalDevice");
	renderStage |= RSVK_DESCRIPTORPOOL;
	
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
	resultVk = vkCreateDescriptorPool(device, &poolInfo, allocator, &descriptorPool); AssertVk(resultVk, "failed to create descriptor pool");
}

//allocates in the descriptor pool and creates the descriptor sets
local void
CreateDescriptorSets(){DPZoneScoped;
	AssertRS(RSVK_DESCRIPTORPOOL | RSVK_UNIFORMBUFFER, "CreateLayouts called before CreateDescriptorPool or CreateUniformBuffer");
	renderStage |= RSVK_DESCRIPTORSETS;
	
	VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	allocInfo.descriptorPool     = descriptorPool;
	allocInfo.pSetLayouts        = &descriptorSetLayouts.base;
	allocInfo.descriptorSetCount = 1;
	
	VkWriteDescriptorSet writeDescriptorSets[2]{};
	
	{//base descriptor sets
		resultVk = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.base); AssertVk(resultVk);
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
		
		vkUpdateDescriptorSets(device, 2, writeDescriptorSets, 0, 0);
	}
	
	{//offscreen shadow map generation descriptor set
		resultVk = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.offscreen); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)descriptorSets.offscreen, "Offscreen descriptor set");
		
		//binding 0: vertex shader ubo
		writeDescriptorSets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].dstSet          = descriptorSets.offscreen;
		writeDescriptorSets[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSets[0].dstBinding      = 0;
		writeDescriptorSets[0].pBufferInfo     = &uboVSoffscreen.bufferDescriptor;
		writeDescriptorSets[0].descriptorCount = 1;
		
		vkUpdateDescriptorSets(device, 1, writeDescriptorSets, 0, 0);
	}
	
	//geometry descriptor sets
	if(enabledFeatures.geometryShader){
		allocInfo.pSetLayouts = &descriptorSetLayouts.geometry;
		resultVk = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.geometry); AssertVk(resultVk);
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
		
		vkUpdateDescriptorSets(device, 2, writeDescriptorSets, 0, 0);
		allocInfo.pSetLayouts = &descriptorSetLayouts.base;
	}
	
	{//DEBUG show shadow map descriptor set
		resultVk = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.shadowMap_debug); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)descriptorSets.shadowMap_debug, "DEBUG Shadowmap descriptor set");
		
		//binding 1: fragment shader shadow sampler
		writeDescriptorSets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].dstSet          = descriptorSets.shadowMap_debug;
		writeDescriptorSets[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSets[0].dstBinding      = 1;
		writeDescriptorSets[0].pImageInfo      = &offscreen.depthDescriptor;
		writeDescriptorSets[0].descriptorCount = 1;
		
		vkUpdateDescriptorSets(device, 1, writeDescriptorSets, 0, 0);
	}
}

local void
CreatePipelineCache(){DPZoneScoped;
	PrintVk(2, "Creating pipeline cache");
	AssertRS(RSVK_LOGICALDEVICE, "CreatePipelineCache called before CreateLogicalDevice");
	Stopwatch watch = start_stopwatch();
	
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
	
	//try to read pipeline cache file if exists
	if(file_exists(str8_lit("data/pipelines.cache"))){
		str8 data = file_read_simple(str8_lit("data/pipelines.cache"), deshi_temp_allocator);
		pipelineCacheCreateInfo.initialDataSize = data.count;
		pipelineCacheCreateInfo.pInitialData    = data.str;
	}
	resultVk = vkCreatePipelineCache(device, &pipelineCacheCreateInfo, 0, &pipelineCache); AssertVk(resultVk, "failed to create pipeline cache");
	PrintVk(5, "Finished creating pipeline cache in ",peek_stopwatch(watch),"ms");
}

local void
SetupPipelineCreation(){DPZoneScoped;
	PrintVk(2, "Setting up pipeline creation");
	AssertRS(RSVK_LAYOUTS | RSVK_RENDERPASS, "SetupPipelineCreation called before CreateLayouts or CreateRenderPasses");
	renderStage |= RSVK_PIPELINESETUP;
	
	//vertex input flow control
	//https://renderdoc.org/vkspec_chunked/chap23.html#VkPipelineVertexInputStateCreateInfo
	vertex_input_bindings = { //binding:u32, stride:u32, inputRate:VkVertexInputRate
		{0, sizeof(MeshVertex), VK_VERTEX_INPUT_RATE_VERTEX},
	};
	vertex_input_attributes = { //location:u32, binding:u32, format:VkFormat, offset:u32
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MeshVertex, pos)},
		{1, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(MeshVertex, uv)},
		{2, 0, VK_FORMAT_R8G8B8A8_UNORM,   offsetof(MeshVertex, color)},
		{3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MeshVertex, normal)},
	};
	vertex_input_state.vertexBindingDescriptionCount   = (u32)vertex_input_bindings.count;
	vertex_input_state.pVertexBindingDescriptions      = vertex_input_bindings.data;
	vertex_input_state.vertexAttributeDescriptionCount = (u32)vertex_input_attributes.count;
	vertex_input_state.pVertexAttributeDescriptions    = vertex_input_attributes.data;
	
	twod_vertex_input_bindings = { //binding:u32, stride:u32, inputRate:VkVertexInputRate
		{0, sizeof(Vertex2), VK_VERTEX_INPUT_RATE_VERTEX},
	};
	twod_vertex_input_attributes = { //location:u32, binding:u32, format:VkFormat, offset:u32
		{0, 0, VK_FORMAT_R32G32_SFLOAT,  offsetof(Vertex2, pos)},
		{1, 0, VK_FORMAT_R32G32_SFLOAT,  offsetof(Vertex2, uv)},
		{2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(Vertex2, color)},
	};
	twod_vertex_input_state.vertexBindingDescriptionCount   = (u32)twod_vertex_input_bindings.count;
	twod_vertex_input_state.pVertexBindingDescriptions      = twod_vertex_input_bindings.data;
	twod_vertex_input_state.vertexAttributeDescriptionCount = (u32)twod_vertex_input_attributes.count;
	twod_vertex_input_state.pVertexAttributeDescriptions    = twod_vertex_input_attributes.data;
	
	//determines how to group vertices together
	//https://renderdoc.org/vkspec_chunked/chap22.html#VkPipelineInputAssemblyStateCreateInfo
	input_assembly_state.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state.primitiveRestartEnable = VK_FALSE;
	
	//container for viewports and scissors
	//https://renderdoc.org/vkspec_chunked/chap27.html#VkPipelineViewportStateCreateInfo
	viewport_state.viewportCount = 1;
	viewport_state.pViewports    = 0;
	viewport_state.scissorCount  = 1;
	viewport_state.pScissors     = 0;
	
	//how to draw/cull/depth things
	//https://renderdoc.org/vkspec_chunked/chap28.html#VkPipelineRasterizationStateCreateInfo
	rasterization_state.depthClampEnable        = VK_FALSE; //look into for shadowmapping
	rasterization_state.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state.polygonMode             = VK_POLYGON_MODE_FILL; //draw mode: fill, wireframe, vertices
	rasterization_state.cullMode                = VK_CULL_MODE_BACK_BIT;
	rasterization_state.frontFace               = VK_FRONT_FACE_CLOCKWISE; //VK_FRONT_FACE_COUNTER_CLOCKWISE
	rasterization_state.depthBiasEnable         = VK_FALSE;
	rasterization_state.depthBiasConstantFactor = 0.0f;
	rasterization_state.depthBiasClamp          = 0.0f;
	rasterization_state.depthBiasSlopeFactor    = 0.0f;
	rasterization_state.lineWidth               = 1.0f;
	
	//useful for multisample anti-aliasing (MSAA)
	//https://renderdoc.org/vkspec_chunked/chap28.html#VkPipelineMultisampleStateCreateInfo
	multisample_state.rasterizationSamples  = msaaSamples;
	multisample_state.sampleShadingEnable   = (msaaSamples != VK_SAMPLE_COUNT_1_BIT) ? VK_TRUE : VK_FALSE;
	multisample_state.minSampleShading      = .2f; //min fraction for sample shading; closer to one is smoother
	multisample_state.pSampleMask           = 0;
	multisample_state.alphaToCoverageEnable = VK_FALSE;
	multisample_state.alphaToOneEnable      = VK_FALSE;
	
	//depth testing and discarding
	//https://renderdoc.org/vkspec_chunked/chap29.html#VkPipelineDepthStencilStateCreateInfo
	depth_stencil_state.depthTestEnable       = VK_TRUE;
	depth_stencil_state.depthWriteEnable      = VK_TRUE;
	depth_stencil_state.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_state.stencilTestEnable     = VK_FALSE;
	depth_stencil_state.minDepthBounds        = 0.0f;
	depth_stencil_state.maxDepthBounds        = 1.0f;
	depth_stencil_state.stencilTestEnable     = VK_FALSE;
	depth_stencil_state.front                 = {};
	depth_stencil_state.back.compareOp        = VK_COMPARE_OP_ALWAYS;
	
	//how to combine colors; alpha: options to allow alpha blending
	//https://renderdoc.org/vkspec_chunked/chap30.html#VkPipelineColorBlendAttachmentState
	color_blend_attachment_state.blendEnable         = VK_TRUE;
	color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
	color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;
	color_blend_attachment_state.colorWriteMask      = 0xF; //RGBA
	
	//container struct for color blend attachments with overall blending constants
	//https://renderdoc.org/vkspec_chunked/chap30.html#VkPipelineColorBlendStateCreateInfo
	color_blend_state.logicOpEnable     = VK_FALSE;
	color_blend_state.logicOp           = VK_LOGIC_OP_COPY; //TODO(delle) maybe VK_LOGIC_OP_CLEAR?
	color_blend_state.attachmentCount   = 1;
	color_blend_state.pAttachments      = &color_blend_attachment_state;
	color_blend_state.blendConstants[0] = 0.0f;
	color_blend_state.blendConstants[1] = 0.0f;
	color_blend_state.blendConstants[2] = 0.0f;
	color_blend_state.blendConstants[3] = 0.0f;
	
	//dynamic states that can vary in the command buffer
	//https://renderdoc.org/vkspec_chunked/chap11.html#VkPipelineDynamicStateCreateInfo
	dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	dynamic_state.dynamicStateCount = (u32)dynamic_states.count;
	dynamic_state.pDynamicStates    = dynamic_states.data;
	
	//base pipeline info and options
	pipeline_create_info.stageCount          = 0;
	pipeline_create_info.pStages             = shader_stages;
	pipeline_create_info.pVertexInputState   = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pTessellationState  = 0;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.layout              = pipelineLayouts.base;
	pipeline_create_info.renderPass          = renderPass;
	pipeline_create_info.subpass             = 0;
	pipeline_create_info.basePipelineHandle  = VK_NULL_HANDLE;
	pipeline_create_info.basePipelineIndex   = -1;
} //SetupPipelineCreation


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_pipelines_creation
local void
CreatePipelines(){DPZoneScoped;
	PrintVk(2, "Creating pipelines");
	AssertRS(RSVK_PIPELINESETUP, "CreatePipelines called before SetupPipelineCreation");
	renderStage |= RSVK_PIPELINECREATE;
	
	//destroy previous pipelines
	forI(ArrayCount(pipelines.array)){
		if(pipelines.array[i]) vkDestroyPipeline(device, pipelines.array[i], 0);
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
	specializationInfo.pData = &renderSettings.shadowPCF;
*/
	
	{//base pipeline
		//flag that this pipeline will be used as a base
		pipeline_create_info.flags              = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
		pipeline_create_info.basePipelineIndex  = -1;
		
		shader_stages[0] = load_shader(STR8("base.vert"), baked_shader_base_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("base.frag"), baked_shader_base_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.base); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.base, "Base pipeline");
		
		//flag that all other pipelines are derivatives
		pipeline_create_info.flags              = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
		pipeline_create_info.basePipelineHandle = pipelines.base;
		pipeline_create_info.basePipelineIndex  = -1; //can either use handle or index, not both (section 9.5 of vulkan spec)
	}
	
	{//selected (base with no cull or depth test)
		rasterization_state.cullMode = VK_CULL_MODE_NONE;
		depth_stencil_state.depthTestEnable = VK_FALSE;
		
		shader_stages[1].pSpecializationInfo = &specializationInfo;
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.selected); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.selected, "Selected pipeline");
		
		rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
		depth_stencil_state.depthTestEnable = VK_TRUE;
	}
	
	{//null pipeline
		shader_stages[0] = load_shader(STR8("null.vert"), baked_shader_null_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("null.frag"), baked_shader_null_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		shader_stages[1].pSpecializationInfo = &specializationInfo;
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.null); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.null, "Null pipeline");
	}
	
	{//flat pipeline
		shader_stages[0] = load_shader(STR8("flat.vert"), baked_shader_flat_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("flat.frag"), baked_shader_flat_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		shader_stages[1].pSpecializationInfo = &specializationInfo;
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.flat); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.flat, "Flat pipeline");
	}
	
	{//phong
		shader_stages[0] = load_shader(STR8("phong.vert"), baked_shader_phong_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("phong.frag"), baked_shader_phong_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		shader_stages[1].pSpecializationInfo = &specializationInfo;
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.phong); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.phong, "Phong pipeline");
	}
	
	{//2d
		pipeline_create_info.pVertexInputState = &twod_vertex_input_state;
		pipeline_create_info.layout            = pipelineLayouts.twod;
		rasterization_state.cullMode  = VK_CULL_MODE_NONE;
		rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		depth_stencil_state.depthTestEnable = VK_FALSE;
		
		shader_stages[0] = load_shader(STR8("twod.vert"), baked_shader_twod_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("twod.frag"), baked_shader_twod_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		shader_stages[1].pSpecializationInfo = &specializationInfo;
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.twod); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.twod, "2D pipeline");
		
		{//ui
			shader_stages[0] = load_shader(STR8("ui.vert"), baked_shader_ui_vert, VK_SHADER_STAGE_VERTEX_BIT);
			shader_stages[1] = load_shader(STR8("ui.frag"), baked_shader_ui_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
			shader_stages[1].pSpecializationInfo = &specializationInfo;
			pipeline_create_info.stageCount = 2;
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.ui);
			DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.ui, "UI pipeline");
		}
		
		pipeline_create_info.pVertexInputState = &vertex_input_state;
		pipeline_create_info.layout            = pipelineLayouts.base;
		rasterization_state.cullMode  = VK_CULL_MODE_BACK_BIT;
		rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
		depth_stencil_state.depthTestEnable = VK_TRUE;
	}
	
	{//pbr
		shader_stages[0] = load_shader(STR8("pbr.vert"), baked_shader_pbr_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("pbr.frag"), baked_shader_pbr_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		shader_stages[1].pSpecializationInfo = &specializationInfo;
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.pbr); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.pbr, "PBR pipeline");
	}
	
	//wireframe
	if(deviceFeatures.fillModeNonSolid){
		color_blend_attachment_state.blendEnable = VK_FALSE;
		rasterization_state.polygonMode = VK_POLYGON_MODE_LINE;
		rasterization_state.cullMode    = VK_CULL_MODE_NONE;
		depth_stencil_state.depthTestEnable = VK_FALSE;
		
		shader_stages[0] = load_shader(STR8("wireframe.vert"), baked_shader_wireframe_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("wireframe.frag"), baked_shader_wireframe_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.wireframe); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.wireframe, "Wireframe pipeline");
		
		{//wireframe with depth test
			depth_stencil_state.depthTestEnable = VK_TRUE;
			
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.wireframe_depth); AssertVk(resultVk);
			DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.wireframe_depth, "Wireframe Depth pipeline");
			
			depth_stencil_state.depthTestEnable = VK_FALSE;
		}
		
		{ //collider gets a specific colored wireframe
			color_blend_attachment_state.blendEnable         = VK_TRUE;
			color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
			color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
			
			color_blend_state.blendConstants[0] = (f32)renderSettings.colliderColor.r;
			color_blend_state.blendConstants[1] = (f32)renderSettings.colliderColor.g;
			color_blend_state.blendConstants[2] = (f32)renderSettings.colliderColor.b;
			color_blend_state.blendConstants[3] = (f32)renderSettings.colliderColor.a;
			
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.collider); AssertVk(resultVk);
			DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.collider, "Collider pipeline");
			
			color_blend_attachment_state.blendEnable         = VK_FALSE;
			color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			color_blend_state.blendConstants[0] = 0.f;
			color_blend_state.blendConstants[1] = 0.f;
			color_blend_state.blendConstants[2] = 0.f;
			color_blend_state.blendConstants[3] = 1.0f;
		}
		
		color_blend_attachment_state.blendEnable = VK_TRUE;
		rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_state.cullMode    = VK_CULL_MODE_BACK_BIT;
		depth_stencil_state.depthTestEnable = VK_TRUE;
	}
	
	{//offscreen
		color_blend_state.attachmentCount = 0; //no color attachments used
		depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; //cull front faces
		rasterization_state.depthBiasEnable = VK_TRUE; //enable depth bias
		multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_state.sampleShadingEnable  = VK_FALSE;
		dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS, };
		dynamic_state.dynamicStateCount = (u32)dynamic_states.count; //add depth bias to dynamic state so
		dynamic_state.pDynamicStates    = dynamic_states.data;       //it can be changed at runtime
		pipeline_create_info.renderPass = offscreen.renderpass;
		
		shader_stages[0] = load_shader(STR8("offscreen.vert"), baked_shader_offscreen_vert, VK_SHADER_STAGE_VERTEX_BIT);
		pipeline_create_info.stageCount = 1;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.offscreen); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.offscreen, "Offscreen pipeline");
		
		color_blend_state.attachmentCount = 1;
		depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		rasterization_state.depthBiasEnable = VK_FALSE;
		multisample_state.rasterizationSamples = msaaSamples;
		multisample_state.sampleShadingEnable  = (msaaSamples != VK_SAMPLE_COUNT_1_BIT) ? VK_TRUE : VK_FALSE;
		dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, };
		dynamic_state.dynamicStateCount = (u32)dynamic_states.count;
		dynamic_state.pDynamicStates    = dynamic_states.data;
		pipeline_create_info.renderPass = renderPass;
	}
} //CreatePipelines

local VkPipeline
GetPipelineFromShader(u32 shader){DPZoneScoped;
	switch(shader){
		case(Shader_NULL):default:{ return pipelines.null;      }
		case(Shader_Flat):        { return pipelines.flat;      }
		case(Shader_Phong):       { return pipelines.phong;     }
		case(Shader_PBR):         { return pipelines.pbr;       }
		case(Shader_Wireframe):   { return pipelines.wireframe; }
	}
}

local void
UpdateMaterialPipelines(){DPZoneScoped;
	PrintVk(5, "Updating material pipelines");
	for(auto& mat : vkMaterials){
		mat.pipeline = GetPipelineFromShader(mat.base->shader);
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_commands_setup
local void
SetupCommands(){DPZoneScoped;
	//create 2D vertex and index buffers
	if(renderTwodIndexCount){
		size_t ui_vb_size = RoundUpTo(Max(1000*sizeof(Vertex2),         renderTwodVertexCount * sizeof(Vertex2)), physicalDeviceProperties.limits.nonCoherentAtomSize);
		size_t ui_ib_size = RoundUpTo(Max(3000*sizeof(RenderTwodIndex), renderTwodIndexCount  * sizeof(RenderTwodIndex)), physicalDeviceProperties.limits.nonCoherentAtomSize);
		
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
		resultVk = vkMapMemory(device, uiVertexBuffer.memory, 0, ui_vb_size, 0, &vb_data); AssertVk(resultVk);
		resultVk = vkMapMemory(device, uiIndexBuffer.memory,  0, ui_ib_size, 0, &ib_data); AssertVk(resultVk);
		{
			CopyMemory(vb_data, renderTwodVertexArray, ui_vb_size);
			CopyMemory(ib_data, renderTwodIndexArray,  ui_ib_size);
			
			VkMappedMemoryRange range[2] = {};
			range[0].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[0].memory = uiVertexBuffer.memory;
			range[0].size   = VK_WHOLE_SIZE;
			range[1].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[1].memory = uiIndexBuffer.memory;
			range[1].size   = VK_WHOLE_SIZE;
			resultVk = vkFlushMappedMemoryRanges(device, 2, range); AssertVk(resultVk);
		}
		vkUnmapMemory(device, uiVertexBuffer.memory);
		vkUnmapMemory(device, uiIndexBuffer.memory);
		
		//name buffers for debugging
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)uiVertexBuffer.buffer, "2D vertex buffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)uiIndexBuffer.buffer, "2D index buffer");
	}
	
	//create temp vertex and index buffers
	if(renderTempWireframeIndexCount || renderTempFilledIndexCount){
		size_t temp_wire_vb_size = renderTempWireframeVertexCount*sizeof(MeshVertex);
		size_t temp_fill_vb_size = renderTempFilledVertexCount*sizeof(MeshVertex);
		size_t temp_wire_ib_size = renderTempWireframeIndexCount*sizeof(RenderTempIndex);
		size_t temp_fill_ib_size = renderTempFilledIndexCount*sizeof(RenderTempIndex);
		size_t temp_vb_size = RoundUpTo(Max(1000*sizeof(MeshVertex), temp_wire_vb_size+temp_fill_vb_size), physicalDeviceProperties.limits.nonCoherentAtomSize);
		size_t temp_ib_size = RoundUpTo(Max(3000*sizeof(RenderTempIndex), temp_wire_ib_size+temp_fill_ib_size), physicalDeviceProperties.limits.nonCoherentAtomSize);
		
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
		resultVk = vkMapMemory(device, tempVertexBuffer.memory, 0, temp_vb_size, 0, &vb_data); AssertVk(resultVk);
		resultVk = vkMapMemory(device, tempIndexBuffer.memory,  0, temp_ib_size, 0, &ib_data); AssertVk(resultVk);
		{
			CopyMemory(vb_data, renderTempWireframeVertexArray, temp_wire_vb_size);
			CopyMemory(ib_data, renderTempWireframeIndexArray,  temp_wire_ib_size);
			CopyMemory((u8*)vb_data+temp_wire_vb_size, renderTempFilledVertexArray, temp_fill_vb_size);
			CopyMemory((u8*)ib_data+temp_wire_ib_size, renderTempFilledIndexArray,  temp_fill_ib_size);
			
			VkMappedMemoryRange range[2] = {};
			range[0].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[0].memory = tempVertexBuffer.memory;
			range[0].size   = VK_WHOLE_SIZE;
			range[1].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[1].memory = tempIndexBuffer.memory;
			range[1].size   = VK_WHOLE_SIZE;
			resultVk = vkFlushMappedMemoryRanges(device, 2, range); AssertVk(resultVk);
		}
		vkUnmapMemory(device, tempVertexBuffer.memory);
		vkUnmapMemory(device, tempIndexBuffer.memory);
		
		//name buffers for debugging
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)tempVertexBuffer.buffer, "Temp vertex buffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)tempIndexBuffer.buffer, "Temp index buffer");
	}
	
	//create debug vertex and index buffers
	if(renderDebugWireframeIndexCount || renderDebugFilledIndexCount){
		size_t debug_wire_vb_size = renderDebugWireframeVertexCount*sizeof(MeshVertex);
		size_t debug_fill_vb_size = renderDebugFilledVertexCount*sizeof(MeshVertex);
		size_t debug_wire_ib_size = renderDebugWireframeIndexCount*sizeof(RenderTempIndex);
		size_t debug_fill_ib_size = renderDebugFilledIndexCount*sizeof(RenderTempIndex);
		size_t debug_vb_size = RoundUpTo(Max(1000*sizeof(MeshVertex), debug_wire_vb_size+debug_fill_vb_size), physicalDeviceProperties.limits.nonCoherentAtomSize);
		size_t debug_ib_size = RoundUpTo(Max(3000*sizeof(RenderTempIndex), debug_wire_ib_size+debug_fill_ib_size), physicalDeviceProperties.limits.nonCoherentAtomSize);
		
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
		resultVk = vkMapMemory(device, debugVertexBuffer.memory, 0, debug_vb_size, 0, &vb_data); AssertVk(resultVk);
		resultVk = vkMapMemory(device, debugIndexBuffer.memory,  0, debug_ib_size, 0, &ib_data); AssertVk(resultVk);
		{
			CopyMemory(vb_data, renderDebugWireframeVertexArray, debug_wire_vb_size);
			CopyMemory(ib_data, renderDebugWireframeIndexArray,  debug_wire_ib_size);
			CopyMemory((u8*)vb_data+debug_wire_vb_size, renderDebugFilledVertexArray, debug_fill_vb_size);
			CopyMemory((u8*)ib_data+debug_wire_ib_size, renderDebugFilledIndexArray,  debug_fill_ib_size);
			
			VkMappedMemoryRange range[2] = {};
			range[0].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[0].memory = debugVertexBuffer.memory;
			range[0].size   = VK_WHOLE_SIZE;
			range[1].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[1].memory = debugIndexBuffer.memory;
			range[1].size   = VK_WHOLE_SIZE;
			resultVk = vkFlushMappedMemoryRanges(device, 2, range); AssertVk(resultVk);
		}
		vkUnmapMemory(device, debugVertexBuffer.memory);
		vkUnmapMemory(device, debugIndexBuffer.memory);
		
		//name buffers for debugging
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)debugVertexBuffer.buffer, "Debug vertex buffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)debugIndexBuffer.buffer, "Debug index buffer");
	}
}

local void
ResetCommands(){DPZoneScoped;
	{//2D commands
		forI(TWOD_LAYERS+1){
			ZeroMemory(renderTwodCmdArrays[renderActiveSurface][i], renderTwodCmdCounts[renderActiveSurface][i]*sizeof(RenderTwodCmd));
			renderTwodCmdCounts[renderActiveSurface][i] = 0;
		}
		renderTwodVertexCount = 0;
		renderTwodIndexCount  = 0;
	}
	
	{//temp commands
		renderTempWireframeVertexCount = 0;
		renderTempWireframeIndexCount  = 0;
		renderTempFilledVertexCount = 0;
		renderTempFilledIndexCount  = 0;
	}
	
	{//model commands
		renderModelCmdCount = 0;
	}
	
#ifdef BUILD_INTERNAL
	renderBookKeeperCount = 0;
#endif
	
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_commands_build
local vec4 render_pass_color = Vec4(0.78f, 0.54f, 0.12f, 1.0f);
local vec4 draw_group_color  = Vec4(0.50f, 0.76f, 0.34f, 1.0f);
local vec4 draw_cmd_color    = Vec4(0.40f, 0.61f, 0.27f, 1.0f);

//we define a call order to command buffers so they can be executed by vkSubmitQueue()
local void
BuildCommands(){DPZoneScoped;
	//PrintVk(2, "Building Command Buffers");
	AssertRS(RSVK_DESCRIPTORSETS | RSVK_PIPELINECREATE, "BuildCommandBuffers called before CreateDescriptorSets or CreatePipelines");
	
	VkClearValue clearValues[2]{};
	VkCommandBufferBeginInfo cmdBufferInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	VkViewport viewport{}; //scales the image
	VkRect2D   scissor{};  //cuts the scaled image
	
	forI(activeSwapchain.imageCount){
		VkCommandBuffer cmdBuffer = activeSwapchain.frames[i].commandBuffer;
		resultVk = vkBeginCommandBuffer(cmdBuffer, &cmdBufferInfo); AssertVk(resultVk, "failed to begin recording command buffer");
		
		////////////////////////////
		//// @first render pass ////
		////////////////////////////
		{//generate shadow map by rendering the scene offscreen
			clearValues[0].depthStencil             = {1.0f, 0};
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
			vkCmdSetDepthBias(cmdBuffer, renderSettings.depthBiasConstant, 0.0f, renderSettings.depthBiasSlope);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.base, 0, 1, &descriptorSets.offscreen, 0, 0);
			
			if(renderModelCmdCount){
				DebugBeginLabelVk(cmdBuffer, "Meshes", draw_group_color);
				VkDeviceSize offsets[1] = {0}; //reset vertex buffer offsets
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &meshVertexBuffer.buffer, offsets);
				vkCmdBindIndexBuffer(cmdBuffer, meshIndexBuffer.buffer, 0, INDEX_TYPE_VK_MESH);
				forI(renderModelCmdCount){
					RenderModelCmd& cmd = renderModelCmdArray[i];
					MaterialVk& mat = vkMaterials[cmd.material];
					DebugInsertLabelVk(cmdBuffer, cmd.name, draw_cmd_color);
					vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &cmd.matrix);
					vkCmdDrawIndexed(cmdBuffer, cmd.index_count, 1, cmd.index_offset, cmd.vertex_offset, 0);
					renderStats.drawnIndices += cmd.index_count;
				}
				DebugEndLabelVk(cmdBuffer);
			}
			
			//draw voxels
			if(render_voxel_chunk_pool && memory_pool_count(render_voxel_chunk_pool)){
				DebugBeginLabelVk(cmdBuffer, "Voxels", draw_group_color);
				VkDeviceSize offsets[1]; //reset vertex buffer offsets
				for_pool(render_voxel_chunk_pool){
					if(!it->hidden){
						mat4 matrix = mat4::TransformationMatrix(it->position, it->rotation, vec3_ONE());
						offsets[0] = 0;
						vkCmdBindVertexBuffers(cmdBuffer, 0, 1, (VkBuffer*)&it->vertex_buffer->buffer_handle, offsets);
						vkCmdBindIndexBuffer(cmdBuffer, (VkBuffer)it->index_buffer->buffer_handle, 0, INDEX_TYPE_VK_MESH);
						vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &matrix);
						vkCmdDrawIndexed(cmdBuffer, it->index_count, 1, 0, 0, 0);
						renderStats.drawnIndices += it->index_count;
					}
				}
				DebugEndLabelVk(cmdBuffer);
			}
			
			vkCmdEndRenderPass(cmdBuffer);
			DebugEndLabelVk(cmdBuffer);
		}
		
		//NOTE explicit synchronization is not required because it is done via the subpass dependencies
		
		///////////////////////////// //TODO(delle) separate 2d rendering into it's own renderpass
		//// @second render pass ////
		/////////////////////////////
		{//scene rendering with applied shadow map
			clearValues[0].color        = {renderSettings.clearColor.r, renderSettings.clearColor.g, renderSettings.clearColor.b, renderSettings.clearColor.a};
			clearValues[1].depthStencil = {1.0f, 0};
			renderPassInfo.renderPass        = renderPass;
			renderPassInfo.framebuffer       = activeSwapchain.frames[i].framebuffer;
			renderPassInfo.clearValueCount   = 2;
			renderPassInfo.pClearValues      = clearValues;
			renderPassInfo.renderArea.offset = {0, 0};
			renderPassInfo.renderArea.extent = activeSwapchain.extent;
			viewport.x        = 0;
			viewport.y        = 0;
			viewport.width    = (f32)activeSwapchain.width;
			viewport.height   = (f32)activeSwapchain.height;
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			scissor.offset.x      = 0;
			scissor.offset.y      = 0;
			scissor.extent.width  = activeSwapchain.width;
			scissor.extent.height = activeSwapchain.height;
			
			DebugBeginLabelVk(cmdBuffer, "Scene Render Pass", render_pass_color);
			vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.base, 0, 1, &descriptorSets.base, 0, 0);
			
			//draw meshes
			if(renderModelCmdCount){
				DebugBeginLabelVk(cmdBuffer, "Meshes", draw_group_color);
				VkDeviceSize offsets[1] = {0}; //reset vertex buffer offsets
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &meshVertexBuffer.buffer, offsets);
				vkCmdBindIndexBuffer(cmdBuffer, meshIndexBuffer.buffer, 0, INDEX_TYPE_VK_MESH);
				forI(renderModelCmdCount){
					RenderModelCmd& cmd = renderModelCmdArray[i];
					MaterialVk& mat = vkMaterials[cmd.material];
					DebugInsertLabelVk(cmdBuffer, cmd.name, draw_cmd_color);
					vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &cmd.matrix);
					
					if(renderSettings.wireframeOnly){
						vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe);
					}else{
						vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mat.pipeline);
						vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.base, 1, 1, &mat.descriptorSet, 0, 0);
					}
					vkCmdDrawIndexed(cmdBuffer, cmd.index_count, 1, cmd.index_offset, cmd.vertex_offset, 0);
					renderStats.drawnIndices += cmd.index_count;
					
					//wireframe overlay
					if(renderSettings.meshWireframes){
						vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe);
						vkCmdDrawIndexed(cmdBuffer, cmd.index_count, 1, cmd.index_offset, cmd.vertex_offset, 0);
						renderStats.drawnIndices += cmd.index_count;
					}
				}
				DebugEndLabelVk(cmdBuffer);
			}
			
			//draw voxels
			if(render_voxel_chunk_pool && memory_pool_count(render_voxel_chunk_pool)){
				DebugBeginLabelVk(cmdBuffer, "Voxels", draw_group_color);
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.flat);
				VkDeviceSize offsets[1] = {0}; //reset vertex buffer offsets
				for_pool(render_voxel_chunk_pool){
					if(!it->hidden){
						mat4 matrix = mat4::TransformationMatrix(it->position, it->rotation, vec3_ONE());
						offsets[0] = 0;
						vkCmdBindVertexBuffers(cmdBuffer, 0, 1, (VkBuffer*)&it->vertex_buffer->buffer_handle, offsets);
						vkCmdBindIndexBuffer(cmdBuffer, (VkBuffer)it->index_buffer->buffer_handle, 0, INDEX_TYPE_VK_MESH);
						vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &matrix);
						vkCmdDrawIndexed(cmdBuffer, it->index_count, 1, 0, 0, 0);
						renderStats.drawnIndices += it->index_count;
					}
				}
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe);
				DebugEndLabelVk(cmdBuffer);
			}
			
			//draw temporary stuff
			if(renderTempWireframeVertexCount && renderTempWireframeIndexCount ||
			   renderTempFilledVertexCount    && renderTempFilledIndexCount){
				DebugBeginLabelVk(cmdBuffer, "Temp", draw_group_color);
				VkDeviceSize offsets[1] = {0};
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &tempVertexBuffer.buffer, offsets);
				vkCmdBindIndexBuffer(cmdBuffer, tempIndexBuffer.buffer, 0, INDEX_TYPE_VK_TEMP);
				
				//wireframe
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,(renderSettings.tempMeshOnTop) ? pipelines.wireframe : pipelines.wireframe_depth);
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &mat4::IDENTITY);
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.base, 0, 1, &descriptorSets.base, 0, 0);
				vkCmdDrawIndexed(cmdBuffer, renderTempWireframeIndexCount, 1, 0, 0, 0);
				renderStats.drawnIndices += renderTempWireframeIndexCount;
				
				//filled
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.flat);
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &mat4::IDENTITY);
				vkCmdDrawIndexed(cmdBuffer, renderTempFilledIndexCount, 1, renderTempWireframeIndexCount, renderTempWireframeVertexCount, 0);
				renderStats.drawnIndices += renderTempFilledIndexCount;
				
				DebugEndLabelVk(cmdBuffer);
			}
			
			//draw debug stuff
			if(renderDebugWireframeVertexCount > 0 && renderDebugWireframeIndexCount > 0){
				DebugBeginLabelVk(cmdBuffer, "Debug", draw_group_color);
				VkDeviceSize offsets[1] = {0};
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &debugVertexBuffer.buffer, offsets);
				vkCmdBindIndexBuffer(cmdBuffer, debugIndexBuffer.buffer, 0, INDEX_TYPE_VK_TEMP);
				
				//wireframe
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,(renderSettings.tempMeshOnTop) ? pipelines.wireframe : pipelines.wireframe_depth);
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &mat4::IDENTITY);
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.base, 0, 1, &descriptorSets.base, 0, 0);
				vkCmdDrawIndexed(cmdBuffer, renderDebugWireframeIndexCount, 1, 0, 0, 0);
				renderStats.drawnIndices += renderDebugWireframeIndexCount;
				
				//filled
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.selected);
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &mat4::IDENTITY);
				vkCmdDrawIndexed(cmdBuffer, renderDebugFilledIndexCount, 1, renderDebugWireframeIndexCount, renderDebugWireframeVertexCount, 0);
				renderStats.drawnIndices += renderDebugFilledIndexCount;
				
				DebugEndLabelVk(cmdBuffer);
			}
			
			//draw mesh normals overlay
			if(enabledFeatures.geometryShader && renderSettings.meshNormals){
				DebugBeginLabelVk(cmdBuffer, "Debug Mesh Normals", draw_group_color);
				forI(renderModelCmdCount){
					RenderModelCmd& cmd = renderModelCmdArray[i];
					MaterialVk& mat = vkMaterials[cmd.material];
					DebugInsertLabelVk(cmdBuffer, cmd.name, draw_cmd_color);
					vkCmdPushConstants(cmdBuffer, pipelineLayouts.geometry, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(mat4), &cmd.matrix);
					vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.normals_debug);
					vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.geometry, 0, 1, &descriptorSets.geometry, 0, 0);
					vkCmdDrawIndexed(cmdBuffer, cmd.index_count, 1, cmd.index_offset, cmd.vertex_offset, 0);
					renderStats.drawnIndices += cmd.index_count;
				}
				DebugEndLabelVk(cmdBuffer);
			}
			
			//draw 2D stuff
			if(renderTwodVertexCount > 0 && renderTwodIndexCount > 0){
				DebugBeginLabelVk(cmdBuffer, "Twod", draw_group_color);
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.ui);//TODO(sushi) should this be twod?
				VkDeviceSize offsets[1] = {0};
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &uiVertexBuffer.buffer, offsets);
				vkCmdBindIndexBuffer(cmdBuffer, uiIndexBuffer.buffer, 0, INDEX_TYPE_VK_UI);
				Push2DVk push{};
				push.scale.x = 2.0f / (f32)activeSwapchain.width;
				push.scale.y = 2.0f / (f32)activeSwapchain.height;
				push.translate.x = -1.0f;
				push.translate.y = -1.0f;
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.twod, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push2DVk), &push);
				
				forX(layer, TWOD_LAYERS){
					forX(cmd_idx, renderTwodCmdCounts[renderActiveSurface][layer]){
						if(renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].index_count == 0) continue;
						
						auto cmd = renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx];

						scissor.offset.x = (u32)cmd.scissor_offset.x;
						scissor.offset.y = (u32)cmd.scissor_offset.y;
						scissor.extent.width  = (u32)cmd.scissor_extent.x;
						scissor.extent.height = (u32)cmd.scissor_extent.y;
						vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

						if(cmd.debug_info.count) 
							DebugInsertLabelVk(cmdBuffer, (char*)cmd.debug_info.str, draw_cmd_color);
						
						if(renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].handle){
							vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.twod, 0, 1,
													(VkDescriptorSet*)&renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].handle, 0, 0);
							vkCmdDrawIndexed(cmdBuffer, renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].index_count, 1,
											 renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].index_offset, 0, 0);
						}
					}
				}
				renderStats.drawnIndices += renderTwodIndexCount;
				
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				scissor.extent.width = activeSwapchain.width;
				scissor.extent.height = activeSwapchain.height;
				
				DebugEndLabelVk(cmdBuffer);
			}
			
			//draw external 2D buffers
			BufferVk* exvbuffs = (BufferVk*)externalVertexBuffers->start;
			BufferVk* exibuffs = (BufferVk*)externalIndexBuffers->start;
			forI(externalBufferCount){
				DebugBeginLabelVk(cmdBuffer, "External 2D Buffers", draw_group_color);
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.ui); //TODO(sushi) should this be twod?
				VkDeviceSize offsets[1] = {0};
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &exvbuffs[i].buffer, offsets);
				vkCmdBindIndexBuffer(cmdBuffer, exibuffs[i].buffer, 0, INDEX_TYPE_VK_UI);
				Push2DVk push{};
				push.scale.x = 2.0f / (f32)activeSwapchain.width;
				push.scale.y = 2.0f / (f32)activeSwapchain.height;
				push.translate.x = -1.0f;
				push.translate.y = -1.0f;
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.twod, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push2DVk), &push);
				
				forX(cmd, renderExternalCmdCounts[i]){
					RenderTwodCmd tcmd = renderExternalCmdArrays[i][cmd];
					if(!tcmd.index_count) continue;
					
					scissor.offset.x = (u32)tcmd.scissor_offset.x;
					scissor.offset.y = (u32)tcmd.scissor_offset.y;
					scissor.extent.width  = (u32)tcmd.scissor_extent.x;
					scissor.extent.height = (u32)tcmd.scissor_extent.y;
					vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
					
					if(tcmd.handle){
						vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.twod, 0, 1, (VkDescriptorSet*)&tcmd.handle, 0, 0);
						vkCmdDrawIndexed(cmdBuffer, tcmd.index_count, 1, tcmd.index_offset, 0, 0);
					}
				}
				//renderStats.drawnIndices += renderTwodIndexCount;
				
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				scissor.extent.width = activeSwapchain.width;
				scissor.extent.height = activeSwapchain.height;
				
				DebugEndLabelVk(cmdBuffer);
				
			}
			
			//DEBUG draw shadow map
			if(renderSettings.showShadowMap){
				viewport.x      = (f32)(activeSwapchain.width - 400);
				viewport.y      = (f32)(activeSwapchain.height - 400);
				viewport.width  = 400.f;
				viewport.height = 400.f;
				vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
				vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
				
				DebugBeginLabelVk(cmdBuffer, "DEBUG Shadow map quad", draw_group_color);
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.base, 0, 1, &descriptorSets.shadowMap_debug, 0, 0);
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.shadowmap_debug);
				vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
				DebugEndLabelVk(cmdBuffer);
				
				viewport.x      = 0;
				viewport.y      = 0;
				viewport.width  = (f32)activeSwapchain.width;
				viewport.height = (f32)activeSwapchain.height;
			}
			
			//draw topmost stuff (custom window decorations for now)
			if(renderTwodCmdCounts[renderActiveSurface][TWOD_LAYERS] > 1){
				DebugBeginLabelVk(cmdBuffer, "Topmost 2D (Window Decorations)", draw_group_color);
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.ui);
				VkDeviceSize offsets[1] = { 0 };
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &uiVertexBuffer.buffer, offsets);
				vkCmdBindIndexBuffer(cmdBuffer, uiIndexBuffer.buffer, 0, INDEX_TYPE_VK_UI);
				Push2DVk push{};
				push.scale.x = 2.0f / (f32)activeSwapchain.width;
				push.scale.y = 2.0f / (f32)activeSwapchain.height;
				push.translate.x = -1.0f;
				push.translate.y = -1.0f;
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.twod, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push2DVk), &push);
				
				viewport.x = 0;
				viewport.y = 0;
				viewport.width = (f32)activeSwapchain.width;
				viewport.height = (f32)activeSwapchain.height;
				viewport.minDepth = 0.f;
				viewport.maxDepth = 1.f;
				vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
				
				forX(cmd_idx, renderTwodCmdCounts[renderActiveSurface][TWOD_LAYERS]){
					scissor.offset.x = (u32)renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].scissor_offset.x;
					scissor.offset.y = (u32)renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].scissor_offset.y;
					scissor.extent.width = (u32)renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].scissor_extent.x;
					scissor.extent.height = (u32)renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].scissor_extent.y;
					vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
					
					if(renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].handle){
						vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.twod, 0, 1, (VkDescriptorSet*)&renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].handle, 0, 0);
						vkCmdDrawIndexed(cmdBuffer, renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].index_count, 1, renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].index_offset, 0, 0);
					}
				}
			}
			
			vkCmdEndRenderPass(cmdBuffer);
			DebugEndLabelVk(cmdBuffer);
		}
		
		resultVk = vkEndCommandBuffer(cmdBuffer); AssertVk(resultVk, "failed to end recording command buffer");
	}
}

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_init
void
render_init(){DPZoneScoped;
	DeshiStageInitStart(DS_RENDER, DS_PLATFORM, "Attempted to initialize Vulkan module before initializing Platform module");
	Log("vulkan","Starting vulkan renderer initialization");
	
	//create the shaders directory if it doesn't exist already
	file_create(str8_lit("data/shaders/"));
	
	//// load RenderSettings ////
	render_load_settings();
	if(renderSettings.debugging && renderSettings.printf){
		validation_features_enabled.add(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
		renderSettings.loggingLevel = 4;
	}
	
	Stopwatch t_temp = start_stopwatch();
	//// setup Vulkan instance ////
	SetupAllocator();
	CreateInstance();
	PrintVk(3, "Finished creating instance in ",reset_stopwatch(&t_temp),"ms");
	SetupDebugMessenger();
	PrintVk(3, "Finished setting up debug messenger in ",reset_stopwatch(&t_temp),"ms");
	
	//// grab Vulkan extension functions ////
#if BUILD_INTERNAL
	func_vkSetDebugUtilsObjectNameEXT  = (PFN_vkSetDebugUtilsObjectNameEXT) vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
	func_vkCmdBeginDebugUtilsLabelEXT  = (PFN_vkCmdBeginDebugUtilsLabelEXT) vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
	func_vkCmdEndDebugUtilsLabelEXT    = (PFN_vkCmdEndDebugUtilsLabelEXT)   vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
	func_vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT");
#endif //BUILD_INTERNAL
	
	//// setup Vulkan-OperatingSystem interactions ////
	CreateSurface();
	PrintVk(3, "Finished creating surface in ",reset_stopwatch(&t_temp),"ms");
	PickPhysicalDevice();
	PrintVk(3, "Finished picking physical device in ",reset_stopwatch(&t_temp),"ms");
	CreateLogicalDevice();
	PrintVk(3, "Finished creating logical device in ",reset_stopwatch(&t_temp),"ms");
	
	//// limit RenderSettings to device capabilties ////
	msaaSamples = (VkSampleCountFlagBits)(((1 << renderSettings.msaaSamples) > maxMsaaSamples) ? maxMsaaSamples : 1 << renderSettings.msaaSamples);
	renderSettings.anistropicFiltering = (enabledFeatures.samplerAnisotropy) ? renderSettings.anistropicFiltering : false;
	
	//// setup unchanging Vulkan objects ////
	CreateCommandPool();
	PrintVk(3, "Finished creating command pool in ",reset_stopwatch(&t_temp),"ms");
	CreateUniformBuffers();
	PrintVk(3, "Finished creating uniform buffer in ",reset_stopwatch(&t_temp),"ms");
	SetupShaderCompiler();
	PrintVk(3, "Finished setting up shader compiler in ",reset_stopwatch(&t_temp),"ms");
	CreateLayouts();
	PrintVk(3, "Finished creating layouts in ",reset_stopwatch(&t_temp),"ms");
	CreateDescriptorPool();
	PrintVk(3, "Finished creating descriptor pool in ",reset_stopwatch(&t_temp),"ms");
	SetupOffscreenRendering();
	PrintVk(3, "Finished setting up offscreen rendering in ",reset_stopwatch(&t_temp),"ms");
	
	//// setup window-specific Vulkan objects ////
	CreateSwapchain();
	PrintVk(3, "Finished creating swap chain in ",reset_stopwatch(&t_temp),"ms");
	CreateRenderpasses();
	PrintVk(3, "Finished creating render pass in ",reset_stopwatch(&t_temp),"ms");
	CreateFrames();
	PrintVk(3, "Finished creating frames in ",reset_stopwatch(&t_temp),"ms");
	CreateSyncObjects();
	PrintVk(3, "Finished creating sync objects in ",reset_stopwatch(&t_temp),"ms");
	CreateDescriptorSets();
	PrintVk(3, "Finished creating descriptor sets in ",reset_stopwatch(&t_temp),"ms");
	CreatePipelineCache();
	PrintVk(3, "Finished creating pipeline cache in ",reset_stopwatch(&t_temp),"ms");
	SetupPipelineCreation();
	PrintVk(3, "Finished setting up pipeline creation in ",reset_stopwatch(&t_temp),"ms");
	CreatePipelines();
	PrintVk(3, "Finished creating pipelines in ",reset_stopwatch(&t_temp),"ms");
	
	//// init shared render ////
	memory_pool_init(deshi__render_buffer_pool, 16);
	externalVertexBuffers = memory_create_arena(sizeof(BufferVk)*MAX_EXTERNAL_BUFFERS);
	externalIndexBuffers = memory_create_arena(sizeof(BufferVk)*MAX_EXTERNAL_BUFFERS);
	externalBufferCount = 0;
	
	initialized = true;
	DeshiStageInitEnd(DS_RENDER);
}

//// ~~~ NEW BACKEND STUFF ~~~ ////

u32
find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties) {
	PrintVk(6, "Finding memory types");
	Assert(physical_device != VK_NULL_HANDLE, "find_memory_type called before picking physical device");
	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

	forI(mem_props.memoryTypeCount) {
		if((type_filter & (1 << i)) && HasAllFlags(mem_props.memoryTypes[i].propertyFlags, properties)){
			return i;
		}
	}

	LogE("vulkan", "failed to find suitable memory type");
	Assert(0);
	return 0;
}

void 
create_or_resize_buffer(VkBuffer* buffer, 
		                VkDeviceMemory* buffer_memory, 
						VkDeviceSize* buffer_size, 
		                u64 new_size, 
						VkBufferUsageFlags usage, 
						VkMemoryPropertyFlags properties) {
	VkBuffer old_buffer = *buffer; 
	VkDeviceMemory old_buffer_memory = *buffer_memory; 
	*buffer = VK_NULL_HANDLE;
	*buffer_memory = VK_NULL_HANDLE;

	VkDeviceSize aligned_buffer_size = RoundUpTo(new_size, buffer_memory_alignment);
	VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	buffer_info.size = aligned_buffer_size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	resultVk = vkCreateBuffer(device, &buffer_info, allocator, buffer);
	Assert(resultVk, "failed to create buffer");

	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(device, *buffer, &req);
	buffer_memory_alignment = 
		(buffer_memory_alignment > req.alignment ?
		 buffer_memory_alignment :
		 req.alignment);
	VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	alloc_info.allocationSize = req.size;
	alloc_info.memoryTypeIndex = find_memory_type(req.memoryTypeBits, properties);
	resultVk = vkAllocateMemory(device, &alloc_info, allocator, buffer_memory);
	AssertVk(resultVk, "failed to allocate memory");
	resultVk = vkBindBufferMemory(device, *buffer, *buffer_memory, 0);
	AssertVk(resultVk, "failed to bind buffer memory");

	if(buffer_size) {
		void* old_buffer_data,* new_buffer_data;
		resultVk = vkMapMemory(device, old_buffer_memory, 0, *buffer_size, 0, &old_buffer_data);
		AssertVk(resultVk, "failed to map old buffer memory");
		resultVk = vkMapMemory(device, *buffer_memory, 0, aligned_buffer_size, 0, &new_buffer_data);
		AssertVk(resultVk, "failed to map new buffer memory");

		CopyMemory(new_buffer_data, old_buffer_data, *buffer_size);

		VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
		range.memory = *buffer_memory;
		range.offset = 0;
		range.size   = VK_WHOLE_SIZE;
		resultVk = vkFlushMappedMemoryRanges(device, 1, &range);
		AssertVk(resultVk, "failed to flush mapped memory range");

		vkUnmapMemory(device, old_buffer_memory);
		vkUnmapMemory(device, *buffer_memory);
	}

	if(old_buffer != VK_NULL_HANDLE) vkDestroyBuffer(device, old_buffer, allocator);
	if(old_buffer_memory != VK_NULL_HANDLE) vkFreeMemory(device, old_buffer_memory, allocator);

	*buffer_size = aligned_buffer_size;
}

VkImageView
create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, u32 mip_levels) {
	PrintVk(4, "Creating image view");
	Assert(device != VK_NULL_HANDLE, "create_image_view called before logical device creation");

	VkImageViewCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	info.image    = image;
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.format   = format;
	info.subresourceRange.aspectMask     = aspect_flags;
	info.subresourceRange.baseMipLevel   = 0;
	info.subresourceRange.levelCount     = mip_levels;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount     = 1;

	VkImageView view{};
	resultVk = vkCreateImageView(device, &info, allocator, &view);
	AssertVk(resultVk);

	return view;
}

void
create_image(u32 width, u32 height,
		     u32 mip_levels,
			 VkSampleCountFlagBits num_samples,
			 VkFormat format,
			 VkImageTiling tiling, 
			 VkImageUsageFlags usage,
			 VkMemoryPropertyFlags properties,
			 VkImage* image,
			 VkDeviceMemory* image_memory) {
	PrintVk(4, "Creating image");

	VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	image_info.imageType     = VK_IMAGE_TYPE_2D;
	image_info.extent.width  = width;
	image_info.extent.height = height;
	image_info.extent.depth  = 1;
	image_info.mipLevels     = mip_levels;
	image_info.arrayLayers   = 1;
	image_info.format        = format;
	image_info.tiling        = tiling;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage         = usage;
	image_info.samples       = num_samples;
	image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	resultVk = vkCreateImage(device, &image_info, allocator, image); 
	AssertVk(resultVk, "failed to create image");

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(device, *image, &mem_reqs);
	
	VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	alloc_info.allocationSize = mem_reqs.size;
	alloc_info.memoryTypeIndex = find_memory_type(mem_reqs.memoryTypeBits, properties);
	resultVk = vkAllocateMemory(device, &alloc_info, allocator, image_memory); AssertVk(resultVk, "failed to allocate image memory");
	
	vkBindImageMemory(device, *image, *image_memory, 0);
}

local void
setup_allocator(){DPZoneScoped;
	//!ref: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkAllocationCallbacks.html
	PrintVk(2,"Setting up vulkan allocator");
	Assert(renderStage == RENDERERSTAGE_NONE, "renderer stage was not NONE at SetupAllocator");
	
	Stopwatch watch = start_stopwatch();

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

	PrintVk(2, "Finished setting up vulkan allocator in ",peek_stopwatch(watch),"ms");
}

void
create_instance(Window* window) {
	PrintVk(2,"Creating vulkan instance");
	Assert(renderStage == RENDERERSTAGE_NONE, "renderer stage was not NONE at CreateInstance");
	renderStage |= RSVK_INSTANCE;
	
	Stopwatch watch = start_stopwatch();

	//check for validation layer support
	if(renderSettings.debugging){
		PrintVk(3,"Checking validation layer support");
		b32 has_support = true;
		
		u32 layer_count = 0;
		vkEnumerateInstanceLayerProperties(&layer_count, 0);
		VkLayerProperties* available_layers;
		array_init(available_layers, layer_count, deshi_temp_allocator);
		array_count(available_layers) = layer_count; // NOTE(sushi) no idea if this will work properly
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers);
		
		forI(ArrayCount(validation_layers)){
			bool layer_found = false;
			for_array(available_layers){
				if(strcmp(validation_layers[i], it->layerName) == 0){
					layer_found = true;
					break;
				}
			}
			if(!layer_found) Assert(!"validation layer requested, but not available");
		}
	}
	
	//set instance's application info
	VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	if(window) {
		app_info.pApplicationName = (char*)window->title.str;
	} else {
		// TODO(sushi) need a way to set this if a Window is not provided
		app_info.pApplicationName = "unknown_app_name";
	}
	app_info.applicationVersion = VK_MAKE_VERSION(1,0,0);
	app_info.pEngineName        = "deshi";
	app_info.engineVersion      = VK_MAKE_VERSION(1,0,0);
	app_info.apiVersion         = VK_API_VERSION_1_3;
	
	VkValidationFeaturesEXT validation_features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
	validation_features.disabledValidationFeatureCount = 0;
	validation_features.pDisabledValidationFeatures    = 0;
	validation_features.enabledValidationFeatureCount  = array_count(validation_features_enabled_x);
	validation_features.pEnabledValidationFeatures     = validation_features_enabled_x;
	
	//get required extensions
	PrintVk(3, "Getting required extensions");
#if DESHI_WINDOWS
	const char* extensions[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#  if BUILD_INTERNAL
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#  endif //#if BUILD_INTERNAL
	};
#elif DESHI_LINUX //#if DESHI_WINDOWS
	const char* extensions[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#  if BUILD_INTERNAL
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#  endif //#if BUILD_INTERNAL
	};
#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
#  error "unhandled platform/vulkan interaction"
#endif //#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
	
	//setup instance debug messenger
	VkDebugUtilsMessengerCreateInfoEXT debug_create_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	debug_create_info.messageSeverity = callback_severities;
	debug_create_info.messageType     = callback_types;
	debug_create_info.pfnUserCallback = DebugCallback;
	
	//create the instance
	VkInstanceCreateInfo create_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	create_info.pApplicationInfo        = &app_info;
	create_info.enabledExtensionCount   = (u32)ArrayCount(extensions);
	create_info.ppEnabledExtensionNames = extensions;
	if(renderSettings.debugging){
		create_info.enabledLayerCount   = (u32)ArrayCount(validation_layers);
		create_info.ppEnabledLayerNames = validation_layers;
		debug_create_info.pNext         = &validation_features;
		create_info.pNext               = &debug_create_info;
	} else {
		create_info.enabledLayerCount   = 0;
		create_info.pNext               = 0;
	}
	resultVk = vkCreateInstance(&create_info, allocator, &instance); AssertVk(resultVk, "failed to create instance");

	PrintVk(3, "Finished creating instance in ",peek_stopwatch(watch),"ms");
}

void
setup_debug_messenger() { 
	PrintVk(2, "Setting up debug messenger");
	AssertRS(RSVK_INSTANCE, "SetupDebugMessenger was called before CreateInstance");
	
	Stopwatch watch = start_stopwatch();

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	debug_create_info.messageSeverity = callback_severities;
	debug_create_info.messageType     = callback_types;
	debug_create_info.pfnUserCallback = DebugCallback;
	
	VkResult err = VK_ERROR_EXTENSION_NOT_PRESENT;
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if(func != 0){
		err = func(instance, &debug_create_info, allocator, &debugMessenger);
	}
	AssertVk(err, "failed to setup debug messenger");

	PrintVk(2, "Finished setting up debug messenger in ",peek_stopwatch(watch),"ms");
}

void
create_surface(Window* window) {
	Assert(window, "create_surface given a null window");
	Assert(instance != VK_NULL_HANDLE, "create_surface called before create_instance");

	auto wi = (VkWindowInfo*)window->render_info;

	// TODO(sushi) maybe remake the surface if this happens
	Assert(wi->surface == VK_NULL_HANDLE, "create_surface called for a window that already has a surface");

	// create the surface
#if DESHI_WINDOWS
	PrintVk(2, "Creating win32-vulkan surface");
	VkWin32SurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
	info.hwnd = (HWND)win->handle;
	info.hinstance = (HINSTANCE)win32_console_instance;
	resultVk = vkCreateWin32SurfaceKHR(instance, &info, 0, &surfaces[surface_idx]); AssertVk(resultVk, "failed to create win32 surface");
#elif DESHI_LINUX
	PrintVk(2, "creating x11-vulkan surface");
	VkXlibSurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
	info.window = (X11Window)window->handle;
	info.dpy = linux.x11.display;
	resultVk = vkCreateXlibSurfaceKHR(instance, &info, 0, &wi->surface);
#else
#	error "unsupported platform for renderer"
#endif
}

void
pick_physical_device(Window* window) {
	PrintVk(2, "Picking physical device");

	Stopwatch watch = start_stopwatch();
	
	u32 device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, 0);
	VkPhysicalDevice* devices;
	array_init(devices, device_count, deshi_temp_allocator);
	array_count(devices) = device_count;
	vkEnumeratePhysicalDevices(instance, &device_count, devices);

	for_array(devices) {
		auto device = *it;
		
		// find a device which supports graphics operations
		// TODO(sushi) when somehow specified by the user, also check for compute graphics support
		//             or just always check and if not supported flag it so we can error on any
		//             compute shader requests later

		u32 queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
		VkQueueFamilyProperties* queue_families;
		array_init(queue_families, queue_family_count, deshi_temp_allocator);
		array_count(queue_families) = queue_family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

		forI(queue_family_count) {
			auto family = queue_families[i];
			if(HasFlag(family.queueFlags, VK_QUEUE_GRAPHICS_BIT)) {
				physical_queue_families.found_graphics_family = true;
				physical_queue_families.graphics_family = i;
				if(!window) break;
			}
			
			if(window) {
				VkBool32 present_support = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, ((VkWindowInfo*)window->render_info)->surface, &present_support);
				if(present_support) {
					physical_queue_families.found_present_family = true;
					physical_queue_families.present_family = i;
				}

				if(physical_queue_families.found_graphics_family && physical_queue_families.found_graphics_family) break;
			}
		}

		if(!physical_queue_families.found_graphics_family) continue;

		// check if the chosen device supports enabled extensions
		
		u32 extension_count;
		vkEnumerateDeviceExtensionProperties(device, 0, &extension_count, 0);
		VkExtensionProperties* available_extensions;
		array_init(available_extensions, extension_count, deshi_temp_allocator);
		array_count(available_extensions) = extension_count;
		vkEnumerateDeviceExtensionProperties(device, 0, &extension_count, available_extensions);

		u32 count = 0;
		forI(extension_count) {
			auto extension = available_extensions[i];
			forI(ArrayCount(deviceExtensions)) {
				if(extension.extensionName == deviceExtensions[i]) {
					count++;
					break;
				}
			}
			if(count == ArrayCount(deviceExtensions)) break;
		}

		if(count != ArrayCount(deviceExtensions)) continue;

		if(window) {
			u32 format_count;
			u32 present_mode_count;
			auto surface = ((VkWindowInfo*)window->render_info)->surface;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, 0);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, 0);
			if(!(present_mode_count&&format_count)) continue;
		}

		physical_device = device;
		break;
	}

	if(physical_device == VK_NULL_HANDLE) {
		LogE("vulkan", "failed to find a suitable GPU that supports vulkan");
		Assert(0);
	}

	vkGetPhysicalDeviceFeatures(physical_device, &deviceFeatures);

	PrintVk(2, "Finished picking physical device in ",peek_stopwatch(watch),"ms");
}

void
create_logical_device(Window* window) {
	PrintVk(2, "Creating logical device");

	Stopwatch watch = start_stopwatch();
	
	f32 queue_priority = 1.f;
	VkDeviceQueueCreateInfo* queue_create_infos;
	array_init(queue_create_infos, 1, deshi_temp_allocator);
	VkDeviceQueueCreateInfo queue_create_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queue_create_info.queueFamilyIndex = physical_queue_families.graphics_family;
	queue_create_info.queueCount       = 1;
	queue_create_info.pQueuePriorities = &queue_priority;
	array_push_value(queue_create_infos, queue_create_info);

	if(window && physical_queue_families.present_family != physical_queue_families.graphics_family) {
		queue_create_info.queueFamilyIndex = physical_queue_families.present_family;
		array_push_value(queue_create_infos, queue_create_info);
	}

	if(deviceFeatures.samplerAnisotropy) {
		enabledFeatures.samplerAnisotropy = VK_TRUE; // anistrophic filtering
		enabledFeatures.sampleRateShading = VK_TRUE; // sample shading
	}

	if(deviceFeatures.fillModeNonSolid) {
		enabledFeatures.fillModeNonSolid = VK_TRUE; // wireframe
		if(deviceFeatures.wideLines) {
			enabledFeatures.wideLines = VK_TRUE; // wide lines
		}
	}

	if(renderSettings.debugging) {
		if(deviceFeatures.geometryShader) {
			enabledFeatures.geometryShader = VK_TRUE;
		}
	}

	VkDeviceCreateInfo create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	create_info.pQueueCreateInfos       = queue_create_infos;
	create_info.queueCreateInfoCount    = array_count(queue_create_infos);
	create_info.pEnabledFeatures        = &enabledFeatures;
	create_info.enabledExtensionCount   = (u32)ArrayCount(deviceExtensions);
	create_info.ppEnabledExtensionNames = deviceExtensions;

	if(renderSettings.debugging) {
		create_info.enabledLayerCount   = (u32)ArrayCount(validation_layers);
		create_info.ppEnabledLayerNames = validation_layers;
	} else {
		create_info.enabledLayerCount = 0;
	}

	resultVk = vkCreateDevice(physical_device, &create_info, allocator, &device);
	AssertVk(resultVk, "failed to create logical device");

	vkGetDeviceQueue(device, physical_queue_families.graphics_family, 0, &graphicsQueue);
	if(window)
		vkGetDeviceQueue(device, physical_queue_families.present_family, 0, &presentQueue);

	PrintVk(2, "Finished creating logical device in ",peek_stopwatch(watch),"ms");
}

void
create_command_pool() {
	PrintVk(2, "Creating command pool");
	Assert(device != VK_NULL_HANDLE, "create_command_pool called before logical device was created");

	Stopwatch watch = start_stopwatch();

	VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	// NOTE(sushi) this flag allows any command buffer allocated from a pool to be 
	//             individually reset to its initial state through vkResetCommandBuffer
	//             implicitly by vkBeginCommandBuffer
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = physical_queue_families.graphics_family;

	resultVk = vkCreateCommandPool(device, &pool_info, allocator, &commandPool);
	AssertVk(resultVk, "failed to create command pool");
	
	PrintVk(2, "Finished creating command pool in ", peek_stopwatch(watch), "ms");
}

void
update_uniform_buffers() {
	PrintVk(2, "Updating uniform buffers");

	void* data;

	// offscreen vertex shader ubo
	uboVSoffscreen.values.lightVP =
		Math::LookAtMatrix(vkLights[0].toVec3(), vec3::ZERO).Inverse() *
		Math::PerspectiveProjectionMatrix((f32)renderSettings.shadowResolution, (f32)renderSettings.shadowResolution, 90.0f, renderSettings.shadowNearZ, renderSettings.shadowFarZ);
	
	vkMapMemory(device, uboVSoffscreen.bufferMemory, 0, sizeof(uboVSoffscreen.values), 0, &data);{
		CopyMemory(data, &uboVSoffscreen.values, sizeof(uboVSoffscreen.values));
	}vkUnmapMemory(device, uboVSoffscreen.bufferMemory);

	// scene vertex shader ubo
	uboVS.values.time = DeshTime->totalTime;
	CopyMemory(uboVS.values.lights, vkLights, 10*sizeof(vec4));
	uboVS.values.screen = Vec2((f32)activeSwapchain.extent.width, (f32)activeSwapchain.extent.height);
	uboVS.values.mousepos = input_mouse_position();
	if(initialized) uboVS.values.mouseWorld = Math::ScreenToWorld(input_mouse_position(), uboVS.values.proj, uboVS.values.view, Vec2(DeshWindow->width,DeshWindow->height));
	uboVS.values.enablePCF = renderSettings.shadowPCF;
	uboVS.values.lightVP = uboVSoffscreen.values.lightVP;
	
	vkMapMemory(device, uboVS.bufferMemory, 0, sizeof(uboVS.values), 0, &data);{
		memcpy(data, &uboVS.values, sizeof(uboVS.values));
	}vkUnmapMemory(device, uboVS.bufferMemory);

	if(enabledFeatures.geometryShader) {
		uboGS.values.view = uboVS.values.view;
		uboGS.values.proj = uboVS.values.proj;
		
		vkMapMemory(device, uboGS.bufferMemory, 0, sizeof(uboGS.values), 0, &data);{
			memcpy(data, &uboGS.values, sizeof(uboGS.values));
		}vkUnmapMemory(device, uboGS.bufferMemory);
	}
}

void
create_uniform_buffers() {
	PrintVk(2, "Creating uniform buffers");
	Assert(device != VK_NULL_HANDLE, "create_uniform_buffer called before logical device creation");

	Stopwatch watch = start_stopwatch();

	// create scene vertex shader ubo
	create_or_resize_buffer(
			&uboVS.buffer, &uboVS.bufferMemory, &uboVS.bufferSize,
			sizeof(uboVS.values), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	uboVS.bufferDescriptor.buffer = uboVS.buffer;
	uboVS.bufferDescriptor.offset = 0;
	uboVS.bufferDescriptor.range  = sizeof(uboVS.values);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)uboVS.buffer, "Scene vertex shader UBO");

	// normals geometry shader ubo
	if(enabledFeatures.geometryShader) {
		create_or_resize_buffer(
				&uboGS.buffer, &uboGS.bufferMemory, &uboGS.bufferSize,
				sizeof(uboGS.values), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		uboGS.bufferDescriptor.buffer = uboGS.buffer;
		uboGS.bufferDescriptor.offset = 0;
		uboGS.bufferDescriptor.range  = sizeof(uboVS.values);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)uboGS.buffer, "Geometry shader UBO");
	}

	// offscreen vertex shader ubo
	create_or_resize_buffer(
			&uboVSoffscreen.buffer, &uboVSoffscreen.bufferMemory, &uboVSoffscreen.bufferSize,
			 sizeof(uboVSoffscreen.values), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	uboVSoffscreen.bufferDescriptor.buffer = uboVSoffscreen.buffer;
	uboVSoffscreen.bufferDescriptor.offset = 0;
	uboVSoffscreen.bufferDescriptor.range  = sizeof(uboVSoffscreen.values);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)uboVSoffscreen.buffer, "Offscreen vertex shader UBO");

	update_uniform_buffers();
	PrintVk(2, "Finished creating uniform buffers in ",peek_stopwatch(watch),"ms");
}

void
setup_shader_compiler() {
	PrintVk(2, "Setting up shader compiler");

	Stopwatch watch = start_stopwatch();

	shader_compiler = shaderc_compiler_initialize();
	shader_compiler_options = shaderc_compile_options_initialize();
	if(renderSettings.optimizeShaders) {
		shaderc_compile_options_set_optimization_level(shader_compiler_options, shaderc_optimization_level_performance);
	}

	PrintVk(2, "Finished setting up shader compiler in ", peek_stopwatch(watch), "ms");
}

void
create_layouts() {
	PrintVk(2, "Creating layouts");
	Assert(device != VK_NULL_HANDLE, "create_layouts called before logical device creation");

	Stopwatch watch = start_stopwatch();

	VkDescriptorSetLayoutBinding set_layout_bindings[4]{};
	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	descriptor_set_layout_info.pBindings    = set_layout_bindings;
	descriptor_set_layout_info.bindingCount = 0;

	// base descriptor set layout
	// binding 0: vertex shader scene ubo
	set_layout_bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	set_layout_bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
	set_layout_bindings[0].binding         = 0;
	set_layout_bindings[0].descriptorCount = 1;
	// binding 1: fragment shader shadow map image sampler
	set_layout_bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	set_layout_bindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
	set_layout_bindings[1].binding         = 1;
	set_layout_bindings[1].descriptorCount = 1;

	descriptor_set_layout_info.bindingCount = 2;
	resultVk = vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, allocator, &descriptorSetLayouts.base);
	AssertVk(resultVk, "failed to create descriptor set layout");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)descriptorSetLayouts.base,
							 "Base descriptor set layout");


	// textures descriptor set layout
	// binding 0: fragment shader color/albedo map
	set_layout_bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	set_layout_bindings[0].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
	set_layout_bindings[0].binding         = 0;
	set_layout_bindings[0].descriptorCount = 1;
	
	// binding 1: fragment shader normal map
	set_layout_bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	set_layout_bindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
	set_layout_bindings[1].binding         = 1;
	set_layout_bindings[1].descriptorCount = 1;

	// binding 2: fragment shader specular/reflective map
	set_layout_bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; 
	set_layout_bindings[2].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
	set_layout_bindings[2].binding         = 2;
	set_layout_bindings[2].descriptorCount = 1;

	// binding 3: fragment shader light/emissive map
	set_layout_bindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	set_layout_bindings[3].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
	set_layout_bindings[3].binding         = 3;
	set_layout_bindings[3].descriptorCount = 1;

	descriptor_set_layout_info.bindingCount = 4;
	resultVk = vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, allocator, &descriptorSetLayouts.textures);
	AssertVk(resultVk, "failed to create descriptor set layout");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)descriptorSetLayouts.textures,
							 "Textures descriptor set layout");

	// TODO(sushi) instances descriptor layout
	
	// twod descriptor set layout
	// binding 0: fragment shader font image sampler
	set_layout_bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	set_layout_bindings[0].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
	set_layout_bindings[0].binding         = 0;
	set_layout_bindings[0].descriptorCount = 1;

	descriptor_set_layout_info.bindingCount = 4;
	resultVk = vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, allocator, &descriptorSetLayouts.base);
	AssertVk(resultVk, "failed to create descriptor set layout");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)descriptorSetLayouts.twod,
							 "2D descriptor set layout");

	// geometry descriptor layout
	if(enabledFeatures.geometryShader) {
		// binding 0: vertex shader scene ubo
		set_layout_bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		set_layout_bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
		set_layout_bindings[0].binding         = 0;
		set_layout_bindings[0].descriptorCount = 1;
		//binding 1: geometry shader UBO
		set_layout_bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		set_layout_bindings[1].stageFlags      = VK_SHADER_STAGE_GEOMETRY_BIT;
		set_layout_bindings[1].binding         = 1;
		set_layout_bindings[1].descriptorCount = 1;
		
		descriptor_set_layout_info.bindingCount = 2;
		resultVk = vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, allocator, &descriptorSetLayouts.geometry); 
		AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)descriptorSetLayouts.geometry,
							 "Geometry descriptor set layout");
	}

	
	VkPushConstantRange push_constant_range{};
	VkPipelineLayoutCreateInfo pipeline_layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

	// base pipeline layout
	// push constants for passing model matrix
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	push_constant_range.offset     = 0;
	push_constant_range.size       = sizeof(mat4);
	
	VkDescriptorSetLayout set_layouts[] = {
		descriptorSetLayouts.base, descriptorSetLayouts.textures
	};
	
	pipeline_layout_info.setLayoutCount         = ArrayCount(set_layouts);
	pipeline_layout_info.pSetLayouts            = set_layouts;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges    = &push_constant_range;
	resultVk = vkCreatePipelineLayout(device, &pipeline_layout_info, allocator, &pipelineLayouts.base); AssertVk(resultVk);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)pipelineLayouts.base,
						 "Base pipeline layout");

	// twod pipeline layout
	// push constants for passing scale and translate
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	push_constant_range.offset     = 0;
	push_constant_range.size       = sizeof(Push2DVk);
	
	pipeline_layout_info.setLayoutCount         = 1;
	pipeline_layout_info.pSetLayouts            = &descriptorSetLayouts.twod;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges    = &push_constant_range;
	resultVk = vkCreatePipelineLayout(device, &pipeline_layout_info, allocator, &pipelineLayouts.twod); AssertVk(resultVk);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)pipelineLayouts.twod,
						 "2D pipeline layout");

	if(enabledFeatures.geometryShader) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
		pushConstantRange.offset     = 0;
		pushConstantRange.size       = sizeof(mat4);
		
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		pipelineLayoutInfo.setLayoutCount         = 1;
		pipelineLayoutInfo.pSetLayouts            = &descriptorSetLayouts.geometry;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;
		resultVk = vkCreatePipelineLayout(device, &pipelineLayoutInfo, allocator, &pipelineLayouts.geometry); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)pipelineLayouts.geometry,
							 "Geometry pipeline layout");
	}

	PrintVk(2, "Finished creating layouts in", peek_stopwatch(watch), "ms");
}

void
create_descriptor_pool() {
	PrintVk(2, "Creating descriptor pool");
	Assert(device != VK_NULL_HANDLE, "create_descriptor_pool called before logical device creation");

	Stopwatch watch = start_stopwatch();

	VkDescriptorPoolSize pool_sizes[] = {
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

	VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets       = 1000 * ArrayCount(pool_sizes);
	pool_info.poolSizeCount = ArrayCount(pool_sizes);
	pool_info.pPoolSizes    = pool_sizes;
	resultVk = vkCreateDescriptorPool(device, &pool_info, allocator, &descriptorPool);
	AssertVk(resultVk);

	PrintVk(2, "Finished creating descriptor pool in ",peek_stopwatch(watch),"ms");
}

void
create_pipeline_cache() {
	PrintVk(2, "Creating pipeline cache");
	Assert(device != VK_NULL_HANDLE, "create_pipeline_cache called before logical device creation");

	Stopwatch watch = start_stopwatch();

	VkPipelineCacheCreateInfo pipeline_cache_create_info{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};

	if(file_exists(str8l("data/pipelines.cache"))) {
		str8 data = file_read_simple(str8l("data/pipelines.cache"), deshi_temp_allocator);
		pipeline_cache_create_info.initialDataSize = data.count;
		pipeline_cache_create_info.pInitialData    = data.str;
	}

	resultVk = vkCreatePipelineCache(device, &pipeline_cache_create_info, 0, &pipelineCache);
	AssertVk(resultVk);
	PrintVk(2, "Finished creating pipeline cache in ", peek_stopwatch(watch), "ms");
}

void
setup_pipeline_creation() {
	PrintVk(2, "Setting up pipeline creation");

	Stopwatch watch = start_stopwatch();

	array_init(vertex_input_bindings_x, 1, deshi_allocator);
	array_init(vertex_input_attributes_x, 4, deshi_allocator);
	array_init(twod_vertex_input_bindings_x, 1, deshi_allocator);
	array_init(twod_vertex_input_attributes_x, 3, deshi_allocator);

	*array_push(vertex_input_bindings_x) = {0, sizeof(MeshVertex), VK_VERTEX_INPUT_RATE_VERTEX};

	*array_push(vertex_input_attributes_x) = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MeshVertex, pos)};
	*array_push(vertex_input_attributes_x) = {1, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(MeshVertex, uv)};
	*array_push(vertex_input_attributes_x) = {2, 0, VK_FORMAT_R8G8B8A8_UNORM,   offsetof(MeshVertex, color)};
	*array_push(vertex_input_attributes_x) = {3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MeshVertex, normal)};

	vertex_input_state.vertexBindingDescriptionCount   = (u32)vertex_input_bindings.count;
	vertex_input_state.pVertexBindingDescriptions      = vertex_input_bindings.data;
	vertex_input_state.vertexAttributeDescriptionCount = (u32)vertex_input_attributes.count;
	vertex_input_state.pVertexAttributeDescriptions    = vertex_input_attributes.data;
	
	*array_push(twod_vertex_input_bindings_x) = {0, sizeof(Vertex2), VK_VERTEX_INPUT_RATE_VERTEX};

	*array_push(twod_vertex_input_attributes_x) = {0, 0, VK_FORMAT_R32G32_SFLOAT,  offsetof(Vertex2, pos)};
	*array_push(twod_vertex_input_attributes_x) = {1, 0, VK_FORMAT_R32G32_SFLOAT,  offsetof(Vertex2, uv)};
	*array_push(twod_vertex_input_attributes_x) = {2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(Vertex2, color)};

	twod_vertex_input_state.vertexBindingDescriptionCount   = (u32)array_count(twod_vertex_input_bindings_x);
	twod_vertex_input_state.pVertexBindingDescriptions      = twod_vertex_input_bindings_x;
	twod_vertex_input_state.vertexAttributeDescriptionCount = (u32)array_count(twod_vertex_input_attributes_x);
	twod_vertex_input_state.pVertexAttributeDescriptions    = twod_vertex_input_attributes_x;
	
	//determines how to group vertices together
	//https://renderdoc.org/vkspec_chunked/chap22.html#VkPipelineInputAssemblyStateCreateInfo
	input_assembly_state.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state.primitiveRestartEnable = VK_FALSE;
	
	//container for viewports and scissors
	//https://renderdoc.org/vkspec_chunked/chap27.html#VkPipelineViewportStateCreateInfo
	viewport_state.viewportCount = 1;
	viewport_state.pViewports    = 0;
	viewport_state.scissorCount  = 1;
	viewport_state.pScissors     = 0;
	
	//how to draw/cull/depth things
	//https://renderdoc.org/vkspec_chunked/chap28.html#VkPipelineRasterizationStateCreateInfo
	rasterization_state.depthClampEnable        = VK_FALSE; //look into for shadowmapping
	rasterization_state.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state.polygonMode             = VK_POLYGON_MODE_FILL; //draw mode: fill, wireframe, vertices
	rasterization_state.cullMode                = VK_CULL_MODE_BACK_BIT;
	rasterization_state.frontFace               = VK_FRONT_FACE_CLOCKWISE; //VK_FRONT_FACE_COUNTER_CLOCKWISE
	rasterization_state.depthBiasEnable         = VK_FALSE;
	rasterization_state.depthBiasConstantFactor = 0.0f;
	rasterization_state.depthBiasClamp          = 0.0f;
	rasterization_state.depthBiasSlopeFactor    = 0.0f;
	rasterization_state.lineWidth               = 1.0f;
	
	//useful for multisample anti-aliasing (MSAA)
	//https://renderdoc.org/vkspec_chunked/chap28.html#VkPipelineMultisampleStateCreateInfo
	multisample_state.rasterizationSamples  = msaaSamples;
	multisample_state.sampleShadingEnable   = (msaaSamples != VK_SAMPLE_COUNT_1_BIT) ? VK_TRUE : VK_FALSE;
	multisample_state.minSampleShading      = .2f; //min fraction for sample shading; closer to one is smoother
	multisample_state.pSampleMask           = 0;
	multisample_state.alphaToCoverageEnable = VK_FALSE;
	multisample_state.alphaToOneEnable      = VK_FALSE;
	
	//depth testing and discarding
	//https://renderdoc.org/vkspec_chunked/chap29.html#VkPipelineDepthStencilStateCreateInfo
	depth_stencil_state.depthTestEnable       = VK_TRUE;
	depth_stencil_state.depthWriteEnable      = VK_TRUE;
	depth_stencil_state.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_state.stencilTestEnable     = VK_FALSE;
	depth_stencil_state.minDepthBounds        = 0.0f;
	depth_stencil_state.maxDepthBounds        = 1.0f;
	depth_stencil_state.stencilTestEnable     = VK_FALSE;
	depth_stencil_state.front                 = {};
	depth_stencil_state.back.compareOp        = VK_COMPARE_OP_ALWAYS;
	
	//how to combine colors; alpha: options to allow alpha blending
	//https://renderdoc.org/vkspec_chunked/chap30.html#VkPipelineColorBlendAttachmentState
	color_blend_attachment_state.blendEnable         = VK_TRUE;
	color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
	color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;
	color_blend_attachment_state.colorWriteMask      = 0xF; //RGBA
	
	//container struct for color blend attachments with overall blending constants
	//https://renderdoc.org/vkspec_chunked/chap30.html#VkPipelineColorBlendStateCreateInfo
	color_blend_state.logicOpEnable     = VK_FALSE;
	color_blend_state.logicOp           = VK_LOGIC_OP_COPY; //TODO(delle) maybe VK_LOGIC_OP_CLEAR?
	color_blend_state.attachmentCount   = 1;
	color_blend_state.pAttachments      = &color_blend_attachment_state;
	color_blend_state.blendConstants[0] = 0.0f;
	color_blend_state.blendConstants[1] = 0.0f;
	color_blend_state.blendConstants[2] = 0.0f;
	color_blend_state.blendConstants[3] = 0.0f;
	
	//dynamic states that can vary in the command buffer
	//https://renderdoc.org/vkspec_chunked/chap11.html#VkPipelineDynamicStateCreateInfo
	dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	dynamic_state.dynamicStateCount = (u32)dynamic_states.count;
	dynamic_state.pDynamicStates    = dynamic_states.data;
	
	//base pipeline info and options
	pipeline_create_info.stageCount          = 0;
	pipeline_create_info.pStages             = shader_stages;
	pipeline_create_info.pVertexInputState   = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pTessellationState  = 0;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.layout              = pipelineLayouts.base;
	pipeline_create_info.renderPass          = renderPass;
	pipeline_create_info.subpass             = 0;
	pipeline_create_info.basePipelineHandle  = VK_NULL_HANDLE;
	pipeline_create_info.basePipelineIndex   = -1;

	PrintVk(2, "finished setting up pipeline creation in ", peek_stopwatch(watch), "ms");
}

void
create_pipelines() { 
	PrintVk(2, "Creating pipelines");
	
	Stopwatch watch = start_stopwatch();

	//destroy previous pipelines
	forI(ArrayCount(pipelines.array)){
		if(pipelines.array[i]) vkDestroyPipeline(device, pipelines.array[i], 0);
	}
	
	//setup specialization constants
	/*
VkSpecializationMapEntry entryShadowPCF{};
	entryShadowPCF.constantID = 0;
	entryShadowPCF.offset = 0;
	entryShadowPCF.size = sizeof(bool);
	*/
	
	VkSpecializationInfo specialization_info{};
	/*
specializationInfo.mapEntryCount = 1;
	specializationInfo.pMapEntries = &entryShadowPCF;
	specializationInfo.dataSize = sizeof(bool);
	specializationInfo.pData = &renderSettings.shadowPCF;
*/
	
	{//base pipeline
		//flag that this pipeline will be used as a base
		pipeline_create_info.flags              = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
		pipeline_create_info.basePipelineIndex  = -1;
		
		shader_stages[0] = load_shader(STR8("base.vert"), baked_shader_base_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("base.frag"), baked_shader_base_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.base); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.base, "Base pipeline");
		
		//flag that all other pipelines are derivatives
		pipeline_create_info.flags              = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
		pipeline_create_info.basePipelineHandle = pipelines.base;
		pipeline_create_info.basePipelineIndex  = -1; //can either use handle or index, not both (section 9.5 of vulkan spec)
	}
	
	{//selected (base with no cull or depth test)
		rasterization_state.cullMode = VK_CULL_MODE_NONE;
		depth_stencil_state.depthTestEnable = VK_FALSE;
		
		shader_stages[1].pSpecializationInfo = &specialization_info;
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.selected); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.selected, "Selected pipeline");
		
		rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
		depth_stencil_state.depthTestEnable = VK_TRUE;
	}
	
	{//null pipeline
		shader_stages[0] = load_shader(STR8("null.vert"), baked_shader_null_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("null.frag"), baked_shader_null_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		shader_stages[1].pSpecializationInfo = &specialization_info;
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.null); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.null, "Null pipeline");
	}
	
	{//flat pipeline
		shader_stages[0] = load_shader(STR8("flat.vert"), baked_shader_flat_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("flat.frag"), baked_shader_flat_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		shader_stages[1].pSpecializationInfo = &specialization_info;
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.flat); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.flat, "Flat pipeline");
	}
	
	{//phong
		shader_stages[0] = load_shader(STR8("phong.vert"), baked_shader_phong_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("phong.frag"), baked_shader_phong_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		shader_stages[1].pSpecializationInfo = &specialization_info;
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.phong); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.phong, "Phong pipeline");
	}
	
	{//2d
		pipeline_create_info.pVertexInputState = &twod_vertex_input_state;
		pipeline_create_info.layout            = pipelineLayouts.twod;
		rasterization_state.cullMode  = VK_CULL_MODE_NONE;
		rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		depth_stencil_state.depthTestEnable = VK_FALSE;
		
		shader_stages[0] = load_shader(STR8("twod.vert"), baked_shader_twod_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("twod.frag"), baked_shader_twod_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		shader_stages[1].pSpecializationInfo = &specialization_info;
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.twod); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.twod, "2D pipeline");
		
		{//ui
			shader_stages[0] = load_shader(STR8("ui.vert"), baked_shader_ui_vert, VK_SHADER_STAGE_VERTEX_BIT);
			shader_stages[1] = load_shader(STR8("ui.frag"), baked_shader_ui_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
			shader_stages[1].pSpecializationInfo = &specialization_info;
			pipeline_create_info.stageCount = 2;
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.ui);
			DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.ui, "UI pipeline");
		}
		
		pipeline_create_info.pVertexInputState = &vertex_input_state;
		pipeline_create_info.layout            = pipelineLayouts.base;
		rasterization_state.cullMode  = VK_CULL_MODE_BACK_BIT;
		rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
		depth_stencil_state.depthTestEnable = VK_TRUE;
	}
	
	{//pbr
		shader_stages[0] = load_shader(STR8("pbr.vert"), baked_shader_pbr_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("pbr.frag"), baked_shader_pbr_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		shader_stages[1].pSpecializationInfo = &specialization_info;
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.pbr); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.pbr, "PBR pipeline");
	}
	
	//wireframe
	if(deviceFeatures.fillModeNonSolid){
		color_blend_attachment_state.blendEnable = VK_FALSE;
		rasterization_state.polygonMode = VK_POLYGON_MODE_LINE;
		rasterization_state.cullMode    = VK_CULL_MODE_NONE;
		depth_stencil_state.depthTestEnable = VK_FALSE;
		
		shader_stages[0] = load_shader(STR8("wireframe.vert"), baked_shader_wireframe_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("wireframe.frag"), baked_shader_wireframe_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.wireframe); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.wireframe, "Wireframe pipeline");
		
		{//wireframe with depth test
			depth_stencil_state.depthTestEnable = VK_TRUE;
			
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.wireframe_depth); AssertVk(resultVk);
			DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.wireframe_depth, "Wireframe Depth pipeline");
			
			depth_stencil_state.depthTestEnable = VK_FALSE;
		}
		
		{ //collider gets a specific colored wireframe
			color_blend_attachment_state.blendEnable         = VK_TRUE;
			color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
			color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
			
			color_blend_state.blendConstants[0] = (f32)renderSettings.colliderColor.r;
			color_blend_state.blendConstants[1] = (f32)renderSettings.colliderColor.g;
			color_blend_state.blendConstants[2] = (f32)renderSettings.colliderColor.b;
			color_blend_state.blendConstants[3] = (f32)renderSettings.colliderColor.a;
			
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.collider); AssertVk(resultVk);
			DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.collider, "Collider pipeline");
			
			color_blend_attachment_state.blendEnable         = VK_FALSE;
			color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			color_blend_state.blendConstants[0] = 0.f;
			color_blend_state.blendConstants[1] = 0.f;
			color_blend_state.blendConstants[2] = 0.f;
			color_blend_state.blendConstants[3] = 1.0f;
		}
		
		color_blend_attachment_state.blendEnable = VK_TRUE;
		rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_state.cullMode    = VK_CULL_MODE_BACK_BIT;
		depth_stencil_state.depthTestEnable = VK_TRUE;
	}
	
	{//offscreen
		color_blend_state.attachmentCount = 0; //no color attachments used
		depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; //cull front faces
		rasterization_state.depthBiasEnable = VK_TRUE; //enable depth bias
		multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_state.sampleShadingEnable  = VK_FALSE;
		dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS, };
		dynamic_state.dynamicStateCount = (u32)dynamic_states.count; //add depth bias to dynamic state so
		dynamic_state.pDynamicStates    = dynamic_states.data;       //it can be changed at runtime
		pipeline_create_info.renderPass = offscreen.renderpass;
		
		shader_stages[0] = load_shader(STR8("offscreen.vert"), baked_shader_offscreen_vert, VK_SHADER_STAGE_VERTEX_BIT);
		pipeline_create_info.stageCount = 1;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, allocator, &pipelines.offscreen); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.offscreen, "Offscreen pipeline");
		
		color_blend_state.attachmentCount = 1;
		depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		rasterization_state.depthBiasEnable = VK_FALSE;
		multisample_state.rasterizationSamples = msaaSamples;
		multisample_state.sampleShadingEnable  = (msaaSamples != VK_SAMPLE_COUNT_1_BIT) ? VK_TRUE : VK_FALSE;
		dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, };
		dynamic_state.dynamicStateCount = (u32)dynamic_states.count;
		dynamic_state.pDynamicStates    = dynamic_states.data;
		pipeline_create_info.renderPass = renderPass;
	}

	PrintVk(2, "Finished creating pipelines in ", peek_stopwatch(watch), "ms");
}

VkCompareOp
render_compare_op_to_vulkan(RenderCompareOp x) {
	switch(x) {
		case RenderCompareOp_Never: return VK_COMPARE_OP_NEVER;
		case RenderCompareOp_Less: return VK_COMPARE_OP_LESS;
		case RenderCompareOp_Equal: return VK_COMPARE_OP_EQUAL;
		case RenderCompareOp_Less_Or_Equal: return VK_COMPARE_OP_LESS_OR_EQUAL;
		case RenderCompareOp_Greater: return VK_COMPARE_OP_GREATER;
		case RenderCompareOp_Not_Equal: return VK_COMPARE_OP_NOT_EQUAL;
		case RenderCompareOp_Greater_Or_Equal: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case RenderCompareOp_Always: return VK_COMPARE_OP_ALWAYS;
	}
}

VkShaderStageFlagBits
render_shader_kind_to_vulkan(RenderShaderKind x) {
	VkShaderStageFlags out = 0;
	if(HasFlag(x, RenderShaderKind_Vertex))   AddFlag(out, VK_SHADER_STAGE_VERTEX_BIT);
	if(HasFlag(x, RenderShaderKind_Fragment)) AddFlag(out, VK_SHADER_STAGE_FRAGMENT_BIT);
	if(HasFlag(x, RenderShaderKind_Geometry)) AddFlag(out, VK_SHADER_STAGE_GEOMETRY_BIT);
	if(HasFlag(x, RenderShaderKind_Compute))  AddFlag(out, VK_SHADER_STAGE_COMPUTE_BIT);
	return (VkShaderStageFlagBits)out;
}

VkDescriptorType
render_descriptor_kind_to_vulkan(RenderDescriptorKind x) {
	switch(x) {
		case RenderDescriptorKind_Uniform_Buffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case RenderDescriptorKind_Combined_Image_Sampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	}
}

RenderPipelineLayout*
render_create_pipeline_layout() {
	auto out = memory_pool_push(__render_pool_descriptor_set_layout);
	array_init(out->push_constants, 1, deshi_allocator);
	array_init(out->descriptors, 1, deshi_allocator);
	return out;
}

RenderPipelineLayout*
render_create_base_pipeline_layout() {
	auto pl = render_create_pipeline_layout();
	
	auto d = array_push(pl->descriptors);
	d->shader_stage_flags = RenderShaderKind_Vertex;
	d->              kind = RenderDescriptorKind_Uniform_Buffer;

	forI(5) {
		d = array_push(pl->descriptors);
		d->shader_stage_flags = RenderShaderKind_Fragment;
		d->              kind = RenderDescriptorKind_Combined_Image_Sampler;
	}

	auto p = array_push(pl->push_constants);
	p->shader_stage_flags = RenderShaderKind_Vertex;
	p->              size = sizeof(mat4);

	render_update_pipeline_layout(pl);

	return pl;
}

void
render_update_pipeline_layout(RenderPipelineLayout* x) {
	PrintVk(2, "Updating descriptor set layout ", x->name);

	Stopwatch watch = start_stopwatch();
	
	// destroy any possibly existing layout
	vkDestroyPipelineLayout(device, (VkPipelineLayout)x->handle, allocator);

	u64 n_bindings = array_count(x->descriptors);

	VkDescriptorSetLayoutBinding* bindings;
	array_init(bindings, n_bindings, deshi_temp_allocator);
	array_count(bindings) = n_bindings;

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	descriptor_set_layout_info.pBindings = bindings;
	descriptor_set_layout_info.bindingCount = n_bindings;

	forI(n_bindings) {
		bindings[i].descriptorType = render_descriptor_kind_to_vulkan(x->descriptors[i].kind);
		bindings[i].stageFlags = render_shader_kind_to_vulkan(x->descriptors[i].shader_stage_flags);
		bindings[i].binding = i;
		bindings[i].descriptorCount = 1;
	}

	VkDescriptorSetLayout descriptor_set_layout;
	resultVk = vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, allocator, &descriptor_set_layout);
	AssertVk(resultVk);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)descriptor_set_layout, 
			(char*)to_dstr8v(deshi_temp_allocator, x->name, " descriptor set layout").str);
	
	u64 n_push_constants = array_count(x->push_constants);

	VkPushConstantRange* ranges;
	array_init(ranges, n_push_constants, deshi_temp_allocator);
	array_count(ranges) = n_push_constants;
	
	forI(n_push_constants) {
		ranges[i].stageFlags = render_shader_kind_to_vulkan(x->push_constants[i].shader_stage_flags);
		ranges[i].offset = x->push_constants[i].offset;
		ranges[i].size = x->push_constants[i].size;
	}

	VkPipelineLayoutCreateInfo pipeline_layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = n_push_constants;
	pipeline_layout_info.pPushConstantRanges = ranges;
	resultVk = vkCreatePipelineLayout(device, &pipeline_layout_info, allocator, (VkPipelineLayout*)&x->handle);
	AssertVk(resultVk);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)x->handle, 
			(char*)to_dstr8v(deshi_temp_allocator, x->name, " pipeline layout").str);

	PrintVk(2, "Finished updating ", x->name, " layout in ", peek_stopwatch(watch), "ms");
}

// creates a pipeline and returns a handle to it
// the user is expected to modify this handle then call render_update_pipeline
// later
RenderPipeline*
render_create_pipeline() {
	auto rp = memory_pool_push(__render_pipeline_pool);
	array_init(rp->shader_stages, 1, deshi_allocator);
	array_init(rp->dynamic_states, 1, deshi_allocator);
	return rp;
}

RenderPipeline*
render_create_default_pipeline() {
	RenderPipeline* p = render_create_pipeline();
	
	p->         front_face = RenderPipelineFrontFace_CW;
	p->            culling = RenderPipelineCulling_Back;
	p->       polygon_mode = RenderPipelinePolygonMode_Fill;
	p->         depth_test = true;
	p->   depth_compare_op = RenderCompareOp_Less_Or_Equal;
	p->         depth_bias = false;
	p->depth_bias_constant = 0.f;
	p->   depth_bias_clamp = 0.f;
	p->   depth_bias_slope = 0.f;
	p->         line_width = 1.f;

	// TODO(sushi) msaa
	
	p->           color_blend = true;
	p->        color_blend_op = RenderBlendOp_Add;
	p->color_src_blend_factor = RenderBlendFactor_Source_Alpha;
	p->color_dst_blend_factor = RenderBlendFactor_One_Minus_Source_Alpha;
	p->        alpha_blend_op = RenderBlendOp_Add;
	p->alpha_src_blend_factor = RenderBlendFactor_One_Minus_Source_Alpha;
	p->alpha_dst_blend_factor = RenderBlendFactor_Zero;

	p->blend_constant = color(0,0,0,0);

	*array_push(p->dynamic_states) = RenderDynamicState_Viewport;
	*array_push(p->dynamic_states) = RenderDynamicState_Scissor;

	p->layout = render_create_base_pipeline_layout();

	render_update_pipeline(p);

	return p;
}

void
render_update_pipeline(RenderPipeline* pipeline) {
	PrintVk(2, "Updating ", pipeline->name, " pipeline");

	Stopwatch watch = start_stopwatch();

	u64 n_stages = array_count(pipeline->shader_stages);

	VkPipelineShaderStageCreateInfo* shader_stages;
	array_init(shader_stages, n_stages, deshi_temp_allocator);
	forI(n_stages) {
		*array_push(shader_stages) = load_shader(
				pipeline->shader_stages[i].name, 
				pipeline->shader_stages[i].source, 
				render_shader_kind_to_vulkan(pipeline->shader_stages[i].kind));
	}
	
    auto ias = input_assembly_state;

    auto rs = rasterization_state;
	switch(pipeline->culling) {
		case RenderPipelineCulling_None: rs.cullMode = VK_CULL_MODE_NONE; break;
		case RenderPipelineCulling_Back: rs.cullMode = VK_CULL_MODE_BACK_BIT; break;
		case RenderPipelineCulling_Front: rs.cullMode = VK_CULL_MODE_FRONT_BIT; break;
		case RenderPipelineCulling_Front_Back: rs.cullMode = VK_CULL_MODE_FRONT_AND_BACK; break;
	}
	switch(pipeline->front_face) {
		case RenderPipelineFrontFace_CW: rs.frontFace = VK_FRONT_FACE_CLOCKWISE; break;
		case RenderPipelineFrontFace_CCW: rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; break;
	}
	switch(pipeline->polygon_mode) {
		case RenderPipelinePolygonMode_Point: rs.polygonMode = VK_POLYGON_MODE_POINT; break;
		case RenderPipelinePolygonMode_Fill: rs.polygonMode = VK_POLYGON_MODE_FILL; break;
		case RenderPipelinePolygonMode_Line: rs.polygonMode = VK_POLYGON_MODE_LINE; break;
	}
	rs.depthBiasEnable = (pipeline->depth_bias? VK_TRUE : VK_FALSE);

    auto cbas = color_blend_attachment_state;
	cbas.blendEnable = (pipeline->color_blend? VK_TRUE : VK_FALSE);
	switch(pipeline->color_src_blend_factor) {
		case RenderBlendFactor_Zero: cbas.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO; break;
		case RenderBlendFactor_One: cbas.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; break;
		case RenderBlendFactor_Source_Color: cbas.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR; break;
		case RenderBlendFactor_One_Minus_Source_Color: cbas.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR; break;
		case RenderBlendFactor_Destination_Color: cbas.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR; break;
		case RenderBlendFactor_One_Minus_Destination_Color: cbas.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR; break;
		case RenderBlendFactor_Source_Alpha: cbas.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; break;
		case RenderBlendFactor_One_Minus_Source_Alpha: cbas.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; break;
		case RenderBlendFactor_Destination_Alpha: cbas.srcColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA; break;
		case RenderBlendFactor_One_Minus_Destination_Alpha: cbas.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA; break;
		case RenderBlendFactor_Constant_Color: cbas.srcColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR; break;
		case RenderBlendFactor_One_Minus_Constant_Color: cbas.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR; break;
		case RenderBlendFactor_Constant_Alpha: cbas.srcColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_ALPHA; break;
		case RenderBlendFactor_One_Minus_Constant_Alpha: cbas.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA; break;
	}
	switch(pipeline->alpha_src_blend_factor) {
		case RenderBlendFactor_Zero: cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; break;
		case RenderBlendFactor_One: cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; break;
		case RenderBlendFactor_Source_Color: cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_COLOR; break;
		case RenderBlendFactor_One_Minus_Source_Color: cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR; break;
		case RenderBlendFactor_Destination_Color: cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_DST_COLOR; break;
		case RenderBlendFactor_One_Minus_Destination_Color: cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR; break;
		case RenderBlendFactor_Source_Alpha: cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; break;
		case RenderBlendFactor_One_Minus_Source_Alpha: cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; break;
		case RenderBlendFactor_Destination_Alpha: cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA; break;
		case RenderBlendFactor_One_Minus_Destination_Alpha: cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA; break;
		case RenderBlendFactor_Constant_Color: cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR; break;
		case RenderBlendFactor_One_Minus_Constant_Color: cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR; break;
		case RenderBlendFactor_Constant_Alpha: cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_ALPHA; break;
		case RenderBlendFactor_One_Minus_Constant_Alpha: cbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA; break;
	}

    auto cbs = color_blend_state;
	cbs.blendConstants[0] = pipeline->blend_constant.r/255.f;
	cbs.blendConstants[1] = pipeline->blend_constant.g/255.f;
	cbs.blendConstants[2] = pipeline->blend_constant.b/255.f;
	cbs.blendConstants[3] = pipeline->blend_constant.a/255.f;
	// TODO(sushi) setup a flag to disable this 
	cbs.attachmentCount = 1;

    auto dss  = depth_stencil_state;
	dss.depthTestEnable = (pipeline->depth_test? VK_TRUE : VK_FALSE);
	// TODO(sushi) this will not be set to the default if the user does not set this variable
	//             setup new pipelines to take on all the default values we use in setup_pipeline_creation
	dss.depthCompareOp = render_compare_op_to_vulkan(pipeline->depth_compare_op);

    auto vs   = viewport_state;
    auto ms   = multisample_state;
	// TODO(sushi) multisample settings
    auto vis  = vertex_input_state;
    auto tvis = twod_vertex_input_state;

    auto ds = dynamic_state;
	VkDynamicState* dynamic_states;
	array_init(dynamic_states, array_count(pipeline->dynamic_states), deshi_temp_allocator);
	forI(array_count(pipeline->dynamic_states)) {
		switch(pipeline->dynamic_states[i]) {
			case RenderDynamicState_Viewport: array_push_value(dynamic_states, VK_DYNAMIC_STATE_VIEWPORT); break;
			case RenderDynamicState_Scissor: array_push_value(dynamic_states, VK_DYNAMIC_STATE_SCISSOR); break;
			case RenderDynamicState_Line_Width: array_push_value(dynamic_states, VK_DYNAMIC_STATE_LINE_WIDTH); break;
			case RenderDynamicState_Depth_Bias: array_push_value(dynamic_states, VK_DYNAMIC_STATE_DEPTH_BIAS); break;
			case RenderDynamicState_Blend_Constants: array_push_value(dynamic_states, VK_DYNAMIC_STATE_BLEND_CONSTANTS); break;
			case RenderDynamicState_Depth_Bounds: array_push_value(dynamic_states, VK_DYNAMIC_STATE_DEPTH_BOUNDS); break;
		}
	}
	ds.pDynamicStates = dynamic_states;
	ds.dynamicStateCount = array_count(pipeline->dynamic_states);

	

	VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	info.stageCount          = array_count(pipeline->shader_stages);
	info.pStages             = shader_stages;
	info.pVertexInputState   = &vis;
	info.pInputAssemblyState = &ias;
	info.pTessellationState  = 0;
	info.pViewportState      = &vs;
	info.pRasterizationState = &rs;
	info.pMultisampleState   = &ms;
	info.pDepthStencilState  = &dss;
	info.pColorBlendState    = &cbs;
	info.pDynamicState       = &ds;
	info.layout              = (VkPipelineLayout)pipeline->layout->handle;

	resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &info, allocator, (VkPipeline*)&pipeline->handle);
	Assert(resultVk);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipeline->handle, 
			(char*)to_dstr8v(deshi_temp_allocator, pipeline->name, " pipeline").str);

	PrintVk(2, "Finished creating ", pipeline->name, " pipeline in ", peek_stopwatch(watch), "ms");
}

RenderImage*
render_create_image() {
	auto out = memory_pool_push(__render_pool_images);
	return out;
}

VkImageType
render_image_type_to_vulkan(RenderImageType x) {
	switch(x) {
		case RenderImageType_OneD:   return VK_IMAGE_TYPE_1D;
		case RenderImageType_TwoD:   return VK_IMAGE_TYPE_2D;
		case RenderImageType_ThreeD: return VK_IMAGE_TYPE_3D;
	}
	Assert(0);
}

VkImageUsageFlags
render_image_usage_to_vulkan(RenderImageUsage x) {
	VkImageUsageFlags out;
	if(HasFlag(x, RenderImageUsage_Transfer_Source)) AddFlag(out, VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	if(HasFlag(x, RenderImageUsage_Transfer_Destination)) AddFlag(out, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	if(HasFlag(x, RenderImageUsage_Sampled)) AddFlag(out, VK_IMAGE_USAGE_SAMPLED_BIT);
	if(HasFlag(x, RenderImageUsage_Storage)) AddFlag(out, VK_IMAGE_USAGE_STORAGE_BIT);
	if(HasFlag(x, RenderImageUsage_Color_Attachment)) AddFlag(out, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	if(HasFlag(x, RenderImageUsage_Depth_Stencil_Attachment)) AddFlag(out, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	return out;
}

VkSampleCountFlags
render_sample_count_to_vulkan(RenderSampleCount x) {
	VkSampleCountFlags out;
	if(HasFlag(x, RenderSampleCount_1)) AddFlag(out, VK_SAMPLE_COUNT_1_BIT);
	if(HasFlag(x, RenderSampleCount_2)) AddFlag(out, VK_SAMPLE_COUNT_2_BIT);
	if(HasFlag(x, RenderSampleCount_4)) AddFlag(out, VK_SAMPLE_COUNT_4_BIT);
	if(HasFlag(x, RenderSampleCount_8)) AddFlag(out, VK_SAMPLE_COUNT_8_BIT);
	if(HasFlag(x, RenderSampleCount_16)) AddFlag(out, VK_SAMPLE_COUNT_16_BIT);
	if(HasFlag(x, RenderSampleCount_32)) AddFlag(out, VK_SAMPLE_COUNT_32_BIT);
	if(HasFlag(x, RenderSampleCount_64)) AddFlag(out, VK_SAMPLE_COUNT_64_BIT);
	return out;
}

VkFormat
render_format_to_vulkan(RenderFormat x) {
	switch(x) {
		case RenderFormat_R8G8B8_StandardRGB:                              return VK_FORMAT_R8G8B8_SRGB;
		case RenderFormat_Depth16_UnsignedNoramlized:                      return VK_FORMAT_D16_UNORM;
		case RenderFormat_Depth32_SignedFloat:                             return VK_FORMAT_D32_SFLOAT;
		case RenderFormat_Depth32_SignedFloat_Stencil8_UnsignedInt:        return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case RenderFormat_Depth24_UnsignedNormalized_Stencil8_UnsignedInt: return VK_FORMAT_D24_UNORM_S8_UINT;
	}
	Assert(0);
}

VkMemoryPropertyFlags
render_memory_properties_to_vulkan(RenderMemoryPropertyFlags x) {
	VkMemoryPropertyFlags out;
	if(HasFlag(x, RenderMemoryPropertyFlag_DeviceLocal     
	if(HasFlag(x, RenderMemoryPropertyFlag_HostVisible     
	if(HasFlag(x, RenderMemoryPropertyFlag_HostCoherent    
	if(HasFlag(x, RenderMemoryPropertyFlag_HostCached      
	if(HasFlag(x, RenderMemoryPropertyFlag_LazilyAllocated 
}

void
render_update_image(RenderImage* x) {
	PrintVk(4, "Updating RenderImage");

	vkDestroyImage(device, (VkImage)x->handle, allocator);
	
	create_image(
			x->extent.x, x->extent.y, 
			1, 
			render_sample_count_to_vulkan(x->samples),
			render_format_to_vulkan(x->format),
			(x->linear_tiling? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL),
			render_image_usage_to_vulkan(x->usage),
			
			)

}

RenderPass*
render_create_render_pass() {
	auto out = memory_pool_push(__render_pool_render_passes);
	return out;
}

void 
render_update_render_pass(RenderPass* x) {
	
}

void
create_swapchain(Window* window) {
	PrintVk(2, "Creating swapchain for window ", window->title);
	Assert(device != VK_NULL_HANDLE, "create_swapchain called before logical device creation");

	Stopwatch watch = start_stopwatch();
	auto wininf = (VkWindowInfo*)window->render_info;

	auto old_swapchain = wininf->swapchain;
	wininf->swapchain = VK_NULL_HANDLE;

	vkDeviceWaitIdle(device);

	wininf->width = window->width;
	wininf->height = window->height;

	// check GPU's capabilities for the new swapchain
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, wininf->surface, &wininf->support_details.capabilities);

	u32 format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, wininf->surface, &format_count, 0);
	if(format_count) {
		array_grow(wininf->support_details.formats, format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, wininf->surface, &format_count, wininf->support_details.formats);
	}

	u32 present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, wininf->surface, &present_mode_count, 0);
	if(present_mode_count) {
		array_grow(wininf->support_details.present_modes, present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, wininf->surface, &present_mode_count, wininf->support_details.present_modes);
	}
	
	// choose swapchain's surface format 
	wininf->surface_format = wininf->support_details.formats[0];
	for_array(wininf->support_details.formats) {
		auto format = *it;
		if( format.format == VK_FORMAT_B8G8R8A8_SRGB &&
			format.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) {
			wininf->surface_format = format;
			break;
		}
	}

	// choose swapchain's present mode
	b32 immediate = false;
	b32 fifo_relaxed = false;
	b32 mailbox = false;

	for_array(wininf->support_details.present_modes) {
		auto pm = *it;
		if(pm == VK_PRESENT_MODE_IMMEDIATE_KHR)    immediate = true;
		if(pm == VK_PRESENT_MODE_MAILBOX_KHR)      mailbox = true;
		if(pm == VK_PRESENT_MODE_FIFO_RELAXED_KHR) fifo_relaxed = true;
	}

	if(immediate) {
		renderSettings.vsync = VSyncType_Immediate;
		wininf->present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	} else if(mailbox) {
		renderSettings.vsync = VSyncType_Mailbox;
		wininf->present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
	} else if(fifo_relaxed) {
		renderSettings.vsync = VSyncType_FifoRelaxed;
		wininf->present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	} else {
		renderSettings.vsync = VSyncType_Fifo;
		wininf->present_mode = VK_PRESENT_MODE_FIFO_KHR;
	}

	// find actual extent of the swapchain
	auto capabilities = wininf->support_details.capabilities;
	if(capabilities.currentExtent.width != UINT32_MAX) {
		wininf->extent = capabilities.currentExtent;
	} else {
		wininf->extent = { (u32)wininf->width, (u32)wininf->height };
		wininf->extent.width  = Max(capabilities.minImageExtent.width,  Min(capabilities.maxImageExtent.width,  wininf->extent.width));
		wininf->extent.height = Max(capabilities.minImageExtent.height, Min(capabilities.maxImageExtent.height, wininf->extent.height));
	}

	// get min image count if not specified
	if(!wininf->min_image_count) {
		switch(wininf->present_mode) {
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
			case VK_PRESENT_MODE_MAILBOX_KHR: 
			case VK_PRESENT_MODE_FIFO_KHR: {
				wininf->min_image_count = 2;
			} break;
			case VK_PRESENT_MODE_IMMEDIATE_KHR: {
				wininf->min_image_count = 1;
			} break;
			default: {
				wininf->min_image_count = -1;
			} break;
		}
	}

	u32 queue_family_indices[2] = {
		physical_queue_families.graphics_family,
		physical_queue_families.present_family,
	};

	// create swapchain aand swapchain images, set width and height
	VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	info.surface                   = wininf->surface;
	info.imageFormat               = wininf->surface_format.format;
	info.imageColorSpace           = wininf->surface_format.colorSpace;
	info.imageArrayLayers          = 1;
	info.imageUsage                = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if(physical_queue_families.graphics_family != physical_queue_families.present_family) {
		info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices   = queue_family_indices;
	} else {
		info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
		info.queueFamilyIndexCount = 0;
		info.pQueueFamilyIndices   = 0;
	}
	info.preTransform              = wininf->support_details.capabilities.currentTransform;
	info.compositeAlpha            = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode               = wininf->present_mode;
	info.clipped                   = VK_TRUE;
	info.oldSwapchain              = old_swapchain;
	info.minImageCount             = wininf->min_image_count;
	if(wininf->support_details.capabilities.maxImageCount && info.minImageCount > wininf->support_details.capabilities.maxImageCount) {
		info.minImageCount         = wininf->support_details.capabilities.maxImageCount;
	}
	if(wininf->extent.width == UINT32_MAX) {
		info.imageExtent.width     = wininf->width;
		info.imageExtent.height    = wininf->height;
	} else {
		info.imageExtent.width     = wininf->width  = wininf->extent.width;
		info.imageExtent.height    = wininf->height = wininf->extent.height;
	}

	resultVk = vkCreateSwapchainKHR(device, &info, allocator, &wininf->swapchain);
	Assert(resultVk);

	if(old_swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, old_swapchain, allocator);

	PrintVk(2, "Finished creating swapchain in ", peek_stopwatch(watch), "ms");
}

VkFormat
find_supported_format(VkFormat* formats, u64 format_count, VkImageTiling tiling, VkFormatFeatureFlags features) {
	PrintVk(4, "Finding supported image formats");
	forI(format_count){
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physical_device, formats[i], &props);
		if      (tiling == VK_IMAGE_TILING_LINEAR  && (props.linearTilingFeatures  & features) == features){
			return formats[i];
		}else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features){
			return formats[i];
		}
	}
	
	Assert(!"failed to find supported format");
	return VK_FORMAT_UNDEFINED;
}

VkFormat
find_depth_format() {
	VkFormat depth_formats[] = { 
		VK_FORMAT_D32_SFLOAT, 
		VK_FORMAT_D32_SFLOAT_S8_UINT, 
		VK_FORMAT_D24_UNORM_S8_UINT, 
	};
	return find_supported_format(depth_formats, ArrayCount(depth_formats),
							   VK_IMAGE_TILING_OPTIMAL, 
							   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void
create_renderpasses(Window* window) {
	PrintVk(2, "Creating renderpasses for ", window->title);
	
	Stopwatch watch = start_stopwatch();

	auto wininf = (VkWindowInfo*)window->render_info;

	if(baseRenderPass) vkDestroyRenderPass(device, baseRenderPass, allocator);
	if(msaaRenderPass) vkDestroyRenderPass(device, msaaRenderPass, allocator);

	VkAttachmentDescription attachments[3]{};
	//attachment 0: color
	attachments[0].format         = wininf->surface_format.format;
	attachments[0].samples        = msaaSamples;
	attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	//attachment 1: depth
	attachments[1].format         = find_depth_format();
	attachments[1].samples        = msaaSamples;
	attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	//attachment 2: color resolve
	attachments[2].format         = wininf->surface_format.format;
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
		
		resultVk = vkCreateRenderPass(device, &renderPassInfo, allocator, &msaaRenderPass); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_RENDER_PASS, (u64)msaaRenderPass, "MSAA render pass");
		
		renderPass = msaaRenderPass;
	}else{
		resultVk = vkCreateRenderPass(device, &renderPassInfo, allocator, &baseRenderPass); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_RENDER_PASS, (u64)baseRenderPass, "Base render pass");
		
		renderPass = baseRenderPass;
	}

	PrintVk(2, "Finished creating renderpasses in ", peek_stopwatch(watch), "ms");
}

void
create_frames(Window* window) {
	PrintVk(2, "Creating frames for ", window->title);

	Stopwatch watch = start_stopwatch();

	auto wininf = (VkWindowInfo*)window->render_info;

	vkGetSwapchainImagesKHR(device, wininf->swapchain, &wininf->image_count, 0);
	// TODO(sushi) consider loosening the second restriction
	Assert(wininf->image_count >= wininf->min_image_count, "the window should always have at least the min image count");
	Assert(wininf->image_count < 16, "the window should have less than 16 images, around 2-3 is ideal");
	VkImage images[16] = {};
	vkGetSwapchainImagesKHR(device, wininf->swapchain, &wininf->image_count, images);

	auto attachments = wininf->attachments;

	// color framebuffer attachment
	if(wininf->attachments.color_image) {
		vkDestroyImageView(device, attachments.color_image_view, allocator);
		vkDestroyImage(device, attachments.color_image, allocator);
		vkFreeMemory(device, attachments.color_image_memory, allocator);
	}
	create_image(
			wininf->width, wininf->height, 1, 
			msaaSamples, 
			wininf->surface_format.format,
			VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&wininf->attachments.color_image,
			&wininf->attachments.color_image_memory);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE, (u64)wininf->attachments.color_image, "Framebuffer color image");
	wininf->attachments.color_image_view = 
		create_image_view(wininf->attachments.color_image, wininf->surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)wininf->attachments.color_image_view, "Framebuffer color imageview");

	
	// depth framebuffer attachment
	if(wininf->attachments.depth_image){
		vkDestroyImageView(device, wininf->attachments.depth_image_view, allocator);
		vkDestroyImage(device, wininf->attachments.depth_image, allocator);
		vkFreeMemory(device, wininf->attachments.depth_image_memory, allocator);
	}
	VkFormat depthFormat = findDepthFormat();
	create_image(
			wininf->width, wininf->height, 1, 
			msaaSamples, 
			depthFormat, 
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			&wininf->attachments.depth_image, 
			&wininf->attachments.depth_image_memory);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE, (u64)wininf->attachments.depth_image, "Framebuffer depth image");
	wininf->attachments.depth_image_view = 
		create_image_view(wininf->attachments.depth_image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)wininf->attachments.depth_image_view, "Framebuffer depth imageview");


	array_grow(wininf->frames, wininf->image_count);
	array_count(wininf->frames) = wininf->image_count;
	for(u32 i = 0; i < wininf->image_count; ++i){
		//set the frame images to the swap chain images
		//NOTE the previous image and its memory gets freed when the swapchain gets destroyed
		wininf->frames[i].image = images[i];
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE, (u64)wininf->frames[i].image, (const char*)to_dstr8v(deshi_temp_allocator, "Frame image ", i).str);
		
		//create the image views
		if(wininf->frames[i].imageView) vkDestroyImageView(device, wininf->frames[i].imageView, allocator);
		wininf->frames[i].imageView = create_image_view(wininf->frames[i].image, wininf->surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)wininf->frames[i].imageView, (const char*)to_dstr8v(deshi_temp_allocator, "Frame imageview ", i).str);
		
		//create the framebuffers
		if(wininf->frames[i].framebuffer) vkDestroyFramebuffer(device, wininf->frames[i].framebuffer, allocator);
		
		arrayT<VkImageView> frame_buffer_attachments(deshi_temp_allocator); //TODO(delle) fix scuffed msaa hack
		if(msaaSamples != VK_SAMPLE_COUNT_1_BIT){
			frame_buffer_attachments = { wininf->attachments.color_image_view, wininf->attachments.depth_image_view, wininf->frames[i].imageView };
		}else{
			frame_buffer_attachments = { wininf->frames[i].imageView, wininf->attachments.depth_image_view, };
		}
		
		VkFramebufferCreateInfo info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		info.renderPass      = renderPass;
		info.attachmentCount = frame_buffer_attachments.count;
		info.pAttachments    = frame_buffer_attachments.data;
		info.width           = wininf->width;
		info.height          = wininf->height;
		info.layers          = 1;
		resultVk = vkCreateFramebuffer(device, &info, allocator, &wininf->frames[i].framebuffer); AssertVk(resultVk, "failed to create framebuffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_FRAMEBUFFER, (u64)wininf->frames[i].framebuffer, (const char*)to_dstr8v(deshi_temp_allocator, "Frame framebuffer ", i).str);
		
		//allocate command buffers
		if(wininf->frames[i].commandBuffer) vkFreeCommandBuffers(device, commandPool, 1, &wininf->frames[i].commandBuffer);
		VkCommandBufferAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		alloc_info.commandPool = commandPool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = 1;
		resultVk = vkAllocateCommandBuffers(device, &alloc_info, &wininf->frames[i].commandBuffer); AssertVk(resultVk, "failed to allocate command buffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (u64)wininf->frames[i].commandBuffer, (const char*)to_dstr8v(deshi_temp_allocator, "Frame command buffer ", i).str);
	}

	PrintVk(2, "Finished creating frames in ", peek_stopwatch(watch), "ms");
}

void
create_sync_objects() {
	PrintVk(2, "Creating sync objects");

	Stopwatch watch = start_stopwatch();

	VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

	resultVk = vkCreateSemaphore(device, &semaphore_info, allocator, &imageAcquiredSemaphore);
	AssertVk(resultVk);
	resultVk = vkCreateSemaphore(device, &semaphore_info, allocator, &renderCompleteSemaphore);
	AssertVk(resultVk);

	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SEMAPHORE, (u64)imageAcquiredSemaphore, "Semaphore image acquired");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SEMAPHORE, (u64)renderCompleteSemaphore, "Semaphore render complete");

	PrintVk(2, "Finished creating sync objects in ", peek_stopwatch(watch), "ms");
}

void
create_descriptor_sets() {
	PrintVk(2, "Creating descriptor sets");

	Stopwatch watch = start_stopwatch();

	VkDescriptorSetAllocateInfo alloc_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	alloc_info.descriptorPool     = descriptorPool;
	alloc_info.pSetLayouts        = &descriptorSetLayouts.base;
	alloc_info.descriptorSetCount = 1;

	VkWriteDescriptorSet write_descriptor_sets[2]{};

	// base descriptor sets
	resultVk = vkAllocateDescriptorSets(device, &alloc_info, &descriptorSets.base); AssertVk(resultVk);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)descriptorSets.base, "Base descriptor set");

	// binding 0: vertex shader ubo
	write_descriptor_sets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[0].dstSet          = descriptorSets.base;
	write_descriptor_sets[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_sets[0].dstBinding      = 0;
	write_descriptor_sets[0].pBufferInfo     = &uboVS.bufferDescriptor;
	write_descriptor_sets[0].descriptorCount = 1;
	// binding 1: fragment shader shadow sampler
	write_descriptor_sets[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[1].dstSet          = descriptorSets.base;
	write_descriptor_sets[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_descriptor_sets[1].dstBinding      = 1;
	write_descriptor_sets[1].pImageInfo      = &offscreen.depthDescriptor;
	write_descriptor_sets[1].descriptorCount = 1;

	vkUpdateDescriptorSets(device, 2, write_descriptor_sets, 0, 0);

	// offscreen shadow map generation descriptor set
	resultVk = vkAllocateDescriptorSets(device, &alloc_info, &descriptorSets.offscreen); AssertVk(resultVk);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)descriptorSets.offscreen, "Offscreen descriptor set");

	// binding 0: vertex shader ubo
	write_descriptor_sets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[0].dstSet          = descriptorSets.offscreen;
	write_descriptor_sets[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_sets[0].dstBinding      = 0;
	write_descriptor_sets[0].pBufferInfo     = &uboVSoffscreen.bufferDescriptor;
	write_descriptor_sets[0].descriptorCount = 1;

	vkUpdateDescriptorSets(device, 1, write_descriptor_sets, 0, 0);

	// geometry descriptor sets
	if(enabledFeatures.geometryShader){
		alloc_info.pSetLayouts = &descriptorSetLayouts.geometry;
		resultVk = vkAllocateDescriptorSets(device, &alloc_info, &descriptorSets.geometry); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)descriptorSets.geometry, "Geometry descriptor set");
		
		// binding 0: vertex shader ubo
		write_descriptor_sets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_descriptor_sets[0].dstSet          = descriptorSets.geometry;
		write_descriptor_sets[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write_descriptor_sets[0].dstBinding      = 0;
		write_descriptor_sets[0].pBufferInfo     = &uboVS.bufferDescriptor;
		write_descriptor_sets[0].descriptorCount = 1;
		// binding 1: geometry shader ubo
		write_descriptor_sets[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_descriptor_sets[1].dstSet          = descriptorSets.geometry;
		write_descriptor_sets[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write_descriptor_sets[1].dstBinding      = 1;
		write_descriptor_sets[1].pBufferInfo     = &uboGS.bufferDescriptor;
		write_descriptor_sets[1].descriptorCount = 1;
		
		vkUpdateDescriptorSets(device, 2, write_descriptor_sets, 0, 0);
		alloc_info.pSetLayouts = &descriptorSetLayouts.base;
	}

	resultVk = vkAllocateDescriptorSets(device, &alloc_info, &descriptorSets.shadowMap_debug); AssertVk(resultVk);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)descriptorSets.shadowMap_debug, "DEBUG Shadowmap descriptor set");
	
	// binding 1: fragment shader shadow sampler
	write_descriptor_sets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[0].dstSet          = descriptorSets.shadowMap_debug;
	write_descriptor_sets[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_descriptor_sets[0].dstBinding      = 1;
	write_descriptor_sets[0].pImageInfo      = &offscreen.depthDescriptor;
	write_descriptor_sets[0].descriptorCount = 1;
	
	vkUpdateDescriptorSets(device, 1, write_descriptor_sets, 0, 0);

	PrintVk(2, "Finished creating descriptor sets in ", peek_stopwatch(watch), "ms");
}


void
render_init_x(Window* window) {
	Log("vulkan","Initializing vulkan renderer");
	
	Stopwatch watch = start_stopwatch();
	
	array_init(validation_features_enabled_x, 1, deshi_allocator);
	memory_pool_init(window_infos, 4);

	//create the shaders directory if it doesn't exist already
	file_create(str8_lit("data/shaders/"));
	
	//// load RenderSettings ////
	render_load_settings();
	if(renderSettings.debugging && renderSettings.printf){
		array_push_value(validation_features_enabled_x, VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
		renderSettings.loggingLevel = 4;
	}

	if(window) {
		// create the window info that this window will point to
		// TODO(sushi) user should be able to control how this is allocated 
		//             and default should probably be 
		auto wi = (VkWindowInfo*)(window->render_info = memory_pool_push(window_infos));
	}

	// mostly vulkan-os interaction setup
	setup_allocator();
	create_instance(window);
	if(renderSettings.debugging)
		setup_debug_messenger();
	if(window)
		create_surface(window);
	pick_physical_device(window);
	create_logical_device(window);

	// setup static vulkan objects
	create_command_pool();
	create_uniform_buffers();
	setup_shader_compiler();
	create_layouts();
	create_descriptor_pool();
	
	if(window) {
		create_swapchain(window);
		create_renderpasses(window);
		create_frames(window);
	}

	create_sync_objects();
	create_descriptor_sets();
	create_pipeline_cache();
	setup_pipeline_creation();
	create_pipelines();

	// init shared render
	memory_pool_init(deshi__render_buffer_pool, 16);
	externalVertexBuffers = memory_create_arena(sizeof(BufferVk)*MAX_EXTERNAL_BUFFERS);
	externalIndexBuffers = memory_create_arena(sizeof(BufferVk)*MAX_EXTERNAL_BUFFERS);

	initialized = true;
	


	Log("vulkan", "Finished vulkan renderer initialization in ",peek_stopwatch(watch), "ms");
}

void
setup_commands() {

}

void 
render_update_x(Window* window) {
	Stopwatch watch = start_stopwatch();
	auto wininf = (VkWindowInfo*)window->render_info;

	if(window->resized || remakeWindow) {
		wininf->width = window->width;
		wininf->height = window->height;
		if(wininf->width <= 0 || wininf->height <= 0) return;
		vkDeviceWaitIdle(device);
		create_swapchain(window);
		create_frames(window);
		wininf->frame_index = 0;
		remakeWindow = false;
	}

	renderStats = {};

	u32 image_index = 0;
	VkResult result = vkAcquireNextImageKHR(device, wininf->swapchain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &image_index);
	if(result == VK_ERROR_OUT_OF_DATE_KHR) {
		// the surface has changed in some way that makes it no longer compatible with its swapchain
		// so we need to recreate the swapchain
		remakeWindow = true;
		return;
	} else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		Assert(!"failed to acquire swapchain image");
	}
	
	update_uniform_buffers();
	setup_commands();
	
	
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_update
void
render_update(){DPZoneScoped;
	Stopwatch update_stopwatch = start_stopwatch();
	AssertRS(RSVK_PIPELINECREATE | RSVK_FRAMES | RSVK_SYNCOBJECTS, "Render called before CreatePipelines or CreateFrames or CreateSyncObjects");
	renderStage = RSVK_RENDER;
	
	//TODO this is definitely not the best way to do this, especially if we ever want to have more than 2 windows
	//     implement a count of how many surfaces have been made instead, maybe even use array
	Stopwatch render_stopwatch = start_stopwatch();
	forI(MAX_SURFACES){
		if(!swapchains[i].swapchain) continue;
		renderActiveSurface = i;
		Window* scwin = activeSwapchain.window;
		
		if(scwin->resized) remakeWindow = true;
		if(remakeWindow){
			activeSwapchain.width  = scwin->width;
			activeSwapchain.height = scwin->height;
			if(activeSwapchain.width <= 0 || activeSwapchain.height <= 0){ return; }
			vkDeviceWaitIdle(device);
			CreateSwapchain(scwin, i);
			CreateFrames();
			activeSwapchain.frameIndex = 0;
			remakeWindow = false;
		}
		
		//reset frame renderStats
		renderStats = {};
		
		//get next image from surface
		u32 imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, activeSwapchainKHR, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &imageIndex);
		if(result == VK_ERROR_OUT_OF_DATE_KHR){
			remakeWindow = true;
			return;
		}else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
			Assert(!"failed to acquire swap chain image");
		}
		
		//render stuff
		if(renderSettings.lightFrustrums){
			render_frustrum(vkLights[0].toVec3(), vec3::ZERO, 1, 90, renderSettings.shadowNearZ, renderSettings.shadowFarZ, Color_White);
		}
		UpdateUniformBuffers();
		
		SetupCommands();
		
		//execute draw commands
		BuildCommands();
		
		//submit the command buffer to the queue
		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAcquiredSemaphore;
		submitInfo.pWaitDstStageMask = &wait_stage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &activeSwapchain.frames[imageIndex].commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderCompleteSemaphore;
		resultVk = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE); AssertVk(resultVk, "failed to submit draw command buffer");
		
		if(remakeWindow){
			return;
		}
		
		//present the image
		VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &activeSwapchainKHR;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = 0;
		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		
		if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || remakeWindow){  //!Cleanup remakeWindow is already checked
			vkDeviceWaitIdle(device);
			CreateSwapchain(scwin, i);
			CreateFrames();
			remakeWindow = false;
		}else if(result != VK_SUCCESS){
			Assert(!"failed to present swap chain image");
		}
		
		//iterate the frame index
		activeSwapchain.frameIndex = (activeSwapchain.frameIndex + 1) % activeSwapchain.minImageCount; //loops back to zero after reaching minImageCount
		result = vkQueueWaitIdle(graphicsQueue);
		switch (result){
			case VK_ERROR_OUT_OF_HOST_MEMORY:   LogE("vulkan", "OUT_OF_HOST_MEMORY");   Assert(!"CPU ran out of memory"); break;
			case VK_ERROR_OUT_OF_DEVICE_MEMORY: LogE("vulkan", "OUT_OF_DEVICE_MEMORY"); Assert(!"GPU ran out of memory"); break;
			case VK_ERROR_DEVICE_LOST:          LogE("vulkan", "DEVICE_LOST");          Assert(!"Bad Sync/Overheat/Drive Bug"); break;
			case VK_SUCCESS:default: break;
		}
		ResetCommands();
	}
	
	//update renderStats
	renderStats.drawnTriangles += renderStats.drawnIndices / 3;
	//renderStats.totalVertices  += (u32)vertexBuffer.size() + renderTwodVertexCount + renderTempWireframeVertexCount;
	//renderStats.totalIndices   += (u32)indexBuffer.size()  + renderTwodIndexCount  + renderTempWireframeIndexCount; //!Incomplete
	renderStats.totalTriangles += renderStats.totalIndices / 3;
	renderStats.renderTimeMS = peek_stopwatch(render_stopwatch);
	
	
	if(remakePipelines){
		CreatePipelines();
		UpdateMaterialPipelines();
		remakePipelines = false;
	}
	if(_remakeOffscreen){
		SetupOffscreenRendering();
		_remakeOffscreen = false;
	}
	
	renderActiveSurface = 0;
	DeshTime->renderTime = peek_stopwatch(update_stopwatch);
}

void
render_update_nu(RenderSwapchain* swapchain) {
	
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_reset
void
render_reset(){DPZoneScoped;
	PrintVk(1,"Resetting renderer");
	
	vkDeviceWaitIdle(device); //wait before cleanup
	NotImplemented;
	//TODO(delle) delete things
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_cleanup
//TODO(delle,Vu) maybe cache pipeline creation vars?
void
render_cleanup(){DPZoneScoped;
	PrintVk(1, "Initializing cleanup\n");
	
	render_save_settings();
	//save pipeline cache to disk
	if(pipelineCache != VK_NULL_HANDLE){
		size_t size = 0;
		resultVk = vkGetPipelineCacheData(device, pipelineCache, &size,    0); AssertVk(resultVk, "failed to get pipeline cache data size");
		void* data = memory_talloc(size);
		resultVk = vkGetPipelineCacheData(device, pipelineCache, &size, data); AssertVk(resultVk, "failed to get pipeline cache data");
		file_write_simple(str8_lit("data/pipelines.cache"), data, size);
	}
	
	vkDeviceWaitIdle(device);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_loading
//TODO(delle) upload extra mesh data to an SSBO
void
render_load_mesh(Mesh* mesh){DPZoneScoped;
	AssertRS(RSVK_LOGICALDEVICE, "LoadMesh called before CreateLogicalDevice");
	RenderMesh mvk{};
	mvk.base = mesh;
	mvk.vertexCount = mesh->vertex_count;
	mvk.indexCount = mesh->index_count;
	if(vkMeshes.count){
		mvk.vertexOffset = vkMeshes.last->vertexOffset + vkMeshes.last->vertexCount;
		mvk.indexOffset = vkMeshes.last->indexOffset + vkMeshes.last->indexCount;
	}
	
	u64 mesh_vb_size   = mesh->vertex_count * sizeof(MeshVertex);
	u64 mesh_ib_size   = mesh->index_count * sizeof(MeshIndex);
	u64 mesh_vb_offset = mvk.vertexOffset * sizeof(MeshVertex);
	u64 mesh_ib_offset = mvk.indexOffset * sizeof(MeshIndex);
	u64 total_vb_size  = RoundUpTo(mesh_vb_offset + mesh_vb_size, physicalDeviceProperties.limits.nonCoherentAtomSize);
	u64 total_ib_size  = RoundUpTo(mesh_ib_offset + mesh_ib_size, physicalDeviceProperties.limits.nonCoherentAtomSize);
	u64 vb_mapping_offset = RoundDownTo(mesh_vb_offset, physicalDeviceProperties.limits.nonCoherentAtomSize);
	u64 ib_mapping_offset = RoundDownTo(mesh_ib_offset, physicalDeviceProperties.limits.nonCoherentAtomSize);
	
	//create/resize buffers
	if(meshVertexBuffer.buffer == VK_NULL_HANDLE || meshVertexBuffer.size < total_vb_size){
		CreateOrResizeBuffer(&meshVertexBuffer, total_vb_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)meshVertexBuffer.buffer, "Mesh vertex buffer");
	}
	if(meshIndexBuffer.buffer  == VK_NULL_HANDLE || meshIndexBuffer.size  < total_ib_size){
		CreateOrResizeBuffer(&meshIndexBuffer,  total_ib_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)meshIndexBuffer.buffer,  "Mesh index buffer");
	}
	
	//copy memory to the GPU
	void* vb_data; void* ib_data;
	resultVk = vkMapMemory(device, meshVertexBuffer.memory, vb_mapping_offset, VK_WHOLE_SIZE, 0, &vb_data); AssertVk(resultVk);
	resultVk = vkMapMemory(device, meshIndexBuffer.memory,  ib_mapping_offset, VK_WHOLE_SIZE, 0, &ib_data); AssertVk(resultVk);
	{
		memcpy((u8*)vb_data + (mesh_vb_offset - vb_mapping_offset), mesh->vertex_array, mesh_vb_size);
		memcpy((u8*)ib_data + (mesh_ib_offset - ib_mapping_offset), mesh->index_array,  mesh_ib_size);
		
		VkMappedMemoryRange range[2] = {};
		range[0].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[0].memory = meshVertexBuffer.memory;
		range[0].offset = vb_mapping_offset;
		range[0].size   = VK_WHOLE_SIZE;
		range[1].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[1].memory = meshIndexBuffer.memory;
		range[1].offset = ib_mapping_offset;
		range[1].size   = VK_WHOLE_SIZE;
		resultVk = vkFlushMappedMemoryRanges(device, 2, range); AssertVk(resultVk);
	}
	vkUnmapMemory(device, meshVertexBuffer.memory);
	vkUnmapMemory(device, meshIndexBuffer.memory);
	
	mesh->render_idx = vkMeshes.count;
	vkMeshes.add(mvk);
}

void
render_load_texture(Texture* texture){DPZoneScoped;
	AssertRS(RSVK_COMMANDPOOL, "LoadTexture called before CreateCommandPool");
	TextureVk tvk{};
	tvk.base = texture;
	tvk.size = texture->width * texture->height * 4;
	
	//determine image format
	VkFormat image_format;
	switch(texture->format){ //TODO(delle) handle non RGBA formats properly
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
		case TextureType_OneDimensional:       image_type = VK_IMAGE_TYPE_1D; view_type = VK_IMAGE_VIEW_TYPE_1D;         Assert(!"not implemented yet"); break;
		case TextureType_TwoDimensional:       image_type = VK_IMAGE_TYPE_2D; view_type = VK_IMAGE_VIEW_TYPE_2D;         break;
		case TextureType_ThreeDimensional:     image_type = VK_IMAGE_TYPE_3D; view_type = VK_IMAGE_VIEW_TYPE_3D;         Assert(!"not implemented yet"); break;
		case TextureType_Cube:                 image_type = VK_IMAGE_TYPE_2D; view_type = VK_IMAGE_VIEW_TYPE_CUBE;       Assert(!"not implemented yet"); break;
		case TextureType_Array_OneDimensional: image_type = VK_IMAGE_TYPE_1D; view_type = VK_IMAGE_VIEW_TYPE_1D_ARRAY;   Assert(!"not implemented yet"); break;
		case TextureType_Array_TwoDimensional: image_type = VK_IMAGE_TYPE_2D; view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;   Assert(!"not implemented yet"); break;
		case TextureType_Array_Cube:           image_type = VK_IMAGE_TYPE_2D; view_type = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY; Assert(!"not implemented yet"); break;
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
	resultVk = vkCreateImage(device, &imageInfo, allocator, &tvk.image); AssertVk(resultVk, "failed to create image");
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, tvk.image, &memRequirements);
	VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	resultVk = vkAllocateMemory(device, &allocInfo, allocator, &tvk.memory); AssertVk(resultVk, "failed to allocate image memory");
	vkBindImageMemory(device, tvk.image, tvk.memory, 0);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE,(u64)tvk.image, (const char*)to_dstr8v(deshi_temp_allocator, "Texture image ", texture->name).str);
	
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
							 0, 0, 0, 0, 0, 1, &barrier);
		
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
	resultVk = vkCreateImageView(device, &viewInfo, allocator, &tvk.view); AssertVk(resultVk, "failed to create texture image view");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)tvk.view, (const char*)to_dstr8v(deshi_temp_allocator, "Image View ", texture->name).str);
	
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
	switch(texture->uv_mode){
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
		default: LogE("vulkan", "Unhandled texture address mode: ", texture->uv_mode); break;
	}
	samplerInfo.anisotropyEnable = renderSettings.anistropicFiltering;
	samplerInfo.maxAnisotropy    = (renderSettings.anistropicFiltering) ?  physicalDeviceProperties.limits.maxSamplerAnisotropy : 1.0f;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable    = VK_FALSE;
	samplerInfo.compareOp        = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipLodBias       = 0.0f;
	samplerInfo.minLod           = 0.0f;
	samplerInfo.maxLod           = (f32)texture->mipmaps;
	resultVk = vkCreateSampler(device, &samplerInfo, 0, &tvk.sampler); AssertVk(resultVk, "failed to create texture sampler");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SAMPLER, (u64)tvk.sampler, (const char*)to_dstr8v(deshi_temp_allocator, "Sampler ", texture->name).str);
	
	//fill texture descriptor image info
	tvk.descriptor.imageView   = tvk.view;
	tvk.descriptor.sampler     = tvk.sampler;
	tvk.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	//allocate descriptor set
	VkDescriptorSetAllocateInfo setAllocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	setAllocInfo.descriptorPool = descriptorPool;
	setAllocInfo.pSetLayouts = &descriptorSetLayouts.twod;
	setAllocInfo.descriptorSetCount = 1;
	resultVk = vkAllocateDescriptorSets(device, &setAllocInfo, &tvk.descriptorSet); AssertVk(resultVk);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)tvk.descriptorSet, (const char*)to_dstr8v(deshi_temp_allocator, "Texture descriptor set ", texture->name).str);
	
	VkWriteDescriptorSet set{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	set.dstSet = tvk.descriptorSet;
	set.dstArrayElement = 0;
	set.descriptorCount = 1;
	set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	set.pImageInfo = &tvk.descriptor;
	set.dstBinding = 0;
	
	vkUpdateDescriptorSets(device, 1, &set, 0, 0);
	
	texture->render_idx = textures.count;
	textures.add(tvk);
}

//TODO(delle) this currently requires 4 textures, fix that
void
render_load_material(Material* material){DPZoneScoped;
	AssertRS(RSVK_DESCRIPTORPOOL, "LoadMaterial called before CreateDescriptorPool");
	MaterialVk mvk{};
	mvk.base     = material;
	mvk.pipeline = GetPipelineFromShader(material->shader);
	
	//allocate descriptor set
	VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	allocInfo.descriptorPool     = descriptorPool;
	allocInfo.pSetLayouts        = &descriptorSetLayouts.textures;
	allocInfo.descriptorSetCount = 1;
	resultVk = vkAllocateDescriptorSets(device, &allocInfo, &mvk.descriptorSet); AssertVk(resultVk);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)mvk.descriptorSet, (const char*)to_dstr8v(deshi_temp_allocator, "Material descriptor set ",material->name).str);
	
	//write descriptor set per texture
	arrayT<VkWriteDescriptorSet> sets;
	if(material->texture_array){
		for_stb_array(material->texture_array){
			VkWriteDescriptorSet set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
			set.dstSet          = mvk.descriptorSet;
			set.dstArrayElement = 0;
			set.descriptorCount = 1;
			set.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			set.pImageInfo      = &textures[(*it)->render_idx].descriptor;
			set.dstBinding      = sets.size();
			sets.add(set);
		}
		vkUpdateDescriptorSets(device, sets.size(), sets.data, 0, 0);
	}
	
	//HACK to fix materials with no textures
	if(material->texture_array == 0 || arrlenu(material->texture_array) < 4){
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
		vkUpdateDescriptorSets(device, sets.size(), sets.data, 0, 0);
	}
	
	material->render_idx = vkMaterials.count;
	vkMaterials.add(mvk);
}

void
render_unload_mesh(Mesh* mesh){
	//!NotImplemented
}

void
render_unload_texture(Texture* texture){
	//!NotImplemented
}

void
render_unload_material(Material* material){
	//!NotImplemented
}

void
render_update_material(Material* material){DPZoneScoped;
	MaterialVk* mvk = &vkMaterials[material->render_idx];
	mvk->pipeline = GetPipelineFromShader(material->shader);
	
	//update descriptor set per texture
	arrayT<VkWriteDescriptorSet> sets;
	if(material->texture_array){
		for_stb_array(material->texture_array){
			VkWriteDescriptorSet set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
			set.dstSet          = mvk->descriptorSet;
			set.dstArrayElement = 0;
			set.descriptorCount = 1;
			set.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			set.pImageInfo      = &textures[(*it)->render_idx].descriptor;
			set.dstBinding      = sets.size();
			sets.add(set);
		}
		vkUpdateDescriptorSets(device, sets.size(), sets.data, 0, 0);
	}
	
	//HACK to fix materials with no textures
	if(material->shader == Shader_PBR && (material->texture_array == 0 || arrlenu(material->texture_array) < 4)){
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
		vkUpdateDescriptorSets(device, sets.size(), sets.data, 0, 0);
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_draw_3d
//TODO(delle) redo model drawing, i dont really like it, and i kinda want it to be cached if possible
//    things that need to be fixed/better:
//      1. the matrix is duplicated per batch
//      2. most commands will be remade every frame the exact same with just the matrix differing
//      3. this relies on scene mesh indexes matching renderer mesh indexes
void
render_model(Model* model, mat4* matrix){DPZoneScoped;
	Assert(renderModelCmdCount + arrlenu(model->batch_array) < MAX_MODEL_CMDS, "attempted to draw more than the global maximum number of batches");
	RenderModelCmd* cmd = renderModelCmdArray + renderModelCmdCount;
	forI(arrlenu(model->batch_array)){
		if(!model->batch_array[i].index_count) continue;
		cmd[i].vertex_offset = vkMeshes[model->mesh->render_idx].vertexOffset;
		cmd[i].index_offset  = vkMeshes[model->mesh->render_idx].indexOffset + model->batch_array[i].index_offset;
		cmd[i].index_count   = model->batch_array[i].index_count;
		cmd[i].material     = model->batch_array[i].material->render_idx;
		cmd[i].name         = (char*)model->name;
		cmd[i].matrix       = *matrix;
		renderModelCmdCount += 1;
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_draw_2d
void
deshi__render_start_cmd2(str8 file, u32 line, u32 layer, Texture* texture, vec2 scissorOffset, vec2 scissorExtent){DPZoneScoped;
	renderActiveLayer = layer;
	if(   (renderTwodCmdCounts[renderActiveSurface][layer] == 0)
	   || (renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]-1].handle        != textures[(texture) ? texture->render_idx : 1].descriptorSet)
	   || (renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]-1].scissor_offset != scissorOffset)
	   || (renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]-1].scissor_extent != scissorExtent)){
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].handle        = textures[(texture) ? texture->render_idx : 1].descriptorSet;
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].index_offset   = renderTwodIndexCount;
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].scissor_offset = scissorOffset;
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].scissor_extent = scissorExtent;
		renderTwodCmdCounts[renderActiveSurface][layer] += 1;
	}
	
#if BUILD_INTERNAL
	RenderBookKeeper keeper;
	keeper.type = RenderBookKeeper_Cmd;
	keeper.cmd = &renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]];
	keeper.file = file;
	keeper.line = line;
	renderBookKeeperArray[renderBookKeeperCount++] = keeper;
#endif
}

void
render_start_cmd2_exbuff(RenderTwodBuffer buffer, RenderTwodIndex index_offset, RenderTwodIndex index_count, Vertex2* vertbuff, RenderTwodIndex* indbuff, u32 layer, Texture* texture, vec2 scissorOffset, vec2 scissorExtent){
	renderActiveLayer = layer;
	RenderTwodIndex idx = renderExternalCmdCounts[buffer.idx];
	if((idx==0)
	   || (renderExternalCmdArrays[buffer.idx][idx-1].vertex_buffer  != vertbuff)
	   || (renderExternalCmdArrays[buffer.idx][idx-1].index_buffer   != indbuff)
	   || (renderExternalCmdArrays[buffer.idx][idx-1].handle        != textures[(texture) ? texture->render_idx : 1].descriptorSet)
	   || (renderExternalCmdArrays[buffer.idx][idx-1].scissor_offset != scissorOffset)
	   || (renderExternalCmdArrays[buffer.idx][idx-1].scissor_extent != scissorExtent)){
		renderExternalCmdArrays[buffer.idx][idx].vertex_buffer  = vertbuff;
		renderExternalCmdArrays[buffer.idx][idx].index_buffer   = indbuff;
		renderExternalCmdArrays[buffer.idx][idx].handle        = textures[(texture) ? texture->render_idx : 1].descriptorSet;
		renderExternalCmdArrays[buffer.idx][idx].index_offset   = index_offset;
		renderExternalCmdArrays[buffer.idx][idx].scissor_offset = scissorOffset;
		renderExternalCmdArrays[buffer.idx][idx].scissor_extent = scissorExtent;
		renderExternalCmdArrays[buffer.idx][idx].index_count    = index_count;
		renderExternalCmdCounts[buffer.idx] += 1;
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_buffer
RenderBuffer*
render_buffer_create(void* data, u64 size, RenderBufferUsageFlags usage, RenderMemoryPropertyFlags properties, RenderMemoryMappingType mapping){DPZoneScoped;
	if(!deshi__render_buffer_pool){
		LogEVk("Must not be called before deshi__render_buffer_pool is init.");
		return 0;
	}
	if(size == 0){
		LogEVk("Must not be called with zero size.");
		return 0;
	}
	if(HasFlag(properties,RenderMemoryProperty_HostStreamed) && HasFlag(properties,RenderMemoryPropertyFlag_LazilyAllocated)){
		LogEVk("The flags RenderMemoryPropertyFlag_HostVisible and RenderMemoryPropertyFlag_LazilyAllocated are incompatible.");
		return 0;
	}
	
	RenderBuffer* result = memory_pool_push(deshi__render_buffer_pool);
	
	VkDeviceSize aligned_buffer_size = RoundUpTo(size, buffer_memory_alignment);
	{//create the buffer
		VkBufferUsageFlags usage_flags = 0;
		if(HasFlag(usage,RenderBufferUsage_TransferSource))      usage_flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		if(HasFlag(usage,RenderBufferUsage_TransferDestination)) usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		if(HasFlag(usage,RenderBufferUsage_UniformTexelBuffer))  usage_flags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
		if(HasFlag(usage,RenderBufferUsage_StorageTexelBuffer))  usage_flags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
		if(HasFlag(usage,RenderBufferUsage_UniformBuffer))       usage_flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		if(HasFlag(usage,RenderBufferUsage_StorageBuffer))       usage_flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		if(HasFlag(usage,RenderBufferUsage_IndexBuffer))         usage_flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		if(HasFlag(usage,RenderBufferUsage_VertexBuffer))        usage_flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		if(HasFlag(usage,RenderBufferUsage_IndirectBuffer))      usage_flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		if(mapping == RenderMemoryMapping_None){
			usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT; //RenderMemoryMapping_None uses a staging buffer, so this buffer must be able to receive
		}
		
		VkBufferCreateInfo create_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
		create_info.size        = aligned_buffer_size;
		create_info.usage       = usage_flags;
		create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		resultVk = vkCreateBuffer(device, &create_info, allocator, (VkBuffer*)&result->buffer_handle); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)result->buffer_handle, (const char*)to_dstr8v(deshi_temp_allocator, "Render Buffer(",memory_pool_count(deshi__render_buffer_pool)-1,") Buffer").str);
	}
	
	{//allocate the memory
		VkMemoryPropertyFlags property_flags = 0;
		if(HasFlag(properties,RenderMemoryPropertyFlag_DeviceLocal))     property_flags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		if(HasFlag(properties,RenderMemoryPropertyFlag_HostVisible))     property_flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		if(HasFlag(properties,RenderMemoryPropertyFlag_HostCoherent))    property_flags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		if(HasFlag(properties,RenderMemoryPropertyFlag_HostCached))      property_flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		if(HasFlag(properties,RenderMemoryPropertyFlag_LazilyAllocated)) property_flags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
		
		//get buffer memory requirements (alignment, size, memory type)
		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(device, (VkBuffer)result->buffer_handle, &requirements);
		buffer_memory_alignment = (buffer_memory_alignment > requirements.alignment) ? buffer_memory_alignment : requirements.alignment;
		
		//alloc and bind to the buffer
		VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		alloc_info.allocationSize  = requirements.size;
		alloc_info.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, property_flags);
		resultVk = vkAllocateMemory(device, &alloc_info, allocator, (VkDeviceMemory*)&result->memory_handle); AssertVk(resultVk);
		resultVk = vkBindBufferMemory(device, (VkBuffer)result->buffer_handle, (VkDeviceMemory)result->memory_handle, 0); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DEVICE_MEMORY, (u64)result->memory_handle, (const char*)to_dstr8v(deshi_temp_allocator, "Render Buffer(",memory_pool_count(deshi__render_buffer_pool)-1,") Memory").str);
	}
	
	//map and upload the data depending on the mapping style
	if(mapping == RenderMemoryMapping_None){
		if(data == 0){ //TOOD(delle) support reserving device memory for non-host use (compute shaders)
			LogEVk("Called with RenderMemoryMapping_None but the data pointer was null. This means that the buffer on the device will be empty and cannot be written to.");
		}else if(HasFlag(properties,RenderMemoryPropertyFlag_HostVisible|RenderMemoryPropertyFlag_HostCoherent|RenderMemoryPropertyFlag_HostCached)){
			LogEVk("Called with incompatible mapping and memory flags, RenderMemoryMapping_None and RenderMemoryPropertyFlag_HostVisible or RenderMemoryPropertyFlag_HostCoherent or RenderMemoryPropertyFlag_HostCached.");
		}else{
			//create a staging buffer
			VkBuffer staging_buffer_handle;
			VkBufferCreateInfo staging_buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
			staging_buffer_info.size        = aligned_buffer_size;
			staging_buffer_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			staging_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			resultVk = vkCreateBuffer(device, &staging_buffer_info, allocator, &staging_buffer_handle); AssertVk(resultVk);
			
			//allocate memory for the staging buffer
			VkDeviceMemory staging_memory_handle;
			VkMemoryRequirements staging_memory_requirements;
			vkGetBufferMemoryRequirements(device, staging_buffer_handle, &staging_memory_requirements);
			VkMemoryAllocateInfo staging_memory_alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
			staging_memory_alloc_info.allocationSize  = staging_memory_requirements.size;
			staging_memory_alloc_info.memoryTypeIndex = FindMemoryType(staging_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			resultVk = vkAllocateMemory(device, &staging_memory_alloc_info, allocator, &staging_memory_handle); AssertVk(resultVk);
			resultVk = vkBindBufferMemory(device, staging_buffer_handle, staging_memory_handle, 0); AssertVk(resultVk);
			
			//map the staging buffer and copy the data to it
			void* staging_memory_data;
			resultVk = vkMapMemory(device, staging_memory_handle, 0, aligned_buffer_size, 0, &staging_memory_data); AssertVk(resultVk);
			CopyMemory(staging_memory_data, data, size);
			vkUnmapMemory(device, staging_memory_handle);
			
			//copy from the staging buffer to the device-only buffer
			CopyBuffer(staging_buffer_handle, (VkBuffer)result->buffer_handle, size);
			
			//clean up the staging buffer
			vkDestroyBuffer(device, staging_buffer_handle, allocator);
			vkFreeMemory(device, staging_memory_handle, allocator);
		}
	}else if(mapping == RenderMemoryMapping_MapWriteUnmap){
		if(data){
			resultVk = vkMapMemory(device, (VkDeviceMemory)result->memory_handle, 0, aligned_buffer_size, 0, &result->mapped_data); AssertVk(resultVk);
			
			CopyMemory(result->mapped_data, data, size);
			
			VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
			range.memory = (VkDeviceMemory)result->memory_handle;
			range.offset = 0;
			range.size   = RoundUpTo(size, physicalDeviceProperties.limits.nonCoherentAtomSize); //TODO(delle) test that this works, otherwise use VK_WHOLE_SIZE
			resultVk = vkFlushMappedMemoryRanges(device, 1, &range); AssertVk(resultVk);
			
			vkUnmapMemory(device, (VkDeviceMemory)result->memory_handle);
		}
	}else if(mapping == RenderMemoryMapping_Persistent){
		resultVk = vkMapMemory(device, (VkDeviceMemory)result->memory_handle, 0, aligned_buffer_size, 0, &result->mapped_data); AssertVk(resultVk);
		if(data){
			CopyMemory(result->mapped_data, data, size);
			
			VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
			range.memory = (VkDeviceMemory)result->memory_handle;
			range.offset = 0;
			range.size   = RoundUpTo(size, physicalDeviceProperties.limits.nonCoherentAtomSize); //TODO(delle) test that this works, otherwise use VK_WHOLE_SIZE
			resultVk = vkFlushMappedMemoryRanges(device, 1, &range); AssertVk(resultVk);
		}
	}
	
	StaticAssertAlways(sizeof(VkDeviceSize) == sizeof(u64));
	result->size       = (u64)aligned_buffer_size;
	result->usage      = usage;
	result->properties = properties;
	result->mapping    = mapping;
	return result;
}


void
render_buffer_delete(RenderBuffer* buffer){DPZoneScoped;
	Assert(buffer);
	if(!deshi__render_buffer_pool){
		LogEVk("Must not be called before deshi__render_buffer_pool is init.");
		return;
	}
	if(!buffer || !buffer->buffer_handle || !buffer->memory_handle){
		LogEVk("The input buffer was not properly created with render_buffer_create() or was previously deleted.");
		return;
	}
	
	//unmap before deletion
	if(buffer->mapped_data){
		vkUnmapMemory(device, (VkDeviceMemory)buffer->memory_handle);
	}
	
	vkDestroyBuffer(device, (VkBuffer)buffer->buffer_handle, allocator);
	vkFreeMemory(device, (VkDeviceMemory)buffer->memory_handle, allocator);
	
	memory_pool_delete(deshi__render_buffer_pool, buffer);
}


void
render_buffer_map(RenderBuffer* buffer, u64 offset, u64 size){DPZoneScoped;
	Assert(buffer);
	if(!deshi__render_buffer_pool){
		LogEVk("Must not be called before deshi__render_buffer_pool is init.");
		return;
	}
	if(!buffer || !buffer->buffer_handle || !buffer->memory_handle){
		LogEVk("The input buffer was not properly created with render_buffer_create() or was previously deleted.");
		return;
	}
	if(buffer->mapped_data){
		if(buffer->mapping == RenderMemoryMapping_Persistent){
			LogWVk("Cannot map a persistently mapped buffer since it's always actively mapped.");
		}else{
			LogWVk("Cannot map an actively mapped buffer.");
		}
		return;
	}
	if(buffer->mapping != RenderMemoryMapping_MapWriteUnmap){
		LogEVk("A buffer must have the mapping RenderMemoryMapping_MapWriteUnmap in order to be mapped in the middle of its lifetime.");
		return;
	}
	
	resultVk = vkMapMemory(device, (VkDeviceMemory)buffer->memory_handle, offset, size, 0, &buffer->mapped_data); AssertVk(resultVk);
	buffer->mapped_offset = offset;
	buffer->mapped_size   = size;
}


void
render_buffer_unmap(RenderBuffer* buffer, b32 flush){DPZoneScoped;
	Assert(buffer);
	if(!deshi__render_buffer_pool){
		LogEVk("Must not be called before deshi__render_buffer_pool is init.");
		return;
	}
	if(!buffer || !buffer->buffer_handle || !buffer->memory_handle){
		LogEVk("The input buffer was not properly created with render_buffer_create() or was previously deleted.");
		return;
	}
	if(!buffer->mapped_data){
		LogEVk("The input buffer is not actively mapped.");
		return;
	}
	if(buffer->mapping != RenderMemoryMapping_MapWriteUnmap){
		LogEVk("A buffer must have the mapping RenderMemoryMapping_MapWriteUnmap in order to be unmapped in the middle of its lifetime.");
		return;
	}
	
	//flush before unmapping
	if(flush){
		VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
		range.memory = (VkDeviceMemory)buffer->memory_handle;
		range.offset = buffer->mapped_offset;
		range.size   = RoundUpTo(buffer->mapped_size, physicalDeviceProperties.limits.nonCoherentAtomSize); //TODO(delle) test that this works, otherwise use VK_WHOLE_SIZE
		resultVk = vkFlushMappedMemoryRanges(device, 1, &range); AssertVk(resultVk);
	}
	
	vkUnmapMemory(device, (VkDeviceMemory)buffer->memory_handle);
}


void
render_buffer_flush(RenderBuffer* buffer){DPZoneScoped;
	Assert(buffer);
	if(!deshi__render_buffer_pool){
		LogEVk("Must not be called before deshi__render_buffer_pool is init.");
		return;
	}
	if(!buffer || !buffer->buffer_handle || !buffer->memory_handle){
		LogEVk("The input buffer was not properly created with render_buffer_create() or was previously deleted.");
		return;
	}
	if(!buffer->mapped_data){
		LogEVk("The input buffer is not actively mapped.");
		return;
	}
	
	VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
	range.memory = (VkDeviceMemory)buffer->memory_handle;
	range.offset = buffer->mapped_offset;
	range.size   = RoundUpTo(buffer->mapped_size, physicalDeviceProperties.limits.nonCoherentAtomSize); //TODO(delle) test that this works, otherwise use VK_WHOLE_SIZE
	resultVk = vkFlushMappedMemoryRanges(device, 1, &range); AssertVk(resultVk);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_surface
void
render_register_surface(Window* window){DPZoneScoped;
	Assert(DeshiModuleLoaded(DS_RENDER), "Attempted to register a surface for a window without initializaing Render module first");
	Assert(window->index < MAX_SURFACES);
	CreateSurface(window, window->index);
	
	u32 formatCount;
	u32 presentModeCount;
	VkBool32 presentSupport = false;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surfaces[window->index], &formatCount, 0);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surfaces[window->index], &presentModeCount, 0);
	vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, physicalQueueFamilies.presentFamily.value, surfaces[window->index], &presentSupport);
	
	if(!formatCount || !presentModeCount || !presentSupport){
		LogE("VULKAN", "Vulkan failed to init a new surface on the current physical device for window: ", window->title);
		surfaces[window->index] = VK_NULL_HANDLE;
		return;
	}
	
	CreateSwapchain(window, window->index);
	CreateFrames();
}

void
render_set_active_surface(Window* window){DPZoneScoped;
	Assert(window->index != -1, "Attempt to set draw target to a window who hasnt been registered to the renderer");
	renderActiveSurface = window->index;
}

void
render_set_active_surface_idx(u32 idx){DPZoneScoped;
	Assert(idx < MAX_SURFACES, "Attempt to set draw target to a window who hasnt been registered to the renderer");
	renderActiveSurface = idx;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_light
void
render_update_light(u32 idx, vec3 position, f32 brightness){DPZoneScoped;
	vkLights[idx] = Vec4(position.x, position.y, position.z, brightness);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_camera
void
render_update_camera_position(vec3 position){DPZoneScoped;
	uboVS.values.viewPos = Vec4(position.x, position.y, position.z, 1.f);
}

void
render_update_camera_view(mat4* view_matrix){DPZoneScoped;
	uboVS.values.view = *view_matrix;
}

void
render_update_camera_projection(mat4* projection_matrix){DPZoneScoped;
	uboVS.values.proj = *projection_matrix;
	uboVS.values.proj.arr[5] *= -1;
}

void
render_use_default_camera(){DPZoneScoped;
	uboVS.values.view = Math::LookAtMatrix(vec3::ZERO, (vec3::FORWARD * mat4::RotationMatrix(vec3::ZERO)).normalized()).Inverse();
	uboVS.values.proj = Camera::MakePerspectiveProjectionMatrix((f32)DeshWindow->width, (f32)DeshWindow->height, 90.f, 1000.f, 0.1f);
	uboVS.values.proj.arr[5] *= -1;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shaders
void
render_reload_shader(u32 shader){DPZoneScoped;
	switch(shader){
		case(Shader_NULL):{
			vkDestroyPipeline(device, pipelines.null, 0);
			shader_stages[0] = load_shader(STR8("null.vert"), baked_shader_null_vert, VK_SHADER_STAGE_VERTEX_BIT);
			shader_stages[1] = load_shader(STR8("null.frag"), baked_shader_null_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
			pipeline_create_info.stageCount = 2;
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, 0, &pipelines.null); AssertVk(resultVk);
		}break;
		case(Shader_Flat):{
			vkDestroyPipeline(device, pipelines.flat, 0);
			shader_stages[0] = load_shader(STR8("flag.vert"), baked_shader_flat_vert, VK_SHADER_STAGE_VERTEX_BIT);
			shader_stages[1] = load_shader(STR8("flag.frag"), baked_shader_flat_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
			pipeline_create_info.stageCount = 2;
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, 0, &pipelines.flat); AssertVk(resultVk, "failed to create flat graphics pipeline");
		}break;
		case(Shader_Wireframe):{
			if(deviceFeatures.fillModeNonSolid){
				vkDestroyPipeline(device, pipelines.wireframe, 0);
				rasterization_state.polygonMode = VK_POLYGON_MODE_LINE;
				rasterization_state.cullMode = VK_CULL_MODE_NONE;
				depth_stencil_state.depthTestEnable = VK_FALSE;
				shader_stages[0] = load_shader(STR8("wireframe.vert"), baked_shader_wireframe_vert, VK_SHADER_STAGE_VERTEX_BIT);
				shader_stages[1] = load_shader(STR8("wireframe.frag"), baked_shader_wireframe_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
				pipeline_create_info.stageCount = 2;
				resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, 0, &pipelines.wireframe); AssertVk(resultVk, "failed to create wireframe graphics pipeline");
				rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
				rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
				depth_stencil_state.depthTestEnable = VK_TRUE;
			}
		}break;
		case(Shader_Phong):{
			vkDestroyPipeline(device, pipelines.phong, 0);
			shader_stages[0] = load_shader(STR8("phong.vert"), baked_shader_phong_vert, VK_SHADER_STAGE_VERTEX_BIT);
			shader_stages[1] = load_shader(STR8("phong.frag"), baked_shader_phong_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
			pipeline_create_info.stageCount = 2;
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, 0, &pipelines.phong); AssertVk(resultVk, "failed to create phong graphics pipeline");
		}break;
		case(Shader_PBR):{
			vkDestroyPipeline(device, pipelines.pbr, 0);
			shader_stages[0] = load_shader(STR8("pbr.vert"), baked_shader_pbr_vert, VK_SHADER_STAGE_VERTEX_BIT);
			shader_stages[1] = load_shader(STR8("pbr.frag"), baked_shader_pbr_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
			pipeline_create_info.stageCount = 2;
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, 0, &pipelines.pbr); AssertVk(resultVk, "failed to create pbr graphics pipeline");
		}break;
		default:{
			render_reload_all_shaders();
		}break;
	}
	UpdateMaterialPipelines();
}

void
render_reload_all_shaders(){DPZoneScoped;
	remakePipelines = true;
	
	vkDestroyPipeline(device, pipelines.null, 0);
	shader_stages[0] = load_shader(STR8("null.vert"), baked_shader_null_vert, VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader(STR8("null.frag"), baked_shader_null_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipeline_create_info.stageCount = 2;
	resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, 0, &pipelines.null); AssertVk(resultVk);
	
	vkDestroyPipeline(device, pipelines.flat, 0);
	shader_stages[0] = load_shader(STR8("flat.vert"), baked_shader_flat_vert, VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader(STR8("flat.frag"), baked_shader_flat_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipeline_create_info.stageCount = 2;
	resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, 0, &pipelines.flat); AssertVk(resultVk, "failed to create flat graphics pipeline");
	
	if(deviceFeatures.fillModeNonSolid){
		vkDestroyPipeline(device, pipelines.wireframe, 0);
		rasterization_state.polygonMode = VK_POLYGON_MODE_LINE;
		rasterization_state.cullMode = VK_CULL_MODE_NONE;
		depth_stencil_state.depthTestEnable = VK_FALSE;
		shader_stages[0] = load_shader(STR8("wireframe.vert"), baked_shader_wireframe_vert, VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages[1] = load_shader(STR8("wireframe.frag"), baked_shader_wireframe_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
		pipeline_create_info.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, 0, &pipelines.wireframe); AssertVk(resultVk, "failed to create wireframe graphics pipeline");
		rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
		depth_stencil_state.depthTestEnable = VK_TRUE;
	}
	
	vkDestroyPipeline(device, pipelines.phong, 0);
	shader_stages[0] = load_shader(STR8("phong.vert"), baked_shader_phong_vert, VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader(STR8("phong.frag"), baked_shader_phong_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipeline_create_info.stageCount = 2;
	resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, 0, &pipelines.phong); AssertVk(resultVk, "failed to create phong graphics pipeline");
	
	vkDestroyPipeline(device, pipelines.pbr, 0);
	shader_stages[0] = load_shader(STR8("pbr.vert"), baked_shader_pbr_vert, VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader(STR8("pbr.frag"), baked_shader_pbr_frag, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipeline_create_info.stageCount = 2;
	resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create_info, 0, &pipelines.pbr); AssertVk(resultVk, "failed to create pbr graphics pipeline");
	
	UpdateMaterialPipelines();
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_other
void
render_remake_offscreen(){DPZoneScoped;
	_remakeOffscreen = true;
}

RenderTwodBuffer
render_create_external_2d_buffer(u64 vert_buffsize, u64 ind_buffsize){DPZoneScoped;
	if(externalBufferCount + 1 > MAX_EXTERNAL_BUFFERS){
		LogE("render", "Cannot create more than", MAX_EXTERNAL_BUFFERS, " external buffers.\nChange MAX_ETERNAL_BUFFERS define (vulkan.cpp) to increase limit.");
		return RenderTwodBuffer{0};
	}
	
	RenderTwodBuffer buff;
	buff.vertex_handle = externalVertexBuffers->cursor;
	externalVertexBuffers->cursor += sizeof(BufferVk);
	externalVertexBuffers->used += sizeof(BufferVk);
	buff.index_handle = externalIndexBuffers->cursor;
	externalIndexBuffers->cursor += sizeof(BufferVk);
	externalIndexBuffers->used += sizeof(BufferVk);
	
	buff.idx = externalBufferCount;
	
	externalBufferCount++;
	BufferVk* vkvbuff = (BufferVk*)buff.vertex_handle;
	BufferVk* vkibuff = (BufferVk*)buff.index_handle;
	CreateOrResizeBuffer(vkvbuff, vert_buffsize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	CreateOrResizeBuffer(vkibuff, ind_buffsize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	
	return buff;
}

void render_update_texture(Texture* texture, vec2i offset, vec2i size, u32* data){
	NotImplemented;
}

void
render_update_external_2d_buffer(RenderTwodBuffer* buffer, Vertex2* vb, RenderTwodIndex vcount, RenderTwodIndex* ib, RenderTwodIndex icount){
	BufferVk* vbuff = (BufferVk*)buffer->vertex_handle;
	BufferVk* ibuff = (BufferVk*)buffer->index_handle;
	
	void* vb_data; void* ib_data;
	resultVk = vkMapMemory(device, vbuff->memory, 0, vcount, 0, &vb_data); AssertVk(resultVk);
	resultVk = vkMapMemory(device, ibuff->memory, 0, icount, 0, &ib_data); AssertVk(resultVk);
	{
		CopyMemory(vb_data, vb, vcount);
		CopyMemory(ib_data, ib, icount);
		
		VkMappedMemoryRange range[2] = {};
		range[0].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[0].memory = vbuff->memory;
		range[0].size   = VK_WHOLE_SIZE;
		range[1].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[1].memory = ibuff->memory;
		range[1].size   = VK_WHOLE_SIZE;
		resultVk = vkFlushMappedMemoryRanges(device, 2, range); AssertVk(resultVk);
	}
	vkUnmapMemory(device, vbuff->memory);
	vkUnmapMemory(device, ibuff->memory);
}	
