#pragma once
#ifndef DESHI_STRING_H
#define DESHI_STRING_H

#include "tuple.h"
#include "utils.h"
#include "../defines.h"

#include <cstring>
#include <cstdio>
#include <iostream> //std::ostream operator<<

struct string{
	typedef char CHAR;
	static constexpr u32 npos = -1;
	static constexpr u32 CHAR_SIZE = sizeof(CHAR);
	
	u32   count;
	u32   space;
	CHAR* str;
	
	string();
	string(const char* s);
	string(const char* s, u32 count);
	string(const string& s);
	~string();
	
	CHAR&  operator[](u32 idx);
	void   operator= (const char* s);
	void   operator= (const string& s);
	void   operator+=(const char* s);
	void   operator+=(const string& s);
	string operator--(int);
	string operator+ (const char* c) const;
	string operator+ (const string& s) const;
	bool   operator==(const string& s) const;
	bool   operator==(const char* s) const;
	friend string operator+ (const char* c, const string& s);
	
	void   reserve(u32 _space);
	void   clear();
	void   erase(u32 idx);
	//inserts at specified idx, pushing the character at idx and all following characters to the right
	void   insert(CHAR c, u32 idx);
	//returns a copy of the character at idx
	CHAR   at(u32 idx) const;
	//returns a string including the start and end characters, end equals the end of the string (size-1) if npos
	string substr(u32 start, u32 end = npos) const;
	u32    findFirstStr(const string& s) const;
	//returns first of char from offset
	u32    findFirstChar(CHAR c, u32 offset = 0) const;
	//returns first of char from offset backwards
	u32    findFirstCharNot(CHAR c, u32 offset = 0) const;
	u32    findLastChar(CHAR c, u32 offset = 0) const;
	u32    findLastCharNot(CHAR c) const;
	//returns how many times a char appears in the string
	u32    charCount(CHAR c) const;
	//returns a substring from the beginning to specifiec char, not including the char
	string substrToChar(CHAR c) const;
	
	static string eatSpacesLeading(const string& s);
	static string eatSpacesTrailing(const string& s);
	static string toUpper(const string& s);
	static string toLower(const string& s);
};

///////////////////////
//// @constructors ////
///////////////////////
inline string::string(){
	count  = 0;
	space = 0;
	str   = 0;
};

inline string::string(const char* s){
	count  = strlen(s);
	space = RoundUpTo(count+1, 4);
	Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
	memcpy(str, s, count*CHAR_SIZE);
}

inline string::string(const char* s, u32 _size){
	count  = _size;
	space = RoundUpTo(count+1, 4);
	Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
	memcpy(str, s, count*CHAR_SIZE);
}

inline string::string(const string& s){
	count = s.count;
	space = RoundUpTo(count+1, 4);
	Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
	memcpy(str, s.str, count*CHAR_SIZE);
}

inline string::~string(){
	free(str);
}

////////////////////
//// @operators ////
////////////////////
inline string::CHAR& string::operator[](u32 idx){
	Assert(idx < count+1);
	return str[idx];
}

inline void string::operator= (const char* s){
	free(str);
	count  = strlen(s);
	space = RoundUpTo(count+1, 4);
	Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
	memcpy(str, s, count*CHAR_SIZE);
}

inline void string::operator= (const string& s){
	free(str);
	count = s.count;
	space = RoundUpTo(count+1, 4);
	Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
	memcpy(str, s.str, count*CHAR_SIZE);
}

inline void string::operator+=(const char* s){
	u32 old_len = count;
	u32 str_len = strlen(s);
	if(str_len == 0) return;
	count += str_len;
	
	if(space == 0){
		space = RoundUpTo(count+1, 4);
		Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
		memcpy(str, s, count*CHAR_SIZE);
	}else if(space < count+1){
		space = RoundUpTo(count+1, 4);
		Assert(str = (CHAR*)realloc(str, space*CHAR_SIZE));
		memcpy(str+old_len, s, (str_len+1)*CHAR_SIZE);
	}else{
		memcpy(str+old_len, s, (str_len+1)*CHAR_SIZE);
	}
}

inline void string::operator+=(const string& s){
	u32 old_len = count;
	u32 str_len = s.count;
	if(str_len == 0) return;
	count += str_len;
	
	if(space == 0){
		space = RoundUpTo(count+1, 4);
		Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
		memcpy(str, s.str, count*CHAR_SIZE);
	}else if(space < count+1){
		space = RoundUpTo(count+1, 4);
		Assert(str = (CHAR*)realloc(str, space*CHAR_SIZE));
		memcpy(str+old_len, s.str, (str_len+1)*CHAR_SIZE);
	}else{
		memcpy(str+old_len, s.str, (str_len+1)*CHAR_SIZE);
	}
}

inline string string::operator--(int){
	if(count == 0) return *this;
	str[--count] = '\0';
	return *this;
}

