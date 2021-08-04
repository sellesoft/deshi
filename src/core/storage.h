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
#define DengStorage g_storage

namespace Storage{
	void Init();
	void Reset();
	
	///////////////
	//// @mesh ////
	///////////////
	pair<u32,Mesh*> CreateBoxMesh(f32 width, f32 height, f32 depth, Color color = Color::WHITE);
	pair<u32,Mesh*> CreateMeshFromFile(const char* filename);
	pair<u32,Mesh*> CreateMeshFromMemory(void* data);
	void            DeleteMesh(Mesh* mesh);
	
	std::vector<Vector2> GenerateMeshOutlinePoints(Mesh* mesh, Matrix4 transform, Matrix4 camProjection, Matrix4 camView, Vector3 camPosition, Vector2 screenDims);
	
	inline Mesh*    NullMesh(){ return *DengStorage->meshes.items; };
	inline u32      MeshCount(){ return DengStorage->meshes.size(); };
	inline Mesh*    MeshAt(u32 meshIdx){ return DengStorage->meshes[meshIdx]; };
	inline u32      MeshIndex(Mesh* mesh){ forI(DengStorage->meshes.size()){ if(mesh == DengStorage->meshes[i]) return i; } return -1; };
	inline char*    MeshName(u32 meshIdx){ return DengStorage->meshes[meshIdx]->name; };
	inline void     DeleteMesh(u32 meshIdx){ DeleteMesh(DengStorage->meshes[meshIdx]); };
	
	//////////////////
	//// @texture ////
	//////////////////
	pair<u32,Texture*> CreateTextureFromFile(const char* filename, ImageFormat format = ImageFormat_RGBA, TextureType type = TextureType_2D, bool keepLoaded = false, bool generateMipmaps = true);
	pair<u32,Texture*> CreateTextureFromMemory(void* data, int width, int height, ImageFormat format, TextureType type = TextureType_2D, bool keepLoaded = false, bool generateMipmaps = true);
	void               DeleteTexture(Texture* texture);
	
	inline Texture*    NullTexture(){ return *DengStorage->textures.items; };
	inline u32         TextureCount(){ return DengStorage->textures.size(); };
	inline Texture*    TextureAt(u32 textureIdx){ return DengStorage->textures[textureIdx]; };
	inline u32         TextureIndex(Texture* texture){ forI(DengStorage->textures.size()){ if(texture == DengStorage->textures[i]) return i; } return -1; };
	inline char*       TextureName(u32 textureIdx){ return DengStorage->textures[textureIdx]->name; };
	inline void        DeleteTexture(u32 textureIdx){ DeleteTexture(DengStorage->textures[textureIdx]); };
	
	///////////////////
	//// @material ////
	///////////////////
	pair<u32,Material*> CreateMaterial(const char* name, Shader shader = Shader_PBR, MaterialFlags flags = MaterialFlags_NONE, array<u32> textures = {});
	void                DeleteMaterial(Material* material);
	
	inline Material*    NullMaterial(){ return *DengStorage->materials.items; };
	inline u32          MaterialCount(){ return DengStorage->materials.size(); };
	inline Material*    MaterialAt(u32 materialIdx){ return DengStorage->materials[materialIdx]; };
	inline u32          MaterialIndex(Material* material){ forI(DengStorage->materials.size()){ if(material == DengStorage->materials[i]) return i; } return -1; };
	inline char*        MaterialName(u32 materialIdx){ return DengStorage->materials[materialIdx]->name; };
	inline Shader       MaterialShader(u32 materialIdx){ return DengStorage->materials[materialIdx]->shader; };
	inline u32          MaterialTextureCount(u32 materialIdx){ return DengStorage->materials[materialIdx]->textures.size(); };
	inline Texture*     MaterialTexture(u32 materialIdx, u32 textureIdx){ return DengStorage->textures[DengStorage->materials[materialIdx]->textures[textureIdx]]; };
	inline void         DeleteMaterial(u32 materialIdx){ DeleteMaterial(DengStorage->materials[materialIdx]); };
	
	////////////////
	//// @model ////
	////////////////
	pair<u32,Model*> CreateModelFromOBJ(const char* filename, ModelFlags flags = ModelFlags_NONE, bool forceLoadOBJ = false);
	pair<u32,Model*> CreateModelFromMesh(Mesh* mesh, ModelFlags flags = ModelFlags_NONE);
	pair<u32,Model*> CopyModel(Model* base);
	void             DeleteModel(Model* model);
	
	inline Model*    NullModel(){ return *DengStorage->models.items; };
	inline u32       ModelCount(){ return DengStorage->models.size(); };
	inline Model*    ModelAt(u32 modelIdx){ return DengStorage->models[modelIdx]; };
	inline u32       ModelIndex(Model* model){ forI(DengStorage->models.size()){ if(model == DengStorage->models[i]) return i; } return -1; };
	inline char*     ModelName(u32 modelIdx){ return DengStorage->models[modelIdx]->name; };
	inline u32       ModelBatchCount(u32 modelIdx){ return DengStorage->models[modelIdx]->batches.size(); };
	inline void      DeleteModel(u32 modelIdx){ DeleteModel(DengStorage->models[modelIdx]); };
};

#endif //DESHI_STORAGE_H