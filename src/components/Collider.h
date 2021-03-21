#pragma once
#ifndef COMPONENT_COLLIDER_H
#define COMPONENT_COLLIDER_H

#include "Component.h"
#include "../math/Matrix3.h"
#include "../math/Vector3.h"
#include "../math/InertiaTensors.h"
#include "Physics.h"

struct Command;

struct Collider : public Component {
	Matrix3 inertiaTensor;
	i8 collisionLayer = 0;
	
	bool isTrigger = false;
	Command* command = nullptr; //TODO(p,delle) implement trigger colliders
	
};

//rotatable box
struct BoxCollider : public Collider {
	Vector3 halfDims; //half dimensions, entity's position to the bounding box's locally positive corner
	
	BoxCollider(Entity* e, Vector3 halfDimensions, float mass, i8 collisionLayer = 0);
	BoxCollider(Entity* e, Vector3 halfDimensions, float mass, bool isTrigger, Command* command, i8 collisionLayer = 0);
};

//axis-aligned bounding box
struct AABBCollider : public Collider {
	Vector3 halfDims; //half dimensions, entity's position to the bounding box's locally positive corner
	
	AABBCollider(Entity* e, Vector3 halfDimensions, float mass, i8 collisionLayer = 0);
	AABBCollider(Entity* e, Vector3 halfDimensions, float mass, bool isTrigger, Command* command, i8 collisionLayer = 0);
	
};

struct SphereCollider : public Collider {
	float radius;
	
	SphereCollider(Entity* e, float radius, float mass, i8 collisionLayer = 0);
	SphereCollider(Entity* e, float radius, float mass, bool isTrigger, Command* command, i8 collisionLayer = 0);
};

//TODO(p,delle) implement convexPolyCollider
//TODO(p,delle) implement capsuleCollider
//TODO(p,delle) implement cylinder collider
//TODO(p,delle) implement complex collider (collider list)

#endif //COMPONENT_COLLIDER_H