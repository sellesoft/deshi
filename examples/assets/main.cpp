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
#include "core/render.h"
#include "core/assets.h"
#include "core/threading.h"
#include "core/camera.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/ui_widgets.h"
#include "core/window.h"
#include "math/math.h"

struct {
	f32 time;
	vec4 mix;
} ubo;

int main() {
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	Window* win = window_create(str8l("assets example"));
	window_show(win);
	render_init_x(win);
	assets_init_x(win);

	ShaderStages stages = {0};
	stages.vertex.filename = str8l("null.vert");
	stages.fragment.filename = str8l("null_fancy.frag");
	array_init(stages.fragment.resources, 2, deshi_allocator);
	array_count(stages.fragment.resources) = 2;
	stages.fragment.resources[0] = ShaderResourceType_Texture;
	stages.fragment.resources[1] = ShaderResourceType_UBO;

	UBO* ubo_resource0 = assets_ubo_create(sizeof(ubo));
	UBO* ubo_resource1 = assets_ubo_create(sizeof(ubo));

	Texture* alex = assets_texture_create_from_file_simple(str8l("alex.png"));

	ShaderResource* resources;
	array_init(resources, 2, deshi_temp_allocator);
	array_count(resources) = 2;
	resources[0].type = ShaderResourceType_Texture;
	resources[0].texture = alex;
	resources[1].type = ShaderResourceType_UBO;
	resources[1].ubo = ubo_resource0;

	Material* fancy_null0 = assets_material_create_x(str8l("fancy null"), stages, resources);

	resources[1].ubo = ubo_resource1;
	Material* fancy_null1 = assets_material_duplicate(str8l("fancy null 2"), fancy_null0, resources);

	Model* box = assets_model_create_from_obj(str8l("data/models/box.obj"), 0);

	auto view = Math::LookAtMatrix({0,0,0}, {0,0,1}).Inverse();
	assets_update_camera_view(&view);
	auto proj = Camera::MakePerspectiveProjectionMatrix(win->width, win->height, 90, 1000, 0.1);
	assets_update_camera_projection(&proj);

	while(platform_update()) {	
		mat4 transform0 = mat4::TransformationMatrix(
			{1.5,0,3},
			{0,45*f32(g_time->totalTime)/3000.f,0},
			{1,1,1});

		mat4 transform1 = mat4::TransformationMatrix(
			{-1.5,0,3},
			{0,45*f32(g_time->totalTime)/3000.f,0},
			{1,1,1});

		ubo.time = g_time->totalTime;
		ubo.mix = Vec4(1, 0, 0, 1);
		assets_ubo_update(ubo_resource0, &ubo);
		ubo.mix = Vec4(0, 1, 0, 1);
		assets_ubo_update(ubo_resource1, &ubo);

		box->batch_array[0].material = fancy_null0;
		assets_model_render(win, box, &transform0);
		box->batch_array[0].material = fancy_null1;
		assets_model_render(win, box, &transform1);
		
		render_update_x(win);
	}
}

