//-------------------------------------------------------------------------------------------------
// @OPENGL STRUCTS
struct ModelCmdGl{
	u32   vtxOffset;
	u32   idxOffset;
	u32   idxCount;
	u32   material;
	char* name;
	mat4  matrix;
};

struct TwodCmdGl{
	u16 indexOffset;
	u16 indexCount;
	vec2 scissorOffset;
	vec2 scissorExtent;
	b32 textured;
	u32 texIdx;
};

struct ShaderGl{
	char filename[DESHI_NAME_SIZE];
	u32 handle;
	ShaderStage stage;
};

struct ProgramGl{
	u32 handle;
	u32 shader_count;
	u32 shaders[5];
};

struct MeshGl{
	Mesh* base;
	u32 vtxOffset;
	u32 vtxCount;
	u32 idxOffset;
	u32 idxCount;
};

struct TextureGl{
	Texture* base;
	GLenum format;
	GLenum type;
	u32 handle;
};

struct MaterialGl{
	Material* base;
	ProgramGl* program;
	u32 texture_count;
	u32 textures[4];
};

//-------------------------------------------------------------------------------------------------
// @INTERFACE VARIABLES
local RenderSettings settings;
local ConfigMap configMap = {
	{"#render settings config file",0,0},
	
	{"\n#    //// REQUIRES RESTART ////",  ConfigValueType_PADSECTION,(void*)21},
	{"debugging",            ConfigValueType_Bool, &settings.debugging}, //!Incomplete
	{"printf",               ConfigValueType_Bool, &settings.printf}, //!Incomplete
	{"texture_filtering",    ConfigValueType_Bool, &settings.textureFiltering}, //!Incomplete
	{"anistropic_filtering", ConfigValueType_Bool, &settings.anistropicFiltering}, //!Incomplete
	{"msaa_level",           ConfigValueType_U32,  &settings.msaaSamples}, //!Incomplete
	{"recompile_all_shaders",ConfigValueType_Bool, &settings.recompileAllShaders}, //!Incomplete
	
	{"\n#    //// RUNTIME VARIABLES ////", ConfigValueType_PADSECTION,(void*)15},
	{"logging_level",  ConfigValueType_U32,  &settings.loggingLevel},
	{"crash_on_error", ConfigValueType_Bool, &settings.crashOnError},
	{"vsync_type",     ConfigValueType_U32,  &settings.vsync}, //!Incomplete
	
	{"\n#shaders",                         ConfigValueType_PADSECTION,(void*)17},
	{"optimize_shaders", ConfigValueType_Bool, &settings.optimizeShaders}, //!Incomplete
	
	{"\n#shadows",                         ConfigValueType_PADSECTION,(void*)20},
	{"shadow_pcf",          ConfigValueType_Bool, &settings.shadowPCF}, //!Incomplete
	{"shadow_resolution",   ConfigValueType_U32,  &settings.shadowResolution}, //!Incomplete
	{"shadow_nearz",        ConfigValueType_F32,  &settings.shadowNearZ}, //!Incomplete
	{"shadow_farz",         ConfigValueType_F32,  &settings.shadowFarZ}, //!Incomplete
	{"depth_bias_constant", ConfigValueType_F32,  &settings.depthBiasConstant}, //!Incomplete
	{"depth_bias_slope",    ConfigValueType_F32,  &settings.depthBiasSlope}, //!Incomplete
	{"show_shadow_map",     ConfigValueType_Bool, &settings.showShadowMap}, //!Incomplete
	
	{"\n#colors",                          ConfigValueType_PADSECTION,(void*)15},
	{"clear_color",    ConfigValueType_FV4, &settings.clearColor},
	{"selected_color", ConfigValueType_FV4, &settings.selectedColor},
	{"collider_color", ConfigValueType_FV4, &settings.colliderColor},
	
	{"\n#filters",                         ConfigValueType_PADSECTION,(void*)15},
	{"wireframe_only", ConfigValueType_Bool, &settings.wireframeOnly},
	
	{"\n#overlays",                        ConfigValueType_PADSECTION,(void*)17},
	{"mesh_wireframes",  ConfigValueType_Bool, &settings.meshWireframes},
	{"mesh_normals",     ConfigValueType_Bool, &settings.meshNormals}, //!Incomplete
	{"light_frustrums",  ConfigValueType_Bool, &settings.lightFrustrums}, //!Incomplete
	{"temp_mesh_on_top", ConfigValueType_Bool, &settings.tempMeshOnTop},
};

local RenderStats   stats{};
local RendererStage rendererStage = RENDERERSTAGE_NONE; //!Incomplete


//-------------------------------------------------------------------------------------------------
// @OPENGL VARIABLES
#define INDEX_TYPE_GL_TWOD GL_UNSIGNED_INT
#define INDEX_TYPE_GL_TEMP GL_UNSIGNED_SHORT
#define INDEX_TYPE_GL_MESH GL_UNSIGNED_INT

local array<MeshGl>     glMeshes(deshi_allocator);
local array<ShaderGl>   glShaders(deshi_allocator);
local array<TextureGl>  glTextures(deshi_allocator);
local array<MaterialGl> glMaterials(deshi_allocator);

///////////////////
//// @commands ////
///////////////////
//arbitray limits, change if needed
#define MAX_TWOD_VERTICES 0xFFFF //max u16: 65535
#define MAX_TWOD_INDICES  3*MAX_TWOD_VERTICES
#define MAX_TWOD_CMDS     1000
#define TWOD_LAYERS       11
typedef u32 TwodIndexGl; //if you change this make sure to change whats passed in the vkCmdBindIndexBuffer as well
local TwodIndexGl twodVertexCount = 0;
local TwodIndexGl twodIndexCount  = 0;
local Vertex2     twodVertexArray[MAX_TWOD_VERTICES];
local TwodIndexGl twodIndexArray[MAX_TWOD_INDICES];
local TwodIndexGl twodCmdCounts[TWOD_LAYERS];
local TwodCmdGl   twodCmdArrays[TWOD_LAYERS][MAX_TWOD_CMDS];  //different 2d cmd per texture

#define MAX_TEMP_VERTICES 0xFFFF //max u16: 65535
#define MAX_TEMP_INDICES 3*MAX_TEMP_VERTICES
typedef u16 TempIndexGl;
local TempIndexGl  tempWireframeVertexCount = 0;
local TempIndexGl  tempFilledVertexCount    = 0;
local TempIndexGl  tempWireframeIndexCount  = 0;
local TempIndexGl  tempFilledIndexCount     = 0;
local Mesh::Vertex tempWireframeVertexArray[MAX_TEMP_VERTICES];
local Mesh::Vertex tempFilledVertexArray   [MAX_TEMP_VERTICES];
local TempIndexGl  tempWireframeIndexArray [MAX_TEMP_INDICES];
local TempIndexGl  tempFilledIndexArray    [MAX_TEMP_INDICES];
local TempIndexGl  debugWireframeVertexCount = 0;
local TempIndexGl  debugWireframeIndexCount  = 0;
local Mesh::Vertex debugWireframeVertexArray[MAX_TEMP_VERTICES];
local TempIndexGl  debugWireframeIndexArray [MAX_TEMP_INDICES];
local TempIndexGl  debugFilledVertexCount    = 0;
local TempIndexGl  debugFilledIndexCount     = 0;
local Mesh::Vertex debugFilledVertexArray   [MAX_TEMP_VERTICES];
local TempIndexGl  debugFilledIndexArray    [MAX_TEMP_INDICES];

#define MAX_MODEL_CMDS 10000 
typedef u32 ModelIndexGl;
local ModelIndexGl modelCmdCount = 0;
local ModelCmdGl   modelCmdArray[MAX_MODEL_CMDS];

////////////////
//// @state ////
////////////////
local s32  width  = 0;
local s32  height = 0;
local bool initialized  = false;
local bool remake_window = false;
local s32  opengl_success = 0;
#define OPENGL_INFOLOG_SIZE 512
local char opengl_infolog[OPENGL_INFOLOG_SIZE] = {};

//////////////////
//// @buffers ////
//////////////////
local struct{
	u32 vao_handle;
	u32 vbo_handle;
	u32 ibo_handle;
	u32 vbo_size;
	u32 vbo_alloc;
	u32 ibo_size;
	u32 ibo_alloc;
} meshBuffers{};

local struct{
	u32 vao_handle;
	u32 vbo_handle;
	u32 ibo_handle;
	u32 vbo_size;
	u32 vbo_alloc;
	u32 ibo_size;
	u32 ibo_alloc;
} uiBuffers{};

local struct{
	u32 vao_handle;
	u32 vbo_handle;
	u32 ibo_handle;
	u32 vbo_alloc;
	u32 ibo_alloc;
} tempBuffers{};

local struct{ //vertex shader uniform buffer
	u32 handle;
	u32 binding = 1;
	
	struct{ //416 bytes
		mat4 view;        //camera view matrix
		mat4 proj;        //camera projection matrix
		vec4 lights[10];  //lights
		vec4 viewPos;     //camera pos
		vec2 screen;      //screen dimensions
		vec2 mousepos;    //mouse screen pos
		vec3 mouseWorld;  //point casted out from mouse 
		f32  time;        //total time
		mat4 lightVP;     //first light's view projection matrix
		s32  enablePCF;   //whether to blur shadow edges //TODO(delle,ReVu) convert to specialization constant
		s32  padding[3];
	} values;
} uboVS{};

local struct{ //vertex shader push constant
	u32 handle;
	u32 binding = 2;
	
	struct{ //64 bytes
		mat4 matrix;
	} values;
} pushVS{};

local struct{ //twod vertex shader push constant
	u32 handle;
	u32 binding = 3;
	
	struct{ //32 bytes
		vec2 scale;
		vec2 translate;
		s32  font_idx;
		s32  padding[3];
	} values;
} push2D{};

