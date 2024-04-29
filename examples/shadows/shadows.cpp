/* shadow mapping deshi example
Index:
@offscreen
@scene
@debug_quad
@meshes
@update

Ref:
https://github.com/SaschaWillems/Vulkan/tree/master/shaders/glsl/shadowmapping  (Sascha Willems)
https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
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
#include "core/assets.h"
#include "core/commands.h"
#include "core/console.h"
#include "core/file.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/platform.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/ui_widgets.h"
#include "core/window.h"
#include "core/render.h"
#include "math/math.h"

//// example includes ////
#include "shadows_shaders.cpp"


int main(){
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	Window* win = window_create(str8l("shadows example"));
	window_show(win);
	graphics_init(win);
	assets_init(win);
	
	struct{
		f32 hfov;
		f32 near_z;
		f32 far_z;
		vec3 pos;
		vec3 rot;
		vec3 forward;
		vec3 right;
		vec3 up;
		mat4 view;
		mat4 proj;
	}camera;
	camera.hfov = 90.0f;
	camera.near_z = 0.1f;
	camera.far_z = 10000.0f;
	camera.proj = Math::PerspectiveProjectionMatrix(win->width, win->height, camera.hfov, camera.near_z, camera.far_z);
	camera.view = Math::LookAtMatrix(Vec3(0,0,0), Vec3(0,0,1));
	
	//-////////////////////////////////////////////////////////////////////////////////////////////////
	//// @offscreen
	
	struct{
		u32 width;
		u32 height;
		f32 light_hfov;
		struct{
			f32 near;
			f32 far;
		}depth;
	}offscreen;
	offscreen.width  = 1028;
	offscreen.height = 1028;
	offscreen.light_hfov = 160.0f;
	offscreen.depth.near = 0.1f;
	offscreen.depth.far = 1000.0f;
	
	struct{
		mat4 proj;
		mat4 view;
	}ubo_offscreen;
	
	GraphicsBuffer* offscreen_ubo_buffer = graphics_buffer_create(0, sizeof(ubo_offscreen), GraphicsBufferUsage_UniformBuffer,
																  GraphicsMemoryProperty_HostStreamed, GraphicsMemoryMapping_Persistent);
	offscreen_ubo_buffer->debug_name = str8l("<shadows> offscreen ubo buffer");
	
	GraphicsFramebuffer* offscreen_framebuffer = graphics_framebuffer_allocate();
	{
		offscreen_framebuffer->debug_name = str8l("<shadows> offscreen framebuffer");
		
		//We only need a depth attachment as we just want the distance of pixels from the light
		offscreen_framebuffer->render_pass = graphics_render_pass_allocate();
		offscreen_framebuffer->render_pass->debug_name = str8l("<shadows> offscreen render pass");
		offscreen_framebuffer->render_pass->use_depth_attachment = true;
		offscreen_framebuffer->render_pass->depth_attachment.sample_count = 1;
		offscreen_framebuffer->render_pass->depth_attachment.format = GraphicsFormat_Depth16_UNorm;
		offscreen_framebuffer->render_pass->depth_attachment.load_op = GraphicsLoadOp_Clear;
		offscreen_framebuffer->render_pass->depth_attachment.store_op = GraphicsStoreOp_Store; //We want to store the result of this render pass so we may use it for the final frame
		offscreen_framebuffer->render_pass->depth_attachment.stencil_load_op = GraphicsLoadOp_Dont_Care;
		offscreen_framebuffer->render_pass->depth_attachment.stencil_store_op = GraphicsStoreOp_Dont_Care;
		offscreen_framebuffer->render_pass->depth_attachment.initial_layout = GraphicsImageLayout_Undefined;
		offscreen_framebuffer->render_pass->depth_attachment.final_layout = GraphicsImageLayout_Depth_Stencil_Read_Only_Optimal;
		offscreen_framebuffer->render_pass->depth_clear_values = {1.0f, 0};
		graphics_render_pass_update(offscreen_framebuffer->render_pass);
		
		offscreen_framebuffer->width = offscreen.width;
		offscreen_framebuffer->height = offscreen.height;
		
		GraphicsImage* offscreen_image = graphics_image_allocate();
		offscreen_image->debug_name = str8l("<shadows> offscreen image");
		offscreen_image->format = GraphicsFormat_Depth16_UNorm;
		offscreen_image->extent.x = offscreen.width;
		offscreen_image->extent.y = offscreen.height;
		offscreen_image->extent.z = 1;
		offscreen_image->samples = GraphicsSampleCount_1;
		offscreen_image->linear_tiling = false;
		offscreen_image->usage = GraphicsImageUsage_Depth_Stencil_Attachment | GraphicsImageUsage_Sampled;
		graphics_image_update(offscreen_image);
		
		offscreen_framebuffer->depth_image_view = graphics_image_view_allocate();
		offscreen_framebuffer->depth_image_view->debug_name = str8l("<shadows> offscreen image view");
		offscreen_framebuffer->depth_image_view->image = offscreen_image;
		offscreen_framebuffer->depth_image_view->format = offscreen_image->format;
		offscreen_framebuffer->depth_image_view->aspect_flags = GraphicsImageViewAspectFlags_Depth;
		graphics_image_view_update(offscreen_framebuffer->depth_image_view);
		
		graphics_framebuffer_update(offscreen_framebuffer);
	}
	
	GraphicsPipeline* offscreen_pipeline = graphics_pipeline_allocate();
	{
		offscreen_pipeline->debug_name = str8l("<shadows> offscreen pipeline");
		
		offscreen_pipeline->vertex_shader = graphics_shader_allocate();
		offscreen_pipeline->vertex_shader->debug_name = str8l("<shadows> offscreen.vert");
		offscreen_pipeline->vertex_shader->shader_stage = GraphicsShaderStage_Vertex;
		offscreen_pipeline->vertex_shader->source = baked_shader_offscreen_vert;
		graphics_shader_update(offscreen_pipeline->vertex_shader);
		
		offscreen_pipeline->dynamic_viewport = true;
		offscreen_pipeline->dynamic_scissor = true;
		
		offscreen_pipeline->front_face = GraphicsFrontFace_CCW;
		offscreen_pipeline->culling = GraphicsPipelineCulling_Back;
		offscreen_pipeline->polygon_mode = GraphicsPolygonMode_Fill;
		offscreen_pipeline->line_width = 1.0f;
		
		offscreen_pipeline->depth_test = true;
		offscreen_pipeline->depth_writes = true;
		offscreen_pipeline->depth_compare_op = GraphicsCompareOp_Less_Or_Equal;
		offscreen_pipeline->depth_bias = true;
		offscreen_pipeline->dynamic_depth_bias = true;
		
		offscreen_pipeline->color_blend = true;
		offscreen_pipeline->color_blend_op = GraphicsBlendOp_Add;
		offscreen_pipeline->color_src_blend_factor = GraphicsBlendFactor_Source_Alpha;
		offscreen_pipeline->color_dst_blend_factor = GraphicsBlendFactor_One_Minus_Source_Alpha;
		
		offscreen_pipeline->alpha_blend_op = GraphicsBlendOp_Add;
		offscreen_pipeline->alpha_src_blend_factor = GraphicsBlendFactor_One_Minus_Source_Alpha;
		offscreen_pipeline->alpha_dst_blend_factor = GraphicsBlendFactor_Zero;
		
		offscreen_pipeline->blend_constant = color(10,10,10,255);
		
		array_init_with_elements(offscreen_pipeline->vertex_input_bindings, {
									 {0, sizeof(MeshVertex)}
								 }, deshi_allocator);
		
		array_init_with_elements(offscreen_pipeline->vertex_input_attributes, {
									 {0, 0, GraphicsFormat_R32G32B32_Float, offsetof(MeshVertex, pos)},
									 {1, 0, GraphicsFormat_R32G32_Float,    offsetof(MeshVertex, uv)},
									 {2, 0, GraphicsFormat_R8G8B8A8_UNorm,  offsetof(MeshVertex, color)},
									 {3, 0, GraphicsFormat_R32G32B32_Float, offsetof(MeshVertex, normal)},
								 }, deshi_allocator);
		
		offscreen_pipeline->layout = graphics_pipeline_layout_allocate();
		{
			offscreen_pipeline->layout->debug_name = str8l("<shadows> offscreen pipeline layout");
			
			GraphicsDescriptorSetLayout* offscreen_descriptor_layout = graphics_descriptor_set_layout_allocate();
			offscreen_descriptor_layout->debug_name = str8l("<shadows> offscreen descriptor set");
			offscreen_descriptor_layout->bindings = array<GraphicsDescriptorSetLayoutBinding>::create_with_count(1, deshi_temp_allocator).ptr;
			offscreen_descriptor_layout->bindings[0].n = 0;
			offscreen_descriptor_layout->bindings[0].type = GraphicsDescriptorType_Uniform_Buffer;
			offscreen_descriptor_layout->bindings[0].shader_stages = GraphicsShaderStage_Vertex;
			graphics_descriptor_set_layout_update(offscreen_descriptor_layout);
			array_init_with_elements(offscreen_pipeline->layout->descriptor_layouts, {offscreen_descriptor_layout}, deshi_allocator);
			
			GraphicsPushConstant model_push_constant = {};
			model_push_constant.debug_name = str8l("<shadows> scene push constant");
			model_push_constant.shader_stages = GraphicsShaderStage_Vertex;
			model_push_constant.size = sizeof(mat4) + sizeof(vec3);
			model_push_constant.offset = 0;
			model_push_constant.name_in_shader = "PushConstant";
			array_init_with_elements(offscreen_pipeline->layout->push_constants, {model_push_constant}, deshi_allocator);
			
			graphics_pipeline_layout_update(offscreen_pipeline->layout);
		}
		
		offscreen_pipeline->render_pass = offscreen_framebuffer->render_pass;
		
		graphics_pipeline_update(offscreen_pipeline);
	}
	
	GraphicsDescriptorSet* offscreen_descriptor_set = graphics_descriptor_set_allocate();
	{
		offscreen_descriptor_set->debug_name = str8l("<shadows> offscreen descriptor set");
		offscreen_descriptor_set->layouts = offscreen_pipeline->layout->descriptor_layouts;
		graphics_descriptor_set_update(offscreen_descriptor_set);
		
		auto offscreen_descriptors = array<GraphicsDescriptor>::create_with_count(1, deshi_allocator);
		offscreen_descriptors[0].type = GraphicsDescriptorType_Uniform_Buffer;
		offscreen_descriptors[0].shader_stages = GraphicsShaderStage_Vertex;
		offscreen_descriptors[0].name_in_shader = "OffscreenUBO";
		offscreen_descriptors[0].ubo = {
			offscreen_ubo_buffer,
			0,
			sizeof(ubo_offscreen),
		};
		
		offscreen_descriptor_set->descriptors = offscreen_descriptors.ptr;
		graphics_descriptor_set_write(offscreen_descriptor_set);
	}
	
	GraphicsSampler* offscreen_sampler = graphics_sampler_allocate();
	offscreen_sampler->debug_name = str8l("<shadows> offscreen sampler");
	offscreen_sampler->mag_filter = GraphicsFilter_Nearest;
	offscreen_sampler->min_filter = GraphicsFilter_Nearest;
	offscreen_sampler->address_mode_u = GraphicsSamplerAddressMode_Clamp_To_Border;
	offscreen_sampler->address_mode_v = GraphicsSamplerAddressMode_Clamp_To_Border;
	offscreen_sampler->address_mode_w = GraphicsSamplerAddressMode_Clamp_To_Border;
	offscreen_sampler->border_color = Color_Red;
	graphics_sampler_update(offscreen_sampler);
	
	//-////////////////////////////////////////////////////////////////////////////////////////////////
	//// @scene
	
	struct{
		mat4 proj;
		mat4 view;
		mat4 light_proj;
		mat4 light_view;
		vec3 light_pos;
		f32 time;
	}ubo_scene;
	ubo_scene.light_pos = Vec3(0, -10, 0);
	
	GraphicsPipeline* scene_pipeline = graphics_pipeline_duplicate(offscreen_pipeline);
	{
		scene_pipeline->debug_name = str8l("<shadows> scene pipeline");
		
		scene_pipeline->vertex_shader = graphics_shader_allocate();
		scene_pipeline->vertex_shader->debug_name = str8l("<shadows> scene.vert");
		scene_pipeline->vertex_shader->shader_stage = GraphicsShaderStage_Vertex;
		scene_pipeline->vertex_shader->source = baked_shader_scene_vert;
		graphics_shader_update(scene_pipeline->vertex_shader);
		
		scene_pipeline->fragment_shader = graphics_shader_allocate();
		scene_pipeline->fragment_shader->debug_name = str8l("<shadows> scene.frag");
		scene_pipeline->fragment_shader->shader_stage = GraphicsShaderStage_Fragment;
		scene_pipeline->fragment_shader->source = baked_shader_scene_frag;
		graphics_shader_update(scene_pipeline->fragment_shader);
		
		scene_pipeline->layout = graphics_pipeline_layout_allocate();
		{
			scene_pipeline->layout->debug_name = str8l("<shadows> scene pipeline layout");
			
			GraphicsDescriptorSetLayout* scene_descriptor_layout = graphics_descriptor_set_layout_allocate();
			scene_descriptor_layout->debug_name = str8l("<shadows> scene descriptor set");
			scene_descriptor_layout->bindings = array<GraphicsDescriptorSetLayoutBinding>::create_with_count(2, deshi_temp_allocator).ptr;
			scene_descriptor_layout->bindings[0].n = 0;
			scene_descriptor_layout->bindings[0].type = GraphicsDescriptorType_Uniform_Buffer;
			scene_descriptor_layout->bindings[0].shader_stages = GraphicsShaderStage_Vertex | GraphicsShaderStage_Fragment;
			scene_descriptor_layout->bindings[1].n = 1;
			scene_descriptor_layout->bindings[1].type = GraphicsDescriptorType_Combined_Image_Sampler;
			scene_descriptor_layout->bindings[1].shader_stages = GraphicsShaderStage_Fragment;
			graphics_descriptor_set_layout_update(scene_descriptor_layout);
			array_init_with_elements(scene_pipeline->layout->descriptor_layouts, {scene_descriptor_layout}, deshi_allocator);
			
			GraphicsPushConstant model_push_constant = {};
			model_push_constant.debug_name = str8l("<shadows> scene push constant");
			model_push_constant.shader_stages = GraphicsShaderStage_Vertex;
			model_push_constant.size = sizeof(mat4) + sizeof(vec3);
			model_push_constant.offset = 0;
			model_push_constant.name_in_shader = "PushConstant";
			array_init_with_elements(scene_pipeline->layout->push_constants, {model_push_constant}, deshi_allocator);
			
			graphics_pipeline_layout_update(scene_pipeline->layout);
		}
		
		scene_pipeline->render_pass = graphics_render_pass_of_window_presentation_frames(win);
		
		graphics_pipeline_update(scene_pipeline);
	}
	
	GraphicsBuffer* scene_ubo_buffer = graphics_buffer_create(0, sizeof(ubo_scene), GraphicsBufferUsage_UniformBuffer,
															  GraphicsMemoryProperty_HostStreamed, GraphicsMemoryMapping_Persistent);
	scene_ubo_buffer->debug_name = str8l("<shadows> scene ubo buffer");
	
	GraphicsDescriptorSet* scene_descriptor_set = graphics_descriptor_set_allocate();{
		scene_descriptor_set->debug_name = str8l("<shadows> scene descriptor set");
		scene_descriptor_set->layouts = scene_pipeline->layout->descriptor_layouts;
		graphics_descriptor_set_update(scene_descriptor_set);
		
		auto scene_descriptors = array<GraphicsDescriptor>::create_with_count(2, deshi_allocator);
		scene_descriptors[0].type = GraphicsDescriptorType_Uniform_Buffer;
		scene_descriptors[0].shader_stages = GraphicsShaderStage_Vertex;
		scene_descriptors[0].name_in_shader = "SceneUBO";
		scene_descriptors[0].ubo = {
			scene_ubo_buffer,
			0,
			sizeof(ubo_scene),
		};
		scene_descriptors[1].type = GraphicsDescriptorType_Combined_Image_Sampler;
		scene_descriptors[1].shader_stages = GraphicsShaderStage_Fragment;
		scene_descriptors[1].name_in_shader = "shadow_map";
		scene_descriptors[1].image = {
			offscreen_framebuffer->depth_image_view,
			offscreen_sampler,
			GraphicsImageLayout_Depth_Stencil_Read_Only_Optimal,
		};
		
		scene_descriptor_set->descriptors = scene_descriptors.ptr;
		graphics_descriptor_set_write(scene_descriptor_set);
	}
	
	//-////////////////////////////////////////////////////////////////////////////////////////////////
	//// @debug_quad
	
	GraphicsPipeline* debug_quad_pipeline = graphics_pipeline_duplicate(scene_pipeline);
	{
		debug_quad_pipeline->debug_name = str8l("<shadows> debug quad pipeline");
		
		debug_quad_pipeline->vertex_shader = graphics_shader_allocate();
		debug_quad_pipeline->vertex_shader->debug_name = str8l("<shadows> debug_quad.vert");
		debug_quad_pipeline->vertex_shader->shader_stage = GraphicsShaderStage_Vertex;
		debug_quad_pipeline->vertex_shader->source = baked_shader_debug_quad_vert;
		graphics_shader_update(debug_quad_pipeline->vertex_shader);
		
		debug_quad_pipeline->fragment_shader = graphics_shader_allocate();
		debug_quad_pipeline->fragment_shader->debug_name = str8l("<shadows> debug_quad.frag"); 
		debug_quad_pipeline->fragment_shader->shader_stage = GraphicsShaderStage_Fragment;
		debug_quad_pipeline->fragment_shader->source = baked_shader_debug_quad_frag;
		graphics_shader_update(debug_quad_pipeline->fragment_shader);
		
		debug_quad_pipeline->culling = GraphicsPipelineCulling_None;
		
		debug_quad_pipeline->layout = graphics_pipeline_layout_allocate();
		{
			debug_quad_pipeline->layout->debug_name = str8l("<shadows> debug quad pipeline layout");
			
			GraphicsDescriptorSetLayout* debug_quad_descriptor_layout = graphics_descriptor_set_layout_allocate();
			debug_quad_descriptor_layout->debug_name = str8l("<shadows> scene descriptor set");
			debug_quad_descriptor_layout->bindings = array<GraphicsDescriptorSetLayoutBinding>::create_with_count(2, deshi_temp_allocator).ptr;
			debug_quad_descriptor_layout->bindings[0].n = 0;
			debug_quad_descriptor_layout->bindings[0].type = GraphicsDescriptorType_Uniform_Buffer;
			debug_quad_descriptor_layout->bindings[0].shader_stages = GraphicsShaderStage_Fragment;
			debug_quad_descriptor_layout->bindings[1].n = 1;
			debug_quad_descriptor_layout->bindings[1].type = GraphicsDescriptorType_Combined_Image_Sampler;
			debug_quad_descriptor_layout->bindings[1].shader_stages = GraphicsShaderStage_Fragment;
			graphics_descriptor_set_layout_update(debug_quad_descriptor_layout);
			array_init_with_elements(debug_quad_pipeline->layout->descriptor_layouts, {debug_quad_descriptor_layout}, deshi_allocator);
			
			graphics_pipeline_layout_update(debug_quad_pipeline->layout);
		}
		
		graphics_pipeline_update(debug_quad_pipeline);
	}
	
	GraphicsBuffer* debug_quad_ubo_buffer = graphics_buffer_create(0, sizeof(offscreen.depth), GraphicsBufferUsage_UniformBuffer,
															  GraphicsMemoryProperty_HostStreamed, GraphicsMemoryMapping_Persistent);
	debug_quad_ubo_buffer->debug_name = str8l("<shadows> debug quad ubo buffer");
	CopyMemory(graphics_buffer_mapped_data(debug_quad_ubo_buffer), &offscreen.depth, sizeof(offscreen.depth));
	
	GraphicsDescriptorSet* debug_quad_descriptor_set = graphics_descriptor_set_allocate();
	{
		debug_quad_descriptor_set->debug_name = str8l("<shadows> debug descriptor set");
		debug_quad_descriptor_set->layouts = debug_quad_pipeline->layout->descriptor_layouts;
		graphics_descriptor_set_update(debug_quad_descriptor_set);
		
		auto debug_quad_descriptors = array<GraphicsDescriptor>::create_with_count(2, deshi_allocator);
		debug_quad_descriptors[0].type = GraphicsDescriptorType_Uniform_Buffer;
		debug_quad_descriptors[0].shader_stages = GraphicsShaderStage_Fragment;
		debug_quad_descriptors[0].name_in_shader = "DebugQuadUBO";
		debug_quad_descriptors[0].ubo = {
			debug_quad_ubo_buffer,
			0,
			sizeof(offscreen.depth),
		};
		debug_quad_descriptors[1].type = GraphicsDescriptorType_Combined_Image_Sampler;
		debug_quad_descriptors[1].shader_stages = GraphicsShaderStage_Fragment;
		debug_quad_descriptors[1].name_in_shader = "shadow_map";
		debug_quad_descriptors[1].image = {
			offscreen_framebuffer->depth_image_view,
			offscreen_sampler,
			GraphicsImageLayout_Depth_Stencil_Read_Only_Optimal,
		};
		
		debug_quad_descriptor_set->descriptors = debug_quad_descriptors.ptr;
		graphics_descriptor_set_write(debug_quad_descriptor_set);
	}
	
	//-////////////////////////////////////////////////////////////////////////////////////////////////
	//// @meshes
	
	MeshVertex vertices[4] = {
		{{ 0.5f,  0.5f, 0.f}, {1.f, 0.f}, 0, {0.f, 1.f, 0.f}},
		{{-0.5f,  0.5f, 0.f}, {0.f, 0.f}, 0, {0.f, 1.f, 0.f}},
		{{-0.5f, -0.5f, 0.f}, {0.f, 1.f}, 0, {0.f, 1.f, 0.f}},
		{{ 0.5f, -0.5f, 0.f}, {1.f, 1.f}, 0, {0.f, 1.f, 0.f}},
	};
	MeshIndex indexes[6] = {0, 2, 1, 0, 3, 2};
	
	GraphicsBuffer* plane_vertex_buffer = graphics_buffer_create(vertices, 4*sizeof(MeshVertex), GraphicsBufferUsage_VertexBuffer,
																 GraphicsMemoryPropertyFlag_DeviceLocal, GraphicsMemoryMapping_Never);
	plane_vertex_buffer->debug_name = str8l("<shadows> plane vertex buffer");
	GraphicsBuffer* plane_index_buffer = graphics_buffer_create(indexes, 6*sizeof(MeshIndex), GraphicsBufferUsage_IndexBuffer,
																GraphicsMemoryPropertyFlag_DeviceLocal,GraphicsMemoryMapping_Never);
	plane_index_buffer->debug_name = str8l("<shadows> plane index buffer");
	
	struct{
		mat4 transform;
		vec3 color;
	}planes[2];
	planes[0].color = Vec3(1, 0, 0);
	planes[1].color = Vec3(0, 1, 0);
	
	struct Box{
		mat4 transform;
		vec3 color;
	}boxes[3];
	boxes[0].color = Vec3(0,1,0);
	boxes[1].color = Vec3(0,0,1);
	boxes[2].color = Vec3(1,0,0);
	
	Model* box = assets_model_create_from_obj(str8l("box.obj"), 0);
	
	//-////////////////////////////////////////////////////////////////////////////////////////////////
	//// @update
	
	auto draw_plane = [&](u32 idx, vec3 pos, vec3 rot, vec3 scale){
		planes[idx].transform = mat4::TransformationMatrix(pos, rot, scale);
		graphics_cmd_push_constant(win, GraphicsShaderStage_Vertex, &planes[idx], 0, sizeof(mat4) + sizeof(vec3));
		graphics_cmd_bind_vertex_buffer(win, plane_vertex_buffer);
		graphics_cmd_bind_index_buffer(win, plane_index_buffer);
		graphics_cmd_draw_indexed(win, 6, 0, 0);
	};
	
	auto draw_box = [&](u32 idx, vec3 pos, vec3 rot, vec3 scale){
		boxes[idx].transform = mat4::TransformationMatrix(pos, rot, scale);
		graphics_cmd_push_constant(win, GraphicsShaderStage_Vertex, &boxes[idx], 0, sizeof(mat4) + sizeof(vec3));
		graphics_cmd_bind_vertex_buffer(win, box->mesh->vertex_buffer);
		graphics_cmd_bind_index_buffer(win, box->mesh->index_buffer);
		graphics_cmd_draw_indexed(win, box->mesh->index_count, 0, 0);
	};
	
	auto update_offscreen_ubo = [&](){
		ubo_offscreen.proj = Math::PerspectiveProjectionMatrix(offscreen.width, offscreen.height, offscreen.light_hfov,
															   offscreen.depth.near, offscreen.depth.far);
		ubo_offscreen.view = Math::LookAtMatrix(ubo_scene.light_pos, Vec3(0,0,0)).Inverse();
		CopyMemory(graphics_buffer_mapped_data(offscreen_ubo_buffer), &ubo_offscreen, sizeof(ubo_offscreen));
	};
	
	auto update_scene_ubo = [&](){
		ubo_scene.          proj = camera.proj;
		ubo_scene.          view = camera.view;
		ubo_scene.    light_view = ubo_offscreen.view;
		ubo_scene.    light_proj = ubo_offscreen.proj;
		ubo_scene.time = g_time->totalTime / 1000.f;
		CopyMemory(graphics_buffer_mapped_data(scene_ubo_buffer), &ubo_scene, sizeof(ubo_scene));
	};
	
	b32 first_person = false;
	while(platform_update()){
		if(key_pressed(Key_ESCAPE)){
			break;
		}
		
		if(first_person){
			vec3 inputs = {0};
			if(key_down(Key_W))     inputs += camera.forward;
			if(key_down(Key_S))     inputs -= camera.forward;
			if(key_down(Key_D))     inputs += camera.right;
			if(key_down(Key_A))     inputs -= camera.right;
			if(key_down(Key_SPACE)) inputs += camera.up;
			if(key_down(Key_LCTRL)) inputs -= camera.up;
			
			auto rotdiffx = (g_input->mouseY - (f32)win->center.y) * 0.1f;
			auto rotdiffy = (g_input->mouseX - (f32)win->center.x) * 0.1f;
			
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
			}
		}
		if(win->resized){
			camera.proj = Math::PerspectiveProjectionMatrix(win->width, win->height, camera.hfov, camera.near_z, camera.far_z);
		}
		
		if(key_down(Key_RIGHT)){
			if(input_lshift_down()){
				ubo_scene.light_pos.x += 0.1;
			}else{
				ubo_scene.light_pos.x += 0.01;
			}
			Log("shadows", ubo_scene.light_pos);
		}
		if(key_down(Key_LEFT)){
			if(input_lshift_down()){
				ubo_scene.light_pos.x -= 0.1;
			}else{
				ubo_scene.light_pos.x -= 0.01;
			}
			Log("shadows", ubo_scene.light_pos);
		}
		if(key_down(Key_UP)){
			if(input_lshift_down()){
				ubo_scene.light_pos.z += 0.1;
			}else{
				ubo_scene.light_pos.z += 0.01;
			}
			Log("shadows", ubo_scene.light_pos);
		}
		if(key_down(Key_DOWN)){
			if(input_lshift_down()){
				ubo_scene.light_pos.z -= 0.1;
			}else{
				ubo_scene.light_pos.z -= 0.01;
			}
			Log("shadows", ubo_scene.light_pos);
		}
		if(input_lalt_down()){
			ubo_scene.light_pos.x = roundf(ubo_scene.light_pos.x);
			ubo_scene.light_pos.z = roundf(ubo_scene.light_pos.z);
			Log("shadows", ubo_scene.light_pos);
		}
		
		update_scene_ubo();
		update_offscreen_ubo();
		
		if(key_pressed(Key_C)){
			if(first_person){
				window_set_cursor_mode(win, CursorMode_Default);
			}else{
				window_set_cursor_mode(win, CursorMode_FirstPerson);
			}
			first_person = !first_person;
		}
		
		graphics_cmd_begin_render_pass(win, offscreen_framebuffer->render_pass, offscreen_framebuffer);
		
		graphics_cmd_set_viewport(win, vec2::ZERO, Vec2(offscreen.width, offscreen.height));
		graphics_cmd_set_scissor(win, vec2::ZERO, Vec2(offscreen.width, offscreen.height));
		
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
		draw_box(2, ubo_scene.light_pos, vec3::ZERO, vec3::ONE*0.2);
		
		//graphics_cmd_bind_pipeline(win, debug_quad_pipeline);
		//graphics_cmd_bind_descriptor_set(win, 0, debug_quad_descriptor_set);
		
		graphics_cmd_end_render_pass(win);
		
		graphics_update(win);
	}
}
