#pragma once
#ifndef DESHI_STRING_CONVERSION_H
#define DESHI_STRING_CONVERSION_H

#include "string.h"
#include "cstring.h"
#include "color.h"
#include "../defines.h"
#include "../math/math.h"

#include <cstdarg>
#include <string>

///////////////
//// @stox ////
///////////////
global_ int 
stoi(const string& s){
    int x;
    sscanf(s.str, "%d", &x);
    return x;
}

global_ double
stod(const string& s) {
    return strtod(s.str, 0);
}

////////////////////
//// @to_string ////
////////////////////
global_ string 
to_string(cstring x){
    return string(x.str, x.count);
}

global_ string 
to_string(char* str){ 
	return string(str); 
}

global_ string 
to_string(const string& str){ 
	return str; 
}

global_ string 
to_string(const std::string& str){ 
	return str.c_str(); 
}

global_ string 
to_string(const char* fmt, ...){
    va_list argptr;
    va_start(argptr, fmt);
    string s;
    s.count  = vsnprintf(nullptr, 0, fmt, argptr);
    s.str   = (char*)malloc(s.count+1);
    s.space = s.count+1;
    vsnprintf(s.str, s.count+1, fmt, argptr);
    va_end(argptr);
    return s;
}

global_ string 
to_string(char x){
    string s(&x, 1);
    return s;
}

global_ string 
to_string(s32 x){
    string s;
    s.count = snprintf(nullptr, 0, "%d", x);
    s.str  = (char*)calloc(1, s.count+1);
    s.space = s.count+1;
    snprintf(s.str, s.count+1, "%d", x);
    return s;
}

global_ string 
to_string(u32 x){
    string s;
    s.count = snprintf(nullptr, 0, "%d", x);
    s.str  = (char*)calloc(1, s.count+1);
    s.space = s.count+1;
    snprintf(s.str, s.count+1, "%d", x);
    return s;
}

global_ string 
to_string(f32 x, bool trunc = true){
    string s;
    if(trunc){
        s.count = snprintf(nullptr, 0, "%g", x);
        s.str  = (char*)malloc(s.count+1);
        s.space = s.count+1;
        snprintf(s.str, s.count+1, "%g", x);
    }else{
        s.count = snprintf(nullptr, 0, "%f", x);
        s.str  = (char*)malloc(s.count+1);
        s.space = s.count+1;
        snprintf(s.str, s.count+1, "%f", x);
    }
    return s;
}

global_ string 
to_string(f64 x, bool trunc = true){
    string s;
    if(trunc){
        s.count = snprintf(nullptr, 0, "%g", x);
        s.str  = (char*)malloc(s.count+1);
        s.space = s.count+1;
        snprintf(s.str, s.count+1, "%g", x);
    }else{
        s.count = snprintf(nullptr, 0, "%f", x);
        s.str  = (char*)malloc(s.count+1);
        s.space = s.count+1;
        snprintf(s.str, s.count+1, "%f", x);
    }
    return s;
}

global_ string 
to_string(upt x){
    string s;
    s.count = snprintf(nullptr, 0, "%zu", x);
    s.str  = (char*)malloc(s.count+1);
    s.space = s.count+1;
    snprintf(s.str, s.count+1, "%zu", x);
    return s;
}

global_ string 
to_string(const color& x){
    string s;
    s.count = snprintf(nullptr, 0, "{R:%d, G:%d, B:%d, A:%d}", x.r, x.g, x.b, x.a);
    s.str  = (char*)malloc(s.count+1);
    s.space = s.count+1;
    snprintf(s.str, s.count+1, "{R:%d, G:%d, B:%d, A:%d}", x.r, x.g, x.b, x.a);
    return s;
}

global_ string 
to_string(const vec2& x, bool trunc = true){
    string s;
    if(trunc){
        s.count = snprintf(nullptr, 0, "(%g, %g)", x.x, x.y);
        s.str  = (char*)malloc(s.count+1);
        s.space = s.count+1;
        snprintf(s.str, s.count+1, "(%g, %g)", x.x, x.y);
    }else{
        s.count = snprintf(nullptr, 0, "(%+f, %+f)", x.x, x.y);
        s.str  = (char*)malloc(s.count+1);
        s.space = s.count+1;
        snprintf(s.str, s.count+1, "(%+f, %+f)", x.x, x.y);
    }
    return s;
}

global_ string 
to_string(const vec3& x, bool trunc = true){
    string s;
    if(trunc){
        s.count = snprintf(nullptr, 0, "(%g, %g, %g)", x.x, x.y, x.z);
        s.str  = (char*)malloc(s.count+1);
        s.space = s.count+1;
        snprintf(s.str, s.count+1, "(%g, %g, %g)", x.x, x.y, x.z);
    }else{
        s.count = snprintf(nullptr, 0, "(%+f, %+f, %+f)", x.x, x.y, x.z);
        s.str  = (char*)malloc(s.count+1);
        s.space = s.count+1;
        snprintf(s.str, s.count+1, "(%+f, %+f, %+f)", x.x, x.y, x.z);
    }
    return s;
}

global_ string 
to_string(const vec4& x, bool trunc = true){
    string s;
    if(trunc){
        s.count = snprintf(nullptr, 0, "(%g, %g, %g, %g)", x.x, x.y, x.z, x.w);
        s.str  = (char*)malloc(s.count+1);
        s.space = s.count+1;
        snprintf(s.str, s.count+1, "(%g, %g, %g, %g)", x.x, x.y, x.z, x.w);
    }else{
        s.count = snprintf(nullptr, 0, "(%+f, %+f, %+f, %+f)", x.x, x.y, x.z, x.w);
        s.str  = (char*)malloc(s.count+1);
        s.space = s.count+1;
        snprintf(s.str, s.count+1, "(%+f, %+f, %+f, %+f)", x.x, x.y, x.z, x.w);
    }
    return s;
}

global_ string 
to_string(const mat3& x, bool trunc = true){
    string s;
    forI(3){ 
        s += to_string(*((vec3*)(x.arr+(i*3))), trunc);
        s += "\n";
    }
    return s;
}

global_ string 
to_string(const mat4& x, bool trunc = true){
    string s;
    forI(4){ 
        s += to_string(*((vec4*)(x.arr+(i*4))), trunc);
        s += "\n";
    }
    return s;
}

#define TOSTRING(...) ToString(__VA_ARGS__)
template<class... T> global_ string 
ToString(T... args){
	string str;
	constexpr auto arg_count{sizeof...(T)};
	string arr[arg_count] = {to_string(std::forward<T>(args))...};
	forI(arg_count) str += arr[i];
	return str;
}

#endif //DESHI_STRING_CONVERSION_H
