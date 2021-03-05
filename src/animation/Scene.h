#pragma once
#include "Model.h"

struct Light;
struct Edge3D;

struct Scene{
	std::vector<Model*>  models;
	std::vector<Light*>  lights;
	std::vector<Edge3D*> lines;
	
	//TODO(r,delle) implement all of these
	bool RENDER_WIREFRAME           = false;
	bool RENDER_EDGE_NUMBERS        = false;
	bool RENDER_TEXTURES            = false;
	bool RENDER_LOCAL_AXIS          = false;
	bool RENDER_GLOBAL_AXIS         = false; 
	bool RENDER_TRANSFORMS          = false;
	bool RENDER_PHYSICS             = false;
	bool RENDER_SCREEN_BOUNDING_BOX = false; 
	bool RENDER_MESH_VERTICES       = false; 
	bool RENDER_GRID                = false;
	bool RENDER_LIGHT_RAYS          = false;
	bool RENDER_MESH_NORMALS        = false;
};
