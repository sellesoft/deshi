/* deshi Storage Module
Notes:
Image pixel data is loaded using stb_image.h

Index:
@storage
@mesh
@texture
@shader
@material
@armature
@model
@font
*/
#ifndef DESHI_STORAGE_H
#define DESHI_STORAGE_H
struct Mesh;
struct Texture;
struct Material;
struct Model;
struct Font;
StartLinkageC();


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @storage
typedef struct Storage{ //NOTE(delle) these arrays are non-owning since there is no real need to iterate thru them
	Mesh**     mesh_array;
	Texture**  texture_array;
	Material** material_array;
	Model**    model_array;
	Font**     font_array;
}Storage;
extern Storage* g_storage; //global storage pointer
#define DeshStorage g_storage

//Inits `Storage` memory and creates the null/default objects of each asset
//  requires the `Memory` and `Render` modules to be init
void storage_init();

//Deletes all of the the loaded assets
void storage_reset();

//Draws a window for inspection of `Storage` assets
//  requires the `UI` module to be init
void storage_browser();


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @mesh  //NOTE a mesh is supposed to be 'fixed' in that no element should change post-load
typedef u32 MeshIndex;

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
		u32  neighborCount;
		u32  face;
		b32 removed;
		b32 checked;
		
		u32* neighborArray;
		u8*  edgeArray;
}MeshTriangle;

typedef struct MeshFace{
	vec3 normal;
		vec3 center;
		u32  triangleCount;
		u32  vertexCount;
		u32  outerVertexCount;
		u32  neighborTriangleCount;
		u32  neighborFaceCount;
		
		u32* triangleArray;
		u32* vertexArray;
		u32* outerVertexArray;
		u32* neighborTriangleArray;
		u32* neighborFaceArray;
}MeshFace;

typedef struct Mesh{
	u32 bytes;
	char name[64];
	u32 render_idx; //filled when render_load_mesh() is called
	vec3 aabbMin;
	vec3 aabbMax;
	vec3 center;
	
	u32 vertexCount;
	u32 indexCount;
	u32 triangleCount;
	u32 faceCount;
	u32 totalTriNeighborCount;
	u32 totalFaceVertexCount;
	u32 totalFaceOuterVertexCount;
	u32 totalFaceTriNeighborCount;
	u32 totalFaceFaceNeighborCount;
	
	MeshVertex*   vertexArray;
	//MeshVertexEx* vertexExArray;
	MeshIndex*    indexArray;
	MeshTriangle* triangleArray;
	MeshFace*     faceArray;
}Mesh;

//Returns a pointer to the allocated `Mesh` object (with arrays setup)
Mesh* storage_mesh_allocate(u32 indexCount, u32 vertexCount, u32 faceCount, u32 trianglesNeighborCount,
							u32 facesVertexCount, u32 facesOuterVertexCount, u32 facesNeighborTriangleCount,
							u32 facesNeighborFaceCount);

//Returns a pointer to the created `Mesh` object of a 3D rectangular cuboid with dimensions `width`/`height`/`depth` along the `XYZ` axis and vertex colors as `color`
//  calls `render_load_mesh()` after creation
Mesh* storage_mesh_create_box(f32 width, f32 height, f32 depth, u32 color);

//Returns a pointer to the `Mesh` object created from a `MESH` file named `name` in the `data/models` folder
//  calls `render_load_mesh()` after creation
Mesh* storage_mesh_create_from_file(str8 name);

//Returns a pointer to the `Mesh` object created from a `MESH` file at `path`
//  calls `render_load_mesh()` after creation
Mesh* storage_mesh_create_from_path(str8 path);

//Returns a pointer to the `Mesh` object created from mesh data at the `data` pointer as a memory-mapping of a `MESH` file
//  calls `render_load_mesh()` after creation
Mesh* storage_mesh_create_from_memory(void* data);

//Saves the `Mesh` object at `mesh` to the `data/models` folder as a `MESH` file
void  storage_mesh_save(Mesh* mesh);

//Saves the `Mesh` object at `mesh` to `path` as a `MESH` file
void  storage_mesh_save_to_path(Mesh* mesh, str8 path);

//Deletes the `Mesh` object at `mesh` after calling `render_unload_mesh()`
void  storage_mesh_delete(Mesh* mesh);

//Returns a pointer to the default `Mesh` object which is created when `storage_init()` is called
FORCE_INLINE Mesh*  storage_mesh_null(){ return DeshStorage->mesh_array[0]; };

//Returns the mesh array in `Storage`
FORCE_INLINE Mesh** storage_mesh_array(){ return DeshStorage->mesh_array; };


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
	TextureType_1D,
	TextureType_2D,
	TextureType_3D,
	TextureType_Cube,
	TextureType_Array_1D,
	TextureType_Array_2D,
	TextureType_Array_Cube,
	TextureType_COUNT
}; global const str8 TextureTypeStrings[] = { STR8("1D"), STR8("2D"), STR8("3D"), STR8("Cube"), STR8("1D Array"), STR8("2D Array"), STR8("Cube Array"), };


