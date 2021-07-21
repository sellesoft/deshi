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
init(){
	
}

void DeshiImGui::
cleanup(){
	
}

void DeshiImGui::
newFrame(){
	
}


//-------------------------------------------------------------------------------------------------
// @UI INTERFACE


void UI::
FillRect(f32 x, f32 y, f32 w, f32 h, Color color){
	if(color.a == 0) return;
	//@Incomplete
}


//-------------------------------------------------------------------------------------------------
// @INTERFACE FUNCTIONS


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

void Render::
LoadDefaultAssets(){
	//@Incomplete
}

void Render::
remakeOffscreen(){
	//@Incomplete
}

std::string Render::
SaveMeshTEXT(u32 meshID){
	//@Incomplete
	return "";
}

std::string Render::
SaveMaterialTEXT(u32 matID){
	//@Incomplete
	return "";
}


////////////////////////
//// @trimesh stuff ////
////////////////////////


u32 Render::
LoadBaseMesh(Mesh* m, bool visible){
	//@Incomplete
	return -1;
}

u32 Render::
CreateMesh(Scene* scene, const char* filename, bool new_material){
	//@Incomplete
	return -1;
}

u32 Render::
CreateMesh(Mesh* m, Matrix4 matrix, bool new_material){
	//@Incomplete
	return -1;
}

u32 Render::
CreateMesh(u32 meshID, Matrix4 matrix, bool new_material){
	//@Incomplete
	return -1;
}

void Render::
UnloadBaseMesh(u32 meshID){
	//@Incomplete
}

void Render::
RemoveMesh(u32 meshID){
	//@Incomplete
}

Matrix4 Render::
GetMeshMatrix(u32 meshID){
	//@Incomplete
	return Matrix4(0.f);
}

Mesh* Render::
GetMeshPtr(u32 meshID){
	//@Incomplete
	return nullptr;
}

u32 Render::
GetBaseMeshID(const char* name){
	//@Incomplete
	return -1;
}

void Render::
UpdateMeshMatrix(u32 meshID, Matrix4 matrix){
	//@Incomplete
}

void Render::
TransformMeshMatrix(u32 meshID, Matrix4 transform){
	//@Incomplete
}

void Render::
UpdateMeshBatchMaterial(u32 meshID, u32 batchIndex, u32 matID){
	//@Incomplete
}

void Render::
UpdateMeshVisibility(u32 meshID, bool visible){
	//@Incomplete
}

u32 Render::
MeshBatchCount(u32 meshID){
	//@Incomplete
	return -1;
}

u32 Render::
MeshBatchMaterial(u32 meshID, u32 batchIndex){
	//@Incomplete
	return -1;
}

void Render::
AddSelectedMesh(u32 meshID){
	//@Incomplete
}

void Render::
RemoveSelectedMesh(u32 meshID){
	//@Incomplete
}


////////////////////////
//// @texture stuff ////
////////////////////////


u32 Render::
LoadTexture(const char* filename, u32 type){
	//@Incomplete
	return -1;
}

u32 Render::
LoadTexture(Texture texture){
	//@Incomplete
	return -1;
}

std::string Render::
ListTextures(){
	//@Incomplete
	return "";
}

u32 Render::
CreateFont(u32 textureIdx){
	//@Incomplete
	return -1;
}

/////////////////////////
//// @material stuff ////
/////////////////////////

u32 Render::
CreateMaterial(const char* name, u32 shader, u32 albedoTextureID, u32 normalTextureID, u32 specTextureID, u32 lightTextureID){
	//@Incomplete
	return -1;
}

u32 Render::
CopyMaterial(u32 materialID){
	//@Incomplete
	return -1;
}

void Render::
UpdateMaterialTexture(u32 matID, u32 texType, u32 texID){
	//@Incomplete
}

void Render::
UpdateMaterialShader(u32 matID, u32 shader){
	//@Incomplete
}

std::vector<u32> Render::
GetMaterialIDs(u32 meshID){
	//@Incomplete
	return std::vector<u32>();
}