///////////////////
//// @programs ////
///////////////////
local struct{
	union{
		ProgramGl arr[16];
		struct{
			ProgramGl null;
			//game shaders
			ProgramGl flat;
			ProgramGl phong;
			ProgramGl pbr;
			ProgramGl lavalamp;
			ProgramGl twod;
			ProgramGl ui;
			//development shaders
			ProgramGl base;
			ProgramGl wireframe;
			ProgramGl wireframe_depth;
			ProgramGl selected;
			ProgramGl collider;
			ProgramGl testing0;
			ProgramGl testing1;
			ProgramGl offscreen;
			//debug shaders
			ProgramGl normals_debug;
			ProgramGl shadowmap_debug;
		};
	};
} programs{};

//-------------------------------------------------------------------------------------------------
// @OPENGL FUNCTIONS
////////////////////
//// @utilities ////
////////////////////
template<typename... Args>
local inline void
PrintGl(u32 level, Args... args){
	if(settings.loggingLevel >= level){
		Log("[OpenGL] ", args...);
	}
}

local void 
DebugPostCallback(void *ret, const char *name, GLADapiproc apiproc, s32 len_args, ...){
	GLenum error_code = glad_glGetError();
	if(error_code != GL_NO_ERROR){
		const char* error_flag = 0;
		switch(error_code){
			case GL_INVALID_ENUM:                  error_flag = "GL_INVALID_ENUM"; break; //Set when an enumeration parameter is not legal.
			case GL_INVALID_VALUE:                 error_flag = "GL_INVALID_VALUE"; break; //Set when a value parameter is not legal.
			case GL_INVALID_OPERATION:             error_flag = "GL_INVALID_OPERATION"; break; //Set when the state for a command is not legal for its given parameters.
			case 1283:                             error_flag = "GL_STACK_OVERFLOW"; break; //Set when a stack pushing operation causes a stack overflow.
			case 1284:                             error_flag = "GL_STACK_UNDERFLOW"; break; //Set when a stack popping operation occurs while the stack is at its lowest point.
			case GL_OUT_OF_MEMORY:                 error_flag = "GL_OUT_OF_MEMORY"; break; //    Set when a memory allocation operation cannot allocate (enough) memory.
			case GL_INVALID_FRAMEBUFFER_OPERATION: error_flag = "GL_INVALID_FRAMEBUFFER_OPERATION"; break; //Set when reading or writing to a framebuffer that is not complete.
		}
		PrintGl(0, "ERROR_",error_code," '",error_flag,"' on ",name,"(); Info: http://docs.gl/gl3/",name);
		if(settings.crashOnError) Assert(!"crashing because of error in opengl");
	}
}


///////////////////
//// @commands ////
///////////////////
local void
SetupCommands(){
	//ui vertex and index buffers
	u64 ui_vb_size = Max(1000*sizeof(Vertex2),   twodVertexCount * sizeof(Vertex2));
	u64 ui_ib_size = Max(3000*sizeof(TwodIndexGl), twodIndexCount  * sizeof(TwodIndexGl));
	if(ui_vb_size && ui_ib_size){
		//create vertex array object and buffers if they dont exist
		if(uiBuffers.vao_handle == 0){
			glGenVertexArrays(1, &uiBuffers.vao_handle);
			glGenBuffers(1, &uiBuffers.vbo_handle);
			glGenBuffers(1, &uiBuffers.ibo_handle);
		}
		
		//bind buffers
		glBindVertexArray(uiBuffers.vao_handle);
		glBindBuffer(GL_ARRAY_BUFFER, uiBuffers.vbo_handle);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiBuffers.ibo_handle);
		
		//specify vertex packing
		glVertexAttribPointer(0, 2,  GL_FLOAT,         GL_FALSE, sizeof(Vertex2), (void*)offsetof(Vertex2,pos));
		glVertexAttribPointer(1, 2,  GL_FLOAT,         GL_FALSE, sizeof(Vertex2), (void*)offsetof(Vertex2,uv));
		glVertexAttribPointer(2, 4,  GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Vertex2), (void*)offsetof(Vertex2,color));
		glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2);
		
		//resize buffers if too small and update buffer sizes
		if(uiBuffers.vbo_alloc < ui_vb_size){
			glBufferData(GL_ARRAY_BUFFER, ui_vb_size, 0, GL_STATIC_DRAW);
			uiBuffers.vbo_alloc = ui_vb_size;
		}
		if(uiBuffers.ibo_alloc < ui_ib_size){
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, ui_ib_size, 0, GL_STATIC_DRAW);
			uiBuffers.ibo_alloc = ui_ib_size;
		}
		
		//fill buffers
		glBufferSubData(GL_ARRAY_BUFFER,         0, ui_vb_size, twodVertexArray);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, ui_ib_size, twodIndexArray);
	}
	
	//temp mesh vertex and index buffers
	u64 temp_wire_vb_size = tempWireframeVertexCount*sizeof(Mesh::Vertex);
	u64 temp_fill_vb_size = tempFilledVertexCount*sizeof(Mesh::Vertex);
	u64 temp_wire_ib_size = tempWireframeIndexCount*sizeof(TempIndexGl);
	u64 temp_fill_ib_size = tempFilledIndexCount*sizeof(TempIndexGl);
	u64 temp_vb_size = temp_wire_vb_size+temp_fill_vb_size;
	u64 temp_ib_size = temp_wire_ib_size+temp_fill_ib_size;
	temp_vb_size = Max(1000*sizeof(Mesh::Vertex), temp_vb_size);
	temp_ib_size = Max(3000*sizeof(TempIndexGl),  temp_ib_size);
	if(temp_vb_size && temp_ib_size){
		//create vertex array object and buffers if they dont exist
		if(tempBuffers.vao_handle == 0){
			glGenVertexArrays(1, &tempBuffers.vao_handle);
			glGenBuffers(1, &tempBuffers.vbo_handle);
			glGenBuffers(1, &tempBuffers.ibo_handle);
		}
		
		//bind buffers
		glBindVertexArray(tempBuffers.vao_handle);
		glBindBuffer(GL_ARRAY_BUFFER, tempBuffers.vbo_handle);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tempBuffers.ibo_handle);
		
		//specify vertex packing
		glVertexAttribPointer(0, 3,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,pos));
		glVertexAttribPointer(1, 2,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,uv));
		glVertexAttribPointer(2, 4,  GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,color));
		glVertexAttribPointer(3, 3,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,normal));
		glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2); glEnableVertexAttribArray(3);
		
		//resize buffers if too small and update buffer sizes
		if(tempBuffers.vbo_alloc < temp_vb_size){
			glBufferData(GL_ARRAY_BUFFER, temp_vb_size, 0, GL_STATIC_DRAW);
			tempBuffers.vbo_alloc = temp_vb_size;
		}
		if(tempBuffers.ibo_alloc < temp_ib_size){
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, temp_ib_size, 0, GL_STATIC_DRAW);
			tempBuffers.ibo_alloc = temp_ib_size;
		}
		
		//fill buffers
		glBufferSubData(GL_ARRAY_BUFFER, 0,                 temp_fill_vb_size, tempFilledVertexArray);
		glBufferSubData(GL_ARRAY_BUFFER, temp_fill_vb_size, temp_wire_vb_size, tempWireframeVertexArray);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,                 temp_fill_ib_size, tempFilledIndexArray);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, temp_fill_ib_size, temp_wire_ib_size, tempWireframeIndexArray);
	}
}

local void
ResetCommands(){
	{//UI commands
		twodVertexCount = 0;
		twodIndexCount  = 0;
		forX(layer, 9){
			memset(&twodCmdArrays[layer][0], 0, sizeof(TwodCmdGl) * twodCmdCounts[layer]);
			twodCmdCounts[layer] = 1;
		}
	}
	
	{//temp commands
		tempWireframeVertexCount = 0;
		tempWireframeIndexCount  = 0;
		tempFilledVertexCount = 0;
		tempFilledIndexCount  = 0;
	}
	
	{//model commands
		modelCmdCount = 0;
	}
}


//////////////////
//// @shaders ////
//////////////////
local u32
CompileAndLoadShader(const char* filename, ShaderStage stage){
	Assert(stage > ShaderStage_NONE && stage < ShaderStage_COUNT);
	
	//check if already loaded
	forE(glShaders){ if(strcmp(it->filename, filename) == 0){ return u32(it-it_begin); } }
	
	ShaderGl sgl{};
	cpystr(sgl.filename, filename, DESHI_NAME_SIZE);
	sgl.stage = stage;
	
	//create shader, load file, and compile shader
	switch(stage){ //TODO(delle) opengl4 shader stages
		case ShaderStage_Vertex:   sgl.handle = glCreateShader(GL_VERTEX_SHADER);   break;
		case ShaderStage_TessCtrl: Assert(!"not implemented yet REQUIRES OPENGL4"); break;
		case ShaderStage_TessEval: Assert(!"not implemented yet REQUIRES OPENGL4"); break;
		case ShaderStage_Geometry: sgl.handle = glCreateShader(GL_GEOMETRY_SHADER); break;
		case ShaderStage_Fragment: sgl.handle = glCreateShader(GL_FRAGMENT_SHADER); break;
		case ShaderStage_Compute:  Assert(!"not implemented yet REQUIRES OPENGL4"); break;
	}
	char* filebuff = Assets::readFileAsciiToArray(Assets::dirShaders()+filename);
	if(!filebuff) Assert(!"Failed to load shader");
	glShaderSource(sgl.handle, 1, &filebuff, 0);
	glCompileShader(sgl.handle);
	
	//check for errors
	glGetShaderiv(sgl.handle, GL_COMPILE_STATUS, &opengl_success);
	if(opengl_success != GL_TRUE){
		glGetShaderInfoLog(sgl.handle, OPENGL_INFOLOG_SIZE, 0, opengl_infolog);
		PrintGl(0, "Failed to compile shader '",filename,"':\n",opengl_infolog);
		glDeleteShader(sgl.handle);
		return 0;
	}
	
	glShaders.add(sgl);
	return glShaders.count-1;
}


