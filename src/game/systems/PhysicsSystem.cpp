#include "PhysicsSystem.h"
#include "../admin.h"
#include "../Event.h"
#include "../components/Physics.h"
#include "../components/Camera.h"
#include "../components/Collider.h"
#include "../components/AudioSource.h"
#include "../components/MeshComp.h"
#include "../components/Movement.h"
#include "../../core/console.h"
#include "../../core/time.h"
#include "../../core/renderer.h" //temporary until we guarentee store trimesh neighbors on them
#include "../../core/window.h"
#include "../../core/scene.h"
#include "../../math/Math.h"
#include "../../geometry/Geometry.h"
#include "../../utils/Command.h"

u32 collCount;

bool breakphys = false;

//counter for debugging where in the physics tick something happens
//if something happens in 2 different ticks etc
u32 physTickCounter = 0;

struct PhysicsTuple { 
	Transform* transform = nullptr; 
	Physics*   physics   = nullptr; 
	Collider*  collider  = nullptr;
	PhysicsTuple(Transform* transform, Physics* physics, Collider* collider) 
		: transform(transform), physics(physics), collider(collider) {}
};

/////////////////////
//// integration ////
/////////////////////

inline std::vector<PhysicsTuple> GetPhysicsTuples(Admin* admin) {
	std::vector<PhysicsTuple> out;
	for(int i = 0; i < admin->entities.size(); i++) {
		if (admin->entities[i]) {
			Transform* transform = &admin->entities[i]->transform;
			Physics* physics = nullptr;
			Collider* collider = nullptr;
			
			for (Component* c : admin->entities[i]->components) {
				if (Physics* phy = dynamic_cast<Physics*>(c)) { physics = phy; continue; }
				if (Collider* col = dynamic_cast<Collider*>(c)) { collider = col; continue; }
			}
			if (physics) {
				out.push_back(PhysicsTuple(transform, physics, collider));
			}
		}
	}
	return out;
}

//TODO(delle,Ph) look into bettering this physics tick
//https://gafferongames.com/post/physics_in_3d/
inline void PhysicsTick(PhysicsTuple& t, PhysicsSystem* ps, Time* time) {
	//// translation ////
	
	//add input forces
	t.physics->inputVector.normalize();
	t.physics->AddForce(nullptr, t.physics->inputVector);
	t.physics->inputVector = Vector3::ZERO;
	
	//add gravity 
	t.physics->acceleration += Vector3(0, -ps->gravity, 0);
	
	//add temp air friction force
	t.physics->AddFrictionForce(nullptr, ps->frictionAir);
	
	//contacts is the various contact states it has with each object while
	//contactState is the overall state of the object, regardless of what object its touching
	bool contactMoving = false;
	bool contactStationary = false;
	for (auto c : t.physics->contacts) {
		if (c.second == ContactMoving) {
			if (t.physics->velocity.mag() > 0.01) {
				
				for (auto& m : t.physics->manifolds) {
					Vector3 norm = m.second.norm.normalized();
					Vector3 vPerpNorm = t.physics->velocity - t.physics->velocity.dot(norm) * norm;
					//account for friction along ang surfaces
					Vector3 weight = Vector3::DOWN * t.physics->mass * ps->gravity;
					float primedweight = weight.dot(norm);
					t.physics->forces.push_back(-vPerpNorm.normalized() * t.physics->kineticFricCoef * primedweight);
				}
				
				//t.physics->AddFrictionForce(nullptr, t.physics->kineticFricCoef, ps->gravity);
			}
			else
				t.physics->velocity = Vector3::ZERO;
			contactMoving = true;
		}
		else if (c.second == ContactStationary) contactStationary = true;
	}
	
	if      (contactMoving)     t.physics->contactState = ContactMoving;
	else if (contactStationary) t.physics->contactState = ContactStationary;
	else                        t.physics->contactState = ContactNONE;
	
	//sum up forces to calculate acceleration
	Vector3 netForce;
	for (auto& f : t.physics->forces) {
		netForce += f;
	}
	t.physics->acceleration += netForce / t.physics->mass;
	
	//update linear movement and clamp it to min/max velocity
	if (!t.physics->staticPosition) {
		t.physics->velocity += t.physics->acceleration * time->fixedDeltaTime;
		float velMag = t.physics->velocity.mag();
		if (velMag > ps->maxVelocity) {
			t.physics->velocity /= velMag;
			t.physics->velocity *= ps->maxVelocity;
		}
		else if (velMag < ps->minVelocity) {
			t.physics->velocity = Vector3::ZERO;
			t.physics->acceleration = Vector3::ZERO;
		}
		t.physics->position += t.physics->velocity * time->fixedDeltaTime;
	}
	
	//// rotation ////
	
	//make fake rotational friction
	if (t.physics->rotVelocity != Vector3::ZERO) {
		t.physics->rotAcceleration = Vector3(t.physics->rotVelocity.x > 0 ? -1 : 1, t.physics->rotVelocity.y > 0 ? -1 : 1, t.physics->rotVelocity.z > 0 ? -1 : 1) * ps->frictionAir * t.physics->mass * 100;
	}
	
	//update rotational movement and scuffed vector rotational clamping
	t.physics->rotVelocity += t.physics->rotAcceleration * time->fixedDeltaTime;
	//if(t.physics->rotVelocity.x > ps->maxRotVelocity) {
	//	t.physics->rotVelocity.x = ps->maxRotVelocity;
	//} else if(t.physics->rotVelocity.x < -ps->maxRotVelocity) {
	//	t.physics->rotVelocity.x = -ps->maxRotVelocity;
	//} else if(abs(t.physics->rotVelocity.x) < ps->minRotVelocity) {
	//	t.physics->rotVelocity.x = 0;
	//	t.physics->rotAcceleration.x = 0;
	//}
	//if(t.physics->rotVelocity.y > ps->maxRotVelocity) {
	//	t.physics->rotVelocity.y = ps->maxRotVelocity;
	//} else if(t.physics->rotVelocity.y < -ps->maxRotVelocity) {
	//	t.physics->rotVelocity.y = -ps->maxRotVelocity;
	//} else if(abs(t.physics->rotVelocity.y) < ps->minRotVelocity) {
	//	t.physics->rotVelocity.y = 0;
	//	t.physics->rotAcceleration.y = 0;
	//}
	//if(t.physics->rotVelocity.z > ps->maxRotVelocity) {
	//	t.physics->rotVelocity.z = ps->maxRotVelocity;
	//} else if(t.physics->rotVelocity.z < -ps->maxRotVelocity) {
	//	t.physics->rotVelocity.z = -ps->maxRotVelocity;
	//} else if(abs(t.physics->rotVelocity.z) < ps->minRotVelocity) {
	//	t.physics->rotVelocity.z = 0;
	//	t.physics->rotAcceleration.z = 0;
	//}
	t.physics->rotation += t.physics->rotVelocity * time->fixedDeltaTime;
	
	//reset forces
	t.physics->forces.clear();
	
	//ImGui::DebugDrawText3(t.physics->position.str().c_str(), t.physics->position, ad->mainCamera, DengWindow->dimensions, Color(180, 150, 130));
	//ImGui::DebugDrawText3(t.physics->velocity.str().c_str(), t.physics->position, ad->mainCamera, DengWindow->dimensions, Color(130, 150, 180), Vector2(0, 20));
	//ImGui::DebugDrawText3(t.physics->acceleration.str().c_str(), t.physics->position, ad->mainCamera, DengWindow->dimensions, Color(150, 130, 180), Vector2(0, 40));
	
	t.physics->acceleration = Vector3::ZERO;
}

