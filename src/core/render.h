/* deshi Render Module
Notes:
-All 2D and 3D drawing functions are immediate-mode, except render_debug_line().
-Drawing functions ending in 2 are for 2D, 3 are for 3D, and nothing are implicitly 3D.
-The 2D coordinate system goes from top left as the origin to bottom right.
-The 3D coordinate system works such that +x is right, +y is up, and +z is forward.
-Triangle winding (font-facing) is clockwise in 3D and counter-clockwise in 2D.
-Voxels are bottom-left-back (-x,-y,-z) centered, meaning that the voxel at (0,0,0) has a vertex at (1,1,1).
-Voxels are planar boxes, meaning that faces don't share vertices so that texture UVs are properly rendered.
-Usually, "device" means GPU and "host" means CPU/RAM.

Index:
@render_types
@render_status
@render_surface
@render_buffer
@render_loading
@render_draw_3d
@render_draw_2d
@render_voxel
@render_light
@render_camera
@render_shaders
@render_make_2d
@render_other
@render_shared_status
@render_shared_buffer
@render_shared_surface
@render_shared_draw_3d
@render_shared_draw_2d
@render_shared_make_2d
@render_shared_voxel
@render_shared_other
*/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifndef DESHI_RENDER_H
#define DESHI_RENDER_H
#include "kigu/arrayT.h"
#include "kigu/color.h"
#include "kigu/common.h"
#include "math/math.h"

struct Mesh;
struct Texture;
struct Material;
struct Model;
struct Font;
struct Window;
struct RenderDescriptorSetLayout;
struct RenderDescriptorSet;
struct RenderPipelineLayout;
struct RenderPipeline;
struct RenderBuffer;
struct RenderCommandBuffer;
struct RenderImage;
struct RenderImageView;
struct RenderSampler;
struct RenderPass;
struct RenderFramebuffer;
struct MeshVertex;
StartLinkageC();

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_types
#define RENDER_MAX_ALLOCATIONS 4096 //NOTE(delle): Vulkan drivers are only required to support up to 4096 individual allocations
#define MAX_SURFACES 2

extern u32 renderActiveSurface;

typedef Type VSyncType; enum{
	VSyncType_Immediate,   //no image queue (necessary), display as soon as possible
	VSyncType_Mailbox,     //image queue that replaces current pending image with new one, but waits to display on refresh
	VSyncType_Fifo,        //image queue that only gets removed from on refresh, waits to display on refresh (regular Vsync)
	VSyncType_FifoRelaxed, //same as Fifo, but if the image generates slower than refresh, don't wait to display on next refresh
};

typedef Flags RenderStage; enum{ 
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
};

extern RenderStage renderStage;

typedef Type ShaderStage; enum{
	ShaderStage_NONE,
	ShaderStage_Vertex,
	ShaderStage_TessCtrl,
	ShaderStage_TessEval,
	ShaderStage_Geometry,
	ShaderStage_Fragment,
	ShaderStage_Compute,
	ShaderStage_COUNT,
};

typedef struct RenderStats{
	f64 renderTimeMS;
	
	u64 totalVertices;
	u64 totalIndices;
	u64 drawnIndices;
	u64 totalTriangles;
	u64 drawnTriangles;
	
	u64 totalVoxels;
	u64 totalVoxelChunks;
}RenderStats;

typedef struct RenderSettings{
	//// requires restart ////
	b32 debugging           = true;
	b32 printf              = false;
	b32 recompileAllShaders = false;
	u32 msaaSamples         = 0;
	b32 textureFiltering    = false;
	b32 anistropicFiltering = false;
	
	//// runtime changeable ////
	u32 loggingLevel = 1; //if printf is true in the config file, this will be set to 4
	b32 crashOnError = false;
	VSyncType vsync  = VSyncType_Immediate;
	
	//shaders
	b32 optimizeShaders = false;
	
	//shadows
	b32 shadowPCF         = false;
	u32 shadowResolution  = 2048;
	f32 shadowNearZ       = 1.f;
	f32 shadowFarZ        = 70.f;
	f32 depthBiasConstant = 1.25f;
	f32 depthBiasSlope    = 1.75f;
	b32 showShadowMap     = false;
	
	//colors
	vec4 clearColor   {0.02f,0.02f,0.02f,1.00f};
	vec4 selectedColor{0.80f,0.49f,0.16f,1.00f};
	vec4 colliderColor{0.46f,0.71f,0.26f,1.00f};
	
	//filters
	bool wireframeOnly = false;
	
	//overlays
	bool meshWireframes = false;
	bool meshNormals    = false;
	bool lightFrustrums = false;
	bool tempMeshOnTop  = false;
}RenderSettings;

extern RenderSettings renderSettings;

// NOTE(sushi) remove and replace with a ui vertex type
typedef struct Vertex2{
	vec2 pos;
	vec2 uv;
	u32  color;
}Vertex2;

typedef u32 RenderModelIndex; //NOTE(delle) changing this also requires changing defines in the backend
typedef struct RenderModelCmd{
	u32   vertex_offset;
	u32   index_offset;
	u32   index_count;
	u32   material;
	char* name;
	mat4  matrix;
}RenderModelCmd;

typedef u32 RenderTwodIndex;  //NOTE(delle) changing this also requires changing defines in the backend
typedef struct RenderTwodCmd{
	void*    handle; //NOTE(delle) VkDescriptorSet in vulkan, texture index in OpenGl
	Vertex2* vertex_buffer;
	u32*     index_buffer; // pointer to used index buffer
	u64      index_offset;
	u64      index_count;
	vec2     scissor_offset;
	vec2     scissor_extent;
	str8     debug_info;
}RenderTwodCmd;

typedef struct RenderMesh{
	Mesh* base;
	u32   vertexOffset;
	u32   vertexCount;
	u32   indexOffset;
	u32   indexCount;
}RenderMesh;

//holds handles to vertex and index handles
typedef struct RenderTwodBuffer{
	u32 idx;
	void* vertex_handle;
	void* index_handle;
}RenderTwodBuffer;

// these are flags because in some usecases we are able to specify 
// multiple stages, such as in specifying descriptors
enum RenderShaderStage {
	RenderShaderStage_Vertex   = 1 << 0,
	RenderShaderStage_Geometry = 1 << 1, 
	RenderShaderStage_Fragment = 1 << 2,
	RenderShaderStage_Compute  = 1 << 3,
	// add more stages as we come across uses for them
};

// This determines how an image is laid out in memory on the gpu.
// Throughout the rendering process a GPU will shuffle the data of a image
// around in order to optimize usages of it. Exactly how it does this is 
// implementation specific but we can hint at (at least in Vulkan) the way
// that an image will be used by telling the GPU what layout we'd like the 
// image to be at at certain points.
enum RenderImageLayout {
	RenderImageLayout_Undefined,
	RenderImageLayout_General,
	RenderImageLayout_Color_Attachment_Optimal,
	RenderImageLayout_Depth_Stencil_Attachment_Optimal,
	RenderImageLayout_Depth_Stencil_Read_Only_Optimal,
	RenderImageLayout_Present,
	RenderImageLayout_Shader_Read_Only_Optimal,
};

