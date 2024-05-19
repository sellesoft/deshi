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
[!!!,*  ,23/11/08,Feature] ctrl+f search

`File`
------
[0  ,** ,21/09/16,Feature]  data folder specified on launch (in launch args)
[!  ,*  ,21/12/28,Feature]  add file locking and determination
[!  ,*  ,21/12/28,Feature]  add hard/symbolic link creation/deletion
[!  ,*  ,21/12/28,Feature]  add file hard/symbolic link determination
[!  ,*  ,21/12/28,Feature]  add drive statistics
[0  ,*  ,22/04/28,Optimize] maybe wrap error checking in #if debug clauses?
[!! ,** ,22/06/05,Bug]      config may keep a file locked even after loading it
[0  ,***,23/06/17,System]   mac file IO

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
[!!!,** ,21/12/22,Feature] add threading locks to arenas, generic alloc, and temp alloc
[!  ,***,21/12/22,Feature] add fast generic bins
    ref: https://github.com/lattera/glibc/blob/895ef79e04a953cac1493863bcae29ad85657ee1/malloc/malloc.c#L1555
[!! ,** ,22/01/16,Bug]     memory system sometimes fails to alloc memory from OS (might only be during debugging)
[!! ,** ,22/02/06,Tweak]   add a way to disable Memory module and set deshi_allocator to libc
[!!!,*  ,22/09/08,Feature] add memory_init args to deshi.cfg
[!!!,*  ,22/09/17,Tweak]   fix and test the memory_heap interface, memory_heap_add() isnt even valid
[!!!,*  ,22/09/17,Docs]    write descriptions for generic allocation, temp allocation, memory chunk, memory heap, and memory arena
[!  ,** ,22/09/17,Tweak]   use memory_heap interface internally for the arena and generic heaps
[!!!,*  ,23/06/17,Tweak]   memory's debug stuff doesn't work before we initialize logger, so we cannot properly debug issues that occur before then. 
                           need to change its logging to either use printf, or ideally choose which to use based on if Logger is initialzed
[!! ,***,23/06/17,Bug]     memory_pool seems to be broken. memory_pool_init was pointing to its header incorrectly and would overwrite previous data
                           so I fixed this, but then later usage of memory_pool_push caused something to break in the generic heap, so i am going to leave
                           it for later as I don't feel like diving into fixing it.
[!!!,*  ,22/09/08,Feature] add memory poisoning

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
[!! ,*  ,22/12/03,Feature] add VRAM tracking when creating and deleting device memory
[!! ,***,22/12/03,Feature] memory suballocation (like memory does) since device memory allocation can be quite slow
[!  ,*  ,24/03/23,Tweak]   validate that shaders contain the name_in_shader string specified on descriptors

`Sound`
-------
[!!!,***,21/04/05,System] remake the sound system

`Assets`
---------
[!!!,*  ,21/07/10,Bug]     the program crashes if default asset files are not present
    maybe store the text in the actual source and create the file from the code (null128.png, gohufont-11.bdf)
[!! ,** ,21/08/07,Tweak]   speedup OBJ parsing and face generation
[!!!,*  ,21/08/22,Tweak]   store null128.png and null shader in code
[!! ,*  ,21/08/22,Tweak]   add versioning to Mesh since its saved in a binary format
[!  ,*  ,21/10/20,Tweak]   merge mesh faces with <10 degree normal difference (for physics)
[!  ,** ,21/10/20,Tweak]   add edges and hulls to meshes, remove unused vars
[!! ,** ,21/10/20,Feature] add OBJ MTL parsing
[!!!,** ,21/12/31,Feature] data streaming (load in parts)
[!!!,*  ,22/01/12,Feature] make an interface for updating textures that have already been created
[!!!,** ,22/09/08,Feature] use worker threads to load in the background

`Time`
------
[!!!,***,21/08/07,Feature] add the ability to limit framerate (delta time)

`Threading`
-----------
[!  ,** ,22/09/08,Feature] add an async(tag,data_ptr) macro that works like defer expecting an {} after it to package into a wrapper func to send to the job worker

`UI`
----
[!!!,** ,22/08/09,Feature] add tabs widget
[!! ,** ,22/08/09,Feature] add tables widget
[!  ,*  ,22/08/09,Feature] add combo widget (requires z-layering)
[!  ,** ,22/08/09,Feature] add tree widget
[!  ,*  ,22/08/09,Feature] add spinner widget
[0  ,*  ,22/08/09,Feature] add radio widget
[0  ,***,22/08/09,Feature] add color picker widget
[!!!,** ,22/08/13,Tweak]   ui's memory needs trimmed a LOT. to display little text on screen it takes over 500 bytes due to it being represented by uiItem who uses uiStyle
[!!!,** ,22/09/04,Bug]     there seems to be a bug with drawcmd removal when reallocating text drawinfo. 
    it triggers the assert that checks that the drawcmd being removed does not have the same offset as one that is already removed
    this check may just be invalid. this happens when clicking on text sometimes.
