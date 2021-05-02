#include "EntityAdmin.h"
#include "core.h"

#include "utils/Command.h"
#include "utils/defines.h"
#include "scene/Scene.h"

#include "game/Keybinds.h"
#include "game/Transform.h"
#include "game/Controller.h"
#include "game/components/Orb.h"
#include "game/components/door.h"
#include "game/components/Light.h"
#include "game/components/Camera.h"
#include "game/components/Player.h"
#include "game/components/Physics.h"
#include "game/components/MeshComp.h"
#include "game/components/Collider.h"
#include "game/components/Movement.h"
#include "game/components/Component.h"
#include "game/components/AudioSource.h"
#include "game/components/AudioListener.h"

#include <iostream>
#include <fstream>
#include <utility>
#include <stdexcept>

//// EntityAdmin ////

void EntityAdmin::Init() {
	//decide initial gamestate
#if defined(DESHI_BUILD_PLAY)
	state = GameState_Play;
	pause_phys = pause_sound = false;
	editorCamera = 0;
#elif defined(DESHI_BUILD_DEBUG)
	state = GameState_Debug;
	pause_phys = pause_sound = false;
	editorCamera = 0;
#else
	state = GameState_Editor;
	pause_phys = pause_sound = true;
	editorCamera = new Camera(90.f, .01f, 1000.01f, true); //temporary camera creation on admin
	editorCamera->Init(this);
	mainCamera = editorCamera;
#endif
	
	//reserve arrays
	entities.reserve(1000);
	creationBuffer.reserve(100);
	deletionBuffer.reserve(100);
	for_n(i, ComponentLayer_LAST) freeCompLayers.push_back(ContainerManager<Component*>());
	
	//init singletons
	physics.Init(this);
	canvas.Init(this);
	sound.Init(this);
	scene.Init();
	DengRenderer->LoadScene(&scene);
	keybinds.Init();
	controller.Init(this);
	undoManager.Init();
	
	//default values
	selectedEntity = player = 0;
	skip = false;
	paused = false;
	pause_command = pause_canvas = pause_world = false;
	find_triangle_neighbors = true;
	debugTimes = true;
}

void EntityAdmin::Cleanup() {
	Save("auto.desh");
}

void UpdateLayer(ContainerManager<Component*> cl) {
	for_n(i, cl.size()) if(cl[i]) cl[i].value->Update();
}

void EntityAdmin::Update() {
	ImGui::BeginDebugLayer();
	if(!skip) controller.Update();
	if(!skip) mainCamera->Update();
	
	TIMER_RESET(t_a); 
	if (!skip && !pause_phys && !paused)  { UpdateLayer(freeCompLayers[ComponentLayer_Physics]); }
	DengTime->physLyrTime =   TIMER_END(t_a); TIMER_RESET(t_a);
	if (!skip && !pause_phys && !paused)  { physics.Update(); }
	DengTime->physSysTime =   TIMER_END(t_a); TIMER_RESET(t_a);
	if (!skip && !pause_canvas)           { UpdateLayer(freeCompLayers[ComponentLayer_Canvas]); }
	DengTime->canvasLyrTime = TIMER_END(t_a); TIMER_RESET(t_a);
	if (!skip && !pause_canvas)           { canvas.Update(); }
	DengTime->canvasSysTime = TIMER_END(t_a); TIMER_RESET(t_a);
	if (!skip && !pause_sound && !paused) { UpdateLayer(freeCompLayers[ComponentLayer_Sound]); }
	DengTime->sndLyrTime =    TIMER_END(t_a); TIMER_RESET(t_a);
	if (!skip && !pause_sound && !paused) { sound.Update(); }
	DengTime->sndSysTime =    TIMER_END(t_a);
	ImGui::EndDebugLayer();
}

