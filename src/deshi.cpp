/* deshi
TODO Tags:
As  Assets    Cl  Clean Up Code    Cmd Command         Co  Core        Con Console
En  Entity    Fu  Fun              Fs  Filesystem      Ge  Geometry    In  Input
Ma  Math      Oth Other            Op  Optimization    Ph  Physics     Re  Render
Sh  Shader    So  Sound            Ui  UI              Vu  Vulkan      Wi  Window

TODO Style: TODO(person,tags) description
  eg: TODO(delle) no tag or date for delle
  eg: TODO(sushi,ReOp) render,optimization tags for sushi

The person listed doesn't necessarily have to be you, and can be someone else
if you feel they would handle the problem better. It should generally be you though.

Major Ungrouped TODOs
---------------------
create a demo level
sushi->rewrite the events menu
____also, triggers need to be able to filter what causes them to activate

Minor Ungrouped TODOs
---------------------
add editor settings and config
change text-based saving so material shaders are text rather than ID
rework and simplify entity creation so there is a distinction between development and gameplay creation
make a dynamic timers array on in time.h for cleaner timer stuffs
add a setting for a limit to the number of log files
create a hot-loadable global vars file
detach camera from the renderer so that the camera component isnt calling the renderer
deshi or admin callback function that allows for displaying some sort of indicator that stuff is loading
____the call back function could be on deshi, which updates imgui and/or renderer only and then calls on entity admin
____to update it's canvas system.
think of a way to implement different events being sent between comps as right now it's only one
____is this necessary though? we can define these events at run time, but the connections must be made through UI
____so maybe have a UI option that allows the comps update function to handle it and only connects them.
____actually having an option for anything other than collider is kind of useless soooo maybe 
____get rid of event on every component or just only let u choose that event on colliders
change undo's to never use pointers and have undos that can act like linked lists to chain them
add a general logging system with log levels and locations (for filtering)
add a component_state command to print state of a component (add str methods to all components/systems)
make our own unordered_map and map that is contiguous (array of pairs basically, hash mapped keys)
____also allow it to store up to 3 types
add device_info command (graphics card, sound device, monitor res, etc)
pool/arena components and entities for better performance
replace/remove external dependencies/includes (tinyobj, std)
look into integrating TODOP with Discord
begin reimplementing sound system and maybe rethink its design a bit
fix DESH material and event saving/loading

Render TODOs
------------
fix directional shadow mapping's (projection?) errors
rework lights
add temporary meshes (get reset every frame like imgui)
extract normal debug geometry shader descriptor from generic layout and sets
add omnidirectional shadow mapping
add not-on-screen object culling thru mesh AABBs
add front-to-back sorting for perf gain (and maybe transparency?)
delete shader .spv if failed to compile it after printing error messages
setup more generalized material/pipeline creation
____specialization constants
____uber shaders
____runtime pipeline creation/specialization
redo how lights are stored
look into getting info from shaders, or setting up compute shaders
____ref: https://github.com/SaschaWillems/Vulkan/blob/master/examples/computeparticles/computeparticles.cpp
____the primary reason being that we need to optimize outlining objects, which will
____involve clipping triangles and stuff
redo MeshVk so its only child meshes
____avoid having 3 copies of a mesh (model, meshVK, vulkan)
add standard render/video settings
add face normal and tangents to vertex buffer
fix texture transparency
____check those vulkan-tutorial links for the suggestions and optimizations
add instancing
add buffer pre-allocation and arenas for vertices/indices/textures/etc
multi-threaded command buffers, shader loading, image loading
SSBOs in shaders so we can pass variable length arrays to it

Level Editor and Inspector TODOs
------------------
orthographic grabbing/rotating
add transfering the player pointer between entities that have an actor comp (combo in Global Tab)
orbitting camera for rotating around objects
context menu when right clicking on an object 
scaling objects
typing numbers while grabbing/rotating/scaling for precise manipulation (like in Blender)
implement grabbing/rotating/scaling with a visual tool thing (like in Unreal)
world axis in top right (like we used to have)
orthographic side views
(maybe) multiple viewports
add showing axis lines through object when axis grabbing once we have lines in Vulkan
entity filtering in entity list
combine undo manager into editor file

Physics/Atmos TODOs
-------------
make collider trigger latching a boolean, so it can continuous trigger an event while an obj is in it
____dont forget that it's in a physics tick though, so you have to make sure it doesn't do it 300 times per frame
figure out how to handle multi events being set for a collider. for other components its not really an issue
___because u just choose what event to send at run time, but i don't want to do this inside of the collider
___functions themselves, as it would be way too much clutter. maybe a helper function on collider to choose what to do 
redo main physics loop
add physics collision sweeping
add physics based collision resolution for remaining collider primitives
add physics interaction functions
implement collision manifold generation
implement Complex Colliders

Console TODOs
-------------
!!!implement console popout
tabbing so we can sort different kinds of info into each tab like Errors and Warnings
implement command chaining
add auto complete for commands and arguments
add console flag for showing text in bottom right message bar like error does
command to print all avaliable keys for binding
command to print all keybinds, with (maybe) an option for printing only contextual keybinds
make binds and aliases check if one already exists for a key or a command. if a key already exists probably just overwrite it?
implement filtering console buffer by function and file name (add __FILENAME__ and __FUNCTION__ or whatever it is to the defines)

UI TODOs
--------
look into easier hover checking and input intercepting for imgui
____https://github.com/ocornut/imgui/issues/52
add a UI popup when reloading shaders
add UI color palettes for easy color changing
redo debug bar to be more informative and have different modes
(maybe) implement a way to push data to something in the DebugLayer
____sort of how we had before with BufferLog so you can see it without opening console

Math TODOs
----------
add functions and members similar to what glsl/glm has where you can do stuff like 
____v.xy, v.yz, as well as operators for these things if possible. Prefer them to be member variables and not functions :)
____you probably just need to add a Vector2/3 for each permutation of each vector
____glm/detail/_swizzle.hpp
cleanup math library (remove redundant/old things, make functions more consistent, etc.)
add quaternions and converions between them and other linear algebra primitives

Fun TODOs
---------
look into implementing Lua 
look into making a function that takes in the types on a component and formats binary for saving and what not 
____like what were currently doing for typeHeader in Admin Save()
write a preprocessing/postprocessing compiler that makes saving easier
hotloadable UI

Bug Board       //NOTE mark these with a last-known active date (M/D/Y)
---------
(06/13/21) rotating using R no longer seems to work, it wildly rotates the object
__________ it might have something to do with our rotate by axis function
(07/10/21) the program crashes if default asset files are not present
__________ maybe store the text in the actual source and create the file from the code, like keybinds.cfg
(07/14/21) the config parser sometimes throws a console error that its unable to parse the final empty line of configs
(07/20/21) copy/paste produces an extra mesh in the renderer sometimes
(08/03/21) UI's text sometimes produces artifacts around some letters. it seems like it depends
__________ on the y level of each character and only seems to happen on a, b, i, j, q, r, and z on gohufont-11

*/


