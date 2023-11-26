/* deshi assets module
 
	The primary purpose of assets is the loading and management of typical resources you would use in a game
	such as meshes, models, materials, textures, etc. Assets acts as a wrapper over the render api and the goal
	is to provide an api one can use instead of render when advanced features are not needed.

Notes:
Image pixel data is loaded using stb_image.h

Index:
@assets
@helpers
@mesh
@texture
@shader
@material
@armature
@model
@font
*/
#ifndef DESHI_ASSETS_H
#define DESHI_ASSETS_H
#include "math/vector.h"
#include "math/matrix.h"
struct Mesh;
typedef u32 MeshIndex;
struct MeshVertex;
struct Texture;
struct Material;
struct MaterialInstance;
struct Model;
struct Font;
struct Shader;
struct GraphicsBuffer;
struct GraphicsImage;
struct GraphicsImageView;
struct GraphicsSampler;
struct GraphicsDescriptor;
struct GraphicsDescriptorSet;
struct GraphicsDescriptorSetLayout;
struct GraphicsPipeline;
struct GraphicsShader;
struct GraphicsRenderPass;
struct UniformBufferObject;
struct Window;
StartLinkageC();

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @assets

typedef struct Assets { 
	Mesh**     mesh_map;
	Texture**  texture_map;
	Material** material_map;
	Model**    model_map;
	Font**     font_map;
	Shader**   shader_map;

	Mesh*                mesh_pool;
	Texture*             texture_pool;
	Material*            material_pool;
	MaterialInstance*    material_instance_pool;
	Model*               model_pool;
	Font*                font_pool;
	UniformBufferObject* ubo_pool;
	Shader*              shader_pool;

	Texture*  null_texture;
	Font*     null_font;
	Material* null_material;
	Mesh*     null_mesh;
	Model*    null_model;
	Shader*   null_vertex_shader;
	Shader*   null_fragment_shader;

	GraphicsPipeline* null_pipeline;
	
	// standard layout for ubos used with asset models
	// this is always bound to set 0 binding 0
	GraphicsDescriptorSetLayout* ubo_layout;
	// NOTE(sushi) array containing 1 descriptor so that we can access info about it later on (dunno if this would be useful or not so if not just remove it)
	GraphicsDescriptor*  ubo_descriptors;
	GraphicsDescriptorSet* view_proj_ubo;

	struct {
		mat4 view;
		mat4 proj;
	} base_ubo;
	UniformBufferObject* base_ubo_handle;
	
	// currently this is taken as the render pass that is 
	// set for the presentation frame of the window 
	// passed to assets_init
	// eventually we will want to support drawing to 
	// multiple windows and so this will need to be changed
	GraphicsRenderPass* render_pass;

// TODO(sushi) try using manually managed global buffers 
//             to see if they are more performant
//	u64 mesh_vertexes_cursor;
//	u64 mesh_vertexes_reserved;
//	u64 mesh_indexes_cursor;
//	u64 mesh_indexes_reserved;
//	Mesh** inactive_meshes_vertex_sorted;
//	Mesh** inactive_meshes_index_sorted;
//	// GPU equivalents
//	RenderBuffer* mesh_vertex_buffer;
//	RenderBuffer* mesh_index_buffer;
}Assets;
extern Assets* g_assets; //global assets pointer
#define DeshAssets g_assets

//Inits `Assets` memory and creates the null/default objects of each asset
//  requires the `Memory` and `Render` modules to be init
void assets_init(Window* window);

//Deletes all of the the loaded assets
void assets_reset();

// TODO(sushi) put these somewhere better later

void assets_update_camera_view(mat4* view_matrix);

void assets_update_camera_projection(mat4* projection);

// Generates a unique id from the given name.
u64 assets_make_unique_id(str8 name);

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @helpers

// Sets up the given pipeline for rendering assets related things.
// This does not add any shader stages.
void assets_setup_pipeline(GraphicsPipeline* pipeline);

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @mesh  //NOTE a mesh is supposed to be 'fixed' in that no element should change post-load

typedef struct MeshVertex{ //36 bytes
	vec3 pos;
	vec2 uv;
	u32  color;
	vec3 normal;
}MeshVertex;

