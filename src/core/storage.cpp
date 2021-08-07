#ifdef ParseError
#define TempParseError ParseError
#undef ParseError
#endif
#define ParseError(...) ERROR("Error parsing '",filepath,"' on line '",line_number,"'! ",__VA_ARGS__)

namespace Storage{
	local array<Mesh*>&     meshes    = DengStorage->meshes;
	local array<Texture*>&  textures  = DengStorage->textures;
	local array<Material*>& materials = DengStorage->materials;
	local array<Model*>&    models    = DengStorage->models;
	local array<Light*>&    lights    = DengStorage->lights;
};

///////////////
//// @init ////
///////////////
void Storage::
Init(){
	stbi_set_flip_vertically_on_load(true);
	//null assets      //TODO(delle) store null.png and null shader in a .cpp
	CreateBoxMesh(1.0f, 1.0f, 1.0f); cpystr(NullMesh()->name, "null", DESHI_NAME_SIZE);
	CreateTextureFromFile("null128.png");
	CreateMaterial("null", Shader_NULL, MaterialFlags_NONE, {0});
	CreateModelFromMesh(NullMesh(), Shader_NULL); cpystr(NullModel()->name, "null", DESHI_NAME_SIZE);
}

void Storage::
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
AllocateMesh(u32 indexCount, u32 vertexCount, u32 faceCount, u32 trianglesNeighborCount, u32 facesVertexCount, u32 facesOuterVertexCount, u32 facesNeighborTriangleCount, u32 facesNeighborFaceCount){
	Assert(indexCount && vertexCount && faceCount);
	
	u32 triangleCount = indexCount/3;
	u32 bytes =                    1*sizeof(Mesh)
		+                vertexCount*sizeof(Mesh::Vertex)
		+                 indexCount*sizeof(Mesh::Index)
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
	mesh->bytes         = bytes;
	mesh->indexCount    = indexCount;
	mesh->vertexCount   = vertexCount;
	mesh->triangleCount = triangleCount;
	mesh->faceCount     = faceCount;
	mesh->totalTriNeighborCount      = trianglesNeighborCount;
	mesh->totalFaceVertexCount       = facesVertexCount;
	mesh->totalFaceOuterVertexCount  = facesOuterVertexCount;
	mesh->totalFaceTriNeighborCount  = facesNeighborTriangleCount;
	mesh->totalFaceFaceNeighborCount = facesNeighborFaceCount;
	mesh->vertexArray   = (Mesh::Vertex*)cursor;   cursor +=   vertexCount*sizeof(Mesh::Vertex);
	mesh->indexArray    = (Mesh::Index*)cursor;    cursor +=    indexCount*sizeof(Mesh::Index);
	mesh->triangleArray = (Mesh::Triangle*)cursor; cursor += triangleCount*sizeof(Mesh::Triangle);
	mesh->faceArray     = (Mesh::Face*)cursor;     cursor +=     faceCount*sizeof(Mesh::Face);
	mesh->indexes   = View<Mesh::Index>   (mesh->indexArray,    indexCount);
	mesh->vertexes  = View<Mesh::Vertex>  (mesh->vertexArray,   vertexCount);
	mesh->triangles = View<Mesh::Triangle>(mesh->triangleArray, triangleCount);
	mesh->faces     = View<Mesh::Face>    (mesh->faceArray,     faceCount);
	return mesh;
}

//TODO(delle) change this to take in 8 points
pair<u32,Mesh*> Storage::
CreateBoxMesh(f32 width, f32 height, f32 depth, Color color){
	pair<u32,Mesh*> result(0, NullMesh());
	
	//check if created already
	forI(meshes.size()){
		if((strcmp(meshes[i]->name, "box_mesh") == 0) && (meshes[i]->aabbMax == vec3{width,height,depth})){
			return pair<u32,Mesh*>(i, meshes[i]);
		}
	}
	
	Mesh* mesh = AllocateMesh(36, 8, 6, 36, 24, 24, 24, 24);
	cpystr(mesh->name, "box_mesh", DESHI_NAME_SIZE);
	mesh->idx      = meshes.count;
	mesh->aabbMin  = {-width,-height,-depth};
	mesh->aabbMax  = { width, height, depth};
	mesh->center   = {  0.0f,   0.0f,  0.0f};
	
	Mesh::Vertex*   va = mesh->vertexArray;
	Mesh::Index*    ia = mesh->indexArray;
	Mesh::Triangle* ta = mesh->triangleArray;
	Mesh::Face*     fa = mesh->faceArray;
	vec3 p{width, height, depth};
	vec3 uv{0.0f, 0.0f};
	u32 c = color.R8G8B8A8_UNORM();
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
	ta[0].neighborArray = (u32*)(fa + 6);
	ta[0].edgeArray     = (u8*)(ta[0].neighborArray + 36);
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
		fa[i].center                = ta[i*2].normal * p;
		fa[i].triangleCount         = 2;
		fa[i].vertexCount           = 4;
		fa[i].outerVertexCount      = 4;
		fa[i].neighborTriangleCount = 4;
		fa[i].neighborFaceCount     = 4;
	}
	
	//face array triangle array offsets
	fa[0].triangleArray = (u32*)(ta[0].edgeArray + 36);
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
	fa[0].vertexArray      = (u32*)(fa[0].triangleArray+12);
	fa[0].outerVertexArray = (u32*)(fa[0].vertexArray+24);
	fa[0].vertexes      = View<u32>(fa[0].vertexArray, 4);
	fa[0].outerVertexes = View<u32>(fa[0].outerVertexArray, 4);
	for(int i=1; i<6; ++i){
		fa[i].vertexArray      = fa[i-1].vertexArray+4;
		fa[i].outerVertexArray = fa[i-1].outerVertexArray+4;
		fa[i].vertexes      = View<u32>(fa[i].vertexArray, 4);
		fa[i].outerVertexes = View<u32>(fa[i].outerVertexArray, 4);
	}
	
	//face array vertex array
	fa[0].vertexArray[0]=0; fa[0].vertexArray[1]=4; fa[0].vertexArray[2]=6; fa[0].vertexArray[3]=2; // +y
	fa[1].vertexArray[0]=2; fa[1].vertexArray[1]=6; fa[1].vertexArray[2]=7; fa[1].vertexArray[3]=3; // -z
	fa[2].vertexArray[0]=6; fa[2].vertexArray[1]=4; fa[2].vertexArray[2]=5; fa[2].vertexArray[3]=7; // +x
	fa[3].vertexArray[0]=3; fa[3].vertexArray[1]=7; fa[3].vertexArray[2]=5; fa[3].vertexArray[3]=1; // -y
	fa[4].vertexArray[0]=0; fa[4].vertexArray[1]=2; fa[4].vertexArray[2]=3; fa[4].vertexArray[3]=1; // -x
	fa[5].vertexArray[0]=4; fa[5].vertexArray[1]=0; fa[5].vertexArray[2]=1; fa[5].vertexArray[3]=5; // +z
	
	//face array outer vertex array
	fa[0].outerVertexArray[0]=0; fa[0].outerVertexArray[1]=4; fa[0].outerVertexArray[2]=6; fa[0].outerVertexArray[3]=2; // +y
	fa[1].outerVertexArray[0]=2; fa[1].outerVertexArray[1]=6; fa[1].outerVertexArray[2]=7; fa[1].outerVertexArray[3]=3; // -z
	fa[2].outerVertexArray[0]=6; fa[2].outerVertexArray[1]=4; fa[2].outerVertexArray[2]=5; fa[2].outerVertexArray[3]=7; // +x
	fa[3].outerVertexArray[0]=3; fa[3].outerVertexArray[1]=7; fa[3].outerVertexArray[2]=5; fa[3].outerVertexArray[3]=1; // -y
	fa[4].outerVertexArray[0]=0; fa[4].outerVertexArray[1]=2; fa[4].outerVertexArray[2]=3; fa[4].outerVertexArray[3]=1; // -x
	fa[5].outerVertexArray[0]=4; fa[5].outerVertexArray[1]=0; fa[5].outerVertexArray[2]=1; fa[5].outerVertexArray[3]=5; // +z
	
	//face array neighbor array offsets
	fa[0].neighborTriangleArray = (u32*)(fa[0].outerVertexArray+24);
	fa[0].neighborFaceArray     = (u32*)(fa[0].neighborTriangleArray+24);
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

