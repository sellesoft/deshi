#pragma once
//#include "../components/Transform.h"
//#include "../components/Physics.h"
//#include "../math/Math.h"

enum struct CollisionDetectionMode {
	DISCRETE, /*CONTINUOUS, GJK,*/ NONE
};

enum struct IntegrationMode {
	/*RK4, VERLET,*/ EULER
};

//struct PhysEntity {
//	Vector3 position;
//	Vector3 velocity;
//	Vector3 acceleration;
//
//	Vector3 rotation;
//	Vector3 rotVelocity;
//	Vector3 rotAcceleration;
//};

//TODO(delle,Ph) look into maybe having physics here instead

struct PhysicsWorld {
	//std::map<EntityID, PhysEntity> entityTuples;

	IntegrationMode integrationMode			= IntegrationMode::EULER;
	CollisionDetectionMode collisionMode	= CollisionDetectionMode::DISCRETE;

	float maxVelocity = 100.f;
	float minVelocity = 0.15f;

	float maxRotVelocity =  360.f; //per axis in degrees
	float minRotVelocity = 1.f;

	float gravity		= 9.81f;
	float frictionAir	= 0.01f; //TODO(delle,Ph) this should depend on object shape

	PhysicsWorld() {
		this->integrationMode	= IntegrationMode::EULER;
		this->collisionMode		= CollisionDetectionMode::DISCRETE;
		this->gravity			= 9.81f;
		this->frictionAir		= 0.01f;
	}

	PhysicsWorld(IntegrationMode im, CollisionDetectionMode cm, 
				float gravity = 9.81f, float frictionAir = 0.01f) {
		this->integrationMode = im;
		this->collisionMode = cm;
		this->gravity = gravity;
		this->frictionAir = frictionAir;
	}
};