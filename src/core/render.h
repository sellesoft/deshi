/* deshi Render Module
Notes:
  All 2D and 3D drawing functions are immediate-mode, except render_debug_line().
  Drawing functions ending in 2 are for 2D, 3 are for 3D, and nothing are implicitly 3D.
  The 2D coordinate system goes from top left as the origin to bottom right.

Index:
@render_types
  VSyncType
  RendererStage
  ShaderStage
  RenderStats
  RenderSettings
  Vertex2
  RenderModelCmd
  RenderTwodCmd
  RenderMesh
  RenderDrawCounts
@render_status
  render_init() -> void
  render_update() -> void
  render_reset() -> void
  render_cleanup() -> void
  render_load_settings() -> void
  render_save_settings() -> void
  render_get_settings() -> RenderSettings*
  render_get_stats() -> RenderStats*
  render_get_stage() -> RenderStage*
@render_surface
  render_max_surface_count() -> u32
  render_register_surface(Window* window) -> void
  render_set_active_surface(Window* window) -> void
  render_set_active_surface_idx(u32 idx) -> void
@render_loading
  render_load_mesh(Mesh* mesh) -> void
  render_load_texture(Texture* texture) -> void
  render_load_material(Material* material) -> void
  render_unload_mesh(Mesh* mesh) -> void
  render_unload_texture(Texture* texture) -> void
  render_unload_material(Material* material) -> void
  render_update_material(Material* material) -> void
@render_draw_3d
  render_model(Model* model, mat4* matrix) -> void
  render_model(Model* model, vec3 position, vec3 rotation, vec3 scale) -> void
  render_model_wireframe(Model* model, mat4* matrix, color c) -> void
  render_line3(vec3 start, vec3 end, color c) -> void
  render_triangle3(vec3 p0, vec3 p1, vec3 p2, color c) -> void
  render_triangle_filled3(vec3 p0, vec3 p1, vec3 p2, color c) -> void
  render_quad3(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c) -> void
  render_quad_filled3(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c) -> void
  render_poly3(vec3* points, u64 count, color c) -> void
  render_poly3(const array<vec3>& points, color c = Color_White) -> void
  render_poly_filled3(vec3* points, u64 count, color c) -> void
  render_poly_filled3(const array<vec3>& points, color c = Color_White) -> void
  render_circle3(vec3 position, vec3 rotation, f32 radius, u32 subdivisions, color c) -> void
  render_box(mat4* transform, color c) -> void
  render_box(vec3 position, vec3 rotation, vec3 scale, color c = Color_White) -> void
  render_box_filled(mat4* transform, color c) -> void
  render_box_filled(vec3 position, vec3 rotation, vec3 scale, color c = Color_White) -> void
  render_sphere(vec3 position, vec3 rotation, f32 radius, u32 subdivisions, color cx, color cy, color cz) -> void
  render_sphere(vec3 position, vec3 rotation, f32 radius, u32 subdivisions = 16, color c = Color_White) -> void
  render_frustrum(vec3 position, vec3 target, f32 aspectRatio, f32 fovx, f32 nearZ, f32 farZ, color c) -> void
  render_clear_debug() -> void
  render_debug_line3(vec3 p0, vec3 p1,  color c) -> void
@render_draw_2d
  render_start_cmd2(u32 layer, Texture* texture, vec2 scissorOffset, vec2 scissorExtent) -> void
  render_add_vertices2(u32 layer, Vertex2* vertices, u32 vCount, u32* indices, u32 iCount) -> void
  render_line2(vec2 start, vec2 end, color c) -> void
  render_line_thick2(vec2 start, vec2 end, f32 thickness, color c) -> void
  render_lines_smooth2(vec2* points, u64 count, f32 thickness, color c) -> void
  render_lines_smooth2(array<vec2>& points, f32 thickness = 1, color c = Color_White) -> void
  render_triangle2(vec2 p0, vec2 p1, vec2 p2, color c) -> void
  render_triangle_filled2(vec2 p0, vec2 p1, vec2 p2, color c) -> void
  render_quad2(vec2 p0, vec2 p1, vec2 p2, vec2 p3, color c) -> void
  render_quad_filled2(vec2 p0, vec2 p1, vec2 p2, vec2 p3, color c) -> void
  render_poly2(vec2* points, u64 count, color c) -> void
  render_poly2(const array<vec2>& points, color c = Color_White) -> void
  render_poly_filled2(vec2* points, u64 count, color c) -> void
  render_poly_filled2(const array<vec2>& points, color c = Color_White) -> void
  render_circle2(vec2 position, vec2 rotation, f32 radius, u32 subdivisions, color c) -> void
  render_circle_filled2(vec2 position, vec2 rotation, f32 radius, u32 subdivisions, color c) -> void
  render_text2(str8 text, vec2 position, vec2 scale, color c) -> void
  render_texture2(Texture* texture, vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, f32 transparency) -> void
  render_texture_flat2(Texture* texture, vec2 position, vec2 dimensions, f32 transparency) -> void
  render_texture_rotated2(Texture* texture, vec2 position, vec2 dimensions, f32 rotation, f32 transparency) -> void
  render_top_layer_index() -> u32
  render_decoration_layer_index() -> u32
@render_light
  render_update_light(u32 idx, vec3 position, f32 brightness) -> void
@render_camera
  render_update_camera_position(vec3 position) -> void
  render_update_camera_view(mat4& view_matrix) -> void
  render_update_camera_projection(mat4& projection_matrix) -> void
  render_use_default_camera() -> void
@render_shaders
  render_reload_shader(u32 shader_type) -> void
  render_reload_all_shaders() -> void
@render_make_2d
  render_make_line_counts() -> RenderDrawCounts
  render_make_filledtriangle_counts() -> RenderDrawCounts
  render_make_triangle_counts() -> RenderDrawCounts
  render_make_filledrect_counts() -> RenderDrawCounts
  render_make_rect_counts() -> RenderDrawCounts
  render_make_circle_counts(u32 subdiv) -> RenderDrawCounts
  render_make_filledcircle_counts(u32 subdiv) -> RenderDrawCounts
  render_make_text_counts(u32 charcount) -> RenderDrawCounts
  render_make_texture_counts() -> RenderDrawCounts
  render_make_line(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 start, vec2 end, f32 thickness, color color) -> RenderDrawCounts
  render_make_filledtriangle(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 p1, vec2 p2, vec2 p3, color color) -> RenderDrawCounts
  render_make_triangle(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 p0, vec2 p1, vec2 p2, f32 thickness, color color) -> RenderDrawCounts
  render_make_filledrect(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 pos, vec2 size, color color) -> RenderDrawCounts
  render_make_rect(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 pos, vec2 size, f32 thickness, color color) -> RenderDrawCounts
  render_make_circle(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 pos, f32 radius, u32 subdivisions_int, f32 thickness, color color) -> RenderDrawCounts
  render_make_filledcircle(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 pos, f32 radius, u32 subdivisions_int, color color) -> RenderDrawCounts
  render_make_text(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, str8 text, Font* font, vec2 pos, color color, vec2 scale) -> RenderDrawCounts
  render_make_texture(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, Texture* texture, vec2 p0, vec2 p1, vec2 p2, vec2 p3, f32 alpha, b32 flipx, b32 flipy) -> RenderDrawCounts
@render_other
  render_remake_offscreen() -> void
  render_display_stats() -> void
  render_create_external_buffer -> RenderBuffer
@render_shared_status
@render_shared_surface
@render_shared_draw_3d
@render_shared_draw_2d
@render_shared_make_2d
@render_shared_other
*/

