/* deshi graphics module
	API wrapping a collection of backends which currently includes Vulkan and OpenGL. Planned backends
	include DirectX and possibly a software renderer later on.

	The API is largely based off of Vulkan. It revolves around the use of various types of objects
	used to describe how various things should be rendered. Those who are used to Vulkan should find
	much of the API familiar. 

	The memory of objects used in the api is managed internally. Nearly every object is managed
	through a handle given by `graphics_*_allocate()` functions. These handles may be destroyed
	using `graphics_*_destroy()` functions. This clears any information allocated for the object 
	in the active backend and makes the given handle invalid. 

	GraphicsBuffer is an exception to this pattern as it has many requirements that we will not 
	require the user to consider. They are created via a create info structure much like
	everything in the vulkan api and the object returned consists of members the user must not
	modify. More information about this and how to properly use GraphicsBuffers is provided
	above its declaration.

	An object is always completely zero'd when it is allocated. The properties of objects, aside
	from those inside of the `__internal` member, are to be set by the user. These properties are
	used in `graphics_*_update()` functions to create information for it in the backend. An object
	may be updated with different properties repeatedly, which will deallocate any information 
	created for it in the backend and then recreate it.

	Any object that allocates information in the backend will include a member named `debug_name`
	which will be used to mark the object with a name for debugging in programs like RenderDoc.

Index:
@context
@enums
@buffer
@image
@descriptor
@push_constant
@shader
@pipeline
@render_pass
@framebuffer
@command
@misc

	TODO(sushi) write-up stuff about each thing in the api
*/


#ifndef DESHI_GRAPHICS_H
#define DESHI_GRAPHICS_H

#include "kigu/array.h"

struct Window;
struct GraphicsImage;
struct GraphicsImageView;
struct GraphicsSampler;
struct GraphicsBuffer;
struct GraphicsShader;
struct GraphicsPipeline;
struct GraphicsPipelineLayout;
struct GraphicsRenderPass;
struct GraphicsDescriptorSetLayout;
struct GraphicsDescriptorSet;
struct GraphicsCommandBuffer;
struct GraphicsFramebuffer;


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @context


#define GRAPHICS_INTERNAL_BEGIN struct {
#define GRAPHICS_INTERNAL_END   } __internal;
#define GRAPHICS_INTERNAL(var) var->__internal

// Stored as a singleton within GraphicsGlobal
// and keeps track of various statistics.
typedef struct GraphicsStats {
	f64 update_time;

	u64 total_vertexes;
	u64 total_indexes;
	
	u64 drawn_indexes;

	u64 total_triangles;
	u64 drawn_triangles;
} GraphicsStats;

typedef struct GraphicsGlobal {
	struct { // pools
		GraphicsDescriptorSetLayout* descriptor_set_layouts;
		GraphicsDescriptorSet*       descriptor_sets;
		GraphicsShader*              shaders;
		GraphicsPipelineLayout*      pipeline_layouts;
		GraphicsPipeline*            pipelines;
		GraphicsBuffer*              buffers;
		GraphicsCommandBuffer*       command_buffers;
		GraphicsImage*               images;
		GraphicsImageView*           image_views;
		GraphicsSampler*             samplers;
		GraphicsRenderPass*          render_passes;
		GraphicsFramebuffer*         framebuffers;
	} pools;
	
	struct { // allocators
		// The primary allocator used by the graphics module. May be changed
		// to use a preferred allocator.
		Allocator* primary;
		// Cleared at the end of graphics_init() and graphics_update()
		Allocator* temp;
	} allocators;

	GraphicsStats stats;

	b32 initialized;
	u32 logging_level;
	b32 debugging;
	b32 break_on_error;
} GraphicsGlobal;

extern GraphicsGlobal* g_graphics;

extern Allocator __deshi_graphics_primary_allocator;
extern Allocator __deshi_graphics_temp_allocator;

// Initializes the graphics module for the given window. 
void graphics_init(Window* window);

// Updates the graphics module for the given window. If the module
// has not been initialized for the window an error is emitted and 
// the function returns.
void graphics_update(Window* window);

// Cleans up the graphics module for all windows.
void graphics_cleanup();


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @enums
// All enums used throughout the graphics api


// Specifies a shader stage or stages for use in 
// various properties of graphics objects.
typedef Flags GraphicsShaderStage; enum {
	GraphicsShaderStage_Vertex   = 1 << 0,
	GraphicsShaderStage_Geometry = 1 << 1,
	GraphicsShaderStage_Fragment = 1 << 3,
	GraphicsShaderStage_Compute  = 1 << 4,
};

// This determines how an image is laid out in memory on the gpu.
// Throughout the rendering process a GPU will shuffle the data of a image
// around in order to optimize usages of it. Exactly how it does this is 
// implementation specific but we can hint at (at least in Vulkan) the way
// that an image will be used by telling the GPU what layout we'd like the 
// image to be at at certain points.
typedef Type GraphicsImageLayout; enum {
	GraphicsImageLayout_Undefined,
	GraphicsImageLayout_General,
	GraphicsImageLayout_Color_Attachment_Optimal,
	GraphicsImageLayout_Depth_Stencil_Attachment_Optimal,
	GraphicsImageLayout_Depth_Stencil_Read_Only_Optimal,
	GraphicsImageLayout_Present,
	GraphicsImageLayout_Shader_Read_Only_Optimal,
};

// Various formats supported by the api
typedef Type GraphicsFormat; enum {
	GraphicsFormat_R32G32_Float,
	GraphicsFormat_R32G32B32_Float,
	GraphicsFormat_R8G8B8_SRGB,
	GraphicsFormat_R8G8B8_UNorm,
	GraphicsFormat_R8G8B8A8_SRGB,
	GraphicsFormat_R8G8B8A8_UNorm,
	GraphicsFormat_B8G8R8A8_UNorm,
	GraphicsFormat_Depth16_UNorm,
	GraphicsFormat_Depth32_Float,
	GraphicsFormat_Depth32_Float_Stencil8_UInt,
	GraphicsFormat_Depth24_UNorm_Stencil8_UInt,
	
	GraphicsFormat_COUNT,
	GraphicsFormat_COLOR_FIRST = GraphicsFormat_R32G32_Float,
	GraphicsFormat_COLOR_LAST = GraphicsFormat_B8G8R8A8_UNorm,
	GraphicsFormat_DEPTH_FIRST = GraphicsFormat_Depth16_UNorm,
	GraphicsFormat_DEPTH_LAST = GraphicsFormat_Depth24_UNorm_Stencil8_UInt,
};

