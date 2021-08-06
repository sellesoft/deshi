#pragma once
#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "../defines.h"

struct Admin;
struct Movement;

enum MovementMode_{
	MOVEMENT_MODE_FLYING,
	MOVEMENT_MODE_WALKING,
	MOVEMENT_MODE_SWIMMING,
}; typedef u32 MovementMode;

struct Controller{
	Admin* admin;
	MovementMode mode;
	Movement* playermove;
	
	f32 mouseSensitivity;
	bool cameraLocked = false;
	
	void Init(Admin* a, MovementMode m = MOVEMENT_MODE_FLYING);
	void Update();
};

#endif //GAME_CONTROLLER_H