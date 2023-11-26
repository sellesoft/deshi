#define AssetsError(...)  LogE("assets", __FUNCTION__, "(): ", __VA_ARGS__)
#define AssetsFatal(...) do { AssetsError(__VA_ARGS__); Assert(0); } while(0)
#define AssetsWarning(...) LogW("assets", __FUNCTION__, "(): ", __VA_ARGS__)
#define AssetsNotice(...) Log("assets", __FUNCTION__, "(): ", __VA_ARGS__)
#define AssetsAssert(cond, ...) do { if(!(cond)) AssetsFatal("assert failed: `", STRINGIZE(cond), "`: ", __VA_ARGS__); } while(0)
#define AssetsAssertName(name) AssetsAssert(name, "all resources created through assets must be given a name and this name must be unique among all other resources of the same type.")
#define AssetsExistanceWarning(name, type) AssetsWarning("a ", STRINGIZE(type), " resource with the name '", name, "' already exists. All resources must be given a name unique among all other resources of the same type. The handle to the original resource will be returned.")
#define AssetsResourceBasicInfo(x) "{'", x->name, "', ", x->uid, "}@", (void*)x
#define AssetsDeleteNotFound(x, type) AssetsError("the given " STRINGIZE(type), AssetsResourceBasicInfo(x), "has a uid that does not exist in the " STRINGIZE(type) " map. None of the memory owned by this resource will be altered or deleted as we cannot be sure it is valid.")

//NOTE(delle) creating an allocator here to either use 256 bytes locally or temp allocate more than 256 bytes
persist u8 assets_line_buffer[256];
persist Allocator assets_load_allocator{
	[](upt bytes){
		if(bytes > 256){
			return memory_talloc(bytes);
		}else{
			assets_line_buffer[bytes-1] = '\0'; //NOTE(delle) file_read_line_alloc() requests an extra byte for null-terminator
			return (void*)assets_line_buffer;
		}
	},
	Allocator_ReleaseMemory_Noop,
	Allocator_ResizeMemory_Noop
};

// default ubo used on all materials for the basic information needed to render them
struct ModelUniformBufferObject {
	mat4 view;
	mat4 proj;
} __assets_model_ubo;

struct __Deshi_Assets_Internal {
	array<GraphicsVertexInputBindingDescription> vertex_input_bindings;
	array<GraphicsVertexInputAttributeDescription> vertex_input_attributes;
} __deshi_assets_internal;

#define internal __deshi_assets_internal

template<typename T> FORCE_INLINE pair<spt, b32>
__find_resource(u64 uid, T** map) {
	if(!array_count(map)) return {0, 0};
	spt index = -1;
	spt middle = -1;
	if(array_count(map)) {
		spt left = 0;
		spt right = array_count(map) - 1;
		while(left <= right ) {
			middle = left+(right-left)/2;
			if(map[middle]->uid == uid) {
				index = middle;
				break;
			}
			if(map[middle]->uid < uid) {
				left = middle+1;
				middle = left+(right-left)/2;
			} else {
				right = middle-1;
			}
		}
	}
	return {middle, index != -1};
}

FORCE_INLINE u64
__uid_of_name(str8 name) {
	return str8_hash64(name);
}

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @assets_system
void
assets_init(Window* window) {
	DeshiStageInitStart(DS_ASSETS, DS_RENDER, "Attempted to initialize Assets module before initializing Graphics module");
	
	//create the assets directories if they don't exist already
	file_create(STR8("data/fonts/"));
	file_create(STR8("data/models/"));
	file_create(STR8("data/textures/"));

	// setup pools
	memory_pool_init(g_assets->mesh_pool, 8);
	memory_pool_init(g_assets->texture_pool, 8);
	memory_pool_init(g_assets->material_pool, 8);
	memory_pool_init(g_assets->model_pool, 8);
	memory_pool_init(g_assets->font_pool, 8);
	memory_pool_init(g_assets->ubo_pool, 8);
	memory_pool_init(g_assets->shader_pool, 8);

	array_init(g_assets->mesh_map, 256, deshi_allocator);
	array_init(g_assets->texture_map, 256, deshi_allocator);
	array_init(g_assets->material_map, 256, deshi_allocator);
	array_init(g_assets->model_map, 256, deshi_allocator);
	array_init(g_assets->font_map, 8, deshi_allocator);
	array_init(g_assets->shader_map, 256, deshi_allocator);

	g_assets->render_pass = graphics_render_pass_of_window_presentation_frames(window);
	
	//setup stb_image.h
	stbi_set_flip_vertically_on_load(true);
	
	// setup null assets
	
	g_assets->null_vertex_shader = assets_shader_load_from_file(str8l("null.vert"), ShaderType_Vertex);
	g_assets->null_fragment_shader = assets_shader_load_from_file(str8l("null.frag"), ShaderType_Fragment);
	array_init_with_elements(g_assets->null_fragment_shader->resources, {(ShaderResourceType)ShaderResourceType_Texture});

	g_assets->null_mesh = assets_mesh_create_box(str8l("null"), 1.f, 1.f, 1.f, Color_White.rgba);
	g_assets->null_mesh->name = str8l("null");
	
	g_assets->base_ubo_handle = assets_ubo_create(sizeof(g_assets->base_ubo));

	int width, height, channels;
	u8* pixels = stbi_load_from_memory(baked_texture_null128_png, sizeof(baked_texture_null128_png), &width, &height, &channels, STBI_rgb_alpha);
	g_assets->null_texture = assets_texture_create_from_memory(
															   pixels, str8l("null"), 128, 128,
															   ImageFormat_RGBA,
															   TextureType_TwoDimensional,
															   TextureFilter_Nearest,
															   TextureAddressMode_Repeat, 
															   false);
	stbi_image_free(pixels);
	
	// create null pipeline 
	g_assets->null_pipeline = graphics_pipeline_allocate();
	g_assets->null_pipeline->vertex_shader = g_assets->null_vertex_shader->handle;
	g_assets->null_pipeline->fragment_shader = g_assets->null_fragment_shader->handle;

	assets_setup_pipeline(g_assets->null_pipeline);
	
	// create a descriptor layout for a single uniform buffer
	// (always set 0 in asset compatible pipelines)
	g_assets->ubo_layout = graphics_descriptor_set_layout_allocate();
	g_assets->ubo_layout->debug_name = str8l("ubo descriptor layout");
	array_init_with_elements(g_assets->ubo_layout->bindings, {
				{
					GraphicsDescriptorType_Uniform_Buffer,
					GraphicsShaderStage_Vertex,
					0
				}});
	graphics_descriptor_set_layout_update(g_assets->ubo_layout);
	
	g_assets->view_proj_ubo = graphics_descriptor_set_allocate();
	array_init_with_elements(g_assets->view_proj_ubo->layouts, {g_assets->ubo_layout});
	graphics_descriptor_set_update(g_assets->view_proj_ubo);
	
	array_init(g_assets->ubo_descriptors, 1, deshi_temp_allocator);
	auto d = array_push(g_assets->ubo_descriptors);
	d->type = GraphicsDescriptorType_Uniform_Buffer;
	d->ubo.buffer = g_assets->base_ubo_handle->buffer;
	d->ubo.offset = 0;
	d->ubo.range = sizeof(g_assets->base_ubo);
	graphics_descriptor_set_write_array(g_assets->view_proj_ubo, g_assets->ubo_descriptors);
	
	ShaderStages null_stages = {
		g_assets->null_vertex_shader, 0, g_assets->null_fragment_shader
	};
	
	auto resources = array<ShaderResource>::create(deshi_allocator);
	auto r = resources.push();
	r->type = ShaderResourceType_Texture;
	r->texture = assets_texture_null();

	g_assets->null_material = assets_material_create(str8l("null"), null_stages, resources.ptr);
	
	// TODO(sushi) setup a function for creating a font from memory if possible 
	g_assets->null_font = memory_pool_push(g_assets->font_pool);
	g_assets->null_font->type = FontType_NONE;
	g_assets->null_font->max_width = 6;
	g_assets->null_font->max_height = 12;
	g_assets->null_font->count = 1;
	g_assets->null_font->name = str8l("null");
	g_assets->null_font->uid = __uid_of_name(str8l("null"));
	array_push_value(g_assets->font_map, g_assets->null_font);
	u8 white_pixels[4] = {255,255,255,255};
	auto tex = assets_texture_create_from_memory(
					 white_pixels, 
					 str8l("null font"),
					 2, 2, ImageFormat_BW, 
					 TextureType_TwoDimensional,
					 TextureFilter_Nearest,
					 TextureAddressMode_ClampToWhite,
					 false);
	g_assets->null_font->tex = tex;
	
	DeshiStageInitEnd(DS_ASSETS);
}

void
assets_reset(){DPZoneScoped;
	for_array(g_assets->mesh_map) assets_mesh_delete(*it);
	for_array(g_assets->texture_map) assets_texture_delete(*it);
	for_array(g_assets->material_map) assets_material_delete(*it);
	for_array(g_assets->model_map) assets_model_delete(*it);
	for_array(g_assets->font_map) assets_font_delete(*it);
	array_clear(g_assets->mesh_map);
	array_clear(g_assets->texture_map);
	array_clear(g_assets->material_map);
	array_clear(g_assets->model_map);
	array_clear(g_assets->font_map);
}

void 
assets_update_camera_view(mat4* view_matrix) {
	auto buffer = g_assets->base_ubo_handle->buffer;
	void* data = graphics_buffer_map(buffer, sizeof(g_assets->base_ubo), 0);
	
	g_assets->base_ubo.view = *view_matrix;
	CopyMemory(data, &g_assets->base_ubo, sizeof(g_assets->base_ubo));
	
	graphics_buffer_unmap(buffer, true);
}

void 
assets_update_camera_projection(mat4* projection) {
	auto buffer = g_assets->base_ubo_handle->buffer;
	void* data = graphics_buffer_map(buffer, sizeof(g_assets->base_ubo), 0);
	
	g_assets->base_ubo.proj = *projection;
	CopyMemory(data, &g_assets->base_ubo, sizeof(g_assets->base_ubo));
	
	graphics_buffer_unmap(buffer, true);
}

