#pragma once
#ifndef COMPONENT_MESHCOMP_H
#define COMPONENT_MESHCOMP_H

#include "Component.h"

struct Mesh;
struct Vector3;

struct MeshComp : public Component {
	Mesh* mesh;
	u32 instanceID;
	u32 meshID;
	bool mesh_visible = true;
	bool ENTITY_CONTROL = true;
	
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
	static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif //COMPONENT_MESHCOMP_H