#pragma once
#include "Component.h"
#include "Mesh.h"

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


	Scene() {
		meshes = std::vector<Mesh*>();
		lights = std::vector<Light*>();
		lights = std::vector<Light*>();
	}

	//returns a bounding box of the entire scene two Vector3's, the first being the upper corner of the box
	//i considered returning an AABBcollider instead, but didn't like that it used half dimensions
	std::pair<Vector3, Vector3> SceneBoundingBox() {
		float inf = std::numeric_limits<float>::max();
		Vector3 max(-inf, -inf, -inf);
		Vector3 min( inf,  inf,  inf);

		std::vector<Vector3> vertices;

		for (Mesh* m : meshes) {
			for (Triangle t : m->triangles) {
				for (Vector3 v : t.points) {
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