void EntityAdmin::PostRenderUpdate(){ //no imgui stuff allowed
	TIMER_RESET(t_a);
	if (!skip && !pause_world) UpdateLayer(freeCompLayers[ComponentLayer_World]); 
	DengTime->worldLyrTime = TIMER_END(t_a); TIMER_RESET(t_a);
	
	//deletion buffer
	for(Entity* e : deletionBuffer) {
		if(selectedEntity == e) selectedEntity = 0;
		for(Component* c : e->components){ 
			freeCompLayers[c->layer].remove_from(c->layer_index);
		}
		entities.erase(entities.begin()+e->id);
	}
	deletionBuffer.clear();
	
	//creation buffer
	for(Entity* e : creationBuffer) {
		entities.emplace_back(this, u32(entities.size()), e->transform, e->name, e->components);
		for(Component* c : e->components){ 
			c->entityID = entities.size()-1;
			c->layer_index = freeCompLayers[c->layer].add(c);
			c->Init(this);
		}
		operator delete(e); //call this to delete the staging entity, but not components (doesnt call destructor)
	}
	creationBuffer.clear();
	DengTime->worldSysTime =  TIMER_END(t_a); TIMER_RESET(t_a);
	
	DengTime->paused = paused;
	DengTime->phys_pause = pause_phys;
	skip = false;
}

void EntityAdmin::ChangeState(GameState new_state){
	if(state == new_state) return;
	if(state >= GameState_LAST) return ERROR("Admin attempted to switch to unhandled gamestate: ", new_state);
	
	std::string from, to;
	switch(state){
		//old state: play/debug
		case GameState_Play: 
		case GameState_Debug:     from = "PLAY/DEBUG";
		switch(new_state){
			case GameState_Menu:{   to = "MENU";
				pause_phys = true;
				DengWindow->UpdateCursorMode(CursorMode::DEFAULT);
			}break;
			case GameState_Editor:{ to = "EDITOR";
				pause_phys = pause_sound = true;
				Save("auto.desh");
				Load("temp.desh");
				if(player) player->GetComponent<MeshComp>()->Visible(true);
				DengWindow->UpdateCursorMode(CursorMode::DEFAULT);
			}break;
		}break;
		
		//old state: menu
		case GameState_Menu:      from = "MENU";
		switch(new_state){
			case GameState_Play: 
			case GameState_Debug:{  to = "PLAY/DEBUG";
				pause_phys = false;
				if(!player) ERROR("No player on admin");
				DengWindow->UpdateCursorMode(CursorMode::FIRSTPERSON);
			}break;
			case GameState_Editor:{ to = "EDITOR";
				pause_phys = pause_sound = true;
				Save("auto.desh");
				Load("temp.desh");
				if(player) player->GetComponent<MeshComp>()->Visible(true);
				DengWindow->UpdateCursorMode(CursorMode::DEFAULT);
			}break;
		}break;
		
		//old state: editor
		case GameState_Editor:    from = "EDITOR";
		switch(new_state){
			case GameState_Play: 
			case GameState_Debug:{  to = "PLAY/DEBUG";
				pause_phys = pause_sound = false;
				Save("temp.desh");
				if(player) {
					player->GetComponent<MeshComp>()->Visible(false);
				}else{
					ERROR("No player on admin");
				}
				DengWindow->UpdateCursorMode(CursorMode::FIRSTPERSON);
			}break;
			case GameState_Menu:{   to = "MENU";
				Save("save.desh");
				DengWindow->UpdateCursorMode(CursorMode::DEFAULT);
			}break;
		}break;
	}
	state = new_state;
	SUCCESS("Changed gamestate from ", from, " to ", to);
}