typedef struct Texture{
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
	TextureAddressMode uvMode;
}Texture;

//Returns a pointer to the created `Texture` object from an image file named `name` in the `data/textures` folder
//  `name` should include the extension
//  `format` determines the image format after loading it (does not have to be the same as the format on disk)
//  `type`, `filter`, and `uvMode` arguments determine usage in `Render`
//  `generateMipmaps` determines whether `Render` should generate and use mipmaps for rendering this texture
//  the `pixels` var of the created `Texture` object is freed from memory unless `keepLoaded` is true
//  calls `render_load_texture()` after creation
Texture* storage_texture_create_from_file(str8 name, ImageFormat format, TextureType type, TextureFilter filter,
										  TextureAddressMode uvMode, b32 keepLoaded, b32 generateMipmaps);

//Returns a pointer to the created `Texture` object from an image file named `name` in the `data/textures` folder
//  `name` should include the extension
//  calls `render_load_texture()` after creation
FORCE_INLINE Texture* storage_texture_create_from_file_simple(str8 name){
	return storage_texture_create_from_file(name, ImageFormat_RGBA, TextureType_2D, TextureFilter_Nearest,
											TextureAddressMode_Repeat, false, true);
}

//Returns a pointer to the created `Texture` object from an image file at `path`
//  `format` determines the image format after loading it (does not have to be the same as the format on disk)
//  `type`, `filter`, and `uvMode` arguments determine usage in `Render`
//  `generateMipmaps` determines whether `Render` should generate and use mipmaps for rendering this texture
//  the `pixels` var of the created `Texture` object is freed from memory unless `keepLoaded` is true
//  calls `render_load_texture()` after creation
Texture* storage_texture_create_from_path(str8 path, ImageFormat format, TextureType type, TextureFilter filter,
										  TextureAddressMode uvMode, b32 keepLoaded, b32 generateMipmaps);

//Returns a pointer to the created `Texture` object from an image file at `path`
//  calls `render_load_texture()` after creation
FORCE_INLINE Texture* storage_texture_create_from_path_simple(str8 path){
	return storage_texture_create_from_path(path, ImageFormat_RGBA, TextureType_2D, TextureFilter_Nearest,
											TextureAddressMode_Repeat, false, true);
}

//Returns a pointer to the created `Texture` object from pixel data at the `data` pointer of size `width*height` in the byte format specified by `format`
//  `format` determines the image format after loading it
//  `type`, `filter`, and `uvMode` arguments determine usage in `Render`
//  `generateMipmaps` determines whether `Render` should generate and use mipmaps for rendering this texture
//  calls `render_load_texture()` after creation
Texture* storage_texture_create_from_memory(void* data, str8 name, u32 width, u32 height, ImageFormat format,
											TextureType type, TextureFilter filter, TextureAddressMode uvMode,
											b32 generateMipmaps);

//Returns a pointer to the created `Texture` object from pixel data at the `data` pointer of size `width*height` in the byte format specified by `format`
//  `format` determines the image format after loading it
//  calls `render_load_texture()` after creation
FORCE_INLINE Texture* storage_texture_create_from_memory_simple(void* data, str8 name, u32 width, u32 height, ImageFormat format){
	return storage_texture_create_from_memory(data, name, width, height, format, TextureType_2D, TextureFilter_Nearest,
											  TextureAddressMode_Repeat, true);
}

//Deletes the `Texture` object at `texture` after calling `render_unload_texture()`
void     storage_texture_delete(Texture* texture);

//Returns a pointer to the default `Texture` object which is created when `storage_init()` is called
FORCE_INLINE Texture*  storage_texture_null(){ return DeshStorage->texture_array[0]; };

//Returns the texture array in `Storage`
FORCE_INLINE Texture** storage_texture_array(){ return DeshStorage->texture_array; };


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @shader
typedef Type Shader; enum{ 
	Shader_NULL,
	Shader_Flat,
	Shader_Phong,
	Shader_PBR,
	Shader_Wireframe,
	Shader_Lavalamp,
	Shader_Testing0,
	Shader_Testing1,
	Shader_COUNT,
}; global str8 ShaderStrings[] = { str8_lit("NULL"), str8_lit("Flat"), str8_lit("Phong"), str8_lit("PBR"), str8_lit("Wireframe"), str8_lit("Lavalamp"), str8_lit("Testing0"), str8_lit("Testing1") };


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @material
typedef Flags MaterialFlags; enum{
	MaterialFlags_NONE = 0,
};

typedef struct Material{
	char name[64];
	u32 render_idx; //filled when render_load_material() is called
	Shader shader;
	MaterialFlags flags;
	Texture** textureArray;
}Material;

//Returns a pointer to the allocated `Material` object (with texture array reserved)
Material* storage_material_allocate(u32 textureCount);

//Returns a pointer to the created `Material` object with `shader`, `flags`, and `textures`; where `textures` are indexes in `Storage`
//  calls `render_load_material()` after creation
Material* storage_material_create(str8 name, Shader shader, MaterialFlags flags, Texture** textures, u32 texture_count);

