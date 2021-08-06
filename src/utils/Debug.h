#pragma once
#ifndef DESHI_DEBUG_H
#define DESHI_DEBUG_H

#include "../defines.h"

#include <string>

#include "string.h"
#include "array.h"

//std::cout short form
#define PRINTLN(x) std::cout << x << std::endl;

#define __FILENAME__ (std::strrchr(__FILE__, '\\') ? std::strrchr(__FILE__, '\\') + 1 : __FILE__)

#define TOSTDSTRING(...) ToStdString(__VA_ARGS__)
#define TOSTRING(...) ToString(__VA_ARGS__)

//makes a random number only once and then always returns that same number
//if called by the same object
#define PERM_RAND_INT ([]{ persist int rint = rand() % 100000; return rint;}())

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

static std::string ToStdString(const char* str) { return std::string(str); }
static std::string ToStdString(char* str)       { return std::string(str); }

static std::string ToStdString(const std::string& str) { return str; }

template<class T, typename std::enable_if<!has_str_method<T>::value, bool>::type = true>
static std::string ToStdString(T t) { return ToStdString(std::to_string(t)); }

template<class T, typename std::enable_if<has_str_method<T>::value, bool>::type = true>
static std::string ToStdString(T t) { return ToStdString(t.str()); }

template<class... T>
static std::string ToStdString(T... args) { 
	std::string strings[] = { "", (ToStdString(std::forward<T>(args))) ... };
	std::string str = "";
	for (std::string s : strings) { str += s; }
	
	return str;
}

static string ToString(const char* str) { return string(str); }
static string ToString(char* str) { return string(str); }

static string ToString(const string& str) { return str; }

template<class T, typename std::enable_if<!has_str_method<T>::value, bool>::type = true>
static string ToString(T t) { return ToString(string::toStr(t)); }

template<class T, typename std::enable_if<has_str_method<T>::value, bool>::type = true>
static string ToString(T t) { return ToString(string(t.str())); }

template<class... T>
static string ToString(T... args) {
	//string strings[] = ;
	array<string> strings{ "", (ToString(std::forward<T>(args))) ... };
	//string strings[] = { ToString(args), ... };
	string str = "";
	for (string& s : strings) { str += s; }

	return str;
}

#endif //DESHI_DEBUG_H