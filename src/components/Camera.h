#pragma once
#include "Component.h"
#include "../math/Vector3.h"
#include "../math/Matrix4.h"

struct Camera : public Component {
	//bool yPositiveAxisUp = true;
	bool firstPersonMode = true;
	
	Vector3 position = Vector3(0, 0, -5.f);
	Vector3 rotation = Vector3(0, 0, 0);
	
	Vector3 forward = Vector3::ZERO;
	Vector3 right   = Vector3::ZERO;
	Vector3 up      = Vector3::ZERO;
	
	bool MOUSE_LOOK = false;
	
	float nearZ; //the distance from the camera's position to screen plane
	float farZ; //the maximum render distance
	float fieldOfView;
	
	Matrix4 viewMatrix;
	Matrix4 projectionMatrix;
	
	//not using these
	Vector3 lookDir;
	Vector3 target;
	
	Camera(EntityAdmin* a);
	Camera(float fov, float nearZ = .1f, float farZ = 1000.f);
	
	void Update() override;
	
	//horizontal fov in degrees
	void UsePerspectiveProjection(float fovX, float width, float height, float nearZ, float farZ);
	void UseOrthographicProjection(); //TODO(r,sushi) implement this from .cpp here (ideally without a scene pointer)
};