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
  UpperCamelMemberFuncs
  lower_underscore_unscoped_vars (.cpp only)
  DowhateverLocal_vars (usually lowerCamel or lower_underscore)
  g_global_vars (defined in deshi.cpp only, declared in specific header)

Command TODOs
-------------
implement command chaining
command to print all avaliable keys for binding
command to print all keybinds, with (maybe) an option for printing only contextual keybinds
make binds and aliases check if one already exists for a key or a command (if a key already exists, probably just overwrite it?)
add device_info command (graphics card, sound device, monitor res, etc)
change Run()'s input from string to cstring

Console TODOs
-------------
showing a command's help if tab is pressed when the command is already typed
popout console state (require extra window creation)
tabbing so we can sort different kinds of info into each tab like Errors and Warnings
add auto complete for commands and arguments
implement filtering console buffer by function and file name (add __FILENAME__ and __FUNCTION__ or whatever it is to the defines)
config variable modification
simple terminal emulation

Fun TODOs
---------
look into implementing Lua (or finish su and make it an embeddable language!)
write a preprocessing/postprocessing compiler that makes serialization easier
hotloadable UI

IO TODOs
--------
add file reading (simple and smart)
add file writing (simple and smart)
add file/dir creation
add file/dir renaming
add file locking and determination
add hard/symbolic link creation/deletion
add file hard/symbolic link determination
add drive statistics
safety checks for IO operations
add search filters to get_directory_files
data folder specified on launch
smart text file parser (handles new line checking and formatting)
 linux/mac IO

Input TODOs
-----------
add MouseInsideWindow() func to input or window

Logger TODOs
------------
add push/pop indentation level
make a local Assert macro that logs the message before stopping
make the most recent logging file be named log.txt, while the rest have a date
can probably optimize by using a single buffer instead of strings in Log() and LogA()
look into https://github.com/fmtlib/fmt for fast formatting

Math TODOs
----------
move geometry funcs out of math.h 
add quaternions and converions between them and other linear algebra primitives
add functions and members similar to what glsl/glm has where you can do stuff like
____v.xy, v.yz, as well as operators for these things if possible. Prefer them to be member variables and not functions :)
____you probably just need to add a vec2/3 for each permutation of each vector
____glm/detail/_swizzle.hpp

Memory TODOs
------------
add a way to disable this and set deshi_allocator to libc
maybe temp memory should not default to zero?
consider multiple thread contexts
add fast generic bins
____ref: https://github.com/lattera/glibc/blob/895ef79e04a953cac1493863bcae29ad85657ee1/malloc/malloc.c#L1555

Render TODOs
------------
figure out how to use custom allocators with opengl3 and get opengl3 to use deshi memory
remove usage of STL
replace allocator with temp_allocator in relevant places
rework the lines drawing algorithm and move it to a more appropriate spot like UI or suugu
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

Sound TODOs
-----------
remake the sound system

Storage TODOs
-------------
separate physics mesh info from regular mesh info
merge mesh faces with <10 degree normal difference (for physics)
add edges and hulls to meshes, remove unused vars
make an interface for updating textures that have already been created
add MTL parsing
store null128.png and null shader in code
add versioning to Mesh since its saved in a binary format
data streaming to prevent loading freeze
speedup OBJ parsing and face generation

Time TODOs
----------
remove/abstract the manual DeshTime->frameTime timer handling at the end of update loop
make a dynamic timers array in time.h for cleaner timer stuffs (push/peek/pop)
rename 'updateCount' to 'frame'

UI TODOs
--------
add functionality for resetting certain maps like windows and inputtexts
____maybe even removing certain labels from them
add some markup to text like underlining, bold, etc.
specify Separator() parameters and add one for line height
add PAGEUP and PAGEDOWN scrolling keybinds (CTRL for max scroll up/down)
add stuff like fps and window size to metrics instead of active windows
window snapping (to borders, to other windows, window tabifying when dropped onto another window)
menus that open on hover (the direction they open is flag controlled, down vs right)(ImGui::BeginMenu)
vertical tabs
add button flag: return true on hover
begintab()/button() for images
window flags for no margin and no padding
context menu
text input mouse click to place cursor
text selection (and clipboard)

