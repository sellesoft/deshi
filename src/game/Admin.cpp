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
#elif defined(DESHI_BUILD_DEBUG)
	state = GameState_Debug;
	pause_phys = pause_sound = false;
#else
	state = GameState_Editor;
	pause_phys = pause_sound = true;
	DengTime->phys_pause = true;
	DengTime->fixedAccumulator = 0;
#endif
	
	//reserve arrays
	entities.reserve(1000);
	creationBuffer.reserve(100);
	deletionBuffer.reserve(100);
	for_n(i, ComponentLayer_World+1) freeCompLayers.push_back(ContainerManager<Component*>());
	
	//init singletons
	physics.Init(this);
	canvas.Init(this);
	sound.Init(this);
	scene.Init();
	DengRenderer->LoadScene(&scene);
	keybinds.Init();
	controller.Init(this);
	editor.Init(this);
	mainCamera = editor.camera;//TODO(delle) remove this eventually
	
	//default values
	player = 0;
	skip = false;
	paused = false;
	pause_command = pause_canvas = pause_world = false;
	
	//debug
	debugTimes = true;
}

void EntityAdmin::Cleanup() {
	SaveDESH((state == GameState_Editor) ? "temp.desh" : "auto.desh");
}

void UpdateLayer(ContainerManager<Component*> cl) {
	for_n(i, cl.size()) {
		if (cl[i]) {
			cl[i].value->Update();
		}
	}
}

void EntityAdmin::Update() {
	ImGui::BeginDebugLayer();
	if(!skip && (state == GameState_Editor || state == GameState_Debug)) editor.Update();
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

void EntityAdmin::PostRenderUpdate(){ //no imgui stuff allowed b/c rendering already happened
	TIMER_RESET(t_a);
	if (!skip && !pause_world) UpdateLayer(freeCompLayers[ComponentLayer_World]); 
	DengTime->worldLyrTime = TIMER_END(t_a); TIMER_RESET(t_a);
	
	//deletion buffer
	for(Entity* e : deletionBuffer) {
		for(Component* c : e->components) freeCompLayers[c->layer].remove_from(c->layer_index);
		for(int i = e->id+1; i < entities.size(); ++i) entities[i]->id -= 1;
		entities.erase(entities.begin()+e->id);
		if (e == player) player = nullptr;
		delete e;
	}
	deletionBuffer.clear();
	
	//creation buffer
	for(Entity* e : creationBuffer) {
		e->id = entities.size();
		e->admin = this;
		entities.push_back(e);
		for(Component* c : e->components){ 
			c->admin = this;
			c->entityID = e->id;
			c->compID = compIDcount;
			c->entity = e;
			c->layer_index = freeCompLayers[c->layer].add(c);
			if (c->comptype == ComponentType_Light) scene.lights.push_back(dyncasta(Light, c));
			compIDcount++;
		}
	}
	creationBuffer.clear();
	DengTime->worldSysTime = TIMER_END(t_a); TIMER_RESET(t_a);
	
	//light updating
	for (int i = 0; i < 10; i++) {
		if (i < scene.lights.size()) {
			Vector3 p = scene.lights[i]->position;
			DengRenderer->lights[i] = glm::vec4(p.x, p.y, p.z,
												(scene.lights[i]->active) ? scene.lights[i]->brightness : 0);
		}
		else {
			DengRenderer->lights[i] = glm::vec4(0, 0, 0, -1);
		}
	}
	
	//compIDcount = 0;
	//for (Entity* e : entities) compIDcount += e->components.size();
	
	
	DengTime->paused = paused;
	DengTime->phys_pause = pause_phys;
	skip = false;
}

u32 EntityAdmin::CreateEntity(const char* name) {
	u32 id = entities.size() + creationBuffer.size() - 1;
	creationBuffer.push_back(new Entity(this, id, Transform(), name));
	return id;
}

u32 EntityAdmin::CreateEntity(std::vector<Component*> components, const char* name, Transform transform) {
	u32 id = entities.size() + creationBuffer.size() - 1;
	creationBuffer.push_back(new Entity(this, id, transform, name, components));
	return id;
}

u32 EntityAdmin::CreateEntity(Entity* e) {
	e->admin = this;
	creationBuffer.push_back(e);
	return entities.size() + creationBuffer.size() - 1;
}

Entity* EntityAdmin::CreateEntityNow(std::vector<Component*> components, const char* name, Transform transform) {
	Entity* e = new Entity(this, entities.size(), transform, name, components);
	entities.push_back(e);
	for (Component* c : e->components) {
		c->entityID = e->id;
		c->compID = compIDcount;
		c->entity = e;
		c->layer_index = freeCompLayers[c->layer].add(c);
		if (c->comptype == ComponentType_Light) scene.lights.push_back(dyncasta(Light, c));
		compIDcount++;
	}
	return e;
}

void EntityAdmin::DeleteEntity(u32 id) {
	if (id < entities.size()) {
		deletionBuffer.push_back(entities[id]);
	}
	else {
		ERROR("Attempted to add entity '", id, "' to deletion buffer when it doesn't exist on the admin");
	}
}

void EntityAdmin::DeleteEntity(Entity* e) {
	if (e->id < entities.size()) {
		deletionBuffer.push_back(e);
	}
	else {
		ERROR("Attempted to add entity '", e->id, "' to deletion buffer when it doesn't exist on the admin");
	}
}

void EntityAdmin::ChangeState(GameState new_state){
	if(state == new_state) return;
	if(state >= GameState_COUNT) return ERROR("Admin attempted to switch to unhandled gamestate: ", new_state);
	
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
				SaveDESH("auto.desh");
				LoadDESH("temp.desh");
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
				SaveDESH("auto.desh");
				LoadDESH("temp.desh");
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
				SaveDESH("temp.desh");
				if(player) {
					player->GetComponent<MeshComp>()->Visible(false);
				}else{
					ERROR("No player on admin");
				}
				DengWindow->UpdateCursorMode(CursorMode::FIRSTPERSON);
			}break;
			case GameState_Menu:{   to = "MENU";
				SaveDESH("save.desh");
				DengWindow->UpdateCursorMode(CursorMode::DEFAULT);
			}break;
		}break;
	}
	state = new_state;
	SUCCESS("Changed gamestate from ", from, " to ", to);
}

