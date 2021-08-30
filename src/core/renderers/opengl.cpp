//-------------------------------------------------------------------------------------------------
// @OPENGL STRUCTS
struct ModelCmdGl{
	u32   vertexOffset;
	u32   indexOffset;
	u32   indexCount;
	u32   material;
	char* name;
	mat4  matrix;
};

struct UICmdGl{
	u32 texIdx;
	u16 indexOffset;
	u16 indexCount;
	vec2 scissorOffset;
	vec2 scissorExtent;
};

struct MeshGl{
    Mesh* base;
    u32 vao_handle;
    u32 vbo_handle;
    u32 ebo_handle;
    u32 vertexCount;
    u32 indexCount;
};

struct ShaderGl{
    char filename[DESHI_NAME_SIZE];
    u32 handle;
    ShaderStage stage;
};

struct ProgramGl{
    u32 handle;
    u32 shader_count;
    ShaderGl* shaders[4];
};

//-------------------------------------------------------------------------------------------------
// @INTERFACE VARIABLES
local RenderSettings settings;
local ConfigMap configMap = {
	{"#render settings config file",0,0},
	
	{"\n#    //// REQUIRES RESTART ////",  ConfigValueType_PADSECTION,(void*)21},
	{"debugging",            ConfigValueType_Bool, &settings.debugging},
	{"printf",               ConfigValueType_Bool, &settings.printf},
	{"texture_filtering",    ConfigValueType_Bool, &settings.textureFiltering},
	{"anistropic_filtering", ConfigValueType_Bool, &settings.anistropicFiltering},
	{"msaa_level",           ConfigValueType_U32,  &settings.msaaSamples},
	{"recompile_all_shaders",        ConfigValueType_Bool, &settings.recompileAllShaders},
	
	{"\n#    //// RUNTIME VARIABLES ////", ConfigValueType_PADSECTION,(void*)15},
	{"logging_level",  ConfigValueType_U32,  &settings.loggingLevel},
	{"crash_on_error", ConfigValueType_Bool, &settings.crashOnError},
	{"vsync_type",     ConfigValueType_U32,  &settings.vsync},
	
	{"\n#shaders",                         ConfigValueType_PADSECTION,(void*)17},
	{"optimize_shaders", ConfigValueType_Bool, &settings.optimizeShaders},
	
	{"\n#shadows",                         ConfigValueType_PADSECTION,(void*)20},
	{"shadow_pcf",          ConfigValueType_Bool, &settings.shadowPCF},
	{"shadow_resolution",   ConfigValueType_U32,  &settings.shadowResolution},
	{"shadow_nearz",        ConfigValueType_F32,  &settings.shadowNearZ},
	{"shadow_farz",         ConfigValueType_F32,  &settings.shadowFarZ},
	{"depth_bias_constant", ConfigValueType_F32,  &settings.depthBiasConstant},
	{"depth_bias_slope",    ConfigValueType_F32,  &settings.depthBiasSlope},
	{"show_shadow_map",     ConfigValueType_Bool, &settings.showShadowMap},
	
	{"\n#colors",                          ConfigValueType_PADSECTION,(void*)15},
	{"clear_color",    ConfigValueType_FV4, &settings.clearColor},
	{"selected_color", ConfigValueType_FV4, &settings.selectedColor},
	{"collider_color", ConfigValueType_FV4, &settings.colliderColor},
	
	{"\n#filters",                         ConfigValueType_PADSECTION,(void*)15},
	{"wireframe_only", ConfigValueType_Bool, &settings.wireframeOnly},
	
	{"\n#overlays",                        ConfigValueType_PADSECTION,(void*)17},
	{"mesh_wireframes",  ConfigValueType_Bool, &settings.meshWireframes},
	{"mesh_normals",     ConfigValueType_Bool, &settings.meshNormals},
	{"light_frustrums",  ConfigValueType_Bool, &settings.lightFrustrums},
	{"temp_mesh_on_top", ConfigValueType_Bool, &settings.tempMeshOnTop},
};

local RenderStats   stats{};
local RendererStage rendererStage = RENDERERSTAGE_NONE;