void EntityAdmin::Reset(){
	entities.clear(); entities.reserve(1000);
	for (auto& layer : freeCompLayers) { layer.clear(); }
	selectedEntity = 0;
	undoManager.Reset();
	scene.Reset();
	DengRenderer->Reset();
	DengRenderer->LoadScene(&scene);
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

void EntityAdmin::Save(const char* filename) {
	//std::vector<char> save_data(4096);
	
	//open file
	std::string filepath = deshi::dirSaves() + filename;
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
	if(!file.is_open()){ ERROR("Failed to open file '", filepath, "' when trying to save"); return; }
	
	SaveHeader header{};
	file.write((const char*)&header, sizeof(SaveHeader)); //zero fill header
	header.magic = 1213416772; //DESH
	header.flags = 0;
	
	//// entities ////
	header.entityCount       = entities.size();
	header.entityArrayOffset = sizeof(SaveHeader);
	
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
	std::vector<Player*>         compsPlayer;

	//TODO(delle,Cl) convert these vectors to char vectors and when iterating thru entities
	// and thier components, call the save function of an entity to add to the components
	// vector and then use the final size of that vector for type header offsets
	
	for(auto& e : entities) {
		//write entity
		file.write((const char*)&e.id,                 sizeof(u32));
		file.write(e.name,                             sizeof(char)*64);
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
			}else if (dyncast(d, Player, c)) {
				compsPlayer.push_back(d);
			}else{
				ERROR("Unhandled component '", c->name, "' found when attempting to save");
			}
		}
	}
	
	//// write meshes ////
	header.meshCount = DengRenderer->meshes.size();
	header.meshArrayOffset = file.tellp();
	
	for(auto& m : DengRenderer->meshes){
		b32 base = m.base;
		file.write((const char*)&m.id, sizeof(u32));
		file.write((const char*)&base, sizeof(b32));
		file.write(m.name,             sizeof(char)*64);
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
	typeHeader.size        = sizeof(u32) + sizeof(u32) + sizeof(Matrix3) + sizeof(Vector3);
	typeHeader.count       = compsColliderBox.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//collider aabb
	typeHeader.type        = ComponentType_ColliderAABB;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) + sizeof(u32) + sizeof(Matrix3) + sizeof(Vector3);
	typeHeader.count       = compsColliderAABB.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//collider sphere
	typeHeader.type        = ComponentType_ColliderSphere;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) + sizeof(u32) + sizeof(Matrix3) + sizeof(float);
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
	typeHeader.size        = sizeof(u32) + sizeof(u32)*2 + sizeof(b32)*2; //instanceID, meshID, visible, entity_control
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
		file.write((const char*)&c->entityID,    sizeof(u32));
		file.write((const char*)&c->position,    sizeof(Vector3));
		file.write((const char*)&c->velocity,    sizeof(Vector3));
		file.write((const char*)&c->orientation, sizeof(Vector3));
	}
	
	//audio source
	for(auto c : compsAudioSource){
		file.write((const char*)&c->entityID, sizeof(u32));
	}
	
	//collider box
	for(auto c : compsColliderBox){
		file.write((const char*)&c->entityID,       sizeof(u32));
		file.write((const char*)&c->collisionLayer, sizeof(u32));
		file.write((const char*)&c->inertiaTensor,  sizeof(Matrix3));
		file.write((const char*)&c->halfDims,       sizeof(Vector3));
	}
	
	//collider aabb
	for(auto c : compsColliderAABB){
		file.write((const char*)&c->entityID,       sizeof(u32));
		file.write((const char*)&c->collisionLayer, sizeof(u32));
		file.write((const char*)&c->inertiaTensor,  sizeof(Matrix3));
		file.write((const char*)&c->halfDims,       sizeof(Vector3));
	}
	
	//collider sphere
	for(auto c : compsColliderSphere){
		file.write((const char*)&c->entityID,       sizeof(u32));
		file.write((const char*)&c->collisionLayer, sizeof(u32));
		file.write((const char*)&c->inertiaTensor,  sizeof(Matrix3));
		file.write((const char*)&c->radius,         sizeof(float));
	}
	
	//light
	for(auto c : compsLight){
		file.write((const char*)&c->entityID,   sizeof(u32));
		file.write((const char*)&c->position,   sizeof(Vector3));
		file.write((const char*)&c->direction,  sizeof(Vector3));
		file.write((const char*)&c->strength,   sizeof(float));
	}
	
	//mesh comp
	for(auto c : compsMeshComp){
		b32 bool1 = c->mesh_visible;
		b32 bool2 = c->ENTITY_CONTROL;
		file.write((const char*)&c->entityID,       sizeof(u32));
		file.write((const char*)&c->instanceID,     sizeof(u32));
		file.write((const char*)&c->meshID,         sizeof(u32));
		file.write((const char*)&bool1,             sizeof(b32));
		file.write((const char*)&bool2,             sizeof(b32));
	}
	
	//physics
	for(auto c : compsPhysics){
		b32 isStatic = c->isStatic;
		file.write((const char*)&c->entityID,        sizeof(u32));
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

	for (auto c : compsPlayer) {

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
	SUCCESS("Successfully saved to ", filename);
}

void EntityAdmin::Load(const char* filename) {
	Reset();
	SUCCESS("Loading level: ", deshi::dirSaves() + filename);
	TIMER_START(t_l);
	
	//// read file to char array ////
	u32 cursor = 0;
	std::vector<char> file = deshi::readFileBinary(deshi::dirSaves() + filename);
	const char* data = file.data();
	if(!data) return;
	
	//check for magic number
	u32 magic = 1213416772; //DESH
	if(memcmp(data, &magic, 4) != 0) return ERROR("Invalid magic number when loading save file: ", filename);
	
	//// parse header ////
	SaveHeader header;
	memcpy(&header, data+cursor, sizeof(SaveHeader)); cursor += sizeof(SaveHeader);
	
	//// parse and create entities ////
	if(cursor != header.entityArrayOffset) {
		return ERROR("Load failed because cursor was at '", cursor, 
					 "' when reading entities which start at '", header.entityArrayOffset, "'");
	}
	
	entities.reserve(header.entityCount);
	Entity tempEntity; tempEntity.admin = this;
	for_n(i, header.entityCount){
		memcpy(&tempEntity.id,        data+cursor, sizeof(u32));     cursor += sizeof(u32);
		memcpy(tempEntity.name,       data+cursor, sizeof(char)*64); cursor += sizeof(char)*64;
		memcpy(&tempEntity.transform, data+cursor, sizeof(vec3)*3);  cursor += sizeof(vec3)*3;
		entities.push_back(tempEntity);
	}
	
	//// parse and load/create meshes ////
	if(cursor != header.meshArrayOffset) {
		return ERROR("Load failed because cursor was at '", cursor, 
					 "' when reading meshes which start at '", header.meshArrayOffset, "'");
	}
	
	b32 baseMesh = 0;
	char meshName[64];
	for_n(i, header.meshCount){
		cursor += sizeof(u32);
		memcpy(&baseMesh, data+cursor, sizeof(b32));     cursor += sizeof(b32);
		memcpy(meshName,  data+cursor, sizeof(char)*64); cursor += sizeof(char)*64;
		if(!baseMesh) DengRenderer->CreateMesh(&scene, meshName);
	}
	
	//// parse and create components ////
	if(cursor != header.componentTypeHeaderArrayOffset) {
		return ERROR("Load failed because cursor was at '", cursor, 
					 "' when reading meshes which start at '", header.componentTypeHeaderArrayOffset, "'");
	}
	
	ComponentTypeHeader compHeader;
	for_n(i, header.componentTypeCount){
		cursor = header.componentTypeHeaderArrayOffset + (sizeof(u32)*4)*i;
		memcpy(&compHeader, data+cursor, sizeof(u32)*4);
		cursor = compHeader.arrayOffset;
		
		switch(compHeader.type){
			case(ComponentType_AudioListener):  AudioListener ::Load(entities, data, cursor, compHeader.count); break;
			case(ComponentType_AudioSource):    AudioSource   ::Load(entities, data, cursor, compHeader.count); break;
			case(ComponentType_Camera):         Camera        ::Load(entities, data, cursor, compHeader.count); break;
			case(ComponentType_ColliderBox):    BoxCollider   ::Load(entities, data, cursor, compHeader.count); break;
			case(ComponentType_ColliderAABB):   AABBCollider  ::Load(entities, data, cursor, compHeader.count); break;
			case(ComponentType_ColliderSphere): SphereCollider::Load(entities, data, cursor, compHeader.count); break;
			case(ComponentType_Light):          Light         ::Load(entities, data, cursor, compHeader.count); break;
			case(ComponentType_MeshComp):       MeshComp      ::Load(entities, data, cursor, compHeader.count); break;
			case(ComponentType_OrbManager):     OrbManager    ::Load(entities, data, cursor, compHeader.count); break;
			case(ComponentType_Physics):        Physics       ::Load(entities, data, cursor, compHeader.count); break;
			default:{
				ERROR("Failed to load a component array because of unknown component type '", 
					  compHeader.type, "' at pos: ", cursor);
			}break;
		}
	}
	
	SUCCESS("Finished loading level '", filename, "' in ", TIMER_END(t_l), "ms");
	SkipUpdate();
}


u32 EntityAdmin::CreateEntity(const char* name) {
	u32 id = entities.size() + creationBuffer.size() - 1;
	creationBuffer.push_back(new Entity(this, id, Transform(), name));
	return id;
}

u32 EntityAdmin::CreateEntity(std::vector<Component*> components, const char* name, Transform transform) {
	Entity* e = new Entity;
	e->SetName(name);
	e->admin = this;
	e->transform = transform;
	e->AddComponents(components);
	creationBuffer.push_back(e);
	return entities.size() + creationBuffer.size() - 1;
}

u32 EntityAdmin::CreateEntity(Entity* e) {
	e->admin = this;
	creationBuffer.push_back(e);
	return entities.size() + creationBuffer.size() - 1;
}

Entity* EntityAdmin::CreateEntityNow(std::vector<Component*> components, const char* name, Transform transform) {
	Entity* e = new Entity;
	e->SetName(name);
	e->admin = this;
	e->transform = transform;
	e->AddComponents(components);
	u32 id = entities.size();
	entities.emplace_back(this, id, e->transform, e->name, e->components);
	for (Component* c : e->components) {
		c->entityID = id;
		c->layer_index = freeCompLayers[c->layer].add(c);
		c->Init(this);
	}
	operator delete(e);
	return &entities[id];
}

void EntityAdmin::DeleteEntity(u32 id) {
	if(id < entities.size()){
		deletionBuffer.push_back(&entities[id]);
	}else{
		ERROR("Attempted to add entity '", id, "' to deletion buffer when it doesn't exist on the admin");
	}
}

void EntityAdmin::DeleteEntity(Entity* e) {
	if(e->id < entities.size()){
		deletionBuffer.push_back(e);
	}else{
		ERROR("Attempted to add entity '", e->id, "' to deletion buffer when it doesn't exist on the admin");
	}
}


////////////////
//// Entity ////
////////////////

Entity::Entity(){
	this->admin = 0;
	this->id = -1;
	this->transform = Transform();
	this->name[63] = '\0';
}

Entity::Entity(EntityAdmin* admin, u32 id, Transform transform, const char* name, std::vector<Component*> comps){
	this->admin = admin;
	this->id = id;
	this->transform = transform;
	if(name) cpystr(this->name, name, 63);
	for (Component* c : comps) {
		if(!c) continue;
		this->components.push_back(c);
		c->entityID = id;
		c->admin = admin;
	}
}

Entity::~Entity() {
	for (Component* c : components) delete c;
}

void Entity::SetName(const char* name){
	if(name) cpystr(this->name, name, 63);
}

void Entity::AddComponent(Component* c) {
	if(!c) return;
	components.push_back(c);
	c->entityID = id;
	c->admin = this->admin;
}

void Entity::AddComponents(std::vector<Component*> comps) {
	u32 value = this->components.size();
	for (Component* c : comps) {
		if(!c) continue;
		this->components.push_back(c);
		c->entityID = id;
		c->admin = this->admin;
	}
}

void Entity::RemoveComponent(Component* c) {
	if(!c) return;
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
			if(!comps[i]) continue;
			if(components[i] == comps[0]){ 
				admin->freeCompLayers[comps[i]->layer].remove_from(comps[i]->layer_index);
				delete comps[i]; 
				components.erase(components.begin()+i); 
				comps.erase(components.begin()); 
				break;
			}
		}
	}
}





