#pragma once
#ifndef DESHI_RENDERER_H
#define DESHI_RENDERER_H

#if defined(VULKAN)
#include "renderer_vulkan.h"
#elif defined(DX12)

#else
#include "renderer_vulkan.h"
#endif

#endif //DESHI_RENDERER_H