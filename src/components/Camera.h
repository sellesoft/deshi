#pragma once
#include "Component.h"
#include "../math/Vector3.h"
#include "../math/Matrix4.h"

struct Camera : public Component {
	Vector3 position;
	Vector3 rotation;
	Vector3 lookDir;
	Vector3 target = Vector3(1, 0, 90);
	Vector3 targetrect;
	Vector3 up;

	bool MOUSE_LOOK = false;

	float nearZ; //the distance from the camera's position to screen plane
	float farZ; //the maximum render distance
	float fieldOfView;

	Matrix4 viewMatrix;
	Matrix4 projectionMatrix;

	Camera(EntityAdmin* a);
	Camera(float fov, float nearZ = .1f, float farZ = 1000.f);
	
	void Update() override;
};