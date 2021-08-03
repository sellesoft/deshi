#include "Collider.h"

#include "../admin.h"
#include "../../math/InertiaTensors.h"
#include "../../math/math.h"
#include "../../scene/Model.h"


void Collider::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR_LOC("LoadDESH not setup");
}


//////////////////////
//// Box Collider ////
//////////////////////

BoxCollider::BoxCollider(Vector3 halfDimensions, float mass, u32 collisionLayer, Event event, bool nocollide) {
	type = ComponentType_Collider;
	this->shape = ColliderShape_Box;
	this->collLayer = collisionLayer;
	this->noCollide = nocollide;
	this->event = event;
	this->halfDims = halfDimensions;
	this->tensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
	
}

BoxCollider::BoxCollider(Vector3 halfDimensions, Matrix3& tensor, u32 collisionLayer, Event event, bool nocollide) {
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


AABBCollider::AABBCollider(Mesh* mesh, float mass, u32 collisionLayer, Event event, bool nocollide) {
	type = ComponentType_Collider;
	this->shape = ColliderShape_AABB;
	this->collLayer = collisionLayer;
	this->noCollide = nocollide;
	this->event = event;
	
	if (!mesh) {
		this->halfDims = Vector3::ZERO;
		ERROR("Null mesh passed during AABBCollider creation");
		return;
	}
	if (!mesh->batchArray.size() || !mesh->vertexCount) {
		this->halfDims = Vector3::ZERO;
		ERROR("Mesh passed during AABBCollider creation had no vertices");
		return;
	}
	
	Vector3 min = mesh->batchArray[0].vertexArray[0].pos;
	Vector3 max = mesh->batchArray[0].vertexArray[0].pos;
	for (Batch& batch : mesh->batchArray) {
		for (Vertex& v : batch.vertexArray) {
			if (v.pos.x < min.x) min.x = v.pos.x;
			if (v.pos.y < min.y) min.y = v.pos.y;
			if (v.pos.z < min.z) min.z = v.pos.z;
			if (v.pos.x > max.x) max.x = v.pos.x;
			if (v.pos.y > max.y) max.y = v.pos.y;
			if (v.pos.z > max.z) max.z = v.pos.z;
		}
	}
	
	Vector3 mid = min.midpoint(max);
	if (mid == Vector3::ZERO) {
		this->halfDims.x = (min.x > max.x) ? min.x : max.x;
		this->halfDims.y = (min.y > max.y) ? min.y : max.y;
		this->halfDims.z = (min.z > max.z) ? min.z : max.z;
	}
	else {
		float midMag = mid.mag();
		Vector3 minN = ((min / midMag) - mid).absV(); //normalize to midpoint then center on origin
		Vector3  maxN = ((max / midMag) - mid).absV(); //then abs for comparison
		this->halfDims.x = (minN.x > maxN.x) ? minN.x : maxN.x;
		this->halfDims.y = (minN.y > maxN.y) ? minN.y : maxN.y;
		this->halfDims.z = (minN.z > maxN.z) ? minN.z : maxN.z;
		
		this->halfDims *= midMag; //then unnormalize
	}
	
	this->tensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
}

AABBCollider::AABBCollider(Vector3 halfDimensions, float mass, u32 collisionLayer, Event event, bool nocollide) {
	type = ComponentType_Collider;
	this->shape = ColliderShape_AABB;
	this->collLayer = collisionLayer;
	this->noCollide = nocollide;
	this->event = event;
	this->halfDims = halfDimensions;
	this->tensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
}

AABBCollider::AABBCollider(Vector3 halfDimensions, Matrix3& tensor, u32 collisionLayer, Event event, bool nocollide) {
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

SphereCollider::SphereCollider(float radius, Matrix3& tensor, u32 collisionLayer, Event event, bool nocollide) {
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
	if (!mesh->batchArray.size() || !mesh->vertexCount) {
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