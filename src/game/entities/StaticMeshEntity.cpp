#include "StaticMeshEntity.h"
#include "../admin.h"
#include "../components/MeshComp.h"
#include "../components/Physics.h"
#include "../components/Collider.h"
#include "../../core/console.h"
#include "../../core/model.h"

StaticMesh::StaticMesh(Transform _transform, const char* _name){
	type = EntityType_StaticMesh;
	transform = _transform;
	if(_name) cpystr(name, _name, DESHI_NAME_SIZE);
	
	model    = new ModelInstance();
	physics  = new Physics(transform.position, transform.rotation, vec3::ZERO, vec3::ZERO, vec3::ZERO, vec3::ZERO, 
						   .5f, 1.f, true, true, false, .3f, .42f);
	collider = new BoxCollider(vec3(1,1,1), 1.f);
	AddComponents({ model, physics, collider });
}

StaticMesh::StaticMesh(Model* _model, ColliderShape colliderShape, f32 mass, Transform _transform, const char* _name){
	type = EntityType_StaticMesh;
	transform = _transform;
	if(_name) cpystr(name, _name, DESHI_NAME_SIZE);
	
	model   = new ModelInstance(_model);
	physics = new Physics(transform.position, transform.rotation, vec3::ZERO, vec3::ZERO, vec3::ZERO, vec3::ZERO, 
						  .5f, mass, true, true, false, .3f, .42f);
	switch(colliderShape){
		case ColliderShape_Box:    collider = new BoxCollider(vec3(1,1,1), mass); break;
		case ColliderShape_Sphere: collider = new SphereCollider(1.f, mass); break;
		case ColliderShape_AABB:   collider = new AABBCollider(vec3(1,1,1), mass); break;
		default: ERROR("Invalid component type in StaticMesh creation: ", ColliderShapeStrings[colliderShape]); collider = 0; break;
	}
	AddComponents({ model, physics, collider });
}