typedef struct MeshVertexEx{ //56 bytes
	vec3 tangent;
	vec3 bitangent;
	u32 boneIndexes[4];
	f32 boneWeights[4];
}MeshVertexEx;

typedef struct MeshTriangle{
	vec3 normal;
	vec3 p[3];
	u32  v[3];
	u32  neighbor_count;
	u32  face;
	b32 removed;
	b32 checked;
	
	u32* neighbor_array;
	u8*  edge_array;
}MeshTriangle;

typedef struct MeshFace{
	vec3 normal;
	vec3 center;
	u32  triangle_count;
	u32  vertex_count;
	u32  outer_vertex_count;
	u32  neighbor_triangle_count;
	u32  neighbor_face_count;
	
	u32* triangle_array;
	u32* vertex_array;
	u32* outer_vertex_array;
	u32* neighbor_triangle_array;
	u32* neighbor_face_array;
}MeshFace;

typedef struct Mesh{
	str8 name;
	u64 uid; // unique id used in the maps, just a hash of the given name for now
	u32 bytes;
	u64 render_idx;
	GraphicsBuffer* vertex_buffer;
	GraphicsBuffer* index_buffer;
	vec3 aabb_min;
	vec3 aabb_max;
	vec3 center;
	
	u32 vertex_count;
	u32 index_count;
	u32 triangle_count;
	u32 face_count;
	u32 total_tri_neighbor_count;
	u32 total_face_vertex_count;
	u32 total_face_outer_vertex_count;
	u32 total_face_tri_neighbor_count;
	u32 total_face_face_neighbor_count;
	
	MeshVertex*   vertex_array;
	//MeshVertexEx* vertexExArray;
	MeshIndex*    index_array;
	MeshTriangle* triangle_array;
	MeshFace*     face_array;
}Mesh;

//Returns a pointer to the allocated `Mesh` object (with arrays setup)
Mesh* assets_mesh_allocate(u32 indexCount, u32 vertexCount, u32 faceCount, u32 trianglesNeighborCount,
						   u32 facesVertexCount, u32 facesOuterVertexCount, u32 facesNeighborTriangleCount,
						   u32 facesNeighborFaceCount);

//Returns a pointer to the created `Mesh` object of a 3D rectangular cuboid with dimensions `width`/`height`/`depth` along the `XYZ` axis and vertex colors as `color`
//  calls `render_load_mesh()` after creation
Mesh* assets_mesh_create_box(str8 name, f32 width, f32 height, f32 depth, u32 color);

//Returns a pointer to the `Mesh` object created from a `MESH` file named `name` in the `data/models` folder
//  calls `render_load_mesh()` after creation
Mesh* assets_mesh_create_from_file(str8 name);

//Returns a pointer to the `Mesh` object created from a `MESH` file at `path`
//  calls `render_load_mesh()` after creation
Mesh* assets_mesh_create_from_path(str8 path);

//Returns a pointer to the `Mesh` object created from mesh data at the `data` pointer as a memory-mapping of a `MESH` file
//  calls `render_load_mesh()` after creation
Mesh* assets_mesh_create_from_memory(void* data);

//Saves the `Mesh` object at `mesh` to `path` as a `MESH` file
void  assets_mesh_save_to_path(Mesh* mesh, str8 path);

//Deletes the `Mesh` object at `mesh` after calling `render_unload_mesh()`
void  assets_mesh_delete(Mesh* mesh);

//Returns a pointer to the default `Mesh` object which is created when `assets_init()` is called
FORCE_INLINE Mesh*  assets_mesh_null(){ return g_assets->null_mesh; };

//Returns the mesh array in `Assets`
FORCE_INLINE Mesh** assets_mesh_map(){ return g_assets->mesh_map; };

// Attempts to retrieve a mesh by the name it would have been created with.
// Returns 0 if no mesh could be found.
Mesh* assets_mesh_get_by_name(str8 name);

