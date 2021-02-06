#include "CommandSystem.h"


#include "../core/deshi_input.h"

#include "../systems/WorldSystem.h"


#include "../components/Keybinds.h"
#include "../components/Canvas.h"
#include "../components/Transform.h"
#include "../components/Model.h"
#include "../components/Scene.h"
#include "../components/Physics.h"
#include "../components/Collider.h"
#include "../components/Source.h"


#include "../utils/Command.h"
#include "../utils/defines.h"

//regex for checking paramaters
#define RegPosParam   std::regex("-pos=\\([0-9|.|-]+,[0-9|.|-]+,[0-9|.|-]+\\)")
#define RegRotParam   std::regex("-rot=\\([0-9|.|-]+,[0-9|.|-]+,[0-9|.|-]+\\)")
#define RegScaleParam std::regex("-scale=\\([0-9|.|-]+,[0-9|.|-]+,[0-9|.|-]+\\)")
#define RegSizeParam  std::regex("-size=\\([0-9|.|-]+,[0-9|.|-]+,[0-9|.|-]+\\)")

//this is repetitive because it has to capture 3 different groups in the same way
#define VecNumMatch std::regex("[,\\(]?([0-9|.|-]+)[,\\)]?[,\\(]?([0-9|.|-]+)[,\\)]?[,\\(]?([0-9|.|-]+)[,\\)]?")