/////////////////////
//// collisions  ////
/////////////////////

Matrix4 LocalToWorldInertiaTensor(Physics* physics, Matrix3 inertiaTensor) {
	Matrix4 inverseTransformation = Matrix4::TransformationMatrix(physics->position, physics->rotation, Vector3::ONE).Inverse();
	return inverseTransformation.Transpose() * inertiaTensor.To4x4() * inverseTransformation;
}

bool AABBAABBCollision(Physics* obj1, AABBCollider* obj1Col, Physics* obj2, AABBCollider* obj2Col) {
	vec3 min1 = obj1->position - (obj1Col->halfDims * obj1->entity->transform.scale);
	vec3 max1 = obj1->position + (obj1Col->halfDims * obj1->entity->transform.scale);
	vec3 min2 = obj2->position - (obj2Col->halfDims * obj2->entity->transform.scale);
	vec3 max2 = obj2->position + (obj2Col->halfDims * obj2->entity->transform.scale);
	
	if (//check if overlapping
		(min1.x <= max2.x && max1.x >= min2.x) &&
		(min1.y <= max2.y && max1.y >= min2.y) &&
		(min1.z <= max2.z && max1.z >= min2.z)) {
		
		
		
		
		//triggers and no collision
		if (obj1Col->collided.find(obj2Col) == obj1Col->collided.end()) {
			if (obj1Col->event != 0 && !obj1Col->sentEvent) { obj1Col->sender.SendEvent(obj1Col->event); obj1Col->sentEvent = true; }
			if (obj2Col->event != 0 && !obj2Col->sentEvent) { obj2Col->sender.SendEvent(obj2Col->event); obj2Col->sentEvent = true; }
		}
		
		obj1Col->collided.clear();
		obj2Col->collided.clear();
		
		//store entity
		obj1Col->collided.insert(obj2Col);
		obj2Col->collided.insert(obj1Col);
		
		if(obj1Col->noCollide || obj2Col->noCollide) return false;
		
		float xover, yover, zover;
		
		//we need to know which box is in front over each axis so
		//the overlap is correct
		if (max1.x < max2.x) xover = max1.x - min2.x;
		else                 xover = max2.x - min1.x;
		if (max1.y < max2.y) yover = max1.y - min2.y;
		else                 yover = max2.y - min1.y;
		if (max1.z < max2.z) zover = max1.z - min2.z;
		else                 zover = max2.z - min1.z;
		
		Manifold3 m1;
		Manifold3 m2;
		
		//static resolution
		Vector3 norm;
		if (xover < yover && xover < zover) {
			if(!obj1->staticPosition) obj1->position.x += xover / 2;
			if(!obj2->staticPosition) obj2->position.x -= xover / 2;
			norm = Vector3::LEFT;
		}	
		else if (yover < xover && yover < zover) {
			if(!obj1->staticPosition) obj1->position.y += yover / 2;
			if(!obj2->staticPosition) obj2->position.y -= yover / 2;
			norm = Vector3::DOWN;
		}
		else if (zover < yover && zover < xover) {
			if(!obj1->staticPosition) obj1->position.z += zover / 2;
			if(!obj2->staticPosition) obj2->position.z -= zover / 2;
			norm = Vector3::BACK;
		}
		
		m1.norm = norm;
		m2.norm = norm;
		
		
		//dynamic resolution
		Vector3 rv = obj2->velocity - obj1->velocity;
		
		//PRINTLN(TOSTRING(rv));
		
		float vAlongNorm = rv.dot(norm);
		if (vAlongNorm < 0) {
			//TODO(sushi, Ph) do better elasticity later
			float j = -(1 + (obj1->elasticity + obj2->elasticity) / 2) * vAlongNorm;
			j /= 1 / obj1->mass + 1 / obj2->mass;
			
			Vector3 impulse = j * norm;
			if (!obj1->staticPosition) obj1->velocity -= impulse / obj1->mass;
			if (!obj2->staticPosition) obj2->velocity += impulse / obj2->mass;
			//PRINTLN(obj2->velocity.mag());
			
			
			//setting contact state depending on movement
			if (fabs(obj1->velocity.normalized().dot(norm)) != 1) {
				if (!obj1->staticPosition) {
					obj1->contacts[obj2] = ContactMoving;
				}
				else obj1->contacts[obj2] = ContactStationary;
			}
			else {
				obj1->contacts[obj2] = ContactStationary;
			}
			if (fabs(obj2->velocity.normalized().dot(norm)) != 1) {
				if (!obj2->staticPosition) 
					obj2->contacts[obj1] = ContactMoving;
				else obj2->contacts[obj1] = ContactStationary;
			}
			else {
				obj2->contacts[obj1] = ContactStationary;
			}
		}
		
		//all of this probably isnt nececssary but i'm trying to get it to wo rkr kr k r
		//the player checking is so i can point the normal in the proper direction when trying 
		//to figure out if the player is on the floor or a wall/ceiling
		//TODO(sushi, PhCl) clean this up
		if (DengAdmin->player == obj1->entity) {
			Vector3 pto = obj2->position - obj1->position;
			if (pto.normalized().dot(norm) > 0) { m1.player = 1; m2.player = 0; }
			else                                { m1.player = 0; m2.player = 1; }
			
			obj1->manifolds[obj2] = m1;
			obj2->manifolds[obj1] = m2;
		}
		else if (DengAdmin->player == obj2->entity) {
			Vector3 pto = obj2->position - obj1->position;
			if (pto.normalized().dot(norm) > 0) { m1.player = 1; m2.player = 0; }
			else                                { m1.player = 0; m2.player = 1; }
			
			obj1->manifolds[obj2] = m2;
			obj2->manifolds[obj1] = m1;
		}
		else {
			obj1->manifolds[obj2] = m2;
			obj2->manifolds[obj1] = m1;
		}
		
		return true;
	}	
	obj1->contacts[obj2] = ContactNONE;
	obj2->contacts[obj1] = ContactNONE;
	return false;
}