// Attempts to retrieve a mesh by the unique id it would have been assigned at creation.
// Returns 0 if no mesh could be found.
Mesh* assets_mesh_get_by_uid(u64 uid);

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @texture
typedef Type ImageFormat; enum{ //NOTE value = bytes per pixel
	ImageFormat_BW   = 1,
	ImageFormat_BWA  = 2,
	ImageFormat_RGB  = 3,
	ImageFormat_RGBA = 4,
}; global const str8 ImageFormatStrings[] = { STR8("BW"), STR8("BWA"), STR8("RGB"), STR8("RGBA") };

typedef Type TextureFilter; enum{
	TextureFilter_Nearest, //selects single value
	TextureFilter_Linear,  //combines nearby pixels with linear weight
	TextureFilter_Cubic,   //combines even more pixels with Catmull-Rom weights
	TextureFilter_COUNT,
}; global const str8 TextureFilterStrings[] = { STR8("Nearest"), STR8("Linear"), STR8("Cubic") };

typedef Type TextureAddressMode; enum{ //what happens when uv values are beyond 0..1
	TextureAddressMode_Repeat,             //uv values loop around (1.1 = .1)
	TextureAddressMode_MirroredRepeat,     //uv values loop around but mirrored (1.1 = .9)
	TextureAddressMode_ClampToEdge,        //uv values are the edge value (1.1 = 1)
	TextureAddressMode_ClampToWhite,       //uv values are white
	TextureAddressMode_ClampToBlack,       //uv values are black
	TextureAddressMode_ClampToTransparent, //uv values are transparent
	TextureAddressMode_COUNT,
}; global const str8 TextureAddressModeStrings[] = { STR8("Repeat"), STR8("Mirrored Repeat"), STR8("Clamp To Edge"), STR8("Clamp To White"), STR8("Clamp To Black"), STR8("Clamp To Transparent") };

typedef Type TextureType; enum{
	TextureType_OneDimensional,
	TextureType_TwoDimensional,
	TextureType_ThreeDimensional,
	TextureType_Cube,
	TextureType_Array_OneDimensional,
	TextureType_Array_TwoDimensional,
	TextureType_Array_Cube,
	TextureType_COUNT
}; global const str8 TextureTypeStrings[] = { STR8("1D"), STR8("2D"), STR8("3D"), STR8("Cube"), STR8("1D Array"), STR8("2D Array"), STR8("Cube Array"), };


typedef struct Texture{
	str8 name;
	u64 uid; // unique id used in the maps, just a hash of the given name for now
	s32 width;
	s32 height;
	s32 depth;
	s32 mipmaps;
	u8* pixels; //pixel data allocated during creation
	ImageFormat format;
	TextureType type;
	TextureFilter filter;
	TextureAddressMode uv_mode;

	GraphicsImage* image;
	GraphicsImageView* image_view;
	GraphicsSampler* sampler;

	GraphicsDescriptorSet* ui_descriptor_set;
}Texture;

//Returns a pointer to the created `Texture` object from an image file at `path`
//  `format` determines the image format after loading it (does not have to be the same as the format on disk)
//  `type`, `filter`, and `uvMode` arguments determine usage in `Render`
//  `generateMipmaps` determines whether `Render` should generate and use mipmaps for rendering this texture
//  the `pixels` var of the created `Texture` object is freed from memory unless `keepLoaded` is true
//  calls `render_load_texture()` after creation
Texture* assets_texture_create_from_path(str8 name, str8 path, ImageFormat format, TextureType type, TextureFilter filter,
										 TextureAddressMode uvMode, b32 keepLoaded, b32 generateMipmaps);

//Returns a pointer to the created `Texture` object from an image file at `path`
//  calls `render_load_texture()` after creation
FORCE_INLINE Texture* assets_texture_create_from_path_simple(str8 name, str8 path){
	return assets_texture_create_from_path(name, path, ImageFormat_RGBA, TextureType_TwoDimensional, TextureFilter_Nearest,
										   TextureAddressMode_Repeat, false, true);
}

//Returns a pointer to the created `Texture` object from pixel data at the `data` pointer of size `width*height` in the byte format specified by `format`
//  `format` determines the image format after loading it
//  `type`, `filter`, and `uvMode` arguments determine usage in `Render`
//  `generateMipmaps` determines whether `Render` should generate and use mipmaps for rendering this texture
//  calls `render_load_texture()` after creation
Texture* assets_texture_create_from_memory(void* data, str8 name, u32 width, u32 height, ImageFormat format,
										   TextureType type, TextureFilter filter, TextureAddressMode uvMode,
										   b32 generateMipmaps);