inline void AddSpawnCommands(EntityAdmin* admin) {

	admin->commands["spawn_box"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		//for capturing vector parameters
		std::cmatch m;

		if (args.size() > 0) {
			Vector3 position = Vector3::ZERO;
			Vector3 rotation = Vector3::ZERO;
			Vector3 scale = Vector3::ONE;
			Vector3 size = Vector3::ONE;
			float mass = 1;
			bool isStatic = false;

			for (std::string s : args) { //TODO(o, sushi) see if you can capture the variables when checking for a match
				if (std::regex_match(s, RegPosParam)) { // -pos=(1,2,3)
					std::regex_search(s.c_str(), m, VecNumMatch);
					position = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
				}
				else if(std::regex_match(s, RegRotParam)){ //-rot=(1.1,2,3)
					std::regex_search(s.c_str(), m, VecNumMatch);
					rotation = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
				}
				else if (std::regex_match(s, RegScaleParam)) { //-scale=(0,1,0)
					std::regex_search(s.c_str(), m, VecNumMatch);
					scale = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
				}
				else if (std::regex_match(s, RegSizeParam)) { //-size=(3,1,2)
					std::regex_search(s.c_str(), m, VecNumMatch);
					size = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
				}
				else if (std::regex_match(s, std::regex("-mass=[0-9|.]+"))) {
					std::regex_search(s.c_str(), m, std::regex("[0-9|.]+"));
					mass = std::stof(m[0]);
				}
				else if (std::regex_match(s, std::regex("-static"))) {
					isStatic = true;
				}
				else {
					return "[c:red]Invalid parameter: " + s + "[c]";
				}
			}
			Entity* box = WorldSystem::CreateEntity(admin);
			Transform* t = new Transform(position, rotation, scale);
			Mesh* m = Mesh::CreateBox(box, size, t->position);
			Physics* p = new Physics(t->position, t->rotation, Vector3::ZERO, Vector3::ZERO, Vector3::ZERO, Vector3::ZERO, 0, mass, isStatic);
			Source* s = new Source((char*)"sounds/Kick.wav", p);
			AABBCollider* c = new AABBCollider(box, size, 1);
			WorldSystem::AddComponentsToEntity(box, { t, m, p, s, c });
			DengInput->selectedEntity = box;
			return TOSTRING("box created at ", position);
		}
		else {
			Entity* box = WorldSystem::CreateEntity(admin);
			Transform* t = new Transform(Vector3(0, 0, 3), Vector3::ZERO, Vector3::ONE);
			Mesh* m = Mesh::CreateBox(box, Vector3::ONE, t->position);
			Physics* p = new Physics(t->position, t->rotation);
			Source* s = new Source((char*)"sounds/Kick.wav", p);
			AABBCollider* c = new AABBCollider(box, Vector3::ONE, 1);
			WorldSystem::AddComponentsToEntity(box, { t, m, p, s, c });
			Deng->input->selectedEntity = box;
			return TOSTRING("box created at ", Vector3::ZERO);
		}

		return "Somethings wrong";
	}, "spawn_box", 
		"spawns a box with specified parameters\n"
		"avaliable parameters: \n"
		"-pos=(x,y,z)\n"
		"-rot=(x,y,z)\n"
		"-scale=(x,y,z)\n"
		"-size=(x,y,z)\n"
		"example input:\n"
		"spawn_box -pos=(0,1,0) -rot=(45,0,0)"
		);

	admin->commands["spawn_complex"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		Entity* c = WorldSystem::CreateEntity(admin);

		Transform* t = new Transform(Vector3(0,0,3), Vector3::ZERO, Vector3::ONE);
		Mesh* m = Mesh::CreateComplex(c, "objects/bmonkey.obj", false, t->position);
		WorldSystem::AddComponentsToEntity(c, {t, m});
		admin->input->selectedEntity = c;
		return "";
	}, "spawn_complex", "spawn_box <filePath: String> <hasTexture: Boolean> <position: Vector3> [rotation: Vector3] [scale: Vector3]");

	admin->commands["spawn_complex1"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		Entity* c = WorldSystem::CreateEntity(admin);

		Transform* t = new Transform(Vector3(0,0,3), Vector3::ZERO, Vector3::ONE);
		Mesh* m = Mesh::CreateComplex(c, "objects/whale_ship.obj", false, t->position);
		WorldSystem::AddComponentsToEntity(c, {t, m});
		admin->input->selectedEntity = c;
		return "";
	}, "spawn_complex1", "spawn_box <filePath: String> <hasTexture: Boolean> <position: Vector3> [rotation: Vector3] [scale: Vector3]");

	admin->commands["spawn_complex2"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		Entity* c = WorldSystem::CreateEntity(admin);

		Transform* t = new Transform(Vector3(0,0,3), Vector3::ZERO, Vector3::ONE);
		Mesh* m = Mesh::CreateComplex(c, "objects/24K_Triangles.obj", false, t->position);
		WorldSystem::AddComponentsToEntity(c, {t, m});
		admin->input->selectedEntity = c;
		return "";
	}, "spawn_complex2", "spawn_box <filePath: String> <hasTexture: Boolean> <position: Vector3> [rotation: Vector3] [scale: Vector3]");

	admin->commands["spawn_scene"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		Entity* c = WorldSystem::CreateEntity(admin);

		Transform* t = new Transform(Vector3(0, 0, 3), Vector3::ZERO, Vector3::ONE);
		Mesh* m = Mesh::CreateComplex(c, "scenes/scene_test.obj", true, t->position);
		WorldSystem::AddComponentsToEntity(c, { t, m });

		olc::Sprite* s = new olc::Sprite(1, 1);
		s->SetPixel(Vector2(0, 0), olc::WHITE);

		m->texture = s;

		admin->input->selectedEntity = c;
		return "";
		}, "spawn_scene", "spawn_box <filePath: String> <hasTexture: Boolean> <position: Vector3> [rotation: Vector3] [scale: Vector3]");

}

