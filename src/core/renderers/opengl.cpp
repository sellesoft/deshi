//-------------------------------------------------------------------------------------------------
// @OPENGL STRUCTS


//-------------------------------------------------------------------------------------------------
// @INTERFACE VARIABLES


local RenderSettings settings;
local ConfigMap configMap = {
	{"#render settings config file",0,0},
	{"\n#    //// REQUIRES RESTART ////",  ConfigValueType_PADSECTION,(void*)10},
	{"debugging", ConfigValueType_Bool, &settings.debugging},
	{"printf",    ConfigValueType_Bool, &settings.printf},
	{"recompile_all_shaders",        ConfigValueType_Bool, &settings.recompileAllShaders},
	{"find_mesh_triangle_neighbors", ConfigValueType_Bool, &settings.findMeshTriangleNeighbors},
	{"\n#    //// RUNTIME VARIABLES ////", ConfigValueType_PADSECTION,(void*)15},
	{"logging_level",  ConfigValueType_U32,  &settings.loggingLevel},
	{"crash_on_error", ConfigValueType_Bool, &settings.crashOnError},
	{"vsync_type",     ConfigValueType_U32,  &settings.vsync},
	{"msaa_samples",   ConfigValueType_U32,  &settings.msaaSamples},
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





//-------------------------------------------------------------------------------------------------
// @OPENGL FUNCTIONS





//-------------------------------------------------------------------------------------------------
// @IMGUI FUNCTIONS


local char iniFilepath[256] = {};
void DeshiImGui::
Init(){
	
}

void DeshiImGui::
Vleanup(){
	
}

void DeshiImGui::
NewFrame(){
	
}


//-------------------------------------------------------------------------------------------------
// @UI INTERFACE


void UI::
FillRect(f32 x, f32 y, f32 w, f32 h, Color color){
	if(color.a == 0) return;
	//!Incomplete
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
LoadFont(Font* font){
	//!Incomplete
}

void Render::
LoadTexture(Texture* texture){
	//!Incomplete
}

void Render::
LoadMaterial(Material* material){
	//!Incomplete
}

void Render::
LoadMesh(Mesh* mesh){
	//!Incomplete
}

void Render::
UpdateMaterial(Material* material){
	//!Incomplete
}

/////////////////
//// @unload ////
/////////////////
void Render::
UnloadFont(Font* font){
	//!Incomplete
}

void Render::
UnloadTexture(Texture* texture){
	//!Incomplete
}

void Render::
UnloadMaterial(Material* material){
	//!Incomplete
}

void Render::
UnloadMesh(Mesh* mesh){
	//!Incomplete
}

///////////////
//// @draw ////
///////////////
void Render::
DrawModel(Model* mesh, mat4 matrix){
	//!Incomplete
}

void Render::
DrawLine(vec3 start, vec3 end, Color color){
	if(color.a == 0) return;
	
	vec3 col((f32)color.r / 255.0f, (f32)color.g / 255.0f, (f32)color.b / 255.0f);
	Vertex* vp = tempVertexArray + tempVertexCount;
	u16*    ip = tempIndexArray  + tempIndexCount;
	
	ip[0] = tempVertexCount; 
	ip[1] = tempVertexCount+1; 
	ip[2] = tempVertexCount;
	vp[0].pos = start; vp[0].color = col;
	vp[1].pos = end;   vp[1].color = col;
	
	tempVertexCount += 2;
	tempIndexCount  += 3;
}

void Render::
DrawBox(mat4 transform, Color color){
	if(color.a == 0) return;
	
	vec3 p(0.5f, 0.5f, 0.5f);
	vec3 points[8] = {
		{-p.x, p.y, p.z},
		{-p.x,-p.y, p.z},
		{-p.x, p.y,-p.z},
		{-p.x,-p.y,-p.z},
		{ p.x, p.y, p.z},
		{ p.x,-p.y, p.z},
		{ p.x, p.y,-p.z},
		{ p.x,-p.y,-p.z},
	};
	forI(8){
		points[i] = points[i] * transform;
	}
	
	DrawLine(points[3], points[1], color);
	DrawLine(points[3], points[2], color);
	DrawLine(points[3], points[7], color);
	DrawLine(points[0], points[1], color);
	DrawLine(points[0], points[2], color);
	DrawLine(points[0], points[4], color);
	DrawLine(points[5], points[1], color);
	DrawLine(points[5], points[4], color);
	DrawLine(points[5], points[7], color);
	DrawLine(points[6], points[2], color);
	DrawLine(points[6], points[4], color);
	DrawLine(points[6], points[7], color);
}

void Render::
DrawFrustrum(vec3 position, vec3 target, f32 aspectRatio, f32 fovx, f32 nearZ, f32 farZ, Color color){
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
UpdateCameraPosition(vec3 position){
	//!Incomplete
}

void Render::
UpdateCameraViewMatrix(mat4 m){
	//!Incomplete
}

void Render::
UpdateCameraProjectionMatrix(mat4 m){
	//!Incomplete
}

//////////////////
//// @shaders ////
//////////////////
void Render::
ReloadShader(u32 shader){
	//!Incomplete
}

void Render::
ReloadAllShaders(){
	//!Incomplete
}

////////////////
//// @fixme ////
////////////////
void Render::
UpdateLight(u32 lightIdx, vec4 vec){
	lights[lightIdx] = vec;
}

void Render::
remakeOffscreen(){
	//!Incomplete
}

///////////////
//// @init ////
///////////////
void Render::
Init(){
	//!Incomplete
	//// load RenderSettings ////
	LoadSettings();
	if(settings.debugging && settings.printf){
		settings.loggingLevel = 4;
	}
}

/////////////////
//// @update ////
/////////////////
void Render::
Update(){
	//!Incomplete
}

////////////////
//// @reset ////
////////////////
void Render::
Reset(){
	//!Incomplete
}

//////////////////
//// @cleanup ////
//////////////////
void Render::
Cleanup(){
	//!Incomplete
}