void Render::
RemoveMaterial(u32 matID){
	//@Incomplete
}


/////////////////////
//// @temp stuff ////
/////////////////////


void Render::
TempLine(Vector3 start, Vector3 end, Color color){
	//@Incomplete
}

void Render::
TempBox(Matrix4 transform, Color color){
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
	
	Render::TempLine(points[3], points[1], color);
	Render::TempLine(points[3], points[2], color);
	Render::TempLine(points[3], points[7], color);
	Render::TempLine(points[0], points[1], color);
	Render::TempLine(points[0], points[2], color);
	Render::TempLine(points[0], points[4], color);
	Render::TempLine(points[5], points[1], color);
	Render::TempLine(points[5], points[4], color);
	Render::TempLine(points[5], points[7], color);
	Render::TempLine(points[6], points[2], color);
	Render::TempLine(points[6], points[4], color);
	Render::TempLine(points[6], points[7], color);
}

void Render::
TempFrustrum(Vector3 position, Vector3 target, f32 aspectRatio, f32 fovx, f32 nearZ, f32 farZ, Color color){
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
	
	Render::TempLine(v[0], v[1], color);
	Render::TempLine(v[0], v[2], color);
	Render::TempLine(v[3], v[1], color);
	Render::TempLine(v[3], v[2], color);
	Render::TempLine(v[4], v[5], color);
	Render::TempLine(v[4], v[6], color);
	Render::TempLine(v[7], v[5], color);
	Render::TempLine(v[7], v[6], color);
	Render::TempLine(v[0], v[4], color);
	Render::TempLine(v[1], v[5], color);
	Render::TempLine(v[2], v[6], color);
	Render::TempLine(v[3], v[7], color);
}


//////////////////////
//// @other stuff ////
//////////////////////


void Render::
LoadScene(Scene* sc){
	//@Incomplete
}

void Render::
UpdateCameraPosition(Vector3 position){
	//@Incomplete
}

void Render::
UpdateCameraViewMatrix(Matrix4 m){
	//@Incomplete
}

void Render::
UpdateCameraProjectionMatrix(Matrix4 m){
	//@Incomplete
}

void Render::
SceneBoundingBox(Vector3* min, Vector3* max){
	//@Incomplete
}

void Render::
ReloadShader(u32 shader){
	//@Incomplete
}

void Render::
ReloadAllShaders(){
	//@Incomplete
}

void Render::
UpdateRenderSettings(RenderSettings new_settings){
	settings = new_settings;
};

u32 Render::
MeshCount(){
	//@Incomplete
	return -1;
}

bool Render::
IsBaseMesh(u32 meshIdx){
	//@Incomplete
	return false;
}

char* Render::
MeshName(u32 meshIdx){
	//@Incomplete
	return nullptr;
}

void Render::
UpdateLight(u32 lightIdx, Vector4 vec){
	//@Incomplete
}

u32 Render::
TextureCount(){
	//@Incomplete
	return -1;
}

u32 Render::
MaterialCount(){
	//@Incomplete
	return -1;
}

u32 Render::
MaterialShader(u32 matID){
	//@Incomplete
	return -1;
}

std::vector<u32> Render::
MaterialTextures(u32 matID){
	//@Incomplete
	return {};
}

std::string Render::
ListMaterials(){
	//@Incomplete
	return "";
}

bool Render::
IsMeshVisible(u32 meshIdx){
	//@Incomplete
	return false;
}

char* Render::
MaterialName(u32 matIdx){
	//@Incomplete
	return nullptr;
}

char* Render::
TextureName(u32 texIdx){
	//@Incomplete
	return nullptr;
}

void Render::
Init(){
	//@Incomplete
	//// load RenderSettings ////
	LoadSettings();
	if(settings.debugging && settings.printf){
		settings.loggingLevel = 4;
	}
}

void Render::
Update(){
	//@Incomplete
}

void Render::
Reset(){
	//@Incomplete
}

void Render::
Cleanup(){
	//@Incomplete
}
