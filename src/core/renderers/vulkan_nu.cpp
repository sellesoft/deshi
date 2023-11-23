// When something happens that prevents the entire application from being
// able to continue. Only used in situations in which we can reasonably assume the user
// needs something to happen in order for the program to continue as a whole, such as 
// the initialization function.
#define VulkanFatal(...) do { LogE("graphics.vulkan", __FUNCTION__, "(): ", __VA_ARGS__); Assert(0); } while(0)
// When something happens that is fatal to a single operation, but not to the 
// entire application.
#define VulkanError(...) LogE("graphics.vulkan", __FUNCTION__, "(): ", __VA_ARGS__)
// When something happens that may cause odd behavoir, but we are able to continue.
#define VulkanWarning(...) LogW("graphics.vulkan", __FUNCTION__, "(): ", __VA_ARGS__)
// Normal but signifigant conditions. Things we'd probably want to know about if a user
// sends in some kinda logs.
#define VulkanNotice(...) Log("graphics.vulkan", __FUNCTION__, "(): ", __VA_ARGS__)
// General information about the functioning of the backend.
#define VulkanInfo(...) if(g_graphics->logging_level >= 1) Log("graphics.vulkan", __FUNCTION__, "(): ", __VA_ARGS__)
// Information useful when trying to figure out what's wrong with the backend
#define VulkanDebug(...) if(g_graphics->logging_level >= 2) Log("graphics.vulkan", __FUNCTION__, "(): ", __VA_ARGS__)
// Used to assert that the result of a vulkan function call succeeded
#define VulkanAssertVk(result, ...) if((result) != VK_SUCCESS) VulkanFatal(__VA_ARGS__)
// Used to assert a condition is true
#define VulkanAssert(result, ...) if(!result) VulkanFatal(__VA_ARGS__)

#define is_debugging g_graphics->debugging
#define primary_allocator g_graphics->allocators.primary
#define temp_allocator g_graphics->allocators.temp

// backend specific information stored on windows initialized with graphics
struct WindowInfo {
	VkSwapchainKHR swapchain;
	struct { // support_details
		VkSurfaceCapabilitiesKHR  capabilities;
		array<VkSurfaceFormatKHR> formats;
		array<VkPresentModeKHR>   present_modes;
	} support_details;
	VkSurfaceKHR       surface;
	VkSurfaceFormatKHR surface_format;
	VkPresentModeKHR   present_mode;
	VkExtent2D         extent;
	s32                min_image_count;
	u32                image_count;
	u32                frame_index;

	array<graphics::Framebuffer*> presentation_frames;
	graphics::CommandBuffer* command_buffer;
};

WindowInfo* window_infos;

local VkAllocationCallbacks      vk_allocator_ = {};
local VkAllocationCallbacks*     vk_allocator = &vk_allocator_;
local VkAllocationCallbacks      vk_temp_allocator_ = {};
local VkAllocationCallbacks*     vk_temp_allocator = &vk_temp_allocator_;
local VkInstance                 vk_instance = VK_NULL_HANDLE;
local VkPhysicalDevice           vk_physical_device = VK_NULL_HANDLE;
local VkPhysicalDeviceProperties vk_physical_device_properties = {};
local VkPhysicalDeviceFeatures   vk_physical_device_features = {};
local VkPhysicalDeviceFeatures   vk_physical_device_enabled_features = {};
local VkDebugUtilsMessengerEXT   vk_debug_messenger = VK_NULL_HANDLE;
local VkDevice                   vk_device = VK_NULL_HANDLE;
local VkQueue                    vk_graphics_queue = VK_NULL_HANDLE;
local VkQueue                    vk_present_queue = VK_NULL_HANDLE;

struct QueueFamilyIndexesX {
	b32 found_graphics_family;
	u32 graphics_family;
	b32 found_present_family;
	u32 present_family;
};

local QueueFamilyIndexesX physical_queue_families = {};

local VkSemaphore   vk_semaphore_image_acquired = VK_NULL_HANDLE;
local VkSemaphore   vk_semaphore_render_complete = VK_NULL_HANDLE;
local VkCommandPool vk_command_pool = VK_NULL_HANDLE;

local shaderc_compiler_t vk_shader_compiler;
local shaderc_compile_options_t vk_shader_compiler_options;

local VkDescriptorPool vk_descriptor_pool = VK_NULL_HANDLE;

local VkPipelineCache vk_pipeline_cache = VK_NULL_HANDLE;

local const char* validation_layers[] = {
	"VK_LAYER_KHRONOS_validation"
};

local char* device_extensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME
};

VkDebugUtilsMessageSeverityFlagsEXT vk_callback_severities = 
VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

VkDebugUtilsMessageTypeFlagsEXT vk_callback_types = 
VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @extension_functions


PFN_vkCmdBeginDebugUtilsLabelEXT vkfunc_vkCmdBeginDebugUtilsLabelEXT;
local inline void
vk_debug_begin_label(VkCommandBuffer command_buffer, const char* label_name, vec4 color){DPZoneScoped;
#ifdef BUILD_INTERNAL
	VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
	label.pLabelName = label_name;
	label.color[0]   = color.r;
	label.color[1]   = color.g;
	label.color[2]   = color.b;
	label.color[3]   = color.a;
	vkfunc_vkCmdBeginDebugUtilsLabelEXT(command_buffer, &label);
#endif //BUILD_INTERNAL
}

PFN_vkCmdEndDebugUtilsLabelEXT vkfunc_vkCmdEndDebugUtilsLabelEXT;
local inline void
vk_debug_end_label(VkCommandBuffer command_buffer){DPZoneScoped;
#ifdef BUILD_INTERNAL
	vkfunc_vkCmdEndDebugUtilsLabelEXT(command_buffer);
#endif //BUILD_INTERNAL
}

PFN_vkCmdInsertDebugUtilsLabelEXT vkfunc_vkCmdInsertDebugUtilsLabelEXT;
local inline void
vk_debug_insert_label(VkCommandBuffer command_buffer, const char* label_name, vec4 color){DPZoneScoped;
#ifdef BUILD_INTERNAL
	VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
	label.pLabelName = label_name;
	label.color[0]   = color.r;
	label.color[1]   = color.g;
	label.color[2]   = color.b;
	label.color[3]   = color.a;
	vkfunc_vkCmdInsertDebugUtilsLabelEXT(command_buffer, &label);
#endif //BUILD_INTERNAL
}

