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
	CreateMaterial("null_material.mat", Shader_NULL, MaterialFlags_NONE, {0});
	CreateModelFromMesh(NullMesh(), Shader_NULL);
}

void Scene::
Reset(){
	for(int i=meshes.size()-1;    i>0; --i){ DeleteMesh(meshes[i]);        meshes.pop(); } 
	for(int i=materials.size()-1; i>0; --i){ DeleteMaterial(materials[i]); materials.pop(); } 
	for(int i=textures.size()-1;  i>0; --i){ DeleteTexture(textures[i]);   textures.pop(); } 
	for(int i=models.size()-1;    i>0; --i){ DeleteModel(models[i]);       models.pop(); } 
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
	
	Mesh* mesh = (Mesh*)calloc(1,bytes);   char* cursor = (char*)mesh + (1*sizeof(Mesh));
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
	mesh->indexes   = View<Mesh::Index>   (mesh->indexArray,    indexCount);
	mesh->vertexes  = View<Mesh::Vertex>  (mesh->vertexArray,   vertexCount);
	mesh->triangles = View<Mesh::Triangle>(mesh->triangleArray, triangleCount);
	mesh->faces     = View<Mesh::Face>    (mesh->faceArray,     faceCount);
	return mesh;
}

local void 
DeallocateMesh(Mesh* mesh){
	free(mesh);
}

