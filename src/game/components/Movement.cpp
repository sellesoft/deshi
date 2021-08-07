#include "Movement.h"
#include "Physics.h"
#include "MeshComp.h"
#include "Camera.h"
#include "../admin.h"
#include "../systems/CanvasSystem.h"
#include "../../core/model.h"
#include "../../core/window.h"
#include "../../core/time.h"
#include "../../core/console.h"
#include "../../utils/debug.h"
#include "../../math/math.h"

Movement::Movement() {
	layer = ComponentLayer_NONE;
	type  = ComponentType_Movement;
}

Movement::Movement(Physics* phys) {
	layer = ComponentLayer_NONE;
	type  = ComponentType_Movement;
	this->phys = phys;
	phys->kineticFricCoef = 4;
}

//for loading
Movement::Movement(Physics* phys, float gndAccel, float airAccel, float maxWalkingSpeed, float maxRunningSpeed, float maxCrouchingSpeed, bool jump, float jumpImpulse) {
	layer = ComponentLayer_NONE;
	type  = ComponentType_Movement;
	this->phys = phys;
	//phys->kineticFricCoef = 1;
	//phys->physOverride = true;
	this->gndAccel = gndAccel;
	this->airAccel = airAccel;
	this->maxWalkingSpeed = maxWalkingSpeed;
	this->maxRunningSpeed = maxRunningSpeed;
	this->maxCrouchingSpeed = maxCrouchingSpeed;
	this->jump = jump;
	this->jumpImpulse = jumpImpulse;
}

void Movement::DecideContactState() {
	bool contactMoving = false;
	bool contactStationary = false;
	for (auto c : phys->contacts) {
		if (c.second == ContactMoving) {
			contactMoving = true;
		}
		else if (c.second == ContactStationary) contactStationary = true;
	}
	
	if (contactMoving)          phys->contactState = ContactMoving;
	else if (contactStationary) phys->contactState = ContactStationary;
	else                        phys->contactState = ContactNONE;
}

void Movement::DecideMovementState() {
	vec3 norm;
	
	//check if were on the ground
	if (phys->contactState == ContactNONE) { moveState = InAirNoInput; inAir = true; }
	else {
		bool bonGround = false;
		for (auto& p : phys->manifolds) {
			norm = p.second.norm.normalized();
			if (!p.second.player) norm = -norm;
			float ang = DEGREES(asin(norm.dot(vec3::UP)));
			if (ang > 45) {
				bonGround = true;
			}
		}
		if (bonGround) {
			if (inputs != vec3::ZERO) {
				if (DengInput->KeyDownAnyMod(Key::LCTRL))  moveState = OnGroundCrouching;
				else if (DengInput->KeyDownAnyMod(Key::LSHIFT)) moveState = OnGroundRunning;
				else if (inputs != vec3::ZERO)               moveState = OnGroundWalking;
				
			}
			else moveState = OnGroundNoInput;
			inAir = false;
		}
		else { moveState = InAirNoInput; inAir = true; }
	}
	
	switch (moveState) {
		case OnGroundNoInput:   ImGui::DebugDrawText("onGroundNoInput", DengWindow->dimensions / 2); break;
		case OnGroundCrouching: ImGui::DebugDrawText("onGroundCrouching", DengWindow->dimensions / 2); break;
		case OnGroundWalking:   ImGui::DebugDrawText("onGroundWalking", DengWindow->dimensions / 2); break;
		case OnGroundRunning:   ImGui::DebugDrawText("onGroundRunning", DengWindow->dimensions / 2); break;
		case InAirNoInput:      ImGui::DebugDrawText("InAirNoInput", DengWindow->dimensions / 2); break;
		case InAirCrouching:    ImGui::DebugDrawText("InAirCrouching", DengWindow->dimensions / 2); break;
	}
	
}

void Movement::GrabObject() {
	//TODO(sushi) grabbing objs needs polished
	//-prevent obj from colliding with player
	//-change the objs velocity as the player moves it around
	//-clean up the logic
	persist bool grabbing = false;
	persist int frame = DengTime->updateCount;
	
	//interpolation vars
	persist float timer = 0;
	persist float timetocenter = 0.07;
	
	persist vec3 ogpos;
	
	//static Entity* grabee = nullptr;
	persist Physics* grabeephys = nullptr;
	
	//grab object or drop it if already holding one
	//frame is necessary to avoid this being ran multiple times due to
	//movement being within physics update
	if(DengInput->KeyPressedAnyMod(DengKeys.use) && DengTime->updateCount != frame){
		if(grabbing){
			grabbing = false;
			grabeephys = 0;
			timer = 0;
		}else{
			//NOTE adjusting the projection matrix so the nearZ is at least .1, produces bad results if less
			mat4 adjusted_proj = Camera::MakePerspectiveProjectionMatrix(DengWindow->width, DengWindow->height, camera->fov, 
																		 camera->farZ, Max(.1, camera->nearZ));
			vec3 direction = (Math::ScreenToWorld(DengInput->mousePos, adjusted_proj, camera->viewMat, DengWindow->dimensions) 
							  - camera->position).normalized();
			if(Entity* e = DengAdmin->EntityRaycast(camera->position, direction, maxGrabbingDistance)){
				if(grabeephys = e->GetComponent<Physics>()){
					if(!grabeephys->staticPosition){
						grabbing = true;
						ogpos = grabeephys->position;
					}
					timer = 0;
				}
			}
		}
		frame = DengTime->updateCount;
	}
	
	//if we are grabbing an object, iterpolate it to the center of the screen and make it follow it as well.
	if (grabeephys && grabbing) {
		//TODO(sushi) make the grabbing distance relative to the size of the object
		vec3 cenpos = 
			Math::ScreenToWorld(DengWindow->dimensions / 2, camera->projMat, camera->viewMat, DengWindow->dimensions)
			+ camera->forward * 3;
		Math::clamp(timer, 0, timetocenter);
		if (timer < timetocenter) {
			timer += DengTime->fixedDeltaTime;
			grabeephys->position = Math::lerpv(ogpos, cenpos, timer / timetocenter);
			grabeephys->velocity = vec3::ZERO;
		}
		else {
			grabeephys->position = cenpos;
			grabeephys->velocity = vec3::ZERO;
		}
	}
}


