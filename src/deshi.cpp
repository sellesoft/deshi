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
use Logger instead of directly adding text to the console
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
write a preprocessing/postprocessing compiler that makes saving easier
hotloadable UI

IO TODOs
--------
safety checks for IO operations
add search filters to get_directory_files
data folder specified on launch
text file parser (and cleanup locations doing it manually)
 linux/mac IO

Math TODOs
----------
move geometry funcs out of math.h 
add quaternions and converions between them and other linear algebra primitives
add functions and members similar to what glsl/glm has where you can do stuff like
____v.xy, v.yz, as well as operators for these things if possible. Prefer them to be member variables and not functions :)
____you probably just need to add a vec2/3 for each permutation of each vector
____glm/detail/_swizzle.hpp

Render TODOs
------------
figure out how to use custom allocators with opengl3
remove usage of STL
replace allocator with temp_allocator in relevant places
rework the lines drawing algorithm and move it to a more appropriate spot like UI or suugu
make functions for exposing render's 2D vertex and index arrays so the app can freely make custom
____2D shapes
give text its own stuff in renderer so it can have different settings from other UI (filtering,antialiasing,etc)
find a nice way to not pass Font* to DrawText2D: maybe fixed fonts rather than array? maybe set active font?
add texture/material recreation without restart
revert phong shader so it is like it used to be, but keeps the shadows
fix directional shadow mapping's (projection?) errors
rework lights
add omnidirectional shadow mapping
add not-on-screen object culling thru mesh AABBs (if they dont cast shadow)
add front-to-back sorting for perf gain and transparency?
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
multi-threaded command buffers, shader loading, image loading
SSBOs in shaders so we can pass variable length arrays to it

Storage TODOs
-------------
separate physics mesh info from regular mesh info
merge mesh faces with <10 degree normal difference (for physics)
add edges and hulls to meshes, remove unused vars
add MTL parsing
store null128.png and null shader in code
add versioning to Mesh since its saved in a binary format
speedup OBJ parsing and face generation

UI TODOs
--------
remove BeginChild and EndChild and just do nested window stuff with Begin and End
add functionality for resetting certain maps like windows and inputtexts
____maybe even removing certain labels from them
tabs (vertical and horizontal)
child windows
add some markup to text like underlining, bold, etc.
add a UI popup when reloading shaders
add UI color palettes for easy color changing

Ungrouped TODOs
---------------
restyle map to match the rest of utils
make the most recent logging file be named log.txt, while the rest have a date
allow the generic memory arena to grow if it will be maxed out
add MouseInsideWindow() func to input or window
make the transparent framebuffer a start switch since it hurts frames (it must be set at window creation time)
add the ability to limit framerate
centralize the settings files (combine all deshi.cfg and all game.cfg, make them hot-loadable)
convert std::string to our string throughout the project, primarily .str() methods so i can fully convert toStr to use our string
make a dynamic timers array in time.h for cleaner timer stuffs (push/peek/pop)
deshi or admin callback function that allows for displaying some sort of indicator that stuff is loading
____the call back function could be on deshi, which updates imgui and/or renderer only and then calls on entity admin
____to update it's canvas system.
remake the sound system
look into integrating TODOP with Discord

Bug Board       //NOTE mark these with a last-known active date (M/D/Y)
---------
(07/10/21) the program crashes if default asset files are not present
__________ maybe store the text in the actual source and create the file from the code, like keybinds.cfg
__________ alternatively, we can store those specific assets in the source control
(09/13/21) the program sometimes hangs on close in log file writing to stdout; temp fix: click the cmd, hit enter
__________ this might not be an error with our stuff and just a quirk of the windows console



*/

#include "defines.h"
#include "core/memory.h" //this is included above everything so things can reference deshi_allocator

//// utility headers ////"
//#define DESHI_ARRAY_ALLOCATOR deshi_allocator
//#define DESHI_STRING_ALLOCATOR deshi_allocator
#include "utils/array.h"
#include "utils/string.h"
#include "utils/cstring.h"
#include "utils/color.h"
#include "utils/tuple.h"
#include "utils/container_manager.h"
#include "utils/utils.h"
#include "utils/optional.h"
#include "utils/debug.h"
#include "utils/ring_array.h"
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

//// platform ////
#if   DESHI_WINDOWS
#undef function
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define function static
#undef ERROR
#undef DELETE
#elif DESHI_LINUX //DESHI_WINDOWS

#elif DESHI_MAC   //DESHI_LINUX

#else             //DESHI_MAC
#error "unknown platform"
#endif //DESHI_WINDOWS

