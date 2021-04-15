#include "PhysicsSystem.h"
#include "../../core/console.h"
#include "../../utils/PhysicsWorld.h"
#include "../../math/Math.h"
#include "../../geometry/Geometry.h"

#include "../Transform.h"
#include "../components/Physics.h"
#include "../components/Collider.h"
#include "../components/AudioSource.h"

/////////////////////
//// integration ////
/////////////////////

struct PhysicsTuple { 
	Transform* transform	= nullptr; 
	Physics* physics		= nullptr; 
	Collider* collider		= nullptr;
	PhysicsTuple(Transform* transform, Physics* physics, Collider* collider) : transform(transform), physics(physics), collider(collider) {}
};

inline std::vector<PhysicsTuple> GetPhysicsTuples(EntityAdmin* admin) {
	std::vector<PhysicsTuple> out;
	for(auto& e : admin->entities) {
		Transform* transform = &e.transform;
		Physics*   physics   = nullptr;
		Collider* collider   = nullptr;
		
		for(Component* c : e.components) {
			if(Physics* phy = dynamic_cast<Physics*>(c)) { physics = phy; }
			if(Collider* col = dynamic_cast<Collider*>(c)) { collider = col; }
		}
		if(physics) {
			out.push_back(PhysicsTuple(transform, physics, collider));
		}
	}
	return out;
}

//TODO(delle,Ph) look into bettering this physics tick
//https://gafferongames.com/post/physics_in_3d/
inline void PhysicsTick(PhysicsTuple& t, PhysicsWorld* pw, Time* time, float gravity) {
	//// translation ////
	
	//add input forces
	t.physics->inputVector.normalize();
	t.physics->AddForce(nullptr, t.physics->inputVector);
	t.physics->inputVector = Vector3::ZERO;
	
	//add gravity TODO(,sushi) make this a var and toggle later
	t.physics->AddForce(nullptr, Vector3(0, gravity, 0));
	
	//add temp air friction force
	t.physics->AddFrictionForce(nullptr, pw->frictionAir);
	
	//sum up forces to calculate acceleration
	Vector3 netForce;
	for(auto& f : t.physics->forces) {
		netForce += f;
	}
	t.physics->acceleration = netForce / t.physics->mass * 50;
	
	//update linear movement and clamp it to min/max velocity
	if (!t.physics->isStatic) {
		t.physics->velocity += t.physics->acceleration * time->fixedDeltaTime;
		float velMag = t.physics->velocity.mag();
		if (velMag > pw->maxVelocity) {
			t.physics->velocity /= velMag;
			t.physics->velocity *= pw->maxVelocity;
		}
		else if (velMag < pw->minVelocity) {
			t.physics->velocity = Vector3::ZERO;
			t.physics->acceleration = Vector3::ZERO;
		}
		t.physics->position += t.physics->velocity * time->fixedDeltaTime;
	}
	
	
	//// rotation ////
	
	//make fake rotational friction
	if(t.physics->rotVelocity != Vector3::ZERO) {
		t.physics->rotAcceleration = Vector3(t.physics->rotVelocity.x > 0 ? -1 : 1, t.physics->rotVelocity.y > 0 ? -1 : 1, t.physics->rotVelocity.z > 0 ? -1 : 1) * pw->frictionAir * t.physics->mass * 100;
	}
	
	//update rotational movement and scuffed vector rotational clamping
	t.physics->rotVelocity += t.physics->rotAcceleration * time->fixedDeltaTime;
	//if(t.physics->rotVelocity.x > pw->maxRotVelocity) {
	//	t.physics->rotVelocity.x = pw->maxRotVelocity;
	//} else if(t.physics->rotVelocity.x < -pw->maxRotVelocity) {
	//	t.physics->rotVelocity.x = -pw->maxRotVelocity;
	//} else if(abs(t.physics->rotVelocity.x) < pw->minRotVelocity) {
	//	t.physics->rotVelocity.x = 0;
	//	t.physics->rotAcceleration.x = 0;
	//}
	//if(t.physics->rotVelocity.y > pw->maxRotVelocity) {
	//	t.physics->rotVelocity.y = pw->maxRotVelocity;
	//} else if(t.physics->rotVelocity.y < -pw->maxRotVelocity) {
	//	t.physics->rotVelocity.y = -pw->maxRotVelocity;
	//} else if(abs(t.physics->rotVelocity.y) < pw->minRotVelocity) {
	//	t.physics->rotVelocity.y = 0;
	//	t.physics->rotAcceleration.y = 0;
	//}
	//if(t.physics->rotVelocity.z > pw->maxRotVelocity) {
	//	t.physics->rotVelocity.z = pw->maxRotVelocity;
	//} else if(t.physics->rotVelocity.z < -pw->maxRotVelocity) {
	//	t.physics->rotVelocity.z = -pw->maxRotVelocity;
	//} else if(abs(t.physics->rotVelocity.z) < pw->minRotVelocity) {
	//	t.physics->rotVelocity.z = 0;
	//	t.physics->rotAcceleration.z = 0;
	//}
	t.physics->rotation += t.physics->rotVelocity * time->fixedDeltaTime;
	
	//reset accelerations
	t.physics->forces.clear();
}

