#pragma once
#ifndef DESHI_STORAGE_H
#define DESHI_STORAGE_H

#include "font.h"
#include "model.h"
#include "texture.h"
#include "kigu/array.h"
#include "kigu/color.h"
#include "kigu/pair.h"

struct Light;
struct Storage_{
	array<Mesh*>     meshes;
	array<Texture*>  textures;
	array<Material*> materials;
	array<Model*>    models;
	array<Font*>     fonts;
	array<Light*>    lights; //TODO(delle) move this elsewhere
	
	Mesh*     null_mesh = 0;
	Texture*  null_texture = 0;
	Material* null_material = 0;
	Model*    null_model = 0;
	Font*     null_font = 0;
};

//global storage pointer
extern Storage_* g_storage;
#define DeshStorage g_storage

namespace Storage{
	//initializes `Storage` memory and creates the null/default objects of each asset
	//NOTE requires the `Memory` module
	void Init();
	
	//deletes all of the the loaded assets
	void Reset();
	
	//draws a window for inspection of `Storage` assets
	//NOTE requires the `UI` module
	void StorageBrowserUI();
	
	///////////////
	//// @mesh ////
	///////////////
	//returns index and pointer to the created `Mesh` object of a 3D rectangular cuboid with dimensions `width`/`height`/`depth` along the `XYZ` axis and vertex colors as `color`
	//    calls `Render::LoadMesh()` after creation
	pair<u32,Mesh*> CreateBoxMesh(f32 width, f32 height, f32 depth, color color = Color_White);
	
	//returns index and pointer to the created `Mesh` object from a `MESH` file named `filename` from the `data/models` folder
	//    calls `Render::LoadMesh()` after creation
	pair<u32,Mesh*> CreateMeshFromFile(str8 filename);
	
	//returns index and pointer to the created `Mesh` object from mesh data at the `data` pointer as a memory-mapping of a `MESH` file
	//    calls `Render::LoadMesh()` after creation
	pair<u32,Mesh*> CreateMeshFromMemory(void* data);
	
	//saves the `Mesh` object at `mesh` to the `data/models` folder as a `MESH` file
	void            SaveMesh(Mesh* mesh);
	
	//calls `Render::UnloadMesh()` and deletes the `Mesh` object at the `mesh` pointer if it exists in `Storage`
	//TODO implementation
	void            DeleteMesh(Mesh* mesh);
	
	//calls `Render::UnloadMesh()` and deletes the `Mesh` object at `index` in `Storage`
	inline void     DeleteMesh(u32 index){ DeleteMesh(DeshStorage->meshes[index]); };
	
	//returns a pointer to the `Mesh` object for `null_mesh` which is created when `Init()` is called
	inline Mesh*    NullMesh(){ return DeshStorage->null_mesh; };
	
	//returns the number of `Mesh` objects in `Storage`
	inline u32      MeshCount(){ return DeshStorage->meshes.size(); };
	
	//returns a pointer to the `Mesh` object at `index` in `Storage`
	inline Mesh*    MeshAt(u32 index){ return DeshStorage->meshes[index]; };
	
	//returns the index of the `Mesh` object at the `mesh` pointer in `Storage` if it exists; -1 if not found
	inline u32      MeshIndex(Mesh* mesh){ forI(DeshStorage->meshes.size()){ if(mesh == DeshStorage->meshes[i]) return i; } return -1; };
	
	//returns the name of the `Mesh` object at `index` in `Storage`
	inline str8     MeshName(u32 index){ return str8_from_cstr(DeshStorage->meshes[index]->name); };
	
	//////////////////
	//// @texture ////
	//////////////////
	//returns index and pointer to the created `Texture` object from an image file named `filename` from the `data/textures` folder using stb_image.h to load the pixel data
	//    `type`, `filter`, and `uvMode` arguments determine usage in `Render`
	//    `generateMipmaps` determines whether `Render` should generate and use mipmaps for rendering this texture
	//    the `pixels` var of the created `Texture` object is freed from memory unless `keepLoaded` is true
	//    calls `Render::LoadTexture()` after creation
	pair<u32,Texture*> CreateTextureFromFile(str8 filename, ImageFormat format = ImageFormat_RGBA, TextureType type = TextureType_2D, TextureFilter filter = TextureFilter_Nearest, TextureAddressMode uvMode = TextureAddressMode_Repeat, b32 keepLoaded = false, b32 generateMipmaps = true);
	
