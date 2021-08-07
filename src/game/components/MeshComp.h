#pragma once
#ifndef COMPONENT_MODELINSTANCE_H
#define COMPONENT_MODELINSTANCE_H

#include "Component.h"
#include "../../math/matrix.h"

struct Model;
struct Mesh;
struct Armature;

//NOTE model instance references a model in storage
struct ModelInstance : public Component {
	Model*    model;
	Mesh*     mesh;
	Armature* armature;
	mat4   transform;
	bool      visible;
	bool      control; //overrides entity transform
	
	ModelInstance();
	ModelInstance(Model* model);
	ModelInstance(Mesh* mesh);
	~ModelInstance();
	
	void ToggleVisibility(){ visible = !visible; };
	void ToggleOverride(){ control = !control; };
	
	void ChangeModel(Model* model);
	void ChangeModel(Mesh* mesh);
	
	void Update() override;
	void ReceiveEvent(Event event) override;
	std::string SaveTEXT() override;
	static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif //COMPONENT_MODELINSTANCE_H