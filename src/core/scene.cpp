#define TINYOBJLOADER_IMPLEMENTATION
#include "../external/tinyobjloader/tiny_obj_loader.h"


///////////////
//// @init ////
///////////////
void Scene::
Init(){
	//null assets      //TODO(delle) store null.png and null shader in a .cpp
	CreateBoxMesh(1.0f, 1.0f, 1.0f);
	CreateTextureFromFile("null128.png");
	CreateMaterial("null_material", Shader_NULL, MaterialFlags_NONE, {NullTexture()});
	CreateModelFromMesh(NullMesh(), Shader_NULL);
}

void Scene::
Reset(){
	//@Incomplete
}


///////////////
//// @mesh ////
///////////////
local Mesh* 
AllocateMesh(u32 indexCount, u32 vertexCount, u32 faceCount, u32 planarVertexCount){
	u64 bytes =           1*sizeof(Mesh)
		+        indexCount*sizeof(Mesh::Index)
		+       vertexCount*sizeof(Mesh::Vertex)
		+    (indexCount/3)*sizeof(Mesh::Triangle)
		+         faceCount*sizeof(Mesh::Face)
		+    (indexCount/3)*sizeof(Mesh::Triangle*)
		+ planarVertexCount*sizeof(Mesh::Vertex*);
	
	Mesh* mesh = (Mesh*)calloc(1,bytes);
	mesh->bytes         = bytes;
	mesh->indexCount    = indexCount;
	mesh->vertexCount   = vertexCount;
	mesh->triangleCount = indexCount/3;
	mesh->faceCount     = faceCount;
	mesh->indexArray    = (Mesh::Index*)   (((char*)mesh)                +              1*sizeof(Mesh));
	mesh->vertexArray   = (Mesh::Vertex*)  (((char*)mesh->indexArray)    +     indexCount*sizeof(Mesh::Index));
	mesh->triangleArray = (Mesh::Triangle*)(((char*)mesh->vertexArray)   +    vertexCount*sizeof(Mesh::Vertex));
	mesh->faceArray     = (Mesh::Face*)    (((char*)mesh->triangleArray) + (indexCount/3)*sizeof(Mesh::Triangle));
	mesh->faceArray[0].triangleArray = (Mesh::Triangle**)(((char*)mesh->faceArray) + faceCount*sizeof(Mesh::Face));
	mesh->faceArray[0].vertexArray = (Mesh::Vertex**)(((char*)mesh->faceArray[0].triangleArray) + (indexCount/3)*sizeof(Mesh::Triangle*));
	return mesh;
}

local void 
DeallocateMesh(Mesh* mesh){
	free(mesh);
}