	//returns index and pointer to the created `Texture` object from pixel data at the `data` pointer of size `width*height` in the byte format specified by `format`
	//    `type`, `filter`, and `uvMode` arguments determine usage in `Render`
	//    `generateMipmaps` determines whether `Render` should generate and use mipmaps for rendering this texture
	//    calls `Render::LoadTexture()` after creation
	pair<u32,Texture*> CreateTextureFromMemory(void* data, str8 name, s32 width, s32 height, ImageFormat format, TextureType type = TextureType_2D, TextureFilter filter = TextureFilter_Nearest, TextureAddressMode uvMode = TextureAddressMode_Repeat, b32 generateMipmaps = true);
	
	//calls `Render::UnloadTexture()` and deletes the `Texutre` object at the `texture` pointer if it exists in `Storage`
	//TODO implementation
	void               DeleteTexture(Texture* texture);
	
	//calls `Render::UnloadTexture()` and deletes the `Texture` object at `index` in `Storage`
	inline void        DeleteTexture(u32 index){ DeleteTexture(DeshStorage->textures[index]); };
	
	//returns a pointer to the `Texture` object for `null_texture` which is created when `Init()` is called
	inline Texture*    NullTexture(){ return DeshStorage->null_texture; };
	
	//returns the number of `Texture` objects in `Storage`
	inline u32         TextureCount(){ return DeshStorage->textures.size(); };
	
	//returns a pointer to the `Texture` object at `index` in `Storage`
	inline Texture*    TextureAt(u32 index){ return DeshStorage->textures[index]; };
	
	//returns the index of the `Texture` object at the `texture` pointer in `Storage` if it exists; -1 if not found
	inline u32         TextureIndex(Texture* texture){ forI(DeshStorage->textures.size()){ if(texture == DeshStorage->textures[i]) return i; } return -1; };
	
	//returns the name of the `Texture` object at `index` in `Storage`
	inline str8        TextureName(u32 index){ return str8_from_cstr(DeshStorage->textures[index]->name); };
	
	///////////////////
	//// @material ////
	///////////////////
	//returns index and pointer to the created `Material` object with `shader`, `flags`, and `textures`; where `textures` are indexes in `Storage`
	//    calls `Render::LoadMaterial()` after creation
	pair<u32,Material*> CreateMaterial(str8 name, Shader shader = Shader_PBR, MaterialFlags flags = MaterialFlags_NONE, array<u32> textures = {});
	
	//returns index and pointer to the created `Material` object from a `MAT` file named `filename` from the `data/models` folder
	//    calls `Render::LoadMaterial()` after creation
	pair<u32,Material*> CreateMaterialFromFile(str8 filename);
	
	//saves the `Material` object at `material` to the `data/models` folder as a `MAT` file
	void                SaveMaterial(Material* material);
	
	//calls `Render::UnloadMaterial()` and deletes the `Material` object at the `material` pointer if it exists in `Storage`
	//TODO implementation
	void                DeleteMaterial(Material* material);
	
	//calls `Render::UnloadMaterial()` and deletes the `Material` object at `index` in `Storage`
	inline void         DeleteMaterial(u32 materialIdx){ DeleteMaterial(DeshStorage->materials[materialIdx]); };
	
	//returns a pointer to the `Material` object for `null_material` which is created when `Init()` is called
	inline Material*    NullMaterial(){ return DeshStorage->null_material; };
	
	//returns the number of `Material` objects in `Storage`
	inline u32          MaterialCount(){ return DeshStorage->materials.size(); };
	
	//returns a pointer to the `Material` object at `index` in `Storage`
	inline Material*    MaterialAt(u32 materialIdx){ return DeshStorage->materials[materialIdx]; };
	
	//returns the index of the `Material` object at the `material` pointer in `Storage` if it exists; -1 if not found
	inline u32          MaterialIndex(Material* material){ forI(DeshStorage->materials.size()){ if(material == DeshStorage->materials[i]) return i; } return -1; };
	
	//returns the name of the `Model` object at `index` in `Storage`
	inline str8         MaterialName(u32 index){ return str8_from_cstr(DeshStorage->materials[index]->name); };
	
	//returns the `Shader` of the `Material` object at `index` in `Storage`
	inline Shader       MaterialShader(u32 index){ return DeshStorage->materials[index]->shader; };
	
	//returns the number of `Texture` objects that are referenced by the `Model` object at `index` in `Storage`
	inline u32          MaterialTextureCount(u32 index){ return DeshStorage->materials[index]->textures.count; };
	
