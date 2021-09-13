#pragma once
#ifndef DESHI_DEFINES_H
#define DESHI_DEFINES_H
//NOTE function calls in macros can get executed for each time they are placed (if not optimized away)
//eg: Min(5, sqrt(26)) expands to (5 < sqrt(26)) ? 5 : sqrt(26)

//compile-time print sizeof(); compiler will give an error with the size of the object
//char (*__kaboom)[sizeof( YourTypeHere )] = 1;

//NOTE this file is included is almost every other file of the project, so be frugal with includes here
#include <cstddef>
#include <cstdlib>

//deshi constants //TODO(delle) remove deshi constants
//NOTE arbitrarily chosen size, but its convenient to have a fixed size for names
#define DESHI_NAME_SIZE 64

//math constants and macros
#define M_EPSILON    0.00001f
#define M_PI         3.14159265359f
#define M_2PI        6.28318530718f
#define M_TAU        M_2PI
#define M_E          2.71828182846f
#define M_SQRT_TWO   1.41421356237f
#define M_SQRT_THREE 1.73205080757f
#define RADIANS(x) ((x) * (M_PI / 180.f))
#define DEGREES(x) ((x) * (180.f / M_PI))

//number typedefs
typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed long long   s64;
typedef ptrdiff_t          spt;
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef size_t             upt;
typedef float              f32;
typedef double             f64;

//static defines
#define local    static //inside a .cpp
#define persist  static //inside a function
#define global_  static //inside a .h

//slow macros
#if DESHI_SLOW
//assert that an expression is true
//NOTE the ... is to allow the programmer to put some text to read when the assert fails
//     but it doesnt actually affect the assertion expression
//NOTE we dont place this under DESHI_INTERNAL so that crashes do happen outside of development
#define Assert(expression, ...) if(!(expression)){*(volatile int*)0 = 0;}
#else
#pragma warning(once : 4552)
#pragma warning(once : 4553)
#define Assert(expression, ...) expression
#endif //DESHI_SLOW

//compiler-dependent builtins
#if   defined(_MSC_VER)

#define FORCE_INLINE __forceinline
#define DEBUG_BREAK __debugbreak()
#define ByteSwap16(x) _byteswap_ushort(x)
#define ByteSwap32(x) _byteswap_ulong(x)
#define ByteSwap64(x) _byteswap_uint64(x)

#elif defined(__GNUC__) || defined(__clang__) //_MSC_VER

#define FORCE_INLINE inline __attribute__((always_inline))
#error "unhandled debug breakpoint; look at: https://github.com/scottt/debugbreak"
#define ByteSwap16(x) __builtin_bswap16(x)
#define ByteSwap32(x) __builtin_bswap32(x)
#define ByteSwap64(x) __builtin_bswap64(x)

#else //__GNUC__ || __clang__

#error "unhandled compiler"

#endif

//for-loop shorthands for the simple,sequential iteration case
#define forX(var_name,iterations) for(int var_name=0; var_name<(iterations); ++var_name)
#define forI(iterations) for(int i=0; i<(iterations); ++i)
#define forE(iterable) for(auto it = iterable.begin(), it_begin = iterable.begin(), it_end = iterable.end(); it != it_end; ++it)

// https://stackoverflow.com/a/42060129 by pmttavara
//defers execution inside the block to the end of the current scope; this works by
//placing that code in a lambda that a dummy object will call in its destructor
//NOTE it is kept unique by its line number, so you can't call two on the same line
#ifndef defer
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }
#define DEFER_(LINE) zz_defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif // defer

// https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html
//two level so you can stringize the result of a macro expansion
#define STRINGIZE_(x) #x
#define STRINGIZE(x) STRINGIZE_(x)
#define GLUE_(a,b) a##b
#define GLUE(a,b) GLUE_(a,b)

#define ArrayCount(_ARR) (sizeof((_ARR)) / sizeof(((_ARR))[0])) //length of a static-size c-array
#define Kilobytes(x) ((x) << 10)
#define Megabytes(x) ((x) << 20)
#define Gigabytes(x) ((x) << 30)
#define Terabytes(x) (((u64)(x)) << 40)
#define Clamp(value, min, max) (((signed)(value) < (signed)(min)) ? (min) : (((signed)(value) > (signed)(max)) ? (max) : (value)))
#define Max(a, b) (((a) > (b)) ? (a) : (b))
#define Min(a, b) (((a) < (b)) ? (a) : (b))
#define RoundUpTo(value, multiple) (((size_t)((value) + (((size_t)(multiple))-1)) / (size_t)(multiple)) * (size_t)(multiple))
#define PackU32(x,y,z,w) (((u32)(x) << 24) | ((u32)(y) << 16) | ((u32)(z) << 8) | ((u32)(w) << 0))

//NOTE macros disliked by delle :)
#define cpystr(dst,src,bytes) strncpy((dst), (src), (bytes)); (dst)[(bytes)-1] = '\0' //copy c-string and null-terminate
#define dyncast(child,base) dynamic_cast<child*>(base) //dynamic cast short-hand

template<typename T> inline void Swap(T& a, T& b){T temp = a; a = b; b = temp;};

#endif //DESHI_DEFINES_H