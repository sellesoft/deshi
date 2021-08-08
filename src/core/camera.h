#pragma once
#ifndef DESHI_CAMERA_H
#define DESHI_CAMERA_H

#include "../math/vectormatrix.h"

enum CameraMode_{
	CameraMode_Perspective, 
	CameraMode_Orthographic,
	CameraMode_COUNT,
}; typedef u32 CameraMode;
global_ const char* CameraModeStrings[] = {
	"Perspective", "Orthographic"
};

enum OrthoView_{
	OrthoView_Front,
	OrthoView_Back,
	OrthoView_Right,
	OrthoView_Left,
	OrthoView_Top,
	OrthoView_Bottom,
}; typedef u32 OrthoView;

struct Camera  {
	vec3 position{};
	vec3 rotation{};
	
	float nearZ; //the distance from the camera's position to screen plane
	float farZ; //the maximum render distance
	float fov; //horizontal field of view
	
	bool freeCamera = true;
	
	CameraMode mode = CameraMode_Perspective;
	OrthoView orthoview = OrthoView_Front;
	
	vec3 forward;
	vec3 right;
	vec3 up;
	
	mat4 viewMat;
	mat4 projMat;
	
	static mat4 MakePerspectiveProjectionMatrix(float winWidth, float winHeight, float _fov, float _farZ, float _nearZ) {
		float renderDistance = _farZ - _nearZ;
		float aspectRatio = winHeight / winWidth;
		float fovRad = 1.f / tanf(RADIANS(_fov * .5f));
		return mat4( //NOTE setting (1,1) to negative flips the y-axis
					aspectRatio * fovRad, 0, 0, 0,
					0, -fovRad, 0, 0,
					0, 0, _farZ / renderDistance, 1,
					0, 0, -(_farZ * _nearZ) / renderDistance, 0);
	}
	
	static mat4 MakeOrthographicProjection(float width, float height, float r, float l, float t, float b, float _farZ, float _nearZ) {
		float aspectRatio = width / height;
		
		float f = _farZ;
		float n = _nearZ;
		
		return mat4(2 / (r - l), 0, 0, 0,
					0, 2 / (b - t), 0, 0,
					0, 0, -2 / (f - n), 0,
					-(r + l) / (r - l), -(t + b) / (b - t), -(f + n) / (n - f), 1);
	}
};

#endif //DESHI_CAMERA_H