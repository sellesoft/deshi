#pragma once
#ifndef COMPONENT_MODELINSTANCE_H
#define COMPONENT_MODELINSTANCE_H

#include "Component.h"

struct Model;
struct Mesh;
struct Armature;
struct Vector3;

struct ModelInstance : public Component {
	Model*    model;
	Mesh*     mesh;
	Armature* armature;
	bool   visible;
	
	ModelInstance();
	ModelInstance(Model* model);
	~ModelInstance();
	
	void ToggleVisibility();
	
	void ReceiveEvent(Event event) override;
	void Init() override;
	void Update() override;
	std::string SaveTEXT() override;
	static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif //COMPONENT_MODELINSTANCE_H