void 
assets_setup_pipeline(GraphicsPipeline* pipeline) {
	// a pipeline shouldn't have anything in its vertex input arrays when this is called 
	Assert(!(pipeline->vertex_input_bindings || pipeline->vertex_input_attributes));

	pipeline->vertex_input_bindings = array<GraphicsVertexInputBindingDescription>::create({{0, sizeof(MeshVertex)}}, deshi_allocator).ptr;
	pipeline->vertex_input_attributes = array<GraphicsVertexInputAttributeDescription>::create({
			{0, 0, GraphicsFormat_R32G32B32_Float, offsetof(MeshVertex, pos)},
			{1, 0, GraphicsFormat_R32G32_Float,    offsetof(MeshVertex, uv)},
			{2, 0, GraphicsFormat_R8G8B8A8_UNorm,  offsetof(MeshVertex, color)},
			{3, 0, GraphicsFormat_R32G32B32_Float, offsetof(MeshVertex, normal)}}, deshi_allocator).ptr;

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
	pipeline->           render_pass = g_assets->render_pass;
	
	pipeline->dynamic_viewport = true;
	pipeline->dynamic_scissor = true;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @mesh


Mesh*
assets_mesh_allocate(u32 index_count, 
		             u32 vertex_count, 
					 u32 face_count, 
					 u32 triangles_neighbor_count, 
					 u32 faces_vertex_count, 
					 u32 faces_outer_vertex_count, 
					 u32 faces_neighbor_triangle_count, 
					 u32 faces_neighbor_face_count){DPZoneScoped;
	AssetsAssert(index_count && vertex_count && face_count, "");
	
	u32 triangle_count = index_count/3;
	u32 bytes =                       1*sizeof(Mesh)
		+                  vertex_count*sizeof(MeshVertex)
		+                   index_count*sizeof(MeshIndex)
		+                triangle_count*sizeof(MeshTriangle)
		+                    face_count*sizeof(MeshFace)
		+      triangles_neighbor_count*sizeof(u32) //triangle neighbors
		+      triangles_neighbor_count*sizeof(u8)  //triangle edges
		+                triangle_count*sizeof(u32) //face triangles
		+            faces_vertex_count*sizeof(u32)
		+      faces_outer_vertex_count*sizeof(u32)
		+ faces_neighbor_triangle_count*sizeof(u32)
		+     faces_neighbor_face_count*sizeof(u32);
	
	Mesh* mesh = (Mesh*)memory_alloc(bytes);  char* cursor = (char*)mesh + (1*sizeof(Mesh));
	mesh->bytes          = bytes;
	mesh->index_count    = index_count;
	mesh->vertex_count   = vertex_count;
	mesh->triangle_count = triangle_count;
	mesh->face_count     = face_count;
	mesh->total_tri_neighbor_count       = triangles_neighbor_count;
	mesh->total_face_vertex_count        = faces_vertex_count;
	mesh->total_face_outer_vertex_count  = faces_outer_vertex_count;
	mesh->total_face_tri_neighbor_count  = faces_neighbor_triangle_count;
	mesh->total_face_face_neighbor_count = faces_neighbor_face_count;
	
	mesh->vertex_array   = (MeshVertex*)cursor;    cursor +=   vertex_count*sizeof(MeshVertex);
	mesh->index_array    = (MeshIndex*)cursor;     cursor +=    index_count*sizeof(MeshIndex);
	mesh->triangle_array = (MeshTriangle*)cursor;  cursor += triangle_count*sizeof(MeshTriangle);
	mesh->face_array     = (MeshFace*)cursor;      cursor +=     face_count*sizeof(MeshFace);
	
	return mesh;
}

// Create GraphicsBuffers for the mesh and then writes
// their vertex/index data to them.
void
map_mesh(Mesh* m) {
	m->vertex_buffer = graphics_buffer_create(
											m->vertex_array, 
											sizeof(MeshVertex) * m->vertex_count,
											GraphicsBufferUsage_VertexBuffer,
											GraphicsMemoryPropertyFlag_DeviceLocal,
											GraphicsMemoryMapping_Never);
	m->index_buffer = graphics_buffer_create(
										   m->index_array,
										   sizeof(MeshIndex) * m->index_count,
										   GraphicsBufferUsage_IndexBuffer,
										   GraphicsMemoryPropertyFlag_DeviceLocal,
										   GraphicsMemoryMapping_Never);
}

Mesh*
assets_mesh_create_box(str8 name, f32 width, f32 height, f32 depth, u32 color){DPZoneScoped;
	AssetsAssertName(name);
	AssetsNotice("creating box mesh '", name, "'");

	//TODO(delle) change this to take in 8 points
	u64 uid = __uid_of_name(name);

	width  /= 2.f;
	height /= 2.f;
	depth  /= 2.f;
	
	// get potential position in map and 
	// check if already created
	auto [index, found] = __find_resource(uid, g_assets->mesh_map);
	if(found) {
		AssetsExistanceWarning(name, Mesh);
		return g_assets->mesh_map[index];
	}
	

	Mesh* mesh = assets_mesh_allocate(36, 8, 6, 36, 24, 24, 24, 24);
	mesh->name      = str8l("box_mesh");
	mesh->aabb_min  = {-width,-height,-depth};
	mesh->aabb_max  = { width, height, depth};
	mesh->center    = {  0.0f,   0.0f,  0.0f};
	
	MeshVertex*   va = mesh->vertex_array;
	MeshIndex*    ia = mesh->index_array;
	MeshTriangle* ta = mesh->triangle_array;
	MeshFace*     fa = mesh->face_array;
	vec3 p{width, height, depth};
	vec2 uv{0.0f, 0.0f};
	f32 ir3 = 1.0f / M_SQRT_THREE; // inverse root 3 (component of point on unit circle)
	
	//vertex array {pos, uv, color, normal(from center)}
	va[0]={{-p.x, p.y, p.z}, uv, color, {-ir3, ir3, ir3}}; // -x, y, z  0
	va[1]={{-p.x,-p.y, p.z}, uv, color, {-ir3,-ir3, ir3}}; // -x,-y, z  1
	va[2]={{-p.x, p.y,-p.z}, uv, color, {-ir3, ir3,-ir3}}; // -x, y,-z  2
	va[3]={{-p.x,-p.y,-p.z}, uv, color, {-ir3,-ir3,-ir3}}; // -x,-y,-z  3
	va[4]={{ p.x, p.y, p.z}, uv, color, { ir3, ir3, ir3}}; //  x, y, z  4
	va[5]={{ p.x,-p.y, p.z}, uv, color, { ir3,-ir3, ir3}}; //  x,-y, z  5
	va[6]={{ p.x, p.y,-p.z}, uv, color, { ir3, ir3,-ir3}}; //  x, y,-z  6
	va[7]={{ p.x,-p.y,-p.z}, uv, color, { ir3,-ir3,-ir3}}; //  x,-y,-z  7
	
	//index array
	ia[ 0]=4; ia[ 1]=2; ia[ 2]=0;    ia[ 3]=4; ia[ 4]=6; ia[ 5]=2; // +y face
	ia[ 6]=2; ia[ 7]=7; ia[ 8]=3;    ia[ 9]=2; ia[10]=6; ia[11]=7; // -z face
	ia[12]=6; ia[13]=5; ia[14]=7;    ia[15]=6; ia[16]=4; ia[17]=5; // +x face
	ia[18]=1; ia[19]=7; ia[20]=5;    ia[21]=1; ia[22]=3; ia[23]=7; // -y face
	ia[24]=0; ia[25]=3; ia[26]=1;    ia[27]=0; ia[28]=2; ia[29]=3; // -x face
	ia[30]=4; ia[31]=1; ia[32]=5;    ia[33]=4; ia[35]=0; ia[35]=1; // +z face
	
	//triangle array
	ta[ 0].normal = vec3::UP;      ta[ 1].normal = vec3::UP;
	ta[ 2].normal = vec3::BACK;    ta[ 3].normal = vec3::BACK;
	ta[ 4].normal = vec3::RIGHT;   ta[ 5].normal = vec3::RIGHT;
	ta[ 6].normal = vec3::DOWN;    ta[ 7].normal = vec3::DOWN;
	ta[ 8].normal = vec3::LEFT;    ta[ 9].normal = vec3::LEFT;
	ta[10].normal = vec3::FORWARD; ta[11].normal = vec3::FORWARD;
	for(s32 i = 0; i < 12; ++i){
		ta[i].p[0] = va[ia[(i*3)+0]].pos;
		ta[i].p[1] = va[ia[(i*3)+1]].pos;
		ta[i].p[2] = va[ia[(i*3)+2]].pos;
		ta[i].v[0] = ia[(i*3)+0];
		ta[i].v[1] = ia[(i*3)+1];
		ta[i].v[2] = ia[(i*3)+2];
		ta[i].neighbor_count= 3;
		ta[i].face = i / 2;
	}
	
	//triangle array neighbor array offsets
	ta[0].neighbor_array = (u32*)(fa + 6);
	ta[0].edge_array     = (u8*)(ta[0].neighbor_array + 36);
	for(s32 i = 1; i < 12; ++i){
		ta[i].neighbor_array = ta[i-1].neighbor_array+3;
		ta[i].edge_array     = ta[i-1].edge_array+3;
	}
	
	//triangle array neighbors array
	ta[ 0].neighbor_array[0]= 1; ta[ 0].neighbor_array[1]=11; ta[ 0].neighbor_array[2]= 9;
	ta[ 1].neighbor_array[0]= 0; ta[ 1].neighbor_array[1]= 3; ta[ 1].neighbor_array[2]= 5;
	ta[ 2].neighbor_array[0]= 3; ta[ 2].neighbor_array[1]= 9; ta[ 2].neighbor_array[2]= 7;
	ta[ 3].neighbor_array[0]= 2; ta[ 3].neighbor_array[1]= 1; ta[ 3].neighbor_array[2]= 4;
	ta[ 4].neighbor_array[0]= 5; ta[ 4].neighbor_array[1]= 3; ta[ 4].neighbor_array[2]= 6;
	ta[ 5].neighbor_array[0]= 4; ta[ 5].neighbor_array[1]= 1; ta[ 5].neighbor_array[2]=10;
	ta[ 6].neighbor_array[0]= 7; ta[ 6].neighbor_array[1]= 4; ta[ 6].neighbor_array[2]=10;
	ta[ 7].neighbor_array[0]= 6; ta[ 7].neighbor_array[1]= 2; ta[ 7].neighbor_array[2]= 8;
	ta[ 8].neighbor_array[0]= 9; ta[ 8].neighbor_array[1]=11; ta[ 8].neighbor_array[2]= 7;
	ta[ 9].neighbor_array[0]= 8; ta[ 9].neighbor_array[1]= 0; ta[ 9].neighbor_array[2]= 2;
	ta[10].neighbor_array[0]=11; ta[10].neighbor_array[1]= 5; ta[10].neighbor_array[2]= 6;
	ta[11].neighbor_array[0]=10; ta[11].neighbor_array[1]= 0; ta[11].neighbor_array[2]= 8;
	
	//triangle array edges array
	ta[ 0].edge_array[0]=0; ta[ 0].edge_array[1]=2; ta[ 0].edge_array[2]=1;
	ta[ 1].edge_array[0]=2; ta[ 1].edge_array[1]=1; ta[ 1].edge_array[2]=0;
	ta[ 2].edge_array[0]=0; ta[ 2].edge_array[1]=2; ta[ 2].edge_array[2]=1;
	ta[ 3].edge_array[0]=2; ta[ 3].edge_array[1]=0; ta[ 3].edge_array[2]=1;
	ta[ 4].edge_array[0]=0; ta[ 4].edge_array[1]=2; ta[ 4].edge_array[2]=1;
	ta[ 5].edge_array[0]=2; ta[ 5].edge_array[1]=0; ta[ 5].edge_array[2]=1;
	ta[ 6].edge_array[0]=0; ta[ 6].edge_array[1]=1; ta[ 6].edge_array[2]=2;
	ta[ 7].edge_array[0]=2; ta[ 7].edge_array[1]=1; ta[ 7].edge_array[2]=0;
	ta[ 8].edge_array[0]=0; ta[ 8].edge_array[1]=2; ta[ 8].edge_array[2]=1;
	ta[ 9].edge_array[0]=2; ta[ 9].edge_array[1]=0; ta[ 9].edge_array[2]=1;
	ta[10].edge_array[0]=0; ta[10].edge_array[1]=2; ta[10].edge_array[2]=1;
	ta[11].edge_array[0]=2; ta[11].edge_array[1]=0; ta[11].edge_array[2]=1;
	
	//face array  0=up, 1=back, 2=right, 3=down, 4=left, 5=forward
	for(s32 i = 0; i < 6; ++i){
		fa[i].normal                = ta[i*2].normal;
		fa[i].center                = ta[i*2].normal * p;
		fa[i].triangle_count         = 2;
		fa[i].vertex_count           = 4;
		fa[i].outer_vertex_count      = 4;
		fa[i].neighbor_triangle_count = 4;
		fa[i].neighbor_face_count     = 4;
	}
	
	//face array triangle array offsets
	fa[0].triangle_array = (u32*)(ta[0].edge_array + 36);
	for(s32 i = 1; i < 6; ++i){
		fa[i].triangle_array = fa[i-1].triangle_array+2;
	}
	
	//face array triangle arrays
	for(s32 i = 0; i < 6; ++i){
		fa[i].triangle_array[0]= i*2;
		fa[i].triangle_array[1]=(i*2)+1;
	}
	
	//face array vertex array offsets
	fa[0].vertex_array      = (u32*)(fa[0].triangle_array+12);
	fa[0].outer_vertex_array = (u32*)(fa[0].vertex_array+24);
	for(s32 i = 1; i < 6; ++i){
		fa[i].vertex_array      = fa[i-1].vertex_array+4;
		fa[i].outer_vertex_array = fa[i-1].outer_vertex_array+4;
	}
	
	//face array vertex array
	fa[0].vertex_array[0]=0; fa[0].vertex_array[1]=4; fa[0].vertex_array[2]=6; fa[0].vertex_array[3]=2; // +y
	fa[1].vertex_array[0]=2; fa[1].vertex_array[1]=6; fa[1].vertex_array[2]=7; fa[1].vertex_array[3]=3; // -z
	fa[2].vertex_array[0]=6; fa[2].vertex_array[1]=4; fa[2].vertex_array[2]=5; fa[2].vertex_array[3]=7; // +x
	fa[3].vertex_array[0]=3; fa[3].vertex_array[1]=7; fa[3].vertex_array[2]=5; fa[3].vertex_array[3]=1; // -y
	fa[4].vertex_array[0]=0; fa[4].vertex_array[1]=2; fa[4].vertex_array[2]=3; fa[4].vertex_array[3]=1; // -x
	fa[5].vertex_array[0]=4; fa[5].vertex_array[1]=0; fa[5].vertex_array[2]=1; fa[5].vertex_array[3]=5; // +z
	
	//face array outer vertex array
	fa[0].outer_vertex_array[0]=0; fa[0].outer_vertex_array[1]=4; fa[0].outer_vertex_array[2]=6; fa[0].outer_vertex_array[3]=2; // +y
	fa[1].outer_vertex_array[0]=2; fa[1].outer_vertex_array[1]=6; fa[1].outer_vertex_array[2]=7; fa[1].outer_vertex_array[3]=3; // -z
	fa[2].outer_vertex_array[0]=6; fa[2].outer_vertex_array[1]=4; fa[2].outer_vertex_array[2]=5; fa[2].outer_vertex_array[3]=7; // +x
	fa[3].outer_vertex_array[0]=3; fa[3].outer_vertex_array[1]=7; fa[3].outer_vertex_array[2]=5; fa[3].outer_vertex_array[3]=1; // -y
	fa[4].outer_vertex_array[0]=0; fa[4].outer_vertex_array[1]=2; fa[4].outer_vertex_array[2]=3; fa[4].outer_vertex_array[3]=1; // -x
	fa[5].outer_vertex_array[0]=4; fa[5].outer_vertex_array[1]=0; fa[5].outer_vertex_array[2]=1; fa[5].outer_vertex_array[3]=5; // +z
	
	//face array neighbor array offsets
	fa[0].neighbor_triangle_array = (u32*)(fa[0].outer_vertex_array+24);
	fa[0].neighbor_face_array     = (u32*)(fa[0].neighbor_triangle_array+24);
	for(s32 i = 1; i < 6; ++i){
		fa[i].neighbor_triangle_array = fa[i-1].neighbor_triangle_array+4;
		fa[i].neighbor_face_array     = fa[i-1].neighbor_face_array+4;
	}
	
	//face array neighbor triangle array
	fa[0].neighbor_triangle_array[0]= 9; fa[0].neighbor_triangle_array[1]= 3; fa[0].neighbor_triangle_array[2]= 5; fa[0].neighbor_triangle_array[3]=11;
	fa[1].neighbor_triangle_array[0]= 1; fa[1].neighbor_triangle_array[1]= 4; fa[1].neighbor_triangle_array[2]= 7; fa[1].neighbor_triangle_array[3]= 9;
	fa[2].neighbor_triangle_array[0]= 1; fa[2].neighbor_triangle_array[1]=10; fa[2].neighbor_triangle_array[2]= 6; fa[2].neighbor_triangle_array[3]= 3;
	fa[3].neighbor_triangle_array[0]= 4; fa[3].neighbor_triangle_array[1]=10; fa[3].neighbor_triangle_array[2]= 8; fa[3].neighbor_triangle_array[3]= 2;
	fa[4].neighbor_triangle_array[0]= 0; fa[4].neighbor_triangle_array[1]= 2; fa[4].neighbor_triangle_array[2]= 7; fa[4].neighbor_triangle_array[3]=11;
	fa[5].neighbor_triangle_array[0]= 0; fa[5].neighbor_triangle_array[1]= 8; fa[5].neighbor_triangle_array[2]= 6; fa[5].neighbor_triangle_array[3]= 5;
	
	//face array neighbor face array
	fa[0].neighbor_face_array[0]=1; fa[0].neighbor_face_array[1]=2; fa[0].neighbor_face_array[2]=4; fa[0].neighbor_face_array[3]=5;
	fa[1].neighbor_face_array[0]=0; fa[1].neighbor_face_array[1]=2; fa[1].neighbor_face_array[2]=3; fa[1].neighbor_face_array[3]=4;
	fa[2].neighbor_face_array[0]=0; fa[2].neighbor_face_array[1]=1; fa[2].neighbor_face_array[2]=3; fa[2].neighbor_face_array[3]=5;
	fa[3].neighbor_face_array[0]=1; fa[3].neighbor_face_array[1]=2; fa[3].neighbor_face_array[2]=4; fa[3].neighbor_face_array[3]=5;
	fa[4].neighbor_face_array[0]=0; fa[4].neighbor_face_array[1]=1; fa[4].neighbor_face_array[2]=3; fa[4].neighbor_face_array[3]=5;
	fa[5].neighbor_face_array[0]=0; fa[5].neighbor_face_array[1]=2; fa[5].neighbor_face_array[2]=3; fa[5].neighbor_face_array[3]=4;
	
	map_mesh(mesh);
	array_insert_value(g_assets->mesh_map, index, mesh);
	return mesh;
}

Mesh*
assets_mesh_create_from_file(str8 name){DPZoneScoped;
	AssetsAssertName(name);
	if(str8_equal_lazy(name, STR8("null"))) return assets_mesh_null();

	//prepend the meshes (models) folder
	dstr8 builder;
	dstr8_init(&builder, STR8("data/models/"), deshi_temp_allocator);
	dstr8_append(&builder, name);
	
	//append extension if not provided
	str8 front = str8_eat_until_last(name, '.');
	if(front.count == name.count) dstr8_append(&builder, STR8(".mesh"));
	
	return assets_mesh_create_from_path(dstr8_peek(&builder));
}


Mesh*
assets_mesh_create_from_path(str8 path){DPZoneScoped;
	AssetsAssertName(path);
	str8 contents = file_read_simple(path, deshi_temp_allocator);
	if(!contents) {
		AssetsError("failed to load mesh file.");
		return assets_mesh_null();
	}
	
	return assets_mesh_create_from_memory(contents.str);
}


Mesh*
assets_mesh_create_from_memory(void* data){DPZoneScoped;
	AssetsAssert(data, "passed null data pointer.");
	u32 bytes = *((u32*)data);
	if(bytes < sizeof(Mesh)){
		AssetsError("mesh size was too small when trying to load it from memory.");
		return assets_mesh_null();
	}

	auto [index, found] = __find_resource(((Mesh*)data)->uid, g_assets->mesh_map);
	if(found) {
		AssetsExistanceWarning(((Mesh*)data)->name, Mesh);
		return g_assets->mesh_map[index];
	}

	//allocate and copy from data
	Mesh* mesh = (Mesh*)memory_alloc(bytes);
	CopyMemory(mesh, data, bytes);
	
	//setup arrays
	u8* cursor  = (u8*)mesh + (1*sizeof(Mesh));
	mesh->vertex_array   = (MeshVertex*)cursor;    cursor +=   mesh->vertex_count*sizeof(MeshVertex);
	mesh->index_array    = (MeshIndex*)cursor;     cursor +=    mesh->index_count*sizeof(MeshIndex);
	mesh->triangle_array = (MeshTriangle*)cursor;  cursor += mesh->triangle_count*sizeof(MeshTriangle);
	mesh->face_array     = (MeshFace*)cursor;      cursor +=     mesh->face_count*sizeof(MeshFace);
	mesh->triangle_array[0].neighbor_array = (u32*)(mesh->face_array + mesh->face_count);
	mesh->triangle_array[0].edge_array     = (u8*) (mesh->triangle_array[0].neighbor_array + mesh->total_tri_neighbor_count);
	for(s32 ti = 1; ti < mesh->triangle_count; ++ti){
		mesh->triangle_array[ti].neighbor_array = (u32*)(mesh->triangle_array[ti-1].neighbor_array + mesh->triangle_array[ti-1].neighbor_count);
		mesh->triangle_array[ti].edge_array     = (u8*) (mesh->triangle_array[ti-1].edge_array + mesh->triangle_array[ti-1].neighbor_count);
	}
	mesh->face_array[0].triangle_array          = (u32*)(mesh->triangle_array[0].edge_array          + mesh->total_tri_neighbor_count);
	mesh->face_array[0].vertex_array            = (u32*)(mesh->face_array[0].triangle_array          + mesh->triangle_count);
	mesh->face_array[0].outer_vertex_array      = (u32*)(mesh->face_array[0].vertex_array            + mesh->total_face_vertex_count);
	mesh->face_array[0].neighbor_triangle_array = (u32*)(mesh->face_array[0].outer_vertex_array      + mesh->total_face_outer_vertex_count);
	mesh->face_array[0].neighbor_face_array     = (u32*)(mesh->face_array[0].neighbor_triangle_array + mesh->total_face_tri_neighbor_count);
	for(s32 fi = 1; fi < mesh->face_count; ++fi){
		mesh->face_array[fi].triangle_array          = (u32*)(mesh->face_array[fi-1].triangle_array          + mesh->face_array[fi-1].triangle_count);
		mesh->face_array[fi].vertex_array            = (u32*)(mesh->face_array[fi-1].vertex_array            + mesh->face_array[fi-1].vertex_count);
		mesh->face_array[fi].outer_vertex_array      = (u32*)(mesh->face_array[fi-1].outer_vertex_array      + mesh->face_array[fi-1].outer_vertex_count);
		mesh->face_array[fi].neighbor_triangle_array = (u32*)(mesh->face_array[fi-1].neighbor_triangle_array + mesh->face_array[fi-1].neighbor_triangle_count);
		mesh->face_array[fi].neighbor_face_array     = (u32*)(mesh->face_array[fi-1].neighbor_face_array     + mesh->face_array[fi-1].neighbor_face_count);
	}
	
	map_mesh(mesh);
	array_insert_value(g_assets->mesh_map, index, mesh);
	return mesh;
}


void
assets_mesh_save(Mesh* mesh){DPZoneScoped;
	assets_mesh_save_to_path(mesh, str8_concat3(STR8("data/models/"),mesh->name,STR8(".mesh"), deshi_temp_allocator));
}


void
assets_mesh_save_to_path(Mesh* mesh, str8 path){DPZoneScoped;
	if(file_write_simple(path, mesh, mesh->bytes)) {
		AssetsNotice("successfully saved mesh: ", path);
	}
}


void
assets_mesh_delete(Mesh* mesh){DPZoneScoped;
	AssetsAssert(mesh, "passed null mesh pointer.");

	if(mesh == g_assets->null_mesh) {
		AssetsWarning("attempted to delete null mesh.");
		return;
	}
	
	auto [index, found] = __find_resource(mesh->uid, g_assets->mesh_map);
	if(!found) {
		AssetsError("the given mesh ('", mesh->name, "'@", (void*)mesh, ") has a uid (", mesh->uid, ") that does not exist in the mesh map. The memory of the mesh will not be deleted as we cannot be sure we own it.");
		return;
	}

	array_remove_ordered(g_assets->mesh_map, index);
	graphics_buffer_destroy(mesh->vertex_buffer);
	graphics_buffer_destroy(mesh->index_buffer);
	memory_pool_delete(g_assets->mesh_pool, mesh);
}

FORCE_INLINE Mesh*
assets_mesh_get_by_uid(u64 uid) {
	auto [index, found] = __find_resource(uid, g_assets->mesh_map);
	if(found) return g_assets->mesh_map[index];
	return 0;
}
 
FORCE_INLINE Mesh*
assets_mesh_get_by_name(str8 name) {
	return assets_mesh_get_by_uid(__uid_of_name(name));
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @texture


GraphicsFilter
texture_filter_to_render(TextureFilter x) {
	switch(x) {
		case TextureFilter_Nearest: return GraphicsFilter_Nearest;
		case TextureFilter_Cubic:   return GraphicsFilter_Linear;
	}
	AssetsFatal("unknown TextureFilter: ", (u32)x);
	return {};
}

void 
upload_texture(Texture* texture) {
	texture->image = graphics_image_allocate();
	texture->image->debug_name = to_dstr8v(deshi_temp_allocator, "<assets> texture '", texture->name, "' image").fin;
	texture->image->format = GraphicsFormat_R8G8B8A8_SRGB;
	texture->image->usage = GraphicsImageUsage_Sampled | GraphicsImageUsage_Transfer_Destination;
	texture->image->samples = GraphicsSampleCount_1;
	texture->image->extent = {texture->width, texture->height, 4};
	graphics_image_update(texture->image);
	graphics_image_write(texture->image, texture->pixels, vec2i::ZERO, {texture->width, texture->height});

	texture->image_view = graphics_image_view_allocate();
	texture->image_view->debug_name = to_dstr8v(deshi_temp_allocator, "<assets> texture '", texture->name, "' image view").fin;
	texture->image_view->image = texture->image;
	texture->image_view->format = texture->image->format;
	texture->image_view->aspect_flags = GraphicsImageViewAspectFlags_Color;
	graphics_image_view_update(texture->image_view);
	
	texture->sampler = graphics_sampler_allocate();
	texture->sampler->debug_name = to_dstr8v(deshi_temp_allocator, "<assets> texture '", texture->name, "' sampler").fin;
	texture->sampler->mipmap_mode = GraphicsFilter_Nearest;
	texture->sampler->mag_filter = 
		texture->sampler->min_filter = texture_filter_to_render(texture->filter);
	switch(texture->uv_mode) {
		case TextureAddressMode_Repeat: {
			texture->sampler->address_mode_u = 
				texture->sampler->address_mode_v = 
				texture->sampler->address_mode_w = GraphicsSamplerAddressMode_Repeat;
		} break;
		case TextureAddressMode_MirroredRepeat: {
			texture->sampler->address_mode_u = 
				texture->sampler->address_mode_v = 
				texture->sampler->address_mode_w = GraphicsSamplerAddressMode_Mirrored_Repeat;
		} break;
		case TextureAddressMode_ClampToEdge: {
			texture->sampler->address_mode_u = 
				texture->sampler->address_mode_v = 
				texture->sampler->address_mode_w = GraphicsSamplerAddressMode_Clamp_To_Edge;
		} break;
		case TextureAddressMode_ClampToWhite: {
			texture->sampler->address_mode_u = 
				texture->sampler->address_mode_v = 
				texture->sampler->address_mode_w = GraphicsSamplerAddressMode_Clamp_To_Border;
			texture->sampler->border_color = Color_White;
		} break;
		case TextureAddressMode_ClampToBlack: {
			texture->sampler->address_mode_u = 
				texture->sampler->address_mode_v = 
				texture->sampler->address_mode_w = GraphicsSamplerAddressMode_Clamp_To_Border;
			texture->sampler->border_color = Color_Black;
		} break;
		case TextureAddressMode_ClampToTransparent: {
			texture->sampler->address_mode_u = 
				texture->sampler->address_mode_v = 
				texture->sampler->address_mode_w = GraphicsSamplerAddressMode_Clamp_To_Border;
			texture->sampler->border_color = Color_NONE;
		} break;
	}
	graphics_sampler_update(texture->sampler);
}

Texture*
assets_texture_create_from_file(str8 name, ImageFormat format, TextureType type, TextureFilter filter, TextureAddressMode uvMode, b32 keepLoaded, b32 generateMipmaps){DPZoneScoped;
	AssetsAssertName(name);
	if(str8_equal_lazy(name, STR8("null"))) return assets_texture_null();
	
	AssetsNotice("creating texture '", name, "'.");

	u64 uid = __uid_of_name(name);
	auto [index, found] = __find_resource(uid, g_assets->texture_map);
	if(found) {
		AssetsExistanceWarning(name, Texture);
		return g_assets->texture_map[index];
	}

	str8 path = str8_concat(STR8("data/textures/"),name, deshi_temp_allocator);
	Texture* texture = memory_pool_push(g_assets->texture_pool);
	texture->name    = name;
	texture->uid     = uid;
	texture->format  = format;
	texture->type    = type;
	texture->filter  = filter;
	texture->uv_mode = uvMode;
	texture->pixels  = stbi_load((char*)path.str, &texture->width, &texture->height, &texture->depth, STBI_rgb_alpha);
	if(texture->pixels == 0){
		AssetsError("failed to load texture from path '", path, "' file due to stbi error: ", stbi_failure_reason());
		memory_pool_delete(g_assets->texture_pool, texture);
		return assets_texture_null();
	}
	
	if(generateMipmaps){
		texture->mipmaps = (s32)log2(Max(texture->width, texture->height)) + 1;
	}else{
		texture->mipmaps = 1;
	}
	
	upload_texture(texture);
	if(!keepLoaded){
		stbi_image_free(texture->pixels); 
		texture->pixels = 0;
	}

	array_insert_value(g_assets->texture_map, index, texture);
	return texture;
}


Texture*
assets_texture_create_from_path(
								str8 path, 
								ImageFormat format, 
								TextureType type, 
								TextureFilter filter, 
								TextureAddressMode uvMode, 
								b32 keepLoaded, 
								b32 generateMipmaps){DPZoneScoped;
	AssetsAssert(path, "given an empty path.");
	//check if texture is already loaded
	str8 filename = str8_skip_until_last(path, '/'); str8_advance(&filename);

	u64 uid = __uid_of_name(filename);
	auto [index, found] = __find_resource(uid, g_assets->texture_map);
	if(found) {
		AssetsExistanceWarning(filename, Texture);
		return g_assets->texture_map[index];
	}

	AssetsNotice("creating texture from path '", path, "'.");

	Texture* texture = memory_pool_push(g_assets->texture_pool);
	texture->name    = filename;
	texture->uid     = uid;
	texture->format  = format; //TODO(delle) handle non RGBA formats properly
	texture->type    = type;
	texture->filter  = filter;
	texture->uv_mode = uvMode;
	texture->pixels  = stbi_load((char*)path.str, &texture->width, &texture->height, &texture->depth, STBI_rgb_alpha);
	if(texture->pixels == 0){
		AssetsError("failed to create texture from path '", path, "' due to an stbi error: ", stbi_failure_reason());
		memory_pool_delete(g_assets->texture_pool, texture);
		return assets_texture_null();
	}
	
	if(generateMipmaps){
		texture->mipmaps = (s32)log2(Max(texture->width, texture->height)) + 1;
	}else{
		texture->mipmaps = 1;
	}
	upload_texture(texture);
	if(!keepLoaded){
		stbi_image_free(texture->pixels); 
		texture->pixels = 0;
	}
	
	array_insert_value(g_assets->texture_map, index, texture);
	return texture;
}


Texture*
assets_texture_create_from_memory(
								  void* data, 
								  str8 name, 
								  u32 width, u32 height, 
								  ImageFormat format, 
								  TextureType type, 
								  TextureFilter filter, 
								  TextureAddressMode uvMode, 
								  b32 generateMipmaps){DPZoneScoped;
	AssetsAssert(data, "passed null data pointer.");
	AssetsAssertName(name);
	
	u64 uid = __uid_of_name(name);
	auto [index, found] = __find_resource(uid, g_assets->texture_map);
	if(found) {
		AssetsExistanceWarning(name, Texture);
		return g_assets->texture_map[index];
	}

	AssetsNotice("creating texture '", name, "'.");

	Texture* texture = memory_pool_push(g_assets->texture_pool);
	texture->name    = name;
	texture->uid     = uid;
	texture->format  = format;
	texture->type    = type;
	texture->filter  = filter;
	texture->uv_mode = uvMode;
	texture->width   = width;
	texture->height  = height;
	texture->depth   = 4;
	
	if(generateMipmaps){
		texture->mipmaps = (s32)log2(Max(texture->width, texture->height)) + 1;
	}else{
		texture->mipmaps = 1;
	}
	
	//reinterpret image as RGBA32  //TODO(delle) handle non RGBA formats properly
	if(format != ImageFormat_RGBA){
		AssetsWarning("non-rgba textures are currently reinterpreted as rgba (TODO(sushi) properly handle non-rgba formats)");
		texture->pixels = (u8*)memory_alloc(width * height * 4);
		
		const u8* src = (const u8*)data;
		u32* dst = (u32*)texture->pixels;
		switch(format){
			case ImageFormat_BW:{
				for(s32 i = width*height; i > 0; i--){
					u8 value = *src++;
					// HACK(sushi) I am temporarily defining black pixels to be transparent here as it fixes an issue with bdf fonts
					//             using black where it should be transparent, and that's the only place (along with the null texture) 
					//             that actually use it. We need to decide a better fix eventually, though.
					*dst++ = PackColorU32(value, value, value, (value? 0xFF : 0x00));
				}
			}break;
			case ImageFormat_BWA:{
				for(s32 i = width*height; i > 0; i--){
					u8 value = *src++;
					u8 alpha = *src++;
					*dst++ = PackColorU32(value, value, value, alpha);
				}
			}break;
			case ImageFormat_RGB:{
				for(s32 i = width*height; i > 0; i--){
					u8 r = *src++;
					u8 g = *src++;
					u8 b = *src++;
					*dst++ = PackColorU32(r, g, b, 0xFF);
				}
			}break;
		}
	}else{
		texture->pixels = (u8*)data;
	}
	
	upload_texture(texture);
	array_insert_value(g_assets->texture_map, index, texture);
	return texture;
}


void
assets_texture_delete(Texture* texture){DPZoneScoped;
	AssetsAssert(texture, "passed null texture pointer.");

	if(texture == g_assets->null_texture) {
		AssetsWarning("attempted to delete null texture.");
		return;
	} 
	
	auto [index, found] = __find_resource(texture->uid, g_assets->texture_map);
	if(!found) {
		AssetsDeleteNotFound(texture, Texture);
		return;
	}

	AssetsNotice("deleting texture ", AssetsResourceBasicInfo(texture));

	graphics_sampler_destroy(texture->sampler);
	graphics_image_view_destroy(texture->image_view);
	graphics_image_destroy(texture->image);
	if(texture->pixels) memzfree(texture->pixels); //NOTE(delle) stbi_image_free() simply calls STBI_FREE()
	memory_pool_delete(g_assets->texture_pool, texture);
}

void 
assets_texture_update(Texture* texture, vec2i offset, vec2i extent) {	
	AssetsAssert(texture, "passed a null texture pointer.");

	if(offset.x > texture->width || offset.y > texture->height) {
		AssetsError("cannot update texture '", texture->name, "': the given offset ", offset, " is outside the bounds of the given texture.");
		return;
	}
	
	if(offset.x + extent.x > texture->width || offset.y + extent.y > texture->height) {
		AssetsWarning("while updating texture '", texture->name, "': the given offset ", offset, " plus the given extent ", extent, " go beyond the region of the given texture (size is ", Vec2i(texture->width, texture->height), "). The extent will be clipped to be within the image.");
		extent.x = Min(texture->width - offset.x, extent.x);
		extent.y = Min(texture->height - offset.y, extent.y);
	}

	if(!(extent.x && extent.y)) return;

	graphics_image_write(texture->image, texture->pixels + offset.x*offset.y*texture->format*texture->format, offset, extent);
}

FORCE_INLINE Texture*
assets_texture_get_by_uid(u64 uid) {
	auto [index, found] = __find_resource(uid, g_assets->texture_map);
	if(found) return g_assets->texture_map[index];
	return 0;
}

FORCE_INLINE Texture*
assets_texture_get_by_name(str8 name) {
	return assets_texture_get_by_uid(__uid_of_name(name));
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @shader


UBO* 
assets_ubo_create(u32 size) {
	UBO* ubo = memory_pool_push(g_assets->ubo_pool);
	ubo->size = size;
	ubo->buffer = graphics_buffer_create(
									   0, size,
									   GraphicsBufferUsage_UniformBuffer,
									   GraphicsMemoryPropertyFlag_HostVisible | GraphicsMemoryPropertyFlag_HostCoherent,
									   GraphicsMemoryMapping_Occasional);
	return ubo;
}

void 
assets_ubo_update(UBO* ubo, void* data) {
	void* mapped_data = graphics_buffer_map(ubo->buffer, graphics_buffer_device_size(ubo->buffer), 0);
	
	CopyMemory(mapped_data, data, ubo->size);
	
	graphics_buffer_unmap(ubo->buffer, true);
}

void 
assets_ubo_delete(UBO* ubo) {
	graphics_buffer_destroy(ubo->buffer);
	memory_pool_delete(g_assets->ubo_pool, ubo);
}

Shader*
assets_shader_load_from_source(str8 name, str8 source, ShaderType type) {
	AssetsAssertName(name);
	AssetsAssert(source, "empty source data.");
	
	u64 uid = __uid_of_name(name);
	auto [index, found] = __find_resource(uid, g_assets->shader_map);
	if(found) {
		AssetsExistanceWarning(name, Shader);
		return g_assets->shader_map[index];
	}

	AssetsNotice("loading shader '", name, "'.");

	Shader* out = memory_pool_push(g_assets->shader_pool);
	out->name = name;
	out->uid = uid;
	out->type = type;
	out->handle = graphics_shader_allocate();
	switch(type) {
		case ShaderType_Vertex:   out->handle->shader_stage = GraphicsShaderStage_Vertex; break;
		case ShaderType_Geometry: out->handle->shader_stage = GraphicsShaderStage_Geometry; break;
		case ShaderType_Fragment: out->handle->shader_stage = GraphicsShaderStage_Fragment; break;
		default: {
			AssetsFatal("invalid ShaderType: ", (u32)type);
		} break;
	}
	out->handle->source = source;
	out->handle->debug_name = to_dstr8v(deshi_allocator, "<assets-shader> ", name).fin;
	graphics_shader_update(out->handle);
	array_insert_value(g_assets->shader_map, index, out);
	return out;
}

Shader*
assets_shader_load_from_file(str8 filename, ShaderType type) {
	AssetsAssert(filename, "empty filename.");
	
	str8 path = str8_concat(str8l("data/shaders/"), filename);
	if(!file_exists(path)) {
		AssetsError("the file '", filename, "' does not exist in 'deshi/shaders'");
		switch(type) {
			case ShaderType_Vertex: return g_assets->null_vertex_shader;
			case ShaderType_Fragment: return g_assets->null_fragment_shader;
			default: {
				AssetsFatal("unhandled or invalid ShaderType: ", (u32)type);
			} break;
		}
	}

	str8 source = file_read_simple(path, deshi_temp_allocator);
	
	return assets_shader_load_from_source(filename, source, type);
}

Shader*
assets_shader_load_from_path(str8 name, str8 path, ShaderType type) {
	AssetsAssertName(name);
	AssetsAssert(path, "empty path.");
	
	if(!file_exists(path)) {
		AssetsError("no file exists at path '", path, "'.");
		switch(type) {
			case ShaderType_Vertex: return g_assets->null_vertex_shader;
			case ShaderType_Fragment: return g_assets->null_fragment_shader;
			default: {
				AssetsFatal("unhandled or invalid ShaderType: ", (u32)type);
			} break;
		}
	}

	str8 source = file_read_simple(path, deshi_temp_allocator);

	return assets_shader_load_from_source(name, source, type);
}

void
assets_shader_reload(Shader* shader) {
	AssetsAssert(shader, "passed null Shader pointer.");
	AssetsAssert(shader->handle, "given shader has a null graphics handle.");

	// TODO(sushi) this implementation SUCKS! my original intension when i began this  
	//             whole shader api was to make it so that you can get away with just 
	//             recompiling a single shader and also associating shaders with materials
	//             so we can find the right things to update directly. That is not happening 
	//             here, instead it is much like it was before, but I would like it to be faster
	//             though it does not HAVE to be since this is probably a debug thing anywyas 
	switch(shader->type) {
		case ShaderType_Vertex: {
			forI(array_count(g_assets->material_map)) {
				auto m = g_assets->material_map[i];
				if(m->stages.vertex == shader) graphics_pipeline_update(m->pipeline);
			}
		} break;
		case ShaderType_Geometry: {
			forI(array_count(g_assets->material_map)) {
				auto m = g_assets->material_map[i];
				if(m->stages.geometry == shader) graphics_pipeline_update(m->pipeline);
			}
		} break;
		case ShaderType_Fragment: {
			forI(array_count(g_assets->material_map)) {
				auto m = g_assets->material_map[i];
				if(m->stages.fragment == shader) graphics_pipeline_update(m->pipeline);
			}
		} break;
	}
}

FORCE_INLINE Shader*
assets_shader_get_by_name(str8 name) {
	return assets_shader_get_by_uid(__uid_of_name(name));
}

FORCE_INLINE Shader*
assets_shader_get_by_uid(u64 uid) {
	auto [index, found] = __find_resource(uid, g_assets->shader_map);
	if(found) return g_assets->shader_map[index];
	return 0;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @assets_material


Material*
assets_material_create(str8 name, ShaderStages shaders, ShaderResource* resources) {	
	AssetsAssertName(name);
	
	u64 uid = __uid_of_name(name);
	auto [index, found] = __find_resource(uid, g_assets->material_map);
	if(found) {
		AssetsExistanceWarning(name, Material);
		return g_assets->null_material;
	}

	AssetsNotice("loading material '", name, "'.");

	auto pipeline = graphics_pipeline_allocate();
	pipeline->debug_name = to_dstr8v(deshi_temp_allocator, "<assets-material> ", name, " pipeline").fin;
	assets_setup_pipeline(pipeline);

	auto shader_stages = array<GraphicsShader>::create(deshi_allocator);
	
	if(!shaders.vertex) {
		AssetsError("vertex shader stage pointer is null. A material is required to specify at least a vertex shader.");
		return assets_material_null();
	}
	
	pipeline->vertex_shader = shaders.vertex->handle;
	pipeline->geometry_shader = (shaders.geometry? shaders.geometry->handle : 0);
	pipeline->fragment_shader = (shaders.fragment? shaders.fragment->handle : 0);

	// setup the descriptor layout
	auto descriptor_layout = graphics_descriptor_set_layout_allocate();
	descriptor_layout->debug_name = to_dstr8v(deshi_temp_allocator, "Material ", name, " descriptor layout").fin;
	
	u32 n_vertex_resources   = (shaders.vertex->resources?   array_count(shaders.vertex->resources)   : 0);
	u32 n_geometry_resources = (shaders.geometry && shaders.geometry->resources? array_count(shaders.geometry->resources) : 0);
	u32 n_fragment_resources = (shaders.fragment && shaders.fragment->resources? array_count(shaders.fragment->resources) : 0);
	u32 sum = n_vertex_resources + n_geometry_resources + n_fragment_resources;
	
	if(sum && !resources) {
		AssetsError("material has resoures specified in some shader, but the given resources array is null");
		return g_assets->null_material;
	}
	
	u32 n_resources_given = array_count(resources);
	
	if(n_resources_given != sum) {
		AssetsError("the number of given resources (", n_resources_given, ") is not equal to the number of expected resources for this material (", sum, ")");
		return g_assets->null_material;
	}
		
	auto descriptors = array<GraphicsDescriptor>::create(sum, deshi_temp_allocator);
	auto bindings = array<GraphicsDescriptorSetLayoutBinding>::create(sum, deshi_allocator);

	forI(n_vertex_resources) {
		auto resource = shaders.vertex->resources[i];
		auto resource_given = resources[descriptors.count()];
		auto descriptor = descriptors.push();
		if(resource != resource_given.type) {
			AssetsError("resource type mismatch between vertex resource ", i, " (", ShaderResourceTypeStrings[resource], ") and given resource ", descriptors.count() - 1, " (", ShaderResourceTypeStrings[resource_given.type], ")");
			return g_assets->null_material;
		}	
		
		auto binding = bindings.push();
		binding->n = bindings.count() - 1;
		binding->shader_stages = GraphicsShaderStage_Vertex;

		switch(resource) {
			case ShaderResourceType_UBO: {
				descriptor->type = GraphicsDescriptorType_Uniform_Buffer;
				descriptor->ubo.buffer = resource_given.ubo->buffer;
				descriptor->ubo.range = resource_given.ubo->size;
				descriptor->ubo.offset = 0;
				binding->type = GraphicsDescriptorType_Uniform_Buffer;
			} break;
			case ShaderResourceType_Texture: {
				descriptor->type = GraphicsDescriptorType_Combined_Image_Sampler;
				descriptor->image.view = resource_given.texture->image_view;
				descriptor->image.sampler = resource_given.texture->sampler;
				descriptor->image.layout = GraphicsImageLayout_Shader_Read_Only_Optimal;
				binding->type = GraphicsDescriptorType_Combined_Image_Sampler;
			} break;
			default: {
				AssetsError("resource ", i, " given for vertex shader has an unknown type (", (u32)resource, ")");
				graphics_descriptor_set_layout_destroy(descriptor_layout);
				graphics_pipeline_destroy(pipeline);
				return g_assets->null_material;
			} break;
		}
	}
	
	forI(n_geometry_resources) {
		auto resource = shaders.geometry->resources[i];
		auto resource_given = resources[descriptors.count()];
		auto descriptor = descriptors.push();
		if(resource != resource_given.type) {
			AssetsError("resource type mismatch between geometry resource ", i, " (", ShaderResourceTypeStrings[resource], ") and given resource ", descriptors.count() - 1, " (", ShaderResourceTypeStrings[resource_given.type], ")");
			return g_assets->null_material;
		}	
			
		auto binding = bindings.push();
		binding->n = bindings.count() - 1;
		binding->shader_stages = GraphicsShaderStage_Geometry;

		switch(resource) {
			case ShaderResourceType_UBO: {
				descriptor->type = GraphicsDescriptorType_Uniform_Buffer;
				descriptor->ubo.buffer = resource_given.ubo->buffer;
				descriptor->ubo.range = resource_given.ubo->size;
				descriptor->ubo.offset = 0;
				binding->type = GraphicsDescriptorType_Uniform_Buffer;
			} break;
			case ShaderResourceType_Texture: {
				descriptor->type = GraphicsDescriptorType_Combined_Image_Sampler;
				descriptor->image.view = resource_given.texture->image_view;
				descriptor->image.sampler = resource_given.texture->sampler;
				descriptor->image.layout = GraphicsImageLayout_Shader_Read_Only_Optimal;
				binding->type = GraphicsDescriptorType_Combined_Image_Sampler;
			} break;
			default: {
				AssetsError("resource ", i, " given for geometry shader has an unknown type (", (u32)resource, ")");
				graphics_descriptor_set_layout_destroy(descriptor_layout);
				graphics_pipeline_destroy(pipeline);
				return g_assets->null_material;
			} break;

		}
	}
	
	forI(n_fragment_resources) {
		auto resource = shaders.fragment->resources[i];
		auto resource_given = resources[descriptors.count()];
		auto descriptor = descriptors.push();
		if(resource != resource_given.type) {
			AssetsError("resource type mismatch between fragment resource ", i, " (", ShaderResourceTypeStrings[resource], ") and given resource ", descriptors.count() - 1, " (", ShaderResourceTypeStrings[resource_given.type], ")");
			return g_assets->null_material;
		}	

		auto binding = bindings.push();
		binding->n = bindings.count() - 1;
		binding->shader_stages = GraphicsShaderStage_Fragment;
		
		switch(resource) {
			case ShaderResourceType_UBO: {
				descriptor->type = GraphicsDescriptorType_Uniform_Buffer;
				descriptor->ubo.buffer = resource_given.ubo->buffer;
				descriptor->ubo.range = resource_given.ubo->size;
				descriptor->ubo.offset = 0;
				binding->type = GraphicsDescriptorType_Uniform_Buffer;
			} break;
			case ShaderResourceType_Texture: {
				descriptor->type = GraphicsDescriptorType_Combined_Image_Sampler;
				descriptor->image.view = resource_given.texture->image_view;
				descriptor->image.sampler = resource_given.texture->sampler;
				descriptor->image.layout = GraphicsImageLayout_Shader_Read_Only_Optimal;
				binding->type = GraphicsDescriptorType_Combined_Image_Sampler;
			} break;
			default: {
				AssetsError("resource ", i, " given for fragment shader has an unknown type (", (u32)resource, ")");
				graphics_descriptor_set_layout_destroy(descriptor_layout);
				graphics_pipeline_destroy(pipeline);
				return g_assets->null_material;
			} break;

		}
	}
	descriptor_layout->bindings = bindings.ptr;
	graphics_descriptor_set_layout_update(descriptor_layout);
	
	GraphicsPushConstant transformation;
	transformation.shader_stages = GraphicsShaderStage_Vertex;
	transformation.size = sizeof(mat4);
	transformation.offset = 0;
	
	auto pipeline_layout = graphics_pipeline_layout_allocate();
	pipeline_layout->debug_name = to_dstr8v(deshi_temp_allocator, "Material ", name, " pipeline layout").fin;
	array_init_with_elements(pipeline_layout->descriptor_layouts, {
				g_assets->ubo_layout,
				descriptor_layout
			}, deshi_allocator);
	array_init_with_elements(pipeline_layout->push_constants, {
				transformation,
			}, deshi_allocator);
	graphics_pipeline_layout_update(pipeline_layout);
	
	pipeline->layout = pipeline_layout;
	graphics_pipeline_update(pipeline);
	
	auto descriptor_set = graphics_descriptor_set_allocate();
	descriptor_set->layouts = array<GraphicsDescriptorSetLayout*>::create({descriptor_layout}, deshi_temp_allocator).ptr;
	graphics_descriptor_set_update(descriptor_set);
	graphics_descriptor_set_write_array(descriptor_set, descriptors.ptr);
	
	Material* material = memory_pool_push(g_assets->material_pool);
	material->name = name;
	material->uid = uid;
	material->pipeline = pipeline;
	material->descriptor_set = descriptor_set;
	material->stages = shaders;
	array_init(material->resources, array_count(resources), deshi_allocator);
	array_count(material->resources) = array_count(resources);
	CopyMemory(material->resources, resources, sizeof(ShaderResource) * array_count(resources));
	array_insert_value(g_assets->material_map, index, material);

	AssetsNotice("created material ", AssetsResourceBasicInfo(material));

	return material;
}


Material*
assets_material_create_from_file(str8 name){DPZoneScoped;
	if(str8_equal_lazy(name, STR8("null"))) return assets_material_null();
	
	//prepend the materials (models) folder
	dstr8 builder;
	dstr8_init(&builder, STR8("data/models/"), deshi_temp_allocator);
	dstr8_append(&builder, name);
	
	//append extension if not provided
	str8 front = str8_eat_until_last(name, '.');
	if(front.count == name.count) dstr8_append(&builder, STR8(".mat"));
	
	return assets_material_create_from_path(dstr8_peek(&builder));
}


Material*
assets_material_create_from_path(str8 path){DPZoneScoped;
	// TODO(sushi) update to use new graphics api once one of us actually uses material stuff from disk
	NotImplemented;
//	//check if material is already loaded
//	str8 filename = str8_skip_until_last(path, '/'); str8_advance(&filename);
//	for_stb_array(g_assets->material_array){
//		if(strncmp((*it)->name, (char*)filename.str, 64) == 0){
//			return *it;
//		}
//	}
//	str8 front = str8_eat_until(filename, '.');
//	
//	//load .mat file
//	File* file = file_init(path, FileAccess_Read);
//	if(!file) return assets_material_null();
//	defer{ file_deinit(file); };
//	
//	//parse .mat file
//	str8 mat_name{}; //NOTE(delle) unused b/c we use the filename for loaded name currently
//	Shader mat_shader = 0;
//	arrayT<str8> mat_textures(deshi_temp_allocator);
//	enum{ HEADER_MATERIAL, HEADER_TEXTURES, HEADER_INVALID }header;
//	
//	u32 line_number = 0;
//	while(file->cursor < file->bytes){
//		line_number += 1;
//		
//		//next line
//		str8 line = file_read_line_alloc(file, &assets_load_allocator);
//		if(!line) continue;
//		
//		//skip leading whitespace
//		str8_advance_while(&line, ' ');
//		if(!line) continue;
//		
//		//early out if comment is first character
//		DecodedCodepoint decoded = decoded_codepoint_from_utf8(line.str, 4);
//		if(decoded.codepoint == '#') continue;
//		
//		//check for header
//		if(decoded.codepoint == '>'){
//			if     (str8_begins_with(line, STR8(">material"))) header = HEADER_MATERIAL;
//			else if(str8_begins_with(line, STR8(">textures"))) header = HEADER_TEXTURES;
//			else{ header = HEADER_INVALID; LogE("assets","Error parsing material '",path,"' on line ",line_number,". Invalid Header: ",line); };
//			continue;
//		}
//		
//		//early out invalid header
//		if(header == HEADER_INVALID){
//			LogE("assets","Error parsing material '",path,"' on line ",line_number,". Invalid Header; skipping line");
//			continue;
//		}
//		
//		if(header == HEADER_MATERIAL){
//			//parse key
//			str8 key = str8_eat_until(line, ' ');
//			str8_increment(&line, key.count);
//			
//			//skip separating whitespace
//			str8_advance_while(&line, ' ');
//			if(!line){
//				LogE("config","Error parsing material '",path,"' on line ",line_number,". No value passed to key: ",key);
//				continue;
//			}
//			
//			//early out if comment is first value character
//			decoded = decoded_codepoint_from_utf8(line.str, 4);
//			if(decoded.codepoint == '#'){
//				LogE("assets","Error parsing material '",path,"' on line ",line_number,". No value passed to key: ",key);
//				continue;
//			}
//			
//			if      (str8_equal_lazy(key, STR8("name"))){
//				if(decoded.codepoint != '\"'){
//					LogE("assets","Error parsing material '",path,"' on line ",line_number,". Names must be wrapped in double quotes.");
//					continue;
//				}
//				mat_name = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
//			}else if(str8_equal_lazy(key, STR8("shader"))){
//				forI(Shader_COUNT){
//					if(str8_equal_lazy(line, ShaderStrings[i])){
//						mat_shader = i;
//						break;
//					}
//				}
//			}else{
//				LogE("assets","Error parsing material '",path,"' on line ",line_number,". Invalid key '",key,"' for >material header.");
//				continue;
//			}
//		}else{
//			if(decoded.codepoint != '\"'){
//				LogE("assets","Error parsing material '",path,"' on line ",line_number,". Textures must be wrapped in double quotes.");
//				continue;
//			}
//			
//			mat_textures.add(str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator));
//		}
//	}
//	
//	Material* material = assets_material_allocate(mat_textures.count);
//	CopyMemory(material->name, front.str, ClampMax(front.count, 63));
//	material->shader = mat_shader;
//	forI(mat_textures.count) material->texture_array[i] = assets_texture_create_from_file_simple(mat_textures[i]);
//	render_load_material(material);
//	arrput(g_assets->material_array, material);
//	return material;
	return 0;
}


void
assets_material_save(Material* material){DPZoneScoped;
	assets_material_save_to_path(material, str8_concat3(STR8("data/models/"),material->name,STR8(".mat"), deshi_temp_allocator));
}


void
assets_material_save_to_path(Material* material, str8 path){DPZoneScoped;
	NotImplemented;
	// TODO(sushi)
//	dstr8 builder;
//	dstr8_init(&builder,
//			   to_dstr8v(deshi_temp_allocator,
//						 ">material"
//						 "\nname   \"",material->name,"\""
//						 "\nshader-vertex   ", (material->stages.vertex.filename.str? material->stages.vertex.filename : str8l("NONE")),
//						 "\nshader-geometry ", (material->stages.geometry.filename.str? material->stages.geometry.filename : str8l("NONE")),
//						 "\nshader-fragment ", (material->stages.fragment.filename.str? material->stages.fragment.filename : str8l("NONE")),
//						 "\nshader ",material->stages,
//						 "\n"
//						 "\n>textures").fin,
//			   deshi_temp_allocator);
//	if(material->texture_array){
//		for_array(material->texture_array){
//			dstr8_append(&builder, to_dstr8v(deshi_temp_allocator, "\n\"",(*it)->name,"\""));
//		}
//	}
//	dstr8_append(&builder, STR8("\n"));
//	str8 mat_text = dstr8_peek(&builder);
//	file_write_simple(path, mat_text.str, mat_text.count*sizeof(u8));
//	Log("assets","Successfully saved material: ",path);
}


void
assets_material_delete(Material* material){DPZoneScoped;
	AssetsAssert(material, "passed null Material pointer.");
	
	if(material == g_assets->null_material) {
		AssetsWarning("attempted to delete null material.");
		return;
	}

	auto [index, found] = __find_resource(material->uid, g_assets->material_map);
	if(!found) {
		AssetsDeleteNotFound(material, Material);
		return;
	}

	graphics_pipeline_destroy(material->pipeline);
	graphics_descriptor_set_destroy(material->descriptor_set);
	array_deinit(material->texture_array);
	memory_pool_delete(g_assets->material_pool, material);
}

Material* 
assets_material_duplicate(str8 name, Material* old, ShaderResource* resources) {
	if(!name) {
		AssetsError("name has length zero or is null.");
		return assets_material_null();
	}
	if(!old) {
		AssetsError("material pointer is null.");
		return assets_material_null();
	}
	if(str8_equal(name, old->name)) {
		AssetsError("the name of a duplicated material must be different than the name it is duplicated from.");
		return assets_material_null();
	}
	if(array_count(resources) != array_count(old->resources)) {
		AssetsError("amount of given resources (", array_count(resources), ") is different from the amount of resources on the given material (", array_count(old->resources), ").");
		return assets_material_null();
	}

	u64 uid = __uid_of_name(name);
	auto [index, found] = __find_resource(uid, g_assets->material_map);
	if(found) {
		AssetsError("cannot duplicate material '", old->name, "' to new material with name '", name, "' as another material with the new name already exists.");
		return g_assets->null_material;
	}
	
	Material* nu = memory_pool_push(g_assets->material_pool);
	nu->name = name;
	nu->uid = uid;
	nu->pipeline = old->pipeline;
	nu->stages = old->stages;
	
	auto descriptors = array<GraphicsDescriptor>::create(array_count(resources), deshi_temp_allocator);
	
	forI(array_count(resources)) {
		auto given_resource = resources[i];
		auto old_resource = old->resources[i];
		if(given_resource.type != old_resource.type) {
			AssetsError("resource ", i, " has a different type (", ShaderResourceTypeStrings[given_resource.type], ") than the old material's resource at the same index (", ShaderResourceTypeStrings[old_resource.type], ")");
			return assets_material_null();
		}
		auto d = descriptors.push();
		switch(given_resource.type) {
			case ShaderResourceType_UBO: {
				d->type = GraphicsDescriptorType_Uniform_Buffer;
				d->ubo.buffer = given_resource.ubo->buffer;
				d->ubo.range = given_resource.ubo->size;
				d->ubo.offset = 0;
			} break;
			case ShaderResourceType_Texture: {
				d->type = GraphicsDescriptorType_Combined_Image_Sampler;
				d->image.view = given_resource.texture->image_view;
				d->image.sampler = given_resource.texture->sampler;
				d->image.layout = GraphicsImageLayout_Shader_Read_Only_Optimal;
			} break;
		}
	}
	
	nu->descriptor_set = graphics_descriptor_set_allocate();
	nu->descriptor_set->layouts = old->descriptor_set->layouts;
	graphics_descriptor_set_update(nu->descriptor_set);
	graphics_descriptor_set_write_array(nu->descriptor_set, descriptors.ptr);
	array_insert_value(g_assets->material_map, index, nu);
	return nu;
}

FORCE_INLINE Material*
assets_material_get_by_uid(u64 uid) {
	auto [index, found] = __find_resource(uid, g_assets->material_map);
	if(found) return g_assets->material_map[index];
	return 0;
}

FORCE_INLINE Material*
assets_material_get_by_name(str8 name) {
	return assets_material_get_by_uid(__uid_of_name(name));
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @assets_model


Model*
assets_model_allocate(u32 batchCount){DPZoneScoped;
	Model* model = memory_pool_push(g_assets->model_pool);
	model->batch_array = array<ModelBatch>::create_with_count(batchCount ? batchCount : 1, deshi_allocator).ptr;
	return model;
}


Model* 
assets_model_create_from_file(str8 filename, ModelFlags flags, b32 forceLoadOBJ){DPZoneScoped;
	if(str8_equal_lazy(filename, STR8("null"))) return assets_model_null();
		
	//prepend the models folder
	str8 directory = STR8("data/models/");
	dstr8 builder;
	dstr8_init(&builder, directory, deshi_temp_allocator);
	dstr8_append(&builder, filename);
	
	//append extension if not provided
	str8 front = str8_eat_until_last(filename, '.');
	if(front.count == filename.count) dstr8_append(&builder, STR8(".model"));
	
	u64 uid = __uid_of_name(filename);
	auto [index, found] = __find_resource(uid, g_assets->model_map);
	if(found) {
		AssetsExistanceWarning(filename, Model);
		return g_assets->null_model;
	}

	//check which files need to be parsed
	str8 model_path = dstr8_peek(&builder);
	str8 obj_path  = str8_concat3(directory, front, STR8(".obj"),  deshi_temp_allocator);
	str8 mesh_path = str8_concat3(directory, front, STR8(".mesh"), deshi_temp_allocator);
	b32 parse_obj_mesh  = true;
	b32 parse_obj_model = true;
	if(!forceLoadOBJ){
		if(file_exists(mesh_path))  parse_obj_mesh  = false;
		if(file_exists(model_path)) parse_obj_model = false;
	}
	
	//// load .obj and .mtl ////
	if(parse_obj_model && parse_obj_mesh){
		return assets_model_create_from_obj(obj_path, flags);
	}
	//// load .obj (batch info only), .mtl, and .mesh ////
	else if(parse_obj_model){
		return assets_model_create_from_mesh_obj(assets_mesh_create_from_path(mesh_path), obj_path, flags);
	}
	
	//// load .model and .mesh ////
	Stopwatch load_stopwatch = start_stopwatch();
	str8 model_name;
	str8 model_mesh;
	ModelFlags model_flags;
	arrayT<pair<str8,u32,u32>> model_batches(deshi_temp_allocator);
	enum{ HEADER_MODEL, HEADER_BATCHES, HEADER_INVALID } header;
	
	File* file = file_init(model_path, FileAccess_Read);
	if(!file) return assets_model_null();
	defer{ file_deinit(file); };
	
	u32 line_number = 0;
	while(file->cursor < file->bytes){
		line_number += 1;
		
		//next line
		str8 line = file_read_line_alloc(file, &assets_load_allocator);
		if(!line) continue;
		
		//skip leading whitespace
		str8_advance_while(&line, ' ');
		if(!line) continue;
		
		//early out if comment is first character
		DecodedCodepoint decoded = decoded_codepoint_from_utf8(line.str, 4);
		if(decoded.codepoint == '#') continue;
		
		//check for headers
		if(decoded.codepoint == '>'){
			if     (str8_begins_with(line, STR8(">model"))) header = HEADER_MODEL;
			else if(str8_begins_with(line, STR8(">batches"))) header = HEADER_BATCHES;
			else{ header = HEADER_INVALID; LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". Invalid Header: ",line); };
			continue;
		}
		
		//early out invalid header
		if(header == HEADER_INVALID){
			LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". Invalid Header; skipping line");
			continue;
		}
		
		if(header == HEADER_MODEL){
			//parse key
			str8 key = str8_eat_until(line, ' ');
			str8_increment(&line, key.count);
			
			//skip separating whitespace
			str8_advance_while(&line, ' ');
			if(!line){
				LogE("config","Error parsing model '",model_path,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			
			//early out if comment is first value character
			decoded = decoded_codepoint_from_utf8(line.str, 4);
			if(decoded.codepoint == '#'){
				LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			
			if      (str8_equal_lazy(key, STR8("name"))){
				if(decoded.codepoint != '\"'){
					LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". Names must be wrapped in double quotes.");
					continue;
				}
				model_name = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
			}else if(str8_equal_lazy(key, STR8("flags"))){
				model_flags = (ModelFlags)atoi((char*)line.str);
			}else if(str8_equal_lazy(key, STR8("mesh"))){
				if(decoded.codepoint != '\"'){
					LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". Filenames must be wrapped in double quotes.");
					continue;
				}
				model_mesh = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
			}else if(str8_equal_lazy(key, STR8("armature"))){
				//NOTE currently nothing
			}else{
				LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". Invalid key '",key,"' for >model header.");
				continue;
			}
		}else{
			if(decoded.codepoint != '\"'){
				LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". Names must be wrapped in double quotes. Batch format: '\"material_name\" index_offset index_count'");
				continue;
			}
			
			str8 batch_mat = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
			str8_increment(&line, batch_mat.count+2);
			if(!line){
				LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". No indexes passed. Batch format: '\"material_name\" index_offset index_count'");
				continue;
			}
			
			char* next = (char*)line.str;
			u32 ioffset = strtol(next,&next,10);
			u32 icount  = strtol(next,&next,10);
			
			model_batches.add({batch_mat, ioffset, icount});
		}
	}
	Log("assets","Successfully loaded model ",model_path);
	
	Model* model = assets_model_allocate(model_batches.count);
	model->name     = model_name;
	model->uid      = uid;
	model->flags    = model_flags;
	model->mesh     = assets_mesh_create_from_file(model_mesh);
	model->armature = 0;
	forI(model_batches.count){
		model->batch_array[i] = ModelBatch{
			model_batches[i].second,
			model_batches[i].third,
			assets_material_create_from_file(model_batches[i].first)
		};
	}
	
	array_insert_value(g_assets->model_map, index, model);
	AssetsNotice("finished loading model ", AssetsResourceBasicInfo(model), " in ", peek_stopwatch(load_stopwatch), "ms");
	return model;
}


Model*
assets_model_create_from_obj(str8 obj_path, ModelFlags flags){DPZoneScoped;
#define ParseError(path,...) AssetsError("failed parsing '",path,"' on line '",line_number,"'! ",__VA_ARGS__)
	Stopwatch load_stopwatch = start_stopwatch();
	map<vec3,MeshVertex> vUnique(deshi_temp_allocator);
	set<vec3> vnUnique(deshi_temp_allocator);
	set<pair<u32,str8>> oUnique(deshi_temp_allocator); //index offset, name
	set<pair<u32,str8>> gUnique(deshi_temp_allocator);
	set<pair<u32,str8>> uUnique(deshi_temp_allocator);
	set<pair<u32,str8>> mUnique(deshi_temp_allocator);
	set<pair<u32,vec3>> appliedUniqueNormals(deshi_temp_allocator); //vertex applied on, normal
	arrayT<vec2> vtArray(deshi_temp_allocator); //NOTE UV vertices arent expected to be unique
	arrayT<u32> vArray(deshi_temp_allocator); //index in unique array
	arrayT<u32> vnArray(deshi_temp_allocator);
	arrayT<u32> oArray(deshi_temp_allocator);
	arrayT<u32> gArray(deshi_temp_allocator);
	arrayT<u32> uArray(deshi_temp_allocator);
	arrayT<u32> mArray(deshi_temp_allocator);
	arrayT<MeshIndex>    indexes(deshi_temp_allocator);
	arrayT<MeshTriangle> triangles(deshi_temp_allocator);
	arrayT<MeshFace>     faces(deshi_temp_allocator);
	arrayT<arrayT<pair<u32,u8>>> triNeighbors(deshi_temp_allocator);
	arrayT<arrayT<u32>> faceTriangles(deshi_temp_allocator);
	arrayT<set<u32>>   faceVertexes(deshi_temp_allocator);
	arrayT<arrayT<u32>> faceOuterVertexes(deshi_temp_allocator);
	arrayT<arrayT<u32>> faceTriNeighbors(deshi_temp_allocator);
	arrayT<arrayT<u32>> faceFaceNeighbors(deshi_temp_allocator);
	u32 totalTriNeighbors      = 0;
	u32 totalFaceVertexes      = 0;
	u32 totalFaceOuterVertexes = 0;
	u32 totalFaceTriNeighbors  = 0;
	u32 totalFaceFaceNeighbors = 0;
	vec3 aabb_min{ MAX_F32, MAX_F32, MAX_F32};
	vec3 aabb_max{-MAX_F32,-MAX_F32,-MAX_F32};
	u32 default_color = Color_White.rgba;
	b32 mtllib_found    = false;
	b32 s_warning       = false;
	b32 non_tri_warning = false;
	b32 fatal_error     = false;
	
	File* file = file_init(obj_path, FileAccess_Read);
	if(!file) return assets_model_null();
	defer{ file_deinit(file); };
	
	u64 uid = __uid_of_name(file->front);
	auto [index, found] = __find_resource(uid, g_assets->model_map);
	if(found) {
		AssetsExistanceWarning(file->front, Model);
		return g_assets->model_map[index];
	}

	u32 line_number = 0;
	while(file->cursor < file->bytes){
		line_number += 1;
		
		//next line
		str8 line = file_read_line_alloc(file, &assets_load_allocator);
		if(!line) continue;
		
		//skip leading whitespace
		str8_advance_while(&line, ' ');
		if(!line) continue;
		
		//early out if comment is first character
		DecodedCodepoint decoded = decoded_codepoint_from_utf8(line.str, 4);
		if(decoded.codepoint == '#') continue;
		
		//TODO(delle) add parsing safety checks to strtof and strol
		//TODO(delle) handle non-triangle faces (maybe)
		switch(decoded.codepoint){
			case '\0': case '\n': case '\r': case '#': case ' ': continue; //skip empty and comment lines
			//// vertex, normal, or uv ////
			case 'v':{
				str8_increment(&line, decoded.advance);
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				
				switch(decoded.codepoint){
					//// vertex ////
					case ' ':{
						str8_increment(&line, decoded.advance);
						
						char* next = (char*)line.str;
						f32 x = strtof(next, &next);
						f32 y = strtof(next, &next);
						f32 z = strtof(next, 0);
						vec3 vec{x,y,z};
						vArray.add(vUnique.add(vec, MeshVertex{vec}));
					}continue;
					
					//// uv ////
					case 't':{
						str8_increment(&line, decoded.advance);
						decoded = decoded_codepoint_from_utf8(line.str, 4);
						if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'vt'"); return assets_model_null(); }
						str8_increment(&line, decoded.advance);
						
						char* next = (char*)line.str;
						f32 x = strtof(next, &next);
						f32 y = strtof(next, 0);
						vtArray.add(vec2{x,y});
					}continue;
					
					//// normal ////
					case 'n':{
						str8_increment(&line, decoded.advance);
						decoded = decoded_codepoint_from_utf8(line.str, 4);
						if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'vn'"); return assets_model_null(); }
						str8_increment(&line, decoded.advance);
						
						char* next = (char*)line.str;
						f32 x = strtof(next, &next);
						f32 y = strtof(next, &next);
						f32 z = strtof(next, 0);
						vec3 vec{x,y,z};
						vnArray.add(vnUnique.add(vec, vec));
					}continue;
					default:{
						ParseError(obj_path,"Invalid character after 'v': '",(char)decoded.codepoint,"'");
					}return assets_model_null();
				}
			}continue;
			
			//// face ////
			case 'f':{
				Stopwatch face_stopwatch = start_stopwatch();
				
				str8_increment(&line, decoded.advance);
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'f'"); return assets_model_null(); }
				if(vArray.count == 0){ ParseError(obj_path,"Specifier 'f' before any 'v'"); return assets_model_null(); }
				
				str8_increment(&line, decoded.advance);
				char* next = (char*)line.str;
				u32 v0=strtol(next,&next,10)-1; u32 vt0=strtol(next+1,&next,10)-1; u32 vn0=strtol(next+1,&next,10)-1;
				u32 v1=strtol(next,&next,10)-1; u32 vt1=strtol(next+1,&next,10)-1; u32 vn1=strtol(next+1,&next,10)-1;
				u32 v2=strtol(next,&next,10)-1; u32 vt2=strtol(next+1,&next,10)-1; u32 vn2=strtol(next+1,&next,10)-1;
				u32 o = oArray.count; u32 g = gArray.count; u32 m = mArray.count;
				
				//index
				indexes.add(vArray[v0]);
				indexes.add(vArray[v1]);
				indexes.add(vArray[v2]);
				
				//uv
				if(vtArray.count){
					vUnique.data[vArray[v0]].uv = vtArray[vt0];
					vUnique.data[vArray[v1]].uv = vtArray[vt1];
					vUnique.data[vArray[v2]].uv = vtArray[vt2];
				}
				
				//normal
				if(vnArray.count){
					if(appliedUniqueNormals.add({vArray[v0],vnUnique.data[vnArray[vn0]]}) == appliedUniqueNormals.count-1){
						vUnique.data[vArray[v0]].normal += vnUnique.data[vnArray[vn0]];
					}
					if(appliedUniqueNormals.add({vArray[v1],vnUnique.data[vnArray[vn1]]}) == appliedUniqueNormals.count-1){
						vUnique.data[vArray[v1]].normal += vnUnique.data[vnArray[vn1]];
					}
					if(appliedUniqueNormals.add({vArray[v2],vnUnique.data[vnArray[vn2]]}) == appliedUniqueNormals.count-1){
						vUnique.data[vArray[v2]].normal += vnUnique.data[vnArray[vn2]];
					}
				}
				
				//color
				vUnique.data[vArray[v0]].color = default_color;
				vUnique.data[vArray[v1]].color = default_color;
				vUnique.data[vArray[v2]].color = default_color;
				
				//triangle
				u32 cti = triangles.count;
				MeshTriangle triangle{};
				triangle.p[0] = vUnique.data[vArray[v0]].pos;
				triangle.p[1] = vUnique.data[vArray[v1]].pos;
				triangle.p[2] = vUnique.data[vArray[v2]].pos;
				triangle.v[0] = vArray[v0];
				triangle.v[1] = vArray[v1];
				triangle.v[2] = vArray[v2];
				triangle.face = (u32)-1;
				triangle.normal = (triangle.p[0] - triangle.p[1]).cross(triangle.p[0] - triangle.p[2]).normalized();
				triangles.add(triangle);
				triNeighbors.add(arrayT<pair<u32,u8>>(deshi_temp_allocator));
				
				//triangle neighbors
				for(u32 oti=0; oti<triangles.count-1; ++oti){
					//check that its not already a neighbor
					b32 neighbor_already = false;
					for(u32 tni=0; tni<triNeighbors[oti].count; ++tni){
						if(triNeighbors[oti][tni].first == cti){ neighbor_already = true; break; }
					}
					
					//check for shared vertexes and mark the edges
					if(!neighbor_already){
						b32 ct0_ot0 = (vArray[v0] == triangles[oti].v[0]);
						b32 ct0_ot1 = (vArray[v0] == triangles[oti].v[1]);
						b32 ct0_ot2 = (vArray[v0] == triangles[oti].v[2]);
						b32 ct1_ot0 = (vArray[v1] == triangles[oti].v[0]);
						b32 ct1_ot1 = (vArray[v1] == triangles[oti].v[1]);
						b32 ct1_ot2 = (vArray[v1] == triangles[oti].v[2]);
						b32 ct2_ot0 = (vArray[v2] == triangles[oti].v[0]);
						b32 ct2_ot1 = (vArray[v2] == triangles[oti].v[1]);
						b32 ct2_ot2 = (vArray[v2] == triangles[oti].v[2]);
						
						//current tri v0 && v1
						if(ct0_ot0 && ct1_ot1){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
						if(ct0_ot0 && ct1_ot2){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
						if(ct0_ot1 && ct1_ot0){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
						if(ct0_ot1 && ct1_ot2){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
						if(ct0_ot2 && ct1_ot0){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
						if(ct0_ot2 && ct1_ot1){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
						//current tri v1 && v2
						if(ct1_ot0 && ct2_ot1){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
						if(ct1_ot0 && ct2_ot2){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
						if(ct1_ot1 && ct2_ot0){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
						if(ct1_ot1 && ct2_ot2){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
						if(ct1_ot2 && ct2_ot0){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
						if(ct1_ot2 && ct2_ot1){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
						//current tri v2 && v0
						if(ct2_ot0 && ct0_ot1){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
						if(ct2_ot0 && ct0_ot2){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
						if(ct2_ot1 && ct0_ot0){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
						if(ct2_ot1 && ct0_ot2){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
						if(ct2_ot2 && ct0_ot0){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
						if(ct2_ot2 && ct0_ot1){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
					}
				}
				triangles[cti].neighbor_count= triNeighbors[cti].count;
				f64 load_watch = peek_stopwatch(load_stopwatch);
				if(((u64)(load_watch / 1000.0) % 10 == 0) && ((u64)(load_watch / 1000.0) != 0)){
					Log("assets",obj_path," face ",faces.count," on line ",line_number," finished creation in ",peek_stopwatch(face_stopwatch),"ms");
				}
			}continue;
			
			//// use material ////
			case 'u':{
				if(strncmp((const char*)line.str, "usemtl ", 7) != 0){ ParseError(obj_path,"Specifier started with 'u' but didn't equal 'usemtl '"); return assets_model_null(); }
				
				if(mtllib_found){
					str8_increment(&line, 7);
					pair<u32,str8> usemtl(indexes.count, str8_copy(line, deshi_temp_allocator));
					uArray.add(uUnique.add(usemtl,usemtl));
				}else{
					ParseError(obj_path,"Specifier 'usemtl' used before 'mtllib' specifier");
				}
			}continue;
			
			//// load material ////
			case 'm':{
				if(strncmp((const char*)line.str, "mtllib ", 7) != 0){ ParseError(obj_path,"Specifier started with 'm' but didn't equal 'mtllib '"); return assets_model_null(); }
				
				mtllib_found = true;
				str8_increment(&line, 7);
				pair<u32,str8> mtllib(indexes.count, str8_copy(line, deshi_temp_allocator));
				mArray.add(mUnique.add(mtllib,mtllib));
			}continue;
			
			//// group (batch) ////
			case 'g':{
				str8_increment(&line, decoded.advance);
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'g'"); return assets_model_null(); }
				str8_increment(&line, decoded.advance);
				
				pair<u32,str8> group(indexes.count, str8_copy(line, deshi_temp_allocator));
				gArray.add(gUnique.add(group,group));
			}continue;
			
			//// object ////
			case 'o':{
				str8_increment(&line, decoded.advance);
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'o'"); return assets_model_null(); }
				str8_increment(&line, decoded.advance);
				
				pair<u32,str8> object(indexes.count, str8_copy(line, deshi_temp_allocator));
				oArray.add(oUnique.add(object,object));
			}continue;
			
			//// smoothing ////
			case 's':{
				str8_increment(&line, decoded.advance);
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 's'"); return assets_model_null(); }
				str8_increment(&line, decoded.advance);
				
				s_warning = true;
			}continue;
			default:{
				ParseError(obj_path,"Invalid starting character: '",(char)decoded.codepoint,"'");
			}return assets_model_null();
		}
	}
	
	//// generate mesh faces ////
	forX(bti, triangles.count){
		if(triangles[bti].face != -1) continue;
		
		//create face and add base triange to it
		u32 cfi = faces.count;
		faces.add(MeshFace{});
		faceTriangles.add(arrayT<u32>(deshi_temp_allocator));
		faceVertexes.add(set<u32>(deshi_temp_allocator));
		faceOuterVertexes.add(arrayT<u32>(deshi_temp_allocator));
		faceTriNeighbors.add(arrayT<u32>(deshi_temp_allocator));
		faceFaceNeighbors.add(arrayT<u32>(deshi_temp_allocator));
		faces[cfi].normal = triangles[bti].normal;
		triangles[bti].face = cfi;
		faceTriangles[cfi].add(bti);
		faceVertexes[cfi].add(triangles[bti].v[0],triangles[bti].v[0]);
		faceVertexes[cfi].add(triangles[bti].v[1],triangles[bti].v[1]);
		faceVertexes[cfi].add(triangles[bti].v[2],triangles[bti].v[2]);
		totalFaceVertexes += 3;
		
		arrayT<u32> check_tris({(u32)bti});
		forX(check_tri_idx, check_tris.count){
			u32 cti = check_tris[check_tri_idx];
			forX(nei_tri_idx, triNeighbors[cti].count){
				u32 nti = triNeighbors[cti][nei_tri_idx].first;
				b32 checked = false;
				forI(check_tris.count){ if(check_tris[i] == nti){ checked = true; break; } }
				if(checked) continue;
				
				//check if neighbor triangle has same normal
				if(triangles[cti].normal == triangles[nti].normal){
					check_tris.add(nti);
					triangles[nti].face = cfi;
					faceTriangles[cfi].add(nti);
					faceVertexes[cfi].add(triangles[nti].v[0],triangles[nti].v[0]);
					faceVertexes[cfi].add(triangles[nti].v[1],triangles[nti].v[1]);
					faceVertexes[cfi].add(triangles[nti].v[2],triangles[nti].v[2]);
					totalFaceVertexes += 3;
				}else{
					faceTriNeighbors[cfi].add(nti);
					totalFaceTriNeighbors++;
					u32 v1 = triangles[cti].v[ triNeighbors[cti][nei_tri_idx].second       ];
					u32 v2 = triangles[cti].v[(triNeighbors[cti][nei_tri_idx].second+1) % 3];
					b32 v1_already = false; b32 v2_already = false;
					forX(fovi, faceOuterVertexes[cfi].count){
						if(!v1_already && faceOuterVertexes[cfi][fovi] == v1){ v1_already = true; }
						if(!v2_already && faceOuterVertexes[cfi][fovi] == v2){ v2_already = true; }
						if(v1_already && v2_already) break;
					}
					if(!v1_already){
						faceOuterVertexes[cfi].add(v1); 
						totalFaceOuterVertexes++;
						faces[cfi].center += vUnique.atIdx(v1)->pos;
					}
					if(!v2_already){
						faceOuterVertexes[cfi].add(v2);
						totalFaceOuterVertexes++;
						faces[cfi].center += vUnique.atIdx(v2)->pos;
					}
				}
			}
		}
	}
	
	//generate face neighbors
	forX(cfi, faces.count){
		forX(cnti, faceTriNeighbors[cfi].count){ //check neighbor triangles
			b32 already_added = false;
			forX(nfi, faceFaceNeighbors[cfi].count){ //see if face neighbor already added
				if(triangles[faceTriNeighbors[cfi][cnti]].face == faceFaceNeighbors[cfi][nfi]){
					already_added = true;
					break;
				}
			}
			if(!already_added){
				faceFaceNeighbors[cfi].add(triangles[faceTriNeighbors[cfi][cnti]].face);
				faceFaceNeighbors[triangles[faceTriNeighbors[cfi][cnti]].face].add(cfi);
				totalFaceFaceNeighbors += 2;
			}
		}
	}
	
	//// calculate vertex normals ////
	forI(vUnique.count){
		aabb_min.x = Min(aabb_min.x, vUnique.data.data[i].pos.x);
		aabb_max.x = Max(aabb_max.x, vUnique.data.data[i].pos.x);
		aabb_min.y = Min(aabb_min.y, vUnique.data.data[i].pos.y);
		aabb_max.y = Max(aabb_max.y, vUnique.data.data[i].pos.y);
		aabb_min.z = Min(aabb_min.z, vUnique.data.data[i].pos.z);
		aabb_max.z = Max(aabb_max.z, vUnique.data.data[i].pos.z);
		vUnique.data.data[i].normal.normalize();
	}
	
	//// parsing warnings/errors ////
	if(non_tri_warning)   AssetsWarning("The mesh was not triangulated before parsing; Expect missing triangles!");
	if(s_warning)         AssetsWarning("There were 's' specifiers when parsing ",obj_path,", but those are not evaluated currently");
	if(!vtArray.count){   AssetsWarning("No vertex UVs 'vt' were parsed in ",obj_path); }
	if(!vnArray.count){   AssetsWarning("No vertex normals 'vn' were parsed in ",obj_path); }
	if(fatal_error){      AssetsError("OBJ parsing encountered a fatal error in ",obj_path); return assets_model_null(); }
	if(!vArray.count){    AssetsError("No vertex positions 'v' were parsed in ",obj_path); return assets_model_null(); }
	if(!triangles.count){ AssetsError("No faces 'f' were parsed in ",obj_path); return assets_model_null(); }
	
	//// check if mesh is already loaded ////
	Mesh* mesh = assets_mesh_get_by_name(file->front);
	
	//// create mesh ////
	if(mesh == 0){
		mesh = assets_mesh_allocate(indexes.count, vUnique.count, faces.count, totalTriNeighbors, 
									totalFaceVertexes, totalFaceOuterVertexes, totalFaceTriNeighbors, totalFaceFaceNeighbors);
		//fill base arrays
		mesh->name     = file->front;
		mesh->uid      = __uid_of_name(file->front);
		mesh->aabb_min = aabb_min;
		mesh->aabb_max = aabb_max;
		mesh->center   = {(aabb_max.x+aabb_min.x)/2.0f, (aabb_max.y+aabb_min.y)/2.0f, (aabb_max.z+aabb_min.z)/2.0f};
		memcpy(mesh->vertex_array,   vUnique.data.data, vUnique.count*sizeof(MeshVertex));
		memcpy(mesh->index_array,    indexes.data,      indexes.count*sizeof(MeshIndex));
		memcpy(mesh->triangle_array, triangles.data,    triangles.count*sizeof(MeshTriangle));
		memcpy(mesh->face_array,     faces.data,        faces.count*sizeof(MeshFace));
		
		//setup pointers
		mesh->triangle_array[0].neighbor_array = (u32*)(mesh->face_array + mesh->face_count);
		mesh->triangle_array[0].edge_array     = (u8*)(mesh->triangle_array[0].neighbor_array + totalTriNeighbors);
		for(s32 ti = 1; ti < mesh->triangle_count; ++ti){
			mesh->triangle_array[ti].neighbor_array = (u32*)(mesh->triangle_array[ti-1].neighbor_array + triNeighbors[ti-1].count);
			mesh->triangle_array[ti].edge_array     =  (u8*)(mesh->triangle_array[ti-1].edge_array     + triNeighbors[ti-1].count);
		}
		mesh->face_array[0].triangle_array         = (u32*)(mesh->triangle_array[0].edge_array     + totalTriNeighbors);
		mesh->face_array[0].vertex_array           = (u32*)(mesh->face_array[0].triangle_array         + triangles.count);
		mesh->face_array[0].outer_vertex_array      = (u32*)(mesh->face_array[0].vertex_array           + totalFaceVertexes);
		mesh->face_array[0].neighbor_triangle_array = (u32*)(mesh->face_array[0].outer_vertex_array      + totalFaceOuterVertexes);
		mesh->face_array[0].neighbor_face_array     = (u32*)(mesh->face_array[0].neighbor_triangle_array + totalFaceTriNeighbors);
		for(s32 fi = 1; fi < mesh->face_count; ++fi){
			mesh->face_array[fi].triangle_array         = (u32*)(mesh->face_array[fi-1].triangle_array         + faceTriangles[fi-1].count);
			mesh->face_array[fi].vertex_array           = (u32*)(mesh->face_array[fi-1].vertex_array           + faceVertexes[fi-1].count);
			mesh->face_array[fi].outer_vertex_array      = (u32*)(mesh->face_array[fi-1].outer_vertex_array      + faceOuterVertexes[fi-1].count);
			mesh->face_array[fi].neighbor_triangle_array = (u32*)(mesh->face_array[fi-1].neighbor_triangle_array + faceTriNeighbors[fi-1].count);
			mesh->face_array[fi].neighbor_face_array     = (u32*)(mesh->face_array[fi-1].neighbor_face_array     + faceFaceNeighbors[fi-1].count);
		}
		
		//fill triangle neighbors/edges
		forX(ti, triangles.count){
			forX(ni, triNeighbors[ti].count){
				mesh->triangle_array[ti].neighbor_array[ni] = triNeighbors[ti][ni].first;
				mesh->triangle_array[ti].edge_array[ni]     = triNeighbors[ti][ni].second;
			}
			mesh->triangle_array[ti].neighbor_count= triNeighbors[ti].count;
		}
		
		//fill face tris/vertexes/neighbors
		forX(fi, mesh->face_count){
			mesh->face_array[fi].triangle_count = faceTriangles[fi].count;
			mesh->face_array[fi].vertex_count   = faceVertexes[fi].count;
			mesh->face_array[fi].outer_vertex_count = faceOuterVertexes[fi].count;
			mesh->face_array[fi].neighbor_triangle_count = faceTriNeighbors[fi].count;
			mesh->face_array[fi].neighbor_face_count = faceFaceNeighbors[fi].count;
			mesh->face_array[fi].center = faces[fi].center / (f32)faceOuterVertexes[fi].count;
			forX(fti, mesh->face_array[fi].triangle_count){
				mesh->face_array[fi].triangle_array[fti] = faceTriangles[fi][fti];
			}
			forX(fvi, mesh->face_array[fi].vertex_count){
				mesh->face_array[fi].vertex_array[fvi] = faceVertexes[fi].data[fvi];
			}
			forX(fvi, mesh->face_array[fi].outer_vertex_count){
				mesh->face_array[fi].outer_vertex_array[fvi] = faceOuterVertexes[fi][fvi];
			}
			forX(fvi, mesh->face_array[fi].neighbor_triangle_count){
				mesh->face_array[fi].neighbor_triangle_array[fvi] = faceTriNeighbors[fi][fvi];
			}
			forX(fvi, mesh->face_array[fi].neighbor_face_count){
				mesh->face_array[fi].neighbor_face_array[fvi] = faceFaceNeighbors[fi][fvi];
			}
		}
		map_mesh(mesh);
		auto [index, _] = __find_resource(mesh->uid, g_assets->mesh_map);
		array_insert_value(g_assets->mesh_map, index, mesh);
	}
	AssetsNotice(obj_path, "Parsing and loading OBJ took ",peek_stopwatch(load_stopwatch),"ms");
	
	//parse MTL files
	if(mtllib_found){
		load_stopwatch = start_stopwatch();
		
		//!Incomplete
		
		Log("assets","Parsing and loading MTLs for OBJ '",obj_path,"' took ",peek_stopwatch(load_stopwatch),"ms");
	}
	
	Model* model = assets_model_allocate(mArray.count);
	model->name     = file->front;
	model->uid      = uid;
	model->flags    = flags;
	model->mesh     = mesh;
	model->armature = 0;
	
	//!Incomplete batch materials
	if(mArray.count > 1){
		model->batch_array[mArray.count-1].index_offset = mUnique.data[mArray[mArray.count-1]].first;
		model->batch_array[mArray.count-1].index_count  = indexes.count - model->batch_array[mArray.count-1].index_offset;
		model->batch_array[mArray.count-1].material    = assets_material_null();
		for(u32 bi = mArray.count-2; bi >= 0; --bi){
			model->batch_array[bi].index_offset = mUnique.data[mArray[bi]].first;
			model->batch_array[bi].index_count  = model->batch_array[bi+1].index_offset - model->batch_array[bi].index_offset;
			model->batch_array[bi].material    = assets_material_null();
		}
	}else{
		model->batch_array[0].index_offset = 0;
		model->batch_array[0].index_count  = indexes.count;
		model->batch_array[0].material    = assets_material_null();
	}

	array_insert_value(g_assets->model_map, index, model);
	Log("assets","Finished loading model '",obj_path,"' in ",peek_stopwatch(load_stopwatch),"ms");
	return model;
}

Model*
assets_model_create_from_obj_x(str8 obj_path) {
#define array_def(type, name)                 \
	type* name;                               \
	array_init(name, 1, deshi_temp_allocator);
	
	AssetsNotice("creating a new model from obj file at path ", obj_path, "'");
	
	Stopwatch watch = start_stopwatch();
	
	// NOTE(sushi) we cannot use commas to declare template parameters in macros because C is dumb!
	using IndexNamePair = pair<u32, str8>;
	
	array_def(MeshVertex,    vertex_set); // sorted by the hash of the vertex's position
	array_def(vec3,          vertex_normal_set);
	array_def(IndexNamePair, object_set);
	array_def(IndexNamePair, group_set);
	array_def(IndexNamePair, usemtl_set);
	array_def(IndexNamePair, mtllib_set);
	array_def(vec2,          uv_array);
	array_def(u32,           vertex_refs);
	array_def(u32,           vertex_normal_refs);
	array_def(u32,           object_refs);
	array_def(u32,           group_refs);
	array_def(u32,           usemtl_refs);
	// TODO(sushi) rewrite this eventually 
	return 0;
#undef array_def
}

Model*
assets_model_create_from_mesh(Mesh* mesh, ModelFlags flags){DPZoneScoped;
	Stopwatch load_stopwatch = start_stopwatch();
	
	u64 uid = mesh->uid;
	auto [index, found] = __find_resource(uid, g_assets->model_map);
	if(found) {
		AssetsExistanceWarning(mesh->name, Model);
		return g_assets->model_map[index];
	}

	Model* model = assets_model_allocate(1);
	model->name     = mesh->name;
	model->uid      = uid;
	model->mesh     = mesh;
	model->armature = 0;
	model->batch_array[0] = ModelBatch{0, mesh->index_count, assets_material_null()};
	
	array_insert_value(g_assets->model_map, index, model);
	AssetsNotice("finished loading model '",model->name,"' in ",peek_stopwatch(load_stopwatch),"ms");
	return model;
}


Model*
assets_model_create_from_mesh_obj(Mesh* mesh, str8 obj_path, ModelFlags flags){DPZoneScoped;
	Stopwatch load_stopwatch = start_stopwatch();
	set<pair<u32,str8>> oUnique(deshi_temp_allocator); //index offset, name
	set<pair<u32,str8>> gUnique(deshi_temp_allocator);
	set<pair<u32,str8>> uUnique(deshi_temp_allocator);
	set<pair<u32,str8>> mUnique(deshi_temp_allocator);
	arrayT<u32> oArray(deshi_temp_allocator); //index in unique array
	arrayT<u32> gArray(deshi_temp_allocator);
	arrayT<u32> uArray(deshi_temp_allocator);
	arrayT<u32> mArray(deshi_temp_allocator);
	b32 mtllib_found = false;
	u32 index_count = 0;
	
	File* file = file_init(obj_path, FileAccess_Read);
	if(!file) return assets_model_null();
	defer{ file_deinit(file); };
	
	u64 uid = __uid_of_name(file->front);
	auto [index, found] = __find_resource(uid, g_assets->model_map);
	if(found) {
		AssetsExistanceWarning(file->front, Model);
		return g_assets->model_map[index];
	}

	u32 line_number = 0;
	while(file->cursor < file->bytes){
		line_number += 1;
		
		//next line
		str8 line = file_read_line_alloc(file, &assets_load_allocator);
		if(!line) continue;
		
		//skip leading whitespace
		str8_advance_while(&line, ' ');
		if(!line) continue;
		
		//early out if comment is first character
		DecodedCodepoint decoded = decoded_codepoint_from_utf8(line.str, 4);
		if(decoded.codepoint == '#') continue;
		
		switch(decoded.codepoint){
			//// face ////
			case 'f':{
				str8_increment(&line, decoded.advance);
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				
				if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'f'"); return assets_model_null(); }
				index_count += 3;
			}
			
			//// use material ////
			case 'u':{ //use material
				if(strncmp((const char*)line.str, "usemtl ", 7) != 0){ ParseError(obj_path,"Specifier started with 'u' but didn't equal 'usemtl '"); return assets_model_null(); }
				
				if(mtllib_found){
					str8_increment(&line, 7);
					pair<u32,str8> usemtl(index_count, str8_copy(line, deshi_temp_allocator));
					uArray.add(uUnique.add(usemtl,usemtl));
				}else{
					ParseError(obj_path, "Specifier 'usemtl' used before 'mtllib' specifier");
				}
			}continue;
			
			//// load material ////
			case 'm':{
				if(strncmp((const char*)line.str, "mtllib ", 7) != 0){ ParseError(obj_path,"Specifier started with 'm' but didn't equal 'mtllib '"); return assets_model_null(); }
				
				mtllib_found = true;
				str8_increment(&line, 7);
				pair<u32,str8> mtllib(index_count, str8_copy(line, deshi_temp_allocator));
				mArray.add(mUnique.add(mtllib,mtllib));
			}continue;
			
			//// group (batch) ////
			case 'g':{
				str8_increment(&line, decoded.advance);
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'g'"); return assets_model_null(); }
				str8_increment(&line, decoded.advance);
				
				pair<u32,str8> group(index_count, str8_copy(line, deshi_temp_allocator));
				gArray.add(gUnique.add(group,group));
			}continue;
			
			//// object ////
			case 'o':{
				str8_increment(&line, decoded.advance);
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'o'"); return assets_model_null(); }
				str8_increment(&line, decoded.advance);
				
				pair<u32,str8> object(index_count, str8_copy(line, deshi_temp_allocator));
				oArray.add(oUnique.add(object,object));
			}continue;
			
			default: continue;
		}
	}
	
	//parse MTL files
	if(mtllib_found){
		load_stopwatch = start_stopwatch();
		
		//!Incomplete
		
		Log("assets","Parsing and loading MTLs for OBJ '",obj_path,"' took ",peek_stopwatch(load_stopwatch),"ms");
	}
	
	Model* model = assets_model_allocate(mArray.count);
	model->name     = file->front;
	model->flags    = flags;
	model->mesh     = mesh;
	model->armature = 0;
	
	//!Incomplete batch materials
	if(mArray.count > 1){
		model->batch_array[mArray.count-1].index_offset = mUnique.data[mArray[mArray.count-1]].first;
		model->batch_array[mArray.count-1].index_count  = index_count - model->batch_array[mArray.count-1].index_offset;
		model->batch_array[mArray.count-1].material    = assets_material_null();
		for(u32 bi = mArray.count-2; bi >= 0; --bi){
			model->batch_array[bi].index_offset = mUnique.data[mArray[bi]].first;
			model->batch_array[bi].index_count  = model->batch_array[bi+1].index_offset - model->batch_array[bi].index_offset;
			model->batch_array[bi].material    = assets_material_null();
		}
	}else{
		model->batch_array[0].index_offset = 0;
		model->batch_array[0].index_count  = index_count;
		model->batch_array[0].material    = assets_material_null();
	}
	
	array_insert_value(g_assets->model_map, index, model);
	AssetsNotice("finished loading model '",obj_path,"' in ",peek_stopwatch(load_stopwatch),"ms");
	return model;
}


Model* 
assets_model_duplicate(str8 name, Model* base){DPZoneScoped;
	if(str8_equal(name, base->name)) {
		AssetsError("the name of a copy of a model cannot match the original name.");
		return 0;
	}

	u64 uid = __uid_of_name(name);
	auto [index, found] = __find_resource(uid, g_assets->model_map);
	if(found) {
		AssetsError("cannot duplicate model '", base->name, "' with name '", name, "' because another model with that name already exists.");
		return 0;
	}

	Model* model = assets_model_allocate(arrlenu(base->batch_array));
	model->name     = name;
	model->uid      = uid;
	model->flags    = base->flags;
	model->mesh     = base->mesh;
	model->armature = base->armature;
	forI(arrlenu(base->batch_array)){
		model->batch_array[i].index_offset = base->batch_array[i].index_offset;
		model->batch_array[i].index_count  = base->batch_array[i].index_count;
		model->batch_array[i].material    = base->batch_array[i].material;
	}
	
	array_insert_value(g_assets->model_map, index, model);
	return model;
}


void
assets_model_save(Model* model){
	str8 directory = STR8("data/models/");
	
	if(model->mesh){
		assets_mesh_save(model->mesh);
	}
	
	str8 path = str8_concat3(directory,model->name,STR8(".model"), deshi_temp_allocator);
	dstr8 builder;
	dstr8_init(&builder,
			   to_dstr8v(deshi_temp_allocator,
						 ">model"
						 "\nname     \"",model->name,"\""
						 "\nflags    ", model->flags,
						 "\nmesh     \"", model->mesh->name,"\""
						 "\narmature ", 0,
						 "\n"
						 "\n>batches").fin,
			   deshi_temp_allocator);
	if(model->batch_array){
		for_array(model->batch_array){
			assets_material_save(it->material);
			dstr8_append(&builder, to_dstr8v(deshi_temp_allocator, "\n\"",it->material->name,"\" ",it->index_offset," ",it->index_count));
		}
	}
	dstr8_append(&builder, STR8("\n"));
	str8 model_text = dstr8_peek(&builder);
	file_write_simple(path, model_text.str, model_text.count*sizeof(u8));
	Log("assets","Successfully saved model: ",path);
}


void
assets_model_save_at_path(Model* model, str8 path){DPZoneScoped;
	str8 directory = str8_eat_until_last(path, '/');
	if(directory.str[directory.count] == '/') directory.count += 1;
	
	if(model->mesh){
		assets_mesh_save_to_path(model->mesh, str8_concat3(directory,model->mesh->name,STR8(".mesh"), deshi_temp_allocator));
	}
	
	dstr8 builder;
	dstr8_init(&builder,
			   to_dstr8v(deshi_temp_allocator,
						 ">model"
						 "\nname     \"",model->name,"\""
						 "\nflags    ", model->flags,
						 "\nmesh     \"", model->mesh->name,"\""
						 "\narmature ", 0,
						 "\n"
						 "\n>batches").fin,
			   deshi_temp_allocator);
	if(model->batch_array){
		for_array(model->batch_array){
			assets_material_save_to_path(it->material, str8_concat3(directory,it->material->name,STR8(".mat"), deshi_temp_allocator));
			dstr8_append(&builder, to_dstr8v(deshi_temp_allocator, "\n\"",it->material->name,"\" ",it->index_offset," ",it->index_count));
		}
	}
	dstr8_append(&builder, STR8("\n"));
	str8 model_text = dstr8_peek(&builder);
	file_write_simple(path, model_text.str, model_text.count*sizeof(u8));
	Log("assets","Successfully saved model: ",path);
}


void
assets_model_delete(Model* model){
	AssetsAssert(model, "passed null Model pointer.");

	if(model == g_assets->null_model) {
		AssetsWarning("attempted to delete null model.");
		return;
	}
	
	auto [index, found] = __find_resource(model->uid, g_assets->model_map);
	if(!found) {
		AssetsDeleteNotFound(model, Model);
		return;
	}
	
	array_deinit(model->batch_array);
	array_remove_unordered(g_assets->model_map, index);
	memory_pool_delete(g_assets->model_pool, model);
}

#undef ParseError
//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @assets_font
FontPackedChar*
font_packed_char(Font* font, u32 codepoint){
	//TODO(delle) an overload for specifying range if you know where you're working
	forI(font->num_ranges){
		if(   (codepoint >= font->ranges[i].first_codepoint)
		   && (codepoint <  font->ranges[i].first_codepoint + font->ranges[i].num_chars)){
			return font->ranges[i].chardata_for_range + (codepoint - font->ranges[i].first_codepoint);
		}
	}
	Assert(!"The requested codepoint was not found in any of the ranges. TODO better error handling here.");
	return 0;
}


FontAlignedQuad
font_aligned_quad(Font* font, u32 codepoint, vec2* pos, vec2 scale){
	FontPackedChar* pc = font_packed_char(font, codepoint);
	if(pc){
		FontAlignedQuad q;
		q.x0 = pos->x + pc->xoff * scale.x;
		q.y0 = pos->y + (pc->yoff + font->ascent) * scale.y;
		q.x1 = pos->x + (pc->xoff2 - pc->xoff) * scale.x;
		q.y1 = pos->y + (pc->yoff2 + font->ascent) * scale.y;
		q.u0 = ((f32)pc->x0 / font->ttf_size[0]); //NOTE(sushi) we could maybe store the UV values normalized instead of doing this everytime
		q.v0 = ((f32)pc->y0 / font->ttf_size[1]) + font->uv_yoffset;
		q.u1 = ((f32)pc->x1 / font->ttf_size[0]);
		q.v1 = ((f32)pc->y1 / font->ttf_size[1]) + font->uv_yoffset;
		pos->x += pc->xadvance * scale.x;
		return q;
	}
	Assert(!"The requested codepoint was not found in any of the ranges. TODO better error handling here.");
	return FontAlignedQuad{};
}


vec2
font_visual_size(Font* font, str8 text){
	vec2 result = vec2{0, (f32)font->max_height};
	f32 line_width = 0;
	switch(font->type){
		case FontType_BDF: case FontType_NONE:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if(codepoint == '\n'){
					result.y += font->max_height;
					line_width = 0;
				}
				line_width += font->max_width * font->max_height / font->aspect_ratio / font->max_width;
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		case FontType_TTF:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if(codepoint == '\n'){
					result.y += font->max_height;
					line_width = 0;
				}
				line_width += font_packed_char(font, codepoint)->xadvance * font->max_height / font->aspect_ratio / font->max_width;
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		default: Assert(!"unhandled font type"); break;
	}
	return result;
}


Font*
assets_font_create_from_file(str8 name, u32 height){DPZoneScoped;
	return assets_font_create_from_path(str8_concat(STR8("data/fonts/"),name, deshi_temp_allocator), height);
}


Font*
assets_font_create_from_path(str8 path, u32 height){DPZoneScoped;
	if(str8_ends_with(path, STR8(".bdf"))){
		return assets_font_create_from_path_bdf(path);
	}
	
	if(str8_ends_with(path, STR8(".ttf")) || str8_ends_with(path, STR8(".otf"))){
		return assets_font_create_from_path_ttf(path, height);
	}
	
	LogE("assets","Failed to load font '",path,"'. We only support loading TTF/OTF and BDF fonts at the moment.");
	return assets_font_null();
}


Font*
assets_font_create_from_file_bdf(str8 name){DPZoneScoped;
	if(str8_equal_lazy(name, STR8("null"))) return assets_font_null();
	return assets_font_create_from_path_bdf(str8_concat(STR8("data/fonts/"),name, deshi_temp_allocator));
}


Font*
assets_font_create_from_path_bdf(str8 path){DPZoneScoped;
	//check if font was loaded already
	str8 filename = str8_skip_until_last(path, '/'); str8_advance(&filename);
	u64 uid = __uid_of_name(filename);
	auto [index, found] = __find_resource(uid, g_assets->font_map);
	if(found){
		AssetsExistanceWarning(filename, Font);
		return g_assets->font_map[index];
	}

	b32 in_char   = false;
	b32 in_bitmap = false;
	u32 char_idx = 0;
	u32 bitmap_row = 0;
	u32 glyph_offset = 0;
	u32 top_offset = 0;
	u32 left_offset = 0;
	vec4 current_bbx = vec4::ZERO;
	vec4 font_bbx = vec4::ZERO;
	vec2 font_dpi = vec2::ZERO;
	u16* encodings = 0;
	u8*  pixels = 0;
	
	//init file
	File* file = file_init(path, FileAccess_Read);
	if(!file) return assets_font_null();
	defer{ file_deinit(file); };
	
	str8 first_line = file_read_line_alloc(file, &assets_load_allocator);
	if(!str8_begins_with(first_line, STR8("STARTFONT"))){
		LogE("assets","Error parsing BDF '",path,"' on line 1. The file did not begin with 'STARTFONT'.");
		return assets_font_null();
	}
	
	
	Font* font = memory_pool_push(g_assets->font_pool);
	font->type = FontType_BDF;
	font->name = filename;
	font->uid  = uid;
	array_insert_value(g_assets->font_map, index, font);
	u32 line_number = 1;
	while(file->cursor < file->bytes){
		line_number += 1;
		
		//next line
		str8 line = file_read_line_alloc(file, &assets_load_allocator);
		if(!line) continue;
		
		//skip leading whitespace
		str8_advance_while(&line, ' ');
		if(!line) continue;
		
		//parse key
		str8 key = str8_eat_until(line, ' ');
		str8_increment(&line, key.count);
		
		//skip separating whitespace
		str8_advance_while(&line, ' ');
		
		if(in_bitmap){
			if(str8_equal_lazy(key, STR8("ENDCHAR"))){
				in_char = false;
				in_bitmap = false;
				char_idx++;
				bitmap_row = 0;
				continue;
			}
			Assert(bitmap_row < current_bbx.y);
			
			s32 chars = (s32)key.count;
			Assert(chars <= 4); //TODO(delle) error handling: max 16 pixel width
			u8 scaled[16]{};
			
			//scale each byte to represent one pixel per bit then byteswap the u64 //TODO(delle) only byteswap if little-endian
			//ref: https://stackoverflow.com/questions/9023129/algorithm-for-bit-expansion-duplication/9044057#9044057
			//scale = 8 (1 bit to 1 byte)
			//mask0 = 0x0000000f0000000f, mask1 = 0x0003000300030003, mask2 = 0x0101010101010101
			//shift0 = (1 << 28) + 1, shift1 = (1 << 14) + 1, shift2 = (1 << 7) + 1
			for(s32 i=0; i<chars; i+=2){
				u64 reversed = (((strtoll((const char*)key.str+i,0,16) * 0x10000001 & 0x0000000f0000000f) 
								 * 0x4001 & 0x0003000300030003) * 0x81 & 0x0101010101010101) * 255;
				*(u64*)(scaled+i) = ByteSwap64(reversed);
			}
			CopyMemory(pixels+2*font->max_width+(upt)(glyph_offset + (bitmap_row+top_offset)*font->max_width + left_offset), scaled, upt(current_bbx.x*sizeof(u8)));
			
			bitmap_row++;
			continue;
		}
		
		if(in_char){
			if      (str8_equal_lazy(key, STR8("ENCODING"))){
				encodings[char_idx] = strtol((const char*)line.str, 0, 10);
			}else if(str8_equal_lazy(key, STR8("BITMAP"))){
				in_bitmap = true;
			}else if(str8_equal_lazy(key, STR8("SWIDTH"))){
				//unused
			}else if(str8_equal_lazy(key, STR8("DWIDTH"))){
				//unused in monospace fonts
			}else if(str8_equal_lazy(key, STR8("BBX"))){
				char* cursor = (char*)line.str;
				current_bbx.x = (f32)strtol(cursor,   &cursor, 10); //width
				current_bbx.y = (f32)strtol(cursor+1, &cursor, 10); //height
				current_bbx.z = (f32)strtol(cursor+1, &cursor, 10); //lower-left x
				current_bbx.w = (f32)strtol(cursor+1, &cursor, 10); //lower-left y
				glyph_offset = char_idx*font->max_height*font->max_width;
				top_offset   = u32(font->max_height-(current_bbx.w-font_bbx.w)-current_bbx.y);
				left_offset  = u32(current_bbx.z-font_bbx.z);
				
				Assert(current_bbx.x <= font_bbx.x);
				Assert(current_bbx.y <= font_bbx.y);
				Assert(current_bbx.z >= font_bbx.z);
				Assert(current_bbx.w >= font_bbx.w);
			}else{
				Assert(!"unhandled key");
			}
			continue;
		}
		
		if      (str8_equal_lazy(key, STR8("STARTCHAR"))){
			in_char = true;
		}else if(str8_equal_lazy(key, STR8("SIZE"))){
			if(!line){
				LogE("assets","Error parsing BDF '",path,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			char* cursor = (char*)line.str;
			font_dpi.x = (f32)strtol(cursor+1, &cursor, 10);
			font_dpi.y = (f32)strtol(cursor+1, &cursor, 10);
		}else if(str8_equal_lazy(key, STR8("FONTBOUNDINGBOX"))){
			if(!line){
				LogE("assets","Error parsing BDF '",path,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			char* cursor = (char*)line.str;
			font_bbx.x = (f32)strtol(cursor,   &cursor, 10); //width
			font_bbx.y = (f32)strtol(cursor+1, &cursor, 10); //height
			font_bbx.z = (f32)strtol(cursor+1, &cursor, 10); //lower-left x
			font_bbx.w = (f32)strtol(cursor+1, &cursor, 10); //lower-left y
			font->max_width  = (u32)font_bbx.x;
			font->max_height = (u32)font_bbx.y;
		}else if(str8_equal_lazy(key, STR8("FONT_NAME"))){
			if(!line){
				LogE("assets","Error parsing BDF '",path,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			if(decoded_codepoint_from_utf8(line.str, 4).codepoint != '\"'){
				LogE("assets","Error parsing BDF '",path,"' on line ",line_number,". FONT_NAME must be wrapped in double quotes.");
				continue;
			}
			str8 font_name = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
			//TODO(sushi) replace name on Font with filename and add a name for this guy right here!
		}else if(str8_equal_lazy(key, STR8("WEIGHT_NAME"))){
			if(!line){
				LogE("assets","Error parsing BDF '",path,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			if(decoded_codepoint_from_utf8(line.str, 4).codepoint != '\"'){
				LogE("assets","Error parsing BDF '",path,"' on line ",line_number,". WEIGHT_NAME must be wrapped in double quotes.");
				continue;
			}
			str8 font_weight = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
			cpystr(font->weight, (const char*)font_weight.str, 64);
		}else if(str8_equal_lazy(key, STR8("CHARS"))){
			font->count = strtol((const char*)line.str, 0, 10);
			Assert(font->max_width && font->max_height && font->count);
			encodings = (u16*)memory_talloc(font->count*sizeof(u16));
			pixels = (u8*)memory_talloc(font->count * ((font->max_width*font->max_height + 2*font->max_width) * sizeof(u8)));
			pixels[0] = 255;
			pixels[1] = 255;
			pixels[font->max_width] = 255;
			pixels[font->max_width + 1] = 255;
			font->uv_yoffset = 2.f / (font->max_height * font->count + 2);
		}else{
			continue;
		}
	}
	
	Texture* texture = assets_texture_create_from_memory(pixels, filename, font->max_width, font->max_height*font->count,
														 ImageFormat_BW, TextureType_TwoDimensional, TextureFilter_Nearest,
														 TextureAddressMode_ClampToBlack, false);
	
	font->aspect_ratio = (f32)font->max_height / font->max_width;
	font->tex = texture;
	
	array_insert_value(g_assets->font_map, index, font);
	return font;
}


Font*
assets_font_create_from_file_ttf(str8 name, u32 height){DPZoneScoped;
	if(str8_equal_lazy(name, STR8("null"))) return assets_font_null();
	return assets_font_create_from_path_ttf(str8_concat(STR8("data/fonts/"),name, deshi_temp_allocator), height);
}


Font*
assets_font_create_from_path_ttf(str8 path, u32 size){DPZoneScoped;
	//TODO clean up this function some and add in some stuff to reduce the overhead of adding in a new range
	
	//check if font was loaded already
	//TODO look into why if we load the same font w a different size it gets weird (i took that check out of here for now)
	str8 filename = str8_skip_until_last(path, '/'); str8_advance(&filename);
	u64 uid = __uid_of_name(filename);
	auto [index, found] = __find_resource(uid, g_assets->font_map);
	if(found) {
		AssetsExistanceWarning(filename, Font);
		return g_assets->font_map[index];
	}
	
	str8 contents = file_read_simple(path, deshi_temp_allocator);
	if(!contents) return assets_font_null();
	
	//Codepoint Ranges to Load:
	// ASCII              32 - 126  ~  94 chars
	// Greek and Coptic  880 - 1023 ~ 143 chars
	// Cyrillic         1024 - 1279 ~ 256 chars
	// Super/Subscripts 8304 - 8348 ~  44 chars
	// Currency Symbols 8352 - 8384 ~  32 chars
	// Arrows           8592 - 8703 ~ 111 chars
	// Math Symbols     8704 - 8959 ~ 255 chars
	// Drawing Symbols  9472 - 9727 ~ 255 chars
	// ...and maybe more to come in the future.
	
	//TODO(sushi) maybe implement taking in ranges
	u32 num_ranges = 8;
	stbtt_pack_range* ranges = (stbtt_pack_range*)memory_alloc(num_ranges*sizeof(*ranges));
	ranges[0].num_chars = 94;   ranges[0].first_unicode_codepoint_in_range = 32;
	ranges[1].num_chars = 143;  ranges[1].first_unicode_codepoint_in_range = 880;
	ranges[2].num_chars = 255;  ranges[2].first_unicode_codepoint_in_range = 1024;
	ranges[3].num_chars = 44;   ranges[3].first_unicode_codepoint_in_range = 8304;
	ranges[4].num_chars = 32;   ranges[4].first_unicode_codepoint_in_range = 8352;
	ranges[5].num_chars = 111;  ranges[5].first_unicode_codepoint_in_range = 8592;
	ranges[6].num_chars = 255;  ranges[6].first_unicode_codepoint_in_range = 8704;
	ranges[7].num_chars = 255;  ranges[7].first_unicode_codepoint_in_range = 9472;
	ranges[0].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[0].num_chars*sizeof(stbtt_packedchar));
	ranges[1].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[1].num_chars*sizeof(stbtt_packedchar));
	ranges[2].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[2].num_chars*sizeof(stbtt_packedchar));
	ranges[3].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[3].num_chars*sizeof(stbtt_packedchar));
	ranges[4].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[4].num_chars*sizeof(stbtt_packedchar));
	ranges[5].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[5].num_chars*sizeof(stbtt_packedchar));
	ranges[6].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[6].num_chars*sizeof(stbtt_packedchar));
	ranges[7].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[7].num_chars*sizeof(stbtt_packedchar));
	ranges[0].font_size = (f32)size;
	ranges[1].font_size = (f32)size;
	ranges[2].font_size = (f32)size;
	ranges[3].font_size = (f32)size;
	ranges[4].font_size = (f32)size;
	ranges[5].font_size = (f32)size;
	ranges[6].font_size = (f32)size;
	ranges[7].font_size = (f32)size;
	
	
	/*
	//trying to minimize the texture here, but its difficult due to stbtt packing all of them together
	//i believe this makes it into the smallest square it could be w/o knowing how stbtt packs them together
	//also just doesnt really work well with non-monospaced fonts
	//TODO figure out a better way to do this.
	int x0, y0, x1, y1;
	stbtt_GetFontBoundingBox(&info, &x0, &y0, &x1, &y1);
	f32 widthmax = x1 - x0, heightmax = y1 - y0;
	f32 aspect_ratio = (f32)heightmax / (f32)widthmax;
	u32 tsy = (u32)roundUpToPow2(ceil(size * sqrtf(679) / aspect_ratio));
	u32 tsx = (u32)roundUpToPow2(ceil(widthmax * size /  heightmax * sqrtf(679)) + 2); //add two rows to make room for 4 white pixels to optimize uicmds
	if(tsy > tsx) tsx = tsy;
	if(tsx > tsy) tsy = tsx;
	*/
	
	//init the font info
	int success;
	stbtt_fontinfo info;
	success = stbtt_InitFont(&info, contents.str, 0); Assert(success);
	
	//determine surface area of loadable codepoints  !ref:imgui_draw.cpp@ImFontAtlasBuildWithStbTruetype()
	//TODO(delle) maybe use this info to index into the final texture for rendering
	f32 glyph_scale = stbtt_ScaleForPixelHeight(&info, (f32)size);
	u32 glyph_count = 0;
	int glyph_padding = 1;
	int total_surface = 0;
	forX(range, num_ranges){
		for(u32 codepoint = ranges[range].first_unicode_codepoint_in_range;
			codepoint < ranges[range].first_unicode_codepoint_in_range + ranges[range].num_chars;
			++codepoint)
		{
			int glyph_index_in_font = stbtt_FindGlyphIndex(&info, codepoint);
			if(glyph_index_in_font == 0) continue; //skip any glyphs not in the font
			
			int x0, y0, x1, y1;
			stbtt_GetGlyphBitmapBoxSubpixel(&info, glyph_index_in_font, glyph_scale*1, glyph_scale*1, 0, 0, &x0, &y0, &x1, &y1);
			total_surface += ((x1 - x0) + glyph_padding) * ((y1 - y0) + glyph_padding);
		}
	}
	
	//determine texture size
	//NOTE(delle) texture width is capped due to size limitations on width for GPUs
	int surface_sqrt = (int)sqrt((f32)total_surface) + 1;
	u32 texture_size_x = (surface_sqrt >= 4096 * 0.7f) ? 4096 : (surface_sqrt >= 2048 * 0.7f) ? 2048 : (surface_sqrt >= 1024 * 0.7f) ? 1024 : 512;
	u32 texture_size_y = texture_size_x; //TODO(delle) find accurate height based on packing
	Assert(surface_sqrt <= 4096, "we don't yet handle when the height should be greater than width, see ImGui reference for solution");
	
	//place 4 white pixels in the top-left corner for other UI usage without having to swap textures
	u8* pixels = (u8*)memory_talloc((texture_size_x * texture_size_y)*sizeof(u8));
	pixels[0] = 255; pixels[1] = 255; pixels[texture_size_x] = 255; pixels[texture_size_x+1] = 255;
	
	//perform the font packing
	stbtt_pack_context* pc = (stbtt_pack_context*)memory_talloc(1*sizeof(stbtt_pack_context));
	success = stbtt_PackBegin(pc, pixels + 2*texture_size_x, texture_size_x, texture_size_y-2, 0, glyph_padding, 0); Assert(success);
	stbtt_PackSetSkipMissingCodepoints(pc, true);
	success = stbtt_PackFontRanges(pc, contents.str, 0, ranges, num_ranges); //NOTE(delle) this will return 0 if there are any missing codepoints
	stbtt_PackEnd(pc);
	
	/*
	//normalize and offset the UVs
	forX(range, num_ranges){
		for(u32 codepoint = ranges[range].first_unicode_codepoint_in_range;
			codepoint < ranges[range].first_unicode_codepoint_in_range + ranges[range].num_chars;
			++codepoint)
		{
			
		}
	}
	*/
	
	//get extra font rendering info
	f32 ascent, descent, lineGap;
	stbtt_GetScaledFontVMetrics(contents.str, 0, (f32)size, &ascent, &descent, &lineGap);
	int x0, y0, x1, y1;
	stbtt_GetFontBoundingBox(&info, &x0, &y0, &x1, &y1);
	int max_width = x1 - x0, max_height = y1 - y0;
	f32 aspect_ratio = (f32)max_height / (f32)max_width;
	
	Texture* texture = assets_texture_create_from_memory(pixels, filename, texture_size_x, texture_size_y,
														 ImageFormat_BW, TextureType_TwoDimensional, TextureFilter_Nearest,
														 TextureAddressMode_ClampToWhite, false);
	
	Font* font = (Font*)memory_alloc(sizeof(Font));
	font->type         = FontType_TTF;
	font->name         = filename;
	font->uid          = uid;
	font->max_width    = (u32)((f32)max_width / (f32)max_height * (f32)size);
	font->max_height   = size;
	font->count        = glyph_count;
	font->ttf_size[0]  = texture_size_x;
	font->ttf_size[1]  = texture_size_y;
	font->num_ranges   = num_ranges;
	font->uv_yoffset   = 2.0f / (f32)texture_size_y;
	font->ascent       = ascent;
	font->descent      = descent;
	font->line_gap     = lineGap;
	font->aspect_ratio = aspect_ratio;
	font->tex          = texture;
	font->ranges       = (FontPackRange*)ranges;
	
	array_insert_value(g_assets->font_map, index, font);
	return font;
}

void
assets_font_delete(Font* font){DPZoneScoped;
	AssetsAssert(font, "passed null Font pointer.");

	if(font == g_assets->null_font) {
		AssetsWarning("attempt to delete null font.");
		return;
	}
	
	auto [index, found] = __find_resource(font->uid, g_assets->font_map);
	if(!found) {
		AssetsDeleteNotFound(font, Font);
		return;
	}

	array_remove_ordered(g_assets->font_map, index);
	if(font->type == FontType_TTF){
		forI(font->num_ranges) memory_zfree(font->ranges[i].chardata_for_range);
		memory_zfree(font->ranges);
	}
	assets_texture_delete(font->tex);
	memory_pool_delete(g_assets->font_pool, font);
}
