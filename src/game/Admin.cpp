#include "Admin.h"
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
#include "../core/time.h"
#include "../core/renderer.h"
#include "../core/window.h"
#include "../core/assets.h"
#include "../utils/utils.h"

#include <iostream>
#include <fstream>
#include <utility>
#include <stdexcept>
#include <filesystem>

TIMER_START(t_a);

void Admin::Init() {
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
    forI(ComponentLayer_World+1) freeCompLayers.push_back(ContainerManager<Component*>());
    
    //init singletons
    physics.Init(this);
    canvas.Init(this);
    sound.Init(this);
    scene.Init();
    Render::LoadScene(&scene);
    keybinds.init();
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

void Admin::Cleanup() {
    SaveDESH((state == GameState_Editor) ? "temp.desh" : "auto.desh");
	keybinds.save();
	editor.Cleanup();
}

void UpdateLayer(ContainerManager<Component*> cl) {
    forI(cl.size()) {
        if (cl[i]) {
            cl[i].value->Update();
        }
    }
}

void Admin::Update() {
    ImGui::BeginDebugLayer();
    if(!skip && (state == GameState_Editor || state == GameState_Debug)) editor.Update();
    if(!skip) controller.Update();
    if(!skip) mainCamera->Update();
    
    //NOTE sushi: we need to maybe make a pause_phys_layer thing, because things unrelated to physics in that layer arent getting updated in editor. eg. lights
    //			  or we can just have different update blocks for different game states
    TIMER_RESET(t_a); 
    if(!skip && /*!pause_phys &&*/ !paused)  { UpdateLayer(freeCompLayers[ComponentLayer_Physics]); }
    physLyrTime =   TIMER_END(t_a); TIMER_RESET(t_a);
    if(!skip && !pause_phys && !paused) { physics.Update(); }
    physSysTime =   TIMER_END(t_a); TIMER_RESET(t_a);
    if(!skip && !pause_canvas)          { UpdateLayer(freeCompLayers[ComponentLayer_Canvas]); }
    canvasLyrTime = TIMER_END(t_a); TIMER_RESET(t_a);
    if(!skip && !pause_canvas)          { canvas.Update(); }
    canvasSysTime = TIMER_END(t_a); TIMER_RESET(t_a);
    if(!skip && !pause_sound && !paused){ UpdateLayer(freeCompLayers[ComponentLayer_Sound]); }
    sndLyrTime =    TIMER_END(t_a); TIMER_RESET(t_a);
    if(!skip && !pause_sound && !paused){ sound.Update(); }
    sndSysTime =    TIMER_END(t_a);
    ImGui::EndDebugLayer();
}

void Admin::PostRenderUpdate(){ //no imgui stuff allowed b/c rendering already happened
    TIMER_RESET(t_a);
    if (!skip && !pause_world) UpdateLayer(freeCompLayers[ComponentLayer_World]); 
    worldLyrTime = TIMER_END(t_a); TIMER_RESET(t_a);
    
    //deletion buffer
    for(Entity* e : deletionBuffer) {
        for(Component* c : e->components){
			freeCompLayers[c->layer].remove_from(c->layer_index);
			if(c->comptype == ComponentType_Light) scene.lights.clear();
		}
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
            if (c->comptype == ComponentType_Light) scene.lights.push_back(dyncast(Light, c));
            compIDcount++;
        }
    }
    creationBuffer.clear();
    worldSysTime = TIMER_END(t_a); TIMER_RESET(t_a);
    
    //light updating
    for (int i = 0; i < 10; i++) {
        if(i < scene.lights.size()) {
			Render::UpdateLight(i, vec4(scene.lights[i]->position,
										(scene.lights[i]->active) ? scene.lights[i]->brightness : 0));
        }else{
            Render::UpdateLight(i, vec4(0, 0, 0, -1));
        }
    }
    
    //compIDcount = 0;
    //for (Entity* e : entities) compIDcount += e->components.size();
    
    
    DengTime->paused = paused;
    DengTime->phys_pause = pause_phys;
    skip = false;
}

