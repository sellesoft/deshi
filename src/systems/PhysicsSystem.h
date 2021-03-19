#pragma once
#include "System.h"

struct Vector3;
struct Physics;

struct PhysicsSystem : public System {
	void Init() override;
	void Update() override;
};