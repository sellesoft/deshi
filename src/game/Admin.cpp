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
#include "../core/storage.h"
#include "../core/renderer.h"
#include "../core/window.h"
#include "../core/assets.h"
#include "../core/console.h"
#include "../utils/utils.h"
#include "../utils/debug.h"
#include "../utils/array.h"

#include <iostream>
#include <fstream>
#include <utility>
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


/////////////////
//// @update ////
/////////////////
void Admin::Update() {
	ImGui::BeginDebugLayer();
	TIMER_RESET(t_a);
    if(!skip && (state == GameState_Editor || state == GameState_Debug)) editor.Update();
	editorTime =    TIMER_END(t_a); TIMER_RESET(t_a);
    if(!skip) controller.Update();
    if(!skip) mainCamera->Update();
    
    //NOTE sushi: we need to maybe make a pause_phys_layer thing, because things unrelated to physics in 
	//    that layer arent getting updated in editor. eg. lights
    //    or we can just have different update blocks for different game states
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
			if(c->type == ComponentType_Light) DengStorage->lights.clear();
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
            c->compID = compIDcount;
            c->entity = e;
            c->layer_index = freeCompLayers[c->layer].add(c);
            if(c->type == ComponentType_Light) DengStorage->lights.add(dyncast(Light, c));
            compIDcount++;
        }
    }
    creationBuffer.clear();
    worldSysTime = TIMER_END(t_a); TIMER_RESET(t_a);
    
    //light updating
    for (int i = 0; i < 10; i++) {
        if(i < DengStorage->lights.size()) {
			Render::UpdateLight(i, vec4(DengStorage->lights[i]->position,
										(DengStorage->lights[i]->active) ? DengStorage->lights[i]->brightness : 0));
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


////////////////
//// @state ////
////////////////
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
                if(player) player->GetComponent<ModelInstance>()->visible = true;
                player = nullptr;
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
                if(player) player->GetComponent<ModelInstance>()->visible = true;
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
                    player->GetComponent<ModelInstance>()->visible = false;
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
    Storage::Reset();
    Render::Reset();
    editor.Reset();
    SUCCESS("Finished resetting admin in ", TIMER_END(t_r), "ms");
}

//{P}:physics layer,  {C}:canvas layer,  {W}:world layer,  {S}:send layer
//{p}:physics system, {c}:canvas system, {w}:world system, {s}:send system, 
//{e}:editor
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
				case('e'):{
					out.append(std::to_string(editorTime));
				}i+=2;continue;
			}
		}
		out.push_back(fmt[i]);
	}
	
	out.shrink_to_fit(); return out;
}