Mesh* Scene::
CreateBoxMesh(f32 width, f32 height, f32 depth, Color color){
	//@Incomplete check for existing mesh
	vec3 p{width, height, depth};
	vec3 uv{0.0f, 0.0f};
	vec3 c = vec3(color.r, color.g, color.b) / 255.f;
	f32 ir3 = 1.0f / M_SQRT_THREE; // inverse root 3 (component of point on unit circle)
	
	Mesh* mesh = AllocateMesh(36, 8, 6, 24);
	Mesh::Vertex*   va = mesh->vertexArray;
	Mesh::Index*    ia = mesh->indexArray;
	Mesh::Triangle* ta = mesh->triangleArray;
	Mesh::Face*     fa = mesh->faceArray;
	
	{//vertex array {pos, uv, color, normal(from center)}
		va[0]={{-p.x, p.y, p.z}, uv, c, {-ir3, ir3, ir3}}; // -x, y, z  0
		va[1]={{-p.x,-p.y, p.z}, uv, c, {-ir3,-ir3, ir3}}; // -x,-y, z  1
		va[2]={{-p.x, p.y,-p.z}, uv, c, {-ir3, ir3,-ir3}}; // -x, y,-z  2
		va[3]={{-p.x,-p.y,-p.z}, uv, c, {-ir3,-ir3,-ir3}}; // -x,-y,-z  3
		va[4]={{ p.x, p.y, p.z}, uv, c, { ir3, ir3, ir3}}; //  x, y, z  4
		va[5]={{ p.x,-p.y, p.z}, uv, c, { ir3,-ir3, ir3}}; //  x,-y, z  5
		va[6]={{ p.x, p.y,-p.z}, uv, c, { ir3, ir3,-ir3}}; //  x, y,-z  6
		va[7]={{ p.x,-p.y,-p.z}, uv, c, { ir3,-ir3,-ir3}}; //  x,-y,-z  7
	}{//index array
		ia[ 0]=4; ia[ 1]=2; ia[ 2]=0;    ia[ 3]=4; ia[ 4]=6; ia[ 5]=2; // +y face
		ia[ 6]=2; ia[ 7]=7; ia[ 8]=3;    ia[ 9]=2; ia[10]=6; ia[11]=7; // -z face
		ia[12]=6; ia[13]=5; ia[14]=7;    ia[15]=6; ia[16]=4; ia[17]=5; // +x face
		ia[18]=1; ia[19]=7; ia[20]=5;    ia[21]=1; ia[22]=3; ia[23]=7; // -y face
		ia[24]=0; ia[25]=3; ia[26]=1;    ia[27]=0; ia[28]=2; ia[29]=3; // -x face
		ia[30]=4; ia[31]=1; ia[32]=5;    ia[33]=4; ia[35]=0; ia[35]=1; // +z face
	}{//triangle array
		ta[ 0]={va+4,va+2,va+0,vec3::UP};      ta[ 1]={va+4,va+6,va+2,vec3::UP}; // +y
		ta[ 2]={va+2,va+7,va+3,vec3::BACK};    ta[ 3]={va+2,va+6,va+7,vec3::BACK}; // -z
		ta[ 4]={va+6,va+5,va+7,vec3::RIGHT};   ta[ 5]={va+6,va+4,va+5,vec3::RIGHT}; // +x
		ta[ 6]={va+1,va+7,va+5,vec3::DOWN};    ta[ 7]={va+1,va+3,va+7,vec3::DOWN}; // -y
		ta[ 8]={va+0,va+3,va+1,vec3::LEFT};    ta[ 9]={va+0,va+2,va+3,vec3::LEFT}; // -x
		ta[10]={va+4,va+1,va+5,vec3::FORWARD}; ta[11]={va+4,va+0,va+1,vec3::FORWARD}; // +z
	}{//face array
		fa[0].triangleCount = 2;   fa[0].vertexCount = 4; // +y
		fa[1].triangleCount = 2;   fa[1].vertexCount = 4; // -z
		fa[2].triangleCount = 2;   fa[2].vertexCount = 4; // +x
		fa[3].triangleCount = 2;   fa[3].vertexCount = 4; // -y 
		fa[4].triangleCount = 2;   fa[4].vertexCount = 4; // -x
		fa[5].triangleCount = 2;   fa[5].vertexCount = 4; // +z
	}{//face array triangle array offsets
		fa[1].triangleArray = fa[0].triangleArray+2;
		fa[2].triangleArray = fa[1].triangleArray+2;
		fa[3].triangleArray = fa[2].triangleArray+2;
		fa[4].triangleArray = fa[3].triangleArray+2;
		fa[5].triangleArray = fa[4].triangleArray+2;
	}{//face array triangle arrays
		fa[0].triangleArray[0]=ta+0;   fa[0].triangleArray[1]=ta+1;  // +y
		fa[1].triangleArray[0]=ta+2;   fa[1].triangleArray[1]=ta+3;  // -z
		fa[2].triangleArray[0]=ta+4;   fa[2].triangleArray[1]=ta+5;  // +x
		fa[3].triangleArray[0]=ta+6;   fa[3].triangleArray[1]=ta+7;  // -y
		fa[4].triangleArray[0]=ta+8;   fa[4].triangleArray[1]=ta+9;  // -x
		fa[5].triangleArray[0]=ta+10;  fa[5].triangleArray[1]=ta+11; // +z
	}{//face array vertex array offsets
		fa[1].vertexArray = fa[0].vertexArray+4;
		fa[2].vertexArray = fa[1].vertexArray+4;
		fa[3].vertexArray = fa[2].vertexArray+4;
		fa[4].vertexArray = fa[3].vertexArray+4;
		fa[5].vertexArray = fa[4].vertexArray+4;
	}{//face array vertex arrays (clockwise starting at uv{0,0})
		fa[0].vertexArray[0]=va+0; fa[0].vertexArray[1]=va+4; fa[0].vertexArray[2]=va+6; fa[0].vertexArray[3]=va+2; // +y
		fa[1].vertexArray[0]=va+2; fa[1].vertexArray[1]=va+6; fa[1].vertexArray[2]=va+7; fa[1].vertexArray[3]=va+3; // -z
		fa[2].vertexArray[0]=va+6; fa[2].vertexArray[1]=va+4; fa[2].vertexArray[2]=va+5; fa[2].vertexArray[3]=va+7; // +x
		fa[3].vertexArray[0]=va+3; fa[3].vertexArray[1]=va+7; fa[3].vertexArray[2]=va+5; fa[3].vertexArray[3]=va+1; // -y
		fa[4].vertexArray[0]=va+0; fa[4].vertexArray[1]=va+2; fa[4].vertexArray[2]=va+3; fa[4].vertexArray[3]=va+1; // -x
		fa[5].vertexArray[0]=va+4; fa[5].vertexArray[1]=va+0; fa[5].vertexArray[2]=va+1; fa[5].vertexArray[3]=va+5; // +z
	}
	
	cpystr(mesh->name, "box_mesh", DESHI_NAME_SIZE);
	mesh->aabbMin  = {-width,-height,-depth};
	mesh->aabbMax  = { width, height, depth};
	mesh->center   = {  0.0f,   0.0f,  0.0f};
	mesh->checksum = Utils::dataHash32(mesh, mesh->bytes);
	
	meshes.push_back(mesh);
	//@Incomplete check against existing checksums
	return mesh;
}

