#pragma once
#ifndef GAME_ENTITY_PLAYER_H
#define GAME_ENTITY_PLAYER_H

#include "Entity.h"

struct Player;
struct Movement;
struct AudioListener;
struct Collider;
struct MeshComp;
struct Physics;

struct PlayerEntity : public Entity {
	Player* player;
	Movement* movement;
	AudioListener* listener;
	Collider* collider;
	MeshComp* mesh;
	Physics* physics;
	
	
	
	
};

#endif //GAME_ENTITY_PLAYER_H