/////////////////////
//// collisions  ////
/////////////////////

Matrix4 LocalToWorldInertiaTensor(Physics* physics, Matrix3 inertiaTensor) {
	Matrix4 inverseTransformation = Matrix4::TransformationMatrix(physics->position, physics->rotation, Vector3::ONE).Inverse();
	return inverseTransformation.Transpose() * inertiaTensor.To4x4() * inverseTransformation;
}

inline void AABBAABBCollision(Physics* obj1, AABBCollider* obj1Col, Physics* obj2, AABBCollider* obj2Col) {
	//ERROR("AABB-AABB collision not implemented in PhysicsSystem.cpp");
	std::vector<Vector3> obj1ps;
	std::vector<Vector3> obj2ps;
	
	obj1ps.reserve(8); obj2ps.reserve(8);
	
	//oh bruh
	obj1ps.push_back(obj1->position + obj1Col->halfDims); obj2ps.push_back(obj2->position + obj2Col->halfDims);
	obj1ps.push_back(obj1->position + obj1Col->halfDims.xInvert()); obj2ps.push_back(obj2->position + obj2Col->halfDims.xInvert());
	obj1ps.push_back(obj1->position + obj1Col->halfDims.yInvert()); obj2ps.push_back(obj2->position + obj2Col->halfDims.yInvert());
	obj1ps.push_back(obj1->position + obj1Col->halfDims.zInvert()); obj2ps.push_back(obj2->position + obj2Col->halfDims.zInvert());
	obj1ps.push_back(obj1->position + obj1Col->halfDims.xInvert().yInvert()); obj2ps.push_back(obj2->position + obj2Col->halfDims.xInvert().yInvert());
	obj1ps.push_back(obj1->position + obj1Col->halfDims.xInvert().zInvert()); obj2ps.push_back(obj2->position + obj2Col->halfDims.xInvert().zInvert());
	obj1ps.push_back(obj1->position + obj1Col->halfDims.yInvert().zInvert()); obj2ps.push_back(obj2->position + obj2Col->halfDims.yInvert().zInvert());
	obj1ps.push_back(obj1->position + obj1Col->halfDims.xInvert().yInvert().zInvert());obj2ps.push_back(obj2->position + obj2Col->halfDims.xInvert().yInvert().zInvert());
	
	//calculate min and max values over each axis
	float max = std::numeric_limits<float>::max();
	
	float xmax1 = -max; float ymax1 = -max; float zmax1 = -max;
	float xmax2 = -max; float ymax2 = -max; float zmax2 = -max;
	
	float xmin1 = max; float ymin1 = max; float zmin1 = max;
	float xmin2 = max; float ymin2 = max; float zmin2 = max;
	
	for (int i = 0; i < 8; i++) {
		if      (obj1ps[i].x > xmax1) xmax1 = obj1ps[i].x;
		else if (obj1ps[i].x < xmin1) xmin1 = obj1ps[i].x; 
		if      (obj1ps[i].y > ymax1) ymax1 = obj1ps[i].y;
		else if (obj1ps[i].y < ymin1) ymin1 = obj1ps[i].y;
		if      (obj1ps[i].z > zmax1) zmax1 = obj1ps[i].z;
		else if (obj1ps[i].z < zmin1) zmin1 = obj1ps[i].z;
		
		if      (obj2ps[i].x > xmax2) xmax2 = obj2ps[i].x;
		else if (obj2ps[i].x < xmin2) xmin2 = obj2ps[i].x;
		if      (obj2ps[i].y > ymax2) ymax2 = obj2ps[i].y;
		else if (obj2ps[i].y < ymin2) ymin2 = obj2ps[i].y;
		if      (obj2ps[i].z > zmax2) zmax2 = obj2ps[i].z;
		else if (obj2ps[i].z < zmin2) zmin2 = obj2ps[i].z;
	}
	
	if (//check if overlapping
		(xmin1 <= xmax2 && xmax1 >= xmin2) &&
		(ymin1 <= ymax2 && ymax1 >= ymin2) &&
		(zmin1 <= zmax2 && zmax1 >= zmin2)) {
		
		float xover, yover, zover;
		
		//we need to know which box is in front over each axis so
		//the overlap is correct
		if (xmax1 < xmax2) xover = xmax1 - xmin2;
		else               xover = xmax2 - xmin1;
		if (ymax1 < ymax2) yover = ymax1 - ymin2;
		else               yover = ymax2 - ymin1;
		if (zmax1 < zmax2) zover = zmax1 - zmin2;
		else               zover = zmax2 - zmin1;
		
		//TODO( sushi,So) find a nicer way to determine how loud a collision sound is 
		//obj1->entity->GetComponent<AudioSource>()->RequestPlay(obj1->velocity.mag() + obj2->velocity.mag());
		
		bool xf = false; bool yf = false; bool zf = false;
		
		//static resolution
		if (xover < yover && xover < zover) {
			if(!obj1->isStatic) obj1->position.x += xover / 2;
			if(!obj2->isStatic) obj2->position.x -= xover / 2;
			xf = true;
		}	
		else if (yover < xover && yover < zover) {
			if(!obj1->isStatic) obj1->position.y += yover / 2;
			if(!obj2->isStatic) obj2->position.y -= yover / 2;
			yf = true;
		}
		else if (zover < yover && zover < xover) {
			if(!obj1->isStatic) obj1->position.z += zover / 2;
			if(!obj2->isStatic) obj2->position.z -= zover / 2;
			zf = true;
		}
		
		//dynamic resolution
		//TODO(sushi, Ph) finish implementing AABB dynamic collision
		//get relative velocity
		Vector3 relvel = obj2->velocity - obj1->velocity;
		
		
	}
	
}

