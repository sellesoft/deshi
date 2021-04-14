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
#include "game/components/Component.h"
#include "game/components/Collider.h"
#include "game/components/Camera.h"
#include "game/components/Controller.h"
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
	time = t;
	input = i;
	window = w;
	console = c;
	renderer = r;
	
	systems = std::vector<System*>();
	entities = std::map<EntityID, Entity*>();
	components = std::vector<Component*>();
	physicsWorld = new PhysicsWorld();
	
	//reserve complayers
	for (int i = 0; i < 8; i++) {
		freeCompLayers.push_back(ContainerManager<Component*>());
	}
	
	//systems initialization
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
	
	undoManager.Init();
	
	//singleton initialization
	scene = new Scene();
	mainCamera = new Camera(this, 90.f, .01f, 1000.01f, true);
	mainCamera->layer_index = freeCompLayers[mainCamera->layer].add(mainCamera);
	keybinds = new Keybinds(this);
	controller = new Controller(this);
	controller->layer_index = freeCompLayers[controller->layer].add(controller);
	
	state = GameState::EDITOR;
	
	/*
	//orb testing
	Entity* orbtest = world->CreateEntity(admin);
	orbtest->name = "orbtest";
	orbtest->admin = this;
	Mesh mesh = Mesh::CreateMeshFromOBJ("box.obj", "sphere");
	Texture tex("default1024.png");
	admin->renderer->LoadTexture(tex);
	mesh.batchArray[0].textureArray.push_back(tex);
	mesh.batchArray[0].textureCount = 1;
	mesh.batchArray[0].shader = Shader::PBR;
	Mesh* m = new Mesh(mesh);
	OrbManager* om = new OrbManager(m, this, orbtest);
	admin->world->AddAComponentToEntity(admin, orbtest, om);
	*/
}

void EntityAdmin::Cleanup() {
	//cleanup collections
	//for(System* s : systems)       { delete s; }           systems.clear();
	for (auto pair : entities) { delete pair.second; } entities.clear();
	for (Component* c : components) { delete c; }           components.clear();
	
	delete physicsWorld;
	
	//clean up singletons
	delete world;
	delete canvas;
	delete sound;
	delete physics;
	delete mainCamera;
	delete keybinds;
	delete controller;
}

void UpdateLayer(ContainerManager<Component*> cl) {
	for (int i = 0; i < cl.size(); i++) {
		if (cl[i].test()) {
			cl[i].value->Update();
		}
	}
}

void EntityAdmin::Update() {
	controller->Update();
	mainCamera->Update();
	
	if (!pause_phys && !paused)    UpdateLayer(freeCompLayers[CL0_PHYSICS]);
	if (!pause_phys && !paused)    physics->Update();
	if (!pause_canvas)             UpdateLayer(freeCompLayers[CL1_RENDCANVAS]);
	if (!pause_canvas)             canvas->Update();
	if (!pause_console)            UpdateLayer(freeCompLayers[CL2_WORLD]);
	if (!pause_world && !paused)   world->Update();
	if (!pause_sound && !paused)   UpdateLayer(freeCompLayers[CL3_SOUND]);
	if (!pause_sound && !paused)   sound->Update();
	if (!pause_last && !paused)    UpdateLayer(freeCompLayers[CL4_LAST]);
	
	for (Component* c : components) {
		c->Update();
	}
}

struct SaveHeader{
	u32 magic;
	u32 flags;
	//u32 cameraOffset;
	u32 entityCount;
	u32 entityArrayOffset;
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
	ComponentType_NONE,
	ComponentType_AudioListener, ComponentType_AudioSource,    ComponentType_Camera,     ComponentType_ColliderBox,
	ComponentType_ColliderAABB,  ComponentType_ColliderSphere, ComponentType_Controller, ComponentType_Light, 
	ComponentType_MeshComp,      ComponentType_OrbManager,     ComponentType_Physics, 
	ComponentType_LAST,
} ComponentTypeBits; typedef u32 ComponentType;

