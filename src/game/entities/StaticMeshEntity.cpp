#include "StaticMeshEntity.h"
#include "../admin.h"
#include "../../core/console.h"

#include "../components/MeshComp.h"
#include "../components/Physics.h"
#include "../components/Collider.h"

StaticMesh::StaticMesh(Transform transform, const char* name){
	this->transform = transform;
	if (name) cpystr(this->name, name, 63);
	
	mesh = new MeshComp();
	physics = new Physics(transform.position, transform.rotation, Vector3::ZERO, Vector3::ZERO, Vector3::ZERO, Vector3::ZERO, 
						  .5f, 1.f, true, true, false, .3f, .42f, false);
	collider = 0;
	AddComponents({ physics, mesh });
}

StaticMesh::StaticMesh(u32 meshID, u32 colliderType, f32 mass, Transform transform, const char* name){
	this->transform = transform;
	if (name) cpystr(this->name, name, 63);
	
	mesh = new MeshComp(meshID);
	physics = new Physics(transform.position, transform.rotation, Vector3::ZERO, Vector3::ZERO, Vector3::ZERO, Vector3::ZERO, 
						  .5f, mass, true, true, false, .3f, .42f, false);
	switch(colliderType){
		case ColliderType_Box:    collider = new BoxCollider(Vector3(1,1,1), mass); break;
		case ColliderType_Sphere: collider = new SphereCollider(1.f, mass); break;
		case ColliderType_AABB:   collider = new AABBCollider(Vector3(1,1,1), mass); break;
		default: ERROR("Invalid component type in StaticMesh creation: ", ColliderTypeStrings[colliderType]); collider = 0; break;
	}
	AddComponents({ collider, mesh, physics });

}