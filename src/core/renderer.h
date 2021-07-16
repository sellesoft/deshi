#pragma once
#ifndef DESHI_RENDERER_H
#define DESHI_RENDERER_H

#include "../defines.h"
#include "../math/VectorMatrix.h"
#include "../utils/Color.h"
#include "../utils/tuple.h"

#include <vector>
#include <string>

struct Scene; 
struct Mesh; 
struct Texture;

enum VSyncTypeBits : u32{
    VSyncType_Immediate,   //no image queue (necessary), display as soon as possible
	VSyncType_Mailbox,     //image queue that replaces current pending image with new one, but waits to display on refresh
	VSyncType_Fifo,        //image queue that only gets removed from on refresh, waits to display on refresh (regular Vsync)
	VSyncType_FifoRelaxed, //same as Fifo, but if the image generates slower than refresh, dont wait to display on next refresh
}; typedef u32 VSyncType;

struct RenderSettings{ //loaded from file
	//// requires restart ////
    b32 debugging = true;
    b32 printf    = false;
	b32 recompileAllShaders = true;
    b32 findMeshTriangleNeighbors = true; //TODO(delle,Cl) move this to a better location
	
	//// runtime changeable ////
	u32 loggingLevel = 1; //if printf is true in the config file, this will be set to 4
	b32 crashOnError = false;
	VSyncType vsync  = VSyncType_Immediate;
	u32 msaaSamples  = 0;
	
	//shaders
	b32 optimizeShaders = false;
	
	//shadows
	b32 shadowPCF         = false;
	u32 shadowResolution  = 2048;
	f32 shadowNearZ       = 1.f;
	f32 shadowFarZ        = 70.f;
	f32 depthBiasConstant = 1.25f;
	f32 depthBiasSlope    = 1.75f;
    b32 showShadowMap     = false;
	
    //colors
	Vector4 clearColor   {0.02f,0.02f,0.02f,1.00f};
	Vector4 selectedColor{0.80f,0.49f,0.16f,1.00f};
	Vector4 colliderColor{0.46f,0.71f,0.26f,1.00f};
	
	//filters
    b32 wireframeOnly = false;
	
    //overlays
	b32 meshWireframes  = false;
    b32 meshNormals     = false;
    b32 lightFrustrums  = false;
};

struct RenderStats{
    u32 totalTriangles;
    u32 totalVertices;
    u32 totalIndices;
    u32 drawnTriangles;
    u32 drawnIndices;
    f32 renderTimeMS;
};

enum RendererStageBits : u32 { 
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

///////////////////////////////////////////////////////////////////////////////////////////////////
//TODO(delle,Re) move vulkan stuff to its cpp once we have a better interface for querying info about them

#if defined(_MSC_VER)
#pragma comment(lib,"vulkan-1.lib")
#pragma comment(lib,"glfw3.lib")
#endif
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

struct VertexVk{
	vec4 pos;
	vec4 uv;
	vec4 color;
	vec4 normal;
    
    bool operator==(const VertexVk& other) const {
        return pos == other.pos && color == other.color && uv == other.uv && normal == other.normal;
    }
};

typedef u8 stbi_uc;
struct TextureVk {
    char filename[DESHI_NAME_SIZE];
    u32 id = 0xFFFFFFFF;
    int width, height, channels;
    stbi_uc* pixels;
    u32 mipLevels;
    u32 type;
    
    VkImage        image;
    VkDeviceMemory imageMemory;
    VkDeviceSize   imageSize;
    
    VkImageView   view;
    VkSampler     sampler;
    VkImageLayout layout;
    VkDescriptorImageInfo imageInfo; //just a combo of the previous three vars
};

//a primitive contains the information for one draw call (a batch)
struct PrimitiveVk{
    u32 firstIndex    = 0;
    u32 indexCount    = 0;
    u32 materialIndex = 0;
};

struct MeshVk{
    u32 id      = -1;
    b32 visible = true;
    b32 base    = false;
    Mesh* ptr   = nullptr;
    char name[DESHI_NAME_SIZE];
    mat4 modelMatrix = mat4::IDENTITY;
    std::vector<PrimitiveVk> primitives;
    std::vector<u32> children;
};

struct MeshBrushVk{
    u32  id = -1;
    char name[DESHI_NAME_SIZE];
    b32 visible = true;
    mat4 modelMatrix = mat4::IDENTITY;
    std::vector<VertexVk> vertices;
    std::vector<u32>      indices;
    VkBuffer       vertexBuffer       = 0;
    VkDeviceMemory vertexBufferMemory = 0;
    VkDeviceSize   vertexBufferSize   = 0;
    VkBuffer       indexBuffer        = 0;
    VkDeviceMemory indexBufferMemory  = 0;
    VkDeviceSize   indexBufferSize    = 0;
};

struct MaterialVk{
    u32 id         = -1;
    u32 shader     = 0;
    u32 albedoID   = 0;
    u32 normalID   = 2;
    u32 specularID = 2;
    u32 lightID    = 2;
    char name[DESHI_NAME_SIZE];
    
