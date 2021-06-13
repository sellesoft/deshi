#include "Collider.h"

#include "../admin.h"
#include "../../math/InertiaTensors.h"
#include "../../math/math.h"
#include "../../scene/Model.h"

//////////////////////
//// Box Collider ////
//////////////////////

BoxCollider::BoxCollider(Vector3 halfDimensions, float mass, u32 collisionLayer, Event event, b32 nocollide) {
	admin = g_admin;
	cpystr(name, "BoxCollider", DESHI_NAME_SIZE);
	comptype = ComponentType_Collider;
	sender = new Sender();
	this->type = ColliderType_Box;
	this->collisionLayer = collisionLayer;
	this->noCollide = nocollide;
	this->event = event;
	this->halfDims = halfDimensions;
	this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
	
}

BoxCollider::BoxCollider(Vector3 halfDimensions, Matrix3& tensor, u32 collisionLayer, Event event, b32 nocollide) {
	admin = g_admin;
	cpystr(name, "BoxCollider", 63);
	comptype = ComponentType_Collider;
	sender = new Sender();
	this->type = ColliderType_Box;
	this->collisionLayer = collisionLayer;
	this->inertiaTensor = tensor;
	this->noCollide = nocollide;
	this->event = event;
	this->halfDims = halfDimensions;
}

void BoxCollider::RecalculateTensor(f32 mass) {
	inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
}

std::string BoxCollider::SaveTEXT(){
	return TOSTRING("\n>collider"
					"\ntype      box"
					"\nhalf_dims (",halfDims.x,",",halfDims.y,",",halfDims.z,")"
					"\n");
}