auto eat_spaces(std::string& str){
	size_t idx = str.find_first_not_of(' ');
	return (idx != -1) ? str.substr(idx) : "";
}

auto get_kvPair(std::string& str){
	size_t idx = str.find_first_of(' ');
	if(idx == -1) return std::make_pair(str, std::string(""));
	
	std::string key = str.substr(0, idx);
	std::string val;
	size_t start = str.find_first_not_of(' ', idx);
	size_t end = -1;
	if(str[start] == '\"'){ 
		end = str.find_first_of('\"', start+1);
		val = str.substr(start+1, end-(start+1));
	}else if(str[start] == '\''){
		end = str.find_first_of('\'', start+1);
		val = str.substr(start+1, end-(start+1));
	}else if(str[start] == '('){
		end = str.find_first_of(')', start+1);
		val = str.substr(start+1, end-(start+1));
	}else{ 
		end = str.find_first_of(" #\n", start+1);
		val = str.substr(start, end-start);
	}
	
	//PRINTLN("key: '"<< key<<"' val: '"<<val<<"'");
	return std::make_pair(key, val);
}

auto get_vec3(std::string& str){
	size_t sz, off = 0;
	try{
		f32 x = std::stof(str, &sz);             off += sz+1;
		f32 y = std::stof(str.substr(off), &sz); off += sz+1;
		f32 z = std::stof(str.substr(off), &sz);
		return vec3(x,y,z);
	}catch(std::invalid_argument& ia){
		ERROR("Failed to parse vector3: (", str, "): ", ia.what());
		return Vector3::ZERO;
	}catch(std::out_of_range& oor){
		try{ off = 0;
			f64 x = std::stod(str, &sz);             off += sz+1;
			f64 y = std::stod(str.substr(off), &sz); off += sz+1;
			f64 z = std::stod(str.substr(off), &sz);
			return vec3(x,y,z);
		}catch(...){
			ERROR("Failed to parse vector3: (", str, ")");
			return Vector3::ZERO;
		}
	}
}

