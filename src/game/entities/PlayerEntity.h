#pragma once
#ifndef GAME_ENTITY_PLAYER_H
#define GAME_ENTITY_PLAYER_H

#include "Entity.h"

struct Player;
struct Movement;
struct AudioListener;
struct AudioSource;
struct Collider;
struct ModelInstance;
struct Physics;
struct Transform;
struct Admin;

struct PlayerEntity : public Entity {
	Physics* physics;
	Movement* movement;
	Player* player;
	AudioListener* listener;
	AudioSource* source;
	Collider* collider;
	ModelInstance* model;
	
	PlayerEntity(Transform transform);
	
	void Init() override;
	
};

#endif //GAME_ENTITY_PLAYER_H