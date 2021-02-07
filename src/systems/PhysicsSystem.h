#pragma once
#include "System.h"

struct Vector3;
struct Physics;

struct PhysicsSystem : public System {
	static inline void AddForce(Physics* creator, Physics* target, Vector3 force);
	static inline void AddInput(Physics* target, Vector3 input);
	static inline void AddFrictionForce(Physics* creator, Physics* target, float frictionCoef, float gravity = 9.81f);
	static inline void AddImpulse(Physics* creator, Physics* target, Vector3 impulse, bool ignoreMass = false);

	void Init() override;
	void Update() override;
};