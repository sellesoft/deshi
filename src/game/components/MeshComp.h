#pragma once
#ifndef COMPONENT_MODELINSTANCE_H
#define COMPONENT_MODELINSTANCE_H

#include "Component.h"
#include "../../math/matrix.h"

struct Model;
struct Mesh;
struct Armature;

//NOTE model instance 'owns' a model in the scene
struct ModelInstance : public Component {
	Model*    model;
	Mesh*     mesh;
	Armature* armature;
	Matrix4   transform;
	bool      visible;
	bool      override; //overrides entity transform
	
	ModelInstance();
	ModelInstance(Model* model);
	~ModelInstance();
	
	void ToggleVisibility(){ visible = !visible; };
	void ToggleOverride(){ override = !override; };
	
	void ChangeModel(Model* model);
	
	void Update() override;
	void ReceiveEvent(Event event) override;
	std::string SaveTEXT() override;
	static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif //COMPONENT_MODELINSTANCE_H