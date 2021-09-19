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

//the following are variables that set DirectX apart from the other renderers

//window handle
local HWND windowHandle;
//window rectanglee (int the tut it says this is used to toggle fullscreen state so it may not be necessary)
local RECT windowRect;

//DX12 objects
// maybe remove ComPtr later >:(

using namespace Microsoft::WRL;

#define NumFrames 3 // the number of back buffer surfaces for the swap chain
local ComPtr<ID3D12Device2>             device;
local ComPtr<ID3D12GraphicsCommandList> commandList;
local ComPtr<ID3D12CommandQueue>        commandQueue;
local ComPtr<ID3D12CommandAllocator>    commandAllocators[NumFrames]; 
local ComPtr<IDXGISwapChain4>           swapchain;
local ComPtr<ID3D12Resource>            backBuffers[NumFrames];       
local ComPtr<ID3D12DescriptorHeap>      rtvDescriptorHeap;
local u32                               rtvDescriptorSize;
local u32                               currentBackBufferIdx;

//synchronization
local ComPtr<ID3D12Fence> fence;
local u64                 fenceValue;
local u64                 frameFenceValues[NumFrames]{};
local HANDLE              fenceEvent;

local bool tearingSupported;

//-------------------------------------------------------------------------------------------------
// @DIRECTX FUNCTIONS
#define AssertDx(assertee, message) if(FAILED(assertee)){ *(volatile int*)0 = 0; }


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
Init(){ //!Incomplete
	//Setup Dear ImGui context
	
	//Setup Dear ImGui style
	
	//Setup Platform/Renderer backends
	
	
	
}

void DeshiImGui::
Cleanup(){ //!Incomplete
	
}

void DeshiImGui::
NewFrame(){ //!Incomplete
	
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
DrawTextUI(string text, vec2 pos, color color, vec2 scissorOffset, vec2 scissorExtent){ //!Incomplete
	Assert(scissorOffset.x >= 0 && scissorOffset.y >= 0, "Scissor Offset can't be negative");
	if (color.a == 0) return;
	
	
}

//NOTE: text scaling looks very ugly with bit map fonts as far as i know
void Render::
DrawCharUI(u32 character, vec2 pos, vec2 scale, color color, vec2 scissorOffset, vec2 scissorExtent){ //!Incomplete
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
LoadFont(Font* font, Texture* texture){ //!Incomplete
	
}

void Render::
LoadTexture(Texture* texture){ //!Incomplete
	
}

void Render::
LoadMaterial(Material* material){ //!Incomplete
	
}

void Render::
LoadMesh(Mesh* mesh){ //!Incomplete
	
}

void Render::
UpdateMaterial(Material* material){ //!Incomplete
	
}

/////////////////
//// @unload ////
/////////////////
void Render::
UnloadFont(Font* font){ //!Incomplete
	
}

void Render::
UnloadTexture(Texture* texture){ //!Incomplete
	
}

void Render::
UnloadMaterial(Material* material){ //!Incomplete
	
}

void Render::
UnloadMesh(Mesh* mesh){ //!Incomplete
	
}

///////////////
//// @draw ////
///////////////
void Render::
DrawModel(Model* mesh, mat4 matrix){ //!Incomplete
	
}

void Render::
DrawModelWireframe(Model* mesh, mat4 matrix, color color){ //!Incomplete
	
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
UpdateCameraPosition(vec3 position){ //!Incomplete
	
}

void Render::
UpdateCameraViewMatrix(mat4 m){ //!Incomplete
	
}

void Render::
UpdateCameraProjectionMatrix(mat4 m){ //!Incomplete
	
}

void Render::
UseDefaultViewProjMatrix(vec3 position, vec3 rotation){ //!Incomplete
	
}

//////////////////
//// @shaders ////
//////////////////
void Render::
ReloadShader(u32 shader){ //!Incomplete
	
}

void Render::
ReloadAllShaders(){ //!Incomplete
	
}

/////////////////
//// @remake ////
/////////////////
void Render::
UpdateLight(u32 lightIdx, vec4 vec){ //!Incomplete
	
}

void Render::
remakeOffscreen(){ //!Incomplete
	
}

void Render::
RemakeTextures(){ //!Incomplete
	
}

///////////////
//// @init ////
///////////////


//Init's functions
local ComPtr<IDXGIAdapter4> GetAdapter() {
	//get a compatible adapter for use with DirectX12
	
	IDXGIFactory4* dxgiFactory;
	u32 flags = 0;
#ifdef DESHI_INTERNAL
	flags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	AssertDx(CreateDXGIFactory2(flags, IID_PPV_ARGS(&dxgiFactory)));
	
	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;
	
	SIZE_T maxDedicatedVideoMemory = 0;
	for (u32 i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
		dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);
		
		// Check to see if the adapter can create a D3D12 device without actually 
		// creating it. The adapter with the largest dedicated video memory
		// is favored.
		if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
			SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
										D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
			dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
			AssertDx(dxgiAdapter1.As(&dxgiAdapter4));
			
		}
	}
	
	return dxgiAdapter4;
}

local ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter){
	ComPtr<ID3D12Device> d3d12Device2;
	AssertDx(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));
	
	
	//if were in debug, enable debug messages
