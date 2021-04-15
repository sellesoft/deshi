/* //TODO(sushi) rewrite this list eventually
------Tick Order---||--------Components-----||-------------------------------
  olcPixelGameEngine      || Input							|| N/A
  TimeSystem              || Time							 || N/A
  ScreenSystem            || Screen						   || N/A
  CommandSystem           || Input, Keybinds, Canvas		  || N/A
  SimpleMovementSystem    || Camera						   || Input, Keybinds, MovementState, Time
  PhysicsSystem           || Time, Transform, Physics		 || Camera, Screen
  CameraSystem            || Camera						   || Screen
  RenderCanvasSystem      || Canvas						   || Screen
  WorldSystem             || World, Entity					|| N/A
  TriggeredCommandSystem  || N/A							  || N/A
  DebugSystem             || ALL							  || ALL
*/

#include "EntityAdmin.h"
#include "core.h"

#include "utils/PhysicsWorld.h"
#include "utils/Command.h"
#include "utils/defines.h"
#include "scene/Scene.h"

#include "game/Keybinds.h"
#include "game/Transform.h"
#include "game/Controller.h"
#include "game/components/Component.h"
#include "game/components/Collider.h"
#include "game/components/Camera.h"
#include "game/components/AudioListener.h"
#include "game/components/AudioSource.h"
#include "game/components/MeshComp.h"
#include "game/components/Orb.h"
#include "game/components/Light.h"

#include "game/systems/System.h"
#include "game/systems/PhysicsSystem.h"
#include "game/systems/CanvasSystem.h"
#include "game/systems/WorldSystem.h"
#include "game/systems/SoundSystem.h"

#include <iostream>
#include <fstream>

//// EntityAdmin ////

void EntityAdmin::Init(Input* i, Window* w, Time* t, Renderer* r, Console* c) {
	time = t; input = i; window = w; console = c; renderer = r;
	
	state = GameState::EDITOR;
	entities.reserve(1000);
	
	//reserve complayers
	for (int i = 0; i < 8; i++) {
		freeCompLayers.push_back(ContainerManager<Component*>());
	}
	
	//systems initialization
	physicsWorld = new PhysicsWorld();
	switch (physicsWorld->integrationMode) {
		default: /* Semi-Implicit Euler */ {
			physics = new PhysicsSystem();
		}
	}
	canvas = new CanvasSystem();
	world  = new WorldSystem();
	sound  = new SoundSystem();
	
	physics->Init(this);
	canvas->Init(this);
	world->Init(this);
	sound->Init(this);
	
	scene.Init();
	renderer->LoadScene(&scene);
	keybinds.Init();
	controller.Init(this);
	undoManager.Init();
	
	//singleton initialization
	mainCamera = new Camera(this, 90.f, .01f, 1000.01f, true);
	mainCamera->layer_index = freeCompLayers[mainCamera->layer].add(mainCamera);
	
	/*
	//orb testing
	Mesh* mesh = new Mesh(Mesh::CreateMeshFromOBJ("sphere.obj", "sphere.obj"));
	//Texture tex("default1024.png");
	//admin->renderer->LoadTexture(tex);
	//*mesh.batchArray[0].textureArray.push_back(tex);
	//*mesh.batchArray[0].textureCount = 1;
	mesh->batchArray[0].shader = Shader::WIREFRAME;
	OrbManager* om = new OrbManager(mesh, this);
	world->CreateEntity(admin, {om}, "orbtest");
*/
}

void EntityAdmin::Cleanup() {
	//cleanup collections
	entities.clear();
	freeCompLayers.clear();
	
	delete physicsWorld;
	delete physics;
	delete canvas;
	delete world;
	delete sound;
	delete mainCamera;
}

void UpdateLayer(ContainerManager<Component*> cl) {
	for (int i = 0; i < cl.size(); i++) {
		if (cl[i].test()) {
			cl[i].value->Update();
		}
	}
}

