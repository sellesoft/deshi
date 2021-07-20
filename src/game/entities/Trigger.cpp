#include "Trigger.h"
#include "../components/Collider.h"
#include "../components/Physics.h"
#include "../../scene/Model.h"

Trigger::Trigger(Transform transform) {
	cpystr(name, "trigger", DESHI_NAME_SIZE);
	type = EntityType_Trigger;
	this->transform = transform;
	
}

Trigger::Trigger(Transform transform, Collider* collider) {
	cpystr(name, "trigger", DESHI_NAME_SIZE);
	type = EntityType_Trigger;
	this->transform = transform;
	
	this->collider = collider;
	physics = new Physics(transform.position, transform.rotation,
						  Vector3::ZERO, Vector3::ZERO, Vector3::ZERO, Vector3::ZERO, 0, 1, true);
	collider->noCollide = true;
	
	switch (collider->shape) {
		case ColliderShape_AABB: {
			mesh = Mesh::CreateBox(((AABBCollider*)collider)->halfDims);
		}break;
		case ColliderShape_Sphere: {
			
		}break;
		case ColliderShape_Complex: {
			
		}break;
	}
	
	AddComponents({ physics, collider });
}

void Trigger::Init() {
	
}