PFN_vkSetDebugUtilsObjectNameEXT vkfunc_vkSetDebugUtilsObjectNameEXT;
local inline void
vk_debug_set_object_name(VkDevice device, VkObjectType object_type, u64 object_handle, const char *object_name){DPZoneScoped;
#ifdef BUILD_INTERNAL
	if(!object_handle) return;
	VkDebugUtilsObjectNameInfoEXT nameInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
	nameInfo.objectType   = object_type;
	nameInfo.objectHandle = object_handle;
	nameInfo.pObjectName  = object_name;
	vkfunc_vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
#endif //BUILD_INTERNAL
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @helpers


VkShaderStageFlagBits
graphics_shader_stage_to_vulkan(GraphicsShaderStage x) {
	VkShaderStageFlags out = 0;
	if(HasFlag(x, GraphicsShaderStage_Vertex))   AddFlag(out, VK_SHADER_STAGE_VERTEX_BIT);
	if(HasFlag(x, GraphicsShaderStage_Fragment)) AddFlag(out, VK_SHADER_STAGE_FRAGMENT_BIT);
	if(HasFlag(x, GraphicsShaderStage_Geometry)) AddFlag(out, VK_SHADER_STAGE_GEOMETRY_BIT);
	if(HasFlag(x, GraphicsShaderStage_Compute))  AddFlag(out, VK_SHADER_STAGE_COMPUTE_BIT);
	return (VkShaderStageFlagBits)out;
}

VkFormat
find_supported_format(VkFormat* formats, u64 format_count, VkImageTiling tiling, VkFormatFeatureFlags features) {
	VulkanInfo("finding supported image formats.");
	forI(format_count){
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(vk_physical_device, formats[i], &props);
		if      (tiling == VK_IMAGE_TILING_LINEAR  && (props.linearTilingFeatures  & features) == features){
			return formats[i];
		}else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features){
			return formats[i];
		}
	}
	
	VulkanFatal("failed to find a supported format.");
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

VkFilter
graphics_filter_to_vulkan(GraphicsFilter x) {
	switch(x) {
		case GraphicsFilter_Nearest: return VK_FILTER_NEAREST;
		case GraphicsFilter_Linear:  return VK_FILTER_LINEAR;
	}
	VulkanFatal("invalid GraphicsFilter: ", (u32)x);
	return {};
}

VkImageType
graphics_image_type_to_vulkan(GraphicsImageType x) {
	switch(x) {
		case GraphicsImageType_1D: return VK_IMAGE_TYPE_1D;
		case GraphicsImageType_2D: return VK_IMAGE_TYPE_2D;
		case GraphicsImageType_3D: return VK_IMAGE_TYPE_3D;
	}
	Assert(0);
	return {};
}

VkImageUsageFlags
graphics_image_usage_to_vulkan(GraphicsImageUsage x) {
	VkImageUsageFlags out = 0;
	if(HasFlag(x, GraphicsImageUsage_Transfer_Source))          AddFlag(out, VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	if(HasFlag(x, GraphicsImageUsage_Transfer_Destination))     AddFlag(out, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	if(HasFlag(x, GraphicsImageUsage_Sampled))                  AddFlag(out, VK_IMAGE_USAGE_SAMPLED_BIT);
	if(HasFlag(x, GraphicsImageUsage_Storage))                  AddFlag(out, VK_IMAGE_USAGE_STORAGE_BIT);
	if(HasFlag(x, GraphicsImageUsage_Color_Attachment))         AddFlag(out, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	if(HasFlag(x, GraphicsImageUsage_Depth_Stencil_Attachment)) AddFlag(out, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	return out;
}

VkSampleCountFlagBits
graphics_sample_count_to_vulkan(GraphicsSampleCount x) {
	VkSampleCountFlags out = 0;
	if(HasFlag(x, GraphicsSampleCount_1))  AddFlag(out, VK_SAMPLE_COUNT_1_BIT);
	if(HasFlag(x, GraphicsSampleCount_2))  AddFlag(out, VK_SAMPLE_COUNT_2_BIT);
	if(HasFlag(x, GraphicsSampleCount_4))  AddFlag(out, VK_SAMPLE_COUNT_4_BIT);
	if(HasFlag(x, GraphicsSampleCount_8))  AddFlag(out, VK_SAMPLE_COUNT_8_BIT);
	if(HasFlag(x, GraphicsSampleCount_16)) AddFlag(out, VK_SAMPLE_COUNT_16_BIT);
	if(HasFlag(x, GraphicsSampleCount_32)) AddFlag(out, VK_SAMPLE_COUNT_32_BIT);
	if(HasFlag(x, GraphicsSampleCount_64)) AddFlag(out, VK_SAMPLE_COUNT_64_BIT);
	return (VkSampleCountFlagBits)out;
}

VkMemoryPropertyFlags
graphics_memory_properties_to_vulkan(GraphicsMemoryPropertyFlags x) {
	VkBufferUsageFlags usage_flags = 0;
	if(HasFlag(x, GraphicsBufferUsage_TransferSource))      usage_flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if(HasFlag(x, GraphicsBufferUsage_TransferDestination)) usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if(HasFlag(x, GraphicsBufferUsage_UniformTexelBuffer))  usage_flags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
	if(HasFlag(x, GraphicsBufferUsage_StorageTexelBuffer))  usage_flags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
	if(HasFlag(x, GraphicsBufferUsage_UniformBuffer))       usage_flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if(HasFlag(x, GraphicsBufferUsage_StorageBuffer))       usage_flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if(HasFlag(x, GraphicsBufferUsage_IndexBuffer))         usage_flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if(HasFlag(x, GraphicsBufferUsage_VertexBuffer))        usage_flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if(HasFlag(x, GraphicsBufferUsage_IndirectBuffer))      usage_flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	return usage_flags;
}

VkImageAspectFlags
graphics_image_view_aspect_to_vulkan(GraphicsImageViewAspectFlags x) {
	VkImageAspectFlags out = 0;
	if(HasFlag(x, GraphicsImageViewAspectFlags_Color))   AddFlag(out, VK_IMAGE_ASPECT_COLOR_BIT);
	if(HasFlag(x, GraphicsImageViewAspectFlags_Depth))   AddFlag(out, VK_IMAGE_ASPECT_DEPTH_BIT);
	if(HasFlag(x, GraphicsImageViewAspectFlags_Stencil)) AddFlag(out, VK_IMAGE_ASPECT_STENCIL_BIT);
	return out;
}

VkSamplerAddressMode
graphics_sampler_address_mode_to_vulkan(GraphicsSamplerAddressMode x) {
	switch(x) {
		case GraphicsSamplerAddressMode_Repeat:          return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case GraphicsSamplerAddressMode_Mirrored_Repeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case GraphicsSamplerAddressMode_Clamp_To_Border: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case GraphicsSamplerAddressMode_Clamp_To_Edge:   return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
	VulkanFatal("invalid GraphicsSamplerAddressMode: ", (u32)x);
	return {};
}

VkAttachmentLoadOp
graphics_load_op_to_vulkan(GraphicsLoadOp x) {
	switch(x) {
		case GraphicsLoadOp_Load:      return VK_ATTACHMENT_LOAD_OP_LOAD;
		case GraphicsLoadOp_Clear:     return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case GraphicsLoadOp_Dont_Care: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}
	VulkanFatal("invalid GraphicsLoadOp: ", (u32)x);
	return {};
}

VkAttachmentStoreOp
graphics_store_op_to_vulkan(GraphicsStoreOp x) {
	switch(x) {
		case GraphicsStoreOp_Store:     return VK_ATTACHMENT_STORE_OP_STORE;
		case GraphicsStoreOp_Dont_Care: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
	VulkanFatal("invalid GraphicsStoreOp: ", (u32)x);
	return {};
}

VkBlendFactor
graphics_blend_factor_to_vulkan(GraphicsBlendFactor x) {
	switch(x) {
		case GraphicsBlendFactor_Zero:                        return VK_BLEND_FACTOR_ZERO;
		case GraphicsBlendFactor_One:                         return VK_BLEND_FACTOR_ONE;
		case GraphicsBlendFactor_Source_Color:                return VK_BLEND_FACTOR_SRC_COLOR;
		case GraphicsBlendFactor_One_Minus_Source_Color:      return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case GraphicsBlendFactor_Destination_Color:           return VK_BLEND_FACTOR_DST_COLOR;
		case GraphicsBlendFactor_One_Minus_Destination_Color: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case GraphicsBlendFactor_Source_Alpha:                return VK_BLEND_FACTOR_SRC_ALPHA;
		case GraphicsBlendFactor_One_Minus_Source_Alpha:      return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case GraphicsBlendFactor_Destination_Alpha:           return VK_BLEND_FACTOR_DST_ALPHA;
		case GraphicsBlendFactor_One_Minus_Destination_Alpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case GraphicsBlendFactor_Constant_Color:              return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case GraphicsBlendFactor_One_Minus_Constant_Color:    return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		case GraphicsBlendFactor_Constant_Alpha:              return VK_BLEND_FACTOR_CONSTANT_ALPHA;
		case GraphicsBlendFactor_One_Minus_Constant_Alpha:    return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
	}
	VulkanFatal("invalid GraphicsBlendFactor: ", (u32)x);
	return {};
}

VkFormat
graphics_format_to_vulkan(GraphicsFormat x) {
	switch(x) {
		case GraphicsFormat_R32G32_Float:                return VK_FORMAT_R32G32_SFLOAT;
		case GraphicsFormat_R32G32B32_Float:             return VK_FORMAT_R32G32B32_SFLOAT;
		case GraphicsFormat_R8G8B8_UNorm:                return VK_FORMAT_R8G8B8_UNORM;
		case GraphicsFormat_R8G8B8_SRGB:                 return VK_FORMAT_R8G8B8_SRGB;
		case GraphicsFormat_R8G8B8A8_SRGB:               return VK_FORMAT_R8G8B8A8_SRGB;
		case GraphicsFormat_R8G8B8A8_UNorm:              return VK_FORMAT_R8G8B8A8_UNORM;
		case GraphicsFormat_B8G8R8A8_UNorm:              return VK_FORMAT_B8G8R8A8_UNORM;
		case GraphicsFormat_Depth16_UNorm:               return VK_FORMAT_D16_UNORM;
		case GraphicsFormat_Depth32_Float:               return VK_FORMAT_D32_SFLOAT;
		case GraphicsFormat_Depth32_Float_Stencil8_UInt: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case GraphicsFormat_Depth24_UNorm_Stencil8_UInt: return VK_FORMAT_D24_UNORM_S8_UINT;
	}
	VulkanFatal("invalid GraphicsFormat: ", (u32)x);
	return {};
}

RenderFormat
vulkan_format_to_graphics(VkFormat x) {
	switch(x) {
		case VK_FORMAT_R32G32_SFLOAT:      return RenderFormat_R32G32_Signed_Float;
		case VK_FORMAT_R32G32B32_SFLOAT:   return RenderFormat_R32G32B32_Signed_Float;
		case VK_FORMAT_R8G8B8A8_SRGB:      return RenderFormat_R8G8B8A8_StandardRGB;
		case VK_FORMAT_R8G8B8A8_UNORM:     return RenderFormat_R8G8B8A8_UnsignedNormalized;
		case VK_FORMAT_B8G8R8A8_UNORM:     return RenderFormat_B8G8R8A8_UnsignedNormalized;
		case VK_FORMAT_D16_UNORM:          return RenderFormat_Depth16_UnsignedNormalized;
		case VK_FORMAT_D32_SFLOAT:         return RenderFormat_Depth32_SignedFloat;
		case VK_FORMAT_D32_SFLOAT_S8_UINT: return RenderFormat_Depth32_SignedFloat_Stencil8_UnsignedInt;
		case VK_FORMAT_D24_UNORM_S8_UINT:  return RenderFormat_Depth24_UnsignedNormalized_Stencil8_UnsignedInt;
	}
	VulkanFatal("unhandled VkFormat: ", (u32)x);
	return {};
}

VkImageLayout
graphics_image_layout_to_vulkan(GraphicsImageLayout x) {
	switch(x) {
		case GraphicsImageLayout_Undefined:                        return VK_IMAGE_LAYOUT_UNDEFINED;
		case GraphicsImageLayout_General:                          return VK_IMAGE_LAYOUT_GENERAL;
		case GraphicsImageLayout_Color_Attachment_Optimal:         return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case GraphicsImageLayout_Depth_Stencil_Attachment_Optimal: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case GraphicsImageLayout_Present:                          return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		case GraphicsImageLayout_Shader_Read_Only_Optimal:         return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case GraphicsImageLayout_Depth_Stencil_Read_Only_Optimal:  return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}
	VulkanFatal("invalid GraphicsImageLayout: ", (u32)x);
	return {};
}


VkCompareOp
graphics_compare_op_to_vulkan(GraphicsCompareOp x) {
	switch(x) {
		case GraphicsCompareOp_Never:            return VK_COMPARE_OP_NEVER;
		case GraphicsCompareOp_Less:             return VK_COMPARE_OP_LESS;
		case GraphicsCompareOp_Equal:            return VK_COMPARE_OP_EQUAL;
		case GraphicsCompareOp_Less_Or_Equal:    return VK_COMPARE_OP_LESS_OR_EQUAL;
		case GraphicsCompareOp_Greater:          return VK_COMPARE_OP_GREATER;
		case GraphicsCompareOp_Not_Equal:        return VK_COMPARE_OP_NOT_EQUAL;
		case GraphicsCompareOp_Greater_Or_Equal: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case GraphicsCompareOp_Always:           return VK_COMPARE_OP_ALWAYS;
	}
	VulkanFatal("invalid GraphicsCompareOp: ", (u32)x);
	return {};
}

VkDescriptorType
graphics_descriptor_type_to_vulkan(GraphicsDescriptorType x) {
	switch(x) {
		case GraphicsDescriptorType_Uniform_Buffer:         return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case GraphicsDescriptorType_Combined_Image_Sampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	}
	VulkanFatal("invalid GraphicsDescriptorType: ", (u32)x);
	return {};
}

VkBufferUsageFlags
graphics_buffer_usage_to_vulkan(GraphicsBufferUsage x) {
	VkBufferUsageFlags usage_flags = 0;
	if(HasFlag(x,GraphicsBufferUsage_TransferSource))      usage_flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if(HasFlag(x,GraphicsBufferUsage_TransferDestination)) usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if(HasFlag(x,GraphicsBufferUsage_UniformTexelBuffer))  usage_flags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
	if(HasFlag(x,GraphicsBufferUsage_StorageTexelBuffer))  usage_flags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
	if(HasFlag(x,GraphicsBufferUsage_UniformBuffer))       usage_flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if(HasFlag(x,GraphicsBufferUsage_StorageBuffer))       usage_flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if(HasFlag(x,GraphicsBufferUsage_IndexBuffer))         usage_flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if(HasFlag(x,GraphicsBufferUsage_VertexBuffer))        usage_flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if(HasFlag(x,GraphicsBufferUsage_IndirectBuffer))      usage_flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	return usage_flags;
}

// helpers to make retrieving handles a little nicer to look at

VkBuffer&              get_handle(GraphicsBuffer* x)              { return (VkBuffer&)GRAPHICS_INTERNAL(x).buffer_handle; }
VkDeviceMemory&        get_memory_handle(GraphicsBuffer* x)       { return (VkDeviceMemory&)GRAPHICS_INTERNAL(x).memory_handle; }
VkImage&               get_handle(GraphicsImage* x)               { return (VkImage&)GRAPHICS_INTERNAL(x).handle; }
VkDeviceMemory&        get_memory_handle(GraphicsImage* x)        { return (VkDeviceMemory&)GRAPHICS_INTERNAL(x).memory_handle; }
VkImageView&           get_handle(GraphicsImageView* x)           { return (VkImageView&)GRAPHICS_INTERNAL(x).handle; }
VkSampler&             get_handle(GraphicsSampler* x)             { return (VkSampler&)GRAPHICS_INTERNAL(x).handle; }
VkDescriptorSetLayout& get_handle(GraphicsDescriptorSetLayout* x) { return (VkDescriptorSetLayout&)GRAPHICS_INTERNAL(x).handle; }
VkDescriptorSet&       get_handle(GraphicsDescriptorSet* x)       { return (VkDescriptorSet&)GRAPHICS_INTERNAL(x).handle; }
VkPipelineLayout&      get_handle(GraphicsPipelineLayout* x)      { return (VkPipelineLayout&)GRAPHICS_INTERNAL(x).handle; }
VkPipeline&            get_handle(GraphicsPipeline* x)            { return (VkPipeline&)GRAPHICS_INTERNAL(x).handle; }
VkRenderPass&          get_handle(GraphicsRenderPass* x)          { return (VkRenderPass&)GRAPHICS_INTERNAL(x).handle; }
VkFramebuffer&         get_handle(GraphicsFramebuffer* x)         { return (VkFramebuffer&)GRAPHICS_INTERNAL(x).handle; }

// helpers for various things done several times

local u32 
find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties) {
	VulkanInfo("finding memory types.");
	VkPhysicalDeviceMemoryProperties memprops;
	vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &memprops);
	
	for(u32 i = 0; i < memprops.memoryTypeCount; i++) {
		if(HasFlag(type_filter, (1 << i)) && 
		   HasAllFlags(memprops.memoryTypes[i].propertyFlags, properties)) {
			return i;
		}
	}
	
	VulkanAssertVk(false, "failed to find an appropriate memory type.");
	return 0;
}

local void
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
	
	VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	buffer_info.size = *buffer_size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	auto result = vkCreateBuffer(vk_device, &buffer_info, vk_allocator, buffer);
	VulkanAssertVk(result, "Failed to create buffer.");
	
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(vk_device, *buffer, &req);
	VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	alloc_info.allocationSize = req.size;
	alloc_info.memoryTypeIndex = find_memory_type(req.memoryTypeBits, properties);
	result = vkAllocateMemory(vk_device, &alloc_info, vk_allocator, buffer_memory);
	VulkanAssertVk(result, "failed to allocate memory.");
	result = vkBindBufferMemory(vk_device, *buffer, *buffer_memory, 0);
	VulkanAssertVk(result, "failed to bind buffer memory");
	
	u64 aligned_buffer_size = RoundUpTo(req.alignment, *buffer_size);

	if(buffer_size) {
		void* old_buffer_data,* new_buffer_data;
		result = vkMapMemory(vk_device, old_buffer_memory, 0, *buffer_size, 0, &old_buffer_data);
		VulkanAssertVk(result, "failed to map old buffer memory.");
		result = vkMapMemory(vk_device, *buffer_memory, 0, aligned_buffer_size, 0, &new_buffer_data);
		VulkanAssertVk(result, "failed to map new buffer memory.");
		
		CopyMemory(new_buffer_data, old_buffer_data, *buffer_size);
		
		VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
		range.memory = *buffer_memory;
		range.offset = 0;
		range.size   = VK_WHOLE_SIZE;
		result = vkFlushMappedMemoryRanges(vk_device, 1, &range);
		VulkanAssertVk(result, "failed to flush mapped memory range.");
		
		vkUnmapMemory(vk_device, old_buffer_memory);
		vkUnmapMemory(vk_device, *buffer_memory);
	}
	
	if(old_buffer != VK_NULL_HANDLE) vkDestroyBuffer(vk_device, old_buffer, vk_allocator);
	if(old_buffer_memory != VK_NULL_HANDLE) vkFreeMemory(vk_device, old_buffer_memory, vk_allocator);
	
	*buffer_size = aligned_buffer_size;
}

local VkCommandBuffer
begin_single_time_commands() {
	VkCommandBuffer cmdbuf;
	
	VkCommandBufferAllocateInfo alloc_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	alloc_info.             level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.       commandPool = vk_command_pool;
	alloc_info.commandBufferCount = 1;
	vkAllocateCommandBuffers(vk_device, &alloc_info, &cmdbuf);
	
	VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdbuf, &begin_info);
	
	return cmdbuf;
}

local void
end_single_time_commands(VkCommandBuffer cmdbuf) {
	vkEndCommandBuffer(cmdbuf);
	
	VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submit_info.commandBufferCount = 1;
	submit_info.   pCommandBuffers = &cmdbuf;
	vkQueueSubmit(vk_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	
	vkQueueWaitIdle(vk_graphics_queue);
	vkFreeCommandBuffers(vk_device, vk_command_pool, 1, &cmdbuf);
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @init_functions


local void
setup_allocator() {
	VulkanInfo("setting up allocators.");

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
	
	vk_allocator_.pfnAllocation = deshi_vulkan_allocation_func;
	vk_allocator_.pfnReallocation = deshi_vulkan_reallocation_func;
	vk_allocator_.pfnFree = deshi_vulkan_free_func;
	
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
	
	vk_temp_allocator_.pfnAllocation = deshi_vulkan_temp_allocation_func;
	vk_temp_allocator_.pfnReallocation = deshi_vulkan_temp_reallocation_func;
	vk_temp_allocator_.pfnFree = deshi_vulkan_temp_free_func;

	VulkanInfo("finished setting up allocators in ", peek_stopwatch(watch), "ms");
}

local VKAPI_ATTR VkBool32 VKAPI_CALL
vk_debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
		void* pUserData) {
	switch(messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
			VulkanError(pCallbackData->pMessage);
			if(g_graphics->break_on_error) DebugBreakpoint;
		} break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
			VulkanWarning(pCallbackData->pMessage);
		} break;
		default: {
			VulkanNotice(pCallbackData->pMessage);
		} break;
	}
	return VK_FALSE;
}

void
create_instance(Window* window) {
	VulkanInfo("creating vulkan instance.");

	Stopwatch watch = start_stopwatch();

	// check for validation layer support
	if(is_debugging) {
		VulkanInfo("checking validation layer support.");
		b32 has_support = true;

		u32 layer_count = 0;
		vkEnumerateInstanceLayerProperties(&layer_count, 0);
		auto available_layers = array<VkLayerProperties>::create_with_count(layer_count, temp_allocator);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.ptr);

		forI(ArrayCount(validation_layers)) {
			b32 layer_found = false;
			forX(j, available_layers.count()) {
				if(!strcmp(validation_layers[i], available_layers[j].layerName)) {
					layer_found = true;
					break;
				}
			}
			if(!layer_found) VulkanFatal("a validation layer was requested but it is not available");
		}
	}

	// set application info
	VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	app_info.pApplicationName   = (char*)window->title.str;
	app_info.applicationVersion = VK_MAKE_VERSION(1,0,0);
	app_info.pEngineName        = "deshi";
	app_info.engineVersion      = VK_MAKE_VERSION(1,0,0);
	app_info.apiVersion         = VK_API_VERSION_1_3;
	
	VkValidationFeatureEnableEXT validation_features_arr[] = {
		VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
	};

	VkValidationFeaturesEXT validation_features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
	validation_features.disabledValidationFeatureCount = 0;
	validation_features.pDisabledValidationFeatures    = 0;
	validation_features.enabledValidationFeatureCount  = (is_debugging? ArrayCount(validation_features_arr) : 0);
	validation_features.pEnabledValidationFeatures     = validation_features_arr;
	
	// get required extensions
	VulkanInfo("getting required extensions.");
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
	debug_create_info.messageSeverity = vk_callback_severities;
	debug_create_info.messageType     = vk_callback_types;
	debug_create_info.pfnUserCallback = vk_debug_callback;
	
	//create the instance
	VkInstanceCreateInfo create_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	create_info.pApplicationInfo        = &app_info;
	create_info.enabledExtensionCount   = (u32)ArrayCount(extensions);
	create_info.ppEnabledExtensionNames = extensions;
	if(is_debugging){
		create_info.enabledLayerCount   = (u32)ArrayCount(validation_layers);
		create_info.ppEnabledLayerNames = validation_layers;
		debug_create_info.pNext         = &validation_features;
		create_info.pNext               = &debug_create_info;
	} else {
		create_info.enabledLayerCount   = 0;
		create_info.pNext               = 0;
	}

	auto result = vkCreateInstance(&create_info, vk_allocator, &vk_instance);
	VulkanAssertVk(result, "failed to create instance.");

	VulkanInfo("finished creating instance in ", peek_stopwatch(watch), "ms");

	if(is_debugging) {
		VulkanInfo("setting up debug messenger.");
		
		watch = start_stopwatch();

		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT");
		if(!func) {
			VulkanError("unable to retrieve vkCreateDebugUtilsMessengerEXT");
			return;
		}
		auto result = func(vk_instance, &debug_create_info, vk_allocator, &vk_debug_messenger);
		VulkanAssertVk(result, "failed to create debug messenger.");

		VulkanInfo("finished setting up debug messenger in ", peek_stopwatch(watch), "ms");
	}
}

void
create_surface(Window* window) {
	VulkanInfo("creating surface.");

	Stopwatch watch = start_stopwatch();

	auto wininf = (WindowInfo*)window->render_info;

#if DESHI_WINDOWS
	PrintVk(2, "Creating win32-vulkan surface");
	VkWin32SurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
	info.hwnd = (HWND)window->handle;
	info.hinstance = (HINSTANCE)win32_console_instance;
	resultVk = vkCreateWin32SurfaceKHR(instance, &info, 0, &wi->surface); AssertVk(resultVk, "failed to create win32 surface");
#elif DESHI_LINUX
	VkXlibSurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
	info.window = (X11Window)window->handle;
	info.dpy = linux.x11.display;
	auto result = vkCreateXlibSurfaceKHR(vk_instance, &info, 0, &wininf->surface);
	VulkanAssertVk(result, "failed to create X11 surface.");
#else
#	error "unsupported platform for renderer"
#endif

	VulkanInfo("finished creating surface in ", peek_stopwatch(watch), "ms.");
}

void
pick_physical_device(Window* window) {
	VulkanInfo("picking physical device.");

	Stopwatch watch = start_stopwatch();

	u32 device_count = 0;
	vkEnumeratePhysicalDevices(vk_instance, &device_count, 0);
	auto devices = array<VkPhysicalDevice>::create_with_count(device_count, temp_allocator);
	vkEnumeratePhysicalDevices(vk_instance, &device_count, devices.ptr);

	forI(devices.count()) {
		auto device = devices[i];
		
		// find a device which supports graphics operations
		// TODO(sushi) when somehow specified by the user, also check for compute graphics support
		//             or just always check and if not supported flag it so we can error on any
		//             compute shader requests later
		
		u32 queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
		auto queue_families = array<VkQueueFamilyProperties>::create_with_count(queue_family_count, temp_allocator);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.ptr);

		forI(queue_family_count) {
			auto family = queue_families[i];
			if(HasFlag(family.queueFlags, VK_QUEUE_GRAPHICS_BIT)) {
				physical_queue_families.found_graphics_family = true;
				physical_queue_families.graphics_family = i;
			}
			
			VkBool32 present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, ((WindowInfo*)window->render_info)->surface, &present_support);
			if(present_support) {
				physical_queue_families.found_present_family = true;
				physical_queue_families.present_family = i;
			}
			
			if(physical_queue_families.found_graphics_family && physical_queue_families.found_graphics_family) break;
		}
		
		if(!physical_queue_families.found_graphics_family) continue;
		
		// check if the chosen device supports enabled extensions
		
		u32 extension_count;
		vkEnumerateDeviceExtensionProperties(device, 0, &extension_count, 0);
		auto available_extensions = array<VkExtensionProperties>::create_with_count(extension_count, temp_allocator);
		vkEnumerateDeviceExtensionProperties(device, 0, &extension_count, available_extensions.ptr);
		
		u32 count = 0;
		forI(extension_count) {
			auto extension = available_extensions[i];
			forI(ArrayCount(device_extensions)) {
				if(extension.extensionName == device_extensions[i]) {
					count++;
					break;
				}
			}
			if(count == ArrayCount(device_extensions)) break;
		}
		
		if(count == ArrayCount(device_extensions)) continue;
		
		u32 format_count;
		u32 present_mode_count;
		auto surface = ((WindowInfo*)window->render_info)->surface;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, 0);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, 0);
		if(!(present_mode_count&&format_count)) continue;
		
		vk_physical_device = device;
		break;
	}

	if(vk_physical_device == VK_NULL_HANDLE)
		VulkanFatal("failed to find a suitable physical device.");

	vkGetPhysicalDeviceFeatures(vk_physical_device, &vk_physical_device_features);
	vkGetPhysicalDeviceProperties(vk_physical_device, &vk_physical_device_properties);

	VulkanInfo("finished picking physical device in ", peek_stopwatch(watch), "ms.");
	VulkanInfo("chose device '", vk_physical_device_properties.deviceName, "'.");
}

