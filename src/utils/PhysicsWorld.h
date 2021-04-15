#pragma once
#ifndef DESHI_PHYSICSWORLD_H
#define DESHI_PHYSICSWORLD_H

enum struct CollisionDetectionMode {
	DISCRETE, /*CONTINUOUS, GJK,*/ NONE
};

enum struct IntegrationMode {
	/*RK4, VERLET,*/ EULER
};

struct PhysicsWorld {
	IntegrationMode integrationMode      = IntegrationMode::EULER;
	CollisionDetectionMode collisionMode = CollisionDetectionMode::DISCRETE;
	
	float gravity     = -9.81f;
	float frictionAir = 0.01f; //TODO(delle,Ph) this should depend on object shape
	
	float maxVelocity    = 100.f;
	float minVelocity    = 0.15f;
	float maxRotVelocity = 360.f; //per axis in degrees
	float minRotVelocity = 1.f;
	
	PhysicsWorld() {}
	PhysicsWorld(IntegrationMode im, CollisionDetectionMode cm, float gravity = 9.81f, float frictionAir = 0.01f) {
		this->integrationMode = im;
		this->collisionMode = cm;
		this->gravity = gravity;
		this->frictionAir = frictionAir;
	}
};

#endif //DESHI_PHYSICSWORLD_H