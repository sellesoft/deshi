#include "Collider.h"

#include "../../EntityAdmin.h"



////////////////////////////////////////////////////////////
// Box Collider
//////////////////////////////////////////////////////////



BoxCollider::BoxCollider(Entity* e, Vector3 halfDimensions, float mass, i8 collisionLayer) {
	this->entity = e;
	this->halfDims = halfDimensions;
	this->collisionLayer = collisionLayer;
	this->isTrigger = false;
	this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.x), 2 * abs(halfDims.x), mass);
	name = "BoxCollider";
}

BoxCollider::BoxCollider(Entity* e, Vector3 halfDimensions, float mass, bool isTrigger, Command* command, i8 collisionLayer) {
	this->entity = e;
	this->halfDims = halfDimensions;
	this->collisionLayer = collisionLayer;
	this->isTrigger = isTrigger;
	this->command = command;
	if (!isTrigger) {
		this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.x), 2 * abs(halfDims.x), mass);
	}
	name = "BoxCollider";
}



////////////////////////////////////////////////////////////
// AABB Collider
//////////////////////////////////////////////////////////



AABBCollider::AABBCollider(Entity* e, Vector3 halfDimensions, float mass, i8 collisionLayer) {
	this->entity = e;
	this->halfDims = halfDimensions;
	this->collisionLayer = collisionLayer;
	this->isTrigger = false;
	this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.x), 2 * abs(halfDims.x), mass);
	name = "AABBCollider";
}

AABBCollider::AABBCollider(Entity* e, Vector3 halfDimensions, float mass, bool isTrigger, Command* command, i8 collisionLayer) {
	this->entity = e;
	this->halfDims = halfDimensions;
	this->collisionLayer = collisionLayer;
	this->isTrigger = isTrigger;
	this->command = command;
	if (!isTrigger) {
		this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.x), 2 * abs(halfDims.x), mass);
	}
	name = "AABBCollider";
}



////////////////////////////////////////////////////////////
// Sphere Collider
//////////////////////////////////////////////////////////



SphereCollider::SphereCollider(Entity* e, float radius, float mass, i8 collisionLayer) {
	this->entity = e;
	this->radius = radius;
	this->collisionLayer = collisionLayer;
	this->isTrigger = false;
	this->inertiaTensor = InertiaTensors::SolidSphere(radius, mass);
	name = "SphereCollider";
}

SphereCollider::SphereCollider(Entity* e, float radius, float mass, bool isTrigger, Command* command, i8 collisionLayer) {
	this->entity = e;
	this->radius = radius;
	this->collisionLayer = collisionLayer;
	this->isTrigger = isTrigger;
	this->command = command;
	if (!isTrigger) {
		this->inertiaTensor = InertiaTensors::SolidSphere(radius, mass);
	}
	name = "SphereCollider";
}