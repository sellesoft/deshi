#define TINYOBJLOADER_IMPLEMENTATION
#include "../external/tinyobjloader/tiny_obj_loader.h"


///////////////
//// @init ////
///////////////
void Scene::
Init(){
	//null assets      //TODO(delle) store null.png and null shader in a .cpp
	CreateBoxMesh(1.0f, 1.0f, 1.0f); cpystr(NullMesh()->name, "null_mesh", DESHI_NAME_SIZE);
	CreateTextureFromFile("null128.png"); cpystr(NullTexture()->name, "null_texture", DESHI_NAME_SIZE);
	CreateMaterial("null_material", Shader_NULL, MaterialFlags_NONE, {NullTexture()});
	CreateModelFromMesh(NullMesh(), Shader_NULL); cpystr(NullTexture()->name, "null_model", DESHI_NAME_SIZE);
}

void Scene::
Reset(){
	for(int i=meshes.size()-1;    i>0; --i){ DeleteMesh(&meshes[i]);    meshes.pop(); } 
	for(int i=materials.size()-1; i>0; --i){ DeleteMesh(&materials[i]); materials.pop(); } 
	for(int i=textures.size()-1;  i>0; --i){ DeleteMesh(&textures[i]);  textures.pop(); } 
	for(int i=models.size()-1;    i>0; --i){ DeleteMesh(&models[i]);    models.pop(); } 
	lights.clear();
}


///////////////
//// @mesh ////
///////////////
local Mesh* 
AllocateMesh(u32 indexCount, u32 vertexCount, u32 faceCount, u32 trianglesNeighborCount, u32 facesVertexCount, u32 facesOuterVertexCount, u32 facesNeighborFaceCount, u32 facesNeighborTriangleCount){
	Assert(indexCount && vertexCount && faceCount && trianglesNeighborCount && facesVertexCount && facesOuterVertexCount && facesNeighborFaceCount && facesNeighborTriangleCount);
	
	u32 triangleCount = indexCount/3;
	u32 bytes =                    1*sizeof(Mesh)
		+                 indexCount*sizeof(Mesh::Index)
		+                vertexCount*sizeof(Mesh::Vertex)
		+              triangleCount*sizeof(Mesh::Triangle)
		+                  faceCount*sizeof(Mesh::Face)
		+     trianglesNeighborCount*sizeof(u32) //triangle neighbors
		+     trianglesNeighborCount*sizeof(u8)  //triangle edges
		+              triangleCount*sizeof(u32) //face triangles
		+           facesVertexCount*sizeof(u32)
		+      facesOuterVertexCount*sizeof(u32)
		+ facesNeighborTriangleCount*sizeof(u32)
		+     facesNeighborFaceCount*sizeof(u32);
	
	Mesh* mesh = (Mesh*)calloc(1,bytes);   char* cursor = mesh + (1*sizeof(Mesh));
	mesh->bytes         = bytes;
	mesh->indexCount    = indexCount;
	mesh->vertexCount   = vertexCount;
	mesh->triangleCount = triangleCount;
	mesh->faceCount     = faceCount;
	mesh->indexArray    = (Mesh::Index*)cursor;              cursor +=                 indexCount*sizeof(Mesh::Index);
	mesh->vertexArray   = (Mesh::Vertex*)cursor;             cursor +=                vertexCount*sizeof(Mesh::Vertex);
	mesh->triangleArray = (Mesh::Triangle*)cursor;           cursor +=              triangleCount*sizeof(Mesh::Triangle);
	mesh->faceArray     = (Mesh::Face*)cursor;               cursor +=                  faceCount*sizeof(Mesh::Face);
	mesh->triangleArray[0].neighborArray     = (u32*)cursor; cursor +=     trianglesNeighborCount*sizeof(u32);
	mesh->triangleArray[0].edgeArray         = (u8*)cursor;  cursor +=     trianglesNeighborCount*sizeof(u8);
	mesh->faceArray[0].triangleArray         = (u32*)cursor; cursor +=              triangleCount*sizeof(u32);
	mesh->faceArray[0].vertexArray           = (u32*)cursor; cursor +=           facesVertexCount*sizeof(u32);
	mesh->faceArray[0].outerVertexArray      = (u32*)cursor; cursor +=      facesOuterVertexCount*sizeof(u32);
	mesh->faceArray[0].neighborTriangleArray = (u32*)cursor; cursor += facesNeighborTriangleCount*sizeof(u32);
	mesh->faceArray[0].neighborFaceArray     = (u32*)cursor; //cursor +=     facesNeighborFaceCount*sizeof(u32);
	mesh->indexes   = array_view<Mesh::Index>   (mesh->indexArray,    indexCount,    indexCount);
	mesh->vertexes  = array_view<Mesh::Vertex>  (mesh->vertexArray,   vertexCount,   vertexCount);
	mesh->triangles = array_view<Mesh::Triangle>(mesh->triangleArray, triangleCount, triangleCount);
	mesh->faces     = array_view<Mesh::Face>    (mesh->faceArray,     faceCount,     faceCount);
	return mesh;
}

