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
	static constexpr u32 NPOS = -1;
    static constexpr u32 CHAR_SIZE = sizeof(CHAR);
    
    u32   size;
    u32   space;
	CHAR* str;
	
    string();
    string(char c);
    string(const char* s);
    string(const char* s, u32 size);
    string(const string& s);
    ~string();
    
    CHAR&  operator[](u32 idx);
    void   operator= (CHAR c);
    void   operator= (const char* s);
    void   operator= (const string& s);
    void   operator+=(CHAR c);
    void   operator+=(const char* s);
    void   operator+=(const string& s);
    string operator--(int);
    string operator+ (const char* c) const;
    string operator+ (const string& s) const;
    bool   operator==(const string& s) const;
    bool   operator==(const char* s) const;
    bool   operator==(CHAR c) const;
    friend string operator+ (const char* c, const string& s);
    
    void   clear();
    void   erase(u32 idx);
    //inserts at specified idx, pushing the character at idx and all following characters to the right
	void   insert(CHAR c, u32 idx);
    //returns a copy of the character at idx
    CHAR   at(u32 idx) const;
    //returns a string including the start and end characters, end equals the end of the string (size-1) if NPOS
    string substr(u32 start, u32 end = NPOS) const;
    u32    findFirstStr(const string& s) const;
    //returns first of char from offset
    u32    findFirstChar(CHAR c, u32 offset = 0) const;
    //returns first of char from offset backwards
    u32    findFirstCharNot(CHAR c) const;
    u32    findLastChar(CHAR c, u32 offset = 0) const;
    u32    findLastCharNot(CHAR c) const;
    //returns how many times a char appears in the string
    u32    charCount(CHAR c) const;
    //returns a substring from the beginning to specifiec char, not including the char
    string substrToChar(CHAR c) const;
    
    static string eatSpacesLeading(const string& s);
    static string eatSpacesTrailing(const string& s);
};

///////////////////////
//// @constructors ////
///////////////////////
inline string::string(){
    size  = 0;
    space = 0;
    str   = 0;
};

inline string::string(char c){
    size  = 1;
    space = 4;
    Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
    str[0] = c;
}

inline string::string(const char* s){
    size  = strlen(s);
    space = RoundUpTo(size+1, 4);
    Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
    memcpy(str, s, size*CHAR_SIZE);
}

inline string::string(const char* s, u32 _size){
    size  = _size;
    space = RoundUpTo(size+1, 4);
    Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
    memcpy(str, s, size*CHAR_SIZE);
}

inline string::string(const string& s){
    size = s.size;
    space = RoundUpTo(size+1, 4);
    Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
    memcpy(str, s.str, size*CHAR_SIZE);
}

inline string::~string(){
    free(str);
}

////////////////////
//// @operators ////
////////////////////
inline string::CHAR& string::operator[](u32 idx){
    Assert(idx < size+1);
    return str[idx];
}

inline void string::operator= (CHAR c){
    free(str);
    size  = 1;
    space = 4;
    Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
    str[0] = c;
}

inline void string::operator= (const char* s){
    free(str);
    size  = strlen(s);
    space = RoundUpTo(size+1, 4);
    Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
    memcpy(str, s, size*CHAR_SIZE);
}

inline void string::operator= (const string& s){
    free(str);
    size = s.size;
    space = RoundUpTo(size+1, 4);
    Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
    memcpy(str, s.str, size*CHAR_SIZE);
}

inline void string::operator+=(CHAR c){
    size += 1;
    if(space == 0){
        space = 4;
        Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
        str[0] = c;
        return;
    }else if(space < size+1){
        space = RoundUpTo(space*2, 4);
        Assert(str = (CHAR*)realloc(str, space*CHAR_SIZE));
        str[size-1] = c;
        str[size] = '\0';
    }else{
        str[size-1] = c;
        str[size] = '\0';
    }
}

inline void string::operator+=(const char* s){
    u32 old_len = size;
    u32 str_len = strlen(s);
    if(str_len == 0) return;
    size += str_len;
    
    if(space == 0){
        space = RoundUpTo(size+1, 4);
        Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
        memcpy(str, s, size*CHAR_SIZE);
    }else if(space < size+1){
        space = RoundUpTo(size+1, 4);
        Assert(str = (CHAR*)realloc(str, space*CHAR_SIZE));
        memcpy(str+old_len, s, str_len*CHAR_SIZE);
    }else{
        memcpy(str+old_len, s, (str_len+1)*CHAR_SIZE);
    }
}

inline void string::operator+=(const string& s){
    u32 old_len = size;
    u32 str_len = s.size;
    if(str_len == 0) return;
    size += str_len;
    
    if(space == 0){
        space = RoundUpTo(size+1, 4);
        Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
        memcpy(str, s.str, size*CHAR_SIZE);
    }else if(space < size+1){
        space = RoundUpTo(size+1, 4);
        Assert(str = (CHAR*)realloc(str, space*CHAR_SIZE));
        memcpy(str+old_len, s.str, str_len*CHAR_SIZE);
    }else{
        memcpy(str+old_len, s.str, (str_len+1)*CHAR_SIZE);
    }
}

