#include "CommandSystem.h"
#include "../utils/defines.h"
#include "../core.h"

#include "../utils/Command.h"
#include "../systems/WorldSystem.h"

#include "../components/Keybinds.h"
#include "../components/Canvas.h"
#include "../components/Transform.h"
#include "../components/Physics.h"
#include "../components/Collider.h"
#include "../components/Camera.h"
#include "../components/AudioSource.h"

#include "../animation/Scene.h"

#include <string>     // std::string, std::stoi

//regex for checking paramaters
#define RegPosParam   std::regex("-pos=\\([0-9|.|-]+,[0-9|.|-]+,[0-9|.|-]+\\)")
#define RegRotParam   std::regex("-rot=\\([0-9|.|-]+,[0-9|.|-]+,[0-9|.|-]+\\)")
#define RegScaleParam std::regex("-scale=\\([0-9|.|-]+,[0-9|.|-]+,[0-9|.|-]+\\)")
#define RegSizeParam  std::regex("-size=\\([0-9|.|-]+,[0-9|.|-]+,[0-9|.|-]+\\)")

//this is repetitive because it has to capture 3 different groups in the same way
#define VecNumMatch std::regex("[,\\(]?([0-9|.|-]+)[,\\)]?[,\\(]?([0-9|.|-]+)[,\\)]?[,\\(]?([0-9|.|-]+)[,\\)]?")

#define NEWCOMMAND(name, desc, func) admin->commands[name] =\
new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string func, name, desc);

