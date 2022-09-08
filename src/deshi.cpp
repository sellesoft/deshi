/* deshi

syntax:
[priority (!), difficulty (*), date made, tags (optional)] (TODO)

priority is relative to other todos, if its really not important you can use 0 instead
difficulty can be seen as 
*    : easy
**   : medium
***  : hard
***+ : even harder/tedious
its really meant to be taken as an abstract idea of how hard a task will be at first glance
extra information about a TODO can be placed underneath the todo, tabbed

Common Tags: Feature, Tweak, Bug, System, PWide


`Command`
---------
[!  ,*  , 21/04/12,Feature] command to print all avaliable keys for binding
[!  ,*  , 21/04/12,Feature] command to print all keybinds, with (maybe) an option for printing only contextual keybinds
[0  ,*  , 21/06/09,Feature] add device_info command (graphics card, sound device, monitor res, etc)
[!! ,*  , 21/08/07,Feature] implement command chaining (separated by ';')
[0  ,*  , 22/04/18,Feature] support running commands with nested aliases
[!  ,** , 22/05/06,Feature] add 'exec' command which can run commands from a file

`Console`
---------
[!! ,** ,21/04/12,Feature] add auto complete for commands and arguments
[!! ,*  ,21/04/15,Feature] implement filtering console buffer by tags
[0  ,** ,21/04/15,Feature] popout console state (require extra window creation)
[!! ,*  ,21/06/16,Feature] tabbing so we can sort different kinds of info into each tab like Errors and Warnings
[!! ,** ,21/12/23,Bug]     if the console fills up too much, it crashes
    you can test by setting MEMORY_DO_HEAP_PRINTS to true in core/memory.cpp
[0  ,*  ,21/12/27,Feature] showing a command's help if tab is pressed when the command is already typed (or do python style ? thing)
[!! ,*  ,22/01/13,Feature] config variable modification
[0  ,*  ,22/01/13,Feature] simple terminal emulation (folder nav and filesystem)
[!! ,*  ,22/04/18,Tweak]   draw \t correctly
[!!!,** ,22/05/02,Feature] clipping so we only consider chunks that would be on screen
[!! ,** ,22/05/02,Bug]     fix scrolling to bottom on new log

`File`
------
[0  ,** ,21/09/16,Feature]  data folder specified on launch
[!!!,***,21/10/20,System]   linux/mac file IO
[!  ,*  ,21/12/28,Feature]  add file locking and determination
[!  ,*  ,21/12/28,Feature]  add hard/symbolic link creation/deletion
[!  ,*  ,21/12/28,Feature]  add file hard/symbolic link determination
[!  ,*  ,21/12/28,Feature]  add drive statistics
[0  ,*  ,22/04/28,Optimize] maybe wrap error checking in #if debug clauses?
[!! ,** ,22/06/05,Bug]      config may keep a file locked even after loading it

`Fun`
-----
[0  ,***,21/04/12,System] look into implementing Lua (or finish amu and make it an embeddable language!)
[0  ,***,21/06/09,System] write a preprocessing/postprocessing compiler that makes serialization easier

`Input`
-------
[!  ,*  ,21/10/28,Feature] add MouseInsideWindow() func to input or window
[!  ,** ,22/05/06,Feature] add support for Windows IME input
[!! ,** ,22/05/06,Bug]     enabling the Windows IME freezes the program

`Logger`
--------
[!! ,*  ,22/01/06,Tweak] optimize by using a single buffer or temp allocation instead of strings in logger_comma_log
[!  ,** ,22/08/26,Tweak] strip the log file of color escape sequences when the program terminates

`Math`
------
[!!!,** ,21/03/13,Feature] add quaternions and converions between them and other linear algebra primitives
[!! ,*  ,21/10/20,Tweak]   move geometry funcs out of math.h

`Memory`
--------
[!!!,***,21/12/22,Feature] consider multiple thread contexts
[!  ,***,21/12/22,Feature] add fast generic bins
    ref: https://github.com/lattera/glibc/blob/895ef79e04a953cac1493863bcae29ad85657ee1/malloc/malloc.c#L1555
[!! ,** ,22/01/16,Bug]     memory system sometimes fails to alloc memory from OS (might only be during debugging)
[!! ,** ,22/02/06,Tweak]   add a way to disable Memory module and set deshi_allocator to libc
[!  ,*  ,22/04/26,Feature] create an interface for creating/using Heap

`Render`
--------
[!! ,***,21/03/22,Feature] multi-threaded command buffers, shader loading, image loading
[!!!,*  ,21/04/04,Feature] add instancing
[!!!,** ,21/04/06,Tweak]   fix texture transparency
[!  ,** ,21/04/30,Feature] upload extra mesh info to an SSBO
[!!!,***,21/05/13,Feature] look into getting info from shaders, or setting up compute shaders
    ref: https://github.com/SaschaWillems/Vulkan/blob/master/examples/computeparticles/computeparticles.cpp
    the primary reason being that we need to optimize outlining objects, which will
    involve clipping triangles and stuff
[!! ,** ,21/06/16,Feature] SSBOs in shaders so we can pass variable length arrays to it
[!! ,*  ,21/06/17,Feature] add standard render/video settings
[!!!,** ,21/07/06,Feature] add omnidirectional shadow mapping
[!!!,*  ,21/07/06,Feature] add not-on-screen object culling thru mesh AABBs (if they dont cast shadow)
[!!!,** ,21/07/06,Feature] add front-to-back sorting for perf gain and transparency?
[!! ,***,21/07/06,System]  setup more generalized material/pipeline creation
    specialization constants
    uber shaders
    runtime pipeline creation/specialization
[!!!,** ,21/07/09,System]  rework lights
[!!!,***,21/07/10,Bug]     fix directional shadow mapping's (projection?) errors
[!  ,** ,21/08/07,Feature] add texture/material recreation without restart
[!! ,*  ,21/08/07,Feature] vulkan auto-cleanup so that it frees the unused parts of memory every now and then (chunking, look into vma lib)
[!  ,*  ,21/08/07,Tweak]   revert phong shader so it is like it used to be, but keeps the shadows
[!! ,** ,21/09/26,Tweak]   give text its own stuff in renderer so it can have different settings from other UI (filtering,antialiasing,etc)
[0  ,*  ,21/11/26,Tweak]   rework the lines drawing algorithm and move it to a more appropriate spot like UI or suugu
[!  ,*  ,21/12/18,Tweak]   replace allocator with temp_allocator in relevant places
[!  ,*  ,22/01/04,Tweak]   figure out how to use custom allocators with opengl3 and get opengl3 to use deshi memory
[!!!,** ,22/02/26,Feature] add texture/material/mesh unloading
[!! ,*  ,22/02/26,Feature] OpenGL debug groups
[!! ,** ,22/02/26,Feature] OpenGL pipelines/materials
[!!!,** ,22/02/26,Feature] OpenGL shadows (and test/fix Vulkan shadows)
[!! ,***,22/02/26,Feature] OpenGL render/video settings
[!! ,** ,22/06/04,Tweak]   shrink Vertex2 footprint by using s16 for pos and f16 for uv
[!!!,***,22/08/12,Feature] add support for render taking in external vertex/index buffer to draw from

`Sound`
-------
[!!!,***,21/04/05,System] remake the sound system

`Storage`
---------
[!!!,*  ,21/07/10,Bug]     the program crashes if default asset files are not present
    maybe store the text in the actual source and create the file from the code (null128.png, gohufont-11.bdf)
[!! ,** ,21/08/07,Tweak]   speedup OBJ parsing and face generation
[!!!,*  ,21/08/22,Tweak]   store null128.png and null shader in code
[!! ,*  ,21/08/22,Tweak]   add versioning to Mesh since its saved in a binary format
[!  ,*  ,21/10/20,Tweak]   merge mesh faces with <10 degree normal difference (for physics)
[!  ,** ,21/10/20,Tweak]   add edges and hulls to meshes, remove unused vars
[!! ,** ,21/10/20,Feature] add OBJ MTL parsing
[!!!,** ,21/12/31,Feature] data streaming to prevent loading freeze
[!!!,*  ,22/01/12,Feature] make an interface for updating textures that have already been created
[!  ,*  ,22/09/04,Tweak]   rename to Assets

`Time`
------

`UI`
----
[!!!,** ,22/08/09,System]  remove the old ui system 
[!!!,***,22/08/09,PWide]   replace usage of the old ui with new ui or just disable it so it doesnt error
[!!!,*  ,22/08/09,Tweak]   reimplement slider and checkbox
[!!!,** ,22/08/09,Feature] add tabs widget
[!! ,** ,22/08/09,Feature] add tables widget
[!  ,*  ,22/08/09,Feature] add combo widget
[!  ,** ,22/08/09,Feature] add tree widget
[!  ,*  ,22/08/09,Feature] add spinner widget
[0  ,*  ,22/08/09,Feature] add radio widget
[0  ,***,22/08/09,Feature] add color picker widget
[!! ,*  ,22/08/09,Tweak]   remove widget stuff from ui.h (ui2.h as of right now) and put it in its own file 
[!! ,*  ,22/08/12,Tweak]   either finish the hot loading setup for ui or remove it
[!!!,** ,22/08/13,Tweak]   ui's memory needs trimmed a LOT. to display little text on screen it takes over 500 bytes due to it being represented by uiItem who uses uiStyle
[!!!,** ,22/09/04,Bug]     there seems to be a bug with drawcmd removal when reallocating text drawinfo. 
    it triggers the assert that checks that the drawcmd being removed does not have the same offset as one that is already removed
    this check may just be invalid. this happens when clicking on text sometimes.


`Window`
--------
[!!!,***,21/08/07,Feature] add the ability to limit framerate
[!!!,*  ,21/09/01,Feature] make the transparent framebuffer a start switch since it hurts frames (it must be set at window creation time)

`Ungrouped`
-----------
[!! ,** ,21/07/19,Feature] centralize the settings files (combine all deshi.cfg and all game.cfg, make them hot-loadable)
[!!!,***,21/12/31,System]  remove GLFW and add linux/mac platform specifics
[!  ,*  ,22/02/01,Tweak]   remove commit/decommit from Allocator

*/

