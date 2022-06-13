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
Init(){DPZoneScoped;
	DeshiStageInitStart(DS_STORAGE, DS_RENDER, "Attempted to initialize Logger module before initializing Render module");
	
	//create the storage directories if they don't exist already
	file_create(str8_lit("data/fonts/"));
	file_create(str8_lit("data/models/"));
	file_create(str8_lit("data/textures/"));
	
	stbi_set_flip_vertically_on_load(true);
	
	//setup null assets      //TODO(delle) store null.png and null shader in a .cpp
	DeshStorage->null_mesh     = CreateBoxMesh(1.0f, 1.0f, 1.0f).second; cpystr(NullMesh()->name, "null", 64);
	//DeshStorage->null_texture  = CreateTextureFromMemory(stbi_load_from_memory(null128_png, 338, 0, 0, 0, STBI_rgb_alpha), "null", 128, 128, ImageFormat_RGBA),second;
	DeshStorage->null_texture  = CreateTextureFromFile(str8_lit("null128.png")).second;
	DeshStorage->null_material = CreateMaterial(str8_lit("null"), Shader_NULL, MaterialFlags_NONE, {0}).second;
	DeshStorage->null_model    = CreateModelFromMesh(NullMesh(), ModelFlags_NONE).second; cpystr(DeshStorage->null_model->name, "null", 64);
	
	//create null font (white square)
	DeshStorage->null_font     = (Font*)memory_alloc(sizeof(Font));
	fonts.add(DeshStorage->null_font);
	NullFont()->type = FontType_NONE;
	NullFont()->idx = 0;
	NullFont()->max_width = 6;
	NullFont()->max_height = 12;
	NullFont()->count = 1;
	NullFont()->name = STR8("null");
	u8 white_pixels[4] = {255,255,255,255};
	Texture* nf_tex = CreateTextureFromMemory(white_pixels, str8_lit("null_font"), 2, 2, ImageFormat_BW, TextureType_2D, TextureFilter_Nearest, TextureAddressMode_ClampToWhite, false).second;
	//DeleteTexture(nf_tex); //!Incomplete
	
	DeshiStageInitEnd(DS_STORAGE);
}


///////////////
//// @mesh ////
///////////////
local Mesh* 
AllocateMesh(u32 indexCount, u32 vertexCount, u32 faceCount, u32 trianglesNeighborCount, u32 facesVertexCount, u32 facesOuterVertexCount, u32 facesNeighborTriangleCount, u32 facesNeighborFaceCount){DPZoneScoped;
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
CreateBoxMesh(f32 width, f32 height, f32 depth, color color){DPZoneScoped;
	width /= 2.f; height /= 2.f; depth /= 2.f;
	pair<u32,Mesh*> result(0, NullMesh());
	
	//check if created already
	forI(meshes.size()){
		if((strcmp(meshes[i]->name, "box_mesh") == 0) && (meshes[i]->aabbMax == vec3{width,height,depth})){
			return pair<u32,Mesh*>(i, meshes[i]);
		}
	}
	
	Mesh* mesh = AllocateMesh(36, 8, 6, 36, 24, 24, 24, 24);
	cpystr(mesh->name, "box_mesh", 64);
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
	
	render_load_mesh(mesh);
	
	result.first  = mesh->idx;
	result.second = mesh;
	meshes.add(mesh);
	return result;
}

pair<u32,Mesh*> Storage::
CreateMeshFromFile(str8 filename){DPZoneScoped;
	pair<u32,Mesh*> result(0, NullMesh());
	if(str8_equal_lazy(filename, str8_lit("null"))) return result;
	
	str8_builder builder;
	str8_builder_init(&builder, str8_lit("data/models/"), deshi_temp_allocator);
	str8_builder_append(&builder, filename);
	
	//append extension if not provided
	str8 front = str8_eat_until_last(filename, '.');
	if(front.count == filename.count) str8_builder_append(&builder, str8_lit(".mesh"));
	
	//check if mesh is already loaded
	forI(meshes.count){
		if(strncmp(meshes[i]->name, (const char*)front.str, ClampMax(front.count, 63)) == 0){
			return pair<u32,Mesh*>(i, meshes[i]);
		}
	}
	
	//load .mesh file
	str8 contents = file_read_simple(str8_builder_peek(&builder), deshi_temp_allocator);
	if(!contents) return result;
	
	return CreateMeshFromMemory(contents.str);
}

pair<u32,Mesh*> Storage::
CreateMeshFromMemory(void* data){DPZoneScoped;
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
	
	render_load_mesh(mesh);
	
	result.first  = mesh->idx;
	result.second = mesh;
	meshes.add(mesh);
	return result;
}

void Storage::
SaveMesh(Mesh* mesh){DPZoneScoped;
	str8 path = str8_concat3(str8_lit("data/models/"),str8_from_cstr(mesh->name),str8_lit(".mesh"), deshi_temp_allocator);
	file_write_simple(path, mesh, mesh->bytes);
	Log("storage","Successfully saved mesh: ",path);
}


//////////////////
//// @texture ////
//////////////////
local Texture* 
AllocateTexture(){DPZoneScoped;
	Texture* texture = (Texture*)memory_alloc(sizeof(Texture));
	return texture;
}

pair<u32,Texture*> Storage::
CreateTextureFromFile(str8 filename, ImageFormat format, TextureType type, TextureFilter filter, TextureAddressMode uvMode, b32 keepLoaded, b32 generateMipmaps){DPZoneScoped;
	pair<u32,Texture*> result(0, NullTexture());
	if(str8_equal_lazy(filename, str8_lit("null"))) return result;
	
	//check if texture is already loaded
	forI(textures.count){
		if(strncmp(textures[i]->name, (const char*)filename.str, ClampMax(filename.count, 63)) == 0){
			return pair<u32,Texture*>(i, textures[i]);
		}
	}
	
	str8 path = str8_concat(str8_lit("data/textures/"),filename, deshi_temp_allocator);
	Texture* texture = AllocateTexture();
	CopyMemory(texture->name, filename.str, ClampMax(filename.count, 63));
	texture->idx     = textures.count;
	texture->format  = format;
	texture->type    = type;
	texture->filter  = filter;
	texture->uvMode  = uvMode;
	texture->pixels  = stbi_load((const char*)path.str, &texture->width, &texture->height, &texture->depth, STBI_rgb_alpha);
	texture->loaded  = true;
	if(texture->pixels == 0){ 
		LogE("storage","Failed to create texture '",path,"': ",stbi_failure_reason()); 
		memory_zfree(texture);
		return result; 
	}
	texture->mipmaps = (generateMipmaps) ? (s32)log2(Max(texture->width, texture->height)) + 1 : 1;
	
	render_load_texture(texture);
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
CreateTextureFromMemory(void* data, str8 name, s32 width, s32 height, ImageFormat format, TextureType type, TextureFilter filter, TextureAddressMode uvMode, b32 generateMipmaps){DPZoneScoped;
	pair<u32,Texture*> result(0, NullTexture());
	if(data == 0){ LogE("storage","Failed to create texture '",name,"': No memory passed!"); return result; }
	
	//check if texture is already loaded (with that name)
	forI(textures.count){
		if(strncmp(textures[i]->name, (const char*)name.str, ClampMax(name.count, 63)) == 0){
			return pair<u32,Texture*>(i, textures[i]);
		}
	}
	
	Texture* texture = AllocateTexture();
	CopyMemory(texture->name, name.str, ClampMax(name.count, 63));
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
				NotImplemented; //!Incomplete
			}break;
			case ImageFormat_RGB:{
				NotImplemented; //!Incomplete
			}break;
		}
	}else{
		texture->pixels = (u8*)data;
	}
	
	render_load_texture(texture);
	
	result.first  = texture->idx;
	result.second = texture;
	textures.add(texture);
	return result;
}