u32 Admin::CreateEntity(const char* name) {
    u32 id = entities.size() + creationBuffer.size() - 1;
    creationBuffer.push_back(new Entity(this, id, Transform(), name));
    return id;
}

u32 Admin::CreateEntity(std::vector<Component*> components, const char* name, Transform transform) {
    u32 id = entities.size() + creationBuffer.size() - 1;
    creationBuffer.push_back(new Entity(this, id, transform, name, components));
    return id;
}

u32 Admin::CreateEntity(Entity* e) {
    if(!e) return -1;
	
    e->admin = this;
    creationBuffer.push_back(e);
    return entities.size() + creationBuffer.size() - 1;
}

Entity* Admin::CreateEntityNow(std::vector<Component*> components, const char* name, Transform transform) {
    Entity* e = new Entity(this, entities.size(), transform, name, components);
    entities.push_back(e);
    for (Component* c : e->components) {
        c->entityID = e->id;
        c->compID = compIDcount;
        c->entity = e;
        c->layer_index = freeCompLayers[c->layer].add(c);
        if (c->comptype == ComponentType_Light) scene.lights.push_back(dyncast(Light, c));
        compIDcount++;
    }
    return e;
}

void Admin::DeleteEntity(u32 id) {
    if(id < entities.size()){
        deletionBuffer.push_back(entities[id]);
    }else{
        ERROR("Attempted to add entity '", id, "' to deletion buffer when it doesn't exist on the admin");
    }
}

void Admin::DeleteEntity(Entity* e) {
    if(e && e->id < entities.size()){
        deletionBuffer.push_back(e);
    }else{
        ERROR("Attempted to add entity '", e->id, "' to deletion buffer when it doesn't exist on the admin");
    }
}

void Admin::RemoveButDontDeleteEntity(Entity* e){
	if(!e) return;
	
	for(Component* c : e->components) freeCompLayers[c->layer].remove_from(c->layer_index);
	for(int i = e->id+1; i < entities.size(); ++i) entities[i]->id -= 1;
	entities.erase(entities.begin()+e->id);
	if(e == player) player = nullptr;
}

void Admin::AddComponentToLayers(Component* c){
    if(!c) return;
	
    c->compID = compIDcount;
    c->layer_index = freeCompLayers[c->layer].add(c);
    if(c->comptype == ComponentType_Light) scene.lights.push_back(dyncast(Light, c));
    compIDcount++;
}

void Admin::ChangeState(GameState new_state){
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
                controller.playermove = nullptr;
                SaveDESH("auto.desh");
                //LoadDESH("temp.desh");
                LoadTEXT(editor.level_name);
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
                //LoadDESH("temp.desh");
                LoadTEXT(editor.level_name);
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
                //SaveDESH("temp.desh");
                SaveTEXT(editor.level_name);
                if(player) {
                    player->GetComponent<MeshComp>()->Visible(false);
                }else{
                    ERROR("No player on admin");
                }
                DengWindow->UpdateCursorMode(CursorMode::FIRSTPERSON);
            }break;
            case GameState_Menu:{   to = "MENU";
                //SaveDESH("save.desh");
                SaveTEXT(editor.level_name);
                DengWindow->UpdateCursorMode(CursorMode::DEFAULT);
            }break;
        }break;
    }
    state = new_state;
    SUCCESS("Changed gamestate from ", from, " to ", to);
}

void Admin::Reset(){
    SUCCESS("Resetting admin");
    TIMER_START(t_r);
    for(Entity* e : entities) delete e;
    entities.clear();
    
    for(auto& layer : freeCompLayers) layer.clear();
    scene.Reset();
    Render::Reset();
    Render::LoadScene(&scene);
    editor.Reset();
    SUCCESS("Finished resetting admin in ", TIMER_END(t_r), "ms");
}