void BoxCollider::LoadDESH(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = -1, compID = 0xFFFFFFFF, event = 0xFFFFFFFF;
	u32 layer = -1;
	mat3 tensor{};
	vec3 halfDimensions{};
	
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load box collider component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID); continue;
		}
		memcpy(&compID, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		memcpy(&event, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		
		memcpy(&layer,          data+cursor, sizeof(u32));  cursor += sizeof(u32);
		memcpy(&tensor,         data+cursor, sizeof(mat3)); cursor += sizeof(mat3);
		memcpy(&halfDimensions, data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		BoxCollider* c = new BoxCollider(halfDimensions, tensor, layer);
		EntityAt(entityID)->AddComponent(c);
		c->SetCompID(compID);
		c->SetEvent(event);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
	}
}

///////////////////////
//// AABB Collider ////
///////////////////////

AABBCollider::AABBCollider(Mesh* mesh, float mass, u32 collisionLayer, Event event, b32 nocollide) {
	admin = g_admin;
	cpystr(name, "AABBCollider", DESHI_NAME_SIZE);
	comptype = ComponentType_Collider;
	sender = new Sender();
	this->type = ColliderType_AABB;
	this->collisionLayer = collisionLayer;
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
	
	this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
}

AABBCollider::AABBCollider(Vector3 halfDimensions, float mass, u32 collisionLayer, Event event, b32 nocollide) {
	admin = g_admin;
	cpystr(name, "AABBCollider", DESHI_NAME_SIZE);
	comptype = ComponentType_Collider;
	sender = new Sender();
	this->type = ColliderType_AABB;
	this->collisionLayer = collisionLayer;
	this->noCollide = nocollide;
	this->event = event;
	this->halfDims = halfDimensions;
	this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
}

AABBCollider::AABBCollider(Vector3 halfDimensions, Matrix3& tensor, u32 collisionLayer, Event event, b32 nocollide) {
	admin = g_admin;
	cpystr(name, "AABBCollider", DESHI_NAME_SIZE);
	comptype = ComponentType_Collider;
	sender = new Sender();
	this->type = ColliderType_AABB;
	this->collisionLayer = collisionLayer;
	this->inertiaTensor = tensor;
	this->noCollide = nocollide;
	this->event = event;
	this->halfDims = halfDimensions;
}

void AABBCollider::RecalculateTensor(f32 mass) {
	inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
}

std::string AABBCollider::SaveTEXT(){
	return TOSTRING("\n>collider"
					"\ntype      aabb"
					"\nhalf_dims (",halfDims.x,",",halfDims.y,",",halfDims.z,")"
					"\n");
}

void AABBCollider::LoadDESH(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = -1, compID = 0xFFFFFFFF, event = 0xFFFFFFFF;
	u32 layer = -1;
	mat3 tensor{};
	vec3 halfDimensions{};
	
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load aabb collider component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID); continue;
		}
		memcpy(&compID, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		memcpy(&event, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		
		memcpy(&layer, data+cursor,          sizeof(u32));  cursor += sizeof(u32);
		memcpy(&tensor, data+cursor,         sizeof(mat3)); cursor += sizeof(mat3);
		memcpy(&halfDimensions, data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		AABBCollider* c = new AABBCollider(halfDimensions, tensor, layer);
		EntityAt(entityID)->AddComponent(c);
		c->SetCompID(compID);
		c->SetEvent(event);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
	}
}

/////////////////////////
//// Sphere Collider ////
/////////////////////////

SphereCollider::SphereCollider(float radius, float mass, u32 collisionLayer, Event event, b32 nocollide) {
	admin = g_admin;
	cpystr(name, "SphereCollider", DESHI_NAME_SIZE);
	comptype = ComponentType_Collider;
	sender = new Sender();
	this->type = ColliderType_Sphere;
	this->collisionLayer = collisionLayer;
	this->noCollide = nocollide;
	this->event = event;
	this->radius = radius;
	this->inertiaTensor = InertiaTensors::SolidSphere(radius, mass);
}

SphereCollider::SphereCollider(float radius, Matrix3& tensor, u32 collisionLayer, Event event, b32 nocollide) {
	admin = g_admin;
	cpystr(name, "SphereCollider", DESHI_NAME_SIZE);
	comptype = ComponentType_Collider;
	sender = new Sender();
	this->type = ColliderType_Sphere;
	this->collisionLayer = collisionLayer;
	this->inertiaTensor = tensor;
	this->noCollide = nocollide;
	this->event = event;
	this->radius = radius;
}

void SphereCollider::RecalculateTensor(f32 mass) {
	inertiaTensor = InertiaTensors::SolidSphere(radius, mass);
}

std::string SphereCollider::SaveTEXT(){
	return TOSTRING("\n>collider"
					"\ntype   sphere"
					"\nradius ",radius,
					"\n");
}

void SphereCollider::LoadDESH(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = -1, compID = 0xFFFFFFFF, event = 0xFFFFFFFF;
	u32 layer = -1;
	mat3 tensor{};
	f32 radius = 0.f;
	
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load sphere collider component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID); continue;
		}
		memcpy(&compID, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		memcpy(&event, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		
		memcpy(&layer,  data+cursor, sizeof(u32));  cursor += sizeof(u32);
		memcpy(&tensor, data+cursor, sizeof(mat3)); cursor += sizeof(mat3);
		memcpy(&radius, data+cursor, sizeof(f32));  cursor += sizeof(f32);
		SphereCollider* c = new SphereCollider(radius, tensor, layer);
		c->SetCompID(compID);
		c->SetEvent(event);
		EntityAt(entityID)->AddComponent(c);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
	}
}


////////////////////////////
//// Landscape Collider ////
////////////////////////////


LandscapeCollider::LandscapeCollider(Mesh* mesh, u32 collisionleyer, Event event, b32 nocollide) {
	admin = g_admin;
	cpystr(name, "LandscapeCollider", DESHI_NAME_SIZE);
	comptype = ComponentType_Collider;
	sender = new Sender();
	this->type = ColliderType_Landscape;
	this->collisionLayer = collisionLayer;
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
	return TOSTRING("\n>collider"
					"\ntype landscape"
					"\n");
}


////////////////////////////
///// Complex Collider /////
////////////////////////////

ComplexCollider::ComplexCollider(Mesh* mesh, u32 collisionleyer, Event event, b32 nocollide) {
	admin = g_admin;
	cpystr(name, "ComplexCollider", DESHI_NAME_SIZE);
	comptype = ComponentType_Collider;
	this->type = ColliderType_Complex;


	this->mesh = mesh;
}

std::string ComplexCollider::SaveTEXT(){
	ERROR_LOC("ComplexCollider saving not setup");
	return TOSTRING("\n>collider"
					"\ntype complex"
					"\n");
}