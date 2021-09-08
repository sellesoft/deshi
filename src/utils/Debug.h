#pragma once
#ifndef DESHI_DEBUG_H
#define DESHI_DEBUG_H

#include "string.h"
#include "string_conversion.h"
#include "array.h"
#include "../defines.h"

#include <string>

//std::cout short form
#define PRINTLN(x) std::cout << x << std::endl;

#define __FILENAME__ (std::strrchr(__FILE__, '\\') ? std::strrchr(__FILE__, '\\') + 1 : __FILE__)

#define TOSTDSTRING(...) to_std_string(__VA_ARGS__)
#define TOSTRING(...) ToDeshiString(__VA_ARGS__)

//makes a random number only once and then always returns that same number
//if called by the same object
#define PERM_RAND_INT ([]{ persist int rint = rand() % 100000; return rint;}())

namespace Math { //forward declare average
	template<class T>
		static double average(const T& container, int size);
}

//template magic thanks to fux#2562
/*
template<class T>
struct has_str_method {
	template<class U> static decltype(&U::str, std::true_type{}) test(int);
	template<class> static std::false_type test(...);
	static constexpr bool value = decltype(test<T>(0))::value;
};
*/

//// Primarily for outputting to ingame console, but can return a string from any object that is a c++ number
//// or any of our classes (or yours :) ) that has a .str() member

static std::string to_std_string(const char* str) { return std::string(str); }
static std::string to_std_string(char* str)       { return std::string(str); }

static std::string to_std_string(const std::string& str) { return str; }
static std::string to_std_string(const string& t) { return to_std_string(t.str); }

//template<class T, typename std::enable_if<!has_str_method<T>::value, bool>::type = true>
template<class T>
static std::string to_std_string(T t) { return to_std_string(std::to_string(t)); }

//template<class T, typename std::enable_if<has_str_method<T>::value, bool>::type = true>
//static std::string ToStdString(T t) { return ToStdString(std::to_string(t)); }

template<class... T>
static std::string to_std_string(T... args) { 
	std::string strings[] = { "", (to_std_string(std::forward<T>(args))) ... };
	std::string str = "";
	for (std::string& s : strings) { str += s; }
	
	return str;
}

static string to_string(char* str) { return string(str); }

static string to_string(const string& str) { return str; }
static string to_string(const std::string& str) { return str.c_str(); }

template<class... T>
static string ToDeshiString(T... args) {
	//string strings[] = ;
	array<string> strings{ "", (to_string(std::forward<T>(args))) ... };
	//string strings[] = { to_string(args), ... };
	string str = "";
	for (string& s : strings) { str += s; }
	
	return str;
}

#endif //DESHI_DEBUG_H