[!!!,***,22/12/11,Feature] add z-layering (siblings could maybe be sorted so that higher z-level is last)
                           NOTE(sushi) this can be implemented locally by just creating your own 
                                       uiItems representing layers in the order you want them, and then appending 
                                       to those layers when you want to add to them.
                                       this requires manually pushing items though
[!!!,** ,22/12/11,Feature] add side-specific border styling (left, right, top, bottom)
[!! ,** ,22/12/11,Feature] add the ability to add child items after ending an item (dynamically added items)
[!! ,** ,22/12/11,Feature] add texture support (non-widget like text might be?)
[!! ,** ,22/12/11,Feature] add resizer widget
[!!!,*  ,22/12/11,Feature] add separator widget (with resizeability)
[!  ,*  ,22/12/11,Feature] add window decorator widget (title, maximize/minimize button?, expand/collapse button?, close button)
[!  ,*  ,22/12/11,Feature] add menu bar and menu widgets (requires z-layering)
[!! ,*  ,22/12/11,Feature] add more input text widget flavours (input_number, input_vec2, input_vec3, input_vec4, input_color)
[!  ,*  ,22/12/11,Feature] add button widget (can be done already but add for ease of use, text and image flavours)

`Window`
--------
[!!!,*  ,21/09/01,Feature] make the transparent framebuffer a start switch since it hurts frames (it must be set at window creation time)

`Ungrouped`
-----------
[!! ,** ,21/07/19,Feature] centralize the settings files (combine all deshi.cfg and all game.cfg, make them hot-loadable)
[!  ,*  ,23/01/15,Feature] add regression testing for examples (and a github precommit to run it)

*/

#define __DESHI__ // for various things to detect if deshi is active (eg. utils stuff that can make use of temp alloc)
#define UNICODE
#define _UNICODE

#ifdef BUILD_SLOW
#define DEBUG(code) do { code } while(0);
#else
#define DEBUG(code)
#endif

#include <ctime>
#include "kigu/common.h"
#include "core/memory.h" //NOTE(delle) this is included above everything so things can reference deshi_allocator

//// deshi stages ////
typedef Flags DeshiStage;
enum{
    DS_NONE     = 0,
    DS_MEMORY   = 1 << 0,
    DS_PLATFORM = 1 << 1,
    DS_LOGGER   = 1 << 2,
    DS_NETWORK  = 1 << 3,
    DS_THREAD   = 1 << 4,
    DS_WINDOW   = 1 << 5,
    DS_RENDER   = 1 << 6,
    DS_ASSETS   = 1 << 7,
    DS_UI       = 1 << 8,
    DS_CONSOLE  = 1 << 9,
    DS_CMD      = 1 << 10,
    DS_GRAPHICS = 1 << 11,
};
local DeshiStage deshiStage = DS_NONE;

#define DeshiStageInitStart(stage, dependencies, ...) \
  Assert(((deshiStage & (dependencies)) == (dependencies)) && (deshiStage & (stage)) != (stage)); \
  Stopwatch stopwatch##stage = start_stopwatch()

#define DESHI_LOG_MODULE_INIT 1
#if DESHI_LOG_MODULE_INIT
#  define DeshiStageInitEnd(stage) deshiStage |= stage; LogS("deshi", "Finished " #stage " module initialization in ", peek_stopwatch(stopwatch##stage), "ms")
#else
#  define DeshiStageInitEnd(stage) deshiStage |= stage;
#endif

#define DeshiModuleLoaded(stages) ((deshiStage & (stages)) == (stages))


//// tracy ////
#ifdef TRACY_ENABLE
#undef global
#undef defer
#  include "TracyClient.cpp"
#  include "tracy/Tracy.hpp"
#define global static
#define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif


//// kigu headers ////"
#include "kigu/profiling.h"
//#define KIGU_ARRAY_SHORTHANDS
#include "kigu/array.h"
#include "kigu/arrayT.h"
#include "kigu/array_utils.h"
#include "kigu/color.h"
#include "kigu/hash.h"
#include "kigu/map.h"
#include "kigu/optional.h"
#include "kigu/pair.h"
#include "kigu/ring_array.h"
#include "kigu/string_utils.h"
#include "kigu/unicode.h"
#include "math/math.h"


//// baked data ////
#include "core/baked/shaders.h"
#include "core/baked/textures.h"


//// external ////
// TODO(sushi) remove stb ds once we have our own C map and such 
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


//// core implementations ////
#define DESHI_IMPLEMENTATION
#include "core/assets.h"
#include "core/commands.h"
#ifndef DESHI_DISABLE_CONSOLE
#  include "core/console.h"
#endif // DESHI_DISABLE_CONSOLE
#include "core/input.h"
#include "core/logger.h"
#include "core/file.h"
#include "core/config.h"
#include "core/memory.h"
#include "core/networking.h"
#include "core/platform.h"
#include "core/graphics.h"
#include "core/render.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/ui_widgets.h"
#include "core/ui_graphing.h"
#include "core/window.h"


