//// deshi memory ////
#define KIGU_ARRAY_ALLOCATOR deshi_allocator
#define KIGU_UNICODE_ALLOCATOR deshi_allocator
#define KIGU_STRING_ALLOCATOR deshi_temp_allocator
#include "core/memory.h"

#ifdef TRACY_ENABLE
#  include "Tracy.hpp"
#endif

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
	vec4 camera_pos;
	vec2 screen_dim;
} ubo;

int main() {
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	Window* win = window_create(str8l("render_api"));
	render_init_x(win);
	window_show(win);


	// we need to create a pipeline which describes the path we take
	// to render the scene
	RenderPipeline* pipeline = render_create_pipeline();
	pipeline->name = str8l("flat");
	
	*array_push(pipeline->shader_stages) = {
		RenderShaderKind_Vertex, 
		str8l("flat_vert"), 
		file_read_simple(str8l("flat.vert"), deshi_temp_allocator)
	};
	*array_push(pipeline->shader_stages) = {
		RenderShaderKind_Fragment,
		str8l("flat_frag"),
		file_read_simple(str8l("flat.frag"), deshi_temp_allocator)
	};
	
	pipeline->            front_face = RenderPipelineFrontFace_CW;
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
	pipeline->        blend_constant = color(0,0,0,0);

	*array_push(pipeline->dynamic_states) = RenderDynamicState_Viewport;
	*array_push(pipeline->dynamic_states) = RenderDynamicState_Scissor;

	// We need to describe to the renderer how our shader is going to use 
	// the data that we want to give to it.
	// This is achieved using descriptor layouts, descriptor sets, push constants
	// and pipeline layouts.
	
	// A descriptor layout describes the resources we want the shader to be able to access.
	// In this case the only resource we want is a uniform buffer, which is just a 
	// buffer of data we upload to the gpu every now and then.
	
	// The first thing we need to do is create a buffer on the GPU for our ubo data. 
	
	RenderBuffer* ubo_buffer = render_buffer_create(
		&ubo, 
		sizeof(ubo), 
		RenderBufferUsage_UniformBuffer,
		// we need to be able to map this memory so that we may 
		// update it, so it needs to be visible to the host
		RenderMemoryPropertyFlag_HostVisible | 
		// this flag indicates that the memory does not need to be flushed manually
		// the CPU/GPU will keep things in sync for us
		RenderMemoryPropertyFlag_HostCoherent,
		// indicates that we don't want to keep this memory mapped
		// but would like to map and unmap it later
		RenderMemoryMapping_MapWriteUnmap
	);

	// Now we can create the descriptor layout. This describes the layout of data
	// that we want to give to the shader. We are just adding one descriptor for
	// our UBO.
	
	RenderDescriptorLayout* descriptor_layout = render_create_descriptor_layout();
	RenderDescriptor* descriptor = array_push(descriptor_layout->descriptors);
	descriptor->kind = RenderDescriptorKind_Uniform_Buffer;
	// point the descriptor to our RenderBuffer we've just created
	descriptor->buffer.handle = ubo_buffer;
	descriptor->buffer.offset = 0;
	descriptor->buffer.range = ubo_buffer->size;
	// update the descriptor layout in the backend
	render_update_descriptor_layout(descriptor_layout);

	// We'll also be using a push constant, which is a single value that 
	// we can efficiently upload to the GPU. We will use a push constant 
	// for the model matrix, since it has to change for every single model
	// we wish to update in a frame. If this were stored in the UBO then
	// we would have to map and unmap the ubo to update one member 
	// several times per frame, as many times as we have models and as many 
	// render passes we have passing over those models. 
	
	RenderPushConstant model_push_constant;
	// we only need the model matrix in the vertex shader
	model_push_constant.shader_stage_flags = RenderShaderKind_Vertex;
	model_push_constant.size = sizeof(mat4);
	model_push_constant.offset = 0;
	
	// Now we need to specify the layout of the pipeline's data.
	// This is just a pair of a descriptor layout and a collection
	// of push constants.
	
	RenderPipelineLayout* pipeline_layout = render_create_pipeline_layout();
	// debug name shown in things like RenderDoc
	pipeline_layout->name = str8l("flat");
	pipeline_layout->descriptor_layout = descriptor_layout;
	*array_push(pipeline_layout->push_constants) = model_push_constant;
	render_update_pipeline_layout(pipeline_layout);

	// Now we tell the pipeline what layout to use and update it in the backend.
	
	pipeline->layout = pipeline_layout;
	render_update_pipeline(pipeline);

	// At this point we have described to the backend the sequence of operations we want 
	// to perform on our vertices and textures and the data the stages (shaders) use 
	// throughout this process. But this data has not yet been allocated on the GPU. In order
	// to tell the backend to allocate this information we need to create a descriptor set. 
	// A descriptor set is just an array of descriptor layouts.
	
	RenderDescriptorSet* descriptor_set = render_descriptor_set_create();
	*array_push(descriptor_set->layouts) = descriptor_layout;
	render_descriptor_set_update(descriptor_set);
	


}