typedef struct RenderGlobal {
	
	struct {
		RenderBuffer* vertex_buffer;
		u32 vertex_count;
		RenderBuffer* index_buffer;
		u32 index_count;
		
		RenderPipeline* pipeline;
	} temp_filled;
	
	struct {
		RenderBuffer* vertex_buffer;
		u32 vertex_count;
		RenderBuffer* index_buffer;
		u32 index_count;
		
		RenderPipeline* pipeline;
	} temp_wireframe;
	
	RenderPass* temp_render_pass;
	
	struct {
		mat4 proj;
		mat4 view;
	} temp_camera_ubo;
	RenderBuffer* temp_camera_buffer;
	RenderDescriptorSet* temp_descriptor_set;
	
	struct {
		RenderDescriptorSetLayout* descriptor_set_layouts;
		RenderDescriptorSet*       descriptor_sets;
		RenderPipelineLayout*      pipeline_layouts;
		RenderPipeline*            pipelines;
		RenderBuffer*              buffers;
		RenderCommandBuffer*       command_buffers;
		RenderImage*               images;
		RenderImageView*           image_views;
		RenderSampler*             samplers;
		RenderPass*                passes;
		RenderFramebuffer*         framebuffers;
	} pools;
	
} RenderGlobal;

global RenderGlobal g_render;

enum RenderDescriptorType {
	RenderDescriptorType_Combined_Image_Sampler,
	RenderDescriptorType_Uniform_Buffer,
	// these two seem to be the only ones used for now
	// add more as they seem useful later
};

typedef struct RenderDescriptorLayoutBinding {
	RenderDescriptorType kind;
	RenderShaderStage shader_stages;
	u32 binding;
} RenderDescriptorLayoutBinding;

// A descriptor set layout determines the resources that a part of
// a descriptor set will point to. 
typedef struct RenderDescriptorSetLayout {
	str8 debug_name;
	
	RenderDescriptorLayoutBinding* bindings;
	
	void* handle;
} RenderDescriptorSetLayout;

// Allocates and returns a handle to a descriptor layout.
RenderDescriptorSetLayout* render_descriptor_layout_create();

// Updates the given descriptor layout with the current backend.
void render_descriptor_layout_update(RenderDescriptorSetLayout* x);

// Deallocates the given descriptor layout and frees any resources 
// created for it in the backend.
void render_descriptor_layout_destroy(RenderDescriptorSetLayout* x);

// Represents the memory pointed to by a descriptor set.
typedef struct RenderDescriptor {
	RenderDescriptorType kind;
	RenderShaderStage shader_stages;
	
	union {
		struct {
			RenderImageView* view;
			RenderSampler* sampler;
			RenderImageLayout layout;
		} image;
		
		struct {
			RenderBuffer* handle;
			u64 offset;
			u64 range;
		} buffer;
	};
} RenderDescriptor;

// A descriptor set is a collection of pointers to resources
// used in shaders. For example, if we have a UBO in the first
// layout and a collection of four textures in the second, then 
// the descriptor set will be a pointer pointing at that UBO
// then four pointers pointing at the image memory on the GPU.
// These are useful for quickly swapping resources. For example,
// if you have two collection of objects that use the same textures
// but different UBOs, you can quickly swap the data used by the shader
// by first binding a descriptor set pointing to the first ubo, then
// binding the one that points to the other. 
typedef struct RenderDescriptorSet {
	str8 debug_name;
	
	RenderDescriptorSetLayout** layouts;
	
	void* handle;
} RenderDescriptorSet;

// Allocates a descriptor set and returns a handle for it.
RenderDescriptorSet* render_descriptor_set_create();

// Updates a descriptor set with the backend.
void render_descriptor_set_update(RenderDescriptorSet* x);

// Setup the given descriptor set to point to the resources specified
// in the given descriptors. The descriptors must be given in the correct 
// order relative to the layout specified on the given descriptor set.
// TODO(sushi) writing to a specific binding
void render_descriptor_set_write(RenderDescriptorSet* x, RenderDescriptor* descriptors);

// Deallocates the given descriptor set and frees anything allocated 
// on the backend. Note that this doesn't affect the resources pointed
// to by the descriptor set.
void render_descriptor_set_destroy(RenderDescriptorSet* x);

// A push constant is a somewhat small amount of data that may
// be uploaded efficiently to a shader. Push constants upload 
// their data via a RenderCommand rather than the 'normal' way 
// of using UBOs. The data does not need to be mapped and, since 
// it is uploaded using a command, can be used efficiently
// for data that changes per model.
typedef struct RenderPushConstant {
	// what sort of shader this constant will be pushed to
	RenderShaderStage shader_stage_flags;
	u64 size; // the size of the constant in bytes
	u64 offset; // offset of the constant in bytes
} RenderPushConstant;

// Describes the data that a pipeline will use. See 
// RenderDescriptorLayout and RenderPushConstant for 
// more information.
typedef struct RenderPipelineLayout {
	str8 debug_name;
	
	RenderDescriptorSetLayout** descriptor_layouts;
	RenderPushConstant* push_constants;
	
	// handle to backend's represenatation of descriptor set layouts
	void* handle;
} RenderPipelineLayout;

RenderPipelineLayout* render_pipeline_layout_create();
RenderPipelineLayout* render_pipeline_layout_create_default();
void render_pipeline_layout_update(RenderPipelineLayout* x);

// a shader to be compiled and used as a stage in a RenderPipeline
typedef struct RenderShader {
	RenderShaderStage kind;
	str8 name;
	str8 source;
} RenderShader;

// collection of image formats that deshi's renderer supports
// TODO(sushi) more formats	
enum RenderFormat {
	RenderFormat_R32G32_Signed_Float,
	RenderFormat_R32G32B32_Signed_Float,
	RenderFormat_R8G8B8_StandardRGB,
	RenderFormat_R8G8B8_UnsignedNormalized,
	RenderFormat_R8G8B8A8_StandardRGB,
	RenderFormat_R8G8B8A8_UnsignedNormalized,
	RenderFormat_B8G8R8A8_UnsignedNormalized,
	// one component, a 16 bit unsigned normalized integer depth component
	RenderFormat_Depth16_UnsignedNormalized,
	RenderFormat_Depth32_SignedFloat,
	// two components, a 32 bit floating point depth component and 8 bit unsigned int stencil component
	RenderFormat_Depth32_SignedFloat_Stencil8_UnsignedInt,
	RenderFormat_Depth24_UnsignedNormalized_Stencil8_UnsignedInt,
};

enum RenderPipelineCulling {
	RenderPipelineCulling_None,
	RenderPipelineCulling_Front,
	RenderPipelineCulling_Back,
	RenderPipelineCulling_Front_Back,
};

enum RenderPipelineFrontFace {
	RenderPipelineFrontFace_CCW,
	RenderPipelineFrontFace_CW,
};

enum RenderPipelinePolygonMode {
	RenderPipelinePolygonMode_Point,
	RenderPipelinePolygonMode_Line,
	RenderPipelinePolygonMode_Fill,
};

enum RenderCompareOp {
	RenderCompareOp_Never,
	RenderCompareOp_Less,
	RenderCompareOp_Equal,
	RenderCompareOp_Less_Or_Equal,
	RenderCompareOp_Greater,
	RenderCompareOp_Not_Equal,
	RenderCompareOp_Greater_Or_Equal,
	RenderCompareOp_Always,
};

enum RenderBlendOp {
	RenderBlendOp_Add,
	RenderBlendOp_Sub,
	RenderBlendOp_Reverse_Sub,
	RenderBlendOp_Min,
	RenderBlendOp_Max,
};

