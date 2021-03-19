#pragma once
#ifndef COMPONENT_MOVEMENTSTATE_H
#define COMPONENT_MOVEMENTSTATE_H

#include "Component.h"

#define MOVEMENT_NOCLIP		0
#define MOVEMENT_CLIP		1
#define MOVEMENT_FLYING		2
#define MOVEMENT_WALKING	4
#define MOVEMENT_WATER		8

//NOTE: you can combine these with | and compare them with &

struct MovementState : public Component {
	uint32 movementState;
	
	MovementState(EntityAdmin* a);
	MovementState(uint32 state);
	
	void Update() override;
};

#endif //COMPONENT_MOVEMENTSTATE_H