pair<u32,Mesh*> Storage::
CreateMeshFromFile(const char* filename){
	pair<u32,Mesh*> result(0, NullMesh());
	if(strcmp(filename, "null") == 0) return result;
	
	//split filename into name and extension
	std::string filepath = Assets::dirModels()+filename;
	string fullname(filename);
	string name, extension;
	bool has_extension;
	size_t dot_idx = fullname.find_first_of_lookback('.', fullname.size-1);
	if(dot_idx != -1){
		has_extension = true;
		name = fullname.substr(0, dot_idx-1);
		extension = fullname.substr(dot_idx+1);
	}else{
		has_extension = false;
		name = fullname;
	}
	if(!has_extension){
		filepath += ".mesh";
	}
	
	//check if mesh is already loaded
	forX(mi, meshes.count){
		if((strcmp(meshes[mi]->name, name.str) == 0)){
			return pair<u32,Mesh*>(mi,meshes[mi]);
		}
	}
	
	//load .mesh file
	char* buffer = Assets::readFileBinaryToArray(filepath, 0, true);
	if(!buffer){  return result; }
	defer{ delete[] buffer; };
	
	return CreateMeshFromMemory(buffer);
}

pair<u32,Mesh*> Storage::
CreateMeshFromMemory(void* data){
	pair<u32,Mesh*> result(0, NullMesh());
	
	u32 bytes = *((u32*)data);
	if(bytes < sizeof(Mesh)){
		ERROR("Mesh size was too small when trying to load it from memory");
		return result;
	}
	
	//allocate
	Mesh* mesh = (Mesh*)malloc(bytes);           char* cursor = (char*)mesh + (1*sizeof(Mesh));
	memcpy(mesh, data, bytes);
	mesh->idx = meshes.count;
	mesh->vertexArray   = (Mesh::Vertex*)cursor;       cursor +=   mesh->vertexCount*sizeof(Mesh::Vertex);
	mesh->indexArray    = (Mesh::Index*)cursor;        cursor +=    mesh->indexCount*sizeof(Mesh::Index);
	mesh->triangleArray = (Mesh::Triangle*)cursor;     cursor += mesh->triangleCount*sizeof(Mesh::Triangle);
	mesh->faceArray     = (Mesh::Face*)cursor;         cursor +=     mesh->faceCount*sizeof(Mesh::Face);
	mesh->indexes   = View<Mesh::Index>   (mesh->indexArray,    mesh->indexCount);
	mesh->vertexes  = View<Mesh::Vertex>  (mesh->vertexArray,   mesh->vertexCount);
	mesh->triangles = View<Mesh::Triangle>(mesh->triangleArray, mesh->triangleCount);
	mesh->faces     = View<Mesh::Face>    (mesh->faceArray,     mesh->faceCount);
	mesh->triangles[0].neighborArray = (u32*)(mesh->faceArray + mesh->faceCount);
	mesh->triangles[0].edgeArray     = (u8*) (mesh->triangleArray[0].neighborArray + mesh->totalTriNeighborCount);
	mesh->triangles[0].neighbors = View<u32> (mesh->triangles[0].neighborArray, mesh->triangles[0].neighborCount);
	mesh->triangles[0].edges     = View<u8>  (mesh->triangles[0].edgeArray, mesh->triangles[0].neighborCount);
	for(int ti=1; ti<mesh->triangles.count; ++ti){
		mesh->triangles[ti].neighborArray = (u32*)(mesh->triangles[ti-1].neighborArray + mesh->triangles[ti-1].neighborCount);
		mesh->triangles[ti].edgeArray     = (u8*) (mesh->triangles[ti-1].edgeArray + mesh->triangles[ti-1].neighborCount);
		mesh->triangles[ti].neighbors = View<u32> (mesh->triangles[ti].neighborArray, mesh->triangles[ti].neighborCount);
		mesh->triangles[ti].edges     = View<u8>  (mesh->triangles[ti].edgeArray, mesh->triangles[ti].neighborCount);
	}
	mesh->faces[0].triangleArray         = (u32*)(mesh->triangles[0].edgeArray         + mesh->totalTriNeighborCount);
	mesh->faces[0].vertexArray           = (u32*)(mesh->faces[0].triangleArray         + mesh->triangles.count);
	mesh->faces[0].outerVertexArray      = (u32*)(mesh->faces[0].vertexArray           + mesh->totalFaceVertexCount);
	mesh->faces[0].neighborTriangleArray = (u32*)(mesh->faces[0].outerVertexArray      + mesh->totalFaceOuterVertexCount);
	mesh->faces[0].neighborFaceArray     = (u32*)(mesh->faces[0].neighborTriangleArray + mesh->totalFaceTriNeighborCount);
	mesh->faces[0].triangles         = View<u32>(mesh->faces[0].triangleArray,         mesh->faces[0].triangleCount);
	mesh->faces[0].vertexes          = View<u32>(mesh->faces[0].vertexArray,           mesh->faces[0].vertexCount);
	mesh->faces[0].outerVertexes     = View<u32>(mesh->faces[0].outerVertexArray,      mesh->faces[0].outerVertexCount);
	mesh->faces[0].triangleNeighbors = View<u32>(mesh->faces[0].neighborTriangleArray, mesh->faces[0].neighborTriangleCount);
	mesh->faces[0].faceNeighbors     = View<u32>(mesh->faces[0].neighborFaceArray,     mesh->faces[0].neighborFaceCount);
	for(int fi=1; fi<mesh->faces.count; ++fi){
		mesh->faces[fi].triangleArray         = (u32*)(mesh->faces[fi-1].triangleArray         + mesh->faces[fi-1].triangleCount);
		mesh->faces[fi].vertexArray           = (u32*)(mesh->faces[fi-1].vertexArray           + mesh->faces[fi-1].vertexCount);
		mesh->faces[fi].outerVertexArray      = (u32*)(mesh->faces[fi-1].outerVertexArray      + mesh->faces[fi-1].outerVertexCount);
		mesh->faces[fi].neighborTriangleArray = (u32*)(mesh->faces[fi-1].neighborTriangleArray + mesh->faces[fi-1].neighborTriangleCount);
		mesh->faces[fi].neighborFaceArray     = (u32*)(mesh->faces[fi-1].neighborFaceArray     + mesh->faces[fi-1].neighborFaceCount);
		mesh->faces[fi].triangles         = View<u32>(mesh->faces[fi].triangleArray,         mesh->faces[fi].triangleCount);
		mesh->faces[fi].vertexes          = View<u32>(mesh->faces[fi].vertexArray,           mesh->faces[fi].vertexCount);
		mesh->faces[fi].outerVertexes     = View<u32>(mesh->faces[fi].outerVertexArray,      mesh->faces[fi].outerVertexCount);
		mesh->faces[fi].triangleNeighbors = View<u32>(mesh->faces[fi].neighborTriangleArray, mesh->faces[fi].neighborTriangleCount);
		mesh->faces[fi].faceNeighbors     = View<u32>(mesh->faces[fi].neighborFaceArray,     mesh->faces[fi].neighborFaceCount);
	}
	
	Render::LoadMesh(mesh);
	
	result.first  = mesh->idx;
	result.second = mesh;
	meshes.add(mesh);
	return result;
}