///////////////////
//// @programs ////
///////////////////
//TODO(delle) cleanup shaders maybe? glDeleteShader()
local ProgramGl
CreateProgram(u32 shader_indexes[], u32 shader_count, bool twod = false){
	if(shader_count == 0) return {};
	//TODO(delle) Assert(ubo's setup)
	
	ProgramGl pgl{};
	pgl.shader_count = shader_count;
	string prog_shaders = "|";
	
	//create program, attach shaders and link
	pgl.handle = glCreateProgram();
	forI(shader_count){ 
		glAttachShader(pgl.handle, glShaders[shader_indexes[i]].handle); 
		pgl.shaders[i] = shader_indexes[i]; 
		prog_shaders += glShaders[shader_indexes[i]].filename;
		prog_shaders += "|";
	}
	glLinkProgram(pgl.handle);
	
	//check for errors
	glGetProgramiv(pgl.handle, GL_LINK_STATUS, &opengl_success);
	if(opengl_success != GL_TRUE){
		glGetProgramInfoLog(pgl.handle, OPENGL_INFOLOG_SIZE, 0, opengl_infolog);
		PrintGl(0,"Failed to link program '",prog_shaders,"':\n",opengl_infolog);
		
		//delete broken program
		glDeleteProgram(pgl.handle);
		return {};
	}
	
	//specify UBOs
	if(!twod){
		u32 local_ubi = glGetUniformBlockIndex(pgl.handle,"UniformBufferObject");
		if(local_ubi != -1){ 
			glUniformBlockBinding(pgl.handle, local_ubi, uboVS.binding);
		}else{
			PrintGl(0,"Failed to find UniformBufferObject in vertex shader of program '",prog_shaders,"'");
			if(settings.crashOnError) Assert(false);
			glDeleteProgram(pgl.handle);
			return {};
		}
		
		local_ubi = glGetUniformBlockIndex(pgl.handle,"PushConsts");
		if(local_ubi != -1){ 
			glUniformBlockBinding(pgl.handle, local_ubi, pushVS.binding);
		}else{
			PrintGl(0,"Failed to find PushConsts in vertex shader of program '",prog_shaders,"'");
			if(settings.crashOnError) Assert(false);
			glDeleteProgram(pgl.handle);
			return {};
		}
	}else{
		u32 local_ubi = glGetUniformBlockIndex(pgl.handle,"PushConsts");
		if(local_ubi != -1){ 
			glUniformBlockBinding(pgl.handle, local_ubi, push2D.binding);
		}else{
			PrintGl(0,"Failed to find PushConsts in vertex shader of program '",prog_shaders,"'");
			if(settings.crashOnError) Assert(false);
			glDeleteProgram(pgl.handle);
			return {};
		}
	}
	
	//detach shaders
	forI(shader_count){ glDetachShader(pgl.handle, glShaders[shader_indexes[i]].handle); }
	
	return pgl;
}

//@@
local void
SetupPrograms(){
	u32 shaders[5] = {};
	u32 shader_count = 0;
	
	{//base
		shaders[0] = CompileAndLoadShader("base_gl3.vert", ShaderStage_Vertex);
		shaders[1] = CompileAndLoadShader("base_gl3.frag", ShaderStage_Fragment);
		shader_count = 2;
		programs.base = CreateProgram(shaders, shader_count);
		
		{//selected
			
		}
	}
	
	{//null
		shaders[0] = CompileAndLoadShader("null_gl3.vert", ShaderStage_Vertex);
		shaders[1] = CompileAndLoadShader("null_gl3.frag", ShaderStage_Fragment);
		shader_count = 2;
		programs.null = CreateProgram(shaders, shader_count);
	}
	
	{//flat
		shaders[0] = CompileAndLoadShader("flat_gl3.vert", ShaderStage_Vertex);
		shaders[1] = CompileAndLoadShader("flat_gl3.frag", ShaderStage_Fragment);
		shader_count = 2;
		programs.flat = CreateProgram(shaders, shader_count);
	}
	
	{//phong
		shaders[0] = CompileAndLoadShader("phong_gl3.vert", ShaderStage_Vertex);
		shaders[1] = CompileAndLoadShader("phong_gl3.frag", ShaderStage_Fragment);
		shader_count = 2;
		programs.phong = CreateProgram(shaders, shader_count);
	}
	
	{//pbr
		shaders[0] = CompileAndLoadShader("pbr_gl3.vert", ShaderStage_Vertex);
		shaders[1] = CompileAndLoadShader("pbr_gl3.frag", ShaderStage_Fragment);
		shader_count = 2;
		programs.pbr = CreateProgram(shaders, shader_count);
	}
	
	{//2d
		shaders[0] = CompileAndLoadShader("twod_gl3.vert", ShaderStage_Vertex);
		shaders[1] = CompileAndLoadShader("twod_gl3.frag", ShaderStage_Fragment);
		shader_count = 2;
		programs.twod = CreateProgram(shaders, shader_count, true);
		
		{//ui
			shaders[0] = CompileAndLoadShader("ui_gl3.vert", ShaderStage_Vertex);
			shaders[1] = CompileAndLoadShader("ui_gl3.frag", ShaderStage_Fragment);
			shader_count = 2;
			programs.ui = CreateProgram(shaders, shader_count, true);
			
			
		}
	}
	
	{//wireframe
		shaders[0] = CompileAndLoadShader("wireframe_gl3.vert", ShaderStage_Vertex);
		shaders[1] = CompileAndLoadShader("wireframe_gl3.frag", ShaderStage_Fragment);
		shader_count = 2;
		programs.wireframe = CreateProgram(shaders, shader_count);
		
		{//wireframe with depth test
			
		}
	}
	
	{//lavalamp
		shaders[0] = CompileAndLoadShader("lavalamp_gl3.vert", ShaderStage_Vertex);
		shaders[1] = CompileAndLoadShader("lavalamp_gl3.frag", ShaderStage_Fragment);
		shader_count = 2;
		programs.lavalamp = CreateProgram(shaders, shader_count);
	}
	
	{//offscreen
		shaders[0] = CompileAndLoadShader("offscreen_gl3.vert", ShaderStage_Vertex);
		shader_count = 1;
		programs.offscreen = CreateProgram(shaders, shader_count);
	}
	
	{//testing0
		shaders[0] = CompileAndLoadShader("testing0_gl3.vert", ShaderStage_Vertex);
		shaders[1] = CompileAndLoadShader("testing0_gl3.frag", ShaderStage_Fragment);
		shader_count = 2;
		programs.testing0 = CreateProgram(shaders, shader_count);
	}
	
	{//testing1
		shaders[0] = CompileAndLoadShader("testing1_gl3.vert", ShaderStage_Vertex);
		shaders[1] = CompileAndLoadShader("testing1_gl3.frag", ShaderStage_Fragment);
		shader_count = 2;
		programs.testing1 = CreateProgram(shaders, shader_count);
	}
	
	{//DEBUG mesh normals
		shaders[0] = CompileAndLoadShader("nothing_gl3.vert", ShaderStage_Vertex);
		shaders[1] = CompileAndLoadShader("normaldebug_gl3.geom", ShaderStage_Geometry);
		shaders[2] = CompileAndLoadShader("nothing_gl3.frag", ShaderStage_Fragment);
		shader_count = 3;
		programs.normals_debug = CreateProgram(shaders, shader_count);
	}
	
	{//DEBUG shadow map
		//shaders[0] = CompileAndLoadShader("shadowmapDEBUG.vert", ShaderStage_Vertex);
		//shaders[1] = CompileAndLoadShader("shadowmapDEBUG.frag", ShaderStage_Fragment);
		//shader_count = 2;
		//programs.shadowmap_debug = CreateProgram(shaders, shader_count);
	}
	
} //SetupPrograms()


//////////////////
//// @buffers ////
//////////////////
local void
SetupUniformBuffers(){
	//vertex shader ubo
	glGenBuffers(1, &uboVS.handle);
	glBindBufferRange(GL_UNIFORM_BUFFER, uboVS.binding, uboVS.handle, 0, sizeof(uboVS.values));
	
	//vertex shader push constant
	glGenBuffers(1, &pushVS.handle);
	glBindBufferRange(GL_UNIFORM_BUFFER, pushVS.binding, pushVS.handle, 0, sizeof(pushVS.values));
	glBufferData(GL_UNIFORM_BUFFER, sizeof(pushVS.values), 0, GL_DYNAMIC_DRAW);
	
	//twod vertex shader push constant
	glGenBuffers(1, &push2D.handle);
	glBindBufferRange(GL_UNIFORM_BUFFER, push2D.binding, push2D.handle, 0, sizeof(push2D.values));
	glBufferData(GL_UNIFORM_BUFFER, sizeof(push2D.values), 0, GL_DYNAMIC_DRAW);
}

local void
UpdateUniformBuffers(){
	uboVS.values.screen     = vec2((f32)width, (f32)height);
	uboVS.values.mousepos   = vec2(DeshInput->mousePos.x, DeshInput->mousePos.y);
	uboVS.values.mouseWorld = Math::ScreenToWorld(DeshInput->mousePos, uboVS.values.proj, uboVS.values.view, DeshWindow->dimensions);
	uboVS.values.time       = DeshTime->totalTime;
	uboVS.values.enablePCF  = settings.shadowPCF;
	
	glBindBuffer(GL_UNIFORM_BUFFER, uboVS.handle);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(uboVS.values), &uboVS.values, GL_STREAM_DRAW);
}

//-------------------------------------------------------------------------------------------------
// @IMGUI FUNCTIONS
local char iniFilepath[256] = {};
void DeshiImGui::
Init(){
	AddFlag(deshiStage, DS_IMGUI);
	TIMER_START(t_s);
	
	//Setup Dear ImGui context
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	cpystr(iniFilepath, (Assets::dirConfig() + "imgui.ini").c_str(), 256);
	io.IniFilename = iniFilepath;
	
	//Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	
	//Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(DeshWindow->window, true);
	ImGui_ImplOpenGL3_Init();
	
	Log("deshi","Finished imgui initialization in ",TIMER_END(t_s),"ms");
}