Mesh* Scene::
CreateMeshFromFile(const char* filename){
	//@Incomplete
	return 0;
}

Mesh* Scene::
CreateMeshFromMemory(void* data){
	//@Incomplete
	return 0;
}

void Scene::
DeleteMesh(Mesh* mesh){
	//@Incomplete
}


//////////////////
//// @texture ////
//////////////////
Texture* Scene::
CreateTextureFromFile(const char* filename, TextureType type){
	//@Incomplete
	return 0;
}

Texture* Scene::
CreateTextureFromMemory(void* data, TextureType type){
	//@Incomplete
	return 0;
}

void Scene::
DeleteTexture(Texture* texture){
	//@Incomplete
}


///////////////////
//// @material ////
///////////////////
Material* Scene::
CreateMaterial(const char* name, Shader shader, MaterialFlags flags, std::vector<Texture*> textures){
	//@Incomplete
	return 0;
}

void Scene::
DeleteMaterial(Material* material){
	//@Incomplete
}


////////////////
//// @model ////
////////////////
//TODO(delle,Op) speed this up with tinyobj::LoadOBJWithCallback to not parse twice
Model* Scene::
CreateModelFromOBJ(const char* filename, Shader shader, Color color, bool planarize){
	//setup tinyobj and parse the OBJ file
	tinyobj::ObjReaderConfig reader_config;
	reader_config.triangulate     = true;
	reader_config.vertex_color    = true;
	reader_config.mtl_search_path = Assets::dirModels(); // Path to material files
	tinyobj::ObjReader reader;
	if(!reader.ParseFromFile(Assets::dirModels() + filename, reader_config)) {
		ERROR("Failed to read OBJ file: ", filename);
		if (!reader.Error().empty()) ERROR("TinyObjReader: ", reader.Error());
		return 0;
	}
	if(!reader.Warning().empty()) WARNING("TinyObjReader: " , reader.Warning());
	
	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();
	Assert(shapes[0].mesh.num_face_vertices[0] == 3, "OBJ must be triangulated");
	
	//check which features it has
	bool hasMaterials = materials.size() > 0;
	bool hasNormals = attrib.normals.size() > 0;
	bool hasUVs = attrib.texcoords.size() > 0;
	bool hasColors = attrib.colors.size() > 0;
	
	std::unordered_map<Mesh::Vertex, u32> uniqueVertexes{};
	
	//if planarize, change all vertex normals to equal their face normal
	if(planarize){
		//@Incomplete
	}
	
	Model* model = 0;
	
	//return model pointer
	return model;
}

Model* Scene::
CopyModel(Model* model){
	
}

Model* Scene::
CreateModelFromMesh(Mesh* mesh, Shader shader, Color color){
	//@Incomplete
	return 0;
}

void Scene::
DeleteModel(Model* model){
	//@Incomplete
}
