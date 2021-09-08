#pragma once
#ifndef DESHI_STRING_CONVERSION_H
#define DESHI_STRING_CONVERSION_H

#include "string.h"
#include "cstring.h"
#include "color.h"
#include "../defines.h"
#include "../math/vectormatrix.h"

///////////////
//// @stox ////
///////////////
global_ int stoi(const string& s){
    int x;
    sscanf(s.str, "%d", &x);
    return x;
}

////////////////////
//// @to_string ////
////////////////////
global_ string to_string(cstring x){
    return string(x.str, x.count);
}

global_ string to_string(const char* fmt, ...){
    va_list argptr;
    va_start(argptr, fmt);
    string s;
    s.size  = vsnprintf(nullptr, 0, fmt, argptr);
    s.str   = (char*)malloc(s.size+1);
    s.space = s.size+1;
    vsnprintf(s.str, s.size+1, fmt, argptr);
    va_end(argptr);
    return s;
}

global_ string to_string(s32 x){
    string s;
    s.size = snprintf(nullptr, 0, "%d", x);
    s.str  = (char*)calloc(1, s.size+1);
    s.space = s.size+1;
    snprintf(s.str, s.size+1, "%d", x);
    return s;
}

global_ string to_string(u32 x){
    string s;
    s.size = snprintf(nullptr, 0, "%d", x);
    s.str  = (char*)calloc(1, s.size+1);
    s.space = s.size+1;
    snprintf(s.str, s.size+1, "%d", x);
    return s;
}

global_ string to_string(f32 x, bool trunc = true){
    string s;
    if(trunc){
        s.size = snprintf(nullptr, 0, "%.2f", x);
        s.str  = (char*)malloc(s.size+1);
        s.space = s.size+1;
        snprintf(s.str, s.size+1, "%.2f", x);
    }else{
        s.size = snprintf(nullptr, 0, "%f", x);
        s.str  = (char*)malloc(s.size+1);
        s.space = s.size+1;
        snprintf(s.str, s.size+1, "%f", x);
    }
    return s;
}

global_ string to_string(f64 x, bool trunc = true){
    string s;
    if(trunc){
        s.size = snprintf(nullptr, 0, "%.2f", x);
        s.str  = (char*)malloc(s.size+1);
        s.space = s.size+1;
        snprintf(s.str, s.size+1, "%.2f", x);
    }else{
        s.size = snprintf(nullptr, 0, "%f", x);
        s.str  = (char*)malloc(s.size+1);
        s.space = s.size+1;
        snprintf(s.str, s.size+1, "%f", x);
    }
    return s;
}

global_ string to_string(upt x){
    string s;
    s.size = snprintf(nullptr, 0, "%zu", x);
    s.str  = (char*)malloc(s.size+1);
    s.space = s.size+1;
    snprintf(s.str, s.size+1, "%zu", x);
    return s;
}

global_ string to_string(const color& x){
    string s;
    s.size = snprintf(nullptr, 0, "{R:%d, G:%d, B:%d, A:%d}", x.r, x.g, x.b, x.a);
    s.str  = (char*)malloc(s.size+1);
    s.space = s.size+1;
    snprintf(s.str, s.size+1, "{R:%d, G:%d, B:%d, A:%d}", x.r, x.g, x.b, x.a);
    return s;
}

global_ string to_string(const vec2& x, bool trunc = true){
    string s;
    if(trunc){
        s.size = snprintf(nullptr, 0, "(%+.2f, %+.2f)", x.x, x.y);
        s.str  = (char*)malloc(s.size+1);
        s.space = s.size+1;
        snprintf(s.str, s.size+1, "(%+.2f, %+.2f)", x.x, x.y);
    }else{
        s.size = snprintf(nullptr, 0, "(%+f, %+f)", x.x, x.y);
        s.str  = (char*)malloc(s.size+1);
        s.space = s.size+1;
        snprintf(s.str, s.size+1, "(%+f, %+f)", x.x, x.y);
    }
    return s;
}

global_ string to_string(const vec3& x, bool trunc = true){
    string s;
    if(trunc){
        s.size = snprintf(nullptr, 0, "(%+.2f, %+.2f, %+.2f)", x.x, x.y, x.z);
        s.str  = (char*)malloc(s.size+1);
        s.space = s.size+1;
        snprintf(s.str, s.size+1, "(%+.2f, %+.2f, %+.2f)", x.x, x.y, x.z);
    }else{
        s.size = snprintf(nullptr, 0, "(%+f, %+f, %+f)", x.x, x.y, x.z);
        s.str  = (char*)malloc(s.size+1);
        s.space = s.size+1;
        snprintf(s.str, s.size+1, "(%+f, %+f, %+f)", x.x, x.y, x.z);
    }
    return s;
}

global_ string to_string(const vec4& x, bool trunc = true){
    string s;
    if(trunc){
        s.size = snprintf(nullptr, 0, "(%+.2f, %+.2f, %+.2f, %+.2f)", x.x, x.y, x.z, x.w);
        s.str  = (char*)malloc(s.size+1);
        s.space = s.size+1;
        snprintf(s.str, s.size+1, "(%+.2f, %+.2f, %+.2f, %+.2f)", x.x, x.y, x.z, x.w);
    }else{
        s.size = snprintf(nullptr, 0, "(%+f, %+f, %+f, %+f)", x.x, x.y, x.z, x.w);
        s.str  = (char*)malloc(s.size+1);
        s.space = s.size+1;
        snprintf(s.str, s.size+1, "(%+f, %+f, %+f, %+f)", x.x, x.y, x.z, x.w);
    }
    return s;
}

global_ string to_string(const mat3& x, bool trunc = true){
    string s;
    Assert(!"not implemented yet");
    if(trunc){
        
    }else{
        
    }
    return s;
}

global_ string to_string(const mat4& x, bool trunc = true){
    string s;
    Assert(!"not implemented yet");
    if(trunc){
        
    }else{
        
    }
    return s;
}

#endif //DESHI_STRING_CONVERSION_H
