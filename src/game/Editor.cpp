#include "Editor.h"

#include "admin.h"
#include "components/Camera.h"
#include "components/MeshComp.h"
#include "components/Physics.h"
#include "components/Collider.h"
#include "components/Light.h"
#include "components/Movement.h"
#include "components/Player.h"
#include "components/Orb.h"
#include "components/AudioSource.h"
#include "components/AudioListener.h"
#include "entities/PlayerEntity.h"
#include "../core.h"
#include "../math/Math.h"
#include "../scene/Scene.h"
#include "../geometry/Edge.h"

#include <iomanip> //std::put_time

///////////////////////////////////////////////////////////////////////////////////////////////////
//// inputs

Entity* Editor::SelectEntityRaycast(){
	vec3 pos = Math::ScreenToWorld(DengInput->mousePos, camera->projMat, camera->viewMat, DengWindow->dimensions);
	RenderedEdge3D* ray = new RenderedEdge3D(pos, camera->position);

	//NOTE sushi: this sort of works, but fails sometimes since the position of a larger obj
	//			  can be closer than a smaller one even though the small one appears in front
	//			  this can be mediated by sorting triangles in
	std::vector<Entity*> sorted_ents = admin->entities;
	std::sort(sorted_ents.begin(), sorted_ents.end(),
		[&](Entity* e1, Entity* e2) {
			return (e1->transform.position - camera->position).mag() < (e2->transform.position - camera->position).mag();
		});

	u32 closeindex = -1;
	f32 mint = -INFINITY;

	vec3 p0, p1, p2, normal, intersect;
	mat4 rot, transform;
	f32  t;
	int  index = 0;
	for(Entity* e : sorted_ents) {
		if(MeshComp* mc = e->GetComponent<MeshComp>()) {
			if(mc->mesh_visible) {
				Mesh* m = mc->mesh;
				for(Batch& b : m->batchArray){
					for(u32 i = 0; i < b.indexArray.size(); i += 3){
						transform = e->transform.TransformMatrix();
						p0 = b.vertexArray[b.indexArray[i + 0]].pos * transform;
						p1 = b.vertexArray[b.indexArray[i + 1]].pos * transform;
						p2 = b.vertexArray[b.indexArray[i + 2]].pos * transform;
						normal = b.vertexArray[b.indexArray[i + 0]].normal * transform;
						
						//NOTE sushi: our normal here is now based on whatever the vertices normal is when we load the model
						//			  so if we end up loading models and combining vertices again, this will break
						
						intersect = Math::VectorPlaneIntersect(p0, normal, ray->p[0], ray->p[1], t);
						rot = Matrix4::AxisAngleRotationMatrix(90, Vector4(normal, 0));
						
						if(((p1 - p0) * rot).dot(p0 - intersect) < 0 &&
						   ((p2 - p1) * rot).dot(p1 - intersect) < 0 &&
						   ((p0 - p2) * rot).dot(p2 - intersect) < 0 && t < 0) {
							DeshDebugLine(camera->position, intersect);
							DeshDebugLineCol(p0 + (p1 - p0) * rot, p0, Color::RED);
							DeshDebugLineCol(p1 + (p2 - p1) * rot, p1, Color::RED);
							DeshDebugLineCol(p2 + (p0 - p2) * rot, p2, Color::RED);
							DeshDebugLineCol(p0, intersect, Color::BLUE);
							DeshDebugLineCol(p1, intersect, Color::BLUE);
							DeshDebugLineCol(p2, intersect, Color::BLUE);
							
							
							DeshDebugTri(p0, p1, p2);

							if (t > mint) {
								closeindex = index;
								mint = t;
								PRINTLN(mint);
								goto entend;
							}
						
						}
					}
				}
			}
		}
		entend:
		index++;
	}

	if (closeindex != -1) return sorted_ents[closeindex];
	else return 0;
}

void Editor::TranslateEntity(Entity* e, TransformationAxis axis){
	
}

inline void HandleGrabbing(Entity* sel, Camera* c, EntityAdmin* admin, UndoManager* um) {
	static bool grabbingObj = false;
	
	if (!DengConsole->IMGUI_MOUSE_CAPTURE) { 
		if (DengInput->KeyPressed(DengKeys.grabSelectedObject) || grabbingObj) {
			//Camera* c = admin->mainCamera;
			grabbingObj = true;
			admin->controller.cameraLocked = true;
			
			//bools for if we're in an axis movement mode
			static bool xaxis = false;
			static bool yaxis = false;
			static bool zaxis = false;
			
			static bool initialgrab = true;
			
			static Vector3 initialObjPos;
			static float initialdist; 
			static Vector3 lastFramePos;
			
			//different cases for mode chaning
			if (DengInput->KeyPressed(Key::X)) {
				xaxis = true; yaxis = false; zaxis = false; 
				sel->transform.position = initialObjPos;
			}
			if (DengInput->KeyPressed(Key::Y)) {
				xaxis = false; yaxis = true; zaxis = false; 
				sel->transform.position = initialObjPos;
			}
			if (DengInput->KeyPressed(Key::Z)) {
				xaxis = false; yaxis = false; zaxis = true; 
				sel->transform.position = initialObjPos;
			}
			if (!(xaxis || yaxis || zaxis) && DengInput->KeyPressed(Key::ESCAPE)) { //|| DengInput->MousePressed(1)) {
				//stop grabbing entirely if press esc or right click w no grab mode on
				//TODO(sushi, In) figure out why the camera rotates violently when rightlicking to leave grabbing. probably because of the mouse moving to the object?
				xaxis = false; yaxis = false; zaxis = false; 
				sel->transform.position = initialObjPos;
				initialgrab = true; grabbingObj = false;
				admin->controller.cameraLocked= false;
				return;
			}
			if ((xaxis || yaxis || zaxis) && DengInput->KeyPressed(Key::ESCAPE)) {
				//leave grab mode if in one when pressing esc
				xaxis = false; yaxis = false; zaxis = false; 
				sel->transform.position = initialObjPos; initialgrab = true;
			}
			if (DengInput->KeyPressed(MouseButton::LEFT)) {
				//drop the object if left click
				xaxis = false; yaxis = false; zaxis = false;
				initialgrab = true; grabbingObj = false;  
				admin->controller.cameraLocked = false;
				if(initialObjPos != sel->transform.position){
					um->AddUndoTranslate(&sel->transform, &initialObjPos, &sel->transform.position);
				}
				return;
			}
			
			if (DengInput->KeyPressed(MouseButton::SCROLLDOWN)) initialdist -= 1;
			if (DengInput->KeyPressed(MouseButton::SCROLLUP))   initialdist += 1;
			
			//set mouse to obj position on screen and save that position
			if (initialgrab) {
				Vector2 screenPos = Math::WorldToScreen2(sel->transform.position, c->projMat, c->viewMat, DengWindow->dimensions);
				DengWindow->SetCursorPos(screenPos);
				initialObjPos = sel->transform.position;
				initialdist = (initialObjPos - c->position).mag();
				initialgrab = false;
			}
			
			if (!(xaxis || yaxis || zaxis)) {
				
				Vector3 nuworldpos = Math::ScreenToWorld(DengInput->mousePos, c->projMat,
														 c->viewMat, DengWindow->dimensions);
				
				Vector3 newpos = nuworldpos;
				
				newpos *= Math::WorldToLocal(admin->mainCamera->position);
				newpos.normalize();
				newpos *= initialdist;
				newpos *= Math::LocalToWorld(admin->mainCamera->position);
				
				sel->transform.position = newpos;
				if (Physics* p = sel->GetComponent<Physics>()) {
					p->position = newpos;
				}
				
			}
			else if (xaxis) {
				Vector3 pos = Math::ScreenToWorld(DengInput->mousePos, admin->mainCamera->projMat,
												  admin->mainCamera->viewMat, DengWindow->dimensions);
				pos *= Math::WorldToLocal(admin->mainCamera->position);
				pos.normalize();
				pos *= 1000;
				pos *= Math::LocalToWorld(admin->mainCamera->position);
				
				Vector3 planeinter;
				
				if (Math::AngBetweenVectors(Vector3(c->forward.x, 0, c->forward.z), c->forward) > 60) {
					planeinter = Math::VectorPlaneIntersect(initialObjPos, Vector3::UP, c->position, pos);
				}
				else {
					planeinter = Math::VectorPlaneIntersect(initialObjPos, Vector3::FORWARD, c->position, pos);
				}
				
				sel->transform.position = Vector3(planeinter.x, initialObjPos.y, initialObjPos.z);
				if (Physics* p = sel->GetComponent<Physics>()) {
					p->position = Vector3(planeinter.x, initialObjPos.y, initialObjPos.z);
				}
			}
			else if (yaxis) {
				Vector3 pos = Math::ScreenToWorld(DengInput->mousePos, admin->mainCamera->projMat,
												  admin->mainCamera->viewMat, DengWindow->dimensions);
				pos *= Math::WorldToLocal(admin->mainCamera->position);
				pos.normalize();
				pos *= 1000;
				pos *= Math::LocalToWorld(admin->mainCamera->position);
				
				Vector3 planeinter;
				
				if (Math::AngBetweenVectors(Vector3(c->forward.x, 0, c->forward.z), c->forward) > 60) {
					planeinter = Math::VectorPlaneIntersect(initialObjPos, Vector3::RIGHT, c->position, pos);
				}
				else {
					planeinter = Math::VectorPlaneIntersect(initialObjPos, Vector3::FORWARD, c->position, pos);
				}
				sel->transform.position = Vector3(initialObjPos.x, planeinter.y, initialObjPos.z);
				if (Physics* p = sel->GetComponent<Physics>()) {
					p->position = Vector3(initialObjPos.x, planeinter.y, initialObjPos.z);
				}	
				
			}
			else if (zaxis) {
				Vector3 pos = Math::ScreenToWorld(DengInput->mousePos, admin->mainCamera->projMat,
												  admin->mainCamera->viewMat, DengWindow->dimensions);
				pos *= Math::WorldToLocal(admin->mainCamera->position);
				pos.normalize();
				pos *= 1000;
				pos *= Math::LocalToWorld(admin->mainCamera->position);
				
				Vector3 planeinter;
				
				if (Math::AngBetweenVectors(Vector3(c->forward.x, 0, c->forward.z), c->forward) > 60) {
					planeinter = Math::VectorPlaneIntersect(initialObjPos, Vector3::UP, c->position, pos);
				}
				else {
					planeinter = Math::VectorPlaneIntersect(initialObjPos, Vector3::RIGHT, c->position, pos);
				}
				sel->transform.position = Vector3(initialObjPos.x, initialObjPos.y, planeinter.z);
				if (Physics* p = sel->GetComponent<Physics>()) {
					p->position = Vector3(initialObjPos.x, initialObjPos.y, planeinter.z);
				}
				
			}
			lastFramePos = sel->transform.position;
		} //if(DengInput->KeyPressed(DengKeys.grabSelectedObject) || grabbingObj)
	} //if(!DengConsole->IMGUI_MOUSE_CAPTURE)
}

void Editor::RotateEntity(Entity* e, TransformationAxis axis){
	
}

