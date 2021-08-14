#pragma once
#ifndef DESHI_STRING_H
#define DESHI_STRING_H

#include "tuple.h"
#include "../defines.h"

#include <stdlib.h>
#include <iostream>
#include <vector> //temp and should be removed
#include <string> //temp and should be removed

#define __FILENAME__ (std::strrchr(__FILE__, '\\') ? std::strrchr(__FILE__, '\\') + 1 : __FILE__)
#define ADDRUPDATE(str, size, create) 0//addrupdate((void*)str, size, __FILENAME__, __func__, create)

global_ std::vector<pair<u32, void*, std::string>> addrs;
global_ bool track = false;
global_ u32 inscount = 0;

global_ void setTrack(){ track = true; inscount = 0; }
global_ void endTrack(){ track = false; }

global_ void addrupdate(void* addr, u32 size, const char* file, const char* func, bool create = true){
	if(track){
		std::cout << "\033c";
		char* str = (char*)malloc(255);
		strncpy(str, (char*)addr, size);
		str[size] = '\0';
		if(create){
			addrs.push_back(pair<u32, void*, std::string>(inscount, addr, std::string(func)));
			std::cout << "added addr   " << (int)addr << " ~ " << str << std::endl;
		}
		else {
			for(int i = 0; i < addrs.size(); i++){
				if(addrs[i].second == addr){
					std::cout << "erased addr   " << (int)addr << " at idx " << addrs[i].first << " ~ " << str <<  std::endl;
					addrs.erase(addrs.begin() + i);
					break;
				}
			}
		}
		
		std::cout << "CURRENT ADDR LIST WITH " << inscount << " INSTRUCTIONS AND " << addrs.size() << " ADDRS:" << std::endl;
		
		for(int i = 0; i < addrs.size(); i++){
			std::cout << addrs[i].first << ".  " << (int)addrs[i].second << " - " << addrs[i].third << " ~ " << (char*)addrs[i].second << std::endl;
		}
		std::cout << std::endl;
		free(str);
		inscount++;
	}
}

//TODO(delle) add reserved space on string for small strings, offset it to a proper padding
struct string {
    size_t size = 0;
	char*  str = 0;
	
	//returned when something specified is not found in a search fucntion
	static const size_t npos = -1;
	
    string();
    string(char c);
    string(const char* s);
    string(const char* s, size_t size);
    string(const string& s);
	string(std::string s); //temp
    ~string();
    
    char&  operator[](int i);
    void   operator= (char c);
    void   operator= (const string& s);
    void   operator= (const char* s);
    void   operator+=(char& c);
    void   operator+=(const string& s);
    void   operator+=(const char* ss);
    string operator+ (const string& s);
    string operator+ (const char* c);
    string operator--(int);
    bool   operator==(const string& s) const;
    bool   operator==(const char* s) const;
    bool   operator==(char c) const;
    friend string operator+ (const char* c, string s);
    
    void   clear();
    void   erase(u32 idx);
    //inserts after specified idx, so an idx of -1 inserts at the begining
	void   insert(char c, s32 idx);
    
    //returns a copy of the character at idx
	char   at(u32 idx) const;
    string substr(size_t first, size_t second = npos) const;
    size_t find(const string& text) const;
    //find first of character from offset
    size_t find_first_of(char c, int offset = 0) const;
    //find first of from offset backwards
    size_t find_first_of_lookback(char c, int offset = 0) const; //TODO(sushi) make this for the other functions
    size_t find_first_not_of(char c) const;
    size_t find_last_of(char c) const;
    size_t find_last_not_of(char c) const;
    //counts how many times c appears in the string
    size_t count(char c) const;
    string substrToChar(char c) const;
    
    static u64    hash(const string& str);
    static int    stoi(const string& str);
    static string toStr(int i);
    static string toStr(u32 i);
    static string toStr(float f);
    static string toStr(size_t f);
    static string eatSpacesLeading(const string& text);
    static string eatSpacesTrailing(const string& text);
};

///////////////////////
//// @constructors ////
///////////////////////
inline string::string(){
    size = 0;
    ADDRUPDATE(str, 0, 1);
};

inline string::string(char c){
    size = 1;
    str = (char*)malloc(1 + 1);
    str[0] = c;
    str[1] = '\0';
    ADDRUPDATE(str, size, 1);
}

inline string::string(const char* s){
    size = strlen(s);
    if(size != 0){
        str = (char*)malloc(size + 1);
        strcpy(str, s);
        ADDRUPDATE(str, size, 1);
    }
    else {
        //str = new char[1];
        str = (char*)malloc(1);
        memset(str, '\0', 1);
        ADDRUPDATE(str, size, 1);
    }
}

