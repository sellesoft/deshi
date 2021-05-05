#include "Collider.h"

#include "../../EntityAdmin.h"
#include "../../math/InertiaTensors.h"
#include "../../math/math.h"
#include "../../scene/Model.h"

//////////////////////
//// Box Collider ////
//////////////////////

BoxCollider::BoxCollider(Vector3 halfDimensions, float mass, u32 collisionLayer, Command* command, b32 nocollide) {
	cpystr(name, "BoxCollider", 63);
	comptype = ComponentType_Collider;
	
	this->type = ColliderType_Box;
	this->collisionLayer = collisionLayer;
	this->noCollide = nocollide;
	this->command = command;
	this->halfDims = halfDimensions;
	this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
	
}

BoxCollider::BoxCollider(Vector3 halfDimensions, Matrix3& tensor, u32 collisionLayer, Command* command, b32 nocollide) {
	cpystr(name, "BoxCollider", 63);
	comptype = ComponentType_Collider;
	
	this->type = ColliderType_Box;
	this->collisionLayer = collisionLayer;
	this->inertiaTensor = tensor;
	this->noCollide = nocollide;
	this->command = command;
	this->halfDims = halfDimensions;
}

std::vector<char> BoxCollider::Save() {
	std::vector<char> out;
	return out;
}

void BoxCollider::RecalculateTensor(f32 mass) {
	inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
}

void BoxCollider::Load(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = -1;
	u32 layer = -1;
	mat3 tensor{};
	vec3 halfDimensions{};
	
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load box collider component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID); continue;
		}
		
		memcpy(&layer,          data+cursor, sizeof(u32));  cursor += sizeof(u32);
		memcpy(&tensor,         data+cursor, sizeof(mat3)); cursor += sizeof(mat3);
		memcpy(&halfDimensions, data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		BoxCollider* c = new BoxCollider(halfDimensions, tensor, layer, nullptr);
		admin->entities[entityID].AddComponent(c);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
		c->Init(admin);
	}
}

///////////////////////
//// AABB Collider ////
///////////////////////

AABBCollider::AABBCollider(Mesh* mesh, float mass, u32 collisionLayer, Command* command, b32 nocollide) {
	cpystr(name, "AABBCollider", 63);
	comptype = ComponentType_Collider;
	
	this->type = ColliderType_AABB;
	this->collisionLayer = collisionLayer;
	this->noCollide = nocollide;
	this->command = command;
	
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

AABBCollider::AABBCollider(Vector3 halfDimensions, float mass, u32 collisionLayer, Command* command, b32 nocollide) {
	cpystr(name, "AABBCollider", 63);
	comptype = ComponentType_Collider;
	
	this->type = ColliderType_AABB;
	this->collisionLayer = collisionLayer;
	this->noCollide = nocollide;
	this->command = command;
	this->halfDims = halfDimensions;
	this->inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
}

AABBCollider::AABBCollider(Vector3 halfDimensions, Matrix3& tensor, u32 collisionLayer, Command* command, b32 nocollide) {
	cpystr(name, "AABBCollider", 63);
	comptype = ComponentType_Collider;
	
	this->type = ColliderType_AABB;
	this->collisionLayer = collisionLayer;
	this->inertiaTensor = tensor;
	this->noCollide = nocollide;
	this->command = command;
	this->halfDims = halfDimensions;
}

void AABBCollider::RecalculateTensor(f32 mass) {
	inertiaTensor = InertiaTensors::SolidCuboid(2 * abs(halfDims.x), 2 * abs(halfDims.y), 2 * abs(halfDims.z), mass);
}

std::vector<char> AABBCollider::Save() {
	std::vector<char> out;
	return out;
}

void AABBCollider::Load(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = -1;
	u32 layer = -1;
	mat3 tensor{};
	vec3 halfDimensions{};
	
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load aabb collider component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID); continue;
		}
		
		memcpy(&layer, data+cursor,          sizeof(u32));  cursor += sizeof(u32);
		memcpy(&tensor, data+cursor,         sizeof(mat3)); cursor += sizeof(mat3);
		memcpy(&halfDimensions, data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		AABBCollider* c = new AABBCollider(halfDimensions, tensor, layer, nullptr);
		admin->entities[entityID].AddComponent(c);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
		c->Init(admin);
	}
}

/////////////////////////
//// Sphere Collider ////
/////////////////////////

SphereCollider::SphereCollider(float radius, float mass, u32 collisionLayer, Command* command, b32 nocollide) {
	cpystr(name, "SphereCollider", 63);
	comptype = ComponentType_Collider;
	
	this->type = ColliderType_Sphere;
	this->collisionLayer = collisionLayer;
	this->noCollide = nocollide;
	this->command = command;
	this->radius = radius;
	this->inertiaTensor = InertiaTensors::SolidSphere(radius, mass);
}

