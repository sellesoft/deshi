#pragma once
#ifndef DESHI_RENDERER_H
#define DESHI_RENDERER_H

#if DESHI_VULKAN
#include "renderer_vulkan.h"
#elif DESHI_DX12

#else
#include "renderer_vulkan.h"
#endif

//((size_t)this << __LINE__ ^ (size_t)__FUNCTION__ >> (size_t)__FILENAME__)
//this is trying to make the identifier for the line as unique as possible, taking into account
//the file and function it was called in, as well as the line and object that called it
//idk how much this helps at this point, but im too afraid to remove it
//there is also probably a much better solution, so

//NOTE this gives warnings because the '<< __LINE__' is undefined if the __LINE__ is greater than 63
//     since a u64 can only store 64 bits
//NOTE this also previously gave warning about the casts to s64 because pointer size is depended on machine
//     however, size_t is always the size of a pointer

#define DebugLine(v1, v2, time)\
g_debug->DrawLine(v1, v2, ((size_t)this << __LINE__ ^ (size_t)__FUNCTION__ >> (size_t)__FILENAME__), time, Color::WHITE);

#define DebugLineCol(v1, v2, time, color)\
g_debug->DrawLine(v1, v2, ((size_t)this << __LINE__ ^ (size_t)__FUNCTION__ >> (size_t)__FILENAME__), time, color);

//these are for use in loops, if you'd like to just make a new line everytime something happens (regardless of a loop) use -1 for i
#define DebugLines(i, v1, v2, time)\
g_debug->DrawLine(i, v1, v2, ((size_t)this << __LINE__ ^ (size_t)__FUNCTION__ >> (size_t)__FILENAME__), time, Color::WHITE);

#define DebugLinesStatic(i, v1, v2, time)\
g_debug->DrawLine(i, v1, v2, (__LINE__ ^ (size_t)__FUNCTION__ >> (size_t)__FILENAME__), time, Color::WHITE);

#define DebugLinesCol(i, v1, v2, time, color)\
g_debug->DrawLine(i, v1, v2, ((size_t)this << __LINE__ ^ (size_t)__FUNCTION__ >> (size_t)__FILENAME__), time, color);

#define DebugTriggerStatic(mesh, transform, time)\
g_debug->DrawMesh(mesh, transform, (__LINE__ ^ (size_t)__FUNCTION__ >> (size_t)__FILENAME__), time, Color::WHITE);

#define DebugTriggersStatic(i, mesh, transform, time)\
g_debug->DrawMesh(i, mesh, transform, (__LINE__ ^ (size_t)__FUNCTION__ >> (size_t)__FILENAME__), time, Color::WHITE);

#define DeshDebugTrisCol(name, i, v1, v2, v3, color)\
static std::vector<u32> name; \
if (name.size() < i) name.push_back(DengRenderer->CreateDebugTriangle(v1, v2, v3, color, true));


#define DeshDebugTri(v1, v2, v3) DengRenderer->CreateDebugTriangle(v1, v2, v3, Color::WHITE, true)


#endif //DESHI_RENDERER_H