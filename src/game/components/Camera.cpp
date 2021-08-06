#include "Camera.h"
#include "../admin.h"
#include "../Keybinds.h"
#include "../../core/window.h"
#include "../../core/imgui.h"
#include "../../core/renderer.h"
#include "../../core/console.h"
#include "../../math/Math.h"
#include "../../utils/debug.h"

CameraInstance::CameraInstance(){
	layer = ComponentLayer_NONE;
	type  = ComponentType_Camera;
}

CameraInstance::CameraInstance(float fov, float nearZ, float farZ, bool freeCam) {
	layer = ComponentLayer_NONE;
	type = ComponentType_Camera;
	this->nearZ = nearZ;
	this->farZ = farZ;
	this->fov = fov;
	this->freeCamera = freeCam;

	position = {  4.f,   3.f,-4.f };
	rotation = { 28.f, -45.f, 0.f };
	
	this->forward = (vec3::FORWARD * mat4::RotationMatrix(rotation)).normalized();
	this->right   = vec3::UP.cross(forward).normalized();
	this->up      = right.cross(forward).normalized();
	this->viewMat = Math::LookAtMatrix(position, position + forward).Inverse();
	UpdateProjectionMatrix();
}

void CameraInstance::Update() {
	if(freeCamera){
		//NOTE this can happen whether the camera is free or not so move it out
		//of this scope once we implement that
		persist int wwidth = DengWindow->width; 
		persist int wheight = DengWindow->height;
		
		//clamp camera crotation
		Math::clamp(rotation.x, -89.9f, 89.9f);
		if(rotation.y > 1440.f || rotation.y < -1440.f){ rotation.y = 0.f; }
		
		//update direction vectors
		forward = (vec3::FORWARD * mat4::RotationMatrix(rotation)).normalized();
		right = vec3::UP.cross(forward).normalized();
		up = right.cross(forward).normalized();
		
		viewMat = Math::LookAtMatrix(position, position + forward).Inverse();
		
		//update renderer camera properties
		if (type == CameraMode_Orthographic) {
			float fw = ImGui::GetFontSize() / 2;
			
			switch (orthoview) {
				case FRONT:    position = vec3(0, 0, -999); rotation = vec3(0, 0, 0);     ImGui::DebugDrawText("FRONT (+Z)",  vec2(DengWindow->width - fw * 1.3 * sizeof("FRONT (+Z)"), DengWindow->height - 50));  break;
				case BACK:     position = vec3(0, 0, 999);  rotation = vec3(0, 180, 0);   ImGui::DebugDrawText("BACK (-Z)",   vec2(DengWindow->width - fw * 1.3 * sizeof("BACK (-Z)"), DengWindow->height - 50));   break;
				case RIGHT:    position = vec3(999, 0, 0);  rotation = vec3(0, -90, 0);   ImGui::DebugDrawText("RIGHT (+X)",  vec2(DengWindow->width - fw * 1.3 * sizeof("RIGHT (+X)"), DengWindow->height - 50));  break;
				case LEFT:     position = vec3(-999, 0, 0); rotation = vec3(0, 90, 0);    ImGui::DebugDrawText("LEFT (-X)",   vec2(DengWindow->width - fw * 1.3 * sizeof("LEFT (-X)"), DengWindow->height - 50));   break;
				case TOPDOWN:  position = vec3(0, 999, 0);  rotation = vec3(89.9, 0, 0);  ImGui::DebugDrawText("TOP (-Y)",    vec2(DengWindow->width - fw * 1.3 * sizeof("TOP (-Y)"), DengWindow->height - 50));    break;
				case BOTTOMUP: position = vec3(0, -999, 0); rotation = vec3(-89.9, 0, 0); ImGui::DebugDrawText("BOTTOM (+Y)", vec2(DengWindow->width - fw * 1.3 * sizeof("BOTTOM (+Y)"), DengWindow->height - 50)); break;
			}
			UpdateProjectionMatrix();
		}
		
		//redo projection matrix is window size changes
		if (DengWindow->width != wwidth || DengWindow->height != wheight) {
			wwidth = DengWindow->width;
			wheight = DengWindow->height;
			UpdateProjectionMatrix();
		}
		
		Render::UpdateCameraViewMatrix(viewMat);
		Render::UpdateCameraPosition(position);
		
	}
}

mat4 CameraInstance::MakePerspectiveProjection(){
	return Camera::MakePerspectiveProjectionMatrix(DengWindow->width, DengWindow->height, fov, farZ, nearZ);
}

mat4 CameraInstance::MakeOrthographicProjection() {
	//convert bounding box to camera space
	persist float zoom = 10;
	vec3 maxcam = Math::WorldToCamera3(vec3( zoom, zoom, zoom),  DengAdmin->mainCamera->viewMat);
	vec3 mincam = Math::WorldToCamera3(vec3(-zoom,-zoom,-zoom), DengAdmin->mainCamera->viewMat); 
	
	//make screen box from camera space bounding box
	float maxx = std::max(fabs(mincam.x), fabs(maxcam.x));
	float maxy = std::max(fabs(mincam.y), fabs(maxcam.y));
	float max  = std::max(maxx, maxy);
	
	float aspectRatio = (float)DengWindow->width / DengWindow->height;
	float r = max * aspectRatio, t = max;
	float l = -r, b = -t;
	
	persist float oloffsetx = 0;
	persist float oloffsety = 0;
	persist float offsetx = 0;
	persist float offsety = 0;
	persist vec2 initmouse;
	persist bool initoffset = false;
	
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
	
	return Camera::MakeOrthographicProjection(DengWindow->width, DengWindow->height, r, l, t, b, farZ, nearZ);
}

void CameraInstance::UpdateProjectionMatrix(){
	switch(mode){
		case(CameraMode_Perspective):default:{ projMat = MakePerspectiveProjection(); } break;
		case(CameraMode_Orthographic):       { projMat = MakeOrthographicProjection(); }break;
	}
	Render::UpdateCameraProjectionMatrix(projMat);
}

std::string CameraInstance::str(){
	return TOSTDSTRING("[c:yellow]CameraInstance Info:[c]",
					"\nPosition: ", position,
					"\nRotation: ", rotation,
					"\nNear Plane: ", nearZ,
					"\nFar Plane: ", farZ,
					"\nHorizontal FOV: ", fov,
					"\nMode: ", CameraModeStrings[mode],
					"\nStatic: ", (freeCamera)? "false" : "true");
}

////////////////////////////
//// saving and loading ////
////////////////////////////

std::string CameraInstance::SaveTEXT(){
	return TOSTDSTRING("\n>camera"
					"\nposition (",position.x,",",position.y,",",position.z,")"
					"\nrotation (",rotation.x,",",rotation.y,",",rotation.z,")"
					"\nmode     ", mode,
					"\nnear_z   ", nearZ,
					"\nfar_z    ", farZ,
					"\nfov      ", fov,
					"\n");
}

void CameraInstance::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR("CameraInstance::LoadDESH not setup");
}