//NOTE using a levels directory temporarily so it doesnt cause problems with the save directory
//TODO(delle) this removes the entire level dir and recreates it, optimize by diffing for speed and comment preservation
//TODO add safe-checking so you dont override another level accidentally
//TODO maybe dont save materials that aren't being used
void Admin::SaveTEXT(std::string level_name){
    namespace fs = std::filesystem;
    if(level_name.empty()) return ERROR("Failed to create save text-file: no name passed");
    SUCCESS("Started saving level '", level_name, "'");
    TIMER_START(t_s);
    
    //// setup the level directory ////
    std::string levels_dir = Assets::dirData() + "levels/";
    if(!fs::is_directory(levels_dir)) fs::create_directory(levels_dir);
    
    std::string level_dir = levels_dir + std::string(level_name) + "/";
    if(fs::is_directory(level_dir)) fs::remove_all(level_dir);
    fs::create_directory(level_dir);
    SUCCESS("  Created level directory '", level_dir, "'");
    
    //// level file ////
    std::string level_text; level_text.reserve(2048);
    level_text.append(TOSTRING(">level"
                               "\nname         \"", level_name, "\""
                               "\nentity_count ", entities.size(),
							   "\nlast_updated \"", /*DengTime->FormatDateTime("{M}/{d}/{y} {h}:{m}:{s}"),*/ "\""));
	//NOTE temp disable on last_updated until diff checking is setup
	
	//materials
	level_text.append("\n"
					  "\n>materials");
	for(MaterialVk& mat : *Render::materialArray()){
		level_text.append(TOSTRING("\n",mat.id," \"",mat.name,"\" ",mat.shader," \"",
								   (*Render::textureArray())[mat.albedoID].filename,"\" \"",
								   (*Render::textureArray())[mat.normalID].filename,"\" \"",
								   (*Render::textureArray())[mat.specularID].filename,"\" \"",
								   (*Render::textureArray())[mat.lightID].filename,"\""));
	}
	
	//models
	level_text.append("\n"
					  "\n>meshes");
	for(MeshVk& mesh : *Render::meshArray()){
		if(!mesh.base){
			level_text.append(TOSTRING("\n",mesh.id," \"",mesh.name,"\" ",mesh.visible," \"", mesh.primitives[0].materialIndex));
			for(u32 i=1; i<mesh.primitives.size(); ++i){ level_text.append(TOSTRING(" ", mesh.primitives[i].materialIndex)); }
			level_text.append("\"");
		}
	}
	
	//entities
	level_text.append("\n"
					  "\n>entities");
	forI(entities.size()){
		level_text.append(TOSTRING("\n",i," \"",entities[i]->name,"\""));
	}
	
	//events
	level_text.append("\n"
					  "\n>events");
	for(Entity* e : entities){
		for(Component* c : e->components){
			if(c->sender && c->sender->receivers.size() > 0){
				for(Receiver* r : c->sender->receivers){
					if(Component* comp = dynamic_cast<Component*>(r)){
						level_text.append(TOSTRING("\n",e->id," \"",e->name,"\" ",c->comptype," ",c->event," -> ",
												   comp->entity->id," \"",comp->entity->name,"\" ",comp->comptype));
					}
				}
			}
		}
	}
	
	//// entities files ////
	u32   entity_idx_digits   = ((u32)log10(entities.size())) + 1;
	u32   entity_idx_str_size = sizeof(char) * (entity_idx_digits + 2); //+2 because of _ and null-terminator
	char* entity_idx_str      = (char*)malloc(entity_idx_str_size);
	defer{ free(entity_idx_str); };
	std::string entity_file_name;
	
	forX(idx, entities.size()){
		snprintf(entity_idx_str, entity_idx_str_size, "%0*u_", entity_idx_digits, idx);
		entity_file_name = TOSTRING(entity_idx_str, entities[idx]->name);
		
		std::string entity_text = entities[idx]->SaveTEXT();
		Assets::writeFile(level_dir + entity_file_name, entity_text.c_str(), entity_text.size());
		SUCCESS("  Created entity file '", entity_file_name, "'");
	}
	
	level_text.append("\n");
	Assets::writeFile(level_dir + "_", level_text.c_str(), level_text.size());
	SUCCESS("  Created level file '_'");
	
	SUCCESS("Finished saving level '", level_name, "' in ", TIMER_END(t_s), "ms");
}

