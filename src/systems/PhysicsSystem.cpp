#include "dsh_PhysicsSystem.h"
#include "../utils/dsh_PhysicsWorld.h"
#include "../math/dsh_Math.h"
#include "../geometry/dsh_Geometry.h"

#include "../components/dsh_Transform.h"
#include "../components/dsh_Physics.h"
#include "../components/dsh_Collider.h"

#include "../utils/dsh_Command.h"
#include "../components/dsh_Input.h"
#include "../components/dsh_Time.h"
#include "../components/dsh_Camera.h"
#include "../components/dsh_Screen.h"
#include "../components/dsh_Source.h"

//TODO(ip,delle) update entity movement commands to be based on EntityID
inline void AddSelectedEntityCommands(EntityAdmin* admin) {
//// translation ////
	admin->commands["reset_position"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->acceleration = Vector3::ZERO;
				p->velocity = Vector3::ZERO;
				p->position = Vector3::ZERO;
			}
		}
		return "";
		}, "reset_position", "reset_position <EntityID> [String: xyz]");

	admin->commands["reset_position_x"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->acceleration = Vector3(0, p->acceleration.y, p->acceleration.z);
				p->velocity = Vector3(0, p->velocity.y, p->velocity.z);
				p->position = Vector3(0, p->position.y, p->position.z);
			}
		}
		return "";
		}, "reset_position_x", "temp");

	admin->commands["reset_position_y"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->acceleration = Vector3(p->acceleration.x, 0, p->acceleration.z);
				p->velocity = Vector3(p->velocity.x, 0, p->velocity.z);
				p->position = Vector3(p->position.x, 0, p->position.z);
			}
		}
		return "";
		}, "reset_position_y", "temp");

	admin->commands["reset_position_z"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->acceleration = Vector3(p->acceleration.x, p->acceleration.y, 0);
				p->velocity = Vector3(p->velocity.x, p->velocity.y, 0);
				p->position = Vector3(p->position.x, p->position.y, 0);
			}
		}
		return "";
		}, "reset_position_z", "temp");

	admin->commands["reset_velocity"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->acceleration = Vector3::ZERO;
				p->velocity = Vector3::ZERO;
			}
		}
		return "";
		}, "reset_velocity", "reset_position <EntityID> [String: xyz]");

	admin->commands["translate_right"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				PhysicsSystem::AddInput(p, Vector3::RIGHT);
			}
		}
		return "";
		}, "translate_right", "translate_right <EntityID> <amount> [speed]");

	admin->commands["translate_left"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				PhysicsSystem::AddInput(p, Vector3::LEFT);
			}
		}
		return "";
		}, "translate_left", "translate_left <EntityID> <amount> [speed]");

	admin->commands["translate_up"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				PhysicsSystem::AddInput(p, Vector3::UP);
			}
		}
		return "";
		}, "translate_up", "translate_up <EntityID> <amount> [speed]");

	admin->commands["translate_down"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				PhysicsSystem::AddInput(p, Vector3::DOWN);
			}
		}
		return "";
		}, "translate_down", "translate_down <EntityID> <amount> [speed]");

	admin->commands["translate_forward"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				PhysicsSystem::AddInput(p, Vector3::FORWARD);
			}
		}
		return "";
		}, "translate_forward", "translate_forward <EntityID> <amount> [speed]");

	admin->commands["translate_backward"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				PhysicsSystem::AddInput(p, Vector3::BACK);
			}
		}
		return "";
		}, "translate_backward", "translate_backward <EntityID> <amount> [speed]");

	//// rotation ////

	admin->commands["reset_rotation"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->rotAcceleration = Vector3::ZERO;
				p->rotVelocity = Vector3::ZERO;
				p->rotation = Vector3::ZERO;
			}
		}
		return "";
		}, "reset_rotation", "reset_rotation <EntityID> [String: xyz]");

	admin->commands["reset_rotation_x"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->rotAcceleration = Vector3(0, p->rotAcceleration.y, p->rotAcceleration.z);
				p->rotVelocity = Vector3(0, p->rotVelocity.y, p->rotVelocity.z);
				p->rotation = Vector3(0, p->rotation.y, p->rotation.z);
			}
		}
		return "";
		}, "reset_rotation_x", "temp");

	admin->commands["reset_rotation_y"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->rotAcceleration = Vector3(p->rotAcceleration.x, 0, p->rotAcceleration.z);
				p->rotVelocity = Vector3(p->rotVelocity.x, 0, p->rotVelocity.z);
				p->rotation = Vector3(p->rotation.x, 0, p->rotation.z);
			}
		}
		return "";
		}, "reset_rotation_y", "temp");

	admin->commands["reset_rotation_z"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->rotAcceleration = Vector3(p->rotAcceleration.x, p->rotAcceleration.y, 0);
				p->rotVelocity = Vector3(p->rotVelocity.x, p->rotVelocity.y, 0);
				p->rotation = Vector3(p->rotation.x, p->rotation.y, 0);
			}
		}
		return "";
		}, "reset_rotation_z", "temp");

	admin->commands["reset_rotation_velocity"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->rotAcceleration = Vector3::ZERO;
				p->rotVelocity = Vector3::ZERO;
			}
		}
		return "";
		}, "reset_rotation_velocity", "reset_rotation_velocity <EntityID> [String: xyz]");

	admin->commands["rotate_+x"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->rotVelocity += Vector3(5, 0, 0);
			}
		}
		return "";
		}, "rotate_+x", "rotate_+x <EntityID> <amount> [speed]");

	admin->commands["rotate_-x"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->rotVelocity += Vector3(-5, 0, 0);
			}
		}
		return "";
		}, "rotate_-x", "rotate_-x <EntityID> <amount> [speed]");

	admin->commands["rotate_+y"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->rotVelocity += Vector3(0, 5, 0);
			}
		}
		return "";
		}, "rotate_+y", "rotate_+y <EntityID> <amount> [speed]");

	admin->commands["rotate_-y"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->rotVelocity += Vector3(0, -5, 0);
			}
		}
		return "";
		}, "rotate_-y", "rotate_-y <EntityID> <amount> [speed]");

	admin->commands["rotate_+z"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->rotVelocity += Vector3(0, 0, 5);
			}
		}
		return "";
		}, "rotate_+z", "rotate_+z <EntityID> <amount> [speed]");

	admin->commands["rotate_-z"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (admin->input->selectedEntity) {
			if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
				p->rotVelocity += Vector3(0, 0, -5);
			}
		}
		return "";
		}, "rotate_-z", "rotate_-z <EntityID> <amount> [speed]");

	//// other ////

	admin->commands["add_force"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (USE_ORTHO) { //TODO(, sushi) implement ScreenToWorld for ortho projection
			LOG("\nWarning: ScreenToWorld not yet implemented for orthographic projection. World interaction with mouse will not work.\n");
		}
		else {
			if (admin->input->selectedEntity) {
				if (Physics* p = admin->input->selectedEntity->GetComponent<Physics>()) {
					Vector3 pos = Math::ScreenToWorld(admin->input->mousePos, admin->currentCamera->projectionMatrix,
						admin->currentCamera->viewMatrix, admin->screen->dimensions);
					Vector3 clickPos = Math::ScreenToWorld(admin->input->mouseClickPos, admin->currentCamera->projectionMatrix,
						admin->currentCamera->viewMatrix, admin->screen->dimensions);
					//TODO(pi,delle) test that you can add force to a selected entity
					PhysicsSystem::AddForce(nullptr, p, (pos - clickPos).normalized() * 5);
				}
			}
		}
		return "";
	}, "add_force", "add_force <EntityID> <force_vector> [constant_force?]");
}