inline void HandleRotating(Entity* sel, Camera* c, EntityAdmin* admin, UndoManager* um) {
	static bool rotatingObj = false;
	
	if (!DengConsole->IMGUI_MOUSE_CAPTURE) { 
		if (DengInput->KeyPressed(DengKeys.rotateSelectedObject) || rotatingObj) {
			rotatingObj = true;
			admin->controller.cameraLocked = true;
			
			//bools for if we're in an axis movement mode
			static bool xaxis = false;
			static bool yaxis = false;
			static bool zaxis = false;
			
			static Vector2 origmousepos = DengInput->mousePos;
			
			static bool initialrot = true;
			
			static Vector3 initialObjRot;
			
			//different cases for mode chaning
			if (DengInput->KeyPressed(Key::X)) {
				xaxis = true; yaxis = false; zaxis = false; 
				sel->transform.rotation = initialObjRot;
			}
			if (DengInput->KeyPressed(Key::Y)) {
				xaxis = false; yaxis = true; zaxis = false; 
				sel->transform.rotation = initialObjRot;
			}
			if (DengInput->KeyPressed(Key::Z)) {
				xaxis = false; yaxis = false; zaxis = true; 
				sel->transform.rotation = initialObjRot;
			}
			if (!(xaxis || yaxis || zaxis) && DengInput->KeyPressed(Key::ESCAPE)) {
				//stop rotating entirely if press esc or right click w no rotate mode on
				xaxis = false; yaxis = false; zaxis = false; 
				sel->transform.rotation = initialObjRot;
				initialrot = true; rotatingObj = false;
				admin->controller.cameraLocked = false;
				return;
			}
			if ((xaxis || yaxis || zaxis) && DengInput->KeyPressed(Key::ESCAPE)) {
				//leave rotation mode if in one when pressing esc
				xaxis = false; yaxis = false; zaxis = false; 
				sel->transform.rotation = initialObjRot; initialrot = true;
			}
			if (DengInput->KeyPressed(MouseButton::LEFT)) {
				//drop the object if left click
				xaxis = false; yaxis = false; zaxis = false;
				initialrot = true; rotatingObj = false;  
				admin->controller.cameraLocked = false;
				if(initialObjRot != sel->transform.rotation){
					um->AddUndoRotate(&sel->transform, &initialObjRot, &sel->transform.rotation);
				}
				return;
			}
			
			if (initialrot) {
				initialObjRot = sel->transform.rotation;
				initialrot = false;
				origmousepos = DengInput->mousePos;
			}
			
			//TODO(sushi, InMa) implement rotating over an arbitrary axis in a nicer way everntually
			//TODO(sushi, In) make rotation controls a little more nice eg. probably just make it how far along the screen the mouse is to determine it.
			if (!(xaxis || yaxis || zaxis)) {
				
				Vector2 center = Vector2(DengWindow->width / 2, DengWindow->height / 2);
				Vector2 mousepos = DengInput->mousePos;
				
				Vector2 ctm = mousepos - center;
				
				Vector2 cleft = Vector2(0, DengWindow->height / 2);
				Vector2 cright = Vector2(DengWindow->width, DengWindow->height / 2);
				
				Vector2 mp = DengInput->mousePos;
				
				Vector2 cltmp = mp - cleft;
				
				float dist = (cleft - cright).normalized().dot(cltmp);
				
				float ratio = dist / (cright - cleft).mag();
				
				
				float ang = 360 * ratio;
				
				LOG(ang);
				
				//make angle go between 360 instead of -180 and 180
				//if (ang < 0) {
				//	ang = 180 + (180 + ang);
				//}
				
				sel->transform.rotation = Matrix4::AxisAngleRotationMatrix(ang, Vector4((sel->transform.position - c->position).normalized(), 0)).Rotation();
				
				sel->transform.rotation.x = DEGREES(sel->transform.rotation.x);
				sel->transform.rotation.y = DEGREES(sel->transform.rotation.y);
				sel->transform.rotation.z = DEGREES(sel->transform.rotation.z);
				
			}
			else if (xaxis) {
				Vector2 center = Vector2(DengWindow->width / 2, DengWindow->height / 2);
				Vector2 mousepos = DengInput->mousePos;
				
				Vector2 ctm = mousepos - center;
				
				float ang = Math::AngBetweenVectors(ctm, origmousepos - center);
				
				if (ang < 0) {
					ang = 180 + (180 + ang);
				}
				
				sel->transform.rotation.z = ang;
			}
			else if (yaxis) {
				Vector2 center = Vector2(DengWindow->width / 2, DengWindow->height / 2);
				Vector2 mousepos = DengInput->mousePos;
				
				Vector2 ctm = mousepos - center;
				
				float ang = Math::AngBetweenVectors(ctm, origmousepos - center);
				
				if (ang < 0) {
					ang = 180 + (180 + ang);
				}
				
				sel->transform.rotation.y = ang;
			}
			else if (zaxis) {
				Vector2 center = Vector2(DengWindow->width / 2, DengWindow->height / 2);
				Vector2 mousepos = DengInput->mousePos;
				
				Vector2 ctm = mousepos - center;
				
				float ang = Math::AngBetweenVectors(ctm, origmousepos - center);
				
				if (ang < 0) {
					ang = 180 + (180 + ang);
				}
				
				sel->transform.rotation.x = ang;
			}
		} //if(DengInput->KeyPressed(DengKeys.grabSelectedObject) || grabbingObj)
	} //if(!admin->IMGUI_MOUSE_CAPTURE)
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//// imgui debug funcs



ImVec4 ColToVec4(Color p) {
	return ImVec4((float)p.r / 255, (float)p.g / 255, (float)p.b / 255, p.a / 255);
}

//functions to simplify the usage of our DebugLayer
namespace ImGui {
	void BeginDebugLayer() {
		//ImGui::SetNextWindowSize(ImVec2(DengWindow->width, DengWindow->height));
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ColToVec4(Color(0, 0, 0, 0)));
		ImGui::Begin("DebugLayer", 0, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
	}
	
	//not necessary, but I'm adding it for clarity in code
	void EndDebugLayer() {
		ImGui::PopStyleColor();
		ImGui::End();
	}
	
	void DebugDrawCircle(Vector2 pos, float radius, Color color) {
		ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(pos.x, pos.y), radius, ImGui::GetColorU32(ColToVec4(color)));
	}
	
	void DebugDrawCircle3(Vector3 pos, float radius, Color color) {
		Camera* c = g_admin->mainCamera;
		Vector2 windimen = DengWindow->dimensions;
		Vector2 pos2 = Math::WorldToScreen2(pos, c->projMat, c->viewMat, windimen);
		ImGui::GetBackgroundDrawList()->AddCircle(pos2.ToImVec2(), radius, ImGui::GetColorU32(ColToVec4(color)));
	}
	
	void DebugDrawLine(Vector2 pos1, Vector2 pos2, Color color) {
		Math::ClipLineToBorderPlanes(pos1, pos2, DengWindow->dimensions);
		ImGui::GetBackgroundDrawList()->AddLine(pos1.ToImVec2(), pos2.ToImVec2(), ImGui::GetColorU32(ColToVec4(color)));
	}
	
	void DebugDrawLine3(Vector3 pos1, Vector3 pos2, Color color) {
		Camera* c = g_admin->mainCamera;
		Vector2 windimen = DengWindow->dimensions;
		
		Vector3 pos1n = Math::WorldToCamera3(pos1, c->viewMat);
		Vector3 pos2n = Math::WorldToCamera3(pos2, c->viewMat);
		
		if (Math::ClipLineToZPlanes(pos1n, pos2n, c)) {
			ImGui::GetBackgroundDrawList()->AddLine(
													Math::CameraToScreen2(pos1n, c->projMat, windimen).ToImVec2(), 
													Math::CameraToScreen2(pos2n, c->projMat, windimen).ToImVec2(), ImGui::GetColorU32(ColToVec4(color)));
		}
	}
	
	void DebugDrawText(const char* text, Vector2 pos, Color color) {		
		ImGui::SetCursorPos(pos.ToImVec2());
		
		ImGui::PushStyleColor(ImGuiCol_Text, ColToVec4(color));
		ImGui::Text(text);
		ImGui::PopStyleColor();
	}
	
	void DebugDrawText3(const char* text, Vector3 pos, Color color, Vector2 twoDoffset) {
		Camera* c = g_admin->mainCamera;
		Vector2 windimen = DengWindow->dimensions;
		
		Vector3 posc = Math::WorldToCamera3(pos, c->viewMat);
		if(Math::ClipLineToZPlanes(posc, posc, c)){
			ImGui::SetCursorPos((Math::CameraToScreen2(posc, c->projMat, windimen) + twoDoffset).ToImVec2());
			ImGui::PushStyleColor(ImGuiCol_Text, ColToVec4(color));
			ImGui::Text(text);
			ImGui::PopStyleColor();
		}
	}
	
	void DebugDrawTriangle(Vector2 p1, Vector2 p2, Vector2 p3, Color color) {
		DebugDrawLine(p1, p2);
		DebugDrawLine(p2, p3);
		DebugDrawLine(p3, p1);
	}
	
	void DebugFillTriangle(Vector2 p1, Vector2 p2, Vector2 p3, Color color) {
		ImGui::GetBackgroundDrawList()->AddTriangleFilled(p1.ToImVec2(), p2.ToImVec2(), p3.ToImVec2(), ImGui::GetColorU32(ColToVec4(color)));
	}
	
	void DebugDrawTriangle3(Vector3 p1, Vector3 p2, Vector3 p3, Color color) {
		DebugDrawLine3(p1, p2, color);
		DebugDrawLine3(p2, p3, color);
		DebugDrawLine3(p3, p1, color);
	}
	
	//TODO(sushi, Ui) add triangle clipping to this function
	void DebugFillTriangle3(Vector3 p1, Vector3 p2, Vector3 p3, Color color) {
		Vector2 p1n = Math::WorldToScreen(p1, g_admin->mainCamera->projMat, g_admin->mainCamera->viewMat, DengWindow->dimensions).ToVector2();
		Vector2 p2n = Math::WorldToScreen(p2, g_admin->mainCamera->projMat, g_admin->mainCamera->viewMat, DengWindow->dimensions).ToVector2();
		Vector2 p3n = Math::WorldToScreen(p3, g_admin->mainCamera->projMat, g_admin->mainCamera->viewMat, DengWindow->dimensions).ToVector2();
		
		ImGui::GetBackgroundDrawList()->AddTriangleFilled(p1n.ToImVec2(), p2n.ToImVec2(), p3n.ToImVec2(), ImGui::GetColorU32(ColToVec4(color)));
	}
	
	void DebugDrawGraphFloat(Vector2 pos, float inval, float sizex, float sizey) {
		//display in value
		ImGui::SetCursorPos(ImVec2(pos.x, pos.y - 10));
		ImGui::Text(TOSTRING(inval).c_str());
		
		//how much data we store
		static int prevstoresize = 100;
		static int storesize = 100;
		
		//how often we update
		static int fupdate = 1;
		static int frame_count = 0;
		
		static float maxval = inval + 5;
		static float minval = inval - 5;
		
		//if (inval > maxval) maxval = inval;
		//if (inval < minval) minval = inval;
		
		if (inval > maxval || inval < minval) {
			maxval = inval + 5;
			minval = inval - 5;
		}
		//real values and printed values
		static std::vector<float> values(storesize);
		static std::vector<float> pvalues(storesize);
		
		//if changing the amount of data we're storing we have to reverse
		//each data set twice to ensure the data stays in the right place when we move it
		if (prevstoresize != storesize) {
			std::reverse(values.begin(), values.end());    values.resize(storesize);  std::reverse(values.begin(), values.end());
			std::reverse(pvalues.begin(), pvalues.end());  pvalues.resize(storesize); std::reverse(pvalues.begin(), pvalues.end());
			prevstoresize = storesize;
		}
		
		std::rotate(values.begin(), values.begin() + 1, values.end());
		
		//update real set if we're not updating yet or update the graph if we are
		if (frame_count < fupdate) {
			values[values.size() - 1] = inval;
			frame_count++;
		}
		else {
			float avg = Math::average(values.begin(), values.end(), storesize);
			std::rotate(pvalues.begin(), pvalues.begin() + 1, pvalues.end());
			pvalues[pvalues.size() - 1] = std::floorf(avg);
			
			frame_count = 0;
		}
		
		ImGui::PushStyleColor(ImGuiCol_PlotLines, ColToVec4(Color(0, 255, 200, 255)));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ColToVec4(Color(20, 20, 20, 255)));
		
		ImGui::SetCursorPos(pos.ToImVec2());
		ImGui::PlotLines("", &pvalues[0], pvalues.size(), 0, 0, minval, maxval, ImVec2(sizex, sizey));
		
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
	}
	
	void CopyButton(const char* text) {
		if(ImGui::Button("Copy")){ ImGui::LogToClipboard(); ImGui::LogText(text); ImGui::LogFinish(); }
	}
	
	bool InputVector2(const char* id, Vector2* vecPtr, bool inputUpdate) {
		ImGui::SetNextItemWidth(-FLT_MIN);
		if(inputUpdate) {
			return ImGui::InputFloat2(id, (float*)vecPtr); 
		} else {
			return ImGui::InputFloat2(id, (float*)vecPtr, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
		}
	}
	
	bool InputVector3(const char* id, Vector3* vecPtr, bool inputUpdate) {
		ImGui::SetNextItemWidth(-FLT_MIN);
		if(inputUpdate) {
			return ImGui::InputFloat3(id, (float*)vecPtr); 
		} else {
			return ImGui::InputFloat3(id, (float*)vecPtr, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue); 
		}
	}
	
	bool InputVector4(const char* id, Vector4* vecPtr, bool inputUpdate) {
		ImGui::SetNextItemWidth(-FLT_MIN);
		if(inputUpdate) {
			return ImGui::InputFloat4(id, (float*)vecPtr);
		} else {
			return ImGui::InputFloat4(id, (float*)vecPtr, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue); 
		}
	}
	
	void AddPadding(float x){
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x);
	}
	
} //namespace ImGui



////////////////////////////////////////////////////////////////////////////////////////////////////
//// interface (and some nasty global stuff that should prob get compressed out)



bool  WinHovFlag = false;
float menubarheight   = 0;
float debugbarheight  = 0;
float debugtoolswidth = 0;
float padding         = 0.95f;
float fontw = 0;
float fonth = 0;

//// defines to make repetitve things less ugly and more editable ////
//check if mouse is over window so we can prevent mouse from being captured by engine
#define WinHovCheck if (IsWindowHovered()) WinHovFlag = true 
//allows me to manually set padding so i have a little more control than ImGui gives me (I think idk lol)
#define SetPadding SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2)

