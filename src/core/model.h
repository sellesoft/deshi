#pragma once
#ifndef DESHI_MODEL_H
#define DESHI_MODEL_H

#include "../defines.h"
#include "../math/VectorMatrix.h"

enum MeshFlags_{
	
}; typedef u32 MeshFlags;

struct Mesh{
	u32 checksum;
	
	
	char name[DESHI_NAME_SIZE];
	
	vec3 aabbMin;
	vec3 aabbMax;
	vec3 center;
	
	typedef u32 Index;
	u32    indexCount;
	Index* indexArray;
	
	struct Vertex{
		vec3 pos;
		vec2 uv;
		vec3 color;
		vec3 normal;
		vec3 tangent;
		vec3 bitangent;
		//u32 boneIndexes[4];
		//f32 boneWeights[4];
	}  *vertexArray;
	u32 vertexCount;
	
	struct Triangle{
		Vertex* vertex0;
		Vertex* vertex1;
		Vertex* vertex2;
		vec3    normal;
	}  *triangleArray;
	u32 triangleCount;
	
	struct Face{
		u32 triangleCount;
		u32 vertexCount;
		Triangle** triangleArray;
		Vertex**   vertexArray;
	}  *faceArray;
	u32 faceCount;
};

#endif //DESHI_MODEL_H
