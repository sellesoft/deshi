#pragma once
#ifndef COMPONENT_MESHRENDERER_H
#define COMPONENT_MESHRENDERER_H

#include "Component.h"
#include "../../utils/defines.h"

struct Mesh;

struct MeshComp : public Component {
	uint16 InstanceID;
	Mesh* m;

	MeshComp();
	MeshComp(Mesh* m);

	bool mesh_visible = true;

	void Update() override;
	void ReceiveEvent(Event event) override;
};


#endif