#include "Admin.h"
#include "../core.h"
#include "components/Orb.h"
#include "components/door.h"
#include "components/Light.h"
#include "components/Camera.h"
#include "components/Player.h"
#include "components/Physics.h"
#include "components/MeshComp.h"
#include "components/Collider.h"
#include "components/Movement.h"
#include "components/Component.h"
#include "components/AudioSource.h"
#include "components/AudioListener.h"

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
	mainCamera = editorCamera;
#endif
	
	//reserve arrays
	//TODO(sushi) figure out why the reserve function i set up in ContainerManager tries deleting a nonexistant entity
	//entities.reserve(1000);
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
	
	//debug
	find_triangle_neighbors = true;
	debugTimes = true;
	fast_outline = 0;
}

void EntityAdmin::Cleanup() {
	Save((state == GameState_Editor) ? "temp.desh" : "auto.desh");
}

void UpdateLayer(ContainerManager<Component*> cl) {
	for_n(i, cl.size()) if(cl[i]) cl[i].value->Update();
}

void EntityAdmin::Update() {
	ImGui::BeginDebugLayer();
	if(!skip) controller.Update();
	if(!skip) mainCamera->Update();
	
	TIMER_RESET(t_a); 
	if (!skip && !paused)  { UpdateLayer(freeCompLayers[ComponentLayer_Physics]); }
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
		entities[e->id].getptr()->CleanUp();
		entities.remove_from(e->id);
	}
	deletionBuffer.clear();
	
	//creation buffer
	for(Entity* e : creationBuffer) {
		int index = entities.add(Entity(this, 255, e->transform, e->name, e->components));
		entities[index].getptr()->id = index;
		for(Component* c : e->components){ 
			c->entityID = index;
			c->layer_index = freeCompLayers[c->layer].add(c);
			if (c->comptype == ComponentType_Light) {
				dyncast(d, Light, c);
				scene.lights.push_back(d);
			}
			c->entity = entities[c->entityID].getptr();
		}
		operator delete(e); //call this to delete the staging entity, but not components (doesnt call destructor)
	}
	creationBuffer.clear();
	DengTime->worldSysTime =  TIMER_END(t_a); TIMER_RESET(t_a);
	
	for (int i = 0; i < 10; i++) {
		if (i < scene.lights.size()) {
			Vector3 p = scene.lights[i]->position;
			DengRenderer->lights[i] = glm::vec4(p.x, p.y, p.z, scene.lights[i]->brightness);
		}
		else {
			DengRenderer->lights[i] = glm::vec4(0, 0, 0, -1);
		}
	}
	
	
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
	SUCCESS("Resetting admin");
	entities.clear(); //entities.reserve(1000);
	for (auto& e : entities) {
		if (e) {
			e().CleanUp();
		}
	}
	
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
	header.componentTypeCount = 9;
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
		file.write((const char*)&e.value.id,                 sizeof(u32));
		file.write(e.value.name,                             sizeof(char)*64);
		file.write((const char*)&e.value.transform.position, sizeof(Vector3));
		file.write((const char*)&e.value.transform.rotation, sizeof(Vector3));
		file.write((const char*)&e.value.transform.scale,    sizeof(Vector3));
		
		//sort components
		for(auto c : e.value.components) {
			if(dyncast(d, MeshComp, c)) {
				compsMeshComp.push_back(d);
			}else if(dyncast(d, Physics, c)) {
				compsPhysics.push_back(d);
			}else if(dyncast(col, Collider, c)){
				if      (dyncast(d, BoxCollider, col)){
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
			}else if(dyncast(d, Player, c)){
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
	
	//player
	typeHeader.type        = ComponentType_Player;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) + sizeof(int);
	typeHeader.count       = compsPlayer.size();
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
		file.write((const char*)&c->entityID,    sizeof(u32));
		file.write((const char*)&c->position,    sizeof(Vector3));
		file.write((const char*)&c->direction,   sizeof(Vector3));
		file.write((const char*)&c->brightness,  sizeof(float));
	}
	
	//mesh comp
	for(auto c : compsMeshComp){
		b32 bool1 = c->mesh_visible;
		b32 bool2 = c->ENTITY_CONTROL;
		file.write((const char*)&c->entityID,   sizeof(u32));
		file.write((const char*)&c->instanceID, sizeof(u32));
		file.write((const char*)&c->meshID,     sizeof(u32));
		file.write((const char*)&bool1,         sizeof(b32));
		file.write((const char*)&bool2,         sizeof(b32));
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
	
	//player
	for(auto c : compsPlayer){
		file.write((const char*)&c->entityID, sizeof(u32));
		file.write((const char*)&c->health,   sizeof(int));
	}
	
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
		entities.add(tempEntity);
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
			case(ComponentType_AudioListener):  AudioListener ::Load(this, data, cursor, compHeader.count); break;
			case(ComponentType_AudioSource):    AudioSource   ::Load(this, data, cursor, compHeader.count); break;
			case(ComponentType_Camera):         Camera        ::Load(this, data, cursor, compHeader.count); break;
			case(ComponentType_ColliderBox):    BoxCollider   ::Load(this, data, cursor, compHeader.count); break;
			case(ComponentType_ColliderAABB):   AABBCollider  ::Load(this, data, cursor, compHeader.count); break;
			case(ComponentType_ColliderSphere): SphereCollider::Load(this, data, cursor, compHeader.count); break;
			case(ComponentType_Light):          Light         ::Load(this, data, cursor, compHeader.count); break;
			case(ComponentType_MeshComp):       MeshComp      ::Load(this, data, cursor, compHeader.count); break;
			case(ComponentType_OrbManager):     OrbManager    ::Load(this, data, cursor, compHeader.count); break;
			case(ComponentType_Physics):        Physics       ::Load(this, data, cursor, compHeader.count); break;
			case(ComponentType_Player):         Player        ::Load(this, data, cursor, compHeader.count); break;
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
	entities.add(Entity(this, id, e->transform, e->name, e->components));
	for (Component* c : e->components) {
		c->entityID = id;
		c->layer_index = freeCompLayers[c->layer].add(c);
		c->entity = entities[c->entityID].getptr();
	}
	operator delete(e);
	return &entities[id].value;
}

void EntityAdmin::DeleteEntity(u32 id) {
	if(id < entities.size()){
		deletionBuffer.push_back(entities[id].getptr());
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