void PhysicsSystem::Init() {
	AddSelectedEntityCommands(admin);
}

//// Integration ////

struct PhysicsTuple { 
	Transform* transform	= nullptr; 
	Physics* physics		= nullptr; 
	Collider* collider		= nullptr;
	PhysicsTuple(Transform* transform, Physics* physics, Collider* collider) : transform(transform), physics(physics), collider(collider) {}
};

inline std::vector<PhysicsTuple> GetPhysicsTuples(EntityAdmin* admin) {
	std::vector<PhysicsTuple> out;
	for(auto e : admin->entities) {
		Transform* transform	= nullptr;
		Physics* physics		= nullptr;
		Collider* collider		= nullptr;
		for(Component* c : e.second->components) {
			if(Transform* tra = dynamic_cast<Transform*>(c)) { transform = tra; }
			if(Physics* phy = dynamic_cast<Physics*>(c)) { physics = phy; }
			if(Collider* col = dynamic_cast<Collider*>(c)) { collider = col; }
		}
		if(transform && physics) {
			out.push_back(PhysicsTuple(transform, physics, collider));
		}
	}
	return out;
}

//TODO(p,delle) look into bettering this physics tick
//https://gafferongames.com/post/physics_in_3d/
inline void PhysicsTick(PhysicsTuple& t, PhysicsWorld* pw, Time* time) {
//// translation ////

	//add input forces
	t.physics->inputVector.normalize();
	PhysicsSystem::AddForce(nullptr, t.physics, t.physics->inputVector);
	t.physics->inputVector = Vector3::ZERO;

	//add gravity TODO(,sushi) make this a var and toggle later
	//PhysicsSystem::AddForce(nullptr, t.physics, Vector3(0, -9.8, 0));

	//add temp air friction force
	PhysicsSystem::AddFrictionForce(nullptr, t.physics, pw->frictionAir);

	//sum up forces to calculate acceleration
	Vector3 netForce;
	for(auto& f : t.physics->forces) {
		netForce += f;
	}
	t.physics->acceleration = netForce / t.physics->mass * 50;

	//update linear movement and clamp it to min/max velocity
	if (!t.physics->isStatic) {
		t.physics->velocity += t.physics->acceleration * time->physicsDeltaTime;
		float velMag = t.physics->velocity.mag();
		if (velMag > pw->maxVelocity) {
			t.physics->velocity /= velMag;
			t.physics->velocity *= pw->maxVelocity;
		}
		else if (velMag < pw->minVelocity) {
			t.physics->velocity = Vector3::ZERO;
			t.physics->acceleration = Vector3::ZERO;
		}
		t.physics->position += t.physics->velocity * time->physicsDeltaTime;
	}
	

//// rotation ////

	//make fake rotational friction
	if(t.physics->rotVelocity != Vector3::ZERO) {
		t.physics->rotAcceleration = Vector3(t.physics->rotVelocity.x > 0 ? -1 : 1, t.physics->rotVelocity.y > 0 ? -1 : 1, t.physics->rotVelocity.z > 0 ? -1 : 1) * pw->frictionAir * t.physics->mass * 100;
	}

	//update rotational movement and scuffed vector rotational clamping
	t.physics->rotVelocity += t.physics->rotAcceleration * time->physicsDeltaTime;
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
	t.physics->rotation += t.physics->rotVelocity * time->physicsDeltaTime;

	//reset accelerations
	t.physics->forces.clear();
}

