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
#include "components/Door.h"
#include "components/AudioSource.h"
#include "components/AudioListener.h"
#include "entities/Entity.h"
#include "entities/PlayerEntity.h"
#include "entities/Trigger.h"
#include "../core/assets.h"
#include "../core/console.h"
#include "../core/imgui.h"
#include "../core/renderer.h"
#include "../core/window.h"
#include "../core/input.h"
#include "../core/time.h"
#include "../core/scene.h"
#include "../math/Math.h"
#include "../geometry/Edge.h"

#include <iomanip> //std::put_time
#include <thread>

local f32 font_width = 0;
local std::string copy_path;
local std::vector<pair<u32,u32>> copy_mesh_diffs;

local void ClearCopiedEntities(){
	Assets::deleteFile(copy_path, false);
	copy_mesh_diffs.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// inputs

Entity* Editor::SelectEntityRaycast(){
    vec3 pos = Math::ScreenToWorld(DengInput->mousePos, camera->projMat, camera->viewMat, DengWindow->dimensions);
    
    int closeindex = -1;
    f32 mint = INFINITY;
    
    vec3 p0, p1, p2, normal, intersect;
    mat4 transform, rotation;
    f32  t;
    int  index = 0;
    bool done = false;
    for(Entity* e : admin->entities) {
        transform = e->transform.TransformMatrix();
        rotation = Matrix4::RotationMatrix(e->transform.rotation);
        if(MeshComp* mc = e->GetComponent<MeshComp>()) {
            if(mc->mesh_visible) {
                Mesh* m = mc->mesh;
                for(Batch& b : m->batchArray){
                    for(u32 i = 0; i < b.indexArray.size(); i += 3){
                        //NOTE sushi: our normal here is now based on whatever the vertices normal is when we load the model
                        //			  so if we end up loading models and combining vertices again, this will break
                        p0 = b.vertexArray[b.indexArray[i + 0]].pos * transform;
                        p1 = b.vertexArray[b.indexArray[i + 1]].pos * transform;
                        p2 = b.vertexArray[b.indexArray[i + 2]].pos * transform;
                        normal = b.vertexArray[b.indexArray[i + 0]].normal * rotation;
                        
                        //early out if triangle is not facing us
                        if (normal.dot(p0 - camera->position) < 0) {
                            //find where on the plane defined by the triangle our raycast intersects
                            intersect = Math::VectorPlaneIntersect(p0, normal, camera->position, pos, t);
                            
                            //early out if intersection is behind us
                            if (t > 0) {
                                //make vectors perpendicular to each edge of the triangle
                                Vector3 perp0 = normal.cross(p1 - p0).yInvert().normalized();
                                Vector3 perp1 = normal.cross(p2 - p1).yInvert().normalized();
                                Vector3 perp2 = normal.cross(p0 - p2).yInvert().normalized();
                                
                                //check that the intersection point is within the triangle and its the closest triangle found so far
                                if (
                                    perp0.dot(intersect - p0) > 0 &&
                                    perp1.dot(intersect - p1) > 0 &&
                                    perp2.dot(intersect - p2) > 0) {
                                    
                                    //if its the closest triangle so far we store its index
                                    if (t < mint) {
                                        closeindex = index;
                                        mint = t;
                                        done = true;
                                        break;
                                    }
                                    
                                }
                            }
                        }
                        if(done) break;
                    }
                    if (done) break;
                }
            }
        }
        done = false;
        index++;
    }
    
    if (closeindex != -1) return admin->entities[closeindex];
    else return 0;
}

void Editor::TranslateEntity(Entity* e, TransformationAxis axis){
    
}

inline void HandleGrabbing(Entity* sel, Camera* c, Admin* admin, UndoManager* um) {
	persist bool grabbingObj = false;
    
    if (!DengConsole->IMGUI_MOUSE_CAPTURE) { 
        if (DengInput->KeyPressed(DengKeys.grabSelectedObject) || grabbingObj) {
            //Camera* c = admin->mainCamera;
            grabbingObj = true;
            admin->controller.cameraLocked = true;
            
            //bools for if we're in an axis movement mode
            persist bool xaxis = false;
            persist bool yaxis = false;
            persist bool zaxis = false;
            
            persist bool initialgrab = true;
            
            persist Vector3 initialObjPos;
            persist float initialdist; 
            persist Vector3 lastFramePos;
            
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
            
            //incase u grab in debug mode
            if (Physics* p = sel->GetComponent<Physics>()) {
                p->velocity = Vector3::ZERO;
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
				Render::TempLine(vec3{-1000,sel->transform.position.y,sel->transform.position.z}, 
								 vec3{ 1000,sel->transform.position.y,sel->transform.position.z}, 
								 Color::DARK_RED);
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
                Render::TempLine(vec3{sel->transform.position.x,-1000,sel->transform.position.z}, 
								 vec3{sel->transform.position.x, 1000,sel->transform.position.z}, 
								 Color::DARK_GREEN);
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
                Render::TempLine(vec3{sel->transform.position.x,sel->transform.position.y,-1000}, 
								 vec3{sel->transform.position.x,sel->transform.position.y, 1000}, 
								 Color::DARK_BLUE);
            }
            lastFramePos = sel->transform.position;
        } //if(DengInput->KeyPressed(DengKeys.grabSelectedObject) || grabbingObj)
    } //if(!DengConsole->IMGUI_MOUSE_CAPTURE)
}

void Editor::RotateEntity(Entity* e, TransformationAxis axis){
    
}

inline void HandleRotating(Entity* sel, Camera* c, Admin* admin, UndoManager* um) {
    persist bool rotatingObj = false;
    
    if (!DengConsole->IMGUI_MOUSE_CAPTURE) { 
        if (DengInput->KeyPressed(DengKeys.rotateSelectedObject) || rotatingObj) {
            rotatingObj = true;
            admin->controller.cameraLocked = true;
            
            //bools for if we're in an axis movement mode
            persist bool xaxis = false;
            persist bool yaxis = false;
            persist bool zaxis = false;
            
            persist Vector2 origmousepos = DengInput->mousePos;
            
            persist bool initialrot = true;
            
            persist Vector3 initialObjRot;
            
            //different cases for mode chaning
            if (DengInput->KeyPressed(Key::X)) {
                xaxis = true; yaxis = false; zaxis = false; 
                sel->transform.rotation = initialObjRot;
                if (Physics* p = sel->GetComponent<Physics>()) {
                    p->rotation = initialObjRot;
                }
            }
            if (DengInput->KeyPressed(Key::Y)) {
                xaxis = false; yaxis = true; zaxis = false; 
                sel->transform.rotation = initialObjRot;
                if (Physics* p = sel->GetComponent<Physics>()) {
                    p->rotation = initialObjRot;
                }
            }
            if (DengInput->KeyPressed(Key::Z)) {
                xaxis = false; yaxis = false; zaxis = true; 
                sel->transform.rotation = initialObjRot;
                if (Physics* p = sel->GetComponent<Physics>()) {
                    p->rotation = initialObjRot;
                }
            }
            if (!(xaxis || yaxis || zaxis) && DengInput->KeyPressed(Key::ESCAPE)) {
                //stop rotating entirely if press esc or right click w no rotate mode on
                xaxis = false; yaxis = false; zaxis = false; 
                sel->transform.rotation = initialObjRot;
                if (Physics* p = sel->GetComponent<Physics>()) {
                    p->rotation = initialObjRot;
                }
                initialrot = true; rotatingObj = false;
                admin->controller.cameraLocked = false;
                return;
            }
            if ((xaxis || yaxis || zaxis) && DengInput->KeyPressed(Key::ESCAPE)) {
                //leave rotation mode if in one when pressing esc
                xaxis = false; yaxis = false; zaxis = false; 
                sel->transform.rotation = initialObjRot; initialrot = true;
                if (Physics* p = sel->GetComponent<Physics>()) {
                    p->rotation = initialObjRot;
                }
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
                if (Physics* p = sel->GetComponent<Physics>()) {
                    p->rotation.z = ang;
                }
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
                if (Physics* p = sel->GetComponent<Physics>()) {
                    p->rotation.y = ang;
                }
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
                if (Physics* p = sel->GetComponent<Physics>()) {
                    p->rotation.x = ang;
                }
            }
        } //if(DengInput->KeyPressed(DengKeys.grabSelectedObject) || grabbingObj)
    } //if(!admin->IMGUI_MOUSE_CAPTURE)
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// imgui debug funcs


//functions to simplify the usage of our DebugLayer
namespace ImGui {
    void BeginDebugLayer() {
        //ImGui::SetNextWindowSize(ImVec2(DengWindow->width, DengWindow->height));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorToImVec4(Color(0, 0, 0, 0)));
        ImGui::Begin("DebugLayer", 0, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
    }
    
    //not necessary, but I'm adding it for clarity in code
    void EndDebugLayer() {
        ImGui::PopStyleColor();
        ImGui::End();
    }
    
    void DebugDrawCircle(Vector2 pos, float radius, Color color) {
        ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(pos.x, pos.y), radius, ImGui::GetColorU32(ImGui::ColorToImVec4(color)));
    }
    
    void DebugDrawCircle3(Vector3 pos, float radius, Color color) {
        Camera* c = g_admin->mainCamera;
        Vector2 windimen = DengWindow->dimensions;
        Vector2 pos2 = Math::WorldToScreen2(pos, c->projMat, c->viewMat, windimen);
        ImGui::GetBackgroundDrawList()->AddCircle(ImGui::Vector2ToImVec2(pos2), radius, ImGui::GetColorU32(ImGui::ColorToImVec4(color)));
    }
    
    void DebugDrawCircleFilled3(Vector3 pos, float radius, Color color) {
        Camera* c = g_admin->mainCamera;
        Vector2 windimen = DengWindow->dimensions;
        Vector2 pos2 = Math::WorldToScreen2(pos, c->projMat, c->viewMat, windimen);
        ImGui::GetBackgroundDrawList()->AddCircleFilled(ImGui::Vector2ToImVec2(pos2), radius, ImGui::GetColorU32(ImGui::ColorToImVec4(color)));
    }
    
    void DebugDrawLine(Vector2 pos1, Vector2 pos2, Color color) {
        Math::ClipLineToBorderPlanes(pos1, pos2, DengWindow->dimensions);
        ImGui::GetBackgroundDrawList()->AddLine(ImGui::Vector2ToImVec2(pos1), ImGui::Vector2ToImVec2(pos2), ImGui::GetColorU32(ImGui::ColorToImVec4(color)));
    }
    
    void DebugDrawLine3(Vector3 pos1, Vector3 pos2, Color color) {
        Camera* c = g_admin->mainCamera;
        Vector2 windimen = DengWindow->dimensions;
        
        Vector3 pos1n = Math::WorldToCamera3(pos1, c->viewMat);
        Vector3 pos2n = Math::WorldToCamera3(pos2, c->viewMat);
        
        if (Math::ClipLineToZPlanes(pos1n, pos2n, c->nearZ, c->farZ)) {
            ImGui::GetBackgroundDrawList()->AddLine(ImGui::Vector2ToImVec2(Math::CameraToScreen2(pos1n, c->projMat, windimen)), 
                                                    ImGui::Vector2ToImVec2(Math::CameraToScreen2(pos2n, c->projMat, windimen)), 
                                                    ImGui::GetColorU32(ImGui::ColorToImVec4(color)));
        }
    }
    
    void DebugDrawText(const char* text, Vector2 pos, Color color) {		
        ImGui::SetCursorPos(ImGui::Vector2ToImVec2(pos));
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorToImVec4(color));
        ImGui::TextEx(text);
        ImGui::PopStyleColor();
    }
    
    void DebugDrawText3(const char* text, Vector3 pos, Color color, Vector2 twoDoffset) {
        Camera* c = g_admin->mainCamera;
        Vector2 windimen = DengWindow->dimensions;
        
        Vector3 posc = Math::WorldToCamera3(pos, c->viewMat);
        if(Math::ClipLineToZPlanes(posc, posc, c->nearZ, c->farZ)){
            ImGui::SetCursorPos(ImGui::Vector2ToImVec2(Math::CameraToScreen2(posc, c->projMat, windimen) + twoDoffset));
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorToImVec4(color));
            ImGui::TextEx(text);
            ImGui::PopStyleColor();
        }
    }
    
    void DebugDrawTriangle(Vector2 p1, Vector2 p2, Vector2 p3, Color color) {
        DebugDrawLine(p1, p2);
        DebugDrawLine(p2, p3);
        DebugDrawLine(p3, p1);
    }
    
    void DebugFillTriangle(Vector2 p1, Vector2 p2, Vector2 p3, Color color) {
        ImGui::GetBackgroundDrawList()->AddTriangleFilled(ImGui::Vector2ToImVec2(p1), ImGui::Vector2ToImVec2(p2), ImGui::Vector2ToImVec2(p3), 
                                                          ImGui::GetColorU32(ImGui::ColorToImVec4(color)));
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
        
        ImGui::GetBackgroundDrawList()->AddTriangleFilled(ImGui::Vector2ToImVec2(p1n), ImGui::Vector2ToImVec2(p2n), ImGui::Vector2ToImVec2(p3n), 
                                                          ImGui::GetColorU32(ImGui::ColorToImVec4(color)));
    }
    
    void DebugDrawGraphFloat(Vector2 pos, float inval, float sizex, float sizey) {
        //display in value
        ImGui::SetCursorPos(ImVec2(pos.x, pos.y - 10));
        ImGui::TextEx(TOSTRING(inval).c_str());
        
        //how much data we store
        persist int prevstoresize = 100;
        persist int storesize = 100;
        
        //how often we update
        persist int fupdate = 1;
        persist int frame_count = 0;
        
        persist float maxval = inval + 5;
        persist float minval = inval - 5;
        
        //if (inval > maxval) maxval = inval;
        //if (inval < minval) minval = inval;
        
        if (inval > maxval || inval < minval) {
            maxval = inval + 5;
            minval = inval - 5;
        }
        //real values and printed values
        persist std::vector<float> values(storesize);
        persist std::vector<float> pvalues(storesize);
        
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
        
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImGui::ColorToImVec4(Color(0, 255, 200, 255)));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorToImVec4(Color(20, 20, 20, 255)));
        
        ImGui::SetCursorPos(ImGui::Vector2ToImVec2(pos));
        ImGui::PlotLines("", &pvalues[0], pvalues.size(), 0, 0, minval, maxval, ImVec2(sizex, sizey));
        
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }
    
    void AddPadding(float x){
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x);
    }
	
} //namespace ImGui



