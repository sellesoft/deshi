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
	
	Renderer* r;
	
	MeshComp();
	MeshComp(Mesh* m);
	
	bool mesh_visible = true;
	bool ENTITY_CONTROL = true;
	
	void ToggleVisibility();
	void UpdateMeshTransform(Vector3 position, Vector3 rotation, Vector3 scale);
	
	void ChangeMaterialShader(u32 s);
	void ChangeMaterialTexture(u32 t);

	
	void ReceiveEvent(Event event) override;
	void Update() override;
};


#endif //COMPONENT_MESHCOMP_H