#include "Trigger.h"
#include "../components/Collider.h"
#include "../components/Physics.h"
#include "../../scene/Model.h"

Trigger::Trigger(Transform _transform, const char* _name) {
	cpystr(name, (_name) ? _name : "trigger", DESHI_NAME_SIZE); 
	type = EntityType_Trigger;
	transform = _transform;
}

Trigger::Trigger(Transform _transform, Collider* _collider, const char* _name) {
	cpystr(name, (_name) ? _name : "trigger", DESHI_NAME_SIZE); 
	type = EntityType_Trigger;
	transform = _transform;
	collider = _collider;
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