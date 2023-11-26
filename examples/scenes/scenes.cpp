//// deshi memory ////
#define KIGU_ARRAY_ALLOCATOR deshi_allocator
#define KIGU_UNICODE_ALLOCATOR deshi_allocator
#define KIGU_STRING_ALLOCATOR deshi_temp_allocator
#include "core/memory.h"

//// kigu includes ////
#include "kigu/profiling.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "kigu/common.h"
#include "kigu/map.h"
#include "kigu/unicode.h"
#include "kigu/string_utils.h"

//// deshi includes ////
#define DESHI_DISABLE_IMGUI
#include "core/commands.h"
#include "core/console.h"
#include "core/file.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/platform.h"
#include "core/assets.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/ui_widgets.h"
#include "core/window.h"
#include "core/render.h"
#include "math/math.h"

int main() {
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	Window* win = window_create(str8l("scenes example"));
	window_show(win);
	graphics_init(win);
	assets_init(win);
	render_init();

	render_set_active_window(win);

	Texture* alex = assets_texture_create_from_file_simple(str8l("alex.png"));
	Model* box = assets_model_create_from_obj(str8l("data/models/box.obj"), 0);

	ShaderStages stages = {0};
	stages.vertex = assets_shader_load_from_file(str8l("dither.vert"), ShaderType_Vertex);
	stages.fragment = assets_shader_load_from_file(str8l("dither.frag"), ShaderType_Fragment);
	array_init_with_elements<ShaderResourceType>(stages.fragment->resources, {
				ShaderResourceType_Texture,
				ShaderResourceType_UBO,
			});

	struct {
		f32 time;
	} ubo;

	UBO* ubo_resource = assets_ubo_create(sizeof(ubo));

	auto resources = array<ShaderResource>::create_with_count(2, deshi_temp_allocator);
	resources[0].type = ShaderResourceType_Texture;
	resources[0].texture = alex;
	resources[1].type = ShaderResourceType_UBO;
	resources[1].ubo = ubo_resource;

	Material* dithermat = assets_material_create(str8l("dither"), stages, resources.ptr);
	
	box->batch_array[0].material = dithermat;

	Camera* camera = render_camera_create();
	camera->position = Vec3(0,0,0);
	camera->rotation = Vec3(0,0,0);
	camera->forward = Vec3(0,0,1);
	render_camera_update_perspective_projection(camera, win->width, win->height, 90, 0.1, 1000);
	render_camera_update_view(camera);
	render_set_active_camera(camera);

	mat4 transform = mat4::TransformationMatrix(
			Vec3(0, 0, 2),
			Vec3(0, 0, 0),
			Vec3(1, 1, 1));

	while(platform_update()) {
		transform = mat4::TransformationMatrix(Vec3(0, 0, 3), Vec3(0, g_time->totalTime/3000.f*45, 0), vec3::ONE);
		ubo.time = g_time->totalTime;
		assets_ubo_update(ubo_resource, &ubo);
		render_draw_model(box, &transform);
		render_update();
		graphics_update(win);
	}
}