local void 
DeallocateMesh(Mesh* mesh){
	free(mesh);
}

local bool
TrianglesShareEdge(){
	
}

local void 
GenerateMeshTriangleNeighbors(Mesh* mesh){
	Triangle* t1, t2;
	forX(t1i, mesh->triangleCount){ t1 = mesh->triangleArray[t1i];
		forX(t2i, mesh->triangleCount){ t2 = mesh->triangleArray[t2i]; 
			if(t1i != t2i){
				for(u32 t1n : t1->neighbors){
					if(t2i == t1n){
						//@Incomplete
					}
				}
			}
		}
	}
}

local void GenerateMeshFaces(Mesh* mesh){
	
}

pair<u32,Mesh*> Scene::
CreateBoxMesh(f32 width, f32 height, f32 depth, Color color){
	pair<u32,Mesh*> result(0, NullMesh());
	vec3 p{width, height, depth};
	vec3 uv{0.0f, 0.0f};
	vec3 c = vec3(color.r, color.g, color.b) / 255.f;
	f32 ir3 = 1.0f / M_SQRT_THREE; // inverse root 3 (component of point on unit circle)
	
	Mesh* mesh = AllocateMesh(36, 8, 6, 36, 24, 24, 24, 24);
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
		ta[ 0].normal = vec3::UP;      ta[ 1].normal = vec3::UP;
		ta[ 2].normal = vec3::BACK;    ta[ 3].normal = vec3::BACK;
		ta[ 4].normal = vec3::RIGHT;   ta[ 5].normal = vec3::RIGHT;
		ta[ 6].normal = vec3::DOWN;    ta[ 7].normal = vec3::DOWN;
		ta[ 8].normal = vec3::LEFT;    ta[ 9].normal = vec3::LEFT;
		ta[10].normal = vec3::FORWARD; ta[11].normal = vec3::FORWARD;
		for(int i=0; i<12; ++i){
			ta[i].p[0] = va[ia[(i*3)+0]].pos;
			ta[i].p[1] = va[ia[(i*3)+1]].pos;
			ta[i].p[2] = va[ia[(i*3)+2]].pos;
			ta[i].v[0] = ia[(i*3)+0];
			ta[i].v[1] = ia[(i*3)+1];
			ta[i].v[2] = ia[(i*3)+2];
			ta[i].neighborCount = 3;
			ta[i].face = i / 2;
		}
	}{//face array
		fa[0]={ta[ 0].normal, 2, 4, 4, 4, 4}; // +y
		fa[1]={ta[ 2].normal, 2, 4, 4, 4, 4}; // -z
		fa[2]={ta[ 4].normal, 2, 4, 4, 4, 4}; // +x
		fa[3]={ta[ 6].normal, 2, 4, 4, 4, 4}; // -y
		fa[4]={ta[ 8].normal, 2, 4, 4, 4, 4}; // -x
		fa[5]={ta[10].normal, 2, 4, 4, 4, 4}; // +z
	}{//triangle array neighbor array offsets
		ta[0].neighbors = array_view<u32>(ta[0].neighborArray, 3, 3);
		ta[0].edges = array_view<u8>(ta[0].edgeArray, 3, 3);
		for(int i = 1; i < 12; ++i){
			ta[i].neighborArray = ta[i-1].neighborArray+3;
			ta[i].edgeArray     = ta[i-1].edgeArray+3;
			ta[i].neighbors = array_view<u32>(ta[i].neighborArray, 3, 3);
			ta[i].edges     = array_view<u8>(ta[i].edgeArray, 3, 3);
		}
	}{//triangle array neighbors array
		//!Incomplete
	}{//face array triangle array offsets
		for(int i = 1; i < 6; ++i){
			fa[i].triangleArray = fa[i-1].triangleArray+2;
		}
	}{//face array triangle arrays
		for(int i = 0; i < 6; ++i){
			fa[i].triangleArray[0]= i*2;
			fa[i].triangleArray[1]=(i*2)+1;
		}
	}{//face array vertex array offsets
		for(int i = 1; i < 6; ++i){
			fa[i].vertexArray      = fa[i-1].vertexArray+4;
			fa[i].outerVertexArray = fa[i-1].outerVertexArray+4;
		}
	}{//face array vertex arrays
		fa[0].vertexArray[0]=0; fa[0].vertexArray[1]=4; fa[0].vertexArray[2]=6; fa[0].vertexArray[3]=2; // +y
		fa[1].vertexArray[0]=2; fa[1].vertexArray[1]=6; fa[1].vertexArray[2]=7; fa[1].vertexArray[3]=3; // -z
		fa[2].vertexArray[0]=6; fa[2].vertexArray[1]=4; fa[2].vertexArray[2]=5; fa[2].vertexArray[3]=7; // +x
		fa[3].vertexArray[0]=3; fa[3].vertexArray[1]=7; fa[3].vertexArray[2]=5; fa[3].vertexArray[3]=1; // -y
		fa[4].vertexArray[0]=0; fa[4].vertexArray[1]=2; fa[4].vertexArray[2]=3; fa[4].vertexArray[3]=1; // -x
		fa[5].vertexArray[0]=4; fa[5].vertexArray[1]=0; fa[5].vertexArray[2]=1; fa[5].vertexArray[3]=5; // +z
	}
	
	cpystr(mesh->name, "box_mesh", DESHI_NAME_SIZE);
	mesh->aabbMin  = {-width,-height,-depth};
	mesh->aabbMax  = { width, height, depth};
	mesh->center   = {  0.0f,   0.0f,  0.0f};
	mesh->checksum = Utils::dataHash32(mesh, mesh->bytes);
	
	//check if existing and return
	forI(meshes.size()){
		if(mesh->checksum == meshes[i]->checksum) return pair<u32,Mesh*>(i, meshes[i]);
	}
	result.first = meshes.size();
	result.second = mesh;
	meshes.add(mesh);
	return result;
}

