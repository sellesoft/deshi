#pragma once
#ifndef DESHI_MODEL_H
#define DESHI_MODEL_H

#include "armature.h"
#include "../defines.h"
#include "../math/VectorMatrix.h"
#include "../utils/array.h"

enum ImageFormat_{
	ImageFormat_BW,
	ImageFormat_BWA,
	ImageFormat_RGB,
	ImageFormat_RGBA,
}; typedef u32 ImageFormat;

enum TextureType_{
	TextureType_1D,
	TextureType_2D,
	TextureType_3D,
	TextureType_Cube,
	TextureType_Array_1D,
	TextureType_Array_2D,
	TextureType_Array_Cube,
	TextureType_COUNT
}; typedef u32 TextureType;
global_ const char* TextureTypeStrings[] = {
	"1D", "2D", "3D", "Cube", "1D Array", "2D Array", "Cube Array",
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
	Shader_COUNT,
}; typedef u32 Shader;
global_ const char* ShaderStrings[] = {
	"NULL", "Flat", "Phong", "PBR", "Wireframe", "Lavalamp", "Testing0", "Testing1"
};

enum MaterialFlags_{
	MaterialFlags_NONE = 0,
}; typedef u32 MaterialFlags;

enum ModelFlags_{
	ModelFlags_NONE = 0,
}; typedef u32 ModelFlags;

//NOTE a mesh is supposed to be 'fixed' in that no element should change post-load
struct Mesh{
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
	char name[DESHI_NAME_SIZE];
	int  width;
	int  height;
	int  depth;
	int  mipmaps;
	u8*  pixels;
	bool loaded;
	ImageFormat format;
	TextureType type;
};

struct Material{
	char name[DESHI_NAME_SIZE];
	Shader shader;
	MaterialFlags flags;
	u32* textureArray;
	u32  textureCount;
	array_view<u32> textures;
};

struct Model{
	char name[DESHI_NAME_SIZE];
	ModelFlags flags;
	Mesh*      mesh;
	Armature*  armature;
	
	struct Batch{
		u32  indexOffset;
		u32  indexCount;
		u32  material;
	};
	array<Batch> batches;
};

#endif //DESHI_MODEL_H