//current palette:
//https://lospec.com/palette-list/slso8
//TODO(sushi, Ui) implement menu style file loading sort of stuff yeah
//TODO(sushi, Ui) standardize what UI element each color belongs to
struct {
	Color c1 = Color(0x0d2b45);
	Color c2 = Color(0x203c56);
	Color c3 = Color(0x544e68);
	Color c4 = Color(0x8d697a);
	Color c5 = Color(0xd08159);
	Color c6 = Color(0xffaa5e);
	Color c7 = Color(0xffd4a3);
	Color c8 = Color(0xffecd6);
	Color c9 = Color(20, 20, 20);
}colors;

std::vector<std::string> files;
std::vector<std::string> textures;


void Editor::MenuBar() {
	using namespace ImGui;
	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleColor(ImGuiCol_PopupBg,   ColToVec4(Color(20, 20, 20, 255)));
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ColToVec4(Color(20, 20, 20, 255)));
	
	if(BeginMainMenuBar()) { WinHovCheck; 
		menubarheight = GetWindowHeight();
		if(BeginMenu("File")) { WinHovCheck; 
			
			if (MenuItem("New"))  admin->Reset();
			if (MenuItem("Save")) admin->SaveDESH("save.desh");
			if (BeginMenu("Save As")) {
				static char buff[255] = {};
				if(ImGui::InputText("##saveas_input", buff, 255, ImGuiInputTextFlags_EnterReturnsTrue)) {
					std::string s(buff); s += ".desh";
					admin->SaveDESH(s.c_str());
				}
				EndMenu();
			} 
			if (BeginMenu("Load")) { WinHovCheck;
				static bool fopen = false;
				static std::vector<std::string> saves;
				
				if (!fopen) {
					saves = deshi::iterateDirectory(deshi::dirSaves());
					fopen = false;
				}
				
				for_n(i, saves.size()) {
					if (MenuItem(saves[i].c_str())) {
						admin->LoadDESH(saves[i].c_str());
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if(BeginMenu("Spawn")) { WinHovCheck; 
			for (int i = 0; i < files.size(); i++) {
				if (files[i].find(".obj") != std::string::npos) {
					if(MenuItem(files[i].c_str())) { DengConsole->ExecCommand("load_obj", files[i]); }
				}
			}
			EndMenu();
		}//agh
		if (BeginMenu("Window")) { WinHovCheck; 
			if (MenuItem("Entity Inspector")) showDebugTools = !showDebugTools;
			if (MenuItem("Debug Bar"))        showDebugBar = !showDebugBar;
			if (MenuItem("DebugLayer"))       showDebugLayer = !showDebugLayer;
			if (MenuItem("Timers"))           showTimes = !showTimes;
			if (MenuItem("ImGui Demo"))       showImGuiDemoWindow = !showImGuiDemoWindow;
			EndMenu();
		}
		if (BeginMenu("State")) { WinHovCheck;
			if (MenuItem("Play"))   admin->ChangeState(GameState_Play);
			if (MenuItem("Debug"))  admin->ChangeState(GameState_Debug);
			if (MenuItem("Editor")) admin->ChangeState(GameState_Editor);
			if (MenuItem("Menu"))   admin->ChangeState(GameState_Menu);
			EndMenu();
		}
		EndMainMenuBar();
	}
	
	PopStyleColor(2);
	PopStyleVar(2);
}


//NOTE sushi: this is really bad, but i just need it to work for now
inline void EventsMenu(Entity* current, bool reset = false) {
	using namespace ImGui;
	
	std::vector<Entity*> entities = g_admin->entities;
	static Entity* other = nullptr;
	static Component* selcompr = nullptr;
	static Component* selcompl = nullptr;
	
	if (reset) {
		other = nullptr;
		selcompr = nullptr;
		selcompl = nullptr;
		return;
	}
	
	ImGui::SetNextWindowSize(ImVec2(DengWindow->width / 2, DengWindow->height / 2));
	ImGui::SetNextWindowPos(ImVec2(DengWindow->width / 4, DengWindow->height / 4));
	Begin("EventsMenu", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
	ImVec2 winpos = ImGui::GetWindowPos();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	WinHovCheck;
	float padx = (GetWindowWidth() - (GetWindowWidth() * padding)) / 2;
	float pady = (GetWindowHeight() - (GetWindowHeight() * padding)) / 2;
	
	float width = GetWindowWidth();
	float height = GetWindowHeight();
	
	
	SetCursorPos(ImVec2(padx, pady));
	if (BeginChild("ahahaha", ImVec2(width * padding, height * padding))) {
		ImDrawList* drawListc = ImGui::GetWindowDrawList();
		
		float cwidth = GetWindowWidth();
		float cheight = GetWindowHeight();
		
		winpos = GetWindowPos();
		
		SetCursorPos(ImVec2(padx, (GetWindowHeight() - fonth) / 2));
		Text(current->name);
		
		float maxlen = 0;
		
		for (Entity* e : entities) maxlen = std::max(maxlen, (float)std::string(e->name).size());
		//TODO(sushi, ClUi) make the right list scrollable once it reaches a certain point
		//or just redesign this entirely
		
		
		//if we haven't selected an entity display other entities
		if (other == nullptr) {
			float inc = cheight / (entities.size());
			int i = 1;
			for (Entity* e : entities) {
				if (e != current) {
					PushID(i);
					if (e->connections.find(current) != e->connections.end()) {
						float lx = 1.2 * (padx + CalcTextSize(current->name).x);
						float ly = cheight / 2;
						
						float rx = cwidth - maxlen * fontw * 1.2 - padx * 0.8 + CalcTextSize(e->name).x / 2;
						float ry = i * inc + CalcTextSize(e->name).y / 2;
						
						drawListc->AddLine(
										   ImVec2(winpos.x + lx, winpos.y + ly),
										   ImVec2(winpos.x + rx, winpos.y + ry),
										   GetColorU32(ColToVec4(Color::WHITE)));
					}
					
					SetCursorPos(ImVec2(cwidth - maxlen * fontw * 1.2 - padx, i * inc));
					if (Button(e->name, ImVec2(maxlen * fontw * 1.2, fonth))) {
						other = e;
					}
					i++;
					PopID();
					
				}
			}
		}
		else {
			
			
			
			float rightoffset = cwidth - (float)std::string(other->name).size() * fontw - padx;
			
			
			//display other entity and it's components
			SetCursorPos(ImVec2(rightoffset, (cheight - fonth) / 2 ));
			Text(other->name);
			
			//if we select a comp for each ent, show options for connecting an event
			if (selcompr && selcompl) {
				int currevent = selcompl->event;
				if (Combo("##events_combo2", &currevent, EventStrings, IM_ARRAYSIZE(EventStrings))) {
					switch (currevent) {
						case Event_NONE:
						selcompl->sender->RemoveReceiver(selcompr);
						selcompl->event = Event_NONE;
						selcompl->entity->connections.erase(selcompr->entity);
						selcompr->entity->connections.erase(selcompl->entity);
						break;
						default:
						selcompl->sender->AddReceiver(selcompr);
						selcompl->event = (u32)currevent;
						selcompr->entity->connections.insert(selcompl->entity);
						selcompl->entity->connections.insert(selcompr->entity);
						break;
					}
				}
				
				if (selcompl->sender->HasReceiver(selcompr)) {
					float lx = 1.2 * (padx * 2 + CalcTextSize(current->name).x + CalcTextSize(selcompl->name).x) / 2;
					float ly = cheight / 2;
					
					float rx = rightoffset * 0.8 + ((float)std::string(selcompr->name).size() * fontw) / 2;
					float ry = cheight / 2;
					
					drawListc->AddLine(
									   ImVec2(winpos.x + lx, winpos.y + ly),
									   ImVec2(winpos.x + rx, winpos.y + ry),
									   GetColorU32(ColToVec4(Color::WHITE)));
				}
				
			}
			
			//TODO(sushi, Op) make this run only when we first select the entity
			float maxlen = 0;
			for (Component* c : other->components) maxlen = std::max(maxlen, (float)std::string(c->name).size());
			int i = 0; //increment for IDs
			if (!selcompr) {
				float inc = cheight / (other->components.size() + 1);
				int o = 1;
				
				for (Component* c : other->components) {
					SetCursorPos(ImVec2(rightoffset * 0.8, o * inc));
					PushID(i);
					if (selcompl && selcompl->sender->HasReceiver(c)) {
						float lx = 1.2 * (padx * 2 + CalcTextSize(current->name).x + (CalcTextSize(selcompl->name).x) / 2);
						float ly = cheight / 2;
						
						float rx = rightoffset * 0.8 + CalcTextSize(c->name).y / 2;
						float ry = o * inc + CalcTextSize(c->name).x / 2;
						
						drawListc->AddLine(
										   ImVec2(winpos.x + lx, winpos.y + ly),
										   ImVec2(winpos.x + rx, winpos.y + ry),
										   GetColorU32(ColToVec4(Color::WHITE)));
					}
					if (Button(c->name, ImVec2(maxlen * fontw * 1.2, fonth))) {
						selcompr = c;
					}
					i++; o++;
					PopID();
				}
			}
			else {
				SetCursorPos(ImVec2(rightoffset * 0.8, (cheight - fonth) / 2));
				PushID(i);
				if (Button(selcompr->name)) {
					selcompr = nullptr;
				}
				i++;
				PopID();
			}
			
			maxlen = 0;
			for (Component* c : current->components) maxlen = std::max(maxlen, (float)std::string(c->name).size());
			
			//display initial entities components
			if (!selcompl) {
				float inc = cheight / (current->components.size() + 1);
				int o = 1;
				
				for (Component* c : current->components) {
					SetCursorPos(ImVec2(1.2 * (padx * 2 + (float)std::string(current->name).size() * fontw), o * inc));
					PushID(i);
					if (selcompr && selcompr->sender->HasReceiver(c)) {
						float lx = 1.2 * (padx * 2 + CalcTextSize(current->name).x + CalcTextSize(selcompl->name).x / 2);
						float ly = o * inc + CalcTextSize(c->name).x / 2;
						
						float rx = rightoffset * 0.8 + CalcTextSize(c->name).y / 2;
						float ry = cheight / 2;
						
						drawListc->AddLine(
										   ImVec2(winpos.x + lx, winpos.y + ly),
										   ImVec2(winpos.x + rx, winpos.y + ry),
										   GetColorU32(ColToVec4(Color::WHITE)));
					}
					if (Button(c->name, ImVec2(maxlen * fontw * 1.2, fonth))) {
						selcompl = c;
					}
					i++; o++;
					PopID();
				}
			}
			else {
				SetCursorPos(ImVec2(1.2 * (padx * 2 + (float)std::string(current->name).size() * fontw), cheight / 2 - fonth / 2));
				PushID(i);
				if (Button(selcompl->name)) {
					selcompl = nullptr;
				}
				i++;
				PopID();
			}
			
			SetCursorPos(ImVec2(0, 0));
			if (Button("Back")) { 
				other = nullptr; 
				selcompl = nullptr;
				selcompr = nullptr;
			}
		}
		EndChild();
	}
	else {
		const char* sorry = "No other entities...";
		SetCursorPos(ImVec2(width - sizeof("No other entities...") * fontw, height / 2));
		Text(sorry);
	}
	
	
	End();
}

//TODO(delle,Cl) remove this once integrated into EntitiesTab
inline void ComponentsMenu(Entity* sel) {
	using namespace ImGui;
	int tree_flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoAutoOpenOnLog;
	SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * 0.95)) / 2);
	if (BeginChild("SelectedComponentsWindow", ImVec2(GetWindowWidth() * 0.95, 500), true)) { WinHovCheck;
		//if (ImGui::BeginTable("SelectedComponents", 1)) {
		//ImGui::TableSetupColumn("Comp", ImGuiTableColumnFlags_WidthFixed);
		for (Component* c : sel->components) {
			switch (c->comptype) {
				case ComponentType_Physics:
				if (TreeNodeEx("Physics", tree_flags)) {
					dyncast(d, Physics, c);
					Text("Velocity     "); SameLine(); InputVector3("##phys_vel", &d->velocity);             Separator();
					Text("Accelertaion "); SameLine(); InputVector3("##phys_accel", &d->acceleration);       Separator();
					Text("Rot Velocity "); SameLine(); InputVector3("##phys_rotvel", &d->rotVelocity);       Separator();
					Text("Rot Accel    "); SameLine(); InputVector3("##phys_rotaccel", &d->rotAcceleration); Separator();
					Text("Elasticity   "); SameLine();
					ImGui::SetNextItemWidth(-FLT_MIN); InputFloat("##phys_elastic", &d->elasticity); Separator();
					Text("Mass         "); SameLine();
					ImGui::SetNextItemWidth(-FLT_MIN); InputFloat("##phys_mass", &d->mass); Separator();
					Text("Kinetic Fric "); SameLine();
					ImGui::SetNextItemWidth(-FLT_MIN); InputFloat("##phys_mass", &d->kineticFricCoef); Separator();
					Checkbox("Static Position", &d->isStatic); Separator();
					Checkbox("Static Rotation", &d->staticRotation);
					Checkbox("2D Physics", &d->twoDphys);
					TreePop();
				}
				break;
				
				case ComponentType_Collider: {
					dyncast(col, Collider, c);
					switch (col->type) {
						case ColliderType_Box: {
							if (TreeNodeEx("Box Collider", tree_flags)) {
								dyncast(d, BoxCollider, col);
								Text("Half Dims    "); SameLine(); InputVector3("coll_halfdims", &d->halfDims);
								TextWrapped("TODO sushi implement collider commands/events menu");
								TreePop();
							}
							break;
						}
						case ColliderType_AABB: {
							if (TreeNodeEx("AABB Collider", tree_flags)) {
								dyncast(d, AABBCollider, col);
								Text("Half Dims    "); SameLine(); InputVector3("coll_halfdims", &d->halfDims);
								TextWrapped("TODO sushi implement collider commands/events menu");
								TreePop();
							}
							break;
						}
						case ColliderType_Sphere: {
							if (TreeNodeEx("Sphere Collider", tree_flags)) {
								dyncast(d, SphereCollider, col);
								Text("Radius       "); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); 
								InputFloat("coll_radius", &d->radius);
								TextWrapped("TODO sushi implement collider commands/events menu");
								TreePop();
							}
							break;
						}
					}
					break;
				}
				
				case ComponentType_AudioListener:
				if (TreeNodeEx("Audio Listener", tree_flags)) {
					Text("TODO sushi implement audio listener editing");
					TreePop();
				}
				break;
				
				case ComponentType_AudioSource:
				if (TreeNodeEx("Audio Source", tree_flags)) {
					Text("TODO sushi implement audio source editing");
					TreePop();
				}
				break;
				
				case ComponentType_Light:
				if (TreeNodeEx("Light", tree_flags)) {
					dyncast(d, Light, c);
					Text("Brightness   "); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); 
					InputFloat("brightness", &d->brightness); Separator();
					Text("Position     "); SameLine(); InputVector3("position", &d->position); Separator();
					Text("Direction    "); SameLine(); InputVector3("direction", &d->direction); Separator();
					
					
					TreePop();
				}
				break;
				
				case ComponentType_OrbManager:
				if (TreeNodeEx("Orbs", tree_flags)) {
					dyncast(d, OrbManager, c);
					Text("Orb count   "); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); 
					InputInt("orbcount", &d->orbcount);
					TreePop();
				}
				break;
				
				case ComponentType_Movement:
				if (TreeNodeEx("Movement", tree_flags)) {
					dyncast(d, Movement, c);
					Text("Ground Accel  "); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); 
					InputFloat("gndaccel", &d->gndAccel);
					Text("Air Accel     "); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); 
					InputFloat("airaccel", &d->airAccel);
					Text("Max Walk Speed"); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
					InputFloat("maxwalk ", &d->maxWalkingSpeed);
					TreePop();
				}
				break;
				
				case ComponentType_MeshComp:
				if (TreeNodeEx("Mesh", tree_flags)) {
					dyncast(d, MeshComp, c);
					Text(TOSTRING("Mesh ID: ", d->meshID).c_str());
					Text(TOSTRING("Visible: ", d->mesh_visible).c_str());
					TreePop();
				}
				break;
			}
			
			//TableNextColumn(); //TableNextRow();
			//SetPadding; Text(c->name);
			//SameLine(CalcItemWidth() + 20);
			//if (Button("Del")) {
			//	sel->RemoveComponent(c);
			//}
		}
		//ImGui::EndTable();
		
		EndChild();
	}
}