////////////////////////////////////////////////////////////////////////////////////////////////////
//// interface



bool  WinHovFlag = false;
float menubarheight   = 0;
float debugbarheight  = 0;
float debugtoolswidth = 0;
float padding         = 0.95f;
float fontw = 0;
float fonth = 0;

//// defines to make repetitve things less ugly and more editable ////
//check if mouse is over window so we can prevent mouse from being captured by engine
#define WinHovCheck if(ImGui::IsWindowHovered()) WinHovFlag = true 
//allows me to manually set padding so i have a little more control than ImGui gives me (I think idk lol)
#define SetPadding ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() * padding)) / 2)

//current palette:
//https://lospec.com/palette-list/slso8
//TODO(sushi, Ui) implement menu style file loading sort of stuff yeah
//TODO(sushi, Ui) standardize what UI element each color belongs to
struct {
    Color c1 = Color(0x0d2b45); //midnight blue
    Color c2 = Color(0x203c56); //dark gray blue
    Color c3 = Color(0x544e68); //purple gray
    Color c4 = Color(0x8d697a); //pink gray
    Color c5 = Color(0xd08159); //bleached orange
    Color c6 = Color(0xffaa5e); //above but brighter
    Color c7 = Color(0xffd4a3); //skin white
    Color c8 = Color(0xffecd6); //even whiter skin
    Color c9 = Color(0x141414); //almost black
}colors;

std::vector<std::string> files;
std::vector<std::string> textures;
std::vector<std::string> levels;

