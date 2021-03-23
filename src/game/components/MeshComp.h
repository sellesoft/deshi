#pragma once
#ifndef COMPONENT_MESHRENDERER_H
#define COMPONENT_MESHRENDERER_H

#include "Component.h"
#include "../../utils/defines.h"

struct Mesh;

struct MeshComp : public Component {
	uint16 InstanceID;
	Mesh* m;

	MeshComp() {
		//empty for adding in commands
	}

	MeshComp(Mesh* m) {
		this->m = m;
		name = "MeshComp";
	}

	bool mesh_visible = true;

	void Update() override;
};


#endif