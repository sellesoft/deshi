#pragma once
#ifndef DESHI_STRING_UTILS_H
#define DESHI_STRING_UTILS_H

#include "string.h"
#include "cstring.h"
#include "color.h"
#include "../defines.h"
#include "../math/math.h" //TODO maybe declare the math conversions here, but define them elsewhere so we dont have to include math here

#include <cstdarg>
#include <string>

///////////////
//// @stox ////
///////////////
global_ int 
stoi(const string& s){
	int x;
	(void)sscanf(s.str, "%d", &x);
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
	s.count = vsnprintf(nullptr, 0, fmt, argptr);
	s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count+1);
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
	s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count+1);
	s.space = s.count+1;
	snprintf(s.str, s.count+1, "%d", x);
	return s;
}

global_ string 
to_string(u32 x){
	string s;
	s.count = snprintf(nullptr, 0, "%d", x);
	s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count+1);
	s.space = s.count+1;
	snprintf(s.str, s.count+1, "%d", x);
	return s;
}

global_ string 
to_string(f32 x, bool trunc = true){
	string s;
	if(trunc){
		s.count = snprintf(nullptr, 0, "%g", x);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "%g", x);
	}else{
		s.count = snprintf(nullptr, 0, "%f", x);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
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
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "%g", x);
	}else{
		s.count = snprintf(nullptr, 0, "%f", x);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "%f", x);
	}
	return s;
}

global_ string 
to_string(upt x){
	string s;
	s.count = snprintf(nullptr, 0, "%zu", x);
	s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count+1);
	s.space = s.count+1;
	snprintf(s.str, s.count+1, "%zu", x);
	return s;
}

global_ string
to_string(void* ptr) {
	string s;
	s.count = snprintf(nullptr, 0, "%p", ptr);
	s.str = (char*)s.allocator->reserve(s.count + 1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count + 1);
	s.space = s.count + 1;
	snprintf(s.str, s.count + 1, "%p", ptr);
	return s;
}

global_ string 
to_string(const color& x){
	string s;
	s.count = snprintf(nullptr, 0, "{R:%d, G:%d, B:%d, A:%d}", x.r, x.g, x.b, x.a);
	s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count+1);
	s.space = s.count+1;
	snprintf(s.str, s.count+1, "{R:%d, G:%d, B:%d, A:%d}", x.r, x.g, x.b, x.a);
	return s;
}