inline string string::operator+ (const char* c) const{
	if(count == 0) return string(c);
	u32 str_len = strlen(c);
	if(str_len == 0) return *this;
	
	string result;
	result.count  = count + str_len;
	result.space = RoundUpTo(result.count+1, 4);
	Assert(result.str = (CHAR*)calloc(result.space, CHAR_SIZE));
	memcpy(result.str,      str, count*CHAR_SIZE);
	memcpy(result.str+count, c,   str_len*CHAR_SIZE);
	return result;
}

inline string string::operator+(const string& s) const{
	if(s.count == 0) return *this;
	
	string result;
	result.count  = count + s.count;
	result.space = RoundUpTo(result.count+1, 4);
	Assert(result.str = (CHAR*)calloc(result.space, CHAR_SIZE));
	memcpy(result.str,      str,   count*CHAR_SIZE);
	memcpy(result.str+count, s.str, s.count*CHAR_SIZE);
	return result;
}

inline bool string::operator==(const string& s) const{
	return strcmp(str, s.str) == 0;
}

inline bool string::operator==(const char* s) const{
	return strcmp(str, s) == 0;
}

////////////////////////////
//// @special operators ////
////////////////////////////
inline std::ostream& operator<<(std::ostream& os, const string& m){
	return os << m.str;
}

inline string operator+ (const char* c, const string& s){
	return string(c) + s;
}

////////////////////
//// @functions ////
////////////////////
inline void string::reserve(u32 _space){
	if(_space > space){
		space = RoundUpTo(_space+1, 4);
		Assert(str = (CHAR*)realloc(str, space*CHAR_SIZE));
	}
}

inline void string::clear(){
	free(str);
	count  = 0;
	space = 0;
	str   = 0;
}

inline void string::erase(u32 idx){
	Assert(idx < count && idx >= 0);
	if (count == 1) memset(str, 0, space);
	else           memmove(str+idx, str+idx+1, (--count)*CHAR_SIZE);
}

inline void string::insert(char c, u32 idx){
	Assert(idx <= count);
	count += 1;
	if(space == 0){
		space = 4;
		Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
		str[0] = c;
	}else if(space < count+1){
		space = RoundUpTo(count+1, 4);
		Assert(str = (CHAR*)realloc(str, space*CHAR_SIZE));
		memmove(str+idx+1, str+idx, (count-idx)*CHAR_SIZE);
		str[idx] = c;
	}else{
		memmove(str+idx+1, str+idx, (count-idx)*CHAR_SIZE);
		str[idx] = c;
	}
}

inline char string::at(u32 idx) const{
	Assert(idx <= count);
	return str[idx];
}

inline string string::substr(u32 start, u32 end) const{
	if(end == npos) end = count-1;
	Assert(start <= count && end <= count && start <= end, "check start/end vars");
	return string(str+start, (end-start)+1);
}

inline u32 string::findFirstStr(const string& s) const{
	for(u32 i = 0; i < count; ++i){
		if(strncmp(str+i, s.str, s.count) == 0) return i;
	}
	return npos;
}

inline u32 string::findFirstChar(char c, u32 offset) const{
	for(u32 i = offset; i < count; ++i){
		if(str[i] == c) return i;
	}
	return npos;
}

inline u32 string::findFirstCharNot(char c, u32 offset) const{
	for(u32 i = 0; i < count; ++i){
		if(str[i] != c) return i;
	}
	return npos;
}

inline u32 string::findLastChar(char c, u32 offset) const{
	Assert(offset < count);
	for(u32 i = (offset != 0 ? offset : count - 1); i != 0; --i){
		if(str[i] == c) return i;
	}
	return npos;
}

inline u32 string::findLastCharNot(char c) const{
	for(u32 i = count-1; i != 0; --i){
		if(str[i] != c) return i;
	}
	return npos;
}

inline u32 string::charCount(char c) const{
	u32 sum = 0;
	for(u32 i = 0; i < count; ++i){ if(str[i] == c){ sum++; } }
	return sum;
}

inline string string::substrToChar(char c) const{
	u32 idx = findFirstChar(c);
	return (idx != npos) ? *this : string(str, idx); //!TestMe
}

///////////////////////////
//// @static functions ////
///////////////////////////
inline string string::eatSpacesLeading(const string& text){
	u32 idx = text.findFirstCharNot(' ');
	return (idx != npos) ? text.substr(idx) : string();
}

inline string string::eatSpacesTrailing(const string& text){
	u32 idx = text.findLastCharNot(' ');
	return (idx != npos) ? text.substr(0, idx+1) : string();
}

inline string string::toUpper(const string& in){
	string result = in;
	forI(result.count) if(result.str[i] >= 'a' && result.str[i] <= 'z') result.str[i] -= 32;
	return result;
}

inline string string::toLower(const string& in){
	string result = in;
	forI(result.count) if(result.str[i] >= 'A' && result.str[i] <= 'Z') result.str[i] += 32;
	return result;
}

#endif //DESHI_STRING_H