void EntityAdmin::Save() {
	//open file
	std::string filepath = deshi::dirData() + "test.desh";
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
	if(!file.is_open()){ ERROR("Failed to open file '", filepath, "' when trying to save"); return; }
	
	SaveHeader header;
	file.write((const char*)&header, sizeof(SaveHeader)); //zero fill header
	header.magic                          = 1213416772; //DESH
	header.flags                          = 0;
	//header.cameraOffset                   = 0;
	header.entityCount                    = entities.size();
	header.entityArrayOffset              = sizeof(SaveHeader);
	
	//// entities ////
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
	
	u32 compCount;
	for(auto& pair : entities) {
		//write entity
		Entity* e = pair.second;
		file.write(e->name, 64);
		file.write((const char*)&e->id, sizeof(EntityID));
		compCount = e->components.size();
		file.write((const char*)&compCount, sizeof(u32));
		file.write((const char*)&e->transform.position, sizeof(Vector3));
		file.write((const char*)&e->transform.rotation, sizeof(Vector3));
		file.write((const char*)&e->transform.scale, sizeof(Vector3));
		
		//sort components
		for(auto c : e->components) {
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
	
	header.componentTypeHeaderArrayOffset = file.tellp();
	
	//// write component type headers ////
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
	typeHeader.size        = sizeof(u32) + sizeof(u16)*2 + sizeof(u32)*2; //instanceID, meshID, visible, entity_control
	typeHeader.count       = compsMeshComp.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//physics
	typeHeader.type        = ComponentType_Physics;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) + sizeof(Vector3)*6 + sizeof(float)*2 + sizeof(u32);
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
		file.write((const char*)&c->entity->id,     sizeof(u32));
		file.write((const char*)&c->InstanceID,     sizeof(u16));
		file.write((const char*)&c->MeshID,         sizeof(u16));
		file.write((const char*)&c->mesh_visible,   sizeof(u32));
		file.write((const char*)&c->ENTITY_CONTROL, sizeof(u32));
	}
	
	//physics
	for(auto c : compsPhysics){
		file.write((const char*)&c->entity->id,      sizeof(u32));
		file.write((const char*)&c->position,        sizeof(Vector3));
		file.write((const char*)&c->rotation,        sizeof(Vector3));
		file.write((const char*)&c->velocity,        sizeof(Vector3));
		file.write((const char*)&c->acceleration,    sizeof(Vector3));
		file.write((const char*)&c->rotVelocity,     sizeof(Vector3));
		file.write((const char*)&c->rotAcceleration, sizeof(Vector3));
		file.write((const char*)&c->elasticity,      sizeof(float));
		file.write((const char*)&c->mass,            sizeof(float));
		file.write((const char*)&c->isStatic,        sizeof(u32));
	}
	
	//store camera's size so we know offset to following entities list then store camera
	//int camsize = sizeof(*mainCamera);
	//deshi::appendFileBinary(file, (const char*)&camsize, sizeof(int));
	//deshi::appendFileBinary(file, (const char*)mainCamera, camsize);
	
	
	//// close file ////
	file.seekp(0);
	file.write((const char*)&header, sizeof(SaveHeader));
	
	file.close();
}

void EntityAdmin::Load(const char* filename) {
	
}

void EntityAdmin::AddSystem(System* system) {
	systems.push_back(system);
	system->Init(this);
}

void EntityAdmin::RemoveSystem(System* system) {
	for (int i = 0; i < systems.size(); ++i) {
		if (systems[i] == system) {
			systems.erase(systems.begin() + i);
		}
	}
}

void EntityAdmin::AddComponent(Component* component) {
	components.push_back(component);
	component->entity = 0;
}

void EntityAdmin::RemoveComponent(Component* component) {
	for (int i = 0; i < components.size(); ++i) {
		if (components[i] == component) {
			components.erase(components.begin() + i);
		}
	}
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

std::string Entity::Save() {
	
	return "";
}

u32 Entity::AddComponent(Component* component) {
	components.push_back(component);
	component->entity = this;
	return components.size() - 1;
}

u32 Entity::AddComponents(std::vector<Component*> comps) {
	u32 value = this->components.size();
	for (auto& c : comps) {
		this->components.push_back(c);
		c->entity = this;
	}
	return value;
}

Entity::Entity() {
	transform = Transform();
}

Entity::Entity(vec3 pos, vec3 rot, vec3 scale) {
	transform = Transform(pos, rot, scale);
}

Entity::~Entity() {
	for (Component* c : components) delete c;
}