void Editor::MenuBar() {
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleColor(ImGuiCol_PopupBg,   ImGui::ColorToImVec4(Color(20, 20, 20, 255)));
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImGui::ColorToImVec4(Color(20, 20, 20, 255)));
    
    if(ImGui::BeginMainMenuBar()) { WinHovCheck; 
        menubarheight = ImGui::GetWindowHeight();
        if(ImGui::BeginMenu("File")) { WinHovCheck; 
            if (ImGui::MenuItem("New")) {
                admin->Reset();
            }
            if (ImGui::MenuItem("Save")) {
                if(level_name == ""){
                    ERROR("Level not saved before; Use 'Save As'");
                }else{
                    admin->SaveTEXT(level_name);
                }
            }
            if (ImGui::BeginMenu("Save As")) { WinHovCheck;
                persist char buff[255] = {};
                if(ImGui::InputText("##saveas_input", buff, 255, ImGuiInputTextFlags_EnterReturnsTrue)) {
                    admin->SaveTEXT(buff);
                    level_name = std::string(buff);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Load")) { WinHovCheck;
                forI(levels.size()) {
                    if (ImGui::MenuItem(levels[i].c_str())) {
                        admin->LoadTEXT(levels[i]);
                        level_name = levels[i];
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Spawn")) { WinHovCheck; 
            for (int i = 0; i < files.size(); i++) {
                if (files[i].find(".obj") != std::string::npos) {
                    if(ImGui::MenuItem(files[i].c_str())) { DengConsole->ExecCommand("load_obj", files[i]); }
                }
            }
            ImGui::EndMenu();
        }//agh
        if (ImGui::BeginMenu("Window")) {
            WinHovCheck;
            if (ImGui::MenuItem("Entity Inspector")) {  showDebugTools = !showDebugTools; showEditorWin = false;  }
            if (ImGui::MenuItem("Debug Bar"))           showDebugBar = !showDebugBar;
            if (ImGui::MenuItem("DebugLayer"))          showDebugLayer = !showDebugLayer;
            if (ImGui::MenuItem("Timers"))              showTimes = !showTimes;
            if (ImGui::MenuItem("World Grid"))          showWorldGrid = !showWorldGrid;
            if (ImGui::MenuItem("ImGui Demo"))          showImGuiDemoWindow = !showImGuiDemoWindow;
            if (ImGui::MenuItem("Editor Window"))    {  showEditorWin = !showEditorWin; showDebugTools = false;  }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("State")) { WinHovCheck;
            if (ImGui::MenuItem("Play"))   admin->ChangeState(GameState_Play);
            if (ImGui::MenuItem("Debug"))  admin->ChangeState(GameState_Debug);
            if (ImGui::MenuItem("Editor")) admin->ChangeState(GameState_Editor);
            if (ImGui::MenuItem("Menu"))   admin->ChangeState(GameState_Menu);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}


//NOTE sushi: this is really bad, but i just need it to work for now
//TODO(sushi, ClUi) completely rewrite this menu
inline void EventsMenu(Entity* current) {
    std::vector<Entity*> entities = g_admin->entities;
    persist Entity* other = nullptr;
    persist Component* selcompr = nullptr;
    persist Component* selcompl = nullptr;
    
    if(!current) {
        other = nullptr;
        selcompr = nullptr;
        selcompl = nullptr;
        return;
    }
    
    ImGui::SetNextWindowSize(ImVec2(DengWindow->width / 2, DengWindow->height / 2));
    ImGui::SetNextWindowPos(ImVec2(DengWindow->width / 4, DengWindow->height / 4));
    ImGui::Begin("##EventsMenu", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    ImVec2 winpos = ImGui::GetWindowPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    WinHovCheck;
    float padx = (ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() * padding)) / 2;
    float pady = (ImGui::GetWindowHeight() - (ImGui::GetWindowHeight() * padding)) / 2;
    
    float width = ImGui::GetWindowWidth();
    float height = ImGui::GetWindowHeight();
    
    ImGui::SetCursorPos(ImVec2(padx, pady));
    if (ImGui::BeginChild("ahahaha", ImVec2(width * padding, height * padding))) {
        ImDrawList* drawListc = ImGui::GetWindowDrawList();
        
        float cwidth = ImGui::GetWindowWidth();
        float cheight = ImGui::GetWindowHeight();
        
        winpos = ImGui::GetWindowPos();
        
        ImGui::SetCursorPos(ImVec2(padx, (ImGui::GetWindowHeight() - fonth) / 2));
        ImGui::TextEx(current->name);
        
        float maxlen = 0;
        
        for (Entity* e : entities) maxlen = std::max(maxlen, (float)std::string(e->name).size());
        //TODO(sushi, ClUi) make the right list scrollable once it reaches a certain point
        //or just redesign this entirely
        
        //PRINTLN(maxlen);
        //if we haven't selected an entity display other entities
        if (other == nullptr) {
            float inc = cheight / (entities.size());
            int i = 1;
            for (Entity* e : entities) {
                if (e != current) {
                    ImGui::PushID(i);
                    if (e->connections.find(current) != e->connections.end()) {
                        float lx = 1.2 * (padx + ImGui::CalcTextSize(current->name).x);
                        float ly = cheight / 2;
                        
                        float rx = cwidth - maxlen * fontw * 1.2 - padx * 0.8 + ImGui::CalcTextSize(e->name).x / 2;
                        float ry = i * inc + ImGui::CalcTextSize(e->name).y / 2;
                        
                        drawListc->AddLine(
                                           ImVec2(winpos.x + lx, winpos.y + ly),
                                           ImVec2(winpos.x + rx, winpos.y + ry),
                                           ImGui::GetColorU32(ImGui::ColorToImVec4(Color::WHITE)));
                    }
                    
                    ImGui::SetCursorPos(ImVec2(cwidth - maxlen * fontw * 1.2 - padx, i * inc));
                    if (ImGui::Button(e->name, ImVec2(maxlen * fontw * 1.2, fonth))) {
                        other = e;
                    }
                    i++;
                    ImGui::PopID();
                    
                }
            }
        }
        else {
            
            
            
            float rightoffset = cwidth - (float)std::string(other->name).size() * fontw - padx;
            
            
            //display other entity and it's components
            ImGui::SetCursorPos(ImVec2(rightoffset, (cheight - fonth) / 2 ));
            ImGui::TextEx(other->name);
            
            //if we select a comp for each ent, show options for connecting an event
            if (selcompr && selcompl) {
                int currevent = selcompl->event;
                if (ImGui::Combo("##events_combo2", &currevent, EventStrings, ArrayCount(EventStrings))) {
                    switch (currevent) {
                        case Event_NONE:
                        selcompl->sender.RemoveReceiver(selcompr);
                        selcompl->event = Event_NONE;
                        selcompl->entity->connections.erase(selcompr->entity);
                        selcompr->entity->connections.erase(selcompl->entity);
                        break;
                        default:
                        selcompl->sender.AddReceiver(selcompr);
                        selcompl->event = (u32)currevent;
                        selcompr->entity->connections.insert(selcompl->entity);
                        selcompl->entity->connections.insert(selcompr->entity);
                        break;
                    }
                }
                
                if (selcompl->sender.HasReceiver(selcompr)) {
                    float lx = 1.2 * (padx * 2 + ImGui::CalcTextSize(current->name).x + ImGui::CalcTextSize(ComponentTypeStrings[selcompl->type]).x) / 2;
                    float ly = cheight / 2;
                    
                    float rx = rightoffset * 0.8 + ((float)std::string(ComponentTypeStrings[selcompl->type]).size() * fontw) / 2;
                    float ry = cheight / 2;
                    
                    drawListc->AddLine(
                                       ImVec2(winpos.x + lx, winpos.y + ly),
                                       ImVec2(winpos.x + rx, winpos.y + ry),
                                       ImGui::GetColorU32(ImGui::ColorToImVec4(Color::WHITE)));
                }
                
            }
            
            //TODO(sushi, Op) make this run only when we first select the entity
            float maxlen = 0;
            for (Component* c : other->components) maxlen = std::max(maxlen, (float)std::string(ComponentTypeStrings[c->type]).size());
            int i = 0; //increment for IDs
            if (!selcompr) {
                float inc = cheight / (other->components.size() + 1);
                int o = 1;
                
                for (Component* c : other->components) {
                    ImGui::SetCursorPos(ImVec2(rightoffset * 0.8, o * inc));
                    ImGui::PushID(i);
                    if (selcompl && selcompl->sender.HasReceiver(c)) {
                        float lx = 1.2 * (padx * 2 + ImGui::CalcTextSize(current->name).x + (ImGui::CalcTextSize(ComponentTypeStrings[selcompl->type]).x) / 2);
                        float ly = cheight / 2;
                        
                        float rx = rightoffset * 0.8 + ImGui::CalcTextSize(ComponentTypeStrings[c->type]).y / 2;
                        float ry = o * inc + ImGui::CalcTextSize(ComponentTypeStrings[c->type]).x / 2;
                        
                        drawListc->AddLine(
                                           ImVec2(winpos.x + lx, winpos.y + ly),
                                           ImVec2(winpos.x + rx, winpos.y + ry),
                                           ImGui::GetColorU32(ImGui::ColorToImVec4(Color::WHITE)));
                    }
                    if (ImGui::Button(ComponentTypeStrings[c->type], ImVec2(maxlen * fontw * 1.2, fonth))) {
                        selcompr = c;
                    }
                    i++; o++;
                    ImGui::PopID();
                }
            }
            else {
                ImGui::SetCursorPos(ImVec2(rightoffset * 0.8, (cheight - fonth) / 2));
                ImGui::PushID(i);
                if (ImGui::Button(ComponentTypeStrings[selcompr->type])) {
                    selcompr = nullptr;
                }
                i++;
                ImGui::PopID();
            }
            
            maxlen = 0;
            for (Component* c : current->components) maxlen = std::max(maxlen, (float)std::string(ComponentTypeStrings[c->type]).size());
            
            //display initial entities components
            if (!selcompl) {
                float inc = cheight / (current->components.size() + 1);
                int o = 1;
                
                for (Component* c : current->components) {
                    ImGui::SetCursorPos(ImVec2(1.2 * (padx * 2 + (float)std::string(current->name).size() * fontw), o * inc));
                    ImGui::PushID(i);
                    if (selcompr && selcompr->sender.HasReceiver(c)) {
                        float lx = 1.2 * (padx * 2 + ImGui::CalcTextSize(current->name).x + ImGui::CalcTextSize(ComponentTypeStrings[selcompl->type]).x / 2);
                        float ly = o * inc + ImGui::CalcTextSize(ComponentTypeStrings[c->type]).x / 2;
                        
                        float rx = rightoffset * 0.8 + ImGui::CalcTextSize(ComponentTypeStrings[c->type]).y / 2;
                        float ry = cheight / 2;
                        
                        drawListc->AddLine(
                                           ImVec2(winpos.x + lx, winpos.y + ly),
                                           ImVec2(winpos.x + rx, winpos.y + ry),
                                           ImGui::GetColorU32(ImGui::ColorToImVec4(Color::WHITE)));
                    }
                    if (ImGui::Button(ComponentTypeStrings[c->type], ImVec2(maxlen * fontw * 1.2, fonth))) {
                        selcompl = c;
                    }
                    i++; o++;
                    ImGui::PopID();
                }
            }
            else {
                ImGui::SetCursorPos(ImVec2(1.2 * (padx * 2 + (float)std::string(current->name).size() * fontw), cheight / 2 - fonth / 2));
                ImGui::PushID(i);
                if (ImGui::Button(ComponentTypeStrings[selcompl->type])) {
                    selcompl = nullptr;
                }
                i++;
                ImGui::PopID();
            }
            
            ImGui::SetCursorPos(ImVec2(0, 0));
            if (ImGui::Button("Back")) { 
                other = nullptr; 
                selcompl = nullptr;
                selcompr = nullptr;
            }
        }
        ImGui::EndChild();
    }
    else {
        const char* sorry = "No other entities...";
        ImGui::SetCursorPos(ImVec2(width - sizeof("No other entities...") * fontw, height / 2));
        ImGui::TextEx(sorry);
    }
    
    
    ImGui::End();
}

inline void EntitiesTab(Admin* admin, float fontsize){
    persist bool rename_ent = false;
    persist char rename_buffer[DESHI_NAME_SIZE] = {};
    persist Entity* events_ent = 0;
    
    std::vector<Entity*>& selected = admin->editor.selected;
    
    //// selected entity keybinds ////
    //start renaming first selected entity
    if(selected.size() && DengInput->KeyPressedAnyMod(Key::F2)){
        rename_ent = true;
        DengConsole->IMGUI_KEY_CAPTURE = true;
        if(selected.size() > 1) selected.erase(selected.begin()+1, selected.end());
        cpystr(rename_buffer, selected[0]->name, DESHI_NAME_SIZE);
    }
    //submit renaming entity
    if(rename_ent && DengInput->KeyPressedAnyMod(Key::ENTER)){
        rename_ent = false;
        DengConsole->IMGUI_KEY_CAPTURE = false;
        cpystr(selected[0]->name, rename_buffer, DESHI_NAME_SIZE);
    }
    //stop renaming entity
    if(rename_ent && DengInput->KeyPressedAnyMod(Key::ESCAPE)){
        rename_ent = false;
        DengConsole->IMGUI_KEY_CAPTURE = false;
    }
    //delete selected entities
    if(selected.size() && DengInput->KeyPressedAnyMod(Key::DELETE)){
        //TODO(Ui) re-enable this with a popup to delete OR with undoing on delete
		selected.clear();
    }
    
    //// entity list panel ////
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorToImVec4(Color(25, 25, 25)));
    ImGui::SetCursorPosX(ImGui::GetWindowWidth()*0.025);
    if(ImGui::BeginChild("##entity_list", ImVec2(ImGui::GetWindowWidth() * 0.95f, 100))) {
        //if no entities, draw empty list
        if (admin->entities.size() == 0) { 
            float time = DengTime->totalTime;
            std::string str1 = "Nothing yet...";
            float strlen1 = (fontsize - (fontsize / 2)) * str1.size();
            for (int i = 0; i < str1.size(); i++) {
                ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - strlen1) / 2 + i * (fontsize / 2), (ImGui::GetWindowSize().y - fontsize) / 2 + sin(10 * time + cos(10 * time + (i * M_PI / 2)) + (i * M_PI / 2))));
                ImGui::TextEx(str1.substr(i, 1).c_str());
            }
        }else{
            if (ImGui::BeginTable("##entity_list_table", 5, ImGuiTableFlags_BordersInner)) {
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, font_width * 2.f);  //visible ImGui::Button
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, font_width * 3.f);  //id
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);                  //name
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, font_width * 3.5f); //events button
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, font_width);        //delete button
				
                forX(ent_idx, admin->entities.size()){
                    Entity* ent = admin->entities[ent_idx];
                    if(!ent) assert(!"NULL entity when creating entity list table");
                    ImGui::PushID(ent->id);
                    ImGui::TableNextRow(); 
					
                    //// visible button ////
                    //TODO(sushi,UiEnt) implement visibility for things other than meshes like lights, etc.
                    ImGui::TableSetColumnIndex(0);
                    if(MeshComp* m = ent->GetComponent<MeshComp>()){
                        if(ImGui::Button((m->mesh_visible) ? "(M)" : "( )", ImVec2(-FLT_MIN, 0.0f))){
                            m->ToggleVisibility();
                        }
                    }else if(Light* l = ent->GetComponent<Light>()){
                        if(ImGui::Button((l->active) ? "(L)" : "( )", ImVec2(-FLT_MIN, 0.0f))){
                            l->active = !l->active;
                        }
                    }else{
                        if(ImGui::Button("(?)", ImVec2(-FLT_MIN, 0.0f))){}
                    }
					
                    //// id + label (selectable row) ////
                    ImGui::TableSetColumnIndex(1);
                    char label[8];
                    sprintf(label, " %04d ", ent->id);
                    u32 selected_idx = -1;
                    forI(selected.size()){ if(ent == selected[i]){ selected_idx = i; break; } }
                    bool is_selected = selected_idx != -1;
                    if(ImGui::Selectable(label, is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)){
                        if(is_selected){
                            if(DengInput->LCtrlDown()){
                                selected.erase(selected.begin()+selected_idx);
                            }else{
                                selected.clear();
                                selected.push_back(ent);
                            }
                        }else{
                            if(DengInput->LCtrlDown()){
                                selected.push_back(ent);
                            }else{
                                selected.clear();
                                selected.push_back(ent);
                            }
                        }
                        rename_ent = false;
                        DengConsole->IMGUI_KEY_CAPTURE = false;
                    }
					
                    //// name text ////
                    ImGui::TableSetColumnIndex(2);
                    if(rename_ent && selected_idx == ent_idx){
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorToImVec4(colors.c2));
                        ImGui::InputText("##ent_rename_input", rename_buffer, DESHI_NAME_SIZE, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
                        ImGui::PopStyleColor();
                    }else{
                        ImGui::TextEx(ent->name);
                    }
					
                    //// events button ////
                    ImGui::TableSetColumnIndex(3);
                    if (ImGui::Button("Events", ImVec2(-FLT_MIN, 0.0f))) {
                        events_ent = (events_ent != ent) ? ent : 0;
                    }
                    EventsMenu(events_ent);
					
                    //// delete button ////
                    ImGui::TableSetColumnIndex(4);
                    if(ImGui::Button("X", ImVec2(-FLT_MIN, 0.0f))){
                        if(is_selected) selected.erase(selected.begin()+selected_idx);
                        admin->DeleteEntity(ent);
                    }
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
        ImGui::EndChild();
    }//Entity List Scroll child window
    ImGui::PopStyleColor();
    
    ImGui::Separator();
    
    //// create new entity ////
    persist const char* presets[] = {"Empty", "StaticMesh"};
    persist int current_preset = 0;
	
    ImGui::SetCursorPosX(ImGui::GetWindowWidth()*0.025);
    if(ImGui::Button("New Entity")){
        Entity* ent = 0;
        std::string ent_name = TOSTRING(presets[current_preset], admin->entities.size());
        switch(current_preset){
            case(0):default:{ //Empty
                ent = admin->CreateEntityNow({}, ent_name.c_str());
            }break;
            case(1):{ //Static Mesh
                u32 mesh_id = Render::CreateMesh(&admin->scene, "box.obj");
                MeshComp* mc = new MeshComp(mesh_id);
                Physics* phys = new Physics();
                phys->staticPosition = true;
                Collider* coll = new AABBCollider(vec3{.5f, .5f, .5f}, phys->mass);
				
                ent = admin->CreateEntityNow({mc, phys, coll}, ent_name.c_str());
            }break;
        }
		
        selected.clear();
        if(ent) selected.push_back(ent);
    }
    ImGui::SameLine(); ImGui::Combo("##preset_combo", &current_preset, presets, ArrayCount(presets));
    
    ImGui::Separator();
    
    //// selected entity inspector panel ////
    Entity* sel = admin->editor.selected.size() ? admin->editor.selected[0] : 0;
    if(!sel) return;
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 5.0f);
    ImGui::SetCursorPosX(ImGui::GetWindowWidth()*0.025);
    if(ImGui::BeginChild("##ent_inspector", ImVec2(ImGui::GetWindowWidth() * 0.95f, ImGui::GetWindowHeight() * .9f), true, ImGuiWindowFlags_NoScrollbar)) {
        
        //// name ////
        SetPadding; ImGui::TextEx(TOSTRING(sel->id, ":").c_str()); 
        ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); ImGui::InputText("##ent_name_input", sel->name, DESHI_NAME_SIZE, 
																			   ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        
        //// transform ////
        int tree_flags = ImGuiTreeNodeFlags_DefaultOpen;
        if(ImGui::CollapsingHeader("Transform", 0, tree_flags)){
            ImGui::Indent();
            vec3 oldVec = sel->transform.position;
            
            ImGui::TextEx("Position    "); ImGui::SameLine();
            if(ImGui::InputVector3("##ent_pos", &sel->transform.position)){
                if(Physics* p = sel->GetComponent<Physics>()){
                    p->position = sel->transform.position;
                    admin->editor.undo_manager.AddUndoTranslate(&sel->transform, &oldVec, &p->position);
                }else{
                    admin->editor.undo_manager.AddUndoTranslate(&sel->transform, &oldVec, &sel->transform.position);
                }
            }ImGui::Separator();
            
            oldVec = sel->transform.rotation;
            ImGui::TextEx("Rotation    "); ImGui::SameLine(); 
            if(ImGui::InputVector3("##ent_rot", &sel->transform.rotation)){
                if(Physics* p = sel->GetComponent<Physics>()){
                    p->rotation = sel->transform.rotation;
                    admin->editor.undo_manager.AddUndoRotate(&sel->transform, &oldVec, &p->rotation);
                }else{
                    admin->editor.undo_manager.AddUndoRotate(&sel->transform, &oldVec, &sel->transform.rotation);
                }
            }ImGui::Separator();
            
            oldVec = sel->transform.scale;
            ImGui::TextEx("Scale       "); ImGui::SameLine(); 
            if(ImGui::InputVector3("##ent_scale",   &sel->transform.scale)){
                if(Physics* p = sel->GetComponent<Physics>()){
                    p->scale = sel->transform.scale;
                    admin->editor.undo_manager.AddUndoScale(&sel->transform, &oldVec, &p->scale);
                }else{
                    admin->editor.undo_manager.AddUndoScale(&sel->transform, &oldVec, &sel->transform.scale);
                }
            }ImGui::Separator();
            ImGui::Unindent();
        }
        
        //// components ////
        std::vector<Component*> comp_deleted_queue;
        forX(comp_idx, sel->components.size()){
            Component* c = sel->components[comp_idx];
            bool delete_button = true;
            ImGui::PushID(c);
			
            switch(c->type){
				//mesh
				case ComponentType_MeshComp:{
					if(ImGui::CollapsingHeader("Mesh", &delete_button, tree_flags)){
						ImGui::Indent();
						
						MeshComp* mc = dyncast(MeshComp, c);
                        
						ImGui::TextEx("Visible  "); ImGui::SameLine();
						if(ImGui::Button((Render::IsMeshVisible(mc->meshID)) ? "True" : "False", ImVec2(-FLT_MIN, 0))){
							mc->ToggleVisibility();
						}
                        
						ImGui::TextEx("Mesh     "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1); 
						if(ImGui::BeginCombo("##mesh_combo", Render::MeshName(mc->meshID))){ WinHovCheck;
							forI(Render::MeshCount()){
								if(Render::IsBaseMesh(i) && ImGui::Selectable(Render::MeshName(i), mc->meshID == i)){
									mc->ChangeMesh(i);
								}
							}
							ImGui::EndCombo();
						}
                        
						u32 mesh_batch_idx = 0;
						ImGui::TextEx("Batch    "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1); 
						if(ImGui::BeginCombo("##mesh_batch_combo", mc->mesh->batchArray[mesh_batch_idx].name)){ WinHovCheck;
							forI(mc->mesh->batchArray.size()){
								if(ImGui::Selectable(mc->mesh->batchArray[i].name, mesh_batch_idx == i)){
									mesh_batch_idx = i; 
								}
							}
							ImGui::EndCombo();
						}
                        
						ImGui::TextEx("Material "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1); 
						if(ImGui::BeginCombo("##mesh_mat_combo", Render::MaterialName(Render::MeshBatchMaterial(mc->meshID, mesh_batch_idx)))){ WinHovCheck;
							forI(Render::MaterialCount()){
								if(ImGui::Selectable(Render::MaterialName(i), Render::MeshBatchMaterial(mc->meshID, mesh_batch_idx) == i)){
									Render::UpdateMeshBatchMaterial(mc->meshID, mesh_batch_idx, i);
								}
							}
							ImGui::EndCombo();
						}
						
						ImGui::Unindent();
						ImGui::Separator();
					}
				}break;
                
				//physics
				case ComponentType_Physics:
                if(ImGui::CollapsingHeader("Physics", &delete_button, tree_flags)){
                    ImGui::Indent();
					
                    Physics* d = dyncast(Physics, c);
                    ImGui::TextEx("Velocity     "); ImGui::SameLine(); ImGui::InputVector3("##phys_vel", &d->velocity);
                    ImGui::TextEx("Accelertaion "); ImGui::SameLine(); ImGui::InputVector3("##phys_accel", &d->acceleration);
                    ImGui::TextEx("Rot Velocity "); ImGui::SameLine(); ImGui::InputVector3("##phys_rotvel", &d->rotVelocity);
                    ImGui::TextEx("Rot Accel    "); ImGui::SameLine(); ImGui::InputVector3("##phys_rotaccel", &d->rotAcceleration);
                    ImGui::TextEx("Elasticity   "); ImGui::SameLine();
                    ImGui::SetNextItemWidth(-FLT_MIN); ImGui::InputFloat("##phys_elastic", &d->elasticity);
                    ImGui::TextEx("Mass         "); ImGui::SameLine();
                    ImGui::SetNextItemWidth(-FLT_MIN); ImGui::InputFloat("##phys_mass", &d->mass);
                    ImGui::TextEx("Kinetic Fric "); ImGui::SameLine();
                    ImGui::SetNextItemWidth(-FLT_MIN); ImGui::InputFloat("##phys_kinfric", &d->kineticFricCoef);
                    ImGui::Checkbox("Static Position", &d->staticPosition);
                    ImGui::Checkbox("Static Rotation", &d->staticRotation);
                    ImGui::Checkbox("2D Physics", &d->twoDphys);
					
                    ImGui::Unindent();
                    ImGui::Separator();
                }
				break;
                
				//colliders
				case ComponentType_Collider:{
					if(ImGui::CollapsingHeader("Collider", &delete_button, tree_flags)){
						ImGui::Indent();
						
						Collider* coll = dyncast(Collider, c);
						f32 mass = 1.0f;
						
						ImGui::TextEx("Shape "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
						if(ImGui::BeginCombo("##coll_type_combo", ColliderShapeStrings[coll->shape])){
							forI(ArrayCount(ColliderShapeStrings)){
								if(ImGui::Selectable(ColliderShapeStrings[i], coll->shape == i) && (coll->shape != i)){
									if(Physics* p = sel->GetComponent<Physics>()) mass = p->mass;
									
									sel->RemoveComponent(coll);
									coll = 0;
									switch(i){
										case ColliderShape_AABB:{
											coll = new AABBCollider(vec3{0.5f, 0.5f, 0.5f}, mass);
										}break;
										case ColliderShape_Box:{
											coll = new BoxCollider(vec3{0.5f, 0.5f, 0.5f}, mass);
										}break;
										case ColliderShape_Sphere:{
											coll = new SphereCollider(1.0f, mass);
										}break;
										case ColliderShape_Landscape:{
											//coll = new LandscapeCollider();
											WARNING_LOC("Landscape collider not setup yet");
										}break;
										case ColliderShape_Complex:{
											//coll = new ComplexCollider();
											WARNING_LOC("Complex collider not setup yet");
										}break;
									}
									
									if(coll){
										sel->AddComponent(coll);
										admin->AddComponentToLayers(coll);
									}
								}
							}
							ImGui::EndCombo();
						}
                        
						switch(coll->shape){
							case ColliderShape_Box:{
								BoxCollider* coll_box = dyncast(BoxCollider, coll);
								ImGui::TextEx("Half Dims "); ImGui::SameLine(); 
								if(ImGui::InputVector3("##coll_halfdims", &coll_box->halfDims)){
									if(Physics* p = sel->GetComponent<Physics>()) mass = p->mass;
									coll_box->RecalculateTensor(mass);
								}
							}break;
							case ColliderShape_AABB:{
								AABBCollider* coll_aabb = dyncast(AABBCollider, coll);
								ImGui::TextEx("Half Dims "); ImGui::SameLine(); 
								if(ImGui::InputVector3("##coll_halfdims", &coll_aabb->halfDims)){
									if(Physics* p = sel->GetComponent<Physics>()) mass = p->mass;
									coll_aabb->RecalculateTensor(mass);
								}
							}break;
							case ColliderShape_Sphere:{
								SphereCollider* coll_sphere = dyncast(SphereCollider, coll);
								ImGui::TextEx("Radius    "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
								if(ImGui::InputFloat("##coll_sphere", &coll_sphere->radius)){
									if(Physics* p = sel->GetComponent<Physics>()) mass = p->mass;
									coll_sphere->RecalculateTensor(mass);
								}
							}break;
							case ColliderShape_Landscape:{
								ImGui::TextEx("Landscape collider has no settings yet");
							}break;
							case ColliderShape_Complex:{
								ImGui::TextEx("Complex collider has no settings yet");
							}break;
						}
						
						ImGui::Checkbox("Don't Resolve Collisions", (bool*)&coll->noCollide);
						ImGui::TextEx("Collision Layer"); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
						local u32 min = 0, max = 9;
						ImGui::SliderScalar("##coll_layer", ImGuiDataType_U32, &coll->layer, &min, &max, "%d");
						
						ImGui::Unindent();
						ImGui::Separator();
					}
				}break;
                
				//audio listener
				case ComponentType_AudioListener:{
					if(ImGui::CollapsingHeader("Audio Listener", &delete_button, tree_flags)){
						ImGui::Indent();
						
						ImGui::TextEx("TODO implement audio listener component editing");
						
						ImGui::Unindent();
						ImGui::Separator();
					}
				}break;
                
				//audio source
				case ComponentType_AudioSource:{
					if(ImGui::CollapsingHeader("Audio Source", &delete_button, tree_flags)){
						ImGui::Indent();
						
						ImGui::TextEx("TODO implement audio source component editing");
						
						ImGui::Unindent();
						ImGui::Separator();
					}
				}break;
                
				//camera
				case ComponentType_Camera:{
					if(ImGui::CollapsingHeader("Camera", &delete_button, tree_flags)){
						ImGui::Indent();
						
						ImGui::TextEx("TODO implement camera component editing");
						
						ImGui::Unindent();
						ImGui::Separator();
					}
				}break;
                
				//light
				case ComponentType_Light:{
					if(ImGui::CollapsingHeader("Light", &delete_button, tree_flags)){
						ImGui::Indent();
						
						Light* d = dyncast(Light, c);
						ImGui::TextEx("Brightness   "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
						ImGui::InputFloat("##light_brightness", &d->brightness);
						ImGui::TextEx("Position     "); ImGui::SameLine(); 
						ImGui::InputVector3("##light_position", &d->position);
						ImGui::TextEx("Direction    "); ImGui::SameLine(); 
						ImGui::InputVector3("##light_direction", &d->direction);
						
						ImGui::Unindent();
						ImGui::Separator();
					}
				}break;
                
				//orb manager
				case ComponentType_OrbManager:{
					if(ImGui::CollapsingHeader("Orbs", &delete_button, tree_flags)){
						ImGui::Indent();
						
						OrbManager* d = dyncast(OrbManager, c);
						ImGui::TextEx("Orb Count "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); 
						ImGui::InputInt("##orb_orbcount", &d->orbcount);
						
						ImGui::Unindent();
						ImGui::Separator();
					}
				}break;
                
				//door
				case ComponentType_Door:{
					if(ImGui::CollapsingHeader("Door", &delete_button, tree_flags)){
						ImGui::Indent();
						
						ImGui::TextEx("TODO implement door component editing");
						
						ImGui::Unindent();
						ImGui::Separator();
					}
				}break;
                
				//player
				case ComponentType_Player:{
					if(ImGui::CollapsingHeader("Player", &delete_button, tree_flags)){
						ImGui::Indent();
						
						Player* d = dyncast(Player, c);
						ImGui::TextEx("Health "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); 
						ImGui::InputInt("##player_health", &d->health);
						
						ImGui::Unindent();
						ImGui::Separator();
					}
				}break;
                
				//movement
				case ComponentType_Movement:{
					if(ImGui::CollapsingHeader("Movement", &delete_button, tree_flags)){
						ImGui::Indent();
						
						Movement* d = dyncast(Movement, c);
						ImGui::TextEx("Ground Accel    "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
						ImGui::InputFloat("##move_gndaccel", &d->gndAccel);
						ImGui::TextEx("Air Accel       "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
						ImGui::InputFloat("##move_airaccel", &d->airAccel);
						ImGui::TextEx("Jump Impulse    "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
						ImGui::InputFloat("##move_jimp", &d->jumpImpulse);
						ImGui::TextEx("Max Walk Speed  "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
						ImGui::InputFloat("##move_maxwalk", &d->maxWalkingSpeed);
						ImGui::TextEx("Max Run Speed   "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
						ImGui::InputFloat("##move_maxrun", &d->maxRunningSpeed);
						ImGui::TextEx("Max Crouch Speed"); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
						ImGui::InputFloat("##move_maxcrouch", &d->maxCrouchingSpeed);
						
						ImGui::Unindent();
						ImGui::Separator();
					}
				}break;
            }
			
            if(!delete_button) comp_deleted_queue.push_back(c);
            ImGui::PopID();
        } //for(Component* c : sel->components)
        sel->RemoveComponents(comp_deleted_queue);
        
        //// add component ////
        persist int add_comp_index = 0;
		
        ImGui::SetCursorPosX(ImGui::GetWindowWidth()*0.025);
        if(ImGui::Button("Add Component")){
            switch(1 << (add_comp_index-1)){
                case ComponentType_MeshComp:{
                    Component* comp = new MeshComp(Render::CreateMesh(&admin->scene, "box.obj"));
                    sel->AddComponent(comp);
                    admin->AddComponentToLayers(comp);
                }break;
                case ComponentType_Physics:{
                    Component* comp = new Physics();
                    sel->AddComponent(comp);
                    admin->AddComponentToLayers(comp);
                }break;
                case ComponentType_Collider:{
                    Component* comp = new AABBCollider(vec3{.5f, .5f, .5f}, 1.f);
                    sel->AddComponent(comp);
                    admin->AddComponentToLayers(comp);
                }break;
                case ComponentType_AudioListener:{
                    Component* comp = new AudioListener(sel->transform.position, Vector3::ZERO, sel->transform.rotation);
                    sel->AddComponent(comp);
                    admin->AddComponentToLayers(comp);
                }break;
                case ComponentType_AudioSource:{ //@Incomplete
                    Component* comp = new AudioSource();
                    sel->AddComponent(comp);
                    admin->AddComponentToLayers(comp);
                }break;
                case ComponentType_Camera:{
                    Component* comp = new Camera(90.f);
                    sel->AddComponent(comp);
                    admin->AddComponentToLayers(comp);
                }break;
                case ComponentType_Light:{
                    Component* comp = new Light(sel->transform.position, sel->transform.rotation);
                    sel->AddComponent(comp);
                    admin->AddComponentToLayers(comp);
                }break;
                case ComponentType_OrbManager:{ //@Incomplete
                    Component* comp = new OrbManager(0);
                    sel->AddComponent(comp);
                    admin->AddComponentToLayers(comp);
                }break;
                case ComponentType_Door:{
                    Component* comp = new Door();
                    sel->AddComponent(comp);
                    admin->AddComponentToLayers(comp);
                }break;
                case ComponentType_Player:{ //@Incomplete
                    Component* comp = new Player();
                    sel->AddComponent(comp);
                    admin->AddComponentToLayers(comp);
                }break;
                case ComponentType_Movement:{ //@Incomplete
                    Component* comp = new Movement();
                    sel->AddComponent(comp);
                    admin->AddComponentToLayers(comp);
                }break;
                case(0):default:{ //None
                    //do nothing
                }break;
            }
        }
        ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##add_comp_combo", &add_comp_index, ComponentTypeStrings, ArrayCount(ComponentTypeStrings));
        
        ImGui::EndChild(); //CreateMenu
    }
    ImGui::PopStyleVar(); //ImGuiStyleVar_IndentSpacing
} //EntitiesTab

inline void MaterialsTab(Admin* admin){
    persist u32 selected_mat = -1;
    persist bool rename_mat = false;
    persist char rename_buffer[DESHI_NAME_SIZE] = {};
    
    //// selected material keybinds ////
    //start renaming material
    if(selected_mat != -1 && DengInput->KeyPressedAnyMod(Key::F2)){
        rename_mat = true;
        DengConsole->IMGUI_KEY_CAPTURE = true;
        cpystr(rename_buffer, Render::MaterialName(selected_mat), DESHI_NAME_SIZE);
    }
    //submit renaming material
    if(rename_mat && DengInput->KeyPressedAnyMod(Key::ENTER)){
        rename_mat = false;
        DengConsole->IMGUI_KEY_CAPTURE = false;
        cpystr(Render::MaterialName(selected_mat), rename_buffer, DESHI_NAME_SIZE);
    }
    //stop renaming material
    if(rename_mat && DengInput->KeyPressedAnyMod(Key::ESCAPE)){
        rename_mat = false;
        DengConsole->IMGUI_KEY_CAPTURE = false;
    }
    //delete material
    if(selected_mat != -1 && DengInput->KeyPressedAnyMod(Key::DELETE)){
        //TODO(Ui) re-enable this with a popup to delete OR with undoing on delete
        //Render::RemoveMaterial(selected_mat);
        //selected_mat = -1;
    }
    
    //// material list panel ////
    SetPadding; 
    if(ImGui::BeginChild("##mat_list", ImVec2(ImGui::GetWindowWidth()*0.95, ImGui::GetWindowHeight()*.14f), false)) {
        if(ImGui::BeginTable("##mat_table", 3, ImGuiTableFlags_BordersInner)) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, font_width * 2.5f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, font_width);
            
            forX(mat_idx, Render::MaterialCount()) {
                ImGui::PushID(mat_idx);
                ImGui::TableNextRow();
                
                //// id + label ////
                ImGui::TableSetColumnIndex(0);
                char label[8];
                sprintf(label, " %03d", mat_idx);
                if(ImGui::Selectable(label, selected_mat == mat_idx, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)){
                    selected_mat = (ImGui::GetIO().KeyCtrl) ? -1 : mat_idx; //deselect if CTRL held
                    rename_mat = false;
                    DengConsole->IMGUI_KEY_CAPTURE = false;
                }
                
                //// name ImGui::TextEx ////
                ImGui::TableSetColumnIndex(1);
                if(rename_mat && selected_mat == mat_idx){
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorToImVec4(colors.c2));
                    ImGui::InputText("##mat_rename_input", rename_buffer, DESHI_NAME_SIZE, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
                    ImGui::PopStyleColor();
                }else{
                    ImGui::TextEx(Render::MaterialName(mat_idx));
                }
                
                //// delete ImGui::Button ////
                ImGui::TableSetColumnIndex(2);
                if(ImGui::Button("X", ImVec2(-FLT_MIN, 0.0f))){
                    if(mat_idx == selected_mat) {
                        selected_mat = -1;
                    }else if(selected_mat != -1 && selected_mat > mat_idx) {
                        selected_mat -= 1;
                    }
                    Render::RemoveMaterial(mat_idx);
                }
                ImGui::PopID();
            }
            ImGui::EndTable(); //mat_table
        }
        ImGui::EndChild(); //mat_list
    }
    
    ImGui::Separator();
    
    //// create new material ImGui::Button ////
    ImGui::SetCursorPosX(ImGui::GetWindowWidth()*0.025); //half of 1 - 0.95
    if(ImGui::Button("Create New Material", ImVec2(ImGui::GetWindowWidth()*0.95, 0.0f))){
        selected_mat = Render::CreateMaterial(TOSTRING("material", Render::MaterialCount()).c_str(), Shader_PBR);
    }
    
    ImGui::Separator();
    
    //// selected material inspector panel ////
    if(selected_mat == -1) return;
    SetPadding;
    if(ImGui::BeginChild("##mat_inspector", ImVec2(ImGui::GetWindowWidth()*.95f, ImGui::GetWindowHeight()*.8f), false)) {
        //// name ////
        ImGui::TextEx("Name   "); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); 
        ImGui::InputText("##mat_name_input", Render::MaterialName(selected_mat), DESHI_NAME_SIZE, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        
        //// shader selection ////
        ImGui::TextEx("Shader "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
        if(ImGui::BeginCombo("##mat_shader_combo", ShaderStrings[Render::MaterialShader(selected_mat)])){
            forI(ArrayCount(ShaderStrings)){
                if(ImGui::Selectable(ShaderStrings[i], Render::MaterialShader(selected_mat) == i)){
                    Render::UpdateMaterialShader(selected_mat, i);
                }
            }
            ImGui::EndCombo(); //mat_shader_combo
        }
        
        ImGui::Separator();
        
        //// material properties ////
        //TODO(Ui) setup material editing other than PBR once we have material parameters
        switch(Render::MaterialShader(selected_mat)){
            //// flat shader ////
            case Shader_Flat:{
                
            }break;
            
            //// PBR shader ////
            //TODO(Ui) add texture image previews
            case Shader_PBR:default:{
				std::vector<u32> matTextures = Render::MaterialTextures(selected_mat);
                ImGui::TextEx("Albedo   "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
                if (ImGui::BeginCombo("##mat_albedo_combo", Render::TextureName(matTextures[0]))) {
                    forI(textures.size()) {
                        if (ImGui::Selectable(textures[i].c_str(), strcmp(Render::TextureName(matTextures[0]), textures[i].c_str()) == 0)) {
                            Render::UpdateMaterialTexture(selected_mat, 0, Render::LoadTexture(textures[i].c_str(), TextureType_Albedo));
                        }
                    }
                    ImGui::EndCombo(); //mat_albedo_combo
                }
                ImGui::TextEx("Normal   "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
                if (ImGui::BeginCombo("##mat_normal_combo", Render::TextureName(matTextures[1]))) {
                    forI(textures.size()) {
                        if (ImGui::Selectable(textures[i].c_str(), strcmp(Render::TextureName(matTextures[1]), textures[i].c_str()) == 0)) {
                            Render::UpdateMaterialTexture(selected_mat, 1, Render::LoadTexture(textures[i].c_str(), TextureType_Normal));
                        }
                    }
                    ImGui::EndCombo(); //mat_normal_combo
                }
                ImGui::TextEx("Specular "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
                if (ImGui::BeginCombo("##mat_spec_combo", Render::TextureName(matTextures[2]))) {
                    forI(textures.size()) {
                        if (ImGui::Selectable(textures[i].c_str(), strcmp(Render::TextureName(matTextures[2]), textures[i].c_str()) == 0)) {
                            Render::UpdateMaterialTexture(selected_mat, 2, Render::LoadTexture(textures[i].c_str(), TextureType_Specular));
                        }
                    }
                    ImGui::EndCombo(); //mat_spec_combo
                }
                ImGui::TextEx("Light    "); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
                if (ImGui::BeginCombo("##mat_light_combo", Render::TextureName(matTextures[3]))) {
                    forI(textures.size()) {
                        if (ImGui::Selectable(textures[i].c_str(), strcmp(Render::TextureName(matTextures[3]), textures[i].c_str()) == 0)) {
                            Render::UpdateMaterialTexture(selected_mat, 3, Render::LoadTexture(textures[i].c_str(), TextureType_Light));
                        }
                    }
                    ImGui::EndCombo(); //mat_light_combo
                }
            }break;
        }
        
        ImGui::EndChild(); //mat_inspector
    }
} //MaterialsTab

enum TwodPresets : u32 {
    Twod_NONE = 0, Twod_Line, Twod_Triangle, Twod_Square, Twod_NGon, Twod_Image, 
};

//TODO(,Ui) convert this to use collapsing headers
inline void GlobalTab(Admin* admin){
    SetPadding; 
    if(ImGui::BeginChild("##global__tab", ImVec2(ImGui::GetWindowWidth()*0.95f, ImGui::GetWindowHeight()*.9f))) {
        //// physics properties ////
        {
			ImGui::TextEx("Pause Physics "); ImGui::SameLine();
			if(ImGui::Button((admin->pause_phys) ? "True" : "False", ImVec2(-FLT_MIN, 0))){
				admin->pause_phys = !admin->pause_phys;
			}    
			ImGui::TextEx("Gravity       "); ImGui::SameLine(); ImGui::InputFloat("##global__gravity", &admin->physics.gravity);
			
			//ImGui::TextEx("Phys TPS      "); ImGui::SameLine(); ImGui::InputFloat("##phys_tps", )
        }
		
        //// camera properties ////
        {
			ImGui::Separator();
			ImGui::TextEx("Camera"); ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetWindowWidth()*.5f);
			if(ImGui::Button("Zero", ImVec2(ImGui::GetWindowWidth()*.225f, 0))){
				admin->editor.camera->position = Vector3::ZERO; admin->editor.camera->rotation = Vector3::ZERO;
			} ImGui::SameLine();
			if(ImGui::Button("Reset", ImVec2(ImGui::GetWindowWidth()*.25f, 0))){
				admin->editor.camera->position = {4.f,3.f,-4.f}; admin->editor.camera->rotation = {28.f,-45.f,0.f};
			}
			
			ImGui::TextEx("Position  "); ImGui::SameLine(); ImGui::InputVector3("##cam_pos", &admin->editor.camera->position);
			ImGui::TextEx("Rotation  "); ImGui::SameLine(); ImGui::InputVector3("##cam_rot", &admin->editor.camera->rotation);
			ImGui::TextEx("Near Clip "); ImGui::SameLine(); ImGui::InputFloat("##global__nearz", &admin->editor.camera->nearZ);
			ImGui::TextEx("Far Clip  "); ImGui::SameLine(); ImGui::InputFloat("##global__farz", &admin->editor.camera->farZ);
			ImGui::TextEx("FOV       "); ImGui::SameLine(); ImGui::InputFloat("##global__fov", &admin->editor.camera->fov);
		}
		
		//// render settings ////
		{
			ImGui::Separator();
			local RenderSettings* settings = Render::GetSettings();
			local const char* resolution_strings[] = { "128", "256", "512", "1024", "2048", "4096" };
			local u32 resolution_values[] = { 128, 256, 512, 1024, 2048, 4096 };
			local u32 shadow_resolution_index = 4;
			local vec3 clear_color    = settings->clearColor;
			local vec4 selected_color = settings->selectedColor;
			local vec4 collider_color = settings->colliderColor;
			
			ImGui::TextCentered("Render Settings");
            ImGui::Checkbox("Debugging", (bool*)&settings->debugging);
            ImGui::Checkbox("Shader printf", (bool*)&settings->printf);
            ImGui::Checkbox("Recompile all shaders", (bool*)&settings->recompileAllShaders);
            ImGui::Checkbox("Find mesh tri-neighbors", (bool*)&settings->findMeshTriangleNeighbors);
            ImGui::TextCentered("^ above settings require restart ^");
			ImGui::TextEx("Logging level"); ImGui::SameLine(); ImGui::SliderUInt32("##rs_logging_level", &settings->loggingLevel, 0, 4);
			ImGui::Checkbox("Crash on error", (bool*)&settings->crashOnError);
			ImGui::Checkbox("Compile shaders with optimization", (bool*)&settings->optimizeShaders);
			ImGui::Checkbox("Shadow PCF", (bool*)&settings->shadowPCF);
			ImGui::TextEx("Shadowmap resolution"); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
			if(ImGui::BeginCombo("##rs_shadowres_combo", resolution_strings[shadow_resolution_index])){
				forI(ArrayCount(resolution_strings)){
					if(ImGui::Selectable(resolution_strings[i], shadow_resolution_index == i)){
						Render::remakeOffscreen();
						settings->shadowResolution = resolution_values[i];
						shadow_resolution_index = i;
					}
				}
				ImGui::EndCombo(); //rs_shadowres_combo
			}
			ImGui::TextEx("Shadow near clip"); ImGui::SameLine(); ImGui::InputFloat("##rs_shadow_nearz", &settings->shadowNearZ);
			ImGui::TextEx("Shadow far clip"); ImGui::SameLine(); ImGui::InputFloat("##rs_shadow_farz", &settings->shadowFarZ);
			ImGui::TextEx("Shadow depth bias constant"); ImGui::SameLine(); ImGui::InputFloat("##rs_shadow_depthconstant", &settings->depthBiasConstant);
			ImGui::TextEx("Shadow depth bias slope"); ImGui::SameLine(); ImGui::InputFloat("##rs_shadow_depthslope", &settings->depthBiasSlope);
			ImGui::Checkbox("Show shadowmap texture", (bool*)&settings->showShadowMap);
			ImGui::TextEx("Background"); ImGui::SameLine();
			if(ImGui::ColorEdit3("##rs_clear_color", (f32*)&clear_color.r)){
				settings->clearColor = vec4(clear_color, 1.0f);
			}
			ImGui::TextEx("Selected  "); ImGui::SameLine();
			if(ImGui::ColorEdit4("##rs_selected_color", (f32*)&selected_color.r)){
				settings->selectedColor = selected_color;
			}
			ImGui::TextEx("Collider  "); ImGui::SameLine();
			if(ImGui::ColorEdit4("##rs_collider_color", (f32*)&collider_color.r)){
				settings->colliderColor = collider_color;
			}
			ImGui::Checkbox("Only show wireframe", (bool*)&settings->wireframeOnly);
			ImGui::Checkbox("Draw mesh wireframes", (bool*)&settings->meshWireframes);
			ImGui::Checkbox("Draw mesh normals", (bool*)&settings->meshNormals);
			ImGui::Checkbox("Draw light frustrums", (bool*)&settings->lightFrustrums);
			ImGui::Checkbox("Draw temp meshes on top", (bool*)&settings->tempMeshOnTop);
		}
        
        ImGui::EndChild();
    }
}

void DisplayTriggers(Admin* admin) {
    int i = 0;
    for (Entity* e : admin->entities) {
        if (e->type == EntityType_Trigger) {
            Trigger* t = dyncast(Trigger, e);
            switch (t->collider->shape) {
                case ColliderShape_AABB:{
                    Render::TempBox(e->transform.TransformMatrix(), Color::DARK_MAGENTA);
                }break;
                case ColliderShape_Sphere: {
                    
                }break;
                case ColliderShape_Complex: {
                    
                }break;
            }
        }
        i++;
    }
}

void Editor::DebugTools() {
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
    
    ImGui::PushStyleColor(ImGuiCol_Border,               ImGui::ColorToImVec4(Color( 0,  0,  0)));
    ImGui::PushStyleColor(ImGuiCol_Button,               ImGui::ColorToImVec4(Color(40, 40, 40)));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,         ImGui::ColorToImVec4(Color(48, 48, 48)));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,        ImGui::ColorToImVec4(Color(60, 60, 60)));
    ImGui::PushStyleColor(ImGuiCol_WindowBg,             ImGui::ColorToImVec4(colors.c9));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,              ImGui::ColorToImVec4(Color(20, 20, 20)));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,              ImGui::ColorToImVec4(Color(35, 45, 50)));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,        ImGui::ColorToImVec4(Color(42, 54, 60)));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,       ImGui::ColorToImVec4(Color(54, 68, 75)));
    ImGui::PushStyleColor(ImGuiCol_TitleBg,              ImGui::ColorToImVec4(Color(0,   0,  0)));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive,        ImGui::ColorToImVec4(Color(0,   0,  0)));
    ImGui::PushStyleColor(ImGuiCol_Header,               ImGui::ColorToImVec4(Color(35, 45, 50)));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,         ImGui::ColorToImVec4(Color( 0, 74, 74)));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,        ImGui::ColorToImVec4(Color( 0, 93, 93)));
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight,     ImGui::ColorToImVec4(Color(45, 45, 45)));
    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg,        ImGui::ColorToImVec4(Color(10, 10, 10)));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,          ImGui::ColorToImVec4(Color(10, 10, 10)));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        ImGui::ColorToImVec4(Color(55, 55, 55)));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  ImGui::ColorToImVec4(Color(75, 75, 75)));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImGui::ColorToImVec4(Color(65, 65, 65)));
    ImGui::PushStyleColor(ImGuiCol_TabActive,            ImGui::ColorToImVec4(Color::VERY_DARK_CYAN));
    ImGui::PushStyleColor(ImGuiCol_TabHovered,           ImGui::ColorToImVec4(Color::DARK_CYAN));
    ImGui::PushStyleColor(ImGuiCol_Tab,                  ImGui::ColorToImVec4(colors.c1));
    ImGui::PushStyleColor(ImGuiCol_Separator,            ImGui::ColorToImVec4(Color::VERY_DARK_CYAN));
    
    ImGui::Begin("DebugTools", (bool*)1, ImGuiWindowFlags_NoFocusOnAppearing |  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    //capture mouse if hovering over this window
    WinHovCheck; 
    if(DengInput->mouseX < ImGui::GetWindowPos().x + ImGui::GetWindowWidth()){
        WinHovFlag = true;
    }
    
    SetPadding;
    if (ImGui::BeginTabBar("MajorTabs")) {
        if (ImGui::BeginTabItem("Entities")) {
            EntitiesTab(admin, ImGui::GetFontSize());
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Materials")) {
            MaterialsTab(admin);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Global")) {
            GlobalTab(admin);
            ImGui::EndTabItem();
        }
        //if (ImGui::BeginTabItem("Brushes")) {
        //BrushesTab(admin, ImGui::GetFontSize());
        //ImGui::EndTabItem();
        //}
        ImGui::EndTabBar();
    }
    
    ImGui::PopStyleVar(8);
    ImGui::PopStyleColor(24);
    ImGui::End();
}


