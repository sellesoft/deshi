#pragma once
#ifndef DESHI_MODEL_H
#define DESHI_MODEL_H

#include "texture.h"
#include "kigu/array.h"
#include "kigu/common.h"
#include "math/math.h"

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
global_ str8 ShaderStrings[] = {
	str8_lit("NULL"), str8_lit("Flat"), str8_lit("Phong"), str8_lit("PBR"), str8_lit("Wireframe"), str8_lit("Lavalamp"), str8_lit("Testing0"), str8_lit("Testing1")
};

enum MaterialFlags_{
	MaterialFlags_NONE = 0,
}; typedef u32 MaterialFlags;

enum ModelFlags_{
	ModelFlags_NONE = 0,
}; typedef u32 ModelFlags;

//NOTE a mesh is supposed to be 'fixed' in that no element should change post-load
struct Mesh{
	u32  bytes;
	char name[64];
	u32  idx;
	vec3 aabbMin;
	vec3 aabbMax;
	vec3 center;
	
	u32 vertexCount;
	u32 indexCount;
	u32 triangleCount;
	u32 faceCount;
	u32 totalTriNeighborCount;
	u32 totalFaceVertexCount;
	u32 totalFaceOuterVertexCount;
	u32 totalFaceTriNeighborCount;
	u32 totalFaceFaceNeighborCount;
	
	struct Vertex{ //36 bytes
		vec3 pos;
		vec2 uv;
		u32  color;
		vec3 normal;
	} *vertexArray;
	carray<Vertex> vertexes;
	//struct VertexEx{
	//vec3 tangent;
	//vec3 bitangent;
	//u32 boneIndexes[4];
	//f32 boneWeights[4];
	//}  *vertexExArray;
	
	typedef u32 Index;
	Index* indexArray;
	carray<u32> indexes;
	
	struct Triangle{
		vec3 normal;
		vec3 p[3];
		u32  v[3];
		u32  neighborCount;
		u32  face;
		b32 removed;
		b32 checked;
		
		u32* neighborArray;
		u8*  edgeArray;
		carray<u32> neighbors;
		carray<u8>  edges;
	} *triangleArray;
	carray<Triangle> triangles;
	
	struct Face{
		vec3 normal;
		vec3 center;
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
		carray<u32> triangles;
		carray<u32> vertexes;
		carray<u32> outerVertexes;
		carray<u32> triangleNeighbors;
		carray<u32> faceNeighbors;
	} *faceArray;
	carray<Face> faces;
};
typedef Mesh::Vertex   MeshVertex;
typedef Mesh::Index    MeshIndex;
typedef Mesh::Triangle MeshTriangle;
typedef Mesh::Face     MeshFace;

struct Material{
	MaterialFlags flags;
	char name[64];
	u32  idx;
	Shader     shader;
	array<u32> textures;
};

struct Armature;
struct Model{
	ModelFlags flags;
	char name[64];
	u32  idx;
	Mesh*      mesh;
	Armature*  armature;
	
	struct Batch{
		u32  indexOffset;
		u32  indexCount;
		u32  material;
	};
	array<Batch> batches;
};
typedef Model::Batch ModelBatch;

#endif //DESHI_MODEL_H
