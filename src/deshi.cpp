/* deshi

Style Guidelines
----------------
utlity: (headers only)
  lower_underscore_structs
  lowerCamelMemberVars
  lowerCamelMemberFuncs
core:
  UpperCamelStructs
  UpperCamelNamespaces
  lowerCamelMemberVars
  UowerCamelMemberFuncs
  lower_underscore_unscoped_vars (.cpp only)
  DowhateverLocal_vars (usually lowerCamel)
  g_global_vars (defined in deshi.cpp only, declared in specific header)

Command TODOs
-------------
move commands to their own files (separate for deshi and game)
implement command chaining
command to print all avaliable keys for binding
command to print all keybinds, with (maybe) an option for printing only contextual keybinds
make binds and aliases check if one already exists for a key or a command. if a key already exists probably just overwrite it?
add a component_state command to print state of a component (add str methods to all components/systems)
add device_info command (graphics card, sound device, monitor res, etc)

Console(2) TODOs
-------------
showing a commands help if tab is pressed when the command is already typed
add a setting for a limit to the number of log files
convert all ImGui stuff used in console to UI since console will be in final release
input history from previous inputs on UP and DOWN arrows
add scrolling and scrollbar (PAGEUP and PAGEDOWN binds (CTRL for max scroll))
add a general logging system with log levels and locations (for filtering)
popout and window console states
tabbing so we can sort different kinds of info into each tab like Errors and Warnings
add auto complete for commands and arguments
add console flag for showing text in bottom right message bar like error does
implement filtering console buffer by function and file name (add __FILENAME__ and __FUNCTION__ or whatever it is to the defines)

Fun TODOs
---------
look into implementing Lua (or finish su and make it an embeddable language!)
look into making a function that takes in the types on a component and formats binary for saving and what not 
____like what were currently doing for typeHeader in Admin Save()
write a preprocessing/postprocessing compiler that makes saving easier
hotloadable UI

Math TODOs
----------
add functions and members similar to what glsl/glm has where you can do stuff like 
____v.xy, v.yz, as well as operators for these things if possible. Prefer them to be member variables and not functions :)
____you probably just need to add a vec2/3 for each permutation of each vector
____glm/detail/_swizzle.hpp
cleanup math library (remove redundant/old things, make functions more consistent, etc.)
add quaternions and converions between them and other linear algebra primitives

Render TODOs
------------
add texture/material recreation without restart
revert phong shader so it is like it used to be, but keeps the shadows
fix directional shadow mapping's (projection?) errors
rework lights
add omnidirectional shadow mapping
add not-on-screen object culling thru mesh AABBs
add front-to-back sorting for perf gain (and maybe transparency?)
delete shader .spv if failed to compile it after printing error messages
setup more generalized material/pipeline creation
____specialization constants
____uber shaders
____runtime pipeline creation/specialization
look into getting info from shaders, or setting up compute shaders
____ref: https://github.com/SaschaWillems/Vulkan/blob/master/examples/computeparticles/computeparticles.cpp
____the primary reason being that we need to optimize outlining objects, which will
____involve clipping triangles and stuff
add standard render/video settings
upload extra mesh info to an SSBO
fix texture transparency
____check those vulkan-tutorial links for the suggestions and optimizations
add instancing
vulkan auto-cleanup so that it frees the unused parts of memory every now and then
add buffer pre-allocation and arenas for vertices/indices/textures/etc
multi-threaded command buffers, shader loading, image loading
SSBOs in shaders so we can pass variable length arrays to it

Storage TODOs
-------------
add MTL parsing and extra face info
store null128.png and 
add versionion to Mesh since its saved in a binary format
speedup OBJ parsing and face generation
store fonts in storage

UI TODOs
--------
turn ShowDebugWindowOf into a general metrics/debug window like imgui once we have drop downs and stuff
add functionality for resetting certain maps like windows and inputtexts
____maybe even removing certain labels from them
tabs (vertical and horizontal)
buttons
child windows
add some markup to text like underlining, bold, etc.
add a UI popup when reloading shaders
add UI color palettes for easy color changing
redo debug bar to be more informative and have different modes

Ungrouped TODOs
---------------
add the ability to limit framerate
add a file abstraction so file parsing is simple and not so explicitly handed in different files
memory namespace with arenas and memory management
____funcs: alloc()=Allocate(), zalloc()=ZeroAllocate(), talloc()=TempAllocate()
cleanup utils classes so that they are declaration at top and definition below
centralize the settings files (combine all deshi.cfg and all game.cfg, make them hot-loadable)
update imgui (so we can get disabled items/text)
convert std::string to our string throughout the project, primarily .str() methods so i can fully convert TOSTRING to use our string
make a dynamic timers array in time.h for cleaner timer stuffs
deshi or admin callback function that allows for displaying some sort of indicator that stuff is loading
____the call back function could be on deshi, which updates imgui and/or renderer only and then calls on entity admin
____to update it's canvas system.
remake the sound system
look into integrating TODOP with Discord



Atmos TODOs (move these to atmos at some point)
---------------------------------------------------------------------------------------------------
create a demo level
rework and simplify entity creation so there is a distinction between development and gameplay creation
change entity and admin LoadTEXT to be character based rather than std::string based
fix DESH material and event saving/loading
think of a way to implement different events being sent between comps as right now it's only one
____is this necessary though? we can define these events at run time, but the connections must be made through UI
____so maybe have a UI option that allows the comps update function to handle it and only connects them.
____actually having an option for anything other than collider is kind of useless soooo maybe 
____get rid of event on every component or just only let u choose that event on colliders
pool/arena components and entities for better performance

Physics TODOs
-------------
make collider trigger latching a boolean, so it can continuous trigger an event while an obj is in it
figure out how to handle multi events being set for a collider. for other components its not really an issue
___because u just choose what event to send at run time, but i don't want to do this inside of the collider
___functions themselves, as it would be way too much clutter. maybe a helper function on collider to choose what to do 
redo main physics loop
add physics collision sweeping
add physics based collision resolution for remaining collider primitives
add physics interaction functions
implement collision manifold generation
implement Complex Colliders


Level Editor and Inspector TODOs
------------------
safety check on renaming things so no things have same name
fix mesh viewing bools mixing?
fix extra temp vertexes/indexes (when viewing triangle neighbors)
fix normals viewing on big objects crash https://stackoverflow.com/questions/63092926/what-is-causing-vk-error-device-lost-when-calling-vkqueuesubmit
change undo's to never use pointers and have undos that can act like linked lists to chain them
rewrite the events menu (triggers need to be able to filter what causes them to activate)
add box select for mesh inspector and entity selection
editor settings (world grid, colors, positions)
add safety checks on renaming things so no two meshes/materials/models have the same name
see folders inside different folders when loading things (leading into a asset viewer)
orthographic grabbing/rotating
add transfering the player pointer between entities that have an actor comp (combo in Global Tab)
orbitting camera for rotating around objects
context menu when right clicking on an object 
typing numbers while grabbing/rotating/scaling for precise manipulation (like in Blender)
implement grabbing/rotating/scaling with a visual tool thing (like in Unreal)
orthographic side views
(maybe) multiple viewports
implement orthographic grabbing 
entity filtering in entity list
combine undo manager into editor file
---------------------------------------------------------------------------------------------------

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

//// external for core ////
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "external/stb/stb_image.h"
#include "external/imgui/imgui.cpp"
#include "external/imgui/imgui_demo.cpp"
#include "external/imgui/imgui_draw.cpp"
#include "external/imgui/imgui_tables.cpp"
#include "external/imgui/imgui_widgets.cpp"
#undef ERROR
#undef DELETE

//// utility headers ////
#include "defines.h"
#include "utils/string.h"
#include "utils/cstring.h"
#include "utils/color.h"
#include "utils/tuple.h"
#include "utils/container_manager.h"
#include "utils/utils.h"
#include "utils/optional.h"
#include "utils/debug.h"
#include "utils/ring_array.h"
#include "utils/command.h"
#include "utils/array.h"
#include "utils/hash.h"
#include "utils/map.h"
#include "utils/view.h"
#include "math/math.h"

//// STL for core ////
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <vector>
#include <set>
#include <unordered_map>

//// core headers ////
#include "deshi.h"
#include "core/font.h"

//// renderer cpp (and libs) ////
#if   DESHI_VULKAN
#if defined(_MSC_VER)
#pragma comment(lib,"vulkan-1.lib")
#pragma comment(lib,"glfw3.lib")
#pragma comment(lib,"shaderc_combined.lib")
#endif
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <shaderc/shaderc.h>
#include "external/imgui/imgui_impl_vulkan.cpp"
#include "external/imgui/imgui_impl_glfw.cpp"
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
#endif

//// core cpp ////
#include "core/window.cpp"
#include "core/assets.cpp"
#include "core/console.cpp"
#include "core/console2.cpp"
#include "core/storage.cpp"
#include "core/ui.cpp"
#include "core/commands.cpp"

local Time     time_;   Time*     g_time    = &time_; //time_ because there is a c-func time() D:
local Window   window;  Window*   g_window  = &window;
local Input    input;   Input*    g_input   = &input;
#ifndef DESHI_DISABLE_CONSOLE
local Console  console; Console*  g_console = &console;
#endif
local Storage_ storage; Storage_* g_storage = &storage;

void deshi::init() {
	TIMER_START(t_d); TIMER_START(t_s);
    
	//pre-init setup
	Assets::enforceDirectories();
    
	//init engine core
	TIMER_RESET(t_s); time_.Init(700);        SUCCESS("Finished time initialization in ",              TIMER_END(t_s), "ms");
	TIMER_RESET(t_s); window.Init(1280, 720); SUCCESS("Finished input and window initialization in ",  TIMER_END(t_s), "ms");
#ifndef DESHI_DISABLE_CONSOLE //really ugly lookin huh
	TIMER_RESET(t_s); console.Init(); Console2::Init(); SUCCESS("Finished console initialization in ", TIMER_END(t_s), "ms");
#endif
	TIMER_RESET(t_s); Render::Init();         SUCCESS("Finished render initialization in ",            TIMER_END(t_s), "ms");
	TIMER_RESET(t_s); Storage::Init();        SUCCESS("Finished storage initialization in ",           TIMER_END(t_s), "ms");
#ifndef DESHI_DISABLE_IMGUI
	TIMER_RESET(t_s); DeshiImGui::Init();     SUCCESS("Finished imgui initialization in ",             TIMER_END(t_s), "ms");
#endif
	TIMER_RESET(t_s); UI::Init();             SUCCESS("Finished UI initialization in ",                TIMER_END(t_s), "ms");
	TIMER_RESET(t_s); Cmd::Init();            SUCCESS("Finished Cmd initialization in ",               TIMER_END(t_s), "ms");
    
	SUCCESS("Finished deshi initialization in ", TIMER_END(t_d), "ms");
    
	glfwShowWindow(window.window);
}

void deshi::cleanup() {
	DeshiImGui::Cleanup();
	Render::Cleanup();
	window.Cleanup();
	console.CleanUp(); 
	Console2::Cleanup();
}

bool deshi::shouldClose() {
	glfwPollEvents(); //this maybe should be elsewhere, but i dont want to move glfw includes to deshi.h 
	return glfwWindowShouldClose(window.window) || window.closeWindow;
}