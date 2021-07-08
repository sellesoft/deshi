#pragma once
#ifndef DESHI_DEFINES_H
#define DESHI_DEFINES_H

//math constants and macros
#define M_EPSILON    0.001f
#define M_PI         3.14159265359f
#define M_2PI        6.28318530718f
#define M_E          2.71828182846f
#define M_SQRT_TWO   1.41421356237f
#define M_SQRT_THREE 1.73205080757f
#define RADIANS(x) ((x) * (M_PI / 180.f))
#define DEGREES(x) ((x) * (180.f / M_PI))

//deshi constants
#define DESHI_NAME_SIZE 64

//number typedefs
typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed long long   s64;
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef float              f32;
typedef double             f64;
typedef s32                b32; //int-based boolean so c++ doesnt convert to 0 or 1

//static defines
#define static_internal static
#define local_persist   static
#define global_variable static

//dynamic cast short-hand
#define dyncast(child, base) dynamic_cast<child*>(base)

//for-loop shorthands
#define forX(var_name,iterations) for(int var_name=0; var_name<(iterations); ++var_name)
#define forI(iterations) for(int i=0; i<(iterations); ++i)
#define forR(start, stop) for(int i=stop; i>=stop; --i)

//dst: destination c-string; src: source c-string; bytes: number of characters to copy
//NOTE the last character in the copy is replaced with a null-terminating character
#define cpystr(dst, src, bytes) strncpy_s(dst, src, bytes); dst[bytes-1] = '\0'

//compile-time print sizeof()
//char (*__kaboom)[sizeof( YourTypeHere )] = 1;

// https://stackoverflow.com/a/42060129
#ifndef defer
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }
#define DEFER_(LINE) zz_defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif // defer

// https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html
#define xx_STRINGIZE(x) #x
#define STRINGIZE(x) xx_STRINGIZE(x)

//size of c-style array
#define ArrayCount(_ARR) (sizeof((_ARR)) / sizeof(((_ARR))[0]))

#define Kilobytes(x) ((x)*1024ULL)
#define Megabytes(x) (Kilobytes((x))*1024ULL)
#define Gigabytes(x) (Megabytes((x))*1024ULL)
#define Terabytes(x) (Gigabytes((x))*1024ULL)

//library-less assert
#if DESHI_SLOW
//the ... is to allow the programmer to put some text to read when the assert fails
//but it doesnt actually do affect the assertion expression
#define Assert(expression, ...) if(!(expression)){*(volatile int*)0 = 0;}
#else
#define Assert(expression, ...) expression
#endif //DESHI_SLOW

//debug breakpoint
#ifdef _MSC_VER
#define DEBUG_BREAK __debugbreak()
#else
#define DEBUG_BREAK (void)0
#endif //_MSC_VER

#endif //DESHI_DEFINES_H
