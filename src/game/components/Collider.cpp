#include "Collider.h"

#include "../admin.h"
#include "../../math/InertiaTensors.h"
#include "../../math/math.h"
#include "../../core/model.h"
#include "../../core/console.h"


void Collider::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR_LOC("LoadDESH not setup");
}


//////////////////////
//// Box Collider ////
//////////////////////

BoxCollider::BoxCollider(vec3 halfDimensions, float mass, u32 collisionLayer, Event event, bool nocollide) {
	type = ComponentType_Collider;
	this->shape = ColliderShape_Box;
	this->collLayer = collisionLayer;
	this->noCollide = nocollide;
	this->event = event;
	this->halfDims = halfDimensions;
	this->tensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
	
}

BoxCollider::BoxCollider(vec3 halfDimensions, mat3& tensor, u32 collisionLayer, Event event, bool nocollide) {
	type = ComponentType_Collider;
	this->type = ColliderShape_Box;
	this->collLayer = collisionLayer;
	this->tensor = tensor;
	this->noCollide = nocollide;
	this->event = event;
	this->halfDims = halfDimensions;
}

void BoxCollider::RecalculateTensor(f32 mass) {
	tensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
}

std::string BoxCollider::SaveTEXT(){
	return TOSTDSTRING("\n>collider"
					"\nshape     ",ColliderShapeStrings[shape],
					"\nhalf_dims (",halfDims.x,",",halfDims.y,",",halfDims.z,")"
					"\n");
}


///////////////////////
//// AABB Collider ////
///////////////////////


AABBCollider::AABBCollider(Mesh* mesh, float mass, u32 collisionLayer, Event _event, bool nocollide) {
	type = ComponentType_Collider;
	shape = ColliderShape_AABB;
	collLayer = collisionLayer;
	noCollide = nocollide;
	event = _event;
	
	if(!mesh){
		halfDims = vec3::ZERO;
		ERROR("Null mesh passed during AABBCollider creation");
		return;
	}
	if(!mesh->vertexCount) {
		ERROR("Mesh passed during AABBCollider creation had no vertices");
		return;
	}
	
	vec3 abs_min = mesh->aabbMin.absV();
	vec3 abs_max = mesh->aabbMax.absV();
	halfDims.x = (abs_min.x > abs_max.x) ? abs_min.x : abs_max.x;
	halfDims.y = (abs_min.y > abs_max.y) ? abs_min.y : abs_max.y;
	halfDims.z = (abs_min.z > abs_max.z) ? abs_min.z : abs_max.z;
	tensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
}

AABBCollider::AABBCollider(vec3 halfDimensions, float mass, u32 collisionLayer, Event event, bool nocollide) {
	type = ComponentType_Collider;
	this->shape = ColliderShape_AABB;
	this->collLayer = collisionLayer;
	this->noCollide = nocollide;
	this->event = event;
	this->halfDims = halfDimensions;
	this->tensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
}

AABBCollider::AABBCollider(vec3 halfDimensions, mat3& tensor, u32 collisionLayer, Event event, bool nocollide) {
	type = ComponentType_Collider;
	this->shape = ColliderShape_AABB;
	this->collLayer = collisionLayer;
	this->tensor = tensor;
	this->noCollide = nocollide;
	this->event = event;
	this->halfDims = halfDimensions;
}

void AABBCollider::RecalculateTensor(f32 mass) {
	tensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
}

std::string AABBCollider::SaveTEXT(){
	return TOSTDSTRING("\n>collider"
					"\nshape     ",ColliderShapeStrings[shape],
					"\nhalf_dims (",halfDims.x,",",halfDims.y,",",halfDims.z,")"
					"\n");
}


/////////////////////////
//// Sphere Collider ////
/////////////////////////


SphereCollider::SphereCollider(float radius, float mass, u32 collisionLayer, Event event, bool nocollide) {
	type = ComponentType_Collider;
	this->shape = ColliderShape_Sphere;
	this->collLayer = collisionLayer;
	this->noCollide = nocollide;
	this->event = event;
	this->radius = radius;
	this->tensor = InertiaTensors::SolidSphere(radius, mass);
}

SphereCollider::SphereCollider(float radius, mat3& tensor, u32 collisionLayer, Event event, bool nocollide) {
	type = ComponentType_Collider;
	this->shape = ColliderShape_Sphere;
	this->collLayer = collisionLayer;
	this->tensor = tensor;
	this->noCollide = nocollide;
	this->event = event;
	this->radius = radius;
}

void SphereCollider::RecalculateTensor(f32 mass) {
	tensor = InertiaTensors::SolidSphere(radius, mass);
}

std::string SphereCollider::SaveTEXT(){
	return TOSTDSTRING("\n>collider"
					"\nshape  ",ColliderShapeStrings[shape],
					"\nradius ",radius,
					"\n");
}


////////////////////////////
//// Landscape Collider ////
////////////////////////////


LandscapeCollider::LandscapeCollider(Mesh* mesh, u32 collisionLayer, Event event, bool nocollide) {
	type = ComponentType_Collider;
	this->shape = ColliderShape_Landscape;
	this->collLayer = collisionLayer;
	this->noCollide = nocollide;
	this->event = event;
	
	if (!mesh) {
		ERROR("Null mesh passed during LandscapeCollider creation");
		return;
	}
	if (!mesh->vertexCount) {
		ERROR("Mesh passed during LandscapeCollider creation had no vertices");
		return;
	}
	
}

std::string LandscapeCollider::SaveTEXT(){
	ERROR_LOC("LandscapeCollider saving not setup");
	return TOSTDSTRING("\n>collider"
					"\nshape ",ColliderShapeStrings[shape],
					"\n");
}


////////////////////////////
///// Complex Collider /////
////////////////////////////


ComplexCollider::ComplexCollider(Mesh* mesh, u32 collisionLayer, Event event, bool nocollide) {
	type = ComponentType_Collider;
	this->shape = ColliderShape_Complex;
	this->collLayer = collisionLayer;
	this->noCollide = nocollide;
	this->event = event;
	this->mesh = mesh;
}

std::string ComplexCollider::SaveTEXT(){
	ERROR_LOC("ComplexCollider saving not setup");
	return TOSTDSTRING("\n>collider"
					"\nshape ",ColliderShapeStrings[shape],
					"\n");
}