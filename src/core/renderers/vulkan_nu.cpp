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

	b32 remake_window; 
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

struct BufferVk{
	VkBuffer               buffer;
	VkDeviceMemory         memory;
	VkDeviceSize           size; //size of data, not allocation
	VkDescriptorBufferInfo descriptor;
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

VkBlendOp
graphics_blend_op_to_vulkan(GraphicsBlendOp x) {
	switch(x) {
		case GraphicsBlendOp_Add:         return VK_BLEND_OP_ADD;
		case GraphicsBlendOp_Max:         return VK_BLEND_OP_MAX;
		case GraphicsBlendOp_Min:         return VK_BLEND_OP_MIN;
		case GraphicsBlendOp_Sub:         return VK_BLEND_OP_SUBTRACT;
		case GraphicsBlendOp_Reverse_Sub: return VK_BLEND_OP_REVERSE_SUBTRACT;
	}
	VulkanFatal("invalid GraphicsBlendOp: ", (u32)x);
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

GraphicsFormat
vulkan_format_to_graphics(VkFormat x) {
	switch(x) {
		case VK_FORMAT_R32G32_SFLOAT:      return GraphicsFormat_R32G32_Float;
		case VK_FORMAT_R32G32B32_SFLOAT:   return GraphicsFormat_R32G32B32_Float;
		case VK_FORMAT_R8G8B8A8_SRGB:      return GraphicsFormat_R8G8B8A8_SRGB;
		case VK_FORMAT_R8G8B8A8_UNORM:     return GraphicsFormat_R8G8B8A8_UNorm;
		case VK_FORMAT_B8G8R8A8_UNORM:     return GraphicsFormat_B8G8R8A8_UNorm;
		case VK_FORMAT_D16_UNORM:          return GraphicsFormat_Depth16_UNorm;
		case VK_FORMAT_D32_SFLOAT:         return GraphicsFormat_Depth32_Float;
		case VK_FORMAT_D32_SFLOAT_S8_UINT: return GraphicsFormat_Depth32_Float_Stencil8_UInt;
		case VK_FORMAT_D24_UNORM_S8_UINT:  return GraphicsFormat_Depth24_UNorm_Stencil8_UInt;
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
VkCommandBuffer&       get_handle(GraphicsCommandBuffer* x)       { return (VkCommandBuffer&)GRAPHICS_INTERNAL(x).handle; }

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

local void
create_and_map_buffer(VkBuffer* buffer,
				      VkDeviceMemory* buffer_memory,
					  VkDeviceSize* buffer_size,
					  size_t new_size,
					  void* data,
					  VkBufferUsageFlags usage,
					  VkMemoryPropertyFlags properties) {
	vkDestroyBuffer(vk_device, *buffer, vk_allocator);
	vkFreeMemory(vk_device, *buffer_memory, vk_allocator);
	
	// create buffer
	VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	buffer_info.       size = new_size;
	buffer_info.      usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	auto result = vkCreateBuffer(vk_device, &buffer_info, vk_allocator, buffer);
	VulkanAssertVk(result, "failed to create buffer.");
	
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(vk_device, *buffer, &req);
	
	// allocate buffer
	VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	alloc_info. allocationSize = req.size;
	alloc_info.memoryTypeIndex = find_memory_type(req.memoryTypeBits, properties);
	result = vkAllocateMemory(vk_device, &alloc_info, vk_allocator, buffer_memory);
	VulkanAssertVk(result, "failed to allocate memory.");
	
	if(data != 0) {
		void* mapped = 0;
		result = vkMapMemory(vk_device, *buffer_memory, 0, new_size, 0, &mapped);
		VulkanAssertVk(result, "failed to map memory.");

		CopyMemory(mapped, data, new_size);
		
		if(!HasFlag(properties, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			VkMappedMemoryRange mapped_range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
			mapped_range.memory = *buffer_memory;
			mapped_range.offset = 0;
			mapped_range.  size = new_size;
			vkFlushMappedMemoryRanges(vk_device, 1, &mapped_range);
			
		}
		vkUnmapMemory(vk_device, *buffer_memory);
	}
	
	vkBindBufferMemory(vk_device, *buffer, *buffer_memory, 0);
	*buffer_size = new_size;
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
	VulkanInfo("creating image.");
	
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
	image_info.samples       = VK_SAMPLE_COUNT_1_BIT; // TODO(sushi) more samples
	image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	auto result = vkCreateImage(vk_device, &image_info, vk_allocator, image); 
	VulkanAssertVk(result, "failed to create image");
	
	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(vk_device, *image, &mem_reqs);
	
	VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	alloc_info.allocationSize = mem_reqs.size;
	alloc_info.memoryTypeIndex = find_memory_type(mem_reqs.memoryTypeBits, properties);
	result = vkAllocateMemory(vk_device, &alloc_info, vk_allocator, image_memory); 
	VulkanAssertVk(result, "failed to allocate image memory");
	
	vkBindImageMemory(vk_device, *image, *image_memory, 0);
}

VkImageView
create_image_view(VkImage image, 
		          VkFormat format, 
				  VkImageAspectFlags aspect_flags, 
				  u32 mip_levels) {
	VulkanInfo("creating image view.");
	
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
	auto result = vkCreateImageView(vk_device, &info, vk_allocator, &view);
	VulkanAssertVk(result, "failed to create image view.");

	return view;
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

local VkPipelineShaderStageCreateInfo
load_shader(str8 name, str8 source, VkShaderStageFlagBits stage){DPZoneScoped;
	if(!source) return VkPipelineShaderStageCreateInfo{};
	VulkanInfo("compiling shader: ", name);

	Stopwatch watch = start_stopwatch();
	
	//try to compile from GLSL to SPIR-V binary
	shaderc_compilation_result_t compiled;
	if      (stage == VK_SHADER_STAGE_VERTEX_BIT){
		compiled = shaderc_compile_into_spv(vk_shader_compiler, (const char*)source.str, source.count, shaderc_glsl_vertex_shader,
											(const char*)name.str, "main", vk_shader_compiler_options);
	}else if(stage == VK_SHADER_STAGE_FRAGMENT_BIT){
		compiled = shaderc_compile_into_spv(vk_shader_compiler, (const char*)source.str, source.count, shaderc_glsl_fragment_shader,
											(const char*)name.str, "main", vk_shader_compiler_options);
	}else if(stage == VK_SHADER_STAGE_GEOMETRY_BIT){
		compiled = shaderc_compile_into_spv(vk_shader_compiler, (const char*)source.str, source.count, shaderc_glsl_geometry_shader,
											(const char*)name.str, "main", vk_shader_compiler_options);
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
	file_write_simple(str8_concat3(STR8("data/shaders/"),name,str8_lit(".spv"), deshi_temp_allocator), (void*)shaderc_result_get_bytes(compiled), shaderc_result_get_length(compiled));
	
	//create shader module
	VkShaderModule shaderModule{};
	VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	moduleInfo.codeSize = shaderc_result_get_length(compiled);
	moduleInfo.pCode    = (u32*)shaderc_result_get_bytes(compiled);
	auto result = vkCreateShaderModule(vk_device, &moduleInfo, vk_allocator, &shaderModule); 
	VulkanAssertVk(result, "failed to create shader module.");
	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_SHADER_MODULE, (u64)shaderModule, (const char*)to_dstr8v(deshi_temp_allocator, "Shader Module ",name).str);
	
	//setup shader stage create info
	VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	shaderStage.stage  = stage;
	shaderStage.pName  = "main";
	shaderStage.module = shaderModule;
	
	VulkanInfo("finished compiling shader in ", peek_stopwatch(watch), "ms.");
	return shaderStage;
}

local void
copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
	VkCommandBuffer cb = begin_single_time_commands();
	
	VkBufferCopy copy_region{};
	copy_region.size = size;
	vkCmdCopyBuffer(cb, src, dst, 1, &copy_region);
	
	end_single_time_commands(cb);
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
	
	if(is_debugging) {
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
		wininf->present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	} else if(mailbox) {
		wininf->present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
	} else if(fifo_relaxed) {
		wininf->present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	} else {
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
	color_attachment.         load_op = GraphicsLoadOp_Clear;
	color_attachment.        store_op = GraphicsStoreOp_Store;
	color_attachment. stencil_load_op = GraphicsLoadOp_Dont_Care;
	color_attachment.stencil_store_op = GraphicsStoreOp_Dont_Care;
	color_attachment.  initial_layout = GraphicsImageLayout_Undefined;
	color_attachment.    final_layout = GraphicsImageLayout_Present;
	
	GraphicsRenderPassAttachment depth_attachment;
	depth_attachment.          format = vulkan_format_to_graphics(find_depth_format());
	depth_attachment.         load_op = GraphicsLoadOp_Clear;
	depth_attachment.        store_op = GraphicsStoreOp_Store;
	depth_attachment. stencil_load_op = GraphicsLoadOp_Clear;
	depth_attachment.stencil_store_op = GraphicsStoreOp_Dont_Care;
	depth_attachment.  initial_layout = GraphicsImageLayout_Undefined;
	depth_attachment.    final_layout = GraphicsImageLayout_Depth_Stencil_Attachment_Optimal;
	
	auto render_pass = graphics::RenderPass::allocate();
	render_pass->debug_name = str8l("<graphics> default render pass");
	render_pass->use_color_attachment = true;
	render_pass->color_attachment = color_attachment;
	render_pass->use_depth_attachment = true;
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
		color_image_view->image = color_image;
		depth_image_view->image = depth_image;

		color_image->           format = vulkan_format_to_graphics(wininf->surface_format.format);
		color_image->           extent = {window->width, window->height};
		color_image->            usage = GraphicsImageUsage_Color_Attachment;
		color_image->memory_properties = GraphicsMemoryPropertyFlag_DeviceLocal;
		color_image->__internal.handle = (void*)images[i];
		// NOTE(sushi) no reason to call render_image_update() here because we get the handle to the image from the swapchain (which is kinda stupid but whatever)
		
		depth_image->           format = vulkan_format_to_graphics(find_depth_format());
		depth_image->           extent = {window->width, window->height};
		depth_image->            usage = GraphicsImageUsage_Depth_Stencil_Attachment;
		depth_image->memory_properties = GraphicsMemoryPropertyFlag_DeviceLocal;
		depth_image->update();
		
		color_image_view->format = color_image->format;
		color_image_view->aspect_flags = GraphicsImageViewAspectFlags_Color;
		color_image_view->update();

		depth_image_view->format = depth_image->format;
		depth_image_view->aspect_flags = GraphicsImageViewAspectFlags_Depth;
		depth_image_view->update();

		frame->color_image_view = color_image_view;
		frame->depth_image_view = depth_image_view;


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
recreate_frames(Window* window) {
	// NOTE(sushi) we need to re-create these things because the window will already have 
	//             active render-api handles which we can just reuse. This also avoids an 
	//             issue with render commands that take RenderFrames, as if we don't do this
	//             any command that references a previous handle would still be pointing 
	//             at a destroyed swapchain's image
	// the goal of this function is to recreate all of this information w/o disturbing any 
	// possible references to any handle encountered in the process. This includes:
	// The GraphicsFramebuffer handles
	// The GraphicsImageView handles contained by those frames
	// The GraphicsImage handles that those views point to
	VulkanInfo("recreating frames for window ", window->title);
	
	auto wininf = (WindowInfo*)window->render_info;
	
	GraphicsRenderPass* render_pass = wininf->presentation_frames[0]->render_pass;
	
	u32 old_image_count = wininf->image_count;
	
	vkGetSwapchainImagesKHR(vk_device, wininf->swapchain, &wininf->image_count, 0);
	auto images = array<VkImage>::create_with_count(wininf->image_count, deshi_temp_allocator);
	vkGetSwapchainImagesKHR(vk_device, wininf->swapchain, &wininf->image_count, images.ptr);
	
	// TODO(sushi) I'm not sure if this should ever happen, but if it does and we have less frames
	//             than before, we run the risk of external handles to those frames needing to be 
	//             invalidated (possibly). Handle this if it ever becomes a problem.
	if(old_image_count != wininf->image_count) {
		VulkanFatal("FATAL INTERNAL ERROR: while recreating frames, the amount of swapchain images differed from its original value. This situation is not handled yet and should be reported immediately.");
	}
	
	forI(wininf->image_count) {
		GraphicsFramebuffer* frame = wininf->presentation_frames[i];
		frame->width = window->width;
		frame->height = window->height;
		frame->render_pass = render_pass;
		
		auto color_image_view = frame->color_image_view; 
		auto depth_image_view = frame->depth_image_view; 
		auto color_image      = color_image_view->image; 
		auto depth_image      = depth_image_view->image; 
		
		color_image->extent = {window->width, window->height};
		get_handle(color_image) = images[i];
		
		depth_image->extent = {window->width, window->height};
		graphics_image_update(depth_image);
		
		color_image_view->format = color_image->format;
		color_image_view->aspect_flags = GraphicsImageViewAspectFlags_Color;
		graphics_image_view_update(color_image_view);
		
		depth_image_view->format = depth_image->format;
		depth_image_view->aspect_flags = GraphicsImageViewAspectFlags_Depth;
		graphics_image_view_update(depth_image_view);
		
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
		VulkanAssertVk(result, "failed to recreate framebuffer for window.");
		vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_FRAMEBUFFER, (u64)get_handle(frame), "<graphics> default framebuffer");
	}
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

	g_graphics->debugging = true;
	g_graphics->logging_level = -1;
	g_graphics->break_on_error = true;

	VulkanNotice("initializing graphics module for window '", window->title, "'.");
	
	Stopwatch watch = start_stopwatch();
	
	// TODO(sushi) setup allocators specifically for graphics
	primary_allocator = deshi_allocator;
	temp_allocator = deshi_temp_allocator;

	memory_pool_init(window_infos, 4);

	if(!file_exists(str8l("data/shaders/"))) {
		VulkanNotice("data/shaders/ is missing, creating it now.");
		file_create(str8l("data/shaders/"));
	}

	// TODO(sushi) this should be moved to an implementation shared between backends
	memory_pool_init(g_graphics->pools.descriptor_set_layouts, 8);
	memory_pool_init(g_graphics->pools.descriptor_sets, 8);
	memory_pool_init(g_graphics->pools.pipeline_layouts, 8);
	memory_pool_init(g_graphics->pools.pipelines, 8);
	memory_pool_init(g_graphics->pools.buffers, 8);
	memory_pool_init(g_graphics->pools.command_buffers, 8);
	memory_pool_init(g_graphics->pools.images, 8);
	memory_pool_init(g_graphics->pools.image_views, 8);
	memory_pool_init(g_graphics->pools.samplers, 8);
	memory_pool_init(g_graphics->pools.render_passes, 8);
	memory_pool_init(g_graphics->pools.framebuffers, 8);

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
	wi->command_buffer->commands = array<GraphicsCommand>::create(primary_allocator).ptr;
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
// @init


void
graphics_update(Window* window) {
	Stopwatch watch = start_stopwatch();
	auto wininf = (WindowInfo*)window->render_info;

	if(window->resized || wininf->remake_window) {
		if(window->width <= 0 || window->height <= 0) return;
		vkDeviceWaitIdle(vk_device);
		create_swapchain(window);
		recreate_frames(window);
		wininf->frame_index = 0;
		wininf->remake_window = false;
	}
	
	renderStats = {};
	
	VkResult result = vkAcquireNextImageKHR(vk_device, wininf->swapchain, UINT64_MAX, vk_semaphore_image_acquired, VK_NULL_HANDLE, &wininf->frame_index);
	if(result == VK_ERROR_OUT_OF_DATE_KHR) {
		VulkanNotice("Window ", window->title, "'s surface has changed.");
		// the surface has changed in some way that makes it no longer compatible with its swapchain
		// so we need to recreate the swapchain
		wininf->remake_window = true;
		return;
	} else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		Assert(!"failed to acquire swapchain image");
	}
	

	//// build commands ////
	

	VkClearValue clear_values[2];

	GraphicsPipeline* currently_bound_pipeline = 0;
	GraphicsRenderPass* current_render_pass = 0;

	// for keeping track of what dynamic states 
	// are required and what have been fulfilled
	enum {
		Static,
		DynamicPending,
		DynamicFulfilled,
	};

	struct {
		u32 viewport       : 2;
		u32 scissor        : 2;
		u32 depth_bias     : 2;
		u32 line_width     : 2;
		u32 blend_constant : 2;
	} dynamic_state;

	
	VkCommandBufferBeginInfo cmd_buffer_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	auto cmdbuf = get_handle(wininf->command_buffer);
	result = vkBeginCommandBuffer(cmdbuf, &cmd_buffer_info);
	VulkanAssertVk(result, "failed to begin command buffer for window '", window->title, "'.");
	
	forI(array_count(wininf->command_buffer->commands)) {
		auto cmd = wininf->command_buffer->commands[i];
		switch(cmd.type) {
			case GraphicsCommandType_Begin_Render_Pass: {
				if(current_render_pass) {
					VulkanError("attempted to begin a render pass (", cmd.begin_render_pass.pass->debug_name, ") while one is already in progress (", current_render_pass->debug_name, ")");
					return;
				}
				auto pass = cmd.begin_render_pass.pass;
				auto frame = cmd.begin_render_pass.frame;
				VulkanAssert(pass, "encountered begin render pass command with a null render pass handle.");
				VulkanAssert(frame, "encountered begin render pass command with a null framebuffer handle.");

				VkRenderPassBeginInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
				info.renderPass = get_handle(pass);
				info.framebuffer = get_handle(frame);
				info.renderArea.offset = {0,0};
				info.renderArea.extent = {frame->width, frame->height};
				info.pClearValues = clear_values;

				if(pass->use_color_attachment) {
					clear_values[0].color = {
						pass->color_clear_values.r,
						pass->color_clear_values.g,
						pass->color_clear_values.b,
						pass->color_clear_values.a
					};
					if(pass->use_depth_attachment) {
						clear_values[1].depthStencil = {
							pass->depth_clear_values.depth,
							pass->depth_clear_values.stencil,
						};
						info.clearValueCount = 2;
					} else info.clearValueCount = 1;
				} else if(pass->use_depth_attachment) {
					clear_values[0].depthStencil = {
						pass->depth_clear_values.depth,
						pass->depth_clear_values.stencil,
					};
					info.clearValueCount = 1;
				}

				vk_debug_begin_label(cmdbuf, (char*)pass->debug_name.str, {0.2, 0.4, 0.8, 1.f});
				vkCmdBeginRenderPass(cmdbuf, &info, VK_SUBPASS_CONTENTS_INLINE);
				current_render_pass = pass;
			} break;
			case GraphicsCommandType_End_Render_Pass: {
				VulkanAssert(current_render_pass, "encountered end render pass command but no render pass has been started yet.");
				vkCmdEndRenderPass(cmdbuf);
				vk_debug_end_label(cmdbuf);
				current_render_pass = 0;
			} break;
			case GraphicsCommandType_Bind_Pipeline: {
				auto pipeline = cmd.bind_pipeline.handle;
				VulkanAssert(pipeline, "encountered bind pipeline command, but the given pipeline handle is null.");
				currently_bound_pipeline = pipeline;
				dynamic_state.viewport       = (pipeline->dynamic_viewport?       DynamicPending : Static);
				dynamic_state.scissor        = (pipeline->dynamic_scissor?        DynamicPending : Static);
				dynamic_state.depth_bias     = (pipeline->dynamic_depth_bias?     DynamicPending : Static);
				dynamic_state.line_width     = (pipeline->dynamic_line_width?     DynamicPending : Static);
				dynamic_state.blend_constant = (pipeline->dynamic_blend_constant? DynamicPending : Static);
				vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, get_handle(pipeline));
			} break;
			case GraphicsCommandType_Set_Viewport: {
				if(dynamic_state.viewport == Static) {
					VulkanError("encountered set viewport command, but the currently bound pipeline (", currently_bound_pipeline->debug_name, ") does not have viewport set as dynamic.");
					continue;
				}
				VkViewport v = {0};
				v.x = cmd.set_viewport.offset.x;
				v.y = cmd.set_viewport.offset.y;
				v.width = cmd.set_viewport.extent.x;
				v.height = cmd.set_viewport.extent.y;
				v.minDepth = 0.f;
				v.maxDepth = 1.f;
				vkCmdSetViewport(cmdbuf, 0, 1, &v);
				dynamic_state.viewport = DynamicFulfilled;
			} break;
			case GraphicsCommandType_Set_Scissor: {
				if(dynamic_state.scissor == Static) {
					VulkanError("encountered set scissor command, but the currently bound pipeline (", currently_bound_pipeline->debug_name, ") does not have scissors set to dynamic.");
					continue;
				}
				VkRect2D s = {0};
				s.offset.x = cmd.set_scissor.offset.x;
				s.offset.y = cmd.set_scissor.offset.y;
				s.extent.width = cmd.set_scissor.extent.x;
				s.extent.height = cmd.set_scissor.extent.y;
				vkCmdSetScissor(cmdbuf, 0, 1, &s);
				dynamic_state.scissor = DynamicFulfilled;
			} break;
			case GraphicsCommandType_Set_Depth_Bias: {
				if(dynamic_state.depth_bias == Static) {
					VulkanError("encountered set depth bias command, but the currently bound pipeline (", currently_bound_pipeline->debug_name, ") does not have depth bias set as dynamic.");
					continue;
				}
				vkCmdSetDepthBias(cmdbuf, cmd.set_depth_bias.constant, cmd.set_depth_bias.clamp, cmd.set_depth_bias.slope);
				dynamic_state.depth_bias = DynamicFulfilled;
			} break;
			case GraphicsCommandType_Bind_Vertex_Buffer: {
				VulkanAssert(cmd.bind_vertex_buffer.handle, "encountered bind vertex buffer command, but the buffer handle is null.");
				VkDeviceSize offsets[1] = {0};
				vkCmdBindVertexBuffers(cmdbuf, 0, 1, &get_handle(cmd.bind_vertex_buffer.handle), offsets);
			} break;
			case GraphicsCommandType_Bind_Index_Buffer: {
				VulkanAssert(cmd.bind_index_buffer.handle, "encountered bind index buffer command, but the buffer handle is null.");
				vkCmdBindIndexBuffer(cmdbuf, get_handle(cmd.bind_index_buffer.handle), 0, VK_INDEX_TYPE_UINT32);
			} break;
			case GraphicsCommandType_Bind_Descriptor_Set: {
				vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, 
										get_handle(currently_bound_pipeline->layout),
										cmd.bind_descriptor_set.set_index, 1, 
										&get_handle(cmd.bind_descriptor_set.handle),
										0, 0);
			} break;
			case GraphicsCommandType_Push_Constant: {
				VulkanAssert(currently_bound_pipeline, "encountered push constant command, but no pipeline has been bound yet.");
				vkCmdPushConstants(cmdbuf, 
								   get_handle(currently_bound_pipeline->layout),
								   graphics_shader_stage_to_vulkan(cmd.push_constant.info.shader_stages),
								   cmd.push_constant.info.offset, cmd.push_constant.info.size, cmd.push_constant.data);
			} break;
			case GraphicsCommandType_Draw_Indexed: {
				// TODO(sushi) checks for dynamic state stuff having been set before this occurs
				vkCmdDrawIndexed(cmdbuf, cmd.draw_indexed.index_count, 1, cmd.draw_indexed.index_offset, cmd.draw_indexed.vertex_offset, 0);
			} break;
		}
	}

	result = vkEndCommandBuffer(cmdbuf);
	VulkanAssertVk(result, "failed to end command buffer.");

	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &vk_semaphore_image_acquired;
	submit_info.pWaitDstStageMask = &wait_stage;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &get_handle(wininf->command_buffer);
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &vk_semaphore_render_complete;
	result = vkQueueSubmit(vk_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	VulkanAssertVk(result, "failed to submit commands to queue.");

	if(wininf->remake_window) return;

	VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &vk_semaphore_render_complete;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = (VkSwapchainKHR*)&wininf->swapchain;
	present_info.pImageIndices = &wininf->frame_index;
	present_info.pResults = 0;
	result = vkQueuePresentKHR(vk_present_queue, &present_info);
	
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || wininf->remake_window) {
		VulkanNotice("remaking swapchain and frames for window '", window->title, "'.");
		vkDeviceWaitIdle(vk_device);
		create_swapchain(window);
		recreate_frames(window);
		wininf->remake_window = false;
	} else if(result != VK_SUCCESS) {
		VulkanFatal("failed to queue present.");
	}
	
	array_clear(wininf->command_buffer->commands);
	
	wininf->frame_index = (wininf->frame_index + 1) % wininf->min_image_count;
	
	result = vkQueueWaitIdle(vk_graphics_queue);
	switch (result){
		case VK_ERROR_OUT_OF_HOST_MEMORY:   VulkanFatal("Host is out of memory!"); break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: VulkanFatal("Device is out of memory!"); break;
		case VK_ERROR_DEVICE_LOST:          VulkanFatal("Device lost!"); break;
		case VK_SUCCESS:default: break;
	}
	
	//update renderStats
	renderStats.drawnTriangles += renderStats.drawnIndices / 3;
	//renderStats.totalVertices  += (u32)vertexBuffer.size() + renderTwodVertexCount + renderTempWireframeVertexCount;
	//renderStats.totalIndices   += (u32)indexBuffer.size()  + renderTwodIndexCount  + renderTempWireframeIndexCount; //!Incomplete
	renderStats.totalTriangles += renderStats.totalIndices / 3;
	renderStats.renderTimeMS = peek_stopwatch(watch);
	
	
	g_time->renderTime = peek_stopwatch(watch);
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
	VulkanInfo("creating a buffer.");

	VulkanAssert(requested_size, "cannot create a buffer with size 0.");
	if(HasFlag(properties, GraphicsMemoryProperty_HostStreamed) && 
	   HasFlag(properties, GraphicsMemoryPropertyFlag_LazilyAllocated)) {
		VulkanError("the memory property flags 'HostStreamed' and 'LazilyAllocated' are incompatible.");
		return 0;
	}

	GraphicsBuffer* out = memory_pool_push(g_graphics->pools.buffers);

	VkBufferCreateInfo create_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	create_info.size = requested_size;
	create_info.usage = graphics_buffer_usage_to_vulkan(usage);
	if(mapping_behavoir == GraphicsMemoryMapping_Never) {
		create_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	auto result = vkCreateBuffer(vk_device, &create_info, vk_allocator, &get_handle(out));
	VulkanAssertVk(result, "failed to create buffer.");
	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_BUFFER, (u64)get_handle(out), "<graphics> buffer");

	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(vk_device, get_handle(out), &req);
	GRAPHICS_INTERNAL(out).size = req.size;

	VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	alloc_info.allocationSize = req.size;
	alloc_info.memoryTypeIndex = find_memory_type(req.memoryTypeBits, graphics_memory_properties_to_vulkan(properties));
	result = vkAllocateMemory(vk_device, &alloc_info, vk_allocator, &get_memory_handle(out));
	VulkanAssertVk(result, "failed to allocate memory for buffer.");
	result = vkBindBufferMemory(vk_device, get_handle(out), get_memory_handle(out), 0);
	VulkanAssertVk(result, "failed to bind buffer to memory.");
	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_DEVICE_MEMORY, (u64)get_memory_handle(out), "<graphics> memory");

	if(mapping_behavoir == GraphicsMemoryMapping_Never) {
		if(!data) {
			VulkanWarning("the mapping behavoir of the buffer we are creating is set to 'Never', but the given data pointer is null. I'm not sure yet if there is a legitamate usecase for this, so remove this warning if we ever come across one.");
		} else {
			VulkanAssert(!HasFlag(properties,GraphicsMemoryPropertyFlag_HostCoherent|GraphicsMemoryPropertyFlag_HostVisible|GraphicsMemoryPropertyFlag_HostCached), 
					"incompatible mapping behavoir and memory flags. Mapping behavoir is set to 'Never', which is incompatible with the memory properties 'HostVisible', 'HostCoherent', or 'HostCached'.");
			// create a staging buffer
			VkBuffer stage;
			VkBufferCreateInfo staging_buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
			staging_buffer_info.size        = requested_size;
			staging_buffer_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			staging_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			result = vkCreateBuffer(vk_device, &staging_buffer_info, vk_allocator, &stage); 
			VulkanAssertVk(result, "failed to create staging buffer for GraphicsBuffer whose mapping behavoir is 'Never'");

			VkDeviceMemory stage_memory;
			VkMemoryRequirements stage_req;
			vkGetBufferMemoryRequirements(vk_device, stage, &stage_req);
			VkMemoryAllocateInfo stage_alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
			stage_alloc_info.allocationSize = stage_req.size;
			stage_alloc_info.memoryTypeIndex = find_memory_type(stage_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			result = vkAllocateMemory(vk_device, &stage_alloc_info, vk_allocator, &stage_memory);
			VulkanAssertVk(result, "failed to allocate memory for staging buffer.");
			result = vkBindBufferMemory(vk_device, stage, stage_memory, 0);
			VulkanAssertVk(result, "failed to bind staging buffer to memory.");

			void* stage_data;
			result = vkMapMemory(vk_device, stage_memory, 0, req.size, 0, &stage_data);
			VulkanAssertVk(result, "failed to map staging buffer's memory.");
			CopyMemory(stage_data, data, requested_size);
			vkUnmapMemory(vk_device, stage_memory);
				
			copy_buffer(stage, get_handle(out), requested_size);

			vkDestroyBuffer(vk_device, stage, vk_allocator);
			vkFreeMemory(vk_device, stage_memory, vk_allocator);
		} 
	} else if(mapping_behavoir == GraphicsMemoryMapping_Occasional) {
		if(data) {
			auto result = vkMapMemory(vk_device, get_memory_handle(out), 0, req.size, 0, &GRAPHICS_INTERNAL(out).mapped.data);
			VulkanAssertVk(result, "failed to map memory for initial data copy to GraphicsBuffer. (Mapping behavoir is 'Occasional')");

			CopyMemory(GRAPHICS_INTERNAL(out).mapped.data, data, requested_size);

			VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
			range.memory = get_memory_handle(out);
			range.offset = 0;
			range.  size = req.size;
			result = vkFlushMappedMemoryRanges(vk_device, 1, &range);
			VulkanAssertVk(result, "failed to flush memory range while performing initial copy to GraphicsBuffer. (Mapping behavoir is set to 'Occasional')");

			vkUnmapMemory(vk_device, get_memory_handle(out));
		}
		GRAPHICS_INTERNAL(out).mapped = {};
	} else if(mapping_behavoir == GraphicsMemoryMapping_Persistent) {
		auto result = vkMapMemory(vk_device, get_memory_handle(out), 0, req.size, 0, &GRAPHICS_INTERNAL(out).mapped.data);
		VulkanAssertVk(result, "failed to perform initial mapping for GraphicsBuffer with 'Persistent' mapping behavoir.");

		if(data) {
			CopyMemory(GRAPHICS_INTERNAL(out).mapped.data, data, requested_size);

			VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
			range.memory = get_memory_handle(out);
			range.offset = 0;
			range.  size = req.size;
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
		VulkanAssertVk(result, "failed to flush memory to device.");
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


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @image


GraphicsImage*
graphics_image_allocate() {
	return memory_pool_push(g_graphics->pools.images);
}

void
graphics_image_update(GraphicsImage* x) {
	VulkanAssert(x, "passed null GraphicsImage pointer.");

	VulkanInfo("updating a GraphicsImage (name: ", x->debug_name, "; addr: ", (void*)x, ").");

	vkDestroyImage(vk_device, get_handle(x), vk_allocator);
	vkFreeMemory(vk_device, get_memory_handle(x), vk_allocator);

	create_image(
		 x->extent.x, x->extent.y, 
		 1, 
		 graphics_sample_count_to_vulkan(x->samples),
		 graphics_format_to_vulkan(x->format),
		 (x->linear_tiling? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL),
		 graphics_image_usage_to_vulkan(x->usage),
		 graphics_memory_properties_to_vulkan(x->memory_properties),
		 &get_handle(x),
		 &get_memory_handle(x));

	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_IMAGE, (u64)get_handle(x), (char*)x->debug_name.str);
}

void
graphics_image_destroy(GraphicsImage* x) {
	VulkanAssert(x, "passed null GraphicsImage pointer.");
	VulkanInfo("destroying image '", x->debug_name, "'");
	vkFreeMemory(vk_device, get_memory_handle(x), vk_allocator);
	vkDestroyImage(vk_device, get_handle(x), vk_allocator);
	ZeroMemory(x, sizeof(GraphicsImage));
}

void
graphics_image_write(GraphicsImage* x, u8* pixels, vec2i offset, vec2i extent) {
	VulkanAssert(x, "passed null GraphicsImage pointer.");

	if(!(get_handle(x) && get_memory_handle(x))) {
		VulkanError("one of the backend handles is null indicating the object has been deleted, is corrupt, or has not been updated (graphics_image_update()) yet.");
		return;
	}

	if(!extent.y || !extent.x) {
#if BUILD_SLOW
		VulkanWarning("called with an extent ", extent, " with 0 in some axis.");
#endif
		return;
	}

	VkDeviceSize image_memsize = extent.x * extent.y * 4;
	
	// create a staging buffer so we can map the pixel data from the CPU
	BufferVk stage{};
	create_and_map_buffer(
		  &stage.buffer, 
		  &stage.memory, 
		  &image_memsize, 
		  (size_t)image_memsize, 
		  pixels,
		  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
		  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	VkCommandBuffer cmdbuf = begin_single_time_commands();
	
	VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	barrier.srcAccessMask       = 0;
	barrier.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = get_handle(x);
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = 1; // TODO(sushi) mipmaps
	barrier.subresourceRange.baseArrayLayer = 0; //NOTE(delle) use image flags here?
	barrier.subresourceRange.layerCount     = 1; //NOTE(delle) use image flags here?
	vkCmdPipelineBarrier(cmdbuf, 
						 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						 0,0,0,0,0,1, &barrier);
	
	VkBufferImageCopy region{};
	region.bufferOffset      = 0;
	region.bufferRowLength   = 0;
	region.bufferImageHeight = 0;
	region.imageOffset       = {offset.x, offset.y, 0};
	region.imageExtent       = {(u32)extent.x, (u32)extent.y, 1};
	region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel       = 0;
	region.imageSubresource.baseArrayLayer = 0; //NOTE(delle) use image flags here?
	region.imageSubresource.layerCount     = 1; //NOTE(delle) use image flags here?
	vkCmdCopyBufferToImage(cmdbuf, 
						   stage.buffer, get_handle(x),
						   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
						   1, &region);
	
	barrier.srcAccessMask       = 0;
	barrier.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = get_handle(x);
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = 1; // TODO(sushi) mipmaps
	barrier.subresourceRange.baseArrayLayer = 0; //NOTE(delle) use image flags here?
	barrier.subresourceRange.layerCount     = 1; //NOTE(delle) use image flags here?
	vkCmdPipelineBarrier(cmdbuf, 
						 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						 0,0,0,0,0,1, &barrier);
	
	end_single_time_commands(cmdbuf);
	
	vkDestroyBuffer(vk_device, stage.buffer, vk_allocator);
	vkFreeMemory(vk_device, stage.memory, vk_allocator);
}

GraphicsImageView* 
graphics_image_view_allocate() {
	return memory_pool_push(g_graphics->pools.image_views);
}

void 
graphics_image_view_update(GraphicsImageView* x) {
	VulkanAssert(x, "passed null GraphicsImageView pointer.");
	VulkanInfo("updating image view '", x->debug_name, "'.");
	
	vkDestroyImageView(vk_device, get_handle(x), vk_allocator);
	
	get_handle(x) = create_image_view(get_handle(x->image), graphics_format_to_vulkan(x->format), graphics_image_view_aspect_to_vulkan(x->aspect_flags), 1);
	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)get_handle(x), (char*)x->debug_name.str);
}

void 
graphics_image_view_destroy(GraphicsImageView* x) {
	VulkanAssert(x, "passed null GraphicsImageView pointer.");
	VulkanInfo("destroying image view '", x->debug_name, "'.");
	vkDestroyImageView(vk_device, get_handle(x), vk_allocator);
	ZeroMemory(x, sizeof(GraphicsImageView));
	memory_pool_delete(g_graphics->pools.image_views, x);
}

GraphicsSampler* 
graphics_sampler_allocate() {
	return memory_pool_push(g_graphics->pools.samplers);
}

void 
graphics_sampler_update(GraphicsSampler* x) {
	VulkanAssert(x, "passed null GraphicsSampler pointer");
	VulkanInfo("updating sampler '", x->debug_name, "'.");

	VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	
	info.magFilter = graphics_filter_to_vulkan(x->mag_filter);
	info.minFilter = graphics_filter_to_vulkan(x->min_filter);
	info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // TODO)(sushi) add this to the api
	info.addressModeU = graphics_sampler_address_mode_to_vulkan(x->address_mode_u);
	info.addressModeV = graphics_sampler_address_mode_to_vulkan(x->address_mode_v);
	info.addressModeW = graphics_sampler_address_mode_to_vulkan(x->address_mode_w);
	info.mipLodBias = 0.f;
	info.anisotropyEnable = VK_FALSE; // TODO(sushi) add to api
	info.unnormalizedCoordinates = VK_FALSE;
	info.compareEnable = VK_FALSE; // TODO(sushi) possibly add to api
	info.minLod = 0.f; // TODO(sushi) add to api
	info.maxLod = 0.f;
	
	auto result = vkCreateSampler(vk_device, &info, vk_allocator, &get_handle(x));
	VulkanAssertVk(result, "failed to create sampler.");
	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_SAMPLER, (u64)get_handle(x), (char*)x->debug_name.str);
}

void 
graphics_sampler_destroy(GraphicsSampler* x) {
	VulkanAssert(x, "passed null GraphicsSampler pointer.");
	VulkanInfo("destroying sampler '", x->debug_name, "'.");
	vkDestroySampler(vk_device, get_handle(x), vk_allocator);
	ZeroMemory(x, sizeof(GraphicsSampler));
	memory_pool_delete(g_graphics->pools.samplers, x);
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @descriptor


GraphicsDescriptorSetLayout* 
graphics_descriptor_set_layout_allocate() {
	return memory_pool_push(g_graphics->pools.descriptor_set_layouts);
}

void 
graphics_descriptor_set_layout_update(GraphicsDescriptorSetLayout* x) {
	VulkanAssert(x, "passed a null GraphicsDescriptorSetLayout pointer.");
	VulkanInfo("updating descriptor set layout '", x->debug_name, "'.");

	auto bindings = array_from(x->bindings);

	VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	if(!bindings.ptr) {
		info.pBindings = 0;
		info.bindingCount = 0;
		auto result = vkCreateDescriptorSetLayout(vk_device, &info, vk_allocator, &get_handle(x));
		VulkanAssertVk(result, "failed to create descriptor set layout (also, the provided GraphicsDescriptorSetLayout has a null binding array, so that may be a cause!).");
	}

	auto bindings_out = array<VkDescriptorSetLayoutBinding>::create_with_count(bindings.count(), temp_allocator);
	info.pBindings = bindings_out.ptr;
	info.bindingCount = bindings.count();

	forI(bindings.count()) {
		auto b = bindings[i];
		switch(b.type) {
			case GraphicsDescriptorType_Combined_Image_Sampler: {
				bindings_out[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			} break;
			case GraphicsDescriptorType_Uniform_Buffer: {
				bindings_out[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			} break;
		}
		bindings_out[i].stageFlags = graphics_shader_stage_to_vulkan(b.shader_stages);
		bindings_out[i].binding = b.n;
		bindings_out[i].descriptorCount = 1;
	}

	auto result = vkCreateDescriptorSetLayout(vk_device, &info, vk_allocator, &get_handle(x));
	VulkanAssertVk(result, "failed to create descriptor set layout.");
	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)get_handle(x), (char*)x->debug_name.str);
}

void 
graphics_descriptor_set_layout_destroy(GraphicsDescriptorSetLayout* x) {
	vkDestroyDescriptorSetLayout(vk_device, get_handle(x), vk_allocator);
	ZeroMemory(x, sizeof(GraphicsDescriptorSetLayout));
}

GraphicsDescriptorSet* 
graphics_descriptor_set_allocate() {
	return memory_pool_push(g_graphics->pools.descriptor_sets);
}

void 
graphics_descriptor_set_update(GraphicsDescriptorSet* x) {
	VulkanAssert(x, "passed a null GraphicsDescriptorSet pointer.");
	VulkanInfo("updating descriptor set '", x->debug_name, "'.");

	if(!x->layouts) {
		VulkanError("given descriptor set has a null layouts array. At least a single layout is required.");
		return;
	}

	auto layouts = array_from(x->layouts);
	auto layouts_out = array<VkDescriptorSetLayout>::create_with_count(layouts.count(), temp_allocator);
	
	forI(layouts.count()) {
		layouts_out[i] = get_handle(layouts[i]);
	}

	VkDescriptorSetAllocateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	info.descriptorPool = vk_descriptor_pool;
	info.pSetLayouts = layouts_out.ptr;
	info.descriptorSetCount = 1;

	auto result = vkAllocateDescriptorSets(vk_device, &info, &get_handle(x));
	VulkanAssertVk(result, "failed to allocate descriptor set.");
	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)get_handle(x), (char*)x->debug_name.str);
}

void 
graphics_descriptor_set_destroy(GraphicsDescriptorSet* x) {
	VulkanAssert(x, "passed a null GraphicsDescriptorSet pointer.");
	VulkanInfo("destroying descriptor set '", x->debug_name, "'.");
	vkFreeDescriptorSets(vk_device, vk_descriptor_pool, 1, &get_handle(x));
	ZeroMemory(x, sizeof(GraphicsDescriptorSet));
}

void 
graphics_descriptor_set_write(GraphicsDescriptorSet* x, u32 binding, GraphicsDescriptor descriptor) {
	VulkanAssert(x, "passed a null GraphicsDescriptorSet pointer.");
	VulkanInfo("writing a descriptor to descriptor set '", x->debug_name, "'. Binding: ", binding);
	
	if(!get_handle(x)) {
		VulkanError("given descriptor set has a null backend pointer, indicating deletion, corruption, or the descriptor set has not been updated yet (graphics_descriptor_set_update()).");
		return;
	}

	VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
	write.dstSet = get_handle(x);
	write.dstBinding = binding;
	write.descriptorCount = 1;

	union {
		VkDescriptorBufferInfo buffer_info;
		VkDescriptorImageInfo image_info;
	};

	switch(descriptor.type) {
		case GraphicsDescriptorType_Uniform_Buffer: {
			buffer_info.buffer = get_handle(descriptor.ubo.buffer);
			buffer_info.range = descriptor.ubo.range;
			buffer_info.offset = descriptor.ubo.offset;
			write.pBufferInfo = &buffer_info;
			write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		} break;
		case GraphicsDescriptorType_Combined_Image_Sampler: {
			image_info.imageView = get_handle(descriptor.image.view);
			image_info.sampler = get_handle(descriptor.image.sampler);
			image_info.imageLayout = graphics_image_layout_to_vulkan(descriptor.image.layout);
			write.pImageInfo = &image_info;
			write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		} break;
	}

	vkUpdateDescriptorSets(vk_device, 1, &write, 0, 0);
}

void 
graphics_descriptor_set_write_array(GraphicsDescriptorSet* x, GraphicsDescriptor* descriptors_) {
	VulkanAssert(x, "passed null GraphicsDescriptorSet pointer.");
	VulkanAssert(descriptors_, "passed null descriptors array.");
	VulkanInfo("writing descriptor array to descriptor set '", x->debug_name, "'.");

	if(!get_handle(x)) {
		VulkanError("given descriptor set has a null backend pointer, indicating deletion, corruption, or the descriptor set has not been updated yet (graphics_descriptor_set_update()).");
		return;
	}

	auto descriptors = array_from(descriptors_);

	auto writes = array<VkWriteDescriptorSet>::create_with_count(descriptors.count(), deshi_temp_allocator);
	// NOTE(sushi) these things can't move cause the writes have to point to them so we just allocate them
	//             with enough space to hold all the descriptors
	auto buffer_infos = array<VkDescriptorBufferInfo>::create(descriptors.count(), deshi_temp_allocator);
	auto image_infos = array<VkDescriptorImageInfo>::create(descriptors.count(), deshi_temp_allocator);
	
	forI(descriptors.count()) {
		auto d = descriptors[i];
		auto w = writes.ptr + i;
		*w = {};
		w->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		w->dstSet = get_handle(x);
		w->dstBinding = i;
		w->descriptorCount = 1;
		switch(d.type) {
			case GraphicsDescriptorType_Uniform_Buffer: {
				VulkanAssert(d.ubo.buffer, "descriptor ", i, " has a null buffer pointer.");
				VulkanAssert(get_handle(d.ubo.buffer) && get_memory_handle(d.ubo.buffer), "one of the backend handles of descriptor ", i, " is null, indicating deletion, corruption, or the object was never updated.");
				auto b = buffer_infos.push();
				b->buffer = get_handle(d.ubo.buffer);
				b-> range = d.ubo.range;
				b->offset = d.ubo.offset;
				w->pBufferInfo = b;
				w->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			} break;
			case GraphicsDescriptorType_Combined_Image_Sampler: {
				VulkanAssert(d.image.view, "descriptor ", i, " has a null image view pointer.");
				VulkanAssert(d.image.sampler, "descriptor ", i, " has a null sampler pointer.");
				VulkanAssert(get_handle(d.image.sampler), "the given sampler for descriptor ", i, " has a null backend handle.");
				VulkanAssert(get_handle(d.image.view), "the given image view for descriptor ", i, " has a null backend handle.");
				auto b = image_infos.push();
				b->imageView = get_handle(d.image.view);
				b->sampler = get_handle(d.image.sampler);
				b->imageLayout = graphics_image_layout_to_vulkan(d.image.layout);
				w->pImageInfo = b;
				w->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			} break;
		}
	}
	
	vkUpdateDescriptorSets(vk_device, writes.count(), writes.ptr, 0, 0);	
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @pipeline


GraphicsPipelineLayout* 
graphics_pipeline_layout_allocate() {
	return memory_pool_push(g_graphics->pools.pipeline_layouts);
}

void 
graphics_pipeline_layout_update(GraphicsPipelineLayout* x) {
	VulkanAssert(x, "passed a null GraphicsPipelineLayout pointer.");
	VulkanInfo("updating pipeline layout '", x->debug_name, "'.");

	vkDestroyPipelineLayout(vk_device, get_handle(x), vk_allocator);
	
	auto layouts = array_from(x->descriptor_layouts);
	auto constants = array_from(x->push_constants);
	
	auto layouts_out = array<VkDescriptorSetLayout>::create_with_count(layouts.count(), temp_allocator);
	forI(layouts.count()) {
		layouts_out[i] = get_handle(layouts[i]);
	}

	auto ranges = array<VkPushConstantRange>::create_with_count(constants.count(), temp_allocator);
	forI(constants.count()) {
		ranges[i].size = x->push_constants[i].size;
		ranges[i].offset = x->push_constants[i].offset;
		ranges[i].stageFlags = graphics_shader_stage_to_vulkan(x->push_constants[i].shader_stages);
	}

	VkPipelineLayoutCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	info.        setLayoutCount = layouts_out.count();
	info.           pSetLayouts = layouts_out.ptr;
	info.pushConstantRangeCount = ranges.count();
	info.   pPushConstantRanges = ranges.ptr;
	auto result = vkCreatePipelineLayout(vk_device, &info, vk_allocator, &get_handle(x));
	VulkanAssertVk(result, "failed to create pipeline layout.");
	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)get_handle(x), (char*)x->debug_name.str);
}

void 
graphics_pipeline_layout_destroy(GraphicsPipelineLayout* x) {
	VulkanAssert(x, "passed a null GraphicsPipelineLayout pointer.");
	VulkanInfo("destroying pipeline layout '", x->debug_name, "'.");
	vkDestroyPipelineLayout(vk_device, get_handle(x), vk_allocator);
	ZeroMemory(x, sizeof(GraphicsPipelineLayout));
}

GraphicsPipeline* 
graphics_pipeline_allocate() {
	return memory_pool_push(g_graphics->pools.pipelines);
}

void 
graphics_pipeline_update(GraphicsPipeline* x) {
	VulkanAssert(x, "passed null GraphicsPipeline pointer");
	VulkanAssert(x->layout, "the given pipeline has a null layout pointer. All pipelines must specify a GraphicsPipelineLayout.");
	VulkanAssert(get_handle(x->layout), "the given pipeline has a layout but its backend handle is null. Did you call graphics_pipeline_layout_update() on it?");
	VulkanInfo("updating pipeline '", x->debug_name, "'.");

	if(!x->shader_stages) {
		VulkanError("null shader stages array. A pipeline is required to at least define a vertex shader.");
		return;
	}

	auto shader_stages = array<VkPipelineShaderStageCreateInfo>::create_with_count(array_count(x->shader_stages), temp_allocator);
	forI(array_count(x->shader_stages)) {
		shader_stages[i] = load_shader(x->shader_stages[i].name, x->shader_stages[i].source, graphics_shader_stage_to_vulkan(x->shader_stages[i].shader_stage));
	}

   	VkPipelineInputAssemblyStateCreateInfo ias{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	ias.              topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	ias.primitiveRestartEnable = VK_FALSE;
	
    VkPipelineViewportStateCreateInfo vs{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	vs.viewportCount = 1;
	vs.   pViewports = 0;
	vs. scissorCount = 1;
	vs.    pScissors = 0;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	input_assembly_state.              topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_state{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewport_state.viewportCount = 1;
	viewport_state.   pViewports = 0;
	viewport_state. scissorCount = 1;
	viewport_state.    pScissors = 0;

	VkPipelineRasterizationStateCreateInfo rasterization_state{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	rasterization_state.       depthClampEnable = VK_FALSE;
	rasterization_state.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state.        depthBiasEnable = (x->depth_bias? VK_TRUE : VK_FALSE);
	rasterization_state.         depthBiasClamp = x->depth_bias_clamp;
	rasterization_state.   depthBiasSlopeFactor = x->depth_bias_slope;
	rasterization_state.depthBiasConstantFactor = x->depth_bias_constant;
	rasterization_state.              lineWidth = 1.f;
	switch(x->polygon_mode) {
		case GraphicsPolygonMode_Point: rasterization_state.polygonMode = VK_POLYGON_MODE_POINT; break;
		case GraphicsPolygonMode_Fill:  rasterization_state.polygonMode = VK_POLYGON_MODE_FILL; break;
		case GraphicsPolygonMode_Line:  rasterization_state.polygonMode = VK_POLYGON_MODE_LINE; break;
	}
	switch(x->culling) {
		case GraphicsPipelineCulling_None:       rasterization_state.cullMode = VK_CULL_MODE_NONE; break;
		case GraphicsPipelineCulling_Back:       rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT; break;
		case GraphicsPipelineCulling_Front:      rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT; break;
		case GraphicsPipelineCulling_Front_Back: rasterization_state.cullMode = VK_CULL_MODE_FRONT_AND_BACK; break;
	}
	switch(x->front_face) {
		case GraphicsFrontFace_CW:  rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE; break;
		case GraphicsFrontFace_CCW: rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; break;
	}

	VkPipelineColorBlendAttachmentState    color_blend_attachment_state{};
	color_blend_attachment_state.        blendEnable = (x->color_blend? VK_TRUE : VK_FALSE);
	color_blend_attachment_state.srcColorBlendFactor = graphics_blend_factor_to_vulkan(x->color_src_blend_factor);
	color_blend_attachment_state.srcAlphaBlendFactor = graphics_blend_factor_to_vulkan(x->alpha_src_blend_factor);
	color_blend_attachment_state.       colorBlendOp = graphics_blend_op_to_vulkan(x->color_blend_op);
	color_blend_attachment_state.dstColorBlendFactor = graphics_blend_factor_to_vulkan(x->color_dst_blend_factor);
	color_blend_attachment_state.dstAlphaBlendFactor = graphics_blend_factor_to_vulkan(x->alpha_dst_blend_factor);	
	color_blend_attachment_state.       alphaBlendOp = graphics_blend_op_to_vulkan(x->alpha_blend_op);
	color_blend_attachment_state.     colorWriteMask = 0xf;

	VkPipelineColorBlendStateCreateInfo    color_blend_state{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	color_blend_state.    logicOpEnable = VK_FALSE;
	color_blend_state.          logicOp = VK_LOGIC_OP_COPY;
	color_blend_state.  attachmentCount = 1;
	color_blend_state.     pAttachments = &color_blend_attachment_state;
	color_blend_state.blendConstants[0] = x->blend_constant.r/255.f;
	color_blend_state.blendConstants[1] = x->blend_constant.g/255.f;
	color_blend_state.blendConstants[2] = x->blend_constant.b/255.f;
	color_blend_state.blendConstants[3] = x->blend_constant.a/255.f;

	VkPipelineDepthStencilStateCreateInfo  depth_stencil_state{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
	depth_stencil_state.      depthTestEnable = (x->depth_test? VK_TRUE : VK_FALSE);
	depth_stencil_state.     depthWriteEnable = (x->depth_writes? VK_TRUE : VK_FALSE);
	depth_stencil_state.       depthCompareOp = graphics_compare_op_to_vulkan(x->depth_compare_op);
	depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_state.    stencilTestEnable = VK_FALSE;
	depth_stencil_state.       minDepthBounds = 0.f;
	depth_stencil_state.       maxDepthBounds = 1.f;
	depth_stencil_state.    stencilTestEnable = VK_FALSE;
	depth_stencil_state.                front = {};
	depth_stencil_state.       back.compareOp = VK_COMPARE_OP_ALWAYS;

	VkPipelineMultisampleStateCreateInfo   multisample_state{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	multisample_state.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	multisample_state. sampleShadingEnable  = VK_FALSE;
	multisample_state.    minSampleShading  = .2f;
	multisample_state.         pSampleMask  = 0;
	multisample_state.alphaToCoverageEnable = VK_FALSE;
	multisample_state.     alphaToOneEnable = VK_FALSE;

	VkPipelineVertexInputStateCreateInfo   vertex_input_state{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
	if(!x->vertex_input_bindings) {
		VulkanError("null vertex input bindings array. A pipeline is required to specify the vertex bindings used by the vertex shader.");
		return;
	}
	if(!x->vertex_input_attributes) {
		VulkanError("null vertex input attributes array. A pipeline is required to specify the attributes of the vertexes used in the vertex shader.");
		return;
	}
	auto vertex_input_bindings = array_from(x->vertex_input_bindings);
	auto vertex_input_attributes = array_from(x->vertex_input_attributes);
	auto vertex_input_bindings_out = array<VkVertexInputBindingDescription>::create_with_count(vertex_input_bindings.count(), temp_allocator);
	auto vertex_input_attributes_out = array<VkVertexInputAttributeDescription>::create_with_count(vertex_input_attributes.count(), temp_allocator);
	forI(vertex_input_bindings.count()) {
		vertex_input_bindings_out[i].binding = vertex_input_bindings[i].binding;
		vertex_input_bindings_out[i].stride = vertex_input_bindings[i].stride;
		vertex_input_bindings_out[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	}
	forI(vertex_input_attributes.count()) {
		vertex_input_attributes_out[i].binding = vertex_input_attributes[i].binding;
		vertex_input_attributes_out[i].location = vertex_input_attributes[i].location;
		vertex_input_attributes_out[i].offset = vertex_input_attributes[i].offset;
		vertex_input_attributes_out[i].format = graphics_format_to_vulkan(vertex_input_attributes[i].format);
	}
	vertex_input_state.pVertexBindingDescriptions = vertex_input_bindings_out.ptr;
	vertex_input_state.vertexBindingDescriptionCount = vertex_input_bindings_out.count();
	vertex_input_state.pVertexAttributeDescriptions = vertex_input_attributes_out.ptr;
	vertex_input_state.vertexAttributeDescriptionCount = vertex_input_attributes_out.count();

	VkPipelineDynamicStateCreateInfo       dynamic_state{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	auto dynamic_states = array<VkDynamicState>::create(5, temp_allocator);
	if(x->dynamic_scissor)        dynamic_states.push(VK_DYNAMIC_STATE_SCISSOR);
	if(x->dynamic_viewport)       dynamic_states.push(VK_DYNAMIC_STATE_VIEWPORT);
	if(x->dynamic_depth_bias)     dynamic_states.push(VK_DYNAMIC_STATE_DEPTH_BIAS);
	if(x->dynamic_line_width)     dynamic_states.push(VK_DYNAMIC_STATE_LINE_WIDTH);
	if(x->dynamic_blend_constant) dynamic_states.push(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
	dynamic_state.pDynamicStates = dynamic_states.ptr;
	dynamic_state.dynamicStateCount = dynamic_states.count();
	
	VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	info.stageCount          = shader_stages.count();
	info.pStages             = shader_stages.ptr;
	info.pVertexInputState   = &vertex_input_state;
	info.pInputAssemblyState = &input_assembly_state;
	info.pTessellationState  = 0;
	info.pViewportState      = &viewport_state;
	info.pRasterizationState = &rasterization_state;
	info.pMultisampleState   = &multisample_state;
	info.pDepthStencilState  = &depth_stencil_state;
	info.pColorBlendState    = &color_blend_state;
	info.pDynamicState       = &dynamic_state;
	info.layout              = get_handle(x->layout);
	info.renderPass          = get_handle(x->render_pass);
	
	auto result = vkCreateGraphicsPipelines(vk_device, vk_pipeline_cache, 1, &info, vk_allocator, &get_handle(x));
	VulkanAssertVk(result, "failed to create graphics pipeline.");

	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_PIPELINE, (u64)get_handle(x), (char*)x->debug_name.str);
}

void 
graphics_pipeline_destroy(GraphicsPipeline* x) {
	VulkanAssert(x, "passed a null GraphicsPipeline pointer.");
	VulkanInfo("destroying pipeline '", x->debug_name, "'.");
	vkDestroyPipeline(vk_device, get_handle(x), vk_allocator);
	ZeroMemory(x, sizeof(GraphicsPipeline));
	memory_pool_delete(g_graphics->pools.pipelines, x);
}

// Creates a new pipeline with the same settings as the one given.
GraphicsPipeline* 
graphics_pipeline_duplicate(GraphicsPipeline* x) {
	auto out = graphics_pipeline_allocate();
	CopyMemory(out, x, sizeof(GraphicsPipeline));
	if(out->shader_stages) {
		out->shader_stages = array_copy(out->shader_stages).ptr;
	}
	if(out->vertex_input_bindings) {
		out->vertex_input_bindings = array_copy(out->vertex_input_bindings).ptr;
	}
	if(out->vertex_input_attributes) {
		out->vertex_input_attributes = array_copy(out->vertex_input_attributes).ptr;
	}
	get_handle(out) = 0;
	return out;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @render_pass


GraphicsRenderPass* 
graphics_render_pass_allocate() {
	return memory_pool_push(g_graphics->pools.render_passes);
}

void 
graphics_render_pass_update(GraphicsRenderPass* x) {
	VulkanAssert(x, "passed null GraphicsRenderPass pointer.");
	VulkanInfo("updating render pass '", x->debug_name, "'.");

	vkDestroyRenderPass(vk_device, get_handle(x), vk_allocator);
	
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	
	VkAttachmentDescription attachments[2] = {};
	
	VkAttachmentReference color_attachment_reference{};
	VkAttachmentReference depth_attachment_reference{};
	VkAttachmentReference stencil_attachment_reference{};
	
	u32 index = 0;
	
	if(x->use_color_attachment) {
		attachments[index].        format =	graphics_format_to_vulkan(x->color_attachment.format);
		attachments[index].       samples = VK_SAMPLE_COUNT_1_BIT; // TODO(sushi) msaa
		attachments[index].        loadOp = graphics_load_op_to_vulkan(x->color_attachment.load_op);
		attachments[index].       storeOp = graphics_store_op_to_vulkan(x->color_attachment.store_op);
		attachments[index]. stencilLoadOp = graphics_load_op_to_vulkan(x->color_attachment.stencil_load_op);
		attachments[index].stencilStoreOp = graphics_store_op_to_vulkan(x->color_attachment.stencil_store_op);
		attachments[index]. initialLayout = graphics_image_layout_to_vulkan(x->color_attachment.initial_layout);
		attachments[index].   finalLayout = graphics_image_layout_to_vulkan(x->color_attachment.final_layout);
		
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_reference;
		color_attachment_reference.attachment = index;
		color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		index++;
	}
	
	if(x->use_depth_attachment) {
		attachments[index].        format = graphics_format_to_vulkan(x->depth_attachment.format);
		attachments[index].       samples = VK_SAMPLE_COUNT_1_BIT; // TODO(sushi) msaa
		attachments[index].        loadOp = graphics_load_op_to_vulkan(x->depth_attachment.load_op);
		attachments[index].       storeOp = graphics_store_op_to_vulkan(x->depth_attachment.store_op);
		attachments[index]. stencilLoadOp = graphics_load_op_to_vulkan(x->depth_attachment.stencil_load_op);
		attachments[index].stencilStoreOp = graphics_store_op_to_vulkan(x->depth_attachment.stencil_store_op);
		attachments[index]. initialLayout =	graphics_image_layout_to_vulkan(x->depth_attachment.initial_layout);
		attachments[index].   finalLayout = graphics_image_layout_to_vulkan(x->depth_attachment.final_layout);
		
		subpass.pDepthStencilAttachment = &depth_attachment_reference;
		depth_attachment_reference.attachment = index;
		depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		index++;
	}
	
	// always define a dependency between render passes
	// for now.
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
	
	VkRenderPassCreateInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
	info.attachmentCount = index;
	info.pAttachments = attachments;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = dependencies;
	
	auto result = vkCreateRenderPass(vk_device, &info, vk_allocator, &get_handle(x));
	VulkanAssertVk(result, "failed to create render pass.");
	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_RENDER_PASS, (u64)get_handle(x), (char*)x->debug_name.str);
}

void 
graphics_render_pass_destroy(GraphicsRenderPass* x) {
	VulkanAssert(x, "passed null GraphicsRenderPass pointer");
	VulkanInfo("destroying render pass '", x->debug_name, "'.");
	vkDestroyRenderPass(vk_device, get_handle(x), vk_allocator);
	ZeroMemory(x, sizeof(GraphicsRenderPass));
	memory_pool_delete(g_graphics->pools.render_passes, x);
}

GraphicsRenderPass* 
graphics_render_pass_of_window_presentation_frames(Window* window) {
	return ((WindowInfo*)window->render_info)->presentation_frames[0]->render_pass;
}

GraphicsFramebuffer* 
graphics_framebuffer_allocate() {
	return memory_pool_push(g_graphics->pools.framebuffers);
}

void 
graphics_framebuffer_update(GraphicsFramebuffer* x) {
	VulkanAssert(x, "passed null GraphicsFramebuffer pointer.");
	VulkanAssert(x->render_pass, "null render pass pointer on framebuffer. All framebuffers require a render pass.");
	VulkanInfo("updating framebuffer '", x->debug_name, "'.");

	auto attachments = array<VkImageView>::create(temp_allocator);
	
	if(x->render_pass->use_color_attachment)
		attachments.push(get_handle(x->color_image_view));
	
	if(x->render_pass->use_depth_attachment) 
		attachments.push(get_handle(x->depth_image_view));
	
	VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
	info.     renderPass = get_handle(x->render_pass);
	info.attachmentCount = attachments.count();
	info.   pAttachments = attachments.ptr;
	info.          width = x->width;
	info.         height = x->height;
	info.         layers = 1;
	auto result = vkCreateFramebuffer(vk_device, &info, vk_allocator, &get_handle(x));
	VulkanAssertVk(result, "failed to create framebuffer.");
	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_FRAMEBUFFER, (u64)get_handle(x), (char*)x->debug_name.str);
}

void 
graphics_framebuffer_destroy(GraphicsFramebuffer* x) {
	VulkanAssert(x, "passed null GraphicsFramebuffer pointer");
	VulkanInfo("destroying framebuffer '", x->debug_name, "'");
	vkDestroyFramebuffer(vk_device, get_handle(x), vk_allocator);
	ZeroMemory(x, sizeof(GraphicsFramebuffer));
	memory_pool_delete(g_graphics->pools.framebuffers, x);
}

GraphicsFramebuffer* 
graphics_current_present_frame_of_window(Window* window) {
	auto w = (WindowInfo*)window->render_info;
	return w->presentation_frames[w->frame_index];
}

GraphicsCommandBuffer* 
graphics_command_buffer_allocate() {
	return memory_pool_push(g_graphics->pools.command_buffers);
}

void 
graphics_command_buffer_update(GraphicsCommandBuffer* x) {
	VulkanAssert(x, "passed null GraphicsCommandBuffer pointer.");
	VulkanInfo("updating command buffer '", x->debug_name, "'.");

	vkFreeCommandBuffers(vk_device, vk_command_pool, 1, &get_handle(x));

	VkCommandBufferAllocateInfo info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	info.       commandPool = vk_command_pool;
	info.             level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = 1;
	auto result = vkAllocateCommandBuffers(vk_device, &info, &get_handle(x));
	VulkanAssertVk(result, "failed to allocate command buffer.");
	vk_debug_set_object_name(vk_device, VK_OBJECT_TYPE_COMMAND_BUFFER, (u64)get_handle(x), (char*)x->debug_name.str);
}

void 
graphics_command_buffer_destroy(GraphicsCommandBuffer* x) {
	VulkanAssert(x, "passed null GraphicsCommandBuffer pointer.");
	VulkanInfo("destroying command buffer '", x->debug_name, "'.");
	vkFreeCommandBuffers(vk_device, vk_command_pool, 1, &get_handle(x));
	ZeroMemory(x, sizeof(GraphicsCommandBuffer));
	memory_pool_delete(g_graphics->pools.command_buffers, x);
}

GraphicsCommandBuffer* 
graphics_command_buffer_of_window(Window* window) {
	return ((WindowInfo*)window->render_info)->command_buffer;
}

#undef primary_allocator
#undef temp_allocator
#undef VulkanFatal
#undef VulkanError
#undef VulkanWarning
#undef VulkanNotice
#undef VulkanInfo
#undef VulkanDebug
