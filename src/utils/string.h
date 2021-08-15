#pragma once
#ifndef DESHI_STRING_H
#define DESHI_STRING_H

#include "tuple.h"
#include "utils.h"
#include "../defines.h"

#include <cstring>
#include <cstdio>
#include <ostream> //std::ostream operator<<

//NOTE i store the allocated number of characters in the first 4 bytes of the buffer when buffer is not being used
//NOTE DEFAULTS: growth rate: 2x; min allocation: 20 bytes; struct size: 32 bytes
struct string{
    typedef char CHAR;
	static constexpr u32 NPOS = -1;
    static constexpr u32 BUFFER_SIZE = 20; //number of characters, not bytes
    static constexpr u32 CHAR_SIZE = sizeof(CHAR);
    
	CHAR* str;
    u32   size;
    CHAR  buffer[BUFFER_SIZE];
	
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
    size = 0;
    str = buffer;
    memset(buffer, 0, BUFFER_SIZE*CHAR_SIZE);
};

inline string::string(char c){
    size = 1;
    str = buffer;
    memset(buffer, 0, BUFFER_SIZE*CHAR_SIZE);
    str[0] = c;
}

inline string::string(const char* s){
    size = strlen(s);
    if(size > BUFFER_SIZE-1){
        u32 alloc_size = RoundUpTo(size+1, BUFFER_SIZE);
        Assert(str = (CHAR*)calloc(alloc_size, CHAR_SIZE));
        memcpy(str, s, size*CHAR_SIZE);
        *(u32*)buffer = alloc_size;
    }else{
        str = buffer;
        memset(buffer, 0, BUFFER_SIZE*CHAR_SIZE);
        memcpy(buffer, s, size*CHAR_SIZE);
    }
}

inline string::string(const char* s, u32 _size){
    size = _size;
    if(size > BUFFER_SIZE-1){
        u32 alloc_size = RoundUpTo(size+1, BUFFER_SIZE);
        Assert(str = (CHAR*)calloc(alloc_size, CHAR_SIZE));
        memcpy(str, s, size*CHAR_SIZE);
        *(u32*)buffer = alloc_size;
    }else{
        str = buffer;
        memset(buffer, 0, BUFFER_SIZE*CHAR_SIZE);
        memcpy(buffer, s, size*CHAR_SIZE);
    }
}

inline string::string(const string& s){
    size = s.size;
    if(size > BUFFER_SIZE-1){
        Assert(str = (CHAR*)malloc((size+1)*CHAR_SIZE));
        memcpy(str, s.str, (size+1)*CHAR_SIZE);
    }else{
        str = buffer;
        memset(buffer, 0, BUFFER_SIZE*CHAR_SIZE);
        memcpy(buffer, s.str, (size+1)*CHAR_SIZE);
    }
}

//persist set<char*> test_string();
inline string::~string(){
    printf("deleted: %p    %s\n", str, str);
    if(str != buffer) free(str);
}

////////////////////
//// @operators ////
////////////////////
inline string::CHAR& string::operator[](u32 idx){
    Assert(idx < size+1);
    return str[idx];
}

inline void string::operator= (CHAR c){
    if(str != buffer) free(str);
    size = 1;
    str = buffer;
    memset(buffer, 0, BUFFER_SIZE*CHAR_SIZE);
    str[0] = c;
}

inline void string::operator= (const char* s){
    if(str != buffer) free(str);
    size = strlen(s);
    if(size > BUFFER_SIZE-1){
        u32 alloc_size = RoundUpTo(size+1, BUFFER_SIZE);
        Assert(str = (CHAR*)calloc(alloc_size, CHAR_SIZE));
        memcpy(str, s, size*CHAR_SIZE);
        *(u32*)buffer = alloc_size;
    }else{
        str = buffer;
        memset(buffer, 0, BUFFER_SIZE*CHAR_SIZE);
        memcpy(buffer, s, size*CHAR_SIZE);
    }
}

inline void string::operator= (const string& s){
    if(str != buffer) free(str);
    size = s.size;
    if(size > BUFFER_SIZE-1){
        Assert(str = (CHAR*)malloc((size+1)*CHAR_SIZE));
        memcpy(str, s.str, (size+1)*CHAR_SIZE);
    }else{
        str = buffer;
        memset(buffer, 0, BUFFER_SIZE*CHAR_SIZE);
        memcpy(buffer, s.str, (size+1)*CHAR_SIZE);
    }
}

inline void string::operator+=(CHAR c){
    size += 1;
    if(str == buffer){
        if(size > BUFFER_SIZE-1){
            u32 alloc_size = RoundUpTo(size+1, BUFFER_SIZE); 
            Assert(str = (CHAR*)calloc(alloc_size, CHAR_SIZE));
            memcpy(str, buffer, size*CHAR_SIZE);
            str[size-1] = c;
            *(u32*)buffer = alloc_size;
        }else{
            str[size-1] = c;
        }
    }else{
        if(size+1 > *(u32*)buffer){
            u32 alloc_size = RoundUpTo((*(u32*)buffer)+1, BUFFER_SIZE); 
            Assert(str = (CHAR*)realloc(str, alloc_size*CHAR_SIZE));
            str[size-1] = c;
            str[size] = '\0';
            *(u32*)buffer = alloc_size;
        }else{
            str[size-1] = c;
            str[size] = '\0';
        }
    }
}

