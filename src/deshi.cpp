/* deshi

`TODO`
Format:
[MM/DD/YY,DIFFICULTY,Tags...] description
    continued description
Assumed Difficulties: EASY, MEDI, HARD
Common Tags: Feature, Tweak, Bug, System

`Command`
---------
[04/12/21,EASY,Feature] command to print all avaliable keys for binding
[04/12/21,EASY,Feature] command to print all keybinds, with (maybe) an option for printing only contextual keybinds
[06/09/21,EASY,Feature] add device_info command (graphics card, sound device, monitor res, etc)
[08/07/21,EASY,Feature] implement command chaining (separated by ';')
[04/18/22,EASY,Feature] support running commands with nested aliases
[05/06/22,MEDI,Feature] add 'exec' command which can run commands from a file

`Console`
---------
[04/12/21,MEDI,Feature] add auto complete for commands and arguments
[04/15/21,EASY,Feature] implement filtering console buffer by tags
[04/15/21,MEDI,Feature] popout console state (require extra window creation)
[06/16/21,EASY,Feature] tabbing so we can sort different kinds of info into each tab like Errors and Warnings
[12/23/21,MEDI,Bug]     if the console fills up too much, it crashes
    you can test by setting MEMORY_DO_HEAP_PRINTS to true in core/memory.cpp
[12/27/21,EASY,Feature] showing a command's help if tab is pressed when the command is already typed (or do python style ? thing)
[01/13/22,EASY,Feature] config variable modification
[01/13/22,EASY,Feature] simple terminal emulation (folder nav and filesystem)
[04/18/22,EASY,Tweak]   draw \t correctly
[05/02/22,MEDI,Feature] clipping so we only consider chunks that would be on screen
[05/02/22,MEDI,Bug]     fix scrolling to bottom on new log

`File`
------
[09/16/21,MEDI,Feature]  data folder specified on launch
[10/20/21,HARD,System]   linux/mac file IO
[12/28/21,EASY,Feature]  add file locking and determination
[12/28/21,EASY,Feature]  add hard/symbolic link creation/deletion
[12/28/21,EASY,Feature]  add file hard/symbolic link determination
[12/28/21,EASY,Feature]  add drive statistics
[04/28/22,EASY,Optimize] maybe wrap error checking in #if debug clauses?

`Fun`
-----
[04/12/21,HARD,System]  look into implementing Lua (or finish su and make it an embeddable language!)
[06/09/21,HARD,System]  write a preprocessing/postprocessing compiler that makes serialization easier
[07/26/21,HARD,Feature] hotloadable UI

`Input`
-------
[10/28/21,EASY,Feature] add MouseInsideWindow() func to input or window
[05/06/22,MEDI,Feature] add support for Windows IME input
[05/06/22,MEDI,Bug]     enabling the Windows IME freezes the program

`Logger`
--------
[01/06/22,EASY,Tweak] optimize by using a single buffer or temp allocation instead of strings in logger_comma_log

`Math`
------
[03/13/21,MEDI,Feature] add quaternions and converions between them and other linear algebra primitives
[05/03/21,MEDI,Feature] add functions and members similar to what glsl/glm has where you can do stuff like
    v.xy, v.yz, as well as operators for these things if possible. Prefer them to be member variables and not functions :)
    you probably just need to add a vec2/3 for each permutation of each vector (!ref: glm/detail/_swizzle.hpp)
[10/20/21,EASY,Tweak]   move geometry funcs out of math.h 

`Memory`
--------
[12/22/21,HARD,Feature] consider multiple thread contexts
[12/22/21,HARD,Feature] add fast generic bins
    ref: https://github.com/lattera/glibc/blob/895ef79e04a953cac1493863bcae29ad85657ee1/malloc/malloc.c#L1555
[01/16/22,MEDI,Bug]     memory system sometimes fails to alloc memory from OS (might only be during debugging)
[02/06/22,MEDI,Tweak]   add a way to disable Memory module and set deshi_allocator to libc
[04/26/22,EASY,Feature] create an interface for creating/using Heap

`Render`
--------
[03/22/21,HARD,Feature] multi-threaded command buffers, shader loading, image loading
[04/04/21,EASY,Feature] add instancing
[04/06/21,MEDI,Tweak]   fix texture transparency
[04/30/21,MEDI,Feature] upload extra mesh info to an SSBO
[05/13/21,HARD,Feature] look into getting info from shaders, or setting up compute shaders
    ref: https://github.com/SaschaWillems/Vulkan/blob/master/examples/computeparticles/computeparticles.cpp
    the primary reason being that we need to optimize outlining objects, which will
    involve clipping triangles and stuff
[06/16/21,MEDI,Feature] SSBOs in shaders so we can pass variable length arrays to it
[06/17/21,EASY,Feature] add standard render/video settings
[07/06/21,MEDI,Feature] add omnidirectional shadow mapping
[07/06/21,EASY,Feature] add not-on-screen object culling thru mesh AABBs (if they dont cast shadow)
[07/06/21,MEDI,Feature] add front-to-back sorting for perf gain and transparency?
[07/06/21,HARD,System]  setup more generalized material/pipeline creation
    specialization constants
    uber shaders
    runtime pipeline creation/specialization
[07/09/21,MEDI,System]  rework lights
[07/10/21,HARD,Bug]     fix directional shadow mapping's (projection?) errors
[08/07/21,MEDI,Feature] add texture/material recreation without restart
[08/07/21,EASY,Feature] vulkan auto-cleanup so that it frees the unused parts of memory every now and then (chunking, look into vma lib)
[08/07/21,EASY,Tweak]   revert phong shader so it is like it used to be, but keeps the shadows
[09/26/21,MEDI,Tweak]   give text its own stuff in renderer so it can have different settings from other UI (filtering,antialiasing,etc)
[09/26/21,EASY,Tweak]   find a nice way to not pass Font* to DrawText2D: maybe fixed fonts rather than array? maybe set active font?
[11/26/21,EASY,Tweak]   rework the lines drawing algorithm and move it to a more appropriate spot like UI or suugu
[12/18/21,EASY,Tweak]   replace allocator with temp_allocator in relevant places
[01/04/22,EASY,Tweak]   figure out how to use custom allocators with opengl3 and get opengl3 to use deshi memory
[02/26/22,MEDI,Feature] add texture/material/mesh unloading
[02/26/22,EASY,Feature] OpenGL debug groups
[02/26/22,MEDI,Feature] OpenGL pipelines/materials
[02/26/22,MEDI,Feature] OpenGL shadows (and test/fix Vulkan shadows)
[02/26/22,HARD,Feature] OpenGL render/video settings

`Sound`
-------
[04/05/21,HARD,System] remake the sound system

`Storage`
---------
[07/10/21,EASY,Bug]     the program crashes if default asset files are not present
    maybe store the text in the actual source and create the file from the code (null128.png, gohufont-11.bdf)
[08/07/21,MEDI,Tweak]   speedup OBJ parsing and face generation
[08/22/21,EASY,Tweak]   store null128.png and null shader in code
[08/22/21,EASY,Tweak]   add versioning to Mesh since its saved in a binary format
[10/20/21,EASY,Tweak]   separate physics mesh info from regular mesh info
[10/20/21,EASY,Tweak]   merge mesh faces with <10 degree normal difference (for physics)
[10/20/21,MEDI,Tweak]   add edges and hulls to meshes, remove unused vars
[10/20/21,MEDI,Feature] add OBJ MTL parsing
[12/31/21,MEDI,Feature] data streaming to prevent loading freeze
[01/12/22,EASY,Feature] make an interface for updating textures that have already been created
[02/26/22,MEDI,Feature] replace the arrays with arenas and remove item indexing

`Time`
------
[12/31/21,EASY,Tweak] remove/abstract the manual DeshTime->frameTime timer handling at the end of update loop

`UI`
----
[08/05/21,MEDI,Feature] add functionality for resetting certain maps like windows and inputtexts
    maybe even removing certain labels from them
[08/09/21,MEDI,Feature] vertical tabs
[08/15/21,MEDI,Feature] add some markup to text like underlining, bold, etc.
[01/09/22,EASY,Tweak]   specify Separator() parameters and add one for line height
[02/12/22,EASY,Tweak]   add PAGEUP and PAGEDOWN scrolling keybinds (CTRL for max scroll up/down)
[02/12/22,MEDI,Feature] window snapping (to borders, to other windows, window tabifying when dropped onto another window)
[02/12/22,MEDI,Feature] menus that open on hover (the direction they open is flag controlled, down vs right)(ImGui::BeginMenu)
[02/12/22,EASY,Tweak]   add button flag: return true on hover (also can have the function return what flags triggered the success)
[02/12/22,EASY,Feature] begintab()/button() for images
[02/12/22,EASY,Tweak]   window flags for no margin and no padding
[02/12/22,HARD,Feature] context menu
[02/12/22,Easy,Feature] text input mouse click to place cursor
[02/12/22,HARD,System]  text selection (and clipboard)
[02/20/22,EASY,Tweak,ProjectWide] move all of the structs and enums in ui.h to be under the UI namesapce and remove the UI prefix from them
    UI::Window would conflict with our current Window, so either rename Window to OSWindow or put it in some kind of namespace
[02/27/22,EASY,Feature] add displaying a window's flags to metrics

better flag descriptions on how they interact with other flags (no scroll vs no scroll bar, no focus vs focus on hover, what is NoMinimize?)
pushvar type mismatch: if ui funcs were macros, we could use compiler counters to compile-time check for push/pop mismatches, begin/end mismatches, and pushvar type mismatches
buttons dont work with UIWindowFlags_FitAllElements since they depend on eachother (buttons shrink to fit rather than get cut off)
popout windows pass thru inputs to their parent window
floating point accumulation in placement
row isnt a good name for a table behaviouring item
setting the size of a row
get max space for next item (after sameline)
i expected UI::RowSetupRelativeColumnWidths({1,1,1}) to be default behaviour
scrollbar isnt draggable if the window is moveable
rows arent nestable
tab bar buttons pass their input thru to the window (for dragging)

`Window`
--------
[08/07/21,HARD,Feature] add the ability to limit framerate
[09/01/21,EASY,Feature] make the transparent framebuffer a start switch since it hurts frames (it must be set at window creation time)

`Ungrouped`
-----------
[07/19/21,MEDI,Feature] centralize the settings files (combine all deshi.cfg and all game.cfg, make them hot-loadable)
[12/31/21,HARD,System]  remove GLFW and add linux/mac platform specifics
[02/01/22,EASY,Tweak]   remove commit/decommit from defines.h
[03/02/22,EASY,Tweak]   check that deshi::init, deshi::shouldCLose and deshi::cleanup are all still up to date as well as how deshi.h handles including things from core
[03/12/22,MEDI,Feature] add deshi::DisplaySettingsWindow() and deshi::DisplaySettings(), global settings for each of deshi's modules
    this could also be separated into individual functions for each module as well, so you could call deshi::DisplayRenderSettings()
*/