// Determines which side of faces are culled.
typedef Type GraphicsCulling; enum {
 	GraphicsPipelineCulling_None,
	GraphicsPipelineCulling_Front,
	GraphicsPipelineCulling_Back,
	GraphicsPipelineCulling_Front_Back,
};

// How the backend should determine which side of a triangle is considered
// to be the front face. It is determined by the order in which the triangle's
// indexes are defined.
typedef Type GraphicsFrontFace; enum {
 	GraphicsFrontFace_CW,
	GraphicsFrontFace_CCW,
};

// How to draw polygons. Each vertex as a point, each edge as a line
// or fill each face.
typedef Type GraphicsPolygonMode; enum {
	GraphicsPolygonMode_Point,
	GraphicsPolygonMode_Line,
	GraphicsPolygonMode_Fill,
};

// Various comparison operators used by some objects.
typedef Type GraphicsCompareOp; enum {
	GraphicsCompareOp_Never,
	GraphicsCompareOp_Less,
	GraphicsCompareOp_Equal,
	GraphicsCompareOp_Less_Or_Equal,
	GraphicsCompareOp_Greater,
	GraphicsCompareOp_Not_Equal,
	GraphicsCompareOp_Greater_Or_Equal,
	GraphicsCompareOp_Always,
};

// Various ways to blend two values used by some objects.
typedef Type GraphicsBlendOp; enum {
	GraphicsBlendOp_Add,
	GraphicsBlendOp_Sub,
	GraphicsBlendOp_Reverse_Sub,
	GraphicsBlendOp_Min,
	GraphicsBlendOp_Max,
};

// By what factor to consider each component of a color in
// a fragment shader. TODO(sushi) better explanation and examples.
typedef Type GraphicsBlendFactor; enum {
	GraphicsBlendFactor_Zero,
	GraphicsBlendFactor_One,
	GraphicsBlendFactor_Source_Color,
	GraphicsBlendFactor_One_Minus_Source_Color,
	GraphicsBlendFactor_Destination_Color,
	GraphicsBlendFactor_One_Minus_Destination_Color,
	GraphicsBlendFactor_Source_Alpha,
	GraphicsBlendFactor_One_Minus_Source_Alpha,
	GraphicsBlendFactor_Destination_Alpha,
	GraphicsBlendFactor_One_Minus_Destination_Alpha,
	GraphicsBlendFactor_Constant_Color,
	GraphicsBlendFactor_One_Minus_Constant_Color,
	GraphicsBlendFactor_Constant_Alpha,
	GraphicsBlendFactor_One_Minus_Constant_Alpha,
};

// How many times to sample various things. 
// Used in MSAA (TODO) and multisampling images (TODO)
typedef Flags GraphicsSampleCount; enum {
	GraphicsSampleCount_1  = 1 << 0,
	GraphicsSampleCount_2  = 1 << 1,
	GraphicsSampleCount_4  = 1 << 2,
	GraphicsSampleCount_8  = 1 << 3,
	GraphicsSampleCount_16 = 1 << 4,
	GraphicsSampleCount_32 = 1 << 5,
	GraphicsSampleCount_64 = 1 << 6,
};

// Allows various properties to be set dynamically rather
// than be baked into a pipeline. This currently only applies
// to the Vulkan backend (but should be enforced in other 
// backends to ensure consistency). 
typedef Type GraphicsDynamicState; enum {
	GraphicsDynamicState_Viewport,
	GraphicsDynamicState_Scissor,
	GraphicsDynamicState_Line_Width,
	GraphicsDynamicState_Depth_Bias,
	GraphicsDynamicState_Blend_Constants,
	GraphicsDynamicState_Depth_Bounds,
};

// The type of resource a descriptor represents.
// AKA what sort of thing is a shader to expect.
typedef Type GraphicsDescriptorType; enum {
	GraphicsDescriptorType_Combined_Image_Sampler,
	GraphicsDescriptorType_Uniform_Buffer,
	// these two seem to be the only ones used for now
	// add more as they seem useful later
};

// The way in which a GraphicsBuffer is expected to be used.
typedef Flags GraphicsBufferUsage; enum {
	GraphicsBufferUsage_TransferSource      = 1 << 0,
	GraphicsBufferUsage_TransferDestination = 1 << 1,
	GraphicsBufferUsage_UniformTexelBuffer  = 1 << 2,
	GraphicsBufferUsage_StorageTexelBuffer  = 1 << 3,
	GraphicsBufferUsage_UniformBuffer       = 1 << 4,
	GraphicsBufferUsage_StorageBuffer       = 1 << 5,
	GraphicsBufferUsage_IndexBuffer         = 1 << 6,
	GraphicsBufferUsage_VertexBuffer        = 1 << 7,
	GraphicsBufferUsage_IndirectBuffer      = 1 << 8,
};