////////////////////////
//// @serialization ////
////////////////////////
//NOTE using a levels directory temporarily so it doesnt cause problems with the save directory
//TODO(delle) this removes the entire level dir and recreates it, optimize by diffing for speed and comment preservation
//TODO add safe-checking so you dont override another level accidentally
//TODO maybe dont save things that aren't being used
void Admin::SaveTEXT(std::string level_name){
    namespace fs = std::filesystem;
    if(level_name.empty()) return ERROR("Failed to create TEXT save: no name passed");
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
    level_text.append(TOSTDSTRING(">level"
								  "\nname         \"", level_name, "\""
								  "\nentity_count ", entities.size(),
								  "\nlast_updated \"", /*DengTime->FormatDateTime("{M}/{d}/{y} {h}:{m}:{s}"),*/ "\""));
	//NOTE temp disable on last_updated until diff checking is setup
	
	level_text.append("\n\n>meshes");
	for(u32 i = 1; i < Storage::MeshCount(); ++i){ //NOTE skipping first null mesh
		Mesh* mesh = Storage::MeshAt(i);
		level_text.append(TOSTDSTRING("\n\"",mesh->name,"\""));
	}
	
	level_text.append("\n\n>textures");
	for(u32 i = 1; i < Storage::TextureCount(); ++i){ //NOTE skipping first null mesh
		Texture* texture = Storage::TextureAt(i);
		bool gen_mipmaps = (texture->mipmaps > 1);
		level_text.append(TOSTDSTRING("\n\"",texture->name,"\" ",(texture->loaded)?"true":"false"," ",(gen_mipmaps)?"true":"false"));
	}
	
	level_text.append("\n\n>materials");
	for(int i = 1; i < Storage::MaterialCount(); ++i){ //NOTE skipping first null material
		Material* mat = Storage::MaterialAt(i);
		level_text.append(TOSTDSTRING("\n\"",mat->name,"\""));
	}
	
	level_text.append("\n\n>models");
	for(int i=1; i<DengStorage->models.count; ++i){ //NOTE skipping first null model
		Model* model = DengStorage->models[i];
		level_text.append(TOSTDSTRING("\n\"",model->name,"\""));
	}
	
	level_text.append("\n\n>entities");
	forI(entities.size()){
		level_text.append(TOSTDSTRING("\n",i," \"",entities[i]->name,"\""));
	}
	
	level_text.append("\n\n>events");
	for(Entity* e : entities){
		for(Component* c : e->components){
			if(c->sender.receivers.size() > 0){
				for(Receiver* r : c->sender.receivers){
					if(Component* comp = dynamic_cast<Component*>(r)){
						level_text.append(TOSTDSTRING("\n",e->id," \"",e->name,"\" ",c->type," ",c->event," -> ",
													  comp->entity->id," \"",comp->entity->name,"\" ",comp->type));
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
		entity_file_name = TOSTDSTRING(entity_idx_str, entities[idx]->name);
		
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
	INVALID, LEVEL, MESHES, TEXTURES, MATERIALS, MODELS, ENTITIES, EVENTS
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
	
	//// parse level file ////
	u32 entity_count = 0;
	std::vector<pair<u32,u32,u32,u32,u32>> events; //send_ent_id, send_comp_type, event_type, receive_ent_id, receive_comp_type
	char* buffer = Assets::readFileAsciiToArray(level_dir+"_");
	if(!buffer) return;
	defer{ delete[] buffer; };
	
	LevelHeader header = LevelHeader::INVALID;
	std::string line;
	char* new_line = buffer-1;
	char* line_start;
	bool has_cr = false;
	for(u32 line_number = 1; ;line_number++){
		//get the next line
		line_start = (has_cr) ? new_line+2 : new_line+1;
		if((new_line = strchr(line_start, '\n')) == 0) break; //end of file if cant find '\n'
		if(has_cr || *(new_line-1) == '\r'){ has_cr = true; new_line -= 1; }
		if(line_start == new_line) continue;
		
		line = std::string(line_start, new_line-line_start);
		line = Utils::eatComments(line, "#");
		line = Utils::eatSpacesLeading(line);
		line = Utils::eatSpacesTrailing(line);
		if(line.empty()) continue;
		
		//headers
		if(line[0] == '>'){
			if     (line == ">level")    { header = LevelHeader::LEVEL;     }
			else if(line == ">meshes")   { header = LevelHeader::MESHES; }
			else if(line == ">textures") { header = LevelHeader::TEXTURES; }
			else if(line == ">materials"){ header = LevelHeader::MATERIALS; }
			else if(line == ">models")   { header = LevelHeader::MODELS;    }
			else if(line == ">entities") { header = LevelHeader::ENTITIES;  }
			else if(line == ">events")   { header = LevelHeader::EVENTS;  }
			else{ header = LevelHeader::INVALID; ERROR(ParsingError,"'! Unknown header: ", line); }
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
			
			case(LevelHeader::MESHES):{
				if(split.size() < 1){ ERROR(ParsingError,"'! Mesh lines should have at least 1 value"); continue; }
				Storage::CreateMeshFromFile(split[0].c_str());
			}break;
			
			case(LevelHeader::TEXTURES):{
				if(split.size() < 3){ ERROR(ParsingError,"'! Texture lines should have at least 3 values"); continue; }
				
				bool mipmap = Assets::parse_bool(split[2], level_dir.c_str(), line_number);
				bool keep_loaded = Assets::parse_bool(split[1], level_dir.c_str(), line_number);
				Storage::CreateTextureFromFile(split[0].c_str(), ImageFormat_RGBA, TextureType_2D, keep_loaded, mipmap);
			}break;
			
			case(LevelHeader::MATERIALS):{
				if(split.size() < 1){ ERROR(ParsingError,"'! Material lines should have at least 1 value"); continue; }
				Storage::CreateMaterialFromFile(split[0].c_str());
			}break;
			
			case(LevelHeader::MODELS):{
				if(split.size() < 1){ ERROR(ParsingError,"'! Model lines should have at least 1 value"); continue; }
				Storage::CreateModelFromFile(split[1].c_str());
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
	
	//// parse entity files ////
	std::vector<Entity*> ents; ents.reserve(entity_count);
	for(std::string& file : Assets::iterateDirectory(level_dir)){
		if(file == "_") continue;
		if(Entity* e = Entity::LoadTEXT(this, level_dir+file)){
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
					if(c->type == events[i].fifth){
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
					if(c->type == events[i].second){
						c->event = events[i].third;
						c->sender.AddReceiver(rec_comp);
						rec_comp->entity->connections.insert(e);
						SUCCESS("Added event '",EventStrings[events[i].third],"': ",
								e->name," ",c->type," -> ",rec_comp->entity->name," ",rec_comp->type);
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

void Admin::SaveDESH(const char* filename) {
	
}

void Admin::LoadDESH(const char* filename) {
    
}


//////////////////
//// @storage ////
////////////////// //TODO(delle) error messages on invalid arguments
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
        c->compID = compIDcount;
        c->entity = e;
        c->layer_index = freeCompLayers[c->layer].add(c);
        if (c->type == ComponentType_Light) DengStorage->lights.add(dyncast(Light, c));
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
    if(c->type == ComponentType_Light) DengStorage->lights.add(dyncast(Light, c));
    compIDcount++;
}


////////////////
//// @query ////
////////////////
Entity* Admin::EntityRaycast(Vector3 origin, Vector3 direction, f32 maxDistance){ //!FixMe
	Entity* result = 0;
    f32 min_depth = INFINITY;
    f32 depth;
    vec3 p0, p1, p2, normal;
	vec3 intersect;
	vec3 perp01, perp12, perp20;
    mat4 transform, rotation;
	Mesh::Triangle* tri;
    for(Entity* e : entities){
        transform = e->transform.TransformMatrix();
        rotation = Matrix4::RotationMatrix(e->transform.rotation);
        if(ModelInstance* mc = e->GetComponent<ModelInstance>()){
            if(!mc->visible) continue;
			forX(tri_idx, mc->mesh->triangleCount){
				tri = &mc->mesh->triangleArray[tri_idx];
				p0 = tri->p[0] * transform;
				p1 = tri->p[1] * transform;
				p2 = tri->p[2] * transform;
				normal = tri->normal * rotation;
				
				//early out if triangle is not facing us
				if(normal.dot(p0 - origin) >= 0) continue;
				
				//find where on the plane defined by the triangle our raycast intersects
				depth     = (p0 - origin).dot(normal) / direction.dot(normal);
				intersect = origin + (direction * depth);
				
				//early out if intersection is behind us
				if(depth <= 0) continue;
				
				//make vectors perpendicular to each edge of the triangle
				perp01 = normal.cross(p1 - p0).yInvert().normalized();
				perp12 = normal.cross(p2 - p1).yInvert().normalized();
				perp20 = normal.cross(p0 - p2).yInvert().normalized();
				
				//check that the intersection point is within the triangle and its the closest triangle found so far
				if(perp01.dot(intersect - p0) > 0 &&
				   perp12.dot(intersect - p1) > 0 &&
				   perp20.dot(intersect - p2) > 0){
					
					//if its the closest triangle so far we store its index
					if(depth < min_depth){
						result = e;
						min_depth = depth;
						break;
					}
				}
			}
        }
    }
    
    if(result && depth <= maxDistance){
		return result;
	}else{
		return 0;
	}
}
