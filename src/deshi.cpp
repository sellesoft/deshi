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
convert all ImGui stuff used in console to UI since console will be in final release
showing a commands help if tab is pressed when the command is already typed
add a setting for a limit to the number of log files
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
store null128.png and null shader in code
add versioning to Mesh since its saved in a binary format
speedup OBJ parsing and face generation

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
add a logging core separate from the console
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

Bug Board       //NOTE mark these with a last-known active date (M/D/Y)
---------
(07/10/21) the program crashes if default asset files are not present
__________ maybe store the text in the actual source and create the file from the code, like keybinds.cfg
(07/14/21) the config parser sometimes throws a console error that its unable to parse the final empty line of configs

*/

//// external for core ////
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb/stb_image.h>
#include <imgui/imgui.cpp>
#include <imgui/imgui_demo.cpp>
#include <imgui/imgui_draw.cpp>
#include <imgui/imgui_tables.cpp>
#include <imgui/imgui_widgets.cpp>
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
#include "memory.h"
#include "deshi.h"
#include "core/font.h"

//// renderer cpp (and libs) ////
#if   DESHI_VULKAN
#if defined(_MSC_VER)
#pragma comment(lib,"vulkan-1.lib")
#pragma comment(lib,"glfw3.lib")
#pragma comment(lib,"shaderc_combined.lib")
#endif //_MSC_VER
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <shaderc/shaderc.h>
#include <imgui/imgui_impl_vulkan.cpp>
#include <imgui/imgui_impl_glfw.cpp>
#include "core/renderers/vulkan.cpp"
#elif DESHI_OPENGL //DESHI_VULKAN
#if defined(_MSC_VER)
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"glfw3.lib")
#endif //_MSC_VER
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include <imgui/imgui_impl_opengl3.cpp>
#include <imgui/imgui_impl_glfw.cpp>
#include "core/renderers/opengl.cpp"
#elif DESHI_DIRECTX //DESHI_OPENGL

#else  //DESHI_DIRECTX
#error "no renderer selected"
#endif //DESHI_VULKAN

//// core cpp ////
#include "core/window.cpp"
#include "core/assets.cpp"
#include "core/console.cpp"
#include "core/console2.cpp"
#include "core/storage.cpp"
#include "core/ui.cpp"
#include "core/commands.cpp"

local Time     deshi_time;    Time*     g_time    = &deshi_time;
local Window   deshi_window;  Window*   g_window  = &deshi_window;
local Input    deshi_input;   Input*    g_input   = &deshi_input;
local Console  deshi_console; Console*  g_console = &deshi_console;
local Storage_ deshi_storage; Storage_* g_storage = &deshi_storage;

void deshi::init(){
	TIMER_START(t_d); TIMER_START(t_s);
	Assets::enforceDirectories();
	TIMER_RESET(t_s); deshi_time.Init(300);         SUCCESS("Finished time initialization in ",              TIMER_END(t_s), "ms");
	TIMER_RESET(t_s); deshi_window.Init("deshi", 1280, 720); SUCCESS("Finished input and window initialization in ",  TIMER_END(t_s), "ms");
#ifndef DESHI_DISABLE_CONSOLE //really ugly lookin huh
	TIMER_RESET(t_s); deshi_console.Init(); Console2::Init(); SUCCESS("Finished console initialization in ", TIMER_END(t_s), "ms");
#endif
	TIMER_RESET(t_s); Render::Init();               SUCCESS("Finished render initialization in ",            TIMER_END(t_s), "ms");
	TIMER_RESET(t_s); Storage::Init();              SUCCESS("Finished storage initialization in ",           TIMER_END(t_s), "ms");
#ifndef DESHI_DISABLE_IMGUI
	TIMER_RESET(t_s); DeshiImGui::Init();           SUCCESS("Finished imgui initialization in ",             TIMER_END(t_s), "ms");
#endif
	TIMER_RESET(t_s); UI::Init();                   SUCCESS("Finished UI initialization in ",                TIMER_END(t_s), "ms");
	TIMER_RESET(t_s); Cmd::Init();                  SUCCESS("Finished commands initialization in ",          TIMER_END(t_s), "ms");
    
	SUCCESS("Finished deshi initialization in ", TIMER_END(t_d), "ms");
    
	glfwShowWindow(deshi_window.window);
}

void deshi::cleanup(){
	DeshiImGui::Cleanup();
	Render::Cleanup();
	deshi_window.Cleanup();
	deshi_console.CleanUp(); 
	Console2::Cleanup();
}

bool deshi::shouldClose(){
	glfwPollEvents(); //this maybe should be elsewhere, but i dont want to move glfw includes to deshi.h 
	return glfwWindowShouldClose(deshi_window.window) || deshi_window.closeWindow;
}