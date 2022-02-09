#pragma once
#ifndef DESHI_CAMERA_H
#define DESHI_CAMERA_H

#include "math/math.h"

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
	
	f32 nearZ; //the distance from the camera's position to screen plane
	f32 farZ; //the maximum render distance
	f32 fov; //horizontal field of view
	
	b32 freeCamera = true;
	
	CameraMode mode = CameraMode_Perspective;
	OrthoView orthoview = OrthoView_Front;
	
	vec3 forward;
	vec3 right;
	vec3 up;
	
	mat4 viewMat;
	mat4 projMat;
	
	static mat4 MakePerspectiveProjectionMatrix(f32 winWidth, f32 winHeight, f32 _fov, f32 _farZ, f32 _nearZ) {
		f32 renderDistance = _farZ - _nearZ;
		f32 aspectRatio = winHeight / winWidth;
		f32 fovRad = 1.f / tanf(Radians(_fov * .5f));
		return mat4( //NOTE setting (1,1) to negative flips the y-axis
					aspectRatio * fovRad, 0, 0, 0,
					0, -fovRad, 0, 0,
					0, 0, _farZ / renderDistance, 1,
					0, 0, -(_farZ * _nearZ) / renderDistance, 0);
	}
	
	static mat4 MakeOrthographicProjection(f32 width, f32 height, f32 r, f32 l, f32 t, f32 b, f32 _farZ, f32 _nearZ) {
		f32 aspectRatio = width / height;
		
		f32 f = _farZ;
		f32 n = _nearZ;
		
		return mat4(2 / (r - l), 0, 0, 0,
					0, 2 / (b - t), 0, 0,
					0, 0, -2 / (f - n), 0,
					-(r + l) / (r - l), -(t + b) / (b - t), -(f + n) / (n - f), 1);
	}
};

#endif //DESHI_CAMERA_H