#pragma once
#ifndef DESHI_IMGUI_H
#define DESHI_IMGUI_H

//keeping this include here so i can just include this header to get imgui stuff
#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui_internal.h"

#if DESHI_VULKAN
#include "imgui_vulkan.h"
#elif DESHI_DX12

#else
#include "imgui_vulkan.h"
#endif


#endif //DESHI_IMGUI_H