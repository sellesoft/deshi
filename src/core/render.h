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
StartLinkageC();


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_types
#define RENDER_MAX_ALLOCATIONS 4096 //NOTE(delle): Vulkan drivers are only required to support up to 4096 individual allocations

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

typedef struct Vertex2{
	vec2 pos;
	vec2 uv;
	u32  color;
}Vertex2;

typedef u32 RenderTempIndex;  //NOTE(delle) changing this also requires changing defines in the backend
typedef u32 RenderModelIndex; //NOTE(delle) changing this also requires changing defines in the backend
typedef struct RenderModelCmd{
	u32   vertexOffset;
	u32   indexOffset;
	u32   indexCount;
	u32   material;
	char* name;
	mat4  matrix;
}RenderModelCmd;

typedef u32 RenderTwodIndex;  //NOTE(delle) changing this also requires changing defines in the backend
typedef struct RenderTwodCmd{
	void*    handle; //NOTE(delle) VkDescriptorSet in vulkan, texture index in OpenGl
	Vertex2* vertexBuffer;
	u32*     indexBuffer; // pointer to used index buffer
	u64      indexOffset;
	u64      indexCount;
	vec2     scissorOffset;
	vec2     scissorExtent;
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

enum RenderShaderKind {
	RenderShaderKind_Vertex,
	// TODO(sushi) if it ever seems necessary
	// RenderShaderKind_Tesselation,
	RenderShaderKind_Geometry,
	RenderShaderKind_Fragment,
	RenderShaderKind_Compute,
};

// a shader to be compiled and used as a stage in a RenderPipeline
typedef struct RenderShader {
	RenderShaderKind kind;
	str8 name;
	str8 source;
} RenderShader;

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

enum RenderSampleCount {
	RenderSampleCount_1,
	RenderSampleCount_2,
	RenderSampleCount_4,
	RenderSampleCount_8,
	RenderSampleCount_16,
	RenderSampleCount_32,
	RenderSampleCount_64,
};

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
} RenderPipeline;

// compute pipelines seem to be distinct from graphics pipelines 
// so we'll use a separate type for them
typedef struct RenderComputePipeline {

} RenderComputePipeline;

// possibly elements of RenderPasses?
typedef struct RenderAttachment {

} RenderAttachment;

enum RenderPassKind {
	RenderPassKind_
};

// a collection of buffers and commands using those buffers
typedef struct RenderPass {

} RenderPass;

// representation of a framebuffer
typedef struct RenderFrame {

} RenderFrame;

// interface for swapchains, which to us will likely just be a collection of framebuffers 
// there's no such thing as a swapchain in opengl, but you can define multiple 
// framebuffers, so in that backend this will just serve as a collection of those
// buffers
typedef struct RenderSwapchain {
	s32 width;
	s32 height;
} RenderSwapchain;

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

