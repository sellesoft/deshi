//NOTE(delle) creating an allocator here to either use 256 bytes locally or temp allocate more than 256 bytes
persist u8 assets_line_buffer[256];
persist Allocator assets_load_allocator{
	[](upt bytes){
		if(bytes > 256){
			return memory_talloc(bytes);
		}else{
			assets_line_buffer[bytes-1] = '\0'; //NOTE(delle) file_read_line_alloc() requests an extra byte for null-terminator
			return (void*)assets_line_buffer;
		}
	},
	Allocator_ChangeMemory_Noop,
	Allocator_ChangeMemory_Noop,
	Allocator_ReleaseMemory_Noop,
	Allocator_ResizeMemory_Noop
};

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @assets_system
void
assets_init(){DPZoneScoped;
	DeshiStageInitStart(DS_ASSETS, DS_RENDER, "Attempted to initialize Assets module before initializing Render module");
	
	//create the assets directories if they don't exist already
	file_create(STR8("data/fonts/"));
	file_create(STR8("data/models/"));
	file_create(STR8("data/textures/"));
	
	//setup arrays
	arrsetcap(DeshAssets->mesh_array,256);
	arrsetcap(DeshAssets->texture_array,256);
	arrsetcap(DeshAssets->material_array,256);
	arrsetcap(DeshAssets->model_array,256);
	arrsetcap(DeshAssets->font_array,8);
	
	//setup stb_image.h
	stbi_set_flip_vertically_on_load(true);
	
	//setup null assets
	assets_mesh_create_box(1.0f, 1.0f, 1.0f, Color_White.rgba); cpystr(assets_mesh_null()->name, "null", 64);
	int null_x, null_y, null_channels;
	unsigned char* null_data = stbi_load_from_memory(baked_texture_null128_png, sizeof(baked_texture_null128_png), &null_x, &null_y, &null_channels, STBI_rgb_alpha);
	assets_texture_create_from_memory(null_data, STR8("null"), 128, 128, ImageFormat_RGBA, TextureType_2D, TextureFilter_Nearest, TextureAddressMode_Repeat, false);
	stbi_image_free(null_data);
	assets_material_create(STR8("null"), Shader_NULL, MaterialFlags_NONE, DeshAssets->texture_array, 1);
	assets_model_create_from_mesh(assets_mesh_null(), ModelFlags_NONE); cpystr(assets_model_null()->name, "null", 64);
	
	//create null font (white square)
	Font* null_font = arrput(DeshAssets->font_array, (Font*)memory_alloc(sizeof(Font)));
	null_font->type = FontType_NONE;
	null_font->max_width = 6;
	null_font->max_height = 12;
	null_font->count = 1;
	null_font->name = STR8("null");
	u8 white_pixels[4] = {255,255,255,255};
	Texture* nf_tex = assets_texture_create_from_memory(white_pixels, STR8("null_font"), 2, 2, ImageFormat_BW, TextureType_2D,
														TextureFilter_Nearest, TextureAddressMode_ClampToWhite, false);
	assets_texture_delete(nf_tex);
	
	DeshiStageInitEnd(DS_ASSETS);
}


void
assets_reset(){DPZoneScoped;
	for_stb_array(DeshAssets->mesh_array) assets_mesh_delete(*it);
	for_stb_array(DeshAssets->texture_array) assets_texture_delete(*it);
	for_stb_array(DeshAssets->material_array) assets_material_delete(*it);
	for_stb_array(DeshAssets->model_array) assets_model_delete(*it);
	for_stb_array(DeshAssets->font_array) assets_font_delete(*it);
}


