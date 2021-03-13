/*
---Systems Tick Order---||--------Read/Write Components-----||------------Read Only Components-------------------
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
#include "core/deshi.h"

#include "utils/PhysicsWorld.h"
#include "utils/Command.h"
#include "utils/defines.h"

#include "components/Component.h"
#include "components/World.h"
#include "components/Camera.h"
#include "components/Keybinds.h"
#include "components/MovementState.h"
#include "components/Canvas.h"
#include "components/Console.h"
#include "components/Listener.h"

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
	//AddSystem(new CameraSystem());
	//AddSystem(new MeshSystem());
	AddSystem(new RenderCanvasSystem());
	AddSystem(new WorldSystem());
	//AddSystem(new TriggeredCommandSystem());

	console = new Console();
	AddSystem(new ConsoleSystem());

	AddSystem(new SoundSystem());
	
	
	
	//singleton initialization
	world = new World();
	
	//current admin components
	currentCamera = new Camera(this);
	currentCamera->layer_index = freeCompLayers[currentCamera->layer].add(currentCamera);
	
	currentKeybinds = new Keybinds(this);
	
	//temporary singletons
	tempMovementState = new MovementState(this);
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
	delete tempMovementState;
	delete tempCanvas;
}

void EntityAdmin::Update() {
	if (!paused) {
		for (System* s : systems) {
			steady_clock::time_point startTime = steady_clock::now(); //TODO(,delle) test that system durations work
			s->Update();
			s->time = duration_cast<duration<double>>(steady_clock::now() - startTime).count();
		}
	}
	else {
		for (System* s : systems) {
			//if      (ScreenSystem* a = dynamic_cast<ScreenSystem*>(s))             { a->Update(); }
			if (ConsoleSystem* b = dynamic_cast<ConsoleSystem*>(s))           { b->Update(); }
			else if (CommandSystem* c = dynamic_cast<CommandSystem*>(s))           { c->Update(); }
			else if (RenderCanvasSystem* e = dynamic_cast<RenderCanvasSystem*>(s)) { e->Update(); }
			//else if (TimeSystem* f = dynamic_cast<TimeSystem*>(s))                 { f->Update(); }
		}
	}
	//NOTE temporary
	currentCamera->Update();
	tempMovementState->Update();
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