void EntityAdmin::Update() {
	if(!skip) controller.Update();
	if(!skip) mainCamera->Update();
	
	TIMER_RESET(t_a); if (!skip && !pause_phys && !paused)  { UpdateLayer(freeCompLayers[CL0_PHYSICS]); }	 time->physLyrTime = TIMER_END(t_a);
	TIMER_RESET(t_a); if (!skip && !pause_phys && !paused)  { physics->Update(); }							 time->physSysTime = TIMER_END(t_a);
	TIMER_RESET(t_a); if (!skip && !pause_canvas)           { UpdateLayer(freeCompLayers[CL1_RENDCANVAS]); } time->canvasLyrTime = TIMER_END(t_a);
	TIMER_RESET(t_a); if (!skip && !pause_canvas)           { canvas->Update(); }						     time->canvasSysTime = TIMER_END(t_a);
	TIMER_RESET(t_a); if (!skip && !pause_console)          { UpdateLayer(freeCompLayers[CL2_WORLD]); }	     time->worldLyrTime = TIMER_END(t_a);
	TIMER_RESET(t_a); if (!skip && !pause_world && !paused) { world->Update(); }						     time->worldSysTime = TIMER_END(t_a);
	TIMER_RESET(t_a); if (!skip && !pause_sound && !paused) { UpdateLayer(freeCompLayers[CL3_SOUND]); }	     time->sndLyrTime = TIMER_END(t_a);
	TIMER_RESET(t_a); if (!skip && !pause_sound && !paused) { sound->Update(); }						     time->sndSysTime = TIMER_END(t_a);
	TIMER_RESET(t_a); if (!skip && !pause_last && !paused)  { UpdateLayer(freeCompLayers[CL4_LAST]); }	     time->lastLyrTime = TIMER_END(t_a);

	time->paused = paused;
	time->phys_pause = pause_phys;
	
}

struct SaveHeader{
	u32 magic;
	u32 flags;
	//u32 cameraOffset;
	u32 entityCount;
	u32 entityArrayOffset;
	u32 meshCount;
	u32 meshArrayOffset;
	u32 componentTypeCount;
	u32 componentTypeHeaderArrayOffset;
};

struct ComponentTypeHeader{
	u32 type;
	u32 size;
	u32 count;
	u32 arrayOffset;
};

typedef enum ComponentTypeBits : u32{
	ComponentType_NONE = 0,
	ComponentType_AudioListener, ComponentType_AudioSource,    ComponentType_Camera,     ComponentType_ColliderBox,
	ComponentType_ColliderAABB,  ComponentType_ColliderSphere, ComponentType_Controller, ComponentType_Light, 
	ComponentType_MeshComp,      ComponentType_OrbManager,     ComponentType_Physics, 
	ComponentType_LAST = 0xFFFFFFFF,
} ComponentTypeBits;