global_ string 
to_string(const vec2& x, bool trunc = true){
	string s;
	if(trunc){
		s.count = snprintf(nullptr, 0, "(%g, %g)", x.x, x.y);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%g, %g)", x.x, x.y);
	}else{
		s.count = snprintf(nullptr, 0, "(%+f, %+f)", x.x, x.y);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
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
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%g, %g, %g)", x.x, x.y, x.z);
	}else{
		s.count = snprintf(nullptr, 0, "(%+f, %+f, %+f)", x.x, x.y, x.z);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
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
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%g, %g, %g, %g)", x.x, x.y, x.z, x.w);
	}else{
		s.count = snprintf(nullptr, 0, "(%+f, %+f, %+f, %+f)", x.x, x.y, x.z, x.w);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
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

global_ string
to_string(const matN& x, bool trunc = true) {
	if (x.rows == 0 || x.cols == 0) {
		return "|Zero dimension matrix|";
	}
	
	string str = to_string(x.rows) + "x" + to_string(x.cols) + " matN<n,m>:\n|";
	if (x.rows == 1) {
		for (int i = 0; i < x.cols - 1; ++i) {
			char buffer[15];
			snprintf(buffer, 15, "%+g", x.data[i]);
			str += string(buffer) + ", ";
		}
		char buffer[15];
		snprintf(buffer, 15, "%+g", x.data[x.elementCount - 1]);
		str += string(buffer) + "|";
		return str;
	}
	
	for (int i = 0; i < x.elementCount - 1; ++i) {
		char buffer[15];
		snprintf(buffer, 15, "%+g", x.data[i]);
		str += string(buffer);
		if ((i + 1) % x.cols != 0) {
			str += ", ";
		}
		else {
			str += "|\n|";
		}
	}
	char buffer[15];
	snprintf(buffer, 15, "%+g", x.data[x.elementCount - 1]);
	str += string(buffer) + "|";
	return str;
}

#define toStr(...) ToString(__VA_ARGS__)
template<class... T> global_ string 
ToString(T... args){
	string str;
	constexpr auto arg_count{sizeof...(T)};
	string arr[arg_count] = {to_string(std::forward<T>(args))...};
	forI(arg_count) str += arr[i];
	return str;
}


global_ u32 
find_first_char(const char* str, u32 strsize, char c, u32 offset = 0) {
	for (u32 i = offset; i < strsize; ++i)
		if (str[i] == c) return i;
	return npos;
}
global_ u32 find_first_char(const char* str, char c, u32 offset = 0)    { return find_first_char(str, strlen(str), c, offset); } 
global_ u32 find_first_char(const cstring& str, char c, u32 offset = 0) { return find_first_char(str.str, str.count, c, offset); } 
global_ u32 find_first_char(const string& str, char c, u32 offset = 0)  { return find_first_char(str.str, str.count, c, offset); }

global_ u32 
find_first_char_not(const char* str, u32 strsize, char c, u32 offset = 0) {
	for (u32 i = 0; i < strsize; ++i)
		if (str[i] != c) return i;
	return npos;
}
global_ u32 find_first_char_not(const char*    str, char c, u32 offset = 0) { return find_first_char_not(str, strlen(str), c, offset); }
global_ u32 find_first_char_not(const cstring& str, char c, u32 offset = 0) { return find_first_char_not(str.str, str.count, c, offset); }
global_ u32 find_first_char_not(const string&  str, char c, u32 offset = 0) { return find_first_char_not(str.str, str.count, c, offset); }

global_ u32 
find_last_char(const char* str, u32 strsize, char c, u32 offset = 0) {
	for (u32 i = (offset != 0 ? offset : strsize - 1); i != 0; --i)
		if (str[i] == c) return i;
	return npos;
}
global_ u32 find_last_char(const char*    str, char c, u32 offset = 0) { return find_last_char(str, strlen(str), c, offset); } 
global_ u32 find_last_char(const cstring& str, char c, u32 offset = 0) { return find_last_char(str.str, str.count, c, offset); } 
global_ u32 find_last_char(const string&  str, char c, u32 offset = 0) { return find_last_char(str.str, str.count, c, offset); }

global_ u32 
find_last_char_not(const char* str, u32 strsize, char c, u32 offset = 0) {
	for (u32 i = strsize - 1; i != 0; --i)
		if (str[i] != c) return i;
	return npos;
}
global_ u32 find_last_char_not(const char*    str, char c, u32 offset = 0) { return find_last_char_not(str, strlen(str), c, offset); }
global_ u32 find_last_char_not(const cstring& str, char c, u32 offset = 0) { return find_last_char_not(str.str, str.count, c, offset); }
global_ u32 find_last_char_not(const string&  str, char c, u32 offset = 0) { return find_last_char_not(str.str, str.count, c, offset); }

FORCE_INLINE global_ b32
str_begins_with(const char* buf1, const char* buf2, u32 buf2len) {return !memcmp(buf1, buf2, buf2len);}
global_ b32 str_begins_with(const string&  buf1, const string&  buf2) { return str_begins_with(buf1.str, buf2.str, buf2.count); }
global_ b32 str_begins_with(const cstring& buf1, const cstring& buf2) { return str_begins_with(buf1.str, buf2.str, buf2.count); }
global_ b32 str_begins_with(const string&  buf1, const cstring& buf2) { return str_begins_with(buf1.str, buf2.str, buf2.count); }
global_ b32 str_begins_with(const cstring& buf1, const string&  buf2) { return str_begins_with(buf1.str, buf2.str, buf2.count); }
global_ b32 str_begins_with(const char*    buf1, const string&  buf2) { return str_begins_with(buf1, buf2.str, buf2.count); }
global_ b32 str_begins_with(const string&  buf1, const char*    buf2) { return str_begins_with(buf1.str, buf2, strlen(buf2)); }
global_ b32 str_begins_with(const char*    buf1, const cstring& buf2) { return str_begins_with(buf1, buf2.str, buf2.count); }
global_ b32 str_begins_with(const cstring& buf1, const char*    buf2) { return str_begins_with(buf1.str, buf2, strlen(buf2)); }
global_ b32 str_begins_with(const char*    buf1, const char*    buf2) { return str_begins_with(buf1, buf2, strlen(buf2)); }

FORCE_INLINE global_ b32
str_ends_with(const char* buf1, u32 buf1len, const char* buf2, u32 buf2len) {return (buf2len > buf1len ? 0 : !memcmp(buf1 + (buf2len - buf1len), buf2, buf2len);}
global_ b32 str_ends_with(const string&  buf1, const string&  buf2) { return str_ends_with(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_ends_with(const cstring& buf1, const cstring& buf2) { return str_ends_with(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_ends_with(const string&  buf1, const cstring& buf2) { return str_ends_with(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_ends_with(const cstring& buf1, const string&  buf2) { return str_ends_with(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_ends_with(const char*    buf1, const string&  buf2) { return str_ends_with(buf1, strlen(buf1), buf2.str, buf2.count); }
global_ b32 str_ends_with(const string&  buf1, const char*    buf2) { return str_ends_with(buf1.str, buf1.count, buf2, strlen(buf2)); }
global_ b32 str_ends_with(const char*    buf1, const cstring& buf2) { return str_ends_with(buf1, strlen(buf1), buf2.str, buf2.count); }
global_ b32 str_ends_with(const cstring& buf1, const char*    buf2) { return str_ends_with(buf1.str, buf1.count, buf2, strlen(buf2)); }
global_ b32 str_ends_with(const char*    buf1, const char*    buf2) { return str_ends_with(buf1, strlen(buf1), buf2, strlen(buf2)); }

global_ b32
str_contains(const char* buf1, u32 buf1len, const char* buf2, u32 buf2len) {
	if (buf2len > buf1len) return false;
	for (u32 i = 0; i < buf2len - buf1len; i++) {
		if (!memcmp(buf2, buf1 + i, buf2len)) return true;
	}
	return false;
}
global_ b32 str_contains(const string&  buf1, const string&  buf2) { return str_contains(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_contains(const cstring& buf1, const cstring& buf2) { return str_contains(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_contains(const string&  buf1, const cstring& buf2) { return str_contains(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_contains(const cstring& buf1, const string&  buf2) { return str_contains(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_contains(const char*    buf1, const string&  buf2) { return str_contains(buf1, strlen(buf1), buf2.str, buf2.count); }
global_ b32 str_contains(const string&  buf1, const char*    buf2) { return str_contains(buf1.str, buf1.count, buf2, strlen(buf2)); }
global_ b32 str_contains(const char*    buf1, const cstring& buf2) { return str_contains(buf1, strlen(buf1), buf2.str, buf2.count); }
global_ b32 str_contains(const cstring& buf1, const char*    buf2) { return str_contains(buf1.str, buf1.count, buf2, strlen(buf2)); }
global_ b32 str_contains(const char*    buf1, const char*    buf2) { return str_contains(buf1, strlen(buf1), buf2, strlen(buf2)); }


#endif //DESHI_STRING_UTILS_H