void DeshiImGui::
Cleanup(){
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void DeshiImGui::
NewFrame(){
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}


//-------------------------------------------------------------------------------------------------
// @2D @TWOD INTERFACE
//TODO(sushi) find a nicer way to keep track of this
//NOTE im not sure yet if i should be keeping track of this for each primitive or not yet but i dont think i have to
vec2 prevScissorOffset = vec2::ZERO;
vec2 prevScissorExtent = vec2::ZERO;

void CheckUICmdArrays(u32 layer, Texture* tex, b32 textured, vec2 scissorOffset, vec2 scissorExtent){
	if((twodCmdArrays[layer][twodCmdCounts[layer] - 1].textured != textured)
	   || ((tex) ? twodCmdArrays[layer][twodCmdCounts[layer] - 1].texIdx != tex->idx : 0)
	   || (scissorOffset != prevScissorOffset)   //im doing these 2 because we have to know if we're drawing in a new window
	   || (scissorExtent != prevScissorExtent)){ //and you could do text last in one, and text first in another {  
		prevScissorExtent = scissorExtent;
		prevScissorOffset = scissorOffset;         //NOTE null_font is the default texture for 2D items, as its just a white square
		twodCmdArrays[layer][twodCmdCounts[layer]].texIdx = (tex) ? tex->idx : 1;
		twodCmdArrays[layer][twodCmdCounts[layer]].indexOffset = twodIndexCount;
		twodCmdArrays[layer][twodCmdCounts[layer]].textured = textured;
		twodCmdCounts[layer]++;
	}
	Assert(twodCmdCounts[layer] <= MAX_TWOD_CMDS);
}

void Render::FillTriangle2D(vec2 p1, vec2 p2, vec2 p3, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0,
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	CheckUICmdArrays(layer, 0, 0, scissorOffset, scissorExtent);
	
	u32       col = color.rgba;
	Vertex2*   vp = twodVertexArray + twodVertexCount;
	TwodIndexGl* ip = twodIndexArray + twodIndexCount;
	
	ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
	vp[0].pos = p1; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = p2; vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = p3; vp[2].uv = { 0,0 }; vp[2].color = col;
	
	twodVertexCount += 3;
	twodIndexCount += 3;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 3;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorExtent = scissorExtent;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorOffset = scissorOffset;
	
}

void Render::DrawTriangle2D(vec2 p1, vec2 p2, vec2 p3, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0,
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	
	DrawLine2D(p1, p2, 1, color, layer, scissorOffset, scissorExtent);
	DrawLine2D(p2, p3, 1, color, layer, scissorOffset, scissorExtent);
	DrawLine2D(p3, p1, 1, color, layer, scissorOffset, scissorExtent);
}

void Render::FillRect2D(vec2 pos, vec2 dimensions, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0, 
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	CheckUICmdArrays(layer, 0, 0, scissorOffset, scissorExtent);
	
	u32       col = color.rgba;
	Vertex2*   vp = twodVertexArray + twodVertexCount;
	TwodIndexGl* ip = twodIndexArray + twodIndexCount;
	
	ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
	ip[3] = twodVertexCount; ip[4] = twodVertexCount + 2; ip[5] = twodVertexCount + 3;
	vp[0].pos = { pos.x + 0,           pos.y + 0 };            vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = { pos.x + dimensions.w,pos.y + 0 };            vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = { pos.x + dimensions.w,pos.y + dimensions.h }; vp[2].uv = { 0,0 }; vp[2].color = col;
	vp[3].pos = { pos.x + 0,           pos.y + dimensions.h }; vp[3].uv = { 0,0 }; vp[3].color = col;
	
	twodVertexCount += 4;
	twodIndexCount += 6;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorExtent = scissorExtent;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorOffset = scissorOffset;
}

//this func is kind of scuffed i think because of the line thickness stuff when trying to draw
//straight lines, see below
void Render::DrawRect2D(vec2 pos, vec2 dimensions, f32 thickness, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0, 
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	
	//top left to top right 
	DrawLine2D(pos.xAdd(-thickness), pos.xAdd(thickness/2)+ dimensions.ySet(0), thickness, color, layer, scissorOffset, scissorExtent);
	//top left to bottom left
	DrawLine2D(pos, pos + dimensions.xSet(0), thickness, color, layer, scissorOffset, scissorExtent);
	//bottom right to top right
	DrawLine2D(pos + dimensions, pos + dimensions.ySet(0), thickness, color, layer, scissorOffset, scissorExtent);
	//bottom right to bottom left
	DrawLine2D((pos + dimensions).xAdd(thickness/2), pos + dimensions.xSet(0).xAdd(-thickness), thickness, color, layer, scissorOffset, scissorExtent);
}

void Render::DrawCircle2D(vec2 pos, f32 radius, u32 subdivisions_int, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0, 
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	CheckUICmdArrays(layer, 0, 0, scissorOffset, scissorExtent);
	
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (f32(i - 1) * M_2PI) / subdivisions;
		f32 a1 = (f32(i) * M_2PI) / subdivisions;
		f32 x0 = pos.x + radius * cosf(a0); f32 x1 = pos.x + radius * cosf(a1);
		f32 y0 = pos.y + radius * sinf(a0); f32 y1 = pos.y + radius * sinf(a1);
		DrawLine2D(vec2(x0, y0), vec2(x1, y1), 1, color, layer, scissorOffset, scissorExtent);
	}
}

void Render::FillCircle2D(vec2 pos, f32 radius, u32 subdivisions_int, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0, 
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	CheckUICmdArrays(layer, 0, 0, scissorOffset, scissorExtent);
	
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (f32(i - 1) * M_2PI) / subdivisions;
		f32 a1 = (f32(i) * M_2PI) / subdivisions;
		f32 x0 = pos.x + radius * cosf(a0); f32 x1 = pos.x + radius * cosf(a1);
		f32 y0 = pos.y + radius * sinf(a0); f32 y1 = pos.y + radius * sinf(a1);
		FillTriangle2D(pos, vec2(x0, y0), vec2(x1, y1), color, layer, scissorOffset, scissorExtent);
	}
}

//TODO(sushi) implement special line drawing for straight lines, since we dont need to do the normal thing
//when drawing them straight
void Render::DrawLine2D(vec2 start, vec2 end, f32 thickness, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0, 
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	CheckUICmdArrays(layer, 0, 0, scissorOffset, scissorExtent);
	
	u32       col = color.rgba;
	Vertex2*   vp = twodVertexArray + twodVertexCount;
	TwodIndexGl* ip = twodIndexArray + twodIndexCount;
	
	vec2 ott = end - start;
	vec2 norm = vec2(ott.y, -ott.x).normalized();
	
	ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
	ip[3] = twodVertexCount; ip[4] = twodVertexCount + 2; ip[5] = twodVertexCount + 3;
	vp[0].pos = { start.x,start.y }; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = { end.x,  end.y };   vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = { end.x,  end.y };   vp[2].uv = { 0,0 }; vp[2].color = col;
	vp[3].pos = { start.x,start.y }; vp[3].uv = { 0,0 }; vp[3].color = col;
	
	vp[0].pos += norm * thickness / 2;
	vp[1].pos += norm * thickness / 2;
	vp[2].pos -= norm * thickness / 2;
	vp[3].pos -= norm * thickness / 2;
	
	twodVertexCount += 4;
	twodIndexCount += 6;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorExtent = scissorExtent;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorOffset = scissorOffset;
}

void Render::DrawLines2D(array<vec2>& points, f32 thickness, color color, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0, 
		   "Scissor Offset and Extent can't be negative");
	Assert(points.count > 1, "Lines need at least 2 points");
	if(color.a == 0 || thickness == 0) return;
	CheckUICmdArrays(layer, 0, 0, scissorOffset, scissorExtent);
	
	f32 halfthick = thickness / 2;
	
	u32       col = color.rgba;
	Vertex2*   vp = twodVertexArray + twodVertexCount;
	TwodIndexGl* ip = twodIndexArray + twodIndexCount;
	
	{// first point
		
		vec2 ott = points[1] - points[0];
		vec2 norm = vec2(ott.y, -ott.x).normalized();
		
		vp[0].pos = points[0] + norm * halfthick; vp[0].uv = { 0,0 }; vp[0].color = col;
		vp[1].pos = points[0] - norm * halfthick; vp[1].uv = { 0,0 }; vp[1].color = col;
		
		ip[0] = twodVertexCount;
		ip[1] = twodVertexCount + 1;
		ip[3] = twodVertexCount;
		
		twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 3;
		
		twodVertexCount += 2;
		twodIndexCount += 3;
		vp += 2;
	}
	
	//in betweens
	s32 flip = -1;
	for(s32 i = 1; i < points.count - 1; i++, flip *= -1){
		vec2 last, curr, next, norm;
		
		last = points[i - 1];
		curr = points[i];
		next = points[i + 1];
		
		//figure out average norm
		vec2
			p01 = curr - last,
		p12 = next - curr,
		p02 = next - last,
		//norm01 = vec2{ p01.y, -p01.x } * flip, //we flip the normal everytime to keep up the pattern
		//norm12 = vec2{ p12.y, -p12.x } * flip,
		normav;//((norm01 + norm12) / 2).normalized();
		
		f32 a = p01.mag(), b = p12.mag(), c = p02.mag();
		f32 ang = Radians(Math::AngBetweenVectors(-p01, p12));
		
		//this is the critical angle where the thickness of the 2 lines cause them to overlap at small angles
		//if(fabs(ang) < 2 * atanf(thickness / (2 * p02.mag()))){
		//	ang = 2 * atanf(thickness / (2 * p02.mag()));
		//}
		
		normav = p12.normalized();
		normav = Math::vec2RotateByAngle(-Degrees(ang) / 2, normav);
		normav *= (f32)flip;
		
		//this is where we calc how wide the thickness of the inner line is meant to be
		normav = normav.normalized() * thickness / ( 2 * sinf(ang / 2));
		
		vec2 normavout = normav;
		vec2 normavin = -normav;
		
		normavout.clampMag(0, thickness * 2);//sqrt(2) / 2 * thickness );
		normavin.clampMag(0, thickness * 4);
		
		//set indicies by pattern
		s32 ipidx = 6 * (i - 1) + 2;
		ip[ipidx + 0] =
			ip[ipidx + 2] =
			ip[ipidx + 4] =
			ip[ipidx + 7] =
			twodVertexCount;
		
		ip[ipidx + 3] =
			ip[ipidx + 5] =
			twodVertexCount + 1;
		
		vp[0].pos = curr + normavout; vp[0].uv = { 0,0 }; vp[0].color = col;//PackColorU32(255, 0, 0, 255);
		vp[1].pos = curr + normavin; vp[1].uv = { 0,0 }; vp[1].color = col;//PackColorU32(255, 0, 255, 255);
		
		twodVertexCount += 2;
		twodIndexCount += 6;
		vp += 2;
		
		twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
		
	}
	
	{//last point
		vec2 ott = points[points.count - 1] - points[points.count - 2];
		vec2 norm = vec2(ott.y, -ott.x).normalized() * (f32)flip;
		
		vp[0].pos = points[points.count - 1] + norm * halfthick; vp[0].uv = { 0,0 }; vp[0].color = col;//PackColorU32(255, 50, 255, 255);
		vp[1].pos = points[points.count - 1] - norm * halfthick; vp[1].uv = { 0,0 }; vp[1].color = col;//PackColorU32(255, 50, 100, 255);
		
		//set final indicies by pattern
		u32 ipidx = 6 * (points.count - 2) + 2;
		ip[ipidx + 0] = twodVertexCount;
		ip[ipidx + 2] = twodVertexCount;
		ip[ipidx + 3] = twodVertexCount + 1;
		
		twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 3;
		
		twodVertexCount += 2;
		twodIndexCount += 3;
		vp += 2; ip += 3;
	}
	
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorExtent = scissorExtent;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorOffset = scissorOffset;
}

