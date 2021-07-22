#pragma once
#ifndef DESHI_SCENE_H
#define DESHI_SCENE_H

#include "model.h"
#include "../utils/color.h"

#include <vector>

struct Light;

struct Scene{
	std::vector<Texture*>  textures;
	std::vector<Material*> materials;
	std::vector<Mesh*>     meshes;
	std::vector<Model*>    models;
	std::vector<Light*>    lights;
	
	Mesh* NullMesh();
	Mesh* CreateBoxMesh(f32 width, f32 height, f32 depth, Color color = Color::WHITE);
	void DeleteMesh(Mesh* mesh);
	
	Texture* NullTexture();
	Texture* CreateTexture(const char* filename, TextureType type = TextureType_Albedo);
	
	Material* NullMaterial();
	Material* CreateMaterial(const char* name, Shader shader = Shader_Flat, MaterialFlags flags = MaterialFlags_NONE, std::vector<Texture*> textures = {});
	
	Model* NullModel();
	Model* CreateModelFromOBJ(const char* filename, Shader shader = Shader_Flat, Color color = Color::WHITE, bool planarize = false);
	Model* CreateModelFromMesh(Mesh* mesh, Shader shader = Shader_Flat, Color color = Color::WHITE);
	void DeleteModel(Model* model);
	void UpdateModelBatchMaterial(Model* model, u32 batchIdx, Material* material);
	
	void Init();
	void Reset();
};

//global scene pointer
extern Scene* g_scene;
#define DengScene g_scene

#endif //DESHI_SCENE_H