void
assets_browser(){DPZoneScoped;
	using namespace UI;
	PushColor(UIStyleCol_Border, Color_Grey);
	PushColor(UIStyleCol_Separator, Color_Grey);
	Begin(STR8("AssetsBrowserUI"), vec2::ONE * 200, Vec2(400, 600));
	
	
	BeginTabBar(STR8("AssetsBrowserUITabBar"), UITabBarFlags_NoIndent);
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
	if(BeginTab(STR8("Meshes")))
	{
		SetNextWindowSize(Vec2(MAX_F32, MAX_F32));
		BeginChild(STR8("AssetsBrowserUIMeshes"), Vec2(MAX_F32, MAX_F32));
		TextOld(STR8("TODO"));
		EndChild();
		
		EndTab();
	}
	if(BeginTab(STR8("Textures")))
	{
		//TODO make all of this stuff get checked only when necessary
		b32 new_selected = 0;
		persist Texture* selected = 0;
		
		Texture* largest = DeshAssets->texture_array[0];
		Texture* smallest = DeshAssets->texture_array[0];
		
		//gather size of textures in memory
		upt texture_bytes = 0;
		
		
		for_stb_array(DeshAssets->texture_array){
			texture_bytes += (*it)->width * (*it)->height * u8size;
			if((*it)->width * (*it)->height > largest->width * largest->height)   largest = (*it);
			if((*it)->width * (*it)->height < smallest->width * smallest->height) smallest = (*it);
		}
		
		AddItemFlags(UIItemType_Header, UIHeaderFlags_NoBorder);
		
		
		SetNextWindowSize(Vec2(MAX_F32, MAX_F32));
		BeginChild(STR8("AssetsBrowserUI_Textures"), vec2::ZERO, UIWindowFlags_NoBorder);
		
		BeginRow(STR8("AssetsBrowserUI_Row1"),2, 0, UIRowFlags_AutoSize);
		RowSetupColumnAlignments({ {1, 0.5}, {0, 0.5} });
		
		TextF(STR8("Textures Loaded: %d"),       arrlenu(DeshAssets->texture_array));
		TextF(STR8("Memory Occupied: %lld %cB"), texture_bytes / bytesDivisor(texture_bytes), bytesUnit(texture_bytes));
		
		EndRow();
		
		if(BeginCombo(STR8("AssetsBrowserUI_Texture_Selection_Combo"), (selected ? str8_from_cstr(selected->name) : STR8("select texture")))){
			for_stb_array(DeshAssets->texture_array){
				if(Selectable(str8_from_cstr((*it)->name), (*it) == selected)){
					selected = (*it);
					new_selected = 1;
				}
			}
			EndCombo();
		}
		
		Separator(9);
		
		if(BeginHeader(STR8("Stats"))){
			BeginRow(STR8("AssetsBrowserUI_Row2"), 3, 0, UIRowFlags_AutoSize);
			RowSetupColumnAlignments({ {1, 0.5}, {0, 0.5}, {0.5, 0.5} });
			
			TextF(STR8("Largest Texture: %s"), largest->name);
			if(Button(STR8("select"))){ selected = largest; new_selected = 1;}
			
			TextF(STR8("Smallest Texture: %s"), smallest->name);
			if(Button(STR8("select"))){ selected = smallest; new_selected = 1; }
			
			EndRow();
			
			EndHeader();
		}
		
		Separator(9);
		
		if(selected){
			BeginRow(STR8("AssetsBrowserUI_Texture_Selected"), 2, 0, UIRowFlags_AutoSize);
			RowSetupColumnAlignments({ {0, 0.5}, {0, 0.5} });
			
			u32 texbytes = selected->width * selected->height * u8size;
			
			TextF(STR8("Name: %s"), selected->name);
			TextF(STR8("Render Index: %d"), selected->render_idx);
			TextF(STR8("Width: %d"), selected->width);
			TextF(STR8("Height: %d"), selected->height);
			TextF(STR8("Depth: %d"), selected->depth);
			TextF(STR8("MipMaps: %d"), selected->mipmaps);
			TextF(STR8("Format: %s"), ImageFormatStrings[selected->format - 1].str);
			TextF(STR8("Type: %s"), TextureTypeStrings[selected->type].str);
			TextF(STR8("Filter: %s"), TextureFilterStrings[selected->filter].str);
			TextF(STR8("UV Mode: %s"), TextureAddressModeStrings[selected->uvMode].str);
			TextF(STR8("Memory Used: %lld %cB"), texbytes / bytesDivisor(texbytes), bytesUnit(texbytes));
			
			EndRow();
			PushColor(UIStyleCol_WindowBg, 0x073030ff);
			
			SetNextWindowSize(Vec2(MAX_F32, MAX_F32));
			BeginChild(STR8("AssetsBrowserUI_Texture_ImageInspector"), vec2::ZERO, UIWindowFlags_NoInteract);
			persist f32  zoom = 300;
			persist vec2 mpl;
			persist vec2 imagepos;
			persist vec2 imageposlatch;
			persist UIImageFlags flags;
			
			vec2 mp = input_mouse_position();
			
			if(Button(STR8("Flip x"))) 
				ToggleFlag(flags, UIImageFlags_FlipX);
			SameLine();
			if(Button(STR8("Flip y"))) 
				ToggleFlag(flags, UIImageFlags_FlipY);
			
			if(new_selected){
				zoom = f32(GetWindow()->width) / selected->width ;
				//imagepos = Vec2(
				//				(GetWindow()->width - selected->width) / 2,
				//				(GetWindow()->height - selected->height) / 2
				//				);
				imagepos = vec2::ZERO;
			}
			
			string z = toStr(zoom);
			TextOld(str8{(u8*)z.str, (s64)z.count});
			
			if(IsWinHovered()){
				SetPreventInputs();
				
				if(DeshInput->scrollY){
					f32 val = 10 * DeshInput->scrollY;
					zoom += zoom / val;
					//TODO make it zoom to the mouse 
					vec2 imtomp = (mp - GetWindow()->position) - GetWindow()->dimensions / 2;
					//imagepos -= imtomp.normalized() * val * 4;
				}
				if(input_lmouse_pressed()){
					mpl = mp;
					imageposlatch = imagepos;
				}
				if(input_lmouse_down()){
					imagepos = imageposlatch - (mpl - mp);
				}
				
			}
			else SetAllowInputs();
			
			SetNextItemSize(Vec2(zoom * selected->width, zoom * selected->height));
			Image(selected, imagepos, 1, flags);
			
			EndChild();
			PopColor();
		}
		
		
		EndChild();
		ResetItemFlags(UIItemType_Header);
		
		EndTab();
	}
	if(BeginTab(STR8("Materials")))
	{
		SetNextWindowSize(Vec2(MAX_F32, MAX_F32));
		BeginChild(STR8("AssetsBrowserUI_Materials"), vec2::ZERO, UIWindowFlags_NoBorder);
		
		Separator(5);
		
		SetNextWindowSize(Vec2(MAX_F32, 200));
		BeginChild(STR8("AssetsBrowserUI_Materials_List"), vec2::ZERO, UIWindowFlags_NoInteract); {
			BeginRow(STR8("AssetsBrowserUI_Materials_List"), 2, 0, UIRowFlags_AutoSize);
			RowSetupColumnAlignments({ {1, 0.5}, {0, 0.5} });
			
			forI(arrlenu(DeshAssets->material_array)){
				string s = toStr(i, "  ");
				TextOld(str8{(u8*)s.str, (s64)s.count});
				TextOld(str8_from_cstr(DeshAssets->material_array[i]->name));
			}
			
			EndRow();
		}EndChild();
		
		Separator(5);
		
		
		EndChild();
		
		EndTab();
	}
	if(BeginTab(STR8("Models")))
	{
		SetNextWindowSize(Vec2(MAX_F32, MAX_F32));
		BeginChild(STR8("AssetsBrowserUIModels"), Vec2(MAX_F32, MAX_F32));
		TextOld(STR8("TODO"));
		EndChild();
		
		EndTab();
	}
	if(BeginTab(STR8("Fonts")))
	{
		SetNextWindowSize(Vec2(MAX_F32, MAX_F32));
		BeginChild(STR8("AssetsBrowserUIFonts"), Vec2(MAX_F32, MAX_F32));
		TextOld(STR8("TODO"));
		
		EndTab();
	}
	EndTabBar();
	
	End();
	PopColor(11);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @assets_mesh
Mesh*
assets_mesh_allocate(u32 indexCount, u32 vertexCount, u32 faceCount, u32 trianglesNeighborCount, u32 facesVertexCount, u32 facesOuterVertexCount, u32 facesNeighborTriangleCount, u32 facesNeighborFaceCount){DPZoneScoped;
	Assert(indexCount && vertexCount && faceCount);
	
	u32 triangleCount = indexCount/3;
	u32 bytes =                    1*sizeof(Mesh)
		+                vertexCount*sizeof(MeshVertex)
		+                 indexCount*sizeof(MeshIndex)
		+              triangleCount*sizeof(MeshTriangle)
		+                  faceCount*sizeof(MeshFace)
		+     trianglesNeighborCount*sizeof(u32) //triangle neighbors
		+     trianglesNeighborCount*sizeof(u8)  //triangle edges
		+              triangleCount*sizeof(u32) //face triangles
		+           facesVertexCount*sizeof(u32)
		+      facesOuterVertexCount*sizeof(u32)
		+ facesNeighborTriangleCount*sizeof(u32)
		+     facesNeighborFaceCount*sizeof(u32);
	
	Mesh* mesh = (Mesh*)memory_alloc(bytes);  char* cursor = (char*)mesh + (1*sizeof(Mesh));
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
	mesh->vertexArray   = (MeshVertex*)cursor;    cursor +=   vertexCount*sizeof(MeshVertex);
	mesh->indexArray    = (MeshIndex*)cursor;     cursor +=    indexCount*sizeof(MeshIndex);
	mesh->triangleArray = (MeshTriangle*)cursor;  cursor += triangleCount*sizeof(MeshTriangle);
	mesh->faceArray     = (MeshFace*)cursor;      cursor +=     faceCount*sizeof(MeshFace);
	return mesh;
}


Mesh*
assets_mesh_create_box(f32 width, f32 height, f32 depth, u32 color){DPZoneScoped;
	//TODO(delle) change this to take in 8 points
	
	width  /= 2.f;
	height /= 2.f;
	depth  /= 2.f;
	
	//check if created already
	for_stb_array(DeshAssets->mesh_array){
		if((strcmp((*it)->name, "box_mesh") == 0) && vec3_equal((*it)->aabbMax, Vec3(width,height,depth))){
			return *it;
		}
	}
	
	Mesh* mesh = assets_mesh_allocate(36, 8, 6, 36, 24, 24, 24, 24);
	cpystr(mesh->name, "box_mesh", 64);
	mesh->aabbMin  = {-width,-height,-depth};
	mesh->aabbMax  = { width, height, depth};
	mesh->center   = {  0.0f,   0.0f,  0.0f};
	
	MeshVertex*   va = mesh->vertexArray;
	MeshIndex*    ia = mesh->indexArray;
	MeshTriangle* ta = mesh->triangleArray;
	MeshFace*     fa = mesh->faceArray;
	vec3 p{width, height, depth};
	vec2 uv{0.0f, 0.0f};
	f32 ir3 = 1.0f / M_SQRT_THREE; // inverse root 3 (component of point on unit circle)
	
	//vertex array {pos, uv, color, normal(from center)}
	va[0]={{-p.x, p.y, p.z}, uv, color, {-ir3, ir3, ir3}}; // -x, y, z  0
	va[1]={{-p.x,-p.y, p.z}, uv, color, {-ir3,-ir3, ir3}}; // -x,-y, z  1
	va[2]={{-p.x, p.y,-p.z}, uv, color, {-ir3, ir3,-ir3}}; // -x, y,-z  2
	va[3]={{-p.x,-p.y,-p.z}, uv, color, {-ir3,-ir3,-ir3}}; // -x,-y,-z  3
	va[4]={{ p.x, p.y, p.z}, uv, color, { ir3, ir3, ir3}}; //  x, y, z  4
	va[5]={{ p.x,-p.y, p.z}, uv, color, { ir3,-ir3, ir3}}; //  x,-y, z  5
	va[6]={{ p.x, p.y,-p.z}, uv, color, { ir3, ir3,-ir3}}; //  x, y,-z  6
	va[7]={{ p.x,-p.y,-p.z}, uv, color, { ir3,-ir3,-ir3}}; //  x,-y,-z  7
	
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
	for(s32 i = 0; i < 12; ++i){
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
	for(s32 i = 1; i < 12; ++i){
		ta[i].neighborArray = ta[i-1].neighborArray+3;
		ta[i].edgeArray     = ta[i-1].edgeArray+3;
	}
	
	//triangle array neighbors array
	ta[ 0].neighborArray[0]= 1; ta[ 0].neighborArray[1]=11; ta[ 0].neighborArray[2]= 9;
	ta[ 1].neighborArray[0]= 0; ta[ 1].neighborArray[1]= 3; ta[ 1].neighborArray[2]= 5;
	ta[ 2].neighborArray[0]= 3; ta[ 2].neighborArray[1]= 9; ta[ 2].neighborArray[2]= 7;
	ta[ 3].neighborArray[0]= 2; ta[ 3].neighborArray[1]= 1; ta[ 3].neighborArray[2]= 4;
	ta[ 4].neighborArray[0]= 5; ta[ 4].neighborArray[1]= 3; ta[ 4].neighborArray[2]= 6;
	ta[ 5].neighborArray[0]= 4; ta[ 5].neighborArray[1]= 1; ta[ 5].neighborArray[2]=10;
	ta[ 6].neighborArray[0]= 7; ta[ 6].neighborArray[1]= 4; ta[ 6].neighborArray[2]=10;
	ta[ 7].neighborArray[0]= 6; ta[ 7].neighborArray[1]= 2; ta[ 7].neighborArray[2]= 8;
	ta[ 8].neighborArray[0]= 9; ta[ 8].neighborArray[1]=11; ta[ 8].neighborArray[2]= 7;
	ta[ 9].neighborArray[0]= 8; ta[ 9].neighborArray[1]= 0; ta[ 9].neighborArray[2]= 2;
	ta[10].neighborArray[0]=11; ta[10].neighborArray[1]= 5; ta[10].neighborArray[2]= 6;
	ta[11].neighborArray[0]=10; ta[11].neighborArray[1]= 0; ta[11].neighborArray[2]= 8;
	
	//triangle array edges array
	ta[ 0].edgeArray[0]=0; ta[ 0].edgeArray[1]=2; ta[ 0].edgeArray[2]=1;
	ta[ 1].edgeArray[0]=2; ta[ 1].edgeArray[1]=1; ta[ 1].edgeArray[2]=0;
	ta[ 2].edgeArray[0]=0; ta[ 2].edgeArray[1]=2; ta[ 2].edgeArray[2]=1;
	ta[ 3].edgeArray[0]=2; ta[ 3].edgeArray[1]=0; ta[ 3].edgeArray[2]=1;
	ta[ 4].edgeArray[0]=0; ta[ 4].edgeArray[1]=2; ta[ 4].edgeArray[2]=1;
	ta[ 5].edgeArray[0]=2; ta[ 5].edgeArray[1]=0; ta[ 5].edgeArray[2]=1;
	ta[ 6].edgeArray[0]=0; ta[ 6].edgeArray[1]=1; ta[ 6].edgeArray[2]=2;
	ta[ 7].edgeArray[0]=2; ta[ 7].edgeArray[1]=1; ta[ 7].edgeArray[2]=0;
	ta[ 8].edgeArray[0]=0; ta[ 8].edgeArray[1]=2; ta[ 8].edgeArray[2]=1;
	ta[ 9].edgeArray[0]=2; ta[ 9].edgeArray[1]=0; ta[ 9].edgeArray[2]=1;
	ta[10].edgeArray[0]=0; ta[10].edgeArray[1]=2; ta[10].edgeArray[2]=1;
	ta[11].edgeArray[0]=2; ta[11].edgeArray[1]=0; ta[11].edgeArray[2]=1;
	
	//face array  0=up, 1=back, 2=right, 3=down, 4=left, 5=forward
	for(s32 i = 0; i < 6; ++i){
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
	for(s32 i = 1; i < 6; ++i){
		fa[i].triangleArray = fa[i-1].triangleArray+2;
	}
	
	//face array triangle arrays
	for(s32 i = 0; i < 6; ++i){
		fa[i].triangleArray[0]= i*2;
		fa[i].triangleArray[1]=(i*2)+1;
	}
	
	//face array vertex array offsets
	fa[0].vertexArray      = (u32*)(fa[0].triangleArray+12);
	fa[0].outerVertexArray = (u32*)(fa[0].vertexArray+24);
	for(s32 i = 1; i < 6; ++i){
		fa[i].vertexArray      = fa[i-1].vertexArray+4;
		fa[i].outerVertexArray = fa[i-1].outerVertexArray+4;
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
	for(s32 i = 1; i < 6; ++i){
		fa[i].neighborTriangleArray = fa[i-1].neighborTriangleArray+4;
		fa[i].neighborFaceArray     = fa[i-1].neighborFaceArray+4;
	}
	
	//face array neighbor triangle array
	fa[0].neighborTriangleArray[0]= 9; fa[0].neighborTriangleArray[1]= 3; fa[0].neighborTriangleArray[2]= 5; fa[0].neighborTriangleArray[3]=11;
	fa[1].neighborTriangleArray[0]= 1; fa[1].neighborTriangleArray[1]= 4; fa[1].neighborTriangleArray[2]= 7; fa[1].neighborTriangleArray[3]= 9;
	fa[2].neighborTriangleArray[0]= 1; fa[2].neighborTriangleArray[1]=10; fa[2].neighborTriangleArray[2]= 6; fa[2].neighborTriangleArray[3]= 3;
	fa[3].neighborTriangleArray[0]= 4; fa[3].neighborTriangleArray[1]=10; fa[3].neighborTriangleArray[2]= 8; fa[3].neighborTriangleArray[3]= 2;
	fa[4].neighborTriangleArray[0]= 0; fa[4].neighborTriangleArray[1]= 2; fa[4].neighborTriangleArray[2]= 7; fa[4].neighborTriangleArray[3]=11;
	fa[5].neighborTriangleArray[0]= 0; fa[5].neighborTriangleArray[1]= 8; fa[5].neighborTriangleArray[2]= 6; fa[5].neighborTriangleArray[3]= 5;
	
	//face array neighbor face array
	fa[0].neighborFaceArray[0]=1; fa[0].neighborFaceArray[1]=2; fa[0].neighborFaceArray[2]=4; fa[0].neighborFaceArray[3]=5;
	fa[1].neighborFaceArray[0]=0; fa[1].neighborFaceArray[1]=2; fa[1].neighborFaceArray[2]=3; fa[1].neighborFaceArray[3]=4;
	fa[2].neighborFaceArray[0]=0; fa[2].neighborFaceArray[1]=1; fa[2].neighborFaceArray[2]=3; fa[2].neighborFaceArray[3]=5;
	fa[3].neighborFaceArray[0]=1; fa[3].neighborFaceArray[1]=2; fa[3].neighborFaceArray[2]=4; fa[3].neighborFaceArray[3]=5;
	fa[4].neighborFaceArray[0]=0; fa[4].neighborFaceArray[1]=1; fa[4].neighborFaceArray[2]=3; fa[4].neighborFaceArray[3]=5;
	fa[5].neighborFaceArray[0]=0; fa[5].neighborFaceArray[1]=2; fa[5].neighborFaceArray[2]=3; fa[5].neighborFaceArray[3]=4;
	
	render_load_mesh(mesh);
	arrput(DeshAssets->mesh_array, mesh);
	return mesh;
}


Mesh*
assets_mesh_create_from_file(str8 name){DPZoneScoped;
	if(str8_equal_lazy(name, STR8("null"))) return assets_mesh_null();
	
	//prepend the meshes (models) folder
	str8_builder builder;
	str8_builder_init(&builder, STR8("data/models/"), deshi_temp_allocator);
	str8_builder_append(&builder, name);
	
	//append extension if not provided
	str8 front = str8_eat_until_last(name, '.');
	if(front.count == name.count) str8_builder_append(&builder, STR8(".mesh"));
	
	return assets_mesh_create_from_path(str8_builder_peek(&builder));
}


Mesh*
assets_mesh_create_from_path(str8 path){DPZoneScoped;
	str8 contents = file_read_simple(path, deshi_temp_allocator);
	if(!contents) return assets_mesh_null();
	
	return assets_mesh_create_from_memory(contents.str);
}


Mesh*
assets_mesh_create_from_memory(void* data){DPZoneScoped;
	u32 bytes = *((u32*)data);
	if(bytes < sizeof(Mesh)){
		LogE("assets","Mesh size was too small when trying to load it from memory");
		return assets_mesh_null();
	}
	
	//allocate and copy from data
	Mesh* mesh = (Mesh*)memory_alloc(bytes);
	CopyMemory(mesh, data, bytes);
	
	//check if mesh is already loaded
	for_stb_array(DeshAssets->mesh_array){
		if(strcmp((*it)->name, mesh->name) == 0){
			memory_zfree(mesh);
			return *it;
		}
	}
	
	//setup arrays
	u8* cursor  = (u8*)mesh + (1*sizeof(Mesh));
	mesh->vertexArray   = (MeshVertex*)cursor;    cursor +=   mesh->vertexCount*sizeof(MeshVertex);
	mesh->indexArray    = (MeshIndex*)cursor;     cursor +=    mesh->indexCount*sizeof(MeshIndex);
	mesh->triangleArray = (MeshTriangle*)cursor;  cursor += mesh->triangleCount*sizeof(MeshTriangle);
	mesh->faceArray     = (MeshFace*)cursor;      cursor +=     mesh->faceCount*sizeof(MeshFace);
	mesh->triangleArray[0].neighborArray = (u32*)(mesh->faceArray + mesh->faceCount);
	mesh->triangleArray[0].edgeArray     = (u8*) (mesh->triangleArray[0].neighborArray + mesh->totalTriNeighborCount);
	for(s32 ti = 1; ti < mesh->triangleCount; ++ti){
		mesh->triangleArray[ti].neighborArray = (u32*)(mesh->triangleArray[ti-1].neighborArray + mesh->triangleArray[ti-1].neighborCount);
		mesh->triangleArray[ti].edgeArray     = (u8*) (mesh->triangleArray[ti-1].edgeArray + mesh->triangleArray[ti-1].neighborCount);
	}
	mesh->faceArray[0].triangleArray         = (u32*)(mesh->triangleArray[0].edgeArray         + mesh->totalTriNeighborCount);
	mesh->faceArray[0].vertexArray           = (u32*)(mesh->faceArray[0].triangleArray         + mesh->triangleArray.count);
	mesh->faceArray[0].outerVertexArray      = (u32*)(mesh->faceArray[0].vertexArray           + mesh->totalFaceVertexCount);
	mesh->faceArray[0].neighborTriangleArray = (u32*)(mesh->faceArray[0].outerVertexArray      + mesh->totalFaceOuterVertexCount);
	mesh->faceArray[0].neighborFaceArray     = (u32*)(mesh->faceArray[0].neighborTriangleArray + mesh->totalFaceTriNeighborCount);
	for(s32 fi = 1; fi < mesh->faceCount; ++fi){
		mesh->faceArray[fi].triangleArray         = (u32*)(mesh->faceArray[fi-1].triangleArray         + mesh->faceArray[fi-1].triangleCount);
		mesh->faceArray[fi].vertexArray           = (u32*)(mesh->faceArray[fi-1].vertexArray           + mesh->faceArray[fi-1].vertexCount);
		mesh->faceArray[fi].outerVertexArray      = (u32*)(mesh->faceArray[fi-1].outerVertexArray      + mesh->faceArray[fi-1].outerVertexCount);
		mesh->faceArray[fi].neighborTriangleArray = (u32*)(mesh->faceArray[fi-1].neighborTriangleArray + mesh->faceArray[fi-1].neighborTriangleCount);
		mesh->faceArray[fi].neighborFaceArray     = (u32*)(mesh->faceArray[fi-1].neighborFaceArray     + mesh->faceArray[fi-1].neighborFaceCount);
	}
	
	render_load_mesh(mesh);
	arrput(DeshAssets->mesh_array, mesh);
	return mesh;
}


void
assets_mesh_save(Mesh* mesh){DPZoneScoped;
	assets_mesh_save_to_path(mesh, str8_concat3(STR8("data/models/"),str8_from_cstr(mesh->name),STR8(".mesh"), deshi_temp_allocator));
}


void
assets_mesh_save_to_path(Mesh* mesh, str8 path){DPZoneScoped;
	file_write_simple(path, mesh, mesh->bytes);
	Log("assets","Successfully saved mesh: ",path);
}


void
assets_mesh_delete(Mesh* mesh){DPZoneScoped;
	if(mesh == assets_mesh_null()) return;
	
	for_stb_array(DeshAssets->mesh_array) if(*it == mesh) arrdelswap(DeshAssets->mesh_array, it - DeshAssets->mesh_array);
	render_unload_mesh(mesh);
	memory_zfree(mesh);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @assets_texture
Texture*
assets_texture_create_from_file(str8 name, ImageFormat format, TextureType type, TextureFilter filter, TextureAddressMode uvMode, b32 keepLoaded, b32 generateMipmaps){DPZoneScoped;
	if(str8_equal_lazy(name, STR8("null"))) return assets_texture_null();
	
	//check if texture is already loaded
	for_stb_array(DeshAssets->texture_array){
		if(strncmp((*it)->name, (char*)name.str, 64) == 0){
			return *it;
		}
	}
	
	str8 path = str8_concat(STR8("data/textures/"),name, deshi_temp_allocator);
	Texture* texture = (Texture*)memory_alloc(sizeof(Texture));
	CopyMemory(texture->name, name.str, ClampMax(name.count,63));
	texture->format  = format;
	texture->type    = type;
	texture->filter  = filter;
	texture->uvMode  = uvMode;
	texture->pixels  = stbi_load((char*)path.str, &texture->width, &texture->height, &texture->depth, STBI_rgb_alpha);
	if(texture->pixels == 0){
		LogE("assets","Failed to create texture '",path,"': ",stbi_failure_reason()); 
		memory_zfree(texture);
		return assets_texture_null();
	}
	
	if(generateMipmaps){
		texture->mipmaps = (s32)log2(Max(texture->width, texture->height)) + 1;
	}else{
		texture->mipmaps = 1;
	}
	
	render_load_texture(texture);
	if(!keepLoaded){
		stbi_image_free(texture->pixels); 
		texture->pixels = 0;
	}
	
	arrput(DeshAssets->texture_array, texture);
	return texture;
}


Texture*
assets_texture_create_from_path(str8 path, ImageFormat format, TextureType type, TextureFilter filter, TextureAddressMode uvMode, b32 keepLoaded, b32 generateMipmaps){DPZoneScoped;
	//check if texture is already loaded
	str8 filename = str8_skip_until_last(path, '/'); str8_advance(&filename);
	for_stb_array(DeshAssets->texture_array){
		if(strncmp((*it)->name, (char*)filename.str, 64) == 0){
			return *it;
		}
	}
	
	Texture* texture = (Texture*)memory_alloc(sizeof(Texture));
	CopyMemory(texture->name, filename.str, ClampMax(filename.count,63));
	texture->format  = format; //TODO(delle) handle non RGBA formats properly
	texture->type    = type;
	texture->filter  = filter;
	texture->uvMode  = uvMode;
	texture->pixels  = stbi_load((char*)path.str, &texture->width, &texture->height, &texture->depth, STBI_rgb_alpha);
	if(texture->pixels == 0){
		LogE("assets","Failed to create texture '",path,"': ",stbi_failure_reason()); 
		memory_zfree(texture);
		return assets_texture_null();
	}
	
	if(generateMipmaps){
		texture->mipmaps = (s32)log2(Max(texture->width, texture->height)) + 1;
	}else{
		texture->mipmaps = 1;
	}
	
	render_load_texture(texture);
	if(!keepLoaded){
		stbi_image_free(texture->pixels); 
		texture->pixels = 0;
	}
	
	arrput(DeshAssets->texture_array, texture);
	return texture;
}


Texture*
assets_texture_create_from_memory(void* data, str8 name, u32 width, u32 height, ImageFormat format, TextureType type, TextureFilter filter, TextureAddressMode uvMode, b32 generateMipmaps){DPZoneScoped;
	if(data == 0){
		LogE("assets","Failed to create texture '",name,"': No memory passed!");
		return assets_texture_null();
	}
	
	//check if texture is already loaded (with that name)
	for_stb_array(DeshAssets->texture_array){
		if(strncmp((*it)->name, (char*)name.str, 64) == 0){
			return *it;
		}
	}
	
	Texture* texture = (Texture*)memory_alloc(sizeof(Texture));
	CopyMemory(texture->name, name.str, ClampMax(name.count,63));
	texture->format  = format;
	texture->type    = type;
	texture->filter  = filter;
	texture->uvMode  = uvMode;
	texture->width   = width;
	texture->height  = height;
	texture->depth   = 4;
	
	if(generateMipmaps){
		texture->mipmaps = (s32)log2(Max(texture->width, texture->height)) + 1;
	}else{
		texture->mipmaps = 1;
	}
	
	//reinterpret image as RGBA32  //TODO(delle) handle non RGBA formats properly
	if(format != ImageFormat_RGBA){
		texture->pixels = (u8*)memory_alloc(width * height * 4);
		
		const u8* src = (const u8*)data;
		u32* dst = (u32*)texture->pixels;
		switch(format){
			case ImageFormat_BW:{
				for(s32 i = width*height; i > 0; i--){
					u8 value = *src++;
					*dst++ = PackColorU32(value, value, value, 0xFF);
				}
			}break;
			case ImageFormat_BWA:{
				for(s32 i = width*height; i > 0; i--){
					u8 value = *src++;
					u8 alpha = *src++;
					*dst++ = PackColorU32(value, value, value, alpha);
				}
			}break;
			case ImageFormat_RGB:{
				for(s32 i = width*height; i > 0; i--){
					u8 r = *src++;
					u8 g = *src++;
					u8 b = *src++;
					*dst++ = PackColorU32(r, g, b, 0xFF);
				}
			}break;
		}
	}else{
		texture->pixels = (u8*)data;
	}
	
	render_load_texture(texture);
	arrput(DeshAssets->texture_array, texture);
	return texture;
}


void
assets_texture_delete(Texture* texture){DPZoneScoped;
	if(texture == assets_texture_null()) return;
	
	for_stb_array(DeshAssets->texture_array){
		if(*it == texture){
			arrdelswap(DeshAssets->texture_array, it - DeshAssets->texture_array);
		}
	}
	render_unload_texture(texture);
	if(texture->pixels) memory_zfree(texture->pixels); //NOTE(delle) stbi_image_free() simply calls STBI_FREE()
	memory_zfree(texture);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @assets_material
Material*
assets_material_allocate(u32 textureCount){DPZoneScoped;
	Material* material = (Material*)memory_alloc(sizeof(Material));
	arrsetlen(material->textureArray, textureCount);
	return material;
}


Material*
assets_material_create(str8 name, Shader shader, MaterialFlags flags, Texture** textures, u32 texture_count){DPZoneScoped;
	//check if material is already loaded
	for_stb_array(DeshAssets->material_array){
		if(strncmp((*it)->name, (char*)name.str, 64) == 0){
			return *it;
		}
	}
	
	Material* material = assets_material_allocate(texture_count);
	CopyMemory(material->name, name.str, ClampMax(name.count,63));
	material->shader = shader;
	material->flags  = flags;
	forI(texture_count) material->textureArray[i] = textures[i];
	
	render_load_material(material);
	arrput(DeshAssets->material_array, material);
	return material;
}


Material*
assets_material_create_from_file(str8 name){DPZoneScoped;
	if(str8_equal_lazy(name, STR8("null"))) return assets_material_null();
	
	//prepend the materials (models) folder
	str8_builder builder;
	str8_builder_init(&builder, STR8("data/models/"), deshi_temp_allocator);
	str8_builder_append(&builder, name);
	
	//append extension if not provided
	str8 front = str8_eat_until_last(name, '.');
	if(front.count == name.count) str8_builder_append(&builder, STR8(".mat"));
	
	return assets_material_create_from_path(str8_builder_peek(&builder));
}


Material*
assets_material_create_from_path(str8 path){DPZoneScoped;
	//check if material is already loaded
	str8 filename = str8_skip_until_last(path, '/'); str8_advance(&filename);
	for_stb_array(DeshAssets->material_array){
		if(strncmp((*it)->name, (char*)filename.str, 64) == 0){
			return *it;
		}
	}
	str8 front = str8_eat_until(filename, '.');
	
	//load .mat file
	File* file = file_init(path, FileAccess_Read);
	if(!file) return assets_material_null();
	defer{ file_deinit(file); };
	
	//parse .mat file
	str8 mat_name{}; //NOTE(delle) unused b/c we use the filename for loaded name currently
	Shader mat_shader = 0;
	MaterialFlags mat_flags = 0;
	arrayT<str8> mat_textures(deshi_temp_allocator);
	enum{ HEADER_MATERIAL, HEADER_TEXTURES, HEADER_INVALID }header;
	
	u32 line_number = 0;
	while(file->cursor < file->bytes){
		line_number += 1;
		
		//next line
		str8 line = file_read_line_alloc(file, &assets_load_allocator);
		if(!line) continue;
		
		//skip leading whitespace
		str8_advance_while(&line, ' ');
		if(!line) continue;
		
		//early out if comment is first character
		DecodedCodepoint decoded = decoded_codepoint_from_utf8(line.str, 4);
		if(decoded.codepoint == '#') continue;
		
		//check for header
		if(decoded.codepoint == '>'){
			if     (str8_begins_with(line, STR8(">material"))) header = HEADER_MATERIAL;
			else if(str8_begins_with(line, STR8(">textures"))) header = HEADER_TEXTURES;
			else{ header = HEADER_INVALID; LogE("assets","Error parsing material '",path,"' on line ",line_number,". Invalid Header: ",line); };
			continue;
		}
		
		//early out invalid header
		if(header == HEADER_INVALID){
			LogE("assets","Error parsing material '",path,"' on line ",line_number,". Invalid Header; skipping line");
			continue;
		}
		
		if(header == HEADER_MATERIAL){
			//parse key
			str8 key = str8_eat_until(line, ' ');
			str8_increment(&line, key.count);
			
			//skip separating whitespace
			str8_advance_while(&line, ' ');
			if(!line){
				LogE("config","Error parsing material '",path,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			
			//early out if comment is first value character
			decoded = decoded_codepoint_from_utf8(line.str, 4);
			if(decoded.codepoint == '#'){
				LogE("assets","Error parsing material '",path,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			
			if      (str8_equal_lazy(key, STR8("name"))){
				if(decoded.codepoint != '\"'){
					LogE("assets","Error parsing material '",path,"' on line ",line_number,". Names must be wrapped in double quotes.");
					continue;
				}
				mat_name = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
			}else if(str8_equal_lazy(key, STR8("flags"))){
				mat_flags = (ModelFlags)atoi((char*)line.str);
			}else if(str8_equal_lazy(key, STR8("shader"))){
				forI(Shader_COUNT){
					if(str8_equal_lazy(line, ShaderStrings[i])){
						mat_shader = i;
						break;
					}
				}
			}else{
				LogE("assets","Error parsing material '",path,"' on line ",line_number,". Invalid key '",key,"' for >material header.");
				continue;
			}
		}else{
			if(decoded.codepoint != '\"'){
				LogE("assets","Error parsing material '",path,"' on line ",line_number,". Textures must be wrapped in double quotes.");
				continue;
			}
			
			mat_textures.add(str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator));
		}
	}
	
	Material* material = assets_material_allocate(mat_textures.count);
	CopyMemory(material->name, front.str, ClampMax(front.count, 63));
	material->shader = mat_shader;
	material->flags  = mat_flags;
	forI(mat_textures.count) material->textureArray[i] = assets_texture_create_from_file_simple(mat_textures[i]);
	
	render_load_material(material);
	arrput(DeshAssets->material_array, material);
	return material;
}


void
assets_material_save(Material* material){DPZoneScoped;
	assets_material_save_to_path(material, str8_concat3(STR8("data/models/"),str8_from_cstr(material->name),STR8(".mat"), deshi_temp_allocator));
}


void
assets_material_save_to_path(Material* material, str8 path){DPZoneScoped;
	str8_builder builder;
	str8_builder_init(&builder,
					  ToString8(deshi_temp_allocator,
								">material"
								"\nname   \"",material->name,"\""
								"\nshader ",ShaderStrings[material->shader],
								"\nflags  ",material->flags,
								"\n"
								"\n>textures"),
					  deshi_temp_allocator);
	if(material->textureArray){
		for_stb_array(material->textureArray){
			str8_builder_append(&builder, ToString8(deshi_temp_allocator, "\n\"",(*it)->name,"\""));
		}
	}
	str8_builder_append(&builder, STR8("\n"));
	str8 mat_text = str8_builder_peek(&builder);
	file_write_simple(path, mat_text.str, mat_text.count*sizeof(u8));
	Log("assets","Successfully saved material: ",path);
}


void
assets_material_delete(Material* material){DPZoneScoped;
	if(material == assets_material_null()) return;
	
	for_stb_array(DeshAssets->material_array){
		if(*it == material){
			arrdelswap(DeshAssets->material_array, it - DeshAssets->material_array);
		}
	}
	render_unload_material(material);
	arrfree(material->textureArray);
	memory_zfree(material);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @assets_model
#define ParseError(path,...) LogE("assets","Failed parsing '",path,"' on line '",line_number,"'! ",__VA_ARGS__)

Model*
assets_model_allocate(u32 batchCount){DPZoneScoped;
	Model* model = (Model*)memory_alloc(sizeof(Model));
	arrsetlen(model->batchArray, (batchCount) ? batchCount : 1);
	return model;
}


Model* assets_model_create_from_file(str8 filename, ModelFlags flags, b32 forceLoadOBJ){DPZoneScoped;
	if(str8_equal_lazy(filename, STR8("null"))) return assets_model_null();
	
	//prepend the models folder
	str8 directory = STR8("data/models/");
	str8_builder builder;
	str8_builder_init(&builder, directory, deshi_temp_allocator);
	str8_builder_append(&builder, filename);
	
	//append extension if not provided
	str8 front = str8_eat_until_last(filename, '.');
	if(front.count == filename.count) str8_builder_append(&builder, STR8(".model"));
	
	//check if model is already loaded
	for_stb_array(DeshAssets->model_array){
		if(strncmp((*it)->name, (char*)front.str, 64) == 0){
			return *it;
		}
	}
	
	//check which files need to be parsed
	str8 model_path = str8_builder_peek(&builder);
	str8 obj_path  = str8_concat3(directory, front, STR8(".obj"),  deshi_temp_allocator);
	str8 mesh_path = str8_concat3(directory, front, STR8(".mesh"), deshi_temp_allocator);
	b32 parse_obj_mesh  = true;
	b32 parse_obj_model = true;
	if(!forceLoadOBJ){
		if(file_exists(mesh_path))  parse_obj_mesh  = false;
		if(file_exists(model_path)) parse_obj_model = false;
	}
	
	//// load .obj and .mtl ////
	if(parse_obj_model && parse_obj_mesh){
		return assets_model_create_from_obj(obj_path, flags);
	}
	//// load .obj (batch info only), .mtl, and .mesh ////
	else if(parse_obj_model){
		return assets_model_create_from_mesh_obj(assets_mesh_create_from_path(mesh_path), obj_path, flags);
	}
	
	//// load .model and .mesh ////
	Stopwatch load_stopwatch = start_stopwatch();
	str8 model_name;
	str8 model_mesh;
	ModelFlags model_flags;
	arrayT<pair<str8,u32,u32>> model_batches(deshi_temp_allocator);
	enum{ HEADER_MODEL, HEADER_BATCHES, HEADER_INVALID } header;
	
	File* file = file_init(model_path, FileAccess_Read);
	if(!file) return assets_model_null();
	defer{ file_deinit(file); };
	
	u32 line_number = 0;
	while(file->cursor < file->bytes){
		line_number += 1;
		
		//next line
		str8 line = file_read_line_alloc(file, &assets_load_allocator);
		if(!line) continue;
		
		//skip leading whitespace
		str8_advance_while(&line, ' ');
		if(!line) continue;
		
		//early out if comment is first character
		DecodedCodepoint decoded = decoded_codepoint_from_utf8(line.str, 4);
		if(decoded.codepoint == '#') continue;
		
		//check for headers
		if(decoded.codepoint == '>'){
			if     (str8_begins_with(line, STR8(">model"))) header = HEADER_MODEL;
			else if(str8_begins_with(line, STR8(">batches"))) header = HEADER_BATCHES;
			else{ header = HEADER_INVALID; LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". Invalid Header: ",line); };
			continue;
		}
		
		//early out invalid header
		if(header == HEADER_INVALID){
			LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". Invalid Header; skipping line");
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
				LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			
			if      (str8_equal_lazy(key, STR8("name"))){
				if(decoded.codepoint != '\"'){
					LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". Names must be wrapped in double quotes.");
					continue;
				}
				model_name = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
			}else if(str8_equal_lazy(key, STR8("flags"))){
				model_flags = (ModelFlags)atoi((char*)line.str);
			}else if(str8_equal_lazy(key, STR8("mesh"))){
				if(decoded.codepoint != '\"'){
					LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". Filenames must be wrapped in double quotes.");
					continue;
				}
				model_mesh = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
			}else if(str8_equal_lazy(key, STR8("armature"))){
				//NOTE currently nothing
			}else{
				LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". Invalid key '",key,"' for >model header.");
				continue;
			}
		}else{
			if(decoded.codepoint != '\"'){
				LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". Names must be wrapped in double quotes. Batch format: '\"material_name\" index_offset index_count'");
				continue;
			}
			
			str8 batch_mat = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
			str8_increment(&line, batch_mat.count+2);
			if(!line){
				LogE("assets","Error parsing model '",model_path,"' on line ",line_number,". No indexes passed. Batch format: '\"material_name\" index_offset index_count'");
				continue;
			}
			
			char* next = (char*)line.str;
			u32 ioffset = strtol(next,&next,10);
			u32 icount  = strtol(next,&next,10);
			
			model_batches.add({batch_mat, ioffset, icount});
		}
	}
	Log("assets","Successfully loaded model ",model_path);
	
	Model* model = assets_model_allocate(model_batches.count);
	cpystr(model->name, (char*)model_name.str, 64);
	model->flags    = model_flags;
	model->mesh     = assets_mesh_create_from_file(model_mesh);
	model->armature = 0;
	forI(model_batches.count){
		model->batchArray[i] = ModelBatch{
			model_batches[i].second,
			model_batches[i].third,
			assets_material_create_from_file(model_batches[i].first)
		};
	}
	
	arrput(DeshAssets->model_array, model);
	Log("assets","Finished loading model '",filename,"' in ",peek_stopwatch(load_stopwatch),"ms");
	return model;
}


Model*
assets_model_create_from_obj(str8 obj_path, ModelFlags flags){DPZoneScoped;
	Stopwatch load_stopwatch = start_stopwatch();
	map<vec3,MeshVertex> vUnique(deshi_temp_allocator);
	set<vec3> vnUnique(deshi_temp_allocator);
	set<pair<u32,str8>> oUnique(deshi_temp_allocator); //index offset, name
	set<pair<u32,str8>> gUnique(deshi_temp_allocator);
	set<pair<u32,str8>> uUnique(deshi_temp_allocator);
	set<pair<u32,str8>> mUnique(deshi_temp_allocator);
	set<pair<u32,vec3>> appliedUniqueNormals(deshi_temp_allocator); //vertex applied on, normal
	arrayT<vec2> vtArray(deshi_temp_allocator); //NOTE UV vertices arent expected to be unique
	arrayT<u32> vArray(deshi_temp_allocator); //index in unique array
	arrayT<u32> vnArray(deshi_temp_allocator);
	arrayT<u32> oArray(deshi_temp_allocator);
	arrayT<u32> gArray(deshi_temp_allocator);
	arrayT<u32> uArray(deshi_temp_allocator);
	arrayT<u32> mArray(deshi_temp_allocator);
	arrayT<MeshIndex>    indexes(deshi_temp_allocator);
	arrayT<MeshTriangle> triangles(deshi_temp_allocator);
	arrayT<MeshFace>     faces(deshi_temp_allocator);
	arrayT<arrayT<pair<u32,u8>>> triNeighbors(deshi_temp_allocator);
	arrayT<arrayT<u32>> faceTriangles(deshi_temp_allocator);
	arrayT<set<u32>>   faceVertexes(deshi_temp_allocator);
	arrayT<arrayT<u32>> faceOuterVertexes(deshi_temp_allocator);
	arrayT<arrayT<u32>> faceTriNeighbors(deshi_temp_allocator);
	arrayT<arrayT<u32>> faceFaceNeighbors(deshi_temp_allocator);
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
	
	File* file = file_init(obj_path, FileAccess_Read);
	if(!file) return assets_model_null();
	defer{ file_deinit(file); };
	
	u32 line_number = 0;
	while(file->cursor < file->bytes){
		line_number += 1;
		
		//next line
		str8 line = file_read_line_alloc(file, &assets_load_allocator);
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
						vArray.add(vUnique.add(vec, MeshVertex{vec}));
					}continue;
					
					//// uv ////
					case 't':{
						str8_increment(&line, decoded.advance);
						decoded = decoded_codepoint_from_utf8(line.str, 4);
						if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'vt'"); return assets_model_null(); }
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
						if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'vn'"); return assets_model_null(); }
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
					}return assets_model_null();
				}
			}continue;
			
			//// face ////
			case 'f':{
				Stopwatch face_stopwatch = start_stopwatch();
				
				str8_increment(&line, decoded.advance);
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'f'"); return assets_model_null(); }
				if(vArray.count == 0){ ParseError(obj_path,"Specifier 'f' before any 'v'"); return assets_model_null(); }
				
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
				MeshTriangle triangle{};
				triangle.p[0] = vUnique.data[vArray[v0]].pos;
				triangle.p[1] = vUnique.data[vArray[v1]].pos;
				triangle.p[2] = vUnique.data[vArray[v2]].pos;
				triangle.v[0] = vArray[v0];
				triangle.v[1] = vArray[v1];
				triangle.v[2] = vArray[v2];
				triangle.face = (u32)-1;
				triangle.normal = (triangle.p[0] - triangle.p[1]).cross(triangle.p[0] - triangle.p[2]).normalized();
				triangles.add(triangle);
				triNeighbors.add(arrayT<pair<u32,u8>>(deshi_temp_allocator));
				
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
					Log("assets",obj_path," face ",faces.count," on line ",line_number," finished creation in ",peek_stopwatch(face_stopwatch),"ms");
				}
			}continue;
			
			//// use material ////
			case 'u':{
				if(strncmp((const char*)line.str, "usemtl ", 7) != 0){ ParseError(obj_path,"Specifier started with 'u' but didn't equal 'usemtl '"); return assets_model_null(); }
				
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
				if(strncmp((const char*)line.str, "mtllib ", 7) != 0){ ParseError(obj_path,"Specifier started with 'm' but didn't equal 'mtllib '"); return assets_model_null(); }
				
				mtllib_found = true;
				str8_increment(&line, 7);
				pair<u32,str8> mtllib(indexes.count, str8_copy(line, deshi_temp_allocator));
				mArray.add(mUnique.add(mtllib,mtllib));
			}continue;
			
			//// group (batch) ////
			case 'g':{
				str8_increment(&line, decoded.advance);
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'g'"); return assets_model_null(); }
				str8_increment(&line, decoded.advance);
				
				pair<u32,str8> group(indexes.count, str8_copy(line, deshi_temp_allocator));
				gArray.add(gUnique.add(group,group));
			}continue;
			
			//// object ////
			case 'o':{
				str8_increment(&line, decoded.advance);
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'o'"); return assets_model_null(); }
				str8_increment(&line, decoded.advance);
				
				pair<u32,str8> object(indexes.count, str8_copy(line, deshi_temp_allocator));
				oArray.add(oUnique.add(object,object));
			}continue;
			
			//// smoothing ////
			case 's':{
				str8_increment(&line, decoded.advance);
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 's'"); return assets_model_null(); }
				str8_increment(&line, decoded.advance);
				
				s_warning = true;
			}continue;
			default:{
				ParseError(obj_path,"Invalid starting character: '",(char)decoded.codepoint,"'");
			}return assets_model_null();
		}
	}
	
	//// generate mesh faces ////
	forX(bti, triangles.count){
		if(triangles[bti].face != -1) continue;
		
		//create face and add base triange to it
		u32 cfi = faces.count;
		faces.add(MeshFace{});
		faceTriangles.add(arrayT<u32>(deshi_temp_allocator));
		faceVertexes.add(set<u32>(deshi_temp_allocator));
		faceOuterVertexes.add(arrayT<u32>(deshi_temp_allocator));
		faceTriNeighbors.add(arrayT<u32>(deshi_temp_allocator));
		faceFaceNeighbors.add(arrayT<u32>(deshi_temp_allocator));
		faces[cfi].normal = triangles[bti].normal;
		triangles[bti].face = cfi;
		faceTriangles[cfi].add(bti);
		faceVertexes[cfi].add(triangles[bti].v[0],triangles[bti].v[0]);
		faceVertexes[cfi].add(triangles[bti].v[1],triangles[bti].v[1]);
		faceVertexes[cfi].add(triangles[bti].v[2],triangles[bti].v[2]);
		totalFaceVertexes += 3;
		
	arrayT<u32> check_tris({(u32)bti});
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
	if(non_tri_warning)   LogW("assets","The mesh was not triangulated before parsing; Expect missing triangles!");
	if(s_warning)         LogW("assets","There were 's' specifiers when parsing ",obj_path,", but those are not evaluated currently");
	if(!vtArray.count){   LogW("assets","No vertex UVs 'vt' were parsed in ",obj_path); }
	if(!vnArray.count){   LogW("assets","No vertex normals 'vn' were parsed in ",obj_path); }
	if(fatal_error){      LogE("assets","OBJ parsing encountered a fatal error in ",obj_path); return assets_model_null(); }
	if(!vArray.count){    LogE("assets","No vertex positions 'v' were parsed in ",obj_path); return assets_model_null(); }
	if(!triangles.count){ LogE("assets","No faces 'f' were parsed in ",obj_path); return assets_model_null(); }
	
	//// check if mesh is already loaded ////
	Mesh* mesh = 0;
	for_stb_array(DeshAssets->mesh_array){
		if(strncmp((*it)->name, (char*)file->front.str, 64) == 0){
			mesh = *it;
			break;
		}
	}
	
	//// create mesh ////
	if(mesh == 0){
		mesh = assets_mesh_allocate(indexes.count, vUnique.count, faces.count, totalTriNeighbors, 
									totalFaceVertexes, totalFaceOuterVertexes, totalFaceTriNeighbors, totalFaceFaceNeighbors);
		//fill base arrays
		cpystr(mesh->name, (char*)file->front.str, 64);
		mesh->aabbMin  = aabb_min;
		mesh->aabbMax  = aabb_max;
		mesh->center   = {(aabb_max.x+aabb_min.x)/2.0f, (aabb_max.y+aabb_min.y)/2.0f, (aabb_max.z+aabb_min.z)/2.0f};
		memcpy(mesh->vertexArray,   vUnique.data.data, vUnique.count*sizeof(MeshVertex));
		memcpy(mesh->indexArray,    indexes.data,      indexes.count*sizeof(MeshIndex));
		memcpy(mesh->triangleArray, triangles.data,    triangles.count*sizeof(MeshTriangle));
		memcpy(mesh->faceArray,     faces.data,        faces.count*sizeof(MeshFace));
		
		//setup pointers
		mesh->triangleArray[0].neighborArray = (u32*)(mesh->faceArray + mesh->faceCount);
		mesh->triangleArray[0].edgeArray     = (u8*)(mesh->triangleArray[0].neighborArray + totalTriNeighbors);
		for(s32 ti = 1; ti < mesh->triangleCount; ++ti){
			mesh->triangleArray[ti].neighborArray = (u32*)(mesh->triangleArray[ti-1].neighborArray + triNeighbors[ti-1].count);
			mesh->triangleArray[ti].edgeArray     =  (u8*)(mesh->triangleArray[ti-1].edgeArray     + triNeighbors[ti-1].count);
		}
		mesh->faceArray[0].triangleArray         = (u32*)(mesh->triangleArray[0].edgeArray     + totalTriNeighbors);
		mesh->faceArray[0].vertexArray           = (u32*)(mesh->faceArray[0].triangleArray         + triangles.count);
		mesh->faceArray[0].outerVertexArray      = (u32*)(mesh->faceArray[0].vertexArray           + totalFaceVertexes);
		mesh->faceArray[0].neighborTriangleArray = (u32*)(mesh->faceArray[0].outerVertexArray      + totalFaceOuterVertexes);
		mesh->faceArray[0].neighborFaceArray     = (u32*)(mesh->faceArray[0].neighborTriangleArray + totalFaceTriNeighbors);
		for(s32 fi = 1; fi < mesh->faceCount; ++fi){
			mesh->faceArray[fi].triangleArray         = (u32*)(mesh->faceArray[fi-1].triangleArray         + faceTriangles[fi-1].count);
			mesh->faceArray[fi].vertexArray           = (u32*)(mesh->faceArray[fi-1].vertexArray           + faceVertexes[fi-1].count);
			mesh->faceArray[fi].outerVertexArray      = (u32*)(mesh->faceArray[fi-1].outerVertexArray      + faceOuterVertexes[fi-1].count);
			mesh->faceArray[fi].neighborTriangleArray = (u32*)(mesh->faceArray[fi-1].neighborTriangleArray + faceTriNeighbors[fi-1].count);
			mesh->faceArray[fi].neighborFaceArray     = (u32*)(mesh->faceArray[fi-1].neighborFaceArray     + faceFaceNeighbors[fi-1].count);
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
		forX(fi, mesh->faceCount){
			mesh->faceArray[fi].triangleCount = faceTriangles[fi].count;
			mesh->faceArray[fi].vertexCount   = faceVertexes[fi].count;
			mesh->faceArray[fi].outerVertexCount = faceOuterVertexes[fi].count;
			mesh->faceArray[fi].neighborTriangleCount = faceTriNeighbors[fi].count;
			mesh->faceArray[fi].neighborFaceCount = faceFaceNeighbors[fi].count;
			mesh->faceArray[fi].center = faces[fi].center / (f32)faceOuterVertexes[fi].count;
			forX(fti, mesh->faceArray[fi].triangleCount){
				mesh->faceArray[fi].triangleArray[fti] = faceTriangles[fi][fti];
			}
			forX(fvi, mesh->faceArray[fi].vertexCount){
				mesh->faceArray[fi].vertexArray[fvi] = faceVertexes[fi].data[fvi];
			}
			forX(fvi, mesh->faceArray[fi].outerVertexCount){
				mesh->faceArray[fi].outerVertexArray[fvi] = faceOuterVertexes[fi][fvi];
			}
			forX(fvi, mesh->faceArray[fi].neighborTriangleCount){
				mesh->faceArray[fi].neighborTriangleArray[fvi] = faceTriNeighbors[fi][fvi];
			}
			forX(fvi, mesh->faceArray[fi].neighborFaceCount){
				mesh->faceArray[fi].neighborFaceArray[fvi] = faceFaceNeighbors[fi][fvi];
			}
		}
		
		render_load_mesh(mesh);
		arrput(DeshAssets->mesh_array, mesh);
	}
	Log("assets","Parsing and loading OBJ '",obj_path,"' took ",peek_stopwatch(load_stopwatch),"ms");
	
	//parse MTL files
	if(mtllib_found){
		load_stopwatch = start_stopwatch();
		
		//!Incomplete
		
		Log("assets","Parsing and loading MTLs for OBJ '",obj_path,"' took ",peek_stopwatch(load_stopwatch),"ms");
	}
	
	Model* model = assets_model_allocate(mArray.count);
	cpystr(model->name, (char*)file->front.str, 64);
	model->flags    = flags;
	model->mesh     = mesh;
	model->armature = 0;
	
	//!Incomplete batch materials
	if(mArray.count > 1){
		model->batchArray[mArray.count-1].indexOffset = mUnique.data[mArray[mArray.count-1]].first;
		model->batchArray[mArray.count-1].indexCount  = indexes.count - model->batchArray[mArray.count-1].indexOffset;
		model->batchArray[mArray.count-1].material    = assets_material_null();
		for(u32 bi = mArray.count-2; bi >= 0; --bi){
			model->batchArray[bi].indexOffset = mUnique.data[mArray[bi]].first;
			model->batchArray[bi].indexCount  = model->batchArray[bi+1].indexOffset - model->batchArray[bi].indexOffset;
			model->batchArray[bi].material    = assets_material_null();
		}
	}else{
		model->batchArray[0].indexOffset = 0;
		model->batchArray[0].indexCount  = indexes.count;
		model->batchArray[0].material    = assets_material_null();
	}
	
	arrput(DeshAssets->model_array, model);
	Log("assets","Finished loading model '",obj_path,"' in ",peek_stopwatch(load_stopwatch),"ms");
	return model;
}


Model*
assets_model_create_from_mesh(Mesh* mesh, ModelFlags flags){DPZoneScoped;
	Stopwatch load_stopwatch = start_stopwatch();
	
	//check if created already
	str8 model_name = str8_from_cstr(mesh->name);
	for_stb_array(DeshAssets->model_array){
		if(   (*it)->mesh == mesh
		   && strncmp((*it)->name, (char*)model_name.str, 64) == 0
		   && (*it)->flags == flags
		   && (*it)->batchArray != 0
		   && arrlenu((*it)->batchArray) == 1
		   && (*it)->batchArray[0].indexOffset == 0
		   && (*it)->batchArray[0].indexCount == mesh->indexCount
		   && (*it)->batchArray[0].material == assets_material_null())
		{
			return *it;
		}
	}
	
	Model* model = assets_model_allocate(1);
	cpystr(model->name, (char*)model_name.str, 64);
	model->mesh     = mesh;
	model->armature = 0;
	model->batchArray[0] = ModelBatch{0, mesh->indexCount, assets_material_null()};
	
	arrput(DeshAssets->model_array, model);
	Log("assets","Finished loading model '",model_name,"' in ",peek_stopwatch(load_stopwatch),"ms");
	return model;
}


Model*
assets_model_create_from_mesh_obj(Mesh* mesh, str8 obj_path, ModelFlags flags){DPZoneScoped;
	Stopwatch load_stopwatch = start_stopwatch();
	set<pair<u32,str8>> oUnique(deshi_temp_allocator); //index offset, name
	set<pair<u32,str8>> gUnique(deshi_temp_allocator);
	set<pair<u32,str8>> uUnique(deshi_temp_allocator);
	set<pair<u32,str8>> mUnique(deshi_temp_allocator);
	arrayT<u32> oArray(deshi_temp_allocator); //index in unique array
	arrayT<u32> gArray(deshi_temp_allocator);
	arrayT<u32> uArray(deshi_temp_allocator);
	arrayT<u32> mArray(deshi_temp_allocator);
	b32 mtllib_found = false;
	u32 index_count = 0;
	
	File* file = file_init(obj_path, FileAccess_Read);
	if(!file) return assets_model_null();
	defer{ file_deinit(file); };
	
	u32 line_number = 0;
	while(file->cursor < file->bytes){
		line_number += 1;
		
		//next line
		str8 line = file_read_line_alloc(file, &assets_load_allocator);
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
				
				if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'f'"); return assets_model_null(); }
				index_count += 3;
			}
			
			//// use material ////
			case 'u':{ //use material
				if(strncmp((const char*)line.str, "usemtl ", 7) != 0){ ParseError(obj_path,"Specifier started with 'u' but didn't equal 'usemtl '"); return assets_model_null(); }
				
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
				if(strncmp((const char*)line.str, "mtllib ", 7) != 0){ ParseError(obj_path,"Specifier started with 'm' but didn't equal 'mtllib '"); return assets_model_null(); }
				
				mtllib_found = true;
				str8_increment(&line, 7);
				pair<u32,str8> mtllib(index_count, str8_copy(line, deshi_temp_allocator));
				mArray.add(mUnique.add(mtllib,mtllib));
			}continue;
			
			//// group (batch) ////
			case 'g':{
				str8_increment(&line, decoded.advance);
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'g'"); return assets_model_null(); }
				str8_increment(&line, decoded.advance);
				
				pair<u32,str8> group(index_count, str8_copy(line, deshi_temp_allocator));
				gArray.add(gUnique.add(group,group));
			}continue;
			
			//// object ////
			case 'o':{
				str8_increment(&line, decoded.advance);
				decoded = decoded_codepoint_from_utf8(line.str, 4);
				if(decoded.codepoint != ' '){ ParseError(obj_path,"No space after 'o'"); return assets_model_null(); }
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
		
		Log("assets","Parsing and loading MTLs for OBJ '",obj_path,"' took ",peek_stopwatch(load_stopwatch),"ms");
	}
	
	Model* model = assets_model_allocate(mArray.count);
	cpystr(model->name, (char*)file->front.str, 64);
	model->flags    = flags;
	model->mesh     = mesh;
	model->armature = 0;
	
	//!Incomplete batch materials
	if(mArray.count > 1){
		model->batchArray[mArray.count-1].indexOffset = mUnique.data[mArray[mArray.count-1]].first;
		model->batchArray[mArray.count-1].indexCount  = index_count - model->batchArray[mArray.count-1].indexOffset;
		model->batchArray[mArray.count-1].material    = assets_material_null();
		for(u32 bi = mArray.count-2; bi >= 0; --bi){
			model->batchArray[bi].indexOffset = mUnique.data[mArray[bi]].first;
			model->batchArray[bi].indexCount  = model->batchArray[bi+1].indexOffset - model->batchArray[bi].indexOffset;
			model->batchArray[bi].material    = assets_material_null();
		}
	}else{
		model->batchArray[0].indexOffset = 0;
		model->batchArray[0].indexCount  = index_count;
		model->batchArray[0].material    = assets_material_null();
	}
	
	arrput(DeshAssets->model_array, model);
	Log("assets","Finished loading model '",obj_path,"' in ",peek_stopwatch(load_stopwatch),"ms");
	return model;
}


Model* assets_model_copy(Model* base){DPZoneScoped;
	Model* model = assets_model_allocate(arrlenu(base->batchArray));
	cpystr(model->name, base->name, 64);
	model->flags    = base->flags;
	model->mesh     = base->mesh;
	model->armature = base->armature;
	forI(arrlenu(base->batchArray)){
		model->batchArray[i].indexOffset = base->batchArray[i].indexOffset;
		model->batchArray[i].indexCount  = base->batchArray[i].indexCount;
		model->batchArray[i].material    = base->batchArray[i].material;
	}
	
	arrput(DeshAssets->model_array, model);
	return model;
}


void
assets_model_save(Model* model){
	str8 directory = STR8("data/models/");
	
	if(model->mesh){
		assets_mesh_save(model->mesh);
	}
	
	str8 path = str8_concat3(directory,str8_from_cstr(model->name),STR8(".model"), deshi_temp_allocator);
	str8_builder builder;
	str8_builder_init(&builder,
					  ToString8(deshi_temp_allocator,
								">model"
								"\nname     \"",model->name,"\""
								"\nflags    ", model->flags,
								"\nmesh     \"", model->mesh->name,"\""
								"\narmature ", 0,
								"\n"
								"\n>batches"),
					  deshi_temp_allocator);
	if(model->batchArray){
		for_stb_array(model->batchArray){
			assets_material_save(it->material);
			str8_builder_append(&builder, ToString8(deshi_temp_allocator, "\n\"",it->material->name,"\" ",it->indexOffset," ",it->indexCount));
		}
	}
	str8_builder_append(&builder, STR8("\n"));
	str8 model_text = str8_builder_peek(&builder);
	file_write_simple(path, model_text.str, model_text.count*sizeof(u8));
	Log("assets","Successfully saved model: ",path);
}


void
assets_model_save_at_path(Model* model, str8 path){DPZoneScoped;
	str8 directory = str8_eat_until_last(path, '/');
	if(directory.str[directory.count] == '/') directory.count += 1;
	
	if(model->mesh){
		assets_mesh_save_to_path(model->mesh, str8_concat3(directory,str8_from_cstr(model->mesh->name),STR8(".mesh"), deshi_temp_allocator));
	}
	
	str8_builder builder;
	str8_builder_init(&builder,
					  ToString8(deshi_temp_allocator,
								">model"
								"\nname     \"",model->name,"\""
								"\nflags    ", model->flags,
								"\nmesh     \"", model->mesh->name,"\""
								"\narmature ", 0,
								"\n"
								"\n>batches"),
					  deshi_temp_allocator);
	if(model->batchArray){
		for_stb_array(model->batchArray){
			assets_material_save_to_path(it->material, str8_concat3(directory,str8_from_cstr(it->material->name),STR8(".mat"), deshi_temp_allocator));
			str8_builder_append(&builder, ToString8(deshi_temp_allocator, "\n\"",it->material->name,"\" ",it->indexOffset," ",it->indexCount));
		}
	}
	str8_builder_append(&builder, STR8("\n"));
	str8 model_text = str8_builder_peek(&builder);
	file_write_simple(path, model_text.str, model_text.count*sizeof(u8));
	Log("assets","Successfully saved model: ",path);
}


void
assets_model_delete(Model* model){
	if(model == assets_model_null()) return;
	
	for_stb_array(DeshAssets->model_array){
		if(*it == model){
			arrdelswap(DeshAssets->model_array, it - DeshAssets->model_array);
		}
	}
	arrfree(model->batchArray);
	memory_zfree(model);
}


#undef ParseError
//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @assets_font
FontPackedChar*
font_packed_char(Font* font, u32 codepoint){
	//TODO(delle) an overload for specifying range if you know where you're working
	forI(font->num_ranges){
		if(   (codepoint >= font->ranges[i].first_codepoint)
		   && (codepoint <  font->ranges[i].first_codepoint + font->ranges[i].num_chars)){
			return font->ranges[i].chardata_for_range + (codepoint - font->ranges[i].first_codepoint);
		}
	}
	Assert(!"The requested codepoint was not found in any of the ranges. TODO better error handling here.");
	return 0;
}


FontAlignedQuad
font_aligned_quad(Font* font, u32 codepoint, vec2* pos, vec2 scale){
	FontPackedChar* pc = font_packed_char(font, codepoint);
	if(pc){
		FontAlignedQuad q;
		q.x0 = pos->x + pc->xoff * scale.x;
		q.y0 = pos->y + (pc->yoff + font->ascent) * scale.y;
		q.x1 = pos->x + (pc->xoff2 - pc->xoff) * scale.x;
		q.y1 = pos->y + (pc->yoff2 + font->ascent) * scale.y;
		q.u0 = ((f32)pc->x0 / font->ttf_size[0]); //NOTE(sushi) we could maybe store the UV values normalized instead of doing this everytime
		q.v0 = ((f32)pc->y0 / font->ttf_size[1]) + font->uv_yoffset;
		q.u1 = ((f32)pc->x1 / font->ttf_size[0]);
		q.v1 = ((f32)pc->y1 / font->ttf_size[1]) + font->uv_yoffset;
		pos->x += pc->xadvance * scale.x;
		return q;
	}
	Assert(!"The requested codepoint was not found in any of the ranges. TODO better error handling here.");
	return FontAlignedQuad{};
}


vec2
font_visual_size(Font* font, str8 text){
	vec2 result = vec2{0, (f32)font->max_height};
	f32 line_width = 0;
	switch(font->type){
		case FontType_BDF: case FontType_NONE:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if(codepoint == '\n'){
					result.y += font->max_height;
					line_width = 0;
				}
				line_width += font->max_width * font->max_height / font->aspect_ratio / font->max_width;
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		case FontType_TTF:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if(codepoint == '\n'){
					result.y += font->max_height;
					line_width = 0;
				}
				line_width += font_packed_char(font, codepoint)->xadvance * font->max_height / font->aspect_ratio / font->max_width;
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		default: Assert(!"unhandled font type"); break;
	}
	return result;
}


Font*
assets_font_create_from_file(str8 name, u32 height){DPZoneScoped;
	return assets_font_create_from_path(str8_concat(STR8("data/fonts/"),name, deshi_temp_allocator), height);
}


Font*
assets_font_create_from_path(str8 path, u32 height){DPZoneScoped;
	if(str8_ends_with(path, STR8(".bdf"))){
		return assets_font_create_from_path_bdf(path);
	}
	
	if(str8_ends_with(path, STR8(".ttf")) || str8_ends_with(path, STR8(".otf"))){
		return assets_font_create_from_path_ttf(path, height);
	}
	
	LogE("assets","Failed to load font '",path,"'. We only support loading TTF/OTF and BDF fonts at the moment.");
	return assets_font_null();
}


Font*
assets_font_create_from_file_bdf(str8 name){DPZoneScoped;
	if(str8_equal_lazy(name, STR8("null"))) return assets_font_null();
	return assets_font_create_from_path_bdf(str8_concat(STR8("data/fonts/"),name, deshi_temp_allocator));
}


Font*
assets_font_create_from_path_bdf(str8 path){DPZoneScoped;
	//check if font was loaded already
	str8 filename = str8_skip_until_last(path, '/'); str8_advance(&filename);
	for_stb_array(DeshAssets->font_array){
		if(str8_equal_lazy((*it)->name, filename)){
			return *it;
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
	
	//init file
	File* file = file_init(path, FileAccess_Read);
	if(!file) return assets_font_null();
	defer{ file_deinit(file); };
	
	str8 first_line = file_read_line_alloc(file, &assets_load_allocator);
	if(!str8_begins_with(first_line, STR8("STARTFONT"))){
		LogE("assets","Error parsing BDF '",path,"' on line 1. The file did not begin with 'STARTFONT'.");
		return assets_font_null();
	}
	
	
	Font* font = (Font*)memory_alloc(sizeof(Font));
	font->type = FontType_BDF;
	font->name = filename;
	u32 line_number = 1;
	while(file->cursor < file->bytes){
		line_number += 1;
		
		//next line
		str8 line = file_read_line_alloc(file, &assets_load_allocator);
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
			if(str8_equal_lazy(key, STR8("ENDCHAR"))){
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
			if      (str8_equal_lazy(key, STR8("ENCODING"))){
				encodings[char_idx] = strtol((const char*)line.str, 0, 10);
			}else if(str8_equal_lazy(key, STR8("BITMAP"))){
				in_bitmap = true;
			}else if(str8_equal_lazy(key, STR8("SWIDTH"))){
				//unused
			}else if(str8_equal_lazy(key, STR8("DWIDTH"))){
				//unused in monospace fonts
			}else if(str8_equal_lazy(key, STR8("BBX"))){
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
		
		if      (str8_equal_lazy(key, STR8("STARTCHAR"))){
			in_char = true;
		}else if(str8_equal_lazy(key, STR8("SIZE"))){
			if(!line){
				LogE("assets","Error parsing BDF '",path,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			char* cursor = (char*)line.str;
			font_dpi.x = (f32)strtol(cursor+1, &cursor, 10);
			font_dpi.y = (f32)strtol(cursor+1, &cursor, 10);
		}else if(str8_equal_lazy(key, STR8("FONTBOUNDINGBOX"))){
			if(!line){
				LogE("assets","Error parsing BDF '",path,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			char* cursor = (char*)line.str;
			font_bbx.x = (f32)strtol(cursor,   &cursor, 10); //width
			font_bbx.y = (f32)strtol(cursor+1, &cursor, 10); //height
			font_bbx.z = (f32)strtol(cursor+1, &cursor, 10); //lower-left x
			font_bbx.w = (f32)strtol(cursor+1, &cursor, 10); //lower-left y
			font->max_width  = (u32)font_bbx.x;
			font->max_height = (u32)font_bbx.y;
		}else if(str8_equal_lazy(key, STR8("FONT_NAME"))){
			if(!line){
				LogE("assets","Error parsing BDF '",path,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			if(decoded_codepoint_from_utf8(line.str, 4).codepoint != '\"'){
				LogE("assets","Error parsing BDF '",path,"' on line ",line_number,". FONT_NAME must be wrapped in double quotes.");
				continue;
			}
			str8 font_name = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
			//TODO(sushi) replace name on Font with filename and add a name for this guy right here!
		}else if(str8_equal_lazy(key, STR8("WEIGHT_NAME"))){
			if(!line){
				LogE("assets","Error parsing BDF '",path,"' on line ",line_number,". No value passed to key: ",key);
				continue;
			}
			if(decoded_codepoint_from_utf8(line.str, 4).codepoint != '\"'){
				LogE("assets","Error parsing BDF '",path,"' on line ",line_number,". WEIGHT_NAME must be wrapped in double quotes.");
				continue;
			}
			str8 font_weight = str8_copy(str8_eat_until(str8{line.str+1,line.count-1}, '\"'), deshi_temp_allocator);
			cpystr(font->weight, (const char*)font_weight.str, 64);
		}else if(str8_equal_lazy(key, STR8("CHARS"))){
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
	
	Texture* texture = assets_texture_create_from_memory(pixels, filename, font->max_width, font->max_height*font->count,
														 ImageFormat_BW, TextureType_2D, TextureFilter_Nearest,
														 TextureAddressMode_ClampToWhite, false);
	
	font->aspect_ratio = (f32)font->max_height / font->max_width;
	font->tex = texture;
	
	arrput(DeshAssets->font_array, font);
	return font;
}


Font*
assets_font_create_from_file_ttf(str8 name, u32 height){DPZoneScoped;
	if(str8_equal_lazy(name, STR8("null"))) return assets_font_null();
	return assets_font_create_from_path_ttf(str8_concat(STR8("data/fonts/"),name, deshi_temp_allocator), height);
}


Font*
assets_font_create_from_path_ttf(str8 path, u32 size){DPZoneScoped;
	//TODO clean up this function some and add in some stuff to reduce the overhead of adding in a new range
	
	//check if font was loaded already
	//TODO look into why if we load the same font w a different size it gets weird (i took that check out of here for now)
	str8 filename = str8_skip_until_last(path, '/'); str8_advance(&filename);
	for_stb_array(DeshAssets->font_array){
		if(str8_equal_lazy((*it)->name, filename)){
			return *it;
		}
	}
	
	str8 contents = file_read_simple(path, deshi_temp_allocator);
	if(!contents) return assets_font_null();
	
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
	stbtt_pack_context* pc = (stbtt_pack_context*)memory_talloc(1*sizeof(stbtt_pack_context));
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
	
	Texture* texture = assets_texture_create_from_memory(pixels, filename, texture_size_x, texture_size_y,
														 ImageFormat_BW, TextureType_2D, TextureFilter_Nearest,
														 TextureAddressMode_ClampToWhite, false);
	
	Font* font = (Font*)memory_alloc(sizeof(Font));
	font->type         = FontType_TTF;
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
	font->ranges       = (FontPackRange*)ranges;
	
	arrput(DeshAssets->font_array, font);
	return font;
}

void
assets_font_delete(Font* font){DPZoneScoped;
	if(font == assets_font_null()) return;
	
	for_stb_array(DeshAssets->font_array){
		if(*it == font){
			arrdelswap(DeshAssets->font_array, it - DeshAssets->font_array);
		}
	}
	if(font->type == FontType_TTF){
		forI(font->num_ranges) memory_zfree(font->ranges[i].chardata_for_range);
		memory_zfree(font->ranges);
	}
	assets_texture_delete(font->tex);
	memory_zfree(font);
}