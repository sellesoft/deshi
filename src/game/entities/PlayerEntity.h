#pragma once
#ifndef GAME_ENTITY_PLAYER_H
#define GAME_ENTITY_PLAYER_H

#include "Entity.h"
//#include "../../utils/defines.h"

struct Player;
struct Movement;
struct AudioListener;
struct AudioSource;
struct Collider;
struct MeshComp;
struct Physics;
struct Transform;
struct Admin;

struct PlayerEntity : public Entity {
	Player* player;
	Movement* movement;
	AudioListener* listener;
	AudioSource* source;
	Collider* collider;
	MeshComp* mesh;
	Physics* physics;
	
	PlayerEntity(Transform transform);

	void Init() override;
	
};

#endif //GAME_ENTITY_PLAYER_H