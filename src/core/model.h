#pragma once
#ifndef DESHI_MODEL_H
#define DESHI_MODEL_H

#include "armature.h"
#include "../defines.h"
#include "../math/VectorMatrix.h"

enum TextureType_{ 
	TextureType_Albedo,   //albedo, color, diffuse
	TextureType_Normal,   //normal, bump
	TextureType_Specular, //specular, metallic, roughness
	TextureType_Light,    //light, ambient
	TextureType_COUNT
}; typedef u32 TextureType;
global_ const char* TextureTypeStrings[] = {
	"Albedo", "Normal", "Specular", "Light"
};

enum Shader_{ 
	Shader_NULL,
	Shader_Flat,
	Shader_Phong,
	Shader_PBR,
	Shader_Wireframe,
	Shader_Lavalamp,
	Shader_Testing0,
	Shader_Testing1,
}; typedef u32 Shader;
global_ const char* ShaderStrings[] = {
	"NULL", "Flat", "Phong", "PBR", "Wireframe", "Lavalamp", "Testing0", "Testing1"
};

enum MaterialFlags_{
	MaterialFlags_NONE,
}; typedef u32 MaterialFlags;

enum ModelFlags_{
	ModelFlags_NONE,
}; typedef u32 ModelFlags;

//NOTE a mesh is supposed to be 'fixed' in that no element should change post-load
struct Face;
struct Mesh{
	u64 bytes;
	u32 checksum;
	char name[DESHI_NAME_SIZE];
	
	vec3 aabbMin;
	vec3 aabbMax;
	vec3 center;
	
	typedef u32 Index;
	u32    indexCount;
	Index* indexArray;
	
	struct Vertex{ //44bytes
		vec3 pos;
		vec2 uv;
		vec3 color;
		vec3 normal;
	}  *vertexArray;
	//struct VertexEx{
	//vec3 tangent;
	//vec3 bitangent;
	//u32 boneIndexes[4];
	//f32 boneWeights[4];
	//}  *vertexExArray;
	u32 vertexCount;
	
	struct Triangle{
		vec3 normal;
		
		union{  
			Vertex* v[3]; 
			struct{
				Vertex* v0;
				Vertex* v1;
				Vertex* v2;
			};
		};
		
		//NOTE neighbor info to be cleaned
		u32 neighborCount;
		Triangle** neighborArray;
		u8*        edgeArray;
		Face* face;
		bool removed;
		bool checked;
	}  *triangleArray;
	u32 triangleCount;
	
	struct Face{
		vec3 normal;
		
		u32 triangleCount;
		u32 vertexCount;
		u32 outerVertexCount;
		Triangle** triangleArray;
		Vertex**   vertexArray;
		Vertex**   outerVertexArray;
		
		//NOTE neighbor info to be cleaned
		u32 neighborFaceCount;
		u32 neighborTriangleCount;
		Face**     neighborFaceArray;
		Triangle** neighborTriangleArray;
	}  *faceArray;
	u32 faceCount;
};

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
	Shader        shader;
	MaterialFlags flags;
};

struct Model{
	u32 checksum;
	char name[DESHI_NAME_SIZE];
	ModelFlags flags;
	Mesh*      mesh;
	Armature*  armature;
	
	struct Batch{
		char name[DESHI_NAME_SIZE];
		u32 indexOffset;
		u32 indexCount;
		Material* material;
	}  *batchArray;
	u32 batchCount;
};

#endif //DESHI_MODEL_H
