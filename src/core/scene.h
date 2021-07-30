#pragma once
#ifndef DESHI_SCENE_H
#define DESHI_SCENE_H

#include "model.h"
#include "../utils/color.h"
#include "../utils/array.h"
#include "../utils/tuple.h"

struct Light;

struct Scene{
	array<Mesh*>     meshes;
	array<Texture*>  textures;
	array<Material*> materials;
	array<Model*>    models;
	array<Light*>    lights;
	
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
	
	inline Mesh*    NullMesh(){ return meshes[0]; };
	inline u32      MeshCount(){ return meshes.size(); };
	inline Mesh*    MeshAt(u32 meshIdx){ return meshes[meshIdx]; };
	inline u32      MeshIndex(Mesh* mesh){ forI(meshes.size()){ if(mesh == meshes[i]) return i; } return -1; };
	inline char*    MeshName(u32 meshIdx){ return meshes[meshIdx]->name; };
	inline void     DeleteMesh(u32 meshIdx){ DeleteMesh(meshes[meshIdx]); };
	
	//////////////////
	//// @texture ////
	//////////////////
	pair<u32,Texture*> CreateTextureFromFile(const char* filename, TextureFlags flags = TextureFlags_Default, bool keepLoaded = false, bool generateMipmaps = true);
	pair<u32,Texture*> CreateTextureFromMemory(void* data, ImageLayout layout, TextureFlags flags = TextureFlags_Default, bool keepLoaded = false, bool generateMipmaps = true);
	void               DeleteTexture(Texture* texture);
	
	inline Texture*    NullTexture(){ return textures[0]; };
	inline u32         TextureCount(){ return textures.size(); };
	inline Texture*    TextureAt(u32 textureIdx){ return textures[textureIdx]; };
	inline u32         TextureIndex(Texture* texture){ forI(textures.size()){ if(texture == textures[i]) return i; } return -1; };
	inline char*       TextureName(u32 textureIdx){ return textures[textureIdx]->name; };
	inline void        DeleteTexture(u32 textureIdx){ DeleteTexture(textures[textureIdx]); };
	
	///////////////////
	//// @material ////
	///////////////////
	pair<u32,Material*> CreateMaterial(const char* name, Shader shader = Shader_PBR, MaterialFlags flags = MaterialFlags_NONE, std::vector<u32> textures = {});
	void                DeleteMaterial(Material* material);
	
	inline Material*    NullMaterial(){ return materials[0]; };
	inline u32          MaterialCount(){ return materials.size(); };
	inline Material*    MaterialAt(u32 materialIdx){ return materials[materialIdx]; };
	inline u32          MaterialIndex(Material* material){ forI(materials.size()){ if(material == materials[i]) return i; } return -1; };
	inline char*        MaterialName(u32 materialIdx){ return materials[materialIdx]->name; };
	inline Shader       MaterialShader(u32 materialIdx){ return materials[materialIdx]->shader; };
	inline u32          MaterialTextureCount(u32 materialIdx){ return materials[materialIdx]->textureCount; };
	inline Texture*     MaterialTexture(u32 materialIdx, u32 textureIdx){ return textures[materials[materialIdx]->textureArray[textureIdx]]; };
	inline void         DeleteMaterial(u32 materialIdx){ DeleteMaterial(materials[materialIdx]); };
	
	////////////////
	//// @model ////
	////////////////
	pair<u32,Model*> CreateModelFromOBJ(const char* filename, Shader shader = Shader_PBR, Color color = Color::WHITE);
	pair<u32,Model*> CreateModelFromMesh(Mesh* mesh, Shader shader = Shader_PBR, Color color = Color::WHITE);
	pair<u32,Model*> CopyModel(Model* base);
	void             DeleteModel(Model* model);
	
	inline Model*    NullModel(){ return models[0]; };
	inline u32       ModelCount(){ return models.size(); };
	inline Model*    ModelAt(u32 modelIdx){ return models[modelIdx]; };
	inline u32       ModelIndex(Model* model){ forI(models.size()){ if(model == models[i]) return i; } return -1; };
	inline char*     ModelName(u32 modelIdx){ return models[modelIdx]->name; };
	inline u32       ModelBatchCount(u32 modelIdx){ return models[modelIdx]->batches.size(); };
	inline void      DeleteModel(u32 modelIdx){ DeleteModel(models[modelIdx]); };
};

//global scene pointer
extern Scene* g_scene;
#define DengScene g_scene

#endif //DESHI_SCENE_H