// NOTE(sushi) originally written by delle
// Choosing the correct RenderMemoryPropertyFlags is pretty important for performance and usability.
// The correct choice of course depends on use case, but there are tiers of speed and access patterns
// that can be understood to better make that choice. I will attempt to explain those here.
// 
// Speed:
// RenderMemoryPropertyFlag_DeviceLocal has the fastest device access because the memory is stored on
// the device. RenderMemoryPropertyFlag_HostVisible allows mapping memory between the host and device,
// but that means the device has to send the memory to the host before the host can make any changes,
// which it will have to send back to the device. A possible way to alleviate part of that transfer time
// is to use RenderMemoryPropertyFlag_HostCached, which will keep a copy of the memory on the host and
// only update the memory that has changed on the device. But, that also means you have two copies of
// the memory at no benefit to sending updates to the device, so it's often useful for host readback
// of data that's being written by the device.
//
// Access:
// If you never need to update the data, then it's simplest to just upload the memory to the device
// and disallow mapping it back to the host. RenderMemoryProperty_DeviceOnly and 
// RenderMemoryProperty_DeviceOnlyLazy are the choices for that, where lazy allocation means that the
// memory is only allocated as needed. Lazy allocation is mainly useful on tiled architectures
// (render tile-by-tile) with large render targets that don't need to save their result after rendering,
// like MSAA images or depth images, since the memory space for a finished tile can be reused. However,
// if you do need to update the data, then the choices differentiate based on how often it's
// necessary to update that data. RenderMemoryProperty_DeviceOnly might still be preferable if the
// memory has a high degree of random access, like with dynamic textures or large storage buffers.
// RenderMemoryProperty_DeviceMappable is ideal for frequent updates, like uniform buffers or dynamic
// vertex/index buffers, since you can map specific sections of the memory and it doesn't require going
// thru a staging buffer like RenderMemoryProperty_DeviceOnly. RenderMemoryProperty_HostStreamed is mainly
// used for staging buffers, but should probably be used in the above use case if
// RenderMemoryProperty_DeviceMappable is not available.
//
// Examples:
// RenderMemoryProperty_DeviceOnly:     static textures, large storage buffers, randomly accessed dynamic textures
// RenderMemoryProperty_DeviceOnlyLazy: MSAA image, depth image
// RenderMemoryProperty_DeviceMappable: uniform buffers, dynamic vertex/index buffers
// RenderMemoryProperty_HostStreamed:   staging buffers
//
// Notes:
// Choosing a RenderMemoryPropertyFlags can also depend on what GPU is being used, as the different
//   brands organize their device memory in different ways.
// Allocating too many resources with RenderMemoryPropertyFlag_DeviceLocal can result in VRAM
//   oversubscription (running out of memory).
// RenderMemoryPropertyFlag_LazilyAllocated and RenderMemoryPropertyFlag_HostCached are ignored on OpenGL as there
//   is no way to specify such behaviour in that backend (to my knowledge).
//
// References:
// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryPropertyFlagBits.html
// https://asawicki.info/news_1740_vulkan_memory_types_on_pc_and_how_to_use_them
// https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer
typedef Flags GraphicsMemoryPropertyFlags; enum {
	// The memory will be stored only on the device and isn't expected to be read or written to by the CPU.
	GraphicsMemoryPropertyFlag_DeviceLocal     = 1 << 0, 
	// The memory is stored on the device, but is expected to be mapped for host access.
	// Not compatible with LazilyAllocated.
	GraphicsMemoryPropertyFlag_HostVisible     = 1 << 1, 
	// The memory is stored on the host and can be read by the device over PCIe as needed. Changes do not 
	// need to be flushed.
	// Implies HostVisible.
	GraphicsMemoryPropertyFlag_HostCoherent    = 1 << 2,
	// Device memory that is also cached on the host.
	// Requires HostVisible.
	GraphicsMemoryPropertyFlag_HostCached      = 1 << 3, 
	// Device only access, commits memory as needed.
	// Incompatible with HostVisible.
	GraphicsMemoryPropertyFlag_LazilyAllocated = 1 << 4, 
	// Not mappable and requires the use of staging buffers.
	// Requires MemoryMapping_Never.
	GraphicsMemoryProperty_DeviceOnly     = GraphicsMemoryPropertyFlag_DeviceLocal,
	// Device only and committed as used.
	// Requires MemoryMapping_Never.
	GraphicsMemoryProperty_DeviceOnlyLazy = GraphicsMemoryPropertyFlag_DeviceLocal | GraphicsMemoryPropertyFlag_LazilyAllocated, 
	// Device dependent. Changes must be flushed.
	GraphicsMemoryProperty_DeviceMappable = GraphicsMemoryPropertyFlag_DeviceLocal | GraphicsMemoryPropertyFlag_HostVisible,
	// Host local memory. must be read over PCIe.
	GraphicsMemoryProperty_HostStreamed   = GraphicsMemoryPropertyFlag_HostVisible | GraphicsMemoryPropertyFlag_HostCoherent,    
};

typedef Type GraphicsMemoryMappingBehavoir; enum {
	// The buffer will never be mapped after the initial upload.
	// Incompatible with memory properties HostVisible, HostCoherent, and HostCached.
	GraphicsMemoryMapping_Never,
	// The buffer may be mapped and unmapped whenever the user wishes.
	// Requires one of the the memory properties: HostVisible, HostCoherent, or HostCached.
	GraphicsMemoryMapping_Occasional,
	// The buffer is mapped right after is is allocated and isn't unmapped until it 
	// is destroyed.
	// Requires one of the memory properties: HostVisible, HostCoherent, or HostCached.
	GraphicsMemoryMapping_Persistent,
};

// The dimensions of a given image.
typedef Type GraphicsImageType; enum {
	GraphicsImageType_1D,
	GraphicsImageType_2D,
	GraphicsImageType_3D,
};

// How an image is intended to be used.
typedef Type GraphicsImageUsage; enum {
	// This image is the source of a transfer operation. 
	// Eg. data will be copied from this image to some other location.
	GraphicsImageUsage_Transfer_Source          = 1 << 0,
	// This image is the destination of a transfer operation.
	// Eg. data will be copied from some location to this image.
	GraphicsImageUsage_Transfer_Destination     = 1 << 1,
	// This image will be sampled in a shader.
	GraphicsImageUsage_Sampled                  = 1 << 2, 
	// TODO(sushi)
	// This image may be used in a descriptor set slot of type StorageImage
	GraphicsImageUsage_Storage                  = 1 << 3,
	// This image will be used as the color attachment in a Framebuffer
	GraphicsImageUsage_Color_Attachment         = 1 << 4,
	// This image will be used as the depth attacment in a Framebuffer
	GraphicsImageUsage_Depth_Stencil_Attachment = 1 << 5,
};

// Which aspect of an image we are viewing.
typedef Type GraphicsImageViewAspectFlags; enum {
	GraphicsImageViewAspectFlags_Color   = 1 << 0,
	GraphicsImageViewAspectFlags_Depth   = 1 << 1,
	GraphicsImageViewAspectFlags_Stencil = 1 << 2,
};