//Updates the `Render` module
void render_update();

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
typedef Flags RenderMemoryPropertyFlags; enum{
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
void deshi__render_add_vertices2(u32 layer, Vertex2* vertices, u32 vCount, u32* indices, u32 iCount);
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
//// @render_make_2d
// NOTE(sushi): if you change any of the drawing algorithms do not forget to count the verticies and indices and update these functions!!
FORCE_INLINE vec2i render_make_line_counts()                  {return { 4, 6};};
FORCE_INLINE vec2i render_make_filledtriangle_counts()        {return { 3, 3};};
FORCE_INLINE vec2i render_make_triangle_counts()              {return {12,18};};
FORCE_INLINE vec2i render_make_filledrect_counts()            {return { 4, 6};};
FORCE_INLINE vec2i render_make_rect_counts()                  {return {16,24};};
FORCE_INLINE vec2i render_make_circle_counts(u32 subdiv)      {return {2*(s32)subdiv,6*(s32)subdiv};};
FORCE_INLINE vec2i render_make_filledcircle_counts(u32 subdiv){return {1+(s32)subdiv,3*(s32)subdiv};};
FORCE_INLINE vec2i render_make_text_counts(u32 charcount)     {return {4*(s32)charcount,6*(s32)charcount};};
FORCE_INLINE vec2i render_make_texture_counts()               {return { 8,24};};

vec2i render_make_line(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 start, vec2 end, f32 thickness, color color);
vec2i render_make_filledtriangle(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 p1, vec2 p2, vec2 p3, color color);
vec2i render_make_triangle(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 p0, vec2 p1, vec2 p2, f32 thickness, color color);
vec2i render_make_filledrect(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 pos, vec2 size, color color);
vec2i render_make_rect(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 pos, vec2 size, f32 thickness, color color);
vec2i render_make_circle(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 pos, f32 radius, u32 subdivisions_int, f32 thickness, color color);
vec2i render_make_filledcircle(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 pos, f32 radius, u32 subdivisions_int, color color);
vec2i render_make_text(Vertex2* putverts, u32* putindices, vec2i offsets, str8 text, Font* font, vec2 pos, color color, vec2 scale);
vec2i render_make_texture(Vertex2* putverts, u32* putindices, vec2i offsets, Texture* texture, vec2 p0, vec2 p1, vec2 p2, vec2 p3, f32 alpha, b32 flipx, b32 flipy);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_other
//NOTE temporary function for vulkan shadow stuff
void render_remake_offscreen();

//displays render stats into a UI Window, this does NOT make it's own window, implemented in core_ui.cpp
void render_display_stats();

// updates a subregion of a texture
void render_update_texture(Texture* texture, vec2i offset, vec2i size);


EndLinkageC();
#endif //DESHI_RENDER_H
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if defined(DESHI_IMPLEMENTATION) && !defined(DESHI_RENDER_IMPL)
#define DESHI_RENDER_IMPL
#include "assets.h"
#include "config.h"

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_status
local RenderStats    renderStats{};
local RenderStage    renderStage = RENDERERSTAGE_NONE;
local RenderSettings renderSettings;
local ConfigMapItem  renderConfigMap[] = {
	{str8_lit("#render settings config file"),0,0},
	
	{str8_lit("\n#    //// REQUIRES RESTART ////"),  ConfigValueType_PADSECTION,(void*)21},
	{str8_lit("debugging"),            ConfigValueType_B32, &renderSettings.debugging},
	{str8_lit("printf"),               ConfigValueType_B32, &renderSettings.printf},
	{str8_lit("texture_filtering"),    ConfigValueType_B32, &renderSettings.textureFiltering},
	{str8_lit("anistropic_filtering"), ConfigValueType_B32, &renderSettings.anistropicFiltering},
	{str8_lit("msaa_level"),           ConfigValueType_U32, &renderSettings.msaaSamples},
	{str8_lit("recompile_all_shaders"),ConfigValueType_B32, &renderSettings.recompileAllShaders},
	
	{str8_lit("\n#    //// RUNTIME VARIABLES ////"), ConfigValueType_PADSECTION,(void*)15},
	{str8_lit("logging_level"),  ConfigValueType_U32, &renderSettings.loggingLevel},
	{str8_lit("crash_on_error"), ConfigValueType_B32, &renderSettings.crashOnError},
	{str8_lit("vsync_type"),     ConfigValueType_U32, &renderSettings.vsync},
	
	{str8_lit("\n#shaders"),                         ConfigValueType_PADSECTION,(void*)17},
	{str8_lit("optimize_shaders"), ConfigValueType_B32, &renderSettings.optimizeShaders},
	
	{str8_lit("\n#shadows"),                         ConfigValueType_PADSECTION,(void*)20},
	{str8_lit("shadow_pcf"),          ConfigValueType_B32, &renderSettings.shadowPCF},
	{str8_lit("shadow_resolution"),   ConfigValueType_U32, &renderSettings.shadowResolution},
	{str8_lit("shadow_nearz"),        ConfigValueType_F32, &renderSettings.shadowNearZ},
	{str8_lit("shadow_farz"),         ConfigValueType_F32, &renderSettings.shadowFarZ},
	{str8_lit("depth_bias_constant"), ConfigValueType_F32, &renderSettings.depthBiasConstant},
	{str8_lit("depth_bias_slope"),    ConfigValueType_F32, &renderSettings.depthBiasSlope},
	{str8_lit("show_shadow_map"),     ConfigValueType_B32, &renderSettings.showShadowMap},
	
	{str8_lit("\n#colors"),                          ConfigValueType_PADSECTION,(void*)15},
	{str8_lit("clear_color"),    ConfigValueType_FV4, &renderSettings.clearColor},
	{str8_lit("selected_color"), ConfigValueType_FV4, &renderSettings.selectedColor},
	{str8_lit("collider_color"), ConfigValueType_FV4, &renderSettings.colliderColor},
	
	{str8_lit("\n#filters"),                         ConfigValueType_PADSECTION,(void*)15},
	{str8_lit("wireframe_only"), ConfigValueType_B32, &renderSettings.wireframeOnly},
	
	{str8_lit("\n#overlays"),                        ConfigValueType_PADSECTION,(void*)17},
	{str8_lit("mesh_wireframes"),  ConfigValueType_B32, &renderSettings.meshWireframes},
	{str8_lit("mesh_normals"),     ConfigValueType_B32, &renderSettings.meshNormals},
	{str8_lit("light_frustrums"),  ConfigValueType_B32, &renderSettings.lightFrustrums},
	{str8_lit("temp_mesh_on_top"), ConfigValueType_B32, &renderSettings.tempMeshOnTop},
};


void
render_save_settings(){
	config_save(str8_lit("data/cfg/render.cfg"), renderConfigMap, ArrayCount(renderConfigMap));
}

void
render_load_settings(){
	config_load(str8_lit("data/cfg/render.cfg"), renderConfigMap, ArrayCount(renderConfigMap));
}

RenderSettings*
render_get_settings(){
	return &renderSettings;
}

RenderStats*
render_get_stats(){
	return &renderStats;
}

RenderStage*
render_get_stage(){
	return &renderStage;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_buffer
local RenderBuffer* deshi__render_buffer_pool;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_surface
#define MAX_SURFACES 2
local u32 renderActiveSurface = 0;

u32
render_max_surface_count(){
	return MAX_SURFACES;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_draw_3d
#define MAX_TEMP_VERTICES 0xFFFFF //max u16: 65535
#define MAX_TEMP_INDICES 3*MAX_TEMP_VERTICES
local RenderTempIndex  renderTempWireframeVertexCount = 0;
local RenderTempIndex  renderTempWireframeIndexCount  = 0;
local MeshVertex       renderTempWireframeVertexArray[MAX_TEMP_VERTICES];
local RenderTempIndex  renderTempWireframeIndexArray[MAX_TEMP_INDICES];
local RenderTempIndex  renderTempFilledVertexCount    = 0;
local RenderTempIndex  renderTempFilledIndexCount     = 0;
local MeshVertex       renderTempFilledVertexArray[MAX_TEMP_VERTICES];
local RenderTempIndex  renderTempFilledIndexArray[MAX_TEMP_INDICES];
local RenderTempIndex  renderDebugWireframeVertexCount = 0;
local RenderTempIndex  renderDebugWireframeIndexCount  = 0;
local MeshVertex       renderDebugWireframeVertexArray[MAX_TEMP_VERTICES];
local RenderTempIndex  renderDebugWireframeIndexArray[MAX_TEMP_INDICES];
local RenderTempIndex  renderDebugFilledVertexCount    = 0;
local RenderTempIndex  renderDebugFilledIndexCount     = 0;
local MeshVertex       renderDebugFilledVertexArray[MAX_TEMP_VERTICES];
local RenderTempIndex  renderDebugFilledIndexArray[MAX_TEMP_INDICES];

#define MAX_MODEL_CMDS 10000 
local RenderModelIndex renderModelCmdCount = 0;
local RenderModelCmd   renderModelCmdArray[MAX_MODEL_CMDS];


void
render_line3(vec3 start, vec3 end, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32 rgba = c.rgba;
	MeshVertex*    vp = renderTempWireframeVertexArray + renderTempWireframeVertexCount;
	RenderTempIndex* ip = renderTempWireframeIndexArray  + renderTempWireframeIndexCount;
	
	ip[0] = renderTempWireframeVertexCount; 
	ip[1] = renderTempWireframeVertexCount+1; 
	ip[2] = renderTempWireframeVertexCount;
	vp[0].pos = start; vp[0].color = rgba;
	vp[1].pos = end;   vp[1].color = rgba;
	
	renderTempWireframeVertexCount += 2;
	renderTempWireframeIndexCount  += 3;
}

void
render_triangle3(vec3 p0, vec3 p1, vec3 p2, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32 rgba = c.rgba;
	MeshVertex*    vp = renderTempWireframeVertexArray + renderTempWireframeVertexCount;
	RenderTempIndex* ip = renderTempWireframeIndexArray  + renderTempWireframeIndexCount;
	
	ip[0] = renderTempWireframeVertexCount; 
	ip[1] = renderTempWireframeVertexCount+1; 
	ip[2] = renderTempWireframeVertexCount+2;
	vp[0].pos = p0; vp[0].color = rgba;
	vp[1].pos = p1; vp[1].color = rgba;
	vp[2].pos = p2; vp[2].color = rgba;
	
	renderTempWireframeVertexCount += 3;
	renderTempWireframeIndexCount  += 3;
}

void
render_triangle_filled3(vec3 p0, vec3 p1, vec3 p2, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32 rgba = c.rgba;
	MeshVertex*    vp = renderTempFilledVertexArray + renderTempFilledVertexCount;
	RenderTempIndex* ip = renderTempFilledIndexArray  + renderTempFilledIndexCount;
	
	ip[0] = renderTempFilledVertexCount; 
	ip[1] = renderTempFilledVertexCount+1; 
	ip[2] = renderTempFilledVertexCount+2;
	vp[0].pos = p0; vp[0].color = rgba;
	vp[1].pos = p1; vp[1].color = rgba;
	vp[2].pos = p2; vp[2].color = rgba;
	
	renderTempFilledVertexCount += 3;
	renderTempFilledIndexCount  += 3;
}

void
render_quad3(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c){DPZoneScoped;
	if(c.a == 0) return;
	render_line3(p0, p1, c);
	render_line3(p1, p2, c);
	render_line3(p2, p3, c);
	render_line3(p3, p0, c);
}

void
render_quad_filled3(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c){DPZoneScoped;
	if(c.a == 0) return;
	render_triangle_filled3(p0, p1, p2, c);
	render_triangle_filled3(p0, p2, p3, c);
}

void
render_poly3(vec3* points, u64 count, color c){DPZoneScoped;
	if(c.a == 0) return;
	if(count < 2){
		LogE("render","render_poly_filled3() was passed less than 3 points.");
		return;
	}
	
	for(s32 i=1; i < count-1; ++i) render_line3(points[i-1], points[i], c);
	render_line3(points[count-2], points[count-1], c);
}

void
render_poly_filled3(vec3* points, u64 count, color c){DPZoneScoped;
	if(c.a == 0) return;
	if(count < 3){
		LogE("render","render_poly_filled3() was passed less than 3 points.");
		return;
	}
	
	for(s32 i=2; i < count-1; ++i) render_triangle_filled3(points[i-2], points[i-1], points[i], c);
	render_triangle_filled3(points[count-3], points[count-2], points[count-1], c);
}

void
render_circle3(vec3 position, vec3 rotation, f32 radius, u32 subdivisions_int, color c){DPZoneScoped;
	if(c.a == 0) return;
	mat4 transform = mat4::TransformationMatrix(position, rotation, vec3::ONE);
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (f32(i-1)*M_2PI) / subdivisions;
		f32 a1 = (f32(i  )*M_2PI) / subdivisions;
		f32 x0 = radius*cosf(a0); f32 x1 = radius*cosf(a1);
		f32 y0 = radius*sinf(a0); f32 y1 = radius*sinf(a1);
		vec3 xaxis0 = vec3{0, y0, x0} * transform; vec3 xaxis1 = vec3{0, y1, x1} * transform;
		render_line3(xaxis0, xaxis1, c);
	}
}

void
render_box(mat4* transform, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	vec3 p{0.5f, 0.5f, 0.5f};
	vec3 points[8] = {
		{-p.x, p.y, p.z},{-p.x,-p.y, p.z},{-p.x, p.y,-p.z},{-p.x,-p.y,-p.z},
		{ p.x, p.y, p.z},{ p.x,-p.y, p.z},{ p.x, p.y,-p.z},{ p.x,-p.y,-p.z},
	};
	forI(8){ points[i] += vec3::ONE*0.5; points[i] = points[i] * (*transform); }
	render_line3(points[3], points[1], c); render_line3(points[3], points[2], c); render_line3(points[3], points[7], c);
	render_line3(points[0], points[1], c); render_line3(points[0], points[2], c); render_line3(points[0], points[4], c);
	render_line3(points[5], points[1], c); render_line3(points[5], points[4], c); render_line3(points[5], points[7], c);
	render_line3(points[6], points[2], c); render_line3(points[6], points[4], c); render_line3(points[6], points[7], c);
}

void
render_box_filled(mat4* transform, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	vec3 p{0.5f, 0.5f, 0.5f};
	vec3 points[8] = {
		{-p.x, p.y, p.z},{-p.x,-p.y, p.z},{-p.x, p.y,-p.z},{-p.x,-p.y,-p.z},
		{ p.x, p.y, p.z},{ p.x,-p.y, p.z},{ p.x, p.y,-p.z},{ p.x,-p.y,-p.z},
	};
	forI(8){ points[i] += vec3::ONE*0.5; points[i] = points[i] * (*transform); }
	render_triangle_filled3(points[4], points[2], points[0], c); render_triangle_filled3(points[4], points[6], points[2], c);
	render_triangle_filled3(points[2], points[7], points[3], c); render_triangle_filled3(points[2], points[6], points[7], c);
	render_triangle_filled3(points[6], points[5], points[7], c); render_triangle_filled3(points[6], points[4], points[5], c);
	render_triangle_filled3(points[1], points[7], points[5], c); render_triangle_filled3(points[1], points[3], points[7], c);
	render_triangle_filled3(points[0], points[3], points[1], c); render_triangle_filled3(points[0], points[2], points[3], c);
	render_triangle_filled3(points[4], points[1], points[5], c); render_triangle_filled3(points[4], points[0], points[1], c);
}

void
render_sphere(vec3 position, vec3 rotation, f32 radius, u32 subdivisions_int, color cx, color cy, color cz){DPZoneScoped;
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
		render_line3(xaxis0, xaxis1, cx); render_line3(yaxis0, yaxis1, cy); render_line3(zaxis0, zaxis1, cz);
	}
}

void
render_frustrum(vec3 position, vec3 target, f32 aspectRatio, f32 fovx, f32 nearZ, f32 farZ, color c){DPZoneScoped;
	if(c.a == 0) return;
	
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
	
	render_line3(v[0], v[1], c);
	render_line3(v[0], v[2], c);
	render_line3(v[3], v[1], c);
	render_line3(v[3], v[2], c);
	render_line3(v[4], v[5], c);
	render_line3(v[4], v[6], c);
	render_line3(v[7], v[5], c);
	render_line3(v[7], v[6], c);
	render_line3(v[0], v[4], c);
	render_line3(v[1], v[5], c);
	render_line3(v[2], v[6], c);
	render_line3(v[3], v[7], c);
}

void
render_clear_debug(){DPZoneScoped;
	renderDebugWireframeVertexCount = 0;
	renderDebugWireframeIndexCount  = 0;
	renderDebugFilledVertexCount = 0;
	renderDebugFilledIndexCount  = 0;
}

void
render_debug_line3(vec3 start, vec3 end, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32            color = c.rgba;
	MeshVertex*     vp = renderDebugWireframeVertexArray + renderDebugWireframeVertexCount;
	RenderTempIndex*  ip = renderDebugWireframeIndexArray + renderDebugWireframeIndexCount;
	
	ip[0] = renderDebugWireframeVertexCount; 
	ip[1] = renderDebugWireframeVertexCount+1; 
	ip[2] = renderDebugWireframeVertexCount;
	vp[0].pos = start; vp[0].color = color;
	vp[1].pos = end;   vp[1].color = color;
	
	renderDebugWireframeVertexCount += 2;
	renderDebugWireframeIndexCount  += 3;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_draw_2d
#define MAX_TWOD_VERTICES  0xFFFFFF // 16 777 215
#define MAX_TWOD_INDICES   3*MAX_TWOD_VERTICES
#define MAX_TWOD_CMDS      1000
#define TWOD_LAYERS        11
local RenderTwodIndex renderTwodVertexCount = 0;
local RenderTwodIndex renderTwodIndexCount  = 0;
local Vertex2         renderTwodVertexArray[MAX_TWOD_VERTICES];
local RenderTwodIndex renderTwodIndexArray [MAX_TWOD_INDICES];
local RenderTwodIndex renderTwodCmdCounts[MAX_SURFACES][TWOD_LAYERS+1]; //these always start with 1
local RenderTwodCmd   renderTwodCmdArrays[MAX_SURFACES][TWOD_LAYERS+1][MAX_TWOD_CMDS]; //different UI cmd per texture
local RenderBookKeeper renderBookKeeperArray[MAX_TWOD_CMDS]; // keeps track of different kinds of allocations
local u32 renderBookKeeperCount = 0;
local u32 renderActiveLayer = 5;

//external buffers
#define MAX_EXTERNAL_BUFFERS 6
local RenderTwodIndex renderExternalCmdCounts[MAX_EXTERNAL_BUFFERS];
local RenderTwodCmd   renderExternalCmdArrays[MAX_EXTERNAL_BUFFERS][MAX_TWOD_CMDS]; 

void
deshi__render_add_vertices2(str8 file, u32 line, u32 layer, Vertex2* vertices, u32 vCount, u32* indices, u32 iCount) {DPZoneScoped;
	Assert(vCount + renderTwodVertexCount < MAX_TWOD_VERTICES);
	Assert(iCount + renderTwodIndexCount  < MAX_TWOD_INDICES);
	
	Vertex2* vp         = renderTwodVertexArray + renderTwodVertexCount;
	RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
	
	
	
	CopyMemory(vp, vertices, vCount*sizeof(Vertex2));
	forI(iCount){
		Assert(indices[i] < vCount, "Index out of range of given number of vertices!\nMake sure your indices weren't overwritten by something.");
		ip[i] = renderTwodVertexCount + indices[i];
	} 
	
#ifdef BUILD_INTERNAL
	RenderBookKeeper keeper;
	keeper.type = RenderBookKeeper_Vertex;
	keeper.vertex.start = vp;
	keeper.vertex.count = vCount;
	keeper.file = file;
	keeper.line = line;
	renderBookKeeperArray[renderBookKeeperCount++] = keeper;
	
	keeper.type = RenderBookKeeper_Index;
	keeper.index.start = ip;
	keeper.index.count = iCount;
	keeper.file = file;
	keeper.line = line;
	renderBookKeeperArray[renderBookKeeperCount++] = keeper;
#endif
	
	renderTwodVertexCount += vCount;
	renderTwodIndexCount  += iCount;
	renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer] - 1].indexCount += iCount;
	
	
}