inline void EntitiesTab(EntityAdmin* admin, float fontsize){
	using namespace ImGui;
	
	PushStyleColor(ImGuiCol_ChildBg, ColToVec4(Color(25, 25, 25)));
	SetPadding; if(BeginChild("entityListScroll", ImVec2(GetWindowWidth() * 0.95, 100), false)) { WinHovCheck; 
		if (admin->entities.size() == 0) {
			float time = DengTime->totalTime;
			std::string str1 = "Nothing yet...";
			float strlen1 = (fontsize - (fontsize / 2)) * str1.size();
			for (int i = 0; i < str1.size(); i++) {
				SetCursorPos(ImVec2((GetWindowSize().x - strlen1) / 2 + i * (fontsize / 2), (GetWindowSize().y - fontsize) / 2 + sin(10 * time + cos(10 * time + (i * M_PI / 2)) + (i * M_PI / 2))));
				Text(str1.substr(i, 1).c_str());
			}
		}
		else {
			if (BeginTable("split3", 4, ImGuiTableFlags_BordersInner)) {
				
				std::string str1 = "ID";
				float strlen1 = (fontsize - (fontsize / 2)) * str1.size();
				
				TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
				TableSetupColumn("Vis", ImGuiTableColumnFlags_WidthFixed);
				TableSetupColumn("Name");
				TableSetupColumn("Components");
				
				int counter = 0;
				for (Entity* entity : admin->entities) {
					counter++;	
					PushID(counter);
					TableNextRow(); TableNextColumn();
					std::string id = std::to_string(entity->id);
					MeshComp* m = entity->GetComponent<MeshComp>();
					
					//SetCursorPosX((GetColumnWidth() - (fontsize - (fontsize / 2)) * id.size()) / 2);
					if (ImGui::Button(id.c_str())) {
						admin->editor.selected.clear();
						admin->editor.selected.push_back(entity);
						//if(m) DengRenderer->SetSelectedMesh(m->meshID);
					}
					TableNextColumn();
					
					//TODO(UiEnt, sushi) implement visibility for things other than meshes like lights, etc.
					if (m) {
						if (m->mesh_visible) {
							if (SmallButton("O")) m->ToggleVisibility();
						}
						else {
							if (SmallButton("X")) m->ToggleVisibility();
						}
					}
					else {
						Light* l = entity->GetComponent<Light>();
						if (l) {
							//TODO(sushi, UiCl) find a nicer way of indicating light on/off later
							if (l->active) {
								if (SmallButton("L")) l->active = false;
							}
							else {
								if (SmallButton("l")) l->active = true;
							}
						}
						else {
							Text("NM");
						}
					}
					
					TableNextColumn();
					static bool rename = false;
					static char buff[64] = {};
					static char ogname[64] = {}; //TODO(delle,Op) maybe optimize this by making one buffer before loop for all entities
					static int renameid = 0;
					if(!rename) Text(TOSTRING(" ", entity->name).c_str());
					if (IsItemClicked()) {
						renameid = counter;
						rename = true;
						cpystr(buff, entity->name, 63);
						cpystr(ogname, entity->name, 63);
					}
					
					if(rename) DengConsole->IMGUI_KEY_CAPTURE = true;
					if (rename && counter == renameid) {
						if (InputText("##ent_name_input", buff, sizeof(buff), ImGuiInputTextFlags_EnterReturnsTrue)) {
							cpystr(entity->name, buff, 63);
							rename = false;
							DengConsole->IMGUI_KEY_CAPTURE = false;
						}
						if (DengInput->KeyPressed(Key::ESCAPE)) {
							cpystr(entity->name, ogname, 63);
							rename = false;
							DengConsole->IMGUI_KEY_CAPTURE = false;
						}
					}
					
					TableNextColumn();
					static b32 showevents = false;
					static Entity* shownent = nullptr;
					if (Button("Events")) {
						if (!showevents) {
							shownent = entity;
							showevents = true;
						}
						else if (shownent != entity) {
							shownent = entity;
						}
						else {
							shownent = nullptr;
							showevents = false;
							EventsMenu(nullptr, true);
						}
					}
					
					if (showevents && shownent == entity) EventsMenu(shownent);
					
					SameLine();
					if (Button("Del")) {
						g_admin->DeleteEntity(entity);
					}
					PopID();
				}
				ImGui::EndTable();
			}
		}
		EndChild();
	}
	PopStyleColor();
	
	Separator();
	
	//// selected entity inspector panel ////
	Entity* sel = admin->editor.selected.size() ? admin->editor.selected[0] : 0;
	if(!sel) return;
	PushStyleVar(ImGuiStyleVar_IndentSpacing, 5.0f);
	SetPadding; if (BeginChild("EntityInspector", ImVec2(GetWindowWidth() * 0.95f, GetWindowHeight() * .9f), true, ImGuiWindowFlags_NoScrollbar)) { WinHovCheck;
		
		//// name ////
		SetPadding; Text(TOSTRING(sel->id, ":").c_str()); 
		SameLine(); SetNextItemWidth(-FLT_MIN); InputText("##ent_name_input", sel->name, 64, 
														  ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		
		//// transform ////
		int tree_flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoAutoOpenOnLog;
		SetNextItemOpen(true, ImGuiCond_Once);
		if (TreeNodeEx("Transform", tree_flags)){
			vec3 oldVec = sel->transform.position;
			
			Text("Position    "); SameLine();
			if(InputVector3("##ent_pos", &sel->transform.position)){
				if(Physics* p = sel->GetComponent<Physics>()){
					p->position = sel->transform.position;
					admin->editor.undo_manager.AddUndoTranslate(&sel->transform, &oldVec, &p->position);
				}else{
					admin->editor.undo_manager.AddUndoTranslate(&sel->transform, &oldVec, &sel->transform.position);
				}
			}Separator();
			
			oldVec = sel->transform.rotation;
			Text("Rotation    "); SameLine(); 
			if(InputVector3("##ent_rot", &sel->transform.rotation)){
				if(Physics* p = sel->GetComponent<Physics>()){
					p->rotation = sel->transform.rotation;
					admin->editor.undo_manager.AddUndoRotate(&sel->transform, &oldVec, &p->rotation);
				}else{
					admin->editor.undo_manager.AddUndoRotate(&sel->transform, &oldVec, &sel->transform.rotation);
				}
			}Separator();
			
			oldVec = sel->transform.scale;
			Text("Scale       "); SameLine(); 
			if(InputVector3("##ent_scale",   &sel->transform.scale)){
				if(Physics* p = sel->GetComponent<Physics>()){
					p->scale = sel->transform.scale;
					admin->editor.undo_manager.AddUndoScale(&sel->transform, &oldVec, &p->scale);
				}else{
					admin->editor.undo_manager.AddUndoScale(&sel->transform, &oldVec, &sel->transform.scale);
				}
			}Separator();
			TreePop();
		}
		
		//// components ////
		for(Component* c : sel->components){
			switch(c->comptype){
				//mesh
				case ComponentType_MeshComp:{
					MeshComp* mc = dyncasta(MeshComp, c);
					if(mc && TreeNodeEx("Mesh", tree_flags)){
						MeshVk& mvk = DengRenderer->meshes[mc->meshID];
						Text("Mesh     "); SameLine(); SetNextItemWidth(-1); 
						if(BeginCombo("##mesh_combo", mvk.name)){ WinHovCheck;
							for_n(i, DengRenderer->meshes.size()){
								if(DengRenderer->meshes[i].base && Selectable(DengRenderer->meshes[i].name, mc->meshID == i)){
									mc->ChangeMesh(i);
								}
							}
							EndCombo();}
						
						u32 mesh_batch_idx = 0;
						Text("Batch    "); SameLine(); SetNextItemWidth(-1); 
						if(BeginCombo("##mesh_batch_combo", mc->mesh->batchArray[mesh_batch_idx].name)){ WinHovCheck;
							for_n(i, mc->mesh->batchArray.size()){
								if(Selectable(mc->mesh->batchArray[i].name, mesh_batch_idx == i)){
									mesh_batch_idx = i; 
								}
							}
							EndCombo();
						}
						
						Text("Material "); SameLine(); SetNextItemWidth(-1); 
						if(BeginCombo("##mesh_mat_combo", DengRenderer->materials[mvk.primitives[mesh_batch_idx].materialIndex].name)){ WinHovCheck;
							for_n(i, DengRenderer->materials.size()){
								if(Selectable(DengRenderer->materials[i].name, mvk.primitives[mesh_batch_idx].materialIndex == i)){
									DengRenderer->UpdateMeshBatchMaterial(mc->meshID, mesh_batch_idx, i);
								}
							}
							EndCombo();
						}
						
						Indent();{
							Text("Shader "); SameLine(); SetNextItemWidth(-1);
							if(BeginCombo("##mesh_shader_combo", ShaderStrings[DengRenderer->materials[mvk.primitives[mesh_batch_idx].materialIndex].shader])){ WinHovCheck;
								for_n(i, IM_ARRAYSIZE(ShaderStrings)){
									if(Selectable(ShaderStrings[i], DengRenderer->materials[mvk.primitives[mesh_batch_idx].materialIndex].shader == i)){
										DengRenderer->UpdateMaterialShader(mvk.primitives[mesh_batch_idx].materialIndex, i);
									}
								}
								EndCombo();
							}
							
							Text("Albedo   "); SameLine(); SetNextItemWidth(-1);
							if(BeginCombo("##mesh_albedo_combo", DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_idx].materialIndex].albedoID].filename)){ WinHovCheck;
								for_n(i, textures.size()){
									if(Selectable(textures[i].c_str(), strcmp(DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_idx].materialIndex].albedoID].filename, textures[i].c_str()) == 0)){
										DengRenderer->UpdateMaterialTexture(mvk.primitives[mesh_batch_idx].materialIndex, 0,
																			DengRenderer->LoadTexture(textures[i].c_str(), TextureType_Albedo));
									}
								}
								EndCombo();
							}
							
							Text("Normal   "); SameLine(); SetNextItemWidth(-1);
							if(BeginCombo("##mesh_normal_combo", DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_idx].materialIndex].normalID].filename)){ WinHovCheck;
								for_n(i, textures.size()){
									if(Selectable(textures[i].c_str(), strcmp(DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_idx].materialIndex].normalID].filename, textures[i].c_str()) == 0)){
										DengRenderer->UpdateMaterialTexture(mvk.primitives[mesh_batch_idx].materialIndex, 0,
																			DengRenderer->LoadTexture(textures[i].c_str(), TextureType_Normal));
									}
								}
								EndCombo();
							}
							
							Text("Specular "); SameLine(); SetNextItemWidth(-1);
							if(BeginCombo("##mesh_specular_combo", DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_idx].materialIndex].specularID].filename)){ WinHovCheck;
								for_n(i, textures.size()){
									if(Selectable(textures[i].c_str(), strcmp(DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_idx].materialIndex].specularID].filename, textures[i].c_str()) == 0)){
										DengRenderer->UpdateMaterialTexture(mvk.primitives[mesh_batch_idx].materialIndex, 0,
																			DengRenderer->LoadTexture(textures[i].c_str(), TextureType_Specular));
									}
								}
								EndCombo();
							}
							
							Text("Light    "); SameLine(); SetNextItemWidth(-1);
							if(BeginCombo("##mesh_light_combo", DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_idx].materialIndex].lightID].filename)){ WinHovCheck;
								for_n(i, textures.size()){
									if(Selectable(textures[i].c_str(), strcmp(DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_idx].materialIndex].lightID].filename, textures[i].c_str()) == 0)){
										DengRenderer->UpdateMaterialTexture(mvk.primitives[mesh_batch_idx].materialIndex, 0,
																			DengRenderer->LoadTexture(textures[i].c_str(), TextureType_Light));
									}
								}
								EndCombo();
							}
							
						}Unindent();
						TreePop();
					}
				}break;
				
				//mesh2D
				//TODO() implement mesh2D component inspector
				
				//physics
				case ComponentType_Physics:
					if (TreeNodeEx("Physics", tree_flags)) {
						dyncast(d, Physics, c);
						Text("Velocity     "); SameLine(); InputVector3("##phys_vel", &d->velocity);             Separator();
						Text("Accelertaion "); SameLine(); InputVector3("##phys_accel", &d->acceleration);       Separator();
						Text("Rot Velocity "); SameLine(); InputVector3("##phys_rotvel", &d->rotVelocity);       Separator();
						Text("Rot Accel    "); SameLine(); InputVector3("##phys_rotaccel", &d->rotAcceleration); Separator();
						Text("Elasticity   "); SameLine();
						ImGui::SetNextItemWidth(-FLT_MIN); InputFloat("##phys_elastic", &d->elasticity); Separator();
						Text("Mass         "); SameLine();
						ImGui::SetNextItemWidth(-FLT_MIN); InputFloat("##phys_mass", &d->mass); Separator();
						Text("Kinetic Fric "); SameLine();
						ImGui::SetNextItemWidth(-FLT_MIN); InputFloat("##phys_mass", &d->kineticFricCoef); Separator();
						Checkbox("Static Position", &d->isStatic); Separator();
						Checkbox("Static Rotation", &d->staticRotation);
						Checkbox("2D Physics", &d->twoDphys);
						TreePop();
					}
					break;
				
				//colliders
				case ComponentType_Collider:{
					Collider* coll = dyncasta(Collider, c);
					if(coll && TreeNodeEx("Collider", tree_flags)){
						//Text("Shape "); SameLine(); SetNextItemWidth(-1);
						//if(BeginCombo("##coll_type_combo", )){ WinHovCheck;
						//for_n(i, IM_ARRAYSIZE(ColliderTypeStrings)){
						//if(Selectable(ColliderTypeStrings[i], coll->type == i)){
						//
						//}
						//}
						//EndCombo();
						//}
						Text("Collider changing not setup");
						
						switch(coll->type){
							case ColliderType_Box:{
								BoxCollider* coll_box = dyncasta(BoxCollider, coll);
								Text("Half Dims "); SameLine(); InputVector3("##coll_halfdims", &coll_box->halfDims);
							}break;
							case ColliderType_AABB:{
								
							}break;
							case ColliderType_Sphere:{
								
							}break;
							case ColliderType_Landscape:{
								
							}break;
						}
						
						
						TreePop();
					}
				}break;
				
				//audio listener
				case ComponentType_AudioListener:{
					
				}break;
				
				//audio source
				case ComponentType_AudioSource:{
					
				}break;
				
				//camera
				case ComponentType_Camera:{
					
				}break;
				
				//light
				case ComponentType_Light:
					if (TreeNodeEx("Light", tree_flags)) {
						dyncast(d, Light, c);
						Text("Brightness   "); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
						InputFloat("brightness", &d->brightness); Separator();
						Text("Position     "); SameLine(); InputVector3("position", &d->position); Separator();
						Text("Direction    "); SameLine(); InputVector3("direction", &d->direction); Separator();


						TreePop();
					}
					break;
				
				//orb manager
				case ComponentType_OrbManager:{
					
				}break;
				
				//door
				case ComponentType_Door:{
					
				}break;
				
				//player
				case ComponentType_Player:{
					
				}break;
				
				//movement
				case ComponentType_Movement:{
					if (TreeNodeEx("Movement", tree_flags)) {
						dyncast(d, Movement, c);
						Text("Ground Accel    "); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
						InputFloat("gndaccel ", &d->gndAccel);
						Text("Air Accel       "); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
						InputFloat("airaccel ", &d->airAccel);
						Text("Jump Impulse    "); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
						InputFloat("jimp     ", &d->jumpImpulse);
						Text("Max Walk Speed  "); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
						InputFloat("maxwalk  ", &d->maxWalkingSpeed);
						Text("Max Run Speed   "); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
						InputFloat("maxrun   ", &d->maxRunningSpeed);
						Text("Max Crouch Speed"); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
						InputFloat("maxcrouch", &d->maxCrouchingSpeed);
						TreePop();
					}
				}break;
			}
		}
		
		//// add/remove component ////
		
		
		EndChild(); //CreateMenu
	}
	PopStyleVar(); //ImGuiStyleVar_IndentSpacing
}