//Returns a pointer to the created `Material` object from a `MAT` file named `name` from the `data/models` folder
//  calls `render_load_material()` after creation
Material* storage_material_create_from_file(str8 name);

//Returns a pointer to the created `Material` object from a `MAT` file at `path`
//  calls `render_load_material()` after creation
Material* storage_material_create_from_path(str8 path);

//Saves the `Material` object at `material` to the `data/models` folder as a `MAT` file
void      storage_material_save(Material* material);

//Saves the `Material` object at `material` to `path` as a `MAT` file
void      storage_material_save_to_path(Material* material, str8 path);

//Deletes the `Material` object at `material` after calling `render_unload_material()`
void      storage_material_delete(Material* material);

//Returns a pointer to the default `Material` object which is created when `storage_init()` is called
FORCE_INLINE Material*  storage_material_null(){ return DeshStorage->material_array[0]; };

//Returns the material array in `Storage`
FORCE_INLINE Material** storage_material_array(){ return DeshStorage->material_array; };


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @armature
struct Armature;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @model
typedef Flags ModelFlags; enum{
	ModelFlags_NONE = 0,
};

typedef struct ModelBatch{
	u32 indexOffset;
		u32 indexCount;
	Material* material;
}ModelBatch;

typedef struct Model{
	 char name[64];
	ModelFlags flags;
	Mesh* mesh;
	Armature* armature;
	 ModelBatch* batchArray;
}Model;

//Returns a pointer to the allocated `Model` object (with batch array reserved)
Model* storage_model_allocate(u32 batchCount);

//Returns a pointer to the created `Model` object from either `OBJ`, `MODEL`/`MESH`, `MESH`/`OBJ`/`MAT`, or `OBJ`/`MAT` file pairs named `name` with `flags` from the `data/models` folder
//  creates a new `Mesh` object in `Storage` from an `OBJ` file if no `MESH` file is found or `forceLoadOBJ` is true and then calls `render_load_mesh()`
Model* storage_model_create_from_file(str8 name, ModelFlags flags, b32 forceLoadOBJ);

//Returns a pointer to the created `Model` object from the `OBJ` file at `obj_path` with `flags`
//  creates a new mesh if the `OBJ` hasn't been loaded already and then calls `render_load_mesh()`
Model* storage_model_create_from_obj(str8 obj_path, ModelFlags flags);

//Returns a pointer to the created `Model` object from a `Mesh` pointer with `flags`
Model* storage_model_create_from_mesh(Mesh* mesh, ModelFlags flags);

//Returns a pointer to the created `Model` object from a `Mesh` pointer and the materials from the `OBJ` file at `obj_path` with `flags`
//  calls `render_load_mesh()` after creation
Model* storage_model_create_from_mesh_obj(Mesh* mesh, str8 obj_path, ModelFlags flags);

//Eeturns a pointer to a new copy in `Storage` of the `Model` object at `base`
Model* storage_model_copy(Model* base);

//Saves the `Model` object at `model` to the `data/models` folder as a `MODEL` file
void   storage_model_save(Model* model);

//Saves the `Model` object at `model` to `path` as a `MODEL` file
void   storage_model_save_to_path(Model* model, str8 path);

//Deletes the `Model` object at `model`
void   storage_model_delete(Model* model);

//Returns a pointer to the default `Model` object which is created when `storage_init()` is called
FORCE_INLINE Model*  storage_model_null(){ return DeshStorage->model_array[0]; };

//Returns the model array in `Storage`
FORCE_INLINE Model** storage_model_array(){ return DeshStorage->model_array; };


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
Font* storage_font_create_from_file(str8 name, u32 height);

//Returns a pointer to the created `Font` object from a `TTF` or `BDF` file at `path` with `height` in pixels
//  loading a `BDF` font ignores the `height` argument since the font height is baked into the file
Font* storage_font_create_from_path(str8 path, u32 height);

//Returns a pointer to the created `Font` object from a `BDF` file named `name` from the `data/fonts` folder
Font* storage_font_create_from_file_bdf(str8 name);

//Returns a pointer to the created `Font` object from a `BDF` file at `path`
Font* storage_font_create_from_path_bdf(str8 path);

//Returns a pointer to the created `Font` object from a `TTF` file named `name` with `height` in pixels from the `data/fonts` folder
Font* storage_font_create_from_file_ttf(str8 name, u32 height);

//Returns a pointer to the created `Font` object from a `TTF` file at `path` with `height` in pixels
Font* storage_font_create_from_path_ttf(str8 path, u32 height);

//Deletes the `Font` object at `font` after calling `storage_texture_delete()` on the font's texture
void  storage_font_delete(Font* font);

//Returns a pointer to the default `Font` object which is created when `storage_init()` is called
FORCE_INLINE Font*  storage_font_null(){ return DeshStorage->font_array[0]; };

//Returns the font array in `Storage`
FORCE_INLINE Font** storage_font_array(){ return DeshStorage->font_array; };


EndLinkageC();
#endif //DESHI_STORAGE_H