inline void AABBSphereCollision(Physics* aabb, AABBCollider* aabbCol, Physics* sphere, SphereCollider* sphereCol) {
	Vector3 aabbPoint = Geometry::ClosestPointOnAABB(aabb->position, (aabbCol->halfDims * aabb->entity->transform.scale), sphere->position);
	Vector3 vectorBetween = aabbPoint - sphere->position; //sphere towards aabb
	float distanceBetween = vectorBetween.mag();
	if(distanceBetween < sphereCol->radius) {
		//triggers and no collision
		if (aabbCol->event != 0)   aabbCol->sender.SendEvent(aabbCol->event);
		if (sphereCol->event != 0) sphereCol->sender.SendEvent(sphereCol->event);
		if (aabbCol->noCollide || sphereCol->noCollide) return;
		
		//aabb->entity->GetComponent<AudioSource>()->request_play = true;
		//static resolution
		if (aabbPoint == sphere->position) { 
			//NOTE if the closest point is the same, the vector between will have no direction; this 
			//is supposed to be a remedy to that by offsetting in the direction between thier centers
			vectorBetween = aabb->position - sphere->position;
		}
		float overlap = .5f * (sphereCol->radius - distanceBetween);
		Vector3 normal = -vectorBetween.normalized();
		vectorBetween = -normal * overlap;
		if(aabb->staticPosition){
			sphere->position -= vectorBetween;
		}else if(sphere->staticPosition){
			aabb->position += vectorBetween;
		}else{
			aabb->position += vectorBetween / 2.f;
			sphere->position -= vectorBetween / 2.f;
		}
		
		//dynamic resolution
		Matrix4 sphereInertiaTensorInverse = LocalToWorldInertiaTensor(sphere, sphereCol->tensor).Inverse();
		Vector3 ra = sphere->position + Geometry::ClosestPointOnSphere(sphere->position, sphereCol->radius, aabbPoint);
		Vector3 sphereAngularVelocityChange = normal.cross(ra);
		sphereAngularVelocityChange *= sphereInertiaTensorInverse;
		float inverseMassA = 1.f / sphere->mass;
		float scalar = inverseMassA + sphereAngularVelocityChange.cross(ra).dot(normal);
		
		Matrix4 aabbInertiaTensorInverse = LocalToWorldInertiaTensor(aabb, aabbCol->tensor).Inverse();
		Vector3 rb = aabb->position + aabbPoint;
		Vector3 aabbAngularVelocityChange = normal.cross(rb);
		aabbAngularVelocityChange *= aabbInertiaTensorInverse;
		float inverseMassB = 1.f / aabb->mass; 
		scalar += inverseMassB + aabbAngularVelocityChange.cross(rb).dot(normal);
		
		float coefRest = (aabb->elasticity + sphere->elasticity) / 2; 
		float impulseMod = (coefRest + 1) * (sphere->velocity - aabb->velocity).mag(); //this too :)
		Vector3 impulse = normal * impulseMod;
		aabb->AddImpulse(sphere, -impulse);
		//sphere->rotVelocity -= sphereAngularVelocityChange;
		//aabb->entity->rotVelocity -= aabbAngularVelocityChange; //we dont do this because AABB shouldnt rotate
	}
}

inline void AABBBoxCollision(Physics* aabb, AABBCollider* aabbCol, Physics* box, BoxCollider* boxCol) {
	ERROR("AABB-Box collision not implemented in PhysicsSystem.cpp");
}

inline bool SphereSphereCollision(Physics* s1, SphereCollider* sc1, Physics* s2, SphereCollider* sc2) {
	//static resolution
	float dist = (s1->position - s2->position).mag();
	float rsum = sc1->radius + sc2->radius;
	if (rsum > dist) {
		//triggers and no collision
		if(sc1->event != Event_NONE) sc1->sender.SendEvent(sc1->event);
		if(sc2->event != Event_NONE) sc2->sender.SendEvent(sc2->event);
		if(sc1->noCollide || sc2->noCollide) return false;
		
		Vector3 s1t2 = s2->position - s1->position;
		float overlap = (rsum - dist) / 2;
		if (!s1->staticPosition) s1->position -= s1t2.normalized() * overlap;
		if (!s2->staticPosition) s2->position += s1t2.normalized() * overlap;
		
		//dynamic resolution
		//from https://www.gamasutra.com/view/feature./131424/pool_hall_lessons_fast_accurate_.php?print=1
		Vector3 n = s1->position - s2->position;
		n.normalize();
		
		float a1 = s1->velocity.dot(n);
		float a2 = s2->velocity.dot(n);
		
		float opP = (2 * (a1 - a2)) / (s1->mass + s2->mass);
		
		s1->velocity = s1->velocity - opP * s2->mass * n;
		s2->velocity = s2->velocity + opP * s1->mass * n;
		return true;
	}
	return false;
}

inline void SphereLandscapeCollision(Physics* s, SphereCollider* sc, Physics* ls, SphereCollider* lsc) {
	
}

inline void SphereBoxCollision(Physics* sphere, SphereCollider* sphereCol, Physics* box, BoxCollider* boxCol) {
	ERROR("Sphere-Box collision not implemented in PhysicsSystem.cpp");
}