void EntityAdmin::Reset(){
	SUCCESS("Resetting admin");
	for (Entity* e : entities) if(e) delete e;
	entities.clear();
	
	for (auto& layer : freeCompLayers) { layer.clear(); }
	editor.Reset();
	scene.Reset();
	DengRenderer->Reset();
	DengRenderer->LoadScene(&scene);
}

struct SaveHeader{
	u32 magic;
	u32 flags;
	u32 entityCount;
	u32 entityArrayOffset;
	u32 textureCount;
	u32 textureArrayOffset;
	u32 materialCount;
	u32 materialArrayOffset;
	u32 meshCount;
	u32 meshArrayOffset;
	u32 componentTypeCount;
	u32 componentTypeHeaderArrayOffset;
};

void EntityAdmin::SaveDESH(const char* filename) {
	//std::vector<char> save_data(16384);
	
	//open file
	std::string filepath = deshi::dirSaves() + filename;
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
	if(!file.is_open()){ ERROR("Failed to open file '", filepath, "' when trying to save"); return; }
	
	SaveHeader header;
	file.write((const char*)&header, sizeof(SaveHeader)); //fill header
	header.magic = 1213416772; //DESH
	header.flags = 0;
	
	//// entities ////
	header.entityCount       = entities.size();
	header.entityArrayOffset = sizeof(SaveHeader);
	
	//store sorted components and write entities
	header.componentTypeCount = 10;
	std::vector<AudioListener*>  compsAudioListener;
	std::vector<AudioSource*>    compsAudioSource;
	std::vector<BoxCollider*>    compsColliderBox;
	std::vector<AABBCollider*>   compsColliderAABB;
	std::vector<SphereCollider*> compsColliderSphere;
	std::vector<Light*>          compsLight;
	std::vector<MeshComp*>       compsMeshComp;
	std::vector<Physics*>        compsPhysics;
	std::vector<Movement*>       compsMovement;
	std::vector<Player*>         compsPlayer;
	//TODO(delle,Cl) convert these vectors to char vectors and when iterating thru entities
	// and thier components, call the save function of an entity to add to the components
	// vector and then use the final size of that vector for type header offsets
	//Or, loop thru layers
	
	for(Entity* e : entities) {
		//write entity
		file.write((const char*)&e->id,                 sizeof(u32));
		file.write(e->name,                             sizeof(char)*64);
		file.write((const char*)&e->transform.position, sizeof(Vector3));
		file.write((const char*)&e->transform.rotation, sizeof(Vector3));
		file.write((const char*)&e->transform.scale,    sizeof(Vector3));
		
		//sort components
		for (Component* c : e->components) {
			switch (c->comptype) {
				case ComponentType_Physics:       compsPhysics.push_back(dyncasta(Physics, c)); break;
				case ComponentType_Collider: {
					dyncast(col, Collider, c);
					switch (col->type) {
						case ColliderType_Box:    compsColliderBox.push_back(dyncasta(BoxCollider, col)); break;
						case ColliderType_AABB:   compsColliderAABB.push_back(dyncasta(AABBCollider, col)); break;
						case ColliderType_Sphere: compsColliderSphere.push_back(dyncasta(SphereCollider, col)); break;
					}
					break;
				}
				case ComponentType_AudioListener: compsAudioListener.push_back(dyncasta(AudioListener, c)); break;
				case ComponentType_AudioSource:   compsAudioSource.push_back(dyncasta(AudioSource, c)); break;
				case ComponentType_Light:         compsLight.push_back(dyncasta(Light, c)); break;
				case ComponentType_OrbManager:    /*TODO(sushi) impl orb saving*/ break;
				case ComponentType_Movement:      compsMovement.push_back(dyncasta(Movement, c)); break;
				case ComponentType_MeshComp:      compsMeshComp.push_back(dyncasta(MeshComp, c)); break;
				case ComponentType_Player:        compsPlayer.push_back(dyncasta(Player, c)); break;
			}
		}
	}
	
	//// write textures ////
	header.textureCount = DengRenderer->textures.size();
	header.textureArrayOffset = file.tellp();
	for(auto& t : DengRenderer->textures){
		file.write((const char*)&t.type, sizeof(u32));
		file.write(t.filename,           sizeof(char)*64);
	}
	
	//// write materials ////
	header.materialCount = DengRenderer->materials.size();
	header.materialArrayOffset = file.tellp();
	for(auto& m : DengRenderer->materials){
		file.write((const char*)&m.shader,     sizeof(u32));
		file.write((const char*)&m.albedoID,   sizeof(u32));
		file.write((const char*)&m.normalID,   sizeof(u32));
		file.write((const char*)&m.specularID, sizeof(u32));
		file.write((const char*)&m.lightID,    sizeof(u32));
		file.write(m.name,                     sizeof(char)*64);
	}
	
	//// write meshes //// //TODO(delle) support multiple materials per mesh
	header.meshCount = DengRenderer->meshes.size();
	header.meshArrayOffset = file.tellp();
	for(auto& m : DengRenderer->meshes){
		b32 base = m.base;
		file.write((const char*)&m.primitives[0].materialIndex, sizeof(u32));
		file.write((const char*)&base,                          sizeof(b32));
		file.write(m.name,                                      sizeof(char)*64);
	}
	
	//// write component type headers //// //TODO(delle) move these to thier respective files
	header.componentTypeHeaderArrayOffset = file.tellp();
	ComponentTypeHeader typeHeader;
	
	//audio listener 0
	typeHeader.type        = ComponentType_AudioListener;
	typeHeader.arrayOffset = header.componentTypeHeaderArrayOffset + sizeof(ComponentTypeHeader) * header.componentTypeCount;
	typeHeader.size        = sizeof(u32) * 3 + sizeof(Vector3)*3;
	typeHeader.count       = compsAudioListener.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//audio source 1
	typeHeader.type        = ComponentType_AudioSource;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) * 3 + 0; //TODO(sushi) tell delle what data is important to save on a source
	typeHeader.count       = compsAudioSource.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//collider box 2
	typeHeader.type        = ComponentType_ColliderBox;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) * 3 + sizeof(u32) + sizeof(Matrix3) + sizeof(Vector3);
	typeHeader.count       = compsColliderBox.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//collider aabb 3
	typeHeader.type        = ComponentType_ColliderAABB;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) * 3 + sizeof(u32) + sizeof(Matrix3) + sizeof(Vector3);
	typeHeader.count       = compsColliderAABB.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//collider sphere 4
	typeHeader.type        = ComponentType_ColliderSphere;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) * 3 + sizeof(u32) + sizeof(Matrix3) + sizeof(float);
	typeHeader.count       = compsColliderSphere.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//light 5
	typeHeader.type        = ComponentType_Light;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) * 3 + sizeof(Vector3)*2 + sizeof(float);
	typeHeader.count       = compsLight.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//mesh comp 6
	typeHeader.type        = ComponentType_MeshComp;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) * 3 + sizeof(u32)*2 + sizeof(b32)*2; //instanceID, meshID, visible, entity_control
	typeHeader.count       = compsMeshComp.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//physics 7
	typeHeader.type        = ComponentType_Physics;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) * 3 + sizeof(Vector3)*6 + sizeof(float)*2 + sizeof(b32) * 3 + sizeof(float) * 2;
	typeHeader.count       = compsPhysics.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//NOTE sushi: this is kind of scuffed because movement has a pointer to physics, and player
	// 			  has a pointer to movement so they have to be loaded in this order in order
	//			  for movement, physics, and player to find the pointers they need on their entity
	// 			  this should probably be done better at some point :). maybe it is already idk
	//movement 8
	typeHeader.type        = ComponentType_Movement;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) * 3 + sizeof(Vector3) + sizeof(float) * 6 + sizeof(b32);
	typeHeader.count       = compsMovement.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//player 9
	typeHeader.type        = ComponentType_Player;
	typeHeader.arrayOffset = typeHeader.arrayOffset + typeHeader.size * typeHeader.count;
	typeHeader.size        = sizeof(u32) * 3 + sizeof(int);
	typeHeader.count       = compsPlayer.size();
	file.write((const char*)&typeHeader, sizeof(ComponentTypeHeader));
	
	//// write components ////
	
	//audio listener
	for(auto c : compsAudioListener){
		file.write((const char*)&c->entityID,    sizeof(u32));
		file.write((const char*)&c->compID,      sizeof(u32));
		file.write((const char*)&c->event,       sizeof(u32));
		file.write((const char*)&c->position,    sizeof(Vector3));
		file.write((const char*)&c->velocity,    sizeof(Vector3));
		file.write((const char*)&c->orientation, sizeof(Vector3));
	}
	
	//audio source
	for(auto c : compsAudioSource){
		file.write((const char*)&c->entityID, sizeof(u32));
		file.write((const char*)&c->compID,   sizeof(u32));
		file.write((const char*)&c->event,    sizeof(u32));
		
	}
	
	//collider box
	for(auto c : compsColliderBox){
		file.write((const char*)&c->entityID,       sizeof(u32));
		file.write((const char*)&c->compID,         sizeof(u32));
		file.write((const char*)&c->event,          sizeof(u32));
		file.write((const char*)&c->collisionLayer, sizeof(u32));
		file.write((const char*)&c->inertiaTensor,  sizeof(Matrix3));
		file.write((const char*)&c->halfDims,       sizeof(Vector3));
	}
	
	//collider aabb
	for(auto c : compsColliderAABB){
		file.write((const char*)&c->entityID,       sizeof(u32));
		file.write((const char*)&c->compID,         sizeof(u32));
		file.write((const char*)&c->event,          sizeof(u32));
		file.write((const char*)&c->collisionLayer, sizeof(u32));
		file.write((const char*)&c->inertiaTensor,  sizeof(Matrix3));
		file.write((const char*)&c->halfDims,       sizeof(Vector3));
	}
	
	//collider sphere
	for(auto c : compsColliderSphere){
		file.write((const char*)&c->entityID,       sizeof(u32));
		file.write((const char*)&c->compID,         sizeof(u32));
		file.write((const char*)&c->event,          sizeof(u32));
		file.write((const char*)&c->collisionLayer, sizeof(u32));
		file.write((const char*)&c->inertiaTensor,  sizeof(Matrix3));
		file.write((const char*)&c->radius,         sizeof(float));
	}
	
	//light
	for(auto c : compsLight){
		file.write((const char*)&c->entityID,    sizeof(u32));
		file.write((const char*)&c->compID,      sizeof(u32));
		file.write((const char*)&c->event,       sizeof(u32));
		file.write((const char*)&c->position,    sizeof(Vector3));
		file.write((const char*)&c->direction,   sizeof(Vector3));
		file.write((const char*)&c->brightness,  sizeof(float));
	}
	
	//mesh comp
	for(auto c : compsMeshComp){
		b32 bool1 = c->mesh_visible;
		b32 bool2 = c->ENTITY_CONTROL;
		file.write((const char*)&c->entityID,   sizeof(u32));
		file.write((const char*)&c->compID,     sizeof(u32));
		file.write((const char*)&c->event,      sizeof(u32));
		file.write((const char*)&c->instanceID, sizeof(u32));
		file.write((const char*)&c->meshID,     sizeof(u32));
		file.write((const char*)&bool1,         sizeof(b32));
		file.write((const char*)&bool2,         sizeof(b32));
	}
	
	//physics
	for(auto c : compsPhysics){
		b32 isStatic = c->isStatic;
		b32 staticRotation = c->staticRotation;
		b32 twoDphys = c->twoDphys;
		file.write((const char*)&c->entityID,        sizeof(u32));
		file.write((const char*)&c->compID,          sizeof(u32));
		file.write((const char*)&c->event,           sizeof(u32));
		file.write((const char*)&c->position,        sizeof(Vector3));
		file.write((const char*)&c->rotation,        sizeof(Vector3));
		file.write((const char*)&c->velocity,        sizeof(Vector3));
		file.write((const char*)&c->acceleration,    sizeof(Vector3));
		file.write((const char*)&c->rotVelocity,     sizeof(Vector3));
		file.write((const char*)&c->rotAcceleration, sizeof(Vector3));
		file.write((const char*)&c->elasticity,      sizeof(float));
		file.write((const char*)&c->mass,            sizeof(float));
		file.write((const char*)&isStatic,           sizeof(b32));
		file.write((const char*)&staticRotation,     sizeof(b32));
		file.write((const char*)&twoDphys,           sizeof(b32));
		file.write((const char*)&c->kineticFricCoef, sizeof(float));
		file.write((const char*)&c->staticFricCoef,  sizeof(float));
	}
	
	//movement
	for (auto c : compsMovement) {
		b32 jump = c->jump;
		file.write((const char*)&c->entityID,          sizeof(u32));
		file.write((const char*)&c->compID,            sizeof(u32));
		file.write((const char*)&c->event,             sizeof(u32));
		file.write((const char*)&c->inputs,            sizeof(Vector3));
		file.write((const char*)&c->gndAccel,          sizeof(float));
		file.write((const char*)&c->airAccel,          sizeof(float));
		file.write((const char*)&c->maxWalkingSpeed,   sizeof(float));
		file.write((const char*)&c->maxRunningSpeed,   sizeof(float));
		file.write((const char*)&c->maxCrouchingSpeed, sizeof(float));
		file.write((const char*)&jump,                 sizeof(b32));
		file.write((const char*)&c->jumpImpulse,       sizeof(float));
	}
	
	//player
	for(auto c : compsPlayer){
		file.write((const char*)&c->entityID, sizeof(u32));
		file.write((const char*)&c->compID,   sizeof(u32));
		file.write((const char*)&c->event,    sizeof(u32));
		file.write((const char*)&c->health,   sizeof(int));
	}
	
	//finish header
	file.seekp(0);
	file.write((const char*)&header, sizeof(SaveHeader));
	
	//// close file ////
	file.close();
	SUCCESS("Successfully saved to ", filename);
}