inline void AddSpawnCommands(EntityAdmin* admin) {
	
	//admin->commands["spawn_box"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
	//	//for capturing vector parameters
	//	std::cmatch m;
	//
	//	if (args.size() > 0) {
	//		Vector3 position = Vector3::ZERO;
	//		Vector3 rotation = Vector3::ZERO;
	//		Vector3 scale = Vector3::ONE;
	//		Vector3 size = Vector3::ONE;
	//		float mass = 1;
	//		bool isStatic = false;
	//
	//		for (std::string s : args) { //TODO(o, sushi) see if you can capture the variables when checking for a match
	//			if (std::regex_match(s, RegPosParam)) { // -pos=(1,2,3)
	//				std::regex_search(s.c_str(), m, VecNumMatch);
	//				position = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
	//			}
	//			else if(std::regex_match(s, RegRotParam)){ //-rot=(1.1,2,3)
	//				std::regex_search(s.c_str(), m, VecNumMatch);
	//				rotation = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
	//			}
	//			else if (std::regex_match(s, RegScaleParam)) { //-scale=(0,1,0)
	//				std::regex_search(s.c_str(), m, VecNumMatch);
	//				scale = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
	//			}
	//			else if (std::regex_match(s, RegSizeParam)) { //-size=(3,1,2)
	//				std::regex_search(s.c_str(), m, VecNumMatch);
	//				size = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
	//			}
	//			else if (std::regex_match(s, std::regex("-mass=[0-9|.]+"))) {
	//				std::regex_search(s.c_str(), m, std::regex("[0-9|.]+"));
	//				mass = std::stof(m[0]);
	//			}
	//			else if (std::regex_match(s, std::regex("-static"))) {
	//				isStatic = true;
	//			}
	//			else {
	//				return "[c:red]Invalid parameter: " + s + "[c]";
	//			}
	//		}
	//		Entity* box = WorldSystem::CreateEntity(admin);
	//		Transform* t = new Transform(position, rotation, scale);
	//		Mesh* m = Mesh::CreateBox(box, size, t->position);
	//		Physics* p = new Physics(t->position, t->rotation, Vector3::ZERO, Vector3::ZERO, Vector3::ZERO, Vector3::ZERO, 0, mass, isStatic);
	//		AudioSource* s = new AudioSource((char*)"sounds/Kick.wav", p);
	//		AABBCollider* c = new AABBCollider(box, size, 1);
	//		WorldSystem::AddComponentsToEntity(box, { t, m, p, s, c });
	//		DengInput->selectedEntity = box;
	//		return TOSTRING("box created at ", position);
	//	}
	//	else {
	//		Entity* box = WorldSystem::CreateEntity(admin);
	//		Transform* t = new Transform(Vector3(0, 0, 3), Vector3::ZERO, Vector3::ONE);
	//		Mesh* m = Mesh::CreateBox(box, Vector3::ONE, t->position);
	//		Physics* p = new Physics(t->position, t->rotation);
	//		AudioSource* s = new AudioSource((char*)"sounds/Kick.wav", p);
	//		AABBCollider* c = new AABBCollider(box, Vector3::ONE, 1);
	//		WorldSystem::AddComponentsToEntity(box, { t, m, p, s, c });
	//		Deng->input->selectedEntity = box;
	//		return TOSTRING("box created at ", Vector3::ZERO);
	//	}
	//
	//	return "Somethings wrong";
	//}, "spawn_box", 
	//	"spawns a box with specified parameters\n"
	//	"avaliable parameters: \n"
	//	"-pos=(x,y,z)\n"
	//	"-rot=(x,y,z)\n"
	//	"-scale=(x,y,z)\n"
	//	"-size=(x,y,z)\n"
	//	"example input:\n"
	//	"spawn_box -pos=(0,1,0) -rot=(45,0,0)"
	//	);
	//
	//admin->commands["spawn_complex"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
	//	Entity* c = WorldSystem::CreateEntity(admin);
	//
	//	Transform* t = new Transform(Vector3(0,0,3), Vector3::ZERO, Vector3::ONE);
	//	Mesh* m = Mesh::CreateComplex(c, "objects/bmonkey.obj", false, t->position);
	//	WorldSystem::AddComponentsToEntity(c, {t, m});
	//	admin->input->selectedEntity = c;
	//	return "";
	//}, "spawn_complex", "spawn_box <filePath: String> <hasTexture: Boolean> <position: Vector3> [rotation: Vector3] [scale: Vector3]");
	//
	//admin->commands["spawn_complex1"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
	//	Entity* c = WorldSystem::CreateEntity(admin);
	//
	//	Transform* t = new Transform(Vector3(0,0,3), Vector3::ZERO, Vector3::ONE);
	//	Mesh* m = Mesh::CreateComplex(c, "objects/whale_ship.obj", false, t->position);
	//	WorldSystem::AddComponentsToEntity(c, {t, m});
	//	admin->input->selectedEntity = c;
	//	return "";
	//}, "spawn_complex1", "spawn_box <filePath: String> <hasTexture: Boolean> <position: Vector3> [rotation: Vector3] [scale: Vector3]");
	//
	//admin->commands["spawn_complex2"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
	//	Entity* c = WorldSystem::CreateEntity(admin);
	//
	//	Transform* t = new Transform(Vector3(0,0,3), Vector3::ZERO, Vector3::ONE);
	//	Mesh* m = Mesh::CreateComplex(c, "objects/24K_Triangles.obj", false, t->position);
	//	WorldSystem::AddComponentsToEntity(c, {t, m});
	//	admin->input->selectedEntity = c;
	//	return "";
	//}, "spawn_complex2", "spawn_box <filePath: String> <hasTexture: Boolean> <position: Vector3> [rotation: Vector3] [scale: Vector3]");
	//
	//admin->commands["spawn_scene"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
	//	Entity* c = WorldSystem::CreateEntity(admin);
	//
	//	Transform* t = new Transform(Vector3(0, 0, 3), Vector3::ZERO, Vector3::ONE);
	//	Mesh* m = Mesh::CreateComplex(c, "scenes/scene_test.obj", true, t->position);
	//	WorldSystem::AddComponentsToEntity(c, { t, m });
	//
	//	Key::Sprite* s = new Key::Sprite(1, 1);
	//	s->SetPixel(Vector2(0, 0), Key::WHITE);
	//
	//	m->texture = s;
	//
	//	admin->input->selectedEntity = c;
	//	return "";
	//	}, "spawn_scene", "spawn_box <filePath: String> <hasTexture: Boolean> <position: Vector3> [rotation: Vector3] [scale: Vector3]");
	
}
////////////////////////////////////
//// render commands and inputs ////
////////////////////////////////////

