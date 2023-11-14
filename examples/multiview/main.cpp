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
	mat4 view;
	mat4 proj;
} ubo;

int main() {
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	Window* win = window_create(str8l("render_api"));
	window_show(win);
	render_init_x(win);
	assets_init_x(win);

	Model* box = assets_model_create_from_obj(str8l("data/models/box.obj"), 0);
	
	auto pipeline = render_pipeline_create();
	pipeline->name = str8l("multiview pipeline");

	*array_push(pipeline->shader_stages) = {
		RenderShaderStage_Vertex,
		str8l("flat.vert"),
		file_read_simple(str8l("data/shaders/flat.vert"), deshi_temp_allocator),
	};

	*array_push(pipeline->shader_stages) = {
		RenderShaderStage_Fragment,
		str8l("flat.frag"),
		file_read_simple(str8l("data/shaders/flat.frag"), deshi_temp_allocator),
	};

	pipeline->            front_face = RenderPipelineFrontFace_CCW;
	pipeline->               culling = RenderPipelineCulling_Back;
	pipeline->          polygon_mode = RenderPipelinePolygonMode_Fill;
	pipeline->            depth_test = true;
	pipeline->      depth_compare_op = RenderCompareOp_Less;
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

	*array_push(pipeline->vertex_input_bindings) = {0, sizeof(MeshVertex)};
	array_grow(pipeline->vertex_input_attributes, 4);
	array_count(pipeline->vertex_input_attributes) = 4;
	pipeline->vertex_input_attributes[0] = {0, 0, RenderFormat_R32G32B32_Signed_Float,      offsetof(MeshVertex, pos)};
	pipeline->vertex_input_attributes[1] = {1, 0, RenderFormat_R32G32_Signed_Float,         offsetof(MeshVertex, uv)};
	pipeline->vertex_input_attributes[2] = {2, 0, RenderFormat_R8G8B8A8_UnsignedNormalized, offsetof(MeshVertex, color)};
	pipeline->vertex_input_attributes[3] = {3, 0, RenderFormat_R32G32B32_Signed_Float,      offsetof(MeshVertex, normal)};
	
	RenderBuffer* view0_buffer = render_buffer_create(
			0, sizeof(ubo),
			RenderBufferUsage_UniformBuffer,
			RenderMemoryPropertyFlag_HostVisible | RenderMemoryPropertyFlag_HostCoherent,
			RenderMemoryMapping_MapWriteUnmap);

	RenderBuffer* view1_buffer = render_buffer_create(
			0, sizeof(ubo),
			RenderBufferUsage_UniformBuffer,
			RenderMemoryPropertyFlag_HostVisible | RenderMemoryPropertyFlag_HostCoherent,
			RenderMemoryMapping_MapWriteUnmap);

	// create a layout for the camera info ubo
	// this is what we will change between views
	RenderDescriptorLayout* camera_descriptor_layout = render_descriptor_layout_create();
	camera_descriptor_layout->debug_name = str8l("multiview camera descriptor layout");
	RenderDescriptorLayoutBinding* camera_binding = array_push(camera_descriptor_layout->bindings);
	camera_binding->kind = RenderDescriptorType_Uniform_Buffer;
	camera_binding->shader_stages = RenderShaderStage_Vertex;
	camera_binding->binding = 0;
	render_descriptor_layout_update(camera_descriptor_layout);
	
	// create a layout for the texture
	RenderDescriptorLayout* texture_descriptor_layout = render_descriptor_layout_create();
	texture_descriptor_layout->debug_name = str8l("multiview texture descriptor layout");
	RenderDescriptorLayoutBinding* texture_binding = array_push(texture_descriptor_layout->bindings);
	texture_binding->kind = RenderDescriptorType_Combined_Image_Sampler;
	texture_binding->shader_stages = RenderShaderStage_Fragment;
	texture_binding->binding = 0;
	render_descriptor_layout_update(texture_descriptor_layout);

	RenderPushConstant model_push_constant = {};
	model_push_constant.shader_stage_flags = RenderShaderStage_Vertex;
	model_push_constant.size = sizeof(mat4);
	model_push_constant.offset = 0;

	RenderPipelineLayout* pipeline_layout = render_pipeline_layout_create();
	pipeline_layout->debug_name = str8l("multiview pipeline layout");
	array_grow(pipeline_layout->descriptor_layouts, 2);
	array_count(pipeline_layout->descriptor_layouts) = 2;
	pipeline_layout->descriptor_layouts[0] = camera_descriptor_layout;
	pipeline_layout->descriptor_layouts[1] = texture_descriptor_layout;
	*array_push(pipeline_layout->push_constants) = model_push_constant;
	render_pipeline_layout_update(pipeline_layout);

	pipeline->layout = pipeline_layout;
	render_pipeline_update(pipeline);

	RenderDescriptorSet* camera_descriptor_set0 = render_descriptor_set_create();
	*array_push(camera_descriptor_set0->layouts) = camera_descriptor_layout;
	render_descriptor_set_update(camera_descriptor_set0);

	RenderDescriptor* descriptors;
	array_init(descriptors, 1, deshi_temp_allocator);
	auto d = array_push(descriptors);
	d->kind = RenderDescriptorType_Uniform_Buffer;
	d->shader_stages = RenderShaderStage_Vertex;
	d->buffer.handle = view0_buffer;
	d->buffer.range = view0_buffer->size;
	d->buffer.offset = 0;

	render_descriptor_set_write(camera_descriptor_set0, descriptors);

	RenderDescriptorSet* camera_descriptor_set1 = render_descriptor_set_create();
	*array_push(camera_descriptor_set1->layouts) = camera_descriptor_layout;
	render_descriptor_set_update(camera_descriptor_set1);

	d->buffer.handle = view1_buffer;
	d->buffer.range = view1_buffer->size;
	d->buffer.offset = 0;

	render_descriptor_set_write(camera_descriptor_set1, descriptors);

	RenderDescriptorSet* texture_descriptor_set = render_descriptor_set_create();
	*array_push(texture_descriptor_set->layouts) = texture_descriptor_layout;
	render_descriptor_set_update(texture_descriptor_set);
	
	Texture* texture = assets_texture_create_from_file_simple(str8l("alex.png"));

	d->kind = RenderDescriptorType_Combined_Image_Sampler;
	d->shader_stages = RenderShaderStage_Fragment;
	d->image.view = texture->image_view;
	d->image.sampler = texture->sampler;
	d->image.layout = RenderImageLayout_Shader_Read_Only_Optimal;

	render_descriptor_set_write(texture_descriptor_set, descriptors);
	

	render_buffer_map(view0_buffer, 0, view0_buffer->size);
	
	ubo.view = Math::LookAtMatrix(Vec3(0,0,0), Vec3(0,0,1));
	ubo.proj = Camera::MakePerspectiveProjectionMatrix(win->width, win->height, 90, 1000, 0.1);
	CopyMemory(view0_buffer->mapped_data, &ubo, sizeof(ubo));

	render_buffer_unmap(view0_buffer, true);

	render_buffer_map(view1_buffer, 0, view1_buffer->size);
	
	ubo.view = Math::LookAtMatrix(Vec3(4,2,1), Vec3(0,0,4));
	ubo.proj = Camera::MakePerspectiveProjectionMatrix(win->width, win->height, 90, 1000, 0.1);
	CopyMemory(view1_buffer->mapped_data, &ubo, sizeof(ubo));

	render_buffer_unmap(view1_buffer, true);

	while(platform_update()) {
		auto transform = mat4::TransformationMatrix(
			{0, 0, 4},
			{0, f32(g_time->totalTime/5000.f*45), 0},
			{1, 1, 1});

		auto frame = render_current_present_frame_of_window(win);
		render_cmd_bind_pipeline(frame, pipeline);
		render_cmd_bind_descriptor_set(frame, 1, texture_descriptor_set);
		render_cmd_set_viewport(frame, Vec2(0,0), Vec2(win->width/2, win->height/2));
		render_cmd_push_constant(frame, &transform, model_push_constant);
		render_cmd_bind_vertex_buffer(frame, box->mesh->vertex_buffer);
		render_cmd_bind_index_buffer(frame, box->mesh->index_buffer);
		// bind the first camera info ubo and draw
		render_cmd_bind_descriptor_set(frame, 0, camera_descriptor_set0);
		render_cmd_draw_indexed(frame, box->mesh->index_count, 0, 0);
		// adjust viewport then bind the other view's information
		render_cmd_set_viewport(frame, Vec2(win->width/2, win->height/2), Vec2(win->width/2, win->height/2));
		render_cmd_bind_descriptor_set(frame, 0, camera_descriptor_set1);
		render_cmd_draw_indexed(frame, box->mesh->index_count, 0, 0);

		render_update_x(win);

	}
	
}
