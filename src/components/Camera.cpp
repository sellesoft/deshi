#include "Camera.h"
#include "../EntityAdmin.h"
#include "../math/Math.h"
#include "../core/deshi_glfw.h"
#include "../core/deshi_renderer.h"
#include "../core/deshi_input.h"
#include "../core/deshi_time.h"

#include "../animation/Model.h" //temp for debug

Camera::Camera(EntityAdmin* a) : Component(a) {
	Window* window = admin->window;
	Renderer* renderer = admin->renderer;
	
	nearZ = 0.1f;
	farZ  = 1000.1f;
	fieldOfView = 90.f;
	
	renderer->UpdateCameraProjectionProperties(fieldOfView, nearZ, farZ, false);
	UsePerspectiveProjection(fieldOfView, window->width, window->height, nearZ, farZ);
	renderer->UpdateCameraProjectionMatrix(projectionMatrix);
	
	layer = CL2_RENDSCENE;
}

Camera::Camera(float fov, float nearZ, float farZ) {
	this->nearZ = nearZ;
	this->farZ = farZ;
	this->fieldOfView = fov;
	
	layer = CL2_RENDSCENE;
}

Matrix4 MakeViewMatrix(Camera* camera) {
	//camera->lookDir = Vector3::FORWARD * Matrix3::RotationMatrixY(camera->rotation.y);
	
	camera->lookDir = Math::SphericalToRectangularCoords(camera->target);
	//camera->lookDir = camera->targetrect - camera->position;
	return Math::LookAtMatrix(camera->position, camera->lookDir + camera->position, camera->up).Inverse();
}

Matrix4 MakePerspectiveMatrix(Camera* camera, float screenWidth, float screenHeight) {
	float renderDistance = camera->farZ - camera->nearZ;
	float aspectRatio = screenHeight / screenWidth;
	float fovRad = 1.f / tanf(camera->fieldOfView * .5f * TO_RADIANS);
	
	return Matrix4( //NOTE setting (1,1) to negative flips the y-axis
				   aspectRatio * fovRad,	0,			0,												0,
				   0,					   -fovRad,	0,												0,
				   0,					   0,			camera->farZ / renderDistance,					1,
				   0,					   0,			(-camera->farZ*camera->nearZ) / renderDistance,	0);
}
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
	Time* time = admin->time;
	
	//clamp camera yaw (x-rotation)
	rotation.x = Math::clamp(rotation.x, -89.f, 89.f);
	
	//update direction vectors
	forward = (Vector4(Vector3::FORWARD, 0.f) * Matrix4::RotationMatrix(rotation)).ToVector3().normalized();
	right = Vector3::UP.cross(forward).normalized();
	up = right.cross(forward).normalized();
	
	//update view matrix
	
	//update projection matrix
	
	renderer->UpdateCameraPosition(position);
	renderer->UpdateCameraRotation(rotation);
	
	if (DengInput->KeyPressed(Key::NUMPADPLUS)) { 
		fieldOfView += 5;
		renderer->UpdateCameraProjectionProperties(fieldOfView, nearZ, farZ, false);
	}
	if (DengInput->KeyPressed(Key::NUMPADMINUS)) {
		fieldOfView -= 5;
		renderer->UpdateCameraProjectionProperties(fieldOfView, nearZ, farZ, false);
	}
	
	renderer->UpdateCameraViewMatrix(viewMatrix);
	
	//temp debugging
	if(DengInput->KeyPressed(Key::Z)){
		renderer->UpdateMeshBatchShader(0, 0, 3);
		renderer->UpdateMeshBatchShader(1, 0, 3);
	}
	if(DengInput->KeyPressed(Key::X)){
		renderer->UpdateMeshBatchShader(0, 0, 0);
		renderer->UpdateMeshBatchShader(1, 0, 0);
	}
	if(DengInput->KeyPressed(Key::F5)){
		renderer->ReloadShaders();
	}
	if(DengInput->KeyPressed(Key::B, INPUT_SHIFT_HELD)){
		Model box = Model::CreateBox(Vector3(1, 1, 1));
		uint32 id = renderer->LoadMesh(&box.mesh);
		renderer->TransformMeshMatrix(id, Matrix4::TranslationMatrix(position + forward*10));
	}
	
	//renderer->TransformMeshMatrix(0, Matrix4::RotationMatrixY(90.f * time->deltaTime));
}