inline void BoxBoxCollision(Physics* box, BoxCollider* boxCol, Physics* other, BoxCollider* otherCol) {
	ERROR("Box-Box collision not implemented in PhysicsSystem.cpp");
}

//This is currently done by SAT
inline bool ComplexComplexCollision(Physics* obj1, ComplexCollider* obj1Col, Physics* obj2, ComplexCollider* obj2Col) {
	//LOG("tick ", physTickCounter, " with ", obj1->entity->id, " against ", obj2->entity->id, " ---------------------------------------------------------");
	
	Physics* o1 = obj1; Physics* o2 = obj2;
	ComplexCollider* o1c = obj1Col; ComplexCollider* o2c = obj2Col;
	Physics* refphys = nullptr;
	Physics* incphys = nullptr;
	ComplexCollider* refcol = nullptr;
	ComplexCollider* inccol = nullptr;
	
	float minpen = -INFINITY;
	Vector3 bestnorm;
	
	auto dist = [](vec3 p, vec3 plane_n, vec3 plane_p) {
		//return (plane_n.x * p.x + plane_n.y * p.y + plane_n.z * p.z - plane_n.dot(plane_p));
		return (p - plane_p).dot(plane_n);
	};
	
	vec3 p0, p1, p2, normal, normal2, intersect;
	f32  t;
	int  index = 0;
	bool done = false;
	f32  saved = 0;
	f32  saved2 = 0;
	
	//NOTE find which object is the inclement and which is the reference
	for(int shape = 0; shape < 2; shape++){
		if (shape == 1) { 
			o1c = obj2Col; o2c = obj1Col; 
			o1 = obj2; o2 = obj1;
		}
		
		mat4 o1transform = Matrix4::TransformationMatrix(o1->position, o1->rotation, o1->entity->transform.scale);//o1->entity->transform.TransformMatrix();
		mat4 o2transform = Matrix4::TransformationMatrix(o2->position, o2->rotation, o2->entity->transform.scale);//o2->entity->transform.TransformMatrix();
		
		mat4 o1rotation = Matrix4::RotationMatrix(o1->rotation);
		mat4 o2rotation = Matrix4::RotationMatrix(o2->rotation);
		
		//PRINTLN("o1 rot: " << o1->rotation.str());
		//PRINTLN("o2 rot: " << o2->rotation.str());
		
		//// Face implementation ////
		/*
		Mesh::Face* lastface = 0;
		forX(oc1Index, o1c->mesh->faceCount){ Mesh::Face* f = &o1c->mesh->faceArray[oc1Index];
			p0 = f->vertexArray[0]->pos * o1transform;
			normal = f->normal * o1rotation;
			float deepest = INFINITY;
			forX(oc2Index, o2c->mesh->faceCount){ Mesh::Face* f2 = &o2c->mesh->faceArray[oc2Index];
				forX(f2vIndex, f2->outerVertexCount){
					p1 = f2->outerVertexArray[f2vIndex]->pos * o2transform;
					float vertdepth = Math::DistPointToPlane(p1, normal, p0);
					//ImGui::DebugDrawLine3(p1, p1 - normal * vertdepth, Color(0, 255.0 * (i / (float)f2->points.size()), 0));
					if (vertdepth < deepest) {
						deepest = vertdepth;
					}
					lastface = f2;
				}
				if (deepest > 0) {
					forX(lfvIndex, f2->outerVertexCount){
						ImGui::DebugDrawLine3(lastface->outerVertexArray[lfvIndex]->pos * o2transform, lastface->outerVertexArray[lfvIndex + 1]->pos * o2transform, Color::MAGENTA);
					}
					ImGui::DebugDrawLine3(p1, p1 - normal * deepest, Color::BLACK);
					ERROR("func failed with deepest ", deepest);
					return false;
				}
				else if (deepest > minpen) {
					minpen = deepest;
					bestnorm = normal;
					refphys = o1;
					incphys = o2;
					refcol = o1c;
					inccol = o2c;
				}
			}
			
		}
		*/
		
		//// Triangle implementation ////
		for(Mesh::Triangle& t1 : o1c->mesh->triangles){
			p0     = t1.p[0]   * o1transform;
			normal = t1.normal * o1rotation;
			f32 deepest = INFINITY;
			for(Mesh::Triangle& t2 : o2c->mesh->triangles){
				normal2 = t2.normal * o2rotation;
				forX(vert_idx, 3){
					vec3 vert = t2.p[vert_idx] * o2transform;
					f32 vertdepth = Math::DistPointToPlane(vert, normal, p0);
					//ImGui::DebugDrawLine3(vert, vert - normal * vertdepth, Color(0, 255, 0));
					//ImGui::DebugDrawText3(TOSTRING(vertdepth).c_str(), p0 + normal2 * vertdepth, Color::BLACK);
					if(vertdepth < deepest) deepest = vertdepth;
				}
			}
			//LOG(deepest);
			if(deepest > 0){
				//ImGui::DebugDrawLine3(p1, p1 - normal * deepest, Color::BLACK);
				//ERROR("func failed with deepest ", deepest);
				return false;
			}else if(deepest > minpen){
				minpen = deepest;
				bestnorm = normal;
				refphys = o1;
				incphys = o2;
				refcol = o1c;
				inccol = o2c;
			}
		}
	}
	
	Mesh* refMesh = refcol->mesh;
	Mesh* incMesh = inccol->mesh;
	
	//SUCCESS("func succeded with minpen ", minpen);
	
	//ImGui::DebugDrawLine3(save, save + minpen * saven, Color::BLACK);
	//ImGui::DebugDrawLine3(save2, save2 + saved2 * saven2, Color::DARK_YELLOW);
	//ImGui::DebugDrawCircle3(save2 + minpen * bestnorm, 5, Color::MAGENTA);
	//ImGui::DebugDrawLine3(save + minpen * saven, save2);
	//
	//ImGui::DebugDrawLine3(obj1->position, obj2->position, Color::RED);
	
	
	//////////////////
	//// Clipping ////
	//////////////////
	
	
	//float furthest = -INFINITY;
	//Triangle* best;
	//for (Triangle* t : refMesh->triangles) {
	//	u32 furthestid = 0;
	//	for (int i = 0; i < t->p.size(); i++) {
	//		float dist = bestnorm.dot(t->p[i]);
	//		if (dist > furthest) {
	//			furthest = dist;
	//			best = t;
	//		}
	//	}
	//}
	
	//NOTE find triangle that's most aligned with normal
	u32 furthestTriRef = Geometry::FurthestTriangleAlongNormal(refcol->mesh, Matrix4::RotationMatrix(refphys->rotation), bestnorm);
	u32 furthestTriInc = Geometry::FurthestTriangleAlongNormal(inccol->mesh, Matrix4::RotationMatrix(incphys->rotation), -bestnorm);
	Mesh::Triangle* triRef = &refMesh->triangleArray[furthestTriRef];
	Mesh::Triangle* triInc = &incMesh->triangleArray[furthestTriInc];
	
	Mesh::Face* refFace = &refMesh->faces[triRef->face];
	Mesh::Face* incFace = &incMesh->faces[triInc->face];
	
	//NOTE draw collision normal
	//ImGui::DebugDrawTriangle3(Geometry::MeshTriangleMidpoint(triRef), Geometry::MeshTriangleMidpoint(triRef) + bestnorm, Color::YELLOW);
	
	Matrix4 refTransform = Matrix4::TransformationMatrix(refphys->position, refphys->rotation, refphys->entity->transform.scale);
	Matrix4 incTransform = Matrix4::TransformationMatrix(incphys->position, incphys->rotation, incphys->entity->transform.scale);
	ImGui::DebugDrawLine3(Geometry::MeshTriangleMidpoint(triRef) * refTransform, Geometry::MeshTriangleMidpoint(triRef) * refTransform + bestnorm, Color::YELLOW);
	
	Matrix4 refRotation = Matrix4::RotationMatrix(refphys->rotation);
	Matrix4 incRotation = Matrix4::RotationMatrix(incphys->rotation);
	ImGui::DebugDrawLine3(Geometry::MeshTriangleMidpoint(triRef) * incTransform, Geometry::MeshTriangleMidpoint(triRef) * incTransform + triInc->normal * incRotation,  Color::VERY_DARK_YELLOW);
	
	//NOTE draw inclement face's triangles
	//for (Triangle* t : triInc->face->tris) {
	//	ImGui::DebugDrawTriangle3(
	//		t->v0->pos * incphys->entity->transform.TransformMatrix(),
	//		t->v1->pos * incphys->entity->transform.TransformMatrix(),
	//		t->v2->pos * incphys->entity->transform.TransformMatrix(), Color::MAGENTA);
	//}
	
	//NOTE draw inclement face's edges
	/*for (int i = 0; i < triInc->face->outerVertexCount; i += 2) {
		ImGui::DebugDrawLine3(triInc->face->outerVertexArray[i]->pos * incTransform,
							  triInc->face->outerVertexArray[i + 1]->pos * incTransform, 
							  Color::MAGENTA);
	}*/
	
	//NOTE draw reference face's neighboring triangles
	//for(u32 idx : refFace->neighborTriangles){ Mesh::Triangle* t = &refMesh->triangles[idx];
	//ImGui::DebugDrawTriangle3(t->p[0] * refTransform, t->p[1] * refTransform, t->p[2] * refTransform, Color::MAGENTA);
	//}
	
	//NOTE draw reference face's triangles
	for(u32 idx : refFace->triangles){ Mesh::Triangle* t = &refMesh->triangles[idx];
		ImGui::DebugDrawTriangle3(t->p[0] * refTransform, t->p[1] * refTransform, t->p[2] * refTransform, Color::GREEN);
	}
	
	//TODO(delle,Op) most of these positions and normals are getting multiplied against matricies again
	//NOTE clip inc face's points against ref's adjacent faces
	std::vector<vec3> colPoints;
	for(u32 rftnIdx : refFace->triangleNeighbors){ Mesh::Triangle* rftn = &refMesh->triangles[rftnIdx];
		vec3 refP = rftn->p[0] * refTransform;
		for(u32 iftIdx : incFace->triangles){ Mesh::Triangle* ift = &incMesh->triangles[iftIdx];
			forI(3) {
				vec3 incP     = ift->p[i          ] * incTransform;
				vec3 incPLast = ift->p[(i + 2) % 3] * incTransform;
				
				float dCurr = -Math::DistPointToPlane(incP, rftn->normal * refRotation, refP);
				float dLast = -Math::DistPointToPlane(incPLast, rftn->normal * refRotation, refP);
				
				if(dCurr < 0 && dLast > 0){
					//ImGui::DebugDrawText3(TOSTRING(dCurr).c_str(), incP);
					ImGui::DebugDrawLine3(incP, incP + rftn->normal * dCurr, Color::GREEN);
					//ImGui::DebugDrawText3(TOSTRING(dLast).c_str(), incPLast);
					ImGui::DebugDrawLine3(incPLast, incPLast + rftn->normal * dLast, Color::RED);
					vec3 inter = Math::VectorPlaneIntersect(refP, rftn->normal * refRotation, incPLast, incP);
					
					colPoints.push_back(inter);
				}else{
					//colPoints.push_back(t->p[i]);
				}
			}
		}
	}
	
	for(vec3& v : colPoints){
		ImGui::DebugDrawCircle3(v, 5, Color(255, 255, 0));
		float distance = Math::DistPointToPlane(v, triRef->normal * refRotation, triRef->p[0] * refTransform);
		if(distance < 0){
			ImGui::DebugDrawCircleFilled3(o1->position - bestnorm * distance / 2, 5, Color(0, 0, 255));
			//ImGui::DebugDrawText3(TOSTRING(physTickCounter).c_str(), o1->position - bestnorm * distance / 2, Color::BLACK);
			ImGui::DebugDrawCircleFilled3(o2->position + bestnorm * distance / 2, 5, Color(255, 0, 0));
			//ImGui::DebugDrawText3(TOSTRING(physTickCounter).c_str(), o2->position + bestnorm * distance / 2, Color::BLACK);
			ImGui::DebugDrawCircleFilled3(o2->position, 5, Color::GREEN);
			ImGui::DebugDrawCircleFilled3(o1->position, 5, Color::GREEN);
			ImGui::DebugDrawText3(TOSTRING(distance).c_str(), v, Color::BLACK);
			
			//PRINTLN(physTickCounter << " ------------------------------------------ " << distance);
			//PRINTLN(o1 << " before: " << TOSTRING(o1->position));
			o1->position -= bestnorm * distance / 2;
			//PRINTLN(o1 << " change: " << TOSTRING(bestnorm * distance / 2));
			//PRINTLN(o1 << " after:  " << TOSTRING(o1->position));
			//PRINTLN(o2 << " before: " << TOSTRING(o2->position));
			//PRINTLN(o2 << " change: " << TOSTRING(-bestnorm * distance / 2));
			o2->position += bestnorm * distance / 2;
			//PRINTLN(o2 << " after:  " << TOSTRING(o2->position));
		}
	}
	return true;
}