//TODO(delle) change this to take in 8 points
pair<u32,Mesh*> Scene::
CreateBoxMesh(f32 width, f32 height, f32 depth, Color color){
	pair<u32,Mesh*> result(0, NullMesh());
	
	//check if created already
	forI(meshes.size()){
		if((strcmp(meshes[i]->name, "box_mesh.mesh") == 0) && (meshes[i]->aabbMax == vec3{width,height,depth})){
			return pair<u32,Mesh*>(i, meshes[i]);
		}
	}
	
	Mesh* mesh = AllocateMesh(36, 8, 6, 36, 24, 24, 24, 24);
	cpystr(mesh->name, "box_mesh.mesh", DESHI_NAME_SIZE);
	mesh->idx      = meshes.size();
	mesh->aabbMin  = {-width,-height,-depth};
	mesh->aabbMax  = { width, height, depth};
	mesh->center   = {  0.0f,   0.0f,  0.0f};
	
	Mesh::Vertex*   va = mesh->vertexArray;
	Mesh::Index*    ia = mesh->indexArray;
	Mesh::Triangle* ta = mesh->triangleArray;
	Mesh::Face*     fa = mesh->faceArray;
	vec3 p{width, height, depth};
	vec3 uv{0.0f, 0.0f};
	vec3 c = vec3(color.r, color.g, color.b) / 255.f;
	f32 ir3 = 1.0f / M_SQRT_THREE; // inverse root 3 (component of point on unit circle)
	
	//vertex array {pos, uv, color, normal(from center)}
	va[0]={{-p.x, p.y, p.z}, uv, c, {-ir3, ir3, ir3}}; // -x, y, z  0
	va[1]={{-p.x,-p.y, p.z}, uv, c, {-ir3,-ir3, ir3}}; // -x,-y, z  1
	va[2]={{-p.x, p.y,-p.z}, uv, c, {-ir3, ir3,-ir3}}; // -x, y,-z  2
	va[3]={{-p.x,-p.y,-p.z}, uv, c, {-ir3,-ir3,-ir3}}; // -x,-y,-z  3
	va[4]={{ p.x, p.y, p.z}, uv, c, { ir3, ir3, ir3}}; //  x, y, z  4
	va[5]={{ p.x,-p.y, p.z}, uv, c, { ir3,-ir3, ir3}}; //  x,-y, z  5
	va[6]={{ p.x, p.y,-p.z}, uv, c, { ir3, ir3,-ir3}}; //  x, y,-z  6
	va[7]={{ p.x,-p.y,-p.z}, uv, c, { ir3,-ir3,-ir3}}; //  x,-y,-z  7
	
	//index array
	ia[ 0]=4; ia[ 1]=2; ia[ 2]=0;    ia[ 3]=4; ia[ 4]=6; ia[ 5]=2; // +y face
	ia[ 6]=2; ia[ 7]=7; ia[ 8]=3;    ia[ 9]=2; ia[10]=6; ia[11]=7; // -z face
	ia[12]=6; ia[13]=5; ia[14]=7;    ia[15]=6; ia[16]=4; ia[17]=5; // +x face
	ia[18]=1; ia[19]=7; ia[20]=5;    ia[21]=1; ia[22]=3; ia[23]=7; // -y face
	ia[24]=0; ia[25]=3; ia[26]=1;    ia[27]=0; ia[28]=2; ia[29]=3; // -x face
	ia[30]=4; ia[31]=1; ia[32]=5;    ia[33]=4; ia[35]=0; ia[35]=1; // +z face
	
	//triangle array
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
	
	//triangle array neighbor array offsets
	ta[0].neighbors = View<u32>(ta[0].neighborArray, 3);
	ta[0].edges     = View<u8>(ta[0].edgeArray, 3);
	for(int i=1; i<12; ++i){
		ta[i].neighborArray = ta[i-1].neighborArray+3;
		ta[i].edgeArray     = ta[i-1].edgeArray+3;
		ta[i].neighbors = View<u32>(ta[i].neighborArray, 3);
		ta[i].edges     = View<u8>(ta[i].edgeArray, 3);
	}
	
	//triangle array neighbors array
	ta[ 0].neighbors[0]= 1; ta[ 0].neighbors[1]=11; ta[ 0].neighbors[2]= 9;
	ta[ 1].neighbors[0]= 0; ta[ 1].neighbors[1]= 3; ta[ 1].neighbors[2]= 5;
	ta[ 2].neighbors[0]= 3; ta[ 2].neighbors[1]= 9; ta[ 2].neighbors[2]= 7;
	ta[ 3].neighbors[0]= 2; ta[ 3].neighbors[1]= 1; ta[ 3].neighbors[2]= 4;
	ta[ 4].neighbors[0]= 5; ta[ 4].neighbors[1]= 3; ta[ 4].neighbors[2]= 6;
	ta[ 5].neighbors[0]= 4; ta[ 5].neighbors[1]= 1; ta[ 5].neighbors[2]=10;
	ta[ 6].neighbors[0]= 7; ta[ 6].neighbors[1]= 4; ta[ 6].neighbors[2]=10;
	ta[ 7].neighbors[0]= 6; ta[ 7].neighbors[1]= 2; ta[ 7].neighbors[2]= 8;
	ta[ 8].neighbors[0]= 9; ta[ 8].neighbors[1]=11; ta[ 8].neighbors[2]= 7;
	ta[ 9].neighbors[0]= 8; ta[ 9].neighbors[1]= 0; ta[ 9].neighbors[2]= 2;
	ta[10].neighbors[0]=11; ta[10].neighbors[1]= 5; ta[10].neighbors[2]= 6;
	ta[11].neighbors[0]=10; ta[11].neighbors[1]= 0; ta[11].neighbors[2]= 8;
	
	//triangle array edges array
	ta[ 0].edges[0]=0; ta[ 0].edges[1]=2; ta[ 0].edges[2]=1;
	ta[ 1].edges[0]=2; ta[ 1].edges[1]=1; ta[ 1].edges[2]=0;
	ta[ 2].edges[0]=0; ta[ 2].edges[1]=2; ta[ 2].edges[2]=1;
	ta[ 3].edges[0]=2; ta[ 3].edges[1]=0; ta[ 3].edges[2]=1;
	ta[ 4].edges[0]=0; ta[ 4].edges[1]=2; ta[ 4].edges[2]=1;
	ta[ 5].edges[0]=2; ta[ 5].edges[1]=0; ta[ 5].edges[2]=1;
	ta[ 6].edges[0]=0; ta[ 6].edges[1]=1; ta[ 6].edges[2]=2;
	ta[ 7].edges[0]=2; ta[ 7].edges[1]=1; ta[ 7].edges[2]=0;
	ta[ 8].edges[0]=0; ta[ 8].edges[1]=2; ta[ 8].edges[2]=1;
	ta[ 9].edges[0]=2; ta[ 9].edges[1]=0; ta[ 9].edges[2]=1;
	ta[10].edges[0]=0; ta[10].edges[1]=2; ta[10].edges[2]=1;
	ta[11].edges[0]=2; ta[11].edges[1]=0; ta[11].edges[2]=1;
	
	//face array  0=up, 1=back, 2=right, 3=down, 4=left, 5=forward
	for(int i=0; i<6; ++i){
		fa[i].normal                = ta[i*2].normal;
		fa[i].triangleCount         = 2;
		fa[i].vertexCount           = 4;
		fa[i].outerVertexCount      = 4;
		fa[i].neighborTriangleCount = 4;
		fa[i].neighborFaceCount     = 4;
	}
	
	//face array triangle array offsets
	fa[0].triangles = View<u32>(fa[0].triangleArray, 2);
	for(int i=1; i<6; ++i){
		fa[i].triangleArray = fa[i-1].triangleArray+2;
		fa[i].triangles = View<u32>(fa[i].triangleArray, 2);
	}
	
	//face array triangle arrays
	for(int i=0; i<6; ++i){
		fa[i].triangleArray[0]= i*2;
		fa[i].triangleArray[1]=(i*2)+1;
	}
	
	//face array vertex array offsets
	fa[0].vertexes      = View<u32>(fa[0].vertexArray, 4);
	fa[0].outerVertexes = View<u32>(fa[0].outerVertexArray, 4);
	for(int i=1; i<6; ++i){
		fa[i].vertexArray      = fa[i-1].vertexArray+4;
		fa[i].outerVertexArray = fa[i-1].outerVertexArray+4;
		fa[i].vertexes      = View<u32>(fa[i].vertexArray, 4);
		fa[i].outerVertexes = View<u32>(fa[i].outerVertexArray, 4);
	}
	
	//face array vertex arrays
	fa[0].vertexArray[0]=0; fa[0].vertexArray[1]=4; fa[0].vertexArray[2]=6; fa[0].vertexArray[3]=2; // +y
	fa[1].vertexArray[0]=2; fa[1].vertexArray[1]=6; fa[1].vertexArray[2]=7; fa[1].vertexArray[3]=3; // -z
	fa[2].vertexArray[0]=6; fa[2].vertexArray[1]=4; fa[2].vertexArray[2]=5; fa[2].vertexArray[3]=7; // +x
	fa[3].vertexArray[0]=3; fa[3].vertexArray[1]=7; fa[3].vertexArray[2]=5; fa[3].vertexArray[3]=1; // -y
	fa[4].vertexArray[0]=0; fa[4].vertexArray[1]=2; fa[4].vertexArray[2]=3; fa[4].vertexArray[3]=1; // -x
	fa[5].vertexArray[0]=4; fa[5].vertexArray[1]=0; fa[5].vertexArray[2]=1; fa[5].vertexArray[3]=5; // +z
	
	//face array neighbor array offsets
	fa[0].triangleNeighbors = View<u32>(fa[0].neighborTriangleArray, 4);
	fa[0].faceNeighbors     = View<u32>(fa[0].neighborFaceArray, 4);
	for(int i=1; i<6; ++i){
		fa[i].neighborTriangleArray = fa[i-1].neighborTriangleArray+4;
		fa[i].neighborFaceArray     = fa[i-1].neighborFaceArray+4;
		fa[i].triangleNeighbors = View<u32>(fa[i].neighborTriangleArray, 4);
		fa[i].faceNeighbors     = View<u32>(fa[i].neighborFaceArray, 4);
	}
	
	//face array neighbor triangle array
	fa[0].triangleNeighbors[0]= 9; fa[0].triangleNeighbors[1]= 3; fa[0].triangleNeighbors[2]= 5; fa[0].triangleNeighbors[3]=11;
	fa[1].triangleNeighbors[0]= 1; fa[1].triangleNeighbors[1]= 4; fa[1].triangleNeighbors[2]= 7; fa[1].triangleNeighbors[3]= 9;
	fa[2].triangleNeighbors[0]= 1; fa[2].triangleNeighbors[1]=10; fa[2].triangleNeighbors[2]= 6; fa[2].triangleNeighbors[3]= 3;
	fa[3].triangleNeighbors[0]= 4; fa[3].triangleNeighbors[1]=10; fa[3].triangleNeighbors[2]= 8; fa[3].triangleNeighbors[3]= 2;
	fa[4].triangleNeighbors[0]= 0; fa[4].triangleNeighbors[1]= 2; fa[4].triangleNeighbors[2]= 7; fa[4].triangleNeighbors[3]=11;
	fa[5].triangleNeighbors[0]= 0; fa[5].triangleNeighbors[1]= 8; fa[5].triangleNeighbors[2]= 6; fa[5].triangleNeighbors[3]= 5;
	
	//face array neighbor face array
	fa[0].faceNeighbors[0]=1; fa[0].faceNeighbors[1]=2; fa[0].faceNeighbors[2]=4; fa[0].faceNeighbors[3]=5;
	fa[1].faceNeighbors[0]=0; fa[1].faceNeighbors[1]=2; fa[1].faceNeighbors[2]=3; fa[1].faceNeighbors[3]=4;
	fa[2].faceNeighbors[0]=0; fa[2].faceNeighbors[1]=1; fa[2].faceNeighbors[2]=3; fa[2].faceNeighbors[3]=5;
	fa[3].faceNeighbors[0]=1; fa[3].faceNeighbors[1]=2; fa[3].faceNeighbors[2]=4; fa[3].faceNeighbors[3]=5;
	fa[4].faceNeighbors[0]=0; fa[4].faceNeighbors[1]=1; fa[4].faceNeighbors[2]=3; fa[4].faceNeighbors[3]=5;
	fa[5].faceNeighbors[0]=0; fa[5].faceNeighbors[1]=2; fa[5].faceNeighbors[2]=3; fa[5].faceNeighbors[3]=4;
	
	Render::LoadMesh(mesh);
	
	result.first  = mesh->idx;
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
			if(mesh->triangles[t->neighbors[i]].removed){
				outline.push_back(Math::WorldToScreen(t->p[t->edges[i]        ], camProjection, camView, screenDims).ToVector2());
				outline.push_back(Math::WorldToScreen(t->p[(t->edges[i]+1) % 3], camProjection, camView, screenDims).ToVector2());
			}
		}
	}
	return outline;
}