Window TODOs
------------
make the transparent framebuffer a start switch since it hurts frames (it must be set at window creation time)
add the ability to limit framerate

Ungrouped TODOs
---------------
remove commit/decommit from defines.h
move config saving/loading to its own core file
remove GLFW and add platform layers
restyle map to match the rest of utils
centralize the settings files (combine all deshi.cfg and all game.cfg, make them hot-loadable)
convert std::string to our string throughout the project, primarily .str() methods so i can fully convert toStr to use our string
look into integrating TODOP with Discord

Bug Board       //NOTE mark these with a last-known active date (MM/DD/YY)
---------
(07/10/21) the program crashes if default asset files are not present
__________ maybe store the text in the actual source and create the file from the code, like keybinds.cfg
__________ alternatively, we can store those specific assets in the source control
(12/23/21) if the console fills up too much, it crashes
__________ you can test by setting MEMORY_DO_HEAP_PRINTS to true in core/memory.cpp
(01/10/22) color formatting does not work thru Log()
__________ see commands.cpp 'test' command
(01/16/22) memory system sometimes fails to alloc memory from OS (HACK fixed with looping until success)
*/



#include "kigu/common.h"
#include "core/memory.h" //NOTE this is included above everything so things can reference deshi_allocator

#ifdef TRACY_ENABLE
#include "TracyClient.cpp"
#include "Tracy.hpp"
#undef ERROR
#undef DELETE
#endif

#include "core/profiling.h"

//// kigu headers ////"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "kigu/carray.h"
#include "kigu/cstring.h"
#include "kigu/color.h"
#include "kigu/debug.h"
#include "kigu/hash.h"
#include "kigu/map.h"
#include "kigu/optional.h"
#include "kigu/pair.h"
#include "kigu/ring_array.h"
#include "kigu/string.h"
#include "kigu/string_utils.h"
#include "kigu/unicode.h"
#include "kigu/utils.h"
#include "math/math.h"

//// libcpp for core ////
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <vector>
#include <set>
#include <unordered_map>

enum DeshiStage{
	DS_NONE    = 0,
	DS_MEMORY  = 1 << 0,
	DS_LOGGER  = 1 << 1,
	DS_CONSOLE = 1 << 2,
	DS_TIME    = 1 << 3,
	DS_WINDOW  = 1 << 4,
	DS_RENDER  = 1 << 5,
	DS_IMGUI   = 1 << 6,
	DS_STORAGE = 1 << 7,
	DS_UI      = 1 << 8,
	DS_CMD     = 1 << 9,
};
local Flags deshiStage = DS_NONE;

#define AssertDS(stages, ...) Assert((deshiStage & (stages)) == (stages))
#define DeshiModuleLoaded(stages) ((deshiStage & (stages)) == (stages))

//// core headers ////
#include "deshi.h"
#include "core/assets.h"
#include "core/camera.h"
#include "core/commands.h"
#ifndef DESHI_DISABLE_CONSOLE
#  include "core/console.h"
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
#include "core/platform.h"
#include "core/renderer.h"
#include "core/storage.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"


//// platform ////
#if   DESHI_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <windowsx.h>
#  include "core/platforms/win32_deshi.cpp"
#  undef ERROR
#  undef DELETE
#elif DESHI_LINUX //DESHI_WINDOWS
#  include "core/platforms/linux_deshi.cpp"
#elif DESHI_MAC   //DESHI_LINUX
#  include "core/platforms/osx_deshi.cpp"
#else             //DESHI_MAC
#  error "unknown platform"
#endif //DESHI_WINDOWS

