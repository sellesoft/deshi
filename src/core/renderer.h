#pragma once
#ifndef DESHI_RENDERER_H
#define DESHI_RENDERER_H

#include "../defines.h"
#include "../math/VectorMatrix.h"
#include "../utils/color.h"
#include "../utils/string.h"
#include "../utils/array.h"

enum VSyncType_{
    VSyncType_Immediate,   //no image queue (necessary), display as soon as possible
	VSyncType_Mailbox,     //image queue that replaces current pending image with new one, but waits to display on refresh
	VSyncType_Fifo,        //image queue that only gets removed from on refresh, waits to display on refresh (regular Vsync)
	VSyncType_FifoRelaxed, //same as Fifo, but if the image generates slower than refresh, dont wait to display on next refresh
}; typedef u32 VSyncType;

enum RendererStage_{ 
    RENDERERSTAGE_NONE  = 0, 
    RSVK_INSTANCE       = 1 << 0,
    RSVK_SURFACE        = 1 << 1,
    RSVK_PHYSICALDEVICE = 1 << 2,
    RSVK_LOGICALDEVICE  = 1 << 3,
    RSVK_SWAPCHAIN      = 1 << 4,
    RSVK_RENDERPASS     = 1 << 5,
    RSVK_COMMANDPOOL    = 1 << 6,
    RSVK_FRAMES         = 1 << 7,
    RSVK_SYNCOBJECTS    = 1 << 8,
    RSVK_UNIFORMBUFFER  = 1 << 9,
    RSVK_LAYOUTS        = 1 << 10,
	RSVK_DESCRIPTORPOOL = 1 << 11,
    RSVK_DESCRIPTORSETS = 1 << 12,
    RSVK_PIPELINESETUP  = 1 << 13,
    RSVK_PIPELINECREATE = 1 << 14,
    RSVK_RENDER      = 0xFFFFFFFF,
}; typedef u32 RendererStage;

enum ShaderStage_{
    ShaderStage_NONE,
    ShaderStage_Vertex,
    ShaderStage_TessCtrl,
    ShaderStage_TessEval,
    ShaderStage_Geometry,
    ShaderStage_Fragment,
    ShaderStage_Compute,
    ShaderStage_COUNT,
}; typedef u32 ShaderStage;

struct RenderStats{
    u32 totalTriangles;
    u32 totalVertices;
    u32 totalIndices;
    u32 drawnTriangles;
    u32 drawnIndices;
    f32 renderTimeMS;
};

struct RenderSettings{
	//// requires restart ////
    bool debugging = true;
    bool printf    = false;
	bool recompileAllShaders = true;
	u32  msaaSamples = 0;
	bool textureFiltering = false;
	bool anistropicFiltering = false;
	
	//// runtime changeable ////
	u32  loggingLevel = 1; //if printf is true in the config file, this will be set to 4
	bool crashOnError = false;
	VSyncType vsync   = VSyncType_Immediate;
	
	//shaders
	bool optimizeShaders = false;
	
	//shadows
	bool shadowPCF         = false;
	u32  shadowResolution  = 2048;
	f32  shadowNearZ       = 1.f;
	f32  shadowFarZ        = 70.f;
	f32  depthBiasConstant = 1.25f;
	f32  depthBiasSlope    = 1.75f;
    bool showShadowMap     = false;
	
    //colors
	vec4 clearColor   {0.02f,0.02f,0.02f,1.00f};
	vec4 selectedColor{0.80f,0.49f,0.16f,1.00f};
	vec4 colliderColor{0.46f,0.71f,0.26f,1.00f};
	
	//filters
    bool wireframeOnly = false;
	
    //overlays
	bool meshWireframes  = false;
    bool meshNormals     = false;
    bool lightFrustrums  = false;
	bool tempMeshOnTop   = false;
};

struct Vertex2{
	vec2 pos;
	vec2 uv;
	u32  color;
};

struct Mesh;
struct Texture;
struct Material;
struct Model;
struct Font;
namespace Render{
	
    void LoadSettings();
    void SaveSettings();
	RenderSettings* GetSettings();
    RenderStats*    GetStats();
    RendererStage*  GetStage();
	
	void LoadMesh(Mesh* mesh);
    void LoadTexture(Texture* texture);
	void LoadMaterial(Material* material);
	void LoadFont(Font* font, Texture* texture);
	
	void RemakeTextures();
	void UpdateMaterial(Material* material);
	
	void UnloadFont(Font* font);
    void UnloadTexture(Texture* texture);
	void UnloadMaterial(Material* material);
	void UnloadMesh(Mesh* mesh);
    
	void DrawModel(Model* model, mat4 matrix);
	void DrawModelWireframe(Model* model, mat4 matrix, color _color = Color_White);
	void DrawLine(vec3 start, vec3 end, color _color = Color_White);
	void DrawTriangle(vec3 p0, vec3 p1, vec3 p2, color _color = Color_White);
	void DrawTriangleFilled(vec3 p0, vec3 p1, vec3 p2, color _color = Color_White);
	void DrawQuad(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color _color = Color_White);
	void DrawQuadFilled(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color _color = Color_White);
	void DrawPoly(array<vec3>& points, color _color = Color_White);
	void DrawPolyFilled(array<vec3>& points, color _color = Color_White);
	void DrawBox(mat4 transform, color _color = Color_White);
	void DrawBoxFilled(mat4 transform, color _color = Color_White);
    void DrawFrustrum(vec3 position, vec3 target, f32 aspectRatio, f32 fovx, f32 nearZ, f32 farZ, color _color = Color_White);
	
    //ui drawing functions
	void FillRectUI(vec2 pos, vec2 dimensions, color _color = Color_White, vec2 scissorOffset = vec2(0, 0), vec2 scissorExtent = vec2(-1, -1));
	void DrawRectUI(vec2 pos, vec2 dimensions, color _color = Color_White, vec2 scissorOffset = vec2(0, 0), vec2 scissorExtent = vec2(-1, -1));
	void DrawLineUI(vec2 start, vec2 end, float thickness = 1, color _color = Color_White, vec2 scissorOffset = vec2(0, 0), vec2 scissorExtent = vec2(-1, -1));
	void DrawTextUI(string text, vec2 pos, color _color = Color_White, vec2 scissorOffset = vec2(0, 0), vec2 scissorExtent = vec2(-1, -1));
	void DrawCharUI(u32 character, vec2 pos, vec2 scale = vec2::ONE, color _color = Color_White, vec2 scissorOffset = vec2(0, 0), vec2 scissorExtent = vec2(-1, -1));
	
	void UpdateLight(u32 lightIdx, vec4 vec);
    void UpdateCameraPosition(vec3 position);
    void UpdateCameraViewMatrix(mat4 m);
    void UpdateCameraProjectionMatrix(mat4 m);
	void UseDefaultViewProjMatrix(vec3 position = vec3::ZERO, vec3 rotation = vec3::ZERO);
	
    //signals vulkan to remake the pipelines
    void ReloadShader(u32 shaderID);
    void ReloadAllShaders();
	
	//temp funcs
	void remakeOffscreen();
	
	void Init();
	void Update();
	void Reset();
	void Cleanup();
	
}; //namespace Render



#endif //DESHI_RENDERER_H