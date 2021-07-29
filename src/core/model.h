#pragma once
#ifndef DESHI_MODEL_H
#define DESHI_MODEL_H

#include "armature.h"
#include "../defines.h"
#include "../math/VectorMatrix.h"
#include "../utils/array.h"

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
struct Mesh{
	u32 bytes;
	u32 checksum;
	char name[DESHI_NAME_SIZE];
	
	vec3 aabbMin;
	vec3 aabbMax;
	vec3 center;
	
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
	array_view<Vertex> vertexes;
	
	typedef u32 Index;
	u32    indexCount;
	Index* indexArray;
	array_view<u32> indexes;
	
	struct Triangle{
		vec3 normal;
		vec3 p[3];
		u32  v[3];
		u32  neighborCount;
		u32  face;
		bool removed;
		bool checked;
		
		u32* neighborArray;
		u8*  edgeArray;
		array_view<u32> neighbors;
		array_view<u8>  edges;
	}  *triangleArray;
	u32 triangleCount;
	array_view<Triangle> triangles;
	
	struct Face{
		vec3 normal;
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
		array_view<u32> triangles;
		array_view<u32> vertexes;
		array_view<u32> outerVertexes;
		array_view<u32> triangleNeighbors;
		array_view<u32> faceNeighbors;
	}  *faceArray;
	u32 faceCount;
	array_view<Face> faces;
};

struct Texture{
	u32  checksum;
	char name[DESHI_NAME_SIZE];
	int  width;
	int  height;
	int  depth;
	u32  mipmaps;
	TextureType type;
};

struct Material{
	u32  checksum;
	char name[DESHI_NAME_SIZE];
	Shader        shader;
	MaterialFlags flags;
	u32  textureCount;
	u32* textureArray;
	array_view<u32> textures;
};

struct Model{
	u32  checksum;
	char name[DESHI_NAME_SIZE];
	ModelFlags flags;
	Mesh*      mesh;
	Armature*  armature;
	
	struct Batch{
		char name[DESHI_NAME_SIZE];
		u32  indexOffset;
		u32  indexCount;
		u32  material;
	};
	array<Batch> batches;
};

#endif //DESHI_MODEL_H