//// Collision ////

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

		//TODO(, sushi) find a nicer way to determine how loud a collision sound is 
		//obj1->entity->GetComponent<Source>()->RequestPlay(obj1->velocity.mag() + obj2->velocity.mag());

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
		Vector3 ovel;

		//if the first objects vel is zero we must use the other obj to avoid
		//dividing by zero. if both are 0 then we dont resolve anything
		//i dont think this is necessary anymore since i fixed something in
		//Vector3 but im gonna keep it just incase
		if (obj1->velocity != Vector3::ZERO) {
			ovel = obj1->velocity;
		}
		else if (obj2->velocity != Vector3::ZERO) {
			ovel = obj2->velocity;
		}
		else {
			ovel = Vector3::ZERO;
		}

		Vector3 vcm;
		Vector3 v1f;
		Vector3 v2f;

		//resolution by the Center of Mass Frame Reference Method
		//http://www.ww-p.org/common/pages/DisplayFile.aspx?itemId=8889217
		if (ovel != Vector3::ZERO) {
			
			vcm = ((obj1->mass * obj1->velocity) + (obj2->mass * obj2->velocity)) / (obj1->mass + obj2->mass);
			v1f = -obj1->velocity + (2 * vcm);
			v2f = -obj2->velocity + (2 * vcm);

			if (!obj1->isStatic) obj1->velocity = v1f;
			if (!obj2->isStatic) obj2->velocity = v2f;

			//account for elasticity
			//makes object reflect off of whichever surface of the aabb it hit
			//TODO(,sushi) implement elasticity
			//if (obj1->elasticity != 0 || obj2->elasticity != 0) {
			//	if (xf) {
			//		//this rotates the vel vector 180 degrees around the vector perp to the surface it hit.
			//		Vector3 velcomp = -ovel.compOn(Vector3(1, 0, 0) * (ovel.x / fabs(ovel.x))); //the division is so sign is correct
			//		Vector3 velperp = -ovel - velcomp;
			//		obj->velocity = obj->elasticity * ovel.mag() * (-velperp + velcomp).normalized();
			//		oobj->velocity = oobj->velocity.mag() * -obj->velocity.normalized();
			//		
			//
			//
			//	}
			//	else if (yf) {
			//		Vector3 velcomp = -ovel.compOn(Vector3(0, 1, 0) * (ovel.y / fabs(ovel.y)));
			//		Vector3 velperp = -ovel - velcomp;
			//		obj->velocity = obj->elasticity * ovel.mag() * (-velperp + velcomp).normalized();
			//		oobj->velocity = oobj->velocity.mag() * -obj->velocity.normalized();
			//	}
			//	else if (zf) {
			//		Vector3 velcomp = -ovel.compOn(Vector3(0, 0, 1) * (ovel.z / fabs(ovel.z)));
			//		Vector3 velperp = -ovel - velcomp;
			//		obj->velocity = obj->elasticity * ovel.mag() * (-velperp + velcomp).normalized();
			//		oobj->velocity = oobj->velocity.mag() * -obj->velocity.normalized();
			//
			//	}
			//}
			
		}
		



		


	}


	

}