//-------------------------------------------------------------------------------------------------
// @OPENGL VARIABLES
local array<MeshGl>    glMeshes;
local array<ShaderGl>  glShaders;
local array<ProgramGl> glPrograms;

///////////////////
//// @commands ////
///////////////////
//arbitray limits, change if needed
#define MAX_UI_VERTICES 0xFFFF //max u16: 65535
#define MAX_UI_INDICES  3*MAX_UI_VERTICES
#define MAX_UI_CMDS     1000
typedef u32 UIIndexGl; //if you change this make sure to change whats passed in the vkCmdBindIndexBuffer as well
local UIIndexGl uiVertexCount = 0;
local UIIndexGl uiIndexCount  = 0;
local UIIndexGl uiCmdCount    = 1; //start with 1
local Vertex2   uiVertexArray[MAX_UI_VERTICES];
local UIIndexGl uiIndexArray [MAX_UI_INDICES];
local UICmdGl   uiCmdArray   [MAX_UI_CMDS]; //different UI cmd per font/texture

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
local int  opengl_success = 0;
#define OPENGL_INFOLOG_SIZE 512
local char opengl_infolog[OPENGL_INFOLOG_SIZE] = {};

//////////////////
//// @buffers ////
//////////////////
local struct{ //uniform buffer for the vertex shaders
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
		int  enablePCF;   //whether to blur shadow edges //TODO(delle,ReVu) convert to specialization constant
        int  padding[3];
	} values;
} uboVS{};

local struct{ //uniform buffer for the vertex shaders
	u32 handle;
    u32 binding = 2;
	
	struct{ //64 bytes
		mat4 matrix;
	} values;
} pushVS{};

//-------------------------------------------------------------------------------------------------
// @OPENGL FUNCTIONS
////////////////////
//// @utilities ////
////////////////////
template<typename... Args>
local inline void
PrintGl(u32 level, Args... args){
	if(settings.loggingLevel >= level){
		LOG("[OpenGL] ", args...);
	}
}

