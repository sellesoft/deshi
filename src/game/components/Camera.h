#pragma once
#ifndef COMPONENT_CAMERA_H
#define COMPONENT_CAMERA_H

#include "../../core/camera.h"

#include "Component.h"
#include "../../math/vec.h"
#include "../../math/mat.h"

struct CameraInstance : public Component, public Camera {
	CameraInstance();
	CameraInstance(float fov, float nearZ = .01f, float farZ = 1000.01f, bool freeCam = true);
	
	void Update() override;
	
	mat4 MakePerspectiveProjection();
	mat4 MakeOrthographicProjection();
	
	void UpdateProjectionMatrix();
	
	std::string str() override;
	std::string SaveTEXT() override;
	static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif //COMPONENT_CAMERA_H