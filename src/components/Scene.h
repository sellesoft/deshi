#pragma once
#include "Component.h"
#include "Model.h"

struct Light;
struct Edge3D;
//struct Mesh;

struct Scene : public Component {
	std::vector<Mesh*> meshes;
	std::vector<Edge3D*> lines;
	std::vector<Light*> lights;

	bool RENDER_WIREFRAME				= true;
	bool RENDER_EDGE_NUMBERS			= false;
	bool RENDER_TEXTURES				= false;
	bool RENDER_LOCAL_AXIS				= true;
	bool RENDER_GLOBAL_AXIS				= true; //TODO(r,delle) implement global axis like in blender
	bool RENDER_TRANSFORMS				= false;
	bool RENDER_PHYSICS					= true;
	bool RENDER_SCREEN_BOUNDING_BOX		= false; 
	bool RENDER_MESH_VERTICES			= false; 
	bool RENDER_GRID					= false; //TODO(r,delle) upgrade grid to follow camera in smart way
	bool RENDER_LIGHT_RAYS				= false;
	bool RENDER_MESH_NORMALS			= false;


	Scene(EntityAdmin* a);

	//I dont think scene needs this but im getting tired and unsure
	//void Update() override;
};