// TODO(sushi) write some explanation for what this is doing
//             and some examples
enum RenderBlendFactor {
	RenderBlendFactor_Zero,
	RenderBlendFactor_One,
	RenderBlendFactor_Source_Color,
	RenderBlendFactor_One_Minus_Source_Color,
	RenderBlendFactor_Destination_Color,
	RenderBlendFactor_One_Minus_Destination_Color,
	RenderBlendFactor_Source_Alpha,
	RenderBlendFactor_One_Minus_Source_Alpha,
	RenderBlendFactor_Destination_Alpha,
	RenderBlendFactor_One_Minus_Destination_Alpha,
	RenderBlendFactor_Constant_Color,
	RenderBlendFactor_One_Minus_Constant_Color,
	RenderBlendFactor_Constant_Alpha,
	RenderBlendFactor_One_Minus_Constant_Alpha,
	// TODO(sushi) implement the other blend factors described at 
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#framebuffer-blendfactors
	// if they seem useful
};

// NOTE(sushi) at this time the only supported sample count is 1
//             as we do not yet support msaa or multisampled images
enum RenderSampleCount {
	RenderSampleCount_1  = 1 << 0,
	RenderSampleCount_2  = 1 << 1,
	RenderSampleCount_4  = 1 << 2,
	RenderSampleCount_8  = 1 << 3,
	RenderSampleCount_16 = 1 << 4,
	RenderSampleCount_32 = 1 << 5,
	RenderSampleCount_64 = 1 << 6,
};

// TODO(sushi) this may not be necessary but I'm including it for now 
//             just so that migrating is simpler
// All this does is tell the renderer that we aren't using values stored on the pipeline
// and are instead setting them immediately using some function. 
// This currently only applies to Vulkan (in OpenGL everything is immediate)
// and I don't know if DirectX does anything similar, so we'll probably want to 
// remove this eventually
enum RenderDynamicState {
	RenderDynamicState_Viewport,
	RenderDynamicState_Scissor,
	RenderDynamicState_Line_Width,
	RenderDynamicState_Depth_Bias,
	RenderDynamicState_Blend_Constants,
	RenderDynamicState_Depth_Bounds,
};

typedef struct RenderVertexInputBindingDescription {
	u32 binding;
	u32 stride; // number of bytes between one entry and the next (aka the vertex's size)
} RenderVertexInputBindingDescription;

typedef struct RenderVertexInputAttributeDescription {
	u32 location;
	u32 binding;
	RenderFormat format;
	u32 offset;
} RenderVertexInputAttributeDescription;

typedef struct RenderPipeline {
	str8 name;
	// kigu array of shaders 
	RenderShader* shader_stages;
	
	//// rasterization settings ////
	
	// How the front facing direction of a triangle is determined
	RenderPipelineFrontFace front_face;
	// How faces should be culled before fragments are produced
	// NOTE(sushi) set when creating pipelines in vulkan
	//             for opengl:
	//             		glCullFace(...)
	//             		glEnable/Disable(GL_CULL_FACE)
	RenderPipelineCulling culling;
	// how polygons are drawn, as points, lines, or filled faces
	RenderPipelinePolygonMode polygon_mode;
	// whether or not to perform depth testing
	b32 depth_test;
	// whether or not to allow writing depth
	b32 depth_write;
	// how depth values are compared
	RenderCompareOp depth_compare_op;
	// whether or not to apply a bias to depth testing 
	// for opengl: https://stackoverflow.com/questions/45314290/depth-offset-in-opengl
	b32 depth_bias;
	f32 depth_bias_constant;
	f32 depth_bias_clamp;
	f32 depth_bias_slope;
	// the width of lines drawn when using line polygon mode (maybe)
	f32 line_width;
	
	//// MSAA ////
	RenderSampleCount msaa_samples;
	b32 sample_shading;
	// TODO(sushi) more settings if seems useful
	
	//// color blending ////
	
	// perform color blending?
	b32 color_blend;
	// how colors are blended
	RenderBlendOp color_blend_op;
	RenderBlendFactor color_src_blend_factor;
	RenderBlendFactor color_dst_blend_factor;
	RenderBlendOp alpha_blend_op;
	RenderBlendFactor alpha_src_blend_factor;
	RenderBlendFactor alpha_dst_blend_factor;
	// a constant color to blend with 
	color blend_constant;
	// TODO(sushi) logical ops for color blending if it ever seems useful
	
	RenderVertexInputBindingDescription* vertex_input_bindings;
	RenderVertexInputAttributeDescription* vertex_input_attributes;
	
	// TODO(sushi) this doesn't need to be a dynamic array
	RenderDynamicState* dynamic_states;
	// pointer to a RenderPipelineLayout object retrieved from render
	RenderPipelineLayout* layout;
	
	RenderPass* render_pass;
	
	// handle the to backend's representation of a pipeline
	void* handle;
} RenderPipeline;

// Allocates and returns a new RenderPipeline.
// Properties on the given handle may then be set followed by a call
// to render_pipeline_update().
RenderPipeline* render_pipeline_create();

// Allocates and returns a new RenderPipeline with 'default' properties set.
RenderPipeline* render_pipeline_create_default();

// Creates a new pipeline with the same settings as the one given.
RenderPipeline* render_pipeline_duplicate(RenderPipeline* x);

// Updates the given pipeline with the current backend.
void render_pipeline_update(RenderPipeline* x);

// Deallocates the given pipeline handle.
void render_pipeline_destroy(RenderPipeline* x);

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_buffer
typedef Flags RenderBufferUsageFlags; enum{
	RenderBufferUsage_TransferSource      = (1 << 0),
	RenderBufferUsage_TransferDestination = (1 << 1),
	RenderBufferUsage_UniformTexelBuffer  = (1 << 2),
	RenderBufferUsage_StorageTexelBuffer  = (1 << 3),
	RenderBufferUsage_UniformBuffer       = (1 << 4),
	RenderBufferUsage_StorageBuffer       = (1 << 5),
	RenderBufferUsage_IndexBuffer         = (1 << 6),
	RenderBufferUsage_VertexBuffer        = (1 << 7),
	RenderBufferUsage_IndirectBuffer      = (1 << 8),
};

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
typedef Flags RenderMemoryPropertyFlags; enum {
	RenderMemoryPropertyFlag_DeviceLocal     = (1 << 0), //device memory, fastest device access
	RenderMemoryPropertyFlag_HostVisible     = (1 << 1), //device memory that can be mapped for host access (not compatible with RenderMemoryPropertyFlag_LazilyAllocated)
	RenderMemoryPropertyFlag_HostCoherent    = (1 << 2), //host memory that can be read by device over PCIe as needed, changes don't need to be flushed (implies RenderMemoryPropertyFlag_HostVisible)
	RenderMemoryPropertyFlag_HostCached      = (1 << 3), //device memory that is also cached on the host (requires RenderMemoryPropertyFlag_HostVisible)
	RenderMemoryPropertyFlag_LazilyAllocated = (1 << 4), //device only access, commits memory as needed (not compatible with RenderMemoryPropertyFlag_HostVisible)
	
	RenderMemoryProperty_DeviceOnly     = RenderMemoryPropertyFlag_DeviceLocal,                                            //not mappable, uses staging buffers (must have RenderMemoryMapping_None)
	RenderMemoryProperty_DeviceOnlyLazy = RenderMemoryPropertyFlag_DeviceLocal | RenderMemoryPropertyFlag_LazilyAllocated, //device only committed as used (must have RenderMemoryMapping_None)
	RenderMemoryProperty_DeviceMappable = RenderMemoryPropertyFlag_DeviceLocal | RenderMemoryPropertyFlag_HostVisible,     //device-dependent, changes must be flushed
	RenderMemoryProperty_HostStreamed   = RenderMemoryPropertyFlag_HostVisible | RenderMemoryPropertyFlag_HostCoherent,    //host local memory, read by device over PCIe
};