//TODO(delle) move component creation to after collecting all vars
Entity* Entity::CreateEntityFromFile(EntityAdmin* admin, std::string& filename){
	if(filename.find(".entity") == -1) filename += ".entity";
	std::ifstream file(deshi::dirEntities() + filename, std::ifstream::in);
	if(!file.is_open()) { ERROR("Failed to open file: ", filename.c_str()); return 0; }
	
	Entity* e = 0;
	AudioListener* al = 0; AudioSource* as = 0; Camera* cam = 0;     Door* door = 0;    
	Collider* coll = 0;    Light* light = 0;    MeshComp* mesh = 0;  Physics* phys = 0;
	
	vec3 pos{}, rot{}, scale = Vector3::ONE;
	vec3 halfDims = Vector3::ONE; f32 radius = 1.f; b32 noCollide = 0;
	
	//parse file
	std::string line;
	u32 line_number = 0;
	while(std::getline(file, line)){
		++line_number;
		if(line[0] == ' ') line = eat_spaces(line);
		if(line.empty() || line[0] == '#') continue;
		
		if(line[0] == '>'){
			if(line.size() < 2) {
				ERROR("Error parsing ", filename, " at line ", line_number, "! Line starting with '>' must have a header following it"); continue;
			}
			
			find_header:
			if(line.empty()) continue;
			if(line.find(">entity") != -1){
				e = new Entity;
				while(true){
					std::getline(file, line); line_number++;
					if(line.empty() || line[0] == '>') goto find_header;
					if(line[0] == ' ') line = eat_spaces(line);
					if(line[0] == '#') continue;
					
					auto pair = get_kvPair(line);
					if(pair.second == ""){
						ERROR("Error parsing ", filename, " at line ", line_number, "! Unable to extract key-value pair from:\n    ", line);
						continue;
					}
					
					if(pair.first == "name"){
						cpystr(e->name, pair.second.c_str(), 63);
					}else if(pair.first == "position"){
						pos = get_vec3(pair.second);
					}else if(pair.first == "rotation"){
						rot = get_vec3(pair.second);
					}else if(pair.first == "scale"){
						scale = get_vec3(pair.second);
					}else{
						ERROR("Error parsing ", filename, " at line ", line_number, "! Invalid attribute '", pair.first, "' for header 'entity'");
					}
				}
			}else if(line.find(">audio listener") != -1){
				al = new AudioListener(pos);
			}else if(line.find(">audio source") != -1){
				as = new AudioSource;
			}else if(line.find(">camera") != -1){
				//TODO(delle) handle camera component
				cam = new Camera(90.f);
			}else if(line.find(">collider") != -1){
				while(true){
					std::getline(file, line); line_number++;
					if(line.empty() || line[0] == '>') goto find_header;
					if(line[0] == ' ') line = eat_spaces(line);
					if(line[0] == '#') continue;
					
					auto pair = get_kvPair(line);
					if(pair.second == ""){
						ERROR("Error parsing ", filename, " at line ", line_number, "! Unable to extract key-value pair from:\n    ", line);
						continue;
					}
					
					if(pair.first == "type"){
						if(pair.second == "aabb"){
							coll = new AABBCollider(Vector3::ONE, 1);
						}else if(pair.second == "box"){
							coll = new BoxCollider(Vector3::ONE, 1);
						}else if(pair.second == "sphere"){
							coll = new SphereCollider(1, 1);
						}else{
							ERROR("Error parsing ", filename, " at line ", line_number, "! Unhandled collider type '", pair.second, "'"); break;
						}
					}else if(pair.first == "halfdims"){
						halfDims = get_vec3(pair.second);
					}else if(pair.first == "radius"){
						try{ radius = std::stof(pair.second); }catch(...){ 
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse float '", pair.second,"'");
						}
					}else if(pair.first == "noCollide"){
						if(pair.second == "true" || pair.second == "1"){
							noCollide = 1;
						}else if(pair.second == "false" || pair.second == "0"){
							noCollide = 0;
						}else{
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse boolean '", pair.second,"'");
						}
					}else if(pair.first == "command"){
						WARNING("Collider command not handled in entity files");
					}else{
						ERROR("Error parsing ", filename, " at line ", line_number, "! Invalid attribute '", pair.first, "' for header 'collider'");
					}
				}
			}else if(line.find(">light") != -1){
				//TODO(delle) handle light component
				light = new Light(pos, rot);
			}else if(line.find(">mesh") != -1){
				mesh = new MeshComp;
				while(true){
					std::getline(file, line); line_number++;
					if(line.empty() || line[0] == '>') goto find_header;
					if(line[0] == ' ') line = eat_spaces(line);
					if(line[0] == '#') continue;
					
					auto pair = get_kvPair(line);
					if(pair.second == ""){
						ERROR("Error parsing ", filename, " at line ", line_number, "! Unable to extract key-value pair from:\n    ", line);
						continue;
					}
					
					if(pair.first == "name"){
						mesh->meshID = DengRenderer->CreateMesh(&admin->scene, pair.second.c_str());
					}else if(pair.first == "visible"){
						if(pair.second == "true" || pair.second == "1"){
							mesh->mesh_visible = 1;
						}else if(pair.second == "false" || pair.second == "0"){
							mesh->mesh_visible = 0;
						}else{
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse boolean '", pair.second,"'");
						}
					}else{
						ERROR("Error parsing ", filename, " at line ", line_number, "! Invalid attribute '", pair.first, "' for header 'mesh'");
					}
				}
			}else if(line.find(">physics") != -1){
				phys = new Physics(pos, rot);
				while(true){
					std::getline(file, line); line_number++;
					if(line.empty() || line[0] == '>') goto find_header;
					if(line[0] == ' ') line = eat_spaces(line);
					if(line[0] == '#') continue;
					
					auto pair = get_kvPair(line);
					if(pair.second == ""){
						ERROR("Error parsing ", filename, " at line ", line_number, "! Unable to extract key-value pair from:\n    ", line);
						continue;
					}
					
					if(pair.first == "velocity"){
						phys->velocity = get_vec3(pair.second);
					}else if(pair.first == "acceleration"){
						phys->acceleration = get_vec3(pair.second);
					}else if(pair.first == "rotVelocity"){
						phys->rotVelocity = get_vec3(pair.second);
					}else if(pair.first == "rotAcceleration"){
						phys->rotAcceleration = get_vec3(pair.second);
					}else if(pair.first == "elasticity"){
						try{ phys->elasticity = std::stof(pair.second); }catch(...){ 
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse float '", pair.second,"'");
						}
					}else if(pair.first == "mass"){
						try{ phys->mass = std::stof(pair.second); }catch(...){ 
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse float '", pair.second,"'");
						}
					}else if(pair.first == "staticPosition"){
						if(pair.second == "true" || pair.second == "1"){
							phys->isStatic = 1;
						}else if(pair.second == "false" || pair.second == "0"){
							phys->isStatic = 0;
						}else{
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse boolean '", pair.second,"'");
						}
					}else if(pair.first == "staticRotation"){
						if(pair.second == "true" || pair.second == "1"){
							phys->staticRotation = 1;
						}else if(pair.second == "false" || pair.second == "0"){
							phys->staticRotation = 0;
						}else{
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse boolean '", pair.second,"'");
						}
					}else{
						ERROR("Error parsing ", filename, " at line ", line_number, "! Invalid attribute '", pair.first, "' for header 'physics'");
					}
				}
			}else if(line.find(">door") != -1){
				door = new Door;
				while(true){
					std::getline(file, line); line_number++;
					if(line.empty() || line[0] == '>') goto find_header;
					if(line[0] == ' ') line = eat_spaces(line);
					if(line[0] == '#') continue;
					
					auto pair = get_kvPair(line);
					if(pair.second == ""){
						ERROR("Error parsing ", filename, " at line ", line_number, "! Unable to extract key-value pair from:\n    ", line);
						continue;
					}
					
					if(pair.first == "isOpen"){
						if(pair.second == "true" || pair.second == "1"){
							door->isOpen = 1;
						}else if(pair.second == "false" || pair.second == "0"){
							door->isOpen = 1;
						}else{
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse boolean '", pair.second,"'");
						}
					}else{
						ERROR("Error parsing ", filename, " at line ", line_number, "! Invalid attribute '", pair.first, "' for header 'door'");
					}
				}
			}else{
				ERROR("Error parsing ", filename, " at line ", line_number, "! Unhandled header"); continue;
			}
		}
	}
	
	file.close();
	if(e){
		e->transform.position = pos;
		e->transform.rotation = rot;
		e->transform.scale = scale;
		if(coll){
			coll->noCollide = noCollide;
			switch(coll->type){
				case ColliderType_AABB:  { ((AABBCollider*)coll)->halfDims = halfDims; }break;
				case ColliderType_Box:   { ((BoxCollider*)coll)->halfDims = halfDims;  }break;
				case ColliderType_Sphere:{ ((SphereCollider*)coll)->radius = radius;   }break;
			}
		}
		e->AddComponents({ al, as, cam, coll, light, mesh, phys });
		return e;
	}else{
		delete al; delete as; delete cam; delete coll; delete light; delete mesh; delete phys;
		return 0;
	}
}