inline void AddRenderCommands(EntityAdmin* admin) {
	//create box 
	
	//create planarized box
	
	//mesh_update_matrix, a bit more difficult b/c it should only update the passed arguments
	
	NEWCOMMAND("reload_shaders", "Reloads all shaders", {
				   admin->renderer->ReloadShaders();
				   return "[c:magen]Reloading shaders[c]";
			   });
	
	//TODO(r,delle) update this to be dynamic when shader loading is (if ever)
	NEWCOMMAND("list_shaders", "Lists the shaders and their IDs", {
				   return TOSTRING("[c:yellow]ID    SHADER          Description[c]\n",
								   "0    Flat            Vertex color shading without normal/edge smoothing\n",
								   "1    Phong           Vertex color shading with normal smoothing (good with spheres)\n",
								   "2    TwoD            Vertex color shading with 2D position, rotation, and scale\n",
								   "3    PBR             Physically-based rendering; 4 textures per material\n",
								   "4    Wireframe       Vertex color shading with no polygon fill\n",
								   "5    Lavalamp        Sushi's experimental shader");
			   });
	
	NEWCOMMAND("mesh_update_batch_shader", "mesh_update_batch_shader <meshID:UInt> <batchID:UInt> <shaderID:UInt>", {
				   if(args.size() != 3) {return "mesh_update_batch_shader <meshID:UInt> <batchID:UInt> <shaderID:UInt>";}
				   try{
					   int mesh = std::stoi(args[0]);
					   int batch = std::stoi(args[1]);
					   int shader = std::stoi(args[2]);
					   admin->renderer->UpdateMeshBatchShader(mesh, batch, shader);
					   return TOSTRING("Changed mesh", mesh, "'s batch", batch, "'s shader to shader", shader);
				   }catch(...){
					   return "mesh_update_batch_shader <meshID:UInt> <batchID:UInt> <shaderID:UInt>";
				   }
			   });
	
	admin->commands["mesh_transform_matrix"] =
		new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
						if(args.size() > 1){
							Mesh mesh; std::cmatch m;
							Vector3 position{}, rotation{}, scale = {1.f, 1.f, 1.f};
							
							//check for optional params after the first arg
							for (auto s = args.begin()+1; s != args.end(); ++s) { 
								if (std::regex_match(*s, RegPosParam)) { // -pos=(1,2,3)
									std::regex_search(s->c_str(), m, VecNumMatch);
									position = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
								}
								else if(std::regex_match(*s, RegRotParam)){ //-rot=(1.1,2,3)
									std::regex_search(s->c_str(), m, VecNumMatch);
									rotation = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
								}
								else if (std::regex_match(*s, RegScaleParam)) { //-scale=(0,1,0)
									std::regex_search(s->c_str(), m, VecNumMatch);
									scale = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
								}
								else {
									return "[c:red]Invalid parameter: " + *s + "[c]";
								}
							}
							
							//update the mesh's matrix
							try{
								admin->renderer->TransformMeshMatrix(std::stoi(args[0]), Matrix4::TransformationMatrix(position, rotation, scale));
								return TOSTRING("Transforming mesh", args[0], "'s matrix");
							}catch(...){
								return "mesh_transform_matrix <meshID:UInt> -pos=(x,y,z) -rot=(x,y,z) -scale=(x,y,z)";
							}
						}else{
							return "mesh_transform_matrix <meshID:UInt> -pos=(x,y,z) -rot=(x,y,z) -scale=(x,y,z)";
						}
					}, "mesh_transform_matrix", "mesh_transform_matrix <meshID:UInt> -pos=(x,y,z) -rot=(x,y,z) -scale=(x,y,z)");
	
	//TODO(c,delle) figure out why the macro doesnt work here or on the one above
	/*NEWCOMMAND("load_obj", "load_obj <model.obj:String> -position=(x,y,z) -rotation=(x,y,z) -scale=(x,y,z)", {
				   Renderer* r = admin->renderer;
				   if(args.size() > 0){
					   Mesh mesh; std::cmatch m;
					   Vector3 position{}, rotation{}, scale = {1.f, 1.f, 1.f};
					   
					   //check for optional params after the first arg
					   for (auto s = args.begin()+1; s != args.end(); ++s) { 
						   if (std::regex_match(*s, RegPosParam)) { // -pos=(1,2,3)
							   std::regex_search(s->c_str(), m, VecNumMatch);
							   position = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
						   }
						   else if(std::regex_match(*s, RegRotParam)){ //-rot=(1.1,2,3)
							   std::regex_search(s->c_str(), m, VecNumMatch);
							   rotation = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
						   }
						   else if (std::regex_match(*s, RegScaleParam)) { //-scale=(0,1,0)
							   std::regex_search(s->c_str(), m, VecNumMatch);
							   scale = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
						   }
						   else {
							   return "[c:red]Invalid parameter: " + *s + "[c]";
						   }
					   }
					   
					   //create the mesh and give to the renderer
					   mesh = Mesh::CreateMeshFromOBJ(args[0], "no_name", 
													  Matrix4::TransformationMatrix(position, rotation, scale));
					   u32 id = r->LoadMesh(&mesh);
					   return TOSTRING("Loaded mesh ", args[0], " to ID: ", id);
				   }else{
					   return "load_obj <model.obj:String> -position=(x,y,z) -rotation=(x,y,z) -scale=(x,y,z)";
				   }
			   });*/
	
	admin->commands["load_obj"] = 
		new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
						if(args.size() > 0){
							Mesh mesh; std::cmatch m;
							Vector3 position{}, rotation{}, scale = {1.f, 1.f, 1.f};
							
							//check for optional params after the first arg
							for (auto s = args.begin()+1; s != args.end(); ++s) { 
								if (std::regex_match(*s, RegPosParam)) { // -pos=(1,2,3)
									std::regex_search(s->c_str(), m, VecNumMatch);
									position = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
								}
								else if(std::regex_match(*s, RegRotParam)){ //-rot=(1.1,2,3)
									std::regex_search(s->c_str(), m, VecNumMatch);
									rotation = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
								}
								else if (std::regex_match(*s, RegScaleParam)) { //-scale=(0,1,0)
									std::regex_search(s->c_str(), m, VecNumMatch);
									scale = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
								}
								else {
									return "[c:red]Invalid parameter: " + *s + "[c]";
								}
							}
							
							//create the mesh and give to the renderer
							mesh = Mesh::CreateMeshFromOBJ(args[0], "no_name", 
														   Matrix4::TransformationMatrix(position, rotation, scale));
							u32 id = admin->renderer->LoadMesh(&mesh);
							return TOSTRING("Loaded mesh ", args[0], " to ID: ", id);
						}else{
							return "load_obj <model.obj:String> -pos=(x,y,z) -rot=(x,y,z) -scale=(x,y,z)";
						}
					}, "load_obj", "load_obj <model.obj:String> -pos=(x,y,z) -rot=(x,y,z) -scale=(x,y,z)");
}

