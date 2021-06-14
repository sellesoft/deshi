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
#include "entities/Trigger.h"
#include "../core.h"
#include "../math/Math.h"
#include "../scene/Scene.h"
#include "../geometry/Edge.h"

#include <iomanip> //std::put_time
#include <thread>

f32 font_width = 0;

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
#define WinHovCheck if (IsWindowHovered()) WinHovFlag = true 
//allows me to manually set padding so i have a little more control than ImGui gives me (I think idk lol)
#define SetPadding SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2)

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
    using namespace ImGui;
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleColor(ImGuiCol_PopupBg,   ColToVec4(Color(20, 20, 20, 255)));
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ColToVec4(Color(20, 20, 20, 255)));
    
    if(BeginMainMenuBar()) { WinHovCheck; 
        menubarheight = GetWindowHeight();
        if(BeginMenu("File")) { WinHovCheck; 
            if (MenuItem("New")) {
                admin->Reset();
            }
            if (MenuItem("Save")) {
                if(level_name == ""){
                    ERROR("Level not saved before; Use 'Save As'");
                }else{
                    admin->SaveTEXT(level_name.c_str());
                }
            }
            if (BeginMenu("Save As")) { WinHovCheck;
                static char buff[255] = {};
                if(InputText("##saveas_input", buff, 255, ImGuiInputTextFlags_EnterReturnsTrue)) {
                    admin->SaveTEXT(buff);
                }
                EndMenu();
            }
            if (BeginMenu("Load")) { WinHovCheck;
                for_n(i, levels.size()) {
                    if (MenuItem(levels[i].c_str())) {
                        admin->LoadTEXT(levels[i].c_str());
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
//TODO(sushi, ClUi) completely rewrite this menu
inline void EventsMenu(Entity* current) {
    using namespace ImGui;
    
    std::vector<Entity*> entities = g_admin->entities;
    static Entity* other = nullptr;
    static Component* selcompr = nullptr;
    static Component* selcompl = nullptr;
    
    if(!current) {
        other = nullptr;
        selcompr = nullptr;
        selcompl = nullptr;
        return;
    }
    
    ImGui::SetNextWindowSize(ImVec2(DengWindow->width / 2, DengWindow->height / 2));
    ImGui::SetNextWindowPos(ImVec2(DengWindow->width / 4, DengWindow->height / 4));
    Begin("##EventsMenu", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
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
        
        //PRINTLN(maxlen);
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

inline void EntitiesTab(EntityAdmin* admin, float fontsize){
    using namespace ImGui;
	
    local_persist b32 rename_ent = false;
    local_persist char rename_buffer[DESHI_NAME_SIZE] = {};
    local_persist Entity* events_ent = 0;
	
    std::vector<Entity*>& selected = admin->editor.selected;
	
    //// selected entity keybinds ////
    //start renaming first selected entity
    if(selected.size() && DengInput->KeyPressed(Key::F2 | INPUTMOD_ANY)){
        rename_ent = true;
        DengConsole->IMGUI_KEY_CAPTURE = true;
        if(selected.size() > 1) selected.erase(selected.begin()+1, selected.end());
        cpystr(rename_buffer, selected[0]->name, DESHI_NAME_SIZE);
    }
    //submit renaming entity
    if(rename_ent && DengInput->KeyPressed(Key::ENTER | INPUTMOD_ANY)){
        rename_ent = false;
        DengConsole->IMGUI_KEY_CAPTURE = false;
        cpystr(selected[0]->name, rename_buffer, DESHI_NAME_SIZE);
    }
    //stop renaming entity
    if(rename_ent && DengInput->KeyPressed(Key::ESCAPE | INPUTMOD_ANY)){
        rename_ent = false;
        DengConsole->IMGUI_KEY_CAPTURE = false;
    }
    //delete selected entities
    if(selected.size() && DengInput->KeyPressed(Key::DELETE | INPUTMOD_ANY)){
        //TODO(Ui) re-enable this with a popup to delete OR with undoing on delete
    }
	
    //// entity list panel ////
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ColToVec4(Color(25, 25, 25)));
    ImGui::SetCursorPosX(ImGui::GetWindowWidth()*0.025);
    if(ImGui::BeginChild("##entity_list", ImVec2(ImGui::GetWindowWidth() * 0.95f, 100))) {
        //if no entities, draw empty list, draw create new button, and early out
        if (admin->entities.size() == 0) { 
            float time = DengTime->totalTime;
            std::string str1 = "Nothing yet...";
            float strlen1 = (fontsize - (fontsize / 2)) * str1.size();
            for (int i = 0; i < str1.size(); i++) {
                ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - strlen1) / 2 + i * (fontsize / 2), (ImGui::GetWindowSize().y - fontsize) / 2 + sin(10 * time + cos(10 * time + (i * M_PI / 2)) + (i * M_PI / 2))));
                ImGui::Text(str1.substr(i, 1).c_str());
            }
			
            ImGui::EndChild();
            PopStyleColor();
            ImGui::Separator();
			
            //// create new entity button ////
            ImGui::SetCursorPosX(ImGui::GetWindowWidth()*0.025); //half of 1 - 0.95
            if(ImGui::Button("Create New Entity", ImVec2(ImGui::GetWindowWidth()*0.95, 0.0f))){
                selected.clear();
                //TODO(delle,Ui) setup create new entity button
            }
            ImGui::Separator();
            return;
        }
        
        if (ImGui::BeginTable("##entity_list_table", 5, ImGuiTableFlags_BordersInner)) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, font_width * 2.f);  //visible button
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, font_width * 3.f); //id
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);                  //name
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, font_width * 3.5f); //events button
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, font_width);        //delete button
			
            for_n(ent_idx, admin->entities.size()){
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
                for_n(i, selected.size()){ if(ent == selected[i]){ selected_idx = i; break; } }
                bool is_selected = selected_idx != -1;
                if(ImGui::Selectable(label, is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)){
					if(is_selected){
						if(DengInput->CtrlDown()){
							selected.erase(selected.begin()+selected_idx);
						}else{
							selected.clear();
							selected.push_back(ent);
						}
					}else{
						if(DengInput->CtrlDown()){
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
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ColToVec4(colors.c2));
                    ImGui::InputText("##ent_rename_input", rename_buffer, DESHI_NAME_SIZE, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
                    ImGui::PopStyleColor();
                }else{
                    ImGui::Text(ent->name);
                }
				
                //// events button ////
                ImGui::TableSetColumnIndex(3);
                if (Button("Events", ImVec2(-FLT_MIN, 0.0f))) {
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
        ImGui::EndChild();
    }//Entity List Scroll child window
    PopStyleColor();
    
    Separator();
	
    //// create new entity button ////
    SetCursorPosX(ImGui::GetWindowWidth()*0.025); //half of 1 - 0.95
    if(ImGui::Button("Create New Entity", ImVec2(ImGui::GetWindowWidth()*0.95, 0.0f))){
        selected.clear();
        //TODO(delle,Ui) setup create new entity button
    }
	
    Separator();
    
    //// selected entity inspector panel ////
    Entity* sel = admin->editor.selected.size() ? admin->editor.selected[0] : 0;
    if(!sel) return;
    PushStyleVar(ImGuiStyleVar_IndentSpacing, 5.0f);
    ImGui::SetCursorPosX(ImGui::GetWindowWidth()*0.025);
    if(BeginChild("##ent_inspector", ImVec2(GetWindowWidth() * 0.95f, GetWindowHeight() * .9f), true, ImGuiWindowFlags_NoScrollbar)) {
        
        //// name ////
        SetPadding; Text(TOSTRING(sel->id, ":").c_str()); 
        SameLine(); SetNextItemWidth(-FLT_MIN); InputText("##ent_name_input", sel->name, DESHI_NAME_SIZE, 
                                                          ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        
        //// transform ////
        int tree_flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoAutoOpenOnLog | ImGuiTreeNodeFlags_DefaultOpen;
        //SetNextItemOpen(true, ImGuiCond_Once);
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
                    MeshComp* mc = dyncast(MeshComp, c);
                    if(mc && TreeNodeEx("Mesh", tree_flags)){
                        MeshVk& mvk = DengRenderer->meshes[mc->meshID];
						
                        Text("Visible  "); SameLine();
                        if(Button((mvk.visible) ? "True" : "False", ImVec2(-FLT_MIN, 0))){
                            mc->ToggleVisibility();
                        }
						
                        Text("Mesh     "); SameLine(); SetNextItemWidth(-1); 
                        if(BeginCombo("##mesh_combo", mvk.name)){ WinHovCheck;
                            for_n(i, DengRenderer->meshes.size()){
                                if(DengRenderer->meshes[i].base && Selectable(DengRenderer->meshes[i].name, mc->meshID == i)){
                                    mc->ChangeMesh(i);
                                }
                            }
                            EndCombo();
                        }
                        
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
                        TreePop();
                    }
                }break;
                
                //mesh2D
                //TODO implement mesh2D component inspector
                
                //physics
                case ComponentType_Physics:
                if (TreeNodeEx("Physics", tree_flags)) {
                    Physics* d = dyncast(Physics, c);
                    Text("Velocity     "); SameLine(); InputVector3("##phys_vel", &d->velocity);
                    Text("Accelertaion "); SameLine(); InputVector3("##phys_accel", &d->acceleration);
                    Text("Rot Velocity "); SameLine(); InputVector3("##phys_rotvel", &d->rotVelocity);
                    Text("Rot Accel    "); SameLine(); InputVector3("##phys_rotaccel", &d->rotAcceleration);
                    Text("Elasticity   "); SameLine();
                    SetNextItemWidth(-FLT_MIN); InputFloat("##phys_elastic", &d->elasticity);
                    Text("Mass         "); SameLine();
                    SetNextItemWidth(-FLT_MIN); InputFloat("##phys_mass", &d->mass);
                    Text("Kinetic Fric "); SameLine();
                    SetNextItemWidth(-FLT_MIN); InputFloat("##phys_kinfric", &d->kineticFricCoef);
                    Checkbox("Static Position", &d->isStatic);
                    Checkbox("Static Rotation", &d->staticRotation);
                    Checkbox("2D Physics", &d->twoDphys);
                    TreePop();
                }
                break;
                
                //colliders
                case ComponentType_Collider:{
                    Collider* coll = dyncast(Collider, c);
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
                        Text("Collider changing not setup; Updating properties doesnt work yet");
                        
                        switch(coll->type){
                            case ColliderType_Box:{
                                BoxCollider* coll_box = dyncast(BoxCollider, coll);
                                Text("Half Dims "); SameLine(); 
                                if(InputVector3("##coll_halfdims", &coll_box->halfDims)){
                                    //coll_box->RecalculateTensor();
                                }
                            }break;
                            case ColliderType_AABB:{
                                AABBCollider* coll_aabb = dyncast(AABBCollider, coll);
                                Text("Half Dims "); SameLine(); 
                                if(InputVector3("##coll_halfdims", &coll_aabb->halfDims)){
                                    //coll_aabb->RecalculateTensor();
                                }
                            }break;
                            case ColliderType_Sphere:{
                                SphereCollider* coll_sphere = dyncast(SphereCollider, coll);
                                Text("Radius    "); SameLine(); SetNextItemWidth(-FLT_MIN);
                                if(InputFloat("##coll_sphere", &coll_sphere->radius)){
                                    //coll_sphere->RecalculateTensor();
                                }
                            }break;
                        }
                        TreePop();
                    }
                }break;
                
                //audio listener
                case ComponentType_AudioListener:{
                    if (TreeNodeEx("Audio Listener", tree_flags)) {
                        Text("TODO implement audio listener component editing");
                        TreePop();
                    }
                }break;
                
                //audio source
                case ComponentType_AudioSource:{
                    if (TreeNodeEx("Audio Source", tree_flags)) {
                        Text("TODO implement audio source component editing");
                        TreePop();
                    }
                }break;
                
                //camera
                case ComponentType_Camera:{
                    if (TreeNodeEx("Camera", tree_flags)) {
                        Text("TODO implement camera component editing");
                        TreePop();
                    }
                }break;
                
                //light
                case ComponentType_Light:
                if (TreeNodeEx("Light", tree_flags)) {
                    Light* d = dyncast(Light, c);
                    Text("Brightness   "); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
                    InputFloat("##light_brightness", &d->brightness);
                    Text("Position     "); SameLine(); 
                    InputVector3("##light_position", &d->position);
                    Text("Direction    "); SameLine(); 
                    InputVector3("##light_direction", &d->direction);
                    TreePop();
                }
                break;
                
                //orb manager
                case ComponentType_OrbManager:{
                    if (TreeNodeEx("Orbs", tree_flags)) {
                        OrbManager* d = dyncast(OrbManager, c);
                        Text("Orb Count "); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); 
                        InputInt("##orb_orbcount", &d->orbcount);
                        TreePop();
                    }
                }break;
                
                //door
                case ComponentType_Door:{
                    if (TreeNodeEx("Door", tree_flags)) {
                        Text("TODO implement door component editing");
                        TreePop();
                    }
                }break;
                
                //player
                case ComponentType_Player:{
                    if (TreeNodeEx("Player", tree_flags)) {
                        Player* d = dyncast(Player, c);
                        Text("Health "); SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); 
                        InputInt("##player_health", &d->health);
                        TreePop();
                    }
                }break;
                
                //movement
                case ComponentType_Movement:{
                    if (TreeNodeEx("Movement", tree_flags)) {
                        Movement* d = dyncast(Movement, c);
                        Text("Ground Accel    "); SameLine(); SetNextItemWidth(-FLT_MIN);
                        InputFloat("##move_gndaccel", &d->gndAccel);
                        Text("Air Accel       "); SameLine(); SetNextItemWidth(-FLT_MIN);
                        InputFloat("##move_airaccel", &d->airAccel);
                        Text("Jump Impulse    "); SameLine(); SetNextItemWidth(-FLT_MIN);
                        InputFloat("##move_jimp", &d->jumpImpulse);
                        Text("Max Walk Speed  "); SameLine(); SetNextItemWidth(-FLT_MIN);
                        InputFloat("##move_maxwalk", &d->maxWalkingSpeed);
                        Text("Max Run Speed   "); SameLine(); SetNextItemWidth(-FLT_MIN);
                        InputFloat("##move_maxrun", &d->maxRunningSpeed);
                        Text("Max Crouch Speed"); SameLine(); SetNextItemWidth(-FLT_MIN);
                        InputFloat("##move_maxcrouch", &d->maxCrouchingSpeed);
                        TreePop();
                    }
                }break;
            }
        }
        
        //// add/remove component ////
        
        
        EndChild(); //CreateMenu
    }
    PopStyleVar(); //ImGuiStyleVar_IndentSpacing
} //EntitiesTab