// Different methods of filtering.
typedef Type GraphicsFilter; enum {
	GraphicsFilter_Nearest,
	GraphicsFilter_Linear,
};

// How a sampler behaves when sampling outside of the range [0, 1)
typedef Type GraphicsSamplerAddressMode; enum {
	GraphicsSamplerAddressMode_Repeat,
	GraphicsSamplerAddressMode_Mirrored_Repeat,
	GraphicsSamplerAddressMode_Clamp_To_Edge,
	GraphicsSamplerAddressMode_Clamp_To_Border,
};

// Determines if and how something is stored.
typedef Type GraphicsStoreOp; enum {
	GraphicsStoreOp_Store,
	GraphicsStoreOp_Dont_Care,
};

// Determines if and how something is loaded.
typedef Type GraphicsLoadOp; enum {
	GraphicsLoadOp_Load,
	GraphicsLoadOp_Clear,
	GraphicsLoadOp_Dont_Care,
};

typedef Type GraphicsCommandType; enum {
	GraphicsCommandType_Bind_Pipeline,
	GraphicsCommandType_Bind_Vertex_Buffer,
	GraphicsCommandType_Bind_Index_Buffer,
	GraphicsCommandType_Bind_Descriptor_Set,
	GraphicsCommandType_Push_Constant,
	GraphicsCommandType_Draw_Indexed,
	GraphicsCommandType_Begin_Render_Pass,
	GraphicsCommandType_End_Render_Pass,
	GraphicsCommandType_Set_Viewport,
	GraphicsCommandType_Set_Scissor,
	GraphicsCommandType_Set_Depth_Bias,
};


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @buffer
// Represents memory allocated on the device.
//
// The api of GraphisBuffer is different than the rest of graphics due to its unique use cases 
// we support. 

typedef struct GraphicsBuffer {
	str8 debug_name;
	GRAPHICS_INTERNAL_BEGIN
		u64 size;
		GraphicsBufferUsage usage;
		GraphicsMemoryPropertyFlags memory_properties;
		GraphicsMemoryMappingBehavoir mapping_behavior;

		struct {
			void* data;
			u64 offset;
			u64 size;
		} mapped;

		void* buffer_handle;
		void* memory_handle;
	GRAPHICS_INTERNAL_END
} GraphicsBuffer;

// Creates a GraphicsBuffer *and* actually creates the buffer on the device. 
//   'requested_size' is the minimum size the user needs the buffer to be, but the actual size of the buffer
//   on the device may be different due to alignment requirements. To retrieve the actual size of the buffer
//   use graphics_buffer_device_size()
//
//   'data' is an optional pointer to data to be immediately coppied to the created buffer.
GraphicsBuffer* graphics_buffer_create(void* data, u64 requested_size, GraphicsBufferUsage usage, GraphicsMemoryPropertyFlags properties, GraphicsMemoryMappingBehavoir mapping_behavior);

// Destroys the given GraphicsBuffer, deallocating its memory on the device and invaliding the given handle.
void graphics_buffer_destroy(GraphicsBuffer* x);

// Reallocates the memory on the device to a buffer with the given size.
// Note that much like the create function, 'new_size' is the minimum size the user needs the buffer to be
// but due to alignment requirements the actual device size of the buffer may be larger. 
void graphics_buffer_reallocate(GraphicsBuffer* x, u64 new_size);

// Maps 'size' bytes at 'offset' from an unmapped buffer.
// The buffer must have been created with these properties:
// 		memory_properties = DeviceLocal | HostVisible
// 		mapping_behavior  = Occasionally
// If the given buffer is set to persistent mapping a warning is given
// and the function returns the mapped data pointer. If the buffer was already
// mapped then a warning is given and the function returns the mapped data
// pointer.
// If the mapping is successful then a pointer to the mapped data is returned.
// Otherwise 0 is returned.
void* graphics_buffer_map(GraphicsBuffer* x, u64 size, u64 offset);

// Unmaps a buffer that was previously mapped an optionally flushes it to the device.
// The buffer must have been previously mapped.
// If the given buffer is set to persistent mapping then a warning is given
// and the function returns.
void graphics_buffer_unmap(GraphicsBuffer* x, b32 flush);

// Flushs the data on the host to the device.
void graphics_buffer_flush(GraphicsBuffer* x);

// Retrieve the size of the buffer as it is on the device.
u64 graphics_buffer_device_size(GraphicsBuffer* x);

// Retrieves a pointer to the mapped data of the given buffer.
// The buffer must be mapped, otherwise 0 is returned.
void* graphics_buffer_mapped_data(GraphicsBuffer* x);

// Retrieves the amount of data currently mapped for the
// given buffer. If the buffer is not mapped then 0 is returned.
u64 graphics_buffer_mapped_size(GraphicsBuffer* x);

// Retrieves the mapped offset of the given buffer.
// If the buffer is not mapped then -1 is returned.
u64 graphics_buffer_mapped_offset(GraphicsBuffer* x);


#if COMPILER_FEATURE_CPP
namespace graphics {

struct Buffer : public GraphicsBuffer {
	static Buffer* 
	create(void* data, u64 requested_size, GraphicsBufferUsage usage, GraphicsMemoryPropertyFlags properties, GraphicsMemoryMappingBehavoir mapping_behavior) {
		return (Buffer*)graphics_buffer_create(data, requested_size, usage, properties, mapping_behavior);
	}

	void  reallocate(u64 new_size) { graphics_buffer_reallocate(this, new_size); }
	void  destroy() { graphics_buffer_destroy(this); }
	void* map(u64 size, u64 offset) { return graphics_buffer_map(this, size, offset); }
	void  unmap(b32 flush = false) { graphics_buffer_unmap(this, flush); }
	void  flush() { graphics_buffer_flush(this); }
	u64   device_size() { return graphics_buffer_device_size(this); }
	void* mapped_data() { return graphics_buffer_mapped_data(this); }
	u64   mapped_size() { return graphics_buffer_mapped_size(this); }
	u64   mapped_offset() { return graphics_buffer_mapped_offset(this); }
};

} // namespace graphics
#endif


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @image