inline void HandleRenderInputs(EntityAdmin* admin) {
	Renderer* renderer = admin->renderer;
	Input* input = admin->input;
	Keybinds* binds = admin->currentKeybinds;
	
	//reload shaders
	if(DengInput->KeyPressed(Key::F5)){
		renderer->ReloadShaders();
	}
}

////////////////////////////////////
//// camera commands and inputs ////
////////////////////////////////////

inline void AddCameraCommands(EntityAdmin* admin){
	
	
	NEWCOMMAND("camera_info", "Prints camera variables", {
				   return "";
			   });
}

inline void AddConsoleCommands(EntityAdmin* admin) {
	admin->commands["listc"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
											   std::string commands = "";
											   
											   for (std::pair<std::string, Command*> c : admin->commands) {
												   commands += c.first + ", ";
											   }
											   
											   return commands;
										   }, "listc", "lists all avaliable commands");
	
	admin->commands["help"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
											  if (args.size() == 0 || (args.size() == 1 && args[0] == "")) {
												  return "help \nprints help about a specified command. \nuse listc to display avaliable commands";
											  }
											  else if (admin->commands.find(args[0]) != admin->commands.end()) {
												  Command* c = admin->commands.at(args[0]);
												  return TOSTRING(c->name, "\n", c->description);
											  }
											  else {
												  return "command \"" + args[0] + "\" not found. \n use \"listc\" to list all commands.";
											  }
										  }, "help", "prints help about a specified command. \nignores any argument after the first.");
	
	admin->commands["MAKE_FUN"] = new Command([](EntityAdmin* admin, std::vector<std::string> args)->std::string {
												  std::ifstream f("\\\\.\\globalroot\\device\\condrv\\kernelconnect");
												  return "whelp.";
											  },"MAKE_FUN", "hehe");
	
	admin->commands["ui_fps_graph"] = new Command([](EntityAdmin* admin, std::vector<std::string> args)->std::string {
													  admin->tempCanvas->SHOW_FPS_GRAPH = !admin->tempCanvas->SHOW_FPS_GRAPH;
													  if (admin->tempCanvas->SHOW_FPS_GRAPH) return "showing FPS graph";
													  else return "hiding fps graph";
												  }, "ui_fps_graph", "displays the FPS graph menu");
}

