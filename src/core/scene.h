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
StartLinkageC();

// Global scene object used for keep track of various things.
struct SceneGlobal {
	Window* window; // window we are actively drawing to

	struct {
		Camera* camera;
	} pools;

	struct {
		RenderBuffer* vertex_buffer;
		RenderBuffer* index_buffer;
	} temp;
} g_scene;

// Initialize the scene module. 
void scene_init();
void scene_render(Window* window);

// A view into a scene.
// Variables outside of 'internal' are those set by the user and are
// used to determine how to render the scene from the camera's POV when 
// it is provided in other scene_* functions.
// Note that a camera is not updated when it is used, only when 
// scene_camera_update is called with it.
struct Camera {
	vec3 position;
	vec3 rotation;
	
	// the distance from the camera's position to the screen plane
	f32 near_z;
	// the max render distance
	f32 far_z;
	// horizontal field of view
	f32 fov;
	
	// true to use orthographic projection rather than perspective
	b32 orthographic;

	// information regarding the camera's 
	// orientation
	vec3 forward;
	vec3 right;
	vec3 up;

	mat4 proj;
	mat4 view;
};

Camera* scene_camera_create();
void scene_camera_update(Camera* camera);
void scene_camera_destroy(Camera* camera);
void scene_camera_draw_frustrum(Camera* camera);

struct SceneDrawModel {
	Model*  model;
	mat4    transform;
};

// Draws 'model' with 'transform' as viewed through 'camera'.
void scene_draw_model(Model* model, mat4 transform);

EndLinkageC();
#endif // DESHI_SCENE_H
