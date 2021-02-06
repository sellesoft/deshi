/* General TODOs and NOTEs board
   TODOs should be ordered about NOTEs and TODOs should be listed in order of
   severity.

Tags: + GitIssue, s Severe, u Unimportant, p Physics, r Render, e Entity, i Input,
	  m Math, o Optimization, g General, c Clean Up Code

TODO(p,delle) add physics based collision resolution for all entities

*/

/* TODOs

1.  create templated component tuple iterator that loops thru a vector and returns an iterator of components of templated type
2.  store Light and other components on entities
3.  cut away alot of triangle
4.  store all components in an object pool so that i can loop over that instead of entities [combine with 1]
5.  add looking up/down
6.  cleanup some warnings
7.  cut down physics to be better
8.  figure out why rotation degenerates in collision, interpolation, and at higher than max velocities
9.  local y-axis rotation is visually wrong
10. add a .str() method to Component.h
11. finish Scene.h render option todos
12. spawning some complexes doesnt work
13. fix rotation interpolation
14. add auto organization to UI
15. implement string returns, better descriptions, and parameter parsing on every command (use spawn_box as reference)

*/

/*
---Systems Tick Order---||--------Read/Write Components-----||------------Read Only Components-------------------
  olcPixelGameEngine	|| Input							|| N/A
  TimeSystem			|| Time								|| N/A
  ScreenSystem			|| Screen							|| N/A
  CommandSystem			|| Input, Keybinds, Canvas			|| N/A 
  SimpleMovementSystem	|| Camera							|| Input, Keybinds, MovementState, Time
  PhysicsSystem			|| Time, Transform, Physics			|| Camera, Screen
  CameraSystem			|| Camera							|| Screen
  MeshSystem			|| Mesh								|| Transform
  RenderSceneSystem		|| Scene							|| Mesh, Camera, Screen, Light, Time, 
						||									||	Transform, Physics
  RenderCanvasSystem	|| Canvas							|| Screen
  WorldSystem			|| World, Entity					|| N/A
  TriggeredCommandSystem|| N/A								|| N/A
  DebugSystem			|| ALL								|| ALL
*/

#include "EntityAdmin.h"						//UsefulDefines.h, Debug.h
#include "utils/PhysicsWorld.h"					//
#include "utils/Command.h"						//Debug.h
#include "utils/defines.h"				//olcPixelGameEngine.h
//#include "math/Math.h"						//UsefulDefines.h, Vector3.h, Vector4.h, Matrix3.h, Matrix4.h, MatrixN.h,
												//	<math.h>, <algorithm>, <numeric>
//#include "geometry/Edge.h"					//Math.h
//#include "geometry/Triangle.h"				//Math.h, Edge.h


//component includes
#include "components/Component.h"				//UsefulDefines.h, <vector>
#include "components/Input.h"					//Component.h, Vector3.h
#include "components/Screen.h"					//Component.h, Vector3.h
#include "components/Time.h"					//Component.h, <time.h>
#include "components/World.h"					//Component.h
#include "components/Camera.h"					//Component.h, Vector3.h, Matrix4.h
#include "components/Keybinds.h"				//Component.h
#include "components/MovementState.h"			//Component.h
#include "components/Scene.h"					//Component.h
#include "components/Canvas.h"					//Component.h, UI.h
#include "components/Console.h"
#include "components/Listener.h"
//#include "components/Mesh.h"					//Component.h, Vector3.h, Triangle.h, Armature.h
//#include "components/Light.h"					//Component.h, Vector3.h 
//#include "components/Physics.h"				//Component.h, Vector3.h
//#include "components/Transform.h"				//Component.h, Vector3.h, Matrix4.h