inline void AABBSphereCollision(Physics* aabb, AABBCollider* aabbCol, Physics* sphere, SphereCollider* sphereCol) {
	Vector3 aabbPoint = Geometry::ClosestPointOnAABB(aabb->position, aabbCol->halfDims, sphere->position);
	Vector3 vectorBetween = aabbPoint - sphere->position; //sphere towards aabb
	float distanceBetween = vectorBetween.mag();
	if(distanceBetween < sphereCol->radius) {
		if(!aabbCol->isTrigger && !sphereCol->isTrigger) {
			SUCCESS("collision happened");
			aabb->entity->GetComponent<Source>()->request_play = true;
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
			PhysicsSystem::AddImpulse(sphere, aabb, -impulse);
			sphere->rotVelocity -= sphereAngularVelocityChange;
			//aabb->entity->rotVelocity -= aabbAngularVelocityChange; //we dont do this because AABB shouldnt rotate
		}
	}
}

inline void AABBBoxCollision(Physics* aabb, AABBCollider* aabbCol, Physics* box, BoxCollider* boxCol) {
	ERROR("AABB-Box collision not implemented in PhysicsSystem.cpp");
}

inline void SphereSphereCollision(Physics* sphere, SphereCollider* sphereCol, Physics* other, SphereCollider* otherCol) {
	ERROR("Sphere-Sphere collision not implemented in PhysicsSystem.cpp");
}

inline void SphereBoxCollision(Physics* sphere, SphereCollider* sphereCol, Physics* box, BoxCollider* boxCol) {
	ERROR("Sphere-Box collision not implemented in PhysicsSystem.cpp");
}

inline void BoxBoxCollision(Physics* box, BoxCollider* boxCol, Physics* other, BoxCollider* otherCol) {
	ERROR("Box-Box collision not implemented in PhysicsSystem.cpp");
}

//NOTE make sure you are using the right physics component, because the collision 
//functions dont check that the provided one matches the tuple
inline void CheckCollision(PhysicsTuple& tuple, PhysicsTuple& other) {
	if(AABBCollider* col = dynamic_cast<AABBCollider*>(tuple.collider)) {
		if(AABBCollider* col2 = dynamic_cast<AABBCollider*>(other.collider)) {
			AABBAABBCollision(tuple.physics, col, other.physics, col2);
		} else if(SphereCollider* col2 = dynamic_cast<SphereCollider*>(other.collider)) {
			AABBSphereCollision(tuple.physics, col, other.physics, col2);
		} else if(BoxCollider* col2 = dynamic_cast<BoxCollider*>(other.collider)) {
			AABBBoxCollision(tuple.physics, col, other.physics, col2);
		}
	} else if(SphereCollider* col = dynamic_cast<SphereCollider*>(tuple.collider)) {
		if(AABBCollider* col2 = dynamic_cast<AABBCollider*>(other.collider)) {
			AABBSphereCollision(other.physics, col2, tuple.physics, col);
		} else if(SphereCollider* col2 = dynamic_cast<SphereCollider*>(other.collider)) {
			SphereSphereCollision(tuple.physics, col, other.physics, col2);
		} else if(BoxCollider* col2 = dynamic_cast<BoxCollider*>(other.collider)) {
			SphereBoxCollision(tuple.physics, col, other.physics, col2);
		}
	} else if(BoxCollider* col = dynamic_cast<BoxCollider*>(tuple.collider)) {
		if(AABBCollider* col2 = dynamic_cast<AABBCollider*>(other.collider)) {
			AABBBoxCollision(other.physics, col2, tuple.physics, col);
		} else if(SphereCollider* col2 = dynamic_cast<SphereCollider*>(other.collider)) {
			SphereBoxCollision(other.physics, col2, tuple.physics, col);
		} else if(BoxCollider* col2 = dynamic_cast<BoxCollider*>(other.collider)) {
			BoxBoxCollision(tuple.physics, col, other.physics, col2);
		}
	}
}