void EntityAdmin::LoadDESH(const char* filename) {
	Reset();
	LOG("Loading level: ", deshi::dirSaves() + filename);
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
	Transform entTrans{}; char entName[64];
	for_n(i, header.entityCount){
		cursor += sizeof(u32); //skipped
		memcpy(entName,   data+cursor, sizeof(char)*64); cursor += sizeof(char)*64;
		memcpy(&entTrans, data+cursor, sizeof(vec3)*3);  cursor += sizeof(vec3)*3;
		entities.push_back(new Entity(this, entities.size(), entTrans, entName, {}));
	}
	
	//// parse and load textures ////
	if(cursor != header.textureArrayOffset) {
		return ERROR("Load failed because cursor was at '", cursor, 
					 "' when reading textures which start at '", header.textureArrayOffset, "'");
	}
	Texture tex{};
	for_n(i, header.textureCount){
		memcpy(&tex.type,    data+cursor, sizeof(u32));     cursor += sizeof(u32);
		memcpy(tex.filename, data+cursor, sizeof(char)*64); cursor += sizeof(char)*64;
		if(i>3) DengRenderer->LoadTexture(tex);
	}
	
	//// parse and create materials ////
	if(cursor != header.materialArrayOffset) {
		return ERROR("Load failed because cursor was at '", cursor, 
					 "' when reading materials which start at '", header.materialArrayOffset, "'");
	}
	u32 shader = 0, albedoID = 0, normalID = 2, specularId = 2, lightID = 2; char matName[64];
	for_n(i, header.materialCount){
		memcpy(&shader,     data+cursor, sizeof(u32));     cursor += sizeof(u32);
		memcpy(&albedoID,   data+cursor, sizeof(u32));     cursor += sizeof(u32);
		memcpy(&normalID,   data+cursor, sizeof(u32));     cursor += sizeof(u32);
		memcpy(&specularId, data+cursor, sizeof(u32));     cursor += sizeof(u32);
		memcpy(&lightID,    data+cursor, sizeof(u32));     cursor += sizeof(u32);
		memcpy(matName,     data+cursor, sizeof(char)*64); cursor += sizeof(char)*64;
		if(i>5) DengRenderer->CreateMaterial(shader, albedoID, normalID, specularId, lightID, matName);
	}
	
	//// parse and load/create meshes ////
	if(cursor != header.meshArrayOffset) {
		return ERROR("Load failed because cursor was at '", cursor, 
					 "' when reading meshes which start at '", header.meshArrayOffset, "'");
	}
	b32 matID = 0, baseMesh = 0; char meshName[64];
	for_n(i, header.meshCount){
		memcpy(&matID,    data+cursor, sizeof(u32));     cursor += sizeof(u32);
		memcpy(&baseMesh, data+cursor, sizeof(b32));     cursor += sizeof(b32);
		memcpy(meshName,  data+cursor, sizeof(char)*64); cursor += sizeof(char)*64;
		if(!baseMesh) {
			u32 meshID = DengRenderer->CreateMesh(&scene, meshName);
			DengRenderer->UpdateMeshBatchMaterial(meshID, 0, matID);
		}
	}
	
	//// parse and create components ////
	if(cursor != header.componentTypeHeaderArrayOffset) {
		return ERROR("Load failed because cursor was at '", cursor, 
					 "' when reading component headers which start at '", header.componentTypeHeaderArrayOffset, "'");
	}
	
	ComponentTypeHeader compHeader;
	for_n(i, header.componentTypeCount){
		cursor = header.componentTypeHeaderArrayOffset + (sizeof(u32)*4)*i;
		memcpy(&compHeader, data+cursor, sizeof(u32)*4);
		cursor = compHeader.arrayOffset;
		
		switch(compHeader.type){
			case(ComponentType_AudioListener):  AudioListener ::LoadDESH(this, data, cursor, compHeader.count); break;
			case(ComponentType_AudioSource):    AudioSource   ::LoadDESH(this, data, cursor, compHeader.count); break;
			case(ComponentType_Camera):         Camera        ::LoadDESH(this, data, cursor, compHeader.count); break;
			case(ComponentType_ColliderBox):    BoxCollider   ::LoadDESH(this, data, cursor, compHeader.count); break;
			case(ComponentType_ColliderAABB):   AABBCollider  ::LoadDESH(this, data, cursor, compHeader.count); break;
			case(ComponentType_ColliderSphere): SphereCollider::LoadDESH(this, data, cursor, compHeader.count); break;
			case(ComponentType_Light):          Light         ::LoadDESH(this, data, cursor, compHeader.count); break;
			case(ComponentType_MeshComp):       MeshComp      ::LoadDESH(this, data, cursor, compHeader.count); break;
			case(ComponentType_OrbManager):     OrbManager    ::LoadDESH(this, data, cursor, compHeader.count); break;
			case(ComponentType_Physics):        Physics       ::LoadDESH(this, data, cursor, compHeader.count); break;
			case(ComponentType_Movement):       Movement      ::LoadDESH(this, data, cursor, compHeader.count); break;
			case(ComponentType_Player):         Player        ::LoadDESH(this, data, cursor, compHeader.count); break;
			default:{
				ERROR("Failed to load a component array because of unknown component type '", 
					  compHeader.type, "' at pos: ", cursor);
			}break;
		}
	}
	
	SUCCESS("Finished loading level '", filename, "' in ", TIMER_END(t_l), "ms");
	SkipUpdate();
}





