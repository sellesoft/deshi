#pragma once
#ifndef DESHI_DEBUG_H
#define DESHI_DEBUG_H

#include "defines.h"
#include "Color.h"

#include <map>
#include <stack>
#include <regex>
#include <chrono>
#include <string>
#include <vector>
#include <sstream>
#include <cstdarg>
#include <fstream>
#include <iostream>
#include <stdexcept>

#define PRINT(x) std::cout << x << std::endl;
#ifndef NDEBUG
#   define ASSERTWARN(condition, message) \
do { \
if (! (condition)) { \
std::string file = __FILENAME__; \
std::cout << "Warning '" #condition "' failed in " + file + " line " + std::to_string(__LINE__) + ": \n" #message << std::endl; \
} \
} while (false)
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

#define __FILENAME__ (std::strrchr(__FILE__, '\\') ? std::strrchr(__FILE__, '\\') + 1 : __FILE__)

#define LOG(...)     admin->console->PushConsole(TOSTRING("[c:yellow]", __VA_ARGS__, "[c]"))
#define ERROR(...)   admin->console->PushConsole(TOSTRING("[c:error]", __VA_ARGS__, "[c]"))
#define SUCCESS(...) admin->console->PushConsole(TOSTRING("[c:green]", __VA_ARGS__, "[c]"))
//#define PRINT(...)   admin->GetSystem<Console>()->PushConsole(TOSTRING(__VA_ARGS__))

//additionally prints where function was called
#define LOG_LOC(...)     admin->console->PushConsole(TOSTRING("[c:yellow]In ", __FILENAME__, " at ", __LINE__ , ": \n[c]", "[c:yellow]", __VA_ARGS__, "[c]"))
#define ERROR_LOC(...)   admin->console->PushConsole(TOSTRING("[c:error]In ", __FILENAME__, " at ", __LINE__, ": \n[c]", "[c:error]", __VA_ARGS__, "[c]"))
#define SUCCESS_LOC(...) admin->console->PushConsole(TOSTRING("[c:green]In ", __FILENAME__, " at ", __LINE__, ": \n[c]", "[c:green]", __VA_ARGS__, "[c]"))

#define DASSERT(condition, message)     if(!(condition) && !admin->paused){ ERROR_LOC("Assertion '" #condition "' failed: \n", message); admin->paused = true;}
#define DASSERTWARN(condition, message) if(!(condition) && !admin->paused) LOG_LOC("Assertion '" #condition "' failed: \n", message)

#define TOSTRING(...) Debug::ToString(__VA_ARGS__)

//stringize certain macros
//this is all probably REALLY slow but will be as is unless I find a more elegent solution
#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)
#define LINE_NUM  STRINGIZE(__LINE__)


#define LOGFUNC LOG(__FUNCTION__, " called")
#define LOGFUNCM(...) LOG(__FUNCTION__, " called ", __VA_ARGS__)

//makes a random number only once and then always returns that same number
//if called by the same object
#define PERM_RAND_INT ([]{ static int rint = rand() % 100000; return rint;}())

//#define BUFFER_IDENTIFIER ([]{ static int id_ini = buffer_size++; return id_ini; }())
//#define BUFFERLOG(...) g_cBuffer.add(new StringedBuffer(TOSTRING(__VA_ARGS__), BUFFER_IDENTIFIER))

#define BUFFERLOG(i, ...) DEBUG g_cBuffer.add_to_index(TOSTRING(__VA_ARGS__), i)

#define BUFFERLOGI(i, o, ...) DEBUG ([&]{static int iter = 0; if(iter == o){g_cBuffer.add_to_index(TOSTRING(__VA_ARGS__), i); iter = 0;} else iter++;}())

using namespace std::chrono;

extern bool GLOBAL_DEBUG;

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

namespace Debug {
	
	//// Primarily for outputting to ingame console, but can return a string from any object that is a c++ number
	//// or any of our classes (or yours :) ) that has a .str() member
	
	//returns the string for in engine printing
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
};

#endif //DESHI_DEBUG_H