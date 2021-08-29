//-------------------------------------------------------------------------------------------------
// @DIRECTX STRUCTS
struct ModelCmdDx{
	u32   vertexOffset;
	u32   indexOffset;
	u32   indexCount;
	u32   material;
	char* name;
	mat4  matrix;
};

struct UICmdDx{
	u32 texIdx;
	u16 indexOffset;
	u16 indexCount;
	vec2 scissorOffset;
	vec2 scissorExtent;
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
// @DIRECTX VARIABLES
//arbitray limits, change if needed
#define MAX_UI_VERTICES 0xFFFF //max u16: 65535
#define MAX_UI_INDICES  3*MAX_UI_VERTICES
#define MAX_UI_CMDS     1000
typedef u32 UIIndexDx; //if you change this make sure to change whats passed in the vkCmdBindIndexBuffer as well
local UIIndexDx uiVertexCount = 0;
local UIIndexDx uiIndexCount  = 0;
local UIIndexDx uiCmdCount    = 1; //start with 1
local Vertex2   uiVertexArray[MAX_UI_VERTICES];
local UIIndexDx uiIndexArray [MAX_UI_INDICES];
local UICmdDx   uiCmdArray   [MAX_UI_CMDS]; //different UI cmd per font/texture

#define MAX_TEMP_VERTICES 0xFFFF //max u16: 65535
#define MAX_TEMP_INDICES 3*MAX_TEMP_VERTICES
typedef u16 TempIndexDx;
local TempIndexDx  tempWireframeVertexCount = 0;
local TempIndexDx  tempFilledVertexCount    = 0;
local TempIndexDx  tempWireframeIndexCount  = 0;
local TempIndexDx  tempFilledIndexCount     = 0;
local Mesh::Vertex tempWireframeVertexArray[MAX_TEMP_VERTICES];
local Mesh::Vertex tempFilledVertexArray   [MAX_TEMP_VERTICES];
local TempIndexDx  tempWireframeIndexArray [MAX_TEMP_INDICES];
local TempIndexDx  tempFilledIndexArray    [MAX_TEMP_INDICES];

#define MAX_MODEL_CMDS 10000 
typedef u32 ModelIndexDx;
local ModelIndexDx modelCmdCount = 0;
local ModelCmdDx   modelCmdArray[MAX_MODEL_CMDS];

local s32  width  = 0;
local s32  height = 0;
local bool initialized  = false;
local bool remakeWindow = false;


//-------------------------------------------------------------------------------------------------
// @DIRECTX FUNCTIONS
template<typename... Args>
local inline void
PrintDx(u32 level, Args... args){
	if(settings.loggingLevel >= level){
		LOG("[DirectX] ", args...);
	}
}


//-------------------------------------------------------------------------------------------------
// @IMGUI FUNCTIONS
local char iniFilepath[256] = {};
void DeshiImGui::
Init(){ //!!Incomplete
    //Setup Dear ImGui context
    
    //Setup Dear ImGui style
    
    //Setup Platform/Renderer backends
    
}

void DeshiImGui::
Cleanup(){ //!!Incomplete
	
}

void DeshiImGui::
NewFrame(){ //!!Incomplete
	
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
	UIIndexDx* ip = uiIndexArray + uiIndexCount;
    
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
	UIIndexDx* ip = uiIndexArray + uiIndexCount;
    
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

void Render::
LoadMesh(Mesh* mesh){ //!!Incomplete
    
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
	TempIndexDx*  ip = tempWireframeIndexArray + tempWireframeIndexCount;
	
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
	TempIndexDx*  ip = tempWireframeIndexArray + tempWireframeIndexCount;
	
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
	TempIndexDx*  ip = tempFilledIndexArray + tempFilledIndexCount;
	
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
	
	f32 y = tanf(RADIANS(fovx / 2.0f));
	f32 x = y * aspectRatio;
	f32 nearX = x * nearZ;
	f32 farX  = x * farZ;
	f32 nearY = y * nearZ;
	f32 farY  = y * farZ;
	
	vec4 faces[8] = {
		//near face
		{ nearX,  nearY, nearZ, 1},
		{-nearX,  nearY, nearZ, 1},
		{ nearX, -nearY, nearZ, 1},
		{-nearX, -nearY, nearZ, 1},
		
		//far face
		{ farX,  farY, farZ, 1},
		{-farX,  farY, farZ, 1},
		{ farX, -farY, farZ, 1},
		{-farX, -farY, farZ, 1},
	};
	
	mat4 mat = Math::LookAtMatrix(position, target);
	vec3 v[8];
	forI(8){
		vec4 temp = faces[i] * mat;
		v[i].x = temp.x / temp.w;
		v[i].y = temp.y / temp.w;
		v[i].z = temp.z / temp.w;
	}
	
	DrawLine(v[0], v[1], color);
	DrawLine(v[0], v[2], color);
	DrawLine(v[3], v[1], color);
	DrawLine(v[3], v[2], color);
	DrawLine(v[4], v[5], color);
	DrawLine(v[4], v[6], color);
	DrawLine(v[7], v[5], color);
	DrawLine(v[7], v[6], color);
	DrawLine(v[0], v[4], color);
	DrawLine(v[1], v[5], color);
	DrawLine(v[2], v[6], color);
	DrawLine(v[3], v[7], color);
}

/////////////////
//// @camera ////
/////////////////
void Render::
UpdateCameraPosition(vec3 position){ //!!Incomplete
    
}

void Render::
UpdateCameraViewMatrix(mat4 m){ //!!Incomplete
    
}

void Render::
UpdateCameraProjectionMatrix(mat4 m){ //!!Incomplete
    
}

void Render::
UseDefaultViewProjMatrix(vec3 position, vec3 rotation){ //!!Incomplete
    
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
Init(){ //!!Incomplete
    //// load RenderSettings ////
	LoadSettings();
	if(settings.debugging && settings.printf) settings.loggingLevel = 4;
    
    //// setup debug callback ////
    
    //// temp testing ////
    
    initialized = true;
}

/////////////////
//// @update ////
/////////////////
void Render::
Update(){ //!!Incomplete
    TIMER_START(t_u);
    
    //handle window resize
    
    //render stuff
    
    //execute draw commands
    
    //present stuff
    
    //reset stuff
    
    
    DeshTime->renderTime = TIMER_END(t_u);
}

////////////////
//// @reset ////
////////////////
void Render::
Reset(){ //!!Incomplete
    
}

//////////////////
//// @cleanup ////
//////////////////
void Render::
Cleanup(){ //!!Incomplete
    
}
