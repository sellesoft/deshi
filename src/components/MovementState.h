#pragma once
#include "dsh_Component.h"

#define MOVEMENT_NOCLIP		0
#define MOVEMENT_CLIP		1
#define MOVEMENT_FLYING		2
#define MOVEMENT_WALKING	4
#define MOVEMENT_WATER		8

//NOTE: you can combine these with | and compare them with &

struct MovementState : public Component {
	uint32 movementState;

	MovementState() {
		movementState = MOVEMENT_NOCLIP | MOVEMENT_FLYING;
	}

	MovementState(uint32 state) {
		movementState = state;
	}
};