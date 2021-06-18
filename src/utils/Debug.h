#pragma once
#ifndef DESHI_DEBUG_H
#define DESHI_DEBUG_H

#include "defines.h"

#include <string>
#include <iostream>
#include <unordered_map>
#include <assert.h>

//std::cout short form
#define PRINTLN(x) std::cout << x << std::endl;

#define __FILENAME__ (std::strrchr(__FILE__, '\\') ? std::strrchr(__FILE__, '\\') + 1 : __FILE__)

//assert
#ifndef NDEBUG
#define ASSERT(condition, message) \
do { \
if (! (condition)) { \
std::string file = __FILENAME__; \
std::cout << "Assertion '" #condition "' failed in " + file + " line " + std::to_string(__LINE__) + ": \n" #message << std::endl;  \
std::terminate(); \
} \
} while (false)
#else
#   define ASSERT(condition, message) condition;
#endif

//debug breakpoint
#ifdef _MSC_VER
#define DEBUG_BREAK __debugbreak()
#else
#define DEBUG_BREAK (void)0
#endif

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
	
};

extern Debug* g_debug;

#endif //DESHI_DEBUG_H