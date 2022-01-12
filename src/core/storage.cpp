#define ParseError(...) LogE("storage","Failed parsing '",filepath,"' on line '",line_number,"'! ",__VA_ARGS__)

namespace Storage{
	local u8 null128_png[] = { //TODO(delle) fix this
		0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44,0x52,
		0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x08,0x02,0x00,0x00,0x00,0x4C,0x5C,0xF6,
		0x9C,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x2E,0x23,0x00,0x00,0x2E,
		0x23,0x01,0x78,0xA5,0x3F,0x76,0x00,0x00,0x01,0x04,0x49,0x44,0x41,0x54,0x78,0xDA,
		0xED,0xD5,0xB1,0x11,0xC0,0x20,0x10,0x04,0xB1,0xC5,0x7D,0x51,0x3D,0x85,0xE1,0x8C,
		0x12,0x20,0xD1,0x46,0x97,0x33,0xBC,0xDA,0xAD,0xDD,0xAA,0x2A,0xFB,0xFE,0xFE,0xD2,
		0xD3,0xC6,0x79,0x93,0x6A,0x34,0xED,0xCB,0xDB,0x0F,0x78,0x9D,0x5B,0xCC,0x00,0x06,
		0xB8,0xC5,0x0C,0x60,0x80,0xCD,0x00,0x06,0xB8,0xCB,0x0C,0x60,0x80,0xCD,0x00,0x06,
		0xD8,0x0C,0x60,0x80,0xCD,0x00,0x06,0xD8,0x0C,0x60,0x80,0xCD,0x00,0x06,0xD8,0x0C,
		0x60,0x80,0xCD,0x00,0x06,0xD8,0x0C,0x60,0x80,0xCD,0x00,0x06,0xD8,0x0C,0x60,0x80,
		0xCD,0x00,0x06,0xD8,0x0C,0x60,0x80,0xCD,0x00,0x06,0xD8,0x0C,0x60,0x80,0xCD,0x00,
		0x06,0xD8,0x0C,0x60,0x80,0xCD,0x00,0x06,0xD8,0x0C,0x60,0x80,0xCD,0x00,0x06,0xD8,
		0x0C,0x60,0x80,0xCD,0x00,0x06,0xD8,0x0C,0x60,0x80,0xCD,0x00,0x06,0xD8,0x0C,0x60,
		0x80,0xCD,0x00,0x06,0xD8,0x0C,0x60,0x80,0xCD,0x00,0x06,0xD8,0x7E,0x00,0x03,0x18,
		0x20,0x06,0x30,0x40,0x0C,0x60,0x80,0x18,0xC0,0x00,0x31,0x80,0x01,0x62,0x00,0x03,
		0xC4,0x00,0x06,0x88,0x01,0x0C,0x10,0x03,0x18,0x20,0x06,0x30,0x40,0x0C,0x60,0x80,
		0x18,0xC0,0x00,0x31,0x80,0x01,0x62,0x00,0x03,0xC4,0x00,0x06,0x88,0x01,0x0C,0x10,
		0x03,0x18,0x20,0x06,0x30,0x40,0x0C,0x60,0x80,0x18,0xC0,0x00,0x31,0x80,0x01,0x62,
		0x00,0x03,0xC4,0x00,0x06,0x88,0x01,0x0C,0x10,0x03,0x18,0x20,0x06,0x30,0x40,0x0C,
		0x60,0x80,0x18,0xC0,0x00,0x31,0x80,0x01,0x62,0x00,0x03,0x74,0xFA,0x01,0xD7,0x41,
		0x0A,0x2B,0x59,0xA3,0x46,0xD2,0x00,0x00,0x00,0x00,0x49,0x45,0x4E,0x44,0xAE,0x42,
		0x60,0x82 
	};
	
	local array<Mesh*>&     meshes    = DeshStorage->meshes;
	local array<Texture*>&  textures  = DeshStorage->textures;
	local array<Material*>& materials = DeshStorage->materials;
	local array<Model*>&    models    = DeshStorage->models;
	local array<Font*>&     fonts     = DeshStorage->fonts;
	local array<Light*>&    lights    = DeshStorage->lights;
};

///////////////
//// @init ////
///////////////
void Storage::
Init(){
	AssertDS(DS_MEMORY, "Attempt to load Storage without loading Memory first");
	deshiStage |= DS_STORAGE;
	
	TIMER_START(t_s);
	
	stbi_set_flip_vertically_on_load(true);
	//setup null assets      //TODO(delle) store null.png and null shader in a .cpp
	DeshStorage->null_mesh     = CreateBoxMesh(1.0f, 1.0f, 1.0f).second; cpystr(NullMesh()->name, "null", DESHI_NAME_SIZE);
	//DeshStorage->null_texture  = CreateTextureFromMemory(stbi_load_from_memory(null128_png, 338, 0, 0, 0, STBI_rgb_alpha), "null", 128, 128, ImageFormat_RGBA),second;
	DeshStorage->null_texture  = CreateTextureFromFile("null128.png").second;
	DeshStorage->null_material = CreateMaterial("null", Shader_NULL, MaterialFlags_NONE, {0}).second;
	DeshStorage->null_model    = CreateModelFromMesh(NullMesh(), ModelFlags_NONE).second; cpystr(DeshStorage->null_model->name, "null", DESHI_NAME_SIZE);
	
	//create null font (white square)
	DeshStorage->null_font     = (Font*)memory_alloc(sizeof(Font));
	fonts.add(DeshStorage->null_font);
	NullFont()->type = FontType_NONE;
	NullFont()->idx = 0;
	NullFont()->max_width = 6;
	NullFont()->max_height = 12;
	NullFont()->count = 1;
	cpystr(NullFont()->name,"null",DESHI_NAME_SIZE);
	u8 white_pixels[4] = {255,255,255,255};
	Texture* nf_tex = CreateTextureFromMemory(&white_pixels, "null_font", 2, 2, ImageFormat_BW, TextureType_2D, TextureFilter_Nearest, TextureAddressMode_ClampToWhite, false, false).second;
	
	
	//DeleteTexture(nf_tex); //!Incomplete
	
	LogS("deshi","Finished storage initialization in ",TIMER_END(t_s),"ms");
}

void Storage::
Reset(){
	for(s32 i=meshes.size()-1;    i>0; --i){ DeleteMesh(meshes[i]);        meshes.pop(); } 
	for(s32 i=materials.size()-1; i>0; --i){ DeleteMaterial(materials[i]); materials.pop(); } 
	for(s32 i=textures.size()-1;  i>0; --i){ DeleteTexture(textures[i]);   textures.pop(); } 
	for(s32 i=models.size()-1;    i>0; --i){ DeleteModel(models[i]);       models.pop(); } 
	for(s32 i=fonts.size()-1;     i>0; --i){ DeleteFont(fonts[i]);         fonts.pop(); } 
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
	
	Mesh* mesh = (Mesh*)memory_alloc(bytes);   char* cursor = (char*)mesh + (1*sizeof(Mesh));
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
	mesh->indexes   = {mesh->indexArray,    indexCount};
	mesh->vertexes  = {mesh->vertexArray,   vertexCount};
	mesh->triangles = {mesh->triangleArray, triangleCount};
	mesh->faces     = {mesh->faceArray,     faceCount};
	return mesh;
}

