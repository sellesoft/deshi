#pragma once
#ifndef DESHI_H
#define DESHI_H

//// core headers ////
#include "kigu/common.h"
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
#include "core/file.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/model.h"
#include "core/render.h"
#include "core/storage.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"

namespace deshi {
	void init(u32 winWidth = 1280, u32 winHeight = 720);
	void cleanup();
	b32  shouldClose();
}

//////////////////////////
//// deshi quickstart ////
//////////////////////////
#define deshi_init() \
  Stopwatch deshi_watch = start_stopwatch(); \
  memory_init(Gigabytes(1), Gigabytes(1)); \
  platform_init(); \
  logger_init(); \
  console_init(); \
  DeshWindow->Init(str8_lit("deshi"), 1280, 720); \
  render_init(); \
  Storage::Init(); \
  UI::Init(); \
  cmd_init(); \
  DeshWindow->ShowWindow(); \
  render_use_default_camera(); \
  DeshThreadManager->init(); \
  LogS("deshi","Finished deshi initialization in ",peek_stopwatch(deshi_watch),"ms");

#define deshi_cleanup() \
  render_cleanup(); \
  DeshWindow->Cleanup(); \
  logger_cleanup(); \
  memory_cleanup(); \

#define deshi_loop_start() \
  Stopwatch frame_stopwatch = start_stopwatch(); \
  while(!DeshWindow->ShouldClose()){ \
    DeshWindow->Update(); \
    platform_update(); \

#define deshi_loop_end() \
    console_update(); \
    UI::Update(); \
    render_update(); \
    memory_clear_temp(); \
    DeshTime->frameTime = reset_stopwatch(&frame_stopwatch); \
  }

#endif //DESHI_H