#define __DESHI__ //for various things to detect if deshi is active (eg. utils stuff that can make use of temp alloc)
#define UNICODE
#define _UNICODE

#include "kigu/common.h"
#include "core/memory.h" //NOTE(delle) this is included above everything so things can reference deshi_allocator


//// deshi stages ////
typedef Flags DeshiStage; enum{
	DS_NONE      = 0,
	DS_MEMORY    = 1 << 0,
	DS_PLATFORM  = 1 << 1,
	DS_LOGGER    = 1 << 2,
	DS_CONSOLE   = 1 << 3,
	DS_RENDER    = 1 << 6,
	DS_IMGUI     = 1 << 7,
	DS_STORAGE   = 1 << 8,
	DS_UI        = 1 << 9,
	DS_CMD       = 1 << 10,
};
local DeshiStage deshiStage = DS_NONE;

#define AssertDS(stages, ...) Assert((deshiStage & (stages)) == (stages))

#define DeshiStageInitStart(stage,dependencies,...) \
Assert((deshiStage & (dependencies)) == (dependencies)); \
deshiStage |= stage; \
Stopwatch stopwatch##stage = start_stopwatch()

#define DeshiStageInitEnd(stage) \
LogS("deshi","Finished " #stage " module initialization in ",peek_stopwatch(stopwatch##stage),"ms")

#define DeshiModuleLoaded(stages) ((deshiStage & (stages)) == (stages))


//// tracy ////
#ifdef TRACY_ENABLE
#  include "TracyClient.cpp"
#  include "Tracy.hpp"
#endif


//// kigu headers ////"
#include "kigu/profiling.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "kigu/carray.h"
#include "kigu/color.h"
#include "kigu/debug.h"
#include "kigu/hash.h"
#include "kigu/map.h"
#include "kigu/optional.h"
#include "kigu/pair.h"
#include "kigu/ring_array.h"
#include "kigu/unicode.h"
#include "math/math.h"


//// core headers ////
#define DESHI_IMPLEMENTATION
#include "deshi.h"
#include "core/camera.h"
#include "core/commands.h"
#include "core/config.h"
#ifndef DESHI_DISABLE_CONSOLE
#  include "core/console.h"
#endif //DESHI_DISABLE_CONSOLE
#include "core/font.h"
#ifndef DESHI_DISABLE_IMGUI
#  define IMGUI_DEFINE_MATH_OPERATORS
#  include "core/imgui.h"
#endif //DESHI_DISABLE_IMGUI
#include "core/input.h"
#include "core/file.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/model.h"
#include "core/platform.h"
#include "core/render.h"
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
#elif DESHI_LINUX //DESHI_WINDOWS
#  include "core/platforms/linux_deshi.cpp"
#elif DESHI_MAC   //DESHI_LINUX
#  include "core/platforms/osx_deshi.cpp"
#else             //DESHI_MAC
#  error "unknown platform"
#endif //DESHI_WINDOWS


//// external for core ////
#define STBI_MALLOC(sz) memory_alloc(sz)
#define STBI_REALLOC(p,newsz) memory_realloc(p,newsz)
#define STBI_FREE(p) memory_zfree(p)
#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#ifndef DESHI_DISABLE_IMGUI
//<stb/stb_truetype.h> included by imgui
//<stb/stb_sprintf.h> included by imgui
#  define IMGUI_USE_STB_SPRINTF
#  include <imgui/imgui.cpp>
#  include <imgui/imgui_demo.cpp>
#  include <imgui/imgui_draw.cpp>
#  include <imgui/imgui_tables.cpp>
#  include <imgui/imgui_widgets.cpp>
#else
#  define STB_TRUETYPE_IMPLEMENTATION
#  include <stb/stb_truetype.h>
#  define STB_SPRINTF_IMPLEMENTATION
#  include <stb/stb_sprintf.h>
#  define STB_RECT_PACK_IMPLEMENTATION
#  include <imgui/stb_rectpack.h>
#endif


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
#
#  include "core/renderers/vulkan.cpp"
#elif DESHI_OPENGL
#  define GLAD_WGL_IMPLEMENTATION
#  include <glad/wgl.h>
#  define GLAD_GL_IMPLEMENTATION
#  include <glad/gl.h>
#  define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#  include <imgui/imgui_impl_opengl3.cpp>
#
#  include "core/renderers/opengl.cpp"
#elif DESHI_DIRECTX12
#  include "d3dx12/d3dx12.h"
#  include <d3d12.h>
#  include <wrl/client.h> //ComPtr
#  include <dxgi1_6.h>
//#  include <d3dcompiler.h> this is for compiling HLSL shaders at runtime, which ideally we wont do, but ill keep it just incase
// if we do, dont forget to link against d3dcompiler.lib and copy D3dcompiler_47.dll to the same file as our exe
#  include <DirectXMath.h>
#
#  include "core/renderers/directx.cpp"
#else
#  error "no renderer selected"
#endif


//// core cpp ////
#include "core/memory.cpp"
#include "core/logger.cpp"
#include "core/console.cpp"
#include "core/storage.cpp"
#include "core/ui.cpp"
#include "core/commands.cpp"


local Time          deshi_time;           Time*          g_time     = &deshi_time;
local Input         deshi_input;          Input*         g_input    = &deshi_input;
local Storage_      deshi_storage;        Storage_*      g_storage  = &deshi_storage;
local ThreadManager deshi_thread_manager; ThreadManager* g_tmanager = &deshi_thread_manager;
Window* g_window = 0;