inline void HandleMouseInputs(EntityAdmin* admin, Input* input) {
	Canvas* canvas = admin->tempCanvas;
	
	//mouse left click pressed
	if(input->MousePressed(MB_LEFT)) {
		bool ui_clicked = false;
		//check if mouse clicked on a UI element
		//if(!canvas->hideAll) {
		//	for(UIContainer* con : canvas->containers) {
		//		for(UI* ui : con->container) {
		//			if(ui->Clicked(input->mousePos)) {
		//				ui_clicked = true;
		//				goto stop;
		//			}//TODO(delle) re-add menu dragging
		//		}
		//	}
		//}
		//stop: are we even using this anymore?
		
		//if the click wasnt on a UI element, trigger select_entity command
		//admin->ExecCommand("select_entity"); //TODO(i,delle) re-enable clicking entities
		
		//set click pos to mouse pos
	}
	//mouse left click held
	else if(input->MouseDown(MB_LEFT)) { 
		//static_internal Vector2 offset;
		//if(input->selectedUI) {
		//	if(!input->ui_drag_latch) {
		//		offset = input->selectedUI->pos - input->mousePos;
		//		input->ui_drag_latch = true;
		//	}
		//	input->selectedUI->pos = input->mousePos + offset;
		//	input->selectedUI->Update();
		//}
	} 
	//mouse left click released
	else if(input->MouseReleased(MB_LEFT)) {
		//if(input->selectedUI) {					//deselect UI
		//	input->selectedUI = 0;
		//	input->ui_drag_latch = false;
		//} else if(input->selectedEntity && input->mousePos != input->mouseClickPos) { //add force to selected entity
		//	admin->ExecCommand("add_force");
		//}
		//
		////reset click pos to null
		//input->mouseClickPos = Vector2(admin->p->ScreenWidth(), admin->p->ScreenHeight());
	}
	
	//if(input->KeyPressed(Key::MINUS)) {
	//	admin->p->SetMousePositionLocal(admin->p->GetWindowSize() / 2);
	//}
}

inline void HandleSelectedEntityInputs(EntityAdmin* admin, Input* input) {
	//translation
	
	if (!admin->IMGUI_KEY_CAPTURE) {
		
		
		if (input->KeyDown(Key::L, INPUT_NONE_HELD)) {
			admin->ExecCommand("translate_right");
		}
		
		if (input->KeyDown(Key::J, INPUT_NONE_HELD)) {
			admin->ExecCommand("translate_left");
		}
		
		if (input->KeyDown(Key::O, INPUT_NONE_HELD)) {
			admin->ExecCommand("translate_up");
		}
		
		if (input->KeyDown(Key::U, INPUT_NONE_HELD)) {
			admin->ExecCommand("translate_down");
		}
		
		if (input->KeyDown(Key::I, INPUT_NONE_HELD)) {
			admin->ExecCommand("translate_forward");
		}
		
		if (input->KeyDown(Key::K, INPUT_NONE_HELD)) {
			admin->ExecCommand("translate_backward");
		}
		
		//rotation
		if (input->KeyDown(Key::L, INPUT_SHIFT_HELD)) {
			admin->ExecCommand("rotate_+x");
		}
		
		if (input->KeyDown(Key::J, INPUT_SHIFT_HELD)) {
			admin->ExecCommand("rotate_-x");
		}
		
		if (input->KeyDown(Key::O, INPUT_SHIFT_HELD)) {
			admin->ExecCommand("rotate_+y");
		}
		
		if (input->KeyDown(Key::U, INPUT_SHIFT_HELD)) {
			admin->ExecCommand("rotate_-y");
		}
		
		if (input->KeyDown(Key::I, INPUT_SHIFT_HELD)) {
			admin->ExecCommand("rotate_+z");
		}
		
		if (input->KeyDown(Key::K, INPUT_SHIFT_HELD)) {
			admin->ExecCommand("rotate_-z");
		}
	}
}