    VkDescriptorSet descriptorSet;
    VkPipeline      pipeline = 0;
};
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace Render{
	
    void LoadSettings();
    void SaveSettings();
	
	RenderSettings* GetSettings();
    RenderStats*    GetStats();
    RendererStage*  GetStage();
	
    //returns a mesh brush ID
    u32 CreateDebugLine(Vector3 start, Vector3 end, Color color, bool visible = false);
    void UpdateDebugLine(u32 id, Vector3 start, Vector3 end, Color color);
    //returns a mesh brush ID
    u32 CreateDebugTriangle(Vector3 v1, Vector3 v2, Vector3 v3, Color color, bool visible = false);
    //creates a mesh with editable vertices that requires getting its buffers updated
    u32 CreateMeshBrush(Mesh* m, Matrix4 matrix, b32 log_creation = false);
    void UpdateMeshBrushMatrix(u32 meshID, Matrix4 transform);
    void UpdateMeshBrushBuffers(u32 meshBrushIdx);
    void RemoveMeshBrush(u32 meshBrushIdx);
	void UpdateMeshBrushVisibility(u32 meshID, bool visible);
	u32 MeshBrushCount();
	
    //2d primitives where points are defined from -1 to 1, where -1 is the bottom/left and 1 is the top/right of the screen
    u32 CreateTwod(std::vector<Vector2> points);
    u32 CreateTwod(std::vector<Vector2> points, Vector2 position, Vector2 scale);
    u32 CreateTwod(std::vector<Vector2> points, Vector2 position, Vector2 scale, Color color, float rotation);
    //position: center of the image
    u32 CreateImage(Texture texture, Vector2 position);
    //removes the twod with twodID from the 2d shader's vertex buffer
    void RemoveTwod(u32 twodID);
    void UpdateTwodColor(u32 twodID, Color color);
    void UpdateTwodPosition(u32 twodID, Vector2 position);
    void UpdateTwodRotation(u32 twodID, float rotation);
    void UpdateTwodVisibility(u32 twodID, b32 visible);
    
    //loads a mesh to the different shaders specified in its batches
    //returns the ID of the mesh
    u32 LoadBaseMesh(Mesh* m, bool visible = false);
    u32 GetBaseMeshID(const char* name);
    u32 CreateMesh(Scene* scene, const char* filename, b32 new_material = false);
    u32 CreateMesh(Mesh* mesh, Matrix4 matrix, b32 new_material = false);
    u32 CreateMesh(u32 meshID, Matrix4 matrix, b32 new_material = false);
    void UnloadBaseMesh(u32 meshID);
    void RemoveMesh(u32 meshID);
    Matrix4 GetMeshMatrix(u32 meshID);
    Mesh* GetMeshPtr(u32 meshID);
    //updates a mesh's model matrix: translation, rotation, scale
    void UpdateMeshMatrix(u32 meshID, Matrix4 matrix);
    void TransformMeshMatrix(u32 meshID, Matrix4 transform);
    void UpdateMeshBatchMaterial(u32 meshID, u32 batchIndex, u32 matID);
    void UpdateMeshVisibility(u32 meshID, bool visible);
	u32 MeshCount();
	b32 IsBaseMesh(u32 meshIdx);
	char* MeshName(u32 meshIdx);
	b32 IsMeshVisible(u32 meshIdx);
	
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
    //unloads a texture from the GPU
    //NOTE the previously used texture ID will not be used again
    void UnloadTexture(u32 textureID);
    std::string ListTextures();
	u32 TextureCount();
	char* TextureName(u32 textureIdx);
    
    u32 CreateMaterial(const char* name, u32 shader, u32 albedoTextureID = 0, u32 normalTextureID = 2, u32 specTextureID = 2, u32 lightTextureID = 2);
    u32  CopyMaterial(u32 materialID);
    void UpdateMaterialTexture(u32 matID, u32 textureType, u32 textureID);
    void UpdateMaterialShader(u32 matID, u32 shader);
    std::vector<u32> GetMaterialIDs(u32 MeshID);
    void RemoveMaterial(u32 materialID);
	u32 MaterialCount();
	char* MaterialName(u32 materialIdx);
    
    void LoadDefaultAssets();
    //loads a new scene to the GPU
    void LoadScene(Scene* scene);
    
	void UpdateLight(u32 lightIdx, Vector4 vec);
	
    void UpdateCameraPosition(Vector3 position);
    void UpdateCameraViewMatrix(Matrix4 m);
    void UpdateCameraProjectionMatrix(Matrix4 m);
	
    pair<Vector3, Vector3> SceneBoundingBox(); 
    
    //signals vulkan to remake the pipelines
    void ReloadShader(u32 shaderID);
    void ReloadAllShaders();
    void UpdateRenderSettings(RenderSettings settings);
	
	
	//TODO(delle,Re) make a better interface so other code doesnt need to access these directly
	//temporary funcs
	void remakeOffscreen();
	std::vector<VertexVk>*    vertexArray();
	std::vector<u32>*         indexArray();
	std::vector<TextureVk>*   textureArray();
	std::vector<MeshVk>*      meshArray();
	std::vector<MaterialVk>*  materialArray();
	std::vector<MeshBrushVk>* meshBrushArray();
	std::vector<u32>*         selectedArray();
	vec4*                     lightArray();
	
	void Init();
	void Update();
	void Reset();
	void Cleanup();
	
}; //namespace Render

//functions in this namespace are Immediate Mode, so they only last 1 frame
namespace UI{
	
	void DrawRect(vec2 position, vec2 dimensions, Color color = Color::WHITE);
	
}; //namespace UI

#endif //DESHI_RENDERER_H