#pragma once
#ifndef DESHI_RENDER_H
#define DESHI_RENDER_H

#include "kigu/array.h"
#include "kigu/color.h"
#include "kigu/common.h"
#include "math/math.h"

struct Mesh;
struct Texture;
struct Material;
struct Model;
struct Font;
struct Window;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_types
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

external struct RenderStats{
	u32 totalTriangles;
	u32 totalVertices;
	u32 totalIndices;
	u32 drawnTriangles;
	u32 drawnIndices;
	f32 renderTimeMS;
};

external struct RenderSettings{
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
};

external struct Vertex2{
	vec2 pos;
	vec2 uv;
	u32  color;
};

typedef u16 RenderTempIndex;  //NOTE(delle) changing this also requires changing defines in the backend
typedef u32 RenderModelIndex; //NOTE(delle) changing this also requires changing defines in the backend
external struct RenderModelCmd{
	u32   vertexOffset;
	u32   indexOffset;
	u32   indexCount;
	u32   material;
	char* name;
	mat4  matrix;
};

typedef u32 RenderTwodIndex;  //NOTE(delle) changing this also requires changing defines in the backend
external struct RenderTwodCmd{
	void*    handle; //NOTE(delle) VkDescriptorSet in vulkan, texture index in OpenGl
	Vertex2* vertexBuffer;
	u32*     indexBuffer; // pointer to used index buffer
	u64      indexOffset;
	u64      indexCount;
	vec2     scissorOffset;
	vec2     scissorExtent;
};

external struct RenderMesh{
	Mesh* base;
	u32   vertexOffset;
	u32   vertexCount;
	u32   indexOffset;
	u32   indexCount;
};

//NOTE(sushi) simple replacement of vec2i till we make vecs compatible with external
external struct RenderDrawCounts{
	u64 vertices, indices;
	RenderDrawCounts operator* (u64 rhs){return {vertices*rhs,indices*rhs};}
	RenderDrawCounts operator+ (RenderDrawCounts rhs){return {vertices+rhs.vertices,indices+rhs.indices};}
	void             operator+=(RenderDrawCounts rhs){vertices+=rhs.vertices;indices+=rhs.indices;}
};

//holds handles to vertex and index handles
external struct RenderTwodBuffer{
	u32 idx;
	void* vertex_handle;
	void* index_handle;
};

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_status
//Initializes the `Render` module
external void render_init();

//Updates the `Render` module
external void render_update();

//Resets the `Render` module
external void render_reset();

//Cleans up the `Render` module
external void render_cleanup();

//Loads render settings from the render config file
external void render_load_settings();

//Saves current render settings to the render config file
external void render_save_settings();

//Returns the internal `RenderSettings`
external RenderSettings* render_get_settings();

//Returns the internal `RenderStats`
external RenderStats* render_get_stats();

//Returns the internal `RenderStage`
external RenderStage* render_get_stage();


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_surface
//Returns the maximum number of surfaces the backend supports
external u32 render_max_surface_count();

//Creates a render surface for `window` with `idx`
external void render_register_surface(Window* window);

//Sets the render surface for `window` to the active one
external void render_set_active_surface(Window* window);

