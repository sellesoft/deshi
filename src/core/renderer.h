#pragma once
#ifndef DESHI_RENDERER_H
#define DESHI_RENDERER_H

#if defined(VULKAN)
#include "renderer_vulkan.h"
#elif defined(DX12)

#else
#include "renderer_vulkan.h"
#endif

//dunno where to put these yet
//TODO(sushi) update this define to change each vertices positions each call
#define DeshDebugLines(name, i, v1, v2)\
static std::vector<u32> name; \
if (name.size() < i) name.push_back(DengRenderer->CreateDebugLine(v1, v2, Color::WHITE, true));

#define DeshDebugLinesCol(name, i, v1, v2, color)\
static std::vector<u32> name; \
if (name.size() < i) name.push_back(DengRenderer->CreateDebugLine(v1, v2, color, true));

#define DeshDebugTrisCol(name, i, v1, v2, v3, color)\
static std::vector<u32> name; \
if (name.size() < i) name.push_back(DengRenderer->CreateDebugTriangle(v1, v2, v3, color, true));

#define DeshDebugLine(v1, v2) DengRenderer->CreateDebugLine(v1, v2, Color::WHITE, true)
#define DeshDebugLineCol(v1, v2, color) DengRenderer->CreateDebugLine(v1, v2, color, true)

#define DeshDebugTri(v1, v2, v3) DengRenderer->CreateDebugTriangle(v1, v2, v3, Color::WHITE, true)


#endif //DESHI_RENDERER_H