void Render::
DrawText2D(Font* font, cstring text, vec2 pos, color color, vec2 scale, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0, 
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	CheckUICmdArrays(layer, font->tex, 0, scissorOffset, scissorExtent);
	
	switch (font->type){
		//// BDF (and NULL) font rendering ////
		case FontType_BDF: case FontType_NONE:{
			forI(text.count){
				u32       col = color.rgba;
				Vertex2*   vp = twodVertexArray + twodVertexCount;
				TwodIndexGl* ip = twodIndexArray + twodIndexCount;
				
				f32 w = font->max_width * scale.x;
				f32 h = font->max_height * scale.y;
				f32 dy = 1.f / (f32)font->count;
				
				f32 idx = f32(text[i] - 32);
				f32 topoff = idx * dy;
				f32 botoff = topoff + dy;
				
				ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
				ip[3] = twodVertexCount; ip[4] = twodVertexCount + 2; ip[5] = twodVertexCount + 3;
				vp[0].pos = { pos.x + 0,pos.y + 0 }; vp[0].uv = { 0,topoff+font->uvOffset }; vp[0].color = col;
				vp[1].pos = { pos.x + w,pos.y + 0 }; vp[1].uv = { 1,topoff+font->uvOffset }; vp[1].color = col;
				vp[2].pos = { pos.x + w,pos.y + h }; vp[2].uv = { 1,botoff+font->uvOffset }; vp[2].color = col;
				vp[3].pos = { pos.x + 0,pos.y + h }; vp[3].uv = { 0,botoff+font->uvOffset }; vp[3].color = col;
				
				twodVertexCount += 4;
				twodIndexCount += 6;
				twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
				twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorExtent = scissorExtent;
				twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorOffset = scissorOffset;
				
				pos.x += font->max_width * scale.x;
			}
		}break;
		//// TTF font rendering ////
		case FontType_TTF:{
			forI(text.count){
				u32       col = color.rgba;
				Vertex2*   vp = twodVertexArray + twodVertexCount;
				TwodIndexGl* ip = twodIndexArray + twodIndexCount;
				
				aligned_quad q = font->GetPackedQuad(text[i], &pos, scale);
				
				ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
				ip[3] = twodVertexCount; ip[4] = twodVertexCount + 2; ip[5] = twodVertexCount + 3;
				vp[0].pos={q.x0,q.y0}; vp[0].uv={q.s0,q.t0+font->uvOffset}; vp[0].color=col;
				vp[1].pos={q.x1,q.y0}; vp[1].uv={q.s1,q.t0+font->uvOffset}; vp[1].color=col;
				vp[2].pos={q.x1,q.y1}; vp[2].uv={q.s1,q.t1+font->uvOffset}; vp[2].color=col;
				vp[3].pos={q.x0,q.y1}; vp[3].uv={q.s0,q.t1+font->uvOffset}; vp[3].color=col;
				
				twodVertexCount += 4;
				twodIndexCount += 6;
				twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
				twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorExtent = scissorExtent;
				twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorOffset = scissorOffset;
			}break;
			default: Assert(!"unhandled font type"); break;
		}
	}
}

void Render::
DrawText2D(Font* font, wcstring text, vec2 pos, color color, vec2 scale, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0, 
		   "Scissor Offset and Extent can't be negative");
	if(color.a == 0) return;
	CheckUICmdArrays(layer, font->tex, 0, scissorOffset, scissorExtent);
	
	switch (font->type){
		//// BDF (and NULL) font rendering ////
		case FontType_BDF: case FontType_NONE:{
			forI(text.count){
				u32       col = color.rgba;
				Vertex2*   vp = twodVertexArray + twodVertexCount;
				TwodIndexGl* ip = twodIndexArray + twodIndexCount;
				
				f32 w = font->max_width * scale.x;
				f32 h = font->max_height * scale.y;
				f32 dy = 1.f / (f32)font->count;
				
				f32 idx = f32(text[i] - 32);
				f32 topoff = idx * dy;
				f32 botoff = topoff + dy;
				
				ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
				ip[3] = twodVertexCount; ip[4] = twodVertexCount + 2; ip[5] = twodVertexCount + 3;
				vp[0].pos = { pos.x + 0,pos.y + 0 }; vp[0].uv = { 0,topoff }; vp[0].color = col;
				vp[1].pos = { pos.x + w,pos.y + 0 }; vp[1].uv = { 1,topoff }; vp[1].color = col;
				vp[2].pos = { pos.x + w,pos.y + h }; vp[2].uv = { 1,botoff }; vp[2].color = col;
				vp[3].pos = { pos.x + 0,pos.y + h }; vp[3].uv = { 0,botoff }; vp[3].color = col;
				
				twodVertexCount += 4;
				twodIndexCount += 6;
				twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
				if(scissorExtent.x != -1){
					twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorExtent = scissorExtent;
					twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorOffset = scissorOffset;
				}else{
					twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorExtent = vec2((f32)width, (f32)height);
					twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorOffset = vec2(0, 0);
				}
				
				pos.x += font->max_width * scale.x;
			}
		}break;
		//// TTF font rendering ////
		case FontType_TTF:{
			forI(text.count){
				u32       col = color.rgba;
				Vertex2*   vp = twodVertexArray + twodVertexCount;
				TwodIndexGl* ip = twodIndexArray + twodIndexCount;
				
				aligned_quad q = font->GetPackedQuad(text[i], &pos, scale);
				
				ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
				ip[3] = twodVertexCount; ip[4] = twodVertexCount + 2; ip[5] = twodVertexCount + 3;
				vp[0].pos = { q.x0,q.y0}; vp[0].uv = { q.s0,q.t0 }; vp[0].color = col;
				vp[1].pos = { q.x1,q.y0}; vp[1].uv = { q.s1,q.t0 }; vp[1].color = col;
				vp[2].pos = { q.x1,q.y1}; vp[2].uv = { q.s1,q.t1 }; vp[2].color = col;
				vp[3].pos = { q.x0,q.y1}; vp[3].uv = { q.s0,q.t1 }; vp[3].color = col;
				
				twodVertexCount += 4;
				twodIndexCount += 6;
				twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
				twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorExtent = scissorExtent;
				twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorOffset = scissorOffset;
			}break;
			default: Assert(!"unhandled font type"); break;
		}
	}
}

void Render::
DrawTexture2D(Texture* texture, vec2 p0, vec2 p1, vec2 p2, vec2 p3, f32 alpha, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0 && scissorExtent.x >= 0 && scissorExtent.y >= 0,
		   "Scissor Offset and Extent can't be negative");
	if(alpha == 0) return;
	CheckUICmdArrays(layer, texture, 1, scissorOffset, scissorExtent);
	
	u32         col = PackColorU32(255, 255, 255, 255.f * alpha);
	Vertex2*     vp = twodVertexArray + twodVertexCount;
	TwodIndexGl* ip = twodIndexArray + twodIndexCount;
	
	ip[0] = twodVertexCount; ip[1] = twodVertexCount + 1; ip[2] = twodVertexCount + 2;
	ip[3] = twodVertexCount; ip[4] = twodVertexCount + 2; ip[5] = twodVertexCount + 3;
	vp[0].pos = p0; vp[0].uv = { 0,1 }; vp[0].color = col;
	vp[1].pos = p1; vp[1].uv = { 1,1 }; vp[1].color = col;
	vp[2].pos = p2; vp[2].uv = { 1,0 }; vp[2].color = col;
	vp[3].pos = p3; vp[3].uv = { 0,0 }; vp[3].color = col;
	
	twodVertexCount += 4;
	twodIndexCount += 6;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += 6;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorExtent = scissorExtent;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].scissorOffset = scissorOffset;
}