inline void AddRenderCommands(EntityAdmin* admin) {
	admin->commands["r_wireframe"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->currentScene->RENDER_WIREFRAME = !admin->currentScene->RENDER_WIREFRAME;
		if (admin->currentScene->RENDER_WIREFRAME) return "render_wireframe = true";
		else return "render_wireframe = false";
	}, "r_wireframe", "toggles rendering wireframe");

	admin->commands["r_textures"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->currentScene->RENDER_TEXTURES = !admin->currentScene->RENDER_TEXTURES;
		if (admin->currentScene->RENDER_TEXTURES) return "render_textures = true";
		else return "render_textures = false";
	}, "r_textures", "toggles rendering textuires");

	admin->commands["r_display_edges"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->currentScene->RENDER_EDGE_NUMBERS = !admin->currentScene->RENDER_EDGE_NUMBERS;
		if (admin->currentScene->RENDER_EDGE_NUMBERS) return "render_edge_numbers = true";
		else return "render_edge_numbers = false";
		}, "r_display_edges", "toggles diaplying edge numbers on triangles");

	admin->commands["r_local_axis"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->currentScene->RENDER_LOCAL_AXIS = !admin->currentScene->RENDER_LOCAL_AXIS;
		if (admin->currentScene->RENDER_LOCAL_AXIS) return "render_local_axis = true";
		else return "render_local_axis = false";
		}, "r_local_axis", "toggles rendering the local axis on entities");

	admin->commands["r_global_axis"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->currentScene->RENDER_GLOBAL_AXIS = !admin->currentScene->RENDER_GLOBAL_AXIS;
		if (admin->currentScene->RENDER_GLOBAL_AXIS) return "render_global_axis = true";
		else return "render_global_axis = false";
		}, "r_global_axis", "toggles rendering the global axis relatie to camera orientation in the top right of the screen");

	admin->commands["r_transforms"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->currentScene->RENDER_TRANSFORMS = !admin->currentScene->RENDER_TRANSFORMS;
		if (admin->currentScene->RENDER_TRANSFORMS) return "render_transforms = true";
		else return "render_transforms = false";
		}, "r_transforms", "toggles diaplaying tranform information on entities");

	admin->commands["r_physics"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->currentScene->RENDER_PHYSICS = !admin->currentScene->RENDER_PHYSICS;
		if (admin->currentScene->RENDER_PHYSICS) return "render_physics = true";
		else return "render_physics = false";
		}, "r_physics", "toggles rendering velocity and acceleration vectors on entities");

	admin->commands["r_screen_bounding_box"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->currentScene->RENDER_SCREEN_BOUNDING_BOX = !admin->currentScene->RENDER_SCREEN_BOUNDING_BOX;
		if (admin->currentScene->RENDER_SCREEN_BOUNDING_BOX) return "render_screen_bounding_box = true";
		else return "render_screen_bounding_box = false";
		}, "r_screen_bounding_box", "toggles rendering of the screen space bounding box of entities");

	admin->commands["r_mesh_vertices"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->currentScene->RENDER_MESH_VERTICES = !admin->currentScene->RENDER_MESH_VERTICES;
		if (admin->currentScene->RENDER_MESH_VERTICES) return "render_mesh_vertices = true";
		else return "render_mesh_vertices = false";
		}, "r_mesh_vertices", "toggles rendering of mesh vertices");

	admin->commands["r_grid"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->currentScene->RENDER_GRID = !admin->currentScene->RENDER_GRID;
		if (admin->currentScene->RENDER_GRID) return "render_grid = true";
		else return "render_grid = false";
		}, "r_grid", "toggles rendering the world grid");

	admin->commands["r_light_rays"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->currentScene->RENDER_LIGHT_RAYS = !admin->currentScene->RENDER_LIGHT_RAYS;
		if (admin->currentScene->RENDER_LIGHT_RAYS) return "render_light_rays = true";
		else return "render_light_rays = false";
		}, "r_light_rays", "toggles rendering light rays");

	admin->commands["r_mesh_normals"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->currentScene->RENDER_MESH_NORMALS = !admin->currentScene->RENDER_MESH_NORMALS;
		if (admin->currentScene->RENDER_MESH_NORMALS) return "render_mesh_normals = true";
		else return "render_mesh_normals = false";
	}, "r_mesh_normals", "toggles rendering mesh normals");
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
	}, "MAKE_FUN", "hehe");

	admin->commands["ui_fps_graph"] = new Command([](EntityAdmin* admin, std::vector<std::string> args)->std::string {
		admin->tempCanvas->SHOW_FPS_GRAPH = !admin->tempCanvas->SHOW_FPS_GRAPH;
		if (admin->tempCanvas->SHOW_FPS_GRAPH) return "showing FPS graph";
		else return "hiding fps graph";
		}, "ui_fps_graph", "displays the FPS graph menu");
}

