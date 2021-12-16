#pragma once
#ifndef DESHI_DEFINES_H
#define DESHI_DEFINES_H
///////////////////////// //NOTE this file is included is almost every other file of the project, so be frugal with includes here
//// common includes ////
/////////////////////////
#include <cstddef> //size_t, ptrdiff_t
#include <cstdlib> //malloc, calloc, free

/////////////////////
//// math macros ////
/////////////////////
#define M_EPSILON    0.00001f
#define M_FOURTHPI   0.78539816339f
#define M_HALFPI     1.57079632679f
#define M_PI         3.14159265359f
#define M_PId        3.14159265358979
#define M_2PI        6.28318530718f
#define M_TAU        M_2PI
#define M_E          2.71828182846f
#define M_SQRT_TWO   1.41421356237f
#define M_SQRT_THREE 1.73205080757f
#define RADIANS(x) ((x) * (M_PI / 180.f))
#define DEGREES(x) ((x) * (180.f / M_PI))

///////////////////////
//// static macros ////
///////////////////////
#define local    static //inside a .cpp
#define persist  static //inside a function
#define global_  static //inside a .h

/////////////////////// //assert that an expression is true
//// assert macros //// //NOTE we dont place these under DESHI_INTERNAL so that crashes DO happen outside of development
/////////////////////// //NOTE the ... is for a programmer message at the assert; it is unused otherwise
#if DESHI_SLOW
#  define Assert(expression, ...) if(!(expression)){*(volatile int*)0 = 0;}
#else
#  pragma warning(once : 4552)
#  pragma warning(once : 4553)
#  define Assert(expression, ...) expression
#endif //DESHI_SLOW
#define NotImplemented Assert(false, "not implemented yet")

/////////////////////////////////////
//// compiler-dependent builtins ////
/////////////////////////////////////
#if   defined(_MSC_VER)
#  define FORCE_INLINE __forceinline
#  define DEBUG_BREAK __debugbreak()
#  define ByteSwap16(x) _byteswap_ushort(x)
#  define ByteSwap32(x) _byteswap_ulong(x)
#  define ByteSwap64(x) _byteswap_uint64(x)
#elif defined(__GNUC__) || defined(__clang__) //_MSC_VER
#  define FORCE_INLINE inline __attribute__((always_inline))
#  error "unhandled debug breakpoint; look at: https://github.com/scottt/debugbreak"
#  define ByteSwap16(x) __builtin_bswap16(x)
#  define ByteSwap32(x) __builtin_bswap32(x)
#  define ByteSwap64(x) __builtin_bswap64(x)
#else //__GNUC__ || __clang__
#  error "unhandled compiler"
#endif

/////////////////////////
//// for-loop macros ////
/////////////////////////
#define forX(var_name,iterations) for(int var_name=0; var_name<(iterations); ++var_name)
#define forI(iterations) for(int i=0; i<(iterations); ++i)
#define forE(iterable) for(auto it = iterable.begin(), it_begin = iterable.begin(), it_end = iterable.end(); it != it_end; ++it)

//////////////////////
//// common types ////
//////////////////////
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
typedef s32                b32;
typedef char16_t           uchar;

typedef u32 Type;
typedef u32 Flags;

typedef void* (*BaseAllocator_Reserve_Func)(void* ctx, upt bytes);
typedef void  (*BaseAllocator_ChangeMemory_Func)(void* ctx, void* ptr, upt bytes);
struct BaseAllocator{
	BaseAllocator_Reserve_Func*      reserve;  //ask for memory from OS
	BaseAllocator_ChangeMemory_Func* commit;   //grab memory for use
	BaseAllocator_ChangeMemory_Func* decommit; //not using memory anymore
	BaseAllocator_ChangeMemory_Func* release;  //tell OS we dont need memory
	void* ctx;
};
struct STLAllocator{
	void* allocate(upt bytes){return malloc(bytes);}
	void* callocate(upt count, upt size){return calloc(count,size);}
	void  deallocate(void* ptr){free(ptr);};
};