void Render::
DrawTexture2D(Texture* texture, vec2 pos, vec2 size, f32 rotation, f32 alpha, u32 layer, vec2 scissorOffset, vec2 scissorExtent){
	vec2
		center = (pos + size) / 2,
	p0 = Math::vec2RotateByAngle(rotation, pos              - center) + center,
	p1 = Math::vec2RotateByAngle(rotation, pos.xAdd(size.x) - center) + center,
	p2 = Math::vec2RotateByAngle(rotation, pos + size       - center) + center,
	p3 = Math::vec2RotateByAngle(rotation, pos.yAdd(size.y) - center) + center;
	
	DrawTexture2D(texture, p0, p1, p2, p3, alpha, layer, scissorOffset, scissorExtent);
}

void Render::
StartNewTwodCmd(u32 layer, Texture* tex, vec2 scissorOffset, vec2 scissorExtent){
	twodCmdArrays[layer][twodCmdCounts[layer]].scissorOffset = scissorOffset;
	twodCmdArrays[layer][twodCmdCounts[layer]].scissorExtent = scissorExtent;
	twodCmdArrays[layer][twodCmdCounts[layer]].texIdx        = (tex) ? tex->idx : 0;
	twodCmdArrays[layer][twodCmdCounts[layer]].indexOffset   = twodIndexCount;
	twodCmdArrays[layer][twodCmdCounts[layer]].textured      = (tex) ? true : false;
	twodCmdCounts[layer]++;
}

void Render::
AddTwodVertices(u32 layer, Vertex2* vertstart, u32 vertcount, u32* indexstart, u32 indexcount){
	Assert(vertcount + twodVertexCount < MAX_TWOD_VERTICES);
	Assert(indexcount + twodIndexCount < MAX_TWOD_INDICES);
	
	Vertex2*     vp = twodVertexArray + twodVertexCount;
	TwodIndexGl* ip = twodIndexArray + twodIndexCount;
	
	memcpy(vp, vertstart, vertcount * sizeof(Vertex2));
	forI(indexcount) ip[i] = twodVertexCount + indexstart[i];
	
	twodVertexCount += vertcount;
	twodIndexCount += indexcount;
	twodCmdArrays[layer][twodCmdCounts[layer] - 1].indexCount += indexcount;
}


//-------------------------------------------------------------------------------------------------
// @INTERFACE FUNCTIONS
///////////////////
//// @settings ////
///////////////////
void Render::
SaveSettings(){
	Assets::saveConfig("render.cfg", configMap);
}

void Render::
LoadSettings(){
	Assets::loadConfig("render.cfg", configMap);
}

RenderSettings* Render::
GetSettings(){
	return &settings;
}

RenderStats* Render::
GetStats(){
	return &stats;
}

RendererStage* Render::
GetStage(){
	return &rendererStage;
}

///////////////
//// @load ////
///////////////
void Render::
LoadTexture(Texture* texture){
	TextureGl tgl{};
	tgl.base = texture;
	
	//determine image format
	switch(texture->format){ //TODO(delle) handle non RGBA formats
		case ImageFormat_BW:   tgl.format = GL_RGBA; break;
		case ImageFormat_BWA:  tgl.format = GL_RGBA; break;
		case ImageFormat_RGB:  tgl.format = GL_RGBA; break;
		case ImageFormat_RGBA: tgl.format = GL_RGBA; break;
		default: PrintGl(0,"Unhandled image format when loading texture: ", texture->name); return;
	}
	
	//determine image type
	switch(texture->type){
		case TextureType_1D:         tgl.type = GL_TEXTURE_1D; break;
		case TextureType_2D:         tgl.type = GL_TEXTURE_2D; break;
		case TextureType_3D:         tgl.type = GL_TEXTURE_3D; break;
		case TextureType_Cube:       tgl.type = GL_TEXTURE_CUBE_MAP; break;
		case TextureType_Array_1D:   tgl.type = GL_TEXTURE_1D_ARRAY; break;
		case TextureType_Array_2D:   tgl.type = GL_TEXTURE_2D_ARRAY; break;
#if GLAD_VERSION_MAJOR(opengl_version) >= 4
		case TextureType_Array_Cube: tgl.type = GL_TEXTURE_CUBE_MAP_ARRAY; break;
#else
		case TextureType_Array_Cube: Assert(!"GL_TEXTURE_CUBE_MAP_ARRAY requires OpenGL4"); break;
#endif
		default: PrintGl(0,"Uknown image type when loading texture: ", texture->name); return;
	}
	
	//create texture
	glGenTextures(1, &tgl.handle);
	glBindTexture(tgl.type, tgl.handle);
	
	//load texture to GPU
	if      (texture->type == TextureType_1D){
		glTexImage1D(tgl.type, 0, tgl.format, texture->width, 0, tgl.format, GL_UNSIGNED_BYTE, texture->pixels);
	}else if(texture->type == TextureType_2D || texture->type == TextureType_Array_1D){
		glTexImage2D(tgl.type, 0, tgl.format, texture->width, texture->height, 0, tgl.format, GL_UNSIGNED_BYTE, texture->pixels);
	}else if(texture->type == TextureType_Cube || texture->type == TextureType_Array_Cube || 
			 texture->type== TextureType_3D   || texture->type == TextureType_Array_2D){
		glTexImage3D(tgl.type, 0, tgl.format, texture->width, texture->height, texture->depth, 0, tgl.format, GL_UNSIGNED_BYTE, texture->pixels);
	}
	
	glTexParameteri(tgl.type, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	glTexParameteri(tgl.type, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//TODO(delle) EXT_texture_filter_anisotropic
	if(texture->mipmaps > 1){
		glTexParameteri(tgl.type, GL_TEXTURE_MIN_FILTER, (settings.textureFiltering) ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(tgl.type, GL_TEXTURE_MAG_FILTER, (settings.textureFiltering) ? GL_LINEAR : GL_NEAREST);
		glGenerateMipmap(tgl.type);
	}else{
		glTexParameteri(tgl.type, GL_TEXTURE_MIN_FILTER, (settings.textureFiltering) ? GL_LINEAR : GL_NEAREST);
		glTexParameteri(tgl.type, GL_TEXTURE_MAG_FILTER, (settings.textureFiltering) ? GL_LINEAR : GL_NEAREST);
	}
	
	glTextures.add(tgl);
}

void Render::
LoadMaterial(Material* material){ //!Incomplete
	
}

//TODO(delle) one large vertex/index array maybe
void Render::
LoadMesh(Mesh* mesh){
	MeshGl mgl{};
	mgl.base = mesh;
	mgl.vtxCount = mesh->vertexCount;
	mgl.idxCount = mesh->indexCount;
	if(glMeshes.count){
		mgl.vtxOffset = glMeshes.last->vtxOffset + glMeshes.last->vtxCount;
		mgl.idxOffset = glMeshes.last->idxOffset + glMeshes.last->idxCount;
	}
	
	u64 mesh_vb_size   = mesh->vertexCount*sizeof(Mesh::Vertex);
	u64 mesh_ib_size   = mesh->indexCount*sizeof(Mesh::Index);
	u64 total_vb_size  = meshBuffers.vbo_size + mesh_vb_size;
	u64 total_ib_size  = meshBuffers.ibo_size + mesh_ib_size;
	total_vb_size = Max(1024*sizeof(Mesh::Vertex), total_vb_size); //minimum of 1024 vertexes to avoid early growths
	total_ib_size = Max(4096*sizeof(Mesh::Index),  total_ib_size); //minimum of 4096 indexes to avoid early growths
	
	//create/bind vertex array object
	if(meshBuffers.vao_handle){
		glBindVertexArray(meshBuffers.vao_handle);
	}else{
		glGenVertexArrays(1, &meshBuffers.vao_handle);
		glBindVertexArray(meshBuffers.vao_handle);
	}
	
	//create/resize buffers: allocate new buffer, copy old buffer to new buffer, delete old buffer
	if(meshBuffers.vbo_alloc < total_vb_size){
		u32 old_vbo_handle = meshBuffers.vbo_handle;
		glGenBuffers(1, &meshBuffers.vbo_handle);
		glBindBuffer(GL_ARRAY_BUFFER, meshBuffers.vbo_handle);
		glBufferData(GL_ARRAY_BUFFER, total_vb_size, 0, GL_STATIC_DRAW); //TODO(delle) maybe growth rate/fit?
		if(old_vbo_handle){
			glBindBuffer(GL_COPY_READ_BUFFER, old_vbo_handle);
			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER, 0, 0, meshBuffers.vbo_size);
			glDeleteBuffers(1, &old_vbo_handle);
		}
	}
	if(meshBuffers.ibo_alloc < total_ib_size){
		u32 old_ibo_handle = meshBuffers.ibo_handle;
		glGenBuffers(1, &meshBuffers.ibo_handle);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffers.ibo_handle);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_ib_size, 0, GL_STATIC_DRAW); //TODO(delle) maybe growth rate/fit?
		if(old_ibo_handle){
			glBindBuffer(GL_COPY_READ_BUFFER, old_ibo_handle);
			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ELEMENT_ARRAY_BUFFER, 0, 0, meshBuffers.ibo_size);
			glDeleteBuffers(1, &old_ibo_handle);
		}
	}
	
	//copy mesh to buffers
	glBindBuffer(GL_ARRAY_BUFFER,         meshBuffers.vbo_handle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffers.ibo_handle);
	glBufferSubData(GL_ARRAY_BUFFER,         meshBuffers.vbo_size, mesh_vb_size, mesh->vertexArray);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, meshBuffers.ibo_size, mesh_ib_size, mesh->indexArray);
	
	//specify vertex packing
	glVertexAttribPointer(0, 3,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,pos));
	glVertexAttribPointer(1, 2,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,uv));
	glVertexAttribPointer(2, 4,  GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,color));
	glVertexAttribPointer(3, 3,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,normal));
	glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2); glEnableVertexAttribArray(3);
	
	//update buffer sizes
	meshBuffers.vbo_alloc = total_vb_size;
	meshBuffers.ibo_alloc = total_ib_size;
	meshBuffers.vbo_size = meshBuffers.vbo_size + mesh_vb_size;
	meshBuffers.ibo_size = meshBuffers.ibo_size + mesh_ib_size;
	
	glMeshes.add(mgl);
}

