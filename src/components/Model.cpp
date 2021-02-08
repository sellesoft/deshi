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



Vertex::Vertex(Vector3 pos, Vector3 color, Vector2 uv) {
	this->pos = pos; this->color = color; this->uv = uv;
}



////////////////////////////////////////////////////////////
// Batch
//////////////////////////////////////////////////////////



Batch::Batch(const char* name, std::vector<Vertex> vertexArray, std::vector<uint32> indexArray,
	const char* shaderName, std::vector<Texture> textureArray, ShaderFlags shaderFlags) {
	strncpy_s(this->name, name, 16); this->name[15] = '\0';
	strncpy_s(this->shaderName, shaderName, 16); this->shaderName[15] = '\0';
	this->vertexArray = vertexArray;	this->vertexCount = vertexArray.size();
	this->indexArray = indexArray;		this->indexCount = indexArray.size();
	this->textureArray = textureArray;	this->textureCount = textureArray.size();
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
		{Vector3(p.x, p.y, p.z),  Vector3(color.r, color.g, color.b), Vector2(0, 0)},		// x, y, z	0
		{Vector3(-p.x, p.y, p.z), Vector3(color.r, color.g, color.b), Vector2(0, 0)},	//-x, y, z	1
		{Vector3(p.x,-p.y, p.z),  Vector3(color.r, color.g, color.b), Vector2(0, 0)},	// x,-y, z	2
		{Vector3(p.x, p.y,-p.z),  Vector3(color.r, color.g, color.b), Vector2(0, 0)},	// x, y,-z	3
		{Vector3(-p.x,-p.y, p.z), Vector3(color.r, color.g, color.b), Vector2(0, 0)},	//-x,-y, z	4
		{Vector3(-p.x, p.y,-p.z), Vector3(color.r, color.g, color.b), Vector2(0, 0)},	//-x, y,-z	5
		{Vector3(p.x,-p.y,-p.z),  Vector3(color.r, color.g, color.b), Vector2(0, 0)},	// x,-y,-z	6
		{Vector3(-p.x,-p.y,-p.z), Vector3(color.r, color.g, color.b), Vector2(0, 0)}	//-x,-y,-z	7
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
	Mesh mesh("default_box", { batch });
	Model* model = new Model(e, mesh);
	return model;
}

void Model::Update() {

}
