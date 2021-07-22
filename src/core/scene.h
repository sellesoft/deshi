#pragma once
#ifndef DESHI_SCENE_H
#define DESHI_SCENE_H

#include "armature.h"
#include "mesh.h"
#include "../utils/color.h"

#include <vector>

struct Light;

enum TextureTypeBits{ 
	TextureType_Albedo,   //albedo, color, diffuse
	TextureType_Normal,   //normal, bump
	TextureType_Specular, //specular, metallic, roughness
	TextureType_Light,    //light, ambient
	TextureType_COUNT
}; typedef u32 TextureType;

enum ShaderBits{ 
	Shader_Flat, 
	Shader_Phong, 
	Shader_PBR, 
	Shader_Wireframe, 
	Shader_Lavalamp, 
	Shader_Testing0, 
	Shader_Testing1,
}; typedef u32 Shader;
global_ const char* ShaderStrings[] = {
	"Flat", "Phong", "PBR", "Wireframe", "Lavalamp", "Testing0", "Testing1"
};

enum MaterialFlags_{
	MaterialFlags_NONE,
}; typedef u32 MaterialFlags;

struct Texture{
	u32 checksum;
	char name[DESHI_NAME_SIZE];
	int width;
	int height;
	int depth;
	u32 mipmaps;
	TextureType type;
};

struct Material{
	u32 checksum;
	char name[DESHI_NAME_SIZE];
	u32       textureCount;
	Texture** textureArray;
	Shader      shader;
	MaterialFlags flags;
};

struct Model{
	char name[DESHI_NAME_SIZE];
	Mesh*     mesh;
	Armature* armature;
	
	struct Batch{
		char name[DESHI_NAME_SIZE];
		u32 indexOffset;
		u32 indexCount;
		Material* material;
	}  *batchArray;
	u32 batchCount;
};

struct Scene{
	std::vector<Texture*>  textures;
	std::vector<Material*> materials;
	std::vector<Mesh*>     meshes;
	std::vector<Model*>    models;
	std::vector<Light*>    lights;
	
	Mesh* CreateMeshFromOBJ(const char* filename, Shader shader = Shader_Flat, Color color = Color::WHITE, bool planarize = true);
	Mesh* CreateBox(f32 width, f32 height, f32 depth, Shader shader = Shader_Flat, Color color = Color::WHITE, bool planarize = true);
	void  DeleteMesh(Mesh* mesh);
	
	void Init();
	void Reset();
};

//global scene pointer
extern Scene* g_scene;
#define DengScene g_scene

#endif //DESHI_SCENE_H