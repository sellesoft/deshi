#include "Collider.h"

#include "../../math/math.h"
#include "../../scene/Model.h"

//////////////////////
//// Box Collider ////
//////////////////////

BoxCollider::BoxCollider(Vector3 halfDimensions, float mass, i8 collisionLayer, Command* command) {
	cpystr(name, "BoxCollider", 63);
	sortid = 2;
	
	this->halfDims = halfDimensions;
	this->collisionLayer = collisionLayer;
	if(command){
		this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.x), 2 * abs(halfDims.x), mass);
	}else{
		this->inertiaTensor = Matrix3::IDENTITY;
	}
}

///////////////////////
//// AABB Collider ////
///////////////////////

AABBCollider::AABBCollider(Mesh* mesh, float mass, i8 collisionLayer, Command* command) {
	cpystr(name, "AABBCollider", 63);
	sortid = 3;
	
	this->collisionLayer = collisionLayer;
	if(command){
		this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.x), 2 * abs(halfDims.x), mass);
	}else{
		this->inertiaTensor = Matrix3::IDENTITY;
	}
	
	if(!mesh) {
		this->halfDims = Vector3::ZERO;
		ERROR("Null mesh passed during AABBCollider creation");
		return;
	}
	if(!mesh->batchArray.size() || !mesh->vertexCount) {
		this->halfDims = Vector3::ZERO;
		ERROR("Mesh passed during AABBCollider creation had no vertices");
		return;
	}
	
	Vector3 min = mesh->batchArray[0].vertexArray[0].pos;
	Vector3 max = mesh->batchArray[0].vertexArray[0].pos;
	for(Batch& batch : mesh->batchArray){
		for(Vertex& v : batch.vertexArray){
			if(v.pos.x < min.x) min.x = v.pos.x;
			if(v.pos.y < min.y) min.y = v.pos.y;
			if(v.pos.z < min.z) min.z = v.pos.z;
			if(v.pos.x > max.x) max.x = v.pos.x;
			if(v.pos.y > max.y) max.y = v.pos.y;
			if(v.pos.z > max.z) max.z = v.pos.z;
		}
	}
	
	Vector3 mid = min.midpoint(max);
	if(mid == Vector3::ZERO){
		this->halfDims.x = (min.x > max.x) ? min.x : max.x;
		this->halfDims.y = (min.y > max.y) ? min.y : max.y;
		this->halfDims.z = (min.z > max.z) ? min.z : max.z;
	}else{
		float midMag = mid.mag();
		Vector3 minN = ((min / midMag) - mid).absV(); //normalize to midpoint then center on origin
		Vector3 maxN = ((max / midMag) - mid).absV(); //then abs for comparison
		this->halfDims.x = (minN.x > maxN.x) ? minN.x : maxN.x;
		this->halfDims.y = (minN.y > maxN.y) ? minN.y : maxN.y;
		this->halfDims.z = (minN.z > maxN.z) ? minN.z : maxN.z;
		
		this->halfDims *= midMag; //then unnormalize
	}
}

AABBCollider::AABBCollider(Vector3 halfDimensions, float mass, i8 collisionLayer, Command* command) {
	cpystr(name, "AABBCollider", 63);
	sortid = 3;
	
	this->halfDims = halfDimensions;
	this->collisionLayer = collisionLayer;
	if(command){
		this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.x), 2 * abs(halfDims.x), mass);
	}else{
		this->inertiaTensor = Matrix3::IDENTITY;
	}
}

/////////////////////////
//// Sphere Collider ////
/////////////////////////

SphereCollider::SphereCollider(float radius, float mass, i8 collisionLayer, Command* command) {
	cpystr(name, "SphereCollider", 63);
	sortid = 4;
	
	this->radius = radius;
	this->collisionLayer = collisionLayer;
	if(command){
		this->inertiaTensor = InertiaTensors::SolidSphere(radius, mass);
	}else{
		this->inertiaTensor = Matrix3::IDENTITY;
	}
}
