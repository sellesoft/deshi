#pragma once
#ifndef DESHI_DEBUG_H
#define DESHI_DEBUG_H

#include "../defines.h"

#include <string>
#include <iostream>
#include <unordered_map>
#include <assert.h>

//std::cout short form
#define PRINTLN(x) std::cout << x << std::endl;

#define __FILENAME__ (std::strrchr(__FILE__, '\\') ? std::strrchr(__FILE__, '\\') + 1 : __FILE__)

#define TOSTRING(...) ToString(__VA_ARGS__)

//makes a random number only once and then always returns that same number
//if called by the same object
#define PERM_RAND_INT ([]{ static int rint = rand() % 100000; return rint;}())

namespace Math { //forward declare average
	template<class T>
		static double average(const T& container, int size);
}

//template magic thanks to fux#2562
template<class T>
struct has_str_method {
	template<class U> static decltype(&U::str, std::true_type{}) test(int);
	template<class> static std::false_type test(...);
	static constexpr bool value = decltype(test<T>(0))::value;
};


//// Primarily for outputting to ingame console, but can return a string from any object that is a c++ number
//// or any of our classes (or yours :) ) that has a .str() member

static std::string ToString(const char* str) { return std::string(str); }
static std::string ToString(char* str)       { return std::string(str); }

static std::string ToString(const std::string& str) { return str; }

template<class T, typename std::enable_if<!has_str_method<T>::value, bool>::type = true>
static std::string ToString(T t) { return ToString(std::to_string(t)); }

template<class T, typename std::enable_if<has_str_method<T>::value, bool>::type = true>
static std::string ToString(T t) { return ToString(t.str()); }

template<class... T>
static std::string ToString(T... args) { 
	std::string strings[] = { "", (ToString(std::forward<T>(args))) ... };
	std::string str = "";
	for (std::string s : strings) { str += s; }
	
	return str;
}




struct Vector3;
struct Color;
struct Matrix4;
struct Mesh;

struct MeshInfo {
	u32 meshID;
	float idleTime;
	float allowedTime = 0;
	b32 calledThisFrame;
	b32 clearNextFrame;
	b32 wasInvis;
	Vector3* last[2]; //TODO(sushi, Op) figure out why checking if a line has changed doesnt work
	
};

//GPU debug mesh handler that could also end up being used for more stuff later, so i'll keep it named Debug
struct Debug {
	std::unordered_map<int, MeshInfo> meshes;
	
	//TODO(sushi) find a better solution than this, because this variable with eventually wrap around and cause issues after a long period of time
	int miter = 0;
	
	
	void Update();
	
	void DrawLine(Vector3 v1, Vector3 v2, size_t unique, Color color);
	void DrawLine(Vector3 v1, Vector3 v2, size_t unique, float time, Color color);
	void DrawLine(int i, Vector3 v1, Vector3 v2, size_t unique, Color color);
	void DrawLine(int i, Vector3 v1, Vector3 v2, size_t unique, float time, Color color);
	
	void DrawMesh(Mesh* mesh, Matrix4 transform, size_t unique, Color color);
	void DrawMesh(Mesh* mesh, Matrix4 transform, size_t unique, float time, Color color);
	void DrawMesh(int i, Mesh* mesh, Matrix4 transform, size_t unique, Color color);
	void DrawMesh(int i, Mesh* mesh, Matrix4 transform, size_t unique, float time, Color color);
	void DrawFrustrum(Vector3 position, Vector3 target, f32 aspectRatio, f32 fovx, f32 nearZ, f32 farZ, f32 time, Color color);
};

extern Debug* g_debug;

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

#endif //DESHI_DEBUG_H