#pragma once
#ifndef DESHI_MODEL_H
#define DESHI_MODEL_H

#include "Armature.h"
#include "../defines.h"
#include "../math/Matrix.h"
#include "../math/Vector.h"
#include "../utils/Color.h"

#include <vector>

enum TextureTypeBits : u32 { 
	TextureType_Albedo   = 0 << 0, //albedo, color, diffuse
	TextureType_Normal   = 1 << 0, //normal, bump
	TextureType_Specular = 1 << 1, //specular, metallic, roughness
	TextureType_Light    = 1 << 2, //light, ambient
	TextureType_Cube     = 1 << 3, //not supported yet
	TextureType_Sphere   = 1 << 4, //not supported yet
}; typedef u32 TextureType;

struct Texture {
	char filename[DESHI_NAME_SIZE];
	TextureType type;
	Texture() {}
	Texture(const char* filename, TextureType textureType = TextureType_Albedo);
};

enum ShaderFlagsBits : u32 {
	ShaderFlags_NONE = 0,
};
typedef u32 ShaderFlags;

enum ShaderBits : u32{ 
	Shader_Flat, Shader_Phong, Shader_Twod, Shader_PBR, Shader_Wireframe, Shader_Lavalamp, Shader_Testing0, Shader_Testing1
}; typedef u32 Shader;
static const char* ShaderStrings[] = {
	"Flat", "Phong", "TwoD", "PBR", "Wireframe", "Lavalamp", "Testing0", "Testing1"
};

struct Material{
	char name[DESHI_NAME_SIZE];
	u32 shader;
	u32 shaderFlags;
	std::vector<Texture> textureArray;
};

struct Vertex {
	Vector3 pos{};
	Vector2 uv;
	Vector3 color{1.f, 1.f, 1.f}; //between 0 and 1
	Vector3 normal{};
	//bone index		4tuple
	//bone weight		4tuple
	Vertex() {}
	Vertex(Vector3 pos, Vector2 uv, Vector3 color, Vector3 normal);
	
	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && uv == other.uv && normal == other.normal;
	}
};

//this is a hash function to compare vertices for a hash map
//pattern: OR unshifted and L-shifted, then R-shift the combo, 
//then OR that with L-shifted, then R-shift the combo and repeat
//until the last combo which is not R-shifted ((x^(y<<))>>)^(z<<)
namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<Vector3>()(vertex.pos) ^ (hash<Vector2>()(vertex.uv) << 1)) >> 1) ^ (hash<Vector3>()(vertex.color) << 1);
		}
	};
};

//NOTE indices should be clockwise
struct Batch {
	char name[DESHI_NAME_SIZE];
	u32 vertexCount  = 0;
	u32 indexCount   = 0;
	u32 textureCount = 0;
	std::vector<Vertex>  vertexArray;
	std::vector<u32>     indexArray;
	std::vector<Texture> textureArray;
	
	u32         shader;
	ShaderFlags shaderFlags;
	
	Batch() {}
	Batch(const char* name, std::vector<Vertex> vertexArray, std::vector<u32> indexArray, std::vector<Texture> textureArray, Shader shader = Shader_Flat, ShaderFlags shaderFlags = ShaderFlags_NONE);
	
	void SetName(const char* name);
};

struct Face;

//primarily for caching triangles neighbors
struct Triangle {
	Vector3 p[3];
	Vector3 norm;
	
	Face* face = nullptr;
	
	//parallel vectors for storing the neighbors and their respective points
	std::vector<Triangle*> nbrs;
	std::vector<u8> sharededge; //index of first point where second is the first plus one
	bool removed = false; //for checking if triangle was culled
	
	bool checked = false; //for findFaceNbrs
	
	Vector3 midpoint() {
		return Vector3(
					   (p[0].x + p[1].x + p[2].x) / 3,
					   (p[0].y + p[1].y + p[2].y) / 3,
					   (p[0].z + p[1].z + p[2].z) / 3);
	}
};

struct Face {
	std::vector<Triangle*> tris;
	std::vector<Face*> nbrs;
	std::vector<Vector3> points; //every other point is followed by the next point of its edge
	Vector3 norm;
};

struct Mesh {
	char name[DESHI_NAME_SIZE];
	
	u32 vertexCount  = 0;
	u32 indexCount   = 0;
	u32 textureCount = 0;
	
	u32 batchCount = 0;
	std::vector<Batch> batchArray;
	std::vector<Triangle*> triangles;
	std::vector<Face*> faces;
	
	Mesh() {}
	Mesh(const char* name, std::vector<Batch> batchArray);
	
	void SetName(const char* name);
	std::vector<Vector2> GenerateOutlinePoints(Matrix4 transform, Matrix4 proj, Matrix4 view, Vector2 windimen, Vector3 camPosition);
	
	//filename: filename and extension, name: loaded mesh name, transform: pos,rot,scale of mesh
	static Mesh* CreateMeshFromOBJ(std::string filename);
	static Mesh* CreateMeshFromOBJ(std::string filename, Shader shader, Color color);
	static Mesh* CreateBox(Vector3 halfDims, Color color = Color::WHITE);
	static Mesh* CreatePlanarBox(Vector3 halfDims, Color color = Color::WHITE);
	static Mesh* CreatePlanarBox(Vector3 halfDims, Texture texture);
};


//temp 2d
struct Poly2 {
	
};

//NOTE a model should not change after loading
struct Model{
	Armature armature;
	Mesh* mesh;
	
	Model() {}
	Model(Mesh* mesh);
};

#endif //DESHI_MODEL_H