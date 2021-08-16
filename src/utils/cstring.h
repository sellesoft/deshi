#pragma once
#ifndef DESHI_CSTRING_H
#define DESHI_CSTRING_H

#include "../defines.h"

struct cstring{
    char* str;
    u64   count;
    
    inline char operator[](u32 idx){ return str[idx]; }
    inline explicit operator bool(){ return count; }
    inline bool operator==(cstring s){ return (str==s.str) && (count==s.count); }
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

#endif //DESHI_CSTRING_H
