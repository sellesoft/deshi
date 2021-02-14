#pragma once
#include "Component.h"
#include "../math/Vector3.h"
#include "../math/Vector2.h"
#include "../animation/Armature.h"

enum TextureType : uint32 { 
	ALBEDO, NORMAL, LIGHT, SPECULAR 
};

struct Texture {
	char filename[16];
	TextureType type;
	Texture() {}
	Texture(const char* filename, TextureType type);
};

struct Vertex {
	Vector3 pos;
	Vector2 uv;
	Vector3 color;
	Vector3 normal;
	//bone index		4tuple
	//bone weight		4tuple
	Vertex() {}
	Vertex(Vector3 pos, Vector2 uv, Vector3 color, Vector3 normal);
};

enum ShaderFlagsBits : uint32 {
	NONE, NO_TEXTURE
};
typedef uint32 ShaderFlags;

//NOTE indices should be counter-clockwise
struct Batch {
	char name[16];
	uint32 vertexCount;
	std::vector<Vertex>	vertexArray;
	uint32 indexCount;
	std::vector<uint32>	indexArray;
	
	char shaderName[16];
	uint32 textureCount;
	std::vector<Texture> textureArray;
	ShaderFlags	shaderFlags;
	
	Batch() {}
	Batch(const char* name, std::vector<Vertex> vertexArray, std::vector<uint32> indexArray,
		  const char* shaderName, std::vector<Texture> textureArray, ShaderFlags shaderFlags);
};

struct Mesh {
	char name[16];
	uint32 vertexCount;
	uint32 indexCount;
	uint32 textureCount;
	
	uint32 batchCount;
	std::vector<Batch> batchArray;
	
	Mesh() {}
	Mesh(const char* name, std::vector<Batch> batchArray);
};

//NOTE changes to a model after creation can be expensive due to vector resizing
struct Model : public Component {
	//Armature armature;
	Mesh mesh;
	
	Model() {}
	Model(Entity* e, Mesh mesh);
	
	static Model* CreateBox(Entity* e, Vector3 halfDims, Color color = Color::WHITE);
	
	
	void Update() override;
};