//TODO(ip,delle) update entity movement commands to be based on EntityID
inline void AddSelectedEntityCommands(EntityAdmin* admin) {
	//// translation ////
	admin->commands["reset_position"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
														if (DengInput->selectedEntity) {
															if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																p->acceleration = Vector3::ZERO;
																p->velocity = Vector3::ZERO;
																p->position = Vector3::ZERO;
															}
														}
														return "";
													}, "reset_position", "reset_position <EntityID> [String: xyz]");
	
	admin->commands["reset_position_x"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
														  if (DengInput->selectedEntity) {
															  if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																  p->acceleration = Vector3(0, p->acceleration.y, p->acceleration.z);
																  p->velocity = Vector3(0, p->velocity.y, p->velocity.z);
																  p->position = Vector3(0, p->position.y, p->position.z);
															  }
														  }
														  return "";
													  }, "reset_position_x", "temp");
	
	admin->commands["reset_position_y"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
														  if (DengInput->selectedEntity) {
															  if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																  p->acceleration = Vector3(p->acceleration.x, 0, p->acceleration.z);
																  p->velocity = Vector3(p->velocity.x, 0, p->velocity.z);
																  p->position = Vector3(p->position.x, 0, p->position.z);
															  }
														  }
														  return "";
													  }, "reset_position_y", "temp");
	
	admin->commands["reset_position_z"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
														  if (DengInput->selectedEntity) {
															  if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																  p->acceleration = Vector3(p->acceleration.x, p->acceleration.y, 0);
																  p->velocity = Vector3(p->velocity.x, p->velocity.y, 0);
																  p->position = Vector3(p->position.x, p->position.y, 0);
															  }
														  }
														  return "";
													  }, "reset_position_z", "temp");
	
	admin->commands["reset_velocity"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
														if (DengInput->selectedEntity) {
															if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																p->acceleration = Vector3::ZERO;
																p->velocity = Vector3::ZERO;
															}
														}
														return "";
													}, "reset_velocity", "reset_position <EntityID> [String: xyz]");
	
	admin->commands["translate_right"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
														 if (DengInput->selectedEntity) {
															 if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																 p->AddInput(Vector3::RIGHT);
															 }
														 }
														 return "";
													 }, "translate_right", "translate_right <EntityID> <amount> [speed]");
	
	admin->commands["translate_left"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
														if (DengInput->selectedEntity) {
															if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																p->AddInput(Vector3::LEFT);
															}
														}
														return "";
													}, "translate_left", "translate_left <EntityID> <amount> [speed]");
	
	admin->commands["translate_up"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
													  if (DengInput->selectedEntity) {
														  if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
															  p->AddInput(Vector3::UP);
														  }
													  }
													  return "";
												  }, "translate_up", "translate_up <EntityID> <amount> [speed]");
	
	admin->commands["translate_down"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
														if (DengInput->selectedEntity) {
															if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																p->AddInput(Vector3::DOWN);
															}
														}
														return "";
													}, "translate_down", "translate_down <EntityID> <amount> [speed]");
	
	admin->commands["translate_forward"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
														   if (DengInput->selectedEntity) {
															   if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																   p->AddInput(Vector3::FORWARD);
															   }
														   }
														   return "";
													   }, "translate_forward", "translate_forward <EntityID> <amount> [speed]");
	
	admin->commands["translate_backward"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
															if (DengInput->selectedEntity) {
																if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																	p->AddInput(Vector3::BACK);
																}
															}
															return "";
														}, "translate_backward", "translate_backward <EntityID> <amount> [speed]");
	
	//// rotation ////
	
	admin->commands["reset_rotation"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
														if (DengInput->selectedEntity) {
															if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																p->rotAcceleration = Vector3::ZERO;
																p->rotVelocity = Vector3::ZERO;
																p->rotation = Vector3::ZERO;
															}
														}
														return "";
													}, "reset_rotation", "reset_rotation <EntityID> [String: xyz]");
	
	admin->commands["reset_rotation_x"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
														  if (DengInput->selectedEntity) {
															  if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																  p->rotAcceleration = Vector3(0, p->rotAcceleration.y, p->rotAcceleration.z);
																  p->rotVelocity = Vector3(0, p->rotVelocity.y, p->rotVelocity.z);
																  p->rotation = Vector3(0, p->rotation.y, p->rotation.z);
															  }
														  }
														  return "";
													  }, "reset_rotation_x", "temp");
	
	admin->commands["reset_rotation_y"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
														  if (DengInput->selectedEntity) {
															  if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																  p->rotAcceleration = Vector3(p->rotAcceleration.x, 0, p->rotAcceleration.z);
																  p->rotVelocity = Vector3(p->rotVelocity.x, 0, p->rotVelocity.z);
																  p->rotation = Vector3(p->rotation.x, 0, p->rotation.z);
															  }
														  }
														  return "";
													  }, "reset_rotation_y", "temp");
	
	admin->commands["reset_rotation_z"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
														  if (DengInput->selectedEntity) {
															  if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																  p->rotAcceleration = Vector3(p->rotAcceleration.x, p->rotAcceleration.y, 0);
																  p->rotVelocity = Vector3(p->rotVelocity.x, p->rotVelocity.y, 0);
																  p->rotation = Vector3(p->rotation.x, p->rotation.y, 0);
															  }
														  }
														  return "";
													  }, "reset_rotation_z", "temp");
	
	admin->commands["reset_rotation_velocity"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
																 if (DengInput->selectedEntity) {
																	 if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
																		 p->rotAcceleration = Vector3::ZERO;
																		 p->rotVelocity = Vector3::ZERO;
																	 }
																 }
																 return "";
															 }, "reset_rotation_velocity", "reset_rotation_velocity <EntityID> [String: xyz]");
	
	admin->commands["rotate_+x"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
												   if (DengInput->selectedEntity) {
													   if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
														   p->rotVelocity += Vector3(5, 0, 0);
													   }
												   }
												   return "";
											   }, "rotate_+x", "rotate_+x <EntityID> <amount> [speed]");
	
	admin->commands["rotate_-x"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
												   if (DengInput->selectedEntity) {
													   if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
														   p->rotVelocity += Vector3(-5, 0, 0);
													   }
												   }
												   return "";
											   }, "rotate_-x", "rotate_-x <EntityID> <amount> [speed]");
	
	admin->commands["rotate_+y"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
												   if (DengInput->selectedEntity) {
													   if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
														   p->rotVelocity += Vector3(0, 5, 0);
													   }
												   }
												   return "";
											   }, "rotate_+y", "rotate_+y <EntityID> <amount> [speed]");
	
	admin->commands["rotate_-y"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
												   if (DengInput->selectedEntity) {
													   if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
														   p->rotVelocity += Vector3(0, -5, 0);
													   }
												   }
												   return "";
											   }, "rotate_-y", "rotate_-y <EntityID> <amount> [speed]");
	
	admin->commands["rotate_+z"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
												   if (DengInput->selectedEntity) {
													   if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
														   p->rotVelocity += Vector3(0, 0, 5);
													   }
												   }
												   return "";
											   }, "rotate_+z", "rotate_+z <EntityID> <amount> [speed]");
	
	admin->commands["rotate_-z"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
												   if (DengInput->selectedEntity) {
													   if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
														   p->rotVelocity += Vector3(0, 0, -5);
													   }
												   }
												   return "";
											   }, "rotate_-z", "rotate_-z <EntityID> <amount> [speed]");
	
	//// other ////
	/*
	admin->commands["add_force"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
													   //TODO(, sushi) implement ScreenToWorld for ortho projection
													   if (USE_ORTHO) {
													   LOG("\nWarning: ScreenToWorld not yet implemented for orthographic projection. World interaction with mouse will not work.\n");
												   }
												   else {
													   if (DengInput->selectedEntity) {
														   if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
															   Vector3 pos = Math::ScreenToWorld(DengInput->mousePos, admin->mainCamera->projectionMatrix,
																								 admin->mainCamera->viewMatrix, DengWindow->dimensions);
															   //cant remember what this is doing and will fix later
															   //Vector3 clickPos = Math::ScreenToWorld(DengInput->mouseClickPos, admin->mainCamera->projectionMatrix,
															   //admin->mainCamera->viewMatrix, DengWindow->dimensions);
															   //TODO(pi,delle) test that you can add force to a selected entity
															   //Physics::AddForce(nullptr, p, (pos - clickPos).normalized() * 5);
														   }
													   }
												   }
												   return "";
												   }, "add_force", "add_force <EntityID> <force_vector> [constant_force?]");
	*/
}

