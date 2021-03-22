#pragma once
#ifndef COMPONENT_CONTROLLER_H
#define COMPONENT_CONTROLLER_H

#include "Component.h"

enum MovementMode : u32 {
	MOVEMENT_MODE_FLYING,
	MOVEMENT_MODE_WALKING,
	MOVEMENT_MODE_SWIMMING,
};

struct Controller : public Component {
	MovementMode mode;
	float mouseSensitivity = 2.5f;
	bool noclip = true;
	
	Controller(EntityAdmin* a, MovementMode s = MOVEMENT_MODE_FLYING);
	
	void Update() override;
};

#endif //COMPONENT_CONTROLLER_H