void EntityAdmin::Save() {
	//assertions so things stay up to date; last updated: 4/14/2021 by delle //TODO(delle) move these to component files
	ASSERT(32 == sizeof(SaveHeader), "SaveHeader size is out of date");
	ASSERT(16 == sizeof(ComponentTypeHeader), "ComponentTypeHeader is out of date");
	ASSERT(160 == sizeof(Entity), "Entity is out of date");
	ASSERT(176 == sizeof(MeshVk), "MeshVk is out of date");
	ASSERT(152 == sizeof(AudioListener), "AudioListener is out of date");
	ASSERT(160 == sizeof(AudioSource), "AudioSource is out of date");
	ASSERT(336 == sizeof(Camera), "Camera is out of date");
	ASSERT(176 == sizeof(BoxCollider), "BoxCollider is out of date");
	ASSERT(176 == sizeof(AABBCollider), "AABBCollider is out of date");
	ASSERT(168 == sizeof(SphereCollider), "SphereCollider is out of date");
	ASSERT(144 == sizeof(Light), "Light is out of date");
	ASSERT(136 == sizeof(MeshComp), "MeshComp is out of date");
	ASSERT(232 == sizeof(Physics), "Physics is out of date");
	
	//open file
	std::string filepath = deshi::dirData() + "save.desh";
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
	if(!file.is_open()){ ERROR("Failed to open file '", filepath, "' when trying to save"); return; }
	
	SaveHeader header{};
	file.write((const char*)&header, sizeof(SaveHeader)); //zero fill header
	header.magic                          = 1213416772; //DESH
	header.flags                          = 0;
	//header.cameraOffset                   = 0;
	
	//// entities ////
	header.entityCount                    = entities.size();
	header.entityArrayOffset              = sizeof(SaveHeader);
	
	//store sorted components and write entities
	header.componentTypeCount = 8;
	std::vector<AudioListener*>  compsAudioListener;
	std::vector<AudioSource*>    compsAudioSource;
	std::vector<BoxCollider*>    compsColliderBox;
	std::vector<AABBCollider*>   compsColliderAABB;
	std::vector<SphereCollider*> compsColliderSphere;
	std::vector<Light*>          compsLight;
	std::vector<MeshComp*>       compsMeshComp;
	std::vector<Physics*>        compsPhysics;
	
	for(auto& e : entities) {
		//write entity
		file.write((const char*)&e.id,                 sizeof(u32));
		file.write(e.name,                             64);
		file.write((const char*)&e.transform.position, sizeof(Vector3));
		file.write((const char*)&e.transform.rotation, sizeof(Vector3));
		file.write((const char*)&e.transform.scale,    sizeof(Vector3));
		
		//sort components
		for(auto c : e.components) {
			if(dyncast(d, MeshComp, c)) {
				compsMeshComp.push_back(d);
			}else if(dyncast(d, Physics, c)) {
				compsPhysics.push_back(d);
			}else if(dyncast(col, Collider, c)){
				if (dyncast(d, BoxCollider, col)){
					compsColliderBox.push_back(d);
				}else if(dyncast(d, AABBCollider, col)){
					compsColliderAABB.push_back(d);
				}else if(dyncast(d, SphereCollider, col)){
					compsColliderSphere.push_back(d);
				}else{
					ERROR("Unhandled collider component '", c->name, "' found when attempting to save");
				}
			}else if(dyncast(d, AudioListener, c)){
				compsAudioListener.push_back(d);
			}else if(dyncast(d, AudioSource, c)){
				compsAudioSource.push_back(d);
			}else{
				ERROR("Unhandled component '", c->name, "' found when attempting to save");
			}
		}
	}
	
	//// write meshes ////
	header.meshCount = renderer->meshes.size();
	header.meshArrayOffset = file.tellp();
	
	for(auto& m : renderer->meshes){
		b32 base = m.base;
		std::string temp = m.name; temp.resize(64);
		file.write((const char*)&m.id, sizeof(u32));
		file.write((const char*)&base, sizeof(b32));
		file.write(temp.c_str(),       64); //NOTE(delle) using the mesh name doesnt work on obj loaded meshes?
	}
	
	//// write component type headers //// //TODO(delle) move these to thier respective files
	header.componentTypeHeaderArrayOffset = file.tellp();
	ComponentTypeHeader typeHeader;
	
	//audio listener
	typeHeader.type        = ComponentType_AudioListener;
	typeHeader.arrayOffset = header.componentTypeHeaderArrayOffset + sizeof(ComponentTypeHeader) * header.componentTypeCount;
	typeHeader.size        = sizeof(u32) + sizeof(Vector3)*3;
	typeHeader.count       = compsAudioListener.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//audio source
	typeHeader.type        = ComponentType_AudioSource;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) + 0; //TODO(sushi) tell delle what data is important to save on a source
	typeHeader.count       = compsAudioSource.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//TODO(delle) update when colliders have triggers
	//collider box
	typeHeader.type        = ComponentType_ColliderBox;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) + sizeof(Matrix3) + sizeof(i8) + sizeof(Vector3);
	typeHeader.count       = compsColliderBox.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//collider aabb
	typeHeader.type        = ComponentType_ColliderAABB;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) + sizeof(Matrix3) + sizeof(i8) + sizeof(Vector3);
	typeHeader.count       = compsColliderAABB.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//collider sphere
	typeHeader.type        = ComponentType_ColliderSphere;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) + sizeof(Matrix3) + sizeof(i8) + sizeof(float);
	typeHeader.count       = compsColliderSphere.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//light
	typeHeader.type        = ComponentType_Light;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) + sizeof(Vector3)*2 + sizeof(float);
	typeHeader.count       = compsLight.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//mesh comp
	typeHeader.type        = ComponentType_MeshComp;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) + sizeof(u16)*2 + sizeof(b32)*2; //instanceID, meshID, visible, entity_control
	typeHeader.count       = compsMeshComp.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//physics
	typeHeader.type        = ComponentType_Physics;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) + sizeof(Vector3)*6 + sizeof(float)*2 + sizeof(b32);
	typeHeader.count       = compsPhysics.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//// write components ////
	
	//audio listener
	for(auto c : compsAudioListener){
		file.write((const char*)&c->entity->id,  sizeof(u32));
		file.write((const char*)&c->position,    sizeof(Vector3));
		file.write((const char*)&c->velocity,    sizeof(Vector3));
		file.write((const char*)&c->orientation, sizeof(Vector3));
	}
	
	//audio source
	for(auto c : compsAudioSource){
		file.write((const char*)&c->entity->id, sizeof(u32));
	}
	
	//collider box
	for(auto c : compsColliderBox){
		file.write((const char*)&c->entity->id,     sizeof(u32));
		file.write((const char*)&c->inertiaTensor,  sizeof(Matrix3));
		file.write((const char*)&c->collisionLayer, sizeof(i8));
		file.write((const char*)&c->halfDims,       sizeof(Vector3));
	}
	
	//collider aabb
	for(auto c : compsColliderAABB){
		file.write((const char*)&c->entity->id,     sizeof(u32));
		file.write((const char*)&c->inertiaTensor,  sizeof(Matrix3));
		file.write((const char*)&c->collisionLayer, sizeof(i8));
		file.write((const char*)&c->halfDims,       sizeof(Vector3));
	}
	
	//collider sphere
	for(auto c : compsColliderSphere){
		file.write((const char*)&c->entity->id,     sizeof(u32));
		file.write((const char*)&c->inertiaTensor,  sizeof(Matrix3));
		file.write((const char*)&c->collisionLayer, sizeof(i8));
		file.write((const char*)&c->radius,         sizeof(float));
	}
	
	//light
	for(auto c : compsLight){
		file.write((const char*)&c->entity->id, sizeof(u32));
		file.write((const char*)&c->position,   sizeof(Vector3));
		file.write((const char*)&c->direction,  sizeof(Vector3));
		file.write((const char*)&c->strength,   sizeof(float));
	}
	
	//mesh comp
	for(auto c : compsMeshComp){
		b32 bool1 = c->mesh_visible;
		b32 bool2 = c->ENTITY_CONTROL;
		file.write((const char*)&c->entity->id,     sizeof(u32));
		file.write((const char*)&c->InstanceID,     sizeof(u16));
		file.write((const char*)&c->MeshID,         sizeof(u16));
		file.write((const char*)&bool1,             sizeof(b32));
		file.write((const char*)&bool2,             sizeof(b32));
	}
	
	//physics
	for(auto c : compsPhysics){
		b32 isStatic = c->isStatic;
		file.write((const char*)&c->entity->id,      sizeof(u32));
		file.write((const char*)&c->position,        sizeof(Vector3));
		file.write((const char*)&c->rotation,        sizeof(Vector3));
		file.write((const char*)&c->velocity,        sizeof(Vector3));
		file.write((const char*)&c->acceleration,    sizeof(Vector3));
		file.write((const char*)&c->rotVelocity,     sizeof(Vector3));
		file.write((const char*)&c->rotAcceleration, sizeof(Vector3));
		file.write((const char*)&c->elasticity,      sizeof(float));
		file.write((const char*)&c->mass,            sizeof(float));
		file.write((const char*)&isStatic,           sizeof(b32));
	}
	
	//store camera's size so we know offset to following entities list then store camera
	//int camsize = sizeof(*mainCamera);
	//deshi::appendFileBinary(file, (const char*)&camsize, sizeof(int));
	//deshi::appendFileBinary(file, (const char*)mainCamera, camsize);
	
	
	//finish header
	file.seekp(0);
	file.write((const char*)&header, sizeof(SaveHeader));
	
	//// close file ////
	file.close();
}