local void 
DebugPostCallback(void *ret, const char *name, GLADapiproc apiproc, int len_args, ...){
    GLenum error_code = glad_glGetError();
    if(error_code != GL_NO_ERROR){
        const char* error_flag;
        switch(error_code){
            case GL_INVALID_ENUM:                  error_flag = "GL_INVALID_ENUM"; break; //Set when an enumeration parameter is not legal.
            case GL_INVALID_VALUE:                 error_flag = "GL_INVALID_VALUE"; break; //Set when a value parameter is not legal.
            case GL_INVALID_OPERATION:             error_flag = "GL_INVALID_OPERATION"; break; //Set when the state for a command is not legal for its given parameters.
            case 1283:                             error_flag = "GL_STACK_OVERFLOW"; break; //Set when a stack pushing operation causes a stack overflow.
            case 1284:                             error_flag = "GL_STACK_UNDERFLOW"; break; //Set when a stack popping operation occurs while the stack is at its lowest point.
            case GL_OUT_OF_MEMORY:                 error_flag = "GL_OUT_OF_MEMORY"; break; //	Set when a memory allocation operation cannot allocate (enough) memory.
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
ResetCommands(){
	{//UI commands
		uiVertexCount = 0;
		uiIndexCount  = 0;
		memset(&uiCmdArray[0], 0, sizeof(UICmdGl)*uiCmdCount);
		uiCmdCount    = 1;
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
local void
CompileAndLoadShader(const char* filename, ShaderStage stage){
    Assert(stage > ShaderStage_NONE && stage < ShaderStage_COUNT);
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
        PrintGl(2, "  Failed to compile shader '",filename,"':\n",opengl_infolog);
        
        //delete broken shader
        glDeleteShader(sgl.handle);
        return;
    }
    
    glShaders.add(sgl);
}


///////////////////
//// @programs ////
///////////////////
//TODO(delle) cleanup shaders maybe? glDeleteShader()
local void 
CreateProgram(ShaderGl* shaders, u32 shader_count){
    //TODO(delle) Assert(ubo's setup)
    
    ProgramGl pgl{};
    pgl.shader_count = shader_count;
    
    //create program, attach shaders and link
    pgl.handle = glCreateProgram();
    forI(shader_count){ glAttachShader(pgl.handle, shaders[i].handle); pgl.shaders[i] = &shaders[i]; }
    glLinkProgram(pgl.handle);
    
    //check for errors
    glGetProgramiv(pgl.handle, GL_LINK_STATUS, &opengl_success);
    if(opengl_success != GL_TRUE){
        glGetProgramInfoLog(pgl.handle, OPENGL_INFOLOG_SIZE, 0, opengl_infolog);
        PrintGl(0, "Failed to link program '",pgl.handle,"':\n",opengl_infolog);
        
        //delete broken program
        glDeleteProgram(pgl.handle);
        return;
    }
    
    //specify UBOs
    u32 local_ubi = glGetUniformBlockIndex(pgl.handle,"UniformBufferObject");
    if(local_ubi != -1){ 
        glUniformBlockBinding(pgl.handle, local_ubi, uboVS.binding);
    }else{
        PrintGl(0, "Failed to find UniformBufferObject in vertex shader of program '",pgl.handle,"'");
        if(settings.crashOnError) Assert(false);
        glDeleteProgram(pgl.handle);
        return; 
    }
    
    local_ubi = glGetUniformBlockIndex(pgl.handle,"PushConsts");
    if(local_ubi != -1){ 
        glUniformBlockBinding(pgl.handle, local_ubi, pushVS.binding);
    }else{
        PrintGl(0, "Failed to find PushConsts in vertex shader of program '",pgl.handle,"'");
        if(settings.crashOnError) Assert(false);
        glDeleteProgram(pgl.handle);
        return;
    }
    
    //detach shaders
    forI(shader_count){ glDetachShader(pgl.handle, shaders[i].handle); }
    
    glPrograms.add(pgl);
}

//@@
local void
SetupPrograms(){
    //collect shaders files
    array<string> files;
	for(auto& entry : std::filesystem::directory_iterator(Assets::dirShaders())){
		if(entry.path().extension() == ".vert" ||
		   entry.path().extension() == ".frag" ||
		   entry.path().extension() == ".geom" ||
           entry.path().extension() == ".tesc" ||
           entry.path().extension() == ".tese" ||
           entry.path().extension() == ".comp"){
			files.add(entry.path().filename().string().c_str());
		}
	}
    
    //!!Incomplete
}


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
}

local void
UpdateUniformBuffers(){
    uboVS.values.screen     = vec2(width, height);
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
// @UI INTERFACE
enum texTypes : u32 {
	UITEX_WHITE,
	UITEX_FONT
};

//TODO(sushi) find a nicer way to keep track of this
//NOTE im not sure yet if i should be keeping track of this for each primitive or not yet but i dont think i have to
vec2 prevScissorOffset = vec2( 0,  0);
vec2 prevScissorExtent = vec2(-1, -1);

void Render::FillRectUI(vec2 pos, vec2 dimensions, color color, vec2 scissorOffset, vec2 scissorExtent){
    Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0, "Scissor Offset can't be negative");
	if (color.a == 0) return;
    
	if(uiCmdArray[uiCmdCount - 1].texIdx != UITEX_WHITE ||
       scissorOffset != prevScissorOffset || //im doing these 2 because we have to know if we're drawing in a new window
       scissorExtent != prevScissorExtent){  //and you could do text last in one, and text first in another
		prevScissorExtent = scissorExtent;
		prevScissorOffset = scissorOffset;
		uiCmdArray[uiCmdCount].indexOffset = uiIndexCount;
		uiCmdCount++;
	}
    
	u32       col = color.rgba;
	Vertex2*   vp = uiVertexArray + uiVertexCount;
	UIIndexGl* ip = uiIndexArray + uiIndexCount;
    
	ip[0] = uiVertexCount; ip[1] = uiVertexCount + 1; ip[2] = uiVertexCount + 2;
	ip[3] = uiVertexCount; ip[4] = uiVertexCount + 2; ip[5] = uiVertexCount + 3;
	vp[0].pos = { pos.x + 0,           pos.y + 0 };            vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = { pos.x + dimensions.w,pos.y + 0 };            vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = { pos.x + dimensions.w,pos.y + dimensions.h }; vp[2].uv = { 0,0 }; vp[2].color = col;
	vp[3].pos = { pos.x + 0,           pos.y + dimensions.h }; vp[3].uv = { 0,0 }; vp[3].color = col;
    
	uiVertexCount += 4;
	uiIndexCount += 6;
	uiCmdArray[uiCmdCount - 1].indexCount += 6;
	uiCmdArray[uiCmdCount - 1].texIdx = UITEX_WHITE;
	if(scissorExtent.x != -1){
		uiCmdArray[uiCmdCount - 1].scissorExtent = scissorExtent;
		uiCmdArray[uiCmdCount - 1].scissorOffset = scissorOffset;
	}else{
		uiCmdArray[uiCmdCount - 1].scissorExtent = vec2(width, height);
		uiCmdArray[uiCmdCount - 1].scissorOffset = vec2(0, 0);
	}
}

//this func is kind of scuffed i think because of the line thickness stuff when trying to draw
//straight lines, see below
void Render::DrawRectUI(vec2 pos, vec2 dimensions, color color, vec2 scissorOffset, vec2 scissorExtent){
    Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0, "Scissor Offset can't be negative");
	if (color.a == 0) return;
	
	//top, left, right, bottom
	DrawLineUI(pos.xAdd(-1),     pos + dimensions.ySet(0),          1, color, scissorOffset, scissorExtent);
	DrawLineUI(pos,              pos + dimensions.xSet(0),          1, color, scissorOffset, scissorExtent);
	DrawLineUI(pos + dimensions, pos + dimensions.ySet(0),          1, color, scissorOffset, scissorExtent);
	DrawLineUI(pos + dimensions, pos + dimensions.xSet(0).xAdd(-1), 1, color, scissorOffset, scissorExtent);
}

//TODO(sushi) implement special line drawing for straight lines, since we dont need to do the normal thing
//when drawing them straight
void Render::DrawLineUI(vec2 start, vec2 end, float thickness, color color, vec2 scissorOffset, vec2 scissorExtent){
    Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0, "Scissor Offset can't be negative");
	if(color.a == 0) return;
    
	if(uiCmdArray[uiCmdCount - 1].texIdx != UITEX_WHITE ||
       scissorOffset != prevScissorOffset || //im doing these 2 because we have to know if we're drawing in a new window
       scissorExtent != prevScissorExtent){  //and you could do text last in one, and text first in another
		prevScissorExtent = scissorExtent;
		prevScissorOffset = scissorOffset;
		uiCmdArray[uiCmdCount].indexOffset = uiIndexCount;
		uiCmdCount++;
	}
    
	u32       col = color.rgba;
	Vertex2*   vp = uiVertexArray + uiVertexCount;
	UIIndexGl* ip = uiIndexArray + uiIndexCount;
    
	vec2 ott = end - start;
	vec2 norm = vec2(ott.y, -ott.x).normalized();
    
	ip[0] = uiVertexCount; ip[1] = uiVertexCount + 1; ip[2] = uiVertexCount + 2;
	ip[3] = uiVertexCount; ip[4] = uiVertexCount + 2; ip[5] = uiVertexCount + 3;
	vp[0].pos = { start.x,start.y }; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = { end.x,  end.y };   vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = { end.x,  end.y };   vp[2].uv = { 0,0 }; vp[2].color = col;
	vp[3].pos = { start.x,start.y }; vp[3].uv = { 0,0 }; vp[3].color = col;
    
	vp[0].pos += norm * thickness / 2;
	vp[1].pos += norm * thickness / 2;
	vp[2].pos -= norm * thickness / 2;
	vp[3].pos -= norm * thickness / 2;
    
	uiVertexCount += 4;
	uiIndexCount += 6;
	uiCmdArray[uiCmdCount - 1].indexCount += 6;
	uiCmdArray[uiCmdCount - 1].texIdx = UITEX_WHITE;
	if(scissorExtent.x != -1){
		uiCmdArray[uiCmdCount - 1].scissorExtent = scissorExtent;
		uiCmdArray[uiCmdCount - 1].scissorOffset = scissorOffset;
	}else{
		uiCmdArray[uiCmdCount - 1].scissorExtent = vec2(width, height);
		uiCmdArray[uiCmdCount - 1].scissorOffset = vec2(0, 0);
	}
}

void Render::
DrawTextUI(string text, vec2 pos, color color, vec2 scissorOffset, vec2 scissorExtent){ //!!Incomplete
    Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0, "Scissor Offset can't be negative");
	if (color.a == 0) return;
	
	
}