////////////////////
//
//
// 2D Stuff, will probably be moved later
//
//


//returns what point in a vector of 2D vectors is furthest along a normal
int FurthestAlongNormal(std::vector<Vector2> p, Vector2 n) {
	float furthest = -INFINITY;
	int furthestID = 0;
	for (int i = 0; i < p.size(); i++) {
		float dist = n.dot(p[i]);
		if (dist > furthest) {
			furthest = dist;
			furthestID = i;
		}
	}
	return furthestID;
}

int GetFaceID(poly* p, Vector2 n) {
	int furthestPointID = FurthestAlongNormal(p->p, n);
	float mostSimilar = -1;
	int ID = 0;
	
	for (int i = 0; i < p->p.size(); i++) {
		int o = (i + 1) % p->p.size();
		
		Vector2 p1 = p->p[i];
		Vector2 p2 = p->p[o];
		
		if (i == furthestPointID || o == furthestPointID) {
			Vector2 norm = (p2 - p1).perp().normalized();
			float similarity = norm.dot(n);
			if (similarity > mostSimilar) {
				mostSimilar = similarity;
				ID = i;
			}
		}
	}
	return ID;
}

void ClipSide(Vector2* colpoints, poly* p, int faceID) {
	
	int outside = 0;
	int inside = 0;
	
	Vector2 fp1 = p->p[faceID];
	Vector2	fp2 = p->p[(faceID + 1) % p->p.size()];
	Vector2 norm = (fp2 - fp1).perp().normalized();
	
	float dists[2];
	
	for (int i = 0; i < 2; i++) {
		float dist = (colpoints[i] - fp1).dot(norm);
		dists[i] = dist;
		if (dist > 0) outside++;
		else          inside++;
	}
	
	if (inside == 2) return;
	else if (inside == 1) {
		float total = abs(dists[0]) + abs(dists[1]);
		int insideID = (dists[0] > 0) ? 1 : 0;
		int outsideID = !insideID;
		float ratio = abs(dists[insideID]) / total;
		Vector2 v = (colpoints[outsideID] - colpoints[insideID]) * ratio;
		Vector2 np = colpoints[insideID] + v;
		colpoints[outsideID] = np;
	}
}