void EntityAdmin::Load(const char* filename) {
	//// clear current stuff ////
	entities.clear(); entities.reserve(1000);
	for (auto& layer : freeCompLayers) { layer.clear(); } //TODO(delle) see if this causes a memory leak
	
	input->selectedEntity = 0;
	undoManager.Reset();
	scene.Reset();
	renderer->Reset();
	renderer->LoadScene(&scene);
	
	SUCCESS("Cleaned up previous level");
	SUCCESS("Loading level: ", filename);
	
	//// read file to char array ////
	u32 cursor = 0;
	std::vector<char> file = deshi::readFileBinary(filename);
	const char* data = file.data();
	if(!data) return;
	
	//check for magic number
	u32 magic = 1213416772; //DESH
	if(memcmp(data, &magic, 4) != 0) return ERROR("Invalid magic number when loading save file: ", filename);
	cursor += 4;
	
	//parse header
	u32 flags, entityCount, entityArrayOffset, meshCount, meshArrayOffset, compTypeCount, compTypeArrayOffset;
	memcpy(&flags,               data+cursor, sizeof(u32)); cursor += sizeof(u32);
	memcpy(&entityCount,         data+cursor, sizeof(u32)); cursor += sizeof(u32);
	memcpy(&entityArrayOffset,   data+cursor, sizeof(u32)); cursor += sizeof(u32);
	memcpy(&meshCount,           data+cursor, sizeof(u32)); cursor += sizeof(u32);
	memcpy(&meshArrayOffset,     data+cursor, sizeof(u32)); cursor += sizeof(u32);
	memcpy(&compTypeCount,       data+cursor, sizeof(u32)); cursor += sizeof(u32);
	memcpy(&compTypeArrayOffset, data+cursor, sizeof(u32)); cursor += sizeof(u32);
	
	//parse and create entities
	Entity tempEntity;
	for_n(i,entityCount){
		tempEntity.admin = this;
		memcpy(&tempEntity.id, data+cursor, sizeof(u32) + 64 + sizeof(Vector3)*3);
		cursor += sizeof(u32) + 64 + sizeof(Vector3)*3;
	}
	
	//parse and load/create meshes
	u32 id; char meshName[64];
	for_n(i,meshCount){
		
		renderer->CreateMesh(&scene, meshName);
	}
	
	//skip any ongoing updates
	skip = true;
}

