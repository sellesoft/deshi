#include "Entity.h"
#include "../admin.h"
#include "../components/Component.h"
#include "../components/Orb.h"
#include "../components/door.h"
#include "../components/Light.h"
#include "../components/Camera.h"
#include "../components/Player.h"
#include "../components/Physics.h"
#include "../components/MeshComp.h"
#include "../components/Collider.h"
#include "../components/Movement.h"
#include "../components/Component.h"
#include "../components/AudioSource.h"
#include "../components/AudioListener.h"
#include "../../core/assets.h"
#include "../../core/renderer.h"
#include "../../core/time.h"
#include "../../utils/tuple.h"

#include <iostream>
#include <fstream>
#include <string_view>

Entity::Entity() {
    this->admin = 0;
    this->id = -1;
    this->transform = Transform();
    this->name[63] = '\0';
}

Entity::Entity(Admin* admin, u32 id, Transform transform, const char* name, std::vector<Component*> comps) {
    this->admin = admin;
    this->id = id;
    this->transform = transform;
    if (name) cpystr(this->name, name, DESHI_NAME_SIZE);
    for (Component* c : comps) {
        if (!c) continue;
        this->components.push_back(c);
        c->entityID = id;
        c->admin = admin;
    }
}

Entity::~Entity() {
    for (Component* c : components) if(c) delete c;
}

void Entity::operator=(Entity& e) {
    admin = e.admin;
    id = e.id;
    cpystr(name, e.name, DESHI_NAME_SIZE);
    transform = e.transform;
    //std::copy(e.components.begin(), e.components.end(), components);
    for (Component* c : e.components) {
        if (!c) continue;
        this->components.push_back(c);
    }
}

void Entity::Init(){
    forI(components.size()) components[i]->Init();
}

inline void Entity::SetName(const char* name) {
    if (name) cpystr(this->name, name, DESHI_NAME_SIZE);
}

void Entity::AddComponent(Component* c) {
    if (!c) return;
    components.push_back(c);
    c->entityID = id;
    c->entity = this;
    c->admin = this->admin;
    if (c->comptype == ComponentType_Light) {
        DengAdmin->scene.lights.push_back(dyncast(Light, c));
    }
}

void Entity::AddComponents(std::vector<Component*> comps) {
    u32 value = this->components.size();
    for (Component* c : comps) {
        if (!c) continue;
        this->components.push_back(c);
        c->entityID = id;
        c->admin = this->admin;
        c->entity = this;
    }
}

void Entity::RemoveComponent(Component* c) {
    if (!c) return;
    forI(components.size()) {
        if (components[i] == c) {
            admin->freeCompLayers[c->layer].remove_from(c->layer_index);
            delete c;
            components.erase(components.begin() + i);
            return;
        }
    }
}

void Entity::RemoveComponents(std::vector<Component*> comps) {
    while (comps.size()) {
        forI(components.size()) {
            if (!comps[i]) continue;
            if (components[i] == comps[0]) {
                admin->freeCompLayers[comps[i]->layer].remove_from(comps[i]->layer_index);
                delete comps[i];
                components.erase(components.begin() + i);
                comps.erase(components.begin());
                break;
            }
        }
    }
}

////////////////////////////
//// saving and loading ////
////////////////////////////

std::string Entity::SaveTEXT(){
    std::string result; result.reserve(2048);
    result.append(TOSTRING(">entity",
                           "\nid       ", id,
                           "\ntype     ", EntityTypeStrings[type],
                           "\nname     \"",name,"\"",
                           "\nposition (",transform.position.x,",",transform.position.y,",",transform.position.z,")",
                           "\nrotation (",transform.rotation.x,",",transform.rotation.y,",",transform.rotation.z,")",
                           "\nscale    (",transform.scale.x,",",transform.scale.y,",",transform.scale.z,")"
                           "\n"));

    //sort components by ComponentType for consistent saving order
    std::vector<Component*> sorted_components = components;
    Component* temp;
    for(int i = 1; i < sorted_components.size(); ++i){
        if(sorted_components[i]->comptype < sorted_components[i-1]->comptype){
            temp = sorted_components[i];
            sorted_components[i] = sorted_components[i-1];
            sorted_components[i-1] = temp;
        }
    }

    for(Component* c : sorted_components) result.append(c->SaveTEXT());
    result.shrink_to_fit();
    return result;
}

