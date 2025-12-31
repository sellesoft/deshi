/* topdown_raytracing

An exploration of top-down raytracing for shading and colored transulcent materials.
The main reference is the paper "Polar-Ray Traced Lighting" By Alexander S. Griffis:
https://github.com/Yaazarai/2D-QuickRayTracing-GLSL

TODO:
stars background
tiled scene
light info
shading
glass
colored glass (8bits raylength whole, 8bits raylength fractional, 5bits red, 6bits green, 5bits blue)
upload textures as part of an array and use push constants to index into the array
*/
#include "core/assets.h"
#include "core/baked/fonts.h"
#include "core/graphics.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/ui.h"
#include "core/window.h"
#include "kigu/common.h"
#include "topdown_raytracing_shaders.cpp"

typedef u32 TileType; enum{
	TileType_Empty,
	TileType_FloorTilesLightGray,
	TileType_FloorTilesDarkGray,
	TileType_OliveWallBottomLeft,
	TileType_OliveWallBottomLeftCap,
	TileType_OliveWallBottomRight,
	TileType_OliveWallHorizontal,
	TileType_OliveWallHorizontalLeftCap,
	TileType_OliveWallHorizontalRightCap,
	TileType_OliveWallIntersection,
	TileType_OliveWallTopLeft,
	TileType_OliveWallTopLeftCap,
	TileType_OliveWallTopRight,
	TileType_OliveWallTopRightCap,
	TileType_OliveWallVertical,
	TileType_OliveWallVerticalBottomCap,
	TileType_OliveWallVerticalTopCap,
	TileType_GlassTableBottomLeft,
	TileType_GlassTableBottomRight,
	TileType_GlassTableHorizontal,
	TileType_GlassTableHorizontalLeftCap,
	TileType_GlassTableHorizontalRightCap,
	TileType_GlassTableIsland,
	TileType_GlassTableTopLeft,
	TileType_GlassTableTopRight,
	TileType_GlassTableVertical,
	TileType_GlassTableVerticalBottomCap,
	TileType_GlassTableVerticalTopCap,
	TileType_SteelTableBottomLeft,
	TileType_SteelTableBottomRight,
	TileType_SteelTableHorizontal,
	TileType_SteelTableHorizontalLeftCap,
	TileType_SteelTableHorizontalRightCap,
	TileType_SteelTableIsland,
	TileType_SteelTableTopLeft,
	TileType_SteelTableTopRight,
	TileType_SteelTableVertical,
	TileType_SteelTableVerticalBottomCap,
	TileType_SteelTableVerticalTopCap,
	TileType_COUNT
};

str8 tile_texture_paths[] = {
	str8{},
	STR8("data/FloorTilesLightGray.png"),
	STR8("data/FloorTilesDarkGray.png"),
	STR8("data/OliveWallBottomLeft.png"),
	STR8("data/OliveWallBottomLeftCap.png"),
	STR8("data/OliveWallBottomRight.png"),
	STR8("data/OliveWallHorizontal.png"),
	STR8("data/OliveWallHorizontalLeftCap.png"),
	STR8("data/OliveWallHorizontalRightCap.png"),
	STR8("data/OliveWallIntersection.png"),
	STR8("data/OliveWallTopLeft.png"),
	STR8("data/OliveWallTopLeftCap.png"),
	STR8("data/OliveWallTopRight.png"),
	STR8("data/OliveWallTopRightCap.png"),
	STR8("data/OliveWallVertical.png"),
	STR8("data/OliveWallVerticalBottomCap.png"),
	STR8("data/OliveWallVerticalTopCap.png"),
	STR8("data/GlassTableBottomLeft.png"),
	STR8("data/GlassTableBottomRight.png"),
	STR8("data/GlassTableHorizontal.png"),
	STR8("data/GlassTableHorizontalLeftCap.png"),
	STR8("data/GlassTableHorizontalRightCap.png"),
	STR8("data/GlassTableIsland.png"),
	STR8("data/GlassTableTopLeft.png"),
	STR8("data/GlassTableTopRight.png"),
	STR8("data/GlassTableVertical.png"),
	STR8("data/GlassTableVerticalBottomCap.png"),
	STR8("data/GlassTableVerticalTopCap.png"),
	STR8("data/SteelTableBottomLeft.png"),
	STR8("data/SteelTableBottomRight.png"),
	STR8("data/SteelTableHorizontal.png"),
	STR8("data/SteelTableHorizontalLeftCap.png"),
	STR8("data/SteelTableHorizontalRightCap.png"),
	STR8("data/SteelTableIsland.png"),
	STR8("data/SteelTableTopLeft.png"),
	STR8("data/SteelTableTopRight.png"),
	STR8("data/SteelTableVertical.png"),
	STR8("data/SteelTableVerticalBottomCap.png"),
	STR8("data/SteelTableVerticalTopCap.png"),
};
StaticAssert(TileType_COUNT == ArrayCount(tile_texture_paths));