inline void HandleMouseInputs(EntityAdmin* admin, Input* input) {
	Canvas* canvas = admin->tempCanvas;

	//mouse left click pressed
	if(input->MousePressed(INPUT_MOUSE_LEFT)) {
		bool ui_clicked = false;
		//check if mouse clicked on a UI element
		if(!canvas->hideAll) {
			for(UIContainer* con : canvas->containers) {
				for(UI* ui : con->container) {
					if(ui->Clicked(input->mousePos)) {
						ui_clicked = true;
						goto stop;
					}//TODO(delle) re-add menu dragging
				}
			}
		}
		stop:

		//if the click wasnt on a UI element, trigger select_entity command
		//admin->ExecCommand("select_entity"); //TODO(i,delle) re-enable clicking entities

		//set click pos to mouse pos
		input->mouseClickPos = input->mousePos;
	}
	//mouse left click held
	else if(input->MouseHeld(INPUT_MOUSE_LEFT)) { 
		static_internal Vector2 offset;
		if(input->selectedUI) {
			if(!input->ui_drag_latch) {
				offset = input->selectedUI->pos - input->mousePos;
				input->ui_drag_latch = true;
			}
			input->selectedUI->pos = input->mousePos + offset;
			input->selectedUI->Update();
		}
	} 
	//mouse left click released
	else if(input->MouseReleased(INPUT_MOUSE_LEFT)) {
		if(input->selectedUI) {					//deselect UI
			input->selectedUI = 0;
			input->ui_drag_latch = false;
		} else if(input->selectedEntity && input->mousePos != input->mouseClickPos) { //add force to selected entity
			admin->ExecCommand("add_force");
		}

		//reset click pos to null
		input->mouseClickPos = Vector2(admin->p->ScreenWidth(), admin->p->ScreenHeight());
	}

	if(input->KeyPressed(olc::MINUS)) {
		admin->p->SetMousePositionLocal(admin->p->GetWindowSize() / 2);
	}
}

inline void HandleSelectedEntityInputs(EntityAdmin* admin, Input* input) {
	//translation

	if (!admin->IMGUI_KEY_CAPTURE) {


		if (input->KeyDown(olc::L, INPUT_NONE_HELD)) {
			admin->ExecCommand("translate_right");
		}

		if (input->KeyDown(olc::J, INPUT_NONE_HELD)) {
			admin->ExecCommand("translate_left");
		}

		if (input->KeyDown(olc::O, INPUT_NONE_HELD)) {
			admin->ExecCommand("translate_up");
		}

		if (input->KeyDown(olc::U, INPUT_NONE_HELD)) {
			admin->ExecCommand("translate_down");
		}

		if (input->KeyDown(olc::I, INPUT_NONE_HELD)) {
			admin->ExecCommand("translate_forward");
		}

		if (input->KeyDown(olc::K, INPUT_NONE_HELD)) {
			admin->ExecCommand("translate_backward");
		}

		//rotation
		if (input->KeyDown(olc::L, INPUT_SHIFT_HELD)) {
			admin->ExecCommand("rotate_+x");
		}

		if (input->KeyDown(olc::J, INPUT_SHIFT_HELD)) {
			admin->ExecCommand("rotate_-x");
		}

		if (input->KeyDown(olc::O, INPUT_SHIFT_HELD)) {
			admin->ExecCommand("rotate_+y");
		}

		if (input->KeyDown(olc::U, INPUT_SHIFT_HELD)) {
			admin->ExecCommand("rotate_-y");
		}

		if (input->KeyDown(olc::I, INPUT_SHIFT_HELD)) {
			admin->ExecCommand("rotate_+z");
		}

		if (input->KeyDown(olc::K, INPUT_SHIFT_HELD)) {
			admin->ExecCommand("rotate_-z");
		}
	}
}

