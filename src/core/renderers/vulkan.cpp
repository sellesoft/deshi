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


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_vars
#define INDEX_TYPE_VK_UI   VK_INDEX_TYPE_UINT32
#define INDEX_TYPE_VK_TEMP VK_INDEX_TYPE_UINT16
#define INDEX_TYPE_VK_MESH VK_INDEX_TYPE_UINT32

local array<RenderMesh>  vkMeshes(deshi_allocator);
local array<TextureVk>   textures(deshi_allocator);
local array<MaterialVk>  vkMaterials(deshi_allocator);
local vec4 vkLights[10]{ vec4(0,0,0,-1) };

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
local VkPhysicalDevice         physicalDevice         = VK_NULL_HANDLE;
local VkPhysicalDeviceProperties physicalDeviceProperties{0};
local VkSampleCountFlagBits    maxMsaaSamples         = VK_SAMPLE_COUNT_1_BIT;
local VkPhysicalDeviceFeatures deviceFeatures         = {0};
local VkPhysicalDeviceFeatures enabledFeatures        = {0};
local QueueFamilyIndices       physicalQueueFamilies  = {};
local VkDevice                 device                 = VK_NULL_HANDLE;
local VkQueue                  graphicsQueue          = VK_NULL_HANDLE;
local VkQueue                  presentQueue           = VK_NULL_HANDLE; 
local VkDeviceSize             bufferMemoryAlignment  = 256;

local const char* validationLayers[] = {
	"VK_LAYER_KHRONOS_validation"
};
local char* deviceExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
	VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME 
};
local array<VkValidationFeatureEnableEXT> validationFeaturesEnabled;

local VkDebugUtilsMessageSeverityFlagsEXT callbackSeverities = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
local VkDebugUtilsMessageTypeFlagsEXT     callbackTypes      = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
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
	array<FrameVk>           frames;
	FramebufferAttachmentsVk attachments{};
};

