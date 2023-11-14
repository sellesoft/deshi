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
	RenderPass* render_pass;
	// the image we will be rendering to
	RenderImage* image;
	RenderImageView* image_view;
	RenderSampler* sampler;
	
	RenderFrame* frame;

	u32 width, height;

	f32 light_fov;

	f32 z_near, z_far;
} offscreen;

struct Vertex {
	vec3 pos;
	vec2 uv;
	vec3 color;
	vec3 normal;
};

int main() {
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	Window* win = window_create(str8l("render_api"));
	window_show(win);
	render_init_x(win);
		
	offscreen.width  = 1028;
	offscreen.height = 1028;

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
		mat4 model;
		mat4 depth_bias_mvp;
		vec4 light_pos;
		f32 z_near;
		f32 z_far;
	} ubo_scene;
	
	struct {
		mat4 depth_mvp;
	} ubo_offscreen;

	RenderBuffer* scene_ubo_buffer = render_buffer_create(
			0, sizeof(ubo_scene),
			RenderBufferUsage_UniformBuffer,
			RenderMemoryPropertyFlag_HostVisible | 
			RenderMemoryPropertyFlag_HostCoherent,
			RenderMemoryMapping_Persistent);

	auto update_scene_ubo = [&]() {
		ubo_scene.          proj = camera.proj;
		ubo_scene.          view = camera.view;
		ubo_scene.         model = mat4::IDENTITY;
		ubo_scene.     light_pos = scene.light_pos.toVec4();
		ubo_scene.depth_bias_mvp = ubo_offscreen.depth_mvp;
		ubo_scene.        z_near = camera.near_z;
		ubo_scene.         z_far = camera.far_z;
		CopyMemory(scene_ubo_buffer->mapped_data, &ubo_scene, sizeof(ubo_scene));
	};
	
	RenderBuffer* offscreen_ubo_buffer = render_buffer_create(
			0, sizeof(ubo_offscreen),
			RenderBufferUsage_UniformBuffer,
			RenderMemoryPropertyFlag_HostVisible | 
			RenderMemoryPropertyFlag_HostCoherent,
			RenderMemoryMapping_Persistent);

	auto update_offscreen_ubo = [&]() {
		mat4 depth_proj_mat = Camera::MakePerspectiveProjectionMatrix(
				offscreen.width, offscreen.height,
				offscreen.light_fov,
				offscreen.z_far, offscreen.z_near);
		mat4 depth_view_mat = Math::LookAtMatrix(ubo_scene.light_pos.toVec3(), vec3::ZERO);
		mat4 depth_model_mat = mat4::IDENTITY;

		ubo_offscreen.depth_mvp = depth_proj_mat * depth_view_mat * depth_model_mat;

		CopyMemory(offscreen_ubo_buffer->mapped_data, &ubo_offscreen, sizeof(ubo_offscreen));
	};

	auto update_light = [&]() {
		auto time = g_time->totalTime/1000.f;
		ubo_scene.light_pos = Vec4(
			cos(Radians(time*360.f)) * 40.f,
			-50.f + sin(Radians(time*360.f)) * 20.f,
			25.f + sin(Radians(time*360.f)) * 5.f,
			1
		);
	};


	// We only need a depth attachment as we just want
	// the distance of pixels from the light
	RenderPassAttachment depth_attachment = {};
	depth_attachment.format = RenderFormat_Depth16_UnsignedNormalized;
	depth_attachment.load_op = RenderAttachmentLoadOp_Clear;
	// We want to store the result of this render pass so we may use it for the final frame
	depth_attachment.store_op = RenderAttachmentStoreOp_Store;
	depth_attachment.stencil_load_op = RenderAttachmentLoadOp_Dont_Care;
	depth_attachment.stencil_store_op = RenderAttachmentStoreOp_Dont_Care;

	offscreen.render_pass = render_pass_create();
	offscreen.render_pass->debug_name = str8l("offscreen render pass");
	offscreen.render_pass->debug_color = Color_Red;
	offscreen.render_pass->depth_attachment = &depth_attachment;
	render_pass_update(offscreen.render_pass);

	offscreen.image = render_image_create();
	offscreen.image->debug_name = str8l("offscreen image");
	offscreen.image->format = RenderFormat_Depth16_UnsignedNormalized;
	offscreen.image->extent.x = offscreen.width;
	offscreen.image->extent.y = offscreen.height;
	offscreen.image->extent.z = 1;
	offscreen.image->samples = RenderSampleCount_1;
	offscreen.image->linear_tiling = false;
	offscreen.image->usage = (RenderImageUsage)(RenderImageUsage_Depth_Stencil_Attachment | RenderImageUsage_Sampled);
	render_image_update(offscreen.image);

	offscreen.image_view = render_image_view_create();
	offscreen.image_view->debug_name = str8l("offscreen image view");
	offscreen.image_view->image = offscreen.image;
	offscreen.image_view->format = offscreen.image->format;
	offscreen.image_view->aspect_flags = RenderImageViewAspectFlags_Depth;
	render_image_view_update(offscreen.image_view);

	offscreen.sampler = render_sampler_create();
	offscreen.sampler->mag_filter =
	offscreen.sampler->min_filter = RenderFilter_Nearest;
	offscreen.sampler->address_mode_u = 
	offscreen.sampler->address_mode_v = 
	offscreen.sampler->address_mode_w = RenderSamplerAddressMode_Clamp_To_Edge;
	offscreen.sampler->border_color = Color_White;
	render_sampler_update(offscreen.sampler);

	offscreen.frame = render_frame_create(win);
	offscreen.frame->debug_name = str8l("offscreen frame");
	offscreen.frame->render_pass = offscreen.render_pass;
	offscreen.frame->width = offscreen.width;
	offscreen.frame->height = offscreen.height;
	offscreen.frame->depth_image_view = offscreen.image_view;
	render_frame_update(offscreen.frame);

	RenderDescriptorLayout* descriptor_layout = render_descriptor_layout_create();
	array_grow(descriptor_layout->bindings, 2);
	array_count(descriptor_layout->bindings) = 2;
	descriptor_layout->bindings[0].binding = 0;
	descriptor_layout->bindings[0].kind = RenderDescriptorType_Uniform_Buffer;
	descriptor_layout->bindings[0].shader_stages = (RenderShaderStage)(RenderShaderStage_Vertex | RenderShaderStage_Fragment);
	descriptor_layout->bindings[1].binding = 1;
	descriptor_layout->bindings[1].kind = RenderDescriptorType_Combined_Image_Sampler;
	descriptor_layout->bindings[1].shader_stages = RenderShaderStage_Fragment;
	render_descriptor_layout_update(descriptor_layout);
	
	RenderPushConstant model_push_constant = {};
	model_push_constant.shader_stage_flags = RenderShaderStage_Vertex;
	model_push_constant.size = sizeof(mat4);
	model_push_constant.offset = 0;

	RenderPipelineLayout* pipeline_layout = render_pipeline_layout_create();
	pipeline_layout->debug_name = str8l("shadows shared pipeline layout");
	*array_push(pipeline_layout->descriptor_layouts) = descriptor_layout;
	*array_push(pipeline_layout->push_constants) = model_push_constant;
	render_pipeline_layout_update(pipeline_layout);

	RenderPipeline* shadow_debug_quad_pipeline = render_pipeline_create();
	RenderPipeline* scene_pipeline = 0;
	RenderPipeline* offscreen_pipeline = 0;

	auto pipeline = shadow_debug_quad_pipeline;
	pipeline->layout = pipeline_layout;

	// setup settings shared by all three pipelines
	pipeline->            front_face = RenderPipelineFrontFace_CCW;
	pipeline->               culling = RenderPipelineCulling_Back;
	pipeline->          polygon_mode = RenderPipelinePolygonMode_Fill;
	pipeline->            depth_test = true;
	pipeline->      depth_compare_op = RenderCompareOp_Less_Or_Equal;
	pipeline->            depth_bias = false;
	pipeline->            line_width = 1.f;
	pipeline->           color_blend = true;
	pipeline->        color_blend_op = RenderBlendOp_Add;
	pipeline->color_src_blend_factor = RenderBlendFactor_Source_Alpha;
	pipeline->color_dst_blend_factor = RenderBlendFactor_One_Minus_Source_Alpha;
	pipeline->        alpha_blend_op = RenderBlendOp_Add;
	pipeline->alpha_src_blend_factor = RenderBlendFactor_One_Minus_Source_Alpha;
	pipeline->alpha_dst_blend_factor = RenderBlendFactor_Zero;
	pipeline->        blend_constant = color(10,10,10,255);
	pipeline->           render_pass = render_pass_of_window_presentation_frame(win);

	*array_push(pipeline->dynamic_states) = RenderDynamicState_Viewport;
	*array_push(pipeline->dynamic_states) = RenderDynamicState_Scissor;

	scene_pipeline = render_pipeline_duplicate(shadow_debug_quad_pipeline);

	// shadow map debug quad display
	shadow_debug_quad_pipeline->culling = RenderPipelineCulling_None;
	*array_push(shadow_debug_quad_pipeline->shader_stages) = {
		RenderShaderStage_Vertex,
		str8l("quad.vert"),
		file_read_simple(str8l("data/shaders/quad.vert"), deshi_temp_allocator),
	};
	*array_push(shadow_debug_quad_pipeline->shader_stages) = {
		RenderShaderStage_Fragment,
		str8l("quad.frag"),
		file_read_simple(str8l("data/shaders/quad.frag"), deshi_temp_allocator),
	};

	// the vertex state has nothing 
	render_pipeline_update(shadow_debug_quad_pipeline);

	// scene with shadows
	*array_push(scene_pipeline->vertex_input_bindings) = {0, sizeof(MeshVertex)};
	array_grow(scene_pipeline->vertex_input_attributes, 4);
	array_count(scene_pipeline->vertex_input_attributes) = 4;
	scene_pipeline->vertex_input_attributes[0] = {0, 0, RenderFormat_R32G32B32_Signed_Float,      offsetof(MeshVertex, pos)};
	scene_pipeline->vertex_input_attributes[1] = {1, 0, RenderFormat_R32G32_Signed_Float,         offsetof(MeshVertex, uv)};
	scene_pipeline->vertex_input_attributes[2] = {2, 0, RenderFormat_R8G8B8A8_UnsignedNormalized, offsetof(MeshVertex, color)};
	scene_pipeline->vertex_input_attributes[3] = {3, 0, RenderFormat_R32G32B32_Signed_Float,      offsetof(MeshVertex, normal)};

	offscreen_pipeline = render_pipeline_duplicate(scene_pipeline);
	offscreen_pipeline->render_pass = offscreen.render_pass;

	*array_push(scene_pipeline->shader_stages) = {
		RenderShaderStage_Vertex,
		str8l("scene.vert"),
		file_read_simple(str8l("data/shaders/scene.vert"), deshi_temp_allocator),
	};
	*array_push(scene_pipeline->shader_stages) = {
		RenderShaderStage_Fragment,
		str8l("scene.frag"),
		file_read_simple(str8l("data/shaders/scene.frag"), deshi_temp_allocator),
	};
	render_pipeline_update(scene_pipeline);

	// offscreen vertex (vertex shader only)
	*array_push(offscreen_pipeline->shader_stages) = {
		RenderShaderStage_Vertex,
		str8l("offscreen.vert"),
		file_read_simple(str8l("data/shaders/offscreen.vert"), deshi_temp_allocator),
	};
	*array_push(offscreen_pipeline->dynamic_states) = RenderDynamicState_Depth_Bias;
	render_pipeline_update(offscreen_pipeline);
	

	RenderDescriptor* descriptors;
	array_init(descriptors, 2, deshi_temp_allocator);
	

	RenderDescriptorSet* debug_descriptor_set = render_descriptor_set_create();
	*array_push(debug_descriptor_set->layouts) = descriptor_layout;
	render_descriptor_set_update(debug_descriptor_set);
	
	array_count(descriptors) = 2;
	descriptors[0].kind = RenderDescriptorType_Uniform_Buffer;
	descriptors[0].shader_stages = RenderShaderStage_Fragment;
	descriptors[0].buffer = {
		scene_ubo_buffer,
		0,
		scene_ubo_buffer->size,
	};

	descriptors[1].kind = RenderDescriptorType_Combined_Image_Sampler;
	descriptors[1].shader_stages = RenderShaderStage_Fragment;
	descriptors[1].image = {
		offscreen.image_view,
		offscreen.sampler,
		RenderImageLayout_Depth_Stencil_Read_Only_Optimal,
	};
	render_descriptor_set_write(debug_descriptor_set, descriptors);

	RenderDescriptorSet* offscreen_descriptor_set = render_descriptor_set_create();
	*array_push(offscreen_descriptor_set->layouts) = descriptor_layout;
	render_descriptor_set_update(offscreen_descriptor_set);

	array_count(descriptors) = 1;
	descriptors[0].shader_stages = RenderShaderStage_Vertex;
	descriptors[0].buffer = {
		offscreen_ubo_buffer,
		0,
		offscreen_ubo_buffer->size,
	};
	render_descriptor_set_write(offscreen_descriptor_set, descriptors);
	
	RenderDescriptorSet* scene_descriptor_set = render_descriptor_set_create();
	*array_push(scene_descriptor_set->layouts) = descriptor_layout;
	render_descriptor_set_update(scene_descriptor_set);

	array_count(descriptors) = 2;
	descriptors[0].buffer = {
		scene_ubo_buffer,
		0,
		scene_ubo_buffer->size,
	};
	descriptors[1].image = {
		offscreen.image_view,
		offscreen.sampler,
		RenderImageLayout_Depth_Stencil_Read_Only_Optimal,
	};
	render_descriptor_set_write(scene_descriptor_set, descriptors);

	MeshVertex vertices[4] = {
		{{ 0.5f,  0.5f, 0.f}, {1.f, 0.f}, 0, {0.f, 1.f, 0.f}},
		{{-0.5f,  0.5f, 0.f}, {0.f, 0.f}, 0, {0.f, 1.f, 0.f}},
		{{-0.5f, -0.5f, 0.f}, {0.f, 1.f}, 0, {0.f, 1.f, 0.f}},
		{{ 0.5f, -0.5f, 0.f}, {1.f, 1.f}, 0, {0.f, 1.f, 0.f}},
	};
	MeshIndex indexes[6] = {0, 2, 1, 0, 3, 2};

	RenderBuffer* plane_vertex_buffer = render_buffer_create(
			vertices,
			sizeof(MeshVertex) * 4,
			RenderBufferUsage_VertexBuffer,
			RenderMemoryPropertyFlag_DeviceLocal,
			RenderMemoryMapping_None);

	RenderBuffer* plane_index_buffer = render_buffer_create(
			indexes,
			sizeof(MeshIndex) * 6,
			RenderBufferUsage_IndexBuffer,
			RenderMemoryPropertyFlag_DeviceLocal,
			RenderMemoryMapping_None);

	mat4 plane_transform;
	auto draw_plane = [&](vec3 pos, vec3 rot, vec3 scale) {
		plane_transform = mat4::TransformationMatrix(pos, rot, scale);
		render_cmd_push_constant(win, &plane_transform, model_push_constant);
		render_cmd_bind_vertex_buffer(win, plane_vertex_buffer);
		render_cmd_bind_index_buffer(win, plane_index_buffer);
		render_cmd_draw_indexed(win, 6, 0, 0);
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
				camera.proj = Camera::MakePerspectiveProjectionMatrix(win->width, win->height, 90, 1000, 0.1);

				update_scene_ubo();
				update_offscreen_ubo();
			}
		}

		update_light();
		update_offscreen_ubo();

		if(key_pressed(Key_C)) {
			if(fps)
				window_set_cursor_mode(win, CursorMode_Default);
			else
				window_set_cursor_mode(win, CursorMode_FirstPerson);
			fps = !fps;
		}

		render_cmd_set_depth_bias(win, 1.25f, 0.f, 1.75f);
		render_cmd_bind_pipeline(win, offscreen_pipeline);
		render_cmd_bind_descriptor_set(win, 0, offscreen_descriptor_set);
		draw_plane(Vec3(0,0,2), Vec3(0,0,0), vec3::ONE);

		render_update_x(win);
	}
}