Command* EntityAdmin::GetCommand(std::string command) {
	try {
		return console->commands.at(command);
	} catch (std::exception e) {
		//ERROR("Command \"", command, "\" does not exist");
		this->console->PushConsole(TOSTRING("\n[c:red]", "Command \"", command, "\" does not exist", "[c]"));
		return 0;
	}
}

bool EntityAdmin::ExecCommand(std::string command) {
	try {
		console->commands.at(command)->Exec(this);
		return true;
	} catch (std::exception e) {
		//ERROR("Command \"", command, "\" does not exist");
		this->console->PushConsole(TOSTRING("\n[c:red]", "Command \"", command, "\" does not exist", "[c]"));
		return false;
	}
}

bool EntityAdmin::ExecCommand(std::string command, std::string args) {
	try {
		console->commands.at(command)->Exec(admin, args);
		return true;
	} catch (std::exception e) {
		this->console->PushConsole(TOSTRING("\n[c:red]", "Command \"", command, "\" does not exist", "[c]"));
		return false;
	}
}

//// Entity ////

Entity::Entity(){
	this->transform = Transform();
	this->name[63] = '\0';
}

Entity::Entity(EntityAdmin* admin, u32 id, Transform transform, const char* name, std::vector<Component*> components){
	this->admin = admin;
	this->id = id;
	this->transform = transform;
	if(name){ strncpy_s(this->name, name, 63); } this->name[63] = '\0';
	for (Component* c : components) this->components.push_back(c);
}

Entity::~Entity() {
	for (Component* c : components) delete c;
}

std::string Entity::Save() {
	
	return "";
}

void Entity::SetName(const char* name){
	if(name) strncpy_s(this->name, name, 63);
	this->name[63] = '\0';
}

void Entity::AddComponent(Component* component) {
	components.push_back(component);
	component->layer_index = admin->freeCompLayers[component->layer].add(component);
	component->entity = this;
	component->admin = this->admin;
}

void Entity::AddComponents(std::vector<Component*> comps) {
	u32 value = this->components.size();
	for (Component* c : comps) {
		this->components.push_back(c);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
		c->entity = this;
		c->admin = this->admin;
	}
}

void Entity::RemoveComponent(Component* c) {
	for_n(i,components.size()){
		if(components[i] == c){
			admin->freeCompLayers[c->layer].remove_from(c->layer_index);
			delete c; 
			components.erase(components.begin()+i); 
			return;
		}
	}
}

void Entity::RemoveComponents(std::vector<Component*> comps) {
	while(comps.size()){
		for_n(i,components.size()){
			if(components[i] == comps[0]){ 
				delete comps[i]; 
				components.erase(components.begin()+i); 
				comps.erase(components.begin()); 
				break;
			}
		}
	}
}
