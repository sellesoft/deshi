#include "Collider.h"

#include "../../EntityAdmin.h"
#include "../../math/InertiaTensors.h"
#include "../../math/math.h"
#include "../../scene/Model.h"

//////////////////////
//// Box Collider ////
//////////////////////

BoxCollider::BoxCollider(Vector3 halfDimensions, float mass, u32 collisionLayer, Command* command) {
	cpystr(name, "BoxCollider", 63);
	sortid = 2;
	
	this->type = ColliderType_Box;
	this->collisionLayer = collisionLayer;
	this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
	this->command = command;
	this->halfDims = halfDimensions;
}

BoxCollider::BoxCollider(Vector3 halfDimensions, Matrix3& tensor, u32 collisionLayer, Command* command) {
	cpystr(name, "BoxCollider", 63);
	sortid = 2;
	
	this->type = ColliderType_Box;
	this->collisionLayer = collisionLayer;
	this->inertiaTensor = tensor;
	this->command = command;
	this->halfDims = halfDimensions;
}

void BoxCollider::Load(std::vector<Entity>& entities, const char* data, u32& cursor, u32 count){
	u32 entityID = -1;
	u32 layer = -1;
	mat3 tensor{};
	vec3 halfDimensions{};
	
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); 
		cursor += sizeof(u32);
		if(entityID >= entities.size()) {
			ERROR("Failed to load box collider component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		
		memcpy(&layer, data+cursor,          sizeof(u32));  cursor += sizeof(u32);
		memcpy(&tensor, data+cursor,         sizeof(mat3)); cursor += sizeof(mat3);
		memcpy(&halfDimensions, data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		entities[entityID].AddComponent(new BoxCollider(halfDimensions, tensor, layer, nullptr));
	}
}

///////////////////////
//// AABB Collider ////
///////////////////////

//TODO(delle) test this
AABBCollider::AABBCollider(Mesh* mesh, float mass, u32 collisionLayer, Command* command) {
	cpystr(name, "AABBCollider", 63);
	sortid = 3;
	
	this->type = ColliderType_AABB;
	this->collisionLayer = collisionLayer;
	this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
	this->command = command;
	
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

AABBCollider::AABBCollider(Vector3 halfDimensions, float mass, u32 collisionLayer, Command* command) {
	cpystr(name, "AABBCollider", 63);
	sortid = 3;
	
	this->type = ColliderType_AABB;
	this->collisionLayer = collisionLayer;
	this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
	this->command = command;
	this->halfDims = halfDimensions;
}

AABBCollider::AABBCollider(Vector3 halfDimensions, Matrix3& tensor, u32 collisionLayer, Command* command) {
	cpystr(name, "AABBCollider", 63);
	sortid = 3;
	
	this->type = ColliderType_AABB;
	this->collisionLayer = collisionLayer;
	this->inertiaTensor = tensor;
	this->command = command;
	this->halfDims = halfDimensions;
}

void AABBCollider::Load(std::vector<Entity>& entities, const char* data, u32& cursor, u32 count){
	u32 entityID = -1;
	u32 layer = -1;
	mat3 tensor{};
	vec3 halfDimensions{};
	
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); 
		cursor += sizeof(u32);
		if(entityID >= entities.size()) {
			ERROR("Failed to load aabb collider component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		
		memcpy(&layer, data+cursor,          sizeof(u32));  cursor += sizeof(u32);
		memcpy(&tensor, data+cursor,         sizeof(mat3)); cursor += sizeof(mat3);
		memcpy(&halfDimensions, data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		entities[entityID].AddComponent(new AABBCollider(halfDimensions, tensor, layer, nullptr));
	}
}

/////////////////////////
//// Sphere Collider ////
/////////////////////////

SphereCollider::SphereCollider(float radius, float mass, u32 collisionLayer, Command* command) {
	cpystr(name, "SphereCollider", 63);
	sortid = 4;
	
	this->type = ColliderType_Sphere;
	this->collisionLayer = collisionLayer;
	this->inertiaTensor = InertiaTensors::SolidSphere(radius, mass);
	this->command = command;
	this->radius = radius;
}

SphereCollider::SphereCollider(float radius, Matrix3& tensor, u32 collisionLayer, Command* command) {
	cpystr(name, "SphereCollider", 63);
	sortid = 4;
	
	this->type = ColliderType_Sphere;
	this->collisionLayer = collisionLayer;
	this->inertiaTensor = tensor;
	this->command = command;
	this->radius = radius;
}

void SphereCollider::Load(std::vector<Entity>& entities, const char* data, u32& cursor, u32 count){
	u32 entityID = -1;
	u32 layer = -1;
	mat3 tensor{};
	f32 radius = 0.f;
	
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); 
		cursor += sizeof(u32);
		if(entityID >= entities.size()) {
			ERROR("Failed to load sphere collider component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		
		memcpy(&layer, data+cursor,  sizeof(u32));  cursor += sizeof(u32);
		memcpy(&tensor, data+cursor, sizeof(mat3)); cursor += sizeof(mat3);
		memcpy(&radius, data+cursor, sizeof(f32));  cursor += sizeof(f32);
		entities[entityID].AddComponent(new SphereCollider(radius, tensor, layer, nullptr));
	}
}