inline void AABBSphereCollision(Physics* aabb, AABBCollider* aabbCol, Physics* sphere, SphereCollider* sphereCol) {
	Vector3 aabbPoint = Geometry::ClosestPointOnAABB(aabb->position, aabbCol->halfDims, sphere->position);
	Vector3 vectorBetween = aabbPoint - sphere->position; //sphere towards aabb
	float distanceBetween = vectorBetween.mag();
	if(distanceBetween < sphereCol->radius) {
		if(!aabbCol->isTrigger && !sphereCol->isTrigger) {
			//SUCCESS("collision happened");
			aabb->entity->GetComponent<AudioSource>()->request_play = true;
			//static resolution
			if (aabbPoint == sphere->position) { 
				//NOTE if the closest point is the same, the vector between will have no direction; this 
				//is supposed to be a remedy to that by offsetting in the direction between thier centers
				vectorBetween = aabb->position - sphere->position;
			}
			float overlap = .5f * (sphereCol->radius - distanceBetween);
			Vector3 normal = -vectorBetween.normalized();
			vectorBetween = -normal * overlap;
			aabb->position += vectorBetween;
			sphere->position -= vectorBetween;
			
			//dynamic resolution
			Matrix4 sphereInertiaTensorInverse = LocalToWorldInertiaTensor(sphere, sphereCol->inertiaTensor).Inverse();
			Vector3 ra = sphere->position + Geometry::ClosestPointOnSphere(sphere->position, sphereCol->radius, aabbPoint);
			Vector3 sphereAngularVelocityChange = normal.cross(ra);
			sphereAngularVelocityChange *= sphereInertiaTensorInverse;
			float inverseMassA = 1.f / sphere->mass;
			float scalar = inverseMassA + sphereAngularVelocityChange.cross(ra).dot(normal);
			
			Matrix4 aabbInertiaTensorInverse = LocalToWorldInertiaTensor(aabb, aabbCol->inertiaTensor).Inverse();
			Vector3 rb = aabb->position + aabbPoint;
			Vector3 aabbAngularVelocityChange = normal.cross(rb);
			aabbAngularVelocityChange *= aabbInertiaTensorInverse;
			float inverseMassB = 1.f / aabb->mass; 
			scalar += inverseMassB + aabbAngularVelocityChange.cross(rb).dot(normal);
			
			float coefRest = (aabb->elasticity + sphere->elasticity); //this is completely unfounded is science :)
			float impulseMod = (coefRest + 1) * (sphere->velocity - aabb->velocity).mag(); //this too :)
			Vector3 impulse = normal * impulseMod;
			aabb->AddImpulse(sphere, -impulse);
			sphere->rotVelocity -= sphereAngularVelocityChange;
			//aabb->entity->rotVelocity -= aabbAngularVelocityChange; //we dont do this because AABB shouldnt rotate
		}
	}
}

inline void AABBBoxCollision(EntityAdmin* admin, Physics* aabb, AABBCollider* aabbCol, Physics* box, BoxCollider* boxCol) {
	ERROR("AABB-Box collision not implemented in PhysicsSystem.cpp");
}

inline void SphereSphereCollision(EntityAdmin* admin, Physics* sphere, SphereCollider* sphereCol, Physics* other, SphereCollider* otherCol) {
	ERROR("Sphere-Sphere collision not implemented in PhysicsSystem.cpp");
}

