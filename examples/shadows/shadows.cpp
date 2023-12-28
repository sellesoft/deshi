/*
	Setting up shadow mapping using deshi's render api. 
	This is largely based on Sascha Willems' shadowmapping example for Vulkan:
		https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmapping/shadowmapping.cpp
*/

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
	Window* win = window_create(str8l("graphics_api"));
	window_show(win);
	graphics_init(win);
	assets_init(win);
		
	struct {
		GraphicsRenderPass* render_pass;
		// the image we will be rendering to
		GraphicsImage* image;
		GraphicsImageView* image_view;
		GraphicsSampler* sampler;

		GraphicsFramebuffer* frame;

		u32 width, height;

		f32 light_fov;

		f32 z_near, z_far;
	} offscreen;

	offscreen.width  = 1028;
	offscreen.height = 1028;
	offscreen.light_fov = 160.f;
	offscreen.z_far = 1000;
	offscreen.z_near = 0.1;

	struct {
		vec3 pos;
		vec3 rot;

		f32 near_z;
		f32 far_z;

		mat4 proj;
		mat4 view;

		vec3 forward,
			 right,
			 up;
	} camera;

	struct {
		vec3 light_pos;
	} scene;

	struct {
		mat4 proj;
		mat4 view;
		mat4 light_proj;
		mat4 light_view;
		vec4 light_pos;
		f32 z_near;
		f32 z_far;
		f32 time;
	} ubo_scene;

	scene.light_pos = Vec3(0, -10, 0);
	
	struct {
		mat4 proj;
		mat4 view;
	} ubo_offscreen;

	GraphicsBuffer* scene_ubo_buffer = graphics_buffer_create(
			0, sizeof(ubo_scene),
			GraphicsBufferUsage_UniformBuffer,
			GraphicsMemoryPropertyFlag_HostVisible | 
			GraphicsMemoryPropertyFlag_HostCoherent,
			GraphicsMemoryMapping_Persistent);

	auto update_scene_ubo = [&]() {
		ubo_scene.          proj = camera.proj;
		ubo_scene.          view = camera.view;
		ubo_scene.     light_pos = scene.light_pos.toVec4();
		ubo_scene.    light_view = ubo_offscreen.view;
		ubo_scene.    light_proj = ubo_offscreen.proj;
		ubo_scene.        z_near = camera.near_z;
		ubo_scene.         z_far = camera.far_z;
		ubo_scene.time = g_time->totalTime/1000.f;
		CopyMemory(graphics_buffer_mapped_data(scene_ubo_buffer), &ubo_scene, sizeof(ubo_scene));
	};
	
	GraphicsBuffer* offscreen_ubo_buffer = graphics_buffer_create(
			0, sizeof(ubo_offscreen),
			GraphicsBufferUsage_UniformBuffer,
			GraphicsMemoryPropertyFlag_HostVisible | 
			GraphicsMemoryPropertyFlag_HostCoherent,
			GraphicsMemoryMapping_Persistent);

	auto update_offscreen_ubo = [&]() {
		ubo_offscreen.proj = Math::PerspectiveProjectionMatrix(
				offscreen.width, offscreen.height,
				offscreen.light_fov,
				offscreen.z_near, offscreen.z_far);
		ubo_offscreen.view = Math::LookAtMatrix(ubo_scene.light_pos.toVec3(), Vec3(0,0,0)).Inverse();
		CopyMemory(graphics_buffer_mapped_data(offscreen_ubo_buffer), &ubo_offscreen, sizeof(ubo_offscreen));
	};

	auto update_light = [&]() {
		auto time = g_time->totalTime/1000.f;
		ubo_scene.light_pos = Vec4(
			0,
			-10,
			0,
			1
		);
	};

	// We only need a depth attachment as we just want
	// the distance of pixels from the light
	GraphicsRenderPassAttachment depth_attachment = {};
	depth_attachment.format = GraphicsFormat_Depth16_UNorm;
	depth_attachment.load_op = GraphicsLoadOp_Clear;
	// We want to store the result of this render pass so we may use it for the final frame
	depth_attachment.store_op = GraphicsStoreOp_Store;
	depth_attachment.stencil_load_op = GraphicsLoadOp_Dont_Care;
	depth_attachment.stencil_store_op = GraphicsStoreOp_Dont_Care;
	depth_attachment.initial_layout = GraphicsImageLayout_Undefined;
	depth_attachment.final_layout = GraphicsImageLayout_Depth_Stencil_Read_Only_Optimal;

	offscreen.render_pass = graphics_render_pass_allocate();
	offscreen.render_pass->debug_name = str8l("offscreen render pass");
	offscreen.render_pass->use_depth_attachment = true;
	offscreen.render_pass->depth_attachment = depth_attachment;
	offscreen.render_pass->depth_clear_values = {1.f, 0};
	graphics_render_pass_update(offscreen.render_pass);

	offscreen.image = graphics_image_allocate();
	offscreen.image->debug_name = str8l("offscreen image");
	offscreen.image->format = GraphicsFormat_Depth16_UNorm;
	offscreen.image->extent.x = offscreen.width;
	offscreen.image->extent.y = offscreen.height;
	offscreen.image->extent.z = 1;
	offscreen.image->samples = GraphicsSampleCount_1;
	offscreen.image->linear_tiling = false;
	offscreen.image->usage = GraphicsImageUsage_Depth_Stencil_Attachment | GraphicsImageUsage_Sampled;
	graphics_image_update(offscreen.image);

	offscreen.image_view = graphics_image_view_allocate();
	offscreen.image_view->debug_name = str8l("offscreen image view");
	offscreen.image_view->image = offscreen.image;
	offscreen.image_view->format = offscreen.image->format;
	offscreen.image_view->aspect_flags = GraphicsImageViewAspectFlags_Depth;
	graphics_image_view_update(offscreen.image_view);

	offscreen.sampler = graphics_sampler_allocate();
	offscreen.sampler->mag_filter =
	offscreen.sampler->min_filter = GraphicsFilter_Nearest;
	offscreen.sampler->address_mode_u = 
	offscreen.sampler->address_mode_v = 
	offscreen.sampler->address_mode_w = GraphicsSamplerAddressMode_Clamp_To_Border;
	offscreen.sampler->border_color = Color_Red;
	graphics_sampler_update(offscreen.sampler);

	offscreen.frame = graphics_framebuffer_allocate();
	offscreen.frame->debug_name = str8l("offscreen frame");
	offscreen.frame->render_pass = offscreen.render_pass;
	offscreen.frame->width = offscreen.width;
	offscreen.frame->height = offscreen.height;
	offscreen.frame->depth_image_view = offscreen.image_view;
	graphics_framebuffer_update(offscreen.frame);

	GraphicsDescriptorSetLayout* descriptor_layout = graphics_descriptor_set_layout_allocate();
	descriptor_layout->bindings = array<GraphicsDescriptorSetLayoutBinding>::create_with_count(2, deshi_temp_allocator).ptr;
	descriptor_layout->bindings[0].n = 0;
	descriptor_layout->bindings[0].type = GraphicsDescriptorType_Uniform_Buffer;
	descriptor_layout->bindings[0].shader_stages = GraphicsShaderStage_Vertex | GraphicsShaderStage_Fragment;
	descriptor_layout->bindings[1].n = 1;
	descriptor_layout->bindings[1].type = GraphicsDescriptorType_Combined_Image_Sampler;
	descriptor_layout->bindings[1].shader_stages = GraphicsShaderStage_Fragment;
	graphics_descriptor_set_layout_update(descriptor_layout);
	
	GraphicsPushConstant model_push_constant = {};
	model_push_constant.shader_stages = GraphicsShaderStage_Vertex;
	model_push_constant.size = sizeof(mat4) + sizeof(vec3);
	model_push_constant.offset = 0;

	GraphicsPipelineLayout* pipeline_layout = graphics_pipeline_layout_allocate();
	pipeline_layout->debug_name = str8l("shadows shared pipeline layout");
	array_init_with_elements(pipeline_layout->descriptor_layouts, {descriptor_layout}, deshi_allocator);
	array_init_with_elements(pipeline_layout->push_constants, {model_push_constant}, deshi_allocator);
	graphics_pipeline_layout_update(pipeline_layout);

	GraphicsPipeline* shadow_debug_quad_pipeline = graphics_pipeline_allocate();
	GraphicsPipeline* scene_pipeline = 0;
	GraphicsPipeline* offscreen_pipeline = 0;

	auto pipeline = shadow_debug_quad_pipeline;
	pipeline->layout = pipeline_layout;

	// setup settings shared by all three pipelines
	pipeline->            front_face = GraphicsFrontFace_CCW;
	pipeline->               culling = GraphicsPipelineCulling_Back;
	pipeline->          polygon_mode = GraphicsPolygonMode_Fill;
	pipeline->            depth_test = true;
	pipeline->          depth_writes = true;
	pipeline->      depth_compare_op = GraphicsCompareOp_Less_Or_Equal;
	pipeline->            depth_bias = true;
	pipeline->            line_width = 1.f;
	pipeline->           color_blend = true;
	pipeline->        color_blend_op = GraphicsBlendOp_Add;
	pipeline->color_src_blend_factor = GraphicsBlendFactor_Source_Alpha;
	pipeline->color_dst_blend_factor = GraphicsBlendFactor_One_Minus_Source_Alpha;
	pipeline->        alpha_blend_op = GraphicsBlendOp_Add;
	pipeline->alpha_src_blend_factor = GraphicsBlendFactor_One_Minus_Source_Alpha;
	pipeline->alpha_dst_blend_factor = GraphicsBlendFactor_Zero;
	pipeline->        blend_constant = color(10,10,10,255);
	pipeline->           render_pass = graphics_render_pass_of_window_presentation_frames(win);

	pipeline->dynamic_depth_bias =
	pipeline->dynamic_viewport   = 
	pipeline->dynamic_scissor    = true;

	scene_pipeline = graphics_pipeline_duplicate(shadow_debug_quad_pipeline);

	// shadow map debug quad display
	shadow_debug_quad_pipeline->culling = GraphicsPipelineCulling_None;
	shadow_debug_quad_pipeline->vertex_shader = graphics_shader_allocate();
	shadow_debug_quad_pipeline->vertex_shader->debug_name = str8l("quad.vert");
	shadow_debug_quad_pipeline->vertex_shader->shader_stage = GraphicsShaderStage_Vertex;
	shadow_debug_quad_pipeline->vertex_shader->source = file_read_simple(str8l("quad.vert"), deshi_temp_allocator);
	graphics_shader_update(shadow_debug_quad_pipeline->vertex_shader);
	shadow_debug_quad_pipeline->fragment_shader = graphics_shader_allocate();
	shadow_debug_quad_pipeline->fragment_shader->debug_name = str8l("quad.frag"); 
	shadow_debug_quad_pipeline->fragment_shader->shader_stage = GraphicsShaderStage_Fragment;
	shadow_debug_quad_pipeline->fragment_shader->source = file_read_simple(str8l("quad.frag"), deshi_temp_allocator);
	graphics_shader_update(shadow_debug_quad_pipeline->fragment_shader);
	
	// the vertex state has nothing 
	graphics_pipeline_update(shadow_debug_quad_pipeline);

	// scene with shadows
	array_init_with_elements(scene_pipeline->vertex_input_bindings, 
			{{0, sizeof(MeshVertex)}}, deshi_allocator);

	array_init_with_elements(scene_pipeline->vertex_input_attributes, {
			{0, 0, GraphicsFormat_R32G32B32_Float,      offsetof(MeshVertex, pos)},
			{1, 0, GraphicsFormat_R32G32_Float,         offsetof(MeshVertex, uv)},
			{2, 0, GraphicsFormat_R8G8B8A8_UNorm,       offsetof(MeshVertex, color)},
			{3, 0, GraphicsFormat_R32G32B32_Float,      offsetof(MeshVertex, normal)},
			}, deshi_allocator);

	offscreen_pipeline = graphics_pipeline_duplicate(scene_pipeline);
	offscreen_pipeline->render_pass = offscreen.render_pass;
	offscreen_pipeline->culling = GraphicsPipelineCulling_Back;

	scene_pipeline->vertex_shader = graphics_shader_allocate();
	scene_pipeline->vertex_shader->debug_name = str8l("scene.vert");
	scene_pipeline->vertex_shader->shader_stage = GraphicsShaderStage_Vertex;
	scene_pipeline->vertex_shader->source = file_read_simple(str8l("scene.vert"), deshi_temp_allocator);
	graphics_shader_update(scene_pipeline->vertex_shader);
	scene_pipeline->fragment_shader = graphics_shader_allocate();
	scene_pipeline->fragment_shader->debug_name = str8l("scene.frag");
	scene_pipeline->fragment_shader->shader_stage = GraphicsShaderStage_Fragment;
	scene_pipeline->fragment_shader->source = file_read_simple(str8l("scene.frag"), deshi_temp_allocator);
	graphics_shader_update(scene_pipeline->fragment_shader);
	
	graphics_pipeline_update(scene_pipeline);

	offscreen_pipeline->vertex_shader = graphics_shader_allocate();
	offscreen_pipeline->vertex_shader->debug_name = str8l("offscreen.vert");
	offscreen_pipeline->vertex_shader->shader_stage = GraphicsShaderStage_Vertex;
	offscreen_pipeline->vertex_shader->source = file_read_simple(str8l("offscreen.vert"), deshi_temp_allocator);
	graphics_shader_update(offscreen_pipeline->vertex_shader);

	graphics_pipeline_update(offscreen_pipeline);

	GraphicsDescriptorSet* debug_descriptor_set = graphics_descriptor_set_allocate();
	array_init_with_elements(debug_descriptor_set->layouts, {descriptor_layout});
	graphics_descriptor_set_update(debug_descriptor_set);

	auto debug_descriptors = array<GraphicsDescriptor>::create(2, deshi_allocator);
	debug_descriptors[0].type = GraphicsDescriptorType_Uniform_Buffer;
	debug_descriptors[0].shader_stages = GraphicsShaderStage_Fragment;
	debug_descriptors[0].ubo = {
		scene_ubo_buffer,
		0,
		sizeof(ubo_scene),
	};

	debug_descriptors[1].type = GraphicsDescriptorType_Combined_Image_Sampler;
	debug_descriptors[1].shader_stages = GraphicsShaderStage_Fragment;
	debug_descriptors[1].image = {
		offscreen.image_view,
		offscreen.sampler,
		GraphicsImageLayout_Depth_Stencil_Read_Only_Optimal,
	};
	debug_descriptor_set->descriptors = debug_descriptors.ptr;
	graphics_descriptor_set_write(debug_descriptor_set);

	GraphicsDescriptorSet* offscreen_descriptor_set = graphics_descriptor_set_allocate();
	array_init_with_elements(offscreen_descriptor_set->layouts, {descriptor_layout});
	graphics_descriptor_set_update(offscreen_descriptor_set);

	auto offscreen_descriptors = array<GraphicsDescriptor>::create(1, deshi_allocator);
	offscreen_descriptors[0].type = GraphicsDescriptorType_Uniform_Buffer;
	offscreen_descriptors[0].shader_stages = GraphicsShaderStage_Vertex;
	offscreen_descriptors[0].ubo = {
		offscreen_ubo_buffer,
		0,
		sizeof(ubo_offscreen),
	};
	offscreen_descriptor_set->descriptors = offscreen_descriptors.ptr;
	graphics_descriptor_set_write(offscreen_descriptor_set);
	
	GraphicsDescriptorSet* scene_descriptor_set = graphics_descriptor_set_allocate(); 
	array_init_with_elements(scene_descriptor_set->layouts, {descriptor_layout});
	graphics_descriptor_set_update(scene_descriptor_set);

	auto scene_descriptors = array<GraphicsDescriptor>::create(2, deshi_allocator);
	scene_descriptors[0].ubo = {
		scene_ubo_buffer,
		0,
		sizeof(ubo_scene),
	};
	scene_descriptors[1].image = {
		offscreen.image_view,
		offscreen.sampler,
		GraphicsImageLayout_Depth_Stencil_Read_Only_Optimal,
	};
	scene_descriptor_set->descriptors = scene_descriptors.ptr;
	graphics_descriptor_set_write(scene_descriptor_set);

	MeshVertex vertices[4] = {
		{{ 0.5f,  0.5f, 0.f}, {1.f, 0.f}, 0, {0.f, 1.f, 0.f}},
		{{-0.5f,  0.5f, 0.f}, {0.f, 0.f}, 0, {0.f, 1.f, 0.f}},
		{{-0.5f, -0.5f, 0.f}, {0.f, 1.f}, 0, {0.f, 1.f, 0.f}},
		{{ 0.5f, -0.5f, 0.f}, {1.f, 1.f}, 0, {0.f, 1.f, 0.f}},
	};
	MeshIndex indexes[6] = {0, 2, 1, 0, 3, 2};

	GraphicsBuffer* plane_vertex_buffer = graphics_buffer_create(
			vertices,
			sizeof(MeshVertex) * 4,
			GraphicsBufferUsage_VertexBuffer,
			GraphicsMemoryPropertyFlag_DeviceLocal,
			GraphicsMemoryMapping_Never);

	GraphicsBuffer* plane_index_buffer = graphics_buffer_create(
			indexes,
			sizeof(MeshIndex) * 6,
			GraphicsBufferUsage_IndexBuffer,
			GraphicsMemoryPropertyFlag_DeviceLocal,
			GraphicsMemoryMapping_Never);

	struct {
		mat4 transform;
		vec3 color;
	} planes[2];

	planes[0].color = Vec3(1, 0, 0);
	planes[1].color = Vec3(0, 1, 0);

	auto draw_plane = [&](u32 idx, vec3 pos, vec3 rot, vec3 scale) {
		planes[idx].transform = mat4::TransformationMatrix(pos, rot, scale);
		graphics_cmd_push_constant(win, &planes[idx], model_push_constant);
		graphics_cmd_bind_vertex_buffer(win, plane_vertex_buffer);
		graphics_cmd_bind_index_buffer(win, plane_index_buffer);
		graphics_cmd_draw_indexed(win, 6, 0, 0);
	};

	camera.proj = Math::PerspectiveProjectionMatrix(win->width, win->height, 90, 0.1, 1000);
	camera.view = Math::LookAtMatrix(Vec3(0,0,0), Vec3(0,0,1));

	update_light();
	update_scene_ubo();
	update_offscreen_ubo();

	Model* box = assets_model_create_from_obj(str8l("box.obj"), 0);
	
	struct Box {
		mat4 transform;
		vec3 color;
	} boxes[3];

	boxes[0].color = Vec3(0,1,0);
	boxes[1].color = Vec3(0,0,1);
	boxes[2].color = Vec3(1,0,0);

	auto draw_box = [&](u32 idx, vec3 pos, vec3 rot, vec3 scale) {
		boxes[idx].transform = mat4::TransformationMatrix(pos, rot, scale);
		graphics_cmd_push_constant(win, &boxes[idx], model_push_constant);
		graphics_cmd_bind_vertex_buffer(win, box->mesh->vertex_buffer);
		graphics_cmd_bind_index_buffer(win, box->mesh->index_buffer);
		graphics_cmd_draw_indexed(win, box->mesh->index_count, 0, 0);
	};

	b32 fps = false;
	while(platform_update()) {
		if(key_pressed(Key_ESCAPE)) {
			break;
		}

		if(fps) {
			vec3 inputs = {0};
			if(key_down(Key_W))     inputs += camera.forward;
			if(key_down(Key_S))     inputs -= camera.forward;
			if(key_down(Key_D))     inputs += camera.right;
			if(key_down(Key_A))     inputs -= camera.right;
			if(key_down(Key_SPACE)) inputs += camera.up;
			if(key_down(Key_LCTRL)) inputs -= camera.up;

			auto rotdiffx = (g_input->mouseY - (f32)g_window->center.y) * 0.1f;
			auto rotdiffy = (g_input->mouseX - (f32)g_window->center.x) * 0.1f;

			if(rotdiffx || rotdiffy || inputs.x || inputs.y || inputs.z) {
				camera.rot.y += rotdiffy;
				camera.rot.x -= rotdiffx;

				f32 multiplier = 8.f;
				if(input_lshift_down()) multiplier = 32.f;
				else if(input_lalt_down()) multiplier = 4.f;

				camera.pos += inputs * multiplier * (g_time->deltaTime / 1000);
				camera.rot.x = Clamp(camera.rot.x, -80.f, 80.f);
				if(camera.rot.y >  1440.f) camera.rot.y -= 1440.f;
				if(camera.rot.y < -1440.f) camera.rot.y += 1440.f;

				camera.forward = vec3_normalized(vec3_FORWARD() * mat4::RotationMatrix(camera.rot));
				camera.right   = vec3_normalized(vec3_cross(vec3_UP(), camera.forward));
				camera.up      = vec3_normalized(vec3_cross(camera.forward, camera.right));
				
				camera.view = Math::LookAtMatrix(camera.pos, camera.pos + camera.forward).Inverse();
				camera.proj = Math::PerspectiveProjectionMatrix(win->width, win->height, 90, 0.1, 1000);
			}
		}

		if(key_pressed(Key_RIGHT)) scene.light_pos.x += 0.1;
		if(key_pressed(Key_LEFT)) scene.light_pos.x -= 0.1;
		if(key_pressed(Key_UP)) scene.light_pos.z += 0.1;
		if(key_pressed(Key_DOWN)) scene.light_pos.z -= 0.1;
		if(any_key_pressed()) {
			Log("", scene.light_pos);
		}

		update_scene_ubo();
		update_offscreen_ubo();

		if(key_pressed(Key_C)) {
			if(fps)
				window_set_cursor_mode(win, CursorMode_Default);
			else
				window_set_cursor_mode(win, CursorMode_FirstPerson);
			fps = !fps;
		}

		graphics_cmd_begin_render_pass(win, offscreen.render_pass, offscreen.frame);

		graphics_cmd_set_viewport(win, vec2::ZERO, Vec2(offscreen.width, offscreen.height));
		graphics_cmd_set_scissor(win, vec2::ZERO, Vec2(offscreen.width, offscreen.height));

		// graphics_cmd_set_depth_bias(win, 0, 0, 0);
		graphics_cmd_set_depth_bias(win, 1.25f, 0.f, 1.75f);
		graphics_cmd_bind_pipeline(win, offscreen_pipeline);
		graphics_cmd_bind_descriptor_set(win, 0, offscreen_descriptor_set);
		draw_box(1, Vec3(0, 10, 0), Vec3(0,0,0), Vec3(100, 1, 100));
		draw_box(0, Vec3(0, 3, 5), Vec3(0,0,0), vec3::ONE);

		graphics_cmd_end_render_pass(win);

		graphics_cmd_begin_render_pass(win, graphics_render_pass_of_window_presentation_frames(win), graphics_current_present_frame_of_window(win));

		graphics_cmd_set_viewport(win, vec2::ZERO, win->dimensions.toVec2());
		graphics_cmd_set_scissor(win, vec2::ZERO, win->dimensions.toVec2());

		graphics_cmd_bind_pipeline(win, scene_pipeline);
		graphics_cmd_bind_descriptor_set(win, 0, scene_descriptor_set);
		draw_box(1, Vec3(0, 10, 0), Vec3(0,0,0), Vec3(100, 1, 100));
		draw_box(0, Vec3(0, 3, 5), Vec3(0,0,0), vec3::ONE);
		draw_box(2, scene.light_pos, vec3::ZERO, vec3::ONE*0.2);

		graphics_cmd_end_render_pass(win);

		graphics_update(win);
	}
}