enum TwodPresets : u32 {
	Twod_NONE = 0, Twod_Line, Twod_Triangle, Twod_Square, Twod_NGon, Twod_Image, 
};

inline void CreateTab(EntityAdmin* admin, float fontsize){
	using namespace ImGui;
	
	//// creation variables ////
	local_persist const char* presets[] = {"None", "AABB", "Box", "Sphere", "2D", "Player"};
	local_persist const char* premades[] = { "Player" };
	local_persist int current_preset = 0;
	local_persist int current_premade = 0;
	local_persist char entity_name[64] = {};
	local_persist vec3 entity_pos{}, entity_rot{}, entity_scale = Vector3::ONE;
	local_persist bool comp_audiolistener{}, comp_audiosource{}, comp_collider{}, comp_mesh{};
	local_persist bool comp_light{}, comp_physics{}, comp_2d{}, comp_player{};
	local_persist const char* colliders[] = {"None", "Box", "AABB", "Sphere"};
	local_persist int  collider_type = ColliderType_NONE;
	local_persist vec3 collider_halfdims = Vector3::ONE;
	local_persist f32  collider_radius  = 1.f;
	local_persist bool collider_trigger = false;
	local_persist bool collider_nocollide = false;
	local_persist char collider_command[64] = {};
	local_persist const char* mesh_name;
	local_persist u32  mesh_id = -1, mesh_instance_id = 0;
	local_persist f32  light_strength = 1.f;
	local_persist vec3 physics_velocity{}, physics_accel{}, physics_rotVel{}, physics_rotAccel{};
	local_persist f32  physics_elasticity = .5f, physics_mass = 1.f;
	local_persist bool physics_staticPosition{}, physics_staticRotation{};
	local_persist const char* twods[] = {"None", "Line", "Triangle", "Square", "N-Gon", "Image"};
	local_persist int  twod_type = 0, twod_vert_count = 0;
	local_persist u32  twod_id = -1;
	local_persist vec4 twod_color = vec4::ONE;
	local_persist f32  twod_radius = 1.f;
	local_persist std::vector<vec2> twod_verts;
	u32 entity_id = admin->entities.size();
	
	//// create panel ////
	PushStyleVar(ImGuiStyleVar_IndentSpacing, 5.0f);
	SetPadding; if (BeginChild("CreateMenu", ImVec2(GetWindowWidth() * 0.95f, GetWindowHeight() * .9f), true)) { WinHovCheck;
		if (Button("Create")){
			//create components
			AudioListener* al = 0;
			if(comp_audiolistener){ 
				al = new AudioListener(entity_pos, Vector3::ZERO, entity_rot); 
			}
			AudioSource* as = 0;
			if(comp_audiosource) {
				as = new AudioSource();
			}
			Collider* coll = 0;
			if(comp_collider){
				switch(collider_type){
					case ColliderType_Box:{
						coll = new BoxCollider(collider_halfdims, physics_mass, 0, Event_NONE, collider_nocollide);
					}break;
					case ColliderType_AABB:{
						coll = new AABBCollider(collider_halfdims, physics_mass, 0, Event_NONE, collider_nocollide);
					}break;
					case ColliderType_Sphere:{
						coll = new SphereCollider(collider_radius, physics_mass, 0, Event_NONE, collider_nocollide);
					}break;
				}
			}
			MeshComp* mc = 0;
			if(comp_mesh){
				u32 new_mesh_id = DengRenderer->CreateMesh(mesh_id, mat4::TransformationMatrix(entity_pos, entity_rot, entity_scale));
				mesh_name = DengRenderer->meshes[mesh_id].name;
				mc = new MeshComp(new_mesh_id, mesh_instance_id);
			}
			Light* light = 0;
			if(comp_light){
				light = new Light(entity_pos, entity_rot, light_strength);
			}
			Physics* phys = 0;
			Movement* move = 0;
			Player* pl = 0;
			if(comp_physics){
				phys = new Physics(entity_pos, entity_rot, physics_velocity, physics_accel, physics_rotVel,
								   physics_rotAccel, physics_elasticity, physics_mass, physics_staticPosition);
				if(comp_audiolistener) al->velocity = physics_velocity;
				if(comp_player) { move = new Movement(phys); pl = new Player(move); }
			}
			
			
			
			admin->CreateEntity({ al, as, coll, mc, light, phys, move, pl }, entity_name,
								Transform(entity_pos, entity_rot, entity_scale));
			
			
			
			
		}Separator();
		
		//// premades ////
		SetPadding; Text("Premades   ");  SameLine(); SetNextItemWidth(-1);
		if (BeginMenu("##premades")) {
			WinHovCheck;
			if(MenuItem("Player")){
				
				if (!admin->player) {
					PlayerEntity* pl = new PlayerEntity(Transform(Vector3::ZERO, Vector3::ZERO, Vector3::ZERO));
					admin->CreateEntity(pl);
					admin->player = pl;
				}
				else {
					ERROR("Player has already been created.");
				}
				
			}
			
			
			EndCombo();
			
		}
		
		
		//// presets ////
		SetPadding; Text("Presets    "); SameLine(); SetNextItemWidth(-1); 
		if(Combo("##preset_combo", &current_preset, presets, IM_ARRAYSIZE(presets))){ WinHovCheck;
			switch(current_preset){
				case(0):default:{
					comp_audiolistener = comp_audiosource = comp_collider = comp_mesh = comp_light = comp_physics = comp_player = false;
					collider_type = ColliderType_NONE;
					collider_halfdims = Vector3::ONE;
					collider_radius  = 1.f;
					mesh_id = mesh_instance_id = 0;
					light_strength = 1.f;
					physics_velocity = Vector3::ZERO; physics_accel    = Vector3::ZERO; 
					physics_rotVel   = Vector3::ZERO; physics_rotAccel = Vector3::ZERO;
					physics_elasticity = .5f; physics_mass = 1.f;
					physics_staticPosition = physics_staticRotation = true;
					cpystr(entity_name, TOSTRING("default", entity_id).c_str(), 63);
				}break;
				case(1):{
					comp_audiolistener = comp_audiosource = comp_light = comp_player = false;
					comp_collider = comp_mesh = comp_physics = true;
					collider_type = ColliderType_AABB;
					collider_halfdims = Vector3::ONE;
					mesh_id = 0;
					physics_staticPosition = physics_staticRotation = true;
					cpystr(entity_name, TOSTRING("aabb", entity_id).c_str(), 63);
				}break;
				case(2):{
					comp_audiolistener = comp_audiosource = comp_light = comp_player = false;
					comp_collider = comp_mesh = comp_physics = true;
					collider_type = ColliderType_Box;
					collider_halfdims = Vector3::ONE;
					mesh_id = 0;
					physics_staticPosition = physics_staticRotation = true;
					cpystr(entity_name, TOSTRING("box", entity_id).c_str(), 63);
				}break;
				case(3):{
					comp_audiolistener = comp_audiosource = comp_light = comp_player = false;
					comp_collider = comp_mesh = comp_physics = true;
					collider_type = ColliderType_Sphere;
					collider_radius = 1.f;
					mesh_id = DengRenderer->GetBaseMeshID("sphere.obj");
					physics_staticPosition = physics_staticRotation = true;
					cpystr(entity_name, TOSTRING("sphere", entity_id).c_str(), 63);
				}break;
				case(4):{
					comp_audiolistener = comp_audiosource = comp_collider = comp_mesh = comp_physics = comp_light = comp_player = false;
					comp_2d = true;
					twod_id = -1;
					twod_type = 2;
					twod_vert_count = 3;
					twod_radius = 25.f;
					twod_verts.resize(3);
					twod_verts[0] = {-100.f, 0.f}; twod_verts[1] = {0.f, 100.f}; twod_verts[2] = {100.f, 0.f};
					entity_pos = {700.f, 400.f, 0};
					cpystr(entity_name, "twod", 63);
				}break;
				case(5):{
					comp_light = false;
					comp_audiolistener = comp_audiosource = comp_collider = comp_mesh = comp_physics = comp_player = true;
					collider_type = ColliderType_AABB; //TODO(delle,PhCl) ideally cylinder/capsule collider
					collider_halfdims = vec3(1, 2, 1);
					mesh_id = DengRenderer->GetBaseMeshID("bmonkey.obj");
					physics_staticPosition = physics_staticRotation = false;
					physics_elasticity = 0.f;
					cpystr(entity_name, "player", 63);
				}break;
			}
			if(mesh_id < DengRenderer->meshes.size()) mesh_name = DengRenderer->meshes[mesh_id].name;
		}
		
		SetPadding; Text("Name: "); 
		SameLine(); SetNextItemWidth(-1); InputText("##unique_id", entity_name, 64, ImGuiInputTextFlags_EnterReturnsTrue | 
													ImGuiInputTextFlags_AutoSelectAll);
		
		//// transform ////
		int tree_flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoAutoOpenOnLog;
		SetNextItemOpen(true, ImGuiCond_Once);
		if (TreeNodeEx("Transform", tree_flags)){
			Text("Position     "); SameLine(); InputVector3("entity_pos",   &entity_pos);   Separator();
			Text("Rotation     "); SameLine(); InputVector3("entity_rot",   &entity_rot);   Separator();
			Text("Scale        "); SameLine(); InputVector3("entity_scale", &entity_scale);
			TreePop();
		}
		
		//// toggle components ////
		SetNextItemOpen(true, ImGuiCond_Once);
		if (TreeNodeEx("Components", tree_flags)){
			Checkbox("Mesh", &comp_mesh);
			SameLine(); Checkbox("Physics", &comp_physics); 
			SameLine(); Checkbox("Collider", &comp_collider);
			Checkbox("Audio Listener", &comp_audiolistener);
			SameLine(); Checkbox("Light", &comp_light);
			Checkbox("Audio Source", &comp_audiosource);
			SameLine(); Checkbox("Mesh2D", &comp_2d);
			Checkbox("Player", &comp_player);
			TreePop();
		}
		
		//// component headers ////
		if(comp_mesh && TreeNodeEx("Mesh", tree_flags)){
			Text(TOSTRING("MeshID: ", mesh_id).c_str());
			SetNextItemWidth(-1); if(BeginCombo("##mesh_combo", mesh_name)){ WinHovCheck;
				for_n(i, DengRenderer->meshes.size()){
					if(DengRenderer->meshes[i].base && Selectable(DengRenderer->meshes[i].name, mesh_id == i)){
						mesh_id = i; 
						mesh_name = DengRenderer->meshes[i].name;
					}
				}
				EndCombo();
			}
			TreePop();
		}
		if(comp_2d && TreeNodeEx("Mesh2D", tree_flags)){
			ImU32 color = ColorConvertFloat4ToU32(ImVec4(twod_color.x, twod_color.y, twod_color.z, twod_color.w));
			SetNextItemWidth(-1); if(Combo("##twod_combo", &twod_type, twods, IM_ARRAYSIZE(twods))){ WinHovCheck; 
				twod_vert_count = twod_type + 1;
				twod_verts.resize(twod_vert_count);
				switch(twod_type){
					case(Twod_Line):{
						twod_verts[0] = {-100.f, -100.f}; twod_verts[1] = {100.f, 100.f};
					}break;
					case(Twod_Triangle):{
						twod_verts[0] = {-100.f, 0.f}; twod_verts[1] = {0.f, 100.f}; twod_verts[2] = {100.f, 0.f};
					}break;
					case(Twod_Square):{
						twod_verts[0] = {-100.f, -100.f}; twod_verts[1] = { 100.f, -100.f};
						twod_verts[2] = { 100.f,  100.f}; twod_verts[3] = {-100.f,  100.f};
					}break;
					case(Twod_NGon):{
						
					}break;
					case(Twod_Image):{
						
					}break;
				}
			}
			
			Text("Color "); SameLine(); SetNextItemWidth(-1); ColorEdit4("##twod_color", (float*)&twod_color); Separator();
			ImDrawList* draw_list = ImGui::GetForegroundDrawList();
			switch(twod_type){
				case(Twod_Line):{
					draw_list->AddLine(ImVec2(entity_pos.x, entity_pos.y), ImVec2(entity_pos.x+twod_verts[0].x, entity_pos.y+twod_verts[0].y), color, 3.f);
					draw_list->AddLine(ImVec2(entity_pos.x, entity_pos.y), ImVec2(entity_pos.x+twod_verts[1].x, entity_pos.y+twod_verts[1].y), color, 3.f);
				}break;
				case(Twod_Triangle):{
					draw_list->AddTriangle(ImVec2(entity_pos.x + twod_verts[0].x, entity_pos.y+twod_verts[0].y), ImVec2(entity_pos.x+twod_verts[1].x, entity_pos.y+twod_verts[1].y), ImVec2(entity_pos.x+twod_verts[2].x, entity_pos.y+twod_verts[2].y), color, 3.f);
				}break;
				case(Twod_Square):{
					draw_list->AddTriangle(ImVec2(entity_pos.x + twod_verts[0].x, entity_pos.y+twod_verts[0].y), ImVec2(entity_pos.x+twod_verts[1].x, entity_pos.y+twod_verts[1].y), ImVec2(entity_pos.x+twod_verts[2].x, entity_pos.y+twod_verts[2].y), color, 3.f);
					draw_list->AddTriangle(ImVec2(entity_pos.x + twod_verts[2].x, entity_pos.y+twod_verts[2].y), ImVec2(entity_pos.x+twod_verts[3].x, entity_pos.y+twod_verts[3].y), ImVec2(entity_pos.x+twod_verts[0].x, entity_pos.y+twod_verts[0].y), color, 3.f);
				}break;
				case(Twod_NGon):{
					draw_list->AddNgon(ImVec2(entity_pos.x, entity_pos.y), twod_radius, color, twod_vert_count, 3.f);
					Text("Vertices "); SameLine(); SliderInt("##vert_cnt", &twod_vert_count, 5, 12); Separator();
					Text("Radius   "); SameLine(); SliderFloat("##vert_rad", &twod_radius, .01f, 100.f); Separator();
				}break;
				case(Twod_Image):{
					Text("Not implemented yet");
				}break;
			}
			
			if(twod_vert_count > 1 && twod_vert_count < 5){
				std::string point("Point 0     ");
				SetNextItemWidth(-1); if(ListBoxHeader("##twod_verts", (int)twod_vert_count, 5)){
					for_n(i,twod_vert_count){
						point[6] = 49 + i;
						Text(point.c_str()); SameLine(); InputVector2(point.c_str(), &twod_verts[0] + i);  Separator();
					}
					ListBoxFooter();
				}
			}
			TreePop();
		}
		if(comp_physics && TreeNodeEx("Physics", tree_flags)){
			Text("Velocity     "); SameLine(); InputVector3("##phys_vel",   &physics_velocity);    Separator();
			Text("Accelertaion "); SameLine(); InputVector3("##phys_accel",   &physics_accel);     Separator();
			Text("Rot Velocity "); SameLine(); InputVector3("##phys_rotvel", &physics_rotVel);     Separator();
			Text("Rot Accel    "); SameLine(); InputVector3("##phys_rotaccel", &physics_rotAccel); Separator();
			Text("Elasticity   "); SameLine(); InputFloat("##phys_elastic", &physics_elasticity);  Separator();
			Text("Mass         "); SameLine(); InputFloat("##phys_mass", &physics_mass);           Separator();
			Checkbox("Static Position", &physics_staticPosition);                                Separator();
			Checkbox("Static Rotation", &physics_staticRotation);
			TreePop();
		}
		if(comp_collider && TreeNodeEx("Collider", tree_flags)){ 
			SetNextItemWidth(-1); Combo("##coll_combo", &collider_type, colliders, IM_ARRAYSIZE(colliders));
			switch(collider_type){
				case ColliderType_Box: case ColliderType_AABB:{
					Text("Half Dims    "); SameLine(); InputVector3("##coll_halfdims", &collider_halfdims);
				}break;
				case ColliderType_Sphere:{
					Text("Radius       "); SameLine(); InputFloat("##coll_radius", &collider_radius);
				}break;
			}
			Checkbox("Don't Resolve Collisions", &collider_nocollide);
			Checkbox("Trigger", &collider_trigger);
			if(collider_trigger){
				Text("Command: "); SameLine(); SetNextItemWidth(-1);
				InputText("##collider_cmd", collider_command, 64, 
						  ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
			}
			TreePop();
		}
		if(comp_audiolistener && TreeNodeEx("Audio Listener", tree_flags)){
			//TODO(sushi,Ui) add audio listener create menu
			TreePop();
		}
		if(comp_audiosource && TreeNodeEx("Audio Source", tree_flags)){
			//TODO(sushi,Ui) add audio source create menu
			TreePop();
		}
		if(comp_light && TreeNodeEx("Light", tree_flags)){
			Text("Strength     "); SameLine(); InputFloat("strength", &physics_mass); Separator();
			TreePop();
		}
		EndChild();
	}
	PopStyleVar();
}

inline void BrushesTab(EntityAdmin* admin, float fontsize){
	using namespace ImGui;
	
	local_persist MeshBrushVk* selected_meshbrush = 0;
	
	//// brush list ////
	PushStyleColor(ImGuiCol_ChildBg, ColToVec4(Color(25, 25, 25)));
	SetPadding; 
	if(BeginChild("##meshbrush_tab", ImVec2(GetWindowWidth() * 0.95, 100), false)) { WinHovCheck; 
		if (DengRenderer->meshBrushes.size() == 0) {
			float time = DengTime->totalTime;
			std::string str1 = "Nothing yet...";
			float strlen1 = (fontsize - (fontsize / 2)) * str1.size();
			for (int i = 0; i < str1.size(); i++) {
				SetCursorPos(ImVec2((GetWindowSize().x - strlen1) / 2 + i * (fontsize / 2), (GetWindowSize().y - fontsize) / 2 + sin(10 * time + cos(10 * time + (i * M_PI / 2)) + (i * M_PI / 2))));
				Text(str1.substr(i, 1).c_str());
			}
		}else{
			if (BeginTable("##meshbrush_table", 4, ImGuiTableFlags_BordersInner)) {
				TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
				TableSetupColumn("Name");
				TableSetupColumn("Delete");
				
				for_n(i, DengRenderer->meshBrushes.size()) {
					PushID(i);
					TableNextRow(); TableNextColumn();
					std::string id = std::to_string(DengRenderer->meshBrushes[i].id);
					if (ImGui::Button(id.c_str())) selected_meshbrush = &DengRenderer->meshBrushes[i];
					
					TableNextColumn();
					Text(DengRenderer->meshBrushes[i].name);
					
					TableNextColumn();
					if(SmallButton("X")) DengRenderer->RemoveMeshBrush(i);
					PopID();
				}
				EndTable();
			}
		}
		EndChild();
	}
	PopStyleColor();
	
	Separator();
	
	//// brush inspector ////
	
	
}

void Editor::DebugTools() {
	using namespace ImGui;
	
	//resize tool menu if main menu bar is open
	ImGui::SetNextWindowSize(ImVec2(DengWindow->width / 5, DengWindow->height - (menubarheight + debugbarheight)));
	ImGui::SetNextWindowPos(ImVec2(0, menubarheight));
	
	//window styling
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 5);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,      ImVec2(0, 2));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,     ImVec2(2, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(1, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0);
	
	ImGui::PushStyleColor(ImGuiCol_Border,               ColToVec4(Color( 0,  0,  0)));
	ImGui::PushStyleColor(ImGuiCol_Button,               ColToVec4(Color(40, 40, 40)));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,         ColToVec4(Color(48, 48, 48)));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,        ColToVec4(Color(60, 60, 60)));
	ImGui::PushStyleColor(ImGuiCol_WindowBg,             ColToVec4(colors.c9));
	ImGui::PushStyleColor(ImGuiCol_PopupBg,              ColToVec4(Color(20, 20, 20)));
	ImGui::PushStyleColor(ImGuiCol_FrameBg,              ColToVec4(Color(35, 45, 50)));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive,        ColToVec4(Color(42, 54, 60)));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,       ColToVec4(Color(54, 68, 75)));
	ImGui::PushStyleColor(ImGuiCol_TitleBg,              ColToVec4(Color(0,   0,  0)));
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive,        ColToVec4(Color(0,   0,  0)));
	ImGui::PushStyleColor(ImGuiCol_Header,               ColToVec4(Color(35, 45, 50)));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive,         ColToVec4(Color( 0, 74, 74)));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered,        ColToVec4(Color( 0, 93, 93)));
	ImGui::PushStyleColor(ImGuiCol_TableBorderLight,     ColToVec4(Color(45, 45, 45)));
	ImGui::PushStyleColor(ImGuiCol_TableHeaderBg,        ColToVec4(Color(10, 10, 10)));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,          ColToVec4(Color(10, 10, 10)));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        ColToVec4(Color(55, 55, 55)));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  ColToVec4(Color(75, 75, 75)));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ColToVec4(Color(65, 65, 65)));
	ImGui::PushStyleColor(ImGuiCol_TabActive,            ColToVec4(Color::VERY_DARK_CYAN));
	ImGui::PushStyleColor(ImGuiCol_TabHovered,           ColToVec4(Color::DARK_CYAN));
	ImGui::PushStyleColor(ImGuiCol_Tab,                  ColToVec4(colors.c1));
	ImGui::PushStyleColor(ImGuiCol_Separator,            ColToVec4(Color::VERY_DARK_CYAN));
	
	ImGui::Begin("DebugTools", (bool*)1, ImGuiWindowFlags_NoFocusOnAppearing |  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	
	//capture mouse if hovering over this window
	WinHovCheck; 
	
	SetPadding;
	if (BeginTabBar("MajorTabs")) {
		if (BeginTabItem("Entities")) {
			EntitiesTab(admin, ImGui::GetFontSize());
			EndTabItem();
		}
		if (BeginTabItem("Create")) {
			CreateTab(admin, ImGui::GetFontSize());
			EndTabItem();
		}
		//if (BeginTabItem("Brushes")) {
		//BrushesTab(admin, ImGui::GetFontSize());
		//EndTabItem();
		//}
		EndTabBar();
	}
	
	ImGui::PopStyleVar(8);
	ImGui::PopStyleColor(24);
	ImGui::End();
}


