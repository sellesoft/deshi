#pragma once
#ifndef DESHI_MATH_UTILS_H
#define DESHI_MATH_UTILS_H

#include "kigu/common.h"
#include <cmath>

//// enable SSE if available ////
//NOTE: October 2021 Steam Hardware Survey says 98% of users support SSE4.2 and below; 94% AVX; 84% AVX2; 30% SSE4a; <4% AVX512
//!ref: https://github.com/HandmadeMath/Handmade-Math/blob/master/HandmadeMath.h
#ifndef DESHI_DISABLE_SSE
#  ifdef _MSC_VER
/* MSVC supports SSE in amd64 mode or _M_IX86_FP >= 1 (2 means SSE2) */
#    if defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
//#pragma message("_MSC_VER && (defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1))")
#      define DESHI_USE_SSE 0 //NOTE(sushi) temp disable because its not working properly
#      include <xmmintrin.h>
#      include <pmmintrin.h>
#      include <smmintrin.h>
#    endif 
#  else//_MSC_VER
#    ifdef __SSE__ /* non-MSVC usually #define __SSE__ if it's supported */
//#pragma message("!_MSC_VER && __SSE__")
#      define DESHI_USE_SSE 1
#      include <xmmintrin.h>
#      include <pmmintrin.h>
#      include <smmintrin.h>
#    endif//__SSE__
#  endif//not _MSC_VER
#endif //not DESHI_DISABLE_SSE

//// CRT wrappers ////
global inline f32 Sqrt(f32 a){
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_set_ss(a);
	temp0 = _mm_sqrt_ss(temp0);
	result = _mm_cvtss_f32(temp0);
#else
	result = sqrtf(a);
#endif
	return result;
};

//NOTE this has decently low precision (.0001)
global inline f32 Rsqrt(f32 a){
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_set_ss(a);
	temp0 = _mm_rsqrt_ss(temp0);
	result = _mm_cvtss_f32(temp0);
#else
	result = 1.0f / Sqrt(a);
#endif
	return result;
};

//// SSE funcs //// //TODO look into having SSE constants (epsilon, negative zero)
#if DESHI_USE_SSE
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

//!ref: https://github.com/HandmadeMath/Handmade-Math/blob/master/HandmadeMath.h
global inline __m128 LinearCombineSSE(__m128 vec, __m128 mat_row0, __m128 mat_row1, __m128 mat_row2, __m128 mat_row3){
	__m128 result =             _mm_mul_ps(_mm_shuffle_ps(vec, vec, _MM_SHUFFLE(0,0,0,0)), mat_row0);
	result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1,1,1,1)), mat_row1));
	result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2,2,2,2)), mat_row2));
	result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(vec, vec, _MM_SHUFFLE(3,3,3,3)), mat_row3));
	return result;
}

#define SSEVecShuffleMask(x,y,z,w) (x | (y<<2) | (z<<4) | (w<<6))
#define SSEVecShuffle(a,b, x,y,z,w) _mm_shuffle_ps(a, b, SSEVecShuffleMask(x,y,z,w))
#define SSEMat2Mul(a,b)    _mm_add_ps(_mm_mul_ps(a, SSEVecSwizzle(b, 0,3,0,3)),\
_mm_mul_ps(SSEVecSwizzle(a, 1,0,3,2), SSEVecSwizzle(b, 2,1,2,1)))
#define SSEMat2AdjMul(a,b) _mm_sub_ps(_mm_mul_ps(SSEVecSwizzle(a, 3,3,0,0), b),\
_mm_mul_ps(SSEVecSwizzle(a, 1,1,2,2), SSEVecSwizzle(b, 2,3,0,1)))
#define SSEMat2MulAdj(a,b) _mm_sub_ps(_mm_mul_ps(a, SSEVecSwizzle(b, 3,0,3,0)),\
_mm_mul_ps(SSEVecSwizzle(a, 1,0,3,2), SSEVecSwizzle(b, 2,1,2,1)))
#define SSEVecSwizzle(a, x,y,z,w) _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), SSEVecShuffleMask(x,y,z,w)))
#endif //DESHI_USE_SSE

#endif //DESHI_MATH_UTILS_H