void Clip(Manifold2& m) {
	int refID = GetFaceID(m.a, m.norm);
	int incID = GetFaceID(m.b, -m.norm);
	
	int asize = m.a->p.size();
	int bsize = m.b->p.size();
	
	Vector2 colpoints[2] = { m.b->p[incID], m.b->p[(incID + 1) % bsize] };
	
	//clip colpoints against adjacent faces of reference
	ClipSide(colpoints, m.a, ((refID + (asize - 1)) % asize));
	ClipSide(colpoints, m.a, ((refID + 1) % asize));
	m.nColPoints = 2;
	
	//get distance from ref face of collision points
	for (int i = 0; i < 2; i++) {
		float dist = (colpoints[i] - m.a->p[refID]).dot(m.norm);
		m.depth[i] = dist;
	}
	m.colpoints[0] = colpoints[0];
	m.colpoints[1] = colpoints[1];
	
	//remove collision points that are above the reference face and reoder them in manifold if needed
	for (int i = 0; i < 2; i++) {
		if (m.depth[i] > 0) {
			m.nColPoints--;
			if (i == 0) {
				m.colpoints[0] = m.colpoints[1];
				m.depth[0] = m.depth[1];
			}
		}
	}
	
	m.refID = refID;
	
}

bool ShapeOverlapSAT(poly& r1, poly& r2, Manifold2& m) {
	//PRINTLN("SAT------------------------------")
	persist int depcount = 0;
	
	poly* p1 = &r1;
	poly* p2 = &r2;
	poly* refpoly = nullptr;
	
	float minpen = -INFINITY;
	Vector2 bnorm;
	
	
	for (int shape = 0; shape < 2; shape++) {
		if (shape == 1) { p1 = &r2; p2 = &r1; }
		
		for (int i = 0; i < p1->p.size(); i += 2) {
			
			Vector2 fp1 = p1->p[i];
			//DrawString(Vector2(fp1.x, fp1.y - 20), std::to_string(i), olc::CYAN);
			Vector2 fp2 = p1->p[i + 1];
			//DrawString(Vector2(fp2.x + 10, fp2.y - 20), std::to_string(i), olc::CYAN);
			
			Vector2 norm = (fp2 - fp1).perp().normalized();
			float deepest = INFINITY;
			
			Vector2 vertp;
			
			for (int j = 0; j < p2->p.size(); j++) {
				
				Vector2 vert = p2->p[j];
				vertp = vert;
				float vertdepth = (fp1 - vert).dot(norm);
				//if(shape == 0) DrawLine(vert, vert - norm * vertdepth, olc::Pixel(255 * (j+1)/p2->p.size(), 255 * (p2->p.size() - j) / p2->p.size(), 255 * (p1->p.size() - i) / p1->p.size()));
				//if(shape == 0) DrawString(vert, std::to_string(j));
				//if(shape == 0) DrawString(Vector2(0 + 300 * shape, GetWindowSize().y - (100 + 9 * depcount)), "dep face " + std::to_string(i) + " point " + std::to_string(j) + ": " + std::to_string(vertdepth), olc::Pixel(255 * (j + 1) / p2->p.size(), 255 * (p2->p.size() - j) / p2->p.size(), 255 * (p1->p.size() - i) / p1->p.size()));
				depcount++;
				if (vertdepth < deepest) deepest = vertdepth;
			}
			//PRINTLN(deepest);
			if (deepest > 0) {
				return false;
			}
			else if (deepest > minpen) {
				minpen = deepest;
				bnorm = norm;
				refpoly = p1;
			}
		}
		depcount = 0;
	}
	
	
	m.a = refpoly;
	m.b = (&r1 == refpoly) ? &r2 : &r1;
	m.norm = bnorm;
	
	Clip(m);
	
	return true;
}