inline void string::operator+=(const char* s){
    u32 old_len = size;
    u32 str_len = strlen(s);
    if(str_len == 0) return;
    
    size += str_len;
    if(str == buffer){
        if(size > BUFFER_SIZE-1){
            u32 alloc_size = RoundUpTo(size+1, BUFFER_SIZE); 
            Assert(str = (CHAR*)calloc(alloc_size, CHAR_SIZE));
            memcpy(str,         buffer, size*CHAR_SIZE);
            memcpy(str+old_len, s,      str_len*CHAR_SIZE);
            *(u32*)buffer = alloc_size;
        }else{
            memcpy(str+old_len, s, (str_len+1)*CHAR_SIZE);
        }
    }else{
        if(size+1 > *(u32*)buffer){
            u32 alloc_size = RoundUpTo((*(u32*)buffer)*2, BUFFER_SIZE); 
            Assert(str = (CHAR*)realloc(str, alloc_size*CHAR_SIZE));
            memcpy(str+old_len, s, str_len*CHAR_SIZE);
            *(u32*)buffer = alloc_size;
        }else{
            memcpy(str+old_len, s, (str_len+1)*CHAR_SIZE);
        }
    }
}

inline void string::operator+=(const string& s){
    u32 old_len = size;
    u32 str_len = s.size;
    if(str_len == 0) return;
    
    size += str_len;
    if(str == buffer){
        if(size > BUFFER_SIZE-1){
            u32 alloc_size = RoundUpTo(size+1, BUFFER_SIZE); 
            Assert(str = (CHAR*)calloc(alloc_size, CHAR_SIZE), "calloc returned nullptr; maybe we ran out of memory?");
            memcpy(str,         buffer, size*CHAR_SIZE);
            memcpy(str+old_len, s.str,  str_len*CHAR_SIZE);
            *(u32*)buffer = alloc_size;
        }else{
            memcpy(str+old_len, s.str, (str_len+1)*CHAR_SIZE);
        }
    }else{
        if(size+1 > *(u32*)buffer){
            u32 alloc_size = RoundUpTo((*(u32*)buffer)*2, BUFFER_SIZE); 
            Assert(str = (CHAR*)realloc(str, alloc_size*CHAR_SIZE), "realloc returned nullptr; maybe we ran out of memory?");
            memcpy(str+old_len, s.str, str_len*CHAR_SIZE);
            *(u32*)buffer = alloc_size;
        }else{
            memcpy(str+old_len, s.str, (str_len+1)*CHAR_SIZE);
        }
    }
}

inline string string::operator--(int){ //!TestMe
    if(size == 0) return *this;
    str[--size] = '\0';
    return *this;
}

inline string string::operator+ (const char* c) const{
    u32 str_len = strlen(c);
    if(str_len == 0) return *this; //!TestMe
    
    string result{};
    result.size = size + str_len;
    if(result.size > BUFFER_SIZE-1){
        u32 alloc_size = RoundUpTo(result.size+1, BUFFER_SIZE);
        Assert(result.str = (CHAR*)calloc(alloc_size, CHAR_SIZE));
        memcpy(result.str,      str, size*CHAR_SIZE);
        memcpy(result.str+size, c,   str_len*CHAR_SIZE);
        *(u32*)result.buffer = alloc_size;
    }else{
        result.str = result.buffer;
        memcpy(result.buffer,      str, size*CHAR_SIZE);
        memcpy(result.buffer+size, c,   str_len*CHAR_SIZE);
    }
    return result;
}

inline string string::operator+(const string& s) const{
    if(s.size == 0) return *this; //!TestMe
    
    string result{};
    result.size = size + s.size;
    if(result.size > BUFFER_SIZE-1){
        u32 alloc_size = RoundUpTo(result.size+1, BUFFER_SIZE);
        Assert(result.str = (CHAR*)calloc(alloc_size, CHAR_SIZE));
        memcpy(result.str,      str,   size*CHAR_SIZE);
        memcpy(result.str+size, s.str, s.size*CHAR_SIZE);
        *(u32*)result.buffer = alloc_size;
    }else{
        result.str = result.buffer;
        memcpy(result.buffer,      str,   size*CHAR_SIZE);
        memcpy(result.buffer+size, s.str, s.size*CHAR_SIZE);
    }
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
    if(str != buffer) free(str);
    size = 0;
    str = buffer;
    memset(buffer, 0, BUFFER_SIZE*CHAR_SIZE);
}

inline void string::erase(u32 idx){
    Assert(idx < size && idx >= 0);
    memmove(str+idx, str+idx+1, (--size)*CHAR_SIZE);
}

inline void string::insert(char c, u32 idx){
    Assert(idx <= size);
    size += 1;
    if(str == buffer){
        if(size > BUFFER_SIZE-1){
            u32 alloc_size = RoundUpTo(size+1, BUFFER_SIZE); 
            str = (CHAR*)calloc(alloc_size, CHAR_SIZE);
            Assert(str, "calloc failed and returned nullptr; maybe we ran out of memory?");
            memcpy(str, buffer, idx*CHAR_SIZE);
            memcpy(str+idx+1, buffer+idx, (size-idx)*CHAR_SIZE);
            str[idx] = c;
            *(u32*)buffer = alloc_size;
        }else{
            memmove(str+idx+1, str+idx, (size-idx)*CHAR_SIZE);
            str[idx] = c;
        }
    }else{
        if(size+1 > *(u32*)buffer){
            u32 alloc_size = RoundUpTo((*(u32*)buffer)+1, BUFFER_SIZE); 
            str = (CHAR*)realloc(str, alloc_size*CHAR_SIZE);
            Assert(str, "realloc failed and returned nullptr; maybe we ran out of memory?");
            memmove(str+idx+1, str+idx, (size-idx)*CHAR_SIZE);
            str[idx] = c;
            *(u32*)buffer = alloc_size;
        }else{
            memmove(str+idx+1, str+idx, (size-idx)*CHAR_SIZE);
            str[idx] = c;
        }
    }
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