//Sets the render surface for `idx` to the active one
external void render_set_active_surface_idx(u32 idx);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_loading
//Loads the `mesh` vertices and indices to the GPU and stores info to the backend for rendering
external void render_load_mesh(Mesh* mesh);

//Loads the `texture` pixels to the GPU and stores info to the backend for rendering
external void render_load_texture(Texture* texture);

//Loads the `material` info to the backend for rendering
external void render_load_material(Material* material);

//Unloads the `mesh` vertices and indices from the GPU and info from the backend
external void render_unload_mesh(Mesh* mesh);

//Unloads the `texture` pixels from the GPU and info from the backend
external void render_unload_texture(Texture* texture);

//Unloads the `material` info from the backend
external void render_unload_material(Material* material);

//Updates the internal material copy of `material` with any new changes
external void render_update_material(Material* material);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_draw_3d
//Renders the `model` with the transform `matrix`
external void render_model(Model* model, mat4* matrix);
FORCE_INLINE void render_model(Model* model, vec3 position, vec3 rotation, vec3 scale){mat4 transform=mat4::TransformationMatrix(position,rotation,scale);render_model(model,&transform);}

//Renders the a wireframe of `model` with the transform `matrix`
external void render_model_wireframe(Model* model, mat4* matrix, color c);

//Renders a 3D line from `start` to `end`
external void render_line3(vec3 start, vec3 end, color c);

//Renders a wireframe 3D triangle using the specified points
external void render_triangle3(vec3 p0, vec3 p1, vec3 p2, color c);

//Renders a filled 3D triangle using the specified points
external void render_triangle_filled3(vec3 p0, vec3 p1, vec3 p2, color c);

//Renders a wireframe 3D quad using the specified points
external void render_quad3(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c);

//Renders a filled 3D quad using the specified points
external void render_quad_filled3(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c);

//Renders a wireframe 3D polygon using `count` points from `points`
external void render_poly3(vec3* points, u64 count, color c);
FORCE_INLINE void render_poly3(const array<vec3>& points, color c = Color_White){ render_poly3(points.data, points.count, c); }

//Renders a filled 3D polygon using `count` points from `points`
external void render_poly_filled3(vec3* points, u64 count, color c);
FORCE_INLINE void render_poly_filled3(const array<vec3>& points, color c = Color_White){ render_poly_filled3(points.data, points.count, c); }

//Renders a wireframe 3D circle centered at `position` with `rotation` and `radius` using `subdivisions` line segments
external void render_circle3(vec3 position, vec3 rotation, f32 radius, u32 subdivisions, color c);

//Renders a wireframe box with a `transform`
external void render_box(mat4* transform, color c);
FORCE_INLINE void render_box(vec3 position, vec3 rotation, vec3 scale, color c = Color_White){mat4 transform=mat4::TransformationMatrix(position, rotation, scale);render_box(&transform,c);}

//Renders a filled-face box with a `transform`
external void render_box_filled(mat4* transform, color c);
FORCE_INLINE void render_box_filled(vec3 position, vec3 rotation, vec3 scale, color c = Color_White){mat4 transform=mat4::TransformationMatrix(position, rotation, scale);render_box_filled(&transform,c);}

//Renders three 3D circles centered at `position` with `rotation` and `radius` using `subdivisions` line segments
external void render_sphere(vec3 position, vec3 rotation, f32 radius, u32 subdivisions, color cx, color cy, color cz);
FORCE_INLINE void render_sphere(vec3 position, vec3 rotation, f32 radius, u32 subdivisions = 16, color c = Color_White){render_sphere(position,rotation,radius,subdivisions,c,c,c);}

//Renders a frustum shape starting at `position` facing towards `target` and with the properties `aspectRatio`, `fovx`, `nearZ`, `farZ`
external void render_frustrum(vec3 position, vec3 target, f32 aspectRatio, f32 fovx, f32 nearZ, f32 farZ, color c);

//Clears the collection of debug lines
external void render_clear_debug();

//Creates a debug line (non-immediate drawing) from `p0` to `p1`
external void render_debug_line3(vec3 p0, vec3 p1,  color c);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_draw_2d
//Starts a new `RenderTwodCmd` on `layer` with the specified values
external void render_start_cmd2(u32 layer, Texture* texture, vec2 scissorOffset, vec2 scissorExtent);

//Starts a new RenderTwodCmd with the specified calues using externally allocated buffers
//NOTE: these buffers must have been mapped using render_update_external_2d_buffer()
external void render_start_cmd2_exbuff(RenderTwodBuffer buffer, RenderTwodIndex index_offset, RenderTwodIndex index_count, Vertex2* vertbuff, RenderTwodIndex* indbuff, u32 layer, Texture* texture, vec2 scissorOffset, vec2 scissorExtent);

//TODO(sushi) a full interface for using external buffers
//eg. render_get_buffer_size, render_close_buffer, etc.
//creates an external GPU buffer for 2D drawing information
external RenderTwodBuffer render_create_external_2d_buffer(u64 vert_buffsize, u64 ind_buffsize);
external void render_update_external_2d_buffer(RenderTwodBuffer* buffer, Vertex2* vb, RenderTwodIndex vcount, RenderTwodIndex* ib, RenderTwodIndex icount);

//Adds `vertices` and `indices` to the active `RenderTwodCmd` on `layer`
//    `indices` values should be local to the addition (start at 0) since they are added to the offset internally
external void render_add_vertices2(u32 layer, Vertex2* vertices, u32 vCount, u32* indices, u32 iCount);