local VkSwapchain swapchains[MAX_SURFACES];
//TODO make all of the Create functions take in a swapchain/surface rather than always indexing with a global var :)
#define activeSwapchain swapchains[renderActiveSurface] //TODO replace these with original text eventually or make better getter
#define activeSwapchainKHR swapchains[renderActiveSurface].swapchain


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
local array<VkDynamicState>                    dynamicStates(deshi_allocator);
local array<VkVertexInputBindingDescription>   vertexInputBindings(deshi_allocator);
local array<VkVertexInputAttributeDescription> vertexInputAttributes(deshi_allocator);
local array<VkVertexInputBindingDescription>   twodVertexInputBindings(deshi_allocator);
local array<VkVertexInputAttributeDescription> twodVertexInputAttributes(deshi_allocator);

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


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_vars_shaders
local VkPipelineShaderStageCreateInfo shaderStages[6];
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
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	
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
	
	VkDeviceSize aligned_buffer_size = RoundUpTo(new_size, bufferMemoryAlignment);
	VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.size        = aligned_buffer_size;
	bufferInfo.usage       = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	resultVk = vkCreateBuffer(device, &bufferInfo, allocator, &buffer); AssertVk(resultVk);
	
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(device, buffer, &req);
	bufferMemoryAlignment = (bufferMemoryAlignment > req.alignment) ? bufferMemoryAlignment : req.alignment;
	VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.allocationSize  = req.size;
	allocInfo.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, properties);
	resultVk = vkAllocateMemory(device, &allocInfo, allocator, &buffer_memory); AssertVk(resultVk);
	resultVk = vkBindBufferMemory(device, buffer, buffer_memory, 0); AssertVk(resultVk);
	
	if(buffer_size){
		void* old_buffer_data; void* new_buffer_data;
		resultVk = vkMapMemory(device, old_buffer_memory, 0, buffer_size, 0, &old_buffer_data); AssertVk(resultVk);
		resultVk = vkMapMemory(device, buffer_memory,     0, new_size,    0, &new_buffer_data); AssertVk(resultVk);
		
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
	
	buffer_size = new_size;
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
	VkDeviceSize alignedBufferSize = ((newSize-1) / bufferMemoryAlignment + 1) * bufferMemoryAlignment;
	VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.size        = alignedBufferSize;
	bufferInfo.usage       = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	resultVk = vkCreateBuffer(device, &bufferInfo, allocator, &buffer); AssertVk(resultVk, "failed to create buffer");
	
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(device, buffer, &req);
	bufferMemoryAlignment = (bufferMemoryAlignment > req.alignment) ? bufferMemoryAlignment : req.alignment;
	
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

//copies a buffer, we use this to copy from CPU to GPU
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
		array<VkLayerProperties> availableLayers(layerCount, deshi_temp_allocator);
		availableLayers.count = layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data);
		
		forI(ArrayCount(validationLayers)){
			bool layerFound = false;
			forE(availableLayers){
				if(strcmp(validationLayers[i], it->layerName) == 0){
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
	validationFeatures.enabledValidationFeatureCount  = validationFeaturesEnabled.count;
	validationFeatures.pEnabledValidationFeatures     = validationFeaturesEnabled.data;
	
	u32 pcount = 0;
	vkEnumerateInstanceExtensionProperties(0, &pcount, 0);
	VkExtensionProperties* eprops = (VkExtensionProperties*)memalloc(pcount*sizeof(VkExtensionProperties));
	vkEnumerateInstanceExtensionProperties(0, &pcount, eprops);
	/*forI(pcount){
		PRINTLN(eprops[i].extensionName);
	}*/
	
	//get required extensions
	PrintVk(3, "Getting required extensions");
	
#if DESHI_WINDOWS
	u32 extensionCount = 2;
	array<const char*> extensions{ VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
#elif DESHI_LINUX
	u32 extensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	array<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#elif DESHI_MAC
	u32 extensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	array<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#endif
	
#if BUILD_INTERNAL
	extensions.add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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
	if(renderSettings.debugging){
		createInfo.enabledLayerCount   = (u32)ArrayCount(validationLayers);
		createInfo.ppEnabledLayerNames = validationLayers;
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
	debugCreateInfo.messageSeverity = callbackSeverities;
	debugCreateInfo.messageType     = callbackTypes;
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
	
	
#ifdef DESHI_WINDOWS
	PrintVk(2, "Creating Win32-Vulkan surface");
	VkWin32SurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
	info.hwnd = (HWND)win->handle;
	info.hinstance = (HINSTANCE)win32_console_instance;
	resultVk = vkCreateWin32SurfaceKHR(instance, &info, 0, &surfaces[surface_idx]); AssertVk(resultVk, "failed to create win32 surface");
#elif DESHI_LINUX
	PrintVk(2, "Creating glfw-Vulkan surface");
	resultVk = glfwCreateWindowSurface(instance, DeshWindow->window, allocator, &surfaces[renderActiveSurface]); AssertVk(resultVk, "failed to create glfw surface");
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
	array<VkPhysicalDevice> devices(deviceCount, deshi_temp_allocator);
	devices.count = deviceCount;
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data);
	
	//test all available GPUs
	for(VkPhysicalDevice device : devices){ 
		{//find device's queue families
			physicalQueueFamilies.graphicsFamily.reset();
			physicalQueueFamilies.presentFamily.reset();
			
			u32 queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, 0);
			array<VkQueueFamilyProperties> queueFamilies(queueFamilyCount, deshi_temp_allocator);
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
			array<VkExtensionProperties> availableExtensions(extensionCount, deshi_temp_allocator);
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
CreateLogicalDevice(){DPZoneScoped;
	PrintVk(2, "Creating logical device");
	AssertRS(RSVK_PHYSICALDEVICE, "CreateLogicalDevice called before PickPhysicalDevice");
	renderStage |= RSVK_LOGICALDEVICE;
	
	//setup device queue create infos
	f32 queuePriority = 1.0f;
	array<VkDeviceQueueCreateInfo> queueCreateInfos(deshi_temp_allocator);
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
		createInfo.enabledLayerCount     = (u32)ArrayCount(validationLayers);
		createInfo.ppEnabledLayerNames   = validationLayers;
	}else{
		createInfo.enabledLayerCount     = 0;
	}
	
	resultVk = vkCreateDevice(physicalDevice, &createInfo, 0, &device); AssertVk(resultVk, "failed to create logical device");
	
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
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surfaces[renderActiveSurface], &activeSwapchain.supportDetails.capabilities);
		
		u32 formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaces[renderActiveSurface], &formatCount, 0);
		if(formatCount != 0){
			activeSwapchain.supportDetails.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaces[renderActiveSurface], &formatCount, activeSwapchain.supportDetails.formats.data);
		}
		
		u32 presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surfaces[renderActiveSurface], &presentModeCount, 0);
		if(presentModeCount != 0){
			activeSwapchain.supportDetails.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surfaces[renderActiveSurface], &presentModeCount, activeSwapchain.supportDetails.presentModes.data);
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
		vkGetPhysicalDeviceFormatProperties(physicalDevice, formats[i], &props);
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
		if(activeSwapchain.attachments.colorImage){
			vkDestroyImageView(device, activeSwapchain.attachments.colorImageView, allocator);
			vkDestroyImage(device, activeSwapchain.attachments.colorImage, allocator);
			vkFreeMemory(device, activeSwapchain.attachments.colorImageMemory, allocator);
		}
		VkFormat colorFormat = activeSwapchain.surfaceFormat.format;
		CreateImage(activeSwapchain.width, activeSwapchain.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, 
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, activeSwapchain.attachments.colorImage, activeSwapchain.attachments.colorImageMemory);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE, (u64)activeSwapchain.attachments.colorImage, "Framebuffer color image");
		activeSwapchain.attachments.colorImageView = CreateImageView(activeSwapchain.attachments.colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)activeSwapchain.attachments.colorImageView, "Framebuffer color imageview");
	}
	
	{//depth framebuffer attachment
		if(activeSwapchain.attachments.depthImage){
			vkDestroyImageView(device, activeSwapchain.attachments.depthImageView, allocator);
			vkDestroyImage(device, activeSwapchain.attachments.depthImage, allocator);
			vkFreeMemory(device, activeSwapchain.attachments.depthImageMemory, allocator);
		}
		VkFormat depthFormat = findDepthFormat();
		CreateImage(activeSwapchain.width, activeSwapchain.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, 
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, activeSwapchain.attachments.depthImage, activeSwapchain.attachments.depthImageMemory);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE, (u64)activeSwapchain.attachments.depthImage, "Framebuffer depth image");
		activeSwapchain.attachments.depthImageView = CreateImageView(activeSwapchain.attachments.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)activeSwapchain.attachments.depthImageView, "Framebuffer depth imageview");
	}
	
	activeSwapchain.frames.resize(activeSwapchain.imageCount);
	for(u32 i = 0; i < activeSwapchain.imageCount; ++i){
		//set the frame images to the swap chain images
		//NOTE the previous image and its memory gets freed when the swapchain gets destroyed
		activeSwapchain.frames[i].image = images[i];
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE, (u64)activeSwapchain.frames[i].image, ToString("Frame image ", i).str);
		
		//create the image views
		if(activeSwapchain.frames[i].imageView) vkDestroyImageView(device, activeSwapchain.frames[i].imageView, allocator);
		activeSwapchain.frames[i].imageView = CreateImageView(activeSwapchain.frames[i].image, activeSwapchain.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)activeSwapchain.frames[i].imageView, ToString("Frame imageview ", i).str);
		
		//create the framebuffers
		if(activeSwapchain.frames[i].framebuffer) vkDestroyFramebuffer(device, activeSwapchain.frames[i].framebuffer, allocator);
		
		array<VkImageView> frameBufferAttachments(deshi_temp_allocator); //TODO(delle) fix scuffed msaa hack
		if(msaaSamples != VK_SAMPLE_COUNT_1_BIT){
			frameBufferAttachments = { activeSwapchain.attachments.colorImageView, activeSwapchain.attachments.depthImageView, activeSwapchain.frames[i].imageView };
		}else{
			frameBufferAttachments = { activeSwapchain.frames[i].imageView, activeSwapchain.attachments.depthImageView, };
		}
		
		VkFramebufferCreateInfo info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		info.renderPass      = renderPass;
		info.attachmentCount = frameBufferAttachments.count;
		info.pAttachments    = frameBufferAttachments.data;
		info.width           = activeSwapchain.width;
		info.height          = activeSwapchain.height;
		info.layers          = 1;
		resultVk = vkCreateFramebuffer(device, &info, allocator, &activeSwapchain.frames[i].framebuffer); AssertVk(resultVk, "failed to create framebuffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_FRAMEBUFFER, (u64)activeSwapchain.frames[i].framebuffer, ToString("Frame framebuffer ", i).str);
		
		//allocate command buffers
		if(activeSwapchain.frames[i].commandBuffer) vkFreeCommandBuffers(device, commandPool, 1, &activeSwapchain.frames[i].commandBuffer);
		VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		resultVk = vkAllocateCommandBuffers(device, &allocInfo, &activeSwapchain.frames[i].commandBuffer); AssertVk(resultVk, "failed to allocate command buffer");
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (u64)activeSwapchain.frames[i].commandBuffer, ToString("Frame command buffer ", i).str);
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
		uboVS.values.screen = vec2((f32)activeSwapchain.extent.width, (f32)activeSwapchain.extent.height);
		uboVS.values.mousepos = input_mouse_position();
		if(initialized) uboVS.values.mouseWorld = Math::ScreenToWorld(input_mouse_position(), uboVS.values.proj, uboVS.values.view, DeshWindow->dimensions);
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

