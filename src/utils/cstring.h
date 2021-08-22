#pragma once
#ifndef DESHI_CSTRING_H
#define DESHI_CSTRING_H

#include "../defines.h"

struct cstring{
    char* str;
    u64   count;
    
    inline char operator[](u32 idx){ return str[idx]; }
    inline explicit operator bool(){ return count; }
    inline bool operator==(cstring s){ return (count==s.count) && (strncmp(str, s.str, count) == 0); }
};

global_ inline void
advance(cstring* s, u64 count){
    s->str += count; s->count -= count;
}

global_ inline cstring 
eat_until_char(cstring s, char c){
    for(u64 i=0; i<s.count; ++i){ if(s[i] == c){ return cstring{s.str, i}; } }
    return cstring{};
}

global_ inline cstring 
eat_until_char_skip_quotes(cstring s, char c){
    bool in_quotes = false;
    for(u64 i=0; i<s.count; ++i){ 
        if(s[i] == '\"') in_quotes = !in_quotes;
        if(!in_quotes && s[i] == c){ 
            return cstring{s.str, i}; 
        }
    }
    return cstring{};
}

global_ inline cstring
eat_spaces(cstring s){
    while(*s.str == ' '){ s.str++; s.count--; }
    return s;
}

global_ inline char*
to_c_string(cstring s){
    char* cs = (char*)calloc(s.count+1, sizeof(char));
    memcpy(cs, s.str, s.count);
    return cs;
}

global_ inline u64
isnumber(char c){
    return (c >= '0' && c <= '9') ? true : false;
}

global_ inline u64
ishex(char c){
    return ((c >= '0' && c <= '9') || 
            (c >= 'A' && c <= 'F') || 
            (c >= 'a' && c <= 'f') || 
            (c == 'x' || c == 'X')) ? true : false;
}

global_ inline u64
b10tou64(cstring s, cstring* next = 0){ //!!TestMe
    u64 result = 0;
    u64 sign = 1;
    u32 idx = 0;
    
    //error cases
    if(!s.str) return 0;
    if(*s.str == '\0') return 0;
    
    //skip whitespace
    while(*s.str == ' '){ s.str++; idx++; }
    
    //check for sign
    if(*s.str == '-'){
        sign = -1;
        s.str++; idx--;
    }
    if(*s.str == '+'){
        s.str++; idx--;
    }
    
    while(*s.str != '\0' && idx < s.count){
        if(!isnumber(*s.str)) break;
        result = (result*10) + (*s.str - '0');
        s.str++; idx++;
    }
    
    if(next){
        next->str = s.str;
        next->count = s.count - idx;
    }
    return sign * result;
}

global_ inline u64
b16tou64(cstring s, cstring* next = 0){ //!!TestMe
    u64 result = 0;
    u64 sign = 1;
    u32 idx = 0;
    
    //error cases
    if(!s.str) return 0;
    if(*s.str == '\0') return 0;
    
    //skip whitespace
    while(*s.str == ' '){ s.str++; idx++; }
    
    //check for sign
    if(*s.str == '-'){
        sign = -1;
        s.str++; idx++;
    }
    if(*s.str == '+'){
        s.str++; idx++;
    }
    
    //check for 0x prefix
    if(s.count > 2 && *s.str == '0' && *(s.str+1) == 'x'){
        s.str+=2; idx+=2;
    }
    
    while(*s.str != '\0' && idx < s.count){
        if      (*s.str >= '0' && *s.str <= '9'){
            result = (result*16) + (*s.str - '0');
        }else if(*s.str >= 'A' && *s.str <= 'F'){
            result = (result*16) + (*s.str - 'A') + 10;
        }else if(*s.str >= 'a' && *s.str <= 'f'){
            result = (result*16) + (*s.str - 'a') + 10;
        }else{
            break;
        }
        s.str++; idx++;
    }
    
    if(next){
        next->str = s.str;
        next->count = s.count - idx;
    }
    return sign * result;
}

#endif //DESHI_CSTRING_H