#define ParsingError "Error parsing '",level_dir,"_' on line '",line_number
enum struct LevelHeader{
	INVALID, LEVEL, MATERIALS, MESHES, ENTITIES, EVENTS
};
void Admin::LoadTEXT(std::string savename){
	namespace fs = std::filesystem;
	
	if(savename.empty()) return ERROR("Failed to load text-file: no name passed");
	std::string levels_dir = Assets::dirData() + "levels/";
	std::string level_dir = levels_dir + savename + "/";
	if(!fs::is_directory(level_dir)) return ERROR("Failed to find directory: ", level_dir);
	
	Reset();
	SUCCESS("Loading level: ", savename);
	TIMER_START(t_l);
	
	u32 entity_count = 0;
	std::vector<pair<u32,u32>> material_id_diffs; //old_id, new_id
	std::vector<pair<u32,u32>> mesh_id_diffs; //old_id, new_id
	std::vector<pair<u32,u32,u32,u32,u32>> events; //send_ent_id, send_comp_type, event_type, receive_ent_id, receive_comp_type
	{//// parse level file ////
		char* buffer = Assets::readFileAsciiToArray(level_dir+"_");
		if(!buffer) return;
		defer{ delete[] buffer; };
		
		//parsing
		LevelHeader header = LevelHeader::INVALID;
		std::string line;
		char* new_line = buffer-1;
		char* line_start;
		for(u32 line_number = 1; ;line_number++){
			//get the next line
			line_start = new_line+1;
			if((new_line = strchr(line_start, '\n')) == 0) break; //end of file if cant find '\n'
			line = std::string(line_start, new_line-line_start);
			
			line = Utils::eatComments(line, "#");
			line = Utils::eatSpacesLeading(line);
			line = Utils::eatSpacesTrailing(line);
			if(line.empty()) continue;
			
			//headers
			if(line[0] == '>'){
				if     (line == ">level")    { header = LevelHeader::LEVEL;     }
				else if(line == ">materials"){ header = LevelHeader::MATERIALS; }
				else if(line == ">meshes")   { header = LevelHeader::MESHES;    }
				else if(line == ">entities") { header = LevelHeader::ENTITIES;  }
				else if(line == ">events")   { header = LevelHeader::EVENTS;  }
				else{
					header = LevelHeader::INVALID;
					ERROR(ParsingError,"'! Unknown header: ", line);
				}
				continue;
			}
			
			//header values (skip if an invalid header)
			if(header == LevelHeader::INVALID) { ERROR(ParsingError,"'! Invalid header; skipping line"); continue; }
			std::vector<std::string> split = Utils::spaceDelimitIgnoreStrings(line);
			
			switch(header){
				case(LevelHeader::LEVEL):{
					if(split.size() != 2){ ERROR(ParsingError,"'! Level lines should have 2 values"); continue; }
					
					if(split[0] == "name"){ editor.level_name = split[1]; }
					//TODO maybe compare entity_count with the number of entities loaded?
				}break;
				case(LevelHeader::MATERIALS):{
					if(split.size() != 7){ ERROR(ParsingError,"'! Material lines should have 7 values"); continue; }
					
					u32 old_id = std::stoi(split[0]);
					u32 new_id = Render::CreateMaterial(split[1].c_str(), std::stoi(split[2]), 
														Render::LoadTexture(split[3].c_str()),
														Render::LoadTexture(split[4].c_str()),
														Render::LoadTexture(split[5].c_str()),
														Render::LoadTexture(split[6].c_str()));
					material_id_diffs.push_back(pair<u32,u32>(old_id,new_id));
				}break;
				case(LevelHeader::MESHES):{
					if(split.size() != 4){ ERROR(ParsingError,"'! Mesh lines should have 4 values"); continue; }
					
					//id
					u32 old_id = std::stoi(split[0]);
					u32 new_id = Render::CreateMesh(&scene, split[1].c_str(), false);
					mesh_id_diffs.push_back(pair<u32,u32>(old_id,new_id));
					
					//visible
					(*Render::meshArray())[new_id].visible = Assets::parse_bool(split[2], level_dir.c_str(), line_number);
					
					//materials
					std::vector<std::string> mat_ids = Utils::spaceDelimit(split[3]); 
					forI((*Render::meshArray())[new_id].primitives.size()){
						u32 old_mat = std::stoi(mat_ids[i]);
						for(auto& diff : material_id_diffs){
							if(diff.first == old_mat){
								(*Render::meshArray())[new_id].primitives[i].materialIndex = diff.second;
							}
						}
					}
				}break;
				case(LevelHeader::ENTITIES):{
					entity_count += 1;
					//TODO maybe can conditionally load entities?
				}break;
				case(LevelHeader::EVENTS):{
					if(split.size() != 8){ ERROR(ParsingError,"'! Material lines should have 8 values"); continue; }
					
					u32 send_ent_id = std::stoi(split[0]);
					std::string send_ent_name = split[1];
					u32 send_comp_type = std::stoi(split[2]);
					u32 event_type = std::stoi(split[3]);
					//dummy arrow split[4]
					u32 rec_ent_id = std::stoi(split[5]);
					std::string rec_ent_name = split[6];
					u32 rec_comp_type = std::stoi(split[7]);
					
					events.push_back(pair<u32,u32,u32,u32,u32>(send_ent_id,send_comp_type,event_type,rec_ent_id,rec_comp_type));
				}break;
			}
		}
	}
	
	//// parse entity files ////
	std::vector<Entity*> ents; ents.reserve(entity_count);
	for(std::string& file : Assets::iterateDirectory(level_dir)){
		if(file == "_") continue;
		if(Entity* e = Entity::LoadTEXT(this, level_dir+file, mesh_id_diffs)){
			ents.push_back(e);
		}
	}
	
	//// events ////
	forI(events.size()){
		//find receiving component
		Component* rec_comp = 0;
		for(Entity* e : ents){
			if(e->id == events[i].fourth){
				for(Component* c : e->components){
					if(c->comptype == events[i].fifth){
						rec_comp = c;
						break;
					}
				}
			}
		}
		if(!rec_comp){
			ERROR("Unable to find component for event: ", i);
			break;
		}
		
		//add event and receiver to sender component
		for(Entity* e : ents){
			if(e->id == events[i].first){
				for(Component* c : e->components){
					if(c->comptype == events[i].second){
						c->event = events[i].third;
						if(!c->sender) c->sender = new Sender;
						c->sender->AddReceiver(rec_comp);
						rec_comp->entity->connections.insert(e);
						SUCCESS("Added event '",EventStrings[events[i].third],"': ",
								e->name," ",c->comptype," -> ",rec_comp->entity->name," ",rec_comp->comptype);
					}
				}
			}
		}
	}
	
	//// create entities ////
	for(Entity* e : ents){
		CreateEntity(e);
	}
	
	SUCCESS("Finished loading level '", savename, "' in ", TIMER_END(t_l), "ms");
	SkipUpdate();
}
#undef ParsingError

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

