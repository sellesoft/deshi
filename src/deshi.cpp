/* deshi
TODO Tags:
As  Assets    Cl  Clean Up Code    Cmd Command         Co  Core        Con Console
En  Entity    Fu  Fun              Fs  Filesystem      Ge  Geometry    In  Input
Ma  Math      Oth Other            Op  Optimization    Ph  Physics     Re  Render
Sh  Shader    So  Sound            Ui  UI              Vu  Vulkan      Wi  Window

TODO Style: TODO(person,tags) description
	  eg: TODO(delle) no tag or date 
eg: TODO(sushi,ReOp) render,optimization tags for sushi

The person listed doesn't necessarily have to be you, and can be someone else
if you feel they would handle the problem better. It should generally be you though.

Major Ungrouped TODOs
---------------------
move times in deshi.cpp to time.h
add console flag forshowing text in bottom right message bar like error does
add device info command (graphics card, sound device, monitor res, etc)
add a component_state command to print state of a component (add str methods to all components/systems)
add option to not use input callbacks so fps doesnt get affected dramatically by input
add a general logging system with log levels and locations
add shaders: PBR (4textures)
settings file(s) [keybinds, video, audio, etc]
fix program stalling when Keybinds cant find the keybind file
make our own unordered_map and map that is contiguous (array of pairs basically, hash mapped keys)
figure out why selecting sometimes selects outside of an object and sometimes doesnt select inside of an object

Minor Ungrouped TODOs
---------------------
cleanup Triangle and remove unused things
pool/arena components and entities for better performance
implement string returns, better descriptions, and parameter parsing on every command (use spawn_box as reference)
replace/remove external dependencies/includes were possible (glm, tinyobj)
add Qol (quality of life) tag to TODOP
look into integrating TODOP with Discord
add yaxis line
begin reimplementing sound system and maybe rethink its design a bit
fix console color parsing regex to be able to match with brackets inside

Render TODOs
------------
add 2D shader and interface functions
add lighting and shadows
add vertex editing
fix texture transparency
look into adding volk for faster loading/function calls
add RenderSettings loading and usage
check those vulkan-tutorial links for the suggestions
avoid having 3 copies of a mesh (model, meshVK, vulkan)
add instancing
add buffer pre-allocation and arenas for vertices/indices/textures/etc
multi-threaded command buffers, shader loading, image loading
find a way to forward declare vulkan stuff and move the include to the cpp

Level Editor TODOs
------------
orbitting camera for rotating around objects
rotating objects
scaling objects
typing numbers while grabbing/rotating/scaling for precise manipulation (like in Blender)
adding components
manipulating component/entity data through UI
changing materials/textures through UI
highlighing selected object (maybe AABB for now but figure out precise outlining later)
implement grabbing/rotating/scaling with a visual tool thing (like in Unreal)
world axis in top right (like we used to have)
orthographic side views
(maybe) multiple viewports
add showing axis lines through object when axis grabbing once we have lines in Vulkan

Physics/Atmos TODOs
-------------
redo main physics loop
add physics collision sweeping
add physics based collision resolution for remaining collider primitives
add physics interaction functions
finish imlementing AABB vs AABB collisions
implement collision manifold generation
implement Sphere vs Sphere collisions
implement Complex Colliders 

UI TODOs
--------
2D shader
add a UI popup when reloading shaders
add UI color palettes for easy color changing
renaming entities from entity list
redo debug bar to be more informative and have different modes

Math NOTEs: Row-Major matrices, Left-Handed coordinate system (clockwise rotation when looking down axis)
Math TODOs
----------
cleanup math library (remove redundant/old things, make functions more consistent, etc.)
add quaternions and converions between them and other linear algebra primitives
replace glm :)
look into scaling not rotating (scaling is probably being done in world not local)


*/

#include "core.h"
#include "EntityAdmin.h"

Console* g_console;

struct DeshiEngine {
	Time time;               float timeTime = 0;
	Window window;           float windowTime = 0;
	Input input;             float inputTime = 0;
	EntityAdmin entityAdmin; float adminTime = 0;
	Console console;         float consoleTime = 0;
	Renderer renderer;       float renderTime = 0;
	deshiImGui imgui;        float frameTime = 0;
	TIMER_START(t_d); TIMER_START(t_f);
	
	//TODO(delle,Fs) setup loading a config file to a config struct
	void LoadConfig() {
		
	}
	
	void Start() {
		//enforce deshi file system
		deshi::enforceDirectories();
		
		//init core
		LoadConfig();
		time.Init(300); //300 tps for physics
		window.Init(&input, 1280, 720); //inits input as well
		console.Init(&time, &input, &window, &entityAdmin); g_console = &console;
		renderer.Init(&time, &input, &window, &imgui); //inits imgui as well
		
		//init game admin
		entityAdmin.Init(&input, &window, &time, &renderer, &console);
		
		LOG("Finished deshi initialization in ",TIMER_END(t_d),"ms");
		
		//start main loop
		while (!glfwWindowShouldClose(window.window) && !window.closeWindow) {
			glfwPollEvents();
			Update();
		}
		
		//cleanup
		imgui.Cleanup();
		renderer.Cleanup();
		window.Cleanup();
		console.CleanUp();
	}
	
	bool Update() {
		TIMER_RESET(t_d); time.Update();        timeTime = TIMER_END(t_d);
		TIMER_RESET(t_d); window.Update();      windowTime = TIMER_END(t_d);
		TIMER_RESET(t_d); input.Update();       inputTime = TIMER_END(t_d);
		imgui.NewFrame();                                                                  //place imgui calls after this
		TIMER_RESET(t_d); entityAdmin.Update(); adminTime = TIMER_END(t_d);
		TIMER_RESET(t_d); console.Update();     consoleTime = TIMER_END(t_d);
		TIMER_RESET(t_d); renderer.Render();    renderTime = TIMER_END(t_d);       //place imgui calls before this
		//entityAdmin.PostRenderUpdate();
		
		frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
		//LOG("Time: ",timeTime,"ms, Window: ",windowTime,"ms, Input: ",inputTime,"ms, Admin: ",adminTime,"ms, Console: ",consoleTime,"ms, Renderer: ",renderTime,"ms, Frame: ",frameTime,"ms");
		return true;
	}
};

int main() {
	DeshiEngine engine;
	engine.Start();
	
	int debug_breakpoint = 0;
}