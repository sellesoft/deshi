#pragma once
#include "Component.h"
#include "../math/Vector3.h"
#include "../math/Matrix4.h"
#include "../animation/Armature.h"

enum TextureType : uint32 { 
	ALBEDO, NORMAL, LIGHT, SPECULAR 
};

//A texture stores the texture filepath and texture type
struct Texture {
	char filename[16];
	TextureType type;
	Texture() {}
	Texture(const char* filename, TextureType type){
		strncpy_s(this->filename, filename, 16); this->filename[15] = '\0'; 
		this->type = type;
	}
};

//A vertex contains the position, color, and texture coordinates
//In the future, it might also contain bone weight information or the normal
struct Vertex {
	Vector3 pos;
	Vector3 color;
	Vector2 uv;
	//bone index        4tuple
	//bone weight       4tuple
	//normal or tangent 3tuple
	Vertex() {}
	Vertex(Vector3 pos, Vector3 color, Vector2 uv){
		this->pos = pos; this->color = color; this->uv = uv;
	}
};

enum ShaderFlagsBits : uint32 {
	NONE, NO_TEXTURE
};
typedef uint32 ShaderFlags;

//NOTE indices should be counter-clockwise
//A batch is a group of triangles, textures, and shader flags to be rendered together
struct Batch {
	char name[16];
	uint32              vertexCount;
	std::vector<Vertex> vertexArray;
	uint32              indexCount;
	std::vector<uint32> indexArray;
	
	char                 shaderName[16];
	uint32               textureCount;
	std::vector<Texture> textureArray;
	ShaderFlags          shaderFlags;
	
	Batch() {}
	Batch(const char* name, std::vector<Vertex> vertexArray, std::vector<uint32> indexArray, 
		  const char* shaderName, std::vector<Texture> textureArray, ShaderFlags shaderFlags) {
		strncpy_s(this->name, name, 16); this->name[15] = '\0'; 
		strncpy_s(this->shaderName, shaderName, 16); this->shaderName[15] = '\0'; 
		this->vertexArray  = vertexArray;  this->vertexCount  = vertexArray.size();
		this->indexArray   = indexArray;   this->indexCount   = indexArray.size();
		this->textureArray = textureArray; this->textureCount = textureArray.size();
	}
};

//A mesh is a collection of batches that use the same model matrix
struct Mesh {
	char name[16];
	uint32 vertexCount;
	uint32 indexCount;
	uint32 textureCount;
	
	uint32 batchCount;
	std::vector<Batch> batchArray;
	Matrix4 modelMatrix;
	
	Mesh() {}
	Mesh(const char* name, std::vector<Batch> batchArray) {
		strncpy_s(this->name, name, 16); this->name[15] = '\0';
		this->batchArray = batchArray; this->batchCount = batchArray.size();
		for(Batch& batch : batchArray) {
			this->vertexCount  += batch.vertexCount;
			this->indexCount   += batch.indexCount;
			this->textureCount += batch.textureCount;
		}
	}
};

//NOTE changes to a model after creation can be expensive due to vector resizing
//A model is the combination of a mesh and in the future: an armature, animation set, mesh array
struct Model : public Component {
	//Armature armature;
	Mesh mesh;
	
	Model() {}
	Model(Entity* e, Mesh mesh) {
		this->e = e;
		this->mesh = mesh;
	}
	
	static Model* CreateBox(Entity* e, Vector3 halfDims, olc::Pixel color = olc::WHITE) {
		Vector3 p = halfDims;
		std::vector<Vertex> vertices = {
			{Vector3( p.x, p.y, p.z), Vector3(color.r, color.g, color.b), Vector2(0, 0)}, // x, y, z	0
			{Vector3(-p.x, p.y, p.z), Vector3(color.r, color.g, color.b), Vector2(0, 0)}, //-x, y, z	1
			{Vector3( p.x,-p.y, p.z), Vector3(color.r, color.g, color.b), Vector2(0, 0)}, // x,-y, z	2
			{Vector3( p.x, p.y,-p.z), Vector3(color.r, color.g, color.b), Vector2(0, 0)}, // x, y,-z	3
			{Vector3(-p.x,-p.y, p.z), Vector3(color.r, color.g, color.b), Vector2(0, 0)}, //-x,-y, z	4
			{Vector3(-p.x, p.y,-p.z), Vector3(color.r, color.g, color.b), Vector2(0, 0)}, //-x, y,-z	5
			{Vector3( p.x,-p.y,-p.z), Vector3(color.r, color.g, color.b), Vector2(0, 0)}, // x,-y,-z	6
			{Vector3(-p.x,-p.y,-p.z), Vector3(color.r, color.g, color.b), Vector2(0, 0)}  //-x,-y,-z	7
		};
		std::vector<uint32> indices = {
			2, 4, 0,    0, 1, 2,	//back face
			7, 6, 3,    3, 5, 7,	//front face
			5, 3, 0,    0, 1, 5,	//top face
			4, 2, 7,    7, 6, 4,	//bottom face
			6, 2, 3,    3, 0, 6,	//right face
			4, 7, 5,    5, 1, 4,	//left face
		};
		
		Batch batch("box", vertices, indices, "default", {}, NO_TEXTURE);
		Mesh mesh("default_box", {batch});
		Model* model = new Model(e, mesh);
		return model;
	}
};