#pragma once
#ifndef DESHI_H
#define DESHI_H

//// core headers ////
#include "defines.h"
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
#include "core/renderer.h"
#include "core/storage.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"

namespace deshi {
	void init(u32 winWidth = 1280, u32 winHeight = 720);
	void cleanup();
	bool shouldClose();
}

//////////////////////////
//// deshi quickstart ////
//////////////////////////
function void deshi_init(u32 winWidth = 1280, u32 winHeight = 720);
function void deshi_cleanup();
#define deshi_loop_start() \
TIMER_START(t_f); \
while(!DeshWindow->ShouldClose()){ \
DeshWindow-> Update(); \
DeshiImGui::NewFrame(); \
DeshInput->  Update();
#define deshi_loop_end() \
DeshConsole->Update(); \
UI::         Update(); \
Render::     Update(); \
DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f); \
}

#endif //DESHI_H