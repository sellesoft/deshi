#pragma once
#ifndef COMPONENT_CAMERA_H
#define COMPONENT_CAMERA_H

#include "Component.h"
#include "../../math/vec.h"
#include "../../math/mat.h"

enum CameraModeBits{
	CameraMode_Perspective, 
	CameraMode_Orthographic,
	CameraMode_COUNT,
}; typedef u32 CameraMode;
global_ const char* CameraModeStrings[] = {
	"Perspective", "Orthographic"
};

enum OrthoViews {
	RIGHT, LEFT, TOPDOWN, BOTTOMUP, FRONT, BACK
};

struct Camera : public Component {
	vec3 position{4.f, 3.f, -4.f};
	vec3 rotation{28.f, -45.f, 0.f};
	float nearZ; //the distance from the camera's position to screen plane
	float farZ; //the maximum render distance
	float fov; //horizontal field of view
	bool freeCamera = true; //whether the camera can move or not (no need to update if false)
	CameraMode mode = CameraMode_Perspective;
	OrthoViews orthoview = FRONT; //TODO(sushi, Cl) combine this with type using bit masking if this is how i decide to keep doing ortho views
	
	vec3 forward;
	vec3 right;
	vec3 up;
	mat4 viewMat;
	mat4 projMat;
	
	Camera();
	Camera(float fov, float nearZ = .01f, float farZ = 1000.01f, bool freeCam = true);
	
	void Update() override;
	
	//horizontal fov in degrees
	mat4 MakePerspectiveProjection();
	mat4 MakeOrthographicProjection();
	void UseOrthographicProjection();
	
	void UpdateProjectionMatrix();
	
	std::string str() override;
	std::string SaveTEXT() override;
	static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif //COMPONENT_CAMERA_H