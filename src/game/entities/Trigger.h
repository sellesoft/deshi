#pragma once
#ifndef ENTITY_TRIGGER_H
#define ENTITY_TRIGGER_H

#include "Entity.h"
//#include "../../utils/defines.h"

struct Physics;
struct Collider;
struct Mesh;

struct Trigger : public Entity {
	
	Physics* physics;
	Collider* collider;
	Mesh* mesh;

	Trigger(Transform transform, const char* name = 0);
	Trigger(Transform transform, Collider* collider, const char* name = 0);

	void Init() override;
	

};

#endif //GAME_ENTITY_PLAYER_H