//NOTE: text scaling looks very ugly with bit map fonts as far as i know
void Render::
DrawCharUI(u32 character, vec2 pos, vec2 scale, color color, vec2 scissorOffset, vec2 scissorExtent){ //!!Incomplete
    Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0, "Scissor Offset can't be negative");
	if(color.a == 0) return;
	
	
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
LoadFont(Font* font, Texture* texture){ //!!Incomplete
	
}

void Render::
LoadTexture(Texture* texture){ //!!Incomplete
	
}

void Render::
LoadMaterial(Material* material){ //!!Incomplete
	
}

//TODO(delle) one large vertex/index array maybe
void Render::
LoadMesh(Mesh* mesh){
    MeshGl mgl{};
    mgl.base = mesh;
    mgl.vertexCount = mesh->vertexCount;
    mgl.indexCount = mesh->indexCount;
    
    //allocate buffers
    glGenVertexArrays(1, &mgl.vao_handle);
    glGenBuffers(1, &mgl.vbo_handle);
    glGenBuffers(1, &mgl.ebo_handle);
    
    //bind and fill buffers
    glBindVertexArray(mgl.vao_handle);
    glBindBuffer(GL_ARRAY_BUFFER, mgl.vbo_handle);
    glBufferData(GL_ARRAY_BUFFER, mgl.vertexCount*sizeof(Mesh::Vertex), mesh->vertexArray, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mgl.ebo_handle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mgl.indexCount*sizeof(Mesh::Index), mesh->indexArray, GL_STATIC_DRAW);
    
    //sepcify how to read vertex buffer
    glVertexAttribPointer(0, 3,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,pos));
    glVertexAttribPointer(1, 2,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,uv));
    glVertexAttribPointer(2, 4,  GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,color));
    glVertexAttribPointer(3, 3,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,normal));
    glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2); glEnableVertexAttribArray(3);
    
    glMeshes.add(mgl);
}

