#pragma once
#ifndef SYSTEM_PHYSICS_H
#define SYSTEM_PHYSICS_H

#include "System.h"

struct PhysicsSystem : public System {
	void Update() override;
};

#endif //SYSTEM_PHYSICS_H