u32
render_top_layer_index(){DPZoneScoped;
	return TWOD_LAYERS-1;
}

u32
render_decoration_layer_index(){DPZoneScoped;
	return TWOD_LAYERS;
}

u32
render_active_layer(){DPZoneScoped;
	return renderActiveLayer;
}

void
render_line2(vec2 start, vec2 end, color c){DPZoneScoped;
	render_line_thick2(start, end, 1, c);
}

void
render_line_thick2(vec2 start, vec2 end, f32 thickness, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32           color = c.rgba;
	Vertex2*         vp = renderTwodVertexArray + renderTwodVertexCount;
	RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
	
	vec2 ott = end - start;
	vec2 norm = Vec2(ott.y, -ott.x).normalized();
	
	ip[0] = renderTwodVertexCount; ip[1] = renderTwodVertexCount + 1; ip[2] = renderTwodVertexCount + 2;
	ip[3] = renderTwodVertexCount; ip[4] = renderTwodVertexCount + 2; ip[5] = renderTwodVertexCount + 3;
	vp[0].pos = { start.x,start.y }; vp[0].uv = { 0,0 }; vp[0].color = color;
	vp[1].pos = { end.x,  end.y   }; vp[1].uv = { 0,0 }; vp[1].color = color;
	vp[2].pos = { end.x,  end.y   }; vp[2].uv = { 0,0 }; vp[2].color = color;
	vp[3].pos = { start.x,start.y }; vp[3].uv = { 0,0 }; vp[3].color = color;
	
	vp[0].pos += norm * thickness / 2.f;
	vp[1].pos += norm * thickness / 2.f;
	vp[2].pos -= norm * thickness / 2.f;
	vp[3].pos -= norm * thickness / 2.f;
	
	renderTwodVertexCount += 4;
	renderTwodIndexCount  += 6;
	renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].indexCount += 6;
}