//Returns the top-most layer for 2D rendering
external u32  render_top_layer_index();

//Returns the window decorations layer for 2D rendering (higher than top-most)
external u32  render_decoration_layer_index();

//Returns the active layer in 2D rendering
external u32  render_active_layer();

//Renders a 2D line to the active `RenderTwodCmd` from `start` to `end` with 1 pixel thickness
external void render_line2(vec2 start, vec2 end, color c);

//Renders a 2D line to the active `RenderTwodCmd` from `start` to `end` with `thickness` (in pixels)
external void render_line_thick2(vec2 start, vec2 end, f32 thickness, color c);

//Renders a set of 2D lines to the active `RenderTwodCmd` with `thickness` (in pixels)
external void render_lines2(vec2* points, u64 count, f32 thickness, color c);
FORCE_INLINE void render_lines2(array<vec2>& points, f32 thickness = 1, color c = Color_White){render_lines2(points.data,points.count,thickness,c);}

//Renders a wireframe 2D triangle to the active `RenderTwodCmd` using the specified points
external void render_triangle2(vec2 p0, vec2 p1, vec2 p2, color c);

//Renders a filled 2D triangle to the active `RenderTwodCmd` using the specified points
external void render_triangle_filled2(vec2 p0, vec2 p1, vec2 p2, color c);

//Renders a wireframe 2D quad to the active `RenderTwodCmd` using the specified points
external void render_quad2(vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, color c);
FORCE_INLINE void render_quad2(vec2 position, vec2 dimensions, color c = Color_White){render_quad2(position,position+dimensions.xComp(),position+dimensions.yComp(),position+dimensions,c);}

//Renders a wireframe 2D quad to the active `RenderTwodCmd` using the specified points and line `thickness`
external void render_quad_thick2(vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, f32 thickness, color c);
FORCE_INLINE void render_quad_thick2(vec2 position, vec2 dimensions, f32 thickness = 1, color c = Color_White){render_quad_thick2(position,position+dimensions.xComp(),position+dimensions.yComp(),position+dimensions,thickness,c);}

//Renders a filled 2D quad to the active `RenderTwodCmd` using the specified points
external void render_quad_filled2(vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, color c);
FORCE_INLINE void render_quad_filled2(vec2 position, vec2 dimensions, color c = Color_White){render_quad_filled2(position,position+dimensions.xComp(),position+dimensions.yComp(),position+dimensions,c);}

//Renders a wireframe 2D polygon to the active `RenderTwodCmd` using `count` points from `points`
external void render_poly2(vec2* points, u64 count, color c);
FORCE_INLINE void render_poly2(array<vec2>& points, color c = Color_White){render_poly2(points.data,points.count,c);}

//Renders a filled 2D polygon to the active `RenderTwodCmd` using `count` points from `points`
external void render_poly_filled2(vec2* points, u64 count, color c);
FORCE_INLINE void render_poly_filled2(array<vec2>& points, color c = Color_White){render_poly_filled2(points.data,points.count,c);}

//Renders a wireframe 2D circle to the active `RenderTwodCmd` centered at `position` with `rotation` (degrees) and `radius` using `subdivisions` line segments
//    `rotation` is in degrees counter-clockwise
external void render_circle2(vec2 position, f32 rotation, f32 radius, u32 subdivisions, color c);

//Renders a filled 2D circle to the active `RenderTwodCmd` centered at `position` with `rotation` and `radius` using `subdivisions` line segments
//    `rotation` is in degrees counter-clockwise
//    this does a triangle pie-fill algorithm, so it's not optimized for vertex count
external void render_circle_filled2(vec2 position, f32 rotation, f32 radius, u32 subdivisions, color c);

//Renders 2D `text` to the active `RenderTwodCmd` at `position` with `font` and `scale`
external void render_text2(Font* font, str8 text, vec2 position, vec2 scale, color c);

//Renders a 2D `texture` to the active `RenderTwodCmd` with `transparency` and points `top_left`, `top_right`, `bot_left`, and `bot_right`
external void render_texture2(Texture* texture, vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, f32 transparency);

//Renders a flat 2D `texture` to the active `RenderTwodCmd` at `position` (top-left) with `dimensions` and `transparency` (0-1)
external void render_texture_flat2(Texture* texture, vec2 position, vec2 dimensions, f32 transparency);

//Renders a 2D `texture` to the active `RenderTwodCmd` rotated by `rotation` at `center` with `dimensions` and `transparency` (0-1)
//    `rotation` is in degrees counter-clockwise
external void render_texture_rotated2(Texture* texture, vec2 center, vec2 dimensions, f32 rotation, f32 transparency);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_light
//Updates the light at `idx` with `position` and `brightness`
external void render_update_light(u32 idx, vec3 position, f32 brightness);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_camera
//Updates the camera's `position`
//TODO remove this since it's redundant with view matrix
external void render_update_camera_position(vec3 position);

//Updates the camera's `view_matrix`
external void render_update_camera_view(mat4* view_matrix);

//Updates the camera's `projection_matrix`
external void render_update_camera_projection(mat4* projection_matrix);

//Updates the camera to the position (0,0) looking at (1,1) with 90 horizontal FOV, 0.1 near plane, and 1000 far plane
external void render_use_default_camera();


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shaders
//Recompiles and loads the shader with `shader_type`
external void render_reload_shader(u32 shader_type);

