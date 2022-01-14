#pragma once
#ifndef DESHI_CSTRING_H
#define DESHI_CSTRING_H

#include "../defines.h"
#include <cstring> //strncmp, memcpy

global_ inline void
advance(cstring* s, upt count = 1){
    s->str += count; s->count -= count;
}

global_ inline b32
equals(cstring a, cstring b){
	return (a.count == b.count) && (strncmp(a.str, b.str, a.count) == 0);
}

global_ inline cstring 
eat_until_char(cstring s, char c){
    for(upt i=0; i<s.count; ++i){ if(s[i] == c){ return cstring{s.str, i}; } }
    return cstring{};
}

global_ inline cstring 
eat_until_char_skip_quotes(cstring s, char c){
    bool in_quotes = false;
    for(upt i=0; i<s.count; ++i){ 
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

global_ inline upt
isnumber(char c){
    return (c >= '0' && c <= '9') ? true : false;
}

global_ inline upt
ishex(char c){
    return ((c >= '0' && c <= '9') || 
            (c >= 'A' && c <= 'F') || 
            (c >= 'a' && c <= 'f') || 
            (c == 'x' || c == 'X')) ? true : false;
}

global_ inline u64
b10tou64(cstring s, cstring* next = 0){
    u64 result = 0;
    u64 sign = 1;
    u32 idx = 0;
    
    //error cases
    if(!s.str) return 0;
    if(*s.str == '\0') return 0;
    if(s.count == 0) return 0;
    
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
        result = (result*10) + (*s.str - (upt)'0');
        s.str++; idx++;
    }
    
    if(next){
        next->str = s.str;
        next->count = s.count - idx;
    }
    return sign * result;
}

global_ inline u64
b16tou64(cstring s, cstring* next = 0){
    u64 result = 0;
    u64 sign = 1;
    u32 idx = 0;
    
    //error cases
    if(!s.str) return 0;
    if(*s.str == '\0') return 0;
	if(s.count == 0) return 0;
    
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
            result = (result*16) + (*s.str - (upt)'0');
        }else if(*s.str >= 'A' && *s.str <= 'F'){
            result = (result*16) + (*s.str - (upt)'A') + 10;
        }else if(*s.str >= 'a' && *s.str <= 'f'){
            result = (result*16) + (*s.str - (upt)'a') + 10;
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



//wcstring

struct wcstring{
    wchar_t* str;
    u64    count;
	
    inline wchar_t operator[](u32 idx){ return str[idx]; }
    inline explicit operator bool(){ return count; }
    inline bool operator==(wcstring s){ return (count == s.count) && (wcsncmp(str, s.str, count) == 0); }
};

#define wcstring_lit(s) wcstring{(wchar_t*)s, sizeof(s)/2-1}
#define wcstr_lit(s) wcstring{(wchar_t*)s, sizeof(s)/2-1}

global_ inline void
advance(wcstring* s, u64 count = 1){
    s->str += count; s->count -= count;
}

global_ inline wcstring
eat_until_char(wcstring s, wchar_t c){
    for (u64 i = 0; i < s.count; ++i){ if(s[i] == c){ return wcstring{ s.str, i }; } }
    return wcstring{};
}

global_ inline wcstring
eat_until_wchar_t_skip_quotes(wcstring s, wchar_t c){
    bool in_quotes = false;
    for (u64 i = 0; i < s.count; ++i){
        if(s[i] == '\"') in_quotes = !in_quotes;
        if(!in_quotes && s[i] == c){
            return wcstring{ s.str, i };
        }
    }
    return wcstring{};
}

global_ inline wcstring
eat_spaces(wcstring s){
    while(*s.str == ' '){ s.str++; s.count--; }
    return s;
}

global_ inline wchar_t*
to_c_string(wcstring s){
    wchar_t* cs = (wchar_t*)calloc(s.count + 1, sizeof(wchar_t));
    memcpy(cs, s.str, s.count);
    return cs;
}

global_ inline u64
isnumber(wchar_t c){
    return (c >= '0' && c <= '9') ? true : false;
}

global_ inline u64
ishex(wchar_t c){
    return ((c >= '0' && c <= '9') ||
			(c >= 'A' && c <= 'F') ||
			(c >= 'a' && c <= 'f') ||
			(c == 'x' || c == 'X')) ? true : false;
}

global_ inline u64
b10tou64(wcstring s, wcstring* next = 0){
    u64 result = 0;
    u64 sign = 1;
    u32 idx = 0;
	
    //error cases
    if(!s.str) return 0;
    if(*s.str == '\0') return 0;
    if(s.count == 0) return 0;
	
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
        result = (result * 10) + (*s.str - '0');
        s.str++; idx++;
    }
	
    if(next){
        next->str = s.str;
        next->count = s.count - idx;
    }
    return sign * result;
}

global_ inline u64
b16tou64(wcstring s, wcstring* next = 0){
    u64 result = 0;
    u64 sign = 1;
    u32 idx = 0;
	
    //error cases
    if(!s.str) return 0;
    if(*s.str == '\0') return 0;
    if(s.count == 0) return 0;
	
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
    if(s.count > 2 && *s.str == '0' && *(s.str + 1) == 'x'){
        s.str += 2; idx += 2;
    }
	
    while(*s.str != '\0' && idx < s.count){
        if      (*s.str >= '0' && *s.str <= '9'){
            result = (result * 16) + (*s.str - '0');
        }else if(*s.str >= 'A' && *s.str <= 'F'){
            result = (result * 16) + (*s.str - 'A') + 10;
        }else if(*s.str >= 'a' && *s.str <= 'f'){
            result = (result * 16) + (*s.str - 'a') + 10;
        }else {
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