#pragma once
#ifndef COMPONENT_MESHCOMP_H
#define COMPONENT_MESHCOMP_H

#include "Component.h"

struct Mesh;

struct MeshComp : public Component {
	Mesh* mesh;
	u32 instanceID;
	u32 meshID;
	b32 mesh_visible = true;
	b32 ENTITY_CONTROL = true;
	
	MeshComp();
	MeshComp(u32 meshID, u32 instanceID = 0);
	~MeshComp();
	
	void ToggleVisibility();
	void Visible(bool visible);
	void UpdateMeshTransform(Vector3 position, Vector3 rotation, Vector3 scale);
	void ChangeMesh(u32 newMeshIdx);
	void ChangeMaterialShader(u32 s);
	void ChangeMaterialTexture(u32 t);
	
	void ReceiveEvent(Event event) override;
	void Init() override;
	void Update() override;
	std::string SaveTEXT() override;
	static void LoadDESH(EntityAdmin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

struct MeshComp2D : public Component {
	u32 twodID;
	b32 visible;
	
	MeshComp2D(u32 twodID);
	
	void ToggleVisibility();
	
	void ReceiveEvent(Event event) override;
	void Init() override;
	void Update() override;
	std::string SaveTEXT() override;
	static void LoadDESH(EntityAdmin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};


#endif //COMPONENT_MESHCOMP_H