#pragma once
#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "../defines.h"

struct Admin;
struct Movement;

enum MovementMode_{
	MovementMode_Flying,
	MovementMode_Walking,
	MovementMode_Swimming,
}; typedef u32 MovementMode;

struct Controller{
	Admin* admin;
	MovementMode mode;
	Movement* playermove;
	
	f32 mouseSensitivity;
	bool cameraLocked = false;
	
	void Init(Admin* a, MovementMode m = MovementMode_Flying);
	void Update();
};

#endif //GAME_CONTROLLER_H