inline void MaterialsTab(EntityAdmin* admin){
    using namespace ImGui;
    
    local_persist u32 selected_mat = -1;
    local_persist b32 rename_mat = false;
    local_persist char rename_buffer[DESHI_NAME_SIZE] = {};
	
    //// selected material keybinds ////
    //start renaming material
    if(selected_mat != -1 && DengInput->KeyPressed(Key::F2 | INPUTMOD_ANY)){
        rename_mat = true;
        DengConsole->IMGUI_KEY_CAPTURE = true;
        cpystr(rename_buffer, DengRenderer->materials[selected_mat].name, DESHI_NAME_SIZE);
    }
    //submit renaming material
    if(rename_mat && DengInput->KeyPressed(Key::ENTER | INPUTMOD_ANY)){
        rename_mat = false;
        DengConsole->IMGUI_KEY_CAPTURE = false;
        cpystr(DengRenderer->materials[selected_mat].name, rename_buffer, DESHI_NAME_SIZE);
    }
    //stop renaming material
    if(rename_mat && DengInput->KeyPressed(Key::ESCAPE | INPUTMOD_ANY)){
        rename_mat = false;
        DengConsole->IMGUI_KEY_CAPTURE = false;
    }
    //delete material
    if(selected_mat != -1 && DengInput->KeyPressed(Key::DELETE | INPUTMOD_ANY)){
        //TODO(Ui) re-enable this with a popup to delete OR with undoing on delete
        //DengRenderer->RemoveMaterial(selected_mat);
        //selected_mat = -1;
    }
	
    //// material list panel ////
    SetPadding; 
    if(BeginChild("##mat_list", ImVec2(GetWindowWidth()*0.95, GetWindowHeight()*.14f), false)) {
        if(BeginTable("##mat_table", 3, ImGuiTableFlags_BordersInner)) {
            TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, font_width * 2.5f);
            TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
            TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, font_width);
            
            for_n(mat_idx, DengRenderer->materials.size()) {
                MaterialVk* mat = &DengRenderer->materials[mat_idx];
                PushID(mat->id);
                TableNextRow();
				
                //// id + label ////
                TableSetColumnIndex(0);
                char label[8];
                sprintf(label, " %03d", mat->id);
                if(Selectable(label, selected_mat == mat_idx, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)){
                    selected_mat = (ImGui::GetIO().KeyCtrl) ? -1 : mat_idx; //deselect if CTRL held
                    rename_mat = false;
                    DengConsole->IMGUI_KEY_CAPTURE = false;
                }
                
                //// name text ////
                TableSetColumnIndex(1);
                if(rename_mat && selected_mat == mat_idx){
                    PushStyleColor(ImGuiCol_FrameBg, ColToVec4(colors.c2));
                    InputText("##mat_rename_input", rename_buffer, DESHI_NAME_SIZE, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
                    PopStyleColor();
                }else{
                    Text(mat->name);
                }
                
                //// delete button ////
                TableSetColumnIndex(2);
                if(Button("X", ImVec2(-FLT_MIN, 0.0f))){
                    if(mat_idx == selected_mat) {
                        selected_mat = -1;
                    }else if(selected_mat != -1 && selected_mat > mat_idx) {
                        selected_mat -= 1;
                    }
                    DengRenderer->RemoveMaterial(mat_idx);
                }
                PopID();
            }
            EndTable(); //mat_table
        }
        EndChild(); //mat_list
    }
    
    Separator();
	
    //// create new material button ////
    SetCursorPosX(GetWindowWidth()*0.025); //half of 1 - 0.95
    if(Button("Create New Material", ImVec2(GetWindowWidth()*0.95, 0.0f))){
        selected_mat = DengRenderer->CreateMaterial(TOSTRING("material", DengRenderer->materials.size()).c_str(), Shader_PBR);
    }
	
    Separator();
    
    //// selected material inspector panel ////
    if(selected_mat == -1) return;
    SetPadding;
    if(BeginChild("##mat_inspector", ImVec2(GetWindowWidth()*.95f, GetWindowHeight()*.8f), false)) {
        MaterialVk* mat = &DengRenderer->materials[selected_mat];
		
        //// name ////
        Text("Name   "); SameLine(); SetNextItemWidth(-FLT_MIN); 
        InputText("##mat_name_input", mat->name, DESHI_NAME_SIZE, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		
        //// shader selection ////
        Text("Shader "); SameLine(); SetNextItemWidth(-1);
        if(BeginCombo("##mat_shader_combo", ShaderStrings[mat->shader])){
            for_n(i, carraysize(ShaderStrings)){
                if(Selectable(ShaderStrings[i], mat->shader == i)){
                    DengRenderer->UpdateMaterialShader(mat->id, i);
                }
            }
            EndCombo(); //mat_shader_combo
        }
		
        Separator();
		
        //// material properties ////
        //TODO(Ui) setup material editing other than PBR once we have material parameters
        switch(mat->shader){
            //// flat shader ////
            case Shader_Flat:{
				
            }break;
			
            //// PBR shader ////
            //TODO(Ui) add texture image previews
            case Shader_PBR:{
                Text("Albedo   "); SameLine(); SetNextItemWidth(-1);
                if(BeginCombo("##mat_albedo_combo", DengRenderer->textures[mat->albedoID].filename)){
                    for_n(i, textures.size()){
                        if(Selectable(textures[i].c_str(), strcmp(DengRenderer->textures[mat->albedoID].filename, textures[i].c_str()) == 0)){
                            DengRenderer->UpdateMaterialTexture(mat->id, 0, DengRenderer->LoadTexture(textures[i].c_str(), TextureType_Albedo));
                        }
                    }
                    EndCombo(); //mat_albedo_combo
                }
                Text("Normal   "); SameLine(); SetNextItemWidth(-1);
                if(BeginCombo("##mat_normal_combo", DengRenderer->textures[mat->normalID].filename)){
                    for_n(i, textures.size()){
                        if(Selectable(textures[i].c_str(), strcmp(DengRenderer->textures[mat->normalID].filename, textures[i].c_str()) == 0)){
                            DengRenderer->UpdateMaterialTexture(mat->id, 1, DengRenderer->LoadTexture(textures[i].c_str(), TextureType_Normal));
                        }
                    }
                    EndCombo(); //mat_normal_combo
                }
                Text("Specular "); SameLine(); SetNextItemWidth(-1);
                if(BeginCombo("##mat_spec_combo", DengRenderer->textures[mat->specularID].filename)){
                    for_n(i, textures.size()){
                        if(Selectable(textures[i].c_str(), strcmp(DengRenderer->textures[mat->specularID].filename, textures[i].c_str()) == 0)){
                            DengRenderer->UpdateMaterialTexture(mat->id, 2, DengRenderer->LoadTexture(textures[i].c_str(), TextureType_Specular));
                        }
                    }
                    EndCombo(); //mat_spec_combo
                }
                Text("Light    "); SameLine(); SetNextItemWidth(-1);
                if(BeginCombo("##mat_light_combo", DengRenderer->textures[mat->lightID].filename)){
                    for_n(i, textures.size()){
                        if(Selectable(textures[i].c_str(), strcmp(DengRenderer->textures[mat->lightID].filename, textures[i].c_str()) == 0)){
                            DengRenderer->UpdateMaterialTexture(mat->id, 3, DengRenderer->LoadTexture(textures[i].c_str(), TextureType_Light));
                        }
                    }
                    EndCombo(); //mat_light_combo
                }
            }break;
        }
		
        EndChild(); //mat_inspector
    }
} //MaterialsTab

