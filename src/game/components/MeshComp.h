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

	MeshComp();
	MeshComp(Mesh* m);

	bool mesh_visible = true;

	void ToggleVisibility();

	void ReceiveEvent(Event event) override;
	void Update() override;
};


#endif