
//// core/game headers ////
#include "core/assets.h"
#ifndef DESHI_DISABLE_CONSOLE
#include "core/console.h"
#include "core/console2.h"
#endif
#ifndef DESHI_DISABLE_IMGUI
#include "core/imgui.h"
#endif
#include "core/input.h"
#include "core/renderer.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"
#include "core/storage.h"

//// utility headers ////
#include "defines.h"
#include "utils/color.h"
#include "utils/tuple.h"
#include "utils/containermanager.h"
#include "utils/utils.h"
#include "utils/optional.h"
#include "utils/debug.h"
#include "utils/ringarray.h"
#include "utils/command.h"
#include "utils/font.h"
#include "utils/array.h"
#include "utils/hash.h"
#include "utils/map.h"
#include "utils/view.h"
#include "math/math.h"

namespace deshi {
	void init();
	void cleanup();
	bool shouldClose();
}