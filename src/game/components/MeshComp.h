#pragma once
#ifndef COMPONENT_MESHCOMP_H
#define COMPONENT_MESHCOMP_H

#include "Component.h"
#include "../../utils/defines.h"

struct Mesh;


struct MeshComp : public Component {
	uint16 InstanceID;
	uint16 MeshID;
	Mesh* m;
	
	MeshComp();
	MeshComp(Mesh* m, u32 meshID = 0, u32 instanceID = 0);
	
	bool mesh_visible = true;
	bool ENTITY_CONTROL = true;
	
	void ToggleVisibility();
	void UpdateMeshTransform(Vector3 position, Vector3 rotation, Vector3 scale);
	
	void ChangeMaterialShader(u32 s);
	void ChangeMaterialTexture(u32 t);
	
	std::string Save() override;
	void Load() override;
	
	
	void ReceiveEvent(Event event) override;
	void Update() override;
};


#endif //COMPONENT_MESHCOMP_H