//TODO(sushi) this function needs to be made more robust as well as cleaned up. currently, if 2 line segments form a
// small enough angle, the thickness stop being preserved. this funciton also needs to be moved out to suugu and
// replaced by a more general render function that allows you to manipulate the vertex/index arrays
void
render_lines2(vec2* points, u64 count, f32 thickness, color c){DPZoneScoped;
	if(count < 2){
		LogE("render","render_lines2() was passed less than 2 points.");
		return;
	}
	if(c.a == 0 || thickness == 0) return;
	
	
	u32           color = c.rgba;
	Vertex2*         vp = renderTwodVertexArray + renderTwodVertexCount;
	RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
	
	f32 halfthick = thickness / 2.f;
	{// first point
		
		vec2 ott = points[1] - points[0];
		vec2 norm = Vec2(ott.y, -ott.x).normalized();
		
		vp[0].pos = points[0] + norm * halfthick; vp[0].uv = { 0,0 }; vp[0].color = color;
		vp[1].pos = points[0] - norm * halfthick; vp[1].uv = { 0,0 }; vp[1].color = color;
		
		ip[0] = renderTwodVertexCount;
		ip[1] = renderTwodVertexCount + 1;
		ip[3] = renderTwodVertexCount;
		
		renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].indexCount += 3;
		
		renderTwodVertexCount += 2;
		renderTwodIndexCount  += 3;
		vp += 2;
	}
	
	//in betweens
	s32 flip = -1;
	for(s32 i = 1; i < count - 1; i++, flip *= -1){
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
			renderTwodVertexCount;
		
		ip[ipidx + 3] =
			ip[ipidx + 5] =
			renderTwodVertexCount + 1;
		
		vp[0].pos = curr + normavout; vp[0].uv = { 0,0 }; vp[0].color = color;//PackColorU32(255, 0, 0, 255);
		vp[1].pos = curr + normavin;  vp[1].uv = { 0,0 }; vp[1].color = color;//PackColorU32(255, 0, 255, 255);
		
		renderTwodVertexCount += 2;
		renderTwodIndexCount  += 6;
		vp += 2;
		
		renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].indexCount += 6;
		
	}
	
	{//last point
		vec2 ott = points[count - 1] - points[count - 2];
		vec2 norm = Vec2(ott.y, -ott.x).normalized() * (f32)flip;
		
		vp[0].pos = points[count - 1] + norm * halfthick; vp[0].uv = { 0,0 }; vp[0].color = color;//PackColorU32(255, 50, 255, 255);
		vp[1].pos = points[count - 1] - norm * halfthick; vp[1].uv = { 0,0 }; vp[1].color = color;//PackColorU32(255, 50, 100, 255);
		
		//set final indicies by pattern
		s32 ipidx = 6 * (count - 2) + 2;
		ip[ipidx + 0] = renderTwodVertexCount;
		ip[ipidx + 2] = renderTwodVertexCount;
		ip[ipidx + 3] = renderTwodVertexCount + 1;
		
		renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].indexCount += 3;
		
		renderTwodVertexCount += 2;
		renderTwodIndexCount  += 3;
		vp += 2; ip += 3;
	}
}

void
render_triangle2(vec2 p1, vec2 p2, vec2 p3, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	render_line2(p1, p2, c);
	render_line2(p2, p3, c);
	render_line2(p3, p1, c);
}

void
render_triangle_filled2(vec2 p1, vec2 p2, vec2 p3, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32           color = c.rgba;
	Vertex2*         vp = renderTwodVertexArray + renderTwodVertexCount;
	RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
	
	ip[0] = renderTwodVertexCount; ip[1] = renderTwodVertexCount + 1; ip[2] = renderTwodVertexCount + 2;
	vp[0].pos = p1; vp[0].uv = { 0,0 }; vp[0].color = color;
	vp[1].pos = p2; vp[1].uv = { 0,0 }; vp[1].color = color;
	vp[2].pos = p3; vp[2].uv = { 0,0 }; vp[2].color = color;
	
	renderTwodVertexCount += 3;
	renderTwodIndexCount  += 3;
	renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].indexCount += 3;
}

void
render_quad2(vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	render_line2(top_left,  top_right, c);
	render_line2(top_right, bot_right, c);
	render_line2(bot_right, bot_left,  c);
	render_line2(bot_left,  top_left,  c);
}

void
render_quad_thick2(vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, f32 thickness, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	render_line_thick2(top_left,  top_right, thickness, c);
	render_line_thick2(top_right, bot_right, thickness, c);
	render_line_thick2(bot_right, bot_left,  thickness, c);
	render_line_thick2(bot_left,  top_left,  thickness, c);
}

void
render_quad_filled2(vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32           color = c.rgba;
	Vertex2*         vp = renderTwodVertexArray + renderTwodVertexCount;
	RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
	
	ip[0] = renderTwodVertexCount; ip[1] = renderTwodVertexCount + 1; ip[2] = renderTwodVertexCount + 2;
	ip[3] = renderTwodVertexCount; ip[4] = renderTwodVertexCount + 2; ip[5] = renderTwodVertexCount + 3;
	vp[0].pos = top_left;  vp[0].uv = { 0,0 }; vp[0].color = color;
	vp[1].pos = top_right; vp[1].uv = { 0,0 }; vp[1].color = color;
	vp[2].pos = bot_right; vp[2].uv = { 0,0 }; vp[2].color = color;
	vp[3].pos = bot_left;  vp[3].uv = { 0,0 }; vp[3].color = color;
	
	renderTwodVertexCount += 4;
	renderTwodIndexCount  += 6;
	renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].indexCount += 6;
}

void
render_circle2(vec2 pos, f32 rotation, f32 radius, u32 subdivisions_int, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	f32 rot_rad = Radians(rotation);
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (((f32)(i-1) * M_2PI) / subdivisions) + rot_rad;
		f32 a1 = (((f32)(i-0) * M_2PI) / subdivisions) + rot_rad;
		f32 x0 = pos.x + radius * cosf(a0); f32 x1 = pos.x + radius * cosf(a1);
		f32 y0 = pos.y + radius * sinf(a0); f32 y1 = pos.y + radius * sinf(a1);
		render_line2(Vec2(x0, y0), Vec2(x1, y1), c);
	}
}

void
render_circle_filled2(vec2 pos, f32 rotation, f32 radius, u32 subdivisions_int, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	f32 rot_rad = Radians(rotation);
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (((f32)(i-1) * M_2PI) / subdivisions) + rot_rad;
		f32 a1 = (((f32)(i-0) * M_2PI) / subdivisions) + rot_rad;
		f32 x0 = pos.x + radius * cosf(a0); f32 x1 = pos.x + radius * cosf(a1);
		f32 y0 = pos.y + radius * sinf(a0); f32 y1 = pos.y + radius * sinf(a1);
		render_triangle_filled2(pos, Vec2(x0, y0), Vec2(x1, y1), c);
	}
}

