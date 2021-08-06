
//// core/game headers ////
#include "src/core/assets.h"
#ifndef DESHI_DISABLE_CONSOLE
#include "src/core/console.h"
#include "src/core/console2.h"
#endif
#ifndef DESHI_DISABLE_IMGUI
#include "src/core/imgui.h"
#endif
#include "src/core/input.h"
#include "src/core/renderer.h"
#include "src/core/time.h"
#include "src/core/ui.h"
#include "src/core/window.h"
#include "src/core/storage.h"

namespace deshi {
	void init();
	void cleanup();
	bool shouldClose();
}