auto get_vec3(std::string& str) {
    size_t sz, off = 0;
    try {
        f32 x = std::stof(str, &sz);             off += sz + 1;
        f32 y = std::stof(str.substr(off), &sz); off += sz + 1;
        f32 z = std::stof(str.substr(off), &sz);
        return vec3(x, y, z);
    }catch (...) {
        ERROR("Failed to parse vector3: (", str, ")");
        return Vector3::ZERO;
    }
}

enum struct Header{
    INVALID, ENTITY, AUDIO_LISTENER, AUDIO_SOURCE, CAMERA, COLLIDER, DOOR, LIGHT, MESH, MOVEMENT, ORB_MANAGER, PHYSICS, PLAYER
};
#define InvalidHeaderKeyError(header) ERROR("Error parsing '",filepath,"' on line '",line_number,"'! Invalid key '",kv.first,"' for header '"header"'.")
//TODO(delle) support multiple components of a type on an entity
Entity* Entity::LoadTEXT(Admin* admin, std::string& filepath, std::vector<pair<u32,u32>>& mesh_id_diffs){
    //load file into char array
    char* buffer = deshi::readFileAsciiToArray(filepath);
    if(!buffer) return 0;
    defer{ delete[] buffer; };
    
    //creation vars
    b32 coll_made = false;
    b32 mesh_found = false;
    Entity* e = 0;
    AudioListener* al = 0; AudioSource* as = 0; Camera* cam = 0;    Door* door = 0;
    Light* light = 0;      MeshComp* mesh = 0;  Movement* move = 0; OrbManager* orbman = 0;
    Physics* phys = 0;     Player* player = 0;
    BoxCollider* box = 0;   SphereCollider* sphere = 0;   LandscapeCollider* land = 0; 
    AABBCollider* aabb = 0; ComplexCollider* complex = 0;
    
    //parse file
    Header header = Header::INVALID;
    std::string line;
    char* new_line = buffer-1;
    char* line_start;
    for(u32 line_number = 1; ;line_number++){
        //get the next line
        line_start = new_line+1;
        if((new_line = strchr(line_start, '\n')) == 0) break; //end of file if cant find '\n'
        line = std::string(line_start, new_line-line_start);
        
        //cleanup the line
        //NOTE could maybe check for empty after each of these for more performance or
        //only running these if conditions are met, but this is easier to read/work with for now
        line = deshi::eat_comments(line);
        line = deshi::eat_spaces_leading(line);
        line = deshi::eat_spaces_trailing(line);
        if(line.empty()) continue;
        
        //headers
        if(line[0] == '>'){
            if(!e){
                if(line == ">entity"){
                    header = Header::ENTITY;
                    e = new Entity;
                }else{
                    ERROR("Error parsing '",filepath,"' on line '",line_number,"'! The entity header must be the first header.");
                    return 0;
                }
            }else{
                if     (line == ">mesh")          { header = Header::MESH;           if(!mesh)  { mesh = new MeshComp(); } }
                else if(line == ">physics")       { header = Header::PHYSICS;
                    if(!phys) phys = new Physics(e->transform.position, e->transform.rotation);
                }
                else if(line == ">collider")      { header = Header::COLLIDER;        }
                else if(line == ">audio source")  { header = Header::AUDIO_SOURCE;   if(!as)    { as = new AudioSource(); } }
                else if(line == ">light")         { header = Header::LIGHT;          if(!light) { light = new Light(); } }
                else if(line == ">door")          { header = Header::DOOR;           if(!door)  { door = new Door(); } }
                else if(line == ">camera")        { header = Header::CAMERA;         if(!cam)   { cam = new Camera(); } }
                else if(line == ">orb manager")   { header = Header::ORB_MANAGER;    if(!orbman){ orbman = new OrbManager(); } }
                else if(line == ">audio listener"){ header = Header::AUDIO_LISTENER; if(!al)    { al = new AudioListener(); } }
                else if(line == ">movement")      { header = Header::MOVEMENT;       if(!move)  { move = new Movement(); } }
                else if(line == ">player")        { header = Header::PLAYER;         if(!player){ player = new Player(); } }
                else if(line == ">entity"){
                    header = Header::INVALID;
                    ERROR("Error parsing '",filepath,"' on line '",line_number,"'! There can only be one entity header.");
                }else{
                    header = Header::INVALID;
                    ERROR("Error parsing '",filepath,"' on line '",line_number,"'! Unknown header: ", line);
                }
            }
            continue;
        }
        
        //key-value pairs
        if(header == Header::INVALID) {
            ERROR("Error parsing '",filepath,"' on line '",line_number,"'! Invalid header; skipping line");
            continue; //skip if an invalid header
        }
        pair<std::string,std::string> kv = deshi::split_keyValue(line);
        if(kv.second == ""){
            ERROR("Error parsing '",filepath,"' on line '",line_number,"'! Unable to extract key-value pair from: ", line);
            continue;
        }
        
        switch(header){
            case(Header::ENTITY):{
                if     (kv.first == "name")    { e->SetName(kv.second.c_str()); }
                else if(kv.first == "id")      { e->id = std::stoi(kv.second); }
                else if(kv.first == "type")    { for (int i = 0; i < NUMENTITYTYPES; i++) if (kv.second == EntityTypeStrings[i]) { e->type = (EntityType)i; break; } }
                else if(kv.first == "position"){ e->transform.position = get_vec3(kv.second); }
                else if(kv.first == "rotation"){ e->transform.rotation = get_vec3(kv.second); }
                else if(kv.first == "scale")   { e->transform.scale = get_vec3(kv.second); }
                else{ InvalidHeaderKeyError("entity"); }
            }break;
            case(Header::AUDIO_LISTENER):{
                if     (kv.first == "position")   { al->position = get_vec3(kv.second); }
                else if(kv.first == "velocity")   { al->velocity = get_vec3(kv.second); }
                else if(kv.first == "orientation"){ al->orientation = get_vec3(kv.second); }
                else{ InvalidHeaderKeyError("audio listener"); }
            }break;
            case(Header::AUDIO_SOURCE):{
                ERROR_LOC("Audio Source loading not setup");
            }break;
            case(Header::CAMERA):{
                if     (kv.first == "position"){ cam->position = get_vec3(kv.second); }
                else if(kv.first == "rotation"){ cam->rotation = get_vec3(kv.second); }
                else if(kv.first == "type")    { cam->type = std::stoi(kv.second); }
                else if(kv.first == "near_z")  { cam->nearZ = std::stof(kv.second); }
                else if(kv.first == "far_z")   { cam->farZ = std::stof(kv.second); }
                else if(kv.first == "fov")     { cam->fov = std::stof(kv.second); }
                else{ InvalidHeaderKeyError("camera"); }
            }break;
            case(Header::COLLIDER):{
                if(kv.first == "type" && !coll_made){ 
                    if(kv.second == "aabb" || kv.second == "2"){
                        aabb = new AABBCollider(Vector3::ZERO, 1.f);
                        coll_made = true;
                    }else if(kv.second == "box" || kv.second == "1"){
                        box = new BoxCollider(Vector3::ZERO, 1.f);
                        coll_made = true;
                    }else if(kv.second == "sphere" || kv.second == "3"){
                        sphere = new SphereCollider(1.f, 1.f);
                        coll_made = true;
                    }else if(kv.second == "landscape" || kv.second == "4"){
                        ERROR_LOC("Landscape Collider loading not setup");
                        coll_made = true;
                    }else if(kv.second == "complex" || kv.second == "5"){
                        ERROR_LOC("Complex Collider loading not setup");
                        coll_made = true;
                    }else{
                        ERROR("Error parsing '",filepath,"' on line '",line_number,"'! Invalid collider type: ", kv.second);
                    }
                }
                else if(kv.first == "half_dims" && coll_made){ 
                    if(aabb){
                        aabb->halfDims = get_vec3(kv.second); 
                    }else if(box){
                        box->halfDims = get_vec3(kv.second); 
                    }
                }
                else if(kv.first == "radius" && coll_made) { 
                    if(sphere){
                        sphere->radius = std::stof(kv.second);
                    }
                }
                else{ InvalidHeaderKeyError("collider"); }
            }break;
            case(Header::DOOR):{
                if(kv.first == "open"){ 
                    door->isOpen = deshi::parse_bool(kv.second, filepath.c_str(), line_number);
                }
                else{ InvalidHeaderKeyError("door"); }
            }break;
            case(Header::LIGHT):{
                if     (kv.first == "position")  { light->position = get_vec3(kv.second); }
                else if(kv.first == "direction") { light->direction = get_vec3(kv.second); }
                else if(kv.first == "brightness"){ light->brightness = std::stof(kv.second); }
                else if(kv.first == "active")    { light->active = deshi::parse_bool(kv.second); }
                else{ InvalidHeaderKeyError("light"); }
            }break;
            case(Header::MESH):{
                if(kv.first == "id"){
                    for(auto& diff : mesh_id_diffs){
                        if(diff.first == std::stoi(kv.second)){
                            mesh->meshID = diff.second;
                            mesh->mesh = DengRenderer->GetMeshPtr(mesh->meshID);
                            mesh_found = true;
                        }
                    }
                }
                else if(kv.first == "name"){
                    if(!mesh_found){ //NOTE only make if ID failed to make it
                        mesh->meshID = DengRenderer->CreateMesh(&admin->scene, kv.second.c_str());
                        mesh->mesh = DengRenderer->GetMeshPtr(mesh->meshID);
                    }
                }
                else if(kv.first == "visible"){ 
                    mesh->mesh_visible = deshi::parse_bool(kv.second, filepath.c_str(), line_number);
                }
                else{ InvalidHeaderKeyError("mesh"); }
            }break;
            case(Header::MOVEMENT):{
                if     (kv.first == "ground_accel"){ move->gndAccel = std::stof(kv.second); }
                else if(kv.first == "air_accel")   { move->airAccel = std::stof(kv.second); }
                else if(kv.first == "jump_impulse"){ move->jumpImpulse = std::stof(kv.second); }
                else if(kv.first == "walk_speed")  { move->maxWalkingSpeed = std::stof(kv.second); }
                else if(kv.first == "run_speed")   { move->maxRunningSpeed = std::stof(kv.second); }
                else if(kv.first == "crouch_speed"){ move->maxCrouchingSpeed = std::stof(kv.second); }
                else{ InvalidHeaderKeyError("movement"); }
            }break;
            case(Header::ORB_MANAGER):{
                ERROR_LOC("Orb Manager loading not setup");
            }break;
            case(Header::PHYSICS):{
                if     (kv.first == "velocity")        { phys->velocity = get_vec3(kv.second); }
                else if(kv.first == "acceleration")    { phys->acceleration = get_vec3(kv.second); }
                else if(kv.first == "rot_velocity")    { phys->rotVelocity = get_vec3(kv.second); }
                else if(kv.first == "rot_acceleration"){ phys->rotAcceleration = get_vec3(kv.second); }
                else if(kv.first == "elasticity")      { phys->elasticity = std::stof(kv.second); }
                else if(kv.first == "mass")            { phys->mass = std::stof(kv.second); }
                else if(kv.first == "friction_kinetic"){ phys->kineticFricCoef = std::stof(kv.second); }
                else if(kv.first == "friction_static") { phys->staticFricCoef = std::stof(kv.second); }
                else if(kv.first == "static_position"){ 
                    phys->isStatic = deshi::parse_bool(kv.second, filepath.c_str(), line_number);
                }
                else if(kv.first == "static_rotation"){ 
                    phys->staticRotation = deshi::parse_bool(kv.second, filepath.c_str(), line_number);
                }
                else if(kv.first == "twod"){ 
                    phys->twoDphys = deshi::parse_bool(kv.second, filepath.c_str(), line_number);
                }
                else{ InvalidHeaderKeyError("physics"); }
            }break;
            case(Header::PLAYER):{
                if(kv.first == "health"){ player->health = std::stoi(kv.second); }
                else{ InvalidHeaderKeyError("player"); }
            }break;
        }
    }
    
    //update dependent components
    if(e && phys){
        if     (box)     box->RecalculateTensor(phys->mass);
        else if(aabb)    aabb->RecalculateTensor(phys->mass);
        else if(sphere)  sphere->RecalculateTensor(phys->mass);
        else if(land)    land->RecalculateTensor(phys->mass);
        else if(complex) complex->RecalculateTensor(phys->mass);
        if(move) {
            move->phys = phys;
            if(player) {
                player->movement = move;
                admin->player = e;
                admin->controller.playermove = move;
                move->camera = admin->mainCamera;
            }
        }
    }
    if(e) e->AddComponents({al, as, cam, door, light, mesh, move, orbman, phys, player, box, aabb, sphere, land, complex});
    return e;
}
#undef InvalidHeaderKeyError