inline string string::operator--(int){
    if(size == 0) return *this;
    str[--size] = '\0';
    return *this;
}

inline string string::operator+ (const char* c) const{
    if(size == 0) return string(c);
    u32 str_len = strlen(c);
    if(str_len == 0) return *this;
    
    string result;
    result.size  = size + str_len;
    result.space = RoundUpTo(result.size+1, 4);
    Assert(result.str = (CHAR*)calloc(result.space, CHAR_SIZE));
    memcpy(result.str,      str, size*CHAR_SIZE);
    memcpy(result.str+size, c,   str_len*CHAR_SIZE);
    return result;
}

inline string string::operator+(const string& s) const{
    if(s.size == 0) return *this;
    
    string result;
    result.size  = size + s.size;
    result.space = RoundUpTo(result.size+1, 4);
    Assert(result.str = (CHAR*)calloc(result.space, CHAR_SIZE));
    memcpy(result.str,      str,   size*CHAR_SIZE);
    memcpy(result.str+size, s.str, s.size*CHAR_SIZE);
    return result;
}

inline bool string::operator==(const string& s) const{
    return strcmp(str, s.str) == 0;
}

inline bool string::operator==(const char* s) const{
    return strcmp(str, s) == 0;
}

inline bool string::operator==(CHAR c) const{
    return ((size == 1) && (*str == c));
}

////////////////////////////
//// @special operators ////
////////////////////////////
inline std::ostream& operator<<(std::ostream& os, const string& m){
    return os << m.str;
}

inline string operator+ (const char* c, const string& s){
    return s + c;
}

////////////////////
//// @functions ////
////////////////////
inline void string::clear(){
    free(str);
    size  = 0;
    space = 0;
    str   = 0;
}

inline void string::erase(u32 idx){
    Assert(idx < size && idx >= 0);
    memmove(str+idx, str+idx+1, (--size)*CHAR_SIZE);
}

inline void string::insert(char c, u32 idx){
    Assert(idx <= size);
    size += 1;
    if(space == 0){
        space = 4;
        Assert(str = (CHAR*)calloc(space, CHAR_SIZE));
        str[0] = c;
    }else if(space < size+1){
        space = RoundUpTo(size+1, 4);
        Assert(str = (CHAR*)realloc(str, space*CHAR_SIZE));
    }
    memmove(str + idx + 1, str + idx, (size - idx) * CHAR_SIZE);
    str[idx] = c;
}

inline char string::at(u32 idx) const{
    Assert(idx <= size);
    return str[idx];
}

inline string string::substr(u32 start, u32 end) const{
    if(end == NPOS) end = size-1;
    Assert(start <= size && end <= size && start <= end, "check start/end vars");
    return string(str+start, (end-start)+1);
}

inline u32 string::findFirstStr(const string& s) const{
    for(u32 i = 0; i < size; ++i){
        if(strncmp(str+i, s.str, s.size) == 0) return i;
    }
    return NPOS;
}

inline u32 string::findFirstChar(char c, u32 offset) const{
    for(u32 i = offset; i < size; ++i){
        if(str[i] == c) return i;
    }
    return NPOS;
}

inline u32 string::findFirstCharNot(char c) const{
    for(u32 i = 0; i < size; i++){
        if(str[i] != c) return i;
    }
    return NPOS;
}

inline u32 string::findLastChar(char c, u32 offset) const{
    Assert(offset < size);
    for(u32 i = (size-1)-offset; i != 0; --i){
        if(str[i] == c) return i;
    }
    return NPOS;
}

inline u32 string::findLastCharNot(char c) const{
    for(u32 i = size-1; i != 0; --i){
        if(str[i] != c) return i;
    }
    return NPOS;
}

inline u32 string::charCount(char c) const{
    u32 sum = 0;
    for(u32 i = 0; i < size; ++i){ if(str[i] == c){ sum++; } }
    return sum;
}

inline string string::substrToChar(char c) const{
    u32 idx = findFirstChar(c);
    return (idx != NPOS) ? *this : string(str, idx); //!TestMe
}

///////////////////////////
//// @static functions ////
///////////////////////////
inline string string::eatSpacesLeading(const string& text){
    u32 idx = text.findFirstCharNot(' ');
    return (idx != NPOS) ? text.substr(idx) : string();
}

inline string string::eatSpacesTrailing(const string& text){
    u32 idx = text.findLastCharNot(' ');
    return (idx != NPOS) ? text.substr(0, idx+1) : string();
}

#endif //DESHI_STRING_H