void
create_logical_device(Window* window) {
	VulkanInfo("creating logical device.");

	Stopwatch watch = start_stopwatch();

	f32 queue_priority = 1.f;
	auto queue_create_infos = array<VkDeviceQueueCreateInfo>::create(temp_allocator);
	VkDeviceQueueCreateInfo queue_create_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queue_create_info.queueFamilyIndex = physical_queue_families.graphics_family;
	queue_create_info.queueCount       = 1;
	queue_create_info.pQueuePriorities = &queue_priority;
	queue_create_infos.push(queue_create_info);
	
	if(physical_queue_families.present_family != physical_queue_families.graphics_family) {
		queue_create_info.queueFamilyIndex = physical_queue_families.present_family;
		queue_create_infos.push(queue_create_info);
	}
	
	if(vk_physical_device_features.samplerAnisotropy) {
		vk_physical_device_enabled_features.samplerAnisotropy = VK_TRUE; // anistrophic filtering
		vk_physical_device_enabled_features.sampleRateShading = VK_TRUE; // sample shading
	}
	
	if(vk_physical_device_features.fillModeNonSolid) {
		vk_physical_device_enabled_features.fillModeNonSolid = VK_TRUE; // wireframe
		if(vk_physical_device_features.wideLines) {
			vk_physical_device_enabled_features.wideLines = VK_TRUE; // wide lines
		}
	}
	
	if(is_debugging) {
		if(vk_physical_device_features.geometryShader) {
			vk_physical_device_enabled_features.geometryShader = VK_TRUE;
		}
	}
	
	VkDeviceCreateInfo create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	create_info.pQueueCreateInfos       = queue_create_infos.ptr;
	create_info.queueCreateInfoCount    = queue_create_infos.count();
	create_info.pEnabledFeatures        = &vk_physical_device_enabled_features;
	create_info.enabledExtensionCount   = (u32)ArrayCount(device_extensions);
	create_info.ppEnabledExtensionNames = device_extensions;
	
	if(renderSettings.debugging) {
		create_info.enabledLayerCount   = (u32)ArrayCount(validation_layers);
		create_info.ppEnabledLayerNames = validation_layers;
	} else {
		create_info.enabledLayerCount = 0;
	}
	
	auto result = vkCreateDevice(vk_physical_device, &create_info, vk_allocator, &vk_device);
	VulkanAssertVk(result, "failed to create logical device");
	
	vkGetDeviceQueue(vk_device, physical_queue_families.graphics_family, 0, &vk_graphics_queue);
	vkGetDeviceQueue(vk_device, physical_queue_families.present_family, 0, &vk_present_queue);

	VulkanInfo("finished creating logical device in ", peek_stopwatch(watch), "ms.");
}

