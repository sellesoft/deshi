#pragma once
#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "../defines.h"

struct Admin;
struct Movement;

enum MovementMode : u32 {
	MOVEMENT_MODE_FLYING,
	MOVEMENT_MODE_WALKING,
	MOVEMENT_MODE_SWIMMING,
};

struct Controller{
	Admin* admin;
	MovementMode mode;
	Movement* playermove;
	
	f32 mouseSensitivity;
	b32 cameraLocked = false;
	
	void Init(Admin* a, MovementMode m = MOVEMENT_MODE_FLYING);
	void Update();
};

#endif //GAME_CONTROLLER_H