void Admin::SaveDESH(const char* filename) {
	//std::vector<char> save_data(16384);
	
	//open file
	std::string filepath = Assets::dirSaves() + filename;
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
		file.write(e->name,                             sizeof(char)*DESHI_NAME_SIZE);
		file.write((const char*)&e->transform.position, sizeof(Vector3));
		file.write((const char*)&e->transform.rotation, sizeof(Vector3));
		file.write((const char*)&e->transform.scale,    sizeof(Vector3));
		
		//sort components
		for (Component* c : e->components) {
			switch (c->comptype) {
				case ComponentType_Physics:       compsPhysics.push_back(dyncast(Physics, c)); break;
				case ComponentType_Collider: {
					Collider* col = dyncast(Collider, c);
					switch (col->type) {
						case ColliderType_Box:    compsColliderBox.push_back(dyncast(BoxCollider, col)); break;
						case ColliderType_AABB:   compsColliderAABB.push_back(dyncast(AABBCollider, col)); break;
						case ColliderType_Sphere: compsColliderSphere.push_back(dyncast(SphereCollider, col)); break;
					}
				} break;
				case ComponentType_AudioListener: compsAudioListener.push_back(dyncast(AudioListener, c)); break;
				case ComponentType_AudioSource:   compsAudioSource.push_back(dyncast(AudioSource, c)); break;
				case ComponentType_Light:         compsLight.push_back(dyncast(Light, c)); break;
				case ComponentType_OrbManager:    /*TODO(sushi) impl orb saving*/ break;
                case ComponentType_Movement:      compsMovement.push_back(dyncast(Movement, c)); break;
                case ComponentType_MeshComp:      compsMeshComp.push_back(dyncast(MeshComp, c)); break;
                case ComponentType_Player:        compsPlayer.push_back(dyncast(Player, c)); break;
            }
        }
    }
    
    //// write textures ////
    header.textureCount = Render::TextureCount();;
    header.textureArrayOffset = file.tellp();
    for(auto& t : *Render::textureArray()){
        file.write((const char*)&t.type, sizeof(u32));
        file.write(t.filename,           sizeof(char)*DESHI_NAME_SIZE);
    }
    
    //// write materials ////
    header.materialCount = Render::MaterialCount();
    header.materialArrayOffset = file.tellp();
    for(auto& m : *Render::materialArray()){
        file.write((const char*)&m.shader,     sizeof(u32));
        file.write((const char*)&m.albedoID,   sizeof(u32));
        file.write((const char*)&m.normalID,   sizeof(u32));
        file.write((const char*)&m.specularID, sizeof(u32));
        file.write((const char*)&m.lightID,    sizeof(u32));
        file.write(m.name,                     sizeof(char)*DESHI_NAME_SIZE);
    }
    
    //// write meshes //// //TODO(delle) support multiple materials per mesh
    header.meshCount = Render::MeshCount();
    header.meshArrayOffset = file.tellp();
    for(auto& m : *Render::meshArray()){
        b32 base = m.base;
        file.write((const char*)&m.primitives[0].materialIndex, sizeof(u32));
        file.write((const char*)&base,                          sizeof(b32));
        file.write(m.name,                                      sizeof(char)*DESHI_NAME_SIZE);
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
        b32 staticPosition = c->staticPosition;
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
        file.write((const char*)&staticPosition,           sizeof(b32));
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

void Admin::LoadDESH(const char* filename) {
    Reset();
    LOG("Loading level: ", Assets::dirSaves() + filename);
    TIMER_START(t_l);
    
    //// read file to char array ////
    u32 cursor = 0;
    std::vector<char> file = Assets::readFileBinary(Assets::dirSaves() + filename);
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
    Transform entTrans{}; char entName[DESHI_NAME_SIZE];
    forI(header.entityCount){
        cursor += sizeof(u32); //skipped
        memcpy(entName,   data+cursor, sizeof(char)*DESHI_NAME_SIZE); cursor += sizeof(char)*DESHI_NAME_SIZE;
        memcpy(&entTrans, data+cursor, sizeof(vec3)*3);  cursor += sizeof(vec3)*3;
        entities.push_back(new Entity(this, entities.size(), entTrans, entName, {}));
    }
    
    //// parse and load textures ////
    if(cursor != header.textureArrayOffset) {
        return ERROR("Load failed because cursor was at '", cursor, 
                     "' when reading textures which start at '", header.textureArrayOffset, "'");
    }
    Texture tex{};
    forI(header.textureCount){
        memcpy(&tex.type,    data+cursor, sizeof(u32));     cursor += sizeof(u32);
        memcpy(tex.filename, data+cursor, sizeof(char)*DESHI_NAME_SIZE); cursor += sizeof(char)*DESHI_NAME_SIZE;
        if(i>3) Render::LoadTexture(tex);
    }
    
    //// parse and create materials ////
    if(cursor != header.materialArrayOffset) {
        return ERROR("Load failed because cursor was at '", cursor, 
                     "' when reading materials which start at '", header.materialArrayOffset, "'");
    }
    u32 shader = 0, albedoID = 0, normalID = 2, specularId = 2, lightID = 2; char matName[DESHI_NAME_SIZE];
    forI(header.materialCount){
        memcpy(&shader,     data+cursor, sizeof(u32)); cursor += sizeof(u32);
        memcpy(&albedoID,   data+cursor, sizeof(u32)); cursor += sizeof(u32);
        memcpy(&normalID,   data+cursor, sizeof(u32)); cursor += sizeof(u32);
        memcpy(&specularId, data+cursor, sizeof(u32)); cursor += sizeof(u32);
        memcpy(&lightID,    data+cursor, sizeof(u32)); cursor += sizeof(u32);
        memcpy(matName,     data+cursor, sizeof(char)*DESHI_NAME_SIZE); cursor += sizeof(char)*DESHI_NAME_SIZE;
        if(i>5) Render::CreateMaterial(matName, shader, albedoID, normalID, specularId, lightID);
    }
    
    //// parse and load/create meshes ////
    if(cursor != header.meshArrayOffset) {
        return ERROR("Load failed because cursor was at '", cursor, 
                     "' when reading meshes which start at '", header.meshArrayOffset, "'");
    }
    b32 matID = 0, baseMesh = 0; char meshName[DESHI_NAME_SIZE];
    forI(header.meshCount){
        memcpy(&matID,    data+cursor, sizeof(u32));     cursor += sizeof(u32);
        memcpy(&baseMesh, data+cursor, sizeof(b32));     cursor += sizeof(b32);
        memcpy(meshName,  data+cursor, sizeof(char)*DESHI_NAME_SIZE); cursor += sizeof(char)*DESHI_NAME_SIZE;
        if(!baseMesh) {
            u32 meshID = Render::CreateMesh(&scene, meshName);
            Render::UpdateMeshBatchMaterial(meshID, 0, matID);
        }
    }
    
    //// parse and create components ////
    if(cursor != header.componentTypeHeaderArrayOffset) {
        return ERROR("Load failed because cursor was at '", cursor, 
                     "' when reading component headers which start at '", header.componentTypeHeaderArrayOffset, "'");
    }
    
    ComponentTypeHeader compHeader;
    forI(header.componentTypeCount){
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

//{P}:physics layer,  {C}:canvas layer,  {W}:world layer,  {S}:send layer
//{p}:physics system, {c}:canvas system, {w}:world system, {s}:send system, 
std::string Admin::FormatAdminTime(std::string fmt){
	std::string out = ""; out.reserve(512);
	forI(fmt.size()){
		if(fmt[i] == '{'){
			switch(fmt[i+1]){
				case('P'):{
					out.append(std::to_string(physLyrTime));
				}i+=2;continue;
				case('p'):{
					out.append(std::to_string(physSysTime));
				}i+=2;continue;
				case('C'):{
					out.append(std::to_string(canvasLyrTime));
				}i+=2;continue;
				case('c'):{
					out.append(std::to_string(canvasSysTime));
				}i+=2;continue;
				case('W'):{
					out.append(std::to_string(worldLyrTime));
				}i+=2;continue;
				case('w'):{
					out.append(std::to_string(worldSysTime));
				}i+=2;continue;
				case('S'):{
					out.append(std::to_string(sndLyrTime));
				}i+=2;continue;
				case('s'):{
					out.append(std::to_string(sndSysTime));
				}i+=2;continue;
			}
		}
		out.push_back(fmt[i]);
	}
	
	out.shrink_to_fit(); return out;
}
