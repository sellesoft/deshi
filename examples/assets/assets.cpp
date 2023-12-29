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
#include "core/graphics.h"
#include "core/assets.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/ui_widgets.h"
#include "core/window.h"
#include "math/math.h"

// TODO(sushi) remove when assets no longer uses this internally
#include "external/stb/stb_ds.h"

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
	graphics_init(win);
	assets_init(win);
	cmd_init();

	Shader* fancy_null_fragment = assets_shader_load_from_path(str8l("null_fancy.frag"), str8l("null_fancy.frag"), ShaderType_Fragment);

	ShaderStages stages = {0};
	stages.vertex =	assets_shader_null_vertex();
	stages.fragment = fancy_null_fragment;
	array_init_with_elements<ShaderResourceType>(stages.fragment->resources, {
			ShaderResourceType_Texture,
			ShaderResourceType_UBO}, deshi_allocator);

	UBO* ubo_resource0 = assets_ubo_create(sizeof(ubo));
	UBO* ubo_resource1 = assets_ubo_create(sizeof(ubo));

	Texture* alex = assets_texture_create_from_path_simple(str8l("alex"), str8l("alex.png"));

	auto resources = array<ShaderResource>::create_with_count(2, deshi_temp_allocator);
	resources[0].type = ShaderResourceType_Texture;
	resources[0].texture = alex;
	resources[1].type = ShaderResourceType_UBO;
	resources[1].ubo = ubo_resource0;

	Material* fancy_null0 = assets_material_create(str8l("fancy null"), stages, resources.ptr);

	resources[1].ubo = ubo_resource1;
	Material* fancy_null1 = assets_material_duplicate(str8l("fancy null 2"), fancy_null0, resources.ptr);

	Model* box = assets_model_create_from_obj(str8l("box.obj"), 0);

	auto view = Math::LookAtMatrix({0,0,0}, {0,0,1}).Inverse();
	assets_update_camera_view(&view);
	auto proj = Math::PerspectiveProjectionMatrix(win->width, win->height, 90, 0.1, 1000);
	assets_update_camera_projection(&proj);
	auto position = Vec4(0,0,0,1);
	assets_update_camera_position(&position);

	cmd_run(str8l("mat_list"));
	cmd_run(str8l("shader_list"));
	cmd_run(str8l("texture_list"));

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

		
		{ using namespace graphics::cmd;
			begin_render_pass(win, graphics_render_pass_of_window_presentation_frames(win), graphics_current_present_frame_of_window(win));
		
			box->batch_array[0].material = fancy_null0;

			forI(array_count(box->batch_array)) {
				auto b = box->batch_array[i];
				bind_pipeline(win, b.material->pipeline);
				set_viewport(win, vec2::ZERO, Vec2(win->width, win->height));
				set_scissor(win, vec2::ZERO, Vec2(win->width, win->height));
				bind_descriptor_set(win, 0, g_assets->view_proj_ubo);
				push_constant(win, GraphicsShaderStage_Vertex, &transform0, 0, sizeof(mat4));
				bind_vertex_buffer(win, box->mesh->vertex_buffer);
				bind_index_buffer(win, box->mesh->index_buffer);
				bind_descriptor_set(win, 1, b.material->descriptor_set);
				draw_indexed(win, b.index_count, b.index_offset, 0);
			}

			box->batch_array[0].material = fancy_null1;

			forI(array_count(box->batch_array)) {
				auto b = box->batch_array[i];
				bind_pipeline(win, b.material->pipeline);
				set_viewport(win, vec2::ZERO, Vec2(win->width, win->height));
				set_scissor(win, vec2::ZERO, Vec2(win->width, win->height));
				bind_descriptor_set(win, 0, g_assets->view_proj_ubo);
				push_constant(win, GraphicsShaderStage_Vertex, &transform1, 0, sizeof(mat4));
				bind_vertex_buffer(win, box->mesh->vertex_buffer);
				bind_index_buffer(win, box->mesh->index_buffer);
				bind_descriptor_set(win, 1, b.material->descriptor_set);
				draw_indexed(win, b.index_count, b.index_offset, 0);
			}

			end_render_pass(win);
		}
		
		graphics_update(win);
	}
}

