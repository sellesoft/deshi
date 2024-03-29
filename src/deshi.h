// deshi Quickstart Includes and Macros
#ifndef DESHI_H
#define DESHI_H
#include "core/assets.h"
#include "core/commands.h"
#include "core/console.h"
#include "core/graphics.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/threading.h"
#include "core/ui.h"
#include "core/ui_widgets.h"
#include "core/window.h"
#include "math/math.h"
#include "kigu/common.h"
#include "kigu/profiling.h"

#define deshi_init()                              \
  profiler_init();                                \
  memory_init(Megabytes(100), Megabytes(512));    \
  platform_init();                                \
  logger_init();                                  \
  Window* window = window_create(str8l("deshi")); \
  window_show(window);                            \
  graphics_init(window);                          \
  assets_init(window);                            \
  render_init();                                  \
  render_set_active_window(window);               \
  render_temp_init(window, 0xffff);               \
  ui_init(window);                                \
  console_init();                                 \
  cmd_init();                                     \
  Camera* camera = render_camera_create();        \
  camera->forward = vec3{0,0,1};                  \
  render_camera_update_perspective_projection(camera, window->width, window->height, 90.0f, 0.1f, 1000.0f); \
  render_camera_update_view(camera);              \
  render_set_active_camera(camera);

#define deshi_init_specify(name,main_size,temp_size) \
  profiler_init();                                   \
  memory_init((main_size), (temp_size));             \
  platform_init();                                   \
  logger_init();                                     \
  Window* window = window_create(str8l(name));       \
  window_show(window);                               \
  graphics_init(window);                             \
  assets_init(window);                               \
  render_init();                                     \
  render_set_active_window(window);                  \
  render_temp_init(window, 0xffff);                  \
  ui_init(window);                                   \
  console_init();                                    \
  cmd_init();                                        \
  Camera* camera = render_camera_create();           \
  camera->forward = vec3{0,0,1};                     \
  render_camera_update_perspective_projection(camera, window->width, window->height, 90.0f, 0.1f, 1000.0f); \
  render_camera_update_view(camera);                 \
  render_set_active_camera(camera);

#define deshi_loop_start()   \
  while(platform_update()){  \
    render_temp_clear();

#define deshi_loop_end()     \
    console_update();        \
    render_update();         \
    ui_update(window);       \
    graphics_update(window); \
    logger_update();         \
    memory_clear_temp();     \
  }

#define deshi_cleanup() \
  logger_cleanup();     \
  memory_cleanup();

#endif //DESHI_H