typedef Type RenderMemoryMappingType; enum{
	RenderMemoryMapping_None,          //the memory is never mapped after initial upload (not compatible with RenderMemoryPropertyFlag_HostVisible, RenderMemoryPropertyFlag_HostCoherent, or RenderMemoryPropertyFlag_HostCached)
	RenderMemoryMapping_MapWriteUnmap, //map the memory to the host, write data to the allocation, and unmap the memory every time (must have RenderMemoryPropertyFlag_HostVisible, RenderMemoryPropertyFlag_HostCoherent, or RenderMemoryPropertyFlag_HostCached)
	RenderMemoryMapping_Persistent,    //map the memory to the host right after it is allocated, and don't unmap until deletion (must have RenderMemoryPropertyFlag_HostVisible, RenderMemoryPropertyFlag_HostCoherent, or RenderMemoryPropertyFlag_HostCached)
};

typedef struct RenderBuffer{
	void* buffer_handle; //VkBuffer in vulkan, GLuint in OpenGL, ... in DirectX
	void* memory_handle; //VkDeviceMemory in vulkan, unused in OpenGL, ... in DirectX
	
	u64 size;
	
	void* mapped_data; //null if not mapped
	u64 mapped_offset;
	u64 mapped_size;
	
	RenderBufferUsageFlags usage;
	RenderMemoryPropertyFlags properties;
	RenderMemoryMappingType mapping;
}RenderBuffer;

//Creates a `RenderBuffer*`, allocates at least `size` bytes on the device, and uploads `size` bytes at `data` to the device
//  `data` can be a null pointer, in which case the buffer memory will be allocated but nothing will be uploaded
//  `usage` determines how the buffer can be used
//  `properties` determines how the buffer memory can be accessed
//  `mapping` determines the duration a buffer will be mapped between the device and host
RenderBuffer* render_buffer_create(void* data, u64 size, RenderBufferUsageFlags usage, RenderMemoryPropertyFlags properties, RenderMemoryMappingType mapping);

//Deletes the `buffer` from the device and host
void render_buffer_delete(RenderBuffer* buffer);

//Maps `size` bytes at `offset` from an unmapped `buffer` with host-visible memory
//  allows the host to modify memory at `buffer.mapped_data` which can be flushed back to the device
//  `buffer.properties` must have these flags set: RenderMemoryPropertyFlag_DeviceLocal | RenderMemoryPropertyFlag_HostVisible
//  `buffer.mapping` must be RenderMemoryMapping_MapWriteUnmap
void render_buffer_map(RenderBuffer* buffer, u64 offset, u64 size);

//Unmaps a mapped `buffer` with host-visible memory
//  `flush` will flush the mapped data back to the device before unmapping
//  `buffer.properties` must have these flags set: RenderMemoryPropertyFlag_DeviceLocal | RenderMemoryPropertyFlag_HostVisible
//  `buffer.mapping` must be RenderMemoryMapping_MapWriteUnmap
void render_buffer_unmap(RenderBuffer* buffer, b32 flush);

//Flushes the contents of a mapped `buffer` back to the device
//  `buffer.properties` must have these flags set: RenderMemoryPropertyFlag_DeviceLocal | RenderMemoryPropertyFlag_HostVisible
//  `buffer.mapping` must be RenderMemoryMapping_MapWriteUnmap
void render_buffer_flush(RenderBuffer* buffer);

enum RenderCommandType {
	RenderCommandType_Bind_Pipeline,
	// TODO(sushi) vertex/index buffer bind pair
	RenderCommandType_Bind_Vertex_Buffer,
	RenderCommandType_Bind_Index_Buffer,
	RenderCommandType_Bind_Descriptor_Set,
	RenderCommandType_Push_Constant,
	// draws a set of indexes from the currently bound index buffer
	RenderCommandType_Draw_Indexed,
	RenderCommandType_Begin_Render_Pass,
	RenderCommandType_End_Render_Pass,
	// Using any of the following commands requires setting 
	// the pipeline's dynamic states.
	RenderCommandType_Set_Viewport,
	RenderCommandType_Set_Scissor,
	RenderCommandType_Set_Depth_Bias,
};