typedef struct GraphicsImage {
	str8 debug_name;

	GraphicsFormat format;
	GraphicsImageUsage usage;
	GraphicsSampleCount samples;

	// the width, height, and number of channels in the image.
	vec3i extent;
	
	// Whether or not to perform linear tiling as opposed to optimial tiling.
	b32 linear_tiling;

	// How the memory of the image is intended to be used.
	GraphicsMemoryPropertyFlags memory_properties;

	u64 mip_levels;
		
	GRAPHICS_INTERNAL_BEGIN
		void* handle;
		void* memory_handle;
	GRAPHICS_INTERNAL_END
} GraphicsImage; 

GraphicsImage* graphics_image_allocate();
void graphics_image_update(GraphicsImage* x);
void graphics_image_destroy(GraphicsImage* x);

// Uploads the given pixels to the given GraphicsImage
// within the region specified by 'offset' and 'extent'.
// This function does nothing and emits a warning if 
//   either axis in 'extent' is 0,
//   the region is completely outside the bounds of the image.
void graphics_image_write(GraphicsImage* x, u8* pixels, vec2i offset, vec2i extent);


typedef struct GraphicsImageView {
	str8 debug_name;

	GraphicsFormat format;
	GraphicsImageViewAspectFlags aspect_flags;

	GraphicsImage* image;

	GRAPHICS_INTERNAL_BEGIN
		void* handle;
	GRAPHICS_INTERNAL_END
} GraphicsImageView;

GraphicsImageView* graphics_image_view_allocate();
void graphics_image_view_update(GraphicsImageView* x);
void graphics_image_view_destroy(GraphicsImageView* x);


typedef struct GraphicsSampler {
	str8 debug_name;

	// Behavoir when the image s magnified or minified.
	GraphicsFilter mag_filter;
	GraphicsFilter min_filter;

	// Behavoir when sampling an image outside of the range 
	// [0, 1) over each axis.
	GraphicsSamplerAddressMode address_mode_u;
	GraphicsSamplerAddressMode address_mode_v;
	GraphicsSamplerAddressMode address_mode_w;
	// The color to clamp to when Clamp_To_Edge is used.
	color border_color;
	
	// The filter applied when doing mipmap lookups.
	GraphicsFilter mipmap_mode;

	// Whether or not to perform anistropic filtering.
	b32 anistropy_enable;
	// The max amount of anistropy to use when enabled.
	f32 anistropy_max;

	GRAPHICS_INTERNAL_BEGIN
		void* handle;
	GRAPHICS_INTERNAL_END
} GraphicsSampler;

GraphicsSampler* graphics_sampler_allocate();
void graphics_sampler_update(GraphicsSampler* x);
void graphics_sampler_destroy(GraphicsSampler* x);


#if COMPILER_FEATURE_CPP
namespace graphics {

struct Image : public GraphicsImage {
	static Image* allocate() { return (Image*)graphics_image_allocate(); }
	
	void update() { graphics_image_update(this); }
	void destroy() { graphics_image_destroy(this); }
	
	void write(u8* pixels, vec2i offset, vec2i extent) { graphics_image_write(this, pixels, offset, extent); }
};

struct ImageView : public GraphicsImageView {
	static ImageView* allocate() { return (ImageView*)graphics_image_view_allocate(); }

	void update() { graphics_image_view_update(this); }
	void destroy() { graphics_image_view_destroy(this); }
};

struct Sampler : public GraphicsSampler {
	static Sampler* allocate() { return (Sampler*)graphics_sampler_allocate(); }

	void update() { graphics_sampler_update(this); }
	void destroy() { graphics_sampler_destroy(this); }
};

} // namespace graphics
#endif


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @descriptor


// Describes the 'n'th binding of a descriptor set
typedef struct GraphicsDescriptorSetLayoutBinding {
	GraphicsDescriptorType type;
	GraphicsShaderStage shader_stages;
	u32 n;
} GraphicsDescriptorSetLayoutBinding;

// A descriptor set layout simply describes the shape of a descriptor set.
// The bindings each describes the type of data being referenced as well as 
// what shader stages it is to be used in and which binding that resource should
// be attached to.
typedef struct GraphicsDescriptorSetLayout {
	str8 debug_name;

	GraphicsDescriptorSetLayoutBinding* bindings;

	GRAPHICS_INTERNAL_BEGIN
		void* handle;
	GRAPHICS_INTERNAL_END
} GraphicsDescriptorSetLayout;

GraphicsDescriptorSetLayout* graphics_descriptor_set_layout_allocate();
void graphics_descriptor_set_layout_update(GraphicsDescriptorSetLayout* x);
void graphics_descriptor_set_layout_destroy(GraphicsDescriptorSetLayout* x);

typedef struct GraphicsDescriptor {
	GraphicsDescriptorType type;
	GraphicsShaderStage shader_stages;

	union {

	struct { // image
		GraphicsImageView*  view;
		GraphicsSampler*    sampler;
		GraphicsImageLayout layout;
	} image;
	
	struct { // ubo
		GraphicsBuffer* buffer;
		u64 offset;
		u64 range;
	} ubo;

	};
} GraphicsDescriptor;

typedef struct GraphicsDescriptorSet {
	str8 debug_name;

	GraphicsDescriptor* descriptors;
	GraphicsDescriptorSetLayout** layouts;

	GRAPHICS_INTERNAL_BEGIN
		void* handle;
	GRAPHICS_INTERNAL_END
} GraphicsDescriptorSet;

GraphicsDescriptorSet* graphics_descriptor_set_allocate();
void graphics_descriptor_set_update(GraphicsDescriptorSet* x);
void graphics_descriptor_set_destroy(GraphicsDescriptorSet* x);
void graphics_descriptor_set_write(GraphicsDescriptorSet* x);

#if COMPILER_FEATURE_CPP 

namespace graphics {

struct Descriptor : public GraphicsDescriptor {};

struct DescriptorSetLayoutBinding : public GraphicsDescriptorSetLayoutBinding {};

struct DescriptorSetLayout : public GraphicsDescriptorSetLayout {
	static DescriptorSetLayout*
	allocate() { return (DescriptorSetLayout*)graphics_descriptor_set_layout_allocate(); }

