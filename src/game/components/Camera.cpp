#include "Camera.h"
#include "../admin.h"
#include "../Keybinds.h"
#include "../systems/CanvasSystem.h"
#include "../../core.h"
#include "../../math/Math.h"
#include "../../scene/Scene.h"

Camera::Camera(){
	admin = g_admin;
	cpystr(name, "Camera", DESHI_NAME_SIZE);
	layer = ComponentLayer_NONE;
	comptype = ComponentType_Camera;
}

Camera::Camera(float fov, float nearZ, float farZ, bool freeCam){
	admin = g_admin;
	cpystr(name, "Camera", DESHI_NAME_SIZE);
	layer = ComponentLayer_NONE;
	comptype = ComponentType_Camera;
	
	this->nearZ = nearZ;
	this->farZ = farZ;
	this->fov = fov;
	this->freeCamera = freeCam;
	
	forward = (Vector3::FORWARD * Matrix4::RotationMatrix(rotation)).normalized();
	right = Vector3::UP.cross(forward).normalized();
	up = right.cross(forward).normalized();
	
	viewMat = Math::LookAtMatrix(position, position + forward).Inverse();
	UpdateProjectionMatrix();
}

void Camera::Update() {
	if(freeCamera){
		Window* window = DengWindow;
		Renderer* renderer = DengRenderer;
		
		//NOTE this can happen whether the camera is free or not so move it out
		//of this scope once we implement that
		static int wwidth = window->width; 
		static int wheight = window->height;
		
		//clamp camera rotation
		Math::clamp(rotation.x, -89.9f, 89.9f);
		if(rotation.y > 1440.f || rotation.y < -1440.f){ rotation.y = 0.f; }
		
		//update direction vectors
		forward = (Vector3::FORWARD * Matrix4::RotationMatrix(rotation)).normalized();
		right = Vector3::UP.cross(forward).normalized();
		up = right.cross(forward).normalized();
		
		
		
		
		target = position + forward;
		
		viewMat = Math::LookAtMatrix(position, target).Inverse();
		
		//update renderer camera properties
		if (type == CameraType_Orthographic) {
			float fw = ImGui::GetFontSize() / 2;
			
			switch (orthoview) {
				case FRONT:    position = Vector3(0, 0, -999); rotation = Vector3(0, 0, 0);     ImGui::DebugDrawText("FRONT (+Z)",  Vector2(window->width - fw * 1.3 * sizeof("FRONT (+Z)"), window->height - 50));  break;
				case BACK:     position = Vector3(0, 0, 999);  rotation = Vector3(0, 180, 0);   ImGui::DebugDrawText("BACK (-Z)",   Vector2(window->width - fw * 1.3 * sizeof("BACK (-Z)"), window->height - 50));   break;
				case RIGHT:    position = Vector3(999, 0, 0);  rotation = Vector3(0, -90, 0);   ImGui::DebugDrawText("RIGHT (+X)",  Vector2(window->width - fw * 1.3 * sizeof("RIGHT (+X)"), window->height - 50));  break;
				case LEFT:     position = Vector3(-999, 0, 0); rotation = Vector3(0, 90, 0);    ImGui::DebugDrawText("LEFT (-X)",   Vector2(window->width - fw * 1.3 * sizeof("LEFT (-X)"), window->height - 50));   break;
				case TOPDOWN:  position = Vector3(0, 999, 0);  rotation = Vector3(89.9, 0, 0);  ImGui::DebugDrawText("TOP (-Y)",    Vector2(window->width - fw * 1.3 * sizeof("TOP (-Y)"), window->height - 50));    break;
				case BOTTOMUP: position = Vector3(0, -999, 0); rotation = Vector3(-89.9, 0, 0); ImGui::DebugDrawText("BOTTOM (+Y)", Vector2(window->width - fw * 1.3 * sizeof("BOTTOM (+Y)"), window->height - 50)); break;
			}
			UpdateProjectionMatrix();
		}
		
		//redo projection matrix is window size changes
		if (window->width != wwidth || window->height != wheight) {
			wwidth = window->width;
			wheight = window->height;
			UpdateProjectionMatrix();
		}
		
		renderer->UpdateCameraViewMatrix(viewMat);
		renderer->UpdateCameraPosition(position);
		
	}
}

void Camera::UseOrthographicProjection() {
}