void
create_command_pool() {
	VulkanInfo("creating command pool.");

	Stopwatch watch = start_stopwatch();
	
	VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	// NOTE(sushi) this flag allows any command buffer allocated from a pool to be 
	//             individually reset to its initial state through vkResetCommandBuffer
	//             implicitly by vkBeginCommandBuffer
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = physical_queue_families.graphics_family;
	
	auto result = vkCreateCommandPool(vk_device, &pool_info, vk_allocator, &vk_command_pool);
	VulkanAssertVk(result, "failed to create command pool");

	VulkanInfo("finished creating command pool in ", peek_stopwatch(watch), "ms.");
}

void
setup_shader_compiler() {
	VulkanInfo("setting up shader compiler.");

	Stopwatch watch = start_stopwatch();

	vk_shader_compiler = shaderc_compiler_initialize();
	vk_shader_compiler_options = shaderc_compile_options_initialize();

	// TODO(sushi) option to optimize shaders
	
	VulkanInfo("finished setting up shader compiler in ", peek_stopwatch(watch), "ms.");
}

void
create_descriptor_pool() {
	VulkanInfo("creating descriptor pool.");
	
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
	auto result = vkCreateDescriptorPool(vk_device, &pool_info, vk_allocator, &vk_descriptor_pool);
	VulkanAssertVk(result, "failed to create descriptor pool.");

	VulkanInfo("finished creating descriptor pool in ", peek_stopwatch(watch), "ms.");
}

