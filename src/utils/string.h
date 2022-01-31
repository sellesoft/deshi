#pragma once
#ifndef DESHI_STRING_H
#define DESHI_STRING_H

#ifndef DESHI_STRING_ALLOCATOR
#  define DESHI_STRING_ALLOCATOR stl_allocator
#endif

#ifdef TRACY_ENABLE
#include "Tracy.hpp"
#endif

#include "../defines.h"

#include <cstring>
#include <cstdio>
#include <iostream> //std::ostream operator<<


struct string{
	typedef char CHAR;
	//static constexpr u32 npos = -1;
	static constexpr u32 CHAR_SIZE = sizeof(CHAR);
	
	u32   count;
	u32   space;
	CHAR* str;
	
	Allocator* allocator; //TODO(delle) maybe make this a constructor arg like array?
	
	string();
	string(const CHAR* s);
	string(const CHAR* s, u32 count);
	string(const string& s);
	string(const cstring& s);
	~string();
	
	CHAR&  operator[](u32 idx);
	CHAR   operator[](u32 idx) const;
	void   operator= (const CHAR* s);
	void   operator= (const string& s);
	void   operator+=(const CHAR* s);
	void   operator+=(const string& s);
	string operator--(int);
	string operator+ (const CHAR* c) const;
	string operator+ (const string& s) const;
	bool   operator==(const string& s) const;
	bool   operator!=(const string& s) const;
	bool   operator==(const CHAR* s) const;
	bool   operator!=(const CHAR* s) const;
	friend string operator+ (const CHAR* c, const string& s);
	inline explicit operator bool(){ return count; }
	
	void   reserve(u32 _space);
	void   clear();
	void   erase(u32 idx);
	void   insert(CHAR c, u32 idx); //inserts at specified idx, pushing the CHARacter at idx and all following CHARacters to the right
	void   replace(CHAR c, const string& with); //replaces all occurences of a char with a string
	CHAR   at(u32 idx) const; //returns a copy of the CHARacter at idx
	string substr(u32 start, u32 end = npos) const; //returns a string including the start and end CHARacters, end equals the end of the string (size-1) if npos
	u32    findFirstStr(const string& s) const;
	u32    findFirstChar(CHAR c, u32 offset = 0) const; //returns first of CHAR from offset
	u32    findFirstCharNot(CHAR c, u32 offset = 0) const; //returns first of CHAR from offset backwards
	u32    findLastChar(CHAR c, u32 offset = 0) const;
	u32    findLastCharNot(CHAR c) const;
	u32    CHARCount(CHAR c) const; //returns how many times a CHAR appears in the string
	string substrToChar(CHAR c) const; //returns a substring from the beginning to specifiec CHAR, not including the CHAR
	b32    beginsWith(const string& s) const;
	b32    endsWith(const string& s) const;
	b32    contains(const string& s) const;
	
	static string eatSpacesLeading(const string& s);
	static string eatSpacesTrailing(const string& s);
	static string toUpper(const string& s);
	static string toLower(const string& s);
};

///////////////////////
//// @constructors ////
///////////////////////

inline string::string(){DPZoneScoped;
	allocator = DESHI_STRING_ALLOCATOR;
	count  = 0;
	space = 0;
	str   = 0;
};


inline string::string(const CHAR* s){DPZoneScoped;
	allocator = DESHI_STRING_ALLOCATOR;
	count  = strlen(s);
	space = RoundUpTo(count+1, 4);
	str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
	allocator->commit(str, space*CHAR_SIZE);
	memcpy(str, s, count*CHAR_SIZE);
}


inline string::string(const CHAR* s, u32 _size){DPZoneScoped;
	allocator = DESHI_STRING_ALLOCATOR;
	count  = _size;
	space = RoundUpTo(count+1, 4);
	str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
	allocator->commit(str, space*CHAR_SIZE);
	memcpy(str, s, count*CHAR_SIZE);
}