enum TwodPresets : u32 {
    Twod_NONE = 0, Twod_Line, Twod_Triangle, Twod_Square, Twod_NGon, Twod_Image, 
};

//TODO(delle,Ui) combine this into the EntitiesTab
inline void CreateTab(EntityAdmin* admin, float fontsize){
    using namespace ImGui;
    
    //// creation variables ////
    local_persist const char* presets[] = {"None", "AABB", "Box", "Sphere", "2D", "Player"};
    local_persist const char* premades[] = { "Player" };
    local_persist int current_preset = 0;
    local_persist int current_premade = 0;
    local_persist char entity_name[DESHI_NAME_SIZE] = {};
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
    local_persist u32  mesh_batch_id = 0;
    local_persist f32  light_strength = 1.f;
    local_persist vec3  physics_velocity{}, physics_accel{}, physics_rotVel{}, physics_rotAccel{};
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
    SetPadding; if (BeginChild("CreateMenu", ImVec2(GetWindowWidth() * 0.95f, GetWindowHeight() * .9f), true)) {
        WinHovCheck;
        if (Button("Create")) {
            //create components
            AudioListener* al = 0;
            if (comp_audiolistener) {
                al = new AudioListener(entity_pos, Vector3::ZERO, entity_rot);
            }
            AudioSource* as = 0;
            if (comp_audiosource) {
                as = new AudioSource();
            }
            Collider* coll = 0;
            if (comp_collider) {
                switch (collider_type) {
					case ColliderType_Box: {
						coll = new BoxCollider(collider_halfdims, physics_mass, 0, Event_NONE, collider_nocollide);
					}break;
					case ColliderType_AABB: {
						coll = new AABBCollider(collider_halfdims, physics_mass, 0, Event_NONE, collider_nocollide);
					}break;
					case ColliderType_Sphere: {
						coll = new SphereCollider(collider_radius, physics_mass, 0, Event_NONE, collider_nocollide);
					}break;
                }
            }
            MeshComp* mc = 0;
            if (comp_mesh) {
                u32 new_mesh_id = DengRenderer->CreateMesh(mesh_id, mat4::TransformationMatrix(entity_pos, entity_rot, entity_scale));
                mesh_name = DengRenderer->meshes[mesh_id].name;
                mc = new MeshComp(new_mesh_id, mesh_instance_id);
            }
            Light* light = 0;
            if (comp_light) {
                light = new Light(entity_pos, entity_rot, light_strength);
            }
            Physics* phys = 0;
            Movement* move = 0;
            Player* pl = 0;
            if (comp_physics) {
                phys = new Physics(entity_pos, entity_rot, physics_velocity, physics_accel, physics_rotVel,
								   physics_rotAccel, physics_elasticity, physics_mass, physics_staticPosition);
                if (comp_audiolistener) al->velocity = physics_velocity;
                if (comp_player) { move = new Movement(phys); pl = new Player(move); }
            }
            
            
            
            admin->CreateEntity({ al, as, coll, mc, light, phys, move, pl }, entity_name,
                                Transform(entity_pos, entity_rot, entity_scale));
            
            
            
            
        }Separator();
		
        //// premades ////
        SetPadding; Text("Premades   ");  SameLine(); SetNextItemWidth(-1);
        if (BeginMenu("##premades")) {
            WinHovCheck;
            if (MenuItem("Player")) {
				
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
        if (Combo("##preset_combo", &current_preset, presets, IM_ARRAYSIZE(presets))) {
            WinHovCheck;
            switch (current_preset) {
				case(0):default: {
					comp_audiolistener = comp_audiosource = comp_collider = comp_mesh = comp_light = comp_physics = comp_player = false;
					collider_type = ColliderType_NONE;
					collider_halfdims = Vector3::ONE;
					collider_radius = 1.f;
					mesh_id = mesh_instance_id = 0;
					light_strength = 1.f;
					physics_velocity = Vector3::ZERO; physics_accel = Vector3::ZERO;
					physics_rotVel = Vector3::ZERO; physics_rotAccel = Vector3::ZERO;
					physics_elasticity = .5f; physics_mass = 1.f;
					physics_staticPosition = physics_staticRotation = true;
					cpystr(entity_name, TOSTRING("default", entity_id).c_str(), DESHI_NAME_SIZE);
				}break;
				case(1): {
					comp_audiolistener = comp_audiosource = comp_light = comp_player = false;
					comp_collider = comp_mesh = comp_physics = true;
					collider_type = ColliderType_AABB;
					collider_halfdims = Vector3::ONE;
					mesh_id = 0;
					physics_staticPosition = physics_staticRotation = true;
					cpystr(entity_name, TOSTRING("aabb", entity_id).c_str(), DESHI_NAME_SIZE);
				}break;
				case(2): {
					comp_audiolistener = comp_audiosource = comp_light = comp_player = false;
					comp_collider = comp_mesh = comp_physics = true;
					collider_type = ColliderType_Box;
					collider_halfdims = Vector3::ONE;
					mesh_id = 0;
					physics_staticPosition = physics_staticRotation = true;
					cpystr(entity_name, TOSTRING("box", entity_id).c_str(), DESHI_NAME_SIZE);
				}break;
				case(3): {
					comp_audiolistener = comp_audiosource = comp_light = comp_player = false;
					comp_collider = comp_mesh = comp_physics = true;
					collider_type = ColliderType_Sphere;
					collider_radius = 1.f;
					mesh_id = DengRenderer->GetBaseMeshID("sphere.obj");
					physics_staticPosition = physics_staticRotation = true;
					cpystr(entity_name, TOSTRING("sphere", entity_id).c_str(), DESHI_NAME_SIZE);
				}break;
				case(4): {
					comp_audiolistener = comp_audiosource = comp_collider = comp_mesh = comp_physics = comp_light = comp_player = false;
					comp_2d = true;
					twod_id = -1;
					twod_type = 2;
					twod_vert_count = 3;
					twod_radius = 25.f;
					twod_verts.resize(3);
					twod_verts[0] = { -100.f, 0.f }; twod_verts[1] = { 0.f, 100.f }; twod_verts[2] = { 100.f, 0.f };
					entity_pos = { 700.f, 400.f, 0 };
					cpystr(entity_name, "twod", DESHI_NAME_SIZE);
				}break;
				case(5): {
					comp_light = false;
					comp_audiolistener = comp_audiosource = comp_collider = comp_mesh = comp_physics = comp_player = true;
					collider_type = ColliderType_AABB; //TODO(delle,PhCl) ideally cylinder/capsule collider
					collider_halfdims = vec3(1, 2, 1);
					mesh_id = DengRenderer->GetBaseMeshID("bmonkey.obj");
					physics_staticPosition = physics_staticRotation = false;
					physics_elasticity = 0.f;
					cpystr(entity_name, "player", DESHI_NAME_SIZE);
				}break;
            }
            if (mesh_id < DengRenderer->meshes.size()) mesh_name = DengRenderer->meshes[mesh_id].name;
        }
        
        SetPadding; Text("Name: "); 
        SameLine(); SetNextItemWidth(-1); InputText("##unique_id", entity_name, DESHI_NAME_SIZE, ImGuiInputTextFlags_EnterReturnsTrue | 
                                                    ImGuiInputTextFlags_AutoSelectAll);
        
        //// transform ////
        int tree_flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoAutoOpenOnLog;
        SetNextItemOpen(true, ImGuiCond_Once);
        if (TreeNodeEx("Transform", tree_flags)) {
            Text("Position     "); SameLine(); InputVector3("entity_pos", &entity_pos);   Separator();
            Text("Rotation     "); SameLine(); InputVector3("entity_rot", &entity_rot);   Separator();
            Text("Scale        "); SameLine(); InputVector3("entity_scale", &entity_scale);
            TreePop();
        }
		
        //// toggle components ////
        SetNextItemOpen(true, ImGuiCond_Once);
        if (TreeNodeEx("Components", tree_flags)) {
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
        if (comp_mesh && TreeNodeEx("Mesh", tree_flags)) {
            Text(TOSTRING("MeshID: ", mesh_id).c_str());
            SetNextItemWidth(-1); if (BeginCombo("##mesh_combo", mesh_name)) {
                WinHovCheck;
                for_n(i, DengRenderer->meshes.size()) {
                    if (DengRenderer->meshes[i].base && Selectable(DengRenderer->meshes[i].name, mesh_id == i)) {
                        mesh_id = i;
                        mesh_name = DengRenderer->meshes[i].name;
                    }
                }
                EndCombo();
            }
			
            if (mesh_id != -1) {
                MeshComp* mc = 0;
                mc = new MeshComp(mesh_id, mesh_instance_id);
                MeshVk& mvk = DengRenderer->meshes[mc->meshID];
                Text("Batch    "); SameLine(); SetNextItemWidth(-1);
                if (BeginCombo("##mesh_batch_combo", mc->mesh->batchArray[mesh_batch_id].name)) {
                    WinHovCheck;
                    for_n(i, mc->mesh->batchArray.size()) {
                        if (Selectable(mc->mesh->batchArray[i].name, mesh_batch_id == i)) {
                            mesh_batch_id = i;
                        }
                    }
                    EndCombo();
                }
				
                Text("Material "); SameLine(); SetNextItemWidth(-1);
                if (BeginCombo("##mesh_mat_combo", DengRenderer->materials[mvk.primitives[mesh_batch_id].materialIndex].name)) {
                    WinHovCheck;
                    for_n(i, DengRenderer->materials.size()) {
                        if (Selectable(DengRenderer->materials[i].name, mvk.primitives[mesh_batch_id].materialIndex == i)) {
                            DengRenderer->UpdateMeshBatchMaterial(mc->meshID, mesh_batch_id, i);
                        }
                    }
                    EndCombo();
                }
                Indent(); {
                    Text("Shader "); SameLine(); SetNextItemWidth(-1);
                    if (BeginCombo("##mesh_shader_combo", ShaderStrings[DengRenderer->materials[mvk.primitives[mesh_batch_id].materialIndex].shader])) {
                        WinHovCheck;
                        for_n(i, IM_ARRAYSIZE(ShaderStrings)) {
                            if (Selectable(ShaderStrings[i], DengRenderer->materials[mvk.primitives[mesh_batch_id].materialIndex].shader == i)) {
                                DengRenderer->UpdateMaterialShader(mvk.primitives[mesh_batch_id].materialIndex, i);
                            }
                        }
                        EndCombo();
                    }
					
                    Text("Albedo   "); SameLine(); SetNextItemWidth(-1);
                    if (BeginCombo("##mesh_albedo_combo", DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_id].materialIndex].albedoID].filename)) {
                        WinHovCheck;
                        for_n(i, textures.size()) {
                            if (Selectable(textures[i].c_str(), strcmp(DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_id].materialIndex].albedoID].filename, textures[i].c_str()) == 0)) {
                                DengRenderer->UpdateMaterialTexture(mvk.primitives[mesh_batch_id].materialIndex, 0,
																	DengRenderer->LoadTexture(textures[i].c_str(), TextureType_Albedo));
                            }
                        }
                        EndCombo();
                    }
					
                    Text("Normal   "); SameLine(); SetNextItemWidth(-1);
                    if (BeginCombo("##mesh_normal_combo", DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_id].materialIndex].normalID].filename)) {
                        WinHovCheck;
                        for_n(i, textures.size()) {
                            if (Selectable(textures[i].c_str(), strcmp(DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_id].materialIndex].normalID].filename, textures[i].c_str()) == 0)) {
                                DengRenderer->UpdateMaterialTexture(mvk.primitives[mesh_batch_id].materialIndex, 0,
																	DengRenderer->LoadTexture(textures[i].c_str(), TextureType_Normal));
                            }
                        }
                        EndCombo();
                    }
					
                    Text("Specular "); SameLine(); SetNextItemWidth(-1);
                    if (BeginCombo("##mesh_specular_combo", DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_id].materialIndex].specularID].filename)) {
                        WinHovCheck;
                        for_n(i, textures.size()) {
                            if (Selectable(textures[i].c_str(), strcmp(DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_id].materialIndex].specularID].filename, textures[i].c_str()) == 0)) {
                                DengRenderer->UpdateMaterialTexture(mvk.primitives[mesh_batch_id].materialIndex, 0,
																	DengRenderer->LoadTexture(textures[i].c_str(), TextureType_Specular));
                            }
                        }
                        EndCombo();
                    }
					
                    Text("Light    "); SameLine(); SetNextItemWidth(-1);
                    if (BeginCombo("##mesh_light_combo", DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_id].materialIndex].lightID].filename)) {
                        WinHovCheck;
                        for_n(i, textures.size()) {
                            if (Selectable(textures[i].c_str(), strcmp(DengRenderer->textures[DengRenderer->materials[mvk.primitives[mesh_batch_id].materialIndex].lightID].filename, textures[i].c_str()) == 0)) {
                                DengRenderer->UpdateMaterialTexture(mvk.primitives[mesh_batch_id].materialIndex, 0,
																	DengRenderer->LoadTexture(textures[i].c_str(), TextureType_Light));
                            }
                        }
                        EndCombo();
                    }
					
                }Unindent();
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
            Text("Velocity     "); SameLine(); InputVector3("##phys_vel", &physics_velocity);
            Text("Accelertaion "); SameLine(); InputVector3("##phys_accel", &physics_accel);
            Text("Rot Velocity "); SameLine(); InputVector3("##phys_rotvel", &physics_rotVel);
            Text("Rot Accel    "); SameLine(); InputVector3("##phys_rotaccel", &physics_rotAccel);
            Text("Elasticity   "); SameLine(); InputFloat("##phys_elastic", &physics_elasticity);
            Text("Mass         "); SameLine(); InputFloat("##phys_mass", &physics_mass);
            Checkbox("Static Position", &physics_staticPosition);
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
            Text("Strength     "); SameLine(); 
            InputFloat("##light_strength", &physics_mass);
            TreePop();
        }
        EndChild();
    }
    PopStyleVar();
}

