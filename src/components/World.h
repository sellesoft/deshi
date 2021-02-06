#pragma once
#include "Component.h"

struct Entity;

struct World : public Component {
	std::vector<Entity*> creationBuffer;
	std::vector<Entity*> deletionBuffer;

	World() {
		creationBuffer = std::vector<Entity*>();
		deletionBuffer = std::vector<Entity*>();
	}

	~World() {
		for(Entity* e : creationBuffer) delete e;
		creationBuffer.clear();
		for(Entity* e : deletionBuffer) delete e;
		deletionBuffer.clear();
	}

	//dunno
	void Update() override;
};