void Editor::DebugBar() {
	using namespace ImGui;
	
	//for getting fps
	ImGuiIO& io = ImGui::GetIO();
	
	int FPS = floor(io.Framerate);
	
	//num of active columns
	int activecols = 7;
	
	//font size for centering text
	float fontsize = ImGui::GetFontSize();
	
	//flags for showing different things
	static bool show_fps = true;
	static bool show_fps_graph = true;
	static bool show_world_stats = true;
	static bool show_selected_stats = true;
	static bool show_floating_fps_graph = false;
	static bool show_time = true;
	
	ImGui::SetNextWindowSize(ImVec2(DengWindow->width, 20));
	ImGui::SetNextWindowPos(ImVec2(0, DengWindow->height - 20));
	
	
	//window styling
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,   ImVec2(0, 2));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(2, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border,           ColToVec4(Color(0, 0, 0, 255)));
	ImGui::PushStyleColor(ImGuiCol_WindowBg,         ColToVec4(Color(20, 20, 20, 255)));
	ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ColToVec4(Color(45, 45, 45, 255)));
	
	ImGui::Begin("DebugBar", (bool*)1, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	debugbarheight = 20;
	//capture mouse if hovering over this window
	WinHovCheck; 
	
	activecols = show_fps + show_fps_graph + 3 * show_world_stats + 2 * show_selected_stats + show_time + 1;
	if (BeginTable("DebugBarTable", activecols, ImGuiTableFlags_BordersV | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_SizingFixedFit)) {
		
		//precalc strings and stuff so we can set column widths appropriately
		std::string str1 = TOSTRING("wents: ", admin->entities.size());
		float strlen1 = (fontsize - (fontsize / 2)) * str1.size();
		std::string str2 = TOSTRING("wtris: ", g_renderer->stats.totalTriangles);
		float strlen2 = (fontsize - (fontsize / 2)) * str2.size();
		std::string str3 = TOSTRING("wverts: ", g_renderer->stats.totalVertices);
		float strlen3 = (fontsize - (fontsize / 2)) * str3.size();
		std::string str4 = TOSTRING("stris: ", "0");
		float strlen4 = (fontsize - (fontsize / 2)) * str4.size();
		std::string str5 = TOSTRING("sverts: ", "0");
		float strlen5 = (fontsize - (fontsize / 2)) * str5.size();
		
		ImGui::TableSetupColumn("FPS",            ImGuiTableColumnFlags_WidthFixed, 64);
		ImGui::TableSetupColumn("FPSGraphInline", ImGuiTableColumnFlags_WidthFixed, 64);
		ImGui::TableSetupColumn("EntCount",       ImGuiTableColumnFlags_None, strlen1 * 1.3);
		ImGui::TableSetupColumn("TriCount",       ImGuiTableColumnFlags_None, strlen2 * 1.3);
		ImGui::TableSetupColumn("VerCount",       ImGuiTableColumnFlags_None, strlen3 * 1.3);
		ImGui::TableSetupColumn("SelTriCount",    ImGuiTableColumnFlags_None, strlen4 * 1.3);
		ImGui::TableSetupColumn("SelVerCount",    ImGuiTableColumnFlags_None, strlen5 * 1.3);
		ImGui::TableSetupColumn("MiddleSep",      ImGuiTableColumnFlags_WidthStretch, 0);
		ImGui::TableSetupColumn("Time",           ImGuiTableColumnFlags_WidthFixed, 64);
		
		
		//FPS
		
		if (TableNextColumn() && show_fps) {
			//trying to keep it from changing width of column
			//actually not necessary anymore but im going to keep it cause 
			//it keeps the numbers right aligned
			if (FPS % 1000 == FPS) {
				Text(TOSTRING("FPS:  ", FPS).c_str());
			}
			else if (FPS % 100 == FPS) {
				Text(TOSTRING("FPS:   ", FPS).c_str());
			}
			else {
				Text(TOSTRING("FPS: ", FPS).c_str());
			}
			
		}
		
		//FPS graph inline
		if (TableNextColumn() && show_fps_graph) {
			//how much data we store
			static int prevstoresize = 100;
			static int storesize = 100;
			
			//how often we update
			static int fupdate = 20;
			static int frame_count = 0;
			
			//maximum FPS
			static int maxval = 0;
			
			//real values and printed values
			static std::vector<float> values(storesize);
			static std::vector<float> pvalues(storesize);
			
			//dynamic resizing that may get removed later if it sucks
			//if FPS finds itself as less than half of what the max used to be we lower the max
			if (FPS > maxval || FPS < maxval / 2) {
				maxval = FPS;
			}
			
			//if changing the amount of data we're storing we have to reverse
			//each data set twice to ensure the data stays in the right place when we move it
			if (prevstoresize != storesize) {
				std::reverse(values.begin(), values.end());    values.resize(storesize);  std::reverse(values.begin(), values.end());
				std::reverse(pvalues.begin(), pvalues.end());  pvalues.resize(storesize); std::reverse(pvalues.begin(), pvalues.end());
				prevstoresize = storesize;
			}
			
			std::rotate(values.begin(), values.begin() + 1, values.end());
			
			//update real set if we're not updating yet or update the graph if we are
			if (frame_count < fupdate) {
				values[values.size() - 1] = FPS;
				frame_count++;
			}
			else {
				float avg = Math::average(values.begin(), values.end(), storesize);
				std::rotate(pvalues.begin(), pvalues.begin() + 1, pvalues.end());
				pvalues[pvalues.size() - 1] = std::floorf(avg);
				
				frame_count = 0;
			}
			
			ImGui::PushStyleColor(ImGuiCol_PlotLines, ColToVec4(Color(0, 255, 200, 255)));
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ColToVec4(Color(20, 20, 20, 255)));
			
			ImGui::PlotLines("", &pvalues[0], pvalues.size(), 0, 0, 0, maxval, ImVec2(64, 20));
			
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
		}
		
		
		//World stats
		
		//Entity Count
		if (TableNextColumn() && show_world_stats) {
			ImGui::SameLine((GetColumnWidth() - strlen1) / 2);
			Text(str1.c_str());
		}
		
		//Triangle Count
		if (TableNextColumn() && show_world_stats) {
			//TODO( sushi,Ui) implement triangle count when its avaliable
			ImGui::SameLine((GetColumnWidth() - strlen2) / 2);
			Text(str2.c_str());
		}
		
		//Vertice Count
		if (TableNextColumn() && show_world_stats) {
			//TODO( sushi,Ui) implement vertice count when its avaliable
			ImGui::SameLine((GetColumnWidth() - strlen3) / 2);
			Text(str3.c_str());
		}
		
		
		
		// Selected Stats
		
		
		
		//Triangle Count
		if (TableNextColumn() && show_selected_stats) {
			//TODO( sushi,Ui) implement triangle count when its avaliable
			//Entity* e = admin->selectedEntity;
			ImGui::SameLine((GetColumnWidth() - strlen4) / 2);
			Text(str4.c_str());
		}
		
		//Vertice Count
		if (TableNextColumn() && show_selected_stats) {
			//TODO( sushi,Ui) implement vertice count when its avaliable
			//Entity* e = admin->selectedEntity;
			ImGui::SameLine((GetColumnWidth() - strlen5) / 2);
			Text(str5.c_str());
		}
		
		//Middle Empty Separator
		if (TableNextColumn()) {
			static float time = DengTime->totalTime;
			if (DengConsole->cons_error_warn) {
				time = DengTime->totalTime;
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, GetColorU32(ColToVec4(Color(255 * (sin(2 * M_PI * time + cos(2 * M_PI * time)) + 1)/2, 0, 0, 255))));
				
				PushItemWidth(-1);
				std::string str6 = DengConsole->last_error;
				float strlen6 = (fontsize - (fontsize / 2)) * str6.size();
				ImGui::SameLine((GetColumnWidth() - strlen6) / 2);
				ImGui::PushStyleColor(ImGuiCol_Text, ColToVec4(Color(255 * -(sin(2 * M_PI * time + cos(2 * M_PI * time)) - 1)/2, 0, 0, 255)));
				Text(str6.c_str());
				PopStyleColor();
			}
			
			
		}
		
		//Show Time
		if (TableNextColumn()) {
			//https://stackoverflow.com/questions/24686846/get-current-time-in-milliseconds-or-hhmmssmmm-format
			using namespace std::chrono;
			
			//get current time
			auto now = system_clock::now();
			
			//convert to std::time_t so we can convert to std::tm
			auto timer = system_clock::to_time_t(now);
			
			//convert to broken time
			std::tm bt = *std::localtime(&timer);
			
			std::ostringstream oss;
			
			oss << std::put_time(&bt, "%H:%M:%S");
			
			std::string str7 = oss.str();
			float strlen7 = (fontsize - (fontsize / 2)) * str7.size();
			ImGui::SameLine(32 - (strlen7 / 2));
			
			Text(str7.c_str());
			
		}
		
		
		//Context menu for toggling parts of the bar
		if (ImGui::IsMouseReleased(1) && IsWindowHovered()) OpenPopup("Context");
		if (BeginPopup("Context")) {
			DengConsole->IMGUI_MOUSE_CAPTURE = true;
			ImGui::Separator();
			if (Button("Open Debug Menu")) {
				//showDebugTools = true;
				CloseCurrentPopup();
			}
			
			EndPopup();
		}
		ImGui::EndTable();
	}
	
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::End();
}

