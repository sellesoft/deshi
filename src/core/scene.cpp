

SceneGlobal deshi__g_scene;
SceneGlobal* g_scene;

void 
scene_init() {
	*g_scene = {};
	memory_pool_init(g_scene->pools.camera, 4);
}

void
scene_render() {
	auto win   = g_scene->active.window;
	auto cam   = g_scene->active.camera;
	auto frame = render_current_present_frame_of_window(win);
	auto pass  = frame->render_pass;


}

//-////////////////////////////////////////////////////////////////////////////////////////////
// @camera

Camera*
scene_camera_create() {
	auto out = memory_pool_push(g_scene->pools.camera);
	return out;
}

void
scene_camera_update_view(Camera* camera) {
	camera->right = camera->forward.cross(vec3::UP).normalized();
	camera->up = camera->forward.cross(camera->right).normalized();
	camera->view = Math::LookAtMatrix(camera->position, camera->position + camera->forward);
}

void
scene_camera_update_perspective_projection(Camera* camera, u32 width, u32 height, f32 fov, f32 near_z, f32 far_z) {
	camera->proj = Math::PerspectiveProjectionMatrix(width, height, fov, near_z, far_z);
}

void
scene_camera_update_orthographic_projection(Camera* camera, f32 right, f32 left, f32 top, f32 bottom, f32 far, f32 near) {
	f32 A =  2 / (right - left),
		B =  2 / (top - bottom),
		C = -2 / (far - near),
		D = -(right + left) / (right - left),
		E = -(top + bottom) / (top - bottom),
		F = -(far + near) / (far - near);
	camera->proj = mat4(
		A, 0, 0, D,
		0, B, 0, E,
		0, 0, C, F,
		0, 0, 0, 1
	);
}

void
scene_camera_destroy(Camera* camera) {
	memory_pool_delete(g_scene->pools.camera, camera);
}

void
scene_camera_draw_frustrum(Camera* camera) {
	// TODO(sushi) implement by transforming 4 points out based on the proj and view matrices
	NotImplemented;
}

//-////////////////////////////////////////////////////////////////////////////////////////////
// @model

void 
scene_draw_model(Model* model, mat4 transform) {
	auto cmd = array_push(g_scene->model_draw_commands);
	cmd->model = model;
	cmd->transform = transform;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @voxel

#define VoxelError(...) LogE("scene.voxels", __FUNCTION__, "(): ", __VA_ARGS__)
#define VoxelWarning(...) LogW("scene.voxels", __FUNCTION__, "(): ", __VA_ARGS__)

namespace __deshi_voxels {

SceneVoxelChunk* chunk_pool;
array<SceneVoxelType> types;
u32 size;

// stupid people polluting my damn namespace
#undef index
FORCE_INLINE SceneVoxel* 
index(SceneVoxelChunk* chunk, vec3i pos) { 
	if(pos.x > chunk->dimensions || pos.y > chunk->dimensions || pos.z > chunk->dimensions) return 0;
	return chunk->voxels[pos.z * chunk->dimensions * chunk->dimensions + pos.y * chunk->dimensions + pos.x]; 
}

FORCE_INLINE SceneVoxel* right(SceneVoxelChunk* chunk, vec3i pos)   { return index(chunk, vec3i{pos.x + 1, pos.y, pos.z}); }
FORCE_INLINE SceneVoxel* left(SceneVoxelChunk* chunk, vec3i pos)    { return index(chunk, vec3i{pos.x - 1, pos.y, pos.z}); }
FORCE_INLINE SceneVoxel* above(SceneVoxelChunk* chunk, vec3i pos)   { return index(chunk, vec3i{pos.x, pos.y + 1, pos.z}); }
FORCE_INLINE SceneVoxel* below(SceneVoxelChunk* chunk, vec3i pos)   { return index(chunk, vec3i{pos.x, pos.y - 1, pos.z}); }
FORCE_INLINE SceneVoxel* forward(SceneVoxelChunk* chunk, vec3i pos) { return index(chunk, vec3i{pos.x, pos.y, pos.z + 1}); }
FORCE_INLINE SceneVoxel* behind(SceneVoxelChunk* chunk, vec3i pos)  { return index(chunk, vec3i{pos.x, pos.y, pos.z - 1}); }

} // namespace __deshi_voxels
#define voxel __deshi_voxels

void
scene_voxel_init(SceneVoxelType* types, u32 voxel_size) {
	voxel::types = array_from(types);
	if(!voxel::types.count()) {
		LogE("scene.voxels", "given types array is empty");
		return;
	}

	memory_pool_init(voxel::chunk_pool, 128);
	for_pool(voxel::chunk_pool) it->hidden = true;
	
	voxel::size = voxel_size;
}

SceneVoxelChunk*
scene_voxel_chunk_create(vec3 pos, vec3 rot, u32 dimensions, SceneVoxel* voxels) {
	if(!dimensions) {
		VoxelError("given dimensions is zero. Chunks are statically sized, so this does not make sense.");
		return 0;
	}

	const u64 dimensions_cubed = dimensions * dimensions * dimensions;
	
	auto chunk = memory_pool_push(voxel::chunk_pool);
	chunk->position = pos;
	chunk->rotation = rot;
	chunk->dimensions = dimensions;
	chunk->modified = false;
	chunk->hidden = false;
}






#undef voxels