//Recompiles and loads the all shaders in the shaders folder
//TODO only reload loaded shaders
external void render_reload_all_shaders();


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_make_2d
// NOTE(sushi): if you change any of the drawing algorithms do not forget to count the verticies and indices and update these functions!!
FORCE_INLINE RenderDrawCounts render_make_line_counts()                  {return { 4, 6};};
FORCE_INLINE RenderDrawCounts render_make_filledtriangle_counts()        {return { 3, 3};};
FORCE_INLINE RenderDrawCounts render_make_triangle_counts()              {return {12,18};};
FORCE_INLINE RenderDrawCounts render_make_filledrect_counts()            {return { 4, 6};};
FORCE_INLINE RenderDrawCounts render_make_rect_counts()                  {return {16,24};};
FORCE_INLINE RenderDrawCounts render_make_circle_counts(u32 subdiv)      {return {2*subdiv,6*subdiv};};
FORCE_INLINE RenderDrawCounts render_make_filledcircle_counts(u32 subdiv){return {1+subdiv,3*subdiv};};
FORCE_INLINE RenderDrawCounts render_make_text_counts(u32 charcount)     {return {4*charcount,6*charcount};};
FORCE_INLINE RenderDrawCounts render_make_texture_counts()               {return { 8,24};};

//TODO(sushi) reformat these to use vec2i for offsets once its compatible with external
external RenderDrawCounts render_make_line(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 start, vec2 end, f32 thickness, color color);
external RenderDrawCounts render_make_filledtriangle(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 p1, vec2 p2, vec2 p3, color color);
external RenderDrawCounts render_make_triangle(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 p0, vec2 p1, vec2 p2, f32 thickness, color color);
external RenderDrawCounts render_make_filledrect(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 pos, vec2 size, color color);
external RenderDrawCounts render_make_rect(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 pos, vec2 size, f32 thickness, color color);
external RenderDrawCounts render_make_circle(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 pos, f32 radius, u32 subdivisions_int, f32 thickness, color color);
external RenderDrawCounts render_make_filledcircle(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 pos, f32 radius, u32 subdivisions_int, color color);
external RenderDrawCounts render_make_text(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, str8 text, Font* font, vec2 pos, color color, vec2 scale);
external RenderDrawCounts render_make_texture(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, Texture* texture, vec2 p0, vec2 p1, vec2 p2, vec2 p3, f32 alpha, b32 flipx, b32 flipy);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_other
//NOTE temporary function for vulkan shadow stuff
external void render_remake_offscreen();

//displays render stats into a UI Window, this does NOT make it's own window, implemented in core_ui.cpp
external void render_display_stats();



#endif //DESHI_RENDER_H
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifdef DESHI_IMPLEMENTATION
#include "config.h"
#include "model.h"
#include "ui.h"


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
//// @render_shared_surface
#define MAX_SURFACES 2
local u32 renderActiveSurface = 0;