inline void GlobalTab(EntityAdmin* admin){
    using namespace ImGui;
	
    SetPadding; 
    if(ImGui::BeginChild("##global_tab", ImVec2(ImGui::GetWindowWidth()*0.95f, ImGui::GetWindowHeight()*.9f))) {
        //// physics properties ////
        ImGui::Text("Pause Physics "); ImGui::SameLine();
        if(ImGui::Button((admin->pause_phys) ? "True" : "False", ImVec2(-FLT_MIN, 0))){
            admin->pause_phys = !admin->pause_phys;
        }    
        ImGui::Text("Gravity       "); ImGui::SameLine(); ImGui::InputFloat("##global_gravity", &admin->physics.gravity);

        //ImGui::Text("Phys TPS      "); ImGui::SameLine(); ImGui::InputFloat("##phys_tps", )

        //// camera properties ////
        ImGui::Separator();
        ImGui::Text("Camera"); ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetWindowWidth()*.5f);
        if(ImGui::Button("Zero", ImVec2(ImGui::GetWindowWidth()*.225f, 0))){
            admin->editor.camera->position = Vector3::ZERO; admin->editor.camera->rotation = Vector3::ZERO;
        } ImGui::SameLine();
        if(ImGui::Button("Reset", ImVec2(ImGui::GetWindowWidth()*.25f, 0))){
            admin->editor.camera->position = {4.f,3.f,-4.f}; admin->editor.camera->rotation = {28.f,-45.f,0.f};
        }
		
        ImGui::Text("Position  "); ImGui::SameLine(); ImGui::InputVector3("##cam_pos", &admin->editor.camera->position);
        ImGui::Text("Rotation  "); ImGui::SameLine(); ImGui::InputVector3("##cam_rot", &admin->editor.camera->rotation);
        ImGui::Text("Near Clip "); ImGui::SameLine(); ImGui::InputFloat("##global_nearz", &admin->editor.camera->nearZ);
        ImGui::Text("Far Clip  "); ImGui::SameLine(); ImGui::InputFloat("##global_farz", &admin->editor.camera->farZ);
        ImGui::Text("FOV       "); ImGui::SameLine(); ImGui::InputFloat("##global_fov", &admin->editor.camera->fov);
		
        EndChild();
    }
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

