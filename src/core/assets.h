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
struct RenderBuffer;
struct RenderImage;
struct RenderImageView;
struct RenderSampler;
struct RenderDescriptor;
struct RenderDescriptorSet;
struct RenderDescriptorSetLayout;
struct RenderPipeline;
struct RenderPass;
struct UniformBufferObject;
struct Window;
StartLinkageC();

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @assets

typedef struct Assets{ //NOTE(delle) these arrays are non-owning since there is no real need to iterate thru them
	// TODO(sushi) convert to pools
	Mesh**     mesh_array;
	Texture**  texture_array;
	Material** material_array;
	Model**    model_array;
	Font**     font_array;

	Mesh*             mesh_pool;
	Texture*          texture_pool;
	Material*         material_pool;
	MaterialInstance* material_instance_pool;
	Model*            model_pool;
	Font*             font_pool;
	UniformBufferObject* ubo_pool;

	Texture* null_texture;
	Font* null_font;
	Material* null_material;

	RenderPipeline* null_pipeline;
	
	// standard layout for ubos used with asset models
	// this is always bound to set 0 binding 0
	RenderDescriptorSetLayout* ubo_layout;
	// NOTE(sushi) array containing 1 descriptor so that we can access info about it later on (dunno if this would be useful or not so if not just remove it)
	RenderDescriptor*  ubo_descriptors;
	RenderDescriptorSet* view_proj_ubo;

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
	RenderPass* render_pass;

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
void assets_init();

void assets_init_x(Window* window);

//Deletes all of the the loaded assets
void assets_reset();

//Draws a window for inspection of `Assets` assets
//  requires the `UI` module to be init
void assets_browser();

// TODO(sushi) put these somewhere better later

void assets_update_camera_view(mat4* view_matrix);

void assets_update_camera_projection(mat4* projection);

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @helpers

// Sets up the given pipeline for rendering assets related things.
// This does not add any shader stages.
void assets_setup_pipeline(RenderPipeline* pipeline);

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
	u32 bytes;
	char name[64];
	u64 render_idx;
	RenderBuffer* vertex_buffer;
	RenderBuffer* index_buffer;
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
Mesh* assets_mesh_create_box(f32 width, f32 height, f32 depth, u32 color);

//Returns a pointer to the `Mesh` object created from a `MESH` file named `name` in the `data/models` folder
//  calls `render_load_mesh()` after creation
Mesh* assets_mesh_create_from_file(str8 name);

//Returns a pointer to the `Mesh` object created from a `MESH` file at `path`
//  calls `render_load_mesh()` after creation
Mesh* assets_mesh_create_from_path(str8 path);

//Returns a pointer to the `Mesh` object created from mesh data at the `data` pointer as a memory-mapping of a `MESH` file
//  calls `render_load_mesh()` after creation
Mesh* assets_mesh_create_from_memory(void* data);

//Saves the `Mesh` object at `mesh` to the `data/models` folder as a `MESH` file
void  assets_mesh_save(Mesh* mesh);

//Saves the `Mesh` object at `mesh` to `path` as a `MESH` file
void  assets_mesh_save_to_path(Mesh* mesh, str8 path);

//Deletes the `Mesh` object at `mesh` after calling `render_unload_mesh()`
void  assets_mesh_delete(Mesh* mesh);

//Returns a pointer to the default `Mesh` object which is created when `assets_init()` is called
FORCE_INLINE Mesh*  assets_mesh_null(){ return DeshAssets->mesh_array[0]; };

//Returns the mesh array in `Assets`
FORCE_INLINE Mesh** assets_mesh_array(){ return DeshAssets->mesh_array; };


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
	str8 name_x;
	char name[64]; //NOTE(delle) includes the extension
	s32 width;
	s32 height;
	s32 depth;
	s32 mipmaps;
	u8* pixels; //pixel data allocated during creation
	u32 render_idx; //filled when render_load_texture() is called
	ImageFormat format;
	TextureType type;
	TextureFilter filter;
	TextureAddressMode uv_mode;

	RenderImage* image;
	RenderImageView* image_view;
	RenderSampler* sampler;

	RenderDescriptorSet* ui_descriptor_set;
}Texture;

//Returns a pointer to the created `Texture` object from an image file named `name` in the `data/textures` folder
//  `name` should include the extension
//  `format` determines the image format after loading it (does not have to be the same as the format on disk)
//  `type`, `filter`, and `uvMode` arguments determine usage in `Render`
//  `generateMipmaps` determines whether `Render` should generate and use mipmaps for rendering this texture
//  the `pixels` var of the created `Texture` object is freed from memory unless `keepLoaded` is true
//  calls `render_load_texture()` after creation
Texture* assets_texture_create_from_file(str8 name, ImageFormat format, TextureType type, TextureFilter filter,
										 TextureAddressMode uvMode, b32 keepLoaded, b32 generateMipmaps);

