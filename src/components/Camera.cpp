#include "Camera.h"

#include "../EntityAdmin.h"

Camera::Camera(EntityAdmin* a) : Component(a) {
	nearZ = .1f;
	farZ = 1000.1f;
	fieldOfView = 90.f;

	layer = CL2_RENDSCENE;
}

Camera::Camera(float fov, float nearZ, float farZ) {
	this->nearZ = nearZ;
	this->farZ = farZ;
	this->fieldOfView = fov;

	layer = CL2_RENDSCENE;
}

void Camera::Update() {

}