//NOTE(delle) this does not free the shaderc_compilation_result_t, use shaderc_result_release() to do so
local shaderc_compilation_result_t
compile_shader(str8 source, str8 name, str8 front, str8 ext){DPZoneScoped;
	PrintVk(4, "Compiling shader: ",name);
	Stopwatch t_s = start_stopwatch();
	
	//try to compile from GLSL to SPIR-V binary
	shaderc_compilation_result_t result;
	if      (str8_equal_lazy(ext, str8_lit("vert"))){
		result = shaderc_compile_into_spv(shader_compiler, (const char*)source.str, source.count, shaderc_glsl_vertex_shader,
										  (const char*)name.str, "main", shader_compiler_options);
	}else if(str8_equal_lazy(ext, str8_lit("frag"))){
		result = shaderc_compile_into_spv(shader_compiler, (const char*)source.str, source.count, shaderc_glsl_fragment_shader,
										  (const char*)name.str, "main", shader_compiler_options);
	}else if(str8_equal_lazy(ext, str8_lit("geom"))){
		result = shaderc_compile_into_spv(shader_compiler, (const char*)source.str, source.count, shaderc_glsl_geometry_shader,
										  (const char*)name.str, "main", shader_compiler_options);
	}else{ return 0; }
	
	//check for errors
	if(!result){ 
		LogE("vulkan",name,": Shader compiler returned a null result");
		return 0; 
	}
	if(shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success){
		LogE("vulkan",shaderc_result_get_error_message(result)); 
		return 0;
	}
	
	//create or overwrite .spv files
	file_write_simple(str8_concat3(str8_lit("data/shaders/"),name,str8_lit(".spv"), deshi_temp_allocator),
					  (void*)shaderc_result_get_bytes(result), shaderc_result_get_length(result));
	PrintVk(5, "Finished compiling shader '",name,"' in ",peek_stopwatch(t_s),"ms");
	return result;
}

//TODO(delle) maybe just save the shader module to disk and load that instead of .spv (since openGL can't load vulkan format .spv)?
local VkPipelineShaderStageCreateInfo
load_shader(str8 name, VkShaderStageFlagBits stage){DPZoneScoped;
	PrintVk(3, "Loading shader: ",name);
	Stopwatch t_s = start_stopwatch();
	str8 path = str8_concat(str8_lit("data/shaders/"), name, deshi_temp_allocator);
	
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
				DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SHADER_MODULE, (u64)shaderModule, ToString("Shader Module ",name).str);
				
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
	if(!shader_source) return VkPipelineShaderStageCreateInfo{};
	
	shaderc_compilation_result_t compiled = compile_shader(shader_source, shader_file.name, shader_file.front, shader_file.ext);
	if(!compiled) return VkPipelineShaderStageCreateInfo{};
	defer{ shaderc_result_release(compiled); };
	
	VkShaderModule shaderModule{};
	VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	moduleInfo.codeSize = shaderc_result_get_length(compiled);
	moduleInfo.pCode    = (u32*)shaderc_result_get_bytes(compiled);
	resultVk = vkCreateShaderModule(device, &moduleInfo, allocator, &shaderModule); AssertVk(resultVk, "failed to create shader module");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SHADER_MODULE, (u64)shaderModule, ToString("Shader Module ",name).str);
	
	VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	shaderStage.stage  = stage;
	shaderStage.pName  = "main";
	shaderStage.module = shaderModule;
	PrintVk(5, "Finished loading shader '",name,"' in ",peek_stopwatch(t_s),"ms");
	return shaderStage;
}