	void update() { return graphics_descriptor_set_layout_update(this); }
	void destroy() { return graphics_descriptor_set_layout_destroy(this); }
};

struct DescriptorSet : public GraphicsDescriptorSet {
	static DescriptorSet*
	allocate() { return (DescriptorSet*)graphics_descriptor_set_allocate(); }

	void update() { graphics_descriptor_set_update(this); }
	void destroy() { graphics_descriptor_set_destroy(this); }

	void write() { graphics_descriptor_set_write(this); }
};

} // namespace graphics

#endif


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @push_constant


typedef struct GraphicsPushConstant {
	GraphicsShaderStage shader_stages;
	u32 size;
	u32 offset;
	str8 shader_block_name;
	
	GRAPHICS_INTERNAL_BEGIN
		u32 shader_block_index;
		u32 shader_block_binding;
		u32 shader_buffer_handle;
	GRAPHICS_INTERNAL_END
} GraphicsPushConstant;


#if COMPILER_FEATURE_CPP
namespace graphics {

struct PushConstant : public GraphicsPushConstant {};

} // namespace graphics
#endif


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @shader


// A compiled shader provided by the user. If you don't already have spv you 
// may use `graphics_shader_compile_to_spv()` to perform compilation.
typedef struct GraphicsShader {
	str8 debug_name;

	GraphicsShaderStage shader_stage;
	str8 source;

	GRAPHICS_INTERNAL_BEGIN
		void* handle;
	GRAPHICS_INTERNAL_END
} GraphicsShader;

GraphicsShader* graphics_shader_allocate();
void graphics_shader_update(GraphicsShader* x);
void graphics_shader_destroy(GraphicsShader* x);


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @pipeline


// Describes the resources used by a pipeline. This is really just a pair
// of an array of descriptor layout and an array of push constants.
typedef struct GraphicsPipelineLayout {
	str8 debug_name;

	GraphicsDescriptorSetLayout** descriptor_layouts;
	GraphicsPushConstant* push_constants;

	GRAPHICS_INTERNAL_BEGIN
		void* handle;
	GRAPHICS_INTERNAL_END
} GraphicPipelineLayout;

GraphicsPipelineLayout* graphics_pipeline_layout_allocate();
void graphics_pipeline_layout_update(GraphicsPipelineLayout* x);
void graphics_pipeline_layout_destroy(GraphicsPipelineLayout* x);


// Determines the size of the vertexes to be expected in vertex shaders
// of a pipeline. The stride is the size of the vertex and the binding is 
// an index into the array of bindings given to the shader.
typedef struct GraphicsVertexInputBindingDescription {
	u32 binding; 
	u32 stride;
} GraphicsVertexInputBindingDescription;

// Describes the structure of the vertexes being passed to the vertex shader.
typedef struct GraphicsVertexInputAttributeDescription {
	u32 location;
	u32 binding;
	GraphicsFormat format;
	u32 offset;
} GraphicsVertexInputAttributeDescription;


// Describes a sequence of operations taken on a set of vertexes in order
// to properly render them to the screen.
typedef struct GraphicsPipeline {
	str8 debug_name;
	

	GraphicsShader* vertex_shader;
	GraphicsShader* geometry_shader;
	GraphicsShader* fragment_shader;


    // Whether or not the viewport is expected to be set dynamically. If this is true
	// the following 2 properties are ignored and the viewport must be set during command
	// recording via `graphics_cmd_set_viewport()`.
	b32 dynamic_viewport;
	// The offset and extent of the region of the frame the resulting color attachement
	// will be drawn to. This can be thought of as displacing and scaling the final image.
	vec2 viewport_offset;
	vec2 viewport_extent;

	// Whether or not the scissor is expected to be set dynamically. If this is 
	// true the following two values are ignored and the scissor must be set during 
	// command recording via `graphics_cmd_set_scissor()`.
	b32 dynamic_scissor;
	// The region of the screen in which to actually display the final image.
	// This can be thought of as cutting the final image.
	vec2 scissor_offset;
	vec2 scissor_extent;


	// How the front facing direction of a triangle is determined
	// based on the order in which its indexes are given.
	GraphicsFrontFace front_face;
	// How faces should be culled before fragments are produced.
	GraphicsCulling culling;
	// How polygons are drawn, as points, lines, or filled faces.
	GraphicsPolygonMode polygon_mode;
	// Whether the line width is expected to be set dynamically. If this is true 
	// then the following property is ignored and line width must be set 
	// via `graphics_cmd_set_line_width()`
	b32 dynamic_line_width;
	// How thick lines should be drawn when in line polygon mode.
	f32 line_width;


	// Whether or not to perform depth testing.
	b32 depth_test;
	// Whether or not to allow writing to the depth attachment when 'depth_test' is true.
	b32 depth_writes;
	// How depths should be compared when performing depth testing.
	GraphicsCompareOp depth_compare_op;
	// Whether or not to apply a bias to depth values.
	b32 depth_bias;
	// Whether or not the depth bias is expected to be set dynamically.
	// If this is true then the following 3 properties will be ignored
	// and the depth bias must be set via `graphics_cmd_set_depth_bias()`
	// during command recording.
	b32 dynamic_depth_bias;
	// Constant value applied to depth values.
	f32 depth_bias_constant;
	// Factor applied to a fragment's slope.
	f32 depth_bias_slope;
	// Maximum (or minimum) value applied to the depth value of a fragment.
	f32 depth_bias_clamp;

	// Whether or not to perform color blending.
	// Eg. how are colors combined when a fragment
	// is written to multiple times.
	b32 color_blend;
	// How the actual color is blended.
	GraphicsBlendOp     color_blend_op;
	GraphicsBlendFactor color_src_blend_factor;
	GraphicsBlendFactor color_dst_blend_factor;
	// How the alphas are blended.
	GraphicsBlendOp     alpha_blend_op;
	GraphicsBlendFactor alpha_src_blend_factor;
	GraphicsBlendFactor alpha_dst_blend_factor;	
	// Whether or not the blend constant is dynamic. If this is true, then 
	// the following property is ignored and the blend constant must be set
	// via `graphics_cmd_set_blend_constant()` during command recording.
	b32 dynamic_blend_constant;
	// The color used when constant blend factors are specified.
	color blend_constant;
	
	// An array of bindings to vertex arrays expected to be used in the vertex shader
	// of this pipeline.
	GraphicsVertexInputBindingDescription* vertex_input_bindings;
	// An array of attributes describing the structure of the vertexes given to
	// the vertex shader.
	GraphicsVertexInputAttributeDescription* vertex_input_attributes;
	
	// The layout of resources used throughout this pipeline.
	GraphicsPipelineLayout* layout;

	// A render pass that this pipeline is expected to be used with. This does not have 
	// to be the render pass that is used, any render pass compatible with it may be used
	// in its place.
	GraphicsRenderPass* render_pass;

	GRAPHICS_INTERNAL_BEGIN
		void* handle;
	GRAPHICS_INTERNAL_END
} GraphicsPipeline;