void
create_pipeline_cache() {
	VulkanInfo("creating pipeline cache.");

	Stopwatch watch = start_stopwatch();

	VkPipelineCacheCreateInfo pipeline_cache_create_info{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
	
	if(file_exists(str8l("data/pipelines.cache"))) {
		str8 data = file_read_simple(str8l("data/pipelines.cache"), deshi_temp_allocator);
		pipeline_cache_create_info.initialDataSize = data.count;
		pipeline_cache_create_info.pInitialData    = data.str;
	}

	auto result = vkCreatePipelineCache(vk_device, &pipeline_cache_create_info, 0, &vk_pipeline_cache);
	VulkanAssertVk(result, "failed to create pipeline cache.");

	VulkanInfo("finished creating pipeline cache in ", peek_stopwatch(watch), "ms.");
}

void
create_swapchain(Window* window) {
	VulkanInfo("creating swapchain.");

	Stopwatch watch = start_stopwatch();

	auto wininf = (WindowInfo*)window->render_info;
	auto old_swapchain = wininf->swapchain;
	wininf->swapchain = VK_NULL_HANDLE;

	vkDeviceWaitIdle(vk_device);

	// check GPU's capabilities for the new swapchain
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, wininf->surface, &wininf->support_details.capabilities);
	
	u32 format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, wininf->surface, &format_count, 0);
	if(format_count) {
		wininf->support_details.formats.recount(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, wininf->surface, &format_count, wininf->support_details.formats.ptr);
	}
	
	u32 present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, wininf->surface, &present_mode_count, 0);
	if(present_mode_count) {
		wininf->support_details.present_modes.recount(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, wininf->surface, &present_mode_count, wininf->support_details.present_modes.ptr);
	}
	
	// choose swapchain's surface format 
	wininf->surface_format = wininf->support_details.formats[0];
	forI(wininf->support_details.formats.count()) {
		auto format = wininf->support_details.formats[i];
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
	
	forI(wininf->support_details.present_modes.count()) {
		auto pm = wininf->support_details.present_modes[i];
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
		wininf->extent = { (u32)window->width, (u32)window->height };
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
	
	// TODO(sushi) see if there's something I'm missing when it comes to 
	//             minImageCount with immediate mode presentation.
	//             Before the rewrite we seemed to have believed that we could just use
	//             one image, but we actually have to determine this from the surface's reported
	//             capabilities.
	wininf->min_image_count = Max(wininf->min_image_count, capabilities.minImageCount);
	
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
		info.imageExtent.width     = window->width;
		info.imageExtent.height    = window->height;
	} else {
		info.imageExtent.width     = window->width  = wininf->extent.width;
		info.imageExtent.height    = window->height = wininf->extent.height;
	}

	auto result = vkCreateSwapchainKHR(vk_device, &info, vk_allocator, &wininf->swapchain);
	VulkanAssertVk(result, "failed to create swapchain.");

	if(old_swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(vk_device, old_swapchain, vk_allocator);

	VulkanInfo("finished creating swapchain in ", peek_stopwatch(watch), "ms.");
}

void
create_render_pass_and_frames(Window* window) {
	VulkanInfo("creating frames for ", window->title);
	
	Stopwatch watch = start_stopwatch();
	
	auto wininf = (WindowInfo*)window->render_info;
	
	GraphicsRenderPassAttachment color_attachment;
	color_attachment.          format = vulkan_format_to_graphics(wininf->surface_format.format);
	color_attachment.         load_op = RenderAttachmentLoadOp_Clear;
	color_attachment.        store_op = RenderAttachmentStoreOp_Store;
	color_attachment. stencil_load_op = RenderAttachmentLoadOp_Dont_Care;
	color_attachment.stencil_store_op = RenderAttachmentStoreOp_Dont_Care;
	color_attachment.  initial_layout = RenderImageLayout_Undefined;
	color_attachment.    final_layout = RenderImageLayout_Present;
	
	GraphicsRenderPassAttachment depth_attachment;
	depth_attachment.          format = vulkan_format_to_graphics(find_depth_format());
	depth_attachment.         load_op = RenderAttachmentLoadOp_Clear;
	depth_attachment.        store_op = RenderAttachmentStoreOp_Store;
	depth_attachment. stencil_load_op = RenderAttachmentLoadOp_Clear;
	depth_attachment.stencil_store_op = RenderAttachmentStoreOp_Dont_Care;
	depth_attachment.  initial_layout = RenderImageLayout_Undefined;
	depth_attachment.    final_layout = RenderImageLayout_Depth_Stencil_Attachment_Optimal;
	
	auto render_pass = graphics::RenderPass::allocate();
	render_pass->debug_name = str8l("Default render pass");
	render_pass->color_attachment = color_attachment;
	render_pass->depth_attachment = depth_attachment;
	render_pass->color_clear_values = vec4::ZERO;
	render_pass->depth_clear_values = {1.f, 0};
	render_pass->update();

	vkGetSwapchainImagesKHR(vk_device, wininf->swapchain, &wininf->image_count, 0);
	auto images = array<VkImage>::create_with_count(wininf->image_count, temp_allocator);
	vkGetSwapchainImagesKHR(vk_device, wininf->swapchain, &wininf->image_count, images.ptr);
	
	wininf->presentation_frames.recount(wininf->image_count);
	
	forI(wininf->image_count) {
		auto frame = wininf->presentation_frames[i] = graphics::Framebuffer::allocate();
		frame->width = window->width;
		frame->height = window->height;
		frame->render_pass = render_pass;
		
		auto color_image_view = graphics::ImageView::allocate();
		auto depth_image_view = graphics::ImageView::allocate();
		auto color_image      = graphics::Image::allocate();
		auto depth_image      = graphics::Image::allocate();
		
		color_image->           format = vulkan_format_to_graphics(wininf->surface_format.format);
		color_image->           extent = {window->width, window->height};
		color_image->            usage = RenderImageUsage_Color_Attachment;
		color_image->memory_properties = RenderMemoryPropertyFlag_DeviceLocal;
		color_image->__internal.handle = (void*)images[i];
		// NOTE(sushi) no reason to call render_image_update() here because we get the handle to the image from the swapchain (which is kinda stupid but whatever)
		
		depth_image->           format = vulkan_format_to_graphics(find_depth_format());
		depth_image->           extent = {window->width, window->height};
		depth_image->            usage = RenderImageUsage_Depth_Stencil_Attachment;
		depth_image->memory_properties = RenderMemoryPropertyFlag_DeviceLocal;
		depth_image->update();
		
		color_image_view->format = color_image->format;
		color_image_view->aspect_flags = RenderImageViewAspectFlags_Color;
		color_image_view->update();

		depth_image_view->format = depth_image->format;
		depth_image_view->aspect_flags = RenderImageViewAspectFlags_Depth;
		depth_image_view->update();

		frame->color_image_view = color_image_view;
		frame->depth_image_view = depth_image_view;
		color_image_view->image = color_image;
		depth_image_view->image = depth_image;

		VkImageView attachments[2] = {
			get_handle(color_image_view),
			get_handle(depth_image_view),
		};
		
		VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		info.renderPass = get_handle(frame->render_pass);
		info.width = window->width;
		info.height = window->height;
		info.layers = 1;
		info.pAttachments = attachments;
		info.attachmentCount = 2;
		auto result = vkCreateFramebuffer(vk_device, &info, vk_allocator, &get_handle(frame));
		VulkanAssertVk(result, "failed to create framebuffer.");

		vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_FRAMEBUFFER, (u64)get_handle(frame), 
				 "Default framebuffer");
	}
	
	VulkanInfo("finished creating default render pass and frames in ", peek_stopwatch(watch), "ms.");
}