void FillManis(std::vector<poly>& polys, std::vector<Manifold2>& manis) {
	Assert(Render::GetSettings()->findMeshTriangleNeighbors, "findMeshTriangleNeighbors must be enabled for 2D-3D physics");
	manis.clear();
	for (int m = 0; m < polys.size(); m++) {
		for (int n = m + 1; n < polys.size(); n++) {
			Manifold2 ma;
			if (ShapeOverlapSAT(polys[m], polys[n], ma)) {
				//PRINTLN("TRUE------------------------");
			}
			else {
				//PRINTLN("FALSE------------------------");
			}
			//polys[m].overlap |= 
			manis.push_back(ma);
		}
	}
}

void SolveManifolds(std::vector<Manifold2> manis) {
	for (Manifold2& m : manis) {
		poly* p1 = m.a;
		poly* p2 = m.b;
		
		if (p1 && p2) {
			
			//if (!p1->staticPosition && m.depth[0] < 0) p1->pos -= m.norm * m.depth[0] / 2;
			//if (!p1->staticPosition && m.depth[1] < 0) p1->pos -= m.norm * m.depth[1] / 2;
			//
			//if (!p2->staticPosition && m.depth[0] < 0) p2->pos += m.norm * m.depth[0] / 2;
			//if (!p2->staticPosition && m.depth[1] < 0) p2->pos += m.norm * m.depth[1] / 2;
			//
			//PRINTLN(TOSTRING("depth1: ", m.norm * m.depth[0] / 2));
			//PRINTLN(TOSTRING("depth2: ", m.norm * m.depth[1] / 2));
			//
			//
			//Vector3 nupos1 = Math::ScreenToWorld(p1->pos, DengCamera->projMat, DengCamera->viewMat, DengWindow->dimensions);
			//p1->ogphys->position = Math::VectorPlaneIntersect(p1->ogphys->position, DengCamera->position - p1->ogphys->position, DengCamera->position, nupos1);
			//
			//Vector3 nupos2 = Math::ScreenToWorld(p2->pos, DengCamera->projMat, DengCamera->viewMat, DengWindow->dimensions);
			//p2->ogphys->position = Math::VectorPlaneIntersect(p2->ogphys->position, DengCamera->position - p2->ogphys->position, DengCamera->position, nupos2);
			
			
			//if (p1->staticPosition) p1->vel = Vector2::ZERO;
			//if (p2->staticPosition) p2->vel = Vector2::ZERO;
			
			Vector2 rv = p1->vel - p2->vel;
			
			float vAlongNorm = rv.dot(m.norm);
			
			if (vAlongNorm < 0) {
				float e = 0;
				float j = -(1 + e) * rv.dot(m.norm);
				j /= 1 / p1->mass + 1 / p2->mass;
				
				Vector2 impulse = j * m.norm;
				if (!p1->staticPosition) {
					
					Vector3 impworld = Math::ScreenToWorld(p1->pos + p1->vel + impulse, DengCamera->projMat, DengCamera->viewMat, DengWindow->dimensions);
					Vector3 impworldpr = Math::VectorPlaneIntersect(p1->ogphys->position, DengCamera->position - p1->ogphys->position, DengCamera->position, impworld);
					
					p1->ogphys->velocity -= (impworldpr - p1->ogphys->position);
				}
				if (!p2->staticPosition) {
					
					Vector3 impworld = Math::ScreenToWorld(p2->pos + p2->vel + impulse, DengCamera->projMat, DengCamera->viewMat, DengWindow->dimensions);
					Vector3 impworldpr = Math::VectorPlaneIntersect(p2->ogphys->position, DengCamera->position - p2->ogphys->position, DengCamera->position, impworld);
					
					p2->ogphys->velocity -= (impworldpr - p2->ogphys->position);
				}
			}
		}
	}
}

poly GeneratePoly(Physics* p) {
	poly poly;
	poly.o = DengScene->GenerateMeshOutlinePoints(p->entity->GetComponent<ModelInstance>()->mesh,
												  Matrix4::TransformationMatrix(p->position, p->rotation, p->entity->transform.scale),
												  DengCamera->projMat, DengCamera->viewMat, DengAdmin->mainCamera->position,
												  DengWindow->dimensions);
	poly.p = poly.o;
	
	poly.pos = Math::WorldToScreen2(p->position, DengCamera->projMat, DengCamera->viewMat, DengWindow->dimensions);
	
	//apply relative velocity if player exists and is moving
	Vector3 relvel = Vector3::ZERO;
	if (DengAdmin->player) {
		relvel = p->velocity - DengAdmin->player->GetComponent<Physics>()->velocity;
	}
	//translate objects 3D velocity into screen space
	Vector3 opv = p->position + relvel;
	Vector2 opvs = Math::WorldToScreen2(opv, DengCamera->projMat, DengCamera->viewMat, DengWindow->dimensions);
	poly.vel = opvs - poly.pos;
	ImGui::DebugDrawLine(poly.pos, opvs, Color::CYAN);
	//maybe make a nicer way of calculating this
	poly.mass = p->mass;
	
	poly.staticPosition = p->staticPosition;
	
	poly.ogphys = p;
	
	return poly;
	
}

