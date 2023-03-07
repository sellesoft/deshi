// deshi Quickstart Includes and Macros
#ifndef DESHI_H
#define DESHI_H
#include "core/assets.h"
#include "core/commands.h"
#include "core/console.h"
#ifndef DESHI_DISABLE_IMGUI
#  define IMGUI_DEFINE_MATH_OPERATORS
#  include "core/imgui.h"
#endif //DESHI_DISABLE_IMGUI
#include "core/logger.h"
#include "core/memory.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/threading.h"
#include "core/ui.h"
#include "core/ui2.h"
#include "core/ui2_widgets.h"
#include "core/window.h"
#include "kigu/common.h"
#include "kigu/profiling.h"

#define deshi_init()                           \
  profiler_init();                             \
  memory_init(Megabytes(100), Megabytes(512)); \
  platform_init();                             \
  logger_init();                               \
  window_create(str8l("deshi"));               \
  render_init();                               \
  assets_init();                               \
  uiInit(g_memory,0);                          \
  UI::Init();                                  \
  console_init();                              \
  cmd_init();                                  \
  window_show(DeshWindow);                     \
  render_use_default_camera();                 \
threader_init();

#define deshi_init_specify(name,main_size,temp_size) \
  profiler_init();                                   \
  memory_init(main_size,temp_size);                  \
  platform_init();                                   \
  logger_init();                                     \
  window_create(str8l(name));                        \
  render_init();                                     \
  assets_init();                                     \
  uiInit(g_memory,0);                                \
  UI::Init();                                        \
  console_init();                                    \
  cmd_init();                                        \
  window_show(DeshWindow);                           \
  render_use_default_camera();                       \
  threader_init();

#define deshi_loop_start()   \
  while(platform_update()){

#define deshi_loop_end() \
    console_update();    \
    UI::Update();        \
    uiUpdate();          \
    render_update();     \
    logger_update();     \
    memory_clear_temp(); \
  }

#define deshi_cleanup() \
  render_cleanup();     \
  logger_cleanup();     \
  memory_cleanup();

#endif //DESHI_H