/////////////////////// //NOTE some are two level so you can use the result of a macro expansion (STRINGIZE, GLUE, etc)
//// common macros ////
///////////////////////
#define UNUSED_VAR(a) ((void)(a))
#define STRINGIZE_(a) #a
#define STRINGIZE(a) STRINGIZE_(a)
#define GLUE_(a,b) a##b
#define GLUE(a,b) GLUE_(a,b)
#define ToggleBool(variable) variable = !variable
#define Kilobytes(a) (((u64)(a)) << 10)
#define Megabytes(a) (((u64)(a)) << 20)
#define Gigabytes(a) (((u64)(a)) << 30)
#define Terabytes(a) (((u64)(a)) << 40)
#define ArrayCount(arr) (sizeof((arr)) / sizeof(((arr))[0])) //length of a static-size c-array
#define RoundUpTo(value, multiple) (((size_t)((value) + (((size_t)(multiple))-1)) / (size_t)(multiple)) * (size_t)(multiple))
#define PackU32(x,y,z,w) (((u32)(x) << 24) | ((u32)(y) << 16) | ((u32)(z) << 8) | ((u32)(w) << 0))
#define PointerDifference(a,b) ((u8*)(a) - (u8*)(b))
#define PointerAsInt(a) PointerDifference(a,0)
#define OffsetOfMember(structName,memberName) PointerAsInt(&(((structName*)0)->memberName))
#define CastFromMember(structName,memberName,ptr) (structName*)((u8*)(ptr) - OffsetOfMember(structName,memberName))

//////////////////////////
//// common functions ////
//////////////////////////
FORCE_INLINE b32 IsPow2(u64 value){return (value != 0) && (value & (value-1) == 0);}
template<typename T> FORCE_INLINE void Swap(T& a, T& b){T temp = a; a = b; b = temp;}
template<typename T> FORCE_INLINE T Max(T a, T b){return (a > b) ? a : b;}
template<typename T> FORCE_INLINE T Min(T a, T b){return (a < b) ? a : b;}
template<typename T> FORCE_INLINE T Clamp(T value, T min, T max){return (value < min) ? min : ((value > max) ? max : value);};
template<typename T,typename U> FORCE_INLINE T Clamp(T value, U min, T max){return (value < min) ? min : ((value > max) ? max : value);}
template<typename T,typename U> FORCE_INLINE T Clamp(T value, T min, U max){return (value < min) ? min : ((value > max) ? max : value);}
template<typename T,typename U> FORCE_INLINE T Clamp(T value, U min, U max){return (value < min) ? min : ((value > max) ? max : value);}
template<typename T> FORCE_INLINE T ClampMin(T value, T min){return (value < min) ? min : value;};
template<typename T> FORCE_INLINE T ClampMax(T value, T max){return (value > max) ? max : value;};
template<typename T,typename U> FORCE_INLINE T ClampMin(T value, U min){return (value < min) ? min : value;};
template<typename T,typename U> FORCE_INLINE T ClampMax(T value, U max){return (value > max) ? max : value;};

///////////////////////////// //TODO remove/rework/rename these
//// to-be-redone macros ////
/////////////////////////////
#define DESHI_NAME_SIZE 64 //NOTE arbitrarily chosen size, but its convenient to have a fixed size for names
#define cpystr(dst,src,bytes) strncpy((dst), (src), (bytes)); (dst)[(bytes)-1] = '\0' //copy c-string and null-terminate
#define dyncast(child,base) dynamic_cast<child*>(base) //dynamic cast short-hand

///////////////
//// other ////
///////////////
//compile-time print sizeof(); compiler will give an error with the size of the object
//char (*__kaboom)[sizeof( YourTypeHere )] = 1;

//ref: https://stackoverflow.com/a/42060129 by pmttavara
//defers execution inside the block to the end of the current scope; this works by placing
//that code in a lambda specific to that linethat a dummy object will call in its destructor
#ifndef defer
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }
#  define DEFER_(LINE) zz_defer##LINE
#  define DEFER(LINE) DEFER_(LINE)
#  define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif // defer

#endif //DESHI_DEFINES_H