//system includes
#include "systems/System.h"						//EntityAdmin.h
//#include "systems/TimeSystem.h"					//System.h |cpp->| Time.h, Command.h
//#include "systems/ScreenSystem.h"				//System.h |cpp->| Screen.h
#include "systems/CommandSystem.h"				//System.h |cpp->| Command.h, Input.h, Canvas.h
//#include "systems/SimpleMovementSystem.h"		//System.h |cpp->| Input.h, Keybinds.h, Camera.h, MovementState.h, Time.h
#include "systems/PhysicsSystem.h"				//System.h |cpp->| PhysicsWorld.h, Math.h, Transform.h, Physics.h, Input.h, Command.h, Input.h, Time.h, Camera.h, Screen.h
//#include "systems/CameraSystem.h"				//System.h |cpp->| Camera.h, Screen.h, Command.h
//#include "systems/MeshSystem.h"					//System.h |cpp->| Mesh.h, Transform.h, Physics.h, Command.h, Input.h, Camera.h, Scene.h, Screen.h, Light.h
#include "systems/RenderSceneSystem.h"			//System.h |cpp->| Math.h, Scene.h, Mesh.h, Camera.h, Light.h, Screen.h, Transform.h, Command.h
#include "systems/RenderCanvasSystem.h"			//System.h |cpp->| Canvas.h, Screen.h
#include "systems/WorldSystem.h"				//System.h |cpp->| World.h, Transform.h, Mesh.h, Command.h, Input.h
//#include "systems/TriggeredCommandSystem.h"		//System.h |cpp->| Command.h
#include "systems/ConsoleSystem.h"				//System.h |cpp->| Console.h
#include "systems/SoundSystem.h"

//// EntityAdmin ////

void EntityAdmin::Create() {
	g_cBuffer.allocate_space(100);

	systems = std::vector<System*>();
	entities = std::map<EntityID, Entity*>();
	components = std::vector<Component*>();
	commands = std::map<std::string, Command*>();
	physicsWorld = new PhysicsWorld();

	//singleton initialization
	input = new Input();
	screen = new Screen();
	time = new Time();
	world = new World();

	//current admin components
	currentCamera = new Camera();
	currentScene = new Scene();
	currentKeybinds = new Keybinds();

	//temporary singletons
	tempMovementState = new MovementState();
	tempCanvas = new Canvas();

	console = new Console();

	//systems initialization
	//AddSystem(new TimeSystem());
	//AddSystem(new ScreenSystem());
	AddSystem(new CommandSystem());
	//AddSystem(new SimpleMovementSystem());
	switch(physicsWorld->integrationMode) {
		default: /* Semi-Implicit Euler */ {
			AddSystem(new PhysicsSystem());
		}
	}
	//AddSystem(new CameraSystem());
	//AddSystem(new MeshSystem());
	AddSystem(new RenderSceneSystem());
	AddSystem(new RenderCanvasSystem());
	AddSystem(new WorldSystem());
	//AddSystem(new TriggeredCommandSystem());
	AddSystem(new ConsoleSystem());
	AddSystem(new SoundSystem());
	
#ifdef DEBUG_P3DPGE
	AddSystem(new DebugSystem());
#endif	
}

void EntityAdmin::Cleanup() {
	//cleanup collections
	for(System* s : systems)		{ delete s; }			systems.clear();
	for(auto pair : entities)		{ delete pair.second; }	entities.clear();
	for(Component* c : components)	{ delete c; }			components.clear();
	for(auto pair : commands)		{ delete pair.second; }	commands.clear();
	delete physicsWorld;

	//clean up singletons
	delete input;
	delete screen;
	delete time;
	delete world;
	delete currentCamera;
	delete currentKeybinds;
	delete tempMovementState;
	delete currentScene;
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
			else if (RenderSceneSystem* d = dynamic_cast<RenderSceneSystem*>(s))   { d->Update(); }
			else if (RenderCanvasSystem* e = dynamic_cast<RenderCanvasSystem*>(s)) { e->Update(); }
			//else if (TimeSystem* f = dynamic_cast<TimeSystem*>(s))                 { f->Update(); }
			
		}
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
		ERROR("Command \"", command, "\" does not exist");
		return 0;
	}
}

bool EntityAdmin::ExecCommand(std::string command) {
	try {
		commands.at(command)->Exec(this);
		return true;
	} catch(std::exception e) {
		//ERROR("Command \"", command, "\" does not exist");
		ERROR("Command \"", command, "\" does not exist");
		return false;
	}
}

bool EntityAdmin::TriggerCommand(std::string command) {
	try {
		commands.at(command)->triggered = true;
		return true;
	} catch(std::exception e) {
		//ERROR("Command \"", command, "\" does not exist");
		ERROR("Command \"", command, "\" does not exist");
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