void Editor::DrawTimes(){
	std::string time1 = DengTime->FormatTickTime ("Time       : {t}\n"
												  "Window     : {w}\n"
												  "Input      : {i}\n");
	time1            += DengTime->FormatAdminTime("Physics Lyr: {P}\n"
												  "        Sys: {p}\n"
												  "Canvas  Lyr: {C}\n"
												  "        Sys: {c}\n"
												  "World   Lyr: {W}\n"
												  "        Sys: {w}\n"
												  "Sound   Lyr: {S}\n"
												  "        Sys: {s}\n");
	time1            += DengTime->FormatTickTime ("Admin      : {a}\n"
												  "Console    : {c}\n"
												  "Render     : {r}\n"
												  "Frame      : {f}");
	
	ImGui::SetCursorPos(ImVec2(DengWindow->width - 150, menubarheight));
	ImGui::Text(time1.c_str());
}

//sort of sandbox for drawing ImGui stuff over the entire screen
void Editor::DebugLayer() {
	ImGui::SetNextWindowSize(ImVec2(DengWindow->width, DengWindow->height));
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ColToVec4(Color(0, 0, 0, 0)));
	Camera* c = admin->mainCamera;
	float time = DengTime->totalTime;
	
	static std::vector<std::pair<float, Vector2>> times;
	
	static std::vector<Vector3> spots;
	
	
	ImGui::Begin("DebugLayer", 0, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
	
	Vector2 mp = DengInput->mousePos;
	
	float fontsize = ImGui::GetFontSize();
	
	
	
	
	//psuedo grid
	int lines = 100;
	for (int i = 0; i < lines * 2; i++) {
		Vector3 cpos = c->position;
		Vector3 v1 = Math::WorldToCamera4(Vector3(floor(cpos.x) + -lines + i, 0, floor(cpos.z) + -lines), c->viewMat).ToVector3();
		Vector3 v2 = Math::WorldToCamera4(Vector3(floor(cpos.x) + -lines + i, 0, floor(cpos.z) + lines), c->viewMat).ToVector3();
		Vector3 v3 = Math::WorldToCamera4(Vector3(floor(cpos.x) + -lines, 0, floor(cpos.z) + -lines + i), c->viewMat).ToVector3();
		Vector3 v4 = Math::WorldToCamera4(Vector3(floor(cpos.x) + lines, 0, floor(cpos.z) + -lines + i), c->viewMat).ToVector3();
		
		
		
		//TODO(sushi, CamMa) make grid lines appear properly when in different orthographic views
		//if (c->type == CameraType::ORTHOGRAPHIC) {
		//
		//	if (c->orthoview == FRONT) {
		//		v1 = Math::WorldToCamera4(Vector3(floor(cpos.x) + -lines + i, 0, floor(cpos.y) + -lines), c->viewMat).ToVector3();
		//		v2 = Math::WorldToCamera4(Vector3(floor(cpos.x) + -lines + i, 0, floor(cpos.y) + lines), c->viewMat).ToVector3();
		//		v3 = Math::WorldToCamera4(Vector3(floor(cpos.x) + -lines, 0, floor(cpos.y) + -lines + i), c->viewMat).ToVector3();
		//		v4 = Math::WorldToCamera4(Vector3(floor(cpos.x) + lines, 0, floor(cpos.y) + -lines + i), c->viewMat).ToVector3();
		//
		//	}
		//	
		//
		//
		//}
		
		bool l1flag = false;
		bool l2flag = false;
		
		if (floor(cpos.x) - lines + i == 0) {
			l1flag = true;
		}
		if (floor(cpos.z) - lines + i == 0) {
			l2flag = true;
		}
		//Vector3 v1t = v1.ToVector3();
		//Vector3 v2t = v2.ToVector3();
		//Vector3 v3t = v3.ToVector3();
		//Vector3 v4t = v4.ToVector3();
		
		
		if (Math::ClipLineToZPlanes(v1, v2, c)) {
			Vector2 v1s = Math::CameraToScreen2(v1, c->projMat, DengWindow->dimensions);
			Vector2 v2s = Math::CameraToScreen2(v2, c->projMat, DengWindow->dimensions);
			Math::ClipLineToBorderPlanes(v1s, v2s, DengWindow->dimensions);
			if (!l1flag) ImGui::GetBackgroundDrawList()->AddLine(v1s.ToImVec2(), v2s.ToImVec2(), ImGui::GetColorU32(ImVec4(1, 1, 1, 0.3)));
			else         ImGui::GetBackgroundDrawList()->AddLine(v1s.ToImVec2(), v2s.ToImVec2(), ImGui::GetColorU32(ImVec4(1, 0, 0, 1)));
		}
		if (Math::ClipLineToZPlanes(v3, v4, c)) {
			Vector2 v3s = Math::CameraToScreen2(v3, c->projMat, DengWindow->dimensions);
			Vector2 v4s = Math::CameraToScreen2(v4, c->projMat, DengWindow->dimensions);
			Math::ClipLineToBorderPlanes(v3s, v4s, DengWindow->dimensions);
			if (!l2flag) ImGui::GetBackgroundDrawList()->AddLine(v3s.ToImVec2(), v4s.ToImVec2(), ImGui::GetColorU32(ImVec4(1, 1, 1, 0.3)));
			else         ImGui::GetBackgroundDrawList()->AddLine(v3s.ToImVec2(), v4s.ToImVec2(), ImGui::GetColorU32(ImVec4(0, 0, 1, 1)));
		}
	}
	
	if (DengInput->KeyPressed(MouseButton::LEFT) && rand() % 100 + 1 == 80) {
		times.push_back(std::pair<float, Vector2>(0.f, mp));
	}
	
	int index = 0;
	for (auto& f : times) {
		ImGui::PushStyleColor(ImGuiCol_Text, ColToVec4(Color(255. * fabs(sinf(time)), 255. * fabs(cosf(time)), 255, 255)));
		
		f.first += DengTime->deltaTime;
		
		Vector2 p = f.second;
		
		ImGui::SetCursorPos(ImVec2(p.x + 20 * sin(2 * time), p.y - 200 * (f.first / 5)));
		
		Vector2 curpos = Vector2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY());
		
		std::string str1 = "hehe!!!!";
		float strlen1 = (fontsize - (fontsize / 2)) * str1.size();
		for (int i = 0; i < str1.size(); i++) {
			ImGui::SetCursorPos(ImVec2(
									   curpos.x + i * fontsize / 2,
									   curpos.y + sin(10 * time + cos(10 * time + (i * M_PI / 2)) + (i * M_PI / 2))
									   ));
			ImGui::Text(str1.substr(i, 1).c_str());
		}
		
		if (f.first >= 5) {
			times.erase(times.begin() + index);
			index--;
		}
		
		ImGui::PopStyleColor();
		index++;
	}
	ImGui::Text("test");
	
	
	if (admin->paused) {
		std::string s = "ENGINE PAUSED";
		float strlen = (fontsize - (fontsize / 2)) * s.size();
		//ImGui::SameLine(32 - (strlen / 2));
		ImGui::SetCursorPos(ImVec2(DengWindow->width - strlen * 1.3, menubarheight));
		ImGui::Text(s.c_str());
	}
	
	ImGui::PopStyleColor();
	ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// editor struct

