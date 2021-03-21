#include "Camera.h"
#include "../EntityAdmin.h"
#include "../math/Math.h"
#include "../core.h"

Camera::Camera(EntityAdmin* a) : Component(a) {
	nearZ = 0.1f;
	farZ  = 1000.1f;
	fieldOfView = 90.f;
	
	admin->renderer->UpdateCameraProjectionProperties(fieldOfView, nearZ, farZ, true);
	UsePerspectiveProjection(fieldOfView, admin->window->width, admin->window->height, nearZ, farZ);
	admin->renderer->UpdateCameraProjectionMatrix(projectionMatrix);
	
	layer = CL2_RENDSCENE;
}

Camera::Camera(EntityAdmin*a, float fov, float nearZ, float farZ) : Component(a) {
	this->nearZ = nearZ;
	this->farZ = farZ;
	this->fieldOfView = fov;
	
	admin->renderer->UpdateCameraProjectionProperties(fieldOfView, nearZ, farZ, false);
	UsePerspectiveProjection(fieldOfView, admin->window->width, admin->window->height, nearZ, farZ);
	admin->renderer->UpdateCameraProjectionMatrix(projectionMatrix);
	
	layer = CL2_RENDSCENE;
}

/*
Matrix4 MakeViewMatrix(Camera* camera) {
	camera->lookDir = Math::SphericalToRectangularCoords(camera->target);
	return Math::LookAtMatrix(camera->position, camera->forward + camera->position, camera->up).Inverse();
}
*/

/*
Matrix4 MakeOrthographicMatrix(Scene* s, Camera* c, float screenWidth, float screenHeight) {
	std::pair<Vector3, Vector3> bbox = s->SceneBoundingBox();
	
	//convert bounding box to camera space
	Vector3 maxcam = Math::WorldToCamera(bbox.first,  c->viewMatrix).ToVector3();
	Vector3 mincam = Math::WorldToCamera(bbox.second, c->viewMatrix).ToVector3(); 
	
	//make screen box from camera space bounding box
	float maxx = std::max(fabs(mincam.x), fabs(maxcam.x));
	float maxy = std::max(fabs(mincam.y), fabs(maxcam.y));
	float max = std::max(maxx, maxy);
	
	float aspectRatio = screenHeight / screenWidth;
	float r = max * aspectRatio, t = max;
	float l = -r, b = -t;
	
	//TODO(r, sushi) implement ortho zooming by adjusting either the scale of objects or adjusting the bounding box
	//r += 10; l -= 10;
	//t += 10; b -= 10;
	
	return Matrix4(
				   2 / (r - l),			0,						0,												0,
				   0,						2 / (t - b),			0,												0,
				   0,						0,						-2 / (c->farZ - c->nearZ),						0,
				   -(r + l) / (r - l),		-(t + b) / (t - b),		-(c->farZ + c->nearZ) / (c->farZ - c->nearZ),	1);
	
}
*/

void Camera::UsePerspectiveProjection(float fovX, float width, float height, float nearZ, float farZ){
	float renderDistance = farZ - nearZ;
	float aspectRatio = height / width;
	float fovRad = 1.f / tanf(fovX * .5f * TO_RADIANS);
	projectionMatrix = Matrix4( //NOTE setting (1,1) to negative flips the y-axis
							   aspectRatio * fovRad, 0,	   0,							  0,
							   0,					-fovRad, 0,							  0,
							   0,					0,	   farZ / renderDistance,		  1,
							   0,					0,	   -(farZ*nearZ) / renderDistance, 0);
}

void Camera::UseOrthographicProjection(){}

void Camera::Update() {
	Window* window = admin->window;
	Renderer* renderer = admin->renderer;
	
	//clamp camera rotation
	rotation.x = Math::clamp(rotation.x, -89.9f, 89.9f);
	if(rotation.y > 1440.f || rotation.y < -1440.f){ rotation.y = 0.f; }
	
	//update direction vectors
	forward = (Vector3::FORWARD * Matrix4::RotationMatrix(rotation)).normalized();
	right = Vector3::UP.cross(forward).normalized();
	up = right.cross(forward).normalized();
	
	//update view matrix
	//TODO(o,delle) precalc this since we already get the direction vectors
	viewMatrix = Math::LookAtMatrix(position, position+forward).Inverse();
	
	//update renderer camera properties
	renderer->UpdateCameraViewMatrix(viewMatrix);
	renderer->UpdateCameraPosition(position);
	renderer->UpdateCameraRotation(rotation);
}