#include <iostream>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <vector>
#include <set>

#include "defines.h"
#include "utils/color.h"
#include "utils/tuple.h"
#include "utils/containermanager.h"
#include "utils/utils.h"
#include "utils/optional.h"
#include "utils/debug.h"
#include "utils/ringarray.h"
#include "utils/command.h"
#include "utils/font.h"
#include "utils/array.h"
#include "utils/map.h"
#include "utils/hash.h"
#include "math/math.h"
#include "scene/Scene.h"

#include "core/assets.h"
#include "core/console.h"
#include "core/console2.h"
#include "core/imgui.h"
#include "core/input.h"
#include "core/renderer.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"
#include "game/admin.h"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"
#include "external/imgui/imgui_impl_glfw.h"

#if   DESHI_VULKAN //DESHI_RENDERER
#if defined(_MSC_VER)
#pragma comment(lib,"vulkan-1.lib")
#pragma comment(lib,"glfw3.lib")
#pragma comment(lib,"shaderc_combined.lib")
#endif
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <shaderc/shaderc.h>
#include "external/imgui/imgui_impl_vulkan.h"

#include "core/renderers/vulkan.cpp"
#elif DESHI_OPENGL
#if defined(_MSC_VER)
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"glfw3.lib")
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "core/renderers/opengl.cpp"
#elif DESHI_DIRECTX