void DisplayTriggers(EntityAdmin* admin) {
    int i = 0;
    for (Entity* e : admin->entities) {
        if (e->type == EntityType_Trigger) {
            Trigger* t = dyncast(Trigger, e);
            switch (t->collider->type) {
                case ColliderType_AABB:{
                    DebugTriggersStatic(i, t->mesh, e->transform.TransformMatrix(), 1);
                }break;
                case ColliderType_Sphere: {
					
                }break;
                case ColliderType_Complex: {
					
                }break;
            }
        }
        i++;
    }
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
    
    ImGui::Begin("DebugTools", (bool*)1, ImGuiWindowFlags_NoFocusOnAppearing |  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    //capture mouse if hovering over this window
    WinHovCheck; 
    if(DengInput->mouseX < GetWindowPos().x + GetWindowWidth()){
        WinHovFlag = true;
    }
    
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
        if (BeginTabItem("Materials")) {
            MaterialsTab(admin);
            EndTabItem();
        }
        if (BeginTabItem("Global")) {
            GlobalTab(admin);
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
        
        //Middle Empty Separator (alert box)
        if (TableNextColumn()) {
            if (DengConsole->show_alert) {
                f32 flicker = (sinf(M_2PI * DengTime->totalTime + cosf(M_2PI * DengTime->totalTime)) + 1)/2;
                Color col_bg = DengConsole->alert_color * flicker;    col_bg.a = 255;
                Color col_text = DengConsole->alert_color * -flicker; col_text.a = 255;
				
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, GetColorU32(ColToVec4(col_bg)));
				
                std::string str6;
                if(DengConsole->alert_count > 1) {
                    str6 = TOSTRING("(",DengConsole->alert_count,") ",DengConsole->alert_message);
                }else{
                    str6 = DengConsole->alert_message;
                }
                float strlen6 = (font_width / 2) * str6.size();
                ImGui::SameLine((GetColumnWidth() - strlen6) / 2); PushItemWidth(-1);
                ImGui::PushStyleColor(ImGuiCol_Text, ColToVec4(Color(col_text)));
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
    int lines = 0;
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


void Editor::WorldGrid(Vector3 cpos) {
    int lines = 20;
    cpos = Vector3::ZERO;
    
    for (int i = 0; i < lines * 2 + 1; i++) {
        Vector3 v1 = Vector3(floor(cpos.x) + -lines + i, 0, floor(cpos.z) + -lines);
        Vector3 v2 = Vector3(floor(cpos.x) + -lines + i, 0, floor(cpos.z) + lines);
        Vector3 v3 = Vector3(floor(cpos.x) + -lines, 0, floor(cpos.z) + -lines + i);
        Vector3 v4 = Vector3(floor(cpos.x) + lines, 0, floor(cpos.z) + -lines + i);
        //
        //DebugLinesStatic(i, v3, v4, -1);
		
        bool l1flag = false;
        bool l2flag = false;
		
        if (floor(cpos.x) - lines + i == 0) {
            l1flag = true;
        }
        if (floor(cpos.z) - lines + i == 0) {
            l2flag = true;
        }
		
        if (l1flag) { DebugLinesCol(i, v1, v2, -1, Color::BLUE); }
        else { DebugLinesCol(i, v1, v2, -1, Color(50, 50, 50, 50)) };
		
        if (l2flag) { DebugLinesCol(i, v3, v4, -1, Color::RED); }
        else { DebugLinesCol(i, v3, v4, -1, Color(50, 50, 50, 50)) };
    }
}

void Editor::ShowSelectedEntityNormals() {
    vec3 p0, p1, p2, normal, intersect;
    mat4 rot, transform;
    f32  t;
    int  index = 0;
    for (Entity* e : selected) {
        if (MeshComp* mc = e->GetComponent<MeshComp>()) {
            if (mc->mesh_visible) {
                Mesh* m = mc->mesh;
                for (Batch& b : m->batchArray) {
                    for (u32 i = 0; i < b.indexArray.size(); i += 3) {
                        transform = e->transform.TransformMatrix();
                        p0 = b.vertexArray[b.indexArray[i + 0]].pos * transform;
                        p1 = b.vertexArray[b.indexArray[i + 1]].pos * transform;
                        p2 = b.vertexArray[b.indexArray[i + 2]].pos * transform;
                        normal = b.vertexArray[b.indexArray[i + 0]].normal * transform;
                        
                        Vector3 perp = normal.cross(p1 - p0).yInvert();
                        Vector3 mid = (p0 + p1 + p2) / 3;
                        
                        DebugLinesCol(i, mid, mid + normal, -1, Color::CYAN);
                        DebugLinesCol(i, p0, p0 + perp, -1, Color::YELLOW);
                        DebugLinesCol(i, mid, p0, -1, Color::MAGENTA);
                    }
                }
            }
        }
        index++;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// editor struct

void Editor::Init(EntityAdmin* a){
    admin = a;
    settings = {};
    
    selected.reserve(8);
    camera = new Camera(90.f, .01f, 1000.01f, true);
    camera->admin = a;
    DengRenderer->UpdateCameraViewMatrix(camera->viewMat);
    DengRenderer->UpdateCameraPosition(camera->position);
    undo_manager.Init();
    level_name = "";
    
    showDebugTools      = true;
    showTimes           = true;
    showDebugBar        = true;
    showMenuBar         = true;
    showImGuiDemoWindow = false;
    showDebugLayer      = true;
    ConsoleHovFlag      = false;
	
    files = deshi::iterateDirectory(deshi::dirModels());
    textures = deshi::iterateDirectory(deshi::dirTextures());
    levels = deshi::iterateDirectory(deshi::dirLevels());
	
    fonth = ImGui::GetFontSize();
    fontw = fonth / 2.f;
}

void Editor::Update(){
    ////////////////////////////
    //// handle user inputs ////
    ////////////////////////////
    
    {//// general ////
        if (DengInput->KeyPressed(Key::P | INPUTMOD_CTRL)) {
            admin->paused = !admin->paused;
        }
    }
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
    }
    {//// camera ////
        //toggle ortho
        static Vector3 ogpos;
        static Vector3 ogrot;
        if (DengInput->KeyPressed(DengKeys.perspectiveToggle)) {
            switch (camera->type) {
                case(CameraType_Perspective): {  
                    ogpos = camera->position;
                    ogrot = camera->rotation;
                    camera->type = CameraType_Orthographic; 
                    camera->farZ = 1000000; 
                } break;
                case(CameraType_Orthographic): { 
                    camera->position = ogpos; 
                    camera->rotation = ogrot;
                    camera->type = CameraType_Perspective; 
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
        if (DengInput->KeyPressed(DengKeys.toggleDebugBar))  showDebugBar = !showDebugBar;
        if (DengInput->KeyPressed(DengKeys.toggleMenuBar))   showMenuBar = !showMenuBar;
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
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1)); {
            if (showImGuiDemoWindow) ImGui::ShowDemoWindow();
        }ImGui::PopStyleColor();
        
        if (!showMenuBar)    menubarheight = 0;
        if (!showDebugBar)   debugbarheight = 0;
        if (!showDebugTools) debugtoolswidth = 0;
        
        Vector3 cpos = camera->position;
        static Vector3 lastcpos = camera->position;
        static bool first = true;
        if ((lastcpos - cpos).mag() > 10000 || first) {
            //TODO(sushi, Op) look into if we can some how load/change things on the GPU in a different thread 
            //std::thread worldgrid(WorldGrid, cpos);
            WorldGrid(cpos);
            //worldgrid.detach();
            lastcpos = camera->position;
            first = false;
        }
		
        DengConsole->IMGUI_MOUSE_CAPTURE = (ConsoleHovFlag || WinHovFlag) ? true : false;
    }
    
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
    
    ////////////////////////////////
    ////  selected entity debug ////
    ////////////////////////////////
    //ShowSelectedEntityNormals();
    DisplayTriggers(admin);
    
}

void Editor::Reset(){
    //camera->position = camera_pos;
    //camera->rotation = camera_rot;
    selected.clear();
    undo_manager.Reset();
    g_debug->meshes.clear();
    WorldGrid(camera->position);
}