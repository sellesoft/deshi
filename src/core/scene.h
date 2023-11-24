/*
	deshi scene module

	An api for typical operations used to interact with a 3D scene.
	While asset's purpose is to provide an api for un/loading and storing 
	various things w/o having to interact with the render, scene's purpose 
	is to provide an api for actually rendering those things w/o having to 
	interact with the render api directly.

	scene_draw_* functions record a thing to draw, but nothing is actually rendered
	until scene_render() is called.
*/

#ifndef DESHI_SCENE_H
#define DESHI_SCENE_H

struct Camera;
struct SceneDrawModel;
struct SceneVoxelType;
StartLinkageC();


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @scene


// Global scene object used for keep track of various things.
typedef struct SceneGlobal {

	// Collection of things allocated and managed
	// by the scene.
	struct {
		Camera* camera;
	} pools;

	// Collection of active elements of the scene 
	// that determine how we render and where we 
	// render to.
	struct {
		Window* window;
		Camera* camera;
	} active;

	// Array of draw commands created from scene_draw_model
	// that will be executed and then cleared next time scene_render
	// is called
	SceneDrawModel* model_draw_commands;	

	struct {
		GraphicsRenderPass* render_pass;

		struct {
			GraphicsBuffer* vertex_buffer;
			u32 vertex_count;
			GraphicsBuffer* index_buffer;
			u32 index_count;
			
			GraphicsPipeline* pipeline;
		} filled, wireframe;

		struct {
			mat4 view;
			mat4 proj;
		} camera_ubo;

		GraphicsBuffer* camera_buffer;
		GraphicsDescriptorSet* descriptor_set;
	} temp;
} SceneGlobal;

extern SceneGlobal* g_scene;

// Initialize the scene module. 
void scene_init();

// Renders the scene to the active window using the active camera
void scene_render();

// Sets the window scene will draw to in the next call to scene_render
void scene_set_active_window(Window* window);

// Sets the camera to render the scene from next time scene_render is called
void scene_set_active_camera(Camera* camera);


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @camera


// A view into a scene.
// Cameras are only updated through the scene_camera_update
// function, rather than automatically being updated when it is used
// so that we may minimize the amount of work we do with the camera.
// proj and view are not meant to be modified by the user but it is 
// fine if they are manually set.
struct Camera {
	vec3 position;
	vec3 rotation;
	
	// determines the direction in which the camera is facing,
	// set by user 
	vec3 forward;
	// these are calculated internally by scene_camera_update_view
	vec3 right;
	vec3 up;

	mat4 proj;
	mat4 view;
};

// Allocates and returns a pointer to a camera
Camera* scene_camera_create();

// Updates a camera's projection and view matrices based on its
// internal properties
void scene_camera_update_view(Camera* camera);

// Updates the camera to use perspective projection
void scene_camera_update_perspective_projection(Camera* camera, u32 width, u32 height, f32 fov, f32 near_z, f32 far_z);

// Updates the camera to use orthographic projection
void scene_camera_update_orthographic_projection(Camera* camera, vec2 x_bounds, vec2 y_bounds, f32 z_bounds);

// Deallocates the given camera
void scene_camera_destroy(Camera* camera);

// Draws the given camera's frustrum using graphics temp drawing
void scene_camera_draw_frustrum(Camera* camera);


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @model


typedef struct SceneDrawModel {
	Model*  model;
	mat4*   transform;
} SceneDrawModel;

// Draws 'model' with 'transform' the next time scene_render() is called.
void scene_draw_model(Model* model, mat4* transform);


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @temp


typedef u32 RenderTempIndex;  
typedef struct RenderTempVertex {
	vec3 pos;
	u32  color;
} RenderTempVertex;

