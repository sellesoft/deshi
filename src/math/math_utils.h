#pragma once
#ifndef DESHI_MATH_UTILS_H
#define DESHI_MATH_UTILS_H

#include "../defines.h"
#include <cmath>
#include <cstring> //memcpy

//// enable SSE if available ////
//!ref: https://github.com/HandmadeMath/Handmade-Math/blob/master/HandmadeMath.h
#ifndef DESHI_DISABLE_SSE
#  ifdef _MSC_VER
/* MSVC supports SSE in amd64 mode or _M_IX86_FP >= 1 (2 means SSE2) */
#    if defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
//#pragma message("_MSC_VER && (defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1))")
#     define DESHI_USE_SSE 1
#     include <xmmintrin.h>
#    endif 
#  else//_MSC_VER
#    ifdef __SSE__ /* non-MSVC usually #define __SSE__ if it's supported */
//#pragma message("!_MSC_VER && __SSE__")
#      define DESHI_USE_SSE 1
#      include <xmmintrin.h>
#    endif//__SSE__
#  endif//not _MSC_VER
#endif //not DESHI_DISABLE_SSE

template<typename T> FORCE_INLINE b32 EpsilonEqual(T a, T b){
	return (abs(a - b) < M_EPSILON);
}

//// CRT wrappers ////
global_ inline f32 Sqrt(f32 a){
	f32 result;
#ifdef DESHI_USE_SSE
	__m128 in = _mm_set_ss(a);
	__m128 out = _mm_sqrt_ss(in);
	result = _mm_cvtss_f32(out);
#else
	result = sqrtf(a);
#endif
	return result;
};

//NOTE this has decently low precision (.0001)
global_ inline f32 Rsqrt(f32 a){
	f32 result;
#ifdef DESHI_USE_SSE
	__m128 in = _mm_set_ss(a);
	__m128 out = _mm_rsqrt_ss(in);
	result = _mm_cvtss_f32(out);
#else
	result = 1.0f / Sqrt(a);
#endif
	return result;
};

//// SSE funcs //// //TODO look into having SSE constants (epsilon, negative zero)
#ifdef DESHI_USE_SSE
FORCE_INLINE __m128 FillSSE(f32 a){
	return _mm_set1_ps(a);
}

FORCE_INLINE __m128 NegateSSE(__m128 a){
	return _mm_xor_ps(a, FillSSE(-0.0f));
}

FORCE_INLINE __m128 AbsoluteSSE(__m128 a){
	return _mm_andnot_ps(FillSSE(-0.0f), a);
}

FORCE_INLINE b32 EpsilonEqualSSE(__m128 a, __m128 b){
	__m128 temp0 = _mm_sub_ps(a, b);
	temp0 = AbsoluteSSE(temp0);
	temp0 = _mm_cmpgt_ps(temp0, FillSSE(M_EPSILON));
	return !(_mm_movemask_ps(temp0));
}
#endif //DESHI_USE_SSE

#endif //DESHI_MATH_UTILS_H