	//returns a pointer to the `Texture` object at `textureIdx` in the `Material` object at `materialIdx` in `Storage`
	inline Texture*     MaterialTexture(u32 mat_index, u32 tex_index){ return DeshStorage->textures[DeshStorage->materials[mat_index]->textures[tex_index]]; };
	
	////////////////
	//// @model ////
	////////////////
	//returns index and pointer to the created `Model` object from either `MODEL`/`MESH`, `MESH`/`OBJ`/`MAT`, `OBJ`/`MAT` file pairs named `filename` with `flags` from the `data/models` folder
	//    creates a new `Mesh` object in `Storage` from an `OBJ` file if no `MESH` file is found or `forceLoadOBJ` is true
	//    calls `Render::LoadMesh()` if new mesh data is generated
	pair<u32,Model*> CreateModelFromFile(str8 filename, ModelFlags flags = ModelFlags_NONE, b32 forceLoadOBJ = false);
	
	//returns index and pointer to the created `Model` object from a `Mesh` pointer with `flags`
	pair<u32,Model*> CreateModelFromMesh(Mesh* mesh, ModelFlags flags = ModelFlags_NONE);
	
	//returns index and pointer to a new copy in `Storage` of the `Model` object at `base`
	pair<u32,Model*> CopyModel(Model* base);
	
	//saves the `Model` object at `model` to the `data/models` folder as a MODEL file
	void             SaveModel(Model* model);
	
	//deletes the `Model` object at the `model` pointer if it exists in `Storage`
	//TODO implementation
	void             DeleteModel(Model* model);
	
	//deletes the `Model` object at `index` in `Storage`
	inline void      DeleteModel(u32 index){ DeleteModel(DeshStorage->models[index]); };
	
	//returns a pointer to the `Model` object for `null_model` which is created when `Init()` is called
	inline Model*    NullModel(){ return DeshStorage->null_model; };
	
	//returns the number of `Model` objects in `Storage`
	inline u32       ModelCount(){ return DeshStorage->models.size(); };
	
	//returns a pointer to the `Model` object at `index` in `Storage`
	inline Model*    ModelAt(u32 modelIdx){ return DeshStorage->models[modelIdx]; };
	
	//returns the index of the `Model` object at the `model` pointer in `Storage` if it exists; -1 if not found
	inline u32       ModelIndex(Model* model){ forI(DeshStorage->models.size()){ if(model == DeshStorage->models[i]) return i; } return -1; };
	
	//returns the name of the `Model` object at `index` in `Storage`
	inline str8      ModelName(u32 index){ return str8_from_cstr(DeshStorage->models[index]->name); };
	
	//returns the number of `Model::Batch` objects that belong to the `Model` object at `index` in `Storage`
	inline u32       ModelBatchCount(u32 index){ return DeshStorage->models[index]->batches.count; };
	
	///////////////
	//// @font ////
	///////////////
	//returns index and pointer to the created `Font` object from a BDF file named `filename` with `height` in pixels from the `data/fonts` folder
	pair<u32,Font*> CreateFontFromFileBDF(str8 filename);
	
	//returns index and pointer to the created `Font` object from a TTF file named `filename` with `height` in pixels from the `data/fonts` folder
	pair<u32,Font*> CreateFontFromFileTTF(str8 filename, u32 height);
	
	//returns index and pointer to the created `Font` object from a TTF or BDF file named `filename` with `height` in pixels from the `data/fonts` folder
	//    loading a BDF font ignores the height variable
	pair<u32,Font*> CreateFontFromFile(str8 filename, u32 height);
	
	//deletes the `Font` object at the `font` pointer if it exists in `Storage`
	//TODO implementation
	void            DeleteFont(Font* font);
	
	//returns a pointer to the `Font` object for `null_font` which is created when `Init()` is called
	inline Font*    NullFont(){ return DeshStorage->null_font; };
	
	//returns the number of `Font` objects in `Storage`
	inline u32      FontCount(){ return DeshStorage->fonts.count; };
	
	//returns a pointer to the `Font` object at `index` in `Storage`
	inline Font*    FontAt(u32 index){ return DeshStorage->fonts[index]; };
	
	//returns the index of the `Font` object at the `font` pointer in `Storage` if it exists; -1 if not found
	inline u32      FontIndex(Font* font){ forI(DeshStorage->fonts.count){ if(font == DeshStorage->fonts[i]) return i; } return -1; };
	
};

#endif //DESHI_STORAGE_H