// Initializes the filled temp and wireframe temp buffers with the given amount of max vertexes.
// The max amount of indexes will be 3 * max_vertexes.
void render_temp_init(Window* window, u32 max_vertexes);
void render_temp_clear();
void render_temp_line(vec3 start, vec3 end, color c);
void render_temp_triangle(vec3 p0, vec3 p1, vec3 p2, color c);
void render_temp_triangle_filled(vec3 p0, vec3 p1, vec3 p2, color c);
void render_temp_quad(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c);
void render_temp_quad_filled(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c);
void render_temp_poly(vec3* points, color c);
void render_temp_poly_filled(vec3* points, color c);
void render_temp_circle(vec3 pos, vec3 rot, f32 radius, u32 subdivisions, color c);
void render_temp_circle_filled(vec3 pos, vec3 rot, f32 radius, u32 subdivisions, color c);
void render_temp_box(mat4 transform, color c);
void render_temp_box_filled(mat4 transform, color c);
void render_temp_sphere(vec3 pos, f32 radius, u32 segments, u32 rings, color c);
void render_temp_sphere_filled(vec3 pos, f32 radius, u32 segments, u32 rings, color c);
void render_temp_frustrum(vec3 position, vec3 target, f32 aspect_ratio, f32 fov, f32 near_z, f32 far_z, color c);

#if COMPILER_FEATURE_CPP
namespace render::temp {

FORCE_INLINE void init(Window* window, u32 max_vertexes){ render_temp_init(window, max_vertexes); }
FORCE_INLINE void clear(){ render_temp_clear(); }
FORCE_INLINE void line(vec3 start, vec3 end, color c = Color_White){ render_temp_line(start, end, c); };
FORCE_INLINE void triangle(vec3 p0, vec3 p1, vec3 p2, color c = Color_White, b32 filled = false){ 
	if(filled) render_temp_triangle_filled(p0,p1,p2,c);
	else render_temp_triangle(p0,p1,p2,c);
};
FORCE_INLINE void quad(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c = Color_White, b32 filled = false){
	if(filled) render_temp_quad_filled(p0,p1,p2,p3,c);
	else render_temp_quad(p0,p1,p2,p3,c);
}
FORCE_INLINE void poly(vec3* points, color c = Color_White, b32 filled = false){
	if(filled) render_temp_poly_filled(points, c);
	else render_temp_poly(points, c);
}
FORCE_INLINE void circle(vec3 pos, vec3 rot, f32 radius, u32 subdivisions, color c = Color_White, b32 filled = false){
	if(filled) render_temp_circle_filled(pos, rot, radius, subdivisions, c);
	else render_temp_circle(pos, rot, radius, subdivisions, c);
}
FORCE_INLINE void box(mat4 transform, color c = Color_White, b32 filled = false){
	if(filled) render_temp_box_filled(transform, c);
	else render_temp_box(transform, c);
}
FORCE_INLINE void sphere(vec3 pos, f32 radius, u32 segments = 3, u32 rings = 3, color c = Color_White, b32 filled = false){
	if(filled) render_temp_sphere_filled(pos, radius, segments, rings, c);
	else render_temp_sphere(pos, radius, segments, rings, c);
}
FORCE_INLINE void frustrum(vec3 position, vec3 target, f32 aspect_ratio, f32 fov, f32 near_z, f32 far_z, color c = Color_White){
	render_temp_frustrum(position, target, aspect_ratio, fov, near_z, far_z, c);
}
	
} // namespace render::temp
#endif


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @voxel


typedef struct SceneVoxelType {
	color col;
	
	// TODO(delle) shape
} SceneVoxelType;

typedef struct SceneVoxel {
	// index into 
	u16 type;
	// offset into chunk local space
	u16 x, y, z;
} SceneVoxel;

// A collection of voxels that get turned into a mesh
// for rendering.
typedef struct SceneVoxelChunk {
	vec3 position;
	vec3 rotation;
	u32  dimensions;

	b32 modified;
	b32 hidden;

	SceneVoxel** voxels;
	u64 voxel_count;

	RenderBuffer* vertex_buffer;
	u64 vertex_count;
	RenderBuffer* index_buffer;
	u64 index_count;
} SceneVoxelChunk;

// Initializes the voxel renderer using an array of voxel types.
// 'voxel_size' determines the size of a voxel in the world.
void scene_voxel_init(SceneVoxelType* types, u32 voxel_size);

// Creates a new voxel chunk for voxels.
// All voxels are transformed into global space by 'pos' and 'rot'.
// 'chunk_size' determines the number of voxels there may be along each dimension.
SceneVoxelChunk* scene_voxel_chunk_create(vec3 pos, vec3 rot, u32 chunk_size, RenderVoxel* voxels);

// Destroys the given chunk freeing all data it owns.
void scene_voxel_chunk_destroy(SceneVoxelChunk* chunk);

EndLinkageC();
#endif // DESHI_SCENE_H