inline string::string(const string& s){DPZoneScoped;
	allocator = DESHI_STRING_ALLOCATOR;
	count = s.count;
	space = RoundUpTo(count + 1, 4);
	str = (CHAR*)allocator->reserve(space * CHAR_SIZE); Assert(str, "Failed to allocate memory");
	allocator->commit(str, space * CHAR_SIZE);
	memcpy(str, s.str, count * CHAR_SIZE);
}


inline string::string(const cstring& s) {DPZoneScoped;
	allocator = DESHI_STRING_ALLOCATOR;
	count = s.count;
	space = RoundUpTo(count + 1, 4);
	str = (CHAR*)allocator->reserve(space * CHAR_SIZE); Assert(str, "Failed to allocate memory");
	allocator->commit(str, space * CHAR_SIZE);
	memcpy(str, s.str, count * CHAR_SIZE);
}


inline string::~string(){DPZoneScoped;
	if(!allocator) allocator = DESHI_STRING_ALLOCATOR;
	allocator->release(str);
}

////////////////////
//// @operators ////
////////////////////

inline string::CHAR& string::operator[](u32 idx){DPZoneScoped;
	Assert(idx < count+1);
	return str[idx];
}


inline string::CHAR string::operator[](u32 idx) const {DPZoneScoped;
	Assert(idx < count + 1);
	return str[idx];
}


inline void string::operator= (const CHAR* s){DPZoneScoped;
	if(!allocator) allocator = DESHI_STRING_ALLOCATOR;
	allocator->release(str);
	
	count  = strlen(s);
	space = RoundUpTo(count+1, 4);
	str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
	allocator->commit(str, space*CHAR_SIZE);
	memcpy(str, s, count*CHAR_SIZE);
}


inline void string::operator= (const string& s){DPZoneScoped;
	if(!allocator) allocator = DESHI_STRING_ALLOCATOR;
	allocator->release(str);
	allocator = s.allocator;
	
	count = s.count;
	space = RoundUpTo(count+1, 4);
	str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
	allocator->commit(str, space*CHAR_SIZE); 
	memcpy(str, s.str, count*CHAR_SIZE);
}