GraphicsPipeline* graphics_pipeline_allocate();
void graphics_pipeline_update(GraphicsPipeline* x);
void graphics_pipeline_destroy(GraphicsPipeline* x);

// Creates a new pipeline with the same settings as the one given.
// This will automatically copy any non-null array on the given pipeline.
GraphicsPipeline* graphics_pipeline_duplicate(GraphicsPipeline* x);

#if COMPILER_FEATURE_CPP
namespace graphics {
struct PipelineLayout : public GraphicsPipelineLayout {
	static PipelineLayout*
	allocate() { return (PipelineLayout*)graphics_pipeline_layout_allocate(); }

	void update() { return graphics_pipeline_layout_update(this); }
	void destroy() { return graphics_pipeline_layout_destroy(this); }
};

struct Pipeline : public GraphicsPipeline {
	static Pipeline*
	allocate() { return (Pipeline*)graphics_pipeline_allocate(); }

	void update() { graphics_pipeline_update(this); }
	void destroy() { graphics_pipeline_destroy(this); }

	Pipeline* duplicate(Pipeline* x) { return (Pipeline*)graphics_pipeline_duplicate(x); }
};
} // namespace graphics
#endif


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @render_pass


typedef struct GraphicsRenderPassAttachment {
	// What format the image associated with this attachment is in.
	GraphicsFormat      format;
	// How many times the image should be sampled.
	GraphicsSampleCount sample_count;
	// The behavoir when loading the image.
	GraphicsLoadOp      load_op;
	// The behavoir when storing the image.
	GraphicsStoreOp     store_op;
	// The behavoir when loading the stencil.
	GraphicsLoadOp      stencil_load_op;
	// The behavoir when storing the stencil.
	GraphicsStoreOp     stencil_store_op;
	// The image layout the attachment is expected to be in 
	// when the render pass starts.
	GraphicsImageLayout initial_layout;
	// The image layout the attachment is expected to be in when
	// the render pass is finished.
	GraphicsImageLayout final_layout;
} GraphicsRenderPassAttachment;

// A description of the attachments used by a framebuffer.
typedef struct GraphicsRenderPass {
	str8 debug_name;
	
	b32 use_color_attachment;
	GraphicsRenderPassAttachment color_attachment;
	b32 use_depth_attachment;
	GraphicsRenderPassAttachment depth_attachment;

	vec4 color_clear_values;
	struct {
		f32 depth;
		u32 stencil;
	} depth_clear_values;

	GRAPHICS_INTERNAL_BEGIN
		void* handle;
	GRAPHICS_INTERNAL_END
} GraphicsRenderPass;

GraphicsRenderPass* graphics_render_pass_allocate();
void graphics_render_pass_update(GraphicsRenderPass* x);
void graphics_render_pass_destroy(GraphicsRenderPass* x);

// Returns the render pass used by the presentation frames of the given window.
GraphicsRenderPass* graphics_render_pass_of_window_presentation_frames(Window* window);


#if COMPILER_FEATURE_CPP
namespace graphics {
struct RenderPass : public GraphicsRenderPass {
	static RenderPass* allocate() { return (RenderPass*)graphics_render_pass_allocate(); }

	void update() { graphics_render_pass_update(this); }
	void destroy() { graphics_render_pass_destroy(this); }

	static RenderPass*
	of_window_presentation_frames(Window* window) { return (RenderPass*)graphics_render_pass_of_window_presentation_frames(window); }
};
} // namespace graphics
#endif


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @framebuffer


typedef struct GraphicsFramebuffer {
	str8 debug_name;

	// Render pass describing how this frame behaves.
	GraphicsRenderPass* render_pass;

	// The size of the frame.
	u32 width, height;

	// Views of the images used for the attachments of this framebuffer.
	GraphicsImageView* color_image_view;
	GraphicsImageView* depth_image_view;

	GRAPHICS_INTERNAL_BEGIN
		void* handle;
	GRAPHICS_INTERNAL_END
} GraphicsFramebuffer;

GraphicsFramebuffer* graphics_framebuffer_allocate();
void graphics_framebuffer_update(GraphicsFramebuffer* x);
void graphics_framebuffer_destroy(GraphicsFramebuffer* x);

// Retrieve the frame that will be presented on the next call to render_update(window)
GraphicsFramebuffer* graphics_current_present_frame_of_window(Window* window);


#if COMPILER_FEATURE_CPP
namespace graphics {

struct Framebuffer : public GraphicsFramebuffer {
	static Framebuffer* allocate() { return (Framebuffer*)graphics_framebuffer_allocate(); }

	void update() { graphics_framebuffer_update(this); }
	void destroy() { graphics_framebuffer_destroy(this); }
};

} // namespace graphics
#endif


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @command


typedef struct GraphicsCommand {
	GraphicsCommandType type;
	union {
		
	struct { // bind_pipeline
		GraphicsPipeline* handle;
	} bind_pipeline;
	
	struct { // bind_vertex_buffer, bind_index_buffer
		GraphicsBuffer* handle;
	} bind_vertex_buffer, bind_index_buffer;
	
	struct { // bind_descriptor_set
		u32 set_index;
		GraphicsDescriptorSet* handle;
	} bind_descriptor_set;
	
	struct { // push_constant
		GraphicsShaderStage shader_stages;
		void* data;
		u32 offset;
		u32 size;
	} push_constant;

	struct { // draw_indexed
		u64 index_count;
		u64 index_offset;
		u64 vertex_offset;
	} draw_indexed;
	
	struct {
		GraphicsRenderPass* pass;
		GraphicsFramebuffer* frame; 
	} begin_render_pass;
	
	struct { // set_viewport
		vec2 offset;
		vec2 extent;
	} set_viewport, set_scissor;
	
	struct {
		f32 constant;
		f32 clamp;
		f32 slope;
	} set_depth_bias;
		
	}; // union
} GraphicsCommand;