pair<u32,Mesh*> Scene::
CreateMeshFromFile(const char* filename){
	pair<u32,Mesh*> result(0, NullMesh());
	return result;
	//!Incomplete
}

pair<u32,Mesh*> Scene::
CreateMeshFromMemory(void* data){
	pair<u32,Mesh*> result(0, NullMesh());
	return result;
	//!Incomplete
}

void Scene::
DeleteMesh(Mesh* mesh){
	//!Incomplete
}

std::vector<Vector2> Scene::
GenerateMeshOutlinePoints(Mesh* mesh, mat4 transform, mat4 camProjection, mat4 camView, vec3 camPosition, vec2 screenDims){ //!TestMe
	std::vector<vec2> outline;
	std::vector<Mesh::Triangle*> nonculled;
	forI(mesh->triangleCount){ Mesh::Triangle* t = &mesh->triangleArray[i];
		t->removed = false;
		if(t->normal.dot(camPosition - (t->p[0] * transform)) < 0){
			nonculled.push_back(t);
		}else{
			t->removed = true;
		}
	}
	for(Mesh::Triangle* t : nonculled){
		forI(t->neighborCount){
			if(t->neighborArray[i]->removed){
				outline.push_back(Math::WorldToScreen(t->p[t->edgeArray[i]        ], camProjection, camView, screenDims).ToVector2());
				outline.push_back(Math::WorldToScreen(t->p[(t->edgeArray[i]+1) % 3], camProjection, camView, screenDims).ToVector2());
			}
		}
	}
	return outline;
}


//////////////////
//// @texture ////
//////////////////
pair<u32,Texture*> Scene::
CreateTextureFromFile(const char* filename, TextureType type){
	pair<u32,Texture*> result(0, NullTexture());
	return result;
	//!Incomplete
}

pair<u32,Texture*> Scene::
CreateTextureFromMemory(void* data, TextureType type){
	pair<u32,Texture*> result(0, NullTexture());
	return result;
	//!Incomplete
}

void Scene::
DeleteTexture(Texture* texture){
	//!Incomplete
}


///////////////////
//// @material ////
///////////////////
pair<u32,Material*> Scene::
CreateMaterial(const char* name, Shader shader, MaterialFlags flags, std::vector<Texture*> textures){
	pair<u32,Material*> result(0, NullMaterial());
	return result;
	//!Incomplete
}

void Scene::
DeleteMaterial(Material* material){
	//!Incomplete
}


////////////////
//// @model ////
////////////////
//TODO(delle,Op) speed this up with tinyobj::LoadOBJWithCallback to not parse twice
pair<u32,Model*> Scene::
CreateModelFromOBJ(const char* filename, Shader shader, Color color){
	pair<u32,Model*> result(0, NullModel());
	//setup tinyobj and parse the OBJ file
	tinyobj::ObjReaderConfig reader_config;
	reader_config.triangulate     = true;
	reader_config.vertex_color    = true;
	reader_config.mtl_search_path = Assets::dirModels(); // Path to material files
	tinyobj::ObjReader reader;
	if(!reader.ParseFromFile(Assets::dirModels() + filename, reader_config)) {
		ERROR("Failed to read OBJ file: ", filename);
		if (!reader.Error().empty()) ERROR("TinyObjReader: ", reader.Error());
		return result;
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
	
	//std::unordered_map<Mesh::Vertex, u32> uniqueVertexes{};
	
	//!Incomplete
	Model* model = 0;
	
	//add the scene and return
	result.first = model.size();
	result.second = model;
	models.add(model);
	return result;
}

pair<u32,Model*> Scene::
CreateModelFromMesh(Mesh* mesh, Shader shader, Color color){
	pair<u32,Model*> result(0, NullModel());
	return result;
	//!Incomplete
}

pair<u32,Model*> Scene::
CopyModel(Model* model){
	pair<u32,Model*> result(0, NullModel());
	return result;
	//!Incomplete
}

void Scene::
DeleteModel(Model* model){
	//!Incomplete
}