void
render_text2(Font* font, str8 text, vec2 pos, vec2 scale, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	switch(font->type){
		//// BDF (and NULL) font rendering ////
		case FontType_BDF: case FontType_NONE:{
			while(text){
				DecodedCodepoint decoded = str8_advance(&text);
				if(decoded.codepoint == 0) break;
				
				u32           color = c.rgba;
				Vertex2*         vp = renderTwodVertexArray + renderTwodVertexCount;
				RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
				
				f32 w  = font->max_width  * scale.x;
				f32 h  = font->max_height * scale.y;
				f32 dy = 1.f / (f32)font->count;
				
				f32 idx    = (f32)(decoded.codepoint - 32);
				f32 topoff = (idx * dy) + font->uv_yoffset;
				f32 botoff = topoff + dy;
				
				ip[0] = renderTwodVertexCount; ip[1] = renderTwodVertexCount + 1; ip[2] = renderTwodVertexCount + 2;
				ip[3] = renderTwodVertexCount; ip[4] = renderTwodVertexCount + 2; ip[5] = renderTwodVertexCount + 3;
				vp[0].pos = { pos.x + 0,pos.y + 0 }; vp[0].uv = { 0,topoff }; vp[0].color = color; //top left
				vp[1].pos = { pos.x + w,pos.y + 0 }; vp[1].uv = { 1,topoff }; vp[1].color = color; //top right
				vp[2].pos = { pos.x + w,pos.y + h }; vp[2].uv = { 1,botoff }; vp[2].color = color; //bot right
				vp[3].pos = { pos.x + 0,pos.y + h }; vp[3].uv = { 0,botoff }; vp[3].color = color; //bot left
				
				renderTwodVertexCount += 4;
				renderTwodIndexCount  += 6;
				renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer]].indexCount += 6;
				pos.x += font->max_width * scale.x;
			}
		}break;
		//// TTF font rendering ////
		case FontType_TTF:{
			while(text){
				DecodedCodepoint decoded = str8_advance(&text);
				if(decoded.codepoint == 0) break;
				
				u32           color = c.rgba;
				Vertex2*         vp = renderTwodVertexArray + renderTwodVertexCount;
				RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
				FontAlignedQuad   q = font_aligned_quad(font, decoded.codepoint, &pos, scale);
				
				ip[0] = renderTwodVertexCount; ip[1] = renderTwodVertexCount + 1; ip[2] = renderTwodVertexCount + 2;
				ip[3] = renderTwodVertexCount; ip[4] = renderTwodVertexCount + 2; ip[5] = renderTwodVertexCount + 3;
				vp[0].pos = { q.x0,q.y0 }; vp[0].uv = { q.u0,q.v0 }; vp[0].color = color; //top left
				vp[1].pos = { q.x1,q.y0 }; vp[1].uv = { q.u1,q.v0 }; vp[1].color = color; //top right
				vp[2].pos = { q.x1,q.y1 }; vp[2].uv = { q.u1,q.v1 }; vp[2].color = color; //bot right
				vp[3].pos = { q.x0,q.y1 }; vp[3].uv = { q.u0,q.v1 }; vp[3].color = color; //bot left
				
				renderTwodVertexCount += 4;
				renderTwodIndexCount  += 6;
				renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].indexCount += 6;
			}break;
			default: Assert(!"unhandled font type"); break;
		}
	}
}

void
render_texture2(Texture* texture, vec2 tl, vec2 tr, vec2 bl, vec2 br, f32 transparency){DPZoneScoped;
	if(transparency == 0) return;
	
	u32           color = PackColorU32(255, 255, 255, (u8)(255.f*transparency));
	Vertex2*         vp = renderTwodVertexArray + renderTwodVertexCount;
	RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
	
	ip[0] = renderTwodVertexCount; ip[1] = renderTwodVertexCount + 1; ip[2] = renderTwodVertexCount + 2;
	ip[3] = renderTwodVertexCount; ip[4] = renderTwodVertexCount + 2; ip[5] = renderTwodVertexCount + 3;
	vp[0].pos = tl; vp[0].uv = { 0,1 }; vp[0].color = color;
	vp[1].pos = tr; vp[1].uv = { 1,1 }; vp[1].color = color;
	vp[2].pos = br; vp[2].uv = { 1,0 }; vp[2].color = color;
	vp[3].pos = bl; vp[3].uv = { 0,0 }; vp[3].color = color;
	
	renderTwodVertexCount += 4;
	renderTwodIndexCount  += 6;
	
	RenderTwodCmd* cmd =  &renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer]];
	cmd->indexCount += 6;
	cmd->handle = (void*)((u64)texture->render_idx);
	cmd->scissorExtent = DeshWindow->dimensions.toVec2();
	cmd->scissorOffset = Vec2(0,0);
	renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] += 1;
}

void
render_texture_flat2(Texture* texture, vec2 pos, vec2 dimensions, f32 transparency){DPZoneScoped;
	if(transparency == 0) return;
	
	vec2 tl = pos,
	tr = vec2{pos.x + dimensions.x, pos.y},
	bl = vec2{pos.x,                pos.y + dimensions.y},
	br = vec2{pos.x + dimensions.x, pos.y + dimensions.y};
	render_texture2(texture, tl, tr, bl, br, transparency);
}