//// core cpp ////
#include "core/memory.cpp"
#include "core/console.cpp"
#include "core/assets.cpp"
#include "core/render.cpp"
#include "core/ui.cpp"
#include "core/graphics.cpp"
#include "core/commands.cpp" //NOTE(delle) this should be the last include so it can reference .cpp vars


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
#elif DESHI_LINUX //#if DESHI_WINDOWS
#  include "unistd.h" // misc stuff, apparently. used for file stuff
#  include "sys/mman.h" // for mapping virtual memory with mmap
#  include "sys/stat.h"
#  include "sys/types.h"
#  include "wctype.h" // unicode string manip functions that we should probably replace
#  include "fcntl.h" // creat
#  include "dirent.h"
#  include "dlfcn.h"
#  include "pthread.h"
#  include "semaphore.h"
#  define Window X11Window
#  define Font X11Font
#  define Time X11Time
#  define KeyCode X11KeyCode
#  include "X11/Xlib.h" // TODO(sushi) Wayland implementation, maybe
#  include "X11/Xatom.h"
#  include "X11/Xresource.h"
#  include "X11/cursorfont.h"
#  include "X11/Xcursor/Xcursor.h"
#  include "X11/Xutil.h"
#  include "X11/Xos.h"
#  include "X11/extensions/Xrandr.h"
#  undef Window
#  undef Font
#  undef Time
#  undef KeyCode
#  undef None 
#  include "core/platforms/linux_deshi.cpp" 
#elif DESHI_MAC //#elif DESHI_LINUX //#if DESHI_WINDOWS
#  include <GLFW/glfw3.h>
#  include "core/platforms/osx_deshi.cpp"
#else //#elif DESHI_MAC //#elif DESHI_LINUX //#if DESHI_WINDOWS
#  error "unknown platform"
#endif //#else //#elif DESHI_MAC //#elif DESHI_LINUX //#if DESHI_WINDOWS


//// renderer cpp (and libs) ////
#if DESHI_VULKAN
#  if DESHI_WINDOWS
#    define VK_USE_PLATFORM_WIN32_KHR
#    include <vulkan/vulkan.h>
#    include <shaderc/shaderc.h>
#  elif DESHI_LINUX //#if DESHI_WINDOWS
#    define VK_USE_PLATFORM_XLIB_KHR
#    define Window  X11Window
#    define Font    X11Font
#    define Time    X11Time
#    define KeyCode X11KeyCode
#    include <vulkan/vulkan.h>
#    include <shaderc/shaderc.h>
#    undef Window
#    undef Font
#    undef Time
#    undef KeyCode
#  else
#    error "unhandled platform/vulkan interaction"
#  endif
#  include "core/renderers/vulkan.cpp"
#elif DESHI_OPENGL //#if DESHI_VULKAN
#  if DESHI_WINDOWS
#    define GLAD_WGL_IMPLEMENTATION
#    include "glad/wgl.h"
#    define GLAD_GL_IMPLEMENTATION
#    include <glad/gl.h>
#  elif DESHI_LINUX //#if DESHI_WINDOWS
#    define GLAD_GL_IMPLEMENTATION
#    define GLAD_GLX_IMPLEMENTATION
#    define Window X11Window
#    define Font X11Font
#    define Time X11Time
#    define KeyCode X11KeyCode
#       define GLAD_UNUSED(x) (void)(x)
#       include <glad/glx.h>
#    undef Window
#    undef Font
#    undef Time
#    undef KeyCode
#  else
#    error "unhandled platform/opengl interaction"
#  endif //#elif DESHI_LINUX //#if DESHI_WINDOWS
#  include "core/renderers/opengl.cpp"
#elif DESHI_DIRECTX12 //#elif DESHI_OPENGL //#if DESHI_VULKAN
#  include "d3dx12/d3dx12.h"
#  include <d3d12.h>
#  include <wrl/client.h> //ComPtr
#  include <dxgi1_6.h>
//#  include <d3dcompiler.h> this is for compiling HLSL shaders at runtime, which ideally we wont do, but ill keep it just incase
// if we do, dont forget to link against d3dcompiler.lib and copy D3dcompiler_47.dll to the same file as our exe
#  include <DirectXMath.h>
#  include "core/renderers/directx.cpp"
#else //#elif DESHI_DIRECTX12 //#elif DESHI_OPENGL //#if DESHI_VULKAN
#  error "no renderer selected"
#endif //#else //#elif DESHI_DIRECTX12 //#elif DESHI_OPENGL //#if DESHI_VULKAN

//// global definitions ////
local Time deshi_time;
Time *g_time = &deshi_time;

local Input deshi_input;
Input *g_input = &deshi_input;

local Assets deshi_assets;
Assets *g_assets = &deshi_assets;

local ThreadManager deshi_threader;
ThreadManager *g_threader = &deshi_threader;

local uiContext deshi_ui{};
uiContext *g_ui = &deshi_ui;
local uiStyle deshi_ui_initial_style{};
uiStyle *ui_initial_style = &deshi_ui_initial_style;

Window *g_window = 0;

MemoryContext *g_memory = 0;
