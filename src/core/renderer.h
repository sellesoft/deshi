#pragma once
#ifndef DESHI_RENDERER_H
#define DESHI_RENDERER_H

#include "../defines.h"
#include "../math/VectorMatrix.h"
#include "../utils/Color.h"

#include <vector>
#include <string>

struct Scene; 
struct Mesh; 
struct Texture;

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
	
	//// runtime changeable ////
	u32 loggingLevel = 1; //if printf is true in the config file, this will be set to 4
	bool crashOnError = false;
	VSyncType vsync  = VSyncType_Immediate;
	u32 msaaSamples  = 0;
	
	//shaders
	bool optimizeShaders = false;
	
	//shadows
	bool shadowPCF         = false;
	u32 shadowResolution  = 2048;
	f32 shadowNearZ       = 1.f;
	f32 shadowFarZ        = 70.f;
	f32 depthBiasConstant = 1.25f;
	f32 depthBiasSlope    = 1.75f;
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

namespace Render{
	
    void LoadSettings();
    void SaveSettings();
	
	RenderSettings* GetSettings();
    RenderStats*    GetStats();
    RendererStage*  GetStage();
	
    //loads a mesh to the different shaders specified in its batches
    //returns the ID of the mesh
    u32 LoadBaseMesh(Mesh* m, bool visible = false);
    u32 GetBaseMeshID(const char* name);
    u32 CreateMesh(Scene* scene, const char* filename, bool new_material = false);
    u32 CreateMesh(Mesh* mesh, Matrix4 matrix = Matrix4::IDENTITY, bool new_material = false);
    u32 CreateMesh(u32 meshID, Matrix4 matrix = Matrix4::IDENTITY, bool new_material = false);
    u32 MeshCount();
    void UnloadBaseMesh(u32 meshID);
    void RemoveMesh(u32 meshID);
    //updates a mesh's model matrix: translation, rotation, scale
    void UpdateMeshMatrix(u32 meshID, Matrix4 matrix);
    void TransformMeshMatrix(u32 meshID, Matrix4 transform);
    void UpdateMeshBatchMaterial(u32 meshID, u32 batchIndex, u32 matID);
    void UpdateMeshVisibility(u32 meshID, bool visible);
	u32  MeshBatchCount(u32 meshID);
	u32  MeshBatchMaterial(u32 meshID, u32 batchIdx);
    Matrix4 GetMeshMatrix(u32 meshID);
    Mesh*   GetMeshPtr(u32 meshID);
	bool    IsBaseMesh(u32 meshIdx);
    bool    IsMeshVisible(u32 meshIdx);
	char*   MeshName(u32 meshIdx);
	
    void AddSelectedMesh(u32 meshID);
    //passing -1 will remove all
    void RemoveSelectedMesh(u32 meshID);
    
    u32  MakeInstance(u32 meshID, Matrix4 matrix);
    void RemoveInstance(u32 instanceID);
    //updates an instance's model matrix: translation, rotation, scale
    void UpdateInstanceMatrix(u32 instanceID, Matrix4 matrix);
    void TransformInstanceMatrix(u32 instanceID, Matrix4 transform);
    void UpdateInstanceVisibility(u32 instanceID, bool visible);
    
    //loads a texture onto the GPU
    //returns the texture's id
    u32 LoadTexture(const char* filename, u32 type);
    u32 LoadTexture(Texture texure);
    u32 TextureCount();
    //unloads a texture from the GPU
    //NOTE the previously used texture ID will not be used again
    void UnloadTexture(u32 textureID);
	char* TextureName(u32 textureIdx);
    std::string ListTextures();
	
	u32 CreateFont(u32 textureIdx);
    
    u32 CreateMaterial(const char* name, u32 shader, u32 albedoTextureID = 0, u32 normalTextureID = 2, u32 specTextureID = 2, u32 lightTextureID = 2);
    u32 CopyMaterial(u32 materialID);
    u32 MaterialCount();
	u32 MaterialShader(u32 matID);
    void UpdateMaterialTexture(u32 matID, u32 textureType, u32 textureID);
    void UpdateMaterialShader(u32 matID, u32 shader);
    void RemoveMaterial(u32 materialID);
	char* MaterialName(u32 materialIdx);
    std::vector<u32> GetMaterialIDs(u32 MeshID);
	std::vector<u32> MaterialTextures(u32 matID);
	std::string ListMaterials();
	
	void TempLine(Vector3 start, Vector3 end, Color color = Color::WHITE);
	void TempBox(Matrix4 transform, Color color = Color::WHITE);
    void TempFrustrum(Vector3 position, Vector3 target, f32 aspectRatio, f32 fovx, f32 nearZ, f32 farZ, Color color = Color::WHITE);
    
    void LoadDefaultAssets();
    void LoadScene(Scene* scene);
    
	void UpdateLight(u32 lightIdx, Vector4 vec);
	
    void UpdateCameraPosition(Vector3 position);
    void UpdateCameraViewMatrix(Matrix4 m);
    void UpdateCameraProjectionMatrix(Matrix4 m);
	
	//fills the min and max vec3's with the furthest vertices' positions in the scene
	void SceneBoundingBox(Vector3* min, Vector3* max); 
    
    //signals vulkan to remake the pipelines
    void ReloadShader(u32 shaderID);
    void ReloadAllShaders();
    void UpdateRenderSettings(RenderSettings settings);
	
	//temp funcs
	void remakeOffscreen();
	std::string SaveMeshTEXT(u32 meshID);
	std::string SaveMaterialTEXT(u32 matID);
	
	void Init();
	void Update();
	void Reset();
	void Cleanup();
	
}; //namespace Render

//functions in this namespace are Immediate Mode, so they only last 1 frame
namespace UI{
	
	void FillRect(f32 x, f32 y, f32 width, f32 height, Color color = Color::WHITE);
	
}; //namespace UI

#endif //DESHI_RENDERER_H