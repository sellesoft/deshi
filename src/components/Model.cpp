#include "Model.h"
#include "../EntityAdmin.h"



////////////////////////////////////////////////////////////
// Texture
//////////////////////////////////////////////////////////



Texture::Texture(const char* filename, TextureType type) {
	strncpy_s(this->filename, filename, 16); this->filename[15] = '\0';
	this->type = type;
}



////////////////////////////////////////////////////////////
// Vertex
//////////////////////////////////////////////////////////



Vertex::Vertex(Vector3 pos, Vector2 uv, Vector3 color, Vector3 normal) {
	this->pos = pos; this->color = color; this->uv = uv; this->normal = normal;
}



////////////////////////////////////////////////////////////
// Batch
//////////////////////////////////////////////////////////



Batch::Batch(const char* name, std::vector<Vertex> vertexArray, std::vector<uint32> indexArray, std::vector<Texture> textureArray, Shader shader, ShaderFlags shaderFlags) {
	strncpy_s(this->name, name, 16); this->name[15] = '\0';
	this->shader = shader; this->shaderFlags = shaderFlags;
	this->vertexArray = vertexArray;   this->vertexCount = vertexArray.size();
	this->indexArray = indexArray;     this->indexCount = indexArray.size();
	this->textureArray = textureArray; this->textureCount = textureArray.size();
}



////////////////////////////////////////////////////////////
// Mesh
//////////////////////////////////////////////////////////



Mesh::Mesh(const char* name, std::vector<Batch> batchArray) {
	strncpy_s(this->name, name, 16); this->name[15] = '\0';
	this->batchArray = batchArray; this->batchCount = batchArray.size();
	for (Batch& batch : batchArray) {
		this->vertexCount += batch.vertexCount;
		this->indexCount += batch.indexCount;
		this->textureCount += batch.textureCount;
	}
}



////////////////////////////////////////////////////////////
// Model
//////////////////////////////////////////////////////////



Model::Model(Entity* e, Mesh mesh) {
	this->mesh = mesh;
	
	layer = CL2_RENDSCENE;
}

Model* Model::CreateBox(Entity* e, Vector3 halfDims, Color color) {
	Vector3 p = halfDims;
	std::vector<Vertex> vertices = {
		{Vector3(p.x, p.y, p.z),  Vector2(0, 0), Vector3(color.r, color.g, color.b) / 255.f, Vector3::ZERO}, // x, y, z	0
		{Vector3(-p.x, p.y, p.z), Vector2(0, 0), Vector3(color.r, color.g, color.b) / 255.f, Vector3::ZERO}, //-x, y, z	1
		{Vector3(p.x,-p.y, p.z),  Vector2(0, 0), Vector3(color.r, color.g, color.b) / 255.f, Vector3::ZERO}, // x,-y, z	2
		{Vector3(p.x, p.y,-p.z),  Vector2(0, 0), Vector3(color.r, color.g, color.b) / 255.f, Vector3::ZERO}, // x, y,-z	3
		{Vector3(-p.x,-p.y, p.z), Vector2(0, 0), Vector3(color.r, color.g, color.b) / 255.f, Vector3::ZERO}, //-x,-y, z	4
		{Vector3(-p.x, p.y,-p.z), Vector2(0, 0), Vector3(color.r, color.g, color.b) / 255.f, Vector3::ZERO}, //-x, y,-z	5
		{Vector3(p.x,-p.y,-p.z),  Vector2(0, 0), Vector3(color.r, color.g, color.b) / 255.f, Vector3::ZERO}, // x,-y,-z	6
		{Vector3(-p.x,-p.y,-p.z), Vector2(0, 0), Vector3(color.r, color.g, color.b) / 255.f, Vector3::ZERO}  //-x,-y,-z	7
	};
	std::vector<uint32> indices = {
		2, 4, 0,    0, 1, 2,	//back face
		7, 6, 3,    3, 5, 7,	//front face
		5, 3, 0,    0, 1, 5,	//top face
		4, 2, 7,    7, 6, 4,	//bottom face
		6, 2, 3,    3, 0, 6,	//right face
		4, 7, 5,    5, 1, 4,	//left face
	};
	
	Batch batch("box", vertices, indices, {});
	Mesh mesh("default_box", { batch });
	Model* model = new Model(e, mesh);
	return model;
}

void Model::Update() {
	
}