void Editor::DebugBar() {
    //for getting fps
    ImGuiIO& io = ImGui::GetIO();
    
    int FPS = floor(io.Framerate);
    
    //num of active columns
    int activecols = 7;
    
    //font size for centering ImGui::TextEx
    float fontsize = ImGui::GetFontSize();
    
    //flags for showing different things
    persist bool show_fps = true;
    persist bool show_fps_graph = true;
    persist bool show_world_stats = true;
    persist bool show_selected_stats = true;
    persist bool show_floating_fps_graph = false;
    persist bool show_time = true;
    
    ImGui::SetNextWindowSize(ImVec2(DengWindow->width, 20));
    ImGui::SetNextWindowPos(ImVec2(0, DengWindow->height - 20));
    
    
    //window styling
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,   ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(2, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border,           ImGui::ColorToImVec4(Color(0, 0, 0, 255)));
    ImGui::PushStyleColor(ImGuiCol_WindowBg,         ImGui::ColorToImVec4(Color(20, 20, 20, 255)));
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImGui::ColorToImVec4(Color(45, 45, 45, 255)));
    
    ImGui::Begin("DebugBar", (bool*)1, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    debugbarheight = 20;
    //capture mouse if hovering over this window
    WinHovCheck; 
    
    activecols = show_fps + show_fps_graph + 3 * show_world_stats + 2 * show_selected_stats + show_time + 1;
    if (ImGui::BeginTable("DebugBarTable", activecols, ImGuiTableFlags_BordersV | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_SizingFixedFit)) {
        
        //precalc strings and stuff so we can set column widths appropriately
        std::string str1 = TOSTRING("wents: ", admin->entities.size());
        float strlen1 = (fontsize - (fontsize / 2)) * str1.size();
        std::string str2 = TOSTRING("wtris: ", Render::GetStats()->totalTriangles);
        float strlen2 = (fontsize - (fontsize / 2)) * str2.size();
        std::string str3 = TOSTRING("wverts: ",Render::GetStats()->totalVertices);
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
        
        if (ImGui::TableNextColumn() && show_fps) {
            //trying to keep it from changing width of column
            //actually not necessary anymore but im going to keep it cause 
            //it keeps the numbers right aligned
            if (FPS % 1000 == FPS) {
                ImGui::TextEx(TOSTRING("FPS:  ", FPS).c_str());
            }
            else if (FPS % 100 == FPS) {
                ImGui::TextEx(TOSTRING("FPS:   ", FPS).c_str());
            }
            else {
                ImGui::TextEx(TOSTRING("FPS: ", FPS).c_str());
            }
            
        }
        
        //FPS graph inline
        if (ImGui::TableNextColumn() && show_fps_graph) {
            //how much data we store
            persist int prevstoresize = 100;
            persist int storesize = 100;
            
            //how often we update
            persist int fupdate = 20;
            persist int frame_count = 0;
            
            //maximum FPS
            persist int maxval = 0;
            
            //real values and printed values
            persist std::vector<float> values(storesize);
            persist std::vector<float> pvalues(storesize);
            
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
            
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImGui::ColorToImVec4(Color(0, 255, 200, 255)));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorToImVec4(Color(20, 20, 20, 255)));
            
            ImGui::PlotLines("", &pvalues[0], pvalues.size(), 0, 0, 0, maxval, ImVec2(64, 20));
            
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
        }
        
        
        //World stats
        
        //Entity Count
        if (ImGui::TableNextColumn() && show_world_stats) {
            ImGui::SameLine((ImGui::GetColumnWidth() - strlen1) / 2);
            ImGui::TextEx(str1.c_str());
        }
        
        //Triangle Count
        if (ImGui::TableNextColumn() && show_world_stats) {
            //TODO( sushi,Ui) implement triangle count when its avaliable
            ImGui::SameLine((ImGui::GetColumnWidth() - strlen2) / 2);
            ImGui::TextEx(str2.c_str());
        }
        
        //Vertice Count
        if (ImGui::TableNextColumn() && show_world_stats) {
            //TODO( sushi,Ui) implement vertice count when its avaliable
            ImGui::SameLine((ImGui::GetColumnWidth() - strlen3) / 2);
            ImGui::TextEx(str3.c_str());
        }
        
        
        
        // Selected Stats
        
        
        
        //Triangle Count
        if (ImGui::TableNextColumn() && show_selected_stats) {
            //TODO( sushi,Ui) implement triangle count when its avaliable
            //Entity* e = admin->selectedEntity;
            ImGui::SameLine((ImGui::GetColumnWidth() - strlen4) / 2);
            ImGui::TextEx(str4.c_str());
        }
        
        //Vertice Count
        if (ImGui::TableNextColumn() && show_selected_stats) {
            //TODO( sushi,Ui) implement vertice count when its avaliable
            //Entity* e = admin->selectedEntity;
            ImGui::SameLine((ImGui::GetColumnWidth() - strlen5) / 2);
            ImGui::TextEx(str5.c_str());
        }
        
        //Middle Empty ImGui::Separator (alert box)
        if (ImGui::TableNextColumn()) {
            if (DengConsole->show_alert) {
                f32 flicker = (sinf(M_2PI * DengTime->totalTime + cosf(M_2PI * DengTime->totalTime)) + 1)/2;
                Color col_bg = DengConsole->alert_color * flicker;    col_bg.a = 255;
                Color col_text = DengConsole->alert_color * -flicker; col_text.a = 255;
                
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImGui::ColorToImVec4(col_bg)));
                
                std::string str6;
                if(DengConsole->alert_count > 1) {
                    str6 = TOSTRING("(",DengConsole->alert_count,") ",DengConsole->alert_message);
                }else{
                    str6 = DengConsole->alert_message;
                }
                float strlen6 = (font_width / 2) * str6.size();
                ImGui::SameLine((ImGui::GetColumnWidth() - strlen6) / 2); ImGui::PushItemWidth(-1);
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorToImVec4(Color(col_text)));
                ImGui::TextEx(str6.c_str());
                ImGui::PopStyleColor();
            }
            
            
        }
        
        //Show Time
        if (ImGui::TableNextColumn()) {
            //https://stackoverflow.com/questions/24686846/get-current-time-in-milliseconds-or-hhmmssmmm-format
			
            //get current time
            auto now = std::chrono::system_clock::now();
            
            //convert to std::time_t so we can convert to std::tm
            auto timer = std::chrono::system_clock::to_time_t(now);
            
            //convert to broken time
            std::tm bt = *std::localtime(&timer);
            
            std::ostringstream oss;
            
            oss << std::put_time(&bt, "%H:%M:%S");
            
            std::string str7 = oss.str();
            float strlen7 = (fontsize - (fontsize / 2)) * str7.size();
            ImGui::SameLine(32 - (strlen7 / 2));
            
            ImGui::TextEx(str7.c_str());
            
        }
        
        
        //Context menu for toggling parts of the bar
        if (ImGui::IsMouseReleased(1) && ImGui::IsWindowHovered()) ImGui::OpenPopup("Context");
        if (ImGui::BeginPopup("Context")) {
            DengConsole->IMGUI_MOUSE_CAPTURE = true;
            ImGui::Separator();
            if (ImGui::Button("Open Debug Menu")) {
                //showDebugTools = true;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        ImGui::EndTable();
    }
    
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(3);
    ImGui::End();
}

