#pragma once
#ifndef DESHI_RENDERER_H
#define DESHI_RENDERER_H

#include "../defines.h"
#include "../math/VectorMatrix.h"
#include "../utils/Color.h"
#include "../utils/string.h"

#include <vector>
#include <string>

enum VSyncTypeBits{
    VSyncType_Immediate,   //no image queue (necessary), display as soon as possible
	VSyncType_Mailbox,     //image queue that replaces current pending image with new one, but waits to display on refresh
	VSyncType_Fifo,        //image queue that only gets removed from on refresh, waits to display on refresh (regular Vsync)
	VSyncType_FifoRelaxed, //same as Fifo, but if the image generates slower than refresh, dont wait to display on next refresh
}; typedef u32 VSyncType;

enum RendererStageBits{ 
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
    bool findMeshTriangleNeighbors = true; //TODO(delle,Cl) move this to a better location
	u32  msaaSamples = 0;
	bool textureFiltering = false;
	bool anistropicFiltering = false;
	
	//// runtime changeable ////
	u32  loggingLevel = 1; //if printf is true in the config file, this will be set to 4
	bool crashOnError = false;
	VSyncType vsync  = VSyncType_Immediate;
	
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
	Vector4 clearColor   {0.02f,0.02f,0.02f,1.00f};
	Vector4 selectedColor{0.80f,0.49f,0.16f,1.00f};
	Vector4 colliderColor{0.46f,0.71f,0.26f,1.00f};
	
	//filters
    bool wireframeOnly = false;
	
    //overlays
	bool meshWireframes  = false;
    bool meshNormals     = false;
    bool lightFrustrums  = false;
	bool tempMeshOnTop   = false;
};

struct Texture;
struct Material;
struct Model;
struct Mesh;
struct Font;
namespace Render{
	
    void LoadSettings();
    void SaveSettings();
	RenderSettings* GetSettings();
    RenderStats*    GetStats();
    RendererStage*  GetStage();
	
	void LoadFont(Font* font);
    void LoadTexture(Texture* texture);
	void LoadMaterial(Material* material);
	void LoadMesh(Mesh* mesh);
	
	void UnloadFont(Font* font);
    void UnloadTexture(Texture* texture);
	void UnloadMaterial(Material* material);
	void UnloadMesh(Mesh* mesh);
    
	void DrawModel(Model* model, Matrix4 matrix);
	void DrawModelWireframe(Model* model, Matrix4 matrix, Color color = Color::WHITE);
	void DrawLine(Vector3 start, Vector3 end, Color color = Color::WHITE);
	void DrawTriangle(Vector3 p0, Vector3 p1, Vector3 p2, Color color = Color::WHITE);
	void DrawBox(Matrix4 transform, Color color = Color::WHITE);
    void DrawFrustrum(Vector3 position, Vector3 target, f32 aspectRatio, f32 fovx, f32 nearZ, f32 farZ, Color color = Color::WHITE);
	
    //ui drawing functions
    void FillRectUI(f32 x, f32 y, f32 width, f32 height, Color color = Color::WHITE);
    void DrawLineUI(f32 x1, f32 y1, f32 x2, f32 y2, float thickness = 1, Color color = Color::WHITE);
    void DrawLineUI(vec2 start, vec2 end, float thickness = 1, Color color = Color::WHITE);
    void DrawTextUI(string text, vec2 pos, Color color = Color::WHITE);
    void DrawCharUI(u32 character, vec2 pos, vec2 scale = vec2::ONE, Color color = Color::WHITE);
	
    
	void UpdateLight(u32 lightIdx, Vector4 vec);
    void UpdateCameraPosition(Vector3 position);
    void UpdateCameraViewMatrix(Matrix4 m);
    void UpdateCameraProjectionMatrix(Matrix4 m);
	
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