#pragma once
#include "Component.h"

struct Entity;

struct World : public Component {
	std::vector<Entity*> creationBuffer;
	std::vector<Entity*> deletionBuffer;

	World();
	~World();

	//dunno
	void Update() override;
};