void Render::
UpdateMaterial(Material* material){
	Assert(!"not implemented yet"); //!Incomplete
}

/////////////////
//// @unload ////
/////////////////
void Render::
UnloadTexture(Texture* texture){
	Assert(!"not implemented yet"); //!Incomplete
}

void Render::
UnloadMaterial(Material* material){
	Assert(!"not implemented yet"); //!Incomplete
}

void Render::
UnloadMesh(Mesh* mesh){
	Assert(!"not implemented yet"); //!Incomplete
}

///////////////
//// @draw ////
///////////////
void Render::
DrawModel(Model* model, mat4 matrix){
	Assert(modelCmdCount + model->batches.count < MAX_MODEL_CMDS, "attempted to draw more than the global maximum number of batches");
	
	ModelCmdGl* cmd = modelCmdArray + modelCmdCount;
	forI(model->batches.count){
		if(!model->batches[i].indexCount) continue;
		cmd[i].vtxOffset = glMeshes[model->mesh->idx].vtxOffset;
		cmd[i].idxOffset = glMeshes[model->mesh->idx].idxOffset + model->batches[i].indexOffset;
		cmd[i].idxCount  = model->batches[i].indexCount;
		cmd[i].material  = model->batches[i].material;
		cmd[i].name      = model->name;
		cmd[i].matrix    = matrix;
		modelCmdCount += 1;
	}
}

void Render::
DrawModelWireframe(Model* model, mat4 matrix, color color){
	Assert(!"not implemented yet"); //!Incomplete
}

void Render::
DrawLine(vec3 start, vec3 end, color color){
	if(color.a == 0) return;
	
	u32 col = color.rgba;
	Mesh::Vertex* vp = tempWireframeVertexArray + tempWireframeVertexCount;
	TempIndexGl*  ip = tempWireframeIndexArray + tempWireframeIndexCount;
	
	ip[0] = tempWireframeVertexCount; 
	ip[1] = tempWireframeVertexCount+1; 
	ip[2] = tempWireframeVertexCount;
	vp[0].pos = start; vp[0].color = col;
	vp[1].pos = end;   vp[1].color = col;
	
	tempWireframeVertexCount += 2;
	tempWireframeIndexCount  += 3;
}

void Render::
DrawTriangle(vec3 p0, vec3 p1, vec3 p2, color color){
	if(color.a == 0) return;
	
	u32 col = color.rgba;
	Mesh::Vertex* vp = tempWireframeVertexArray + tempWireframeVertexCount;
	TempIndexGl*  ip = tempWireframeIndexArray + tempWireframeIndexCount;
	
	ip[0] = tempWireframeVertexCount; 
	ip[1] = tempWireframeVertexCount+1; 
	ip[2] = tempWireframeVertexCount+2;
	vp[0].pos = p0; vp[0].color = col;
	vp[1].pos = p1; vp[1].color = col;
	vp[2].pos = p2; vp[2].color = col;
	
	tempWireframeVertexCount += 3;
	tempWireframeIndexCount  += 3;
}

void Render::
DrawTriangleFilled(vec3 p0, vec3 p1, vec3 p2, color color){
	if(color.a == 0) return;
	
	u32 col = color.rgba;
	Mesh::Vertex* vp = tempFilledVertexArray + tempFilledVertexCount;
	TempIndexGl*  ip = tempFilledIndexArray + tempFilledIndexCount;
	
	ip[0] = tempFilledVertexCount; 
	ip[1] = tempFilledVertexCount+1; 
	ip[2] = tempFilledVertexCount+2;
	vp[0].pos = p0; vp[0].color = col;
	vp[1].pos = p1; vp[1].color = col;
	vp[2].pos = p2; vp[2].color = col;
	
	tempFilledVertexCount += 3;
	tempFilledIndexCount  += 3;
}

void Render::
DrawQuad(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color color){
	if(color.a == 0) return;
	DrawLine(p0, p1, color);
	DrawLine(p1, p2, color);
	DrawLine(p2, p3, color);
	DrawLine(p3, p0, color);
}

inline void Render::
DrawQuadFilled(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color color){
	if(color.a == 0) return;
	DrawTriangleFilled(p0, p1, p2, color);
	DrawTriangleFilled(p0, p2, p3, color);
}

void Render::
DrawPoly(array<vec3>& points, color color){
	Assert(points.count > 2);
	if(color.a == 0) return;
	for(s32 i=1; i<points.count-1; ++i) DrawLine(points[i-1], points[i], color);
	DrawLine(points[points.count-2], points[points.count-1], color);
}

void Render::
DrawPolyFilled(array<vec3>& points, color color){
	Assert(points.count > 2);
	if(color.a == 0) return;
	for(u32 i=2; i<points.count-1; ++i) DrawTriangleFilled(points[i-2], points[i-1], points[i], color);
	DrawTriangle(points[points.count-3], points[points.count-2], points[points.count-1], color);
}

void Render::
DrawBox(const mat4& transform, color color){
	if(color.a == 0) return;
	
	vec3 p(0.5f, 0.5f, 0.5f);
	vec3 points[8] = {
		{-p.x, p.y, p.z},{-p.x,-p.y, p.z},{-p.x, p.y,-p.z},{-p.x,-p.y,-p.z},
		{ p.x, p.y, p.z},{ p.x,-p.y, p.z},{ p.x, p.y,-p.z},{ p.x,-p.y,-p.z},
	};
	forI(8){ points[i] = points[i] * transform; }
	DrawLine(points[3], points[1], color); DrawLine(points[3], points[2], color); DrawLine(points[3], points[7], color);
	DrawLine(points[0], points[1], color); DrawLine(points[0], points[2], color); DrawLine(points[0], points[4], color);
	DrawLine(points[5], points[1], color); DrawLine(points[5], points[4], color); DrawLine(points[5], points[7], color);
	DrawLine(points[6], points[2], color); DrawLine(points[6], points[4], color); DrawLine(points[6], points[7], color);
}

void Render::
DrawBoxFilled(const mat4& transform, color color){
	if(color.a == 0) return;
	
	vec3 p(0.5f, 0.5f, 0.5f);
	vec3 points[8] = {
		{-p.x, p.y, p.z},{-p.x,-p.y, p.z},{-p.x, p.y,-p.z},{-p.x,-p.y,-p.z},
		{ p.x, p.y, p.z},{ p.x,-p.y, p.z},{ p.x, p.y,-p.z},{ p.x,-p.y,-p.z},
	};
	forI(8){ points[i] = points[i] * transform; }
	DrawTriangleFilled(points[4], points[2], points[0], color); DrawTriangleFilled(points[4], points[6], points[2], color);
	DrawTriangleFilled(points[2], points[7], points[3], color); DrawTriangleFilled(points[2], points[6], points[7], color);
	DrawTriangleFilled(points[6], points[5], points[7], color); DrawTriangleFilled(points[6], points[4], points[5], color);
	DrawTriangleFilled(points[1], points[7], points[5], color); DrawTriangleFilled(points[1], points[3], points[7], color);
	DrawTriangleFilled(points[0], points[3], points[1], color); DrawTriangleFilled(points[0], points[2], points[3], color);
	DrawTriangleFilled(points[4], points[1], points[5], color); DrawTriangleFilled(points[4], points[0], points[1], color);
}

void Render::
DrawCircle(vec3 position, vec3 rotation, f32 radius, u32 subdivisions_int, color c){
	mat4 transform = mat4::TransformationMatrix(position, rotation, vec3::ONE);
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (f32(i-1)*M_2PI) / subdivisions;
		f32 a1 = (f32(i  )*M_2PI) / subdivisions;
		f32 x0 = radius*cosf(a0); f32 x1 = radius*cosf(a1);
		f32 y0 = radius*sinf(a0); f32 y1 = radius*sinf(a1);
		vec3 xaxis0 = vec3{0, y0, x0} * transform; vec3 xaxis1 = vec3{0, y1, x1} * transform;
		DrawLine(xaxis0, xaxis1, c);
	}
}

void Render::
DrawSphere(vec3 position, vec3 rotation, f32 radius, u32 subdivisions_int, color cx, color cy, color cz){
	mat4 transform = mat4::TransformationMatrix(position, rotation, vec3::ONE);
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (f32(i-1)*M_2PI) / subdivisions;
		f32 a1 = (f32(i  )*M_2PI) / subdivisions;
		f32 x0 = radius*cosf(a0); f32 x1 = radius*cosf(a1);
		f32 y0 = radius*sinf(a0); f32 y1 = radius*sinf(a1);
		vec3 xaxis0 = vec3{0, y0, x0} * transform; vec3 xaxis1 = vec3{0, y1, x1} * transform;
		vec3 yaxis0 = vec3{x0, 0, y0} * transform; vec3 yaxis1 = vec3{x1, 0, y1} * transform;
		vec3 zaxis0 = vec3{x0, y0, 0} * transform; vec3 zaxis1 = vec3{x1, y1, 0} * transform;
		DrawLine(xaxis0, xaxis1, cx); DrawLine(yaxis0, yaxis1, cy); DrawLine(zaxis0, zaxis1, cz);
	}
}

