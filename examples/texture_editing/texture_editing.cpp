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
#include "core/time.h"
#include "core/ui.h"
#include "core/ui_widgets.h"
#include "core/window.h"
#include "math/math.h"

int main() {
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	Window* win = window_create(str8l("render_api"));
	window_show(win);
	render_init_x(win);

	struct TwodVertex {
		vec2 pos;
		vec2 uv;
	};
	typedef u32 TwodIndex;

	struct PushConstant {
		vec2 scale;
		vec2 translation;
	};

	RenderPipeline* pipeline = render_pipeline_create();
	pipeline->name = str8l("twod pipeline");
	
	array_wrap_and_push(pipeline->shader_stages, {
		{
			RenderShaderStage_Vertex,
			str8l("twod.vert"),
			file_read_simple(str8l("data/shaders/twod.vert"), deshi_temp_allocator)
		},
		{
			RenderShaderStage_Fragment,
			str8l("twod.frag"),
			file_read_simple(str8l("data/shaders/twod.frag"), deshi_temp_allocator)
		}});

	pipeline->            front_face = RenderPipelineFrontFace_CCW;
	pipeline->               culling = RenderPipelineCulling_None;
	pipeline->          polygon_mode = RenderPipelinePolygonMode_Fill;
	pipeline->            depth_test = false;
	pipeline->           color_blend = true;
	pipeline->        color_blend_op = RenderBlendOp_Add;
	pipeline->color_src_blend_factor = RenderBlendFactor_Source_Alpha;
	pipeline->color_dst_blend_factor = RenderBlendFactor_One_Minus_Source_Alpha;
	pipeline->        alpha_blend_op = RenderBlendOp_Add;
	pipeline->alpha_src_blend_factor = RenderBlendFactor_One_Minus_Source_Alpha;
	pipeline->alpha_dst_blend_factor = RenderBlendFactor_Zero;
	pipeline->        blend_constant = color(10,10,10,255);
	pipeline->           render_pass = render_pass_of_window_presentation_frame(win);

	array_wrap_and_push(pipeline->dynamic_states, {
			RenderDynamicState_Viewport,
			RenderDynamicState_Scissor});

	array_wrap_and_push(pipeline->vertex_input_bindings,
			{0, sizeof(TwodVertex)});

	array_wrap_and_push(pipeline->vertex_input_attributes,{
			{0, 0, RenderFormat_R32G32_Signed_Float, offsetof(TwodVertex, pos)},
			{1, 0, RenderFormat_R32G32_Signed_Float, offsetof(TwodVertex, uv)}});

	RenderDescriptorSetLayout* descriptor_layout = render_descriptor_layout_create();
	descriptor_layout->debug_name = str8l("twod descriptor layout");
	array_wrap_and_push(descriptor_layout->bindings, {
			{
				RenderDescriptorType_Combined_Image_Sampler,
				RenderShaderStage_Fragment,
				0
			}});
	render_descriptor_layout_update(descriptor_layout);

	RenderPushConstant twod_push_constant;
	twod_push_constant.size = sizeof(PushConstant);
	twod_push_constant.offset = 0;
	twod_push_constant.shader_stage_flags = RenderShaderStage_Vertex;

	RenderPipelineLayout* pipeline_layout = render_pipeline_layout_create();
	array_wrap_and_push(pipeline_layout->descriptor_layouts, descriptor_layout);
	array_wrap_and_push(pipeline_layout->push_constants, twod_push_constant);
	render_pipeline_layout_update(pipeline_layout);

	pipeline->layout = pipeline_layout;
	render_pipeline_update(pipeline);

	RenderBuffer* vertex_buffer = render_buffer_create(
			0, sizeof(TwodVertex) * 16, 
			RenderBufferUsage_VertexBuffer,
			RenderMemoryPropertyFlag_HostCoherent,
			RenderMemoryMapping_Persistent);

	RenderBuffer* index_buffer = render_buffer_create(
			0, sizeof(TwodIndex) * 64, 
			RenderBufferUsage_IndexBuffer,
			RenderMemoryPropertyFlag_HostCoherent,
			RenderMemoryMapping_Persistent);

	// Create the image that we'll be modifying 
	s32 width = 256,
		height = 256;

	u32* pixels = (u32*)memalloc(width*height*16);

	forI(width*height) {
		pixels[i] = randcolor.rgba;
	}

	RenderImage* image = render_image_create();
	image->format = RenderFormat_R8G8B8A8_StandardRGB;
	image->usage = (RenderImageUsage)(RenderImageUsage_Sampled | RenderImageUsage_Transfer_Destination);
	image->samples = RenderSampleCount_1;
	image->extent = {width, height, 4};
	render_image_update(image);
	render_image_upload(image, (u8*)pixels, vec2i::ZERO, {image->extent.x, image->extent.y});

	RenderImageView* image_view = render_image_view_create();
	image_view->image = image;
	image_view->format = image->format;
	image_view->aspect_flags = RenderImageViewAspectFlags_Color;
	render_image_view_update(image_view);

	RenderSampler* sampler = render_sampler_create();
	sampler->mipmaps = 1;
	sampler->mag_filter = RenderFilter_Nearest;
	sampler->min_filter = RenderFilter_Nearest;
	sampler->address_mode_u = 
	sampler->address_mode_v = 
	sampler->address_mode_w = RenderSamplerAddressMode_Clamp_To_Border;
	sampler->border_color = Color_Black;
	render_sampler_update(sampler);

	RenderDescriptorSet* descriptor_set = render_descriptor_set_create();
	descriptor_set->layouts = pipeline->layout->descriptor_layouts;
	render_descriptor_set_update(descriptor_set);

	auto descriptors = array<RenderDescriptor>::create_with_count(1, deshi_temp_allocator);
	descriptors[0].kind = RenderDescriptorType_Combined_Image_Sampler;
	descriptors[0].image = {
		image_view,
		sampler,
		RenderImageLayout_Shader_Read_Only_Optimal
	};
	render_descriptor_set_write(descriptor_set, descriptors.ptr);
	
	f32 w = win->width / 2.f;
	f32 h = w;

	TwodVertex vertexes[4] = {
		{{0, 0}, {0, 1}},
		{{w, 0}, {1, 1}},
		{{w, h}, {1, 0}},
		{{0, h}, {0, 0}},
	};

	TwodIndex indexes[6] = {0, 3, 1, 3, 2, 1};

	CopyMemory(vertex_buffer->mapped_data, vertexes, sizeof(TwodVertex)*4);
	CopyMemory(index_buffer->mapped_data, indexes, sizeof(TwodIndex)*6);

	PushConstant pc = {
		{2.f / win->width, 2.f / win->height},
		{-1.f, -1.f},
	};

	while(platform_update()) {
		{ using namespace render::cmd;
			begin_render_pass(win, render_pass_of_window_presentation_frame(win), render_current_present_frame_of_window(win));
			bind_pipeline(win, pipeline);
			push_constant(win, &pc, twod_push_constant);
			bind_vertex_buffer(win, vertex_buffer);
			bind_index_buffer(win, index_buffer);
			bind_descriptor_set(win, 0, descriptor_set);
			draw_indexed(win, 6, 0, 0);
			end_render_pass(win);
		}

		render_update_x(win);

		// no seizures here
		platform_sleep(100);

		u32 col = color(rand()%255, rand()%255, rand()%255, 255).rgba;

		forX(x, width) {
			forX(y, height) {
				pixels[y * width + x] = col;
			}
		}
		
		vec2i offset = {rand() % image->extent.x, rand() % image->extent.y};
		vec2i extent = {rand() % (image->extent.x - offset.x), rand() % (image->extent.y - offset.y)};

		render_image_upload(image, (u8*)pixels, offset, extent);
	}
}