u32
render_max_surface_count(){
	return MAX_SURFACES;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_draw_3d
#define MAX_TEMP_VERTICES 0xFFFF //max u16: 65535
#define MAX_TEMP_INDICES 3*MAX_TEMP_VERTICES
local RenderTempIndex  renderTempWireframeVertexCount = 0;
local RenderTempIndex  renderTempWireframeIndexCount  = 0;
local Mesh::Vertex     renderTempWireframeVertexArray[MAX_TEMP_VERTICES];
local RenderTempIndex  renderTempWireframeIndexArray[MAX_TEMP_INDICES];
local RenderTempIndex  renderTempFilledVertexCount    = 0;
local RenderTempIndex  renderTempFilledIndexCount     = 0;
local Mesh::Vertex     renderTempFilledVertexArray[MAX_TEMP_VERTICES];
local RenderTempIndex  renderTempFilledIndexArray[MAX_TEMP_INDICES];
local RenderTempIndex  renderDebugWireframeVertexCount = 0;
local RenderTempIndex  renderDebugWireframeIndexCount  = 0;
local Mesh::Vertex     renderDebugWireframeVertexArray[MAX_TEMP_VERTICES];
local RenderTempIndex  renderDebugWireframeIndexArray[MAX_TEMP_INDICES];
local RenderTempIndex  renderDebugFilledVertexCount    = 0;
local RenderTempIndex  renderDebugFilledIndexCount     = 0;
local Mesh::Vertex     renderDebugFilledVertexArray[MAX_TEMP_VERTICES];
local RenderTempIndex  renderDebugFilledIndexArray[MAX_TEMP_INDICES];

#define MAX_MODEL_CMDS 10000 
local RenderModelIndex renderModelCmdCount = 0;
local RenderModelCmd   renderModelCmdArray[MAX_MODEL_CMDS];


void
render_line3(vec3 start, vec3 end, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32 rgba = c.rgba;
	Mesh::Vertex*    vp = renderTempWireframeVertexArray + renderTempWireframeVertexCount;
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
	Mesh::Vertex*    vp = renderTempWireframeVertexArray + renderTempWireframeVertexCount;
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
	Mesh::Vertex*    vp = renderTempWireframeVertexArray + renderTempWireframeVertexCount;
	RenderTempIndex* ip = renderTempWireframeIndexArray  + renderTempWireframeIndexCount;
	
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
	
	vec3 p(0.5f, 0.5f, 0.5f);
	vec3 points[8] = {
		{-p.x, p.y, p.z},{-p.x,-p.y, p.z},{-p.x, p.y,-p.z},{-p.x,-p.y,-p.z},
		{ p.x, p.y, p.z},{ p.x,-p.y, p.z},{ p.x, p.y,-p.z},{ p.x,-p.y,-p.z},
	};
	forI(8){ points[i] = points[i] * (*transform); }
	render_line3(points[3], points[1], c); render_line3(points[3], points[2], c); render_line3(points[3], points[7], c);
	render_line3(points[0], points[1], c); render_line3(points[0], points[2], c); render_line3(points[0], points[4], c);
	render_line3(points[5], points[1], c); render_line3(points[5], points[4], c); render_line3(points[5], points[7], c);
	render_line3(points[6], points[2], c); render_line3(points[6], points[4], c); render_line3(points[6], points[7], c);
}

void
render_box_filled(mat4* transform, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	vec3 p(0.5f, 0.5f, 0.5f);
	vec3 points[8] = {
		{-p.x, p.y, p.z},{-p.x,-p.y, p.z},{-p.x, p.y,-p.z},{-p.x,-p.y,-p.z},
		{ p.x, p.y, p.z},{ p.x,-p.y, p.z},{ p.x, p.y,-p.z},{ p.x,-p.y,-p.z},
	};
	forI(8){ points[i] = points[i] * (*transform); }
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
	Mesh::Vertex*     vp = renderDebugWireframeVertexArray + renderDebugWireframeVertexCount;
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
local u32 renderActiveLayer = 5;

//external buffers
#define MAX_EXTERNAL_BUFFERS 6
local RenderTwodIndex renderExternalCmdCounts[MAX_EXTERNAL_BUFFERS];
local RenderTwodCmd   renderExternalCmdArrays[MAX_EXTERNAL_BUFFERS][MAX_TWOD_CMDS]; 

void
render_add_vertices2(u32 layer, Vertex2* vertices, u32 vCount, u32* indices, u32 iCount) {DPZoneScoped;
	Assert(vCount + renderTwodVertexCount < MAX_TWOD_VERTICES);
	Assert(iCount + renderTwodIndexCount  < MAX_TWOD_INDICES);
	
	Vertex2* vp         = renderTwodVertexArray + renderTwodVertexCount;
	RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
	
	CopyMemory(vp, vertices, vCount*sizeof(Vertex2));
	forI(iCount){
		Assert(indices[i] < vCount, "Index out of range of given number of vertices!\nMake sure your indices weren't overwritten by something.");
		ip[i] = renderTwodVertexCount + indices[i];
	} 
	
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
	vec2 norm = vec2(ott.y, -ott.x).normalized();
	
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
		vec2 norm = vec2(ott.y, -ott.x).normalized();
		
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
		vec2 norm = vec2(ott.y, -ott.x).normalized() * (f32)flip;
		
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
		render_line2(vec2(x0, y0), vec2(x1, y1), c);
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
		render_triangle_filled2(pos, vec2(x0, y0), vec2(x1, y1), c);
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
				renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].indexCount += 6;
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
				aligned_quad q = font_aligned_quad(font, decoded.codepoint, &pos, scale);
				
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
	renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].indexCount += 6;
}

