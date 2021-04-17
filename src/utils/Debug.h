#pragma once
#ifndef DESHI_DEBUG_H
#define DESHI_DEBUG_H

#include "defines.h"

#include <string>
#include <iostream>

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

#define TOSTRING(...) Debug::ToString(__VA_ARGS__)

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