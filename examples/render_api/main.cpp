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

// We're gonna use stb directly so we can show examples of 
// RenderImage w/o having to go through assets
#include "stb/stb_image.h"

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
	window_show(win);
	render_init_x(win);
	assets_init();
	ui_init_x(win);

	// we need to create a pipeline which describes the path we take
	// to render the scene
	RenderPipeline* pipeline = render_pipeline_create();
	pipeline->name = str8l("flat");
	
	// We'll need a vertex and a fragment shader which we acquire by pushing
	// them onto the pipeline's shader stages. The backend will handle compilation.
	*array_push(pipeline->shader_stages) = {
		RenderShaderStage_Vertex, 
		str8l("test"), 
		file_read_simple(str8l("test.vert"), deshi_temp_allocator)
	};

	*array_push(pipeline->shader_stages) = {
		RenderShaderStage_Fragment,
		str8l("test"),
		file_read_simple(str8l("test.frag"), deshi_temp_allocator)
	};
	
	// We need to set all of the properties of the pipeline so that it can correctly
	// render our scenes.
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

	// We need to give the renderer information about the data we are going to be 
	// giving to the vertex shader stage through buffers. We define the binding our 
	// data will be delivered on (0) and then the size of each element of the data. 
	// Finally, we describe how that data is laid out.

	*array_push(pipeline->vertex_input_bindings) = {0, sizeof(MeshVertex)};
	array_grow(pipeline->vertex_input_attributes, 4);
	array_count(pipeline->vertex_input_attributes) = 4;
	pipeline->vertex_input_attributes[0] = {0, 0, RenderFormat_R32G32B32_Signed_Float,      offsetof(MeshVertex, pos)};
	pipeline->vertex_input_attributes[1] = {1, 0, RenderFormat_R32G32_Signed_Float,         offsetof(MeshVertex, uv)};
	pipeline->vertex_input_attributes[2] = {2, 0, RenderFormat_R8G8B8A8_UnsignedNormalized, offsetof(MeshVertex, color)};
	pipeline->vertex_input_attributes[3] = {3, 0, RenderFormat_R32G32B32_Signed_Float,      offsetof(MeshVertex, normal)};

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
	// that we want to give to the shaders. We're going to need a descriptor for
	// our UBO and another one for a texture.
	
	RenderDescriptorLayout* descriptor_layout = render_descriptor_layout_create();
	descriptor_layout->debug_name = str8l("test");
	
	// First is the UBO. It is bound to slot 0 and used in the vertex shader.
	RenderDescriptorLayoutBinding* binding = array_push(descriptor_layout->bindings);
	binding->kind = RenderDescriptorType_Uniform_Buffer;
	binding->shader_stages = RenderShaderStage_Vertex;
	binding->binding = 0;

	// Then the texture, which we bind to slot 1 and use in the fragment shader.
	binding = array_push(descriptor_layout->bindings);
	binding->kind = RenderDescriptorType_Combined_Image_Sampler;
	binding->shader_stages = RenderShaderStage_Fragment;
	binding->binding = 1;

	// update the descriptor layout in the backend
	render_descriptor_layout_update(descriptor_layout);

	// We'll also be using a push constant, which is a single value that 
	// we can efficiently upload to the GPU. We will use a push constant 
	// for the model matrix since it has to change for every single model
	// we wish to update in a frame. If this were stored in the UBO then
	// we would have to map and unmap the ubo to update one member 
	// several times per frame, as many times as we have models and as many 
	// render passes we have passing over those models. 
	
	RenderPushConstant model_push_constant;
	// we only need the model matrix in the vertex shader
	model_push_constant.shader_stage_flags = RenderShaderStage_Vertex;
	model_push_constant.size = sizeof(mat4);
	model_push_constant.offset = 0;
	
	// Now we need to specify the layout of the pipeline's data.
	// This is just a pair of a collection descriptor layouts 
	// and a collection of push constants.
	
	RenderPipelineLayout* pipeline_layout = render_pipeline_layout_create();
	pipeline_layout->debug_name = str8l("test");
	*array_push(pipeline_layout->descriptor_layouts)  = descriptor_layout;
	*array_push(pipeline_layout->push_constants) = model_push_constant;
	render_pipeline_layout_update(pipeline_layout);

	// Now we tell the pipeline what layout to use and update it in the backend.
	
	pipeline->layout = pipeline_layout;
	render_update_pipeline(pipeline);

	// At this point we have described to the backend the sequence of operations we want 
	// to perform on our vertices and textures and the data the stages (shaders) use 
	// throughout this process. But this data has not yet been allocated on the GPU. In order
	// to tell the backend to allocate this information we need to create a descriptor set. 
	// A descriptor set is just an array of descriptor layouts.
	
	RenderDescriptorSet* descriptor_set0 = render_descriptor_set_create();
	// It's important that we use the same layouts that we gave to the pipeline.
	// If we don't, we'll run into problems when we want to bind this set.
	descriptor_set0->layouts = pipeline->layout->descriptor_layouts;
	render_descriptor_set_update(descriptor_set0);

	// The descriptor set isn't fully setup yet. We still need to give it the uniform buffer
	// and the texture we wish to use. We already have the UBO, so now we just need the texture.
	
	// First, we'll load the texture into memory using stb.	
	s32 width, height, channels;
	u8* pixels = stbi_load("data/textures/null128.png", &width, &height, &channels, STBI_rgb_alpha);

	// Next, we'll create a RenderImage, which is similar to a RenderBuffer except 
	// multidimentional.
	RenderImage* image = render_image_create();
	image->format = RenderFormat_R8G8B8A8_StandardRGB;
	// We're going to be sampling this image in our shader and the
	// image is going to be the destination of a transfer when we upload
	// our texture pixels to it.
	image->usage = (RenderImageUsage)(RenderImageUsage_Sampled | RenderImageUsage_Transfer_Destination);
	// We only care for sampling the image once (and atm we only support 1 sample!)
	image->samples = RenderSampleCount_1;
	// The size of our image as well as how many channels we have
	image->extent = {width, height, channels};

	// This call will create the necessary backend information for the information
	render_image_update(image);
	// but to actually upload it we call this function
	render_image_upload(image, pixels);

	// Great, now we just need to create an image view of our image and a sampler
	// for the shader to use.
	
	RenderImageView* image_view = render_image_view_create();
	image_view->image = image;
	image_view->format = image->format;
	image_view->aspect_flags = RenderImageViewAspectFlags_Color;
	render_image_view_update(image_view);

	RenderSampler* sampler = render_sampler_create();
	sampler->mipmaps = 1;
	// This determines how the image is filtered when we get close to it.
	sampler->mag_filter = RenderFilter_Nearest;
	// And this when we get far from it.
	sampler->min_filter = RenderFilter_Nearest;
	// Here we decide how the sampler behaves when we try to read
	// beyond [0,1) over each axis. We will just clamp to border
	// which will just give black outside the bounds of the image.
	sampler->address_mode_u = 
	sampler->address_mode_v = 
	sampler->address_mode_w = RenderSamplerAddressMode_Clamp_To_Border;
	// The color used when we are in clamp to border mode
	sampler->border_color = Color_Black;
	render_sampler_update(sampler);

	// Now we have all the information we need to finally setup the descriptor set.
	// We need to give it an array of RenderDescriptors to write to the set.
	
	RenderDescriptor* descriptors;
	array_init(descriptors, 2, deshi_allocator);
	array_count(descriptors) = 2;

	descriptors[0].         kind = RenderDescriptorType_Uniform_Buffer;
	descriptors[0].shader_stages = RenderShaderStage_Vertex;
	descriptors[0].buffer.offset = 0;
	descriptors[0]. buffer.range = sizeof(ubo);
	descriptors[0].buffer.handle = ubo_buffer;

	descriptors[1].         kind = RenderDescriptorType_Combined_Image_Sampler;
	descriptors[1].shader_stages = RenderShaderStage_Fragment;
	descriptors[1]. image.layout = RenderImageLayout_Shader_Read_Only_Optimal;
	descriptors[1].image.sampler = sampler;
	descriptors[1].   image.view = image_view;

	render_descriptor_set_write(descriptor_set0, descriptors);

	// We're actually going to use two textures, to show descriptor set switching.
	// So we'll do pretty much everything we just did again but with a different texture.
	
	RenderDescriptorSet* descriptor_set1 = render_descriptor_set_create();
	descriptor_set1->layouts = pipeline->layout->descriptor_layouts;
	render_descriptor_set_update(descriptor_set1);

	pixels = stbi_load("data/textures/alex.png", &width, &height, &channels, STBI_rgb_alpha);

	image = render_image_create();
	image->format = RenderFormat_R8G8B8A8_StandardRGB;
	image->usage = (RenderImageUsage)(RenderImageUsage_Sampled | RenderImageUsage_Transfer_Destination);
	image->samples = RenderSampleCount_1;
	image->extent = {width, height, channels};
	render_image_update(image);
	render_image_upload(image, pixels);

	image_view = render_image_view_create();
	image_view->image = image;
	image_view->format = image->format;
	image_view->aspect_flags = RenderImageViewAspectFlags_Color;
	render_image_view_update(image_view);

	sampler = render_sampler_create();
	sampler->mipmaps = 1;
	sampler->mag_filter = RenderFilter_Nearest;
	sampler->min_filter = RenderFilter_Nearest;
	sampler->address_mode_u = 
	sampler->address_mode_v = 
	sampler->address_mode_w = RenderSamplerAddressMode_Clamp_To_Border;
	sampler->border_color = Color_Black;
	render_sampler_update(sampler);

	array_clear(descriptors);
	array_grow(descriptors, 2);
	array_count(descriptors) = 2;

	descriptors[0].         kind = RenderDescriptorType_Uniform_Buffer;
	descriptors[0].shader_stages = RenderShaderStage_Vertex;
	descriptors[0].buffer.offset = 0;
	descriptors[0]. buffer.range = sizeof(ubo);
	descriptors[0].buffer.handle = ubo_buffer;

	descriptors[1].         kind = RenderDescriptorType_Combined_Image_Sampler;
	descriptors[1].shader_stages = RenderShaderStage_Fragment;
	descriptors[1]. image.layout = RenderImageLayout_Shader_Read_Only_Optimal;
	descriptors[1].image.sampler = sampler;
	descriptors[1].   image.view = image_view;

	render_descriptor_set_write(descriptor_set1, descriptors);

	// Now we need a model to render. Assets has a full api for generating 
	// a model without ever needing to touch the render api, but we're going to
	// manually create a plane mesh which we'll use twice to display two 
	// overlapping images. The reason we won't be using Assets is because it 
	// would hide a large part of the usage of the render api, which is what
	// we're trying to show here.
		
	// We'll setup the vertices that our planes will use.
	// The order of initialization is:
	// 		pos (vec3), uv (vec2), color (u32), normal (vec3)
	MeshVertex vertices[4] = {
		{{ 0.5f,  0.5f, 0.f}, {1.f, 0.f}, 0, {0.f, 1.f, 0.f}},
		{{-0.5f,  0.5f, 0.f}, {0.f, 0.f}, 0, {0.f, 1.f, 0.f}},
		{{-0.5f, -0.5f, 0.f}, {0.f, 1.f}, 0, {0.f, 1.f, 0.f}},
		{{ 0.5f, -0.5f, 0.f}, {1.f, 1.f}, 0, {0.f, 1.f, 0.f}},
	};

	// Then we need to define the two triangles of the plane with indicies
	MeshIndex indexes[6] = {0, 2, 1, 0, 3, 2};

	// Next we'll need RenderBuffers so that we can get this 
	// data onto the gpu. For both buffers, the memory is going 
	// to be local to the GPU and we don't plan to remap
	// the memory in this program either. The calls to these functions
	// map, write, and unmap the buffers automatically, so there's no need
	// for us to do it manually like we will with our UBOs in a moment.
	
	RenderBuffer* vertex_buffer = render_buffer_create(
			vertices,
			sizeof(MeshVertex) * 4,
			RenderBufferUsage_VertexBuffer,
			RenderMemoryPropertyFlag_DeviceLocal,
			RenderMemoryMapping_None);

	RenderBuffer* index_buffer = render_buffer_create(
			indexes,
			sizeof(MeshIndex) * 6,
			RenderBufferUsage_IndexBuffer,
			RenderMemoryPropertyFlag_DeviceLocal,
			RenderMemoryMapping_None);

	// Now we can start initializing the scene.
	 
	// First we'll map our ubo buffer and write some initial information to it.
	render_buffer_map(ubo_buffer, 0, ubo_buffer->size);

	ubo.proj = Camera::MakePerspectiveProjectionMatrix(win->width, win->height, 50, 1000, 0.1);
	ubo.view = Math::LookAtMatrix(vec3{0,0,0}, vec3{0,0,1}).Inverse();
	ubo.camera_pos = {0,0,0,0};
	ubo.screen_dim = Vec2(win->width, win->height);

	CopyMemory(ubo_buffer->mapped_data, &ubo, sizeof(ubo));

	render_buffer_unmap(ubo_buffer, true);

	// Next we'll setup some information needed to have a controllable camera.
	vec3 up       = {0}, 
		 right    = {0}, 
		 forward  = {0},
	     position = {0}, 
		 rotation = {0};

	forward = vec3_normalized(vec3_FORWARD() * mat4::RotationMatrix(rotation));
	right   = vec3_normalized(vec3_cross(vec3_UP(), forward));
	up      = vec3_normalized(vec3_cross(forward, right));

	b32 fps = false;

	// We need to create some transformation matrices that describe how 
	// the planes are positioned in the world.
	
	mat4 plane_transform0 = mat4::TransformationMatrix(
			{0, 0, 2},
			{0, 0, 0},
			{1, 1, 1});

	mat4 plane_transform1 = mat4::TransformationMatrix(
			{1, 0, 3},
			{0, 0, 0},
			{1, 1, 1});

	while(platform_update()) {
		
		// In order for the backend to know what to draw we need to issue it 
		// commands. These instruct the backend to do various things such as 
		// bind a pipeline, a descriptor set, buffers, etc. and to draw from
		// those buffers.
		
		// We need to give commands to the frame that we're currently working with
		// which the api provides a function for retrieving 
		RenderFrame* frame = render_current_present_frame_of_window(win);
		RenderCommand* c;

		// We have to bind the pipeline first.
		c = array_push(frame->commands);
		c->type = RenderCommandType_Bind_Pipeline;
		c->bind_pipeline.handle = pipeline;

		// Then we push the transformation push constant for our first plane.
		// Note that this doesn't copy the memory, so it must
		// still be around by the time we update the renderer.
		c = array_push(frame->commands);
		c->type = RenderCommandType_Push_Constant;
		c->push_constant.data = &plane_transform0;
		c->push_constant.info = model_push_constant;

		// We must bind the vertex and index buffers our model
		// is using. These were generated when we asked assets to
		// make our box mesh, so we can get them from it directly.
		c = array_push(frame->commands);
		c->type = RenderCommandType_Bind_Vertex_Buffer;
		c->bind_vertex_buffer.handle = vertex_buffer;
		c = array_push(frame->commands);
		c->type = RenderCommandType_Bind_Index_Buffer;
		c->bind_index_buffer.handle = index_buffer;

		// We need the descriptor set so we know what data we're actually
		// going to be using in the shader.
		c = array_push(frame->commands);
		c->type = RenderCommandType_Bind_Descriptor_Set;
		c->bind_descriptor_set.handle = descriptor_set0;

		// Finally we tell the backend that we want to draw the first plane.
		c = array_push(frame->commands);
		c->type = RenderCommandType_Draw_Indexed;
		c->draw_indexed.index_count = 6;
		c->draw_indexed.index_offset = 0;
		c->draw_indexed.vertex_offset = 0;

		// Now we change the push constant and descriptor set and draw again
		c = array_push(frame->commands);
		c->type = RenderCommandType_Push_Constant;
		c->push_constant.info = model_push_constant;
		c->push_constant.data = &plane_transform1;
		c = array_push(frame->commands);
		c->type = RenderCommandType_Bind_Descriptor_Set;
		c->bind_descriptor_set.handle = descriptor_set1;
		c = array_push(frame->commands);
		c->type = RenderCommandType_Draw_Indexed;
		c->draw_indexed.index_count = 6;
		c->draw_indexed.index_offset = 0;
		c->draw_indexed.vertex_offset = 0;


		render_update_x(win);

		if(key_pressed(Key_ESCAPE)) {
			break;
		}

		if(fps) {
			vec3 inputs = {0};
			if(key_down(Key_W))     inputs += forward;
			if(key_down(Key_S))     inputs -= forward;
			if(key_down(Key_D))     inputs += right;
			if(key_down(Key_A))     inputs -= right;
			if(key_down(Key_SPACE)) inputs += up;
			if(key_down(Key_LCTRL)) inputs -= up;

			auto rotdiffx = (g_input->mouseY - (f32)g_window->center.y) * 0.1f;
			auto rotdiffy = (g_input->mouseX - (f32)g_window->center.x) * 0.1f;

			if(rotdiffx || rotdiffy || inputs.x || inputs.y || inputs.z) {
				rotation.y += rotdiffy;
				rotation.x -= rotdiffx;

				f32 multiplier = 8.f;
				if(input_lshift_down()) multiplier = 32.f;
				else if(input_lalt_down()) multiplier = 4.f;

				position += inputs * multiplier * (g_time->deltaTime / 1000);
				rotation.x = Clamp(rotation.x, -80.f, 80.f);
				if(rotation.y >  1440.f) rotation.y -= 1440.f;
				if(rotation.y < -1440.f) rotation.y += 1440.f;

				forward = vec3_normalized(vec3_FORWARD() * mat4::RotationMatrix(rotation));
				right   = vec3_normalized(vec3_cross(vec3_UP(), forward));
				up      = vec3_normalized(vec3_cross(forward, right));
				
				ubo.camera_pos = Vec4(position.x, position.y, position.z, 0);
				ubo.view = Math::LookAtMatrix(position, position + forward).Inverse();
				
				Log("", inputs, ubo.view.arr[0], rotation);

				render_buffer_map(ubo_buffer, 0, ubo_buffer->size);
				CopyMemory(ubo_buffer->mapped_data, &ubo, sizeof(ubo));
				render_buffer_unmap(ubo_buffer, true);
			}
		}

		if(key_pressed(Key_C)) {
			if(fps)
				window_set_cursor_mode(win, CursorMode_Default);
			else
				window_set_cursor_mode(win, CursorMode_FirstPerson);
			fps = !fps;
		}

	}
}