SphereCollider::SphereCollider(float radius, Matrix3& tensor, u32 collisionLayer, Command* command, b32 nocollide) {
	cpystr(name, "SphereCollider", 63);
	comptype = ComponentType_Collider;
	
	this->type = ColliderType_Sphere;
	this->collisionLayer = collisionLayer;
	this->inertiaTensor = tensor;
	this->noCollide = nocollide;
	this->command = command;
	this->radius = radius;
}

void SphereCollider::RecalculateTensor(f32 mass) {
	inertiaTensor = InertiaTensors::SolidSphere(radius, mass);
}

std::vector<char> SphereCollider::Save() {
	std::vector<char> out;
	return out;
}

void SphereCollider::Load(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = -1;
	u32 layer = -1;
	mat3 tensor{};
	f32 radius = 0.f;
	
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load sphere collider component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID); continue;
		}
		
		memcpy(&layer,  data+cursor, sizeof(u32));  cursor += sizeof(u32);
		memcpy(&tensor, data+cursor, sizeof(mat3)); cursor += sizeof(mat3);
		memcpy(&radius, data+cursor, sizeof(f32));  cursor += sizeof(f32);
		SphereCollider* c = new SphereCollider(radius, tensor, layer, nullptr);
		admin->entities[entityID].AddComponent(c);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
		c->Init(admin);
	}
}


////////////////////////////
//// Landscape Collider ////
////////////////////////////


LandscapeCollider::LandscapeCollider(Mesh* mesh, u32 collisionleyer, Command* command, b32 nocollide) {
	cpystr(name, "LandscapeCollider", 63);
	comptype = ComponentType_Collider;
	
	this->type = ColliderType_Landscape;
	this->collisionLayer = collisionLayer;
	this->noCollide = nocollide;
	this->command = command;
	
	if (!mesh) {
		ERROR("Null mesh passed during LandscapeCollider creation");
		return;
	}
	if (!mesh->batchArray.size() || !mesh->vertexCount) {
		ERROR("Mesh passed during LandscapeCollider creation had no vertices");
		return;
	}
	
	//NOTE this will be optimized later i just need it to work :)
	//currently its just checking all triangles against a single one until it finds a connection to it.
	//to move on to the next triangle it has to find 3 neighbors to the current one,
	//this attempts to make it so connections spread out evenly and don't form lines and other odd shapes
	//that may not be necessary, but im going to implement it anyways
	
	//a way to optimize this probably would be to do something like, check for connections against
	//a couple of sets of triangles and their connections and if two connections come together
	//and joining them would keep then under the connection limit, then join them
	//not sure if that would be faster or not.
	
	//find triangle connections so that we can generate AABBs for each set
	std::vector<std::vector<Vector3>> va; //vertex array of triangles
	std::vector<std::vector<Vector3>> vaperm; //vertex array of triangles that will be used to generate AABBS
	
	std::vector<std::vector<u32>> ia; //index array that stores all connections to the first triangle 
	std::vector<u32> iq; //index queue, queues triangles to check for connections against
	
	//gather all triangles
	for (auto& b : mesh->batchArray) {
		for (int i = 0; i < b.indexArray.size(); i += 3) {
			va.push_back(
						 std::vector<Vector3>{
							 b.vertexArray[b.indexArray[i]].pos,
							 b.vertexArray[b.indexArray[i + 1]].pos,
							 b.vertexArray[b.indexArray[i + 2]].pos
						 }
						 );
		}
	}
	
	//copy temp vertex array to permanent one
	vaperm = va;
	
	auto eqtoany = [](std::vector<Vector3> v, Vector3 t) {
		for (Vector3 a : v) if (t == a) return true;
		return false;
	};
	
	Vector3 ct[3]; //current triangle we're checking against
	iq.push_back(0);
	bool begin = true;
	for (int i = 0; i < va.size(); i++) {
		
		//get next triangle to check against
		if (begin) { 
			ct[0] = va[iq[0]][0]; ct[1] = va[iq[0]][1]; ct[2] = va[iq[0]][2];
			va.erase(va.begin() + i); iq.erase(iq.begin()); 
			ia[0].push_back(i); begin = false;  continue;
		}
		
		//check if two vertices match the current triangles vertices
		if (eqtoany(va[i], ct[0]) && eqtoany(va[i], ct[1]) ||
			eqtoany(va[i], ct[1]) && eqtoany(va[i], ct[2]) ||
			eqtoany(va[i], ct[2]) && eqtoany(va[i], ct[0])) {
			
			
			
		}
	}
}


////////////////////////////
///// Complex Collider /////
////////////////////////////

ComplexCollider::ComplexCollider(Mesh* mesh, u32 collisionleyer, Command* command, b32 nocollide) {
	
}