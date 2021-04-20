#pragma once
#ifndef DESHI_SCENE_H
#define DESHI_SCENE_H

#include "Model.h"
#include <glm/gtc/type_ptr.hpp>

struct Light;
struct RenderedEdge3D;

//TODO(delle,Cl) move scene object to deshiEngine and maybe make it global

struct Scene{
	std::vector<Model>   models;
	std::vector<Light*>  lights;
	std::vector<RenderedEdge3D*> lines;
	
	void Init();
	void Reset();
	//this will be here until i find a better place/way to make this work on GPU or something idk
	std::pair<Vector3, Vector3> SceneBoundingBox();
};

inline void Scene::Init(){
	models.emplace_back(Mesh::CreateBox(Vector3(1,1,1)));
	models.emplace_back(Mesh::CreatePlanarBox(Vector3(1,1,1)));
	models.emplace_back(Mesh::CreatePlanarBox(Vector3(1,1,1), Texture("UV_Grid_Sm.jpg")));
	models.emplace_back(Mesh::CreateMeshFromOBJ("sphere.obj"));
	
	//TODO(delle,ReVu) add local axis, global axis, and grid meshes
}

inline void Scene::Reset(){
	models.clear();
	lights.clear();
	lines.clear();
	
	Init();
}

inline std::pair<Vector3, Vector3> Scene::SceneBoundingBox() {
	float inf = std::numeric_limits<float>::max();
	Vector3 max(-inf, -inf, -inf);
	Vector3 min( inf,  inf,  inf);
	
	Vector3 v;
	for (MeshVk& mesh : DengRenderer->meshes) {
		for (PrimitiveVk& p : mesh.primitives) {
			for(int i = p.firstIndex; i < p.indexCount; ++i){
				v = vec3((float*)glm::value_ptr(DengRenderer->vertexBuffer[i].pos)) +
					mat4((float*)glm::value_ptr(mesh.modelMatrix)).Translation();
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

#endif //DESHI_SCENE_H