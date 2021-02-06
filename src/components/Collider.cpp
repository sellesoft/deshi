#pragma once
#include "Component.h"
#include "../math/Matrix3.h"
#include "../math/Vector3.h"
#include "../math/InertiaTensors.h"
#include "Physics.h"

struct Command;

struct Collider : public Component {
	Matrix3 inertiaTensor;
	int8 collisionLayer = 0;

	bool isTrigger = false;
	Command* command = nullptr; //TODO(p,delle) implement trigger colliders
};

//rotatable box
struct BoxCollider : public Collider {
	Vector3 halfDims; //half dimensions, entity's position to the bounding box's locally positive corner

	BoxCollider(Entity* e, Vector3 halfDimensions, float mass, int8 collisionLayer = 0) {
		this->entity = e;
		this->halfDims = halfDimensions;
		this->collisionLayer = collisionLayer;
		this->isTrigger = false;
		this->inertiaTensor = InertiaTensors::SolidCuboid(2*abs(halfDims.x), 2*abs(halfDims.x), 2*abs(halfDims.x), mass);
	}

	BoxCollider(Entity* e, Vector3 halfDimensions, float mass, bool isTrigger, Command* command, int8 collisionLayer = 0) {
		this->entity = e;
		this->halfDims = halfDimensions;
		this->collisionLayer = collisionLayer;
		this->isTrigger = isTrigger;
		this->command = command;
		if(!isTrigger) {
			this->inertiaTensor = InertiaTensors::SolidCuboid(2*abs(halfDims.x), 2*abs(halfDims.x), 2*abs(halfDims.x), mass);
		}
	}
};

//axis-aligned bounding box
struct AABBCollider : public Collider {
	Vector3 halfDims; //half dimensions, entity's position to the bounding box's locally positive corner

	AABBCollider(Entity* e, Vector3 halfDimensions, float mass, int8 collisionLayer = 0) {
		this->entity = e;
		this->halfDims = halfDimensions;
		this->collisionLayer = collisionLayer;
		this->isTrigger = false;
		this->inertiaTensor = InertiaTensors::SolidCuboid(2*abs(halfDims.x), 2*abs(halfDims.x), 2*abs(halfDims.x), mass);
	}

	AABBCollider(Entity* e, Vector3 halfDimensions, float mass, bool isTrigger, Command* command, int8 collisionLayer = 0) {
		this->entity = e;
		this->halfDims = halfDimensions;
		this->collisionLayer = collisionLayer;
		this->isTrigger = isTrigger;
		this->command = command;
		if(!isTrigger) {
			this->inertiaTensor = InertiaTensors::SolidCuboid(2*abs(halfDims.x), 2*abs(halfDims.x), 2*abs(halfDims.x), mass);
		}
	}
};

struct SphereCollider : public Collider {
	float radius;

	SphereCollider(Entity* e, float radius, float mass, int8 collisionLayer = 0) {
		this->entity = e;
		this->radius= radius;
		this->collisionLayer = collisionLayer;
		this->isTrigger = false;
		this->inertiaTensor = InertiaTensors::SolidSphere(radius, mass);
	}

	SphereCollider(Entity* e, float radius, float mass, bool isTrigger, Command* command, int8 collisionLayer = 0) {
		this->entity = e;
		this->radius= radius;
		this->collisionLayer = collisionLayer;
		this->isTrigger = isTrigger;
		this->command = command;
		if(!isTrigger) {
			this->inertiaTensor = InertiaTensors::SolidSphere(radius, mass);
		}
	}
};

//TODO(p,delle) implement convexPolyCollider
//TODO(p,delle) implement capsuleCollider
//TODO(p,delle) implement cylinder collider
//TODO(p,delle) implement complex collider (collider list)