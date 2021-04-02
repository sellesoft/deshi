#pragma once
#ifndef DESHI_DEFINES_H
#define DESHI_DEFINES_H

#include "ContainerManager.h"

//math constants
#define M_PI         3.14159265359f
#define M_E          2.71828182846f
#define M_TWOTHIRDS  0.66666666666f
#define M_ONETWELFTH 0.08333333333f




//conversions
#define RADIANS(x) (x * (M_PI / 180.f))
#define DEGREES(x) (x * (180.f / M_PI))

//number typedefs
typedef signed char    i8;
typedef signed short   i16;
typedef signed int     i32;
typedef signed long    i64;
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;
typedef float          f32;
typedef double         f64;
typedef i8  int8;
typedef i16 int16;
typedef i32 int32;
typedef i64 int64;
typedef u8  uint8;
typedef u16 uint16;
typedef u32 uint32;
typedef u64 uint64;

//static defines
#define static_internal static
#define local_persist   static
#define global_variable static

//delle's annoyance with c++ cast syntax; which will probably cause problems somewhere
#define i8(x)     static_cast<i8>(x)
#define i16(x)    static_cast<i16>(x)
#define i32(x)    static_cast<i32>(x)
#define i64(x)    static_cast<i64>(x)
#define u8(x)     static_cast<u8>(x)
#define u16(x)    static_cast<u16>(x)
#define u32(x)    static_cast<u32>(x)
#define u64(x)    static_cast<u64>(x)
#define f32(x)    static_cast<f32>(x)
#define f64(x)    static_cast<f64>(x)
#define int8(x)   static_cast<int8>(x)
#define int16(x)  static_cast<int16>(x)
#define int32(x)  static_cast<int32>(x)
#define int64(x)  static_cast<int64>(x)
#define uint8(x)  static_cast<uint8>(x)
#define uint16(x) static_cast<uint16>(x)
#define uint32(x) static_cast<uint32>(x)
#define uint64(x) static_cast<uint64>(x)

#endif //DESHI_DEFINES_H