enum DeshiStage_ {
	DS_NONE = 0,
	DS_MEMORY = 1 << 0,
	DS_LOGGER = 1 << 1,
	DS_CONSOLE = 1 << 2,
	DS_TIME = 1 << 3,
	DS_WINDOW = 1 << 4,
	DS_RENDER = 1 << 5,
	DS_STORAGE = 1 << 6,
	DS_UI = 1 << 7,
	DS_CMD = 1 << 8,
}; typedef u32 DeshiStage;

DeshiStage deshiStage = DS_NONE;

#define AssertDS(stages, ...) Assert((deshiStage & (stages)) == (stages))
#define IsDeshiModuleLoaded (deshiStage & (stages)) == (stages)

//// core headers ////
#include "deshi.h"
#include "defines.h"
#include "core/assets.h"
#include "core/camera.h"
#include "core/commands.h"
#ifndef DESHI_DISABLE_CONSOLE
#  include "core/console.h"
#  include "core/console2.h"
#endif //DESHI_DISABLE_CONSOLE
#include "core/font.h"
#ifndef DESHI_DISABLE_IMGUI
#  define IMGUI_DEFINE_MATH_OPERATORS
#  include "core/imgui.h"
#endif //DESHI_DISABLE_IMGUI
#include "core/input.h"
#include "core/io.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/model.h"
#include "core/renderer.h"
#include "core/storage.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"

//// external for core ////
#define STB_IMAGE_IMPLEMENTATION
//#define STB_TRUETYPE_IMPLEMENTATION
#define STBI_MALLOC(sz) Memory::Allocate(sz)
#define STBI_REALLOC(p,newsz) Memory::Reallocate(p,newsz)
#define STBI_FREE(p) Memory::ZeroFree(p)
#define STBI_FAILURE_USERMSG
#include <stb/stb_image.h>
//#include <stb/stb_truetype.h> //included by imgui
#include <imgui/imgui.cpp>
#include <imgui/imgui_demo.cpp>
#include <imgui/imgui_draw.cpp>
#include <imgui/imgui_tables.cpp>
#include <imgui/imgui_widgets.cpp>

//// renderer cpp (and libs) ////
#if   DESHI_VULKAN
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <shaderc/shaderc.h>
#include <imgui/imgui_impl_vulkan.cpp>
#include <imgui/imgui_impl_glfw.cpp>
#include "core/renderers/vulkan.cpp"
#elif DESHI_OPENGL //DESHI_VULKAN
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include <imgui/imgui_impl_opengl3.cpp>
#include <imgui/imgui_impl_glfw.cpp>
#include "core/renderers/opengl.cpp"
#elif DESHI_DIRECTX12 //DESHI_OPENGL
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "d3dx12/d3dx12.h"
#include <d3d12.h>
#include <wrl/client.h> //ComPtr
#include <dxgi1_6.h>
//#include <d3dcompiler.h> this is for compiling HLSL shaders at runtime, which ideally we wont do, but ill keep it just incase
// if we do, dont forget to link against d3dcompiler.lib and copy D3dcompiler_47.dll to the same file as our exe
#include <DirectXMath.h>
#include "core/renderers/directx.cpp"
#else  //DESHI_DIRECTX12
#error "no renderer selected"
#endif //DESHI_VULKAN

//// core cpp ////
#include "core/io.cpp"
#include "core/memory.cpp"
#include "core/logger.cpp"
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

void deshi::init(u32 winWidth, u32 winHeight){
	TIMER_START(t_s);
	Assets::enforceDirectories();
	Memory::Init(Gigabytes(1), Gigabytes(1));
	Console2::Init();
	Logger::Init(5);
	deshi_time.Init();
	deshi_window.Init("deshi", winWidth, winHeight);
#ifndef DESHI_DISABLE_CONSOLE //really ugly lookin huh
	deshi_console.Init();
#endif
	Render::Init();
	Storage::Init();
#ifndef DESHI_DISABLE_IMGUI
	DeshiImGui::Init();
#endif
	UI::Init();
	Cmd::Init();
	
	glfwShowWindow(deshi_window.window);
	Log("deshi","Finished deshi initialization in ",TIMER_END(t_s),"ms");
}

void deshi::cleanup(){
	DeshiImGui::Cleanup();
	Render::Cleanup();
	deshi_window.Cleanup();
	deshi_console.Cleanup(); Console2::Cleanup();
	Logger::Cleanup();
}

bool deshi::shouldClose(){
	glfwPollEvents(); //this maybe should be elsewhere, but i dont want to move glfw includes to deshi.h 
	return glfwWindowShouldClose(deshi_window.window) || deshi_window.closeWindow;
}