//Returns a pointer to the created `Texture` object from pixel data at the `data` pointer of size `width*height` in the byte format specified by `format`
//  `format` determines the image format after loading it
//  calls `render_load_texture()` after creation
FORCE_INLINE Texture* assets_texture_create_from_memory_simple(void* data, str8 name, u32 width, u32 height, ImageFormat format){
	return assets_texture_create_from_memory(data, name, width, height, format, TextureType_TwoDimensional, TextureFilter_Nearest,
											 TextureAddressMode_Repeat, true);
}

//Deletes the `Texture` object at `texture` after calling `render_unload_texture()`
void assets_texture_delete(Texture* texture);

// Updates the region at 'offset' of size 'extent' in 'texture' using its internal pixels array
void assets_texture_update(Texture* texture, vec2i offset, vec2i extent);

//Returns a pointer to the default `Texture` object which is created when `assets_init()` is called
FORCE_INLINE Texture* assets_texture_null(){ return g_assets->null_texture; };

//Returns the texture array in `Assets`
FORCE_INLINE Texture** assets_texture_map(){ return DeshAssets->texture_map; };

// Attempts to retrieve a texture by the name it would have been created with.
// Returns 0 if no texture could be found.
Texture* assets_texture_get_by_name(str8 name);

// Attempts to retrieve a texture by the unique id it would have been assigned at creation.
// Returns 0 if no texture could be found.
Texture* assets_texture_get_by_uid(u64 uid);


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @shader


// TODO(sushi) if this is kept around, it may be worth making it specify 
//             the actual variables used in the ubo so that we can use 
//             that information in editors and such
typedef struct UniformBufferObject {
	u32 size;
	GraphicsBuffer* buffer;
} UBO;

UBO* assets_ubo_create(u32 size);

// TODO(sushi) offset and size
void assets_ubo_update(UBO* ubo, void* data);

void assets_ubo_delete(UBO* ubo);

typedef Type ShaderType; enum {
	ShaderType_Vertex,
	ShaderType_Geometry,
	ShaderType_Fragment,
};

const str8 ShaderTypeStrings[] = {
	str8l("Vertex"),
	str8l("Geometry"),
	str8l("Fragment"),
};

typedef Type ShaderResourceType; enum {
 	ShaderResourceType_UBO,
	ShaderResourceType_Texture,
};

const str8 ShaderResourceTypeStrings[] = {
	str8l("UBO"),
	str8l("Texture"),
};

// Sum type for specifying resources used by a shader.
typedef struct ShaderResource {
	ShaderResourceType type;
	union {
		UBO* ubo;
		Texture* texture;
	};
} ShaderResource;

typedef struct Shader {
	str8 name;
	u64  uid;
	ShaderType type;
	ShaderResourceType* resources;

	GraphicsShader* handle;
} Shader;


// Load a shader using 'source' as its source code.
Shader* assets_shader_load_from_source(str8 name, str8 source, ShaderType type);

// Load a shader source file from the given path.
Shader* assets_shader_load_from_path(str8 name, str8 path, ShaderType type);

// Reloads the given shader.
// Note that this will remake the backend graphics information for all materials
// that use the given shader.
void assets_shader_reload(Shader* shader);

// Attempt to retrieve a shader by the name it would have been created with.
// Returns 0 if no shader could be found.
Shader* assets_shader_get_by_name(str8 name);

// Attempt to retrieve a shader by the unique id it would have been assigned when it was created.
// Returns 0 if no shader could be found.
Shader* assets_shader_get_by_uid(u64 uid);

FORCE_INLINE Shader* assets_shader_null_vertex() { return g_assets->null_vertex_shader; }
FORCE_INLINE Shader* assets_shader_null_fragment() { return g_assets->null_fragment_shader; }
FORCE_INLINE Shader** assets_shader_map() { return g_assets->shader_map; }

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @material

