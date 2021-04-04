#pragma once
#ifndef COMPONENT_CAMERA_H
#define COMPONENT_CAMERA_H

#include "Component.h"
#include "../../math/Vector.h"
#include "../../math/Matrix.h"

enum struct CameraType{
	PERSPECTIVE, ORTHOGRAPHIC
};

struct Camera : public Component {
	Vector3 position{4.f, 3.f, -4.f};
	Vector3 rotation{28.f, -45.f, 0.f};
	Vector3 forward = Vector3::ZERO;
	Vector3 right   = Vector3::ZERO;
	Vector3 up      = Vector3::ZERO;
	bool freeCamera = true; //whether the camera can move or not (no need to update if false)
	CameraType type = CameraType::PERSPECTIVE;
	
	float nearZ; //the distance from the camera's position to screen plane
	float farZ; //the maximum render distance
	float fov;
	Matrix4 viewMatrix;
	Matrix4 projectionMatrix;
	
	Camera(EntityAdmin*a, float fov, float nearZ = .01f, float farZ = 1000.01f, bool freeCam = true);
	
	void Update() override;
	
	//horizontal fov in degrees
	Matrix4 MakePerspectiveProjection();
	Matrix4 MakeOrthographicProjection();
	void UpdateProjectionMatrix();
	
	std::string str() override;
};

#endif //COMPONENT_CAMERA_H