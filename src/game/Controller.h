#pragma once
#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "../utils/defines.h"

struct EntityAdmin;
struct Movement;

enum MovementMode : u32 {
	MOVEMENT_MODE_FLYING,
	MOVEMENT_MODE_WALKING,
	MOVEMENT_MODE_SWIMMING,
};

struct Controller{
	EntityAdmin* admin;
	MovementMode mode;
	Movement* playermove;
	
	f32 mouseSensitivity;
	b32 cameraLocked = false;
	
	void Init(EntityAdmin* a, MovementMode m = MOVEMENT_MODE_FLYING);
	void Update();
};

#endif //GAME_CONTROLLER_H