inline string::string(const char* s, size_t size){
    this->size = size;
    if(size != 0){
        str = (char*)malloc(size + 1);
        memcpy(str, s, size);
        memset(str + size, '\0', 1);
        ADDRUPDATE(str, size, 1);
    }
    else {
        str = (char*)malloc(1);
        memset(str, '\0', 1);
        ADDRUPDATE(str, size, 1);
    }
}

inline string::string(const string& s){
    
    size = s.size;
    if(size != 0){
        free(str);
        str = (char*)malloc(size + 1);
        memcpy(str, s.str, s.size+1);
        memset(str + size, '\0', 1);
        ADDRUPDATE(str, size, 1);
    }
    else {
        free(str);
        str = (char*)malloc(1);
        size = 0;
        memset(str, '\0', 1);
        ADDRUPDATE(str, 0, 1);
    }
}

//temp
inline string::string(std::string s){
    free(str);
    size = s.size();
    str = (char*)malloc(size + 1);
    memcpy(str, s.c_str(), size);
    memset(str + size, '\0', 1);
}

inline string::~string(){
    ADDRUPDATE(str, size, 0);
    free(str);
    str = nullptr;
    size = 0;
}

////////////////////
//// @operators ////
////////////////////
inline char& string::operator[](int i){
    Assert(i < size+1);
    return str[i];
}

inline void string::operator= (char c){
    size = 1;
    str = (char*)malloc(size + 1);
    memset(str, c, 2);
    memset(str + 1, '\0', 1);
    ADDRUPDATE(str, size, 1);
}

inline void string::operator= (const string& s){
    ADDRUPDATE(str, size, 0);
    free(str);
    size = s.size;
    str = (char*)malloc(size + 1);
    memcpy(str, s.str, size + 1);
    memset(str + size, '\0', 1);
    ADDRUPDATE(str, size, 1);
}

inline void string::operator= (const char* s){
    size = strlen(s);
    str = (char*)realloc(str, size + 1);
    strcpy(str, s);
    ADDRUPDATE(str, size, 1);
}

//i didnt test this
inline void string::operator+=(char& c){
    string s(c);
    this->operator+=(s);
    
    //int newsize = size + 1;
    //str = (char*)realloc(str, newsize + 1);
    //memcpy(str + size, (char*)c, 1);
    //size = newsize;
    //memset(str + size, '\0', 1);
}

inline void string::operator+=(const string& s){
    if(s.size == 0) return;
    int newsize = size + s.size;
    str = (char*)realloc(str, newsize + 1);
    memcpy(str + size, s.str, s.size);
    size = newsize;
    memset(str + size, '\0', 1);
    ADDRUPDATE(str, size, 1);
}

inline void string::operator+=(const char* ss){
    string s(ss); //being lazy
    this->operator+=(s);
                  
     //if(s.size == 0) return;
    //int newsize = size + s.size;
    //char* old = new char[size];
    //memcpy(old, str, size);
    //str = (char*)malloc(newsize + 1);
    //memcpy(str, old, size);
    //memcpy(str + size, s.str, s.size);
    //size = newsize;
    //memset(str + size, '\0', 1);
    //ADDRUPDATE(str, size, 1);
    //delete old;
}

inline string string::operator+(const string& s){
    if(s.size == 0) return *this;
    int newsize = size + s.size;
    char* old = new char[size];
    memcpy(old, str, size);
    string nustr;
    nustr.str = (char*)malloc(newsize + 1);
    memcpy(nustr.str, old, size);
    memcpy(nustr.str + size, s.str, s.size);
    nustr.size = newsize;
    memset(nustr.str + nustr.size, '\0', 1);
    delete old;
    return nustr;
}

inline string string::operator+ (const char* c){
    string s(c);
    return this->operator+(s);
}

inline string string::operator--(int){
    if(size != 1){
        str = (char*)realloc(str, size - 1);
        size--;
        memset(str + size, '\0', 1);
    }
    else {
        str = (char*)realloc(str, 1);
        size = 0;
        memset(str, '\0', 1);
    }
    return *this;
}

inline bool string::operator==(const string& s) const{
    return !strcmp(str, s.str);
}

inline bool string::operator==(const char* s) const{
    return !strcmp(str, s);
}

inline bool string::operator==(char c) const{
    if(size == 1 && *str == c) return true;
    return false;
}

////////////////////////////
//// @special operators ////
////////////////////////////
inline std::ostream& operator<<(std::ostream& os, const string& m){
    return os << m.str;
}

inline string operator+ (const char* c, string s){
    if(s.size == 0){
        string why_do_i_have_to_do_this(c);
        return why_do_i_have_to_do_this;
    }
    string st(c);
    return st + s;
}

////////////////////
//// @functions ////
////////////////////
inline void string::clear(){
    str = (char*)realloc(str, 1);
    str[0] = '\0';
    size = 0;
}

