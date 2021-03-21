#pragma once
#ifndef COMPONENT_CAMERA_H
#define COMPONENT_CAMERA_H

#include "Component.h"
#include "../math/Vector3.h"
#include "../math/Matrix4.h"

struct Camera : public Component {
	Vector3 position = Vector3(0, 0, -5.f);
	Vector3 rotation = Vector3::ZERO;
	
	Vector3 forward = Vector3::ZERO;
	Vector3 right   = Vector3::ZERO;
	Vector3 up      = Vector3::ZERO;
	
	float nearZ; //the distance from the camera's position to screen plane
	float farZ; //the maximum render distance
	float fieldOfView;
	
	Matrix4 viewMatrix;
	Matrix4 projectionMatrix;
	
	Camera(EntityAdmin* a);
	Camera(EntityAdmin*a, float fov, float nearZ = .1f, float farZ = 1000.f);
	
	void Update() override;
	
	//horizontal fov in degrees
	void UsePerspectiveProjection(float fovX, float width, float height, float nearZ, float farZ);
	void UseOrthographicProjection(); //TODO(sushi) implement this from .cpp here (ideally without a scene pointer,Re)
};

#endif //COMPONENT_CAMERA_H