#define __DESHI__ // for various things to detect if deshi is active (eg. utils stuff that can make use of temp alloc)
#define UNICODE
#define _UNICODE

#include "kigu/common.h"
#include "core/memory.h" //NOTE(delle) this is included above everything so things can reference deshi_allocator

//// deshi stages ////
typedef Flags DeshiStage;
enum
{
    DS_NONE     = 0,
    DS_MEMORY   = 1 << 0,
    DS_PLATFORM = 1 << 1,
    DS_LOGGER   = 1 << 2,
    DS_NETWORK  = 1 << 3,
    DS_THREAD   = 1 << 4,
    DS_WINDOW   = 1 << 5,
    DS_RENDER   = 1 << 6,
    DS_IMGUI    = 1 << 7,
    DS_STORAGE  = 1 << 8,
    DS_UI       = 1 << 9,
    DS_UI2      = 1 << 10,
    DS_CONSOLE  = 1 << 11,
    DS_CMD      = 1 << 12,
};
local DeshiStage deshiStage = DS_NONE;

#define DeshiStageInitStart(stage, dependencies, ...)      \
  Assert((deshiStage & (dependencies)) == (dependencies)); \
  deshiStage |= stage;                                     \
  Stopwatch stopwatch##stage = start_stopwatch()

#define DeshiStageInitEnd(stage) \
  LogS("deshi", "Finished " #stage " module initialization in ", peek_stopwatch(stopwatch##stage), "ms")

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
#include "kigu/string_utils.h"
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
#endif // DESHI_DISABLE_CONSOLE
#ifndef DESHI_DISABLE_IMGUI
#  define IMGUI_DEFINE_MATH_OPERATORS
#  include "core/imgui.h"
#endif // DESHI_DISABLE_IMGUI
#include "core/input.h"
#include "core/file.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/networking.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/storage.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/ui2.h"
#include "core/window.h"

//// platform ////
#if DESHI_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <windowsx.h>
#  include <WinSock2.h>
#  include <ws2tcpip.h>
#  include <tlhelp32.h>
#  include <shellapi.h>
#  include <timeapi.h>
#  include "core/platforms/win32_deshi.cpp"
#elif DESHI_LINUX // DESHI_WINDOWS
#  include <GLFW/glfw3.h>
#  include "core/platforms/linux_deshi.cpp"
#elif DESHI_MAC // DESHI_LINUX
#  include <GLFW/glfw3.h>
#  include "core/platforms/osx_deshi.cpp"
#else // DESHI_MAC
#  error "unknown platform"
#endif // DESHI_WINDOWS

//// external for core ////
#define STBDS_REALLOC(ctx,ptr,newsz) memory_realloc(ptr,newsz)
#define STBDS_FREE(ctx,ptr) memory_zfree(ptr)
#define STB_DS_IMPLEMENTATION
#include <stb/stb_ds.h>
#define STBI_MALLOC(sz) memory_alloc(sz)
#define STBI_REALLOC(ptr, newsz) memory_realloc(ptr, newsz)
#define STBI_FREE(ptr) memory_zfree(ptr)
#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>
#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>
#define STBTT_malloc(sz,ctx) memory_alloc(sz)
#define STBTT_free(ptr,ctx) memory_zfree(ptr)
#define STBTT_assert(x) Assert(x)
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#ifndef DESHI_DISABLE_IMGUI
#  define IMGUI_USE_STB_SPRINTF
#  include <imgui/imgui.cpp>
#  include <imgui/imgui_demo.cpp>
#  include <imgui/imgui_draw.cpp>
#  include <imgui/imgui_tables.cpp>
#  include <imgui/imgui_widgets.cpp>
#  if DESHI_WINDOWS
#    define VK_USE_PLATFORM_WIN32_KHR
#    include <imgui/imgui_impl_win32.cpp>
#  else
#    include <imgui/imgui_impl_glfw.cpp>
#  endif
#endif

//// renderer cpp (and libs) ////
#if DESHI_VULKAN
#  include <vulkan/vulkan.h>
#  include <shaderc/shaderc.h>
#  include <imgui/imgui_impl_vulkan.cpp>
#  include "core/renderers/vulkan.cpp"
#elif DESHI_OPENGL
#  define GLAD_WGL_IMPLEMENTATION
#  include <glad/wgl.h>
#  define GLAD_GL_IMPLEMENTATION
#  include <glad/gl.h>
#  define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#  include <imgui/imgui_impl_opengl3.cpp>
#  include "core/renderers/opengl.cpp"
#elif DESHI_DIRECTX12
#  include "d3dx12/d3dx12.h"
#  include <d3d12.h>
#  include <wrl/client.h> //ComPtr
#  include <dxgi1_6.h>
//#  include <d3dcompiler.h> this is for compiling HLSL shaders at runtime, which ideally we wont do, but ill keep it just incase
// if we do, dont forget to link against d3dcompiler.lib and copy D3dcompiler_47.dll to the same file as our exe
#  include <DirectXMath.h>
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
#include "core/ui2.cpp"
#include "core/commands.cpp"

local Time deshi_time;
Time *g_time = &deshi_time;
local Input deshi_input;
Input *g_input = &deshi_input;
local Storage deshi_storage;
Storage *g_storage = &deshi_storage;
local ThreadManager deshi_thread_manager;
ThreadManager *g_tmanager = &deshi_thread_manager;
local uiContext deshi_ui{};
uiContext *g_ui = &deshi_ui;
local uiStyle deshi_ui_initial_style{};
uiStyle *ui_initial_style = &deshi_ui_initial_style;
Window *g_window = 0;
MemoryContext *g_memory = 0;