inline void HandleRenderInputs(EntityAdmin* admin, Input* input, Keybinds* binds) {
	
	if (!admin->IMGUI_KEY_CAPTURE) {
		//toggle wireframe
		if (input->KeyPressed(binds->debugRenderWireframe, INPUT_NONE_HELD)) {
			admin->ExecCommand("render_wireframe");
		}

		//toggle textures
		if (input->KeyPressed(binds->debugRenderWireframe, INPUT_SHIFT_HELD)) {
			admin->ExecCommand("render_textures");
		}

		//toggle edge numbers
		if (input->KeyPressed(binds->debugRenderEdgesNumbers, INPUT_NONE_HELD)) {
			admin->ExecCommand("render_display_edges");
		}

		//toggle edge numbers
		if (input->KeyPressed(binds->debugRenderDisplayAxis, INPUT_NONE_HELD)) {
			admin->ExecCommand("render_local_axis");
		}

		//toggle edge numbers
		if (input->KeyPressed(binds->debugRenderDisplayAxis, INPUT_SHIFT_HELD)) {
			admin->ExecCommand("render_global_axis");
		}
	}
}

//add generic commands here
void CommandSystem::Init() {
	admin->commands["debug_global"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		GLOBAL_DEBUG = !GLOBAL_DEBUG;
		if (GLOBAL_DEBUG) return "GLOBAL_DEBUG = true";
		else return "GLOBAL_DEBUG = false";
	}, "debug_global", "debug_global");

	admin->commands["debug_command_exec"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		Command::CONSOLE_PRINT_EXEC = !Command::CONSOLE_PRINT_EXEC;
		return ""; //i dont know what this does so im not formatting it 
	}, "debug_command_exec", "debug_command_exec");

	admin->commands["engine_pause"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->paused = !admin->paused;
		if (admin->paused) return "engine_pause = true";
		else return "engine_pause = false";
	}, "engine_pause", "toggles pausing the engine");

	admin->commands["selent_play_sound"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->input->selectedEntity->GetComponent<Source>()->request_play = true;
		return TOSTRING("selected entity playing sound: ", admin->input->selectedEntity->GetComponent<Source>()->snd_file);
	}, "selent_play_sound", "plays a sound from the selected entity");

	AddSpawnCommands(admin);
	AddRenderCommands(admin);
	AddConsoleCommands(admin);
}

void CommandSystem::Update() {
	Input* input = admin->input;
	Keybinds* binds = admin->currentKeybinds;

	input->mousePos = admin->p->GetMousePos();

	HandleMouseInputs(admin, input);
	HandleSelectedEntityInputs(admin, input);
	HandleRenderInputs(admin, input, binds);

	if(input->KeyPressed(olc::F1, INPUT_SHIFT_HELD)) {
		Entity* box = WorldSystem::CreateEntity(admin);
		Transform* t = new Transform(Vector3(-10, 0, 20), Vector3::ZERO, Vector3::ONE);
		Mesh* m = Mesh::CreateBox(box, Vector3(5, 5, 5), t->position);
		Physics* p = new Physics(t->position, t->rotation, 100.f, 0.1f); //heavy concrete cube
		AABBCollider* c = new AABBCollider(box, Vector3(5, 5, 5), p->mass);
		WorldSystem::AddComponentsToEntity(box, {t, m, p, c});

		Entity* sphere = WorldSystem::CreateEntity(admin);
		Transform* t2 = new Transform(Vector3(30, 0, 20), Vector3::ZERO, Vector3::ONE);
		Mesh* m2 = Mesh::CreateBox(sphere, Vector3(.3f, .3f, .3f), t2->position);
		Physics* p2 = new Physics(t2->position, t2->rotation, Vector3(-30, 2, 0)); //light rubber ball
		p2->elasticity = .5f;
		SphereCollider* c2 = new SphereCollider(sphere, 1, p2->mass);
		sphere->AddComponents({t2, m2, p2, c2});

		admin->input->selectedEntity = box;
	}
}