void Editor::DrawTimes(){
    std::string time1 = DengTime->FormatTickTime  ("Time       : {t}\n"
                                                   "Window     : {w}\n"
                                                   "Input      : {i}\n");
    time1            += DengAdmin->FormatAdminTime("Physics Lyr: {P}\n"
                                                   "        Sys: {p}\n"
                                                   "Canvas  Lyr: {C}\n"
                                                   "        Sys: {c}\n"
                                                   "World   Lyr: {W}\n"
                                                   "        Sys: {w}\n"
                                                   "Sound   Lyr: {S}\n"
                                                   "Sound   Lyr: {S}\n"
                                                   "        Sys: {s}\n"
												   "Editor     : {e}\n");
    time1            += DengTime->FormatTickTime  ("Admin      : {a}\n"
                                                   "Console    : {c}\n"
                                                   "Render     : {r}\n"
                                                   "Frame      : {f}");
    
    ImGui::SetCursorPos(ImVec2(DengWindow->width - 150, menubarheight));
    ImGui::TextEx(time1.c_str());
}

//sort of sandbox for drawing ImGui stuff over the entire screen
void Editor::DebugLayer() {
    ImGui::SetNextWindowSize(ImVec2(DengWindow->width, DengWindow->height));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorToImVec4(Color(0, 0, 0, 0)));
    Camera* c = admin->mainCamera;
    float time = DengTime->totalTime;
    
    persist std::vector<pair<float, Vector2>> times;
    
    persist std::vector<Vector3> spots;
    
    
    ImGui::Begin("DebugLayer", 0, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
    
    Vector2 mp = DengInput->mousePos;
    
    float fontsize = ImGui::GetFontSize();
    
    
    if (DengInput->KeyPressed(MouseButton::LEFT) && rand() % 100 + 1 == 80) {
        times.push_back(pair<float, Vector2>(0.f, mp));
    }
    
    int index = 0;
    for (auto& f : times) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorToImVec4(Color(255. * fabs(sinf(time)), 255. * fabs(cosf(time)), 255, 255)));
        
        f.first += DengTime->deltaTime;
        
        Vector2 p = f.second;
        
        ImGui::SetCursorPos(ImVec2(p.x + 20 * sin(2 * time), p.y - 200 * (f.first / 5)));
        
        Vector2 curpos = Vector2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY());
        
        std::string str1 = "hehe!!!!";
        float strlen1 = (fontsize - (fontsize / 2)) * str1.size();
        for (int i = 0; i < str1.size(); i++) {
            ImGui::SetCursorPos(ImVec2(curpos.x + i * fontsize / 2,
                                       curpos.y + sin(10 * time + cos(10 * time + (i * M_PI / 2)) + (i * M_PI / 2))));
            ImGui::TextEx(str1.substr(i, 1).c_str());
        }
        
        if (f.first >= 5) {
            times.erase(times.begin() + index);
            index--;
        }
        
        ImGui::PopStyleColor();
        index++;
    }
    
    
    if (admin->paused) {
        std::string s = "ENGINE PAUSED";
        float strlen = (fontsize - (fontsize / 2)) * s.size();
        //ImGui::SameLine(32 - (strlen / 2));
        ImGui::SetCursorPos(ImVec2(DengWindow->width - strlen * 1.3, menubarheight));
        ImGui::TextEx(s.c_str());
    }
    
    ImGui::PopStyleColor();
    ImGui::End();
}


