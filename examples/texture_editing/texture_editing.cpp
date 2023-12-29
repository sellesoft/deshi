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
#include "math/math.h"
#include "core/render.h"

int main() {
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	Window* win = window_create(str8l("render_api"));
	window_show(win);
	graphics_init(win);

	struct TwodVertex {
		vec2 pos;
		vec2 uv;
	};
	typedef u32 TwodIndex;

	struct PushConstant {
		vec2 scale;
		vec2 translation;
	};

	GraphicsPipeline* pipeline = graphics_pipeline_allocate();
	pipeline->debug_name = str8l("twod pipeline");
	pipeline->vertex_shader = graphics_shader_allocate();
	pipeline->vertex_shader->debug_name = str8l("twod.vert");
	pipeline->vertex_shader->shader_stage = GraphicsShaderStage_Vertex;
	pipeline->vertex_shader->source = file_read_simple(str8l("twod.vert"), deshi_temp_allocator);
	graphics_shader_update(pipeline->vertex_shader);
	pipeline->fragment_shader = graphics_shader_allocate();
	pipeline->fragment_shader->debug_name = str8l("twod.frag");
	pipeline->fragment_shader->shader_stage = GraphicsShaderStage_Fragment;
	pipeline->fragment_shader->source = file_read_simple(str8l("twod.frag"), deshi_temp_allocator);
	graphics_shader_update(pipeline->fragment_shader);

	pipeline->            front_face = GraphicsFrontFace_CCW;
	pipeline->               culling = GraphicsPipelineCulling_None;
	pipeline->          polygon_mode = GraphicsPolygonMode_Fill;
	pipeline->            depth_test = false;
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
			{{0, sizeof(TwodVertex)}}, deshi_temp_allocator);

	array_init_with_elements(pipeline->vertex_input_attributes,{
			{0, 0, GraphicsFormat_R32G32_Float, offsetof(TwodVertex, pos)},
			{1, 0, GraphicsFormat_R32G32_Float, offsetof(TwodVertex, uv)}});

	GraphicsDescriptorSetLayout* descriptor_layout = graphics_descriptor_set_layout_allocate();
	descriptor_layout->debug_name = str8l("twod descriptor layout");
	array_init_with_elements(descriptor_layout->bindings, {
			{
				GraphicsDescriptorType_Combined_Image_Sampler,
				GraphicsShaderStage_Fragment,
				0
			}});
	graphics_descriptor_set_layout_update(descriptor_layout);

	GraphicsPushConstant twod_push_constant;
	twod_push_constant.size = sizeof(PushConstant);
	twod_push_constant.offset = 0;
	twod_push_constant.shader_stages = GraphicsShaderStage_Vertex;

	GraphicsPipelineLayout* pipeline_layout = graphics_pipeline_layout_allocate();
	array_init_with_elements(pipeline_layout->descriptor_layouts, {descriptor_layout}, deshi_temp_allocator);
	array_init_with_elements(pipeline_layout->push_constants, {twod_push_constant}, deshi_temp_allocator);
	graphics_pipeline_layout_update(pipeline_layout);

	pipeline->layout = pipeline_layout;
	graphics_pipeline_update(pipeline);

	GraphicsBuffer* vertex_buffer = graphics_buffer_create(
			0, sizeof(TwodVertex) * 16, 
			GraphicsBufferUsage_VertexBuffer,
			GraphicsMemoryPropertyFlag_HostCoherent,
			GraphicsMemoryMapping_Persistent);

	GraphicsBuffer* index_buffer = graphics_buffer_create(
			0, sizeof(TwodIndex) * 64, 
			GraphicsBufferUsage_IndexBuffer,
			GraphicsMemoryPropertyFlag_HostCoherent,
			GraphicsMemoryMapping_Persistent);

	// Create the image that we'll be modifying 
	s32 width = 256,
		height = 256;

	u32* pixels = (u32*)memalloc(width*height*16);

	forI(width*height) {
		pixels[i] = randcolor.rgba;
	}

	GraphicsImage* image = graphics_image_allocate();
	image->format = GraphicsFormat_R8G8B8A8_SRGB;
	image->usage = GraphicsImageUsage_Sampled | GraphicsImageUsage_Transfer_Destination;
	image->samples = GraphicsSampleCount_1;
	image->extent = {width, height, 4};
	graphics_image_update(image);
	graphics_image_write(image, (u8*)pixels, vec2i::ZERO, {image->extent.x, image->extent.y});

	GraphicsImageView* image_view = graphics_image_view_allocate();
	image_view->image = image;
	image_view->format = image->format;
	image_view->aspect_flags = GraphicsImageViewAspectFlags_Color;
	graphics_image_view_update(image_view);

	GraphicsSampler* sampler = graphics_sampler_allocate();
	sampler->mag_filter = GraphicsFilter_Nearest;
	sampler->min_filter = GraphicsFilter_Nearest;
	sampler->address_mode_u = 
	sampler->address_mode_v = 
	sampler->address_mode_w = GraphicsSamplerAddressMode_Clamp_To_Border;
	sampler->border_color = Color_Black;
	graphics_sampler_update(sampler);
	
	GraphicsDescriptor* descriptors = array_create(GraphicsDescriptor, 1, deshi_allocator);
	GraphicsDescriptor* descriptor = array_push(descriptors);
	descriptor->type = GraphicsDescriptorType_Combined_Image_Sampler;
	descriptor->image = {
		image_view,
		sampler,
		GraphicsImageLayout_Shader_Read_Only_Optimal
	};

	GraphicsDescriptorSet* descriptor_set = graphics_descriptor_set_allocate();
	descriptor_set->descriptors = descriptors;
	descriptor_set->layouts = array_copy(pipeline->layout->descriptor_layouts).ptr;
	graphics_descriptor_set_update(descriptor_set);
	graphics_descriptor_set_write(descriptor_set);
	
	f32 w = win->width / 2.f;
	f32 h = w;

	TwodVertex vertexes[4] = {
		{{0, 0}, {0, 1}},
		{{w, 0}, {1, 1}},
		{{w, h}, {1, 0}},
		{{0, h}, {0, 0}},
	};

	TwodIndex indexes[6] = {0, 3, 1, 3, 2, 1};

	CopyMemory(graphics_buffer_mapped_data(vertex_buffer), vertexes, sizeof(TwodVertex)*4);
	CopyMemory(graphics_buffer_mapped_data(index_buffer), indexes, sizeof(TwodIndex)*6);

	PushConstant pc = {
		{2.f / win->width, 2.f / win->height},
		{-1.f, -1.f},
	};

	while(platform_update()) {
		{ using namespace graphics::cmd;
			begin_render_pass(win, graphics_render_pass_of_window_presentation_frames(win), graphics_current_present_frame_of_window(win));
			set_viewport(win, vec2::ZERO, win->dimensions.toVec2());
			set_scissor(win, vec2::ZERO, win->dimensions.toVec2());
			bind_pipeline(win, pipeline);
			push_constant(win, GraphicsShaderStage_Vertex, &pc, 0, sizeof(PushConstant));
			bind_vertex_buffer(win, vertex_buffer);
			bind_index_buffer(win, index_buffer);
			bind_descriptor_set(win, 0, descriptor_set);
			draw_indexed(win, 6, 0, 0);
			end_render_pass(win);
		}

		graphics_update(win);

		// no seizures her
		platform_sleep(100);

		u32 col = color(rand()%255, rand()%255, rand()%255, 255).rgba;

		forX(x, width) {
			forX(y, height) {
				pixels[y * width + x] = col;
			}
		}
		
		vec2i offset = {rand() % image->extent.x, rand() % image->extent.y};
		vec2i extent = {rand() % (image->extent.x - offset.x), rand() % (image->extent.y - offset.y)};

		graphics_image_write(image, (u8*)pixels, offset, extent);
	}
}