typedef struct ShaderStages {
	Shader* vertex;
	Shader* geometry;
	Shader* fragment;
} ShaderStages;

typedef struct Material{
	str8 name;
	u64 uid; // unique id used in the maps, just a hash of the given name for now
	Texture** texture_array;
	
	// description of the stages this material goes through
	// as well as the resources needed for each stage
	ShaderStages stages;

	// The allocated resources this material will use.
	ShaderResource* resources;

	// possibly shared by materials
	GraphicsPipeline* pipeline;
	// unique to each instance of a material
	GraphicsDescriptorSet* descriptor_set;
}Material;

// Returns a pointer to the allocated `Material` object (with texture array reserved)
Material* assets_material_allocate(u32 textureCount);

Material* assets_material_create(str8 name, ShaderStages shader_stages, ShaderResource* resources);

// Returns a pointer to the created `Material` object from a `MAT` file at `path`
//  calls `render_load_material()` after creation
Material* assets_material_create_from_path(str8 name, str8 path);

// Saves the `Material` object at `material` to the `data/models` folder as a `MAT` file
void      assets_material_save(Material* material);

// Saves the `Material` object at `material` to `path` as a `MAT` file
void      assets_material_save_to_path(Material* material, str8 path);

// Deletes the `Material` object at `material` after calling `render_unload_material()`
void      assets_material_delete(Material* material);

// Duplicates the given material but with a newly allocated set of resources and a new name.
Material* assets_material_duplicate(str8 name, Material* material, ShaderResource* new_resources);

//Returns a pointer to the default `Material` object which is created when `assets_init()` is called
FORCE_INLINE Material* assets_material_null(){ return g_assets->null_material; };

//Returns the material array in `Assets`
FORCE_INLINE Material** assets_material_map(){ return g_assets->material_map; };

// Attempt to retrieve a material by the name it would have been created with.
Material* assets_material_get_by_name(str8 name);

// Attempt to retrieve a material by the uid it would have been assigned at creation,
// which is the str8_hash64 of the name it was given at creation.
Material* assets_material_get_by_uid(u64 uid);

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @armature
struct Armature;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @model
typedef Flags ModelFlags; enum{
	ModelFlags_NONE = 0,
};

typedef struct ModelBatch{
	u32 index_offset;
	u32 index_count;
	Material* material;
}ModelBatch;

typedef struct Model{
	str8 name;
	u64 uid;
	ModelFlags flags;
	Mesh* mesh;
	Armature* armature;
	ModelBatch* batch_array;
}Model;

//Returns a pointer to the allocated `Model` object (with batch array reserved)
Model* assets_model_allocate(u32 batchCount);

//Returns a pointer to the created `Model` object from the `OBJ` file at `obj_path` with `flags`
//  creates a new mesh if the `OBJ` hasn't been loaded already and then calls `render_load_mesh()`
Model* assets_model_create_from_obj(str8 obj_path, ModelFlags flags);

//Returns a pointer to the created `Model` object from a `Mesh` pointer with `flags`
Model* assets_model_create_from_mesh(Mesh* mesh, ModelFlags flags);

//Returns a pointer to the created `Model` object from a `Mesh` pointer and the materials from the `OBJ` file at `obj_path` with `flags`
//  calls `render_load_mesh()` after creation
Model* assets_model_create_from_mesh_obj(Mesh* mesh, str8 obj_path, ModelFlags flags);

//Returns a pointer to a new copy in `Assets` of the `Model` object at `base`
Model* assets_model_duplicate(Model* base);

//Saves the `Model` object at `model` to the `data/models` folder as a `MODEL` file
void   assets_model_save(Model* model);

//Saves the `Model` object at `model` to `path` as a `MODEL` file
void   assets_model_save_to_path(Model* model, str8 path);

//Deletes the `Model` object at `model`
void   assets_model_delete(Model* model);

//Returns a pointer to the default `Model` object which is created when `assets_init()` is called
FORCE_INLINE Model*  assets_model_null(){ return g_assets->null_model; };

//Returns the model array in `Assets`
FORCE_INLINE Model** assets_model_map(){ return g_assets->model_map; };

