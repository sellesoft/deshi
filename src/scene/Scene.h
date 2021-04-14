#pragma once
#ifndef DESHI_SCENE_H
#define DESHI_SCENE_H

#include "Model.h"

struct Light;
struct RenderedEdge3D;

struct Scene{
	std::vector<Model>   models;
	std::vector<Light*>  lights;
	std::vector<RenderedEdge3D*> lines;
	
	void Init();
	void Reset();
	//this will be here until i find a better place/way to make this work on GPU or something idk
	std::pair<Vector3, Vector3> SceneBoundingBox();
};

#endif //DESHI_SCENE_H