#ifdef DESHI_INTERNAL
	ComPtr<ID3D12InfoQueue> infoQueue;
	if (SUCCEEDED(d3d12Device2.As(&infoQueue))) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
	}
	
	// Suppress whole categories of messages
	//D3D12_MESSAGE_CATEGORY Categories[] = {};
	
	// Suppress messages based on their severity level
	D3D12_MESSAGE_SEVERITY Severities[] = {
		D3D12_MESSAGE_SEVERITY_INFO
	};
	
	// Suppress individual messages by their ID
	D3D12_MESSAGE_ID DenyIds[] = {
		D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
		D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
		D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
	};
	
	D3D12_INFO_QUEUE_FILTER NewFilter = {};
	//NewFilter.DenyList.NumCategories = _countof(Categories);
	//NewFilter.DenyList.pCategoryList = Categories;
	NewFilter.DenyList.NumSeverities = _countof(Severities);
	NewFilter.DenyList.pSeverityList = Severities;
	NewFilter.DenyList.NumIDs = _countof(DenyIds);
	NewFilter.DenyList.pIDList = DenyIds;
	
	AssertDx(infoQueue->PushStorageFilter(&NewFilter));
	
#endif
	
}

local ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) {
	ComPtr<ID3D12CommandQueue> d3d12CommandQueue;
	
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;
	
	AssertDx(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));
	
	return d3d12CommandQueue;
}

local bool CheckTearingSupport(){
	BOOL allowTearing = FALSE;
	
	// Rather than create the DXGI 1.5 factory interface directly, we create the
	// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
	// graphics debugging tools which will not support the 1.5 factory interface 
	// until a future update.
	ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4)))){
		ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5))){
			if (FAILED(factory5->CheckFeatureSupport(
													 DXGI_FEATURE_PRESENT_ALLOW_TEARING,
													 &allowTearing, sizeof(allowTearing)))){
				allowTearing = FALSE;
			}
		}
	}
	
	
	
	return allowTearing == TRUE;
}

local ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, u32 width, u32 height, u32 bufferCount) {
	ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	u32 createFactoryFlags = 0;
	
#ifdef DESHI_INTERNAL
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	
	AssertDx(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));
	
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.      Width = width;
	swapChainDesc.     Height = height;
	swapChainDesc.     Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.     Stereo = FALSE;
	swapChainDesc. SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.    Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc. SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.  AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// It is recommended to always allow tearing if tearing support is available.
	swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	
	ComPtr<IDXGISwapChain1> swapChain1;
	AssertDx(dxgiFactory4->CreateSwapChainForHwnd(
												  commandQueue.Get(),
												  windowHandle,
												  &swapChainDesc,
												  nullptr,
												  nullptr,
												  &swapChain1));
	
	//disable alt+enter toggling fullscreen
	AssertDx(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
	
	AssertDx(swapChain1.As(&dxgiSwapChain4));
	
	return dxgiSwapChain4;
}

local ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors) {
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;
	
	AssertDx(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));
	
	return descriptorHeap;
}

local void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap) {
	auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
	
	for (int i = 0; i < NumFrames; ++i) {
		ComPtr<ID3D12Resource> backBuffer;
		AssertDx(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
		
		device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
		
		backBuffers[i] = backBuffer;
		
		rtvHandle.Offset(rtvDescriptorSize);
	}
}

local ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) {
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	AssertDx(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));
	
	return commandAllocator;
}

local ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type) {
	ComPtr<ID3D12GraphicsCommandList> commandList;
	AssertDx(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
	
	AssertDx(commandList->Close());
	
	return commandList;
}

local ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device) {
	ComPtr<ID3D12Fence> fence;
	
	AssertDx(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	
	return fence;
}

local HANDLE CreateEventHandle() {
	HANDLE fenceEvent;
	
	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	Assert(fenceEvent, "Failed to create fence event.");
	
	return fenceEvent;
}

local u64 Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, u64& fenceValue) {
	u64 fenceValueForSignal = ++fenceValue;
	AssertDx(commandQueue->Signal(fence.Get(), fenceValueForSignal));
	
	return fenceValueForSignal;
}

