#pragma once
#include "Component.h"
#include "../math/Matrix4.h"
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
	Texture(const char* filename, TextureType type = TextureType::ALBEDO);
};

struct Vertex {
	Vector3 pos;
	Vector2 uv;
	Vector3 color; //between 0 and 1
	Vector3 normal;
	//bone index		4tuple
	//bone weight		4tuple
	Vertex() {}
	Vertex(Vector3 pos, Vector2 uv, Vector3 color, Vector3 normal);
};

enum ShaderFlagsBits : uint32 {
	SHADER_FLAGS_NONE = 0,
};
typedef uint32 ShaderFlags;


enum Shader : uint32 {
	DEFAULT, TWOD, PBR, WIREFRAME //TODO(r,delle) make default into phong
};

//NOTE indices should be counter-clockwise
struct Batch {
	char name[16];
	uint32 vertexCount  = 0;
	uint32 indexCount   = 0;
	uint32 textureCount = 0;
	std::vector<Vertex>  vertexArray;
	std::vector<uint32>  indexArray;
	std::vector<Texture> textureArray;
	
	Shader      shader;
	ShaderFlags shaderFlags;
	
	Batch() {}
	Batch(const char* name, std::vector<Vertex> vertexArray, std::vector<uint32> indexArray, std::vector<Texture> textureArray, Shader shader = Shader::DEFAULT, ShaderFlags shaderFlags = SHADER_FLAGS_NONE);
};

struct Mesh {
	char name[16];
	Matrix4 transform;
	
	uint32 vertexCount  = 0;
	uint32 indexCount   = 0;
	uint32 textureCount = 0;
	
	uint32 batchCount = 0;
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