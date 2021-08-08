#pragma once
#ifndef DESHI_STORAGE_H
#define DESHI_STORAGE_H

#include "model.h"
#include "../utils/color.h"
#include "../utils/array.h"
#include "../utils/tuple.h"

struct Light;

struct Storage_{
	array<Mesh*>     meshes;
	array<Texture*>  textures;
	array<Material*> materials;
	array<Model*>    models;
	array<Light*>    lights;
};

//global storage pointer
extern Storage_* g_storage;
#define DeshStorage g_storage

namespace Storage{
	void Init();
	void Reset();
	
	///////////////
	//// @mesh ////
	///////////////
	pair<u32,Mesh*> CreateBoxMesh(f32 width, f32 height, f32 depth, Color color = Color::WHITE);
	pair<u32,Mesh*> CreateMeshFromFile(const char* filename);
	pair<u32,Mesh*> CreateMeshFromMemory(void* data);
	void            SaveMesh(Mesh* mesh);
	void            DeleteMesh(Mesh* mesh);
	
	array<vec2> GenerateMeshOutlinePoints(Mesh* mesh, mat4 transform, mat4 camProjection, mat4 camView, vec3 camPosition, vec2 screenDims);
	
	inline Mesh*    NullMesh(){ return DeshStorage->meshes.data[0]; };
	inline u32      MeshCount(){ return DeshStorage->meshes.size(); };
	inline Mesh*    MeshAt(u32 meshIdx){ return DeshStorage->meshes[meshIdx]; };
	inline u32      MeshIndex(Mesh* mesh){ forI(DeshStorage->meshes.size()){ if(mesh == DeshStorage->meshes[i]) return i; } return -1; };
	inline char*    MeshName(u32 meshIdx){ return DeshStorage->meshes[meshIdx]->name; };
	inline void     DeleteMesh(u32 meshIdx){ DeleteMesh(DeshStorage->meshes[meshIdx]); };
	
	//////////////////
	//// @texture ////
	//////////////////
	pair<u32,Texture*> CreateTextureFromFile(const char* filename, ImageFormat format = ImageFormat_RGBA, TextureType type = TextureType_2D, bool keepLoaded = false, bool generateMipmaps = true);
	pair<u32,Texture*> CreateTextureFromMemory(void* data, const char* name, int width, int height, ImageFormat format, TextureType type = TextureType_2D, bool keepLoaded = false, bool generateMipmaps = true);
	void               DeleteTexture(Texture* texture);
	
	inline Texture*    NullTexture(){ return DeshStorage->textures.data[0]; };
	inline u32         TextureCount(){ return DeshStorage->textures.size(); };
	inline Texture*    TextureAt(u32 textureIdx){ return DeshStorage->textures[textureIdx]; };
	inline u32         TextureIndex(Texture* texture){ forI(DeshStorage->textures.size()){ if(texture == DeshStorage->textures[i]) return i; } return -1; };
	inline char*       TextureName(u32 textureIdx){ return DeshStorage->textures[textureIdx]->name; };
	inline void        DeleteTexture(u32 textureIdx){ DeleteTexture(DeshStorage->textures[textureIdx]); };
	
	///////////////////
	//// @material ////
	///////////////////
	pair<u32,Material*> CreateMaterial(const char* name, Shader shader = Shader_PBR, MaterialFlags flags = MaterialFlags_NONE, array<u32> textures = {});
	pair<u32,Material*> CreateMaterialFromFile(const char* name, bool warnMissing = true);
	void                SaveMaterial(Material* material);
	void                DeleteMaterial(Material* material);
	
	inline Material*    NullMaterial(){ return DeshStorage->materials.data[0]; };
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
	pair<u32,Model*> CreateModelFromFile(const char* filename, ModelFlags flags = ModelFlags_NONE, bool forceLoadOBJ = false);
	pair<u32,Model*> CreateModelFromMesh(Mesh* mesh, ModelFlags flags = ModelFlags_NONE);
	pair<u32,Model*> CopyModel(Model* base);
	void             SaveModel(Model* model);
	void             DeleteModel(Model* model);
	
	inline Model*    NullModel(){ return DeshStorage->models.data[0]; };
	inline u32       ModelCount(){ return DeshStorage->models.size(); };
	inline Model*    ModelAt(u32 modelIdx){ return DeshStorage->models[modelIdx]; };
	inline u32       ModelIndex(Model* model){ forI(DeshStorage->models.size()){ if(model == DeshStorage->models[i]) return i; } return -1; };
	inline char*     ModelName(u32 modelIdx){ return DeshStorage->models[modelIdx]->name; };
	inline u32       ModelBatchCount(u32 modelIdx){ return DeshStorage->models[modelIdx]->batches.size(); };
	inline void      DeleteModel(u32 modelIdx){ DeleteModel(DeshStorage->models[modelIdx]); };
};

#endif //DESHI_STORAGE_H