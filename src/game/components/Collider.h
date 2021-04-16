#pragma once
#ifndef COMPONENT_COLLIDER_H
#define COMPONENT_COLLIDER_H

#include "Component.h"
#include "../../math/Matrix.h"
#include "../../math/Vector.h"
#include "../../math/InertiaTensors.h"
#include "Physics.h"

struct Command;
struct Mesh;

struct Collider : public Component {
	Matrix3 inertiaTensor;
	i8 collisionLayer = 0;
	
	Command* command = nullptr; //TODO(delle,Ph) implement trigger colliders
};

//rotatable box
struct BoxCollider : public Collider {
	Vector3 halfDims; //half dimensions, entity's position to the bounding box's locally positive corner
	
	BoxCollider(Vector3 halfDimensions, float mass, i8 collisionLayer = 0, Command* command = nullptr);
};

//axis-aligned bounding box
struct AABBCollider : public Collider {
	Vector3 halfDims; //half dimensions, entity's position to the bounding box's locally positive corner
	
	AABBCollider(Mesh* mesh, float mass, i8 collisionLayer = 0, Command* command = nullptr);
	AABBCollider(Vector3 halfDimensions, float mass, i8 collisionLayer = 0, Command* command = nullptr);
	
};

struct SphereCollider : public Collider {
	float radius;
	
	SphereCollider(float radius, float mass, i8 collisionLayer = 0, Command* command = nullptr);
};

//TODO(delle,Ph) implement convexPolyCollider
//TODO(delle,Ph) implement capsuleCollider
//TODO(delle,Ph) implement cylinder collider
//TODO(delle,Ph) implement complex collider (collider list)

#endif //COMPONENT_COLLIDER_H