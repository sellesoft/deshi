#pragma once
#ifndef ENTITY_TRIGGER_H
#define ENTITY_TRIGGER_H

#include "Entity.h"
//#include "../../utils/defines.h"

struct Collider;
struct Physics;

struct Trigger : public Entity {
	
	Physics* physics;
	Collider* collider;
	Mesh* mesh;

	Trigger(Transform transform);
	Trigger(Transform transform, Collider* collider);

	void Init() override;
	

};

#endif //GAME_ENTITY_PLAYER_H