void
create_sync_objects() {
	VulkanInfo("creating sync objects");

	Stopwatch watch = start_stopwatch();
	
	VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	
	auto result = vkCreateSemaphore(vk_device, &semaphore_info, vk_allocator, &vk_semaphore_image_acquired);
	VulkanAssertVk(result, "failed to create image acquisition semaphore.");
	result = vkCreateSemaphore(vk_device, &semaphore_info, vk_allocator, &vk_semaphore_render_complete);
	VulkanAssertVk(result, "failed to create render complete semaphore.");
	
	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_SEMAPHORE, (u64)vk_semaphore_image_acquired, "Semaphore image acquired");
	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_SEMAPHORE, (u64)vk_semaphore_render_complete, "Semaphore render complete");

	VulkanInfo("finished creating sync objects in ", peek_stopwatch(watch), "ms.");
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @init


void
graphics_init(Window* window) {
	if(!window) {
		VulkanFatal("passed a null window pointer.");
		return;
	}

	if(window->render_info) {
		VulkanError("the given window has a non-zero render_info. This likely means that the graphics module has already been initialized with the window or the window was not initialized properly (eg. garbage data from a stack allocated window).");
		return;
	}

	VulkanNotice("initializing graphics module for window '", window->title, "'.");
	
	Stopwatch watch = start_stopwatch();
	
	memory_pool_init(window_infos, 4);

	if(!file_exists(str8l("data/shaders/"))) {
		VulkanNotice("data/shaders/ is missing, creating it now.");
		file_create(str8l("data/shaders/"));
	}

	// TODO(sushi) this should be moved to an implementation shared between backends
	memory_pool_init(g_render.pools.descriptor_set_layouts, 8);
	memory_pool_init(g_render.pools.descriptor_sets, 8);
	memory_pool_init(g_render.pools.pipeline_layouts, 8);
	memory_pool_init(g_render.pools.pipelines, 8);
	memory_pool_init(g_render.pools.buffers, 8);
	memory_pool_init(g_render.pools.command_buffers, 8);
	memory_pool_init(g_render.pools.images, 8);
	memory_pool_init(g_render.pools.image_views, 8);
	memory_pool_init(g_render.pools.samplers, 8);
	memory_pool_init(g_render.pools.passes, 8);
	memory_pool_init(g_render.pools.framebuffers, 8);

	// create the window info that this window will point to 
	auto wi = (WindowInfo*)(window->render_info = memory_pool_push(window_infos));
	wi->support_details.formats = array<VkSurfaceFormatKHR>::create(primary_allocator);
	wi->support_details.present_modes = array<VkPresentModeKHR>::create(primary_allocator);
	wi->presentation_frames = array<graphics::Framebuffer*>::create(primary_allocator);

	setup_allocator();
	create_instance(window);

	//// grab Vulkan extension functions ////
#if BUILD_INTERNAL
	vkfunc_vkSetDebugUtilsObjectNameEXT  = (PFN_vkSetDebugUtilsObjectNameEXT) vkGetInstanceProcAddr(vk_instance, "vkSetDebugUtilsObjectNameEXT");
	vkfunc_vkCmdBeginDebugUtilsLabelEXT  = (PFN_vkCmdBeginDebugUtilsLabelEXT) vkGetInstanceProcAddr(vk_instance, "vkCmdBeginDebugUtilsLabelEXT");
	vkfunc_vkCmdEndDebugUtilsLabelEXT    = (PFN_vkCmdEndDebugUtilsLabelEXT)   vkGetInstanceProcAddr(vk_instance, "vkCmdEndDebugUtilsLabelEXT");
	vkfunc_vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(vk_instance, "vkCmdInsertDebugUtilsLabelEXT");
#endif //BUILD_INTERNAL
	
	create_surface(window);
	pick_physical_device(window);
	create_logical_device(window);
	create_command_pool();

	wi->command_buffer = graphics::CommandBuffer::allocate();
	wi->command_buffer->update();

	setup_shader_compiler();
	create_descriptor_pool();
	
	create_swapchain(window);
	create_render_pass_and_frames(window);

	create_sync_objects();
	create_pipeline_cache();

	VulkanNotice("finished initialization in ", peek_stopwatch(watch), "ms.");
	deshiStage |= DS_RENDER;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @buffer


GraphicsBuffer*
graphics_buffer_create(
		void* data,
		u64 requested_size, 
		GraphicsBufferUsage usage, 
		GraphicsMemoryPropertyFlags properties, 
		GraphicsMemoryMappingBehavoir mapping_behavoir) {
	GraphicsBuffer* out = memory_pool_push(g_graphics->pools.buffers);
	if(!requested_size) {
		VulkanError("the requested size of a GraphicsBuffer must be non-zero.");
		return 0;
	}

	if(HasFlag(properties, GraphicsMemoryProperty_HostStreamed) && 
	   HasFlag(properties, GraphicsMemoryPropertyFlag_LazilyAllocated)) {
		VulkanError("the memory property flags 'HostStreamed' and 'LazilyAllocated' are incompatible.");
		return 0;
	}

	VkDeviceSize size = requested_size;

	// TODO(sushi) decide if we should be using this here. Before we did not, but I feel like doing this keeps the api and internal
	//             stuff consistent. Eg. if we add the ability to do something with buffers in either the api or backend, this requires
	//             us to update both to handle it rather than, for example, only updating the api's implementation to handle it.
	//             Though this may introduce extra work that is not needed for buffers we hand out to the user.
	create_or_resize_buffer(
			&get_handle(out), 
			&get_memory_handle(out),
			&size,
			size,
			graphics_buffer_usage_to_vulkan(usage),
			graphics_memory_properties_to_vulkan(properties));

	GRAPHICS_INTERNAL(out).size = size;

	if(mapping_behavoir == GraphicsMemoryMapping_Never) {
		if(!data) {
			VulkanWarning("the mapping behavoir of the buffer we are creating is set to 'Never', but the given data pointer is null. I'm not sure yet if there is a legitamate usecase for this, so remove this warning if we ever come across one.");
		} else {
			VulkanAssertVk(!HasFlag(properties,GraphicsMemoryPropertyFlag_HostCoherent|GraphicsMemoryPropertyFlag_HostVisible|GraphicsMemoryPropertyFlag_HostCached), 
					"incompatible mapping behavoir and memory flags. Mapping behavoir is set to 'Never', which is incompatible with the memory properties 'HostVisible', 'HostCoherent', or 'HostCached'.");
			// create a staging buffer
			VkBuffer stage = VK_NULL_HANDLE;
			VkDeviceMemory stage_memory = VK_NULL_HANDLE;

			create_or_resize_buffer(
					&stage,
					&stage_memory,
					&size,
					size,
					graphics_buffer_usage_to_vulkan(usage),
					graphics_memory_properties_to_vulkan(properties));
			
			void* mapped = 0;
			auto result = vkMapMemory(vk_device, stage_memory, 0, size, 0, &mapped);
			VulkanAssertVk(result, "failed to map memory for initial data copy to GraphicsBuffer. (Mapping behavoir is 'Never')");

			CopyMemory(mapped, data, requested_size);

			if(!HasFlag(properties, GraphicsMemoryPropertyFlag_HostCoherent)) {
				VkMappedMemoryRange mapped_range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
				mapped_range.memory = stage_memory;
				mapped_range.offset = 0;
				mapped_range.  size = size;
				result = vkFlushMappedMemoryRanges(vk_device, 1, &mapped_range);
				VulkanAssertVk(result, "failed to flush memory range while performing initial copy to GraphicsBuffer. (Mapping behavoir is 'Never')");
			}

			vkUnmapMemory(vk_device, get_memory_handle(out));
		} 
	} else if(mapping_behavoir == GraphicsMemoryMapping_Occasional) {
		if(data) {
			auto result = vkMapMemory(vk_device, get_memory_handle(out), 0, size, 0, &GRAPHICS_INTERNAL(out).mapped.data);
			VulkanAssertVk(result, "failed to map memory for initial data copy to GraphicsBuffer. (Mapping behavoir is 'Occasional')");

			CopyMemory(GRAPHICS_INTERNAL(out).mapped.data, data, requested_size);

			VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
			range.memory = get_memory_handle(out);
			range.offset = 0;
			range.  size = size;
			result = vkFlushMappedMemoryRanges(vk_device, 1, &range);
			VulkanAssertVk(result, "failed to flush memory range while performing initial copy to GraphicsBuffer. (Mapping behavoir is set to 'Occasional')");

			vkUnmapMemory(vk_device, get_memory_handle(out));
		}
		GRAPHICS_INTERNAL(out).mapped = {};
	} else if(mapping_behavoir == GraphicsMemoryMapping_Persistent) {
		auto result = vkMapMemory(vk_device, get_memory_handle(out), 0, size, 0, &GRAPHICS_INTERNAL(out).mapped.data);
		VulkanAssertVk(result, "failed to perform initial mapping for GraphicsBuffer with 'Persistent' mapping behavoir.");

		if(data) {
			CopyMemory(GRAPHICS_INTERNAL(out).mapped.data, data, requested_size);

			VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
			range.memory = get_memory_handle(out);
			range.offset = 0;
			range.  size = size;
			result = vkFlushMappedMemoryRanges(vk_device, 1, &range);
			VulkanAssertVk(result, "failed to flush memory range while performing initial copy to GraphicsBuffer. (MappingBehavoir is set to 'Persistent').");
		}
	} else VulkanAssertVk(!VK_SUCCESS, "invalid mapping_behavoir specified: ", (u32)mapping_behavoir);
	
	StaticAssertAlways(sizeof(VkDeviceSize) == sizeof(u64));
	GRAPHICS_INTERNAL(out).usage = usage;
	GRAPHICS_INTERNAL(out).memory_properties = properties;
	GRAPHICS_INTERNAL(out).mapping_behavoir = mapping_behavoir;

	return out;
}

void
graphics_buffer_destroy(GraphicsBuffer* x_) {
	VulkanAssert(x_, "passed a null GraphicsBuffer pointer.");

	auto x = (graphics::Buffer*)x_;

	if(get_handle(x) == VK_NULL_HANDLE || get_memory_handle(x) == VK_NULL_HANDLE) {
		VulkanError("the given GraphicsBuffer has null backend handles which indicates deletion, corruption, or that the object was not created with graphics_buffer_create().");
		return;
	}

	if(x->mapped_data()) {
		vkUnmapMemory(vk_device, get_memory_handle(x));
	}

	vkDestroyBuffer(vk_device, get_handle(x), vk_allocator);
	vkFreeMemory(vk_device, get_memory_handle(x), vk_allocator);

	memory_pool_delete(g_graphics->pools.buffers, x);
}

void
graphics_buffer_reallocate(GraphicsBuffer* x_, u64 new_size) {
	VulkanAssert(x_, "passed a null GraphicsBuffer pointer");
	VulkanAssert(new_size, "new_size must be non-zero.");

	if(new_size < GRAPHICS_INTERNAL(x_).size) return;

	if(!HasFlag(GRAPHICS_INTERNAL(x_).usage, GraphicsBufferUsage_TransferSource|GraphicsBufferUsage_TransferDestination)) {
		VulkanError("the given GraphicsBuffer did not specify one of the usage flags 'TransferSource' or 'TransferDestination' when it was created. The new buffer will use the same properties as the given buffer, which requires both copying to and from.");
		return;
	}

	auto x = (graphics::Buffer*)x_;

	if(x->mapped_data()) {
		vkUnmapMemory(vk_device, get_memory_handle(x));
	}

	VkBuffer nu = VK_NULL_HANDLE;
	VkDeviceMemory nu_memory = VK_NULL_HANDLE;

	VkDeviceSize device_size = new_size;

	create_or_resize_buffer(
			&nu,
			&nu_memory,
			&device_size,
			device_size,
			graphics_buffer_usage_to_vulkan(GRAPHICS_INTERNAL(x).usage),
			graphics_memory_properties_to_vulkan(GRAPHICS_INTERNAL(x).memory_properties));

	auto cmdbuf = begin_single_time_commands();

	// I'm not sure if this barrier is actually necessary because we create
	// the copy command here and then execute it immediately (in end_single_time_commands())
	// perhaps it will make more sense when the graphics api is thread safe
	VkMemoryBarrier memory_barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
	memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	memory_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	
	vkCmdPipelineBarrier(
			cmdbuf, 
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 
			1, &memory_barrier, 
			0, 0, 0, 0);

	VkBufferCopy region;
	region.size = x->device_size();
	region.srcOffset = region.dstOffset = 0;

	vkCmdCopyBuffer(cmdbuf, get_handle(x), nu, 1, &region);

	end_single_time_commands(cmdbuf);

	// get rid of the old buffer
	vkDestroyBuffer(vk_device, get_handle(x), vk_allocator);
	vkFreeMemory(vk_device, get_memory_handle(x), vk_allocator);
	
	GRAPHICS_INTERNAL(x).size = device_size;
	get_handle(x) = nu;
	get_memory_handle(x) = nu_memory;
}

void*
graphics_buffer_map(GraphicsBuffer* x_, u64 size, u64 offset) {
	VulkanAssert(x_, "passed a null GraphicsBuffer pointer.");
	VulkanAssert(size, "size is 0");

	if(get_handle(x_) == VK_NULL_HANDLE || get_memory_handle(x_) == VK_NULL_HANDLE) {
		VulkanError("the given GraphicsBuffer has null backend handles which indicates deletion, corruption, or that the object was not created with graphics_buffer_create().");
		return 0;
	}

	auto x = (graphics::Buffer*)x_;

	if(x->mapped_data()) {
		if(GRAPHICS_INTERNAL(x).mapping_behavoir == GraphicsMemoryMapping_Persistent) {
			VulkanWarning("useless call on buffer '", x->debug_name, "' (", (void*)x, ") as its mapping behavoir is set to 'Persistent'.");
		} else {
			VulkanWarning("useless call on buffer '", x->debug_name, "' (", (void*)x, ") as it was already previously mapped.");
		}
		return x->mapped_data();
	}

	if(GRAPHICS_INTERNAL(x).mapping_behavoir == GraphicsMemoryMapping_Never) {
		VulkanError("the given buffer's mapping behavoir is 'Never'.");
		return 0;
	}

	auto result = vkMapMemory(vk_device, get_memory_handle(x), offset, size, 0, &GRAPHICS_INTERNAL(x).mapped.data);
	GRAPHICS_INTERNAL(x).mapped.offset = offset;
	GRAPHICS_INTERNAL(x).mapped.size   = size;

	return x->mapped_data();
} 

void
graphics_buffer_unmap(GraphicsBuffer* x_, b32 flush) {
	VulkanAssert(x_, "passed a null GraphicsBuffer pointer.");
	
	if(get_handle(x_) == VK_NULL_HANDLE || get_memory_handle(x_) == VK_NULL_HANDLE) {
		VulkanError("the given GraphicsBuffer has null backend handles which indicates deletion, corruption, or that the object was not created with graphics_buffer_create().");
		return;
	}

	auto x = (graphics::Buffer*)x_;

	if(!x->mapped_data()) {
		VulkanWarning("useless call on buffer '", x->debug_name, "' (", (void*)x, ") as it is not mapped.");
		return;
	}

	if(flush) {
		VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
		range.memory = get_memory_handle(x);
		range.offset = x->mapped_offset();
		range.size   = x->mapped_size();
		auto result = vkFlushMappedMemoryRanges(vk_device, 1, &range);
		VulkanAssert(result, "failed to flush memory to device.");
	}

	vkUnmapMemory(vk_device, get_memory_handle(x));

	GRAPHICS_INTERNAL(x).mapped = {};
}

void
graphics_buffer_flush(GraphicsBuffer* x_) {
	VulkanAssert(x_, "passed a null GraphicsBuffer pointer");

	if(get_handle(x_) == VK_NULL_HANDLE || get_memory_handle(x_) == VK_NULL_HANDLE) {
		VulkanError("the given GraphicsBuffer has null backend handles which indicates deletion, corruption, or that the object was not created with graphics_buffer_create().");
		return;
	}

	auto x = (graphics::Buffer*)x_;

	if(!x->mapped_data()) {
		VulkanError("the given buffer is not mapped.");
		return;
	}

	VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
	range.memory = get_memory_handle(x);
	range.offset = x->mapped_offset();
	range.  size = x->mapped_size();
	auto result = vkFlushMappedMemoryRanges(vk_device, 1, &range);
	VulkanAssertVk(result, "failed to flush memory of buffer '", x->debug_name, "' (", (void*)x, ").");
}

#undef primary_allocator
#undef temp_allocator
#undef VulkanFatal
#undef VulkanError
#undef VulkanWarning
#undef VulkanNotice
#undef VulkanInfo
#undef VulkanDebug
