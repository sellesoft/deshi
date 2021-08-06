#pragma once
#ifndef COMPONENT_ORB_H
#define COMPONENT_ORB_H

#include "Component.h"
#include "../../math/VectorMatrix.h"

struct ModelInstance;
struct Model;

struct Orb {
	vec3 pos;
	vec3 posbflerp;
	vec3 origpos;
	vec3 vel;
	vec3 acc;
	vec3 rotbflerp;
	vec3 rot;
	vec3 rotvel;
	vec3 rotacc;
	
	ModelInstance* mc;
	
	vec3 ito = vec3(2, 0, 2);
	
	Orb(vec3 pos, vec3 rot, vec3 rotvel, vec3 rotacc) {
		this->pos = pos;
		origpos = pos;
		this->rot = rot;
		this->rotvel = rotvel;
		this->rotacc = rotacc;
	}
};

struct OrbManager : public Component {
	int orbcount;
	Model* model = nullptr;
	std::vector<Orb*> orbs;
	
	OrbManager();
	OrbManager(Model* model, int orbcount = 100);
	
	void Init() override;
	void Update() override;
	void ReceiveEvent(Event event) override;
	
	std::string SaveTEXT() override;
	static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif //COMPONENT_ORB_H