#include "Model.h"

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



Mesh::Mesh(const char* name, std::vector<Batch> batchArray, Matrix4 transform) {
	strncpy_s(this->name, name, 15); this->name[15] = '\0';
	this->transform = transform;
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



Model::Model(Mesh mesh) {
	this->mesh = mesh;
}

Model* Model::CreateBox(Vector3 halfDims, Color color) {
	Vector3 p = halfDims;
	Vector3 c = Vector3(color.r, color.g, color.b) / 255.f;
	std::vector<Vertex> vertices = {
		{Vector3(-p.x, p.y, p.z), Vector2(0.f, 0.f), c, Vector3::ZERO}, //-x, y, z	0
		{Vector3(-p.x,-p.y, p.z), Vector2(0.f, 0.f), c, Vector3::ZERO}, //-x,-y, z	1
		{Vector3(-p.x, p.y,-p.z), Vector2(0.f, 0.f), c, Vector3::ZERO}, //-x, y,-z	2
		{Vector3(-p.x,-p.y,-p.z), Vector2(0.f, 0.f), c, Vector3::ZERO}, //-x,-y,-z	3
		{Vector3( p.x, p.y, p.z), Vector2(0.f, 0.f), c, Vector3::ZERO}, // x, y, z	4
		{Vector3( p.x,-p.y, p.z), Vector2(0.f, 0.f), c, Vector3::ZERO}, // x,-y, z	5
		{Vector3( p.x, p.y,-p.z), Vector2(0.f, 0.f), c, Vector3::ZERO}, // x, y,-z	6
		{Vector3( p.x,-p.y,-p.z), Vector2(0.f, 0.f), c, Vector3::ZERO}  // x,-y,-z	7
	};
	std::vector<uint32> indices = {
		4,2,0,    4,6,2,	//top face
		2,7,3,    2,6,7,	//-z face
		6,5,7,    6,4,5,	//right face
		1,7,5,    1,3,7,	//bottom face
		0,3,1,    0,2,3,	//left face
		4,1,5,    4,0,1,	//+z face
	};
	
	Batch batch("box_batch", vertices, indices, {});
	Mesh mesh("default_box", { batch });
	Model* model = new Model(mesh);
	return model;
}

Model* Model::CreatePlanarBox(Vector3 halfDims, Color color) {
	float x = halfDims.x; float y = halfDims.y; float z = halfDims.z;
	Vector2 tl = Vector2(0.f, 0.f);   Vector2 tr = Vector2(1.f, 0.f); 
	Vector2 bl = Vector2(0.f, 1.f);   Vector2 br = Vector2(1.f, 1.f); 
	Vector3 c = Vector3(color.r, color.g, color.b) / 255.f;
	std::vector<Vertex> vertices = {
		//top face
		{Vector3(-x, y, z), tl, c, Vector3::UP},      //0
		{Vector3( x, y, z), tr, c, Vector3::UP},      //1
		{Vector3( x, y,-z), br, c, Vector3::UP},      //2
		{Vector3(-x, y,-z), bl, c, Vector3::UP},      //3
		//back face
		{Vector3(-x, y,-z), tl, c, Vector3::BACK},    //4
		{Vector3( x, y,-z), tr, c, Vector3::BACK},    //5
		{Vector3( x,-y,-z), br, c, Vector3::BACK},    //6
		{Vector3(-x,-y,-z), bl, c, Vector3::BACK},    //7
		//right face
		{Vector3( x, y,-z), tl, c, Vector3::RIGHT},   //8
		{Vector3( x, y, z), tr, c, Vector3::RIGHT},   //9
		{Vector3( x,-y, z), br, c, Vector3::RIGHT},   //10
		{Vector3( x,-y,-z), bl, c, Vector3::RIGHT},   //11
		//bottom face
		{Vector3(-x,-y,-z), tl, c, Vector3::DOWN},    //12
		{Vector3( x,-y,-z), tr, c, Vector3::DOWN},    //13
		{Vector3( x,-y, z), br, c, Vector3::DOWN},    //14
		{Vector3(-x,-y, z), bl, c, Vector3::DOWN},    //15
		//front face
		{Vector3( x, y, z), tl, c, Vector3::FORWARD}, //16
		{Vector3(-x, y, z), tr, c, Vector3::FORWARD}, //17
		{Vector3(-x,-y, z), br, c, Vector3::FORWARD}, //18
		{Vector3( x,-y, z), bl, c, Vector3::FORWARD}, //19
		//left face
		{Vector3(-x, y, z), tl, c, Vector3::LEFT},    //20
		{Vector3(-x, y,-z), tr, c, Vector3::LEFT},    //21
		{Vector3(-x,-y,-z), br, c, Vector3::LEFT},    //22
		{Vector3(-x,-y, z), bl, c, Vector3::LEFT},    //23
	};
	std::vector<uint32> indices = {
		0,1,2,    0,2,3,   //top face
		4,5,6,    4,6,7,   //back face
		8,9,10,   8,10,11, //right face
		12,13,14, 12,14,15,//bottom face
		16,17,18, 16,18,19,//front face
		20,21,22, 20,22,23,//left face
	};
	
	Batch batch("planarbox_batch", vertices, indices, {});
	Mesh mesh("default_planarbox", { batch });
	Model* model = new Model(mesh);
	return model;
}