Matrix4 Camera::MakePerspectiveProjection(){
	float renderDistance = farZ - nearZ;
	float aspectRatio = f32(DengWindow->height) / f32(DengWindow->width);
	float fovRad = 1.f / tanf(RADIANS(fov * .5f));
	return Matrix4( //NOTE setting (1,1) to negative flips the y-axis
				   aspectRatio * fovRad, 0,	   0,							  0,
				   0,					-fovRad, 0,							  0,
				   0,					0,	   farZ / renderDistance,		  1,
				   0,					0,	   -(farZ*nearZ) / renderDistance, 0);
}

Matrix4 Camera::MakeOrthographicProjection() {
	//std::pair<Vector3, Vector3> bbox = admin->scene.SceneBoundingBox();
	//convert bounding box to camera space
	static float zoom = 10;
	Vector3 maxcam = Math::WorldToCamera3(Vector3( zoom, zoom, zoom),  admin->mainCamera->viewMat);
	Vector3 mincam = Math::WorldToCamera3(Vector3(-zoom,-zoom,-zoom), admin->mainCamera->viewMat); 
	
	//make screen box from camera space bounding box
	float maxx = std::max(fabs(mincam.x), fabs(maxcam.x));
	float maxy = std::max(fabs(mincam.y), fabs(maxcam.y));
	float max  = std::max(maxx, maxy);
	
	float aspectRatio = (float)DengWindow->width / DengWindow->height;
	float r = max * aspectRatio, t = max;
	float l = -r, b = -t;
	
	static float oloffsetx = 0;
	static float oloffsety = 0;
	static float offsetx = 0;
	static float offsety = 0;
	static Vector2 initmouse;
	static bool initoffset = false;
	
	PRINTLN(zoom);
	//orthographic view controls
	if (DengInput->KeyPressed(DengKeys.orthoZoomIn) && zoom > 0.0000000009) zoom -= zoom / 5;
	if (DengInput->KeyPressed(DengKeys.orthoZoomOut)) zoom += zoom / 5;
	
	if (DengInput->KeyPressed(DengKeys.orthoOffset)) initoffset = true;
	
	if (DengInput->KeyDown(DengKeys.orthoOffset)) {
		if (initoffset) {
			initmouse = DengInput->mousePos;
			initoffset = false;
		}
		offsetx = 0.0002 * zoom * (DengInput->mousePos.x - initmouse.x);
		offsety = 0.0002 * zoom * (DengInput->mousePos.y - initmouse.y);
	}
	
	if (DengInput->KeyReleased(DengKeys.orthoOffset)) {
		oloffsetx += offsetx; oloffsety += offsety;
		offsetx = 0; offsety = 0;
	}
	
	if (DengInput->KeyPressed(DengKeys.orthoResetOffset)) {
		oloffsetx = 0; oloffsety = 0;
	}
	
	r += zoom * aspectRatio; l = -r;
	r -= offsetx + oloffsetx; l -= offsetx + oloffsetx;
	t += zoom; b -= zoom;
	t += offsety + oloffsety; b += offsety + oloffsety;
	
	float f = admin->mainCamera->farZ;
	float n = admin->mainCamera->nearZ;
	
	return Matrix4(2/(r-l),      0,            0,            0,
				   0,            2/(b-t),      0,            0,
				   0,            0,            -2/(f-n),     0,
				   -(r+l)/(r-l), -(t+b)/(b-t), -(f+n)/(n-f), 1);
}

void Camera::UpdateProjectionMatrix(){
	switch(type){
		case(CameraType_Perspective):default:{ projMat = MakePerspectiveProjection(); } break;
		case(CameraType_Orthographic):{ projMat = MakeOrthographicProjection(); }break;
	}
	DengRenderer->UpdateCameraProjectionMatrix(projMat);
}

std::string Camera::str(){
	return TOSTRING("[c:yellow]Camera Info:[c]",
					"\nPosition: ", position,
					"\nRotation: ", rotation,
					"\nNear Plane: ", nearZ,
					"\nFar Plane: ", farZ,
					"\nHorizontal FOV: ", fov,
					"\nType: ", CameraTypeStrings[type],
					"\nStatic: ", (freeCamera)? "false" : "true");
}

////////////////////////////
//// saving and loading ////
////////////////////////////

std::string Camera::SaveTEXT(){
	return TOSTRING("\n>camera"
					"\nposition (",position.x,",",position.y,",",position.z,")"
					"\nrotation (",rotation.x,",",rotation.y,",",rotation.z,")"
					"\ntype     ", type,
					"\nnear_z   ", nearZ,
					"\nfar_z    ", farZ,
					"\nfov      ", fov,
					"\n");
}

void Camera::LoadDESH(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	ERROR("Camera::LoadDESH not setup");
}