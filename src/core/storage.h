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
	void Init();
	void Reset();
	
	void StorageBrowserUI();
	
	///////////////
	//// @mesh ////
	///////////////
	pair<u32,Mesh*> CreateBoxMesh(f32 width, f32 height, f32 depth, color color = Color_White);
	pair<u32,Mesh*> CreateMeshFromFile(const char* filename);
	pair<u32,Mesh*> CreateMeshFromMemory(void* data);
	void            SaveMesh(Mesh* mesh);
	void            DeleteMesh(Mesh* mesh);
	
	inline Mesh*    NullMesh(){ return DeshStorage->null_mesh; };
	inline u32      MeshCount(){ return DeshStorage->meshes.size(); };
	inline Mesh*    MeshAt(u32 meshIdx){ return DeshStorage->meshes[meshIdx]; };
	inline u32      MeshIndex(Mesh* mesh){ forI(DeshStorage->meshes.size()){ if(mesh == DeshStorage->meshes[i]) return i; } return -1; };
	inline char*    MeshName(u32 meshIdx){ return DeshStorage->meshes[meshIdx]->name; };
	inline void     DeleteMesh(u32 meshIdx){ DeleteMesh(DeshStorage->meshes[meshIdx]); };
	
	//////////////////
	//// @texture ////
	//////////////////
	pair<u32,Texture*> CreateTextureFromFile(const char* filename, ImageFormat format = ImageFormat_RGBA, TextureType type = TextureType_2D, TextureFilter filter = TextureFilter_Nearest, TextureAddressMode uvMode = TextureAddressMode_Repeat, b32 keepLoaded = false, b32 generateMipmaps = true);
	pair<u32,Texture*> CreateTextureFromMemory(void* data, const char* name, s32 width, s32 height, ImageFormat format, TextureType type = TextureType_2D, TextureFilter filter = TextureFilter_Nearest, TextureAddressMode uvMode = TextureAddressMode_Repeat, b32 keepLoaded = false, b32 generateMipmaps = true);
	void               DeleteTexture(Texture* texture);
	
	inline Texture*    NullTexture(){ return DeshStorage->null_texture; };
	inline u32         TextureCount(){ return DeshStorage->textures.size(); };
	inline Texture*    TextureAt(u32 textureIdx){ return DeshStorage->textures[textureIdx]; };
	inline u32         TextureIndex(Texture* texture){ forI(DeshStorage->textures.size()){ if(texture == DeshStorage->textures[i]) return i; } return -1; };
	inline char*       TextureName(u32 textureIdx){ return DeshStorage->textures[textureIdx]->name; };
	inline void        DeleteTexture(u32 textureIdx){ DeleteTexture(DeshStorage->textures[textureIdx]); };
	
	///////////////////
	//// @material ////
	///////////////////
	pair<u32,Material*> CreateMaterial(const char* name, Shader shader = Shader_PBR, MaterialFlags flags = MaterialFlags_NONE, array<u32> textures = {});
	pair<u32,Material*> CreateMaterialFromFile(const char* name, b32 warnMissing = true);
	void                SaveMaterial(Material* material);
	void                DeleteMaterial(Material* material);
	
	inline Material*    NullMaterial(){ return DeshStorage->null_material; };
	inline u32          MaterialCount(){ return DeshStorage->materials.size(); };
	inline Material*    MaterialAt(u32 materialIdx){ return DeshStorage->materials[materialIdx]; };
	inline u32          MaterialIndex(Material* material){ forI(DeshStorage->materials.size()){ if(material == DeshStorage->materials[i]) return i; } return -1; };
	inline char*        MaterialName(u32 materialIdx){ return DeshStorage->materials[materialIdx]->name; };
	inline Shader       MaterialShader(u32 materialIdx){ return DeshStorage->materials[materialIdx]->shader; };
	inline u32          MaterialTextureCount(u32 materialIdx){ return DeshStorage->materials[materialIdx]->textures.size(); };
	inline Texture*     MaterialTexture(u32 materialIdx, u32 textureIdx){ return DeshStorage->textures[DeshStorage->materials[materialIdx]->textures[textureIdx]]; };
	inline void         DeleteMaterial(u32 materialIdx){ DeleteMaterial(DeshStorage->materials[materialIdx]); };
	
	////////////////
	//// @model ////
	////////////////
	pair<u32,Model*> CreateModelFromFile(const char* filename, ModelFlags flags = ModelFlags_NONE, b32 forceLoadOBJ = false);
	pair<u32,Model*> CreateModelFromMesh(Mesh* mesh, ModelFlags flags = ModelFlags_NONE);
	pair<u32,Model*> CopyModel(Model* base);
	void             SaveModel(Model* model);
	void             DeleteModel(Model* model);
	
	inline Model*    NullModel(){ return DeshStorage->null_model; };
	inline u32       ModelCount(){ return DeshStorage->models.size(); };
	inline Model*    ModelAt(u32 modelIdx){ return DeshStorage->models[modelIdx]; };
	inline u32       ModelIndex(Model* model){ forI(DeshStorage->models.size()){ if(model == DeshStorage->models[i]) return i; } return -1; };
	inline char*     ModelName(u32 modelIdx){ return DeshStorage->models[modelIdx]->name; };
	inline u32       ModelBatchCount(u32 modelIdx){ return DeshStorage->models[modelIdx]->batches.size(); };
	inline void      DeleteModel(u32 modelIdx){ DeleteModel(DeshStorage->models[modelIdx]); };
	
	///////////////
	//// @font ////
	///////////////
	//returns index and pointer to the created `Font` object from a BDF file named `filename` with `height` in pixels from the `data/fonts` folder
	pair<u32,Font*> CreateFontFromFileBDF(const char* filename);
	FORCE_INLINE pair<u32,Font*> CreateFontFromFileBDF(cstring filename){ return CreateFontFromFileBDF(filename.str); }
	FORCE_INLINE pair<u32,Font*> CreateFontFromFileBDF(const string& filename){ return CreateFontFromFileBDF(filename.str); }
	
	//returns index and pointer to the created `Font` object from a TTF file named `filename` with `height` in pixels from the `data/fonts` folder
	pair<u32,Font*> CreateFontFromFileTTF(const char* filename, u32 height);
	FORCE_INLINE pair<u32,Font*> CreateFontFromFileTTF(cstring filename, u32 height){ return CreateFontFromFileTTF(filename.str, height); }
	FORCE_INLINE pair<u32,Font*> CreateFontFromFileTTF(const string& filename, u32 height){ return CreateFontFromFileTTF(filename.str, height); }
	
	//returns index and pointer to the created `Font` object from a TTF or BDF file named `filename` with `height` in pixels from the `data/fonts` folder
	//NOTE loading a BDF font ignores the height variable
	pair<u32,Font*> CreateFontFromFile(const char* filename, u32 height);
	FORCE_INLINE pair<u32,Font*> CreateFontFromFile(cstring filename, u32 height){ return CreateFontFromFile(filename.str, height); }
	FORCE_INLINE pair<u32,Font*> CreateFontFromFile(const string& filename, u32 height){ return CreateFontFromFile(filename.str, height); }
	
	//TODO implementation and description 
	void            DeleteFont(Font* font);
	
	//returns a pointer to the `Font` object for `null_font` which is created when `Init()` is called
	inline Font*    NullFont(){ return DeshStorage->null_font; };
	
	//returns the number of loaded `Font` objects in `Storage`
	inline u32      FontCount(){ return DeshStorage->fonts.count; };
	
	//returns a pointer to the loaded `Font` object at `index` in `Storage`
	inline Font*    FontAt(u32 index){ return DeshStorage->fonts[index]; };
	
	//returns the index of the loaded `Font` object at the `font` pointer in `Storage` if it exists; -1 if not
	inline u32      FontIndex(Font* font){ forI(DeshStorage->fonts.count){ if(font == DeshStorage->fonts[i]) return i; } return -1; };
	
	//returns a `c-string` pointer name of the loaded `Font` object at `index` in `Storage`
	//TODO change the return type to cstring
	inline char*    FontName(u32 index){ return DeshStorage->fonts[index]->name; };
};

#endif //DESHI_STORAGE_H