inline void CollisionTick(std::vector<PhysicsTuple>& tuples, PhysicsTuple& t){
	if(t.collider) {
		for(auto& tuple : tuples) {
			if(&t != &tuple && tuple.collider && t.collider->collisionLayer == tuple.collider->collisionLayer) {
				CheckCollision(t, tuple);
			}
		}
	}
}

void PhysicsSystem::Update() {
	Time* time = admin->time;
	PhysicsWorld* pw = admin->physicsWorld;
	
	std::vector<PhysicsTuple> tuples = GetPhysicsTuples(admin);

	//update physics extra times per frame if frame time delta is larger than physics time delta
	while(time->physicsAccumulator >= time->physicsDeltaTime) {
		for(auto& t : tuples) {
			PhysicsTick(t, pw, time);
			CollisionTick(tuples, t);
		}
		time->physicsAccumulator -= time->physicsDeltaTime;
		time->physicsTotalTime += time->physicsDeltaTime;
	}

	//interpolate between new physics position and old transform position by the leftover time
	float alpha = time->physicsAccumulator / time->physicsDeltaTime;
	for(auto& t : tuples) {
		t.transform->prevPosition = t.transform->position;
		t.transform->prevRotation = t.transform->rotation;
		t.transform->position = t.transform->position * (1.f - alpha) + t.physics->position * alpha;
		t.transform->rotation = t.transform->rotation * (1.f - alpha) + t.physics->rotation * alpha;
		//t.transform->rotation = Quaternion::QuatSlerp(t.transform->rotation, t.transform->prevRotation, alpha).ToVector3();

		//t.transform->rotation *= Matrix4::RotationMatrixAroundPoint(t.transform->position, t.transform->rotation*(1.f - alpha) + t.physics->rotation*alpha);
		//TODO(p,delle) look into better rotational interpolation once we switch to quaternions
	}
}

//adds a force to this entity, and this entity applies that force back on the sending object
//simply, changes acceleration by force
inline void PhysicsSystem::AddForce(Physics* creator, Physics* target, Vector3 force) {
	//this->acceleration += bIgnoreMass ? force : force / mass;
	//if (creator) { creator->acceleration -= bIgnoreMass ? force : force / creator->mass; }
	target->forces.push_back(force);
	if(creator) { creator->forces.push_back(-force); }
}

inline void PhysicsSystem::AddInput(Physics* target, Vector3 input) {
	target->inputVector += input;
}

//if no creator, assume air friction
//if creator, assume sliding friction
//TODO(up,delle,11/13/20) change air friction to calculate for shape of object
inline void PhysicsSystem::AddFrictionForce(Physics* creator, Physics* target, float frictionCoef, float gravity) {
	target->forces.push_back(-target->velocity.normalized() * frictionCoef * target->mass);// * gravity);
	if (creator) {
		//TODO(p,delle,12/21/20) implement sliding friction between two objects 
	}
}

//adds an impulse to this entity, and this entity applies that impulse back on the sending object
//simply, changes velocity by impulse force
inline void PhysicsSystem::AddImpulse(Physics* creator, Physics* target, Vector3 impulse, bool ignoreMass) {
	target->velocity += ignoreMass ? impulse : impulse / target->mass;
	if (creator) { creator->velocity -= ignoreMass ? impulse : impulse / creator->mass; }
}