//////////////////
//// @texture ////
//////////////////
local Texture* 
AllocateTexture(){
	Texture* texture = (Texture*)calloc(1,sizeof(Texture));
	return texture;
}

local void 
DeallocateTexture(Texture* texture){
	free(texture);
}

pair<u32,Texture*> Scene::
CreateTextureFromFile(const char* filename, ImageFormat format, TextureType type, bool keepLoaded, bool generateMipmaps){
	pair<u32,Texture*> result(0, NullTexture());
	
	//check if created already
	forI(textures.size()){
		if(strcmp(textures[i]->name, filename) == 0){
			return pair<u32,Texture*>(i,textures[i]);
		}
	}
	
	Texture* texture = AllocateTexture();
	cpystr(texture->name, filename, DESHI_NAME_SIZE);
	texture->idx = textures.size();
	texture->format  = format;
	texture->type    = type;
	texture->pixels  = stbi_load((Assets::dirTextures()+filename).c_str(), &texture->width, &texture->height, &texture->depth, format);
	texture->loaded  = true;
	if(texture->pixels == 0){ 
		ERROR_LOC("Failed to create texture '",filename,"': ",stbi_failure_reason()); 
		DeallocateTexture(texture);
		return result; 
	}
	texture->mipmaps = (generateMipmaps) ? (int)log2(Max(texture->width, texture->height)) + 1 : 1;
	
	Render::LoadTexture(texture);
	if(!keepLoaded){
		stbi_image_free(texture->pixels); 
		texture->pixels = 0;
	}
	
	result.first  = texture->idx;
	result.second = texture;
	textures.add(texture);
	return result;
}

