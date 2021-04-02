#pragma once
#ifndef DESHI_IMGUI_H
#define DESHI_IMGUI_H

//keeping this include here so core.h can include this header and get imgui stuff
#include "../external/imgui/imgui.h"

#if defined(VULKAN)
#include "deshi_imgui_vulkan.h"
#elif defined(DX12)

#else
#include "deshi_imgui_vulkan.h"
#endif


#endif //DESHI_IMGUI_H