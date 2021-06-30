#pragma once
#ifndef COMPONENT_ORB_H
#define COMPONENT_ORB_H

#include "Component.h"
#include "../../math/VectorMatrix.h"

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
	int orbcount;
	Mesh* mesh = nullptr;
	std::vector<Orb*> orbs;
	
	OrbManager();
	OrbManager(Mesh* mesh, int orbcount = 100);
	
	void Init() override;
	void Update() override;
	void ReceiveEvent(Event event) override;
	
	std::string SaveTEXT() override;
	static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif //COMPONENT_ORB_H