//TODO(delle) change this to take in 8 points
pair<u32,Mesh*> Storage::
CreateBoxMesh(f32 width, f32 height, f32 depth, color color){
	width /= 2.f; height /= 2.f; depth /= 2.f;
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
	u32 c = color.rgba;
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
	for(s32 i=0; i<12; ++i){
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
	ta[0].neighbors = {ta[0].neighborArray, 3};
	ta[0].edges     = {ta[0].edgeArray,     3};
	for(s32 i=1; i<12; ++i){
		ta[i].neighborArray = ta[i-1].neighborArray+3;
		ta[i].edgeArray     = ta[i-1].edgeArray+3;
		ta[i].neighbors = {ta[i].neighborArray, 3};
		ta[i].edges     = {ta[i].edgeArray,     3};
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
	for(s32 i=0; i<6; ++i){
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
	fa[0].triangles = {fa[0].triangleArray, 2};
	for(s32 i=1; i<6; ++i){
		fa[i].triangleArray = fa[i-1].triangleArray+2;
		fa[i].triangles = {fa[i].triangleArray, 2};
	}
	
	//face array triangle arrays
	for(s32 i=0; i<6; ++i){
		fa[i].triangleArray[0]= i*2;
		fa[i].triangleArray[1]=(i*2)+1;
	}
	
	//face array vertex array offsets
	fa[0].vertexArray      = (u32*)(fa[0].triangleArray+12);
	fa[0].outerVertexArray = (u32*)(fa[0].vertexArray+24);
	fa[0].vertexes      = {fa[0].vertexArray,      4};
	fa[0].outerVertexes = {fa[0].outerVertexArray, 4};
	for(s32 i=1; i<6; ++i){
		fa[i].vertexArray      = fa[i-1].vertexArray+4;
		fa[i].outerVertexArray = fa[i-1].outerVertexArray+4;
		fa[i].vertexes      = {fa[i].vertexArray,      4};
		fa[i].outerVertexes = {fa[i].outerVertexArray, 4};
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
	fa[0].triangleNeighbors = {fa[0].neighborTriangleArray, 4};
	fa[0].faceNeighbors     = {fa[0].neighborFaceArray, 4};
	for(s32 i=1; i<6; ++i){
		fa[i].neighborTriangleArray = fa[i-1].neighborTriangleArray+4;
		fa[i].neighborFaceArray     = fa[i-1].neighborFaceArray+4;
		fa[i].triangleNeighbors = {fa[i].neighborTriangleArray, 4};
		fa[i].faceNeighbors     = {fa[i].neighborFaceArray, 4};
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
	b32 has_extension;
	u32 dot_idx = fullname.findLastChar('.');
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
		LogE("storage","Mesh size was too small when trying to load it from memory");
		return result;
	}
	
	//allocate
	Mesh* mesh = (Mesh*)memory_alloc(bytes);  char* cursor = (char*)mesh + (1*sizeof(Mesh));
	memcpy(mesh, data, bytes);
	mesh->idx = meshes.count;
	mesh->vertexArray   = (Mesh::Vertex*)cursor;       cursor +=   mesh->vertexCount*sizeof(Mesh::Vertex);
	mesh->indexArray    = (Mesh::Index*)cursor;        cursor +=    mesh->indexCount*sizeof(Mesh::Index);
	mesh->triangleArray = (Mesh::Triangle*)cursor;     cursor += mesh->triangleCount*sizeof(Mesh::Triangle);
	mesh->faceArray     = (Mesh::Face*)cursor;         cursor +=     mesh->faceCount*sizeof(Mesh::Face);
	mesh->indexes   = {mesh->indexArray,    mesh->indexCount};
	mesh->vertexes  = {mesh->vertexArray,   mesh->vertexCount};
	mesh->triangles = {mesh->triangleArray, mesh->triangleCount};
	mesh->faces     = {mesh->faceArray,     mesh->faceCount};
	mesh->triangles[0].neighborArray = (u32*)(mesh->faceArray + mesh->faceCount);
	mesh->triangles[0].edgeArray     = (u8*) (mesh->triangleArray[0].neighborArray + mesh->totalTriNeighborCount);
	mesh->triangles[0].neighbors = {mesh->triangles[0].neighborArray, mesh->triangles[0].neighborCount};
	mesh->triangles[0].edges     = {mesh->triangles[0].edgeArray, mesh->triangles[0].neighborCount};
	for(s32 ti=1; ti<mesh->triangles.count; ++ti){
		mesh->triangles[ti].neighborArray = (u32*)(mesh->triangles[ti-1].neighborArray + mesh->triangles[ti-1].neighborCount);
		mesh->triangles[ti].edgeArray     = (u8*) (mesh->triangles[ti-1].edgeArray + mesh->triangles[ti-1].neighborCount);
		mesh->triangles[ti].neighbors = {mesh->triangles[ti].neighborArray, mesh->triangles[ti].neighborCount};
		mesh->triangles[ti].edges     = {mesh->triangles[ti].edgeArray, mesh->triangles[ti].neighborCount};
	}
	mesh->faces[0].triangleArray         = (u32*)(mesh->triangles[0].edgeArray         + mesh->totalTriNeighborCount);
	mesh->faces[0].vertexArray           = (u32*)(mesh->faces[0].triangleArray         + mesh->triangles.count);
	mesh->faces[0].outerVertexArray      = (u32*)(mesh->faces[0].vertexArray           + mesh->totalFaceVertexCount);
	mesh->faces[0].neighborTriangleArray = (u32*)(mesh->faces[0].outerVertexArray      + mesh->totalFaceOuterVertexCount);
	mesh->faces[0].neighborFaceArray     = (u32*)(mesh->faces[0].neighborTriangleArray + mesh->totalFaceTriNeighborCount);
	mesh->faces[0].triangles         = {mesh->faces[0].triangleArray,         mesh->faces[0].triangleCount};
	mesh->faces[0].vertexes          = {mesh->faces[0].vertexArray,           mesh->faces[0].vertexCount};
	mesh->faces[0].outerVertexes     = {mesh->faces[0].outerVertexArray,      mesh->faces[0].outerVertexCount};
	mesh->faces[0].triangleNeighbors = {mesh->faces[0].neighborTriangleArray, mesh->faces[0].neighborTriangleCount};
	mesh->faces[0].faceNeighbors     = {mesh->faces[0].neighborFaceArray,     mesh->faces[0].neighborFaceCount};
	for(s32 fi=1; fi<mesh->faces.count; ++fi){
		mesh->faces[fi].triangleArray         = (u32*)(mesh->faces[fi-1].triangleArray         + mesh->faces[fi-1].triangleCount);
		mesh->faces[fi].vertexArray           = (u32*)(mesh->faces[fi-1].vertexArray           + mesh->faces[fi-1].vertexCount);
		mesh->faces[fi].outerVertexArray      = (u32*)(mesh->faces[fi-1].outerVertexArray      + mesh->faces[fi-1].outerVertexCount);
		mesh->faces[fi].neighborTriangleArray = (u32*)(mesh->faces[fi-1].neighborTriangleArray + mesh->faces[fi-1].neighborTriangleCount);
		mesh->faces[fi].neighborFaceArray     = (u32*)(mesh->faces[fi-1].neighborFaceArray     + mesh->faces[fi-1].neighborFaceCount);
		mesh->faces[fi].triangles         = {mesh->faces[fi].triangleArray,         mesh->faces[fi].triangleCount};
		mesh->faces[fi].vertexes          = {mesh->faces[fi].vertexArray,           mesh->faces[fi].vertexCount};
		mesh->faces[fi].outerVertexes     = {mesh->faces[fi].outerVertexArray,      mesh->faces[fi].outerVertexCount};
		mesh->faces[fi].triangleNeighbors = {mesh->faces[fi].neighborTriangleArray, mesh->faces[fi].neighborTriangleCount};
		mesh->faces[fi].faceNeighbors     = {mesh->faces[fi].neighborFaceArray,     mesh->faces[fi].neighborFaceCount};
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
	Log("storage","Successfully created ",mesh->name,".mesh");
}

void Storage::
DeleteMesh(Mesh* mesh){ //!Incomplete
	NotImplemented;
}


//////////////////
//// @texture ////
//////////////////
local Texture* 
AllocateTexture(){
	Texture* texture = (Texture*)memory_alloc(sizeof(Texture));
	return texture;
}

pair<u32,Texture*> Storage::
CreateTextureFromFile(const char* filename, ImageFormat format, TextureType type, TextureFilter filter, TextureAddressMode uvMode, b32 keepLoaded, b32 generateMipmaps){
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
	texture->filter  = filter;
	texture->uvMode  = uvMode;
	texture->pixels  = stbi_load((Assets::dirTextures()+filename).c_str(), &texture->width, &texture->height, 
								 &texture->depth, STBI_rgb_alpha);
	texture->loaded  = true;
	if(texture->pixels == 0){ 
		LogE("storage","Failed to create texture '",filename,"': ",stbi_failure_reason()); 
		memory_zfree(texture);
		return result; 
	}
	texture->mipmaps = (generateMipmaps) ? (s32)log2(Max(texture->width, texture->height)) + 1 : 1;
	
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
CreateTextureFromMemory(void* data, const char* name, s32 width, s32 height, ImageFormat format, TextureType type, TextureFilter filter, TextureAddressMode uvMode, b32 keepLoaded, b32 generateMipmaps){
	pair<u32,Texture*> result(0, NullTexture());
	if(data == 0){ LogE("storage","Failed to create texture '",name,"': No memory passed!"); return result; }
	
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
	texture->filter  = filter;
	texture->uvMode  = uvMode;
	texture->width   = width;
	texture->height  = height;
	texture->depth   = 4;
	texture->loaded  = true;
	texture->mipmaps = (generateMipmaps) ? (s32)log2(Max(texture->width, texture->height)) + 1 : 1;
	
	//reinterpret image as RGBA32
	const u8* src = (u8*)data;
	if(format != ImageFormat_RGBA){
		texture->pixels = (u8*)memory_alloc((upt)width * (upt)height * 4);
		data = texture->pixels;
		u32* dst = (u32*)texture->pixels;
		switch(format){
			case ImageFormat_BW:{
				for(s32 i = width*height; i > 0; i--){
					u32 value = (u32)(*src++);
					*dst++ = PackColorU32(value, value, value, value);
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
		memory_zfree(data);
		data = 0;
		texture->pixels = 0;
	}
	
	result.first  = texture->idx;
	result.second = texture;
	textures.add(texture);
	return result;
}

void Storage::
DeleteTexture(Texture* texture){ //!Incomplete
	NotImplemented;
}


///////////////////
//// @material ////
///////////////////
local Material* 
AllocateMaterial(u32 textureCount){
	Material* material = (Material*)memory_alloc(sizeof(Material));
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
CreateMaterialFromFile(const char* filename, b32 warnMissing){
	pair<u32,Material*> result(0, NullMaterial());
	if(strcmp(filename, "null") == 0) return result;
	
	//split filename into name and extension
	std::string filepath = Assets::dirModels()+filename;
	string fullname(filename);
	string name, extension;
	b32 has_extension;
	u32 dot_idx = fullname.findLastChar('.');
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
	char* line_start;  char* line_end = buffer-1;
	char* info_start;  char* info_end;
	char* key_start;   char* key_end;
	char* value_start; char* value_end;
	b32 has_cr = false;
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
		cstring info{info_start, u64(info_end-info_start)};
		
		//check for headers
		if(*info_start == '>'){
			if     (equals(info, cstr_lit(">material"))){ header = MaterialHeader::MATERIAL; }
			else if(equals(info, cstr_lit(">textures"))){ header = MaterialHeader::TEXTURES; }
			else{ header = MaterialHeader::INVALID; ParseError("Uknown header '",info,"'"); }
			continue;
		}
		
		//parse the key-value pair
		if(header == MaterialHeader::INVALID) { ParseError("Invalid header; skipping line"); continue; }
		
		if(header == MaterialHeader::MATERIAL){
			//split the key-value pair
			key_start = info_start;
			key_end   = key_start;
			while(key_end != info_end && *key_end++ != ' ');
			if(key_end == info_end){ ParseError("No key passed."); continue; }
			key_end -= 1;
			cstring key{key_start, u64(key_end-key_start)};
			
			value_end   = info_end;
			value_start = key_end;
			while(*value_start++ == ' ');
			value_start -= 1;
			if(value_end == value_start){ ParseError("No value passed."); continue; }
			cstring value{value_start, u64(value_end-value_start)};
			
			if      (equals(key, cstr_lit("name"))){
				mat_name = string(value_start+1, value_end-value_start-2);
			}else if(equals(key, cstr_lit("flags"))){
				mat_flags = (ModelFlags)b10tou64(value); 
			}else if(equals(key, cstr_lit("shader"))){
				string s = to_string(value);
				forI(Shader_COUNT){ if(strcmp(ShaderStrings[i], s.str) == 0){ mat_shader = i; break; } }
			}else{ ParseError("Invalid key '",key,"' for header '",MaterialHeaderStrings[header],"'"); continue; }
		}else{
			mat_textures.add(string(info_start+1, info_end-info_start-2));
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
	Log("storage","Successfully created ",material->name,".mat");
}

void Storage::
DeleteMaterial(Material* material){ //!Incomplete
	NotImplemented;
}


////////////////
//// @model ////
////////////////
local Model* 
AllocateModel(u32 batchCount){
	Model* model = (Model*)memory_alloc(sizeof(Model));
	model->batches = array<Model::Batch>();
	model->batches.resize((batchCount) ? batchCount : 1);
	return model;
}

pair<u32,Model*> Storage::
CreateModelFromFile(const char* filename, ModelFlags flags, b32 forceLoadOBJ){
	pair<u32,Model*> result(0, NullModel());
	if(strcmp(filename, "null") == 0) return result;
	
	TIMER_START(t_m);
	b32 has_extension;
	std::string filepath = Assets::dirModels() + filename;
	string fullname(filename);
	string name;
	string extension;
	u32 dot_idx = fullname.findLastChar('.');
	if(dot_idx != -1){
		has_extension = true;
		name = fullname.substr(0, dot_idx-1);
		extension = fullname.substr(dot_idx+1);
	}else{
		has_extension = false;
		name = fullname;
	}
	std::string obj_name   = std::string(name.str)+".obj";
	std::string mesh_name  = std::string(name.str)+".mesh";
	std::string model_name = std::string(name.str)+".model";
	
	//check if model is already loaded
	forX(mi, models.count){ if((strcmp(models[mi]->name, name.str) == 0)){ return pair<u32,Model*>(mi,models[mi]); } }
	
	b32 parse_obj_mesh  = true;
	b32 parse_obj_model = true;
	if(!forceLoadOBJ){
		std::vector<std::string> files = Assets::iterateDirectory_(Assets::dirModels());
		for(std::string& file : files){
			if(parse_obj_mesh && file == mesh_name){
				parse_obj_mesh = false;
			}
			if(parse_obj_model && file == model_name){
				parse_obj_model = false;
			}
		}
	}
	
	Model* model = NullModel();
	
	//// load .obj and .mtl ////
	if(parse_obj_model && parse_obj_mesh){
		map<vec3,Mesh::Vertex> vUnique;
		set<vec3> vnUnique;
		set<pair<u32,string>> oUnique, gUnique, uUnique, mUnique; //index offset, name
		set<pair<u32,vec3>> appliedUniqueNormals; //vertex applied on, normal
		array<vec2> vtArray; //NOTE UV vertices arent expected to be unique
		array<u32> vArray, vnArray, oArray, gArray,  uArray,  mArray; //index in unique array
		array<Mesh::Index>    indexes;
		array<Mesh::Triangle> triangles;
		array<Mesh::Face>     faces;
		array<array<pair<u32,u8>>> triNeighbors;
		array<array<u32>> faceTriangles;
		array<set<u32>>   faceVertexes;
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
		u32 default_color = Color_White.rgba;
		b32 mtllib_found    = false;
		b32 s_warning       = false;
		b32 non_tri_warning = false;
		b32 fatal_error     = false;
		
		TIMER_START(t_l);
		char* buffer = Assets::readFileAsciiToArray(filepath, 0, true);
		if(!buffer){  return result; }
		defer{ delete[] buffer; };
		char* line_start;
		char* line_end = buffer - 1;
		b32 has_cr = false;
		for(u32 line_number = 1; ;line_number++){
			//get the next line
			line_start = (has_cr) ? line_end+2 : line_end+1;
			if((line_end = strchr(line_start, '\n')) == 0) break; //EOF if no '\n'
			if(has_cr || *(line_end-1) == '\r') { has_cr = true; line_end -= 1; }
			
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
						b32 neighbor_already = false;
						for(u32 tni=0; tni<triNeighbors[oti].count; ++tni){
							if(triNeighbors[oti][tni].first == cti){ neighbor_already = true; break; }
						}
						
						//check for shared vertexes and mark the edges
						if(!neighbor_already){
							b32 ct0_ot0 = (vArray[v0] == triangles[oti].v[0]);
							b32 ct0_ot1 = (vArray[v0] == triangles[oti].v[1]);
							b32 ct0_ot2 = (vArray[v0] == triangles[oti].v[2]);
							b32 ct1_ot0 = (vArray[v1] == triangles[oti].v[0]);
							b32 ct1_ot1 = (vArray[v1] == triangles[oti].v[1]);
							b32 ct1_ot2 = (vArray[v1] == triangles[oti].v[2]);
							b32 ct2_ot0 = (vArray[v2] == triangles[oti].v[0]);
							b32 ct2_ot1 = (vArray[v2] == triangles[oti].v[1]);
							b32 ct2_ot2 = (vArray[v2] == triangles[oti].v[2]);
							
							//current tri v0 && v1
							if(ct0_ot0 && ct1_ot1){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct0_ot0 && ct1_ot2){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
							if(ct0_ot1 && ct1_ot0){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct0_ot1 && ct1_ot2){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
							if(ct0_ot2 && ct1_ot0){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
							if(ct0_ot2 && ct1_ot1){triNeighbors[cti].add({oti,0}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
							//current tri v1 && v2
							if(ct1_ot0 && ct2_ot1){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct1_ot0 && ct2_ot2){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
							if(ct1_ot1 && ct2_ot0){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct1_ot1 && ct2_ot2){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
							if(ct1_ot2 && ct2_ot0){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
							if(ct1_ot2 && ct2_ot1){triNeighbors[cti].add({oti,1}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
							//current tri v2 && v0
							if(ct2_ot0 && ct0_ot1){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct2_ot0 && ct0_ot2){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
							if(ct2_ot1 && ct0_ot0){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,0}); totalTriNeighbors+=2; continue;}
							if(ct2_ot1 && ct0_ot2){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
							if(ct2_ot2 && ct0_ot0){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,2}); totalTriNeighbors+=2; continue;}
							if(ct2_ot2 && ct0_ot1){triNeighbors[cti].add({oti,2}); triNeighbors[oti].add({cti,1}); totalTriNeighbors+=2; continue;}
						}
					}
					triangles[cti].neighborCount = triNeighbors[cti].count;
					if(((u64)(TIMER_END(t_l) / 1000.0) % 10 == 0) && ((u64)(TIMER_END(t_l) / 1000.0) != 0)){
						PRINTLN(TOSTDSTRING(filename," face ",faces.count," on line ",line_number,
											"finished creation in ",TIMER_END(t_f),"ms"));
					}
				}continue;
				
				//// use material ////
				case 'u':{
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
		forX(bti, triangles.count){
			if(triangles[bti].face != -1) continue;
			
			//create face and add base triange to it
			u32 cfi = faces.count;
			faces.add(Mesh::Face{});
			faceTriangles.add(array<u32>());
			faceVertexes.add(set<u32>());
			faceOuterVertexes.add(array<u32>());
			faceTriNeighbors.add(array<u32>());
			faceFaceNeighbors.add(array<u32>());
			faces[cfi].normal = triangles[bti].normal;
			triangles[bti].face = cfi;
			faceTriangles[cfi].add(bti);
			faceVertexes[cfi].add(triangles[bti].v[0],triangles[bti].v[0]);
			faceVertexes[cfi].add(triangles[bti].v[1],triangles[bti].v[1]);
			faceVertexes[cfi].add(triangles[bti].v[2],triangles[bti].v[2]);
			totalFaceVertexes += 3;
			
			array<u32> check_tris({(u32)bti});
			forX(check_tri_idx, check_tris.count){
				u32 cti = check_tris[check_tri_idx];
				forX(nei_tri_idx, triNeighbors[cti].count){
					u32 nti = triNeighbors[cti][nei_tri_idx].first;
					b32 checked = false;
					forI(check_tris.count){ if(check_tris[i] == nti){ checked = true; break; } }
					if(checked) continue;
					
					//check if neighbor triangle has same normal
					if(triangles[cti].normal == triangles[nti].normal){
						check_tris.add(nti);
						triangles[nti].face = cfi;
						faceTriangles[cfi].add(nti);
						faceVertexes[cfi].add(triangles[nti].v[0],triangles[nti].v[0]);
						faceVertexes[cfi].add(triangles[nti].v[1],triangles[nti].v[1]);
						faceVertexes[cfi].add(triangles[nti].v[2],triangles[nti].v[2]);
						totalFaceVertexes += 3;
					}else{
						faceTriNeighbors[cfi].add(nti);
						totalFaceTriNeighbors++;
						u32 v1 = triangles[cti].v[ triNeighbors[cti][nei_tri_idx].second       ];
						u32 v2 = triangles[cti].v[(triNeighbors[cti][nei_tri_idx].second+1) % 3];
						b32 v1_already = false; b32 v2_already = false;
						forX(fovi, faceOuterVertexes[cfi].count){
							if(!v1_already && faceOuterVertexes[cfi][fovi] == v1){ v1_already = true; }
							if(!v2_already && faceOuterVertexes[cfi][fovi] == v2){ v2_already = true; }
							if(v1_already && v2_already) break;
						}
						if(!v1_already){
							faceOuterVertexes[cfi].add(v1); 
							totalFaceOuterVertexes++;
							faces[cfi].center += vUnique.atIdx(v1)->pos;
						}
						if(!v2_already){
							faceOuterVertexes[cfi].add(v2);
							totalFaceOuterVertexes++;
							faces[cfi].center += vUnique.atIdx(v2)->pos;
						}
					}
				}
			}
		}
		
		//generate face neighbors
		forX(cfi, faces.count){
			forX(cnti, faceTriNeighbors[cfi].count){ //check neighbor triangles
				b32 already_added = false;
				forX(nfi, faceFaceNeighbors[cfi].count){ //see if face neighbor already added
					if(triangles[faceTriNeighbors[cfi][cnti]].face == faceFaceNeighbors[cfi][nfi]){
						already_added = true;
						break;
					}
				}
				if(!already_added){
					faceFaceNeighbors[cfi].add(triangles[faceTriNeighbors[cfi][cnti]].face);
					faceFaceNeighbors[triangles[faceTriNeighbors[cfi][cnti]].face].add(cfi);
					totalFaceFaceNeighbors += 2;
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
		
		//// parsing warnings/errors ////
		if(non_tri_warning)   LogW("storage","The mesh was not triangulated before parsing; Expect missing triangles!");
		if(s_warning)         LogW("storage","There were 's' specifiers when parsing ",filename,", but those are not evaluated currently");
		if(!vtArray.count){   LogW("storage","No vertex UVs 'vt' were parsed in ",filename); }
		if(!vnArray.count){   LogW("storage","No vertex normals 'vn' were parsed in ",filename); }
		if(fatal_error){      LogE("storage","OBJ parsing encountered a fatal error in ",filename); return result; }
		if(!vArray.count){    LogE("storage","No vertex positions 'v' were parsed in ",filename); return result; }
		if(!triangles.count){ LogE("storage","No faces 'f' were parsed in ",filename); return result; }
		
		//// create mesh ////
		Mesh* mesh = AllocateMesh(indexes.count, vUnique.count, faces.count, totalTriNeighbors, 
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
		mesh->triangles[0].edgeArray      = (u8*)(mesh->triangleArray[0].neighborArray + totalTriNeighbors);
		mesh->triangles[0].neighbors = {mesh->triangles[0].neighborArray, triNeighbors[0].count};
		mesh->triangles[0].edges     = {mesh->triangles[0].edgeArray,     triNeighbors[0].count};
		for(s32 ti=1; ti<mesh->triangles.count; ++ti){
			mesh->triangles[ti].neighborArray = (u32*)(mesh->triangles[ti-1].neighborArray + triNeighbors[ti-1].count);
			mesh->triangles[ti].edgeArray      = (u8*)(mesh->triangles[ti-1].edgeArray     + triNeighbors[ti-1].count);
			mesh->triangles[ti].neighbors  = {mesh->triangles[ti].neighborArray, triNeighbors[ti].count};
			mesh->triangles[ti].edges      = {mesh->triangles[ti].edgeArray,     triNeighbors[ti].count};
		}
		mesh->faces[0].triangleArray         = (u32*)(mesh->triangles[0].edgeArray         + totalTriNeighbors);
		mesh->faces[0].vertexArray           = (u32*)(mesh->faces[0].triangleArray         + triangles.count);
		mesh->faces[0].outerVertexArray      = (u32*)(mesh->faces[0].vertexArray           + totalFaceVertexes);
		mesh->faces[0].neighborTriangleArray = (u32*)(mesh->faces[0].outerVertexArray      + totalFaceOuterVertexes);
		mesh->faces[0].neighborFaceArray     = (u32*)(mesh->faces[0].neighborTriangleArray + totalFaceTriNeighbors);
		mesh->faces[0].triangles          = {mesh->faces[0].triangleArray,          faceTriangles[0].count};
		mesh->faces[0].vertexes           = {mesh->faces[0].vertexArray,            faceVertexes[0].count};
		mesh->faces[0].outerVertexes      = {mesh->faces[0].outerVertexArray,       faceOuterVertexes[0].count};
		mesh->faces[0].triangleNeighbors  = {mesh->faces[0].neighborTriangleArray,  faceTriNeighbors[0].count};
		mesh->faces[0].faceNeighbors      = {mesh->faces[0].neighborFaceArray,      faceFaceNeighbors[0].count};
		for(s32 fi=1; fi<mesh->faces.count; ++fi){
			mesh->faces[fi].triangleArray         = (u32*)(mesh->faces[fi-1].triangleArray         + faceTriangles[fi-1].count);
			mesh->faces[fi].vertexArray           = (u32*)(mesh->faces[fi-1].vertexArray           + faceVertexes[fi-1].count);
			mesh->faces[fi].outerVertexArray      = (u32*)(mesh->faces[fi-1].outerVertexArray      + faceOuterVertexes[fi-1].count);
			mesh->faces[fi].neighborTriangleArray = (u32*)(mesh->faces[fi-1].neighborTriangleArray + faceTriNeighbors[fi-1].count);
			mesh->faces[fi].neighborFaceArray     = (u32*)(mesh->faces[fi-1].neighborFaceArray     + faceFaceNeighbors[fi-1].count);
			mesh->faces[fi].triangles          = {mesh->faces[fi-0].triangleArray,          faceTriangles[fi].count};
			mesh->faces[fi].vertexes           = {mesh->faces[fi-0].vertexArray,            faceVertexes[fi].count};
			mesh->faces[fi].outerVertexes      = {mesh->faces[fi-0].outerVertexArray,       faceOuterVertexes[fi].count};
			mesh->faces[fi].triangleNeighbors  = {mesh->faces[fi-0].neighborTriangleArray,  faceTriNeighbors[fi].count};
			mesh->faces[fi].faceNeighbors      = {mesh->faces[fi-0].neighborFaceArray,      faceFaceNeighbors[fi].count};
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
			mesh->faceArray[fi].neighborTriangleCount = faceTriNeighbors[fi].count;
			mesh->faceArray[fi].neighborFaceCount = faceFaceNeighbors[fi].count;
			mesh->faceArray[fi].center = faces[fi].center / (f32)faceOuterVertexes[fi].count;
			forX(fti, mesh->faces[fi].triangles.count){
				mesh->faceArray[fi].triangleArray[fti] = faceTriangles[fi][fti];
			}
			forX(fvi, mesh->faces[fi].vertexes.count){
				mesh->faceArray[fi].vertexArray[fvi] = faceVertexes[fi].data[fvi];
			}
			forX(fvi, mesh->faces[fi].outerVertexes.count){
				mesh->faceArray[fi].outerVertexArray[fvi] = faceOuterVertexes[fi][fvi];
			}
			forX(fvi, mesh->faces[fi].triangleNeighbors.count){
				mesh->faceArray[fi].neighborTriangleArray[fvi] = faceTriNeighbors[fi][fvi];
			}
			forX(fvi, mesh->faces[fi].faceNeighbors.count){
				mesh->faceArray[fi].neighborFaceArray[fvi] = faceFaceNeighbors[fi][fvi];
			}
		}
		
		Render::LoadMesh(mesh); //TODO(delle) check if mesh already loaded
		meshes.add(mesh);
		Log("storage","Parsing and loading OBJ '",filename,"' took ",TIMER_END(t_l),"ms");
		
		//parse MTL files
		if(mtllib_found){
			TIMER_RESET(t_l);
			
			//!Incomplete
			
			Log("storage","Parsing and loading MTLs for OBJ '",filename,"' took ",TIMER_END(t_l),"ms");
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
	}
	//// load .obj (batch info only), .mtl, and .mesh ////
	else if(parse_obj_model){
		Mesh* mesh = CreateMeshFromFile(mesh_name.c_str()).second;
		
		set<pair<u32,string>> oUnique, gUnique, uUnique, mUnique; //index offset, name
		array<u32> oArray, gArray,  uArray,  mArray; //index in unique array
		b32 mtllib_found = false;
		u32 index_count = 0;
		
		TIMER_START(t_l);
		char* buffer = Assets::readFileAsciiToArray(filepath, 0, true);
		if(!buffer){  return result; }
		defer{ delete[] buffer; };
		char* line_start; char* line_end = buffer - 1;
		b32 has_cr = false;
		for(u32 line_number = 1; ;line_number++){
			//get the next line
			line_start = (has_cr) ? line_end+2 : line_end+1;
			if((line_end = strchr(line_start, '\n')) == 0) break; //EOF if no '\n'
			if(has_cr || *(line_end-1) == '\r') { has_cr = true; line_end -= 1; }
			
			switch(*line_start){
				//// face ////
				case 'f':{
					if(*(line_start+1) != ' '){ ParseError("No space after 'f'"); return result; }
					index_count += 3;
				}
				
				//// use material ////
				case 'u':{ //use material
					if(strncmp(line_start, "usemtl ", 7) != 0){ ParseError("Specifier started with 'u' but didn't equal 'usemtl '"); return result; }
					if(mtllib_found){
						pair<u32,string> usemtl(index_count, string(line_start+7, line_end-(line_start+7)));
						uArray.add(uUnique.add(usemtl,usemtl));
					}else{
						ParseError("Specifier 'usemtl' used before 'mtllib' specifier");
					}
				}continue;
				
				//// load material ////
				case 'm':{
					if(strncmp(line_start, "mtllib ", 7) != 0){ ParseError("Specifier started with 'm' but didn't equal 'mtllib '"); return result; }
					mtllib_found = true;
					pair<u32,string> mtllib(index_count, string(line_start+7, line_end-(line_start+7)));
					mArray.add(mUnique.add(mtllib,mtllib));
				}continue;
				
				//// group (batch) ////
				case 'g':{
					if(*(line_start+1) != ' '){ ParseError("No space after 'g'"); return result; }
					pair<u32,string> group(index_count, string(line_start+2, line_end-(line_start+2)));
					gArray.add(gUnique.add(group,group));
				}continue;
				
				//// object ////
				case 'o':{
					if(*(line_start+1) != ' '){ ParseError("No space after 'o'"); return result; }
					pair<u32,string> object(index_count, string(line_start+2, line_end-(line_start+2)));
					oArray.add(oUnique.add(object,object));
				}continue;
				
				default: continue;
			}
		}
		
		//parse MTL files
		if(mtllib_found){
			TIMER_RESET(t_l);
			
			//!Incomplete
			
			Log("storage","Parsing and loading MTLs for OBJ '",filename,"' took ",TIMER_END(t_l),"ms");
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
			model->batches[mArray.count-1].indexCount  = index_count - model->batches[mArray.count-1].indexOffset;
			model->batches[mArray.count-1].material    = 0;
			for(u32 bi = mArray.count-2; bi >= 0; --bi){
				model->batches[bi].indexOffset = mUnique.data[mArray[bi]].first;
				model->batches[bi].indexCount  = model->batches[bi+1].indexOffset - model->batches[bi].indexOffset;
				model->batches[bi].material    = 0;
			}
		}else{
			model->batches[0].indexOffset = 0;
			model->batches[0].indexCount  = index_count;
			model->batches[0].material    = 0;
		}
	}
	//// load .model and .mesh ////
	else{
		//model storage
		string model_load_name;
		string model_load_mesh;
		ModelFlags model_load_flags;
		array<pair<string,u32,u32>> model_load_batches;
		
		enum ModelHeader{ MODEL, BATCHES, INVALID, };
		persist const char* ModelHeaderStrings[] = { "MODEL", "BATCHES", "INVALID", };
		u32 header = ModelHeader::INVALID;
		
		char* buffer = Assets::readFileAsciiToArray(filepath, 0, true);
		if(!buffer){  return result; }
		defer{ delete[] buffer; };
		char* line_start;  char* line_end = buffer-1;
		char* info_start;  char* info_end;
		char* key_start;   char* key_end;
		char* value_start; char* value_end;
		b32 has_cr = false;
		for(u32 line_number = 1; ;line_number++){
			//get the next line
			line_start = (has_cr) ? line_end+2 : line_end+1;
			if((line_end = strchr(line_start, '\n')) == 0) break; //EOF if no '\n'
			if(has_cr || *(line_end-1) == '\r') { has_cr = true; line_end -= 1; }
			if(line_start == line_end) continue;
			
			//format the line
			info_start = line_start + Utils::skipSpacesLeading(line_start, line_end-line_start);  if(info_start == line_end) continue;
			info_end   = info_start + Utils::skipComments(info_start, "#", line_end-info_start);  if(info_start == info_end) continue;
			info_end   = info_start + Utils::skipSpacesTrailing(info_start, info_end-info_start); if(info_start == info_end) continue;
			cstring info{info_start, u64(info_end-info_start)};
			
			//check for headers
			if(*info_start == '>'){
				if     (equals(info, cstr_lit(">model"))){ header = ModelHeader::MODEL; }
				else if(equals(info, cstr_lit(">batches"))){ header = ModelHeader::BATCHES; }
				else{ header = ModelHeader::INVALID; ParseError("Uknown header '",info,"'"); }
				continue;
			}
			
			//split the key-value pair
			key_start = info_start;
			key_end   = key_start;
			while(key_end != info_end && *key_end++ != ' ');
			if(key_end == info_end){ ParseError("No key passed."); continue; }
			key_end -= 1;
			cstring key{key_start, u64(key_end-key_start)};
			
			value_end   = info_end;
			value_start = key_end;
			while(*value_start++ == ' ');
			value_start -= 1;
			if(value_end == value_start){ ParseError("No value passed."); continue; }
			cstring value{value_start, u64(value_end-value_start)};
			
			//parse the key-value pair
			if(header == ModelHeader::INVALID) { ParseError("Invalid header; skipping line"); continue; }
			
			if(header == ModelHeader::MODEL){
				if      (equals(key, cstr_lit("name"))){
					model_load_name = string(value_start+1, value_end-value_start-2);
				}else if(equals(key, cstr_lit("flags"))){
					model_load_flags = (ModelFlags)b10tou64(value); 
				}else if(equals(key, cstr_lit("mesh"))){
					model_load_mesh = string(value_start+1, value_end-value_start-2);
				}else if(equals(key, cstr_lit("armature"))){
					//NOTE currently nothing
				}else{ ParseError("Invalid key '",key,"' for header '",ModelHeaderStrings[header],"'"); continue; }
			}else{
				cstring s = value;
				u32 i0 = (u32)b10tou64(s,&s);
				u32 i1 = (u32)b10tou64(s);
				model_load_batches.add({string(key_start+1,key_end-key_start-2), i0, i1});
			}
		}
		
		model = AllocateModel(model_load_batches.count);
		cpystr(model->name, model_load_name.str, DESHI_NAME_SIZE);
		model->idx      = models.count;
		model->flags    = model_load_flags;
		model->mesh     = CreateMeshFromFile(model_load_mesh.str).second;
		model->armature = 0;
		forI(model_load_batches.count){
			model->batches[i] = Model::Batch{
				model_load_batches[i].second,
				model_load_batches[i].third,
				CreateMaterialFromFile(model_load_batches[i].first.str).first
			};
		}
		
		Log("storage","Successfully loaded ",model->name,".model");
	}
	
	result.first  = model->idx;
	result.second = model;
	models.add(model);
	Log("storage","Finished loading model '",filename,"' in ",TIMER_END(t_m),"ms");
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
	model->batches[0] = {0, mesh->indexCount, 0};
	
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
	SaveMesh(model->mesh);
	std::string model_save = TOSTDSTRING(">model"
										 "\nname     \"",model->name,"\""
										 "\nflags    ", model->flags,
										 "\nmesh     \"", model->mesh->name,"\""
										 "\narmature ", 0,
										 "\n"
										 "\n>batches");
	forI(model->batches.count){
		SaveMaterial(materials[model->batches[i].material]);
		model_save.append(TOSTDSTRING("\n\"",materials[model->batches[i].material]->name,"\" ",
									  model->batches[i].indexOffset," ",model->batches[i].indexCount));
	}
	model_save.append("\n");
	Assets::writeFile(Assets::dirModels()+std::string(model->name)+".model", model_save.c_str(), model_save.size());
	Log("storage","Successfully created ",model->name,".model");
}

void Storage::
DeleteModel(Model* model){ //!Incomplete
	NotImplemented;
}


///////////////
//// @font ////
///////////////
local Font* 
AllocateFont(Type type){
	Font* font = (Font*)memory_alloc(sizeof(Font));
	font->type = type;
	font->idx = Storage::fonts.count;
	return font;
}

pair<u32,Font*> Storage::
CreateFontFromFileBDF(const char* filename){
	pair<u32,Font*> result(0,NullFont());
	
	//check if created already
	forX(fi, fonts.size()){
		if(strncmp(filename, fonts[fi]->name, DESHI_NAME_SIZE) == 0){
			return pair<u32,Font*>(fi,fonts[fi]);
		}
	}
	
	char* buffer = Assets::readFileAsciiToArray(Assets::dirFonts() + filename);
	if(!buffer){ return result; }
	defer{ delete[] buffer; };
	
	Assert(strncmp("STARTFONT", buffer, 9) == 0); //TODO(delle) error handling: incorrect file
	Font* font = AllocateFont(FontType_BDF);
	
	b32 in_char   = false;
	b32 in_bitmap = false;
	u32 char_idx = 0;
	u32 bitmap_row = 0;
	u32 glyph_offset = 0;
	u32 top_offset = 0;
	u32 left_offset = 0;
	vec4 current_bbx = vec4::ZERO;
	vec4 font_bbx = vec4::ZERO;
	vec2 font_dpi = vec2::ZERO;
	u16* encodings = 0;
	u8*  pixels = 0;
	
	char* line_start;  char* line_end = buffer-1;
	char* info_start;  char* info_end;
	char* key_start;   char* key_end;
	char* value_start; char* value_end;
	b32 has_cr = false;
	for(u32 line_number = 1; ;line_number++){
		//get the next line
		line_start = (has_cr) ? line_end+2 : line_end+1;
		if((line_end = strchr(line_start, '\n')) == 0) break; //EOF if no '\n'
		if(has_cr || *(line_end-1) == '\r'){ has_cr = true; line_end -= 1; }
		if(line_start == line_end) continue;
		
		//format the line
		info_start = line_start + Utils::skipSpacesLeading (line_start, line_end-line_start); if(info_start == line_end) continue;
		info_end   = info_start + Utils::skipSpacesTrailing(info_start, line_end-info_start); if(info_start == info_end) continue;
		
		{//split the key-value pair
			key_start = info_start;
			key_end   = key_start;
			while(key_end != info_end && *key_end != ' '){ key_end++; }
			
			value_end   = info_end;
			value_start = key_end;
			while(*value_start == ' '){ value_start++; }
		}
		
		if(in_bitmap){
			if(strncmp("ENDCHAR", key_start, key_end-key_start) == 0){
				in_char = false;
				in_bitmap = false;
				char_idx++;
				bitmap_row = 0;
				continue;
			}
			Assert(bitmap_row < current_bbx.y);
			
			s32 chars = (s32)(info_end-info_start);
			Assert(chars <= 4); //TODO(delle) error handling: max 16 pixel width
			u8 scaled[16]{};
			
			//scale each byte to represent one pixel per bit then byteswap the u64 //TODO(delle) only byteswap if little-endian
			//ref: https://stackoverflow.com/questions/9023129/algorithm-for-bit-expansion-duplication/9044057#9044057
			//scale = 8 (1 bit to 1 byte)
			//mask0 = 0x0000000f0000000f, mask1 = 0x0003000300030003, mask2 = 0x0101010101010101
			//shift0 = (1 << 28) + 1, shift1 = (1 << 14) + 1, shift2 = (1 << 7) + 1
			for(s32 i=0; i<chars; i+=2){
				u64 reversed = (((b16tou64({info_start+i, 2}) * 0x10000001 & 0x0000000f0000000f) 
								 * 0x4001 & 0x0003000300030003) * 0x81 & 0x0101010101010101) * 255;
				*(u64*)(scaled+i) = ByteSwap64(reversed);
			}
			memcpy(pixels+2*font->max_width+(upt)(glyph_offset + (bitmap_row+top_offset)*font->max_width + left_offset), scaled, upt(current_bbx.x*sizeof(u8)));
			
			bitmap_row++;
			continue;
		}
		
		if(in_char){
			if      (strncmp("ENCODING", key_start, key_end-key_start) == 0){
				encodings[char_idx] = strtol(value_start, 0, 10);
			}else if(strncmp("BITMAP", key_start, key_end-key_start) == 0){
				in_bitmap = true;
			}else if(strncmp("SWIDTH", key_start, key_end-key_start) == 0){
				//unused
			}else if(strncmp("DWIDTH", key_start, key_end-key_start) == 0){
				//unused in monospace fonts
			}else if(strncmp("BBX", key_start, key_end-key_start) == 0){
				char* cursor = value_start;
				current_bbx.x = (f32)strtol(cursor,   &cursor, 10); //width
				current_bbx.y = (f32)strtol(cursor+1, &cursor, 10); //height
				current_bbx.z = (f32)strtol(cursor+1, &cursor, 10); //lower-left x
				current_bbx.w = (f32)strtol(cursor+1, &cursor, 10); //lower-left y
				glyph_offset = char_idx*font->max_height*font->max_width;
				top_offset   = u32(font->max_height-(current_bbx.w-font_bbx.w)-current_bbx.y);
				left_offset  = u32(current_bbx.z-font_bbx.z);
				
				Assert(current_bbx.x <= font_bbx.x);
				Assert(current_bbx.y <= font_bbx.y);
				Assert(current_bbx.z >= font_bbx.z);
				Assert(current_bbx.w >= font_bbx.w);
			}else{
				Assert(!"unhandled key");
			}
			continue;
		}
		
		if      (strncmp("STARTCHAR", key_start, key_end-key_start) == 0){
			in_char = true;
		}else if(strncmp("SIZE", key_start, key_end-key_start) == 0){
			char* cursor = value_start;
			font_dpi.x = (f32)strtol(cursor+1, &cursor, 10);
			font_dpi.y = (f32)strtol(cursor+1, &cursor, 10);
		}else if(strncmp("FONTBOUNDINGBOX", key_start, key_end-key_start) == 0){
			char* cursor = value_start;
			font_bbx.x = (f32)strtol(cursor,   &cursor, 10); //width
			font_bbx.y = (f32)strtol(cursor+1, &cursor, 10); //height
			font_bbx.z = (f32)strtol(cursor+1, &cursor, 10); //lower-left x
			font_bbx.w = (f32)strtol(cursor+1, &cursor, 10); //lower-left y
			font->max_width  = (u32)font_bbx.x;
			font->max_height = (u32)font_bbx.y;
		}else if(strncmp("FONT_NAME",   key_start, key_end-key_start) == 0){
			cpystr(font->name,   string(value_start+1, value_end-value_start-2).str,DESHI_NAME_SIZE);
		}else if(strncmp("WEIGHT_NAME", key_start, key_end-key_start) == 0){
			cpystr(font->weight, string(value_start+1, value_end-value_start-2).str,DESHI_NAME_SIZE);
		}else if(strncmp("CHARS",       key_start, key_end-key_start) == 0){
			font->count = strtol(value_start, 0, 10);
			Assert(font->max_width && font->max_height && font->count);
			encodings = (u16*)memory_talloc(font->count*sizeof(u16));
			pixels = (u8*)memory_talloc(font->count * ((font->max_width*font->max_height + 2*font->max_width) * sizeof(u8)));
			pixels[0] = 255;
			pixels[1] = 255;
			pixels[font->max_width] = 255;
			pixels[font->max_width + 1] = 255;
			font->uvOffset = 2.f / (font->max_height * font->count + 2);
		}else{
			continue;
		}
	}
	
	Texture* texture = CreateTextureFromMemory(pixels, font->name, font->max_width, font->max_height*font->count,
											   ImageFormat_BW, TextureType_2D, TextureFilter_Nearest, TextureAddressMode_ClampToWhite, false, false).second;
	//DeleteTexture(texture);
	
	font->aspect_ratio = (f32)font->max_height / font->max_width;
	font->tex = texture;
	
	fonts.add(font);
	result.first  = font->idx;
	result.second = font;
	return result;
}

//TODO clean up this function some and add in some stuff to reduce the overhead of adding in a new range
pair<u32,Font*> Storage::
CreateFontFromFileTTF(const char* filename, u32 size){
	pair<u32,Font*> result(0,NullFont());
	
	//check if created already
	//TODO look into why if we load the same font w a different size it gets weird
	//(i took that check out of here for now)
	forX(fi, fonts.size()) {
		if ((strncmp(filename, fonts[fi]->name, DESHI_NAME_SIZE) == 0)) {
			return pair<u32, Font*>(fi, fonts[fi]);
		}
	}
	
	char* buffer = Assets::readFileBinaryToArray(Assets::dirFonts()+filename);
	if(!buffer){ return result; }
	defer{ delete[] buffer; };
	
	Font* font = AllocateFont(FontType_TTF); 
	
	int x0, y0, x1, y1;
	
	stbtt_fontinfo info;
	stbtt_InitFont(&info, (unsigned char*)buffer, 0);
	stbtt_GetScaledFontVMetrics((u8*)buffer, 0, (f32)size, &font->ascent, &font->decent, &font->line_gap);
	stbtt_GetFontBoundingBox(&info, &x0, &y0, &x1, &y1);
	
	//current ranges:
	// ASCII              32 - 126  ~  94 chars
	// Greek and Coptic  880 - 1023 ~ 143 chars
	// Cyrillic         1024 - 1279 ~ 256 chars
	// Super/Subscripts 8304 - 8348 ~  44 chars (we will want our own method for doing super/subscripts in suugu)
	// Currency Symbols 8352 - 8384 ~  32 chars
	// Arrows           8592 - 8703 ~ 111 chars
	// Math Symbols     8704 - 8959 ~ 255 chars
	// 
	// and maybe more to come eventually.
	// 
	//TODO(sushi) maybe implement taking in ranges 
	
	stbtt_pack_range* ranges = (stbtt_pack_range*)memory_alloc(7*sizeof(*ranges));
	
	ranges[0].num_chars = 94;   ranges[0].first_unicode_codepoint_in_range = 32;
	ranges[1].num_chars = 143;  ranges[1].first_unicode_codepoint_in_range = 880;
	ranges[2].num_chars = 255;  ranges[2].first_unicode_codepoint_in_range = 1024;
	ranges[3].num_chars = 44;   ranges[3].first_unicode_codepoint_in_range = 8304;
	ranges[4].num_chars = 32;   ranges[4].first_unicode_codepoint_in_range = 8352;
	ranges[5].num_chars = 111;  ranges[5].first_unicode_codepoint_in_range = 8592;
	ranges[6].num_chars = 255;  ranges[6].first_unicode_codepoint_in_range = 8704;
	
	ranges[0].font_size = (f32)size; 
	ranges[1].font_size = (f32)size; 
	ranges[2].font_size = (f32)size; 
	ranges[3].font_size = (f32)size; 
	ranges[4].font_size = (f32)size; 
	ranges[5].font_size = (f32)size; 
	ranges[6].font_size = (f32)size;
	
	ranges[0].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[0].num_chars*sizeof(stbtt_packedchar));
	ranges[1].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[1].num_chars*sizeof(stbtt_packedchar));
	ranges[2].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[2].num_chars*sizeof(stbtt_packedchar));
	ranges[3].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[3].num_chars*sizeof(stbtt_packedchar));
	ranges[4].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[4].num_chars*sizeof(stbtt_packedchar));
	ranges[5].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[5].num_chars*sizeof(stbtt_packedchar));
	ranges[6].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[6].num_chars*sizeof(stbtt_packedchar));
	
	stbtt_pack_context* pc = (stbtt_pack_context*)memory_alloc(1*sizeof(*pc));
	
	font->num_ranges = 6;
	font->ttf_pack_ranges = (pack_range*)ranges;
	font->ttf_pack_context = pc;
	
	u32 widthmax = x1 - x0, heightmax = y1 - y0;
	font->aspect_ratio = (f32)heightmax / widthmax;
	
	//trying to minimize the texture here, but its difficult due to stbtt packing all of them together
	//i believe this makes it into the smallest square it could be w/o knowing how stbtt packs them together
	//also just doesnt really work well with non-monospaced fonts
	//TODO figure out a better way to do this.
	u32 tsy = (u32)ceil(size * sqrtf(679) / font->aspect_ratio);
	u32 tsx = (u32)ceil(widthmax * size /  heightmax * sqrtf(679)) + 4; //add four rows to make room for 4 white pixels to optimize uicmds
	
	font->max_height = size;
	font->max_width = u32(f32(widthmax) / f32(heightmax) * size);
	font->count = 679;
	font->ttf_size[0] = tsx;
	font->ttf_size[1] = tsy; 
	cpystr(font->name,filename,DESHI_NAME_SIZE);
	
	u8* pixels = (u8*)memory_talloc((tsx * tsy)*sizeof(u8));
	pixels[0]     = 255;
	pixels[1]     = 255;
	pixels[tsx]   = 255;
	pixels[tsx+1] = 255;
	
	//begin a font pack
	Assert(stbtt_PackBegin(pc, pixels + 2 * tsx, tsx, tsy - 4, 0, 1, nullptr));
	stbtt_PackSetSkipMissingCodepoints(pc, true);
	
	//pack our ranges
	stbtt_PackFontRanges(pc, (u8*)buffer, 0, ranges, 6);
	
	stbtt_PackEnd(pc);
	
	Texture* texture = CreateTextureFromMemory(pixels, font->name, tsx, tsy, 
											   ImageFormat_BW, TextureType_2D, TextureFilter_Nearest, TextureAddressMode_ClampToWhite, false, false).second;
	//DeleteTexture(texture);
	
	font->uvOffset = 2.f / tsy;
	font->tex = texture;
	
	fonts.add(font);
	result.first  = font->idx;
	result.second = font;
	return result;
}

void Storage::
DeleteFont(Font* font){ //!Incomplete
	NotImplemented;
}


void DrawMeshesWindow() { 
	using namespace UI;
	SetNextWindowSize(vec2(MAX_F32, MAX_F32));
	BeginChild("StorageBrowserUIMeshes", vec2(MAX_F32, MAX_F32));
	Text("TODO");
	EndChild();
}

void DrawTexturesWindow() {
	Storage_* st = DeshStorage;
	
	//TODO make all of this stuff get checked only when necessary
	b32 new_selected = 0;
	persist Texture* selected = 0;
	
	Texture* largest = st->textures[0];
	Texture* smallest = st->textures[0];
	
	//gather size of textures in memory
	upt texture_bytes = 0;
	
	
	for (Texture* t : st->textures) {
		texture_bytes += t->width * t->height * u8size;
		if (t->width * t->height > largest->width * largest->height)   largest = t;
		if (t->width * t->height < smallest->width * smallest->height) smallest = t;
	}
	
	using namespace UI;

	AddItemFlags(UIItemType_Header, UIHeaderFlags_NoBorder);
	
	
	SetNextWindowSize(vec2(MAX_F32, MAX_F32));
	BeginChild("StorageBrowserUI_Textures", vec2::ZERO, UIWindowFlags_NoBorder);
	
	BeginRow("StorageBrowserUI_Row1",2, 0, UIRowFlags_LookbackAndResizeToMax);
	RowSetupColumnAlignments({ {1, 0.5}, {0, 0.5} });
	
	Text("Textures Loaded: "); Text(toStr(st->textures.count).str);
	Text("Memory Occupied: "); Text(toStr(texture_bytes / bytesDivisor(texture_bytes), " ", bytesUnit(texture_bytes), "b").str);
	
	EndRow();
	
	if (BeginCombo("StorageBrowserUI_Texture_Selection_Combo", (selected ? selected->name : "select texture"))) {
		for (Texture* t : st->textures) {
			if (Selectable(t->name, t == selected)) {
				selected = t;
				new_selected = 1;
			}
		}
		EndCombo();
	}
	
	Separator(9);
	
	if (BeginHeader("Stats")) {
		BeginRow("StorageBrowserUI_Row2", 3, 0, UIRowFlags_LookbackAndResizeToMax);
		RowSetupColumnAlignments({ {1, 0.5}, {0, 0.5}, {0.5, 0.5} });
		
		Text("Largest Texture: "); Text(largest->name); 
		if (Button("select")) { selected = largest; new_selected = 1;}
		
		Text("Smallest Texture: "); Text(smallest->name);
		if (Button("select")) { selected = smallest; new_selected = 1; }
		
		EndRow();
		
		EndHeader();
	}
	
	Separator(9);

	if (selected) {
		BeginRow("StorageBrowserUI_Texture_Selected", 2, 0, UIRowFlags_LookbackAndResizeToMax);
		RowSetupColumnAlignments({ {0, 0.5}, {0, 0.5} });
		
		u32 texbytes = selected->width * selected->height * u8size;

		Text("Name:");         Text(selected->name);
		Text("Index: ");       Text(toStr(selected->idx).str);
		Text("Width: ");       Text(toStr(selected->width).str);
		Text("Height: ");      Text(toStr(selected->height).str);
		Text("Depth: ");       Text(toStr(selected->depth).str);
		Text("MipMaps: ");     Text(toStr(selected->mipmaps).str);
		Text("Format: ");      Text(ImageFormatStrings[selected->format - 1]);
		Text("Type: ");        Text(TextureTypeStrings[selected->type]);
		Text("Filter: ");      Text(TextureFilterStrings[selected->filter]);
		Text("UV Mode: ");     Text(TextureAddressModeStrings[selected->uvMode]);
		Text("Memory Used: "); Text(toStr(texbytes / bytesDivisor(texbytes), " ", bytesUnit(texbytes), "b").str);
		
		EndRow();
		PushColor(UIStyleCol_WindowBg, 0x073030ff);

		SetNextWindowSize(vec2(MAX_F32, MAX_F32));
		BeginChild("StorageBrowserUI_Texture_ImageInspector", vec2::ZERO, UIWindowFlags_NoInteract);
		persist f32  zoom = 300;
		persist vec2 mpl;
		persist vec2 imagepos;
		persist vec2 imageposlatch;
		persist UIImageFlags flags;
		
		vec2 mp = DeshInput->mousePos;
		
		if (Button("Flip x")) 
			ToggleFlag(flags, UIImageFlags_FlipX);
		SameLine();
		if (Button("Flip y")) 
			ToggleFlag(flags, UIImageFlags_FlipY);
		
		if (new_selected) {
			zoom = f32(GetWindow()->width) / selected->width ;
			//imagepos = vec2(
			//				(GetWindow()->width - selected->width) / 2,
			//				(GetWindow()->height - selected->height) / 2
			//				);
			imagepos = vec2::ZERO;
		}
		
		Text(toStr(zoom).str);
		
		if (IsWinHovered()) {
			SetPreventInputs();
			
			if (DeshInput->scrollY) {
				f32 val = 10 * DeshInput->scrollY;
				zoom += zoom / val;
				//TODO make it zoom to the mouse 
				vec2 imtomp = (mp - GetWindow()->position) - GetWindow()->dimensions / 2;
				//imagepos -= imtomp.normalized() * val * 4;
			}
			if (DeshInput->LMousePressed()) {
				mpl = mp;
				imageposlatch = imagepos;
			}
			if (DeshInput->LMouseDown()) {
				imagepos = imageposlatch - (mpl - mp);
			}
			
		}
		else SetAllowInputs();
		
		SetNextItemSize(vec2(zoom * selected->width, zoom * selected->height));
		Image(selected, imagepos, 1, flags);
		
		EndChild();
		PopColor();
	}
	
	
	EndChild();
	ResetItemFlags(UIItemType_Header);
}

void DrawMaterialsWindow(){
	Storage_* st = DeshStorage;

	using namespace UI;
	SetNextWindowSize(vec2(MAX_F32, MAX_F32));
	BeginChild("StorageBrowserUI_Materials", vec2::ZERO, UIWindowFlags_NoBorder);

	Separator(5);

	SetNextWindowSize(vec2(MAX_F32, 200));
	BeginChild("StorageBrowserUI_Materials_List", vec2::ZERO, UIWindowFlags_NoInteract); {
		BeginRow("StorageBrowserUI_Materials_List", 2, 0, UIRowFlags_LookbackAndResizeToMax);
		RowSetupColumnAlignments({ {1, 0.5}, {0, 0.5} });

		forI(st->materials.count) {
			Text(toStr(i, "  ").str);
			Text(st->materials[i]->name);
		}

		EndRow();
	}EndChild();

	Separator(5);
	
	
	EndChild();
}

void DrawModelsWindow(){
	using namespace UI;
	SetNextWindowSize(vec2(MAX_F32, MAX_F32));
	BeginChild("StorageBrowserUIModels", vec2(MAX_F32, MAX_F32));
	Text("TODO");
	EndChild();
}

void DrawFontsWindow(){
	using namespace UI;
	SetNextWindowSize(vec2(MAX_F32, MAX_F32));
	BeginChild("StorageBrowserUIFonts", vec2(MAX_F32, MAX_F32));
	Text("TODO");
	EndChild();
}


void Storage::
StorageBrowserUI() {
	using namespace UI;
	PushColor(UIStyleCol_Border, Color_Grey);
	PushColor(UIStyleCol_Separator, Color_Grey);
	Begin("StorageBrowserUI", vec2::ONE * 200, vec2(400, 600));
	

	BeginTabBar("StorageBrowserUITabBar", UITabBarFlags_NoIndent);
	Separator(9);
	PushColor(UIStyleCol_HeaderBg,                0x073030ff);
	PushColor(UIStyleCol_HeaderBorder,            Color_Grey);
	PushColor(UIStyleCol_WindowBg,                Color_VeryDarkGrey);
	PushColor(UIStyleCol_ScrollBarDragger,        Color_DarkGrey);
	PushColor(UIStyleCol_ScrollBarDraggerHovered, Color_Grey);
	PushColor(UIStyleCol_ScrollBarDraggerActive,  Color_LightGrey);
	PushColor(UIStyleCol_ScrollBarBg,             Color_VeryDarkRed);
	PushColor(UIStyleCol_ScrollBarBgHovered,      Color_Grey);
	PushColor(UIStyleCol_ScrollBarBgActive,       Color_LightGrey);
	if(BeginTab("Meshes"))   {DrawMeshesWindow();    EndTab();}
	if(BeginTab("Textures")) {DrawTexturesWindow();  EndTab();}
	if(BeginTab("Materials")){DrawMaterialsWindow(); EndTab();}
	if(BeginTab("Models"))   {DrawModelsWindow();    EndTab();}
	if(BeginTab("Fonts"))    {DrawFontsWindow();     EndTab();}
	EndTabBar();
	
	End();
	PopColor(11);
}

#undef ParseError