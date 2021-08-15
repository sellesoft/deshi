
//// core/game headers ////
#include "defines.h"
#include "core/assets.h"
#ifndef DESHI_DISABLE_CONSOLE
#include "core/console.h"
#include "core/console2.h"
#endif
#ifndef DESHI_DISABLE_IMGUI
#include "core/imgui.h"
#endif
#include "core/camera.h"
#include "core/input.h"
#include "core/renderer.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"
#include "core/storage.h"

namespace deshi {
	void init();
	void cleanup();
	bool shouldClose();
}