void Editor::Init(EntityAdmin* a){
	admin = a;
	settings = {};
	
	selected.reserve(8);
	camera = new Camera(90.f, .01f, 1000.01f, true);
	camera->admin = a;
	undo_manager.Init();
	
	showDebugTools      = true;
	showTimes           = true;
	showDebugBar        = true;
	showMenuBar         = true;
	showImGuiDemoWindow = false;
	showDebugLayer      = true;
	ConsoleHovFlag      = false;
	files = deshi::iterateDirectory(deshi::dirModels());
	textures = deshi::iterateDirectory(deshi::dirTextures());
	fonth = ImGui::GetFontSize();
	fontw = fonth / 2;
}

void Editor::Update(){
	////////////////////////////
	//// handle user inputs ////
	////////////////////////////
	
	{//// select ////
		if (!DengConsole->IMGUI_MOUSE_CAPTURE && !admin->controller.cameraLocked) {
			if (DengInput->KeyPressed(MouseButton::LEFT)) {
				Entity* e = SelectEntityRaycast();
				if(!DengInput->ShiftDown()) selected.clear(); 
				if(e) selected.push_back(e);
			}
		}
		if (selected.size()) {
			HandleGrabbing(selected[0], camera, admin, &undo_manager);
			HandleRotating(selected[0], camera, admin, &undo_manager);
		}
	}
	{//// render ////
		//reload all shaders
		if (DengInput->KeyPressed(Key::F5)) { DengConsole->ExecCommand("shader_reload", "-1"); }
		
		//fullscreen toggle
		if (DengInput->KeyPressed(Key::F11)) {
			if(DengWindow->displayMode == DisplayMode::WINDOWED || DengWindow->displayMode == DisplayMode::BORDERLESS){
				DengWindow->UpdateDisplayMode(DisplayMode::FULLSCREEN);
			}else{
				DengWindow->UpdateDisplayMode(DisplayMode::WINDOWED);
			}
		}
		
		if (DengInput->KeyPressed(Key::P | INPUTMOD_CTRL)) {
			admin->paused = !admin->paused;
		}
		
	}
	{//// camera ////
		//toggle ortho
		static Vector3 ogpos;
		static Vector3 ogrot;
		if (DengInput->KeyPressed(DengKeys.perspectiveToggle)) {
			switch (camera->type) {
				case(CameraType::PERSPECTIVE): {  
					ogpos = camera->position;
					ogrot = camera->rotation;
					camera->type = CameraType::ORTHOGRAPHIC; 
					camera->farZ = 1000000; 
				} break;
				case(CameraType::ORTHOGRAPHIC): { 
					camera->position = ogpos; 
					camera->rotation = ogrot;
					camera->type = CameraType::PERSPECTIVE; 
					camera->farZ = 1000; 
					camera->UpdateProjectionMatrix(); 
				} break;
			}
		}
		
		//ortho views
		if      (DengInput->KeyPressed(DengKeys.orthoFrontView))    camera->orthoview = FRONT;
		else if (DengInput->KeyPressed(DengKeys.orthoBackView))     camera->orthoview = BACK;
		else if (DengInput->KeyPressed(DengKeys.orthoRightView))    camera->orthoview = RIGHT;
		else if (DengInput->KeyPressed(DengKeys.orthoLeftView))     camera->orthoview = LEFT;
		else if (DengInput->KeyPressed(DengKeys.orthoTopDownView))  camera->orthoview = TOPDOWN;
		else if (DengInput->KeyPressed(DengKeys.orthoBottomUpView)) camera->orthoview = BOTTOMUP;
	}
	{//// undo/redo ////
		if (DengInput->KeyPressed(DengKeys.undo)) undo_manager.Undo();
		if (DengInput->KeyPressed(DengKeys.redo)) undo_manager.Redo();
	}
	{//// interface ////
		if (DengInput->KeyPressed(DengKeys.toggleDebugMenu)) showDebugTools = !showDebugTools;
		if (DengInput->KeyPressed(DengKeys.toggleDebugBar))  showDebugBar = !showDebugBar;
		if (DengInput->KeyPressed(DengKeys.toggleMenuBar))   showMenuBar = !showMenuBar;
	}
	
	///////////////////////////////
	//// render user interface ////
	///////////////////////////////
	
	WinHovFlag = 0;
	//TODO(delle,Cl) program crashes somewhere in DebugTools() if minimized
	if(!DengWindow->minimized){
		if (showDebugLayer) DebugLayer();
		if (showTimes)      DrawTimes();
		if (showDebugTools) DebugTools();
		if (showDebugBar)   DebugBar();
		if (showMenuBar)    MenuBar();
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1));{
			if (showImGuiDemoWindow) ImGui::ShowDemoWindow();
		}ImGui::PopStyleColor();
		
		if (!showMenuBar)    menubarheight = 0;
		if (!showDebugBar)   debugbarheight = 0;
		if (!showDebugTools) debugtoolswidth = 0;
	}
	DengConsole->IMGUI_MOUSE_CAPTURE = (ConsoleHovFlag || WinHovFlag) ? true : false;
	
	/////////////////////////////////////////
	//// render selected entity outlines ////
	/////////////////////////////////////////
	
	//TODO(delle,Cl) possibly clean up  the renderer's selected mesh stuff by having the 
	//renderer take a pointer of u32 that's stored here, or just dont have the renderer know about
	//selected entities since thats a game thing
	DengRenderer->RemoveSelectedMesh(-1);
	for(Entity* e : selected){
		if(MeshComp* mc = e->GetComponent<MeshComp>()){
			if(settings.fast_outline){
				DengRenderer->AddSelectedMesh(mc->meshID);
			}else{
				std::vector<Vector2> outline = mc->mesh->GenerateOutlinePoints(e->transform.TransformMatrix(), camera->projMat, camera->viewMat,
																			   DengWindow->dimensions, camera->position);
				for (int i = 0; i < outline.size(); i += 2) {
					ImGui::DebugDrawLine(outline[i], outline[i + 1], Color::CYAN);
				}
			}
		}
	}
}

void Editor::Reset(){
	selected.clear();
	undo_manager.Reset();
}