#pragma once
#ifndef DESHI_MODEL_H
#define DESHI_MODEL_H

#include "../defines.h"
#include "../math/math.h"
#include "../utils/array.h"
#include "../utils/view.h"

enum ImageFormat_{ //NOTE value = bytes per pixel
	ImageFormat_BW   = 1,
	ImageFormat_BWA  = 2,
	ImageFormat_RGB  = 3,
	ImageFormat_RGBA = 4,
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
	u32  bytes;
	char name[DESHI_NAME_SIZE];
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
	view<Vertex> vertexes;
	//struct VertexEx{
	//vec3 tangent;
	//vec3 bitangent;
	//u32 boneIndexes[4];
	//f32 boneWeights[4];
	//}  *vertexExArray;
	
	typedef u32 Index;
	Index* indexArray;
	view<u32> indexes;
	
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
		view<u32> neighbors;
		view<u8>  edges;
	} *triangleArray;
	view<Triangle> triangles;
	
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
		view<u32> triangles;
		view<u32> vertexes;
		view<u32> outerVertexes;
		view<u32> triangleNeighbors;
		view<u32> faceNeighbors;
	} *faceArray;
	view<Face> faces;
};
typedef Mesh::Vertex   MeshVertex;
typedef Mesh::Index    MeshIndex;
typedef Mesh::Triangle MeshTriangle;
typedef Mesh::Face     MeshFace;

struct Texture{
	char name[DESHI_NAME_SIZE];
	u32  idx;
	int  width;
	int  height;
	int  depth;
	int  mipmaps;
	u8*  pixels;
	b32  loaded;
	b32  forceLinear;
	ImageFormat format;
	TextureType type;
};

struct Material{
	MaterialFlags flags;
	char name[DESHI_NAME_SIZE];
	u32  idx;
	Shader        shader;
	array<u32>    textures;
};

struct Armature;
struct Model{
	ModelFlags flags;
	char name[DESHI_NAME_SIZE];
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