///////////////////
//// @material ////
///////////////////
local Material* 
AllocateMaterial(u32 textureCount){DPZoneScoped;
	Material* material = (Material*)memory_alloc(sizeof(Material));
	material->textures = array<u32>(textureCount);
	return material;
}

pair<u32,Material*> Storage::
CreateMaterial(str8 name, Shader shader, MaterialFlags flags, array<u32> mat_textures){DPZoneScoped;
	pair<u32,Material*> result(0, NullMaterial());
	
	//check if material is already loaded
	forI(materials.count){
		if(strncmp(materials[i]->name, (const char*)name.str, ClampMax(name.count, 63)) == 0){
			return pair<u32,Material*>(i, materials[i]);
		}
	}
	
	Material* material = AllocateMaterial(mat_textures.count);
	CopyMemory(material->name, name.str, ClampMax(name.count, 63));
	material->idx    = materials.count;
	material->shader = shader;
	material->flags  = flags;
	forI(mat_textures.count) material->textures.add(mat_textures[i]);
	
	render_load_material(material);
	
	result.first  = material->idx;
	result.second = material;
	materials.add(material);
	return result;
}

pair<u32,Material*> Storage::
CreateMaterialFromFile(str8 filename){DPZoneScoped;
	pair<u32,Material*> result(0, NullMaterial());
	if(str8_equal_lazy(filename, str8_lit("null"))) return result;
	
	str8_builder builder;
	str8_builder_init(&builder, str8_lit("data/models/"), deshi_temp_allocator);
	str8_builder_append(&builder, filename);
	
	//append extension if not provided
	str8 front = str8_eat_until_last(filename, '.');
	if(front.count == filename.count) str8_builder_append(&builder, str8_lit(".mat"));
	
	//check if material is already loaded
	forI(materials.count){
		if(strncmp(materials[i]->name, (const char*)front.str, ClampMax(front.count, 63)) == 0){
			return {(u32)i, materials[i]};
		}
	}
	
	//load .mat file
	File* file = file_init(str8_builder_peek(&builder), FileAccess_Read);
	if(!file) return result;
	defer{ file_deinit(file); };
	
	//NOTE(delle) creating an allocator here to either use 256 bytes locally or temp allocate more than 256 bytes
	persist u8 line_buffer[256];
	persist Allocator load_allocator{
		[](upt bytes){
			if(bytes > 256){
				return memory_talloc(bytes);
			}else{
				line_buffer[bytes-1] = '\0'; //NOTE(delle) file_read_line_alloc() requests an extra byte for null-terminator
				return (void*)line_buffer;
			}
		},
		Allocator_ChangeMemory_Noop,
		Allocator_ChangeMemory_Noop,
		Allocator_ReleaseMemory_Noop,
		Allocator_ResizeMemory_Noop
	};
	
	//parse .mat file
	str8 mat_name{}; //NOTE(delle) unused b/c we use the filename for loaded name currently
	Shader mat_shader = 0;
	MaterialFlags mat_flags = 0;
	array<str8> mat_textures(deshi_temp_allocator);
	enum{ HEADER_MATERIAL, HEADER_TEXTURES, HEADER_INVALID }header;
	
	u32 line_number = 0;
	while(file->cursor < file->bytes){
		line_number += 1;
		
		//next line
		str8 line = file_read_line_alloc(file, &load_allocator);
		if(!line) continue;
		
		//skip leading whitespace
		str8_advance_while(&line, ' ');
		if(!line) continue;
		
		//early out if comment is first character
		DecodedCodepoint decoded = decoded_codepoint_from_utf8(line.str, 4);
		if(decoded.codepoint == '#') continue;
		
		//check for header
		if(decoded.codepoint == '>'){
			if     (str8_begins_with(line, str8_lit(">material"))) header = HEADER_MATERIAL;
			else if(str8_begins_with(line, str8_lit(">textures"))) header = HEADER_TEXTURES;
			else{ header = HEADER_INVALID; LogE("storage","Error parsing material '",filename,"' on line ",line_number,". Invalid Header: ",line); };
			continue;
		}
		
		//early out invalid header
		if(header == HEADER_INVALID){
			LogE("storage","Error parsing material '",filename,"' on line ",line_number,". Invalid Header; skipping line");
			continue;
		}
		
		if(header == HEADER_MATERIAL){
			//parse key
			str8 key = str8_eat_until(line, ' ');
			str8_increment(&line, key.count);
			
			//skip separating whitespace
			str8_advance_while(&line, ' ');
			if(!line){
				LogE("config","Error parsing material '",filename,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			
			//early out if comment is first value character
			decoded = decoded_codepoint_from_utf8(line.str, 4);
			if(decoded.codepoint == '#'){
				LogE("storage","Error parsing material '",filename,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			
			if      (str8_equal_lazy(key, str8_lit("name"))){
				if(decoded.codepoint != '\"'){
					LogE("storage","Error parsing material '",filename,"' on line ",line_number,". Names must be wrapped in double quotes.");
					continue;
				}
				mat_name = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
			}else if(str8_equal_lazy(key, str8_lit("flags"))){
				mat_flags = (ModelFlags)atoi((const char*)line.str);
			}else if(str8_equal_lazy(key, str8_lit("shader"))){
				forI(Shader_COUNT){
					if(str8_equal_lazy(line, ShaderStrings[i])){
						mat_shader = i;
						break;
					}
				}
			}else{
				LogE("storage","Error parsing material '",filename,"' on line ",line_number,". Invalid key '",key,"' for >material header.");
				continue;
			}
		}else{
			if(decoded.codepoint != '\"'){
				LogE("storage","Error parsing material '",filename,"' on line ",line_number,". Textures must be wrapped in double quotes.");
				continue;
			}
			
			mat_textures.add(str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator));
		}
	}
	
	Material* material = AllocateMaterial(mat_textures.count);
	CopyMemory(material->name, front.str, ClampMax(front.count, 63));
	material->idx    = materials.count;
	material->shader = mat_shader;
	material->flags  = mat_flags;
	forI(mat_textures.count) material->textures.add(CreateTextureFromFile(mat_textures[i]).first);
	
	render_load_material(material);
	
	result.first  = material->idx;
	result.second = material;
	materials.add(material);
	return result;
}

void Storage::
SaveMaterial(Material* material){DPZoneScoped;
	string mat_text = ToString(">material"
							   "\nname   \"",material->name,"\""
							   "\nshader ",ShaderStrings[material->shader],
							   "\nflags  ",material->flags,
							   "\n"
							   "\n>textures");
	forI(material->textures.count) mat_text += ToString("\n\"",textures[material->textures[i]]->name,"\"");
	mat_text += "\n";
	
	str8 path = str8_concat3(str8_lit("data/models/"),str8_from_cstr(material->name),str8_lit(".mat"), deshi_temp_allocator);
	file_write_simple(path, mat_text.str, mat_text.count*sizeof(char));
	Log("storage","Successfully saved material: ",path);
}


////////////////
//// @model ////
////////////////
#define ParseError(path,...) LogE("storage","Failed parsing '",path,"' on line '",line_number,"'! ",__VA_ARGS__)

local Model* 
AllocateModel(u32 batchCount){DPZoneScoped;
	Model* model = (Model*)memory_alloc(sizeof(Model));
	model->batches = array<Model::Batch>();
	model->batches.resize((batchCount) ? batchCount : 1);
	return model;
}

pair<u32,Model*> Storage::
CreateModelFromFile(str8 filename, ModelFlags flags, b32 forceLoadOBJ){DPZoneScoped;
	pair<u32,Model*> result(0, NullModel());
	if(str8_equal_lazy(filename, str8_lit("null"))) return result;
	
	Stopwatch model_stopwatch = start_stopwatch();
	
	str8 directory = str8_lit("data/models/");
	str8_builder builder;
	str8_builder_init(&builder, directory, deshi_temp_allocator);
	str8_builder_append(&builder, filename);
	
	//append extension if not provided
	str8 front = str8_eat_until_last(filename, '.');
	if(front.count == filename.count) str8_builder_append(&builder, str8_lit(".model"));
	
	//check if model is already loaded
	forI(models.count){
		if(strncmp(models[i]->name, (const char*)front.str, ClampMax(front.count, 63)) == 0){
			return {(u32)i, models[i]};
		}
	}
	
	str8 model_path = str8_builder_peek(&builder);
	str8 obj_path  = str8_concat3(directory, front, str8_lit(".obj"),  deshi_temp_allocator);
	str8 mesh_path = str8_concat3(directory, front, str8_lit(".mesh"), deshi_temp_allocator);
	b32 parse_obj_mesh  = true;
	b32 parse_obj_model = true;
	if(!forceLoadOBJ){
		if(file_exists(mesh_path))  parse_obj_mesh  = false;
		if(file_exists(model_path)) parse_obj_model = false;
	}
	
	//NOTE(delle) creating an allocator here to either use 256 bytes locally or temp allocate more than 256 bytes
	persist u8 line_buffer[256];
	persist Allocator load_allocator{
		[](upt bytes){
			if(bytes > 256){
				return memory_talloc(bytes);
			}else{
				line_buffer[bytes-1] = '\0'; //NOTE(delle) file_read_line_alloc() requests an extra byte for null-terminator
				return (void*)line_buffer;
			}
		},
		Allocator_ChangeMemory_Noop,
		Allocator_ChangeMemory_Noop,
		Allocator_ReleaseMemory_Noop,
		Allocator_ResizeMemory_Noop
	};
	
	//// load .obj and .mtl ////
	Model* model = NullModel();
	if(parse_obj_model && parse_obj_mesh){
		//TODO(delle) use deshi allocators here
		map<vec3,Mesh::Vertex> vUnique(deshi_temp_allocator);
		set<vec3> vnUnique(deshi_temp_allocator);
		set<pair<u32,str8>> oUnique(deshi_temp_allocator); //index offset, name
		set<pair<u32,str8>> gUnique(deshi_temp_allocator);
		set<pair<u32,str8>> uUnique(deshi_temp_allocator);
		set<pair<u32,str8>> mUnique(deshi_temp_allocator);
		set<pair<u32,vec3>> appliedUniqueNormals(deshi_temp_allocator); //vertex applied on, normal
		array<vec2> vtArray(deshi_temp_allocator); //NOTE UV vertices arent expected to be unique
		array<u32> vArray(deshi_temp_allocator); //index in unique array
		array<u32> vnArray(deshi_temp_allocator);
		array<u32> oArray(deshi_temp_allocator);
		array<u32> gArray(deshi_temp_allocator);
		array<u32> uArray(deshi_temp_allocator);
		array<u32> mArray(deshi_temp_allocator);
		array<Mesh::Index>    indexes(deshi_temp_allocator);
		array<Mesh::Triangle> triangles(deshi_temp_allocator);
		array<Mesh::Face>     faces(deshi_temp_allocator);
		array<array<pair<u32,u8>>> triNeighbors(deshi_temp_allocator);
		array<array<u32>> faceTriangles(deshi_temp_allocator);
		array<set<u32>>   faceVertexes(deshi_temp_allocator);
		array<array<u32>> faceOuterVertexes(deshi_temp_allocator);
		array<array<u32>> faceTriNeighbors(deshi_temp_allocator);
		array<array<u32>> faceFaceNeighbors(deshi_temp_allocator);
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
		
		Stopwatch load_stopwatch = start_stopwatch();
		File* file = file_init(obj_path, FileAccess_Read);
		if(!file) return result;
		defer{ file_deinit(file); };
		
		u32 line_number = 0;
		while(file->cursor < file->bytes){
			line_number += 1;
			
			//next line
			str8 line = file_read_line_alloc(file, &load_allocator);
			if(!line) continue;
			
			//skip leading whitespace
			str8_advance_while(&line, ' ');
			if(!line) continue;
			
			//early out if comment is first character
			DecodedCodepoint decoded = decoded_codepoint_from_utf8(line.str, 4);
			if(decoded.codepoint == '#') continue;
			
			//TODO(delle) add parsing safety checks to strtof and strol
			//TODO(delle) handle non-triangle faces (maybe)
			switch(decoded.codepoint){
				case '\0': case '\n': case '\r': case '#': case ' ': continue; //skip empty and comment lines
				//// vertex, normal, or uv ////
				case 'v':{
					str8_increment(&line, decoded.advance);
					decoded = decoded_codepoint_from_utf8(line.str, 4);
					
					switch(decoded.codepoint){
						//// vertex ////
						case ' ':{
							str8_increment(&line, decoded.advance);
							
							char* next = (char*)line.str;
							f32 x = strtof(next, &next);
							f32 y = strtof(next, &next);
							f32 z = strtof(next, 0);
							vec3 vec{x,y,z};
							vArray.add(vUnique.add(vec, Mesh::Vertex{vec}));
						}continue;
						
						//// uv ////
						case 't':{
							str8_increment(&line, decoded.advance);
							decoded = decoded_codepoint_from_utf8(line.str, 4);
							if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'vt'"); return result; }
							str8_increment(&line, decoded.advance);
							
							char* next = (char*)line.str;
							f32 x = strtof(next, &next);
							f32 y = strtof(next, 0);
							vtArray.add(vec2{x,y});
						}continue;
						
						//// normal ////
						case 'n':{
							str8_increment(&line, decoded.advance);
							decoded = decoded_codepoint_from_utf8(line.str, 4);
							if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'vn'"); return result; }
							str8_increment(&line, decoded.advance);
							
							char* next = (char*)line.str;
							f32 x = strtof(next, &next);
							f32 y = strtof(next, &next);
							f32 z = strtof(next, 0);
							vec3 vec{x,y,z};
							vnArray.add(vnUnique.add(vec, vec));
						}continue;
						default:{
							ParseError(obj_path,"Invalid character after 'v': '",(char)decoded.codepoint,"'");
						}return result;
					}
				}continue;
				
				//// face ////
				case 'f':{
					Stopwatch face_stopwatch = start_stopwatch();
					
					str8_increment(&line, decoded.advance);
					decoded = decoded_codepoint_from_utf8(line.str, 4);
					if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'f'"); return result; }
					if(vArray.count == 0){ ParseError(obj_path,"Specifier 'f' before any 'v'"); return result; }
					
					str8_increment(&line, decoded.advance);
					char* next = (char*)line.str;
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
					Mesh::Triangle triangle{};
					triangle.p[0] = vUnique.data[vArray[v0]].pos;
					triangle.p[1] = vUnique.data[vArray[v1]].pos;
					triangle.p[2] = vUnique.data[vArray[v2]].pos;
					triangle.v[0] = vArray[v0];
					triangle.v[1] = vArray[v1];
					triangle.v[2] = vArray[v2];
					triangle.face = (u32)-1;
					triangle.normal = (triangle.p[0] - triangle.p[1]).cross(triangle.p[0] - triangle.p[2]).normalized();
					triangles.add(triangle);
					triNeighbors.add(array<pair<u32,u8>>{});
					
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
					f64 load_watch = peek_stopwatch(load_stopwatch);
					if(((u64)(load_watch / 1000.0) % 10 == 0) && ((u64)(load_watch / 1000.0) != 0)){
						Log("storage",obj_path," face ",faces.count," on line ",line_number," finished creation in ",peek_stopwatch(face_stopwatch),"ms");
					}
				}continue;
				
				//// use material ////
				case 'u':{
					if(strncmp((const char*)line.str, "usemtl ", 7) != 0){ ParseError(obj_path,"Specifier started with 'u' but didn't equal 'usemtl '"); return result; }
					
					if(mtllib_found){
						str8_increment(&line, 7);
						pair<u32,str8> usemtl(indexes.count, str8_copy(line, deshi_temp_allocator));
						uArray.add(uUnique.add(usemtl,usemtl));
					}else{
						ParseError(obj_path,"Specifier 'usemtl' used before 'mtllib' specifier");
					}
				}continue;
				
				//// load material ////
				case 'm':{
					if(strncmp((const char*)line.str, "mtllib ", 7) != 0){ ParseError(obj_path,"Specifier started with 'm' but didn't equal 'mtllib '"); return result; }
					
					mtllib_found = true;
					str8_increment(&line, 7);
					pair<u32,str8> mtllib(indexes.count, str8_copy(line, deshi_temp_allocator));
					mArray.add(mUnique.add(mtllib,mtllib));
				}continue;
				
				//// group (batch) ////
				case 'g':{
					str8_increment(&line, decoded.advance);
					decoded = decoded_codepoint_from_utf8(line.str, 4);
					if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'g'"); return result; }
					str8_increment(&line, decoded.advance);
					
					pair<u32,str8> group(indexes.count, str8_copy(line, deshi_temp_allocator));
					gArray.add(gUnique.add(group,group));
				}continue;
				
				//// object ////
				case 'o':{
					str8_increment(&line, decoded.advance);
					decoded = decoded_codepoint_from_utf8(line.str, 4);
					if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'o'"); return result; }
					str8_increment(&line, decoded.advance);
					
					pair<u32,str8> object(indexes.count, str8_copy(line, deshi_temp_allocator));
					oArray.add(oUnique.add(object,object));
				}continue;
				
				//// smoothing ////
				case 's':{
					str8_increment(&line, decoded.advance);
					decoded = decoded_codepoint_from_utf8(line.str, 4);
					if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 's'"); return result; }
					str8_increment(&line, decoded.advance);
					
					s_warning = true;
				}continue;
				default:{
					ParseError(obj_path,"Invalid starting character: '",(char)decoded.codepoint,"'");
				}return result;
			}
		}
		
		//// generate mesh faces ////
		forX(bti, triangles.count){
			if(triangles[bti].face != -1) continue;
			
			//create face and add base triange to it
			u32 cfi = faces.count;
			faces.add(Mesh::Face{});
			faceTriangles.add(array<u32>(deshi_temp_allocator));
			faceVertexes.add(set<u32>(deshi_temp_allocator));
			faceOuterVertexes.add(array<u32>(deshi_temp_allocator));
			faceTriNeighbors.add(array<u32>(deshi_temp_allocator));
			faceFaceNeighbors.add(array<u32>(deshi_temp_allocator));
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
		if(s_warning)         LogW("storage","There were 's' specifiers when parsing ",obj_path,", but those are not evaluated currently");
		if(!vtArray.count){   LogW("storage","No vertex UVs 'vt' were parsed in ",obj_path); }
		if(!vnArray.count){   LogW("storage","No vertex normals 'vn' were parsed in ",obj_path); }
		if(fatal_error){      LogE("storage","OBJ parsing encountered a fatal error in ",obj_path); return result; }
		if(!vArray.count){    LogE("storage","No vertex positions 'v' were parsed in ",obj_path); return result; }
		if(!triangles.count){ LogE("storage","No faces 'f' were parsed in ",obj_path); return result; }
		
		//// create mesh ////
		Mesh* mesh = AllocateMesh(indexes.count, vUnique.count, faces.count, totalTriNeighbors, 
								  totalFaceVertexes, totalFaceOuterVertexes, totalFaceTriNeighbors, totalFaceFaceNeighbors);
		//fill base arrays
		cpystr(mesh->name, (const char*)front.str, 64);
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
		
		render_load_mesh(mesh); //TODO(delle) check if mesh already loaded
		meshes.add(mesh);
		Log("storage","Parsing and loading OBJ '",obj_path,"' took ",peek_stopwatch(load_stopwatch),"ms");
		
		//parse MTL files
		if(mtllib_found){
			load_stopwatch = start_stopwatch();
			
			//!Incomplete
			
			Log("storage","Parsing and loading MTLs for OBJ '",obj_path,"' took ",peek_stopwatch(load_stopwatch),"ms");
		}
		
		model = AllocateModel(mArray.count);
		cpystr(model->name, (const char*)front.str, 64);
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
		Mesh* mesh = CreateMeshFromFile(front).second;
		
		set<pair<u32,str8>> oUnique(deshi_temp_allocator); //index offset, name
		set<pair<u32,str8>> gUnique(deshi_temp_allocator);
		set<pair<u32,str8>> uUnique(deshi_temp_allocator);
		set<pair<u32,str8>> mUnique(deshi_temp_allocator);
		array<u32> oArray(deshi_temp_allocator); //index in unique array
		array<u32> gArray(deshi_temp_allocator);
		array<u32> uArray(deshi_temp_allocator);
		array<u32> mArray(deshi_temp_allocator);
		b32 mtllib_found = false;
		u32 index_count = 0;
		
		Stopwatch load_stopwatch = start_stopwatch();
		File* file = file_init(obj_path, FileAccess_Read);
		if(!file) return result;
		defer{ file_deinit(file); };
		
		u32 line_number = 0;
		while(file->cursor < file->bytes){
			line_number += 1;
			
			//next line
			str8 line = file_read_line_alloc(file, &load_allocator);
			if(!line) continue;
			
			//skip leading whitespace
			str8_advance_while(&line, ' ');
			if(!line) continue;
			
			//early out if comment is first character
			DecodedCodepoint decoded = decoded_codepoint_from_utf8(line.str, 4);
			if(decoded.codepoint == '#') continue;
			
			switch(decoded.codepoint){
				//// face ////
				case 'f':{
					str8_increment(&line, decoded.advance);
					decoded = decoded_codepoint_from_utf8(line.str, 4);
					
					if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'f'"); return result; }
					index_count += 3;
				}
				
				//// use material ////
				case 'u':{ //use material
					if(strncmp((const char*)line.str, "usemtl ", 7) != 0){ ParseError(obj_path,"Specifier started with 'u' but didn't equal 'usemtl '"); return result; }
					
					if(mtllib_found){
						str8_increment(&line, 7);
						pair<u32,str8> usemtl(index_count, str8_copy(line, deshi_temp_allocator));
						uArray.add(uUnique.add(usemtl,usemtl));
					}else{
						ParseError(obj_path, "Specifier 'usemtl' used before 'mtllib' specifier");
					}
				}continue;
				
				//// load material ////
				case 'm':{
					if(strncmp((const char*)line.str, "mtllib ", 7) != 0){ ParseError(obj_path,"Specifier started with 'm' but didn't equal 'mtllib '"); return result; }
					
					mtllib_found = true;
					str8_increment(&line, 7);
					pair<u32,str8> mtllib(index_count, str8_copy(line, deshi_temp_allocator));
					mArray.add(mUnique.add(mtllib,mtllib));
				}continue;
				
				//// group (batch) ////
				case 'g':{
					str8_increment(&line, decoded.advance);
					decoded = decoded_codepoint_from_utf8(line.str, 4);
					if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'g'"); return result; }
					str8_increment(&line, decoded.advance);
					
					pair<u32,str8> group(index_count, str8_copy(line, deshi_temp_allocator));
					gArray.add(gUnique.add(group,group));
				}continue;
				
				//// object ////
				case 'o':{
					str8_increment(&line, decoded.advance);
					decoded = decoded_codepoint_from_utf8(line.str, 4);
					if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'o'"); return result; }
					str8_increment(&line, decoded.advance);
					
					pair<u32,str8> object(index_count, str8_copy(line, deshi_temp_allocator));
					oArray.add(oUnique.add(object,object));
				}continue;
				
				default: continue;
			}
		}
		
		//parse MTL files
		if(mtllib_found){
			load_stopwatch = start_stopwatch();
			
			//!Incomplete
			
			Log("storage","Parsing and loading MTLs for OBJ '",obj_path,"' took ",peek_stopwatch(load_stopwatch),"ms");
		}
		
		model = AllocateModel(mArray.count);
		cpystr(model->name, (const char*)front.str, 64);
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
		str8 model_name;
		str8 model_mesh;
		ModelFlags model_flags;
		array<pair<str8,u32,u32>> model_batches(deshi_temp_allocator);
		enum{ HEADER_MODEL, HEADER_BATCHES, HEADER_INVALID } header;
		
		File* file = file_init(model_path, FileAccess_Read);
		if(!file) return result;
		defer{ file_deinit(file); };
		
		u32 line_number = 0;
		while(file->cursor < file->bytes){
			line_number += 1;
			
			//next line
			str8 line = file_read_line_alloc(file, &load_allocator);
			if(!line) continue;
			
			//skip leading whitespace
			str8_advance_while(&line, ' ');
			if(!line) continue;
			
			//early out if comment is first character
			DecodedCodepoint decoded = decoded_codepoint_from_utf8(line.str, 4);
			if(decoded.codepoint == '#') continue;
			
			//check for headers
			if(decoded.codepoint == '>'){
				if     (str8_begins_with(line, str8_lit(">model"))) header = HEADER_MODEL;
				else if(str8_begins_with(line, str8_lit(">batches"))) header = HEADER_BATCHES;
				else{ header = HEADER_INVALID; LogE("storage","Error parsing model '",model_path,"' on line ",line_number,". Invalid Header: ",line); };
				continue;
			}
			
			//early out invalid header
			if(header == HEADER_INVALID){
				LogE("storage","Error parsing model '",model_path,"' on line ",line_number,". Invalid Header; skipping line");
				continue;
			}
			
			if(header == HEADER_MODEL){
				//parse key
				str8 key = str8_eat_until(line, ' ');
				str8_increment(&line, key.count);
				
				//skip separating whitespace
				str8_advance_while(&line, ' ');
				if(!line){
					LogE("config","Error parsing model '",model_path,"' on line ",line_number,". No value passed to key: ",key);
					continue;
				}
				
				//early out if comment is first value character
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				if(decoded.codepoint == '#'){
					LogE("storage","Error parsing model '",model_path,"' on line ",line_number,". No value passed to key: ",key);
					continue;
				}
				
				if      (str8_equal_lazy(key, str8_lit("name"))){
					if(decoded.codepoint != '\"'){
						LogE("storage","Error parsing model '",model_path,"' on line ",line_number,". Names must be wrapped in double quotes.");
						continue;
					}
					model_name = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
				}else if(str8_equal_lazy(key, str8_lit("flags"))){
					model_flags = (ModelFlags)atoi((const char*)line.str);
				}else if(str8_equal_lazy(key, str8_lit("mesh"))){
					if(decoded.codepoint != '\"'){
						LogE("storage","Error parsing model '",model_path,"' on line ",line_number,". Filenames must be wrapped in double quotes.");
						continue;
					}
					model_mesh = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
				}else if(str8_equal_lazy(key, str8_lit("armature"))){
					//NOTE currently nothing
				}else{
					LogE("storage","Error parsing model '",model_path,"' on line ",line_number,". Invalid key '",key,"' for >model header.");
					continue;
				}
			}else{
				if(decoded.codepoint != '\"'){
					LogE("storage","Error parsing model '",model_path,"' on line ",line_number,". Names must be wrapped in double quotes. Batch format: '\"material_name\" index_offset index_count'");
					continue;
				}
				
				str8 batch_mat = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
				str8_increment(&line, batch_mat.count+2);
				if(!line){
					LogE("storage","Error parsing model '",model_path,"' on line ",line_number,". No indexes passed. Batch format: '\"material_name\" index_offset index_count'");
					continue;
				}
				
				char* next = (char*)line.str;
				u32 ioffset = strtol(next,&next,10);
				u32 icount  = strtol(next,&next,10);
				
				model_batches.add({batch_mat, ioffset, icount});
			}
		}
		
		model = AllocateModel(model_batches.count);
		cpystr(model->name, (const char*)model_name.str, 64);
		model->idx      = models.count;
		model->flags    = model_flags;
		model->mesh     = CreateMeshFromFile(model_mesh).second;
		model->armature = 0;
		forI(model_batches.count){
			model->batches[i] = Model::Batch{
				model_batches[i].second,
				model_batches[i].third,
				CreateMaterialFromFile(model_batches[i].first).first
			};
		}
		
		Log("storage","Successfully loaded model ",model_path);
	}
	
	result.first  = model->idx;
	result.second = model;
	models.add(model);
	Log("storage","Finished loading model '",filename,"' in ",peek_stopwatch(model_stopwatch),"ms");
	return result;
}

pair<u32,Model*> Storage::
CreateModelFromMesh(Mesh* mesh, ModelFlags flags){DPZoneScoped;
	pair<u32,Model*> result(0, NullModel());
	
	str8 model_name = str8_from_cstr(mesh->name);
	//check if created already
	forI(models.count){
		if((models[i]->mesh == mesh) && (strncmp(models[i]->name, (const char*)model_name.str, 64) == 0) && (models[i]->flags == flags) 
		   && (models[i]->batches.count == 1) && (models[i]->batches[0].indexOffset == 0)
		   && (models[i]->batches[0].indexCount == mesh->indexCount) && (models[i]->batches[0].material == 0)){
			return pair<u32,Model*>(i,models[i]);
		}
	}
	
	Model* model = AllocateModel(1);
	cpystr(model->name, (const char*)model_name.str, 64);
	model->idx = models.count;
	model->mesh = mesh;
	model->armature = 0;
	model->batches[0] = {0, mesh->indexCount, 0};
	
	result.first  = model->idx;
	result.second = model;
	models.add(model);
	return result;
}

pair<u32,Model*> Storage::
CopyModel(Model* _model){DPZoneScoped;
	pair<u32,Model*> result(0, NullModel());
	
	Model* model = AllocateModel(_model->batches.size());
	cpystr(model->name, _model->name, 64);
	model->idx      = models.count;
	model->flags    = _model->flags;
	model->mesh     = _model->mesh;
	model->armature = _model->armature;
	forI(model->batches.count){
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
SaveModel(Model* model){DPZoneScoped;
	SaveMesh(model->mesh);
	string model_save = ToString(">model"
								 "\nname     \"",model->name,"\""
								 "\nflags    ", model->flags,
								 "\nmesh     \"", model->mesh->name,"\""
								 "\narmature ", 0,
								 "\n"
								 "\n>batches");
	forI(model->batches.count){
		SaveMaterial(materials[model->batches[i].material]);
		model_save += ToString("\n\"",materials[model->batches[i].material]->name,"\" ",
							   model->batches[i].indexOffset," ",model->batches[i].indexCount);
	}
	model_save += "\n";
	
	str8 path = str8_concat3(str8_lit("data/models/"),str8_from_cstr(model->name),str8_lit(".model"), deshi_temp_allocator);
	file_write_simple(path, model_save.str, model_save.count*sizeof(char));
	Log("storage","Successfully saved model: ",path);
}


///////////////
//// @font ////
///////////////
local Font* 
AllocateFont(Type type){DPZoneScoped;
	Font* font = (Font*)memory_alloc(sizeof(Font));
	font->type = type;
	font->idx = Storage::fonts.count;
	return font;
}

pair<u32,Font*> Storage::
CreateFontFromFileBDF(str8 filename){DPZoneScoped;
	pair<u32,Font*> result(0,NullFont());
	
	//check if created already
	forI(fonts.count){
		if(!str8_compare(fonts[i]->name, filename)){
			return pair<u32,Font*>(i,fonts[i]);
		}
	}
	
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
	
	//NOTE(delle) creating an allocator here to either use 256 bytes locally or temp allocate more than 256 bytes
	persist u8 line_buffer[256];
	persist Allocator load_allocator{
		[](upt bytes){
			if(bytes > 256){
				return memory_talloc(bytes);
			}else{
				line_buffer[bytes-1] = '\0'; //NOTE(delle) file_read_line_alloc() requests an extra byte for null-terminator
				return (void*)line_buffer;
			}
		},
		Allocator_ChangeMemory_Noop,
		Allocator_ChangeMemory_Noop,
		Allocator_ReleaseMemory_Noop,
		Allocator_ResizeMemory_Noop
	};
	
	//init file
	str8 path = str8_concat(str8_lit("data/fonts/"), filename, deshi_temp_allocator);
	File* file = file_init(path, FileAccess_Read);
	if(!file) return result;
	defer{ file_deinit(file); };
	
	str8 first_line = file_read_line_alloc(file, &load_allocator);
	if(!str8_begins_with(first_line, str8_lit("STARTFONT"))){
		LogE("storage","Error parsing BDF '",filename,"' on line 1. The file did not begin with 'STARTFONT'.");
		return result;
	}

	
	Font* font = AllocateFont(FontType_BDF);
	font->name = filename;
	u32 line_number = 1;
	while(file->cursor < file->bytes){
		line_number += 1;
		
		//next line
		str8 line = file_read_line_alloc(file, &load_allocator);
		if(!line) continue;
		
		//skip leading whitespace
		str8_advance_while(&line, ' ');
		if(!line) continue;
		
		//parse key
		str8 key = str8_eat_until(line, ' ');
		str8_increment(&line, key.count);
		
		//skip separating whitespace
		str8_advance_while(&line, ' ');
		
		if(in_bitmap){
			if(str8_equal_lazy(key, str8_lit("ENDCHAR"))){
				in_char = false;
				in_bitmap = false;
				char_idx++;
				bitmap_row = 0;
				continue;
			}
			Assert(bitmap_row < current_bbx.y);
			
			s32 chars = (s32)key.count;
			Assert(chars <= 4); //TODO(delle) error handling: max 16 pixel width
			u8 scaled[16]{};
			
			//scale each byte to represent one pixel per bit then byteswap the u64 //TODO(delle) only byteswap if little-endian
			//ref: https://stackoverflow.com/questions/9023129/algorithm-for-bit-expansion-duplication/9044057#9044057
			//scale = 8 (1 bit to 1 byte)
			//mask0 = 0x0000000f0000000f, mask1 = 0x0003000300030003, mask2 = 0x0101010101010101
			//shift0 = (1 << 28) + 1, shift1 = (1 << 14) + 1, shift2 = (1 << 7) + 1
			for(s32 i=0; i<chars; i+=2){
				u64 reversed = (((strtoll((const char*)key.str+i,0,16) * 0x10000001 & 0x0000000f0000000f) 
								 * 0x4001 & 0x0003000300030003) * 0x81 & 0x0101010101010101) * 255;
				*(u64*)(scaled+i) = ByteSwap64(reversed);
			}
			CopyMemory(pixels+2*font->max_width+(upt)(glyph_offset + (bitmap_row+top_offset)*font->max_width + left_offset), scaled, upt(current_bbx.x*sizeof(u8)));
			
			bitmap_row++;
			continue;
		}
		
		if(in_char){
			if      (str8_equal_lazy(key, str8_lit("ENCODING"))){
				encodings[char_idx] = strtol((const char*)line.str, 0, 10);
			}else if(str8_equal_lazy(key, str8_lit("BITMAP"))){
				in_bitmap = true;
			}else if(str8_equal_lazy(key, str8_lit("SWIDTH"))){
				//unused
			}else if(str8_equal_lazy(key, str8_lit("DWIDTH"))){
				//unused in monospace fonts
			}else if(str8_equal_lazy(key, str8_lit("BBX"))){
				char* cursor = (char*)line.str;
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
		
		if      (str8_equal_lazy(key, str8_lit("STARTCHAR"))){
			in_char = true;
		}else if(str8_equal_lazy(key, str8_lit("SIZE"))){
			if(!line){
				LogE("storage","Error parsing BDF '",filename,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			char* cursor = (char*)line.str;
			font_dpi.x = (f32)strtol(cursor+1, &cursor, 10);
			font_dpi.y = (f32)strtol(cursor+1, &cursor, 10);
		}else if(str8_equal_lazy(key, str8_lit("FONTBOUNDINGBOX"))){
			if(!line){
				LogE("storage","Error parsing BDF '",filename,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			char* cursor = (char*)line.str;
			font_bbx.x = (f32)strtol(cursor,   &cursor, 10); //width
			font_bbx.y = (f32)strtol(cursor+1, &cursor, 10); //height
			font_bbx.z = (f32)strtol(cursor+1, &cursor, 10); //lower-left x
			font_bbx.w = (f32)strtol(cursor+1, &cursor, 10); //lower-left y
			font->max_width  = (u32)font_bbx.x;
			font->max_height = (u32)font_bbx.y;
		}else if(str8_equal_lazy(key, str8_lit("FONT_NAME"))){
			if(!line){
				LogE("storage","Error parsing BDF '",filename,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			if(decoded_codepoint_from_utf8(line.str, 4).codepoint != '\"'){
				LogE("storage","Error parsing BDF '",filename,"' on line ",line_number,". FONT_NAME must be wrapped in double quotes.");
				continue;
			}
			str8 font_name = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
			//TODO(sushi) replace name on Font with filename and add a name for this guy right here!
		}else if(str8_equal_lazy(key, str8_lit("WEIGHT_NAME"))){
			if(!line){
				LogE("storage","Error parsing BDF '",filename,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			if(decoded_codepoint_from_utf8(line.str, 4).codepoint != '\"'){
				LogE("storage","Error parsing BDF '",filename,"' on line ",line_number,". WEIGHT_NAME must be wrapped in double quotes.");
				continue;
			}
			str8 font_weight = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
			cpystr(font->weight, (const char*)font_weight.str, 64);
		}else if(str8_equal_lazy(key, str8_lit("CHARS"))){
			font->count = strtol((const char*)line.str, 0, 10);
			Assert(font->max_width && font->max_height && font->count);
			encodings = (u16*)memory_talloc(font->count*sizeof(u16));
			pixels = (u8*)memory_talloc(font->count * ((font->max_width*font->max_height + 2*font->max_width) * sizeof(u8)));
			pixels[0] = 255;
			pixels[1] = 255;
			pixels[font->max_width] = 255;
			pixels[font->max_width + 1] = 255;
			font->uv_yoffset = 2.f / (font->max_height * font->count + 2);
		}else{
			continue;
		}
	}
	
	Texture* texture = CreateTextureFromMemory(pixels, filename, font->max_width, font->max_height*font->count,
											   ImageFormat_BW, TextureType_2D, TextureFilter_Nearest, TextureAddressMode_ClampToWhite, false).second;
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
CreateFontFromFileTTF(str8 filename, u32 size){DPZoneScoped;
	pair<u32,Font*> result(0,NullFont());
	
	//check if created already
	//TODO look into why if we load the same font w a different size it gets weird (i took that check out of here for now)
	forI(fonts.count){
		if(!str8_compare(fonts[i]->name, filename)){
			return pair<u32,Font*>(i,fonts[i]);
		}
	}
	
	str8 path = str8_concat(str8_lit("data/fonts/"),filename, deshi_temp_allocator);
	str8 contents = file_read_simple(path, deshi_temp_allocator);
	if(!contents) return result;
	
	//Codepoint Ranges to Load:
	// ASCII              32 - 126  ~  94 chars
	// Greek and Coptic  880 - 1023 ~ 143 chars
	// Cyrillic         1024 - 1279 ~ 256 chars
	// Super/Subscripts 8304 - 8348 ~  44 chars (we will want our own method for doing super/subscripts in suugu)
	// Currency Symbols 8352 - 8384 ~  32 chars
	// Arrows           8592 - 8703 ~ 111 chars
	// Math Symbols     8704 - 8959 ~ 255 chars
	// ...and maybe more to come in the future.
	
	//TODO(sushi) maybe implement taking in ranges
	u32 num_ranges = 7;
	stbtt_pack_range* ranges = (stbtt_pack_range*)memory_alloc(num_ranges*sizeof(*ranges));
	ranges[0].num_chars = 94;   ranges[0].first_unicode_codepoint_in_range = 32;
	ranges[1].num_chars = 143;  ranges[1].first_unicode_codepoint_in_range = 880;
	ranges[2].num_chars = 255;  ranges[2].first_unicode_codepoint_in_range = 1024;
	ranges[3].num_chars = 44;   ranges[3].first_unicode_codepoint_in_range = 8304;
	ranges[4].num_chars = 32;   ranges[4].first_unicode_codepoint_in_range = 8352;
	ranges[5].num_chars = 111;  ranges[5].first_unicode_codepoint_in_range = 8592;
	ranges[6].num_chars = 255;  ranges[6].first_unicode_codepoint_in_range = 8704;
	ranges[0].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[0].num_chars*sizeof(stbtt_packedchar));
	ranges[1].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[1].num_chars*sizeof(stbtt_packedchar));
	ranges[2].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[2].num_chars*sizeof(stbtt_packedchar));
	ranges[3].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[3].num_chars*sizeof(stbtt_packedchar));
	ranges[4].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[4].num_chars*sizeof(stbtt_packedchar));
	ranges[5].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[5].num_chars*sizeof(stbtt_packedchar));
	ranges[6].chardata_for_range = (stbtt_packedchar*)memory_alloc(ranges[6].num_chars*sizeof(stbtt_packedchar));
	ranges[0].font_size = (f32)size;
	ranges[1].font_size = (f32)size;
	ranges[2].font_size = (f32)size;
	ranges[3].font_size = (f32)size;
	ranges[4].font_size = (f32)size;
	ranges[5].font_size = (f32)size;
	ranges[6].font_size = (f32)size;
	
	/*
	//trying to minimize the texture here, but its difficult due to stbtt packing all of them together
	//i believe this makes it into the smallest square it could be w/o knowing how stbtt packs them together
	//also just doesnt really work well with non-monospaced fonts
	//TODO figure out a better way to do this.
	int x0, y0, x1, y1;
	stbtt_GetFontBoundingBox(&info, &x0, &y0, &x1, &y1);
	f32 widthmax = x1 - x0, heightmax = y1 - y0;
	f32 aspect_ratio = (f32)heightmax / (f32)widthmax;
	u32 tsy = (u32)roundUpToPow2(ceil(size * sqrtf(679) / aspect_ratio));
	u32 tsx = (u32)roundUpToPow2(ceil(widthmax * size /  heightmax * sqrtf(679)) + 2); //add two rows to make room for 4 white pixels to optimize uicmds
	if(tsy > tsx) tsx = tsy;
	if(tsx > tsy) tsy = tsx;
	*/
	
	//init the font info
	int success;
	stbtt_fontinfo info;
	success = stbtt_InitFont(&info, contents.str, 0); Assert(success);
	
	//determine surface area of loadable codepoints  !ref:imgui_draw.cpp@ImFontAtlasBuildWithStbTruetype()
	//TODO(delle) maybe use this info to index into the final texture for rendering
	f32 glyph_scale = stbtt_ScaleForPixelHeight(&info, (f32)size);
	u32 glyph_count = 0;
	int glyph_padding = 1;
	int total_surface = 0;
	forX(range, num_ranges){
		for(u32 codepoint = ranges[range].first_unicode_codepoint_in_range;
			codepoint < ranges[range].first_unicode_codepoint_in_range + ranges[range].num_chars;
			++codepoint)
		{
			int glyph_index_in_font = stbtt_FindGlyphIndex(&info, codepoint);
			if(glyph_index_in_font == 0) continue; //skip any glyphs not in the font
			
			int x0, y0, x1, y1;
			stbtt_GetGlyphBitmapBoxSubpixel(&info, glyph_index_in_font, glyph_scale*1, glyph_scale*1, 0, 0, &x0, &y0, &x1, &y1);
			total_surface += ((x1 - x0) + glyph_padding) * ((y1 - y0) + glyph_padding);
		}
	}
	
	//determine texture size
	//NOTE(delle) texture width is capped due to size limitations on width for GPUs
	int surface_sqrt = (int)sqrt((f32)total_surface) + 1;
	u32 texture_size_x = (surface_sqrt >= 4096 * 0.7f) ? 4096 : (surface_sqrt >= 2048 * 0.7f) ? 2048 : (surface_sqrt >= 1024 * 0.7f) ? 1024 : 512;
	u32 texture_size_y = texture_size_x; //TODO(delle) find accurate height based on packing
	Assert(surface_sqrt <= 4096, "we don't yet handle when the height should be greater than width, see ImGui reference for solution");
	
	//place 4 white pixels in the top-left corner for other UI usage without having to swap textures
	u8* pixels = (u8*)memory_talloc((texture_size_x * texture_size_y)*sizeof(u8));
	pixels[0] = 255; pixels[1] = 255; pixels[texture_size_x] = 255; pixels[texture_size_x+1] = 255;
	
	//perform the font packing
	stbtt_pack_context* pc = (stbtt_pack_context*)memory_alloc(1*sizeof(stbtt_pack_context));
	success = stbtt_PackBegin(pc, pixels + 2*texture_size_x, texture_size_x, texture_size_y-2, 0, glyph_padding, 0); Assert(success);
	stbtt_PackSetSkipMissingCodepoints(pc, true);
	success = stbtt_PackFontRanges(pc, contents.str, 0, ranges, num_ranges); //NOTE(delle) this will return 0 if there are any missing codepoints
	stbtt_PackEnd(pc);
	
	/*
	//normalize and offset the UVs
	forX(range, num_ranges){
		for(u32 codepoint = ranges[range].first_unicode_codepoint_in_range;
			codepoint < ranges[range].first_unicode_codepoint_in_range + ranges[range].num_chars;
			++codepoint)
		{
			
		}
	}
	*/
	
	//get extra font rendering info
	f32 ascent, descent, lineGap;
	stbtt_GetScaledFontVMetrics(contents.str, 0, (f32)size, &ascent, &descent, &lineGap);
	int x0, y0, x1, y1;
	stbtt_GetFontBoundingBox(&info, &x0, &y0, &x1, &y1);
	int max_width = x1 - x0, max_height = y1 - y0;
	f32 aspect_ratio = (f32)max_height / (f32)max_width;
	
	Texture* texture = CreateTextureFromMemory(pixels, filename, texture_size_x, texture_size_y,
											   ImageFormat_BW, TextureType_2D, TextureFilter_Nearest,
											   TextureAddressMode_ClampToWhite, false).second;
	
	Font* font = AllocateFont(FontType_TTF);
	font->name         = filename;
	font->max_width    = (u32)((f32)max_width / (f32)max_height * (f32)size);
	font->max_height   = size;
	font->count        = glyph_count;
	font->ttf_size[0]  = texture_size_x;
	font->ttf_size[1]  = texture_size_y;
	font->num_ranges   = num_ranges;
	font->uv_yoffset   = 2.0f / (f32)texture_size_y;
	font->ascent       = ascent;
	font->descent      = descent;
	font->line_gap     = lineGap;
	font->aspect_ratio = aspect_ratio;
	font->tex          = texture;
	font->ranges       = (pack_range*)ranges;
	
	fonts.add(font);
	result.first  = font->idx;
	result.second = font;
	return result;
}

pair<u32,Font*> Storage::
CreateFontFromFile(str8 filename, u32 height){DPZoneScoped;
	if(str8_ends_with(filename, str8_lit(".bdf"))){
		return CreateFontFromFileBDF(filename);
	}
	
	if(str8_ends_with(filename, str8_lit(".ttf")) || str8_ends_with(filename, str8_lit(".otf"))){
		return CreateFontFromFileTTF(filename, height);
	}
	
	LogE("storage","Failed to load font '",filename,"'. We only support loading TTF/OTF and BDF fonts at the moment.");
	return {};
}

void DrawMeshesWindow() {DPZoneScoped; 
	using namespace UI;
	SetNextWindowSize(vec2(MAX_F32, MAX_F32));
	BeginChild(str8_lit("StorageBrowserUIMeshes"), vec2(MAX_F32, MAX_F32));
	Text(str8_lit("TODO"));
	EndChild();
}

void DrawTexturesWindow() {DPZoneScoped;
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
	BeginChild(str8_lit("StorageBrowserUI_Textures"), vec2::ZERO, UIWindowFlags_NoBorder);
	
	BeginRow(str8_lit("StorageBrowserUI_Row1"),2, 0, UIRowFlags_AutoSize);
	RowSetupColumnAlignments({ {1, 0.5}, {0, 0.5} });
	
	TextF(str8_lit("Textures Loaded: %d"),       st->textures.count);
	TextF(str8_lit("Memory Occupied: %lld %cB"), texture_bytes / bytesDivisor(texture_bytes), bytesUnit(texture_bytes));
	
	EndRow();
	
	if (BeginCombo(str8_lit("StorageBrowserUI_Texture_Selection_Combo"), (selected ? str8_from_cstr(selected->name) : str8_lit("select texture")))) {
		for (Texture* t : st->textures) {
			if (Selectable(str8_from_cstr(t->name), t == selected)) {
				selected = t;
				new_selected = 1;
			}
		}
		EndCombo();
	}
	
	Separator(9);
	
	if (BeginHeader(str8_lit("Stats"))) {
		BeginRow(str8_lit("StorageBrowserUI_Row2"), 3, 0, UIRowFlags_AutoSize);
		RowSetupColumnAlignments({ {1, 0.5}, {0, 0.5}, {0.5, 0.5} });
		
		TextF(str8_lit("Largest Texture: %s"), largest->name);
		if (Button(str8_lit("select"))) { selected = largest; new_selected = 1;}
		
		TextF(str8_lit("Smallest Texture: %s"), smallest->name);
		if (Button(str8_lit("select"))) { selected = smallest; new_selected = 1; }
		
		EndRow();
		
		EndHeader();
	}
	
	Separator(9);
	
	if (selected) {
		BeginRow(str8_lit("StorageBrowserUI_Texture_Selected"), 2, 0, UIRowFlags_AutoSize);
		RowSetupColumnAlignments({ {0, 0.5}, {0, 0.5} });
		
		u32 texbytes = selected->width * selected->height * u8size;
		
		TextF(str8_lit("Name: %s"), selected->name);
		TextF(str8_lit("Index: %d"), selected->idx);
		TextF(str8_lit("Width: %d"), selected->width);
		TextF(str8_lit("Height: %d"), selected->height);
		TextF(str8_lit("Depth: %d"), selected->depth);
		TextF(str8_lit("MipMaps: %d"), selected->mipmaps);
		TextF(str8_lit("Format: %s"), ImageFormatStrings[selected->format - 1].str);
		TextF(str8_lit("Type: %s"), TextureTypeStrings[selected->type].str);
		TextF(str8_lit("Filter: %s"), TextureFilterStrings[selected->filter].str);
		TextF(str8_lit("UV Mode: %s"), TextureAddressModeStrings[selected->uvMode].str);
		TextF(str8_lit("Memory Used: %lld %cB"), texbytes / bytesDivisor(texbytes), bytesUnit(texbytes));
		
		EndRow();
		PushColor(UIStyleCol_WindowBg, 0x073030ff);
		
		SetNextWindowSize(vec2(MAX_F32, MAX_F32));
		BeginChild(str8_lit("StorageBrowserUI_Texture_ImageInspector"), vec2::ZERO, UIWindowFlags_NoInteract);
		persist f32  zoom = 300;
		persist vec2 mpl;
		persist vec2 imagepos;
		persist vec2 imageposlatch;
		persist UIImageFlags flags;
		
		vec2 mp = input_mouse_position();
		
		if (Button(str8_lit("Flip x"))) 
			ToggleFlag(flags, UIImageFlags_FlipX);
		SameLine();
		if (Button(str8_lit("Flip y"))) 
			ToggleFlag(flags, UIImageFlags_FlipY);
		
		if (new_selected) {
			zoom = f32(GetWindow()->width) / selected->width ;
			//imagepos = vec2(
			//				(GetWindow()->width - selected->width) / 2,
			//				(GetWindow()->height - selected->height) / 2
			//				);
			imagepos = vec2::ZERO;
		}
		
		string z = toStr(zoom);
		Text(str8{(u8*)z.str, (s64)z.count});
		
		if (IsWinHovered()) {
			SetPreventInputs();
			
			if (DeshInput->scrollY) {
				f32 val = 10 * DeshInput->scrollY;
				zoom += zoom / val;
				//TODO make it zoom to the mouse 
				vec2 imtomp = (mp - GetWindow()->position) - GetWindow()->dimensions / 2;
				//imagepos -= imtomp.normalized() * val * 4;
			}
			if (input_lmouse_pressed()) {
				mpl = mp;
				imageposlatch = imagepos;
			}
			if (input_lmouse_down()) {
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

void DrawMaterialsWindow(){DPZoneScoped;
	Storage_* st = DeshStorage;
	
	using namespace UI;
	SetNextWindowSize(vec2(MAX_F32, MAX_F32));
	BeginChild(str8_lit("StorageBrowserUI_Materials"), vec2::ZERO, UIWindowFlags_NoBorder);
	
	Separator(5);
	
	SetNextWindowSize(vec2(MAX_F32, 200));
	BeginChild(str8_lit("StorageBrowserUI_Materials_List"), vec2::ZERO, UIWindowFlags_NoInteract); {
		BeginRow(str8_lit("StorageBrowserUI_Materials_List"), 2, 0, UIRowFlags_AutoSize);
		RowSetupColumnAlignments({ {1, 0.5}, {0, 0.5} });
		
		forI(st->materials.count) {
			string s = toStr(i, "  ");
			Text(str8{(u8*)s.str, (s64)s.count});
			Text(str8_from_cstr(st->materials[i]->name));
		}
		
		EndRow();
	}EndChild();
	
	Separator(5);
	
	
	EndChild();
}

void DrawModelsWindow(){DPZoneScoped;
	using namespace UI;
	SetNextWindowSize(vec2(MAX_F32, MAX_F32));
	BeginChild(str8_lit("StorageBrowserUIModels"), vec2(MAX_F32, MAX_F32));
	Text(str8_lit("TODO"));
	EndChild();
}

void DrawFontsWindow(){DPZoneScoped;
	using namespace UI;
	SetNextWindowSize(vec2(MAX_F32, MAX_F32));
	BeginChild(str8_lit("StorageBrowserUIFonts"), vec2(MAX_F32, MAX_F32));
	Text(str8_lit("TODO"));
	EndChild();
}


void Storage::
StorageBrowserUI() {DPZoneScoped;
	using namespace UI;
	PushColor(UIStyleCol_Border, Color_Grey);
	PushColor(UIStyleCol_Separator, Color_Grey);
	Begin(str8_lit("StorageBrowserUI"), vec2::ONE * 200, vec2(400, 600));
	
	
	BeginTabBar(str8_lit("StorageBrowserUITabBar"), UITabBarFlags_NoIndent);
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
	if(BeginTab(str8_lit("Meshes")))   {DrawMeshesWindow();    EndTab();}
	if(BeginTab(str8_lit("Textures"))) {DrawTexturesWindow();  EndTab();}
	if(BeginTab(str8_lit("Materials"))){DrawMaterialsWindow(); EndTab();}
	if(BeginTab(str8_lit("Models")))   {DrawModelsWindow();    EndTab();}
	if(BeginTab(str8_lit("Fonts")))    {DrawFontsWindow();     EndTab();}
	EndTabBar();
	
	End();
	PopColor(11);
}

#undef ParseError