void Render::
DrawFrustrum(vec3 position, vec3 target, f32 aspectRatio, f32 fovx, f32 nearZ, f32 farZ, color color){
	if(color.a == 0) return;
	
	f32 y = tanf(Radians(fovx/2.0f));
	f32 x = y*aspectRatio;
	f32 nearX = x*nearZ, farX = x*farZ;
	f32 nearY = y*nearZ, farY = y*farZ;
	vec4 faces[8] = {
		{nearX,nearY,nearZ,1}, {-nearX,nearY,nearZ,1}, {nearX,-nearY,nearZ,1}, {-nearX,-nearY,nearZ,1},
		{farX, farY, farZ, 1}, {-farX, farY, farZ, 1}, {farX, -farY, farZ, 1}, {-farX, -farY, farZ, 1},
	};
	
	mat4 mat = Math::LookAtMatrix(position, target);
	vec3 v[8];
	forI(8){ vec4 temp = faces[i]*mat; v[i].x = temp.x/temp.w; v[i].y = temp.y/temp.w; v[i].z = temp.z/temp.w; }
	
	DrawLine(v[0], v[1], color); DrawLine(v[0], v[2], color); DrawLine(v[3], v[1], color); DrawLine(v[3], v[2], color);
	DrawLine(v[4], v[5], color); DrawLine(v[4], v[6], color); DrawLine(v[7], v[5], color); DrawLine(v[7], v[6], color);
	DrawLine(v[0], v[4], color); DrawLine(v[1], v[5], color); DrawLine(v[2], v[6], color); DrawLine(v[3], v[7], color);
}

////////////////
//// @debug ////
////////////////
void Render::
ClearDebug(){
	debugWireframeVertexCount = 0;
	debugWireframeIndexCount  = 0;
	debugFilledVertexCount = 0;
	debugFilledIndexCount  = 0;
}

void Render::
DebugLine(vec3 start, vec3 end, color color){
	if(color.a == 0) return;
	
	u32 col = color.rgba;
	Mesh::Vertex* vp = debugWireframeVertexArray + debugWireframeVertexCount;
	TempIndexGl*  ip = debugWireframeIndexArray + debugWireframeIndexCount;
	
	ip[0] = debugWireframeVertexCount; 
	ip[1] = debugWireframeVertexCount+1; 
	ip[2] = debugWireframeVertexCount;
	vp[0].pos = start; vp[0].color = col;
	vp[1].pos = end;   vp[1].color = col;
	
	debugWireframeVertexCount += 2;
	debugWireframeIndexCount  += 3;
}

/////////////////
//// @camera ////
/////////////////
void Render::
UpdateCameraPosition(vec3 position){
	uboVS.values.viewPos = vec4(position, 1.f);
}

void Render::
UpdateCameraViewMatrix(mat4 m){
	uboVS.values.view = m;
}

void Render::
UpdateCameraProjectionMatrix(mat4 m){
	uboVS.values.proj = m;
	uboVS.values.proj.arr[5] = -1*uboVS.values.proj.arr[5]; //OpenGL is inverted
}

void Render::
UseDefaultViewProjMatrix(vec3 position, vec3 rotation){
	vec3 forward = (vec3::FORWARD * mat4::RotationMatrix(rotation)).normalized();
	uboVS.values.view = Math::LookAtMatrix(position, position + forward).Inverse();
	uboVS.values.proj = Camera::MakePerspectiveProjectionMatrix((f32)width, (f32)height, 90.f, 1000.f, 0.1f);
	uboVS.values.proj.arr[5] = -1*uboVS.values.proj.arr[5]; //OpenGL is inverted
}

//////////////////
//// @shaders ////
//////////////////
void Render::
ReloadShader(u32 shader){
	Assert(!"not implemented yet"); //!Incomplete
}

void Render::
ReloadAllShaders(){
	Assert(!"not implemented yet"); //!Incomplete
}

/////////////////
//// @remake ////
/////////////////
void Render::
UpdateLight(u32 lightIdx, vec4 vec){
	Assert(!"not implemented yet"); //!Incomplete
}

void Render::
remakeOffscreen(){
	Assert(!"not implemented yet"); //!Incomplete
}

void Render::
RemakeTextures(){
	Assert(!"not implemented yet"); //!Incomplete
}

///////////////
//// @init ////
///////////////
void Render::
Init(){
	AssertDS(DS_MEMORY, "Attempt to load OpenGL without loading Memory first");
	AssertDS(DS_WINDOW, "Attempt to load OpenGL without loading Window first");
	deshiStage |= DS_RENDER;
	
	TIMER_START(t_s);
	
	//// load RenderSettings ////
	LoadSettings();
	if(settings.debugging && settings.printf) settings.loggingLevel = 4;
	
	//// setup debug callback ////
	gladSetGLPostCallback(DebugPostCallback);
	
	//// initialization ////
	glfwGetFramebufferSize(DeshWindow->window, &width, &height);
	SetupUniformBuffers();
	SetupPrograms();
	
	//not sure the appropriate place for this
	forI(TWOD_LAYERS) twodCmdCounts[i] = 1;
	
	//glfwSwapInterval(1); //vsync
	initialized = true;
	
	Log("deshi","Finished opengl renderer initialization in ",TIMER_END(t_s),"ms");
}

/////////////////
//// @update ////
/////////////////
void Render::
Update(){
	TIMER_START(t_d);
	
	//// handle widow resize ////
	if(DeshWindow->resized) remake_window = true;
	if(remake_window){
		glfwGetFramebufferSize(DeshWindow->window, &width, &height);
		if(width <= 0 || height <= 0){ ImGui::EndFrame(); return; }
		glViewport(0,0,width,height);
		remake_window = false;
	}
	
	//// setup stuff ////
	UpdateUniformBuffers();
	SetupCommands();
	glClearColor(settings.clearColor.r, settings.clearColor.g, settings.clearColor.b, settings.clearColor.a);
	glClearDepth(1.0f); //1 rather than 0 because openGL Z direction is opposite ours
	glFrontFace(GL_CW);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glScissor(0, 0, width, height);
	
	//// render stuff ////
	if(DeshiModuleLoaded(DS_IMGUI)){
		ImGui::Render();
	}
	
	//// draw stuff ////
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	//draw meshes
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(glTextures[0].type, glTextures[0].handle);
	
	glBindVertexArray(meshBuffers.vao_handle);
	glUseProgram(programs.null.handle);
	glUniform1i(glGetUniformLocation(programs.null.handle,"nullSampler"),0);
	forI(modelCmdCount){ //TODO(delle) materials/textures
		glBindBuffer(GL_UNIFORM_BUFFER, pushVS.handle);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &modelCmdArray[i].matrix);
		glPolygonMode(GL_FRONT_AND_BACK, (settings.wireframeOnly) ? GL_LINE : GL_FILL);
		glDrawElementsBaseVertex(GL_TRIANGLES, modelCmdArray[i].idxCount, INDEX_TYPE_GL_MESH,
								 (void*)(modelCmdArray[i].idxOffset*sizeof(Mesh::Index)), modelCmdArray[i].vtxOffset);
	}
	
	//draw temp
	if(tempWireframeVertexCount > 0 && tempWireframeIndexCount > 0){
		glBindVertexArray(tempBuffers.vao_handle);
		glBindBuffer(GL_UNIFORM_BUFFER, pushVS.handle);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &mat4::IDENTITY);
		glDisable(GL_CULL_FACE);
		if(settings.tempMeshOnTop){
			glDisable(GL_DEPTH_TEST);
		}else{
			glEnable(GL_DEPTH_TEST);
		}
		
		//filled
		glUseProgram(programs.selected.handle);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawElementsBaseVertex(GL_TRIANGLES, tempFilledIndexCount, INDEX_TYPE_GL_TEMP, 0, 0);
		
		//wireframe
		glUseProgram(programs.wireframe.handle);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElementsBaseVertex(GL_TRIANGLES, tempWireframeIndexCount, INDEX_TYPE_GL_TEMP,
								 (void*)(tempFilledIndexCount*sizeof(TempIndexGl)), 0);
	}
	
	//draw ui
	if(twodVertexCount > 0 && twodIndexCount > 0){
		glBindVertexArray(uiBuffers.vao_handle);
		glBindBuffer(GL_UNIFORM_BUFFER, push2D.handle);
		push2D.values.scale     = {2.0f/f32(width), -2.0f/f32(height)};
		push2D.values.translate = {-1.f, 1.f};
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(push2D.values), &push2D.values);
		
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glFrontFace(GL_CCW);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glUseProgram(programs.ui.handle);
		
		//NOTE I don't think using a 2D array would be any different than having a separate for loop
		//     for each layer here, if its worse tell me and ill fix it.
		forX(layer, TWOD_LAYERS){
			forX(cmd_idx, twodCmdCounts[layer]){
				glScissor(GLint(twodCmdArrays[layer][cmd_idx].scissorOffset.x),
						  GLint((height - twodCmdArrays[layer][cmd_idx].scissorOffset.y) - twodCmdArrays[layer][cmd_idx].scissorExtent.y),
						  GLsizei(twodCmdArrays[layer][cmd_idx].scissorExtent.x),
						  GLsizei(twodCmdArrays[layer][cmd_idx].scissorExtent.y));
				glBindTexture(glTextures[twodCmdArrays[layer][cmd_idx].texIdx].type, glTextures[twodCmdArrays[layer][cmd_idx].texIdx].handle);
				glUniform1i(glGetUniformLocation(programs.ui.handle, "tex"), 0);
				glDrawElementsBaseVertex(GL_TRIANGLES, twodCmdArrays[layer][cmd_idx].indexCount, INDEX_TYPE_GL_TWOD,
										 (void*)(twodCmdArrays[layer][cmd_idx].indexOffset * sizeof(TwodIndexGl)), 0);
			}
		}
		
		glScissor(0, 0, width, height);
	}
	
	//draw imgui
	if(DeshiModuleLoaded(DS_IMGUI)){
		if(ImDrawData* imDrawData = ImGui::GetDrawData()) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
	}
	
	//// present stuff ////
	glfwSwapBuffers(DeshWindow->window);
	
	//// reset stuff ////
	ResetCommands();
	
	DeshTime->renderTime = TIMER_END(t_d);
}

////////////////
//// @reset ////
////////////////
void Render::
Reset(){
	PrintGl(1,"Resetting renderer");
	glFinish();
	//TODO(delle) delete things
}

//////////////////
//// @cleanup ////
//////////////////
void Render::
Cleanup(){
	Render::SaveSettings();
	//TODO(delle) save pipelines in GL4
	glFinish();
}