void
render_texture_rotated2(Texture* texture, vec2 center, vec2 dimensions, f32 rotation, f32 transparency){DPZoneScoped;
	if(transparency == 0) return;
	
	vec2 half_dims = dimensions / 2.f,
	tl = Math::vec2RotateByAngle(rotation, center - half_dims),
	tr = Math::vec2RotateByAngle(rotation, center + half_dims.yInvert()),
	bl = Math::vec2RotateByAngle(rotation, center + half_dims.xInvert()),
	br = Math::vec2RotateByAngle(rotation, center + half_dims);
	render_texture2(texture, tl, tr, bl, br, transparency);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_make_2d
//4 verts, 6 indices
vec2i
render_make_line(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 start, vec2 end, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	vec2 ott = end - start;
	vec2 norm = Vec2(ott.y, -ott.x).normalized();
	
	ip[0] = offsets.x; ip[1] = offsets.x + 1; ip[2] = offsets.x + 2;
	ip[3] = offsets.x; ip[4] = offsets.x + 2; ip[5] = offsets.x + 3;
	vp[0].pos = { start.x,start.y }; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = { end.x,    end.y }; vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = { end.x,    end.y }; vp[2].uv = { 0,0 }; vp[2].color = col;
	vp[3].pos = { start.x,start.y }; vp[3].uv = { 0,0 }; vp[3].color = col;
	
	vp[0].pos += norm * thickness / 2.f;
	vp[1].pos += norm * thickness / 2.f;
	vp[2].pos -= norm * thickness / 2.f;
	vp[3].pos -= norm * thickness / 2.f;
	
	return render_make_line_counts();
}

//3 verts, 3 indices
vec2i 
render_make_filledtriangle(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 p1, vec2 p2, vec2 p3, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	ip[0] = offsets.x; ip[1] = offsets.x + 1; ip[2] = offsets.x + 2;
	vp[0].pos = p1; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = p2; vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = p3; vp[2].uv = { 0,0 }; vp[2].color = col;
	
	return render_make_filledtriangle_counts();
}

vec2i
render_make_triangle(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 p0, vec2 p1, vec2 p2, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	vec2i sum;
	sum += render_make_line(vp, ip, {    0,    0}, p0, p1, 1, color);
	sum += render_make_line(vp, ip, {sum.x,sum.y}, p1, p2, 1, color);
	sum += render_make_line(vp, ip, {sum.x,sum.y}, p2, p0, 1, color);
	
	return sum;
	
	//TODO(sushi) this should be fixed to replace reliance on MakeLine
	//ip[0]  = offsets.y + 0; ip[1]  = offsets.y + 1; ip[2]  = offsets.y + 3;
	//ip[3]  = offsets.y + 0; ip[4]  = offsets.y + 3; ip[5]  = offsets.y + 2;
	//ip[6]  = offsets.y + 2; ip[7]  = offsets.y + 3; ip[8]  = offsets.y + 5;
	//ip[9]  = offsets.y + 2; ip[10] = offsets.y + 5; ip[11] = offsets.y + 4;
	//ip[12] = offsets.y + 4; ip[13] = offsets.y + 5; ip[14] = offsets.y + 1;
	//ip[15] = offsets.y + 4; ip[16] = offsets.y + 1; ip[17] = offsets.y + 0;
	//
	//f32 ang1 = Math::AngBetweenVectors(p1 - p0, p2 - p0)/2;
	//f32 ang2 = Math::AngBetweenVectors(p0 - p1, p2 - p1)/2;
	//f32 ang3 = Math::AngBetweenVectors(p1 - p2, p0 - p2)/2;
	//
	//vec2 p0offset = (Math::vec2RotateByAngle(-ang1, p2 - p0).normalized() * thickness / (2 * sinf(Radians(ang1)))).clampedMag(0, thickness * 2);
	//vec2 p1offset = (Math::vec2RotateByAngle(-ang2, p2 - p1).normalized() * thickness / (2 * sinf(Radians(ang2)))).clampedMag(0, thickness * 2);
	//vec2 p2offset = (Math::vec2RotateByAngle(-ang3, p0 - p2).normalized() * thickness / (2 * sinf(Radians(ang3)))).clampedMag(0, thickness * 2);
	//       
	//vp[0].pos = p0 - p0offset; vp[0].uv = { 0,0 }; vp[0].color = col;
	//vp[1].pos = p0 + p0offset; vp[1].uv = { 0,0 }; vp[1].color = col;
	//vp[2].pos = p1 + p1offset; vp[2].uv = { 0,0 }; vp[2].color = col;
	//vp[3].pos = p1 - p1offset; vp[3].uv = { 0,0 }; vp[3].color = col;
	//vp[4].pos = p2 + p2offset; vp[4].uv = { 0,0 }; vp[4].color = col;
	//vp[5].pos = p2 - p2offset; vp[5].uv = { 0,0 }; vp[5].color = col;
	
	//return vec3(6, 18);
}

vec2i
render_make_filledrect(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 pos, vec2 size, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	vec2 tl = pos;
	vec2 br = pos + size;
	vec2 bl = pos + Vec2(0, size.y);
	vec2 tr = pos + Vec2(size.x, 0);
	
	ip[0] = offsets.x; ip[1] = offsets.x + 1; ip[2] = offsets.x + 2;
	ip[3] = offsets.x; ip[4] = offsets.x + 2; ip[5] = offsets.x + 3;
	vp[0].pos = tl; vp[0].uv = { 0,1 }; vp[0].color = col;
	vp[1].pos = tr; vp[1].uv = { 1,1 }; vp[1].color = col;
	vp[2].pos = br; vp[2].uv = { 1,0 }; vp[2].color = col;
	vp[3].pos = bl; vp[3].uv = { 0,0 }; vp[3].color = col;
	
	return render_make_filledrect_counts();
}

vec2i
render_make_rect(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 pos, vec2 size, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	// vec2 tl = pos;
	// vec2 br = pos + size;
	// vec2 bl = pos + Vec2(0, size.y);
	// vec2 tr = pos + Vec2(size.x, 0);
	
	// vec2i sum = {0};
	// sum += render_make_line(vp, ip, sum, tl,tr,thickness,color);
	// sum += render_make_line(vp, ip, sum, tr,br,thickness,color);
	// sum += render_make_line(vp, ip, sum, br,bl,thickness,color);
	// sum += render_make_line(vp, ip, sum, bl,tl,thickness,color);
	
	//TODO(sushi) test this
	vec2 tl = pos;
	vec2 br = pos + size;
	vec2 bl = vec2{br.x, tl.y};
	vec2 tr = vec2{tl.x, br.y};
	f32 t = thickness; u32 v = offsets.x;
	ip[ 0] = v+0; ip[ 1] = v+1; ip[ 2] = v+3; 
	ip[ 3] = v+0; ip[ 4] = v+3; ip[ 5] = v+2; 
	ip[ 6] = v+2; ip[ 7] = v+3; ip[ 8] = v+5; 
	ip[ 9] = v+2; ip[10] = v+5; ip[11] = v+4; 
	ip[12] = v+4; ip[13] = v+5; ip[14] = v+7; 
	ip[15] = v+4; ip[16] = v+7; ip[17] = v+6; 
	ip[18] = v+6; ip[19] = v+7; ip[20] = v+1; 
	ip[21] = v+6; ip[22] = v+1; ip[23] = v+0;
	vp[0].pos = tl;             vp[0].uv = {0,0}; vp[0].color = color.rgba;
	vp[1].pos = tl+Vec2( t, t); vp[1].uv = {0,0}; vp[1].color = color.rgba;
	vp[2].pos = tr;             vp[2].uv = {0,0}; vp[2].color = color.rgba;
	vp[3].pos = tr+Vec2(-t, t); vp[3].uv = {0,0}; vp[3].color = color.rgba;
	vp[4].pos = br;             vp[4].uv = {0,0}; vp[4].color = color.rgba;
	vp[5].pos = br+Vec2(-t,-t); vp[5].uv = {0,0}; vp[5].color = color.rgba;
	vp[6].pos = bl;             vp[6].uv = {0,0}; vp[6].color = color.rgba;
	vp[7].pos = bl+Vec2( t,-t); vp[7].uv = {0,0}; vp[7].color = color.rgba;
	
	return render_make_rect_counts();
}

vec2i
render_make_circle(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 pos, f32 radius, u32 subdivisions_int, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	f32 subdivisions = f32(subdivisions_int);
	u32 nuindexes = subdivisions * 6;
	
	//first and last point
	vec2 last = pos + Vec2(radius, 0);
	vp[0].pos = last + Vec2(-thickness / 2, 0); vp[0].uv={0,0}; vp[0].color=col;
	vp[1].pos = last + Vec2( thickness / 2, 0); vp[1].uv={0,0}; vp[1].color=col;
	ip[0] = offsets.x + 0; ip[1] = offsets.x + 1; ip[3] = offsets.x + 0;
	ip[nuindexes - 1] = offsets.x + 0; ip[nuindexes - 2] = ip[nuindexes - 4] = offsets.x + 1;
	
	for(s32 i = 1; i < subdivisions_int; i++){
		f32 a1 = (f32(i) * M_2PI) / subdivisions;
		vec2 offset = Vec2(radius * cosf(a1), radius * sinf(a1));
		vec2 point = pos + offset;
		
		u32 idx = i * 2;
		vp[idx].pos = point - offset.normalized() * thickness / 2; vp[idx].uv = { 0,0 }; vp[idx].color = col;
		vp[idx + 1].pos = point + offset.normalized() * thickness / 2; vp[idx + 1].uv = { 0,0 }; vp[idx + 1 ].color = col;
		
		u32 ipidx1 = 6 * (i - 1) + 2;
		u32 ipidx2 = 6 * i - 1;
		ip[ipidx1] = ip[ipidx1 + 2] = ip[ipidx1 + 5] = offsets.x + idx + 1;
		ip[ipidx2] = ip[ipidx2 + 1] = ip[ipidx2 + 4] = offsets.x + idx;
	}
	
	return render_make_circle_counts(subdivisions_int);
}

vec2i 
render_make_filledcircle(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 pos, f32 radius, u32 subdivisions_int, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	vp[0].pos = pos; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = pos + Vec2(radius, 0); vp[1].uv = { 0,0 }; vp[1].color = col;
	u32 nuindexes = 3 * subdivisions_int;
	
	ip[1] = offsets.x + 1;
	for(s32 i = 0; i < nuindexes; i += 3) ip[i] = offsets.x;
	
	ip[nuindexes - 1] = offsets.x + 1;
	
	vec2 sum;
	f32 subdivisions = f32(subdivisions_int);
	for(u32 i = 1; i < subdivisions_int; i++){
		f32 a1 = (f32(i) * M_2PI) / subdivisions;
		vec2 offset = Vec2(radius * cosf(a1), radius * sinf(a1));
		vec2 point = pos + offset;
		
		vp[i+1].pos = point; vp[i+1].uv = { 0,0 }; vp[i+1].color = col;
		
		u32 ipidx = 3 * i - 1;
		ip[ipidx] = ip[ipidx + 2] = offsets.x + i + 1;
	}
	
	return render_make_filledcircle_counts(subdivisions_int);
}

vec2i
render_make_text(Vertex2* putverts, u32* putindices, vec2i offsets, str8 text, Font* font, vec2 pos, color color, vec2 scale){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};

	vec2i sum={0};
	switch (font->type){
		//// BDF (and NULL) font rendering ////
		case FontType_BDF: case FontType_NONE:{
			u32 codepoint;
			str8 remaining = text;
			u32 i = 0;
			while(remaining && (codepoint = str8_advance(&remaining).codepoint)){
				u32     col = color.rgba;
				Vertex2* vp = putverts + offsets.x + 4 * i;
				u32*     ip = putindices + offsets.y + 6 * i;
				
				f32 w = font->max_width * scale.x;
				f32 h = font->max_height * scale.y;
				f32 dy = 1.f / (f32)font->count;
				
				f32 idx = f32(codepoint - 32);
				f32 topoff = (idx * dy) + font->uv_yoffset;
				f32 botoff = topoff + dy;
				
				ip[0] = offsets.x+4*i; ip[1] = offsets.x+4*i + 1; ip[2] = offsets.x+4*i + 2;
				ip[3] = offsets.x+4*i; ip[4] = offsets.x+4*i + 2; ip[5] = offsets.x+4*i + 3;
				vp[0].pos = { pos.x + 0,pos.y + 0 }; vp[0].uv = { 0,topoff }; vp[0].color = col; //top left
				vp[1].pos = { pos.x + w,pos.y + 0 }; vp[1].uv = { 1,topoff }; vp[1].color = col; //top right
				vp[2].pos = { pos.x + w,pos.y + h }; vp[2].uv = { 1,botoff }; vp[2].color = col; //bot right
				vp[3].pos = { pos.x + 0,pos.y + h }; vp[3].uv = { 0,botoff }; vp[3].color = col; //bot left
				pos.x += font->max_width * scale.x;
				i += 1;
				sum+=render_make_text_counts(1);
			}
		}break;
		//// TTF font rendering ////
		case FontType_TTF:{
			u32 codepoint;
			str8 remaining = text;
			u32 i = 0;
			while(remaining && (codepoint = str8_advance(&remaining).codepoint)){
				u32     col = color.rgba;
				Vertex2* vp = putverts + offsets.x + 4 * i;
				u32*     ip = putindices + offsets.y + 6 * i;
				FontAlignedQuad q = font_aligned_quad(font, codepoint, &pos, scale);
				
				ip[0] = offsets.x+4*i; ip[1] = offsets.x+4*i + 1; ip[2] = offsets.x+4*i + 2;
				ip[3] = offsets.x+4*i; ip[4] = offsets.x+4*i + 2; ip[5] = offsets.x+4*i + 3;
				vp[0].pos = { q.x0,q.y0 }; vp[0].uv = { q.u0,q.v0 }; vp[0].color = col; //top left
				vp[1].pos = { q.x1,q.y0 }; vp[1].uv = { q.u1,q.v0 }; vp[1].color = col; //top right
				vp[2].pos = { q.x1,q.y1 }; vp[2].uv = { q.u1,q.v1 }; vp[2].color = col; //bot right
				vp[3].pos = { q.x0,q.y1 }; vp[3].uv = { q.u0,q.v1 }; vp[3].color = col; //bot left
				i += 1;
				sum+=render_make_text_counts(1);
			}
		}break;
		default: Assert(!"unhandled font type"); break;
	}
	return sum;
}