#else
Assert(!"no renderer selected");
#endif //DESHI_RENDERER

#include "core/window.cpp"
#include "core/assets.cpp"
#include "core/console.cpp"
#include "core/console2.cpp"
#include "core/ui.cpp"

local Time    time_;   Time*    g_time    = &time_; //time_ because there is a c-func time() D:
local Window  window;  Window*  g_window  = &window;
local Input   input;   Input*   g_input   = &input;
local Console console; Console* g_console = &console;
local Admin   admin;   Admin*   g_admin   = &admin;

int main() {
	TIMER_START(t_d); TIMER_START(t_f); TIMER_START(t_s);
	//pre-init setup
	Assets::enforceDirectories();
	//init engine core
	TIMER_RESET(t_s); time_.Init(700);        SUCCESS("Finished time initialization in ",              TIMER_END(t_s), "ms");
	TIMER_RESET(t_s); window.Init(1280, 720); SUCCESS("Finished input and window initialization in ",  TIMER_END(t_s), "ms");
	TIMER_RESET(t_s); console.Init(); Console2::Init(); SUCCESS("Finished console initialization in ", TIMER_END(t_s), "ms");
	TIMER_RESET(t_s); Render::Init();         SUCCESS("Finished render initialization in ",            TIMER_END(t_s), "ms");
	TIMER_RESET(t_s); DeshiImGui::init();     SUCCESS("Finished imgui initialization in ",             TIMER_END(t_s), "ms");
	TIMER_RESET(t_s); UI::Init();			  SUCCESS("Finished UI initialization in ",                TIMER_END(t_s), "ms");
	SUCCESS("Finished deshi initialization in ", TIMER_END(t_d), "ms");

	array<vec3> t{ vec3(1,2,3), vec3(3,2,1), vec3(2,1,3) };
	t.swap(0, 1);
	t.swap(1, 2);

	//init game admin
	TIMER_RESET(t_s); admin.Init();           SUCCESS("Finished game initialization in ", TIMER_END(t_s), "ms");
	SUCCESS("Finished total initialization in ", TIMER_END(t_d), "ms\n");
	//start main loop
	while (!glfwWindowShouldClose(window.window) && !window.closeWindow) {
		

		glfwPollEvents();
		
		DeshiImGui::newFrame();                                                         //place imgui calls after this
		TIMER_RESET(t_d); time_.Update();           time_.timeTime   = TIMER_END(t_d);
		TIMER_RESET(t_d); window.Update();          time_.windowTime = TIMER_END(t_d);
		TIMER_RESET(t_d); input.Update();           time_.inputTime  = TIMER_END(t_d);
		TIMER_RESET(t_d); admin.Update();           time_.adminTime  = TIMER_END(t_d);
		TIMER_RESET(t_d); console.Update(); Console2::Update(); time_.consoleTime = TIMER_END(t_d);
		TIMER_RESET(t_d); Render::Update();         time_.renderTime = TIMER_END(t_d);  //place imgui calls before this
		UI::Update();
		TIMER_RESET(t_d); admin.PostRenderUpdate(); time_.adminTime += TIMER_END(t_d);
		{//debugging area
			//setTrack();
			UI::PushVar(UIStyleVar_TitleTextAlign, vec2(0.01, 0.5));
			UI::BeginWindow("test", vec2(300, 300), vec2(300, 300));
			
			string tosend = "";
			for (int i = 32; i < 191 + 32; i++) {
				tosend += (char)i;
				//tosend += '\n';
			}
			
			UI::Text(tosend);
			
			for (int i = 0; i < 100; i++) {
				UI::Text("wow");
			}

			
			UI::EndWindow();
			UI::PopVar();
		}
    	time_.frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}
	
	//cleanup
	admin.Cleanup();
	DeshiImGui::cleanup();
	Render::Cleanup();
	window.Cleanup();
	console.CleanUp(); Console2::Cleanup();
	
#if 0
	DEBUG_BREAK;
#endif
	return 0;
}