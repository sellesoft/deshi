#pragma once
#ifndef COMPONENT_ORB_H
#define COMPONENT_ORB_H

#include "Component.h"
#include "../../utils/defines.h"
#include "../../math/Vector3.h"

struct MeshComp;
struct Mesh;



struct Orb {

	Vector3 pos;
	Vector3 posbflerp;
	Vector3 origpos;
	Vector3 vel;
	Vector3 acc;
	Vector3 rotbflerp;
	Vector3 rot;
	Vector3 rotvel;
	Vector3 rotacc;

	MeshComp* mc;

	Vector3 ito = Vector3(2, 0, 2);

	Orb(Vector3 pos, Vector3 rot, Vector3 rotvel, Vector3 rotacc) {
		this->pos = pos;
		origpos = pos;
		this->rot = rot;
		this->rotvel = rotvel;
		this->rotacc = rotacc;
	}
};

struct OrbManager : public Component {
	
	OrbManager(Mesh* m, EntityAdmin* a, Entity* e);

	int orbcount = 100;

	

	std::vector<Orb*> orbs;




	void ReceiveEvent(Event event) override;
	void Update() override;
};

#endif