vec2i 
render_make_texture(Vertex2* putverts, u32* putindices, vec2i offsets, Texture* texture, vec2 p0, vec2 p1, vec2 p2, vec2 p3, f32 alpha, b32 flipx = 0, b32 flipy = 0){DPZoneScoped;
	Assert(putverts && putindices);
	if(!alpha) return{0,0};
	
	u32     col = PackColorU32(255,255,255,255.f * alpha);
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	ip[0] = offsets.x; ip[1] = offsets.x + 1; ip[2] = offsets.x + 2;
	ip[3] = offsets.x; ip[4] = offsets.x + 2; ip[5] = offsets.x + 3;
	vp[0].pos = p0; vp[0].uv = { 0,1 }; vp[0].color = col;
	vp[1].pos = p1; vp[1].uv = { 1,1 }; vp[1].color = col;
	vp[2].pos = p2; vp[2].uv = { 1,0 }; vp[2].color = col;
	vp[3].pos = p3; vp[3].uv = { 0,0 }; vp[3].color = col;
	
	if(flipx){
		vec2 u0 = vp[0].uv, u1 = vp[1].uv, u2 = vp[2].uv, u3 = vp[3].uv;
		vp[0].uv = u1; vp[1].uv = u0; vp[2].uv = u3; vp[3].uv = u2;
	}
	if(flipy){
		vec2 u0 = vp[0].uv, u1 = vp[1].uv, u2 = vp[2].uv, u3 = vp[3].uv;
		vp[0].uv = u3; vp[1].uv = u2; vp[2].uv = u1; vp[3].uv = u0;
	}
	return render_make_texture_counts();
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_voxel
enum{ //voxel face order
	render_voxel_face_posx = 0,
	render_voxel_face_posy = 1,
	render_voxel_face_posz = 2,
	render_voxel_face_negx = 3,
	render_voxel_face_negy = 4,
	render_voxel_face_negz = 5,
};

enum{ //voxel face vertex order (when looking at the front of the face)
	render_voxel_face_vertex_bl = 0,
	render_voxel_face_vertex_tl = 1,
	render_voxel_face_vertex_tr = 2,
	render_voxel_face_vertex_br = 3,
};


local RenderVoxelType* render_voxel_types;
local u64 render_voxel_types_count;
local RenderVoxelChunk* render_voxel_chunk_pool;
local u32 render_voxel_voxel_size; //width, height, and depth of a voxel in the world

local vec3 render_voxel_unit_vertex_offsets[6][4] = { //unit vertex offsets by face
	{ { 1,0,0 }, { 1,1,0 }, { 1,1,1 }, { 1,0,1 } }, //render_voxel_face_posx
	{ { 0,1,0 }, { 0,1,1 }, { 1,1,1 }, { 1,1,0 } }, //render_voxel_face_posy
	{ { 1,0,1 }, { 1,1,1 }, { 0,1,1 }, { 0,0,1 } }, //render_voxel_face_posz
	{ { 0,0,1 }, { 0,1,1 }, { 0,1,0 }, { 0,0,0 } }, //render_voxel_face_negx
	{ { 0,0,1 }, { 0,0,0 }, { 1,0,0 }, { 1,0,1 } }, //render_voxel_face_negy
	{ { 0,0,0 }, { 0,1,0 }, { 1,1,0 }, { 1,0,0 } }, //render_voxel_face_negz
};

local vec3 render_voxel_face_normals[6] = {
	vec3_RIGHT(),
	vec3_LEFT(),
	vec3_UP(),
	vec3_DOWN(),
	vec3_FORWARD(),
	vec3_BACK(),
};


//NOTE(delle) voxels are linearly laid out like x-y plane at z0, x-y plane at z1, ... with x being the major axis in the x-y plane
#define render_voxel_linear(dims,x,y,z)  (((z) * (dims) * (dims)) + ((y) * (dims)) + (x))
#define render_voxel_right(dims,linear)  ((linear) + 1)
#define render_voxel_left(dims,linear)   ((linear) - 1)
#define render_voxel_above(dims,linear)  ((linear) + (dims))
#define render_voxel_below(dims,linear)  ((linear) - (dims))
#define render_voxel_front(dims,linear)  ((linear) + ((dims)*(dims)))
#define render_voxel_behind(dims,linear) ((linear) - ((dims)*(dims)))


void
render_voxel_init(RenderVoxelType* types, u64 count, u32 voxel_size){
	render_voxel_types = types;
	render_voxel_types_count = count;
	memory_pool_init(render_voxel_chunk_pool, 128);
	for_pool(render_voxel_chunk_pool) it->hidden = true;
	render_voxel_voxel_size = voxel_size;
}


void
render_voxel_make_face_mesh(int direction, RenderVoxelChunk* chunk, RenderVoxel* voxel, MeshVertex* vertex_array, u64* vertex_count, MeshIndex* index_array, u64* index_count){
	vec3 voxel_position = chunk->position + Vec3(voxel->x, voxel->y, voxel->z);
	mat4 transform = mat4::TransformationMatrix(voxel_position, chunk->rotation, vec3_ONE());
	
	vertex_array[*vertex_count+0] = {
		render_voxel_unit_vertex_offsets[direction][render_voxel_face_vertex_bl] * transform,
		Vec2(0,0),
		render_voxel_types[voxel->type].color,
		render_voxel_face_normals[direction] * transform
	};
	vertex_array[*vertex_count+1] = {
		render_voxel_unit_vertex_offsets[direction][render_voxel_face_vertex_tl] * transform,
		Vec2(0,1),
		render_voxel_types[voxel->type].color,
		render_voxel_face_normals[direction] * transform
	};
	vertex_array[*vertex_count+2] = {
		render_voxel_unit_vertex_offsets[direction][render_voxel_face_vertex_tr] * transform,
		Vec2(1,1),
		render_voxel_types[voxel->type].color,
		render_voxel_face_normals[direction] * transform
	};
	vertex_array[*vertex_count+3] = {
		render_voxel_unit_vertex_offsets[direction][render_voxel_face_vertex_br] * transform,
		Vec2(1,0),
		render_voxel_types[voxel->type].color,
		render_voxel_face_normals[direction] * transform
	};
	
	index_array[*index_count+0] = *vertex_count+0;
	index_array[*index_count+1] = *vertex_count+1;
	index_array[*index_count+2] = *vertex_count+2;
	index_array[*index_count+3] = *vertex_count+0;
	index_array[*index_count+4] = *vertex_count+2;
	index_array[*index_count+5] = *vertex_count+3;
	
	*vertex_count += 4;
	*index_count  += 6;
}


RenderVoxelChunk*
render_voxel_create_chunk(vec3 position, vec3 rotation, u32 dimensions, RenderVoxel* voxels, u64 voxels_count){
	Assert(dimensions != 0, "Dimensions can not be zero!");
	Assert(voxels != 0 && voxels_count != 0, "Don't call this with an invalid voxels array!");
	
	//alloc and init chunk
	RenderVoxelChunk* chunk = memory_pool_push(render_voxel_chunk_pool);
	chunk->position    = position;
	chunk->rotation    = rotation;
	chunk->dimensions  = dimensions;
	chunk->modified    = false;
	chunk->hidden      = false;
	chunk->voxel_count = voxels_count;
	
	//calculate some chunk creation info
	upt dimensions_cubed    = dimensions * dimensions * dimensions;
	upt dimensions_stride_x = 1;
	upt dimensions_stride_y = dimensions;
	upt dimensions_stride_z = dimensions * dimensions;
	
	//alloc an arena for chunk creation
	upt array_header_size = sizeof(stbds_array_header);
	upt voxels_array_size = dimensions_cubed * sizeof(RenderVoxel*);
	upt max_vertices_size = dimensions_cubed * 24 * sizeof(MeshVertex);
	upt max_indices_size  = dimensions_cubed * 36 * sizeof(MeshIndex);
	chunk->arena = memory_create_arena(voxels_array_size + max_vertices_size + max_indices_size);
	
	//init voxels array
	chunk->voxels = (RenderVoxel**)memory_arena_push(chunk->arena,voxels_array_size);
	ZeroMemory(chunk->voxels, dimensions_cubed * sizeof(RenderVoxel*));
	for(RenderVoxel* it = voxels; it < voxels+voxels_count; ++it){
		chunk->voxels[render_voxel_linear(dimensions, it->x, it->y, it->z)] = it;
	}
	
	//generate chunk's mesh
	//TODO(delle) combine faces across the chunk where possible
	MeshVertex* vertex_array = (MeshVertex*)memory_arena_push(chunk->arena,max_vertices_size);
	MeshIndex*  index_array  =  (MeshIndex*)memory_arena_push(chunk->arena,max_indices_size);
	u32 dimensions_minus_one = dimensions-1;
	forI(dimensions_cubed){
		if(chunk->voxels[i] == 0) continue; //skip empty voxels
		
		if((chunk->voxels[i]->x == dimensions_minus_one) || (chunk->voxels[i + dimensions_stride_x] == 0))
			render_voxel_make_face_mesh(render_voxel_face_posx, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		if((chunk->voxels[i]->x == 0)                    || (chunk->voxels[i - dimensions_stride_x] == 0))
			render_voxel_make_face_mesh(render_voxel_face_negx, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		if((chunk->voxels[i]->y == dimensions_minus_one) || (chunk->voxels[i + dimensions_stride_y] == 0))
			render_voxel_make_face_mesh(render_voxel_face_posy, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		if((chunk->voxels[i]->y == 0)                    || (chunk->voxels[i - dimensions_stride_y] == 0))
			render_voxel_make_face_mesh(render_voxel_face_negy, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		if((chunk->voxels[i]->z == dimensions_minus_one) || (chunk->voxels[i + dimensions_stride_z] == 0))
			render_voxel_make_face_mesh(render_voxel_face_posz, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		if((chunk->voxels[i]->z == 0)                    || (chunk->voxels[i - dimensions_stride_z] == 0))
			render_voxel_make_face_mesh(render_voxel_face_negz, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
	}
	
	//shift the index_array to the end of the vertex_array
	upt actual_vertices_size = chunk->vertex_count*sizeof(MeshVertex);
	upt actual_indices_size  =  chunk->index_count*sizeof(MeshIndex);
	MeshIndex* new_index_array = (MeshIndex*)((u8*)vertex_array + actual_vertices_size);
	CopyMemory(new_index_array, index_array, actual_indices_size);
	index_array = new_index_array;
	
	//fit the arena to its actually used size
	chunk->arena->used = voxels_array_size + actual_vertices_size + actual_indices_size;
	memory_arena_fit(chunk->arena);
	
	//create the vertex/index GPU buffers and upload the vertex/index data to them
	chunk->vertex_buffer = render_buffer_create(vertex_array, actual_vertices_size, RenderBufferUsage_VertexBuffer,
												RenderMemoryProperty_DeviceMappable, RenderMemoryMapping_MapWriteUnmap);
	chunk->index_buffer  = render_buffer_create(index_array,  actual_indices_size,  RenderBufferUsage_IndexBuffer,
												RenderMemoryProperty_DeviceMappable, RenderMemoryMapping_MapWriteUnmap);
	
	renderStats.totalVoxels += voxels_count;
	renderStats.totalVoxelChunks += 1;
	return chunk;
}


void
render_voxel_delete_chunk(RenderVoxelChunk* chunk){
	Assert(renderStats.totalVoxelChunks > 0);
	
	//dealloc GPU buffers
	render_buffer_delete(chunk->vertex_buffer);
	render_buffer_delete(chunk->index_buffer);
	
	//dealloc chunk arena
	memory_delete_arena(chunk->arena);
	
	//delete the chunk (and set it to hidden since for_pool() doesn't skip deleted chunks)
	memory_pool_delete(render_voxel_chunk_pool, chunk);
	chunk->hidden = true;
	
	renderStats.totalVoxels -= chunk->voxel_count;
	renderStats.totalVoxelChunks -= 1;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_other
void
render_display_stats(){
	FixMe;
    // using namespace UI;
    // BeginRow(str8_lit("renderstatsaligned"), 2, 0, UIRowFlags_AutoSize);{
    //     RowSetupColumnAlignments({{0,0.5},{1,0.5}});
    //     TextF(str8_lit("total triangles: %d"), renderStats.totalTriangles);
    //     TextF(str8_lit("total vertices: %d"),  renderStats.totalVertices);
    //     TextF(str8_lit("total indices: %d"),   renderStats.totalIndices);
    //     TextF(str8_lit("drawn triangles: %d"), renderStats.drawnTriangles);
    //     TextF(str8_lit("drawn indicies: %d"),  renderStats.drawnIndices);
    //     TextF(str8_lit("render time: %g"),     renderStats.renderTimeMS);
    // }EndRow();
}


#endif //DESHI_IMPLEMENTATION