void Storage::
SaveMesh(Mesh* mesh){
	Assets::writeFileBinary(Assets::dirModels()+std::string(mesh->name)+".mesh", mesh, mesh->bytes);
	LOG("Successfully created ",mesh->name,".mesh");
}

void Storage::
DeleteMesh(Mesh* mesh){
	//!Incomplete
	Assert(!"not setup yet");
	free(mesh);
}

array<vec2> Storage::
GenerateMeshOutlinePoints(Mesh* mesh, mat4 transform, mat4 camProjection, mat4 camView, vec3 camPosition, vec2 screenDims){ //!FixMe
	array<vec2> outline;
	array<Mesh::Triangle*> nonculled;
	for(Mesh::Triangle& t :mesh->triangles){
		t.removed = false;
		if(t.normal.dot(camPosition - (t.p[0] * transform)) > 0){
			nonculled.add(&t);
		}else{
			t.removed = true;
		}
	}
	for(Mesh::Triangle* t : nonculled){
		forI(t->neighborCount){
			if(mesh->triangles[t->neighbors[i]].removed){
				outline.add(Math::WorldToScreen(t->p[ t->edges[i]       ]*transform, camProjection, camView, screenDims).toVec2());
				outline.add(Math::WorldToScreen(t->p[(t->edges[i]+1) % 3]*transform, camProjection, camView, screenDims).toVec2());
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

pair<u32,Texture*> Storage::
CreateTextureFromFile(const char* filename, ImageFormat format, TextureType type, bool keepLoaded, bool generateMipmaps){
	pair<u32,Texture*> result(0, NullTexture());
	if(strcmp(filename, "null") == 0) return result;
	
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
	texture->pixels  = stbi_load((Assets::dirTextures()+filename).c_str(), &texture->width, &texture->height, 
								 &texture->depth, STBI_rgb_alpha);
	texture->loaded  = true;
	if(texture->pixels == 0){ 
		ERROR_LOC("Failed to create texture '",filename,"': ",stbi_failure_reason()); 
		free(texture);
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

pair<u32,Texture*> Storage::
CreateTextureFromMemory(void* data, const char* name, int width, int height, ImageFormat format, TextureType type, bool keepLoaded, bool generateMipmaps){
	pair<u32,Texture*> result(0, NullTexture());
	if(data == 0){ ERROR_LOC("Failed to create texture '",name,"': No memory passed!"); return result; }
	
	//check if created already
	forI(textures.size()){ if(strcmp(textures[i]->name, name) == 0){
			return pair<u32,Texture*>(i,textures[i]);
		}
	}
	
	Texture* texture = AllocateTexture();
	cpystr(texture->name, name, DESHI_NAME_SIZE);
	texture->idx     = textures.count;
	texture->format  = format;
	texture->type    = type;
	texture->width   = width;
	texture->height  = height;
	texture->depth   = 4;
	texture->loaded  = true;
	texture->mipmaps = (generateMipmaps) ? (int)log2(Max(texture->width, texture->height)) + 1 : 1;
	
	//reinterpret image as RGBA32
	const u8* src = (u8*)data;
	if(format != ImageFormat_RGBA){
		texture->pixels = (u8*)malloc((size_t)width * (size_t)height * 4);
		data = texture->pixels;
		u32* dst = (u32*)texture->pixels;
		switch(format){
			case ImageFormat_BW:{
				for(int i = width*height; i > 0; i--){
					u32 value = (u32)(*src++);
					*dst++ = PACKCOLORU32(value, value, value, value);
				}
			}break;
			case ImageFormat_BWA:{
				//!Incomplete
				Assert(!"not setup yet");
			}break;
			case ImageFormat_RGB:{
				//!Incomplete
				Assert(!"not setup yet");
			}break;
		}
	}else{
		texture->pixels = (u8*)data;
	}
	
	Render::LoadTexture(texture);
	if(!keepLoaded){
		free(data);
		data = 0;
		texture->pixels = 0;
	}
	
	result.first  = texture->idx;
	result.second = texture;
	textures.add(texture);
	return result;
}

void Storage::
DeleteTexture(Texture* texture){
	//!Incomplete
	Assert(!"not setup yet");
	free(texture);
}


///////////////////
//// @material ////
///////////////////
local Material* 
AllocateMaterial(u32 textureCount){
	Material* material = (Material*)calloc(1, sizeof(Material));
	material->textures = array<u32>(textureCount);
	return material;
}

pair<u32,Material*> Storage::
CreateMaterial(const char* name, Shader shader, MaterialFlags flags, array<u32> mat_textures){
	pair<u32,Material*> result(0, NullMaterial());
	
	//check if created already
	forX(mi, materials.count){ if(strcmp(materials[mi]->name, name) == 0){ return pair<u32,Material*>(mi,materials[mi]); } }
	
	Material* material = AllocateMaterial(mat_textures.count);
	cpystr(material->name, name, DESHI_NAME_SIZE);
	material->idx = materials.count;
	material->shader = shader;
	material->flags  = flags;
	forI(mat_textures.count) material->textures.add(mat_textures[i]);
	
	Render::LoadMaterial(material);
	
	result.first  = material->idx;
	result.second = material;
	materials.add(material);
	return result;
}

pair<u32,Material*> Storage::
CreateMaterialFromFile(const char* filename, bool warnMissing){
	pair<u32,Material*> result(0, NullMaterial());
	if(strcmp(filename, "null") == 0) return result;
	
	//split filename into name and extension
	std::string filepath = Assets::dirModels()+filename;
	string fullname(filename);
	string name, extension;
	bool has_extension;
	size_t dot_idx = fullname.find_first_of_lookback('.', fullname.size-1);
	if(dot_idx != -1){
		has_extension = true;
		name = fullname.substr(0, dot_idx-1);
		extension = fullname.substr(dot_idx+1);
	}else{
		has_extension = false;
		name = fullname;
	}
	if(!has_extension){
		filepath += ".mat";
	}
	
	//check if created already
	forX(mi, materials.count){ if(strcmp(materials[mi]->name, name.str) == 0){ return pair<u32,Material*>(mi,materials[mi]); } }
	
	//material storage
	string mat_name;
	Shader mat_shader;
	MaterialFlags mat_flags;
	array<string> mat_textures;
	
	//parse .mat file
	enum MaterialHeader{ MATERIAL, TEXTURES, INVALID, };
	persist const char* MaterialHeaderStrings[] = { "MATERIAL", "TEXTURES", "INVALID", };
	u32 header = MaterialHeader::INVALID;
	
	char* buffer = Assets::readFileAsciiToArray(filepath, 0, true);
	if(!buffer){ return result; }
	defer{ delete[] buffer; };
	char* line_start;  char* line_end = buffer - 1;
	char* info_start;  char* info_end;
	char* key_start;   char* key_end;
	char* value_start; char* value_end;
	bool has_cr = false;
	for(u32 line_number = 1; ;line_number++){
		//get the next line
		line_start = (has_cr) ? line_end+2 : line_end+1;
		if((line_end = strchr(line_start, '\n')) == 0) break; //EOF if no '\n'
		if(has_cr || *(line_end-1) == '\r'){ has_cr = true; line_end -= 1; }
		if(line_start == line_end) continue;
		
		//format the line
		info_start = line_start + Utils::skipSpacesLeading(line_start, line_end-line_start);  if(info_start == line_end) continue;
		info_end   = info_start + Utils::skipComments(info_start, "#", line_end-info_start);  if(info_start == info_end) continue;
		info_end   = info_start + Utils::skipSpacesTrailing(info_start, info_end-info_start); if(info_start == info_end) continue;
		
		std::string line(info_start, info_end-info_start);
		
		//parse the key-value pair
		if(*info_start == '>'){
			if     (line == ">material"){ header = MaterialHeader::MATERIAL; }
			else if(line == ">textures"){ header = MaterialHeader::TEXTURES; }
			else{ header = MaterialHeader::INVALID; ParseError("Uknown header '",line,"'"); }
			continue;
		}
		
		if(header == MaterialHeader::INVALID) { ParseError("Invalid header; skipping line"); continue; }
		std::vector<std::string> split = Utils::spaceDelimitIgnoreStrings(line);
		
		if(header == MaterialHeader::MATERIAL){
			if(split.size() != 2){ ParseError("Material header attributes should have 2 values"); continue; }
			if      (split[0] == "name")  { 
				mat_name   = string(split[1].c_str(), split[1].size()); 
			}else if(split[0] == "shader"){ 
				forI(Shader_COUNT){ if(strcmp(ShaderStrings[i], split[1].c_str()) == 0){ mat_shader = i; break; } }
			}else if(split[0] == "flags") { 
				mat_flags  = (MaterialFlags)std::stoi(split[1]); 
			}
			else{ ParseError("Invalid key '",split[0],"' for header '",MaterialHeaderStrings[header],"'"); continue; }
		}else{
			mat_textures.add(string(split[0].c_str(), split[0].size()));
		}
	}
	
	Material* material = AllocateMaterial(mat_textures.count);
	cpystr(material->name, mat_name.str, DESHI_NAME_SIZE);
	material->idx = materials.count;
	material->shader = mat_shader;
	material->flags = mat_flags;
	forI(mat_textures.count) material->textures.add(CreateTextureFromFile(mat_textures[i].str).first);
	
	Render::LoadMaterial(material);
	
	result.first  = material->idx;
	result.second = material;
	materials.add(material);
	return result;
}

void Storage::
SaveMaterial(Material* material){
	std::string mat_text = TOSTDSTRING(">material"
									   "\nname   \"", material->name,"\""
									   "\nshader ", ShaderStrings[material->shader],
									   "\nflags  ", material->flags,
									   "\n"
									   "\n>textures");
	forI(material->textures.count){
		mat_text.append(TOSTDSTRING("\n\"",textures[material->textures[i]]->name,"\""));
	}
	mat_text.append("\n");
	Assets::writeFile(Assets::dirModels()+std::string(material->name)+".mat", mat_text.c_str(), mat_text.size());
	LOG("Successfully created ",material->name,".mat");
}

void Storage::
DeleteMaterial(Material* material){
	//!Incomplete
	Assert(!"not setup yet");
	free(material);
}


////////////////
//// @model ////
////////////////
local Model* 
AllocateModel(u32 batchCount){
	Model* model = (Model*)calloc(1, sizeof(Model));
	model->batches.resize((batchCount) ? batchCount : 1);
	return model;
}

pair<u32,Model*> Storage::
CreateModelFromFile(const char* filename, ModelFlags flags, bool forceLoadOBJ){
	pair<u32,Model*> result(0, NullModel());
	if(strcmp(filename, "null") == 0) return result;
	
	TIMER_START(t_m);
	bool has_extension;
	std::string filepath = Assets::dirModels() + filename;
	string fullname(filename);
	string name;
	string extension;
	
	size_t dot_idx = fullname.find_first_of_lookback('.', fullname.size-1);
	if(dot_idx != -1){
		has_extension = true;
		name = fullname.substr(0, dot_idx-1);
		extension = fullname.substr(dot_idx+1);
	}else{
		has_extension = false;
		name = fullname;
	}
	
	//check if model is already loaded
	forX(mi, models.count){
		if((strcmp(models[mi]->name, name.str) == 0)){
			return pair<u32,Model*>(mi,models[mi]);
		}
	}
	
	//!Incomplete check for .mesh and .model
	bool parse_obj_mesh  = true;
	bool parse_obj_model = true;
	if(!forceLoadOBJ){
		std::vector<std::string> files = Assets::iterateDirectory(Assets::dirModels());
		for(std::string& file : files){
			if(file == std::string(name.str)+".mesh"){
				
			}
		}
	}
	
	Mesh*  mesh  = NullMesh();
	Model* model = NullModel();
	
	//parse OBJ file
	if(parse_obj_mesh){ //load .obj and .mtl
		map<vec3,Mesh::Vertex> vUnique;
		Set<vec3> vnUnique;
		Set<pair<u32,string>> oUnique, gUnique, uUnique, mUnique; //index offset, name
		Set<pair<u32,vec3>> appliedUniqueNormals; //vertex applied on, normal
		array<vec2> vtArray; //NOTE UV vertices arent expected to be unique
		array<u32> vArray, vnArray, oArray, gArray,  uArray,  mArray; //index in unique array
		array<Mesh::Index>    indexes;
		array<Mesh::Triangle> triangles;
		array<Mesh::Face>     faces;
		array<array<pair<u32,u8>>> triNeighbors;
		array<array<u32>> faceTriangles;
		array<Set<u32>>   faceVertexes;
		array<array<u32>> faceOuterVertexes;
		array<array<u32>> faceTriNeighbors;
		array<array<u32>> faceFaceNeighbors;
		u32 totalTriNeighbors      = 0;
		u32 totalFaceVertexes      = 0;
		u32 totalFaceOuterVertexes = 0;
		u32 totalFaceTriNeighbors  = 0;
		u32 totalFaceFaceNeighbors = 0;
		vec3 aabb_min{ FLT_MAX, FLT_MAX, FLT_MAX};
		vec3 aabb_max{-FLT_MAX,-FLT_MAX,-FLT_MAX};
		u32 default_color = Color::PackColorU32(Color::WHITE);
		bool mtllib_found    = false;
		bool s_warning       = false;
		bool non_tri_warning = false;
		bool fatal_error     = false;
		
		TIMER_START(t_l);
		char* buffer = Assets::readFileAsciiToArray(filepath, 0, true);
		if(!buffer){  return result; }
		defer{ delete[] buffer; };
		char* line_start;
		char* line_end = buffer - 1;
		bool has_cr = false;
		for(u32 line_number = 1; ;line_number++){
			//get the next line
			line_start = (has_cr) ? line_end+2 : line_end+1;
			if((line_end = strchr(line_start, '\n')) == 0) break; //EOF if no '\n'
			
			//check for CRLF
			if(has_cr || *(line_end-1) == '\r') {
				has_cr = true;
				line_end -= 1;
			}
			
			//TODO(delle) add parsing safety checks to strtof and strol
			//TODO(delle) handle non-triangle faces (maybe)
			switch(*line_start){
				case '\0': case '\n': case '\r': case '#': case ' ': continue; //skip empty and comment lines
				//// vertex, normal, or uv ////
				case 'v':{
					switch(*(line_start+1)){
						
						//// vertex ////
						case ' ':{
							char* next = 0;
							f32 x = strtof(line_start+2, &next);
							f32 y = strtof(next, &next);
							f32 z = strtof(next, 0);
							vec3 vec{x,y,z};
							vArray.add(vUnique.add(vec, Mesh::Vertex{vec}));
						}continue;
						
						//// uv ////
						case 't':{
							if(*(line_start+2) != ' '){ ParseError("No space after 'vt'"); return result; }
							char* next = 0;
							f32 x = strtof(line_start+2, &next);
							f32 y = strtof(next, 0);
							vtArray.add(vec2{x,y});
						}continue;
						
						//// normal ////
						case 'n':{
							if(*(line_start+2) != ' '){ ParseError("No space after 'vn'"); return result; }
							char* next = 0;
							f32 x = strtof(line_start+2, &next);
							f32 y = strtof(next, &next);
							f32 z = strtof(next, 0);
							vec3 vec{x,y,z};
							vnArray.add(vnUnique.add(vec, vec));
						}continue;
						default:{
							ParseError("Invalid character after 'v': '",*(line_start+1),"'");
						}return result;
					}
				}continue;
				
				//// face ////
				case 'f':{
					TIMER_START(t_f);
					if(*(line_start+1) != ' '){ ParseError("No space after 'f'"); return result; }
					if(vArray.count == 0){ ParseError("Specifier 'f' before any 'v'"); return result; }
					
					char* next = line_start+1;
					u32 v0=strtol(next,&next,10)-1; u32 vt0=strtol(next+1,&next,10)-1; u32 vn0=strtol(next+1,&next,10)-1;
					u32 v1=strtol(next,&next,10)-1; u32 vt1=strtol(next+1,&next,10)-1; u32 vn1=strtol(next+1,&next,10)-1;
					u32 v2=strtol(next,&next,10)-1; u32 vt2=strtol(next+1,&next,10)-1; u32 vn2=strtol(next+1,&next,10)-1;
					u32 o = oArray.count; u32 g = gArray.count; u32 m = mArray.count;
					
					//index
					indexes.add(vArray[v0]);
					indexes.add(vArray[v1]);
					indexes.add(vArray[v2]);
					
					//uv
					if(vtArray.count){
						vUnique.data[vArray[v0]].uv = vtArray[vt0];
						vUnique.data[vArray[v1]].uv = vtArray[vt1];
						vUnique.data[vArray[v2]].uv = vtArray[vt2];
					}
					
					//normal
					if(vnArray.count){
						if(appliedUniqueNormals.add({vArray[v0],vnUnique.data[vnArray[vn0]]}) == appliedUniqueNormals.count-1){
							vUnique.data[vArray[v0]].normal += vnUnique.data[vnArray[vn0]];
						}
						if(appliedUniqueNormals.add({vArray[v1],vnUnique.data[vnArray[vn1]]}) == appliedUniqueNormals.count-1){
							vUnique.data[vArray[v1]].normal += vnUnique.data[vnArray[vn1]];
						}
						if(appliedUniqueNormals.add({vArray[v2],vnUnique.data[vnArray[vn2]]}) == appliedUniqueNormals.count-1){
							vUnique.data[vArray[v2]].normal += vnUnique.data[vnArray[vn2]];
						}
					}
					
					//color
					vUnique.data[vArray[v0]].color = default_color;
					vUnique.data[vArray[v1]].color = default_color;
					vUnique.data[vArray[v2]].color = default_color;
					
					//triangle
					u32 cti = triangles.count;
					triangles.add(Mesh::Triangle{
									  (vUnique.data[vArray[v0]].pos - vUnique.data[vArray[v1]].pos)
										  .cross(vUnique.data[vArray[v0]].pos - vUnique.data[vArray[v2]].pos).normalized(),
									  vUnique.data[vArray[v0]].pos, vUnique.data[vArray[v1]].pos, vUnique.data[vArray[v2]].pos,
									  vArray[v0], vArray[v1], vArray[v2],
									  0, (u32)-1
								  });
					triNeighbors.add(array<pair<u32,u8>>());
					
					//triangle neighbors
					for(u32 oti=0; oti<triangles.count-1; ++oti){
						//check that its not already a neighbor
						bool neighbor_already = false;
						for(u32 tni=0; tni<triNeighbors[oti].count; ++tni){
							if(triNeighbors[oti][tni].first == cti){ neighbor_already = true; break; }
						}
						
						//check for shared vertexes and mark the edges
						if(!neighbor_already){
							bool ct0_ot0 = (vArray[v0] == triangles[oti].v[0]);
							bool ct0_ot1 = (vArray[v0] == triangles[oti].v[1]);
							bool ct0_ot2 = (vArray[v0] == triangles[oti].v[2]);
							bool ct1_ot0 = (vArray[v1] == triangles[oti].v[0]);
							bool ct1_ot1 = (vArray[v1] == triangles[oti].v[1]);
							bool ct1_ot2 = (vArray[v1] == triangles[oti].v[2]);
							bool ct2_ot0 = (vArray[v2] == triangles[oti].v[0]);
							bool ct2_ot1 = (vArray[v2] == triangles[oti].v[1]);
							bool ct2_ot2 = (vArray[v2] == triangles[oti].v[2]);
							
							//current tri v0 && v1
							if(ct0_ot0 && ct1_ot0){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct0_ot0 && ct1_ot1){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct0_ot0 && ct1_ot2){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct0_ot1 && ct1_ot0){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
							if(ct0_ot1 && ct1_ot1){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
							if(ct0_ot1 && ct1_ot2){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
							if(ct0_ot2 && ct1_ot0){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
							if(ct0_ot2 && ct1_ot1){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
							if(ct0_ot2 && ct1_ot2){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
							//current tri v1 && v2
							if(ct1_ot0 && ct2_ot0){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct1_ot0 && ct2_ot1){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct1_ot0 && ct2_ot2){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct1_ot1 && ct2_ot0){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
							if(ct1_ot1 && ct2_ot1){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
							if(ct1_ot1 && ct2_ot2){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
							if(ct1_ot2 && ct2_ot0){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
							if(ct1_ot2 && ct2_ot1){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
							if(ct1_ot2 && ct2_ot2){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
							//current tri v2 && v0
							if(ct2_ot0 && ct0_ot0){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct2_ot0 && ct0_ot1){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct2_ot0 && ct0_ot2){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct2_ot1 && ct0_ot0){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
							if(ct2_ot1 && ct0_ot1){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
							if(ct2_ot1 && ct0_ot2){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
							if(ct2_ot2 && ct0_ot0){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
							if(ct2_ot2 && ct0_ot1){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
							if(ct2_ot2 && ct0_ot2){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
						}
					}
					triangles[cti].neighborCount = triNeighbors[cti].count;
					if(((u64)(TIMER_END(t_l) / 1000.0) % 10 == 0) && ((u64)(TIMER_END(t_l) / 1000.0) != 0)){
						PRINTLN(TOSTRING(filename," face ",triangles.count," on line ",line_number,
										 "finished creation in ",TIMER_END(t_f),"ms"));
					}
				}continue;
				
				//// use material ////
				case 'u':{ //use material
					if(strncmp(line_start, "usemtl ", 7) != 0){ ParseError("Specifier started with 'u' but didn't equal 'usemtl '"); return result; }
					if(mtllib_found){
						pair<u32,string> usemtl(indexes.count, string(line_start+7, line_end-(line_start+7)));
						uArray.add(uUnique.add(usemtl,usemtl));
					}else{
						ParseError("Specifier 'usemtl' used before 'mtllib' specifier");
					}
				}continue;
				
				//// load material ////
				case 'm':{
					if(strncmp(line_start, "mtllib ", 7) != 0){ ParseError("Specifier started with 'm' but didn't equal 'mtllib '"); return result; }
					mtllib_found = true;
					pair<u32,string> mtllib(indexes.count, string(line_start+7, line_end-(line_start+7)));
					mArray.add(mUnique.add(mtllib,mtllib));
				}continue;
				
				//// group (batch) ////
				case 'g':{
					if(*(line_start+1) != ' '){ ParseError("No space after 'g'"); return result; }
					pair<u32,string> group(indexes.count, string(line_start+2, line_end-(line_start+2)));
					gArray.add(gUnique.add(group,group));
				}continue;
				
				//// object ////
				case 'o':{
					if(*(line_start+1) != ' '){ ParseError("No space after 'o'"); return result; }
					pair<u32,string> object(indexes.count, string(line_start+2, line_end-(line_start+2)));
					oArray.add(oUnique.add(object,object));
				}continue;
				
				//// smoothing ////
				case 's':{
					if(*(line_start+1) != ' '){ ParseError("No space after 's'"); return result; }
					s_warning = true;
				}continue;
				default:{
					ParseError("Invalid starting character: '",*line_start,"'");
				}return result;
			}
		}
		
		//// generate mesh faces ////
		forX(cti, triangles.count){
			u32 cfi = faces.count;
			if(triangles[cti].face == -1){
				faces.add(Mesh::Face{});
				faceTriangles.add(array<u32>());
				faceVertexes.add(Set<u32>());
				faceOuterVertexes.add(array<u32>());
				faceTriNeighbors.add(array<u32>());
				faceFaceNeighbors.add(array<u32>());
				
				faces[cfi].normal = triangles[cti].normal;
				triangles[cti].face = cfi;
				faceTriangles[cfi].add(cti);
				faceVertexes[cfi].add(triangles[cti].v[0],triangles[cti].v[0]); faces[cfi].center += triangles[cti].p[0];
				faceVertexes[cfi].add(triangles[cti].v[1],triangles[cti].v[1]); faces[cfi].center += triangles[cti].p[1];
				faceVertexes[cfi].add(triangles[cti].v[2],triangles[cti].v[2]); faces[cfi].center += triangles[cti].p[2];
				totalFaceVertexes += 3;
			}else{
				cfi = triangles[cti].face;
			}
			
			forX(ctni, triNeighbors[cti].count){
				u32 oti = triNeighbors[cti][ctni].first;
				if(triangles[oti].face == triangles[cti].face) continue;
				
				//check if normals are the same
				if(triangles[cti].normal == triangles[oti].normal){
					triangles[oti].face = cfi;
					faceTriangles[cfi].add(oti);
					faceVertexes[cfi].add(triangles[oti].v[0],triangles[oti].v[0]); faces[cfi].center += triangles[oti].p[0];
					faceVertexes[cfi].add(triangles[oti].v[1],triangles[oti].v[1]); faces[cfi].center += triangles[oti].p[1];
					faceVertexes[cfi].add(triangles[oti].v[2],triangles[oti].v[2]); faces[cfi].center += triangles[oti].p[2];
					totalFaceVertexes += 3;
				}else{ //if not, add vertexes to face's outer
					u32 v1 = triangles[cti].v[ triNeighbors[cti][ctni].second       ];
					u32 v2 = triangles[cti].v[(triNeighbors[cti][ctni].second+1) % 3];
					bool v1_already = false; bool v2_already = false;
					forX(fovi, faceOuterVertexes[cfi].count){
						if(!v1_already && faceOuterVertexes[cfi][fovi] == v1){ v1_already = true; }
						if(!v2_already && faceOuterVertexes[cfi][fovi] == v2){ v2_already = true; }
						if(v1_already && v2_already) break;
					}
					if(!v1_already){ faceOuterVertexes[cfi].add(v1); totalFaceOuterVertexes++; }
					if(!v2_already){ faceOuterVertexes[cfi].add(v2); totalFaceOuterVertexes++; }
				}
			}
		}
		
		//// calculate vertex normals ////
		forI(vUnique.count){
			aabb_min.x = Min(aabb_min.x, vUnique.data.data[i].pos.x);
			aabb_max.x = Max(aabb_max.x, vUnique.data.data[i].pos.x);
			aabb_min.y = Min(aabb_min.y, vUnique.data.data[i].pos.y);
			aabb_max.y = Max(aabb_max.y, vUnique.data.data[i].pos.y);
			aabb_min.z = Min(aabb_min.z, vUnique.data.data[i].pos.z);
			aabb_max.z = Max(aabb_max.z, vUnique.data.data[i].pos.z);
			vUnique.data.data[i].normal.normalize();
		}
		
		//parsing warnings/errors
		if(non_tri_warning) WARNING("The mesh was not triangulated before parsing; Expect missing triangles!");
		if(s_warning) WARNING("There were 's' specifiers when parsing ",filename,", but those are not evaluated currently");
		if(!vtArray.count){ WARNING_LOC("No vertex UVs 'vt' were parsed in ",filename); }
		if(!vnArray.count){ WARNING_LOC("No vertex normals 'vn' were parsed in ",filename); }
		if(fatal_error){ ERROR_LOC("OBJ parsing encountered a fatal error in ",filename); return result; }
		if(!vArray.count){ ERROR_LOC("No vertex positions 'v' were parsed in ",filename); return result; }
		if(!triangles.count){ ERROR_LOC("No faces 'f' were parsed in ",filename); return result; }
		
		//// create mesh ////
		mesh = AllocateMesh(indexes.count, vUnique.count, faces.count, totalTriNeighbors, 
							totalFaceVertexes, totalFaceOuterVertexes, totalFaceTriNeighbors, totalFaceFaceNeighbors);
		//fill base arrays
		cpystr(mesh->name, name.str, DESHI_NAME_SIZE);
		mesh->idx = meshes.count;
		mesh->aabbMin  = aabb_min;
		mesh->aabbMax  = aabb_max;
		mesh->center   = {(aabb_max.x+aabb_min.x)/2.0f, (aabb_max.y+aabb_min.y)/2.0f, (aabb_max.z+aabb_min.z)/2.0f};
		memcpy(mesh->vertexArray,   vUnique.data.data, vUnique.count*sizeof(Mesh::Vertex));
		memcpy(mesh->indexArray,    indexes.data,      indexes.count*sizeof(Mesh::Index));
		memcpy(mesh->triangleArray, triangles.data,    triangles.count*sizeof(Mesh::Triangle));
		memcpy(mesh->faceArray,     faces.data,        faces.count*sizeof(Mesh::Face));
		
		//setup pointers
		mesh->triangles[0].neighborArray = (u32*)(mesh->faceArray + mesh->faceCount);
		mesh->triangles[0].edgeArray     = (u8*)(mesh->triangleArray[0].neighborArray + totalTriNeighbors);
		mesh->triangles[0].neighbors = View<u32>(mesh->triangles[0].neighborArray, triNeighbors[0].count);
		mesh->triangles[0].edges     = View<u8>(mesh->triangles[0].edgeArray, triNeighbors[0].count);
		for(int ti=1; ti<mesh->triangles.count; ++ti){
			mesh->triangles[ti].neighborArray = (u32*)(mesh->triangles[ti-1].neighborArray + triNeighbors[ti-1].count);
			mesh->triangles[ti].edgeArray     = (u8*)(mesh->triangles[ti-1].edgeArray + triNeighbors[ti-1].count);
			mesh->triangles[ti].neighbors = View<u32>(mesh->triangles[ti].neighborArray, triNeighbors[ti].count);
			mesh->triangles[ti].edges     = View<u8>(mesh->triangles[ti].edgeArray, triNeighbors[ti].count);
		}
		mesh->faces[0].triangleArray         = (u32*)(mesh->triangles[0].edgeArray         + totalTriNeighbors);
		mesh->faces[0].vertexArray           = (u32*)(mesh->faces[0].triangleArray         + triangles.count);
		mesh->faces[0].outerVertexArray      = (u32*)(mesh->faces[0].vertexArray           + totalFaceVertexes);
		mesh->faces[0].neighborTriangleArray = (u32*)(mesh->faces[0].outerVertexArray      + totalFaceOuterVertexes);
		mesh->faces[0].neighborFaceArray     = (u32*)(mesh->faces[0].neighborTriangleArray + totalFaceTriNeighbors);
		mesh->faces[0].triangles         = View<u32>(mesh->faces[0].triangleArray,         faceTriangles[0].count);
		mesh->faces[0].vertexes          = View<u32>(mesh->faces[0].vertexArray,           faceVertexes[0].count);
		mesh->faces[0].outerVertexes     = View<u32>(mesh->faces[0].outerVertexArray,      faceOuterVertexes[0].count);
		mesh->faces[0].triangleNeighbors = View<u32>(mesh->faces[0].neighborTriangleArray, faceTriNeighbors[0].count);
		mesh->faces[0].faceNeighbors     = View<u32>(mesh->faces[0].neighborFaceArray,     faceFaceNeighbors[0].count);
		for(int fi=1; fi<mesh->faces.count; ++fi){
			mesh->faces[fi].triangleArray         = (u32*)(mesh->faces[fi-1].triangleArray         + faceTriangles[fi-1].count);
			mesh->faces[fi].vertexArray           = (u32*)(mesh->faces[fi-1].vertexArray           + faceVertexes[fi-1].count);
			mesh->faces[fi].outerVertexArray      = (u32*)(mesh->faces[fi-1].outerVertexArray      + faceOuterVertexes[fi-1].count);
			mesh->faces[fi].neighborTriangleArray = (u32*)(mesh->faces[fi-1].neighborTriangleArray + faceTriNeighbors[fi-1].count);
			mesh->faces[fi].neighborFaceArray     = (u32*)(mesh->faces[fi-1].neighborFaceArray     + faceFaceNeighbors[fi-1].count);
			mesh->faces[fi].triangles         = View<u32>(mesh->faces[fi].triangleArray,         faceTriangles[fi].count);
			mesh->faces[fi].vertexes          = View<u32>(mesh->faces[fi].vertexArray,           faceVertexes[fi].count);
			mesh->faces[fi].outerVertexes     = View<u32>(mesh->faces[fi].outerVertexArray,      faceOuterVertexes[fi].count);
			mesh->faces[fi].triangleNeighbors = View<u32>(mesh->faces[fi].neighborTriangleArray, faceTriNeighbors[fi].count);
			mesh->faces[fi].faceNeighbors     = View<u32>(mesh->faces[fi].neighborFaceArray,     faceFaceNeighbors[fi].count);
		}
		
		//fill triangle neighbors/edges
		forX(ti, triangles.count){
			forX(ni, triNeighbors[ti].count){
				mesh->triangleArray[ti].neighborArray[ni] = triNeighbors[ti][ni].first;
				mesh->triangleArray[ti].edgeArray[ni]     = triNeighbors[ti][ni].second;
			}
			mesh->triangleArray[ti].neighborCount = triNeighbors[ti].count;
		}
		
		//fill face tris/vertexes/neighbors
		forX(fi, mesh->faces.count){
			mesh->faceArray[fi].triangleCount = faceTriangles[fi].count;
			mesh->faceArray[fi].vertexCount   = faceVertexes[fi].count;
			mesh->faceArray[fi].outerVertexCount = faceOuterVertexes[fi].count;
			mesh->faceArray[fi].center = faces[fi].center / (f32)faceVertexes[fi].count;
			forX(fti, mesh->faces[fi].triangles.count){
				mesh->faceArray[fi].triangleArray[fti] = faceTriangles[fi][fti];
			}
			forX(fvi, mesh->faces[fi].vertexes.count){
				mesh->faceArray[fi].vertexArray[fvi] = faceVertexes[fi].data[fvi];
			}
			forX(fvi, mesh->faces[fi].outerVertexes.count){
				mesh->faceArray[fi].outerVertexArray[fvi] = faceOuterVertexes[fi][fvi];
			}
		}
		
		Render::LoadMesh(mesh); //TODO(delle) check if mesh already loaded
		meshes.add(mesh);
		LOG("Parsing and loading OBJ '",filename,"' took ",TIMER_END(t_l),"ms");
		
		//parse MTL files
		if(mtllib_found){
			TIMER_RESET(t_l);
			
			//!Incomplete
			
			LOG("Parsing and loading MTLs for OBJ '",filename,"' took ",TIMER_END(t_l),"ms");
		}
		
		model = AllocateModel(mArray.count);
		cpystr(model->name,name.str, DESHI_NAME_SIZE);
		model->idx = models.count;
		model->flags = flags;
		model->mesh     = mesh;
		model->armature = 0;
		
		//!Incomplete batch materials
		if(mArray.count > 1){
			model->batches[mArray.count-1].indexOffset = mUnique.data[mArray[mArray.count-1]].first;
			model->batches[mArray.count-1].indexCount  = indexes.count - model->batches[mArray.count-1].indexOffset;
			model->batches[mArray.count-1].material    = 0;
			for(u32 bi = mArray.count-2; bi >= 0; --bi){
				model->batches[bi].indexOffset = mUnique.data[mArray[bi]].first;
				model->batches[bi].indexCount  = model->batches[bi+1].indexOffset - model->batches[bi].indexOffset;
				model->batches[bi].material    = 0;
			}
		}else{
			model->batches[0].indexOffset = 0;
			model->batches[0].indexCount  = indexes.count;
			model->batches[0].material    = 0;
		}
	}else if(parse_obj_model){ //load .obj (batch info only), .mtl, and .mesh
		//!Incomplete
	}else{ //load .model and .mesh
		//!Incomplete
	}
	
	result.first  = model->idx;
	result.second = model;
	models.add(model);
	SUCCESS("Finished loading model '",filename,"' in ",TIMER_END(t_m),"ms");
	return result;
}

pair<u32,Model*> Storage::
CreateModelFromMesh(Mesh* mesh, ModelFlags flags){
	pair<u32,Model*> result(0, NullModel());
	
	string model_name(mesh->name);
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
	model->armature = 0;
	model->batches[0].indexOffset = 0;
	model->batches[0].indexCount = mesh->indexCount;
	model->batches[0].material = 0;
	
	result.first  = model->idx;
	result.second = model;
	models.add(model);
	return result;
}

pair<u32,Model*> Storage::
CopyModel(Model* _model){
	pair<u32,Model*> result(0, NullModel());
	
	Model* model = AllocateModel(_model->batches.size());
	cpystr(model->name, _model->name, DESHI_NAME_SIZE);
	model->idx      = models.size();
	model->flags    = _model->flags;
	model->mesh     = _model->mesh;
	model->armature = _model->armature;
	forI(model->batches.size()){
		model->batches[i].indexOffset = _model->batches[i].indexOffset;
		model->batches[i].indexCount  = _model->batches[i].indexCount;
		model->batches[i].material    = _model->batches[i].material;
	}
	
	result.first  = model->idx;
	result.second = model;
	models.add(model);
	return result;
}

void Storage::
SaveModel(Model* model){
	std::string model_save = TOSTDSTRING(">model"
										 "\nname     \"",model->name,"\""
										 "\nflags    ", model->flags,
										 "\nmesh     \"", model->mesh->name,"\""
										 "\narmature ", 0,
										 "\n"
										 "\n>batches");
	forI(model->batches.count){
		model_save.append(TOSTDSTRING("\n\"",materials[model->batches[i].material]->name,"\" ",
									  model->batches[i].indexOffset," ",model->batches[i].indexCount));
	}
	model_save.append("\n");
	Assets::writeFile(Assets::dirModels()+std::string(model->name)+".model", model_save.c_str(), model_save.size());
	LOG("Successfully created ",model->name,".model");
}

void Storage::
DeleteModel(Model* model){
	//!Incomplete
	Assert(!"not setup yet");
	free(model);
}

#undef ParseError
#ifdef TempParseError
#define ParseError TempParseError
#undef TempParseError
#endif