pair<u32,Texture*> Scene::
CreateTextureFromMemory(void* data, int width, int height, ImageFormat format, TextureType type, bool keepLoaded, bool generateMipmaps){
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
local Material* 
AllocateMaterial(u32 textureCount){
	Material* material = (Material*)calloc(1, sizeof(Material) + textureCount*sizeof(u32));
	material->textureCount = textureCount;
	material->textureArray = (u32*)(material+1);
	material->textures = View<u32>(material->textureArray, textureCount);
	return material;
}

local void 
DeallocateMaterial(Material* material){
	free(material);
}

pair<u32,Material*> Scene::
CreateMaterial(const char* name, Shader shader, MaterialFlags flags, array<u32> textures){
	pair<u32,Material*> result(0, NullMaterial());
	
	//check if created already
	bool textures_equal;
	forX(mi, materials.size()){
		if((strcmp(materials[mi]->name, name) == 0) && (materials[mi]->flags == flags) 
		   && (materials[mi]->shader == shader) && (materials[mi]->textureCount == textures.size())){
			textures_equal = true;
			forX(ti, textures.size()){ if(textures[ti] != materials[mi]->textures[ti]){ textures_equal = false; break; } }
			if(textures_equal) return pair<u32,Material*>(mi,materials[mi]);
		}
	}
	
	Material* material = AllocateMaterial(textures.size());
	material->idx = materials.size();
	cpystr(material->name, name, DESHI_NAME_SIZE);
	material->shader = shader;
	material->flags  = flags;
	forI(textures.size()) material->textures[i] = textures[i];
	
	Render::LoadMaterial(material);
	
	result.first  = materials.size();
	result.second = material;
	materials.add(material);
	return result;
}

void Scene::
DeleteMaterial(Material* material){
	//!Incomplete
}


////////////////
//// @model ////
////////////////
local Model* 
AllocateModel(u32 batchCount){
	Model* model = (Model*)calloc(1, sizeof(Model));
	model->batches.resize(batchCount);
	return model;
}

local void 
DeallocateModel(Model* model){
	free(model);
}

//TODO(delle,Op) speed this up with tinyobj::LoadOBJWithCallback to not parse twice
pair<u32,Model*> Scene::
CreateModelFromOBJ(const char* filename, ModelFlags flags){
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
	
	Model* model = AllocateModel(shapes.size());
	
	map<Mesh::Vertex,u32> uniqueVertexes;
	for(auto& shape : shapes){
		
	}
	
	//!Incomplete
	
	//result.first = models.size();
	//result.second = model;
	//models.add(model);
	return result;
}

pair<u32,Model*> Scene::
CreateModelFromMesh(Mesh* mesh, ModelFlags flags){
	pair<u32,Model*> result(0, NullModel());
	
	string mesh_name(mesh->name);
	string model_name = mesh_name.substr(0, mesh_name.size-6) + ".model";
	//check if created already
	forX(mi, models.size()){
		if((models[mi]->mesh == mesh) && (string(models[mi]->name) == model_name) && (models[mi]->flags == flags) 
		   && (models[mi]->batches.size() == 1) && (models[mi]->batches[0].indexOffset == 0)
		   && (models[mi]->batches[0].indexCount == mesh->indexCount) && (models[mi]->batches[0].material == 0)){
			return pair<u32,Model*>(mi,models[mi]);
		}
	}
	
	Model* model = AllocateModel(1);
	cpystr(model->name, model_name.str, DESHI_NAME_SIZE);
	model->idx = models.size();
	model->mesh = mesh;
	model->batches[0].indexOffset = 0;
	model->batches[0].indexCount = mesh->indexCount;
	model->batches[0].material = 0;
	
	result.first  = model->idx;
	result.second = model;
	models.add(model);
	return result;
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
