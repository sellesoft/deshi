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
#include "core/scene.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/ui_widgets.h"
#include "core/window.h"
#include "math/math.h"

int main() {
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	Window* win = window_create(str8l("scenes example"));
	window_show(win);
	render_init_x(win);
	assets_init();
	scene_init();

	
	// Our scene needs a camera, so we'll create
	// one, set some settings, then ask the scene 
	// module to update it.
	Camera* camera = scene_camera_create();
	camera->position = Vec3(0,0,0);
	camera->rotation = Vec3(0,0,0);
	camera->near_z = 0.1;
	camera->far_z = 1000;
	camera->fov = 90;
	scene_camera_update(camera);

	{ using namespace render::temp;

		line(Vec3(-1, 0, 2), Vec3(1, 0, 2));

	}

	while(platform_update()) {
		render_update_x(win);
	}

}