// A command used to instruct the backend in rendering
// things. Actual use of these is achieved via the 
// render_cmd_* functions below.
typedef struct RenderCommand {
	RenderCommandType type;
	union {
		
		struct { // bind_pipeline
			RenderPipeline* handle;
		} bind_pipeline;
		
		struct { // bind_vertex_buffer, bind_index_buffer
			RenderBuffer* handle;
		} bind_vertex_buffer, bind_index_buffer;
		
		struct { // bind_descriptor_set
			u32 set_index;
			RenderDescriptorSet* handle;
		} bind_descriptor_set;
		
		struct { // push_constant
			void* data;
			RenderPushConstant info;
		} push_constant;

		struct { // push_descriptor_set
			RenderDescriptor* descriptors;
		} push_descriptor_set;
		
		struct { // draw_indexed
			u64 index_count;
			u64 index_offset;
			u64 vertex_offset;
		} draw_indexed;
		
		struct {
			RenderPass* pass;
			RenderFramebuffer* frame; 
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
} RenderCommand;

// Begins the given render pass for the given frame. 
// A render pass MUST be active to record commands.
// Render passes may not be nested.
void render_cmd_begin_render_pass(Window* window, RenderPass* render_pass, RenderFramebuffer* frame);
// Ends the currently active render pass.
void render_cmd_end_render_pass(Window* window);
// Binds the given pipeline. The pipeline must be compatible with the currently active render pass.
void render_cmd_bind_pipeline(Window* window, RenderPipeline* pipeline);
// Binds the given vertex buffer. 
void render_cmd_bind_vertex_buffer(Window* window, RenderBuffer* buffer);
// Binds the given index buffer.
void render_cmd_bind_index_buffer(Window* window, RenderBuffer* buffer);
// Binds the given descriptor set to 'set_index'. The descriptor set must be compatible with the 
// relevant descriptor layout of the currently bound pipeline.
void render_cmd_bind_descriptor_set(Window* window, u32 set_index, RenderDescriptorSet* descriptor_set);
// Register the memory at 'data' to be pushed according to 'info'. The data provided must be kept valid
// until the end of the following render_update call.
void render_cmd_push_constant(Window* window, void* data, RenderPushConstant info);
// Draw vertexes based on the indexes in the currenly bound vertex and index buffers.
void render_cmd_draw_indexed(Window* window, u32 index_count, u32 index_offset, u32 vertex_offset);
// Set the viewport of the current render pass. The currently bound pipeline must have been created 
// with RenderDynamicStates_Viewport in its dynamic states array.
void render_cmd_set_viewport(Window* window, vec2 offset, vec2 extent);
// Set the scissor of the current render pass. The currently bound pipeline must have been created 
// with RenderDynamicStates_Scissor in its dynamic states array.
void render_cmd_set_scissor(Window* window, vec2 offset, vec2 extent);
// Set the depth bias of the current render pass. The currently bound pipeline must have been created
// with RenderDynamicStates_Depth_Bias in its dynamic states array.
void render_cmd_set_depth_bias(Window* window, f32 constant, f32 clamp, f32 slope);

#if COMPILER_FEATURE_CPP
namespace render::cmd {
FORCE_INLINE void begin_render_pass(Window* window, RenderPass* render_pass, RenderFramebuffer* frame){ render_cmd_begin_render_pass(window, render_pass, frame); }
FORCE_INLINE void end_render_pass(Window* window){ render_cmd_end_render_pass(window); }
FORCE_INLINE void bind_pipeline(Window* window, RenderPipeline* pipeline){ render_cmd_bind_pipeline(window, pipeline); }
FORCE_INLINE void bind_vertex_buffer(Window* window, RenderBuffer* buffer){ render_cmd_bind_vertex_buffer(window, buffer); }
FORCE_INLINE void bind_index_buffer(Window* window, RenderBuffer* buffer){ render_cmd_bind_index_buffer(window, buffer); }
FORCE_INLINE void bind_descriptor_set(Window* window, u32 set_index, RenderDescriptorSet* descriptor_set){ render_cmd_bind_descriptor_set(window, set_index, descriptor_set); }
FORCE_INLINE void push_constant(Window* window, void* data, RenderPushConstant info){ render_cmd_push_constant(window, data, info); }
FORCE_INLINE void draw_indexed(Window* window, u32 index_count, u32 index_offset, u32 vertex_offset){ render_cmd_draw_indexed(window, index_count, index_offset, vertex_offset); }
FORCE_INLINE void set_viewport(Window* window, vec2 offset, vec2 extent){ render_cmd_set_viewport(window, offset, extent); } 
FORCE_INLINE void set_scissor(Window* window, vec2 offset, vec2 extent){ render_cmd_set_scissor(window, offset, extent); }
FORCE_INLINE void set_depth_bias(Window* window, f32 constant, f32 clamp, f32 slope){ render_cmd_set_depth_bias(window, constant, clamp, slope); }
} // namespace render::cmd
#endif

// NOTE(sushi) this is currently only used internally
//             We store one on each window and commands are added 
//             by using the render_cmd_* functions.
//             In the future I would like to play around with
//             supporting an api for this so that we can do things
//             like build commands in parallel or have command buffers
//             that aren't meant to be cleared every frame.
typedef struct RenderCommandBuffer {
	str8 debug_name;
	RenderCommand* commands;
	void* handle;
} RenderCommandBuffer;

// Creates a command buffer. 
// NOTE(sushi) see RenderCommandBuffer definition 
//             for notes on why not to use this yet
RenderCommandBuffer* render_command_buffer_create();

// maybe this could actually queue commands?
// currently it just creates the backend's command buffer object
// NOTE(sushi) see RenderCommandBuffer definition 
//             for notes on why not to use this yet
void render_command_buffer_update(RenderCommandBuffer* x);

// NOTE(sushi) see RenderCommandBuffer definition 
//             for notes on why not to use this yet
RenderCommandBuffer* render_command_buffer_of_window(Window* window);

// NOTE(sushi) see RenderCommandBuffer definition 
//             for notes on why not to use this yet
void render_command_buffer_destroy(RenderCommandBuffer* x);

enum RenderImageType {
	RenderImageType_OneD,
	RenderImageType_TwoD,
	RenderImageType_ThreeD,
};

// TODO(sushi) explain how each flag allows the image to be used 
//             with other parts of the api
enum RenderImageUsage {
	RenderImageUsage_Transfer_Source          = 1 << 0,
	RenderImageUsage_Transfer_Destination     = 1 << 1,
	RenderImageUsage_Sampled                  = 1 << 2, 
	RenderImageUsage_Storage                  = 1 << 3,
	RenderImageUsage_Color_Attachment         = 1 << 4,
	RenderImageUsage_Depth_Stencil_Attachment = 1 << 5,
};

// Represents an image allocated on the GPU.
typedef struct RenderImage {
	str8 debug_name;
	// TODO(sushi) support other image types
	// RenderImageType   type;
	RenderFormat      format;
	RenderImageUsage  usage;
	RenderSampleCount samples;
	vec3i             extent;
	b32               linear_tiling;
	
	// how the memory of the image is intended to be used 
	RenderMemoryPropertyFlags memory_properties;
	
	// TODO(sushi)
	//u64 mip_levels;
	//u64 array_layers;
	
	void* handle;
	void* memory_handle;
} RenderImage;

// Allocates a render image and returns a handle for it.
RenderImage* render_image_create();

// Updates the given render image on the backend and allocates 
// backend resources for it.
void render_image_update(RenderImage* x);

// Uploads the given pixels to the given render image
// TODO(sushi) make the api for this closer to that of RenderBuffer
//             so the user has more flexibility in modifying 
//             render images
//             or just make it so that this takes ranges
void render_image_upload(RenderImage* x, u8* pixels, vec2i offset, vec2i extent);

// Deallocates the given render image and frees any resources
// allocated for it in the backend.
void render_image_destroy(RenderImage* image);

enum RenderImageViewType {
	RenderImageViewType_OneD,
	RenderImageViewType_TwoD,
	RenderImageViewType_ThreeD,
	RenderImageViewType_Cube,
	RenderImageViewType_OneD_Array,
	RenderImageViewType_TwoD_Array,
	RenderImageViewType_Cube_Array,
};

enum RenderImageViewAspectFlags {
	RenderImageViewAspectFlags_Color   = 1 << 0,
	RenderImageViewAspectFlags_Depth   = 1 << 1,
	RenderImageViewAspectFlags_Stencil = 1 << 2,
};

typedef struct RenderImageView {
	str8 debug_name;
	// TODO(sushi) support other image types
	// RenderImageViewType type;
	RenderFormat format;
	RenderImageViewAspectFlags aspect_flags;
	
	RenderImage* image;
	
	void* handle;
} RenderImageView;

// Allocates a render image view and returns a handle to it.
RenderImageView* render_image_view_create();

// Updates the given image view on the backend.
// If information already exists on the backend it will be destroyed
// and recreated.
void render_image_view_update(RenderImageView* x);

// Deallocates the given image view and frees any information
// allocated for it on the backend.
void render_image_view_destroy(RenderImageView* x);

enum RenderFilter {
	RenderFilter_Nearest,
	RenderFilter_Linear,
};

enum RenderSamplerAddressMode {
	RenderSamplerAddressMode_Repeat,
	RenderSamplerAddressMode_Mirrored_Repeat,
	RenderSamplerAddressMode_Clamp_To_Edge,
	RenderSamplerAddressMode_Clamp_To_Border,
};

// A sampler, which is typically used in a shader to 
// read an image.
typedef struct RenderSampler {
	// Behavoir when the image is magnified or minified.
	RenderFilter mag_filter;
	RenderFilter min_filter;
	
	// Behavoir when sampling an image outside 
	// of the range [0, 1)
	RenderSamplerAddressMode address_mode_u;
	RenderSamplerAddressMode address_mode_v;
	RenderSamplerAddressMode address_mode_w;
	color border_color;
	
	// TODO(sushi)
	u64 mipmaps;
	
	void* handle;
	
	str8 debug_name;
} RenderSampler;

// Allocates a render sampler and returns a handle for it.
RenderSampler* render_sampler_create();

// Updates the given sampler with the current backend. If 
// information has already been allocated for the given object
// it will be destroyed and recreated.
void render_sampler_update(RenderSampler* x);

// Deallocates the given render sampler and frees any 
// memory created for it in the backend.
void render_sampler_destroy(RenderSampler* x);

enum RenderAttachmentStoreOp {
	RenderAttachmentStoreOp_Store,
	RenderAttachmentStoreOp_Dont_Care,
};

enum RenderAttachmentLoadOp {
	RenderAttachmentLoadOp_Load,
	RenderAttachmentLoadOp_Clear,
	RenderAttachmentLoadOp_Dont_Care,
};

// An attachement represents an image used in a framebuffer
// and describes four things:
// 	1. What format the image is in
// 	2. How many times to sample the image (TODO)
// 	3. Behavoir regarding the loading and storing of the attachment's color and stencils.
// 	4. The layout the image starts in and the layout it should be in when it has finished processing.
typedef struct RenderPassAttachment {
	RenderFormat            format;
	RenderSampleCount       samples;
	RenderAttachmentLoadOp  load_op;
	RenderAttachmentStoreOp store_op;
	RenderAttachmentLoadOp  stencil_load_op;
	RenderAttachmentStoreOp stencil_store_op;
	// the image layout we expect the attachment to be in before we 
	// start processing it
	RenderImageLayout       initial_layout;
	// the image layout we expect the image to be in
	// once it is done being processed
	RenderImageLayout       final_layout;
	
} RenderPassAttachment;

// A render pass represents two attachments and describes
// how the attachments are used over the course of a render pass.
typedef struct RenderPass {
	str8  debug_name;
	color debug_color;
	
	// optional pointers to a color/depth attachment
	// this data must exist during the duration of a call
	// to render_pass_update and is not copied internally
	RenderPassAttachment* color_attachment;
	RenderPassAttachment* depth_attachment;
	
	color color_clear_values;
	struct {
		f32 depth;
		u32 stencil;
	} depth_clear_values;
	
	void* handle;
} RenderPass;

// Allocates a render pass and returns a handle for it.
RenderPass* render_pass_create();

// Creates or recreates information for the given render pass
// in the current backend.
void render_pass_update(RenderPass* x);

// Deallocates the given render pass and frees any information 
// allocated for it in the current backend.
void render_pass_destroy(RenderPass* x);

// Returns the render pass used by the presentation frames of the 
// given window.
RenderPass* render_pass_of_window_presentation_frame(Window* window);

// A framebuffer is a collection of views on images that a 
// render pass uses. While a render pass describes the way 
// attachments are used, a framebuffer actually represents 
// those attachments.
typedef struct RenderFramebuffer {
	str8 debug_name;
	
	// render pass describing how this frame behaves
	RenderPass* render_pass;
	
	u32 width;
	u32 height;
	
	// views over the actual memory the framebuffer works with
	// TODO(sushi) I don't think these need to be render representations anymore
	RenderImageView* color_image_view;
	RenderImageView* depth_image_view;
	void* handle;
} RenderFramebuffer;

// Allocates a render framebuffer and returns a handle to it.
RenderFramebuffer* render_frame_create();

// Creates or recreates backend information for the given
// framebuffer.
void render_frame_update(RenderFramebuffer* x);

// Deallocates the given framebuffer and frees any backend information
// that was created for it.
void render_destroy_frame(RenderFramebuffer* x);

// Retrieve the frame that will be presented on the next call to render_update(window)
RenderFramebuffer* render_current_present_frame_of_window(Window* window);

enum{
	RenderBookKeeper_Vertex,
	RenderBookKeeper_Index,
	RenderBookKeeper_Cmd,
};

// keeps track of vertex, index, and cmd changes
typedef struct RenderBookKeeper{
	Type type;
	str8 file;
	u32  line;
	union {
		struct{
			Vertex2* start;
			u32 count;
		}vertex;
		struct{
			u32* start;
			u32 count;
		}index;
		RenderTwodCmd* cmd;
	};
}RenderBookKeeper;

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_status
//Initializes the `Render` module
void render_init();

// initializes the backend with the given window
// 0 may be passed 
// TODO(sushi) explain what 0 implies
void render_init_x(Window* window);

//Updates the `Render` module
void render_update();

void render_update_x(Window* window);

//Resets the `Render` module
void render_reset();

//Cleans up the `Render` module
void render_cleanup();

//Loads render settings from the render config file
void render_load_settings();

//Saves current render settings to the render config file
void render_save_settings();

//Returns the internal `RenderSettings`
RenderSettings* render_get_settings();

//Returns the internal `RenderStats`
RenderStats* render_get_stats();

//Returns the internal `RenderStage`
RenderStage* render_get_stage();





//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_surface
//Returns the maximum number of surfaces the backend supports
u32 render_max_surface_count();

//Creates a render surface for `window` with `idx`
void render_register_surface(Window* window);

//Sets the render surface for `window` to the active one
void render_set_active_surface(Window* window);

//Sets the render surface for `idx` to the active one
void render_set_active_surface_idx(u32 idx);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_loading
//Loads the `mesh` vertices and indices to the GPU and stores info to the backend for rendering
void render_load_mesh(Mesh* mesh);

//Loads the `texture` pixels to the GPU and stores info to the backend for rendering
// NOTE(sushi) currently this uses the internal twod descriptor layout which is good
//             for textures but we'll need to see if there are any reasons for allowing
//             a user defined descriptor set 
void render_load_texture(Texture* texture);

//Loads the `material` info to the backend for rendering
void render_load_material(Material* material);

//Unloads the `mesh` vertices and indices from the GPU and info from the backend
void render_unload_mesh(Mesh* mesh);

//Unloads the `texture` pixels from the GPU and info from the backend
void render_unload_texture(Texture* texture);

//Unloads the `material` info from the backend
void render_unload_material(Material* material);

//Updates the internal material copy of `material` with any new changes
void render_update_material(Material* material);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_draw_3d
//Renders the `model` with the transform `matrix`
void render_model(Model* model, mat4* matrix);

void render_model_x(RenderFramebuffer* frame, Model* model, mat4* matrix);

//Renders the a wireframe of `model` with the transform `matrix`
void render_model_wireframe(Model* model, mat4* matrix, color c);

//Renders a 3D line from `start` to `end`
void render_line3(vec3 start, vec3 end, color c);

//Renders a wireframe 3D triangle using the specified points
void render_triangle3(vec3 p0, vec3 p1, vec3 p2, color c);

//Renders a filled 3D triangle using the specified points
void render_triangle_filled3(vec3 p0, vec3 p1, vec3 p2, color c);

//Renders a wireframe 3D quad using the specified points
void render_quad3(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c);

//Renders a filled 3D quad using the specified points
void render_quad_filled3(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c);

//Renders a wireframe 3D polygon using `count` points from `points`
void render_poly3(vec3* points, u64 count, color c);

//Renders a filled 3D polygon using `count` points from `points`
void render_poly_filled3(vec3* points, u64 count, color c);

//Renders a wireframe 3D circle centered at `position` with `rotation` and `radius` using `subdivisions` line segments
void render_circle3(vec3 position, vec3 rotation, f32 radius, u32 subdivisions, color c);

//Renders a wireframe box with a `transform`
void render_box(mat4* transform, color c);

//Renders a filled-face box with a `transform`
void render_box_filled(mat4* transform, color c);

//Renders three 3D circles centered at `position` with `rotation` and `radius` using `subdivisions` line segments
void render_sphere(vec3 position, vec3 rotation, f32 radius, u32 subdivisions, color cx, color cy, color cz);

#if COMPILER_FEATURE_CPP
EndLinkageC();
FORCE_INLINE void render_model(Model* model, vec3 position, vec3 rotation, vec3 scale){ mat4 transform = mat4::TransformationMatrix(position,rotation,scale); render_model(model,&transform); }
FORCE_INLINE void render_poly3(const arrayT<vec3>& points, color c = Color_White){ render_poly3(points.data, points.count, c); }
FORCE_INLINE void render_poly_filled3(const arrayT<vec3>& points, color c = Color_White){ render_poly_filled3(points.data, points.count, c); }
FORCE_INLINE void render_box(vec3 position, vec3 rotation, vec3 scale, color c = Color_White){ mat4 transform = mat4::TransformationMatrix(position, rotation, scale); render_box(&transform,c); }
FORCE_INLINE void render_box_filled(vec3 position, vec3 rotation, vec3 scale, color c = Color_White){ mat4 transform = mat4::TransformationMatrix(position, rotation, scale); render_box_filled(&transform,c); }
FORCE_INLINE void render_sphere(vec3 position, vec3 rotation, f32 radius, u32 subdivisions = 16, color c = Color_White){ render_sphere(position,rotation,radius,subdivisions,c,c,c); }
StartLinkageC();
#endif //COMPILER_FEATURE_CPP

//Renders a frustum shape starting at `position` facing towards `target` and with the properties `aspectRatio`, `fovx`, `nearZ`, `farZ`
void render_frustrum(vec3 position, vec3 target, f32 aspectRatio, f32 fovx, f32 nearZ, f32 farZ, color c);

//Clears the collection of debug lines
void render_clear_debug();

//Creates a debug line (non-immediate drawing) from `p0` to `p1`
void render_debug_line3(vec3 p0, vec3 p1,  color c);

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_draw_2d
//Starts a new `RenderTwodCmd` on `layer` with the specified values
void deshi__render_start_cmd2(str8 file, u32 line, u32 layer, Texture* texture, vec2 scissorOffset, vec2 scissorExtent);
#define render_start_cmd2(layer, texture, scissorOffset, scissorExtent) deshi__render_start_cmd2(str8l(__FILE__), __LINE__, (layer), (texture), (scissorOffset), (scissorExtent))

//Starts a new RenderTwodCmd with the specified values using externally allocated buffers
//NOTE: these buffers must have been mapped using render_update_external_2d_buffer()
void render_start_cmd2_exbuff(RenderTwodBuffer buffer, RenderTwodIndex index_offset, RenderTwodIndex index_count, Vertex2* vertbuff, RenderTwodIndex* indbuff, u32 layer, Texture* texture, vec2 scissorOffset, vec2 scissorExtent);

//TODO(sushi) a full interface for using external buffers
//eg. render_get_buffer_size, render_close_buffer, etc.
//creates an external GPU buffer for 2D drawing information
RenderTwodBuffer render_create_external_2d_buffer(u64 vert_buffsize, u64 ind_buffsize);
void render_update_external_2d_buffer(RenderTwodBuffer* buffer, Vertex2* vb, RenderTwodIndex vcount, RenderTwodIndex* ib, RenderTwodIndex icount);

//Adds `vertices` and `indices` to the active `RenderTwodCmd` on `layer`
//  `indices` values should be local to the addition (start at 0) since they are added to the offset internally
void deshi__render_add_vertices2(str8 file, u32 line, u32 layer, Vertex2* vertices, u32 vCount, u32* indices, u32 iCount);
#define render_add_vertices2(layer, vertices, vcount, indices, icount) deshi__render_add_vertices2(str8l(__FILE__), __LINE__, layer, vertices, vcount, indices, icount)

//Returns the top-most layer for 2D rendering
u32  render_top_layer_index();

//Returns the window decorations layer for 2D rendering (higher than top-most)
u32  render_decoration_layer_index();

//Returns the active layer in 2D rendering
u32  render_active_layer();

//Renders a 2D line to the active `RenderTwodCmd` from `start` to `end` with 1 pixel thickness
void render_line2(vec2 start, vec2 end, color c);

//Renders a 2D line to the active `RenderTwodCmd` from `start` to `end` with `thickness` (in pixels)
void render_line_thick2(vec2 start, vec2 end, f32 thickness, color c);

//Renders a set of 2D lines to the active `RenderTwodCmd` with `thickness` (in pixels)
void render_lines2(vec2* points, u64 count, f32 thickness, color c);

//Renders a wireframe 2D triangle to the active `RenderTwodCmd` using the specified points
void render_triangle2(vec2 p0, vec2 p1, vec2 p2, color c);

//Renders a filled 2D triangle to the active `RenderTwodCmd` using the specified points
void render_triangle_filled2(vec2 p0, vec2 p1, vec2 p2, color c);

//Renders a wireframe 2D quad to the active `RenderTwodCmd` using the specified points
void render_quad2(vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, color c);

//Renders a wireframe 2D quad to the active `RenderTwodCmd` using the specified points and line `thickness`
void render_quad_thick2(vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, f32 thickness, color c);

//Renders a filled 2D quad to the active `RenderTwodCmd` using the specified points
void render_quad_filled2(vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, color c);

//Renders a wireframe 2D polygon to the active `RenderTwodCmd` using `count` points from `points`
void render_poly2(vec2* points, u64 count, color c);

//Renders a filled 2D polygon to the active `RenderTwodCmd` using `count` points from `points`
void render_poly_filled2(vec2* points, u64 count, color c);

#if COMPILER_FEATURE_CPP
EndLinkageC();
FORCE_INLINE void render_lines2(arrayT<vec2>& points, f32 thickness = 1, color c = Color_White){ render_lines2(points.data,points.count,thickness,c); }
FORCE_INLINE void render_quad2(vec2 position, vec2 dimensions, color c = Color_White){ render_quad2(position,position+dimensions.xComp(),position+dimensions.yComp(),position+dimensions,c); }
FORCE_INLINE void render_quad_thick2(vec2 position, vec2 dimensions, f32 thickness = 1, color c = Color_White){ render_quad_thick2(position,position+dimensions.xComp(),position+dimensions.yComp(),position+dimensions,thickness,c); }
FORCE_INLINE void render_quad_filled2(vec2 position, vec2 dimensions, color c = Color_White){ render_quad_filled2(position,position+dimensions.xComp(),position+dimensions.yComp(),position+dimensions,c); }
FORCE_INLINE void render_poly2(arrayT<vec2>& points, color c = Color_White){ render_poly2(points.data,points.count,c); }
FORCE_INLINE void render_poly_filled2(arrayT<vec2>& points, color c = Color_White){render_poly_filled2(points.data,points.count,c); }
StartLinkageC();
#endif //COMPILER_FEATURE_CPP

//Renders a wireframe 2D circle to the active `RenderTwodCmd` centered at `position` with `rotation` (degrees) and `radius` using `subdivisions` line segments
//    `rotation` is in degrees counter-clockwise
void render_circle2(vec2 position, f32 rotation, f32 radius, u32 subdivisions, color c);

//Renders a filled 2D circle to the active `RenderTwodCmd` centered at `position` with `rotation` and `radius` using `subdivisions` line segments
//    `rotation` is in degrees counter-clockwise
//    this does a triangle pie-fill algorithm, so it's not optimized for vertex count
void render_circle_filled2(vec2 position, f32 rotation, f32 radius, u32 subdivisions, color c);

//Renders 2D `text` to the active `RenderTwodCmd` at `position` with `font` and `scale`
void render_text2(Font* font, str8 text, vec2 position, vec2 scale, color c);

//Renders a 2D `texture` to the active `RenderTwodCmd` with `transparency` and points `top_left`, `top_right`, `bot_left`, and `bot_right`
void render_texture2(Texture* texture, vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, f32 transparency);

//Renders a flat 2D `texture` to the active `RenderTwodCmd` at `position` (top-left) with `dimensions` and `transparency` (0-1)
void render_texture_flat2(Texture* texture, vec2 position, vec2 dimensions, f32 transparency);

//Renders a 2D `texture` to the active `RenderTwodCmd` rotated by `rotation` at `center` with `dimensions` and `transparency` (0-1)
//    `rotation` is in degrees counter-clockwise
void render_texture_rotated2(Texture* texture, vec2 center, vec2 dimensions, f32 rotation, f32 transparency);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_voxel
typedef u16 RenderVoxelIndex; //NOTE(delle) changing this also requires changing defines in the backend

typedef struct RenderVoxelType{
	u32 color;
	
	//TODO(delle) voxel shape
}RenderVoxelType;

typedef struct RenderVoxel{ //8 bytes
	u16 type; //voxel type index
	u16 x;    //voxel offsets in chunk local space
	u16 y;
	u16 z;
	
	//TODO(delle) possible optimized form allowing more types and still 2^9 positions
	//u32 type;
	//u32 position; //0-9: x, 10-19: y, 20-29: z, 30-31: unused
}RenderVoxel;

//A collection of voxels that gets turned into a mesh for rendering.
typedef struct RenderVoxelChunk{
	vec3 position;
	vec3 rotation;
	u32 dimensions;
	
	b32 modified;
	b32 hidden; //NOTE(delle) temp user controlled culling
	
	Arena* arena;
	RenderVoxel** voxels;
	u64 voxel_count; //number of voxels that are not empty
	
	RenderBuffer* vertex_buffer;
	u64 vertex_count;
	RenderBuffer* index_buffer;
	u64 index_count;
}RenderVoxelChunk;

//Inits the voxel renderer using an array of voxel types `types`
//  `voxel_size` determines the size of a voxel in the world (a voxel's position is calculated with: chunk_position + voxel_size*voxel_offset)
void render_voxel_init(RenderVoxelType* types, u64 count, u32 voxel_size);

//Creates a `RenderVoxelChunk` for `voxels`
//  all voxels are transformed into global space by `position` and `rotation` (voxels describe their position in chunk local space)
//  `chunk_size` determines the number of voxels for each each dimension (cube with `chunk_size` width, height, and depth)
RenderVoxelChunk* render_voxel_create_chunk(vec3 position, vec3 rotation, u32 chunk_size, RenderVoxel* voxels, u64 voxels_count);

//Deletes the `chunk`
void render_voxel_delete_chunk(RenderVoxelChunk* chunk);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_light
//Updates the light at `idx` with `position` and `brightness`
void render_update_light(u32 idx, vec3 position, f32 brightness);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_camera
//Updates the camera's `position`
//TODO remove this since it's redundant with view matrix
void render_update_camera_position(vec3 position);

//Updates the camera's `view_matrix`
void render_update_camera_view(mat4* view_matrix);

//Updates the camera's `projection_matrix`
void render_update_camera_projection(mat4* projection_matrix);

//Updates the camera to the position (0,0) looking at (1,1) with 90 horizontal FOV, 0.1 near plane, and 1000 far plane
void render_use_default_camera();


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shaders
//Recompiles and loads the shader with `shader_type`
void render_reload_shader(u32 shader_type);

//Recompiles and loads the all shaders in the shaders folder
//TODO only reload loaded shaders
void render_reload_all_shaders();

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_other
//NOTE temporary function for vulkan shadow stuff
void render_remake_offscreen();

//displays render stats into a UI Window, this does NOT make it's own window, implemented in core_ui.cpp
void render_display_stats();

// updates a subregion of a texture
void render_update_texture(Texture* texture, vec2i offset, vec2i size);


EndLinkageC();
#define MAX_TWOD_VERTICES  0xFFFFFF // 16 777 215
#define MAX_TWOD_INDICES   3*MAX_TWOD_VERTICES
#define MAX_TWOD_CMDS      1000
#define TWOD_LAYERS        11

typedef u32 RenderTempIndex;  

extern RenderTwodIndex renderTwodVertexCount;
extern RenderTwodIndex renderTwodIndexCount;
extern Vertex2         renderTwodVertexArray[MAX_TWOD_VERTICES];
extern RenderTwodIndex renderTwodIndexArray [MAX_TWOD_INDICES];
extern RenderTwodIndex renderTwodCmdCounts[MAX_SURFACES][TWOD_LAYERS+1]; //these always start with 1
extern RenderTwodCmd   renderTwodCmdArrays[MAX_SURFACES][TWOD_LAYERS+1][MAX_TWOD_CMDS]; //different UI cmd per texture
extern RenderBookKeeper renderBookKeeperArray[MAX_TWOD_CMDS]; // keeps track of different kinds of allocations
extern u32 renderBookKeeperCount;
extern u32 renderActiveLayer;

#define MAX_TEMP_VERTICES 0xFFFFF //max u16: 65535
#define MAX_TEMP_INDICES 3*MAX_TEMP_VERTICES
extern RenderTempIndex  renderTempWireframeVertexCount;
extern RenderTempIndex  renderTempWireframeIndexCount;
extern MeshVertex       renderTempWireframeVertexArray[MAX_TEMP_VERTICES];
extern RenderTempIndex  renderTempWireframeIndexArray[MAX_TEMP_INDICES];
extern RenderTempIndex  renderTempFilledVertexCount;
extern RenderTempIndex  renderTempFilledIndexCount;
extern MeshVertex       renderTempFilledVertexArray[MAX_TEMP_VERTICES];
extern RenderTempIndex  renderTempFilledIndexArray[MAX_TEMP_INDICES];
extern RenderTempIndex  renderDebugWireframeVertexCount;
extern RenderTempIndex  renderDebugWireframeIndexCount;
extern MeshVertex       renderDebugWireframeVertexArray[MAX_TEMP_VERTICES];
extern RenderTempIndex  renderDebugWireframeIndexArray[MAX_TEMP_INDICES];
extern RenderTempIndex  renderDebugFilledVertexCount;
extern RenderTempIndex  renderDebugFilledIndexCount;
extern MeshVertex       renderDebugFilledVertexArray[MAX_TEMP_VERTICES];
extern RenderTempIndex  renderDebugFilledIndexArray[MAX_TEMP_INDICES];

#define MAX_MODEL_CMDS 10000 
extern RenderModelIndex renderModelCmdCount;
extern RenderModelCmd   renderModelCmdArray[MAX_MODEL_CMDS];

extern RenderVoxelType* render_voxel_types;
extern u64 render_voxel_types_count;
extern RenderVoxelChunk* render_voxel_chunk_pool;
extern u32 render_voxel_voxel_size; //width, height, and depth of a voxel in the world

extern RenderStats renderStats;

#define MAX_EXTERNAL_BUFFERS 6
extern RenderTwodIndex renderExternalCmdCounts[MAX_EXTERNAL_BUFFERS];
extern RenderTwodCmd   renderExternalCmdArrays[MAX_EXTERNAL_BUFFERS][MAX_TWOD_CMDS]; 

extern RenderBuffer* deshi__render_buffer_pool;

#endif //DESHI_RENDER_H
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if defined(DESHI_IMPLEMENTATION) && !defined(DESHI_RENDER_IMPL)
#define DESHI_RENDER_IMPL

#endif //DESHI_IMPLEMENTATION
