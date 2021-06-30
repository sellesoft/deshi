#pragma once
#ifndef DESHI_SCENE_H
#define DESHI_SCENE_H

#include "Model.h"
#include "../math/Math.h"
#include "../core/console.h"

struct Light;
struct RenderedEdge3D;

//TODO(delle,Cl) move scene object to deshiEngine and maybe make it global

struct Scene{
	std::vector<Model>   models;
	std::vector<Light*> lights;
	std::vector<RenderedEdge3D*> lines;
	
	void Init();
	void Reset();
};

inline void Scene::Init(){
	models.emplace_back(Mesh::CreatePlanarBox(Vector3(1,1,1)));
	models.emplace_back(Mesh::CreateBox(Vector3(1,1,1)));
	models.emplace_back(Mesh::CreateMeshFromOBJ("sphere.obj"));
	models.emplace_back(Mesh::CreateMeshFromOBJ("arrow.obj"));
	
	//TODO(delle,ReVu) add local axis, global axis, and grid meshes
}

inline void Scene::Reset(){
	SUCCESS("Resetting scene");
	models.clear();
	lights.clear();
	lines.clear();
	
	Init();
}

#endif //DESHI_SCENE_H