void Render::
UpdateMaterial(Material* material){ //!!Incomplete
	
}

/////////////////
//// @unload ////
/////////////////
void Render::
UnloadFont(Font* font){ //!!Incomplete
    
}

void Render::
UnloadTexture(Texture* texture){ //!!Incomplete
    
}

void Render::
UnloadMaterial(Material* material){ //!!Incomplete
    
}

void Render::
UnloadMesh(Mesh* mesh){ //!!Incomplete
    
}

///////////////
//// @draw ////
///////////////
void Render::
DrawModel(Model* mesh, mat4 matrix){ //!!Incomplete
    
}

void Render::
DrawModelWireframe(Model* mesh, mat4 matrix, color color){ //!!Incomplete
	
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
	for(int i=1; i<points.count-1; ++i) DrawLine(points[i-1], points[i], color);
	DrawLine(points[points.count-2], points[points.count-1], color);
}

void Render::
DrawPolyFilled(array<vec3>& points, color color){
	Assert(points.count > 2);
	if(color.a == 0) return;
	for(int i=2; i<points.count-1; ++i) DrawTriangleFilled(points[i-2], points[i-1], points[i], color);
	DrawTriangle(points[points.count-3], points[points.count-2], points[points.count-1], color);
}

void Render::
DrawBox(mat4 transform, color color){
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
DrawBoxFilled(mat4 transform, color color){
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
DrawFrustrum(vec3 position, vec3 target, f32 aspectRatio, f32 fovx, f32 nearZ, f32 farZ, color color){
	if(color.a == 0) return;
	
	f32 y = tanf(RADIANS(fovx/2.0f));
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
    uboVS.values.proj.data[5] = -1*uboVS.values.proj.data[5]; //OpenGL is inverted
}

void Render::
UseDefaultViewProjMatrix(vec3 position, vec3 rotation){
    vec3 forward = (vec3::FORWARD * mat4::RotationMatrix(rotation)).normalized();
	uboVS.values.view = Math::LookAtMatrix(position, position + forward).Inverse();
	uboVS.values.proj = Camera::MakePerspectiveProjectionMatrix(width, height, 90, 1000, 0.1);
    uboVS.values.proj.data[5] = -1*uboVS.values.proj.data[5]; //OpenGL is inverted
}

//////////////////
//// @shaders ////
//////////////////
void Render::
ReloadShader(u32 shader){ //!!Incomplete
    
}

void Render::
ReloadAllShaders(){ //!!Incomplete
    
}

/////////////////
//// @remake ////
/////////////////
void Render::
UpdateLight(u32 lightIdx, vec4 vec){ //!!Incomplete
    
}

void Render::
remakeOffscreen(){ //!!Incomplete
    
}

void Render::
RemakeTextures(){ //!!Incomplete
    
}

///////////////
//// @init ////
///////////////
void Render::
Init(){
    //// load RenderSettings ////
	LoadSettings();
	if(settings.debugging && settings.printf) settings.loggingLevel = 4;
    
    //// setup debug callback ////
    gladSetGLPostCallback(DebugPostCallback);
    
    //// initialization ////
    remake_window = true;
    SetupUniformBuffers();
    glFrontFace(GL_CW);
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    //glEnable(GL_DEPTH_TEST);
    
    //// temp testing ////
    CompileAndLoadShader("base.vert", ShaderStage_Vertex);
    CompileAndLoadShader("base.frag", ShaderStage_Fragment);
    CreateProgram(glShaders.data, 2);
    LoadMesh(Storage::CreateBoxMesh(.5f, .5f, .5f, Color_Cyan).second);
    
    //glfwSwapInterval(1); //vsync
    initialized = true;
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
		int w, h; glfwGetFramebufferSize(DeshWindow->window, &w, &h);
		if(w <= 0 || h <= 0){ ImGui::EndFrame(); return; }
        glViewport(0,0,w,h);
        width = w; height = h;
		remake_window = false;
	}
    
    //// setup stuff ////
    UpdateUniformBuffers();
    
    //// render stuff ////
    ImGui::Render();
    
    //// draw stuff ////
    glClearColor(settings.clearColor.r, settings.clearColor.g, settings.clearColor.b, settings.clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);
    
    {//temp testing
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUseProgram(glPrograms[0].handle);
        glBindVertexArray(glMeshes[0].vao_handle);
        persist mat4 temp_mat = mat4::IDENTITY;
        glBindBuffer(GL_UNIFORM_BUFFER, pushVS.handle);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4), &temp_mat, GL_DYNAMIC_DRAW);
        glDrawElements(GL_TRIANGLES, glMeshes[0].indexCount, GL_UNSIGNED_INT, 0);
    }
    
    //draw meshes //!!Incomplete
    //glBindVertexArray(meshVertexArray);
    forI(modelCmdCount){ //TODO(delle) materials/textures
        glBindBuffer(GL_UNIFORM_BUFFER, pushVS.handle);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4), &modelCmdArray[i].matrix, GL_DYNAMIC_DRAW);
        glPolygonMode(GL_FRONT_AND_BACK, (settings.wireframeOnly) ? GL_LINE : GL_FILL);
        glDrawElementsBaseVertex(GL_TRIANGLES, modelCmdArray[i].indexCount, GL_UNSIGNED_INT,
                                 (void*)(modelCmdArray[i].indexOffset*sizeof(Mesh::Index)), modelCmdArray[i].vertexOffset);
    }
    
    //draw ui
    
    //draw imgui
    if(ImDrawData* imDrawData = ImGui::GetDrawData()) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
    
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
Reset(){ //!!Incomplete
    glFinish();
    
}

//////////////////
//// @cleanup ////
//////////////////
void Render::
Cleanup(){ //!!Incomplete
    glFinish();
    
}