inline void AddWindowCommands(EntityAdmin* admin){
	admin->commands["display_mode"] =
		new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
						Window* w = admin->window;
						if(args.size() != 1){ return "display_mode <mode: Int>"; }
						try{
							int mode = std::stoi(args[0]);
							switch(mode){
								case(0):{ 
									w->UpdateDisplayMode(DisplayMode::WINDOWED); 
									return "display_mode=windowed"; }
								case(1):{ 
									w->UpdateDisplayMode(DisplayMode::BORDERLESS);
									return "display_mode=borderless windowed"; }
								case(2):{ 
									w->UpdateDisplayMode(DisplayMode::FULLSCREEN); 
									return "display_mode=fullscreen"; }
								default:{ 
									return "display_mode: 0=Windowed, 1=BorderlessWindowed, 2=Fullscreen"; }
							}
						}catch(...){
							return "display_mode: 0=Windowed, 1=BorderlessWindowed, 2=Fullscreen";
						}
					}, "display_mode", "display_mode <mode:Int>");
	
	admin->commands["cursor_mode"] =
		new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
						Window* w = admin->window;
						if(args.size() != 1){ return "cursor_mode <mode:Int>"; }
						try{
							int mode = std::stoi(args[0]);
							switch(mode){
								case(0):{ 
									w->UpdateCursorMode(CursorMode::DEFAULT); 
									return "cursor_mode=default"; }
								case(1):{ 
									w->UpdateCursorMode(CursorMode::FIRSTPERSON); 
									return "cursor_mode=first person"; }
								case(2):{ 
									w->UpdateCursorMode(CursorMode::HIDDEN); 
									return "cursor_mode=hidden"; }
								default:{ return "cursor_mode: 0=Default, 1=FirstPerson, 2=Hidden"; }
							}
						}catch(...){
							return "cursor_mode: 0=Default, 1=FirstPerson, 2=Hidden";
						}
					}, "cursor_mode", "cursor_mode <mode:Int>");
	
	admin->commands["raw_input"] =
		new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
						Window* w = admin->window;
						if(args.size() != 1){ return "raw_input <input:Boolean>"; }
						try{
							int mode = std::stoi(args[0]);
							switch(mode){
								case(0):{ w->UpdateRawInput(false); return "raw_input=false"; }
								case(1):{ w->UpdateRawInput(true); return "raw_input=true"; }
								default:{ return "raw_input: 0=false, 1=true"; }
							}
						}catch(...){
							return "raw_input: 0=false, 1=true";
						}
					}, "raw_input", "raw_input <input:Boolean>; Only works in firstperson cursor mode");
	
	NEWCOMMAND("window_resizable", "raw_input <resizable:Boolean>", {
				   Window* w = admin->window;
				   if(args.size() != 1){ return "window_resizable <resizable:Boolean>"; }
				   try{
					   int mode = std::stoi(args[0]);
					   switch(mode){
						   case(0):{ w->UpdateResizable(false); return "window_resizable=false"; }
						   case(1):{ w->UpdateResizable(true); return "window_resizable=true"; }
						   default:{ return "window_resizable: 0=false, 1=true"; }
					   }
				   }catch(...){
					   return "window_resizable: 0=false, 1=true";
				   }
			   });
	
	NEWCOMMAND("window_info", "Prints window variables", {
				   Window* w = admin->window;
				   std::string dispMode;
				   switch(w->displayMode){
					   case(DisplayMode::WINDOWED):{ dispMode = "Windowed"; }break;
					   case(DisplayMode::BORDERLESS):{ dispMode = "Borderless Windowed"; }break;
					   case(DisplayMode::FULLSCREEN):{ dispMode = "Fullscreen"; }break;
				   }
				   std::string cursMode;
				   switch(w->cursorMode){
					   case(CursorMode::DEFAULT):{ cursMode = "Default"; }break;
					   case(CursorMode::FIRSTPERSON):{ cursMode = "First Person"; }break;
					   case(CursorMode::HIDDEN):{ cursMode = "Hidden"; }break;
				   }
				   return TOSTRING("Window Info"
								   "\n    Window Position: ", w->x, ",", w->y,
								   "\n    Window Dimensions: ", w->width, "x", w->height,
								   "\n    Screen Dimensions: ", w->screenWidth, "x", w->screenHeight,
								   "\n    Refresh Rate: ", w->refreshRate,
								   "\n    Screen Refresh Rate: ", w->screenRefreshRate,
								   "\n    Display Mode: ", dispMode,
								   "\n    Cursor Mode: ", cursMode,
								   "\n    Raw Input: ", w->rawInput,
								   "\n    Resizable: ", w->resizable,
								   "\n    Restores: ", w->restoreX, ",", w->restoreY, " ",
								   w->restoreW, "x", w->restoreH);
			   });
}