// Attempts to retrieve a model by the name it would have been given when it was created.
// Returns 0 if no model could be found.
Model* assets_model_get_by_name(str8 name);

// Attempts to retrieve a model by the uid it would have been assigned at creation.
// returns 0 if no model could be found.
Model* assets_model_get_by_uid(u64 uid);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @font
//NOTE to calculate the width of a char from a specified height do: height / aspect_ratio / max_width
//NOTE we mirror stb types so we don't have to include that header here

typedef Type FontType; enum{
	FontType_NONE,
	FontType_BDF,
	FontType_TTF,
	FontType_COUNT
};

typedef struct FontAlignedQuad{ //mirror to stbtt_aligned_quad
	f32 x0, y0, u0, v0; // top-left
	f32 x1, y1, u1, v1; // bottom-right
}FontAlignedQuad;

typedef struct FontPackedChar{ //mirror to stbtt_packedchar
	unsigned short x0, y0, x1, y1; // coordinates of bbox in bitmap
	f32 xoff, yoff, xadvance;
	f32 xoff2, yoff2;
}FontPackedChar;

typedef struct FontPackRange{ //mirror to stbtt_pack_range
	f32 font_size;
	s32 first_codepoint;  // if non-zero, then the chars are continuous, and this is the first codepoints
	s32* array_of_unicode_codepoints;       // if non-zero, then this is an array of unicode codepoints
	s32 num_chars;
	FontPackedChar* chardata_for_range; // output
	unsigned char h_oversample, v_oversample; // don't set these, they're used internally
}FontPackRange;

typedef struct Font{
	FontType type;
	str8 name;
	u64 uid; // unique id used in the maps, just a hash of the given name for now
	char weight[64];
	u32 max_width;
	u32 max_height;
	u32 count;
	u32 ttf_size[2];
	u32 num_ranges;
	f32 uv_yoffset;   //the y offset of UV since we are now packing a white square into every font
	f32 ascent;       //the highest point above baseline a glyph reaches
	f32 descent;      //the lowest point below baseline a glyph reaches
	f32 line_gap;     //the recommended 
	f32 aspect_ratio; //max character height / max character width
	Texture* tex;
	FontPackRange* ranges; //stbtt_pack_range
}Font;

//Returns a pointer to the `FontPackedChar` object that represents `codepoint` in `font`
//  returns 0 if the codepoint is not in the font
FontPackedChar* font_packed_char(Font* font, u32 codepoint);

//Constructs a `FontAlignedQuad` object for the `codepoint` in `font`
//  returns a zeroed struct if the codepoint is not in the font
FontAlignedQuad font_aligned_quad(Font* font, u32 codepoint, vec2* pos, vec2 scale);

//Returns the pixel-size bounding box of the `text` when using `font`
vec2 font_visual_size(Font* font, str8 text);

//Returns a pointer to the created `Font` object from a `TTF` or `BDF` file at `path` with `height` in pixels
//  loading a `BDF` font ignores the `height` argument since the font height is baked into the file
Font* assets_font_create_from_path(str8 path, u32 height);

//Returns a pointer to the created `Font` object from a `BDF` file at `path`
Font* assets_font_create_from_path_bdf(str8 path);

//Returns a pointer to the created `Font` object from a `TTF` file at `path` with `height` in pixels
Font* assets_font_create_from_path_ttf(str8 path, u32 height);

//Deletes the `Font` object at `font` after calling `assets_texture_delete()` on the font's texture
void  assets_font_delete(Font* font);

//Returns a pointer to the default `Font` object which is created when `assets_init()` is called
FORCE_INLINE Font* assets_font_null(){ return g_assets->null_font; };

//Returns the font array in `Assets`
FORCE_INLINE Font** assets_font_map(){ return DeshAssets->font_map; };

// Attempts to retrieve a font by the name it was given when it was created. 
// Returns 0 if no font could be found.
Font* assets_font_get_by_name(str8 name);

// Attempts to retrieve a font by a unique id it would have been given when it was created. 
// Returns 0 if no font could be found.
Font* assets_font_get_by_uid(u64 uid);


EndLinkageC();
#endif //DESHI_ASSETS_H