local void WaitForFenceValue(ComPtr<ID3D12Fence> fence, u64 fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max()) {
	if (fence->GetCompletedValue() < fenceValue) {
		AssertDx(fence->SetEventOnCompletion(fenceValue, fenceEvent));
		::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
	}
}

local void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, u64& fenceValue, HANDLE fenceEvent) {
	u64 fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
	WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
}

void Render::
Init(){ //!Incomplete
	//// load RenderSettings ////
	LoadSettings();
	if(settings.debugging && settings.printf) settings.loggingLevel = 4;
	
	//// setup debug callback ////
	
	//// temp testing ////
	
	//enable Dx debug layer
#ifdef DESHI_INTERNAL
	ComPtr<ID3D12Debug> debugInterface;
	AssertDx(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
	
	//get the HWND already generated by glfw
	windowHandle = glfwGetWin32Window(DeshWindow->window);
	
	tearingSupported = CheckTearingSupport();
	
	device = CreateDevice(GetAdapter());
	
	commandQueue = CreateCommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	
	swapchain = CreateSwapChain(windowHandle, commandQueue, width, height, NumFrames);
	
	currentBackBufferIdx = swapchain->GetCurrentBackBufferIndex();
	
	rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NumFrames);
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	
	UpdateRenderTargetViews(device, swapchain, rtvDescriptorHeap);
	
	for (int i = 0; i < NumFrames; ++i)
	{
		commandAllocators[i] = CreateCommandAllocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	}
	
	commandList = CreateCommandList(
									device, commandAllocators[currentBackBufferIdx], D3D12_COMMAND_LIST_TYPE_DIRECT);
	
	fence = CreateFence(device);
	fenceEvent = CreateEventHandle();
	
	
	
	
	initialized = true;
}

local void Resize(u32 _width, u32 _height) {
	if (width != _width || height != _height) {
		// Don't allow 0 size swap chain back buffers.
		width  = Max(1u, width);
		height = Max(1u, height);
		
		// Flush the GPU queue to make sure the swap chain's back buffers
		// are not being referenced by an in-flight command list.
		Flush(commandQueue, fence, fenceValue, fenceEvent);
		
		for (int i = 0; i < NumFrames; ++i) {
			// Any references to the back buffers must be released
			// before the swap chain can be resized.
			backBuffers[i].Reset();
			frameFenceValues[i] = frameFenceValues[currentBackBufferIdx];
		}
		
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		AssertDx(swapchain->GetDesc(&swapChainDesc));
		AssertDx(swapchain->ResizeBuffers(NumFrames, width, height,
										  swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));
		
		currentBackBufferIdx = swapchain->GetCurrentBackBufferIndex();
		
		UpdateRenderTargetViews(device, swapchain, rtvDescriptorHeap);
	}
}

/////////////////
//// @update ////
/////////////////
void Render::
Update(){ //!Incomplete
	TIMER_START(t_u);
	
	//reset the command allocator and command list to its initial state
	auto commandAllocator = commandAllocators[currentBackBufferIdx];
	auto backBuffer = backBuffers[currentBackBufferIdx];
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);
	
	{//clear render target
		
		//make a transition resource barrier
		//in order to transition resources between states you must use a barrier
		//in this case we are transitioning a resource from a presenting state to a render target state
		
		CD3DX12_RESOURCE_BARRIER barrier = 
			CD3DX12_RESOURCE_BARRIER::Transition(
												 backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		
		commandList->ResourceBarrier(1, &barrier);
		
		float* clearColor = &settings.clearColor.r;
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
										  currentBackBufferIdx, rtvDescriptorSize);
		
		commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	}
	
	
	{// present
		
		//here we are transitioning again, just in the other way
		
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
																				backBuffer.Get(),
																				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		
		commandList->ResourceBarrier(1, &barrier);
		
		AssertDx(commandList->Close());
		
		ID3D12CommandList* const commandLists[] = { commandList.Get() };
		
		commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
		
		UINT syncInterval = 0;
		UINT presentFlags = tearingSupported ? DXGI_PRESENT_ALLOW_TEARING : 0;
		AssertDx(swapchain->Present(syncInterval, presentFlags));
		
		frameFenceValues[currentBackBufferIdx] = Signal(commandQueue, fence, fenceValue);
		
		currentBackBufferIdx = swapchain->GetCurrentBackBufferIndex();
		
		WaitForFenceValue(fence, frameFenceValues[currentBackBufferIdx], fenceEvent);
	}
	
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
Reset(){ //!Incomplete
	
}

//////////////////
//// @cleanup ////
//////////////////
void Render::
Cleanup(){ //!Incomplete
	Flush(commandQueue, fence, fenceValue, fenceEvent);
	
	::CloseHandle(fenceEvent);
	
}
