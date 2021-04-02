#pragma once
#ifndef DESHI_SCENE_H
#define DESHI_SCENE_H

#include "Model.h"
#include "../game/components/Transform.h"

struct Light;
struct Edge3D;

struct Scene{
	std::vector<Model>   models;
	std::vector<Light*>  lights;
	std::vector<Edge3D*> lines;
	
	//TODO(delle,Re) implement all of these
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

	//this will be here until i find a better place/way to make this work on GPU or something idk
	std::pair<Vector3, Vector3> SceneBoundingBox() {
		float inf = std::numeric_limits<float>::max();
		Vector3 max(-inf, -inf, -inf);
		Vector3 min( inf,  inf,  inf);

		std::vector<Vector3> vertices;

		for (Model mo : models) {
			Mesh* m = &mo.mesh;
			for (Batch b : m->batchArray) {
				for (Vertex ver : b.vertexArray) {
					Vector3 v = Vector3(ver.pos.x, ver.pos.y, ver.pos.z) + Matrix4::PositionFromTransform(m->transform);
					if      (v.x < min.x) { min.x = v.x; }
					else if (v.x > max.x) { max.x = v.x; }
					if      (v.y < min.y) { min.y = v.y; }
					else if (v.y > max.y) { max.y = v.y; }
					if      (v.z < min.z) { min.z = v.z; }
					else if (v.z > max.z) { max.z = v.z; }
				}

			}
			
		}

		return std::pair<Vector3, Vector3>(max, min);
	}
};

#endif //DESHI_SCENE_H