local VkPipelineShaderStageCreateInfo
CompileAndLoadShader(str8 name, VkShaderStageFlagBits stage){DPZoneScoped;
	PrintVk(3, "Compiling and loading shader: ",name);
	str8 path = str8_concat(str8_lit("data/shaders/"), name, deshi_temp_allocator);
	
	//load shader source
	File shader_file = file_info(path);
	if(!shader_file.creation_time) return VkPipelineShaderStageCreateInfo{};
	
	str8 shader_source = file_read_simple(path, deshi_temp_allocator);
	if(!shader_source) return VkPipelineShaderStageCreateInfo{};
	
	//compile shader source
	shaderc_compilation_result_t compiled = compile_shader(shader_source, shader_file.name, shader_file.front, shader_file.ext);
	if(!compiled) return VkPipelineShaderStageCreateInfo{};
	defer{ shaderc_result_release(compiled); };
	
	//create shader module
	VkShaderModule shaderModule{};
	VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	moduleInfo.codeSize = shaderc_result_get_length(compiled);
	moduleInfo.pCode    = (u32*)shaderc_result_get_bytes(compiled);
	resultVk = vkCreateShaderModule(device, &moduleInfo, allocator, &shaderModule); AssertVk(resultVk, "failed to create shader module");
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_SHADER_MODULE, (u64)shaderModule, ToString("Shader Module ", name).str);
	
	//setup shader stage create info
	VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	shaderStage.stage  = stage;
	shaderStage.pName  = "main";
	shaderStage.module = shaderModule;
	return shaderStage;
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
	vertexInputBindings = { //binding:u32, stride:u32, inputRate:VkVertexInputRate
		{0, sizeof(Mesh::Vertex), VK_VERTEX_INPUT_RATE_VERTEX},
	};
	vertexInputAttributes = { //location:u32, binding:u32, format:VkFormat, offset:u32
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Mesh::Vertex, pos)},
		{1, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Mesh::Vertex, uv)},
		{2, 0, VK_FORMAT_R8G8B8A8_UNORM,   offsetof(Mesh::Vertex, color)},
		{3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Mesh::Vertex, normal)},
	};
	vertexInputState.vertexBindingDescriptionCount   = (u32)vertexInputBindings.count;
	vertexInputState.pVertexBindingDescriptions      = vertexInputBindings.data;
	vertexInputState.vertexAttributeDescriptionCount = (u32)vertexInputAttributes.count;
	vertexInputState.pVertexAttributeDescriptions    = vertexInputAttributes.data;
	
	twodVertexInputBindings = { //binding:u32, stride:u32, inputRate:VkVertexInputRate
		{0, sizeof(Vertex2), VK_VERTEX_INPUT_RATE_VERTEX},
	};
	twodVertexInputAttributes = { //location:u32, binding:u32, format:VkFormat, offset:u32
		{0, 0, VK_FORMAT_R32G32_SFLOAT,  offsetof(Vertex2, pos)},
		{1, 0, VK_FORMAT_R32G32_SFLOAT,  offsetof(Vertex2, uv)},
		{2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(Vertex2, color)},
	};
	twodVertexInputState.vertexBindingDescriptionCount   = (u32)twodVertexInputBindings.count;
	twodVertexInputState.pVertexBindingDescriptions      = twodVertexInputBindings.data;
	twodVertexInputState.vertexAttributeDescriptionCount = (u32)twodVertexInputAttributes.count;
	twodVertexInputState.pVertexAttributeDescriptions    = twodVertexInputAttributes.data;
	
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
	
	//container struct for color blend attachments with overall blending constants
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
	dynamicState.dynamicStateCount = (u32)dynamicStates.count;
	dynamicState.pDynamicStates    = dynamicStates.data;
	
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
		pipelineCreateInfo.flags              = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCreateInfo.basePipelineIndex  = -1;
		
		shaderStages[0] = load_shader(str8_lit("base.vert"), VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = load_shader(str8_lit("base.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCreateInfo.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.base); AssertVk(resultVk);
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
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.selected); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.selected, "Selected pipeline");
		
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		depthStencilState.depthTestEnable = VK_TRUE;
	}
	
	{//null pipeline
		shaderStages[0] = load_shader(str8_lit("null.vert"), VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = load_shader(str8_lit("null.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.null); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.null, "Null pipeline");
	}
	
	{//flat pipeline
		shaderStages[0] = load_shader(str8_lit("flat.vert"), VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = load_shader(str8_lit("flat.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.flat); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.flat, "Flat pipeline");
	}
	
	{//phong
		shaderStages[0] = load_shader(str8_lit("phong.vert"), VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = load_shader(str8_lit("phong.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.phong); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.phong, "Phong pipeline");
	}
	
	{//2d
		pipelineCreateInfo.pVertexInputState = &twodVertexInputState;
		pipelineCreateInfo.layout            = pipelineLayouts.twod;
		rasterizationState.cullMode  = VK_CULL_MODE_NONE;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		depthStencilState.depthTestEnable = VK_FALSE;
		
		shaderStages[0] = load_shader(str8_lit("twod.vert"), VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = load_shader(str8_lit("twod.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.twod); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.twod, "2D pipeline");
		
		{//ui
			shaderStages[0] = load_shader(str8_lit("ui.vert"), VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = load_shader(str8_lit("ui.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
			shaderStages[1].pSpecializationInfo = &specializationInfo;
			pipelineCreateInfo.stageCount = 2;
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.ui);
			DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.ui, "UI pipeline");
		}
		
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
		pipelineCreateInfo.layout            = pipelineLayouts.base;
		rasterizationState.cullMode  = VK_CULL_MODE_BACK_BIT;
		rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
		depthStencilState.depthTestEnable = VK_TRUE;
	}
	
	{//pbr
		shaderStages[0] = load_shader(str8_lit("pbr.vert"), VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = load_shader(str8_lit("pbr.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.pbr); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.pbr, "PBR pipeline");
	}
	
	//wireframe
	if(deviceFeatures.fillModeNonSolid){
		colorBlendAttachmentState.blendEnable = VK_FALSE;
		rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
		rasterizationState.cullMode    = VK_CULL_MODE_NONE;
		depthStencilState.depthTestEnable = VK_FALSE;
		
		shaderStages[0] = load_shader(str8_lit("wireframe.vert"), VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = load_shader(str8_lit("wireframe.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCreateInfo.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.wireframe); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.wireframe, "Wireframe pipeline");
		
		{//wireframe with depth test
			depthStencilState.depthTestEnable = VK_TRUE;
			
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.wireframe_depth); AssertVk(resultVk);
			DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.wireframe_depth, "Wireframe Depth pipeline");
			
			depthStencilState.depthTestEnable = VK_FALSE;
		}
		
		{ //collider gets a specific colored wireframe
			colorBlendAttachmentState.blendEnable         = VK_TRUE;
			colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
			colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
			
			colorBlendState.blendConstants[0] = (f32)renderSettings.colliderColor.r;
			colorBlendState.blendConstants[1] = (f32)renderSettings.colliderColor.g;
			colorBlendState.blendConstants[2] = (f32)renderSettings.colliderColor.b;
			colorBlendState.blendConstants[3] = (f32)renderSettings.colliderColor.a;
			
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.collider); AssertVk(resultVk);
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
		shaderStages[0] = load_shader(str8_lit("lavalamp.vert"), VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = load_shader(str8_lit("lavalamp.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.lavalamp); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.lavalamp, "Lavalamp pipeline");
	}
	
	{//offscreen
		colorBlendState.attachmentCount = 0; //no color attachments used
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; //cull front faces
		rasterizationState.depthBiasEnable = VK_TRUE; //enable depth bias
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.sampleShadingEnable  = VK_FALSE;
		dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS, };
		dynamicState.dynamicStateCount = (u32)dynamicStates.count; //add depth bias to dynamic state so
		dynamicState.pDynamicStates    = dynamicStates.data;       //it can be changed at runtime
		pipelineCreateInfo.renderPass = offscreen.renderpass;
		
		shaderStages[0] = load_shader(str8_lit("offscreen.vert"), VK_SHADER_STAGE_VERTEX_BIT);
		pipelineCreateInfo.stageCount = 1;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.offscreen); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.offscreen, "Offscreen pipeline");
		
		colorBlendState.attachmentCount = 1;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		rasterizationState.depthBiasEnable = VK_FALSE;
		multisampleState.rasterizationSamples = msaaSamples;
		multisampleState.sampleShadingEnable  = (msaaSamples != VK_SAMPLE_COUNT_1_BIT) ? VK_TRUE : VK_FALSE;
		dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, };
		dynamicState.dynamicStateCount = (u32)dynamicStates.count;
		dynamicState.pDynamicStates    = dynamicStates.data;
		pipelineCreateInfo.renderPass = renderPass;
	}
	
	//NOTE(delle) testing/debug shaders should be removed on release
	{//testing0
		shaderStages[0] = load_shader(str8_lit("testing0.vert"), VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = load_shader(str8_lit("testing0.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.testing0); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.testing0, "Testing0 pipeline");
	}
	
	{//testing1
		shaderStages[0] = load_shader(str8_lit("testing1.vert"), VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = load_shader(str8_lit("testing1.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		pipelineCreateInfo.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.testing1); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.testing1, "Testing1 pipeline");
	}
	
	//DEBUG mesh normals
	if(enabledFeatures.geometryShader){
		pipelineCreateInfo.layout = pipelineLayouts.geometry;
		
		shaderStages[0] = load_shader(str8_lit("nothing.vert"), VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = load_shader(str8_lit("nothing.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[2] = load_shader(str8_lit("normaldebug.geom"), VK_SHADER_STAGE_GEOMETRY_BIT);
		pipelineCreateInfo.stageCount = 3;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.normals_debug); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.normals_debug, "DEBUG Mesh normals pipeline");
		
		pipelineCreateInfo.layout = pipelineLayouts.base;
	}
	
	{//DEBUG shadow map
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		VkPipelineVertexInputStateCreateInfo emptyVertexInputState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
		pipelineCreateInfo.pVertexInputState = &emptyVertexInputState;
		
		shaderStages[0] = load_shader(str8_lit("shadowmapDEBUG.vert"), VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = load_shader(str8_lit("shadowmapDEBUG.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCreateInfo.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, allocator, &pipelines.shadowmap_debug); AssertVk(resultVk);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_PIPELINE, (u64)pipelines.shadowmap_debug, "DEBUG Shadowmap pipeline");
		
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
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
		case(Shader_Lavalamp):    { return pipelines.lavalamp;  }
		case(Shader_Testing0):    { return pipelines.testing0;  }
		case(Shader_Testing1):    { return pipelines.testing1;  }
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
	size_t ui_vb_size = Max(1000*sizeof(Vertex2),         renderTwodVertexCount * sizeof(Vertex2));
	size_t ui_ib_size = Max(3000*sizeof(RenderTwodIndex), renderTwodIndexCount  * sizeof(RenderTwodIndex));
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
	
	//create temp mesh vertex and index buffers
	size_t temp_wire_vb_size = renderTempWireframeVertexCount*sizeof(Mesh::Vertex);
	size_t temp_fill_vb_size = renderTempFilledVertexCount*sizeof(Mesh::Vertex);
	size_t temp_wire_ib_size = renderTempWireframeIndexCount*sizeof(RenderTempIndex);
	size_t temp_fill_ib_size = renderTempFilledIndexCount*sizeof(RenderTempIndex);
	size_t temp_vb_size = temp_wire_vb_size+temp_fill_vb_size;
	size_t temp_ib_size = temp_wire_ib_size+temp_fill_ib_size;
	if(tempVertexBuffer.size == 0) temp_vb_size = 1000*sizeof(Mesh::Vertex);
	if(tempIndexBuffer.size == 0)  temp_ib_size = 3000*sizeof(RenderTempIndex);
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
	
	//create debug mesh vertex and index buffers
	size_t debug_wire_vb_size = renderDebugWireframeVertexCount*sizeof(Mesh::Vertex);
	size_t debug_fill_vb_size = renderDebugFilledVertexCount*sizeof(Mesh::Vertex);
	size_t debug_wire_ib_size = renderDebugWireframeIndexCount*sizeof(RenderTempIndex);
	size_t debug_fill_ib_size = renderDebugFilledIndexCount*sizeof(RenderTempIndex);
	size_t debug_vb_size = debug_wire_vb_size+debug_fill_vb_size;
	size_t debug_ib_size = debug_wire_ib_size+debug_fill_ib_size;
	if(debugVertexBuffer.size == 0) debug_vb_size = 1000*sizeof(Mesh::Vertex);
	if(debugIndexBuffer.size == 0)  debug_ib_size = 3000*sizeof(RenderTempIndex);
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
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @vk_funcs_commands_build
local vec4 render_pass_color = vec4(0.78f, 0.54f, 0.12f, 1.0f);
local vec4 draw_group_color  = vec4(0.50f, 0.76f, 0.34f, 1.0f);
local vec4 draw_cmd_color    = vec4(0.40f, 0.61f, 0.27f, 1.0f);

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
			
			DebugBeginLabelVk(cmdBuffer, "Meshes", draw_group_color);
			VkDeviceSize offsets[1] = {0}; //reset vertex buffer offsets
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &meshVertexBuffer.buffer, offsets);
			vkCmdBindIndexBuffer(cmdBuffer, meshIndexBuffer.buffer, 0, INDEX_TYPE_VK_MESH);
			forI(renderModelCmdCount){
				RenderModelCmd& cmd = renderModelCmdArray[i];
				MaterialVk& mat = vkMaterials[cmd.material];
				DebugInsertLabelVk(cmdBuffer, cmd.name, draw_cmd_color);
				vkCmdPushConstants(cmdBuffer, pipelineLayouts.base, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &cmd.matrix);
				vkCmdDrawIndexed(cmdBuffer, cmd.indexCount, 1, cmd.indexOffset, cmd.vertexOffset, 0);
				renderStats.drawnIndices += cmd.indexCount;
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
				vkCmdDrawIndexed(cmdBuffer, cmd.indexCount, 1, cmd.indexOffset, cmd.vertexOffset, 0);
				renderStats.drawnIndices += cmd.indexCount;
				
				//wireframe overlay
				if(renderSettings.meshWireframes){
					vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe);
					vkCmdDrawIndexed(cmdBuffer, cmd.indexCount, 1, cmd.indexOffset, cmd.vertexOffset, 0);
					renderStats.drawnIndices += cmd.indexCount;
				}
			}
			DebugEndLabelVk(cmdBuffer);
			
			//draw mesh normals overlay
			if(enabledFeatures.geometryShader && renderSettings.meshNormals){
				DebugBeginLabelVk(cmdBuffer, "Debug Normals", draw_group_color);
				forI(renderModelCmdCount){
					RenderModelCmd& cmd = renderModelCmdArray[i];
					MaterialVk& mat = vkMaterials[cmd.material];
					DebugInsertLabelVk(cmdBuffer, cmd.name, draw_cmd_color);
					vkCmdPushConstants(cmdBuffer, pipelineLayouts.geometry, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(mat4), &cmd.matrix);
					vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.normals_debug);
					vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.geometry, 0, 1, &descriptorSets.geometry, 0, 0);
					vkCmdDrawIndexed(cmdBuffer, cmd.indexCount, 1, cmd.indexOffset, cmd.vertexOffset, 0);
					renderStats.drawnIndices += cmd.indexCount;
				}
				DebugEndLabelVk(cmdBuffer);
			}
			
			//draw temporary stuff
			if(renderTempWireframeVertexCount > 0 && renderTempWireframeIndexCount > 0){
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
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.selected);
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
			
			//draw twod stuff
			if(renderTwodVertexCount > 0 && renderTwodIndexCount > 0){
				DebugBeginLabelVk(cmdBuffer, "UI", draw_group_color);
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.ui);
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
						if(renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].indexCount == 0) continue;
						
						scissor.offset.x = (u32)renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].scissorOffset.x;
						scissor.offset.y = (u32)renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].scissorOffset.y;
						scissor.extent.width  = (u32)renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].scissorExtent.x;
						scissor.extent.height = (u32)renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].scissorExtent.y;
						vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
						
						if(renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].handle){
							vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.twod, 0, 1,
													&(VkDescriptorSet)renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].handle, 0, 0);
							vkCmdDrawIndexed(cmdBuffer, renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].indexCount, 1,
											 renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].indexOffset, 0, 0);
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
				DebugBeginLabelVk(cmdBuffer, "Z-Zero", draw_group_color);
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
					scissor.offset.x = (u32)renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].scissorOffset.x;
					scissor.offset.y = (u32)renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].scissorOffset.y;
					scissor.extent.width = (u32)renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].scissorExtent.x;
					scissor.extent.height = (u32)renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].scissorExtent.y;
					vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
					
					if(renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].handle){
						vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.twod, 0, 1, &(VkDescriptorSet)renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].handle, 0, 0);
						vkCmdDrawIndexed(cmdBuffer, renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].indexCount, 1, renderTwodCmdArrays[renderActiveSurface][TWOD_LAYERS][cmd_idx].indexOffset, 0, 0);
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
//// @vk_funcs_imgui
void DeshiImGui::
Init(){DPZoneScoped;
	DeshiStageInitStart(DS_IMGUI, DS_RENDER, "Attempted to initialize ImGui module before initializing Render module");
	
	//Setup Dear ImGui context
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = "data/cfg/imgui.ini";
	
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
	init_info.MinImageCount   = activeSwapchain.minImageCount;
	init_info.ImageCount      = activeSwapchain.imageCount;
	init_info.CheckVkResultFn = [](VkResult result){ AssertVk(result, "imgui vulkan error"); };
	init_info.MSAASamples     = msaaSamples;
	ImGui_ImplVulkan_Init(&init_info, renderPass);
	
	//Upload Fonts
	VkCommandPool   command_pool   = commandPool;
	VkCommandBuffer command_buffer = activeSwapchain.frames[activeSwapchain.frameIndex].commandBuffer;
	
	resultVk = vkResetCommandPool(device, command_pool, 0); AssertVk(resultVk);
	
	VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	resultVk = vkBeginCommandBuffer(command_buffer, &begin_info); AssertVk(resultVk);
	ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
	
	VkSubmitInfo end_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	end_info.commandBufferCount = 1;
	end_info.pCommandBuffers    = &command_buffer;
	resultVk = vkEndCommandBuffer(command_buffer); AssertVk(resultVk);
	
	resultVk = vkQueueSubmit(graphicsQueue, 1, &end_info, VK_NULL_HANDLE); AssertVk(resultVk);
	
	resultVk = vkDeviceWaitIdle(device); AssertVk(resultVk);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
	
	DeshiStageInitEnd(DS_IMGUI);
}

void DeshiImGui::
Cleanup(){DPZoneScoped;
	resultVk = vkDeviceWaitIdle(device); AssertVk(resultVk);
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
NewFrame(){DPZoneScoped;
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
		validationFeaturesEnabled.add(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
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
	
	initialized = true;
	DeshiStageInitEnd(DS_RENDER);
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
			if(activeSwapchain.width <= 0 || activeSwapchain.height <= 0){ ImGui::EndFrame(); return; }
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
		if(DeshiModuleLoaded(DS_IMGUI)){
			ImGui::Render();
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
	mvk.vertexCount = mesh->vertexCount;
	mvk.indexCount = mesh->indexCount;
	if(vkMeshes.count){
		mvk.vertexOffset = vkMeshes.last->vertexOffset + vkMeshes.last->vertexCount;
		mvk.indexOffset = vkMeshes.last->indexOffset + vkMeshes.last->indexCount;
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
	if(meshIndexBuffer.buffer  == VK_NULL_HANDLE || meshIndexBuffer.size  < total_ib_size){
		CreateOrResizeBuffer(&meshIndexBuffer,  total_ib_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		DebugSetObjectNameVk(device, VK_OBJECT_TYPE_BUFFER, (u64)meshIndexBuffer.buffer,  "Mesh index buffer");
	}
	
	//copy memory to the GPU
	void* vb_data; void* ib_data;
	resultVk = vkMapMemory(device, meshVertexBuffer.memory, mesh_vb_offset, mesh_vb_size, 0, &vb_data); AssertVk(resultVk);
	resultVk = vkMapMemory(device, meshIndexBuffer.memory,  mesh_ib_offset, mesh_ib_size, 0, &ib_data); AssertVk(resultVk);
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
		resultVk = vkFlushMappedMemoryRanges(device, 2, range); AssertVk(resultVk);
	}
	vkUnmapMemory(device, meshVertexBuffer.memory);
	vkUnmapMemory(device, meshIndexBuffer.memory);
	
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
	resultVk = vkCreateImage(device, &imageInfo, allocator, &tvk.image); AssertVk(resultVk, "failed to create image");
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, tvk.image, &memRequirements);
	VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	resultVk = vkAllocateMemory(device, &allocInfo, allocator, &tvk.memory); AssertVk(resultVk, "failed to allocate image memory");
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
	samplerInfo.anisotropyEnable = renderSettings.anistropicFiltering;
	samplerInfo.maxAnisotropy    = (renderSettings.anistropicFiltering) ?  physicalDeviceProperties.limits.maxSamplerAnisotropy : 1.0f;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable    = VK_FALSE;
	samplerInfo.compareOp        = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipLodBias       = 0.0f;
	samplerInfo.minLod           = 0.0f;
	samplerInfo.maxLod           = (f32)texture->mipmaps;
	resultVk = vkCreateSampler(device, &samplerInfo, 0, &tvk.sampler); AssertVk(resultVk, "failed to create texture sampler");
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
	resultVk = vkAllocateDescriptorSets(device, &setAllocInfo, &tvk.descriptorSet); AssertVk(resultVk);
	DebugSetObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)tvk.descriptorSet,
						 toStr("Texture descriptor set ", texture->name).str);
	
	VkWriteDescriptorSet set{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	set.dstSet = tvk.descriptorSet;
	set.dstArrayElement = 0;
	set.descriptorCount = 1;
	set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	set.pImageInfo = &tvk.descriptor;
	set.dstBinding = 0;
	
	vkUpdateDescriptorSets(device, 1, &set, 0, 0);
	
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
	vkUpdateDescriptorSets(device, sets.size(), sets.data, 0, 0);
	
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
		vkUpdateDescriptorSets(device, sets.size(), sets.data, 0, 0);
	}
	
	vkMaterials.add(mvk);
}

void
render_update_material(Material* material){DPZoneScoped;
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
	vkUpdateDescriptorSets(device, sets.size(), sets.data, 0, 0);
	
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
	Assert(renderModelCmdCount + model->batches.size() < MAX_MODEL_CMDS, "attempted to draw more than the global maximum number of batches");
	RenderModelCmd* cmd = renderModelCmdArray + renderModelCmdCount;
	forI(model->batches.size()){
		if(!model->batches[i].indexCount) continue;
		cmd[i].vertexOffset = vkMeshes[model->mesh->idx].vertexOffset;
		cmd[i].indexOffset  = vkMeshes[model->mesh->idx].indexOffset + model->batches[i].indexOffset;
		cmd[i].indexCount   = model->batches[i].indexCount;
		cmd[i].material     = model->batches[i].material;
		cmd[i].name         = model->name;
		cmd[i].matrix       = *matrix;
		renderModelCmdCount += 1;
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_draw_2d
void
render_start_cmd2(u32 layer, Texture* texture, vec2 scissorOffset, vec2 scissorExtent){DPZoneScoped;
	renderActiveLayer = layer;
	if(   (renderTwodCmdCounts[renderActiveSurface][layer] == 0)
	   || (renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]-1].handle        != textures[(texture) ? texture->idx : 1].descriptorSet)
	   || (renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]-1].scissorOffset != scissorOffset)
	   || (renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]-1].scissorExtent != scissorExtent)){
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].handle        = textures[(texture) ? texture->idx : 1].descriptorSet;
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].indexOffset   = renderTwodIndexCount;
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].scissorOffset = scissorOffset;
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].scissorExtent = scissorExtent;
		renderTwodCmdCounts[renderActiveSurface][layer] += 1;
	}
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
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaces[window->index], &formatCount, 0);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surfaces[window->index], &presentModeCount, 0);
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, physicalQueueFamilies.presentFamily.value, surfaces[window->index], &presentSupport);
	
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
	vkLights[idx] = vec4(position, brightness);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_camera