inline void string::operator+=(const CHAR* s){DPZoneScoped;
	u32 old_len = count;
	u32 str_len = strlen(s);
	if(str_len == 0) return;
	count += str_len;
	
	if(space == 0){
		space = RoundUpTo(count+1, 4);
		str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
		allocator->commit(str, space*CHAR_SIZE);
		memcpy(str, s, count*CHAR_SIZE);
	}else if(space < count+1){
		space = RoundUpTo(count+1, 4);
		str = (CHAR*)allocator->resize(str, space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
		memcpy(str+old_len, s, (str_len+1)*CHAR_SIZE);
	}else{
		memcpy(str+old_len, s, (str_len+1)*CHAR_SIZE);
	}
}


inline void string::operator+=(const string& s){DPZoneScoped;
	u32 old_len = count;
	u32 str_len = s.count;
	if(str_len == 0) return;
	count += str_len;
	
	if(space == 0){
		space = RoundUpTo(count+1, 4);
		str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
		allocator->commit(str, space*CHAR_SIZE);
		memcpy(str, s.str, count*CHAR_SIZE);
	}else if(space < count+1){
		space = RoundUpTo(count+1, 4);
		str = (CHAR*)allocator->resize(str, space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
		memcpy(str+old_len, s.str, (str_len+1)*CHAR_SIZE);
	}else{
		memcpy(str+old_len, s.str, (str_len+1)*CHAR_SIZE);
	}
}


inline string string::operator--(int){DPZoneScoped;
	if(count == 0) return *this;
	str[--count] = '\0';
	return *this;
}


inline string string::operator+ (const CHAR* c) const{DPZoneScoped;
	if(count == 0) return string(c);
	u32 str_len = strlen(c);
	if(str_len == 0) return *this;
	
	string result;
	result.count  = count + str_len;
	result.space = RoundUpTo(result.count+1, 4);
	result.str = (CHAR*)allocator->reserve(result.space*CHAR_SIZE); Assert(result.str, "Failed to allocate memory");
	allocator->commit(result.str, result.space*CHAR_SIZE);
	memcpy(result.str,       str, count*CHAR_SIZE);
	memcpy(result.str+count, c,   str_len*CHAR_SIZE);
	return result;
}


inline string string::operator+(const string& s) const{DPZoneScoped;
	if(s.count == 0) return *this;
	
	string result;
	result.count  = count + s.count;
	result.space = RoundUpTo(result.count+1, 4);
	result.str = (CHAR*)allocator->reserve(result.space*CHAR_SIZE); Assert(result.str, "Failed to allocate memory");
	allocator->commit(result.str, result.space*CHAR_SIZE);
	memcpy(result.str,       str,   count*CHAR_SIZE);
	memcpy(result.str+count, s.str, s.count*CHAR_SIZE);
	return result;
}


inline bool string::operator==(const string& s) const{DPZoneScoped;
	return !strcmp(str, s.str);
}


inline bool string::operator==(const CHAR* s) const{DPZoneScoped;
	return !strcmp(str, s);
}


inline bool string::operator!=(const string& s) const {DPZoneScoped;
	return strcmp(str, s.str);
}


inline bool string::operator!=(const CHAR* s) const {DPZoneScoped;
	return strcmp(str, s);
}


////////////////////////////
//// @special operators ////
////////////////////////////

inline std::ostream& operator<<(std::ostream& os, const string& m){DPZoneScoped;
	return os << (m.str ? m.str : "");
}


inline string operator+ (const string::CHAR* c, const string& s){DPZoneScoped;
	return string(c) + s;
}

////////////////////
//// @functions ////
////////////////////

inline void string::reserve(u32 _space){DPZoneScoped;
	if(_space > space){
		space = RoundUpTo(_space+1, 4);
		str = (CHAR*)allocator->resize(str, space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
	}
}


inline void string::clear(){DPZoneScoped;
	if(!allocator) allocator = DESHI_STRING_ALLOCATOR;
	allocator->release(str);
	count = 0;
	space = 0;
	str   = 0;
}


inline void string::erase(u32 idx){DPZoneScoped;
	Assert(idx < count && idx >= 0);
	if (count == 1) clear();
	else memmove(str+idx, str+idx+1, (count-- - idx)*CHAR_SIZE);
}


inline void string::insert(CHAR c, u32 idx){DPZoneScoped;
	Assert(idx <= count);
	count += 1;
	if(space == 0){
		space = 4;
		str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
		allocator->commit(str, space*CHAR_SIZE);
		str[0] = c;
	}else if(space < count+1){
		space = RoundUpTo(count+1, 4);
		str = (CHAR*)allocator->resize(str, space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
		memmove(str+idx+1, str+idx, (count-idx)*CHAR_SIZE);
		str[idx] = c;
	}else{
		memmove(str+idx+1, str+idx, (count-idx)*CHAR_SIZE);
		str[idx] = c;
	}
}

//TODO implement this better

inline void string::replace(CHAR c, const string& with) {DPZoneScoped;
	for (u32 i = 0; i < count; i++) {
		if (str[i] == c) {
			erase(i);
			for (u32 o = with.count; o != 0; o--) {
				insert(with[o - 1], i);
			}
		}
	}
}


inline string::CHAR string::at(u32 idx) const{DPZoneScoped;
	Assert(idx <= count);
	return str[idx];
}


inline string string::substr(u32 start, u32 end) const{DPZoneScoped;
	if(end == npos) end = count-1;
	Assert(start <= count && end <= count && start <= end, "check start/end vars");
	return string(str+start, (end-start)+1);
}


inline u32 string::findFirstStr(const string& s) const{DPZoneScoped;
	for(u32 i = 0; i < count; ++i){
		if(strncmp(str+i, s.str, s.count) == 0) return i;
	}
	return npos;
}


inline u32 string::findFirstChar(CHAR c, u32 offset) const{DPZoneScoped;
	for(u32 i = offset; i < count; ++i){
		if(str[i] == c) return i;
	}
	return npos;
}


inline u32 string::findFirstCharNot(CHAR c, u32 offset) const{DPZoneScoped;
	for(u32 i = 0; i < count; ++i){
		if(str[i] != c) return i;
	}
	return npos;
}


inline u32 string::findLastChar(CHAR c, u32 offset) const{DPZoneScoped;
	Assert(offset < count);
	for(u32 i = (offset != 0 ? offset : count - 1); i != 0; --i){
		if(str[i] == c) return i;
	}
	return npos;
}


inline u32 string::findLastCharNot(CHAR c) const{DPZoneScoped;
	for(u32 i = count-1; i != 0; --i){
		if(str[i] != c) return i;
	}
	return npos;
}


inline u32 string::CHARCount(CHAR c) const{DPZoneScoped;
	u32 sum = 0;
	for(u32 i = 0; i < count; ++i){ if(str[i] == c){ sum++; } }
	return sum;
}



inline string string::substrToChar(CHAR c) const{DPZoneScoped;
	u32 idx = findFirstChar(c);
	return (idx != npos) ? *this : string(str, idx); //!TestMe
}


inline b32 string::beginsWith(const string& s) const{DPZoneScoped;
	if (s.count > count) return false;
	return !memcmp(str, s.str, s.count);
}


inline b32 string::endsWith(const string& s) const{DPZoneScoped;
	if (s.count > count) return false;
	return !memcmp(str + (count - s.count), s.str, s.count);
}


inline b32 string::contains(const string& s) const{DPZoneScoped;
	if (s.count > count) return false;
	for (u32 i = 0; i < count - s.count; i++) {
		if (!memcmp(s.str, str + i, s.count)) return true;
	}
	return false;
}


///////////////////////////
//// @static functions ////
///////////////////////////

inline string string::eatSpacesLeading(const string& text){DPZoneScoped;
	u32 idx = text.findFirstCharNot(' ');
	return (idx != npos) ? text.substr(idx) : string();
}


inline string string::eatSpacesTrailing(const string& text){DPZoneScoped;
	u32 idx = text.findLastCharNot(' ');
	return (idx != npos) ? text.substr(0, idx+1) : string();
}


inline string string::toUpper(const string& in){DPZoneScoped;
	string result = in;
	forI(result.count) if(result.str[i] >= 'a' && result.str[i] <= 'z') result.str[i] -= 32;
	return result;
}


inline string string::toLower(const string& in){DPZoneScoped;
	string result = in;
	forI(result.count) if(result.str[i] >= 'A' && result.str[i] <= 'Z') result.str[i] += 32;
	return result;
}


//WSTRING
//unicode version of string



struct wstring {
	typedef wchar_t CHAR;
	static constexpr u32 npos = -1;
	static constexpr u32 CHAR_SIZE = sizeof(CHAR);
	
	u32   count;
	u32   space;
	CHAR* str;
	
	Allocator* allocator; //TODO(delle) maybe make this a constructor arg like array?
	
	wstring();
	wstring(const CHAR* s);
	wstring(const CHAR* s, u32 count);
	wstring(const wstring& s);
	~wstring();
	
	CHAR& operator[](u32 idx);
	void   operator= (const CHAR* s);
	void   operator= (const wstring& s);
	void   operator+=(const CHAR* s);
	void   operator+=(const wstring& s);
	wstring operator--(int);
	wstring operator+ (const CHAR* c) const;
	wstring operator+ (const wstring& s) const;
	bool   operator==(const wstring& s) const;
	bool   operator==(const CHAR* s) const;
	friend wstring operator+ (const CHAR* c, const wstring& s);
	inline explicit operator bool() { return count; }
	
	void   reserve(u32 _space);
	void   clear();
	void   erase(u32 idx);
	//inserts at specified idx, pushing the CHARacter at idx and all following CHARacters to the right
	void   insert(CHAR c, u32 idx);
	//returns a copy of the CHARacter at idx
	CHAR   at(u32 idx) const;
	//returns a wstring including the start and end CHARacters, end equals the end of the wstring (size-1) if npos
	wstring substr(u32 start, u32 end = npos) const;
	u32    findFirstStr(const wstring& s) const;
	//returns first of CHAR from offset
	u32    findFirstChar(CHAR c, u32 offset = 0) const;
	//returns first of CHAR from offset backwards
	u32    findFirstCharNot(CHAR c, u32 offset = 0) const;
	u32    findLastChar(CHAR c, u32 offset = 0) const;
	u32    findLastCharNot(CHAR c) const;
	//returns how many times a CHAR appears in the wstring
	u32    CHARCount(CHAR c) const;
	//returns a subwstring from the beginning to specifiec CHAR, not including the CHAR
	wstring substrToChar(CHAR c) const;
	
	static wstring eatSpacesLeading(const wstring& s);
	static wstring eatSpacesTrailing(const wstring& s);
	static wstring toUpper(const wstring& s);
	static wstring toLower(const wstring& s);
};

///////////////////////
//// @constructors ////
///////////////////////

inline wstring::wstring(){DPZoneScoped;
	allocator = DESHI_STRING_ALLOCATOR;
	count  = 0;
	space = 0;
	str   = 0;
};


inline wstring::wstring(const CHAR* s){DPZoneScoped;
	allocator = DESHI_STRING_ALLOCATOR;
	count  = wcslen(s);
	space = RoundUpTo(count+1, 4);
	str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
	allocator->commit(str, space*CHAR_SIZE);
	memcpy(str, s, count*CHAR_SIZE);
}


inline wstring::wstring(const CHAR* s, u32 _size){DPZoneScoped;
	allocator = DESHI_STRING_ALLOCATOR;
	count  = _size;
	space = RoundUpTo(count+1, 4);
	str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
	allocator->commit(str, space*CHAR_SIZE);
	memcpy(str, s, count*CHAR_SIZE);
}


inline wstring::wstring(const wstring& s){DPZoneScoped;
	allocator = DESHI_STRING_ALLOCATOR;
	count = s.count;
	space = RoundUpTo(count+1, 4);
	str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
	allocator->commit(str, space*CHAR_SIZE);
	memcpy(str, s.str, count*CHAR_SIZE);
}


inline wstring::~wstring(){DPZoneScoped;
	if(!allocator) allocator = DESHI_STRING_ALLOCATOR;
	allocator->release(str);
}

////////////////////
//// @operators ////
////////////////////

inline wstring::CHAR& wstring::operator[](u32 idx){DPZoneScoped;
	Assert(idx < space+1);
	return str[idx];
}


inline void wstring::operator= (const CHAR* s){DPZoneScoped;
	if(!allocator) allocator = DESHI_STRING_ALLOCATOR;
	allocator->release(str);
	
	count  = wcslen(s);
	space = RoundUpTo(count+1, 4);
	str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
	allocator->commit(str, space*CHAR_SIZE);
	memcpy(str, s, count*CHAR_SIZE);
}


inline void wstring::operator= (const wstring& s){DPZoneScoped;
	if(!allocator) allocator = DESHI_STRING_ALLOCATOR;
	allocator->release(str);
	allocator = s.allocator;
	
	count = s.count;
	space = RoundUpTo(count+1, 4);
	str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
	allocator->commit(str, space*CHAR_SIZE); 
	memcpy(str, s.str, count*CHAR_SIZE);
}


inline void wstring::operator+=(const CHAR* s){DPZoneScoped;
	u32 old_len = count;
	u32 str_len = wcslen(s);
	if(str_len == 0) return;
	count += str_len;
	
	if(space == 0){
		space = RoundUpTo(count+1, 4);
		str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
		allocator->commit(str, space*CHAR_SIZE);
		memcpy(str, s, count*CHAR_SIZE);
	}else if(space < count+1){
		space = RoundUpTo(count+1, 4);
		str = (CHAR*)allocator->resize(str, space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
		memcpy(str+old_len, s, (str_len+1)*CHAR_SIZE);
	}else{
		memcpy(str+old_len, s, (str_len+1)*CHAR_SIZE);
	}
}


inline void wstring::operator+=(const wstring& s){DPZoneScoped;
	u32 old_len = count;
	u32 str_len = s.count;
	if(str_len == 0) return;
	count += str_len;
	
	if(space == 0){
		space = RoundUpTo(count+1, 4);
		str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
		allocator->commit(str, space*CHAR_SIZE);
		memcpy(str, s.str, count*CHAR_SIZE);
	}else if(space < count+1){
		space = RoundUpTo(count+1, 4);
		str = (CHAR*)allocator->resize(str, space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
		memcpy(str+old_len, s.str, (str_len+1)*CHAR_SIZE);
	}else{
		memcpy(str+old_len, s.str, (str_len+1)*CHAR_SIZE);
	}
}


inline wstring wstring::operator--(int){DPZoneScoped;
	if(count == 0) return *this;
	str[--count] = '\0';
	return *this;
}


inline wstring wstring::operator+ (const CHAR* c) const{DPZoneScoped;
	if(count == 0) return wstring(c);
	u32 str_len = wcslen(c);
	if(str_len == 0) return *this;
	
	wstring result;
	result.count  = count + str_len;
	result.space = RoundUpTo(result.count+1, 4);
	result.str = (CHAR*)allocator->reserve(result.space*CHAR_SIZE); Assert(result.str, "Failed to allocate memory");
	allocator->commit(result.str, result.space*CHAR_SIZE);
	memcpy(result.str,       str, count*CHAR_SIZE);
	memcpy(result.str+count, c,   str_len*CHAR_SIZE);
	return result;
}


inline wstring wstring::operator+(const wstring& s) const{DPZoneScoped;
	if(s.count == 0) return *this;
	
	wstring result;
	result.count  = count + s.count;
	result.space = RoundUpTo(result.count+1, 4);
	result.str = (CHAR*)allocator->reserve(result.space*CHAR_SIZE); Assert(result.str, "Failed to allocate memory");
	allocator->commit(result.str, result.space*CHAR_SIZE);
	memcpy(result.str,       str,   count*CHAR_SIZE);
	memcpy(result.str+count, s.str, s.count*CHAR_SIZE);
	return result;
}


inline bool wstring::operator==(const wstring& s) const{DPZoneScoped;
	return wcscmp(str, s.str) == 0;
}


inline bool wstring::operator==(const CHAR* s) const{DPZoneScoped;
	return wcscmp(str, s) == 0;
}

////////////////////////////
//// @special operators ////
////////////////////////////

inline std::ostream& operator<<(std::ostream& os, const wstring& m){DPZoneScoped;
	return os << m.str;
}


inline wstring operator+ (const wstring::CHAR* c, const wstring& s){DPZoneScoped;
	return wstring(c) + s;
}

////////////////////
//// @functions ////
////////////////////

inline void wstring::reserve(u32 _space){DPZoneScoped;
	if(_space > space){
		space = RoundUpTo(_space+1, 4);
		str = (CHAR*)allocator->resize(str, space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
	}
}


inline void wstring::clear(){DPZoneScoped;
	if(!allocator) allocator = DESHI_STRING_ALLOCATOR;
	allocator->release(str);
	count = 0;
	space = 0;
	str   = 0;
}


inline void wstring::erase(u32 idx){DPZoneScoped;
	Assert(idx < count && idx >= 0);
	if (count == 1) memset(str, 0, space);
	else            memmove(str+idx, str+idx+1, (--count)*CHAR_SIZE);
}


inline void wstring::insert(CHAR c, u32 idx){DPZoneScoped;
	Assert(idx <= count);
	count += 1;
	if(space == 0){
		space = 4;
		str = (CHAR*)allocator->reserve(space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
		allocator->commit(str, space*CHAR_SIZE);
		str[0] = c;
	}else if(space < count+1){
		space = RoundUpTo(count+1, 4);
		str = (CHAR*)allocator->resize(str, space*CHAR_SIZE); Assert(str, "Failed to allocate memory");
		memmove(str+idx+1, str+idx, (count-idx)*CHAR_SIZE);
		str[idx] = c;
	}else{
		memmove(str+idx+1, str+idx, (count-idx)*CHAR_SIZE);
		str[idx] = c;
	}
}


inline wstring::CHAR wstring::at(u32 idx) const{DPZoneScoped;
	Assert(idx <= count);
	return str[idx];
}


inline wstring wstring::substr(u32 start, u32 end) const{DPZoneScoped;
	if(end == npos) end = count-1;
	Assert(start <= count && end <= count && start <= end, "check start/end vars");
	return wstring(str+start, (end-start)+1);
}


inline u32 wstring::findFirstStr(const wstring& s) const{DPZoneScoped;
	for(u32 i = 0; i < count; ++i){
		if(wcsncmp(str+i, s.str, s.count) == 0) return i;
	}
	return npos;
}


inline u32 wstring::findFirstChar(CHAR c, u32 offset) const{DPZoneScoped;
	for(u32 i = offset; i < count; ++i){
		if(str[i] == c) return i;
	}
	return npos;
}


inline u32 wstring::findFirstCharNot(CHAR c, u32 offset) const{DPZoneScoped;
	for(u32 i = 0; i < count; ++i){
		if(str[i] != c) return i;
	}
	return npos;
}


inline u32 wstring::findLastChar(CHAR c, u32 offset) const{DPZoneScoped;
	Assert(offset < count);
	for(u32 i = (offset != 0 ? offset : count - 1); i != 0; --i){
		if(str[i] == c) return i;
	}
	return npos;
}


inline u32 wstring::findLastCharNot(CHAR c) const{DPZoneScoped;
	for(u32 i = count-1; i != 0; --i){
		if(str[i] != c) return i;
	}
	return npos;
}


inline u32 wstring::CHARCount(CHAR c) const{DPZoneScoped;
	u32 sum = 0;
	for(u32 i = 0; i < count; ++i){ if(str[i] == c){ sum++; } }
	return sum;
}


inline wstring wstring::substrToChar(CHAR c) const{DPZoneScoped;
	u32 idx = findFirstChar(c);
	return (idx != npos) ? *this : wstring(str, idx); //!TestMe
}

///////////////////////////
//// @static functions ////
///////////////////////////

inline wstring wstring::eatSpacesLeading(const wstring& text){DPZoneScoped;
	u32 idx = text.findFirstCharNot(' ');
	return (idx != npos) ? text.substr(idx) : wstring();
}


inline wstring wstring::eatSpacesTrailing(const wstring& text){DPZoneScoped;
	u32 idx = text.findLastCharNot(' ');
	return (idx != npos) ? text.substr(0, idx+1) : wstring();
}


inline wstring wstring::toUpper(const wstring& in){DPZoneScoped;
	wstring result = in;
	forI(result.count) if(result.str[i] >= 'a' && result.str[i] <= 'z') result.str[i] -= 32;
	return result;
}


inline wstring wstring::toLower(const wstring& in){DPZoneScoped;
	wstring result = in;
	forI(result.count) if(result.str[i] >= 'A' && result.str[i] <= 'Z') result.str[i] += 32;
	return result;
}


#endif //DESHI_wstring_H