// Begins the given render pass for the given frame. 
// A render pass MUST be active to record commands.
// Render passes may not be nested.
void graphics_cmd_begin_render_pass(Window* window, GraphicsRenderPass* render_pass, GraphicsFramebuffer* frame);
// Ends the currently active render pass.
void graphics_cmd_end_render_pass(Window* window);
// Binds the given pipeline. The pipeline must be compatible with the currently active render pass.
void graphics_cmd_bind_pipeline(Window* window, GraphicsPipeline* pipeline);
// Binds the given vertex buffer. 
void graphics_cmd_bind_vertex_buffer(Window* window, GraphicsBuffer* buffer);
// Binds the given index buffer.
void graphics_cmd_bind_index_buffer(Window* window, GraphicsBuffer* buffer);
// Binds the given descriptor set to 'set_index'. The descriptor set must be compatible with the 
// relevant descriptor layout of the currently bound pipeline.
void graphics_cmd_bind_descriptor_set(Window* window, u32 set_index, GraphicsDescriptorSet* descriptor_set);
// Register the memory at 'data' to be pushed according to 'info'. The data provided must be kept valid
// until the end of the following graphics_update call.
void graphics_cmd_push_constant(Window* window, GraphicsShaderStage shader_stages, void* data, u32 offset, u32 size);
// Draw vertexes based on the indexes in the currenly bound vertex and index buffers.
void graphics_cmd_draw_indexed(Window* window, u32 index_count, u32 index_offset, u32 vertex_offset);
// Set the viewport of the current render pass. The currently bound pipeline must have been created 
// with RenderDynamicStates_Viewport in its dynamic states array.
void graphics_cmd_set_viewport(Window* window, vec2 offset, vec2 extent);
// Set the scissor of the current render pass. The currently bound pipeline must have been created 
// with RenderDynamicStates_Scissor in its dynamic states array.
void graphics_cmd_set_scissor(Window* window, vec2 offset, vec2 extent);
// Set the depth bias of the current render pass. The currently bound pipeline must have been created
// with RenderDynamicStates_Depth_Bias in its dynamic states array.
void graphics_cmd_set_depth_bias(Window* window, f32 constant, f32 clamp, f32 slope);


#if COMPILER_FEATURE_CPP
namespace graphics::cmd {
FORCE_INLINE void begin_render_pass(Window* window, GraphicsRenderPass* render_pass, GraphicsFramebuffer* frame){ graphics_cmd_begin_render_pass(window, render_pass, frame); }
FORCE_INLINE void end_render_pass(Window* window){ graphics_cmd_end_render_pass(window); }
FORCE_INLINE void bind_pipeline(Window* window, GraphicsPipeline* pipeline){ graphics_cmd_bind_pipeline(window, pipeline); }
FORCE_INLINE void bind_vertex_buffer(Window* window, GraphicsBuffer* buffer){ graphics_cmd_bind_vertex_buffer(window, buffer); }
FORCE_INLINE void bind_index_buffer(Window* window, GraphicsBuffer* buffer){ graphics_cmd_bind_index_buffer(window, buffer); }
FORCE_INLINE void bind_descriptor_set(Window* window, u32 set_index, GraphicsDescriptorSet* descriptor_set){ graphics_cmd_bind_descriptor_set(window, set_index, descriptor_set); }
FORCE_INLINE void push_constant(Window* window, GraphicsShaderStage shader_stages, void* data, u32 offset, u32 size){ graphics_cmd_push_constant(window, shader_stages, data, offset, size); }
FORCE_INLINE void draw_indexed(Window* window, u32 index_count, u32 index_offset, u32 vertex_offset){ graphics_cmd_draw_indexed(window, index_count, index_offset, vertex_offset); }
FORCE_INLINE void set_viewport(Window* window, vec2 offset, vec2 extent){ graphics_cmd_set_viewport(window, offset, extent); } 
FORCE_INLINE void set_scissor(Window* window, vec2 offset, vec2 extent){ graphics_cmd_set_scissor(window, offset, extent); }
FORCE_INLINE void set_depth_bias(Window* window, f32 constant, f32 clamp, f32 slope){ graphics_cmd_set_depth_bias(window, constant, clamp, slope); }
} // namespace render::cmd
#endif


// NOTE(sushi) this is currently only used internally
//             We store one on each window and commands are added 
//             by using the render_cmd_* functions.
//             In the future I would like to play around with
//             supporting an api for this so that we can do things
//             like build commands in parallel or have command buffers
//             that aren't meant to be cleared every frame.
typedef struct GraphicsCommandBuffer {
	str8 debug_name;
	GraphicsCommand* commands;

	GRAPHICS_INTERNAL_BEGIN
		void* handle;
	GRAPHICS_INTERNAL_END
} GraphicsCommandBuffer;

GraphicsCommandBuffer* graphics_command_buffer_allocate();
void graphics_command_buffer_update(GraphicsCommandBuffer* x);
void graphics_command_buffer_destroy(GraphicsCommandBuffer* x);

// Retrieve the command buffer belonging to the given window.
GraphicsCommandBuffer* graphics_command_buffer_of_window(Window* window);


#if COMPILER_FEATURE_CPP
namespace graphics {

struct CommandBuffer : public GraphicsCommandBuffer {
	static CommandBuffer* allocate() { return (CommandBuffer*)graphics_command_buffer_allocate(); }

	void update() { graphics_command_buffer_update(this); }
	void destroy() { graphics_command_buffer_destroy(this); }
}; 

} // namespace graphics


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @misc


GraphicsFormat graphics_format_of_presentation_frames(Window* window);

#endif

#endif // DESHI_GRAPHICS_H
