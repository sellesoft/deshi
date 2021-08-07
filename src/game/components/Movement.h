#pragma once
#ifndef COMPONENT_MOVEMENT_H
#define COMPONENT_MOVEMENT_H

#include "Component.h"
#include "../../math/vector.h"

enum MoveState : u32{
	InAirNoInput, // this isn't necessary i dont think
	InAirCrouching,
	OnGroundNoInput,
	OnGroundWalking,   
	OnGroundRunning,   
	OnGroundCrouching,
	OnGroundCrouched
};

struct Physics;
struct CameraInstance;

struct Movement : public Component {
	vec3 inputs;
	Physics* phys;
	
	CameraInstance* camera;
	
	bool inAir;
	MoveState moveState;
	
	float gndAccel = 100;
	float airAccel = 1000;
	
	float jumpImpulse = 10;
	
	float maxWalkingSpeed   = 5;
	float maxRunningSpeed   = 12;
	float maxCrouchingSpeed = 2.5;
	
	bool jump = false;
	
	float maxGrabbingDistance = 5;
	
	Movement();
	Movement(Physics* phys);
	Movement(Physics* phys, float gndAccel, float airAccel, float maxWalkingSpeed, float maxRunningSpeed, float maxCrouchingSpeed, bool jump, float jumpImpulse);
	
	void Update() override;
	
	void DecideContactState();
	void DecideMovementState();
	
	void GrabObject();
	
	std::string SaveTEXT() override;
	static void LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count);
};





#endif //COMPONENT_MOVEMENT_H