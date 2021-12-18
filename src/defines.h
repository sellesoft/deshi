#pragma once
#ifndef DEFINES_H
#define DEFINES_H
///////////////////////// //NOTE this file is included is almost every other file of the project, so be frugal with includes here
//// common includes ////
/////////////////////////
#include <cstddef> //size_t, ptrdiff_t
#include <cstdlib> //malloc, calloc, free

///////////////////////
//// static macros ////
///////////////////////
#define function static
#define local    static //inside a .cpp
#define persist  static //inside a function
#define global_  static //inside a .h
#define local_const static const
#define global_const static const
#define external extern "C"

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
#define DebugBreakpoint DEBUG_BREAK

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

//TODO(delle) function pointer signature macro
typedef void* (*Allocator_ReserveMemory_Func)(upt bytes);
typedef void  (*Allocator_ChangeMemory_Func)(void* ptr, upt bytes);
typedef void  (*Allocator_ReleaseMemory_Func)(void* ptr);
typedef void* (*Allocator_ResizeMemory_Func)(void* ptr, upt bytes);
function void* Allocator_ReserveMemory_Noop(upt bytes){}
function void  Allocator_ChangeMemory_Noop(void* ptr, upt bytes){}
function void  Allocator_ReleaseMemory_Noop(void* ptr){}
function void* Allocator_ResizeMemory_Noop(void* ptr, upt bytes){}
struct Allocator{
	Allocator_ReserveMemory_Func reserve;  //reserves address space from OS
	Allocator_ChangeMemory_Func  commit;   //allocates memory from reserved space
	Allocator_ChangeMemory_Func  decommit; //returns the memory to reserved state
	Allocator_ReleaseMemory_Func release;  //release the reserved memory back to OS
	Allocator_ResizeMemory_Func  resize;   //resizes reserved memory and moves memory if a new location is required
};

/////////////////////// //NOTE some are two level so you can use the result of a macro expansion (STRINGIZE, GLUE, etc)
//// common macros ////
///////////////////////
#define STMNT(s) do{ s }while(0)
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
#define RADIANS(x) ((x) * (M_PI / 180.f))
#define DEGREES(x) ((x) * (180.f / M_PI))
#define ArrayCount(arr) (sizeof((arr)) / sizeof(((arr))[0])) //length of a static-size c-array
#define RoundUpTo(value, multiple) (((size_t)((value) + (((size_t)(multiple))-1)) / (size_t)(multiple)) * (size_t)(multiple))
#define PackU32(x,y,z,w) (((u32)(x) << 24) | ((u32)(y) << 16) | ((u32)(z) << 8) | ((u32)(w) << 0))
#define PointerDifference(a,b) ((u8*)(a) - (u8*)(b))
#define PointerAsInt(a) PointerDifference(a,0)
#define OffsetOfMember(structName,memberName) PointerAsInt(&(((structName*)0)->memberName))
#define CastFromMember(structName,memberName,ptr) (structName*)((u8*)(ptr) - OffsetOfMember(structName,memberName))
#define StartNamespace(a) namespace a{ (void)0
#define EndNamespace(a) }
#define CastToConst(type,a) const_cast<const type>(a)
#define CastFromConst(type,a) const_cast<type>(a)
#define StaticCast(type,a) static_cast<type>(a)
#define DynamicCast(type,a) dynamic_cast<type>(a)

//////////////////////////
//// common functions ////
//////////////////////////
FORCE_INLINE b32 IsPow2(u64 value){return (value != 0) && ((value & (value-1)) == 0);}
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

//////////////////////////
//// common constants ////
//////////////////////////
global_const u8  MAX_U8  = 0xFF;
global_const u16 MAX_U16 = 0xFFFF;
global_const u32 MAX_U32 = 0xFFFFFFFF;
global_const u64 MAX_U64 = 0xFFFFFFFFFFFFFFFF;

global_const s8  MIN_S8  = -127 - 1;
global_const s8  MAX_S8  = 127;
global_const s16 MIN_S16 = -32767 - 1;
global_const s16 MAX_S16 = 32767;
global_const s32 MIN_S32 = -2147483647 - 1;
global_const s32 MAX_S32 = 2147483647;
global_const s64 MIN_S64 = -9223372036854775807 - 1;
global_const s64 MAX_S64 = 9223372036854775807;

global_const f32 MAX_F32 = 3.402823466e+38f;
global_const f32 MIN_F32 = -MAX_F32;
global_const f64 MAX_F64 = 1.79769313486231e+308;
global_const f64 MIN_F64 = -MAX_F64;

//TODO make these typed
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

/////////////////////// 
//// assert macros //// //NOTE the ... is for a programmer message at the assert; it is unused otherwise
/////////////////////// //TODO(delle) refactor Assert() usages so the expression is not used
#define AssertAlways(expression, ...) STMNT( if(!(expression)){*(volatile int*)0 = 0;} ) //works regardless of SLOW or INTERNAL
#define AssertBreakpoint(expression, ...) STMNT( if(!(expression)){ DebugBreakpoint; } )
#define StaticAssertAlways(expression, ...) char GLUE(__ignore__, GLUE(__LINE__,__default__))[(expression)?1:-1]

#if   DESHI_INTERNAL
#  define Assert(expression, ...) AssertBreakpoint(expression)
#  define StaticAssert(expression, ...) StaticAssertAlways(expression)
#elif DESHI_SLOW
#  define Assert(expression, ...) AssertAlways(expression)
#  define StaticAssert(expression, ...) StaticAssertAlways(expression)
#else
#  define Assert(expression, ...) expression
#  define StaticAssert(expression, ...) 
#endif

#define NotImplemented Assert(false, "not implemented yet")
#define InvalidPath Assert(false, "invalid path")
#define TestMe AssertBreakpoint(false, "this needs to be tested")
#define FixMe AssertBreakpoint(false, "this is broken in some way")

/////////////////////////
//// for-loop macros ////
/////////////////////////
#define forX(var_name,iterations) for(int var_name=0; var_name<(iterations); ++var_name)
#define forI(iterations) for(int i=0; i<(iterations); ++i)
#define forE(iterable) for(auto it = iterable.begin(), it_begin = iterable.begin(), it_end = iterable.end(); it != it_end; ++it)

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

function void* STLAllocator_Reserve(upt bytes){void* a = calloc(1,bytes); Assert(a); return a;}
function void  STLAllocator_Release(void* ptr){free(ptr);}
function void* STLAllocator_Resize(void* ptr, upt bytes){void* a = realloc(ptr,bytes); Assert(a); return a;}
global_ Allocator base_stl_allocator{
	STLAllocator_Reserve,
	Allocator_ChangeMemory_Noop,
	Allocator_ChangeMemory_Noop,
	STLAllocator_Release,
	STLAllocator_Resize
};

///////////////////////////// //TODO remove/rework/rename these
//// to-be-redone macros ////
/////////////////////////////
#define DESHI_NAME_SIZE 64 //NOTE arbitrarily chosen size, but its convenient to have a fixed size for names
#define cpystr(dst,src,bytes) strncpy((dst), (src), (bytes)); (dst)[(bytes)-1] = '\0' //copy c-string and null-terminate
#define dyncast(child,base) dynamic_cast<child*>(base) //dynamic cast short-hand

#endif //DEFINES_H