//NOTE sushi:
//	it's probably important to keep in mind that this function is updated alongside physics components
//	meaning that its updated inside of PhysicsSystem, so it doesn't update once per frame and updates as many
//	times as you have physics updating (default 300 times per second)
void Movement::Update() {
	
	DecideContactState();
	DecideMovementState();
	
	/////////////////////
	//    Crouching    //
	/////////////////////
	
	
	vec3 standpos = DengAdmin->player->transform.position + vec3::UP * 2;
	vec3 crouchpos = DengAdmin->player->transform.position + vec3::UP * 0.5;
	persist vec3 cpos = standpos;
	persist float timer = 0;
	float ttc = 0.2;
	
	if (DengInput->KeyDownAnyMod(DengKeys.movementCrouch)) {
		if (timer < 0.2) timer += DengTime->fixedDeltaTime;
	}
	else {
		if (timer > 0) timer -= DengTime->fixedDeltaTime;
	}
	
	
	///////////////////////////////////
	//    Running/Walking/Jumping    //
	///////////////////////////////////
	
	
	//apply gravity
	phys->velocity += vec3(0, -9.81, 0) * DengTime->fixedDeltaTime;
	
	if (jump) {
		phys->velocity += vec3(0, 10, 0);
		jump = false;
	}
	
	auto accel = [&](float max, float accel) {
		//float projvel = phys->velocity.projectOn(inputs);
		//PRINTLN(TOSTRING("---------\n",
		//	"projvel: ", projvel, "\n",
		//	"vel      ", phys->velocity, "\n",
		//	"input    ", inputs));
		//if (projvel < max - accel * DengTime->fixedDeltaTime) 
		//	phys->velocity += DengTime->fixedDeltaTime * accel * inputs;
		//else if (maxWalkingSpeed - fabs(DengTime->fixedDeltaTime * accel) <= projvel && projvel < max)
		//	phys->velocity += (max - phys->velocity.mag() * cosf(Math::AngBetweenVectors(phys->velocity, inputs))) * inputs;
		phys->velocity += accel * DengTime->fixedDeltaTime * inputs;
		
		if (phys->velocity.mag() > max) phys->velocity.clampMag(0, max);
		
	};
	
	//float projVel = phys->velocity.dot(inputs);
	//
	//if (projVel < maxWalkingSpeed - DengTime->deltaTime * gndAccel) {
	//	phys->velocity += DengTime->fixedDeltaTime * gndAccel * inputs;
	//}
	//else if (maxWalkingSpeed - DengTime->deltaTime * gndAccel <= projVel && projVel < maxWalkingSpeed){
	//	phys->velocity += (maxWalkingSpeed - phys->velocity.mag() * cosf(Math::AngBetweenVectors(phys->velocity, inputs))) * inputs;
	//}
	//else if (velMag < 0.12 && inputs != vec3::ZERO) {
	//	phys->velocity = vec3::ZERO;
	//	phys->acceleration = vec3::ZERO;
	//}
	
	switch (moveState) {
		case InAirNoInput:
		break;
		case InAirCrouching:
		break;
		case OnGroundNoInput:
		break;
		case OnGroundWalking:
		accel(maxWalkingSpeed, gndAccel);
		break;
		case OnGroundRunning:
		accel(maxRunningSpeed, gndAccel);
		break;
		case OnGroundCrouching:
		accel(maxCrouchingSpeed, gndAccel);
		break;
	}
	
	//apply ground friction
	//TODO(sushi) implement friction scaling with velocity and eventually canceling out new velocity
	if (moveState == OnGroundNoInput) {
		if (phys->velocity != vec3::ZERO) {
			if (phys->velocity.mag() > 0.12) {
				for (auto& m : phys->manifolds) {
					vec3 norm = m.second.norm.normalized();
					vec3 vPerpNorm = phys->velocity - phys->velocity.dot(norm) * norm;
					phys->acceleration += vPerpNorm.normalized() * phys->kineticFricCoef * phys->mass * -9.81 / phys->mass;
					phys->velocity += phys->acceleration * DengTime->fixedDeltaTime;
				}
			} else phys->velocity = vec3::ZERO;
		}
	}
	
	phys->position += phys->velocity * DengTime->fixedDeltaTime;
	
	phys->manifolds.clear();
	phys->acceleration = vec3::ZERO;
	
	camera->position = Math::lerpv(standpos, crouchpos, timer / ttc);
	
	////////////////////
	//    Grabbing    // This will eventually be set up for using objects, but maybe that should be in controller/player?
	////////////////////
	
	
	GrabObject();
	
	
}

std::string Movement::SaveTEXT(){
	return TOSTDSTRING("\n>movement"
					   "\nground_accel ", gndAccel,
					   "\nair_accel    ", airAccel,
					   "\njump_impulse ", jumpImpulse,
					   "\nwalk_speed   ", maxWalkingSpeed,
					   "\nrun_speed    ", maxRunningSpeed,
					   "\ncrouch_speed ", maxCrouchingSpeed,
					   "\n");
}

void Movement::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count) {
	ERROR_LOC("LoadDESH not setup");
}

