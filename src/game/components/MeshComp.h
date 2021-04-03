#pragma once
#ifndef COMPONENT_MESHRENDERER_H
#define COMPONENT_MESHRENDERER_H

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

	void ReceiveEvent(Event event) override;
	void Update() override;
};


#endif