inline void string::erase(u32 idx){
    Assert(idx <= size && idx >= 0 && size != 0);
    if(size - 1 != 0){
        for(int i = idx; i < size - 1; i++)
            str[i] = str[i + 1];
        size--;
        str = (char*)realloc(str, size + 1);
        memset(str + size, '\0', 1);
    }
    else {
        size = 0;
        str = (char*)realloc(str, 1);
        memset(str, '\0', 1);
    }
}

inline void string::insert(char c, s32 idx){
    Assert(idx <= (s32)size && idx >= -1);
    
    idx++;
    size++;
    str = (char*)realloc(str, size + 1);
    for(s32 i = size - 1; i >= idx; i--)
        str[i] = str[i - 1];
    str[idx] = c;
}

inline char string::at(u32 idx) const{
    Assert(idx < size+1);
    return str[idx];
}

inline string string::substr(size_t first, size_t second) const{
    if(second == npos) second = size - 1;
    Assert(first <= size && second <= size && second >= first, "check first/second variables");
    return string(str + first, second - first + 1);
}

inline size_t string::find(const string& text) const{
    for(int i = 0; i < size - (text.size - 1); i++){
        //dont use strcmp if text.size is only 1
        if(text.size == 1)
            if(str[i] == text.str[0])
            return i;
        
        //early cont if char doesnt match first char of input
        if(str[i] != text.str[0]) continue;
        else if(!strcmp(substr(i, i + text.size - 1).str, text.str)){
            return i;
        }
    }
    return npos;
}

inline size_t string::find_first_of(char c, int offset) const{
    Assert(offset < size, "attempt to parse string at offset greater than size");
    for(int i = offset; i < size; i++){
        if(c == str[i]) return i;
    }
    return npos;
}

inline size_t string::find_first_of_lookback(char c, int offset) const{
    Assert(offset < size, "attempt to parse string at offset greater than size");
    for(int i = offset; i > 0; i--){
        if(c == str[i]) return i;
    }
    return npos;
}

inline size_t string::find_first_not_of(char c) const{
    for(int i = 0; i < size; i++){
        if(c != str[i]) return i;
    }
    return npos;
}

inline size_t string::find_last_of(char c) const{
    for(int i = size - 1; i != 0; i--){
        if(c == str[i]) return i;
    }
    return npos;
}

inline size_t string::find_last_not_of(char c) const{
    for(int i = size - 1; i != 0; i--){
        if(c != str[i]) return i;
    }
    return npos;
}

inline size_t string::count(char c) const{
    size_t sum = 0;
    for(int i = 0; i < size; i++) if(str[i] == c) sum++;
    return sum;
}

inline string string::substrToChar(char c) const{
    size_t first = find_first_of(c);
    if(first == npos){
        return string(str);
    }else{
        return string(str, first);
    }
}

///////////////////////////
//// @static functions ////
///////////////////////////
//https://cp-algorithms.com/string/string-hashing.html
inline u64 string::hash(const string& str){
    const int p = 31;
    const int m = 1e9 + 9;
    u64 hash_value = 0;
    u64 p_pow = 1;
    for(int i = 0; i < str.size; i++){
        hash_value = (hash_value + (str.str[i] - 'a' + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }
    return hash_value;
}

inline int string::stoi(const string& str){
    int x;
    sscanf(str.str, "%d", &x);
    return x;
}

inline string string::toStr(int i){
    string s;
    s.size = (i == 0) ? 1 : (int)((floor(log10(i)) + 1) * sizeof(char));
    free(s.str);
    s.str = (char*)malloc(s.size + 1);
    sprintf(s.str, "%d", i);
    ADDRUPDATE(s.str, s.size, 1);
    return s;
}

inline string string::toStr(u32 i){
    string s;
    s.size = (i == 0) ? 1 : (int)((floor(log10(i)) + 1) * sizeof(char));
    free(s.str);
    s.str = (char*)malloc(s.size + 1);
    sprintf(s.str, "%d", i);
    ADDRUPDATE(s.str, s.size, 1);
    return s;
}

inline string string::toStr(float f){
    string s;
    s.size = snprintf(nullptr, 0, "%f", f);
    s.str = (char*)malloc(s.size + 1);
    snprintf(s.str, s.size + 1, "%f", f);
    ADDRUPDATE(s.str, s.size, 1);
    return s;
}

inline string string::toStr(size_t f){
    string s;
    s.size = snprintf(nullptr, 0, "%zu", f);
    s.str = (char*)malloc(s.size + 1);
    snprintf(s.str, s.size + 1, "%zu", f);
    ADDRUPDATE(s.str, s.size, 1);
    return s;
}

inline string string::eatSpacesLeading(const string& text){
    size_t idx = text.find_first_of(' ');
    return (idx != npos) ? text.substr(idx) : "";
}

inline string string::eatSpacesTrailing(const string& text){
    size_t idx = text.find_last_not_of(' ');
    return (idx != npos) ? text.substr(0, idx + 1) : "";
}

#endif //DESHI_STRING_H