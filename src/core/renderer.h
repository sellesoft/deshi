#pragma once
#ifndef DESHI_RENDERER_H
#define DESHI_RENDERER_H

#if defined(VULKAN)
#include "renderer_vulkan.h"
#elif defined(DX12)

#else
#include "renderer_vulkan.h"
#endif

//((s64)this << __LINE__ ^ (s64)__FUNCTION__ >> (s64)__FILENAME__)
//this is trying to make the identifier for the line as unique as possible, taking into account
//the file and function it was called in, as well as the line and object that called it
//idk how much this helps at this point, but im too afraid to remove it
//there is also probably a much better solution, so


#define DebugLine(v1, v2, time)\
g_debug->DrawLine(v1, v2, ((s64)this << __LINE__ ^ (s64)__FUNCTION__ >> (s64)__FILENAME__), time, Color::WHITE);

#define DebugLineCol(v1, v2, time, color)\
g_debug->DrawLine(v1, v2, ((s64)this << __LINE__ ^ (s64)__FUNCTION__ >> (s64)__FILENAME__), time, color);

//these are for use in loops, if you'd like to just make a new line everytime something happens (regardless of a loop) use -1 for i
#define DebugLines(i, v1, v2, time)\
g_debug->DrawLine(i, v1, v2, ((s64)this << __LINE__ ^ (s64)__FUNCTION__ >> (s64)__FILENAME__), time, Color::WHITE);

#define DebugLinesStatic(i, v1, v2, time)\
g_debug->DrawLine(i, v1, v2, (__LINE__ ^ (s64)__FUNCTION__ >> (s64)__FILENAME__), time, Color::WHITE);


#define DebugLinesCol(i, v1, v2, time, color)\
g_debug->DrawLine(i, v1, v2, ((s64)this << __LINE__ ^ (s64)__FUNCTION__ >> (s64)__FILENAME__), time, color);

#define DeshDebugTrisCol(name, i, v1, v2, v3, color)\
static std::vector<u32> name; \
if (name.size() < i) name.push_back(DengRenderer->CreateDebugTriangle(v1, v2, v3, color, true));


#define DeshDebugTri(v1, v2, v3) DengRenderer->CreateDebugTriangle(v1, v2, v3, Color::WHITE, true)


#endif //DESHI_RENDERER_H