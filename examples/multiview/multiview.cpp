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
#include "core/file.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/platform.h"
#include "core/graphics.h"
#include "core/assets.h"
#include "core/time.h"
#include "core/window.h"
#include "math/math.h"

#include "core/baked/shaders.h"

struct {
	mat4 view;
	mat4 proj;
	vec4 position;
} ubo;

int main() {
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	Window* win = window_create(str8l("multiview example"));
	window_show(win);
	graphics_init(win);
	assets_init(win);

	Model* box = assets_model_create_from_obj(str8l("box.obj"), 0);
	
	auto pipeline = graphics_pipeline_allocate();
	pipeline->debug_name = str8l("multiview pipeline");

	pipeline->vertex_shader = graphics_shader_allocate();
	pipeline->vertex_shader->debug_name = str8l("flat.vert");
	pipeline->vertex_shader->shader_stage = GraphicsShaderStage_Vertex;
	pipeline->vertex_shader->source = baked_shader_flat_vert;
	graphics_shader_update(pipeline->vertex_shader);

	pipeline->fragment_shader = graphics_shader_allocate();
	pipeline->fragment_shader->debug_name = str8l("flat_textured.frag");
	pipeline->fragment_shader->shader_stage = GraphicsShaderStage_Fragment;
	pipeline->fragment_shader->source =	baked_shader_flat_textured_frag;
	graphics_shader_update(pipeline->fragment_shader);
	
	pipeline->            front_face = GraphicsFrontFace_CCW;
	pipeline->               culling = GraphicsPipelineCulling_Back;
	pipeline->          polygon_mode = GraphicsPolygonMode_Fill;
	pipeline->            depth_test = true;
	pipeline->      depth_compare_op = GraphicsCompareOp_Less;
	pipeline->            depth_bias = false;
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

	pipeline->dynamic_viewport = 
	pipeline->dynamic_scissor  = true;

	array_init_with_elements(pipeline->vertex_input_bindings, 
			{{0, sizeof(MeshVertex)}}, deshi_temp_allocator);

	array_init_with_elements(pipeline->vertex_input_attributes, 
			{
				{0, 0, GraphicsFormat_R32G32B32_Float, offsetof(MeshVertex, pos)},
				{1, 0, GraphicsFormat_R32G32_Float,    offsetof(MeshVertex, uv)},
				{2, 0, GraphicsFormat_R8G8B8A8_UNorm,  offsetof(MeshVertex, color)},
				{3, 0, GraphicsFormat_R32G32B32_Float, offsetof(MeshVertex, normal)}
			}, deshi_temp_allocator);

	GraphicsBuffer* view0_buffer = graphics_buffer_create(
			0, sizeof(ubo),
			GraphicsBufferUsage_UniformBuffer,
			GraphicsMemoryPropertyFlag_HostVisible | GraphicsMemoryPropertyFlag_HostCoherent,
			GraphicsMemoryMapping_Occasional);

	GraphicsBuffer* view1_buffer = graphics_buffer_create(
			0, sizeof(ubo),
			GraphicsBufferUsage_UniformBuffer,
			GraphicsMemoryPropertyFlag_HostVisible | GraphicsMemoryPropertyFlag_HostCoherent,
			GraphicsMemoryMapping_Occasional);

	// create a layout for the camera info ubo
	// this is what we will change between views
	GraphicsDescriptorSetLayout* camera_descriptor_layout = graphics_descriptor_set_layout_allocate();
	camera_descriptor_layout->debug_name = str8l("multiview camera descriptor layout");
	array_init_with_elements(camera_descriptor_layout->bindings, {{
				GraphicsDescriptorType_Uniform_Buffer,
				GraphicsShaderStage_Vertex,
				0
			}}, deshi_temp_allocator);
	graphics_descriptor_set_layout_update(camera_descriptor_layout);
	
	// create a layout for the texture
	GraphicsDescriptorSetLayout* texture_descriptor_layout = graphics_descriptor_set_layout_allocate();
	texture_descriptor_layout->debug_name = str8l("multiview texture descriptor layout");
	array_init_with_elements(texture_descriptor_layout->bindings, {{
				GraphicsDescriptorType_Combined_Image_Sampler,
				GraphicsShaderStage_Fragment,
				0
			}}, deshi_temp_allocator);
	graphics_descriptor_set_layout_update(texture_descriptor_layout);

	GraphicsPushConstant model_push_constant = {};
	model_push_constant.shader_stages = GraphicsShaderStage_Vertex;
	model_push_constant.size = sizeof(mat4);
	model_push_constant.offset = 0;

	GraphicsPipelineLayout* pipeline_layout = graphics_pipeline_layout_allocate();
	pipeline_layout->debug_name = str8l("multiview pipeline layout");
	array_init_with_elements(pipeline_layout->descriptor_layouts, {
				camera_descriptor_layout,
				texture_descriptor_layout
			}, deshi_temp_allocator);
	array_init_with_elements(pipeline_layout->push_constants, {
				model_push_constant
			}, deshi_temp_allocator);
	graphics_pipeline_layout_update(pipeline_layout);

	pipeline->layout = pipeline_layout;
	graphics_pipeline_update(pipeline);
	
	GraphicsDescriptor* descriptors0 = array_create(GraphicsDescriptor, 1, deshi_allocator);
	GraphicsDescriptor* descriptor0 = array_push(descriptors0);
	descriptor0->type = GraphicsDescriptorType_Uniform_Buffer;
	descriptor0->shader_stages = GraphicsShaderStage_Vertex;
	descriptor0->ubo.buffer = view0_buffer;
	descriptor0->ubo.range = sizeof(ubo);
	descriptor0->ubo.offset = 0;

	GraphicsDescriptorSet* camera_descriptor_set0 = graphics_descriptor_set_allocate();
	camera_descriptor_set0->descriptors = descriptors0;
	array_init_with_elements(camera_descriptor_set0->layouts, {
				camera_descriptor_layout
			}, deshi_temp_allocator);
	graphics_descriptor_set_update(camera_descriptor_set0);
	graphics_descriptor_set_write(camera_descriptor_set0);
	
	GraphicsDescriptor* descriptors1 = array_create(GraphicsDescriptor, 1, deshi_allocator);
	GraphicsDescriptor* descriptor1 = array_push(descriptors1);
	descriptor1->type = GraphicsDescriptorType_Uniform_Buffer;
	descriptor1->shader_stages = GraphicsShaderStage_Vertex;
	descriptor1->ubo.buffer = view1_buffer;
	descriptor1->ubo.range = sizeof(ubo);
	descriptor1->ubo.offset = 0;

	GraphicsDescriptorSet* camera_descriptor_set1 = graphics_descriptor_set_allocate();
	camera_descriptor_set1->descriptors = descriptors1;
	array_init_with_elements(camera_descriptor_set1->layouts, {
				camera_descriptor_layout
			}, deshi_temp_allocator);
	graphics_descriptor_set_update(camera_descriptor_set1);
	graphics_descriptor_set_write(camera_descriptor_set1);
	
	GraphicsDescriptor* descriptors2 = array_create(GraphicsDescriptor, 1, deshi_allocator);
	GraphicsDescriptor* descriptor2 = array_push(descriptors2);
	Texture* texture = assets_texture_create_from_path_simple(str8l("alex"), str8l("alex.png"));
	descriptor2->type = GraphicsDescriptorType_Combined_Image_Sampler;
	descriptor2->shader_stages = GraphicsShaderStage_Fragment;
	descriptor2->image.view = texture->image_view;
	descriptor2->image.sampler = texture->sampler;
	descriptor2->image.layout = GraphicsImageLayout_Shader_Read_Only_Optimal;

	GraphicsDescriptorSet* texture_descriptor_set = graphics_descriptor_set_allocate();
	texture_descriptor_set->descriptors = descriptors2;
	array_init_with_elements(texture_descriptor_set->layouts, {
				texture_descriptor_layout
			}, deshi_temp_allocator);
	graphics_descriptor_set_update(texture_descriptor_set);
	graphics_descriptor_set_write(texture_descriptor_set);

	void* mapped_data = graphics_buffer_map(view0_buffer, graphics_buffer_device_size(view0_buffer), 0);
	
	ubo.view = Math::LookAtMatrix(Vec3(0,0,0), Vec3(0,0,1));
	ubo.proj = Math::PerspectiveProjectionMatrix(win->width, win->height, 90, 0.1, 1000);
	ubo.position = Vec4(0,0,0,1);
	CopyMemory(mapped_data, &ubo, sizeof(ubo));

	graphics_buffer_unmap(view0_buffer, true);

	mapped_data = graphics_buffer_map(view1_buffer, graphics_buffer_device_size(view1_buffer), 0);
	
	ubo.view = Math::LookAtMatrix(Vec3(0,0,8), Vec3(0,0,4));
	ubo.proj = Math::PerspectiveProjectionMatrix(win->width, win->height, 90, 0.1, 1000);
	ubo.position = Vec4(0,0,8,1);
	CopyMemory(mapped_data, &ubo, sizeof(ubo));

	graphics_buffer_unmap(view1_buffer, true);

	while(platform_update()) {
		auto transform = mat4::TransformationMatrix(
			{0, 0, 4},
			{0, f32(g_time->totalTime/5000.f*45), 0},
			{1, 1, 1});

		{ using namespace graphics::cmd;
			begin_render_pass(win, graphics_render_pass_of_window_presentation_frames(win), graphics_current_present_frame_of_window(win));

			bind_pipeline(win, pipeline);
			set_scissor(win, vec2::ZERO, win->dimensions.toVec2());
			bind_descriptor_set(win, 1, texture_descriptor_set);
			set_viewport(win, vec2::ZERO, Vec2(win->width/2, win->height/2));
			push_constant(win, GraphicsShaderStage_Vertex, &transform, 0, sizeof(mat4));
			bind_vertex_buffer(win, box->mesh->vertex_buffer);
			bind_index_buffer(win, box->mesh->index_buffer);
			bind_descriptor_set(win, 0, camera_descriptor_set0);
			draw_indexed(win, box->mesh->index_count, 0, 0);
			set_viewport(win, Vec2(win->width/2, win->height/2), Vec2(win->width/2, win->height/2));
			bind_descriptor_set(win, 0, camera_descriptor_set1);
			draw_indexed(win, box->mesh->index_count, 0, 0);

			end_render_pass(win);
		}

		graphics_update(win);
	}
}