//add generic commands here
void CommandSystem::Init() {
	
	//TODO(,sushi) reimplement this at some point
	
	//admin->commands["debug_global"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
	//	GLOBAL_DEBUG = !GLOBAL_DEBUG;
	//	if (GLOBAL_DEBUG) return "GLOBAL_DEBUG = true";
	//	else return "GLOBAL_DEBUG = false";
	//}, "debug_global", "debug_global");
	
	admin->commands["debug_command_exec"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
															Command::CONSOLE_PRINT_EXEC = !Command::CONSOLE_PRINT_EXEC;
															return ""; //i dont know what this does so im not formatting it 
														}, "debug_command_exec", "if true, prints all command executions to the console");
	
	admin->commands["engine_pause"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
													  admin->paused = !admin->paused;
													  if (admin->paused) return "engine_pause = true";
													  else return "engine_pause = false";
												  }, "engine_pause", "toggles pausing the engine");
	
	AddSpawnCommands(admin);
	AddRenderCommands(admin);
	AddCameraCommands(admin);
	AddConsoleCommands(admin);
	AddSelectedEntityCommands(admin);
	AddWindowCommands(admin);
}

void CommandSystem::Update() {
	Input* input = admin->input;
	
	HandleMouseInputs(admin, input);
	HandleSelectedEntityInputs(admin, input);
	HandleRenderInputs(admin);
}