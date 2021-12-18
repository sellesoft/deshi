#pragma once
#ifndef DESHI_IMGUI_H
#define DESHI_IMGUI_H

#include "../math/vector.h"
#include "../utils/Color.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../external/imgui/imgui.h" //includes <float.h>,<stdarg.h>,<stddef.h>,<string.h>
#include "../external/imgui/imgui_internal.h" //includes <stdio.h>,<stdlib.h>,<math.h>,<limits.h>

namespace DeshiImGui{
	
	void Init();
	void Cleanup();
	void NewFrame();
	
}; //namespace DeshiImGui

#endif //DESHI_IMGUI_H