void
render_update_camera_position(vec3 position){DPZoneScoped;
	uboVS.values.viewPos = vec4(position, 1.f);
}

void
render_update_camera_view(mat4* view_matrix){DPZoneScoped;
	uboVS.values.view = *view_matrix;
}

void
render_update_camera_projection(mat4* projection_matrix){DPZoneScoped;
	uboVS.values.proj = *projection_matrix;
}

void
render_use_default_camera(){DPZoneScoped;
	uboVS.values.view = Math::LookAtMatrix(vec3::ZERO, (vec3::FORWARD * mat4::RotationMatrix(vec3::ZERO)).normalized()).Inverse();
	uboVS.values.proj = Camera::MakePerspectiveProjectionMatrix((f32)DeshWindow->width, (f32)DeshWindow->height, 90.f, 1000.f, 0.1f);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shaders
void
render_reload_shader(u32 shader){DPZoneScoped;
	switch(shader){
		case(Shader_NULL):{ 
			vkDestroyPipeline(device, pipelines.null, 0);
			shaderStages[0] = CompileAndLoadShader(str8_lit("null.vert"), VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader(str8_lit("null.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.null); AssertVk(resultVk);
		}break;
		case(Shader_Flat):{ 
			vkDestroyPipeline(device, pipelines.flat, 0);
			shaderStages[0] = CompileAndLoadShader(str8_lit("flat.vert"), VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader(str8_lit("flat.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.flat); AssertVk(resultVk, "failed to create flat graphics pipeline");
		}break;
		case(Shader_Wireframe):{
			if(deviceFeatures.fillModeNonSolid){
				vkDestroyPipeline(device, pipelines.wireframe, 0);
				rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
				rasterizationState.cullMode = VK_CULL_MODE_NONE;
				depthStencilState.depthTestEnable = VK_FALSE;
				shaderStages[0] = CompileAndLoadShader(str8_lit("wireframe.vert"), VK_SHADER_STAGE_VERTEX_BIT);
				shaderStages[1] = CompileAndLoadShader(str8_lit("wireframe.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
				pipelineCreateInfo.stageCount = 2;
				resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.wireframe); AssertVk(resultVk, "failed to create wireframe graphics pipeline");
				rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
				rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
				depthStencilState.depthTestEnable = VK_TRUE;
			}
		}break;
		case(Shader_Phong):{
			vkDestroyPipeline(device, pipelines.phong, 0);
			shaderStages[0] = CompileAndLoadShader(str8_lit("phong.vert"), VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader(str8_lit("phong.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.phong); AssertVk(resultVk, "failed to create phong graphics pipeline");
		}break;
		case(Shader_PBR):{ 
			vkDestroyPipeline(device, pipelines.pbr, 0);
			shaderStages[0] = CompileAndLoadShader(str8_lit("pbr.vert"), VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader(str8_lit("pbr.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.pbr); AssertVk(resultVk, "failed to create pbr graphics pipeline");
		}break;
		case(Shader_Lavalamp):{ 
			vkDestroyPipeline(device, pipelines.lavalamp, 0);
			shaderStages[0] = CompileAndLoadShader(str8_lit("lavalamp.vert"), VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader(str8_lit("lavalamp.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.lavalamp); AssertVk(resultVk, "failed to create lavalamp graphics pipeline");
		}break;
		case(Shader_Testing0):{ 
			vkDestroyPipeline(device, pipelines.testing0, 0);
			shaderStages[0] = CompileAndLoadShader(str8_lit("testing0.vert"), VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader(str8_lit("testing0.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.testing0); AssertVk(resultVk, "failed to create testing0 graphics pipeline");
		}break;
		case(Shader_Testing1):{ 
			vkDestroyPipeline(device, pipelines.testing1, 0);
			shaderStages[0] = CompileAndLoadShader(str8_lit("testing1.vert"), VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader(str8_lit("testing1.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineCreateInfo.stageCount = 2;
			resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.testing1); AssertVk(resultVk, "failed to create testing1 graphics pipeline");
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
	shaderStages[0] = CompileAndLoadShader(str8_lit("null.vert"), VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = CompileAndLoadShader(str8_lit("null.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineCreateInfo.stageCount = 2;
	resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.null); AssertVk(resultVk);
	
	vkDestroyPipeline(device, pipelines.flat, 0);
	shaderStages[0] = CompileAndLoadShader(str8_lit("flat.vert"), VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = CompileAndLoadShader(str8_lit("flat.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineCreateInfo.stageCount = 2;
	resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.flat); AssertVk(resultVk, "failed to create flat graphics pipeline");
	
	if(deviceFeatures.fillModeNonSolid){
		vkDestroyPipeline(device, pipelines.wireframe, 0);
		rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		depthStencilState.depthTestEnable = VK_FALSE;
		shaderStages[0] = CompileAndLoadShader(str8_lit("wireframe.vert"), VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = CompileAndLoadShader(str8_lit("wireframe.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCreateInfo.stageCount = 2;
		resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.wireframe); AssertVk(resultVk, "failed to create wireframe graphics pipeline");
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		depthStencilState.depthTestEnable = VK_TRUE;
	}
	
	vkDestroyPipeline(device, pipelines.phong, 0);
	shaderStages[0] = CompileAndLoadShader(str8_lit("phong.vert"), VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = CompileAndLoadShader(str8_lit("phong.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineCreateInfo.stageCount = 2;
	resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.phong); AssertVk(resultVk, "failed to create phong graphics pipeline");
	
	vkDestroyPipeline(device, pipelines.pbr, 0);
	shaderStages[0] = CompileAndLoadShader(str8_lit("pbr.vert"), VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = CompileAndLoadShader(str8_lit("pbr.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineCreateInfo.stageCount = 2;
	resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.pbr); AssertVk(resultVk, "failed to create pbr graphics pipeline");
	
	vkDestroyPipeline(device, pipelines.lavalamp, 0);
	shaderStages[0] = CompileAndLoadShader(str8_lit("lavalamp.vert"), VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = CompileAndLoadShader(str8_lit("lavalamp.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineCreateInfo.stageCount = 2;
	resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.lavalamp); AssertVk(resultVk, "failed to create lavalamp graphics pipeline");
	
	vkDestroyPipeline(device, pipelines.testing0, 0);
	shaderStages[0] = CompileAndLoadShader(str8_lit("testing0.vert"), VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = CompileAndLoadShader(str8_lit("testing0.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineCreateInfo.stageCount = 2;
	resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.testing0); AssertVk(resultVk, "failed to create testing0 graphics pipeline");
	
	vkDestroyPipeline(device, pipelines.testing1, 0);
	shaderStages[0] = CompileAndLoadShader(str8_lit("testing1.vert"), VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = CompileAndLoadShader(str8_lit("testing1.frag"), VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineCreateInfo.stageCount = 2;
	resultVk = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, 0, &pipelines.testing1); AssertVk(resultVk, "failed to create testing1 graphics pipeline");
	
	UpdateMaterialPipelines();
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_other
void
render_remake_offscreen(){DPZoneScoped;
	_remakeOffscreen = true;
}
