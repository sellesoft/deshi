#include "Camera.h"
#include "../../EntityAdmin.h"
#include "../../math/Math.h"
#include "../../core.h"
#include "../../scene/Scene.h"

Camera::Camera(EntityAdmin* a) : Component(a) {
	nearZ = 0.01f;
	farZ  = 1000.1f;
	fieldOfView = 90.f;
	
	UsePerspectiveProjection(fieldOfView, admin->window->width, admin->window->height, nearZ, farZ);
	
	name = "Camera";
	layer = NONE;
}

Camera::Camera(EntityAdmin*a, float fov, float nearZ, float farZ) : Component(a) {
	this->nearZ = nearZ;
	this->farZ = farZ;
	this->fieldOfView = fov;
	
	UsePerspectiveProjection(fieldOfView, admin->window->width, admin->window->height, nearZ, farZ);
	
	name = "Camera";
	layer = NONE;
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
	Vector3 maxcam = Math::WorldToCamera4(bbox.first,  c->viewMatrix).ToVector3();
	Vector3 mincam = Math::WorldToCamera4(bbox.second, c->viewMatrix).ToVector3(); 
	
	//make screen box from camera space bounding box
	float maxx = std::max(fabs(mincam.x), fabs(maxcam.x));
	float maxy = std::max(fabs(mincam.y), fabs(maxcam.y));
	float max = std::max(maxx, maxy);
	
	float aspectRatio = screenHeight / screenWidth;
	float r = max * aspectRatio, t = max;
	float l = -r, b = -t;
	
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
	float fovRad = 1.f / tanf(RADIANS(fovX * .5f));
	projectionMatrix = Matrix4( //NOTE setting (1,1) to negative flips the y-axis
							   aspectRatio * fovRad, 0,	   0,							  0,
							   0,					-fovRad, 0,							  0,
							   0,					0,	   farZ / renderDistance,		  1,
							   0,					0,	   -(farZ*nearZ) / renderDistance, 0);
	admin->renderer->UpdateCameraProjectionMatrix(projectionMatrix);
}

//TODO(sushi, Re) figure out why we cant see anything when we use this
void Camera::UseOrthographicProjection() {
	std::pair<Vector3, Vector3> bbox = admin->scene->SceneBoundingBox();

	//convert bounding box to camera space
	Vector3 maxcam = Math::WorldToCamera3(bbox.first,  admin->mainCamera->viewMatrix);
	Vector3 mincam = Math::WorldToCamera3(bbox.second, admin->mainCamera->viewMatrix); 

	//make screen box from camera space bounding box
	float maxx = std::max(fabs(mincam.x), fabs(maxcam.x));
	float maxy = std::max(fabs(mincam.y), fabs(maxcam.y));
	float max  = std::max(maxx, maxy);

	float aspectRatio = DengWindow->height / DengWindow->width;
	float r = max * aspectRatio, t = max;
	float l = -r, b = -t;

	r += 10 * aspectRatio; l -= 10;
	t += 10; b -= 10;

	float f = admin->mainCamera->farZ;
	float n = 0.01;//admin->mainCamera->nearZ;
	
	admin->renderer->UpdateCameraProjectionMatrix(Matrix4(
		2 / (r - l),			0,						0,					0,
		0,						2 / (t - b),			0,					0,
		0,						0,						-2 / (f - n),		0,
		-(r + l) / (r - l),		-(t + b) / (t - b),		-(f + n) / (f - n),	1));
}

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
	
	//UseOrthographicProjection();

	//update view matrix
	//TODO(delle,Op) precalc this since we already get the direction vectors
	viewMatrix = Math::LookAtMatrix(position, position+forward).Inverse();
	
	//update renderer camera properties
	renderer->UpdateCameraViewMatrix(viewMatrix);
	renderer->UpdateCameraPosition(position);
}