//Returns a pointer to the created `Texture` object from an image file named `name` in the `data/textures` folder
//  `name` should include the extension
//  calls `render_load_texture()` after creation
FORCE_INLINE Texture* assets_texture_create_from_file_simple(str8 name){
	return assets_texture_create_from_file(name, ImageFormat_RGBA, TextureType_TwoDimensional, TextureFilter_Nearest,
										   TextureAddressMode_Repeat, false, true);
}

//Returns a pointer to the created `Texture` object from an image file at `path`
//  `format` determines the image format after loading it (does not have to be the same as the format on disk)
//  `type`, `filter`, and `uvMode` arguments determine usage in `Render`
//  `generateMipmaps` determines whether `Render` should generate and use mipmaps for rendering this texture
//  the `pixels` var of the created `Texture` object is freed from memory unless `keepLoaded` is true
//  calls `render_load_texture()` after creation
Texture* assets_texture_create_from_path(str8 path, ImageFormat format, TextureType type, TextureFilter filter,
										 TextureAddressMode uvMode, b32 keepLoaded, b32 generateMipmaps);

//Returns a pointer to the created `Texture` object from an image file at `path`
//  calls `render_load_texture()` after creation
FORCE_INLINE Texture* assets_texture_create_from_path_simple(str8 path){
	return assets_texture_create_from_path(path, ImageFormat_RGBA, TextureType_TwoDimensional, TextureFilter_Nearest,
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
FORCE_INLINE Texture*  assets_texture_null(){ 
#ifdef RENDER_REWRITE
	return g_assets->null_texture;
#else
	return DeshAssets->texture_array[0]; 
#endif
};

//Returns the texture array in `Assets`
FORCE_INLINE Texture** assets_texture_array(){ return DeshAssets->texture_array; };


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @shader
typedef Type Shader; enum{ 
	Shader_NULL,
	Shader_Flat,
	Shader_Phong,
	Shader_PBR,
	Shader_Wireframe,
	Shader_COUNT,
}; global str8 ShaderStrings[] = { str8_lit("NULL"), str8_lit("Flat"), str8_lit("Phong"), str8_lit("PBR"), str8_lit("Wireframe") };

typedef Type ShaderType; enum {
	ShaderType_Vertex,
	ShaderType_Geometry,
	ShaderType_Fragment,
};

// TODO(sushi) if this is kept around, it may be worth making it specify 
//             the actual variables used in the ubo so that we can use 
//             that information in editors and such
typedef struct UniformBufferObject {
	u32 size;
	RenderBuffer* buffer;
} UBO;

UBO* assets_ubo_create(u32 size);

// TODO(sushi) offset and size
void assets_ubo_update(UBO* ubo, void* data);

void assets_ubo_delete(UBO* ubo);


enum ShaderResourceType {
 	ShaderResourceType_UBO,
	ShaderResourceType_Texture,
};

const str8 ShaderResourceTypeStrings[] = {
	str8l("UBO"),
	str8l("Texture"),
};

// a realization of a shader resource, eg. the memory for it has been allocated
typedef struct ShaderResource {
	ShaderResourceType type;
	union {
		UBO* ubo;
		Texture* texture;
	};
} ShaderResource;

typedef struct ShaderX {
	str8 filename;
	ShaderResourceType* resources;
} ShaderX;

// used for describing to Materials what shader stages are to be used
// and the resources used within them
typedef struct ShaderStages {
	ShaderX vertex;
	ShaderX geometry;
	ShaderX fragment;
} ShaderStages;

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @material

typedef struct Material{
	str8 name_x;
	char name[64];
	u32 render_idx; //filled when render_load_material() is called
	Shader shader;
	Texture** texture_array;
	
	// description of the stages this material goes through
	// as well as the resources needed for each stage
	ShaderStages stages;

	ShaderResource* resources;

	// possibly shared by materials
	RenderPipeline* pipeline;
	// unique to each instance of a material
	RenderDescriptorSet* descriptor_set;
}Material;

// Returns a pointer to the allocated `Material` object (with texture array reserved)
Material* assets_material_allocate(u32 textureCount);

// Returns a pointer to the created `Material` object with `shader`, `flags`, and `textures`; where `textures` are indexes in `Assets`
//  calls `render_load_material()` after creation
Material* assets_material_create(str8 name, Shader shader, Texture** textures, u32 texture_count);

// NOTE(sushi) currently window is required due to pipeline needing to know what RenderPass they are going to be
//             used in (which is )
Material* assets_material_create_x(str8 name, ShaderStages shader_stages, ShaderResource* resources);

// Returns a pointer to the created `Material` object from a `MAT` file named `name` from the `data/models` folder
//  calls `render_load_material()` after creation
Material* assets_material_create_from_file(str8 name);

// Returns a pointer to the created `Material` object from a `MAT` file at `path`
//  calls `render_load_material()` after creation
Material* assets_material_create_from_path(str8 path);

// Saves the `Material` object at `material` to the `data/models` folder as a `MAT` file
void      assets_material_save(Material* material);

// Saves the `Material` object at `material` to `path` as a `MAT` file
void      assets_material_save_to_path(Material* material, str8 path);

// Deletes the `Material` object at `material` after calling `render_unload_material()`
void      assets_material_delete(Material* material);

// Duplicates the given material but with a newly allocated set of resources and a new name.
Material* assets_material_duplicate(str8 name, Material* material, ShaderResource* new_resources);


//Returns a pointer to the default `Material` object which is created when `assets_init()` is called
FORCE_INLINE Material*  assets_material_null(){ 
#ifdef RENDER_REWRITE
	return g_assets->null_material;
#else
	return DeshAssets->material_array[0]; 
#endif
};

//Returns the material array in `Assets`
FORCE_INLINE Material** assets_material_array(){ return DeshAssets->material_array; };

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
	char name[64];
	ModelFlags flags;
	Mesh* mesh;
	Armature* armature;
	ModelBatch* batch_array;
}Model;

//Returns a pointer to the allocated `Model` object (with batch array reserved)
Model* assets_model_allocate(u32 batchCount);

//Returns a pointer to the created `Model` object from either `OBJ`, `MODEL`/`MESH`, `MESH`/`OBJ`/`MAT`, or `OBJ`/`MAT` file pairs named `name` with `flags` from the `data/models` folder
//  creates a new `Mesh` object in `Assets` from an `OBJ` file if no `MESH` file is found or `forceLoadOBJ` is true and then calls `render_load_mesh()`
Model* assets_model_create_from_file(str8 name, ModelFlags flags, b32 forceLoadOBJ);

//Returns a pointer to the created `Model` object from the `OBJ` file at `obj_path` with `flags`
//  creates a new mesh if the `OBJ` hasn't been loaded already and then calls `render_load_mesh()`
Model* assets_model_create_from_obj(str8 obj_path, ModelFlags flags);

//Returns a pointer to the created `Model` object from a `Mesh` pointer with `flags`
Model* assets_model_create_from_mesh(Mesh* mesh, ModelFlags flags);

//Returns a pointer to the created `Model` object from a `Mesh` pointer and the materials from the `OBJ` file at `obj_path` with `flags`
//  calls `render_load_mesh()` after creation
Model* assets_model_create_from_mesh_obj(Mesh* mesh, str8 obj_path, ModelFlags flags);

//Returns a pointer to a new copy in `Assets` of the `Model` object at `base`
Model* assets_model_copy(Model* base);

//Saves the `Model` object at `model` to the `data/models` folder as a `MODEL` file
void   assets_model_save(Model* model);

//Saves the `Model` object at `model` to `path` as a `MODEL` file
void   assets_model_save_to_path(Model* model, str8 path);

//Deletes the `Model` object at `model`
void   assets_model_delete(Model* model);

//Returns a pointer to the default `Model` object which is created when `assets_init()` is called
FORCE_INLINE Model*  assets_model_null(){ return DeshAssets->model_array[0]; };

//Returns the model array in `Assets`
FORCE_INLINE Model** assets_model_array(){ return DeshAssets->model_array; };

// Renders a model to the given window
// NOTE(sushi) currently assets only supports one window, the one that was passed to assets_init,
//             however we must take in the window here so we can get the frame to render to,
//             even though we could store this on g_assets, I'm not going to do that so that later
//             on when multi window support is implemented for assets, we can minimize how much 
//             we need to fix usage of it 
void assets_model_render(Window* window, Model* model, mat4* transformation);

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

//Returns a pointer to the created `Font` object from a `TTF` or `BDF` file named `name` with `height` in pixels from the `data/fonts` folder
//  loading a `BDF` font ignores the `height` argument since the font height is baked into the file
Font* assets_font_create_from_file(str8 name, u32 height);

//Returns a pointer to the created `Font` object from a `TTF` or `BDF` file at `path` with `height` in pixels
//  loading a `BDF` font ignores the `height` argument since the font height is baked into the file
Font* assets_font_create_from_path(str8 path, u32 height);

//Returns a pointer to the created `Font` object from a `BDF` file named `name` from the `data/fonts` folder
Font* assets_font_create_from_file_bdf(str8 name);

//Returns a pointer to the created `Font` object from a `BDF` file at `path`
Font* assets_font_create_from_path_bdf(str8 path);

//Returns a pointer to the created `Font` object from a `TTF` file named `name` with `height` in pixels from the `data/fonts` folder
Font* assets_font_create_from_file_ttf(str8 name, u32 height);

//Returns a pointer to the created `Font` object from a `TTF` file at `path` with `height` in pixels
Font* assets_font_create_from_path_ttf(str8 path, u32 height);

//Deletes the `Font` object at `font` after calling `assets_texture_delete()` on the font's texture
void  assets_font_delete(Font* font);

//Returns a pointer to the default `Font` object which is created when `assets_init()` is called
FORCE_INLINE Font*  assets_font_null(){ 
#ifdef RENDER_REWRITE
	return g_assets->null_font;
#else
	return DeshAssets->font_array[0]; 
#endif
};

//Returns the font array in `Assets`
FORCE_INLINE Font** assets_font_array(){ return DeshAssets->font_array; };


EndLinkageC();
#endif //DESHI_ASSETS_H