//NOTE make sure you are using the right physics component, because the collision 
//functions dont check that the provided one matches the tuple
inline void CheckCollision(PhysicsTuple& tuple, PhysicsTuple& other, std::vector<poly>& polys) {
	auto genpolys = [&]() {
		if (!tuple.physics->twoDpolygon) {
			tuple.physics->twoDpolygon = new poly(GeneratePoly(tuple.physics));
			polys.push_back(*tuple.physics->twoDpolygon);
		}
		if (!other.physics->twoDpolygon) {
			other.physics->twoDpolygon = new poly(GeneratePoly(other.physics));
			polys.push_back(*other.physics->twoDpolygon);
		}
	};
	
	switch(tuple.collider->shape){
		case(ColliderShape_Box):
		switch(other.collider->shape){
			case(ColliderShape_Box):   { BoxBoxCollision   (tuple.physics, (BoxCollider*)   tuple.collider, 
															other.physics, (BoxCollider*)   other.collider); }break;
			case(ColliderShape_Sphere):{ SphereBoxCollision(other.physics, (SphereCollider*)other.collider, 
															tuple.physics, (BoxCollider*)   tuple.collider); }break;
			case(ColliderShape_AABB):  { AABBBoxCollision  (other.physics, (AABBCollider*)  other.collider, 
															tuple.physics, (BoxCollider*)   tuple.collider); }break;
		}break;
		case(ColliderShape_Sphere):
		switch(other.collider->shape){
			case(ColliderShape_Box):   { SphereBoxCollision   (tuple.physics, (SphereCollider*)tuple.collider, 
															   other.physics, (BoxCollider*)   other.collider); }break;
			case(ColliderShape_Sphere):{ 
				if (!SphereSphereCollision(tuple.physics, (SphereCollider*)tuple.collider,
										   other.physics, (SphereCollider*)other.collider)) {
					if (tuple.physics->twoDphys && other.physics->twoDphys) genpolys();
				}
				
			}break;
			case(ColliderShape_AABB):  { AABBSphereCollision  (other.physics, (AABBCollider*)  other.collider, 
															   tuple.physics, (SphereCollider*)tuple.collider); }break;
		}break;
		case(ColliderShape_AABB):
		switch(other.collider->shape){
			case(ColliderShape_Box):   { AABBBoxCollision   (tuple.physics, (AABBCollider*)  tuple.collider, 
															 other.physics, (BoxCollider*)   other.collider); }break;
			case(ColliderShape_Sphere):{ AABBSphereCollision(tuple.physics, (AABBCollider*)  tuple.collider, 
															 other.physics, (SphereCollider*)other.collider); }break;
			case(ColliderShape_AABB):  { 
				if (!AABBAABBCollision(tuple.physics, (AABBCollider*)tuple.collider,
									   other.physics, (AABBCollider*)other.collider)) {
					if (tuple.physics->twoDphys && other.physics->twoDphys) genpolys();
				}
				
			}break;
		}break;
		case ColliderShape_Complex:
		switch (other.collider->shape) {
			case(ColliderShape_Complex): {
				ComplexComplexCollision(tuple.physics, (ComplexCollider*)tuple.collider, other.physics, (ComplexCollider*)other.collider);
			}
		}
	}
}

inline void CollisionTick(std::vector<PhysicsTuple>& tuples, PhysicsTuple& t){
	std::vector<Manifold2> manis; //TODO(sushi, Ph) put the manifolds vector somewhere better later
	std::vector<poly> polys; 
	if(t.collider) {
		for(auto& t2 : tuples) {
			if(&t != &t2 && t2.collider
			   && t.collider->collLayer == t2.collider->collLayer
			   && (!t.physics->staticPosition || !t2.physics->staticPosition)) {
				CheckCollision(t, t2, polys);
				collCount++;
			}
		}
		FillManis(polys, manis);
		SolveManifolds(manis);
	}
}

//////////////////////////
//// system functions ////
//////////////////////////

void PhysicsSystem::Init(Admin* a) {
	admin = a;
	integrationMode = IntegrationMode::EULER;
	collisionMode   = CollisionDetectionMode::DISCRETE;
	
	gravity        = 9.81;
	frictionAir    = 0.01f; 
	minVelocity    = 0.005f;
	maxVelocity    = 100.f;
	minRotVelocity = 1.f;
	maxRotVelocity = 360.f;
}

void PhysicsSystem::Update() {
	std::vector<PhysicsTuple> tuples = GetPhysicsTuples(admin);
	//update physics extra times per frame if frame time delta is larger than physics time delta
	TIMER_START(physLocalTime);
	while(DengTime->fixedAccumulator >= DengTime->fixedDeltaTime) {
		collCount = 0;
		physTickCounter++;
		for(auto& t : tuples) {
			if (TIMER_END(physLocalTime) > 5000 && breakphys) {
				admin->pause_phys = true;
				ERROR("Physics system took longer than 5 seconds, pausing.");
				goto physend;
			}
			if (admin->player && admin->player == t.physics->entity) {
				admin->player->GetComponent<Movement>()->Update();
			}
			
			if(admin->player != t.physics->entity) 
				PhysicsTick(t, this, DengTime);
			CollisionTick(tuples, t);
		}
		DengTime->fixedAccumulator -= DengTime->fixedDeltaTime;
		DengTime->fixedTotalTime += DengTime->fixedDeltaTime;
		collisionCount = collCount;
	}
	physTickCounter = 0;
	physend:
	//interpolate between new physics position and old transform position by the leftover time
	float alpha = DengTime->fixedAccumulator / DengTime->fixedDeltaTime;
	for(auto& t : tuples) {
		//switch (t.physics->contactState) {
		//	case ContactMoving:
		//	ImGui::DebugDrawText3("ContactMoving", t.transform->position);
		//	break;
		//	case ContactStationary:
		//	ImGui::DebugDrawText3("ContactStationary", t.transform->position);
		//	break;
		//	case ContactNONE:
		//	ImGui::DebugDrawText3("ContactNONE", t.transform->position);
		//	break;
		//}
		
		if (t.physics->twoDpolygon) {
			delete t.physics->twoDpolygon;
			t.physics->twoDpolygon = 0;
		}
		t.transform->prevPosition = t.transform->position;
		t.transform->prevRotation = t.transform->rotation;
		t.transform->position = t.transform->position * (1.f - alpha) + t.physics->position * alpha;
		t.transform->rotation = t.transform->rotation * (1.f - alpha) + t.physics->rotation * alpha;
		if(t.collider) t.collider->sentEvent = false;
		
		//t.transform->rotation = Quaternion::QuatSlerp(t.transform->rotation, t.transform->prevRotation, alpha).ToVector3();
		//t.transform->rotation *= Matrix4::RotationMatrixAroundPoint(t.transform->position, t.transform->rotation*(1.f - alpha) + t.physics->rotation*alpha);
		//TODO(delle,Ph) look into better rotational interpolation once we switch to quaternions
	}
}
