#pragma once
#ifndef DESHI_H
#define DESHI_H

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
#include "core/storage.h"
#include "core/threading.h"
#include "core/ui.h"
#include "core/window.h"
#include "kigu/common.h"
#include "kigu/unicode.h"

#define deshi_init()                           \
  memory_init(Megabytes(100), Megabytes(924)); \
  platform_init();                             \
  logger_init();                               \
  console_init();                              \
  window_create(str8l("deshi"));               \
  render_init();                               \
  Storage::Init();                             \
  UI::Init();                                  \
  cmd_init();                                  \
  window_show(DeshWindow);                     \
  render_use_default_camera();                 \
  DeshThreadManager->init();                   \

#define deshi_loop_start() \
  while(platform_update()){  \

#define deshi_loop_end() \
    console_update();    \
    UI::Update();        \
    render_update();     \
    logger_update();     \
    memory_clear_temp(); \
  }

#define deshi_cleanup() \
  render_cleanup();     \
  logger_cleanup();     \
  memory_cleanup();     \

#endif //DESHI_H