struct Tile{
	TileType bg;
	TileType fg;
};

struct Light{
	struct{
		f32 x;
		f32 y;
		f32 radius;
		u32 color;
		f32 center_x;
		f32 center_y;
		f32 collision_texture_size_x;
		f32 collision_texture_size_y;
		f32 ray_texture_size;
		f32 light_texture_size;
	}info;
	GraphicsImage* image;
	GraphicsRenderPass* render_pass;
	GraphicsPipeline* pipeline;
	GraphicsFramebuffer* framebuffer;
};

struct TileVertex{
	vec2 pos;
	u16 u;
	u16 v;
	u16 layer;
};

typedef u32 TileIndex;

int main(int args_count, char** args){
	//// init systems ////
	
	memory_init(Megabytes(4), Megabytes(1));
	platform_init();
	logger_init();
	Window* window = window_create(STR8("topdown_raytracing"));
	window_show(window);
	graphics_init(window);
	assets_init(window);
	render_init(window, false);
	render_set_active_window(window);
	render_temp_init(window, 100);
	Camera* camera = render_camera_create();
	camera->forward = vec3{0,0,1};
	render_camera_set_projection(camera, mat4{128.0f / (f32)window->width, 0, 0, 0, 0, 128.0f / (f32)window->height, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
	render_camera_set_view(camera, {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -16, -16, 0, 1});
	render_set_active_camera(camera);
	
	//// set initial tiles ////
	
#define WORLD_WIDTH 64
#define WORLD_HEIGHT 64
#define WORLD_AREA (WORLD_WIDTH * WORLD_HEIGHT)
#define to_linear(x,y) ((WORLD_WIDTH*(y))+(x))
#define linear_row(pos) ((pos) % WORLD_WIDTH)
#define linear_col(pos) ((pos) / WORLD_WIDTH)
	
	Tile tiles[WORLD_AREA] = {};
	
	tiles[to_linear(10, 10)] = {TileType_FloorTilesDarkGray, TileType_OliveWallTopLeft};
	for(u32 x = 11, y = 10; x < 19; x++){
		tiles[to_linear(x, y)] = {TileType_FloorTilesDarkGray, TileType_OliveWallHorizontal};
	}
	tiles[to_linear(19, 10)] = {TileType_FloorTilesDarkGray, TileType_OliveWallTopRight};
	
	tiles[to_linear(10, 19)] = {TileType_FloorTilesDarkGray, TileType_OliveWallBottomLeft};
	for(u32 x = 11, y = 19; x < 19; x++){
		tiles[to_linear(x, y)] = {TileType_FloorTilesDarkGray, TileType_OliveWallHorizontal};
	}
	tiles[to_linear(19, 19)] = {TileType_FloorTilesDarkGray, TileType_OliveWallBottomRight};
	
	for(u32 x = 10, y = 11; y < 19; y++){
		tiles[to_linear(x, y)] = {TileType_FloorTilesDarkGray, TileType_OliveWallVertical};
	}
	
	for(u32 x = 19, y = 11; y < 19; y++){
		tiles[to_linear(x, y)] = {TileType_FloorTilesDarkGray, TileType_OliveWallVertical};
	}
	
	for(u32 y = 11; y < 19; y++){
		for(u32 x = 11; x < 19; x++){
			tiles[to_linear(x, y)] = {TileType_FloorTilesLightGray, TileType_Empty};
		}
	}
	
	tiles[to_linear(13, 13)] = {TileType_FloorTilesLightGray, TileType_GlassTableVerticalTopCap};
	tiles[to_linear(13, 14)] = {TileType_FloorTilesLightGray, TileType_GlassTableVertical};
	tiles[to_linear(13, 15)] = {TileType_FloorTilesLightGray, TileType_GlassTableVertical};
	tiles[to_linear(13, 16)] = {TileType_FloorTilesLightGray, TileType_GlassTableVertical};
	tiles[to_linear(13, 17)] = {TileType_FloorTilesLightGray, TileType_GlassTableVerticalBottomCap};
	tiles[to_linear(16, 14)] = {TileType_FloorTilesDarkGray, TileType_OliveWallVerticalTopCap};
	tiles[to_linear(16, 15)] = {TileType_FloorTilesDarkGray, TileType_OliveWallVertical};
	tiles[to_linear(16, 16)] = {TileType_FloorTilesDarkGray, TileType_OliveWallVerticalBottomCap};
	
#define MAX_LIGHTS 64
	u32 lights_count = 0;
	Light lights[MAX_LIGHTS];
	lights[0].info.x = 15.0f;
	lights[0].info.y = 15.0f;
	lights[0].info.radius = 5.0f;
	lights[0].info.color = 0xFFFFFFFF;
	
	//// create lights renderpass, framebuffers, pipeline ////
	
	GraphicsRenderPass* light_raycast_render_pass = graphics_render_pass_allocate();
	light_raycast_render_pass->debug_name = STR8("light_raycast.render_pass");
	light_raycast_render_pass->use_color_attachment = true;
	light_raycast_render_pass->color_attachment.format = GraphicsFormat_R8G8B8A8_SRGB;
	light_raycast_render_pass->color_attachment.sample_count = 1;
	light_raycast_render_pass->color_attachment.load_op = GraphicsLoadOp_Clear;
	light_raycast_render_pass->color_attachment.store_op = GraphicsStoreOp_Store;
	light_raycast_render_pass->color_attachment.stencil_load_op = GraphicsLoadOp_Dont_Care;
	light_raycast_render_pass->color_attachment.stencil_store_op = GraphicsStoreOp_Dont_Care;
	light_raycast_render_pass->color_attachment.initial_layout = GraphicsImageLayout_Undefined;
	light_raycast_render_pass->color_attachment.final_layout = GraphicsImageLayout_Shader_Read_Only_Optimal;
	graphics_render_pass_update(light_raycast_render_pass);
	
	Shader* light_raycast_frag_shader = assets_shader_load_from_source(STR8("light_raycast.frag"), baked_shader_light_raycast_frag, ShaderType_Fragment);
	Shader* light_sample_frag_shader = assets_shader_load_from_source(STR8("light_sample.frag"), baked_shader_light_sample_frag, ShaderType_Fragment);
	
	GraphicsDescriptorSetLayout* lights_descriptor_layout = graphics_descriptor_set_layout_allocate(); 
	lights_descriptor_layout->debug_name = STR8("tiles.descriptor_layout");
	array_init_with_elements(lights_descriptor_layout->bindings, {
								 {GraphicsDescriptorType_Combined_Image_Sampler, GraphicsShaderStage_Fragment, 0},
							 }, deshi_temp_allocator);
	graphics_descriptor_set_layout_update(lights_descriptor_layout);
	
	//// create stars pipeline ////
	
	Shader* fullscreen_vert_shader = assets_shader_load_from_source(STR8("fullscreen.vert"), baked_shader_fullscreen_vert, ShaderType_Vertex);
	Shader* stars_frag_shader = assets_shader_load_from_source(STR8("stars.frag"), baked_shader_stars_fragment, ShaderType_Fragment);
	
	GraphicsDescriptorSetLayout* stars_descriptor_layout = graphics_descriptor_set_layout_allocate(); 
	stars_descriptor_layout->debug_name = STR8("tiles.descriptor_layout");
	array_init_with_elements(stars_descriptor_layout->bindings, {
								 {GraphicsDescriptorType_Uniform_Buffer, GraphicsShaderStage_Fragment, 0},
							 }, deshi_temp_allocator);
	graphics_descriptor_set_layout_update(stars_descriptor_layout);
	
	GraphicsPipelineLayout* stars_pipeline_layout = graphics_pipeline_layout_allocate();
	stars_pipeline_layout->debug_name = STR8("stars.pipeline_layout");
	array_init_with_elements(stars_pipeline_layout->descriptor_layouts, {stars_descriptor_layout}, deshi_temp_allocator);
	graphics_pipeline_layout_update(stars_pipeline_layout);
	
	GraphicsPipeline* stars_pipeline = graphics_pipeline_allocate();
	stars_pipeline->debug_name = STR8("stars.pipeline");
	stars_pipeline->vertex_shader = fullscreen_vert_shader->handle;
	stars_pipeline->fragment_shader = stars_frag_shader->handle;
	array_init_with_elements(stars_pipeline->vertex_input_bindings, {{0, sizeof(TileVertex)}}, deshi_temp_allocator);
	array_init_with_elements(stars_pipeline->vertex_input_attributes, {
								 {0, 0, GraphicsFormat_R32G32_Float, OffsetOfMember(TileVertex, pos)},
								 {1, 0, GraphicsFormat_R16G16_UNorm, OffsetOfMember(TileVertex, u)},
								 {2, 0, GraphicsFormat_R16_UInt, OffsetOfMember(TileVertex, layer)},
							 }, deshi_temp_allocator);
	stars_pipeline->front_face = GraphicsFrontFace_CW;
	stars_pipeline->culling = GraphicsPipelineCulling_None;
	stars_pipeline->polygon_mode = GraphicsPolygonMode_Fill;
	stars_pipeline->depth_test = true;
	stars_pipeline->depth_compare_op = GraphicsCompareOp_Always;
	stars_pipeline->line_width = 1.0f;
	stars_pipeline->color_blend = true;
	stars_pipeline->color_blend_op = GraphicsBlendOp_Add;
	stars_pipeline->color_src_blend_factor = GraphicsBlendFactor_Source_Alpha;
	stars_pipeline->color_dst_blend_factor = GraphicsBlendFactor_One_Minus_Source_Alpha;
	stars_pipeline->alpha_blend_op = GraphicsBlendOp_Add;
	stars_pipeline->alpha_src_blend_factor = GraphicsBlendFactor_One;
	stars_pipeline->alpha_dst_blend_factor = GraphicsBlendFactor_One_Minus_Source_Alpha;
	stars_pipeline->dynamic_viewport = true;
	stars_pipeline->dynamic_scissor = true;
	stars_pipeline->render_pass = graphics_render_pass_of_window_presentation_frames(window);
	stars_pipeline->layout = stars_pipeline_layout;
	graphics_pipeline_update(stars_pipeline);
	
	//// create tiles pipeline ////
	
	Shader* tiles_vert_shader = assets_shader_load_from_source(STR8("tiles.vert"), baked_shader_tiles_vert, ShaderType_Vertex);
	Shader* tiles_frag_shader = assets_shader_load_from_source(STR8("tiles.frag"), baked_shader_tiles_frag, ShaderType_Fragment);
	
	GraphicsDescriptorSetLayout* tiles_descriptor_layout = graphics_descriptor_set_layout_allocate(); 
	tiles_descriptor_layout->debug_name = STR8("tiles.descriptor_layout");
	array_init_with_elements(tiles_descriptor_layout->bindings, {
								 {GraphicsDescriptorType_Uniform_Buffer, GraphicsShaderStage_Vertex, 0},
								 {GraphicsDescriptorType_Combined_Image_Sampler, GraphicsShaderStage_Fragment, 1}
							 }, deshi_temp_allocator);
	graphics_descriptor_set_layout_update(tiles_descriptor_layout);
	
	GraphicsPipelineLayout* tiles_pipeline_layout = graphics_pipeline_layout_allocate();
	tiles_pipeline_layout->debug_name = STR8("tiles.pipeline_layout");
	array_init_with_elements(tiles_pipeline_layout->descriptor_layouts, {tiles_descriptor_layout}, deshi_temp_allocator);
	graphics_pipeline_layout_update(tiles_pipeline_layout);
	
	GraphicsPipeline* tiles_pipeline = graphics_pipeline_duplicate(stars_pipeline);
	tiles_pipeline->debug_name = STR8("tiles.pipeline");
	tiles_pipeline->vertex_shader = tiles_vert_shader->handle;
	tiles_pipeline->fragment_shader = tiles_frag_shader->handle;
	tiles_pipeline->layout = tiles_pipeline_layout;
	graphics_pipeline_update(tiles_pipeline);
	
	//// upload resources to device ////
	
	struct{
		mat4 projection;
		mat4 view;
		f32 resolution_and_time[4]; //x, y = resolution, z = time, w = unused
	}ubo;
	
	ubo.projection = camera->proj;
	ubo.view = camera->view;
	ubo.resolution_and_time[0] = (f32)window->width;
	ubo.resolution_and_time[1] = (f32)window->height;
	ubo.resolution_and_time[2] = (f32)g_time->totalTime / 1000.0f;
	
	GraphicsBuffer* ubo_buffer = graphics_buffer_create(&ubo, sizeof(ubo), GraphicsBufferUsage_UniformBuffer, GraphicsMemoryProperty_DeviceMappable, GraphicsMemoryMapping_Occasional);
	
	Texture* tiles_texture = assets_texture_create(STR8("tiles.texture_array"), 64, 64, TileType_COUNT, ImageFormat_RGBA, TextureType_Array_TwoDimensional, TextureFilter_Nearest, TextureAddressMode_ClampToEdge);
	{
		void* empty_pixels = memory_talloc(64*64*ImageFormat_RGBA);
		assets_texture_update(tiles_texture, empty_pixels, {0, 0}, {64, 64}, 0);
	}
	for(u32 i = 1; i < TileType_COUNT; i++){
		assets_texture_update_from_file(tiles_texture, tile_texture_paths[i], {0, 0}, {64, 64}, i);
	}
	
	Texture* light_collision_texture = assets_texture_create(STR8("tiles.texture_array"), WORLD_WIDTH, WORLD_HEIGHT, 1, ImageFormat_RGBA, TextureType_Array_TwoDimensional, TextureFilter_Nearest, TextureAddressMode_ClampToEdge);
	void* light_collision_pixels = memory_alloc(WORLD_WIDTH*WORLD_HEIGHT*ImageFormat_RGBA);
	assets_texture_update(light_collision_texture, light_collision_pixels, {0, 0}, {WORLD_WIDTH, WORLD_HEIGHT}, 0);
	
	GraphicsDescriptorSet* stars_descriptor_set = graphics_descriptor_set_allocate();
	stars_descriptor_set->debug_name = STR8("stars.descriptor_set");
	array_init_with_elements(stars_descriptor_set->layouts, {stars_descriptor_layout}, deshi_temp_allocator);
	graphics_descriptor_set_update(stars_descriptor_set);
	array_init_with_count(stars_descriptor_set->descriptors, 1, deshi_temp_allocator);
	stars_descriptor_set->descriptors[0].type = GraphicsDescriptorType_Uniform_Buffer;
	stars_descriptor_set->descriptors[0].shader_stages = GraphicsShaderStage_Fragment;
	stars_descriptor_set->descriptors[0].name_in_shader = "UBO";
	stars_descriptor_set->descriptors[0].ubo.offset = 0;
	stars_descriptor_set->descriptors[0].ubo.range = sizeof(ubo);
	stars_descriptor_set->descriptors[0].ubo.buffer = ubo_buffer;
	graphics_descriptor_set_write(stars_descriptor_set);
	
	GraphicsDescriptorSet* tiles_descriptor_set = graphics_descriptor_set_allocate();
	tiles_descriptor_set->debug_name = STR8("tiles.descriptor_set");
	array_init_with_elements(tiles_descriptor_set->layouts, {tiles_descriptor_layout}, deshi_temp_allocator);
	graphics_descriptor_set_update(tiles_descriptor_set);
	array_init_with_count(tiles_descriptor_set->descriptors, 2, deshi_temp_allocator);
	tiles_descriptor_set->descriptors[0].type = GraphicsDescriptorType_Uniform_Buffer;
	tiles_descriptor_set->descriptors[0].shader_stages = GraphicsShaderStage_Vertex;
	tiles_descriptor_set->descriptors[0].name_in_shader = "UBO";
	tiles_descriptor_set->descriptors[0].ubo.offset = 0;
	tiles_descriptor_set->descriptors[0].ubo.range = sizeof(ubo);
	tiles_descriptor_set->descriptors[0].ubo.buffer = ubo_buffer;
	tiles_descriptor_set->descriptors[1].type = GraphicsDescriptorType_Combined_Image_Sampler;
	tiles_descriptor_set->descriptors[1].shader_stages = GraphicsShaderStage_Fragment;
	tiles_descriptor_set->descriptors[1].name_in_shader = "texture_array";
	tiles_descriptor_set->descriptors[1].image.view = tiles_texture->image_view;
	tiles_descriptor_set->descriptors[1].image.sampler = tiles_texture->sampler;
	tiles_descriptor_set->descriptors[1].image.layout = GraphicsImageLayout_Shader_Read_Only_Optimal;
	graphics_descriptor_set_write(tiles_descriptor_set);
	
	TileVertex fullscreen_vertexes[4] = {
		{vec2{0.0f, 0.0f}, 0, MAX_U16, 0},
		{vec2{1.0f, 0.0f}, MAX_U16, MAX_U16, 0},
		{vec2{1.0f, 1.0f}, MAX_U16, 0, 0},
		{vec2{0.0f, 1.0f}, 0, 0, 0},
	};
	GraphicsBuffer* fullscreen_vertex_buffer = graphics_buffer_create(fullscreen_vertexes, 4*sizeof(TileVertex), GraphicsBufferUsage_VertexBuffer, GraphicsMemoryProperty_DeviceOnly, GraphicsMemoryMapping_Never);
	fullscreen_vertex_buffer->debug_name = STR8("fullscreen.vertex_buffer");
	
	TileIndex fullscreen_indexes[6] = { 0, 1, 2, 0, 2, 3 };
	GraphicsBuffer* fullscreen_index_buffer = graphics_buffer_create(fullscreen_indexes, 6*sizeof(TileIndex), GraphicsBufferUsage_IndexBuffer, GraphicsMemoryProperty_DeviceOnly, GraphicsMemoryMapping_Never);
	fullscreen_index_buffer->debug_name = STR8("fullscreen.index_buffer");
	
	GraphicsBuffer* tiles_vertex_buffer = graphics_buffer_create(0, 4*WORLD_AREA*sizeof(TileVertex), GraphicsBufferUsage_VertexBuffer, GraphicsMemoryProperty_DeviceMappable, GraphicsMemoryMapping_Occasional);
	tiles_vertex_buffer->debug_name = STR8("tiles.vertex_buffer");
	
	TileIndex* tiles_indexes = (TileIndex*)memory_talloc(6*WORLD_AREA*sizeof(TileIndex));
	for (u32 i = 0, j = 0; i < 6*WORLD_AREA; j++, i += 6)
	{
		tiles_indexes[i+0] = 4*j + 0;
		tiles_indexes[i+1] = 4*j + 1;
		tiles_indexes[i+2] = 4*j + 2;
		tiles_indexes[i+3] = 4*j + 0;
		tiles_indexes[i+4] = 4*j + 2;
		tiles_indexes[i+5] = 4*j + 3;
	}
	GraphicsBuffer* tiles_index_buffer = graphics_buffer_create(tiles_indexes, 6*WORLD_AREA*sizeof(TileIndex), GraphicsBufferUsage_IndexBuffer, GraphicsMemoryProperty_DeviceOnly, GraphicsMemoryMapping_Never);
	tiles_index_buffer->debug_name = STR8("tiles.index_buffer");
	
	//// start updating ////
	
	while(platform_update()){
		render_camera_set_projection(camera, mat4{128.0f / (f32)window->width, 0, 0, 0, 0, 128.0f / (f32)window->height, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
		render_camera_set_view(camera, {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -15, -15, 0, 1});
		ubo.projection = camera->proj;
		ubo.view = camera->view;
		ubo.resolution_and_time[0] = (f32)window->width;
		ubo.resolution_and_time[1] = (f32)window->height;
		ubo.resolution_and_time[2] = (f32)g_time->totalTime / 1000.0f;
		void* mapped_ubo = graphics_buffer_map(ubo_buffer, graphics_buffer_device_size(ubo_buffer), 0);
		CopyMemory(mapped_ubo, &ubo, sizeof(ubo));
		graphics_buffer_unmap(ubo_buffer, true);
		
		graphics_cmd_begin_render_pass(window, graphics_current_present_frame_of_window(window)->render_pass, graphics_current_present_frame_of_window(window));
		
		graphics_cmd_bind_pipeline(window, stars_pipeline);
		graphics_cmd_set_viewport(window, {0,0}, vec2{(f32)window->width, (f32)window->height});
		graphics_cmd_set_scissor(window, {0,0}, vec2{(f32)window->width, (f32)window->height});
		graphics_cmd_bind_vertex_buffer(window, fullscreen_vertex_buffer);
		graphics_cmd_bind_index_buffer(window, fullscreen_index_buffer);
		graphics_cmd_bind_descriptor_set(window, 0, stars_descriptor_set);
		graphics_cmd_draw_indexed(window, 6, 0, 0);
		
		//NOTE(delle) update indexing logic below if this changes
		StaticAssert(OffsetOfMember(Tile, bg) == 0*sizeof(TileType));
		StaticAssert(OffsetOfMember(Tile, fg) == 1*sizeof(TileType));
		
		graphics_cmd_bind_pipeline(window, tiles_pipeline);
		graphics_cmd_set_viewport(window, {0,0}, vec2{(f32)window->width, (f32)window->height});
		graphics_cmd_set_scissor(window, {0,0}, vec2{(f32)window->width, (f32)window->height});
		u32 vertex_count = 0;
		TileVertex* mapped_vertexes = (TileVertex*)graphics_buffer_map(tiles_vertex_buffer, graphics_buffer_device_size(tiles_vertex_buffer), 0);
		for(u32 i = 0; i < WORLD_AREA; i++){
			for(u32 j = 0; j < 2; j++){
				TileType tile_type = (j == 0) ? tiles[i].bg : tiles[i].fg;
				if(tile_type == TileType_Empty){
					continue;
				}
				
				vec2 pos = {(f32)linear_row(i), (f32)linear_col(i)};
				
				mapped_vertexes[vertex_count].pos = pos + vec2{0.0f, 0.0f};
				mapped_vertexes[vertex_count].u = 0;
				mapped_vertexes[vertex_count].v = MAX_U16;
				mapped_vertexes[vertex_count].layer = tile_type;
				vertex_count++;
				
				mapped_vertexes[vertex_count].pos = pos + vec2{1.0f, 0.0f};
				mapped_vertexes[vertex_count].u = MAX_U16;
				mapped_vertexes[vertex_count].v = MAX_U16;
				mapped_vertexes[vertex_count].layer = tile_type;
				vertex_count++;
				
				mapped_vertexes[vertex_count].pos = pos + vec2{1.0f, 1.0f};
				mapped_vertexes[vertex_count].u = MAX_U16;
				mapped_vertexes[vertex_count].v = 0;
				mapped_vertexes[vertex_count].layer = tile_type;
				vertex_count++;
				
				mapped_vertexes[vertex_count].pos = pos + vec2{0.0f, 1.0f};
				mapped_vertexes[vertex_count].u = 0;
				mapped_vertexes[vertex_count].v = 0;
				mapped_vertexes[vertex_count].layer = tile_type;
				vertex_count++;
			}
		}
		graphics_buffer_unmap(tiles_vertex_buffer, true);
		graphics_cmd_bind_vertex_buffer(window, tiles_vertex_buffer);
		graphics_cmd_bind_index_buffer(window, tiles_index_buffer);
		graphics_cmd_bind_descriptor_set(window, 0, tiles_descriptor_set);
		graphics_cmd_draw_indexed(window, ((vertex_count / 4) * 6), 0, 0);
		
		graphics_cmd_end_render_pass(window);
		
		render_update();
		graphics_update(window);
		logger_update();
		render_temp_clear();
		memory_clear_temp();
	}
	
	logger_cleanup();
	memory_cleanup();
}