void
render_texture_flat2(Texture* texture, vec2 pos, vec2 dimensions, f32 rotation, f32 transparency){DPZoneScoped;
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
RenderDrawCounts
render_make_line(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 start, vec2 end, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.vertices;
	u32*     ip = putindices + offsets.indices;
	
	vec2 ott = end - start;
	vec2 norm = vec2(ott.y, -ott.x).normalized();
	
	ip[0] = offsets.vertices; ip[1] = offsets.vertices + 1; ip[2] = offsets.vertices + 2;
	ip[3] = offsets.vertices; ip[4] = offsets.vertices + 2; ip[5] = offsets.vertices + 3;
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
RenderDrawCounts 
render_make_filledtriangle(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 p1, vec2 p2, vec2 p3, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.vertices;
	u32*     ip = putindices + offsets.indices;
	
	ip[0] = offsets.vertices; ip[1] = offsets.vertices + 1; ip[2] = offsets.vertices + 2;
	vp[0].pos = p1; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = p2; vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = p3; vp[2].uv = { 0,0 }; vp[2].color = col;
	
	return render_make_filledtriangle_counts();
}

RenderDrawCounts
render_make_triangle(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 p0, vec2 p1, vec2 p2, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.vertices;
	u32*     ip = putindices + offsets.indices;
	
	RenderDrawCounts sum;
	sum += render_make_line(vp, ip, {           0,          0}, p0, p1, 1, color);
	sum += render_make_line(vp, ip, {sum.vertices,sum.indices}, p1, p2, 1, color);
	sum += render_make_line(vp, ip, {sum.vertices,sum.indices}, p2, p0, 1, color);
	
	return sum;
	
	//TODO(sushi) this should be fixed to replace reliance on MakeLine
	//ip[0]  = offsets.indices + 0; ip[1]  = offsets.indices + 1; ip[2]  = offsets.indices + 3;
	//ip[3]  = offsets.indices + 0; ip[4]  = offsets.indices + 3; ip[5]  = offsets.indices + 2;
	//ip[6]  = offsets.indices + 2; ip[7]  = offsets.indices + 3; ip[8]  = offsets.indices + 5;
	//ip[9]  = offsets.indices + 2; ip[10] = offsets.indices + 5; ip[11] = offsets.indices + 4;
	//ip[12] = offsets.indices + 4; ip[13] = offsets.indices + 5; ip[14] = offsets.indices + 1;
	//ip[15] = offsets.indices + 4; ip[16] = offsets.indices + 1; ip[17] = offsets.indices + 0;
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

RenderDrawCounts
render_make_filledrect(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 pos, vec2 size, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.vertices;
	u32*     ip = putindices + offsets.indices;
	
	vec2 tl = pos;
	vec2 br = pos + size;
	vec2 bl = pos + vec2(0, size.y);
	vec2 tr = pos + vec2(size.x, 0);
	
	ip[0] = offsets.vertices; ip[1] = offsets.vertices + 1; ip[2] = offsets.vertices + 2;
	ip[3] = offsets.vertices; ip[4] = offsets.vertices + 2; ip[5] = offsets.vertices + 3;
	vp[0].pos = tl; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = tr; vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = br; vp[2].uv = { 0,0 }; vp[2].color = col;
	vp[3].pos = bl; vp[3].uv = { 0,0 }; vp[3].color = col;
	
	return render_make_filledrect_counts();
}

RenderDrawCounts
render_make_rect(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 pos, vec2 size, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.vertices;
	u32*     ip = putindices + offsets.indices;
	
	vec2 tl = pos;
	vec2 br = pos + size;
	vec2 bl = pos + vec2(0, size.y);
	vec2 tr = pos + vec2(size.x, 0);
	
	RenderDrawCounts sum = {0};
	sum += render_make_line(vp, ip, sum, tl,tr,thickness,color);
	sum += render_make_line(vp, ip, sum, tr,br,thickness,color);
	sum += render_make_line(vp, ip, sum, br,bl,thickness,color);
	sum += render_make_line(vp, ip, sum, bl,tl,thickness,color);
	
	//TODO(sushi) test this
	// vec2 tl = pos;
	// vec2 br = pos + size;
	// vec2 bl = vec2{br.x, tl.y};
	// vec2 tr = vec2{tl.x, br.y};
	// u32 t = item->style.border_width;
	// u32 v = counts.vertices; u32 i = counts.indices;
	// ip[i+ 0] = v+0; ip[i+ 1] = v+1; ip[i+ 2] = v+3; 
	// ip[i+ 3] = v+0; ip[i+ 4] = v+3; ip[i+ 5] = v+2; 
	// ip[i+ 6] = v+2; ip[i+ 7] = v+3; ip[i+ 8] = v+5; 
	// ip[i+ 9] = v+2; ip[i+10] = v+5; ip[i+11] = v+4; 
	// ip[i+12] = v+4; ip[i+13] = v+5; ip[i+14] = v+7; 
	// ip[i+15] = v+4; ip[i+16] = v+7; ip[i+17] = v+6; 
	// ip[i+18] = v+6; ip[i+19] = v+7; ip[i+20] = v+1; 
	// ip[i+21] = v+6; ip[i+22] = v+1; ip[i+23] = v+0;
	// vp[v+0].pos = tl;             vp[v+0].uv = {0,0}; vp[v+0].color = item->style.border_color.rgba;
	// vp[v+1].pos = tl+vec2( t, t); vp[v+1].uv = {0,0}; vp[v+1].color = item->style.border_color.rgba;
	// vp[v+2].pos = tr;             vp[v+2].uv = {0,0}; vp[v+2].color = item->style.border_color.rgba;
	// vp[v+3].pos = tr+vec2(-t, t); vp[v+3].uv = {0,0}; vp[v+3].color = item->style.border_color.rgba;
	// vp[v+4].pos = br;             vp[v+4].uv = {0,0}; vp[v+4].color = item->style.border_color.rgba;
	// vp[v+5].pos = br+vec2(-t,-t); vp[v+5].uv = {0,0}; vp[v+5].color = item->style.border_color.rgba;
	// vp[v+6].pos = bl;             vp[v+6].uv = {0,0}; vp[v+6].color = item->style.border_color.rgba;
	// vp[v+7].pos = bl+vec2( t,-t); vp[v+7].uv = {0,0}; vp[v+7].color = item->style.border_color.rgba;
	// counts += {8, 24};
	
	return sum;
}

RenderDrawCounts
render_make_circle(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 pos, f32 radius, u32 subdivisions_int, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.vertices;
	u32*     ip = putindices + offsets.indices;
	
	f32 subdivisions = f32(subdivisions_int);
	u32 nuindexes = subdivisions * 6;
	
	//first and last point
	vec2 last = pos + vec2(radius, 0);
	vp[0].pos = last + vec2(-thickness / 2, 0); vp[0].uv={0,0}; vp[0].color=col;
	vp[1].pos = last + vec2( thickness / 2, 0); vp[1].uv={0,0}; vp[1].color=col;
	ip[0] = offsets.vertices + 0; ip[1] = offsets.vertices + 1; ip[3] = offsets.vertices + 0;
	ip[nuindexes - 1] = offsets.vertices + 0; ip[nuindexes - 2] = ip[nuindexes - 4] = offsets.vertices + 1;
	
	for(s32 i = 1; i < subdivisions_int; i++){
		f32 a1 = (f32(i) * M_2PI) / subdivisions;
		vec2 offset(radius * cosf(a1), radius * sinf(a1));
		vec2 point = pos + offset;
		
		u32 idx = i * 2;
		vp[idx].pos = point - offset.normalized() * thickness / 2; vp[idx].uv = { 0,0 }; vp[idx].color = col;
		vp[idx + 1].pos = point + offset.normalized() * thickness / 2; vp[idx + 1].uv = { 0,0 }; vp[idx + 1 ].color = col;
		
		u32 ipidx1 = 6 * (i - 1) + 2;
		u32 ipidx2 = 6 * i - 1;
		ip[ipidx1] = ip[ipidx1 + 2] = ip[ipidx1 + 5] = offsets.vertices + idx + 1;
		ip[ipidx2] = ip[ipidx2 + 1] = ip[ipidx2 + 4] = offsets.vertices + idx;
	}
	
	return render_make_circle_counts(subdivisions_int);
}

RenderDrawCounts 
render_make_filledcircle(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, vec2 pos, f32 radius, u32 subdivisions_int, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.vertices;
	u32*     ip = putindices + offsets.indices;
	
	vp[0].pos = pos; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = pos + vec2(radius, 0); vp[1].uv = { 0,0 }; vp[1].color = col;
	u32 nuindexes = 3 * subdivisions_int;
	
	ip[1] = offsets.vertices + 1;
	for(s32 i = 0; i < nuindexes; i += 3) ip[i] = offsets.vertices;
	
	ip[nuindexes - 1] = offsets.vertices + 1;
	
	vec2 sum;
	f32 subdivisions = f32(subdivisions_int);
	for(u32 i = 1; i < subdivisions_int; i++){
		f32 a1 = (f32(i) * M_2PI) / subdivisions;
		vec2 offset(radius * cosf(a1), radius * sinf(a1));
		vec2 point = pos + offset;
		
		vp[i+1].pos = point; vp[i+1].uv = { 0,0 }; vp[i+1].color = col;
		
		u32 ipidx = 3 * i - 1;
		ip[ipidx] = ip[ipidx + 2] = offsets.vertices + i + 1;
	}
	
	return render_make_filledcircle_counts(subdivisions_int);
}

RenderDrawCounts
render_make_text(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, str8 text, Font* font, vec2 pos, color color, vec2 scale){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	RenderDrawCounts sum;
	switch (font->type){
		//// BDF (and NULL) font rendering ////
		case FontType_BDF: case FontType_NONE:{
			u32 codepoint;
			str8 remaining = text;
			u32 i = 0;
			while(remaining && (codepoint = str8_advance(&remaining).codepoint)){
				u32     col = color.rgba;
				Vertex2* vp = putverts + offsets.vertices + 4 * i;
				u32*     ip = putindices + offsets.indices + 6 * i;
				
				f32 w = font->max_width * scale.x;
				f32 h = font->max_height * scale.y;
				f32 dy = 1.f / (f32)font->count;
				
				f32 idx = f32(codepoint - 32);
				f32 topoff = (idx * dy) + font->uv_yoffset;
				f32 botoff = topoff + dy;
				
				ip[0] = offsets.vertices+4*i; ip[1] = offsets.vertices+4*i + 1; ip[2] = offsets.vertices+4*i + 2;
				ip[3] = offsets.vertices+4*i; ip[4] = offsets.vertices+4*i + 2; ip[5] = offsets.vertices+4*i + 3;
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
				Vertex2* vp = putverts + offsets.vertices + 4 * i;
				u32*     ip = putindices + offsets.indices + 6 * i;
				aligned_quad q = font_aligned_quad(font, codepoint, &pos, scale);
				
				ip[0] = offsets.vertices+4*i; ip[1] = offsets.vertices+4*i + 1; ip[2] = offsets.vertices+4*i + 2;
				ip[3] = offsets.vertices+4*i; ip[4] = offsets.vertices+4*i + 2; ip[5] = offsets.vertices+4*i + 3;
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

RenderDrawCounts 
render_make_texture(Vertex2* putverts, u32* putindices, RenderDrawCounts offsets, Texture* texture, vec2 p0, vec2 p1, vec2 p2, vec2 p3, f32 alpha, b32 flipx = 0, b32 flipy = 0){DPZoneScoped;
	Assert(putverts && putindices);
	if(!alpha) return{0,0};
	
	u32     col = PackColorU32(255,255,255,255.f * alpha);
	Vertex2* vp = putverts + offsets.vertices;
	u32*     ip = putindices + offsets.indices;
	
	ip[0] = offsets.vertices; ip[1] = offsets.vertices + 1; ip[2] = offsets.vertices + 2;
	ip[3] = offsets.vertices; ip[4] = offsets.vertices + 2; ip[5] = offsets.vertices + 3;
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
//// @render_shared_other
void
render_display_stats(){
    using namespace UI;
    BeginRow(str8_lit("renderstatsaligned"), 2, 0, UIRowFlags_AutoSize);{
        RowSetupColumnAlignments({{0,0.5},{1,0.5}});
        TextF(str8_lit("total triangles: %d"), renderStats.totalTriangles);
        TextF(str8_lit("total vertices: %d"),  renderStats.totalVertices);
        TextF(str8_lit("total indices: %d"),   renderStats.totalIndices);
        TextF(str8_lit("drawn triangles: %d"), renderStats.drawnTriangles);
        TextF(str8_lit("drawn indicies: %d"),  renderStats.drawnIndices);
        TextF(str8_lit("render time: %g"),     renderStats.renderTimeMS);
    }EndRow();
}


#endif //DESHI_IMPLEMENTATION