void Editor::WorldGrid(Vector3 cpos) {
    int lines = 50;
	f32 xp = floor(cpos.x) + lines;
	f32 xn = floor(cpos.x) - lines;
	f32 zp = floor(cpos.z) + lines;
	f32 zn = floor(cpos.z) - lines;
    
	Color color(50, 50, 50);
    for(int i = 0; i < lines * 2 + 1; i++){
        Vector3 v1 = Vector3(xn+i, 0, zn);
        Vector3 v2 = Vector3(xn+i, 0, zp);
        Vector3 v3 = Vector3(xn,   0, zn+i);
        Vector3 v4 = Vector3(xp,   0, zn+i);
        
		if(xn+i != 0) Render::TempLine(v1, v2, color);
		if(zn+i != 0) Render::TempLine(v3, v4, color);
    }
	
	Render::TempLine(vec3{-1000,0,0}, vec3{1000,0,0}, Color::RED);
	Render::TempLine(vec3{0,-1000,0}, vec3{0,1000,0}, Color::GREEN);
	Render::TempLine(vec3{0,0,-1000}, vec3{0,0,1000}, Color::BLUE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// editor struct

void Editor::Init(Admin* a){
    admin = a;
    settings = {};
    
    selected.reserve(8);
    camera = new Camera(90.f, .01f, 1000.01f, true);
    Render::UpdateCameraViewMatrix(camera->viewMat);
    Render::UpdateCameraPosition(camera->position);
    undo_manager.Init();
    level_name = "";
    
    showDebugTools      = true;
    showTimes           = true;
    showDebugBar        = true;
    showMenuBar         = true;
    showImGuiDemoWindow = false;
    showDebugLayer      = true;
	showWorldGrid       = true;
    ConsoleHovFlag      = false;
    
    showEditorWin = false;
    
    files = Assets::iterateDirectory(Assets::dirModels());
    textures = Assets::iterateDirectory(Assets::dirTextures());
    levels = Assets::iterateDirectory(Assets::dirLevels());
    
    fonth = ImGui::GetFontSize();
    fontw = fonth / 2.f;
}

void Editor::Update(){
    ////////////////////////////
    //// handle user inputs ////
    ////////////////////////////
    
    {//// general ////
        if (DengInput->KeyPressed(Key::P | InputMod_Lctrl)) {
            admin->paused = !admin->paused;
        }
    }
	
    {//// select ////
        if (!DengConsole->IMGUI_MOUSE_CAPTURE && !admin->controller.cameraLocked) {
            if (DengInput->KeyPressed(MouseButton::LEFT)) {
                Entity* e = SelectEntityRaycast();
                if(!DengInput->LShiftDown()) selected.clear(); 
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
    }
	
    {//// camera ////
        //toggle ortho
        persist Vector3 ogpos;
        persist Vector3 ogrot;
        if (DengInput->KeyPressed(DengKeys.perspectiveToggle)) {
            switch (camera->mode) {
                case(CameraMode_Perspective): {  
                    ogpos = camera->position;
                    ogrot = camera->rotation;
                    camera->type = CameraMode_Orthographic; 
                    camera->farZ = 1000000; 
                } break;
                case(CameraMode_Orthographic): { 
                    camera->position = ogpos; 
                    camera->rotation = ogrot;
                    camera->mode = CameraMode_Perspective; 
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
        
        //look at selected
        if(DengInput->KeyPressed(DengKeys.gotoSelected)){
            camera->position = selected[0]->transform.position + Vector3(4.f, 3.f, -4.f);
            camera->rotation = {28.f, -45.f, 0.f};
        }
    }
	
    {//// undo/redo ////
        if (DengInput->KeyPressed(DengKeys.undo)) undo_manager.Undo();
        if (DengInput->KeyPressed(DengKeys.redo)) undo_manager.Redo();
    }
	
    {//// interface ////
        if (DengInput->KeyPressed(DengKeys.toggleDebugMenu)) showDebugTools = !showDebugTools;
        if (DengInput->KeyPressed(DengKeys.toggleDebugBar))  showDebugBar   = !showDebugBar;
        if (DengInput->KeyPressed(DengKeys.toggleMenuBar))   showMenuBar    = !showMenuBar;
    }
	
	//TODO(delle,Op) actually do a copy of memory once entities are less anonymous
	//NOTE doesnt copy the events of an entity at the moment
	{//// cut/copy/paste ////
		if(DengInput->KeyPressed(DengKeys.cut)){
			if(selected.size()){
				//copy
				Assets::deleteFile(copy_path, false);
				std::string entity_text = selected[0]->SaveTEXT();
				std::string filepath = Assets::dirTemp() + std::to_string(DengTime->totalTime) + "entcopy";
				Assets::writeFile(filepath, entity_text.c_str(), entity_text.size());
				
				copy_path = filepath;
				for(Component* c : selected[0]->components){
					if(c->type == ComponentType_MeshComp){
						copy_mesh_diffs.push_back(pair<u32,u32>(-1, ((MeshComp*)c)->meshID));
					}
				}
				
				//delete
				DengAdmin->DeleteEntity(selected[0]);
				selected.clear();
			}
		}
		if(DengInput->KeyPressed(DengKeys.copy)){
			if(selected.size()){
				Assets::deleteFile(copy_path, false);
				std::string entity_text = selected[0]->SaveTEXT();
				std::string filepath = Assets::dirTemp() + std::to_string(DengTime->totalTime) + "entcopy";
				Assets::writeFile(filepath, entity_text.c_str(), entity_text.size());
				
				copy_path = filepath;
				for(Component* c : selected[0]->components){
					if(c->type == ComponentType_MeshComp){
						copy_mesh_diffs.push_back(pair<u32,u32>(-1, ((MeshComp*)c)->meshID));
					}
				}
			}
		}
		if(DengInput->KeyPressed(DengKeys.paste)){
			Entity* new_entity = Entity::LoadTEXT(DengAdmin, copy_path, copy_mesh_diffs);
			if(new_entity){
				DengAdmin->CreateEntity(new_entity);
				selected.clear();
				selected.push_back(new_entity);
			}
		}
	}
	
	///////////////////////////////
	//// render user interface ////
	///////////////////////////////
	
	//TODO(delle,Cl) program crashes somewhere in DebugTools() if minimized
	if (!DengWindow->minimized) {
		WinHovFlag = 0;
		font_width = ImGui::GetFontSize();
		
		if (showDebugLayer) DebugLayer();
		if (showTimes)      DrawTimes();
		if (showDebugTools) DebugTools();
		if (showDebugBar)   DebugBar();
		if (showMenuBar)    MenuBar();
		if (showEditorWin)  CreateEditorWin();
		if (showWorldGrid)  WorldGrid(camera->position);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1)); {
			if (showImGuiDemoWindow) ImGui::ShowDemoWindow();
		}ImGui::PopStyleColor();
		
		if (!showMenuBar)    menubarheight = 0;
		if (!showDebugBar)   debugbarheight = 0;
		if (!showDebugTools) debugtoolswidth = 0;
		
		DengConsole->IMGUI_MOUSE_CAPTURE = (ConsoleHovFlag || WinHovFlag) ? true : false;
	}
	
	/////////////////////////////////////////
	//// render selected entity outlines ////
	/////////////////////////////////////////
	
	//TODO(delle,Cl) possibly clean up  the renderer's selected mesh stuff by having the 
	//renderer take a pointer of u32 that's stored here, or just dont have the renderer know about
	//selected entities since thats a game thing
	Render::RemoveSelectedMesh(-1);
	for(Entity* e : selected){
		if(MeshComp* mc = e->GetComponent<MeshComp>()){
			if(!Render::GetSettings()->findMeshTriangleNeighbors){
				Render::AddSelectedMesh(mc->meshID);
			}else{
				std::vector<Vector2> outline = mc->mesh->GenerateOutlinePoints(e->transform.TransformMatrix(), camera->projMat, camera->viewMat,
																			   DengWindow->dimensions, camera->position);
				for (int i = 0; i < outline.size(); i += 2) {
					ImGui::DebugDrawLine(outline[i], outline[i + 1], Color::CYAN);
				}
			}
		}
	}
	
	////////////////////////////////
	////  selected entity debug ////
	////////////////////////////////
	DisplayTriggers(admin);
}

void Editor::Reset(){
    //camera->position = camera_pos;
    //camera->rotation = camera_rot;
    selected.clear();
    undo_manager.Reset();
	ClearCopiedEntities();
}

void Editor::Cleanup(){
	Assets::deleteFile(copy_path, false);
}

void Editor::CreateEditorWin() {
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 5);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(1, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0);
    
    ImGui::PushStyleColor(ImGuiCol_Border,               ImGui::ColorToImVec4(Color(0, 0, 0)));
    ImGui::PushStyleColor(ImGuiCol_Button,               ImGui::ColorToImVec4(Color(40, 40, 40)));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,         ImGui::ColorToImVec4(Color(48, 48, 48)));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,        ImGui::ColorToImVec4(Color(60, 60, 60)));
    ImGui::PushStyleColor(ImGuiCol_WindowBg,             ImGui::ColorToImVec4(colors.c9));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,              ImGui::ColorToImVec4(Color(20, 20, 20)));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,              ImGui::ColorToImVec4(Color(35, 45, 50)));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,        ImGui::ColorToImVec4(Color(42, 54, 60)));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,       ImGui::ColorToImVec4(Color(54, 68, 75)));
    ImGui::PushStyleColor(ImGuiCol_TitleBg,              ImGui::ColorToImVec4(Color(0, 0, 0)));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive,        ImGui::ColorToImVec4(Color(0, 0, 0)));
    ImGui::PushStyleColor(ImGuiCol_Header,               ImGui::ColorToImVec4(Color(35, 45, 50)));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,         ImGui::ColorToImVec4(Color(0, 74, 74)));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,        ImGui::ColorToImVec4(Color(0, 93, 93)));
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight,     ImGui::ColorToImVec4(Color(45, 45, 45)));
    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg,        ImGui::ColorToImVec4(Color(10, 10, 10)));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,          ImGui::ColorToImVec4(Color(10, 10, 10)));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        ImGui::ColorToImVec4(Color(55, 55, 55)));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  ImGui::ColorToImVec4(Color(75, 75, 75)));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImGui::ColorToImVec4(Color(65, 65, 65)));
    ImGui::PushStyleColor(ImGuiCol_TabActive,            ImGui::ColorToImVec4(Color::VERY_DARK_CYAN));
    ImGui::PushStyleColor(ImGuiCol_TabHovered,           ImGui::ColorToImVec4(Color::DARK_CYAN));
    ImGui::PushStyleColor(ImGuiCol_Tab,                  ImGui::ColorToImVec4(colors.c1));
    ImGui::PushStyleColor(ImGuiCol_Separator,            ImGui::ColorToImVec4(Color::VERY_DARK_CYAN));
    ImGui::Begin("Editor Window", 0);
    WinHovCheck;
    if (DengInput->mouseX < ImGui::GetWindowPos().x + ImGui::GetWindowWidth()) {
        WinHovFlag = true;
    }
    if (ImGui::BeginTabBar("MajorTabs")) {
        if (ImGui::BeginTabItem("Entities")) {
            EntitiesTab(admin, ImGui::GetFontSize());
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Materials")) {
            MaterialsTab(admin);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Global")) {
            GlobalTab(admin);
            ImGui::EndTabItem();
        }
        //if (ImGui::BeginTabItem("Brushes")) {
        //BrushesTab(admin, ImGui::GetFontSize());
        //ImGui::EndTabItem();
        //}
        ImGui::EndTabBar();
    }
    ImGui::PopStyleVar(8);
    ImGui::PopStyleColor(24);
    ImGui::End();
}