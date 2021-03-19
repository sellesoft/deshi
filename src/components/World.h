#pragma once
#ifndef COMPONENT_WORLD_H
#define COMPONENT_WORLD_H

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

#endif //COMPONENT_WORLD_H