inline void SphereBoxCollision(EntityAdmin* admin, Physics* sphere, SphereCollider* sphereCol, Physics* box, BoxCollider* boxCol) {
	ERROR("Sphere-Box collision not implemented in PhysicsSystem.cpp");
}

inline void BoxBoxCollision(EntityAdmin* admin, Physics* box, BoxCollider* boxCol, Physics* other, BoxCollider* otherCol) {
	ERROR("Box-Box collision not implemented in PhysicsSystem.cpp");
}

//NOTE make sure you are using the right physics component, because the collision 
//functions dont check that the provided one matches the tuple
inline void CheckCollision(EntityAdmin* admin, PhysicsTuple& tuple, PhysicsTuple& other) {
	if(AABBCollider* col = dynamic_cast<AABBCollider*>(tuple.collider)) {
		if(AABBCollider* col2 = dynamic_cast<AABBCollider*>(other.collider)) {
			AABBAABBCollision(tuple.physics, col, other.physics, col2);
		} else if(SphereCollider* col2 = dynamic_cast<SphereCollider*>(other.collider)) {
			AABBSphereCollision(tuple.physics, col, other.physics, col2);
		} else if(BoxCollider* col2 = dynamic_cast<BoxCollider*>(other.collider)) {
			AABBBoxCollision(admin, tuple.physics, col, other.physics, col2);
		}
	} else if(SphereCollider* col = dynamic_cast<SphereCollider*>(tuple.collider)) {
		if(AABBCollider* col2 = dynamic_cast<AABBCollider*>(other.collider)) {
			AABBSphereCollision(other.physics, col2, tuple.physics, col);
		} else if(SphereCollider* col2 = dynamic_cast<SphereCollider*>(other.collider)) {
			SphereSphereCollision(admin, tuple.physics, col, other.physics, col2);
		} else if(BoxCollider* col2 = dynamic_cast<BoxCollider*>(other.collider)) {
			SphereBoxCollision(admin, tuple.physics, col, other.physics, col2);
		}
	} else if(BoxCollider* col = dynamic_cast<BoxCollider*>(tuple.collider)) {
		if(AABBCollider* col2 = dynamic_cast<AABBCollider*>(other.collider)) {
			AABBBoxCollision(admin, other.physics, col2, tuple.physics, col);
		} else if(SphereCollider* col2 = dynamic_cast<SphereCollider*>(other.collider)) {
			SphereBoxCollision(admin, other.physics, col2, tuple.physics, col);
		} else if(BoxCollider* col2 = dynamic_cast<BoxCollider*>(other.collider)) {
			BoxBoxCollision(admin, tuple.physics, col, other.physics, col2);
		}
	}
}

inline void CollisionTick(EntityAdmin* admin, std::vector<PhysicsTuple>& tuples, PhysicsTuple& t){
	if(t.collider) {
		for(auto& tuple : tuples) {
			if(&t != &tuple && tuple.collider && t.collider->collisionLayer == tuple.collider->collisionLayer) {
				CheckCollision(admin, t, tuple);
			}
		}
	}
}

//////////////////////////
//// system functions ////
//////////////////////////

void PhysicsSystem::Update() {
	Time* time = DengTime;
	PhysicsWorld* pw = admin->physicsWorld;
	
	std::vector<PhysicsTuple> tuples = GetPhysicsTuples(admin);
	
	//update physics extra times per frame if frame time delta is larger than physics time delta
	while(time->fixedAccumulator >= time->fixedDeltaTime) {
		for(auto& t : tuples) {
			PhysicsTick(t, pw, time, gravity);
			CollisionTick(admin, tuples, t);
		}
		time->fixedAccumulator -= time->fixedDeltaTime;
		time->fixedTotalTime += time->fixedDeltaTime;
	}
	
	//interpolate between new physics position and old transform position by the leftover time
	float alpha = time->fixedAccumulator / time->fixedDeltaTime;
	for(auto& t : tuples) {
		t.transform->prevPosition = t.transform->position;
		t.transform->prevRotation = t.transform->rotation;
		t.transform->position = t.transform->position * (1.f - alpha) + t.physics->position * alpha;
		t.transform->rotation = t.transform->rotation * (1.f - alpha) + t.physics->rotation * alpha;
		//t.transform->rotation = Quaternion::QuatSlerp(t.transform->rotation, t.transform->prevRotation, alpha).ToVector3();
		
		//t.transform->rotation *= Matrix4::RotationMatrixAroundPoint(t.transform->position, t.transform->rotation*(1.f - alpha) + t.physics->rotation*alpha);
		//TODO(delle,Ph) look into better rotational interpolation once we switch to quaternions
	}
}
