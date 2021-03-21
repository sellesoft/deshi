/*
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

#include "components/Component.h"
#include "components/Camera.h"
#include "components/Keybinds.h"
#include "components/Controller.h"
#include "components/Canvas.h"
#include "components/Console.h"
#include "components/AudioListener.h"

#include "systems/System.h"
#include "systems/CommandSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/RenderCanvasSystem.h"
#include "systems/WorldSystem.h"
#include "systems/ConsoleSystem.h"
#include "systems/SoundSystem.h"

//// EntityAdmin ////

void EntityAdmin::Init(Input* i, Window* w, Time* t, Renderer* r) {
	window = w;
	input = i;
	time = t;
	renderer = r;
	
	g_cBuffer.allocate_space(100);
	
	systems = std::vector<System*>();
	entities = std::map<EntityID, Entity*>();
	components = std::vector<Component*>();
	commands = std::map<std::string, Command*>();
	physicsWorld = new PhysicsWorld();
	
	//reserve complayers
	for (int i = 0; i < 8; i++) {
		freeCompLayers.push_back(ContainerManager<Component*>());
	}
	
	//systems initialization
	AddSystem(new CommandSystem());
	switch (physicsWorld->integrationMode) {
		default: /* Semi-Implicit Euler */ {
			AddSystem(new PhysicsSystem());
		}
	}
	AddSystem(new RenderCanvasSystem());
	console = new Console();
	AddSystem(new ConsoleSystem());
	AddSystem(new WorldSystem());
	AddSystem(new SoundSystem());
	
	//singleton initialization
	currentCamera = new Camera(this);
	currentCamera->layer_index = freeCompLayers[currentCamera->layer].add(currentCamera);
	currentKeybinds = new Keybinds(this);
	controller = new Controller(this);
	tempCanvas = new Canvas();
}

void EntityAdmin::Cleanup() {
	//cleanup collections
	for(System* s : systems)       { delete s; }           systems.clear();
	for(auto pair : entities)      { delete pair.second; } entities.clear();
	for(Component* c : components) { delete c; }           components.clear();
	for(auto pair : commands)      { delete pair.second; } commands.clear();
	delete physicsWorld;
	
	//clean up singletons
	delete world;
	delete currentCamera;
	delete currentKeybinds;
	delete controller;
	delete tempCanvas;
}

void UpdateLayer(ContainerManager<Component*> cl) {
	for (int i = 0; i < cl.size(); i++) {
		if (cl[i]) {
			cl[i].get()->Update();
		}
	}
}

void EntityAdmin::Update() {

	//aha
	if (!pause_command)	           UpdateLayer(freeCompLayers[CL0_COMMAND]);	 
	if (!pause_command)	           systems[0]->Update(); //Command system
	if (!pause_phys && !paused)    UpdateLayer(freeCompLayers[CL1_PHYSICS]);	 
	if (!pause_phys && !paused)    systems[1]->Update(); //Physics System
	if (!pause_canvas)	           UpdateLayer(freeCompLayers[CL2_RENDCANVAS]); 
	if (!pause_canvas)	           systems[2]->Update(); //Canvas system
	if (!pause_console)            UpdateLayer(freeCompLayers[CL3_CONSOLE]);	 
	if (!pause_console)            systems[3]->Update(); //Console System
	if (!pause_world && !paused)   UpdateLayer(freeCompLayers[CL4_WORLD]);		 
	if (!pause_world && !paused)   systems[4]->Update(); //World system
	if (!pause_sound && !paused)   UpdateLayer(freeCompLayers[CL5_SOUND]);		 
	if (!pause_sound && !paused)   systems[5]->Update(); //Sound System
	if (!pause_last && !paused)    UpdateLayer(freeCompLayers[CL6_LAST]);

	//NOTE temporary
	controller->Update();
	currentCamera->Update();
	for(Component* c : components){
		c->Update();
	}
}

void EntityAdmin::AddSystem(System* system) {
	systems.push_back(system);
	system->admin = this;
	system->Init();
}

void EntityAdmin::RemoveSystem(System* system) {
	for(int i = 0; i < systems.size(); ++i) {
		if(systems[i] == system) {
			systems.erase(systems.begin() + i);
		}
	}
}

void EntityAdmin::AddComponent(Component* component) {
	components.push_back(component);
	component->entity = 0;
}

void EntityAdmin::RemoveComponent(Component* component) {
	for(int i = 0; i < components.size(); ++i) {
		if(components[i] == component) {
			components.erase(components.begin() + i);
		}
	}
}

Command* EntityAdmin::GetCommand(std::string command) {
	try {
		return commands.at(command);
	} catch(std::exception e) {
		//ERROR("Command \"", command, "\" does not exist");
		this->GetSystem<ConsoleSystem>()->PushConsole(TOSTRING("\n[c:red]", "Command \"", command, "\" does not exist", "[c]"));
		return 0;
	}
}

bool EntityAdmin::ExecCommand(std::string command) {
	try {
		commands.at(command)->Exec(this);
		return true;
	} catch(std::exception e) {
		//ERROR("Command \"", command, "\" does not exist");
		this->GetSystem<ConsoleSystem>()->PushConsole(TOSTRING("\n[c:red]", "Command \"", command, "\" does not exist", "[c]"));
		return false;
	}
}

bool EntityAdmin::ExecCommand(std::string command, std::string args) {
	try{
		commands.at(command)->Exec(admin, args);
		return true;
	}catch(std::exception e){
		this->GetSystem<ConsoleSystem>()->PushConsole(TOSTRING("\n[c:red]", "Command \"", command, "\" does not exist", "[c]"));
		return false;
	}
}


//// Entity ////

uint32 Entity::AddComponent(Component* component) {
	components.push_back(component);
	component->entity = this;
	return components.size()-1;
}

uint32 Entity::AddComponents(std::vector<Component*> comps) {
	uint32 value = this->components.size();
	for(auto& c : comps) {
		this->components.push_back(c);
		c->entity = this;
	}
	return value;
}

Entity::~Entity() {
	for(Component* c : components) delete c;
}