//// external for core ////
#define STB_IMAGE_IMPLEMENTATION
//#define STB_TRUETYPE_IMPLEMENTATION
#define STBI_MALLOC(sz) memory_alloc(sz)
#define STBI_REALLOC(p,newsz) memory_realloc(p,newsz)
#define STBI_FREE(p) memory_zfree(p)
#define STBI_FAILURE_USERMSG
#include <stb/stb_image.h>
//#include <stb/stb_truetype.h> //included by imgui
#include <imgui/imgui.cpp>
#include <imgui/imgui_demo.cpp>
#include <imgui/imgui_draw.cpp>
#include <imgui/imgui_tables.cpp>
#include <imgui/imgui_widgets.cpp>


//// platform ////
#if DESHI_WINDOWS
#  define VK_USE_PLATFORM_WIN32_KHR
#  include <imgui/imgui_impl_win32.cpp>
#else //DESHI_WINDOWS
#  include <GLFW/glfw3.h>
#  include <imgui/imgui_impl_glfw.cpp>
#endif

//// renderer cpp (and libs) ////
#if   DESHI_VULKAN
#  include <vulkan/vulkan.h>
#  include <shaderc/shaderc.h>
#  include <imgui/imgui_impl_vulkan.cpp>
#  include "core/renderers/vulkan.cpp"
#elif DESHI_OPENGL //DESHI_VULKAN
#define GLAD_WGL_IMPLEMENTATION
#include <glad/wgl.h>
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include <imgui/imgui_impl_opengl3.cpp>
#include "core/renderers/opengl.cpp"
#elif DESHI_DIRECTX12 //DESHI_OPENGL
#  include <GLFW/glfw3.h>
#  define GLFW_EXPOSE_NATIVE_WIN32
#  include <GLFW/glfw3native.h>
#  include "d3dx12/d3dx12.h"
#  include <d3d12.h>
#  include <wrl/client.h> //ComPtr
#  include <dxgi1_6.h>
//#  include <d3dcompiler.h> this is for compiling HLSL shaders at runtime, which ideally we wont do, but ill keep it just incase
// if we do, dont forget to link against d3dcompiler.lib and copy D3dcompiler_47.dll to the same file as our exe
#  include <DirectXMath.h>
#  include "core/renderers/directx.cpp"
#else  //DESHI_DIRECTX12
#  error "no renderer selected"
#endif

#undef DeleteFont




//// core cpp ////
#include "core/io.cpp"
#include "core/memory.cpp"
#include "core/logger.cpp"
#include "core/assets.cpp"
#include "core/console.cpp"
#include "core/storage.cpp"
#include "core/ui.cpp"
#include "core/commands.cpp"

local Time          deshi_time;           Time*          g_time     = &deshi_time;
local Window        deshi_window;         Window*        g_window   = &deshi_window;
local Input         deshi_input{};        Input*         g_input    = &deshi_input;
local Console       deshi_console;        Console*       g_console  = &deshi_console;
local Storage_      deshi_storage;        Storage_*      g_storage  = &deshi_storage;
local ThreadManager deshi_thread_manager; ThreadManager* g_tmanager = &deshi_thread_manager;

void deshi::init(u32 winWidth, u32 winHeight){
	TIMER_START(t_s);
	Assets::enforceDirectories();
	memory_init(Gigabytes(1), Gigabytes(1));
	Logger::Init(5);
#ifndef DESHI_DISABLE_CONSOLE //really ugly lookin huh
	deshi_console.Init();
#endif
	deshi_time.Init();
	deshi_window.Init("deshi", winWidth, winHeight);
	Render::Init();
	Storage::Init();
#ifndef DESHI_DISABLE_IMGUI
	DeshiImGui::Init();
#endif
	UI::Init();
	Cmd::Init();
	DeshWindow->ShowWindow();
	Render::UseDefaultViewProjMatrix();
	LogS("deshi","Finished deshi initialization in ",TIMER_END(t_s),"ms");
}

void deshi::cleanup(){
	if(DeshiModuleLoaded(DS_IMGUI))
		DeshiImGui::Cleanup();
	Render::Cleanup();
	deshi_window.Cleanup();
	Logger::Cleanup();
	memory_cleanup();
}

b32 deshi::shouldClose(){
	//glfwPollEvents(); //this maybe should be elsewhere, but i dont want to move glfw includes to deshi.h 
	return deshi_window.closeWindow;
}