/* deshi Math Module
Notes:
- October 2021 Steam Hardware Survey says 98% of users support SSE4.2 and below; 94% AVX; 84% AVX2; 30% SSE4a; <4% AVX512
- Matrices are in row-major format and all the functionality follows that format
- Matrices are Left-Handed meaning that multiplication travels right and rotation is clockwise 

//// Accessing Matrix Values ////
You can access the values of a matrix using the () operator.
Acessing matrix values starts at zero for both the row and column: 0...n-1 not 1...n
eg: matrix(0,3); This will return the float on the first row and fourth column
eg: matrix(1,1); This will return the float on the second row and second column

Alternatively, you can access the elements directly by their index in a one-dimensional array
This avoids doing one multiplication and one addition but might be confusing to readers of your code
eg: matrix.data[3]; This will return the float on the first row and fourth column

//// Transformation Matrix ////												
You can create a transformation matrix by providing the translation, rotation,
	and scale to the TransformationMatrix() method.
The transformation matrix will follow the format to the below:
|scaleX * rot, rot,          rot,          0|
|rot,          scaleY * rot, rot,          0|
|rot,          rot,          scaleZ * rot, 0|
|translationX, translationY, translationZ, 1|

Index:
@macros
@libc
@simd
@vec2
@vec2i
@vec3
@vec3i
@vec4
@vec4i
@vec_conversions
@vec_hashing
@vec_tostring
@mat3
@mat4
@mat_conversions
@mat_hashing
@mat_tostring
@mat_vec_interactions
@geometry
@camera
@cpp_only
@other

Ref:
https://github.com/HandmadeMath/HandmadeMath
https://github.com/vectorclass/version2 (Agner Fog)
https://intel.com/content/www/us/en/docs/intrinsics-guide/index.html

TODOs:
- API documentation (disables, types, funcs, macros)
- maybe remove division by zero prevention?
- maybe remove dependence on kigu?
- add mat4_look_at_matrix_inverse if possible
*/
#ifndef DESHI_MATH_H
#define DESHI_MATH_H


#include "kigu/common.h"
#include "kigu/profiling.h"


#ifndef DESHI_MATH_DISABLE_HASHING
#  include "kigu/hash.h"
#endif //#ifndef DESHI_MATH_DISABLE_HASHING


#if !defined(DESHI_MATH_DISABLE_LIBC)
#  define DESHI_MATH_USE_LIBC 1
#  include <math.h>
#else
#  define DESHI_MATH_USE_LIBC 0
#endif //#if !defined(DESHI_MATH_DISABLE_LIBC)


#ifndef DESHI_MATH_DISABLE_SSE
#  if defined(_MSC_VER)
/* MSVC supports SSE in amd64 mode or _M_IX86_FP >= 1 (2 means SSE2) */
#    if defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#      define DESHI_MATH_USE_SSE 1
#      include <xmmintrin.h>
#      include <pmmintrin.h>
#      include <smmintrin.h>
#      include <emmintrin.h>
#    endif //#if defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#  else //#if defined(_MSC_VER)
/* non-MSVC usually #define __SSE__ if it's supported */
#    if defined(__SSE__)
#      define DESHI_MATH_USE_SSE 1
#      include <xmmintrin.h>
#      include <pmmintrin.h>
#      include <smmintrin.h>
#      include <emmintrin.h>
#    endif  //#if defined(__SSE__)
#  endif //#else //#if defined(_MSC_VER)
#else //#ifndef DESHI_MATH_DISABLE_SSE
#  define DESHI_MATH_USE_SSE 0
#endif //#else //#ifndef DESHI_MATH_DISABLE_SSE


struct vec2;
struct vec2i;
struct vec3;
struct vec3i;
struct vec4;
struct vec4i;
struct mat3;
struct mat4;


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @macros


#ifdef __cplusplus
#  define DESHI_MATH_FUNC extern "C"
#  define DESHI_MATH_TYPE extern "C"
#else //#ifdef __cplusplus
#  define DESHI_MATH_FUNC static
#  define DESHI_MATH_TYPE
#endif //#else //#ifdef __cplusplus

#define DESHI_PI_F32 3.14159265359f

#define DESHI_DEGREES_TO_RADIANS_F32(angles) ((angles) * (180.0f / DESHI_PI_F32))

#define DESHI_RADIANS_TO_DEGREES_F32(angles) ((angles) * (DESHI_PI_F32 / 180.0f))


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @libc
#if DESHI_MATH_USE_LIBC


#ifndef DESHI_ABS
#  define DESHI_ABS abs
#endif //#ifndef DESHI_ABS

#ifndef DESHI_ABSF
#  define DESHI_ABSF fabs
#endif //#ifndef DESHI_ABSF

#ifndef DESHI_SQRTF
#  define DESHI_SQRTF sqrtf
#endif //#ifndef DESHI_SQRTF

#ifndef DESHI_SINF
#  define DESHI_SINF sinf
#endif //#ifndef DESHI_SINF

#ifndef DESHI_COSF
#  define DESHI_COSF cosf
#endif //#ifndef DESHI_COSF

#ifndef DESHI_TANF
#  define DESHI_TANF tanf
#endif //#ifndef DESHI_TANF

#ifndef DESHI_ASINF
#  define DESHI_ASINF asinf
#endif //#ifndef DESHI_ASINF

#ifndef DESHI_ACOSF
#  define DESHI_ACOSF acosf
#endif //#ifndef DESHI_ACOSF

#ifndef DESHI_ATANF
#  define DESHI_ATANF atanf
#endif //#ifndef DESHI_ATANF

#ifndef DESHI_CEILF
#  define DESHI_CEILF ceilf
#endif //#ifndef DESHI_CEILF

#ifndef DESHI_FLOORF
#  define DESHI_FLOORF floorf
#endif //#ifndef DESHI_FLOORF

#ifndef DESHI_ROUNDF
#  define DESHI_ROUNDF roundf
#endif //#ifndef DESHI_ROUNDF


#endif //#if DESHI_MATH_USE_LIBC
//~////////////////////////////////////////////////////////////////////////////////////////////////
// @simd
#if DESHI_MATH_USE_SSE


#define m128_shuffle_mask(x,y,z,w) ((x) | ((y) << 2) | ((z) << 4) | ((w) << 6))
#define m128_shuffle(a,b, x,y,z,w) _mm_shuffle_ps((a), (b), m128_shuffle_mask((x),(y),(z),(w)))
#define m128_shuffle_0101(a,b) _mm_movelh_ps((a), (b))
#define m128_shuffle_2323(a,b) _mm_movehl_ps((b), (a))
#define m128_swizzle(a, x,y,z,w) _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), m128_shuffle_mask((x),(y),(z),(w))))
#define m128_swizzle_0022(a) _mm_moveldup_ps(a)
#define m128_swizzle_1133(a) _mm_movehdup_ps(a)

#define m128_add_4f32(lhs,rhs) _mm_add_ps((lhs), (rhs))
#define m128_sub_4f32(lhs,rhs) _mm_sub_ps((lhs), (rhs))
#define m128_mul_4f32(lhs,rhs) _mm_mul_ps((lhs), (rhs))
#define m128_div_4f32(lhs,rhs) _mm_div_ps((lhs), (rhs))
#define m128_abs_4f32(lhs) _mm_andnot_ps(_mm_set1_ps(-0.0f), (lhs))
#define m128_negate_4f32(lhs) _mm_sub_ps(_mm_set1_ps(0.0f), (lhs))
#define m128_floor_4f32(lhs) _mm_floor_ps((lhs))
#define m128_ceil_4f32(lhs) _mm_ceil_ps((lhs))
#define m128_round_4f32(lhs) _mm_round_ps((lhs))
#define m128_min_4f32(lhs) _mm_min_ps((lhs))
#define m128_max_4f32(lhs) _mm_max_ps((lhs))
#define m128_set_4f32(a,b,c,d) _mm_set_ps((a), (b), (c), (d))
#define m128_fill_4f32(lhs) _mm_set1_ps((a))

#define m128_add_2f64(lhs,rhs) _mm_add_pd((lhs), (rhs))
#define m128_sub_2f64(lhs,rhs) _mm_sub_pd((lhs), (rhs))
#define m128_mul_2f64(lhs,rhs) _mm_mul_pd((lhs), (rhs))
#define m128_div_2f64(lhs,rhs) _mm_div_pd((lhs), (rhs))
#define m128_abs_2f64(lhs) _mm_andnot_pd(_mm_set1_pd(-0.0), (lhs))
#define m128_negate_2f64(lhs) _mm_sub_pd(_mm_set1_pd(0.0), (lhs))
#define m128_floor_2f64(lhs) _mm_floor_pd((lhs))
#define m128_ceil_2f64(lhs) _mm_ceil_pd((lhs))
#define m128_round_2f64(lhs) _mm_round_pd((lhs))
#define m128_min_2f64(lhs) _mm_min_pd((lhs))
#define m128_max_2f64(lhs) _mm_max_pd((lhs))
#define m128_set_2f64(a,b) _mm_set_pd((a), (b))
#define m128_fill_2f64(lhs) _mm_set1_pd((lhs))

#define m128_add_4s32(lhs,rhs) _mm_add_epi32((lhs), (rhs))
#define m128_sub_4s32(lhs,rhs) _mm_sub_epi32((lhs), (rhs))
#define m128_mul_4s32(lhs,rhs) _mm_mullo_epi32((lhs), (rhs))
#define m128_abs_4s32(lhs)_mm_abs_epi32((lhs))
#define m128_negate_4s32(lhs) _mm_sub_epi32(_mm_set1_epi32(0), (lhs))
#define m128_min_4s32(lhs) _mm_min_epi32((lhs))
#define m128_max_4s32(lhs) _mm_max_epi32((lhs))
#define m128_set_4s32(a,b,c,d) _mm_set_epi32((a), (b), (c), (d))
#define m128_fill_4s32(lhs) _mm_set1_epi32((a))

DESHI_MATH_FUNC b32
m128_equal_4f32(__m128 lhs, __m128 rhs){DPZoneScoped;
	__m128 temp0 = _mm_sub_ps(lhs, rhs);
	temp0 = _mm_andnot_ps(_mm_set1_ps(-0.0f), temp0);
	temp0 = _mm_cmpgt_ps(temp0, _mm_set1_ps(M_EPSILON));
	return !(_mm_movemask_ps(temp0));
}

DESHI_MATH_FUNC b32
m128_equal_4s32(__m128i lhs, __m128i rhs){DPZoneScoped;
	return !(_mm_movemask_epi8(_mm_cmpeq_epi32(lhs, rhs)));
}

DESHI_MATH_FUNC __m128
m128_linear_combine(__m128 vec, __m128 mat_row0, __m128 mat_row1, __m128 mat_row2, __m128 mat_row3){DPZoneScoped;
	__m128 result =                m128_mul_4f32(m128_swizzle(vec, 0,0,0,0), mat_row0);
	result = m128_add_4f32(result, m128_mul_4f32(m128_swizzle(vec, 1,1,1,1), mat_row1));
	result = m128_add_4f32(result, m128_mul_4f32(m128_swizzle(vec, 2,2,2,2), mat_row2));
	result = m128_add_4f32(result, m128_mul_4f32(m128_swizzle(vec, 3,3,3,3), mat_row3));
	return result;
}


#endif //#if DESHI_MATH_USE_SSE
//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec2


DESHI_MATH_TYPE typedef struct
vec2{
	union{
		f32 arr[2];
		struct{ f32 x, y; };
		struct{ f32 r, g; };
		struct{ f32 w, h; };
		struct{ f32 u, v; };
	};
	
#ifdef __cplusplus
	static constexpr vec2 ZERO  = { 0, 0};
	static constexpr vec2 ONE   = { 1, 1};
	static constexpr vec2 LEFT  = {-1, 0};
	static constexpr vec2 RIGHT = { 1, 0};
	static constexpr vec2 DOWN  = { 0,-1};
	static constexpr vec2 UP    = { 0, 1};
	static constexpr vec2 UNITX = { 1, 0};
	static constexpr vec2 UNITY = { 0, 1};
	f32   operator[](u32 axis)const;
	f32&  operator[](u32 axis);
	vec2  operator+ (const vec2& rhs)const;
	void  operator+=(const vec2& rhs);
	vec2  operator- (const vec2& rhs)const;
	void  operator-=(const vec2& rhs);
	vec2  operator* (const vec2& rhs)const;
	void  operator*=(const vec2& rhs);
	vec2  operator* (f32 rhs)const;
	void  operator*=(f32 rhs);
	vec2  operator/ (const vec2& rhs)const;
	void  operator/=(const vec2& rhs);
	vec2  operator/ (f32 rhs)const;
	void  operator/=(f32 rhs);
	vec2  operator- ()const;
	b32   operator==(const vec2& rhs)const;
	b32   operator!=(const vec2& rhs)const;
	vec2  abs()const;
	f32   dot(const vec2& rhs)const;
	vec2  cross()const;
	f32   mag()const;
	f32   mag_sq()const;
	vec2  normalize()const;
	f32   distance(const vec2& rhs)const;
	f32   distance_sq(const vec2& rhs)const;
	f32   projection(const vec2& rhs)const;
	vec2  component(const vec2& rhs)const;
	vec2  midpoint(const vec2& rhs)const;
	f32   slope(const vec2& rhs)const;
	f32   radians_between(const vec2& rhs)const;
	vec2  floor()const;
	vec2  ceil()const;
	vec2  round()const;
	vec2  round_to(u32 place)const;
	vec2  min(const vec2& rhs)const;
	vec2  max(const vec2& rhs)const;
	vec2  clamp(const vec2& min, const vec2& max)const;
	vec2  clamp_min(const vec2& min)const;
	vec2  clamp_max(const vec2& max)const;
	vec2  clamp_mag(f32 min, f32 max)const;
	vec2  nudge(vec2 target, vec2 delta);
	vec2  lerp(vec2 rhs, f32 t);
	vec2  rotate_radians(f32 angle);
	vec2  x_zero()const;
	vec2  y_zero()const;
	vec2  x_only()const;
	vec2  y_only()const;
	vec2  x_negate()const;
	vec2  y_negate()const;
	vec2  x_set(f32 a)const;
	vec2  y_set(f32 a)const;
	vec2  x_add(f32 a)const;
	vec2  y_add(f32 a)const;
	vec2i to_vec2i()const;
	vec3  to_vec3()const;
	vec3i to_vec3i()const;
	vec4  to_vec4()const;
	vec4i to_vec4i()const;
#endif //#ifdef __cplusplus
} vec2;

DESHI_MATH_FUNC inline vec2
Vec2(f32 x, f32 y){
	return vec2{x, y};
}

DESHI_MATH_FUNC inline vec2 vec2_ZERO() { return vec2{ 0, 0}; }
DESHI_MATH_FUNC inline vec2 vec2_ONE()  { return vec2{ 1, 1}; }
DESHI_MATH_FUNC inline vec2 vec2_UP()   { return vec2{ 0, 1}; }
DESHI_MATH_FUNC inline vec2 vec2_DOWN() { return vec2{ 0,-1}; }
DESHI_MATH_FUNC inline vec2 vec2_LEFT() { return vec2{-1, 0}; }
DESHI_MATH_FUNC inline vec2 vec2_RIGHT(){ return vec2{ 1, 0}; }
DESHI_MATH_FUNC inline vec2 vec2_UNITX(){ return vec2{ 1, 0}; }
DESHI_MATH_FUNC inline vec2 vec2_UNITY(){ return vec2{ 0, 1}; }

DESHI_MATH_FUNC inline f32
vec2_index(vec2 lhs, u32 index){DPZoneScoped;
	return lhs.arr[index];
}

#ifdef __cplusplus
inline f32 vec2::
operator[](u32 index)const{DPZoneScoped;
	return this->arr[index];
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline f32& vec2::
operator[](u32 index){DPZoneScoped;
	return this->arr[index];
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_add(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x + rhs.x;
	v.y = lhs.y + rhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
operator+ (const vec2& rhs)const{DPZoneScoped;
	vec2 v;
	v.x = this->x + rhs.x;
	v.y = this->y + rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec2::
operator+=(const vec2& rhs){DPZoneScoped;
	this->x += rhs.x;
	this->y += rhs.y;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_sub(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x - rhs.x;
	v.y = lhs.y - rhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
operator- (const vec2& rhs)const{DPZoneScoped;
	vec2 v;
	v.x = this->x - rhs.x;
	v.y = this->y - rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec2::
operator-=(const vec2& rhs){DPZoneScoped;
	this->x -= rhs.x;
	this->y -= rhs.y;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_mul(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x * rhs.x;
	v.y = lhs.y * rhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
operator* (const vec2& rhs)const{DPZoneScoped;
	vec2 v;
	v.x = this->x * rhs.x;
	v.y = this->y * rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec2::
operator*=(const vec2& rhs){DPZoneScoped;
	this->x *= rhs.x;
	this->y *= rhs.y;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_mul_f32(vec2 lhs, f32 rhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x * rhs;
	v.y = lhs.y * rhs;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
operator* (f32 rhs)const{DPZoneScoped;
	vec2 v;
	v.x = this->x * rhs;
	v.y = this->y * rhs;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec2::
operator*=(f32 rhs){DPZoneScoped;
	this->x *= rhs;
	this->y *= rhs;
}

#ifdef __cplusplus
inline vec2 vec2::
operator* (f32 lhs, vec2 rhs){DPZoneScoped;
	return rhs * lhs;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_div(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x / rhs.x;
	v.y = lhs.y / rhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
operator/ (const vec2& rhs)const{DPZoneScoped;
	vec2 v;
	v.x = this->x / rhs.x;
	v.y = this->y / rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec2::
operator/=(const vec2& rhs){DPZoneScoped;
	this->x /= rhs.x;
	this->y /= rhs.y;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_div_f32(vec2 lhs, f32 rhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x / rhs;
	v.y = lhs.y / rhs;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
operator/ (f32 rhs)const{DPZoneScoped;
	vec2 v;
	v.x = this->x / rhs;
	v.y = this->y / rhs;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec2::
operator/=(f32 rhs){DPZoneScoped;
	this->x /= rhs;
	this->y /= rhs;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_negate(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = -(lhs.x);
	v.y = -(lhs.y);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
operator- ()const{DPZoneScoped;
	vec2 v;
	v.x = -(this->x);
	v.y = -(this->y);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
vec2_equal(vec2 lhs, vec2 rhs){DPZoneScoped;
	return (DESHI_ABSF(lhs.x - rhs.x) < M_EPSILON)
		&& (DESHI_ABSF(lhs.y - rhs.y) < M_EPSILON);
}

#ifdef __cplusplus
inline b32 vec2::
operator==(const vec2& rhs)const{DPZoneScoped;
	return (DESHI_ABSF(this->x - rhs.x) < M_EPSILON)
		&& (DESHI_ABSF(this->y - rhs.y) < M_EPSILON);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
vec2_nequal(vec2 lhs, vec2 rhs){DPZoneScoped;
	return (DESHI_ABSF(lhs.x - rhs.x) > M_EPSILON)
		|| (DESHI_ABSF(lhs.y - rhs.y) > M_EPSILON);
}

#ifdef __cplusplus
inline b32 vec2::
operator!=(const vec2& rhs)const{DPZoneScoped;
	return (DESHI_ABSF(this->x - rhs.x) > M_EPSILON)
		|| (DESHI_ABSF(this->y - rhs.y) > M_EPSILON);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_abs(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = DESHI_ABSF(lhs.x);
	v.y = DESHI_ABSF(lhs.y);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
abs()const{DPZoneScoped;
	vec2 v;
	v.x = DESHI_ABSF(this->x);
	v.y = DESHI_ABSF(this->y);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2_dot(vec2 lhs, vec2 rhs){DPZoneScoped;
	return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}

#ifdef __cplusplus
inline f32 vec2::
dot(const vec2& rhs)const{DPZoneScoped;
	return (this->x * rhs.x) + (this->y * rhs.y);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_cross(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = -(lhs.y);
	v.y =   lhs.x;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
cross()const{DPZoneScoped;
	vec2 v;
	v.x = -(this->y);
	v.y =   this->x;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2_mag(vec2 lhs){DPZoneScoped;
	return DESHI_SQRTF((lhs.x * lhs.x) + (lhs.y * lhs.y));
}

#ifdef __cplusplus
inline f32 vec2::
mag()const{DPZoneScoped;
	return DESHI_SQRTF((this->x * this->x) + (this->y * this->y));
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32 
vec2_mag_sq(vec2 lhs){DPZoneScoped;
	return (lhs.x * lhs.x) + (lhs.y * lhs.y);
}

#ifdef __cplusplus
inline f32 vec2::
mag_sq()const{DPZoneScoped;
	return (this->x * this->x) + (this->y * this->y);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_normalize(vec2 lhs){DPZoneScoped;
	if(lhs.x > m_EPSILON || lhs.y > m_EPSILON){
		return vec2_div_f32(lhs, vec2_mag(lhs));
	}else{
		return lhs;
	}
}

#ifdef __cplusplus
inline vec2 vec2::
normalize()const{DPZoneScoped;
	if(this->x > m_EPSILON || this->y > m_EPSILON){
		return *this / this->mag();
	}else{
		return *this;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2_distance(vec2 lhs, vec2 rhs){DPZoneScoped;
	return vec2_mag(vec2_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec2::
distance(const vec2& rhs)const{DPZoneScoped;
	return (*this - rhs).mag();
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2_distance_sq(vec2 lhs, vec2 rhs){DPZoneScoped;
	return vec2_mag_sq(vec2_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec2::
distance_sq(const vec2& rhs)const{DPZoneScoped;
	return (*this - rhs).mag_sq();
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2_projection(vec2 lhs, vec2 rhs){DPZoneScoped;
	f32 m = vec2_mag(lhs);
	if(m > M_EPSILON){
		return vec2_dot(lhs,rhs) / m;
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec2::
projection(const vec2& rhs)const{DPZoneScoped;
	f32 m = this->mag();
	if(m > M_EPSILON){
		return this->dot(rhs) / m;
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_component(vec2 lhs, vec2 rhs){DPZoneScoped;
	return vec2_mul_f32(vec2_normalize(rhs), vec2_projection(lhs,rhs));
}

#ifdef __cplusplus
inline vec2 vec2::
component(const vec2& rhs)const{DPZoneScoped;
	return rhs.normalize() * this->projection(rhs);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_midpoint(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = (lhs.x + rhs.x) / 2.0f;
	v.y = (lhs.y + rhs.y) / 2.0f;
	return v;
}

#ifdef __cplusplus
inline vec2
midpoint(const vec2& rhs)const{DPZoneScoped;
	vec2 v;
	v.x = (this->x + rhs.x) / 2.0f;
	v.y = (this->y + rhs.y) / 2.0f;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2_slope(vec2 lhs, vec2 rhs){DPZoneScoped;
	return (rhs.y - lhs.y) / (rhs.x - lhs.x);
}

#ifdef __cplusplus
inline f32 vec2::
slope(const vec2& rhs){DPZoneScoped;
	return (rhs.y - this->y) / (rhs.x - this->x);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2_radians_between(vec2 lhs, vec2 rhs){DPZoneScoped;
	f32 m = vec2_mag(lhs) * vec2_mag(rhs);
	if(m > M_EPSILON){
		return DESHI_ACOSF(vec2_dot(rhs) / m);
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec2::
radians_between(const vec2& rhs)const{DPZoneScoped;
	f32 m = this->mag() * rhs.mag();
	if(m > M_EPSILON){
		return DESHI_ACOSF(this->dot(rhs) / m);
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_floor(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = DESHI_FLOORF(lhs.x);
	v.y = DESHI_FLOORF(lhs.y);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
floor()const{DPZoneScoped;
	vec2 v;
	v.x = DESHI_FLOORF(this->x);
	v.y = DESHI_FLOORF(this->y);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec2
floor(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = DESHI_FLOORF(lhs.x);
	v.y = DESHI_FLOORF(lhs.y);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_ceil(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = DESHI_CEILF(lhs.x);
	v.y = DESHI_CEILF(lhs.y);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
ceil()const{DPZoneScoped;
	vec2 v;
	v.x = DESHI_CEILF(this->x);
	v.y = DESHI_CEILF(this->y);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec2
ceil(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = DESHI_CEILF(lhs.x);
	v.y = DESHI_CEILF(lhs.y);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_round(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = DESHI_ROUNDF(lhs.x);
	v.y = DESHI_ROUNDF(lhs.y);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
round()const{DPZoneScoped;
	vec2 v;
	v.x = DESHI_ROUNDF(this->x);
	v.y = DESHI_ROUNDF(this->y);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec2
round(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = DESHI_ROUNDF(lhs.x);
	v.y = DESHI_ROUNDF(lhs.y);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_round_to(vec2 lhs, u32 place){DPZoneScoped;
	vec2 v;
	v.x = DESHI_FLOORF(lhs.x * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.y = DESHI_FLOORF(lhs.y * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
round_to(u32 place)const{DPZoneScoped;
	vec2 v;
	v.x = DESHI_FLOORF(this->x * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.y = DESHI_FLOORF(this->y * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_min(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
min(const vec2& rhs){DPZoneScoped;
	vec2 v;
	v.x = (this->x < rhs.x) ? this->x : rhs.x;
	v.y = (this->y < rhs.y) ? this->y : rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec2
min(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_max(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
max(const vec2& rhs){DPZoneScoped;
	vec2 v;
	v.x = (this->x > rhs.x) ? this->x : rhs.x;
	v.y = (this->y > rhs.y) ? this->y : rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec2
max(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_clamp(vec2 value, vec2 min, vec2 max){DPZoneScoped;
	vec2 v;
	v.x = (value.x < min.x) ? min.x : ((value.x > max.x) ? max.x : value.x);
	v.y = (value.y < min.y) ? min.y : ((value.y > max.y) ? max.y : value.y);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
clamp(const vec2& min, const vec2& max){DPZoneScoped;
	vec2 v;
	v.x = (this->x < min.x) ? min.x : ((this->x > max.x) ? max.x : value.x);
	v.y = (this->y < min.y) ? min.y : ((this->y > max.y) ? max.y : value.y);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec2
clamp(vec2 value, vec2 min, vec2 max){DPZoneScoped;
	vec2 v;
	v.x = (value.x < min.x) ? min.x : ((value.x > max.x) ? max.x : value.x);
	v.y = (value.y < min.y) ? min.y : ((value.y > max.y) ? max.y : value.y);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_clamp_min(vec2 value, vec2 min){DPZoneScoped;
	vec2 v;
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
clamp_min(const vec2& min){DPZoneScoped;
	vec2 v;
	v.x = (this->x < min.x) ? min.x : this->x;
	v.y = (this->y < min.y) ? min.y : this->y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec2
clamp_min(vec2 value, vec2 min){DPZoneScoped;
	vec2 v;
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_clamp_max(vec2 value, vec2 max){DPZoneScoped;
	vec2 v;
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
clamp_max(const vec2& maX){DPZoneScoped;
	vec2 v;
	v.x = (this->x > max.x) ? max.x : this->x;
	v.y = (this->y > max.y) ? max.y : this->y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec2
clamp_max(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_clamp_mag(vec2 lhs, f32 min, f32 max){DPZoneScoped;
	f32 m = vec2_mag(lhs);
	if      (m < min){
		return vec2_mul_f32(vec2_normalized(lhs), min);
	}else if(m > max){
		return vec2_mul_f32(vec2_normalized(lhs), max);
	}else{
		return lhs;
	}
}

#ifdef __cplusplus
inline vec2 vec2::
clamp_mag(f32 min, f32 max)const{DPZoneScoped;
	f32 m = this->mag();
	if(m < min){
		return this->normalize() * min;
	}else if(m > max){
		return this->normalize() * max;
	}else{
		return *this;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_nudge(vec2 value, vec2 target, vec2 delta){DPZoneScoped;
	vec2 v;
	v.x = (value.x < target.x) ? ((value.x + delta.x < target.x) ? value.x + delta.x : target.x) : ((value.x - delta.x > target.x) ? value.x - delta.x : target.x);
	v.y = (value.y < target.y) ? ((value.y + delta.y < target.y) ? value.y + delta.y : target.y) : ((value.y - delta.y > target.y) ? value.y - delta.y : target.y);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
nudge(vec2 target, vec2 delta){DPZoneScoped;
	vec2 v;
	v.x = (this->x < target.x) ? ((this->x + delta.x < target.x) ? this->x + delta.x : target.x) : ((this->x - delta.x > target.x) ? this->x - delta.x : target.x);
	v.y = (this->y < target.y) ? ((this->y + delta.y < target.y) ? this->y + delta.y : target.y) : ((this->y - delta.y > target.y) ? this->y - delta.y : target.y);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec2
nudge(vec2 value, vec2 target, vec2 delta){DPZoneScoped;
	vec2 v;
	v.x = (value.x < target.x) ? ((value.x + delta.x < target.x) ? value.x + delta.x : target.x) : ((value.x - delta.x > target.x) ? value.x - delta.x : target.x);
	v.y = (value.y < target.y) ? ((value.y + delta.y < target.y) ? value.y + delta.y : target.y) : ((value.y - delta.y > target.y) ? value.y - delta.y : target.y);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_lerp(vec2 lhs, vec2 rhs, f32 t){DPZoneScoped;
	vec2 v;
	v.x = (lhs.x * (1.0f - t)) + (rhs.x * t);
	v.y = (lhs.y * (1.0f - t)) + (rhs.y * t);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
lerp(vec2 rhs, f32 t){DPZoneScoped;
	vec2 v;
	v.x = (this->x * (1.0f - t)) + (rhs.x * t);
	v.y = (this->y * (1.0f - t)) + (rhs.y * t);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec2
lerp(vec2 lhs, vec2 rhs, f32 t){DPZoneScoped;
	vec2 v;
	v.x = (lhs.x * (1.0f - t)) + (rhs.x * t);
	v.y = (lhs.y * (1.0f - t)) + (rhs.y * t);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_rotate_radians(vec2 lhs, f32 angle){
	vec2 v;
	v.x = (lhs.x * DESHI_COSF(angle)) - (lhs.y * DESHI_SINF(angle));
	v.y = (lhs.x * DESHI_SINF(angle)) - (lhs.y * DESHI_COSF(angle));
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
rotate_radians(f32 angle){
	vec2 v;
	v.x = (this->x * DESHI_COSF(angle)) - (this->y * DESHI_SINF(angle));
	v.y = (this->x * DESHI_SINF(angle)) - (this->y * DESHI_COSF(angle));
	return v;
}
#endif //#ifdef __cplusplus

#define vec2_rotate_degrees(lhs,angle) vec2_rotate_radians((lhs), DESHI_DEGREES_TO_RADIANS_F32(angle))

DESHI_MATH_FUNC inline vec2
vec2_x_zero(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = 0;
	v.y = lhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
x_zero()const{DPZoneScoped;
	vec2 v;
	v.x = 0;
	v.y = this->y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_y_zero(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x;
	v.y = 0;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
y_zero()const{DPZoneScoped;
	vec2 v;
	v.x = this->x;
	v.y = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_x_only(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x;
	v.y = 0;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
x_only()const{DPZoneScoped;
	vec2 v;
	v.x = this->x;
	v.y = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_y_only(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = 0;
	v.y = lhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
y_only()const{DPZoneScoped;
	vec2 v;
	v.x = 0;
	v.y = this->y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_x_negate(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = -(lhs.x);
	v.y =   lhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
x_negate()const{DPZoneScoped;
	vec2 v;
	v.x = -(this->x);
	v.y =   this->y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_y_negate(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x =   lhs.x;
	v.y = -(lhs.y);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
y_negate()const{DPZoneScoped;
	vec2 v;
	v.x =   this->x;
	v.y = -(this->y);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_x_set(vec2 lhs, f32 a){DPZoneScoped;
	vec2 v;
	v.x = a;
	v.y = lhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
x_set(f32 a)const{DPZoneScoped;
	vec2 v;
	v.x = a;
	v.y = this->y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_y_set(vec2 lhs, f32 a){DPZoneScoped;
	vec2 v;
	v.x = lhs.x;
	v.y = a;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
y_set(f32 a)const{DPZoneScoped;
	vec2 v;
	v.x = this->x;
	v.y = a;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_x_add(vec2 lhs, f32 a){DPZoneScoped;
	vec2 v;
	v.x = lhs.x + a;
	v.y = lhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
x_add(f32 a)const{DPZoneScoped;
	vec2 v;
	v.x = this->x + a;
	v.y = this->y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2
vec2_y_add(vec2 lhs, f32 a){DPZoneScoped;
	vec2 v;
	v.x = lhs.x;
	v.y = lhs.y + a;
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
y_add(f32 a)const{DPZoneScoped;
	vec2 v;
	v.x = this->x;
	v.y = this->y + a;
	return v;
}
#endif //#ifdef __cplusplus


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec2i


DESHI_MATH_TYPE typedef struct
vec2i{
	union{
		s32 arr[2];
		struct{ s32 x, y; };
		struct{ s32 r, g; };
		struct{ s32 w, h; };
		struct{ s32 u, v; };
	};
	
#ifdef __cplusplus
	static constexpr vec2i ZERO  = { 0, 0};
	static constexpr vec2i ONE   = { 1, 1};
	static constexpr vec2i UP    = { 0, 1};
	static constexpr vec2i DOWN  = { 0,-1};
	static constexpr vec2i LEFT  = {-1, 0};
	static constexpr vec2i RIGHT = { 1, 0};
	static constexpr vec2i UNITX = { 1, 0};
	static constexpr vec2i UNITY = { 0, 1};
	s32   operator[](u32 index)const;
	s32&  operator[](u32 index);
	vec2i operator+ (const vec2i& rhs)const;
	void  operator+=(const vec2i& rhs);
	vec2i operator- (const vec2i& rhs)const;
	void  operator-=(const vec2i& rhs);
	vec2i operator* (const vec2i& rhs)const;
	void  operator*=(const vec2i& rhs);
	vec2i operator* (f32 rhs)const;
	void  operator*=(f32 rhs);
	vec2i operator/ (const vec2i& rhs)const;
	void  operator/=(const vec2i& rhs);
	vec2i operator/ (f32 rhs)const;
	void  operator/=(f32 rhs);
	vec2i operator- ()const;
	b32   operator==(const vec2i& rhs)const;
	b32   operator!=(const vec2i& rhs)const;
	vec2i abs()const;
	f32   dot(const vec2i& rhs)const;
	vec2i cross()const;
	f32   mag()const;
	f32   mag_sq()const;
	vec2i normalize()const;
	f32   distance(const vec2i& rhs)const;
	f32   distance_sq(const vec2i& rhs)const;
	f32   projection(const vec2i& rhs)const;
	vec2i component(const vec2i& rhs)const;
	vec2i midpoint(const vec2i& rhs)const;
	f32   radians_between(const vec2i& rhs)const;
	vec2i min(const vec2i& rhs)const;
	vec2i max(const vec2i& rhs)const;
	vec2i clamp(const vec2i& min, const vec2i& max)const;
	vec2i clamp_min(const vec2i& min)const;
	vec2i clamp_max(const vec2i& max)const;
	vec2i clamp_mag(f32 min, f32 max)const;
	vec2i nudge(vec2i target, vec2i delta);
	vec2i lerp(vec2i rhs, f32 t);
	vec2i rotate_radians(f32 angle);
	vec2i x_zero()const;
	vec2i y_zero()const;
	vec2i x_only()const;
	vec2i y_only()const;
	vec2i x_negate()const;
	vec2i y_negate()const;
	vec2i x_set(f32 a)const;
	vec2i y_set(f32 a)const;
	vec2i x_add(f32 a)const;
	vec2i y_add(f32 a)const;
	vec2  to_vec2()const;
	vec3  to_vec3()const;
	vec3i to_vec3i()const;
	vec4  to_vec4()const;
	vec4i to_vec4i()const;
#endif //#ifdef __cplusplus
} vec2i;

DESHI_MATH_FUNC inline vec2i
Vec2i(s32 x, s32 y){
	return vec2i{x, y};
}

DESHI_MATH_FUNC inline vec2i vec2i_ZERO() { return vec2i{ 0, 0}; }
DESHI_MATH_FUNC inline vec2i vec2i_ONE()  { return vec2i{ 1, 1}; }
DESHI_MATH_FUNC inline vec2i vec2i_UP()   { return vec2i{ 0, 1}; }
DESHI_MATH_FUNC inline vec2i vec2i_DOWN() { return vec2i{ 0,-1}; }
DESHI_MATH_FUNC inline vec2i vec2i_LEFT() { return vec2i{-1, 0}; }
DESHI_MATH_FUNC inline vec2i vec2i_RIGHT(){ return vec2i{ 1, 0}; }
DESHI_MATH_FUNC inline vec2i vec2i_UNITX(){ return vec2i{ 1, 0}; }
DESHI_MATH_FUNC inline vec2i vec2i_UNITY(){ return vec2i{ 0, 1}; }

DESHI_MATH_FUNC inline s32
vec2i_index(vec2i lhs, u32 index){DPZoneScoped;
	return lhs.arr[index];
}

#ifdef __cplusplus
inline s32 vec2i::
operator[](u32 index)const{DPZoneScoped;
	return this->arr[index];
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline s32& vec2i::
operator[](u32 index){DPZoneScoped;
	return this->arr[index];
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_add(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = lhs.x + rhs.x;
	v.y = lhs.y + rhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
operator+ (const vec2i& rhs)const{DPZoneScoped;
	vec2i v;
	v.x = this->x + rhs.x;
	v.y = this->y + rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec2i::
operator+=(const vec2i& rhs){DPZoneScoped;
	this->x += rhs.x;
	this->y += rhs.y;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_sub(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = lhs.x - rhs.x;
	v.y = lhs.y - rhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
operator- (const vec2i& rhs)const{DPZoneScoped;
	vec2i v;
	v.x = this->x - rhs.x;
	v.y = this->y - rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec2i::
operator-=(const vec2i& rhs){DPZoneScoped;
	this->x -= rhs.x;
	this->y -= rhs.y;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_mul(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = lhs.x * rhs.x;
	v.y = lhs.y * rhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
operator* (const vec2i& rhs)const{DPZoneScoped;
	vec2i v;
	v.x = this->x * rhs.x;
	v.y = this->y * rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec2i::
operator*=(const vec2i& rhs){DPZoneScoped;
	this->x *= rhs.x;
	this->y *= rhs.y;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_mul_f32(vec2i lhs, f32 rhs){DPZoneScoped;
	vec2i v;
	v.x = (s32)((f32)lhs.x * rhs);
	v.y = (s32)((f32)lhs.y * rhs);
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
operator* (f32 rhs)const{DPZoneScoped;
	vec2i v;
	v.x = (s32)((f32)this->x * rhs);
	v.y = (s32)((f32)this->y * rhs);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec2i::
operator*=(f32 rhs){DPZoneScoped;
	this->x = (s32)((f32)this->x * rhs);
	this->y = (s32)((f32)this->y * rhs);
}

#ifdef __cplusplus
inline vec2i vec2i::
operator* (f32 lhs, vec2i rhs){DPZoneScoped;
	return rhs * lhs;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_div(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = lhs.x / rhs.x;
	v.y = lhs.y / rhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
operator/ (const vec2i& rhs)const{DPZoneScoped;
	vec2i v;
	v.x = this->x / rhs.x;
	v.y = this->y / rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec2i::
operator/=(const vec2i& rhs){DPZoneScoped;
	this->x /= rhs.x;
	this->y /= rhs.y;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_div_f32(vec2i lhs, f32 rhs){DPZoneScoped;
	vec2i v;
	v.x = (s32)((f32)lhs.x / rhs);
	v.y = (s32)((f32)lhs.y / rhs);
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
operator/ (f32 rhs)const{DPZoneScoped;
	vec2i v;
	v.x = (s32)((f32)this->x / rhs);
	v.y = (s32)((f32)this->y / rhs);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec2i::
operator/=(f32 rhs){DPZoneScoped;
	this->x = (s32)((f32)this->x / rhs);
	this->y = (s32)((f32)this->y / rhs);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_negate(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = -(lhs.x);
	v.y = -(lhs.y);
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
operator- ()const{DPZoneScoped;
	vec2i v;
	v.x = -(this->x);
	v.y = -(this->y);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
vec2i_equal(vec2i lhs, vec2i rhs){DPZoneScoped;
	return lhs.x == rhs.x
		&& lhs.y == rhs.y;
}

#ifdef __cplusplus
inline b32 vec2i::
operator==(const vec2i& rhs)const{DPZoneScoped;
	return this->x == rhs.x
		&& this->y == rhs.y;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
vec2i_nequal(vec2i lhs, vec2i rhs){DPZoneScoped;
	return lhs.x != rhs.x
		&& lhs.y != rhs.y;
}

#ifdef __cplusplus
inline b32 vec2i::
operator!=(const vec2i& rhs)const{DPZoneScoped;
	return this->x != rhs.x
		&& this->y != rhs.y;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_abs(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = DESHI_ABS(lhs.x);
	v.y = DESHI_ABS(lhs.y);
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
abs()const{DPZoneScoped;
	vec2i v;
	v.x = DESHI_ABS(this->x);
	v.y = DESHI_ABS(this->y);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2i_dot(vec2i lhs, vec2i rhs){DPZoneScoped;
	return (f32)((lhs.x * rhs.x) + (lhs.y * rhs.y));
}

#ifdef __cplusplus
inline f32 vec2i::
dot(const vec2i& rhs)const{DPZoneScoped;
	return (f32)((this->x * rhs.x) + (this->y * rhs.y));
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_cross(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = -(lhs.y);
	v.y =   lhs.x;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
cross()const{DPZoneScoped;
	vec2i v;
	v.x = -(this->y);
	v.y =   this->x;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2i_mag(vec2i lhs){DPZoneScoped;
	return DESHI_SQRTF((f32)((lhs.x * lhs.x) + (lhs.y * lhs.y)));
}

#ifdef __cplusplus
inline f32 vec2i::
mag()const{DPZoneScoped;
	return DESHI_SQRTF((f32)((this->x * this->x) + (this->y * this->y)));
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2i_mag_sq(vec2i lhs){DPZoneScoped;
	return (f32)((lhs.x * lhs.x) + (lhs.y * lhs.y));
}

#ifdef __cplusplus
inline f32 vec2i::
mag_sq()const{DPZoneScoped;
	return (f32)((this->x * this->x) + (this->y * this->y));
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_normalize(vec2i lhs){DPZoneScoped;
	if(lhs.x != 0 || lhs.y != 0){
		return vec2i_div_f32(lhs, vec2i_mag(lhs));
	}else{
		return lhs;
	}
}

#ifdef __cplusplus
inline vec2i vec2i::
normalize()const{DPZoneScoped;
	if(this->x != 0 || this->y != 0){
		return *this / this->mag();
	}else{
		return *this;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2i_distance(vec2i lhs, vec2i rhs){DPZoneScoped;
	return vec2i_mag(vec2i_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec2i::
distance(const vec2i& rhs)const{DPZoneScoped;
	return (*this - rhs).mag();
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2i_distance_sq(vec2i lhs, vec2i rhs){DPZoneScoped;
	return vec2i_mag_sq(vec2i_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec2i::
distance_sq(const vec2i& rhs)const{DPZoneScoped;
	return (*this - rhs).mag_sq();
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2i_projection(vec2i lhs, vec2i rhs){DPZoneScoped;
	f32 m = vec2i_mag(lhs);
	if(m > M_EPSILON){
		return vec2i_dot(lhs,rhs) / m;
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec2i::
projection(const vec2i& rhs)const{DPZoneScoped;
	f32 m = this->mag();
	if(m > M_EPSILON){
		return this->dot(rhs) / m;
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_component(vec2i lhs, vec2i rhs){DPZoneScoped;
	return vec2i_mul_f32(vec2i_normalize(rhs), vec2i_projection(lhs,rhs));
}

#ifdef __cplusplus
inline vec2i vec2i::
component(const vec2i& rhs)const{DPZoneScoped;
	return rhs.normalize() * this->projection(rhs);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_midpoint(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = (lhs.x + rhs.x) / 2;
	v.y = (lhs.y + rhs.y) / 2;
	return v;
}

#ifdef __cplusplus
inline vec2i
midpoint(const vec2i& rhs)const{DPZoneScoped;
	vec2i v;
	v.x = (this->x + rhs.x) / 2;
	v.y = (this->y + rhs.y) / 2;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2i_slope(vec2i lhs, vec2i rhs){DPZoneScoped;
	return (f32)(rhs.y - lhs.y) / (f32)(rhs.x - lhs.x);
}

#ifdef __cplusplus
inline f32 vec2i::
slope(const vec2i& rhs){DPZoneScoped;
	return (f32)(rhs.y - this->y) / (f32)(rhs.x - this->x);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec2i_radians_between(vec2i lhs, vec2i rhs){DPZoneScoped;
	f32 m = vec2i_mag(lhs) * vec2i_mag(rhs);
	if(m > M_EPSILON){
		return DESHI_ACOSF(vec2i_dot(lhs,rhs) / m);
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec2i::
radians_between(const vec2i& rhs)const{DPZoneScoped;
	f32 m = this->mag() * rhs.mag();
	if(m > M_EPSILON){
		return DESHI_ACOSF(this->dot(rhs) / m);
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_min(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
min(const vec2i& rhs){DPZoneScoped;
	vec2i v;
	v.x = (this->x < rhs.x) ? this->x : rhs.x;
	v.y = (this->y < rhs.y) ? this->y : rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec2i
min(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_max(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
max(const vec2i& rhs){DPZoneScoped;
	vec2i v;
	v.x = (this->x > rhs.x) ? this->x : rhs.x;
	v.y = (this->y > rhs.y) ? this->y : rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec2i
max(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_clamp(vec2i value, vec2i min, vec2i max){DPZoneScoped;
	vec2i v;
	v.x = (value.x < min.x) ? min.x : ((value.x > max.x) ? max.x : value.x);
	v.y = (value.y < min.y) ? min.y : ((value.y > max.y) ? max.y : value.y);
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
clamp(const vec2i& min, const vec2i& max){DPZoneScoped;
	vec2i v;
	v.x = (this->x < min.x) ? min.x : ((this->x > max.x) ? max.x : value.x);
	v.y = (this->y < min.y) ? min.y : ((this->y > max.y) ? max.y : value.y);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec2i
clamp(vec2i value, vec2i min, vec2i max){DPZoneScoped;
	vec2i v;
	v.x = (value.x < min.x) ? min.x : ((value.x > max.x) ? max.x : value.x);
	v.y = (value.y < min.y) ? min.y : ((value.y > max.y) ? max.y : value.y);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_clamp_min(vec2i value, vec2i min){DPZoneScoped;
	vec2i v;
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
clamp_min(const vec2i& min){DPZoneScoped;
	vec2i v;
	v.x = (this->x < min.x) ? min.x : this->x;
	v.y = (this->y < min.y) ? min.y : this->y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec2i
clamp_min(vec2i value, vec2i min){DPZoneScoped;
	vec2i v;
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_clamp_max(vec2i value, vec2i max){DPZoneScoped;
	vec2i v;
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
clamp_max(const vec2i& maX){DPZoneScoped;
	vec2i v;
	v.x = (this->x > max.x) ? max.x : this->x;
	v.y = (this->y > max.y) ? max.y : this->y;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec2i
clamp_max(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_clamp_mag(vec2i lhs, f32 min, f32 max){DPZoneScoped;
	f32 m = vec2i_mag(lhs);
	if      (m < min){
		return vec2i_mul_f32(vec2i_normalize(lhs), min);
	}else if(m > max){
		return vec2i_mul_f32(vec2i_normalize(lhs), max);
	}else{
		return lhs;
	}
}

#ifdef __cplusplus
inline vec2i vec2i::
clamp_mag(f32 min, f32 max)const{DPZoneScoped;
	f32 m = this->mag();
	if(m < min){
		return this->normalize() * min;
	}else if(m > max){
		return this->normalize() * max;
	}else{
		return *this;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_nudge(vec2i value, vec2i target, vec2i delta){DPZoneScoped;
	vec2i v;
	v.x = (value.x < target.x) ? ((value.x + delta.x < target.x) ? value.x + delta.x : target.x) : ((value.x - delta.x > target.x) ? value.x - delta.x : target.x);
	v.y = (value.y < target.y) ? ((value.y + delta.y < target.y) ? value.y + delta.y : target.y) : ((value.y - delta.y > target.y) ? value.y - delta.y : target.y);
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
nudge(vec2i target, vec2i delta){DPZoneScoped;
	vec2i v;
	v.x = (this->x < target.x) ? ((this->x + delta.x < target.x) ? this->x + delta.x : target.x) : ((this->x - delta.x > target.x) ? this->x - delta.x : target.x);
	v.y = (this->y < target.y) ? ((this->y + delta.y < target.y) ? this->y + delta.y : target.y) : ((this->y - delta.y > target.y) ? this->y - delta.y : target.y);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec2i
nudge(vec2i value, vec2i target, vec2i delta){DPZoneScoped;
	vec2i v;
	v.x = (value.x < target.x) ? ((value.x + delta.x < target.x) ? value.x + delta.x : target.x) : ((value.x - delta.x > target.x) ? value.x - delta.x : target.x);
	v.y = (value.y < target.y) ? ((value.y + delta.y < target.y) ? value.y + delta.y : target.y) : ((value.y - delta.y > target.y) ? value.y - delta.y : target.y);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_lerp(vec2i lhs, vec2i rhs, f32 t){DPZoneScoped;
	vec2i v;
	v.x = (s32)(((f32)lhs.x * (1.0f - t)) + ((f32)rhs.x * t));
	v.y = (s32)(((f32)lhs.y * (1.0f - t)) + ((f32)rhs.y * t));
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
lerp(vec2i rhs, f32 t){DPZoneScoped;
	vec2i v;
	v.x = (s32)(((f32)this->x * (1.0f - t)) + ((f32)rhs.x * t));
	v.y = (s32)(((f32)this->y * (1.0f - t)) + ((f32)rhs.y * t));
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec2i
lerp(vec2i lhs, vec2i rhs, f32 t){DPZoneScoped;
	vec2i v;
	v.x = (s32)(((f32)lhs.x * (1.0f - t)) + ((f32)rhs.x * t));
	v.y = (s32)(((f32)lhs.y * (1.0f - t)) + ((f32)rhs.y * t));
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_rotate_radians(vec2i lhs, f32 angle){
	vec2i v;
	v.x = (s32)(((f32)lhs.x * DESHI_COSF(angle)) - ((f32)lhs.y * DESHI_SINF(angle)));
	v.y = (s32)(((f32)lhs.x * DESHI_SINF(angle)) - ((f32)lhs.y * DESHI_COSF(angle)));
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
rotate_radians(f32 angle){
	vec2i v;
	v.x = (s32)(((f32)this->x * DESHI_COSF(angle)) - ((f32)this->y * DESHI_SINF(angle)));
	v.y = (s32)(((f32)this->x * DESHI_SINF(angle)) - ((f32)this->y * DESHI_COSF(angle)));
	return v;
}
#endif //#ifdef __cplusplus

#define vec2i_rotate_degrees(lhs,angle) vec2i_rotate_radians((lhs), DESHI_DEGREES_TO_RADIANS_F32(angle))

DESHI_MATH_FUNC inline vec2i
vec2i_x_zero(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = 0;
	v.y = lhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
x_zero()const{DPZoneScoped;
	vec2i v;
	v.x = 0;
	v.y = this->y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_y_zero(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = lhs.x;
	v.y = 0;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
y_zero()const{DPZoneScoped;
	vec2i v;
	v.x = this->x;
	v.y = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_x_only(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = lhs.x;
	v.y = 0;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
x_only()const{DPZoneScoped;
	vec2i v;
	v.x = this->x;
	v.y = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_y_only(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = 0;
	v.y = lhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
y_only()const{DPZoneScoped;
	vec2i v;
	v.x = 0;
	v.y = this->y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_x_negate(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = -(lhs.x);
	v.y =   lhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
x_negate()const{DPZoneScoped;
	vec2i v;
	v.x = -(this->x);
	v.y =   this->y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_y_negate(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x =   lhs.x;
	v.y = -(lhs.y);
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
y_negate()const{DPZoneScoped;
	vec2i v;
	v.x =   this->x;
	v.y = -(this->y);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_x_set(vec2i lhs, s32 a){DPZoneScoped;
	vec2i v;
	v.x = a;
	v.y = lhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
x_set(s32 a)const{DPZoneScoped;
	vec2i v;
	v.x = a;
	v.y = this->y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_y_set(vec2i lhs, s32 a){DPZoneScoped;
	vec2i v;
	v.x = lhs.x;
	v.y = a;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
y_set(s32 a)const{DPZoneScoped;
	vec2i v;
	v.x = this->x;
	v.y = a;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_x_add(vec2i lhs, s32 a){DPZoneScoped;
	vec2i v;
	v.x = lhs.x + a;
	v.y = lhs.y;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
x_add(s32 a)const{DPZoneScoped;
	vec2i v;
	v.x = this->x + a;
	v.y = this->y;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec2i
vec2i_y_add(vec2i lhs, s32 a){DPZoneScoped;
	vec2i v;
	v.x = lhs.x;
	v.y = lhs.y + a;
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
y_add(s32 a)const{DPZoneScoped;
	vec2i v;
	v.x = this->x;
	v.y = this->y + a;
	return v;
}
#endif //#ifdef __cplusplus


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec3


DESHI_MATH_TYPE typedef struct
vec3{
	union{
		f32 arr[3];
		struct{ f32 x, y, z; };
		struct{ f32 u, v, w; };
		struct{ f32 r, g, b; };
		struct{ vec2 xy; f32 _z0; };
		struct{ f32 _x0; vec2 yz; };
	};
	
#ifdef __cplusplus
	static constexpr vec3 ZERO     = { 0, 0, 0};
	static constexpr vec3 ONE      = { 1, 1, 1};
	static constexpr vec3 LEFT     = {-1, 0, 0};
	static constexpr vec3 RIGHT    = { 1, 0, 0};
	static constexpr vec3 DOWN     = { 0,-1, 0};
	static constexpr vec3 UP       = { 0, 1, 0};
	static constexpr vec3 BACKWARD = { 0, 0,-1};
	static constexpr vec3 FORWARD  = { 0, 0, 1};
	static constexpr vec3 UNITX    = { 1, 0, 0};
	static constexpr vec3 UNITY    = { 0, 1, 0};
	static constexpr vec3 UNITZ    = { 0, 0, 1};
	f32   operator[](u32 index)const;
	f32&  operator[](u32 index);
	vec3  operator+ (const vec3& rhs)const;
	void  operator+=(const vec3& rhs);
	vec3  operator- (const vec3& rhs)const;
	void  operator-=(const vec3& rhs);
	vec3  operator* (const vec3& rhs)const;
	void  operator*=(const vec3& rhs);
	vec3  operator* (f32 rhs)const;
	void  operator*=(f32 rhs);
	vec4  operator* (const mat3& rhs)const;
	void  operator*=(const mat3& rhs);
	vec3  operator/ (const vec3& rhs)const;
	void  operator/=(const vec3& rhs);
	vec3  operator/ (f32 rhs)const;
	void  operator/=(f32 rhs);
	vec3  operator- ()const;
	b32   operator==(const vec3& rhs)const;
	b32   operator!=(const vec3& rhs)const;
	vec3  abs()const;
	f32   dot(const vec3& rhs)const;
	f32   cross(const vec3& rhs)const;
	f32   mag()const;
	f32   mag_sq()const;
	vec3  normalize()const;
	f32   distance(const vec3& rhs)const;
	f32   distance_sq(const vec3& rhs)const;
	f32   projection(const vec3& rhs)const;
	vec3  component(const vec3& rhs)const;
	vec3  midpoint(const vec3& rhs)const;
	f32   radians_between(const vec3& rhs)const;
	vec3  floor()const;
	vec3  ceil()const;
	vec3  round()const;
	vec3  round_to(u32 place)const;
	vec3  min(const vec3& rhs)const;
	vec3  max(const vec3& rhs)const;
	vec3  clamp(const vec3& min, const vec3& max)const;
	vec3  clamp_min(const vec3& min)const;
	vec3  clamp_max(const vec3& max)const;
	vec3  clamp_mag(f32 min, f32 max)const;
	vec3  nudge(vec3 target, vec3 delta);
	vec3  lerp(vec3 rhs, f32 t);
	vec3  x_zero()const;
	vec3  y_zero()const;
	vec3  z_zero()const;
	vec3  x_only()const;
	vec3  y_only()const;
	vec3  z_only()const;
	vec3  x_negate()const;
	vec3  y_negate()const;
	vec3  z_negate()const;
	vec3  x_set(f32 a)const;
	vec3  y_set(f32 a)const;
	vec3  z_set(f32 a)const;
	vec3  x_add(f32 a)const;
	vec3  y_add(f32 a)const;
	vec3  z_add(f32 a)const;
	vec2  to_vec2()const;
	vec2i to_vec2i()const;
	vec3i to_vec3i()const;
	vec4  to_vec4()const;
	vec4i to_vec4i()const;
#endif //#ifdef __cplusplus
} vec3;

DESHI_MATH_FUNC inline vec3
Vec3(f32 x, f32 y, f32 z){
	return vec3{x, y, z};
}

DESHI_MATH_FUNC inline vec3 vec3_ZERO()   { return vec3{ 0, 0, 0}; }
DESHI_MATH_FUNC inline vec3 vec3_ONE()    { return vec3{ 1, 1, 1}; }
DESHI_MATH_FUNC inline vec3 vec3_LEFT()   { return vec3{-1, 0, 0}; }
DESHI_MATH_FUNC inline vec3 vec3_RIGHT()  { return vec3{ 1, 0, 0}; }
DESHI_MATH_FUNC inline vec3 vec3_DOWN()   { return vec3{ 0,-1, 0}; }
DESHI_MATH_FUNC inline vec3 vec3_UP()     { return vec3{ 0, 1, 0}; }
DESHI_MATH_FUNC inline vec3 vec3_BACK()   { return vec3{ 0, 0,-1}; }
DESHI_MATH_FUNC inline vec3 vec3_FORWARD(){ return vec3{ 0, 0, 1}; }
DESHI_MATH_FUNC inline vec3 vec3_UNITX()  { return vec3{ 1, 0, 0}; }
DESHI_MATH_FUNC inline vec3 vec3_UNITY()  { return vec3{ 0, 1, 0}; }
DESHI_MATH_FUNC inline vec3 vec3_UNITZ()  { return vec3{ 0, 0, 1}; }

DESHI_MATH_FUNC inline f32
vec3_index(vec3 lhs, u32 index){DPZoneScoped;
	return lhs.arr[index];
}

#ifdef __cplusplus
inline f32 vec3::
operator[](u32 index)const{DPZoneScoped;
	return this->arr[index];
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline f32& vec3::
operator[](u32 index){DPZoneScoped;
	return this->arr[index];
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_add(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x + rhs.x;
	v.y = lhs.y + rhs.y;
	v.z = lhs.z + rhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
operator+ (const vec3& rhs)const{DPZoneScoped;
	vec3 v;
	v.x = this->x + rhs.x;
	v.y = this->y + rhs.y;
	v.z = this->z + rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec3::
operator+=(const vec3& rhs){DPZoneScoped;
	this->x += rhs.x;
	this->y += rhs.y;
	this->z += rhs.z;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_sub(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x - rhs.x;
	v.y = lhs.y - rhs.y;
	v.z = lhs.z - rhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
operator- (const vec3& rhs)const{DPZoneScoped;
	vec3 v;
	v.x = this->x - rhs.x;
	v.y = this->y - rhs.y;
	v.z = this->z - rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec3::
operator-=(const vec3& rhs){DPZoneScoped;
	this->x -= rhs.x;
	this->y -= rhs.y;
	this->z -= rhs.z;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_mul(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x * rhs.x;
	v.y = lhs.y * rhs.y;
	v.z = lhs.z * rhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
operator* (const vec3& rhs)const{DPZoneScoped;
	vec3 v;
	v.x = this->x * rhs.x;
	v.y = this->y * rhs.y;
	v.z = this->z * rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec3::
operator*=(const vec3& rhs){DPZoneScoped;
	this->x *= rhs.x;
	this->y *= rhs.y;
	this->z *= rhs.z;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_mul_f32(vec3 lhs, f32 rhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x * rhs;
	v.y = lhs.y * rhs;
	v.z = lhs.z * rhs;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
operator* (f32 rhs)const{DPZoneScoped;
	vec3 v;
	v.x = this->x * rhs;
	v.y = this->y * rhs;
	v.z = this->z * rhs;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec3::
operator*=(f32 rhs){DPZoneScoped;
	this->x *= rhs;
	this->y *= rhs;
	this->z *= rhs;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec3 vec3::
operator* (f32 lhs, vec3 rhs){DPZoneScoped;
	return rhs * lhs;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_div(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x / rhs.x;
	v.y = lhs.y / rhs.y;
	v.z = lhs.z / rhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
operator/ (const vec3& rhs)const{DPZoneScoped;
	vec3 v;
	v.x = this->x / rhs.x;
	v.y = this->y / rhs.y;
	v.z = this->z / rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec3::
operator/=(const vec3& rhs){DPZoneScoped;
	this->x /= rhs.x;
	this->y /= rhs.y;
	this->z /= rhs.z;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_div_f32(vec3 lhs, f32 rhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x / rhs;
	v.y = lhs.y / rhs;
	v.z = lhs.z / rhs;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
operator/ (f32 rhs)const{DPZoneScoped;
	vec3 v;
	v.x = this->x / rhs;
	v.y = this->y / rhs;
	v.z = this->z / rhs;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec3::
operator/=(f32 rhs){DPZoneScoped;
	this->x /= rhs;
	this->y /= rhs;
	this->z /= rhs;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_negate(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = -(lhs.x);
	v.y = -(lhs.y);
	v.z = -(lhs.z);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
operator- ()const{DPZoneScoped;
	vec3 v;
	v.x = -(this->x);
	v.y = -(this->y);
	v.z = -(this->z);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
vec3_equal(vec3 lhs, vec3 rhs){DPZoneScoped;
	return (DESHI_ABSF(lhs.x - rhs.x) < M_EPSILON)
		&& (DESHI_ABSF(lhs.y - rhs.y) < M_EPSILON)
		&& (DESHI_ABSF(lhs.z - rhs.z) < M_EPSILON);
}

#ifdef __cplusplus
inline b32 vec3::
operator==(const vec3& rhs)const{DPZoneScoped;
	return (DESHI_ABSF(this->x - rhs.x) < M_EPSILON)
		&& (DESHI_ABSF(this->y - rhs.y) < M_EPSILON)
		&& (DESHI_ABSF(this->z - rhs.z) < M_EPSILON);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
vec3_nequal(vec3 lhs, vec3 rhs){DPZoneScoped;
	return (DESHI_ABSF(lhs.x - rhs.x) > M_EPSILON)
		|| (DESHI_ABSF(lhs.y - rhs.y) > M_EPSILON)
		|| (DESHI_ABSF(lhs.z - rhs.z) > M_EPSILON);
}

#ifdef __cplusplus
inline b32 vec3::
operator!=(const vec3& rhs)const{DPZoneScoped;
	return (DESHI_ABSF(this->x - rhs.x) > M_EPSILON)
		|| (DESHI_ABSF(this->y - rhs.y) > M_EPSILON)
		|| (DESHI_ABSF(this->z - rhs.z) > M_EPSILON);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_abs(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = DESHI_ABSF(lhs.x);
	v.y = DESHI_ABSF(lhs.y);
	v.z = DESHI_ABSF(lhs.z);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
abs()const{DPZoneScoped;
	vec3 v;
	v.x = DESHI_ABSF(this->x);
	v.y = DESHI_ABSF(this->y);
	v.z = DESHI_ABSF(this->z);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec3_dot(vec3 lhs, vec3 rhs){DPZoneScoped;
	return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

#ifdef __cplusplus
inline f32 vec3::
dot(const vec3& rhs)const{DPZoneScoped;
	return (this->x * rhs.x) + (this->y * rhs.y) + (this->z * rhs.z);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_cross(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = (lhs.y * rhs.z) - (lhs.z * rhs.y);
	v.y = (lhs.z * rhs.x) - (lhs.x * rhs.z);
	v.z = (lhs.x * rhs.y) - (lhs.y * rhs.x);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
cross(const vec3& rhs)const{DPZoneScoped;
	vec3 v;
	v.x = (this->y * rhs.z) - (this->z * rhs.y);
	v.y = (this->z * rhs.x) - (this->x * rhs.z);
	v.z = (this->x * rhs.y) - (this->y * rhs.x);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec3_mag(vec3 lhs){DPZoneScoped;
	return DESHI_SQRTF((lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z));
}

#ifdef __cplusplus
inline f32 vec3::
mag()const{DPZoneScoped;
	return DESHI_SQRTF((this->x * this->x) + (this->y * this->y) + (this->z * this->z));
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32 
vec3_mag_sq(vec3 lhs){DPZoneScoped;
	return (lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z);
}

#ifdef __cplusplus
inline f32 vec3::
mag_sq()const{DPZoneScoped;
	return (this->x * this->x) + (this->y * this->y) + (this->z * this->z);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_normalize(vec3 lhs){DPZoneScoped;
	if(lhs.x > M_EPSILON || lhs.y > M_EPSILON || lhs.z > M_EPSILON){
		return vec3_div_f32(lhs, vec3_mag(lhs));
	}else{
		return lhs;
	}
}

#ifdef __cplusplus
inline vec3 vec3::
normalize()const{DPZoneScoped;
	if(this->x > M_EPSILON || this->y > M_EPSILON || this->z > M_EPSILON){
		return *this / this->mag();
	}else{
		return *this;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec3_distance(vec3 lhs, vec3 rhs){DPZoneScoped;
	return vec3_mag(vec3_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec3::
distance(const vec3& rhs)const{DPZoneScoped;
	return (*this - rhs).mag();
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec3_distance_sq(vec3 lhs, vec3 rhs){DPZoneScoped;
	return vec3_mag_sq(vec3_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec3::
distance_sq(const vec3& rhs)const{DPZoneScoped;
	return (*this - rhs).mag_sq();
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec3_projection(vec3 lhs, vec3 rhs){DPZoneScoped;
	f32 m = vec3_mag(lhs);
	if(m > M_EPSILON){
		return vec3_dot(lhs,rhs) / m;
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec3::
projection(const vec3& rhs)const{DPZoneScoped;
	f32 m = this->mag();
	if(m > M_EPSILON){
		return this->dot(rhs) / m;
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_component(vec3 lhs, vec3 rhs){DPZoneScoped;
	return vec3_mul_f32(vec3_normalize(rhs), vec3_projection(lhs,rhs));
}

#ifdef __cplusplus
inline vec3 vec3::
component(const vec3& rhs)const{DPZoneScoped;
	return rhs.normalize() * this->projection(rhs);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_midpoint(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = (lhs.x + rhs.x) / 2.0f;
	v.y = (lhs.y + rhs.y) / 2.0f;
	v.z = (lhs.z + rhs.z) / 2.0f;
	return v;
}

#ifdef __cplusplus
inline vec3
midpoint(const vec3& rhs)const{DPZoneScoped;
	vec3 v;
	v.x = (this->x + rhs.x) / 2.0f;
	v.y = (this->y + rhs.y) / 2.0f;
	v.z = (this->z + rhs.z) / 2.0f;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec3_radians_between(vec3 lhs, vec3 rhs){DPZoneScoped;
	f32 m = vec3_mag(lhs) * vec3_mag(rhs);
	if(m > M_EPSILON){
		return DESHI_ACOSF(vec3_dot(rhs) / m);
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec3::
radians_between(const vec3& rhs)const{DPZoneScoped;
	f32 m = this->mag() * rhs.mag();
	if(m > M_EPSILON){
		return DESHI_ACOSF(this->dot(rhs) / m);
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_floor(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = DESHI_FLOORF(lhs.x);
	v.y = DESHI_FLOORF(lhs.y);
	v.z = DESHI_FLOORF(lhs.z);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
floor()const{DPZoneScoped;
	vec3 v;
	v.x = DESHI_FLOORF(this->x);
	v.y = DESHI_FLOORF(this->y);
	v.z = DESHI_FLOORF(this->z);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec3
floor(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = DESHI_FLOORF(lhs.x);
	v.y = DESHI_FLOORF(lhs.y);
	v.z = DESHI_FLOORF(lhs.z);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_ceil(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = DESHI_CEILF(lhs.x);
	v.y = DESHI_CEILF(lhs.y);
	v.z = DESHI_CEILF(lhs.z);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
ceil()const{DPZoneScoped;
	vec3 v;
	v.x = DESHI_CEILF(this->x);
	v.y = DESHI_CEILF(this->y);
	v.z = DESHI_CEILF(this->z);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec3
ceil(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = DESHI_CEILF(lhs.x);
	v.y = DESHI_CEILF(lhs.y);
	v.z = DESHI_CEILF(lhs.z);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_round(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = DESHI_ROUNDF(lhs.x);
	v.y = DESHI_ROUNDF(lhs.y);
	v.z = DESHI_ROUNDF(lhs.z);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
round()const{DPZoneScoped;
	vec3 v;
	v.x = DESHI_ROUNDF(this->x);
	v.y = DESHI_ROUNDF(this->y);
	v.z = DESHI_ROUNDF(this->z);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec3
round(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = DESHI_ROUNDF(lhs.x);
	v.y = DESHI_ROUNDF(lhs.y);
	v.z = DESHI_ROUNDF(lhs.z);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_round_to(vec3 lhs, u32 place){DPZoneScoped;
	vec3 v;
	v.x = DESHI_FLOORF(lhs.x * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.y = DESHI_FLOORF(lhs.y * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.z = DESHI_FLOORF(lhs.z * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
round_to(u32 place)const{DPZoneScoped;
	vec3 v;
	v.x = DESHI_FLOORF(this->x * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.y = DESHI_FLOORF(this->y * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.z = DESHI_FLOORF(this->z * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_min(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z < rhs.z) ? lhs.z : rhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
min(const vec3& rhs){DPZoneScoped;
	vec3 v;
	v.x = (this->x < rhs.x) ? this->x : rhs.x;
	v.y = (this->y < rhs.y) ? this->y : rhs.y;
	v.z = (this->z < rhs.z) ? this->z : rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec3
min(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z < rhs.z) ? lhs.z : rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_max(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z > rhs.z) ? lhs.z : rhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
max(const vec3& rhs){DPZoneScoped;
	vec3 v;
	v.x = (this->x > rhs.x) ? this->x : rhs.x;
	v.y = (this->y > rhs.y) ? this->y : rhs.y;
	v.z = (this->z > rhs.z) ? this->z : rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec3
max(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z > rhs.z) ? lhs.z : rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_clamp(vec3 value, vec3 min, vec3 max){DPZoneScoped;
	vec3 v;
	v.x = (value.x < min.x) ? min.x : ((value.x > max.x) ? max.x : value.x);
	v.y = (value.y < min.y) ? min.y : ((value.y > max.y) ? max.y : value.y);
	v.z = (value.z < min.z) ? min.z : ((value.z > max.z) ? max.z : value.z);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
clamp(const vec3& min, const vec3& max){DPZoneScoped;
	vec3 v;
	v.x = (this->x < min.x) ? min.x : ((this->x > max.x) ? max.x : value.x);
	v.y = (this->y < min.y) ? min.y : ((this->y > max.y) ? max.y : value.y);
	v.z = (this->z < min.z) ? min.z : ((this->z > max.z) ? max.z : value.z);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec3
clamp(vec3 value, vec3 min, vec3 max){DPZoneScoped;
	vec3 v;
	v.x = (value.x < min.x) ? min.x : ((value.x > max.x) ? max.x : value.x);
	v.y = (value.y < min.y) ? min.y : ((value.y > max.y) ? max.y : value.y);
	v.z = (value.z < min.z) ? min.z : ((value.z > max.z) ? max.z : value.z);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_clamp_min(vec3 value, vec3 min){DPZoneScoped;
	vec3 v;
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	v.z = (value.z < min.z) ? min.z : value.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
clamp_min(const vec3& min){DPZoneScoped;
	vec3 v;
	v.x = (this->x < min.x) ? min.x : this->x;
	v.y = (this->y < min.y) ? min.y : this->y;
	v.z = (this->z < min.z) ? min.z : this->z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec3
clamp_min(vec3 value, vec3 min){DPZoneScoped;
	vec3 v;
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	v.z = (value.z < min.z) ? min.z : value.z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_clamp_max(vec3 value, vec3 max){DPZoneScoped;
	vec3 v;
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	v.z = (value.z > max.z) ? max.z : value.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
clamp_max(const vec3& maX){DPZoneScoped;
	vec3 v;
	v.x = (this->x > max.x) ? max.x : this->x;
	v.y = (this->y > max.y) ? max.y : this->y;
	v.z = (this->z > max.z) ? max.z : this->z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec3
clamp_max(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	v.z = (value.z > max.z) ? max.z : value.z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_clamp_mag(vec3 lhs, f32 min, f32 max){DPZoneScoped;
	f32 m = vec3_mag(lhs);
	if      (m < min){
		return vec3_mul_f32(vec3_normalize(lhs), min);
	}else if(m > max){
		return vec3_mul_f32(vec3_normalize(lhs), max);
	}else{
		return lhs;
	}
}

#ifdef __cplusplus
inline vec3 vec3::
clamp_mag(f32 min, f32 max)const{DPZoneScoped;
	f32 m = this->mag();
	if(m < min){
		return this->normalize() * min;
	}else if(m > max){
		return this->normalize() * max;
	}else{
		return *this;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_nudge(vec3 value, vec3 target, vec3 delta){DPZoneScoped;
	vec3 v;
	v.x = (value.x < target.x) ? ((value.x + delta.x < target.x) ? value.x + delta.x : target.x) : ((value.x - delta.x > target.x) ? value.x - delta.x : target.x);
	v.y = (value.y < target.y) ? ((value.y + delta.y < target.y) ? value.y + delta.y : target.y) : ((value.y - delta.y > target.y) ? value.y - delta.y : target.y);
	v.z = (value.z < target.z) ? ((value.z + delta.z < target.z) ? value.z + delta.z : target.z) : ((value.z - delta.z > target.z) ? value.z - delta.z : target.z);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
nudge(vec3 target, vec3 delta){DPZoneScoped;
	vec3 v;
	v.x = (this->x < target.x) ? ((this->x + delta.x < target.x) ? this->x + delta.x : target.x) : ((this->x - delta.x > target.x) ? this->x - delta.x : target.x);
	v.y = (this->y < target.y) ? ((this->y + delta.y < target.y) ? this->y + delta.y : target.y) : ((this->y - delta.y > target.y) ? this->y - delta.y : target.y);
	v.z = (this->z < target.z) ? ((this->z + delta.z < target.z) ? this->z + delta.z : target.z) : ((this->z - delta.z > target.z) ? this->z - delta.z : target.z);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec3
nudge(vec3 value, vec3 target, vec3 delta){DPZoneScoped;
	vec3 v;
	v.x = (value.x < target.x) ? ((value.x + delta.x < target.x) ? value.x + delta.x : target.x) : ((value.x - delta.x > target.x) ? value.x - delta.x : target.x);
	v.y = (value.y < target.y) ? ((value.y + delta.y < target.y) ? value.y + delta.y : target.y) : ((value.y - delta.y > target.y) ? value.y - delta.y : target.y);
	v.z = (value.z < target.z) ? ((value.z + delta.z < target.z) ? value.z + delta.z : target.z) : ((value.z - delta.z > target.z) ? value.z - delta.z : target.z);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_lerp(vec3 lhs, vec3 rhs, f32 t){DPZoneScoped;
	vec3 v;
	v.x = (lhs.x * (1.0f - t)) + (rhs.x * t);
	v.y = (lhs.y * (1.0f - t)) + (rhs.y * t);
	v.z = (lhs.z * (1.0f - t)) + (rhs.z * t);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
lerp(vec3 rhs, f32 t){DPZoneScoped;
	vec3 v;
	v.x = (this->x * (1.0f - t)) + (rhs.x * t);
	v.y = (this->y * (1.0f - t)) + (rhs.y * t);
	v.z = (this->z * (1.0f - t)) + (rhs.z * t);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec3
lerp(vec3 lhs, vec3 rhs, f32 t){DPZoneScoped;
	vec3 v;
	v.x = (lhs.x * (1.0f - t)) + (rhs.x * t);
	v.y = (lhs.y * (1.0f - t)) + (rhs.y * t);
	v.z = (lhs.z * (1.0f - t)) + (rhs.z * t);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_x_zero(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = 0;
	v.y = lhs.y;
	v.z = lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
x_zero()const{DPZoneScoped;
	vec3 v;
	v.x = 0;
	v.y = this->y;
	v.z = this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_y_zero(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x;
	v.y = 0;
	v.z = lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
y_zero()const{DPZoneScoped;
	vec3 v;
	v.x = this->x;
	v.y = 0;
	v.z = this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_z_zero(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = 0;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
z_zero()const{DPZoneScoped;
	vec3 v;
	v.x = this->x;
	v.y = this->y;
	v.z = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_x_only(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x;
	v.y = 0;
	v.z = 0;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
x_only()const{DPZoneScoped;
	vec3 v;
	v.x = this->x;
	v.y = 0;
	v.z = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_y_only(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = 0;
	v.y = lhs.y;
	v.z = 0;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
y_only()const{DPZoneScoped;
	vec3 v;
	v.x = 0;
	v.y = this->y;
	v.z = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_z_only(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = 0;
	v.y = 0;
	v.z = lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
z_only()const{DPZoneScoped;
	vec3 v;
	v.x = 0;
	v.y = 0;
	v.z = this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_x_negate(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = -(lhs.x);
	v.y =   lhs.y;
	v.z =   lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
x_negate()const{DPZoneScoped;
	vec3 v;
	v.x = -(this->x);
	v.y =   this->y;
	v.z =   this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_y_negate(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x =   lhs.x;
	v.y = -(lhs.y);
	v.z =   lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
y_negate()const{DPZoneScoped;
	vec3 v;
	v.x =   this->x;
	v.y = -(this->y);
	v.z =   this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_z_negate(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x =   lhs.x;
	v.y =   lhs.y;
	v.z = -(lhs.z);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
z_negate()const{DPZoneScoped;
	vec3 v;
	v.x =   this->x;
	v.y =   this->y;
	v.z = -(this->z);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_x_set(vec3 lhs, f32 a){DPZoneScoped;
	vec3 v;
	v.x = a;
	v.y = lhs.y;
	v.z = lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
x_set(f32 a)const{DPZoneScoped;
	vec3 v;
	v.x = a;
	v.y = this->y;
	v.z = this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_y_set(vec3 lhs, f32 a){DPZoneScoped;
	vec3 v;
	v.x = lhs.x;
	v.y = a;
	v.z = lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
y_set(f32 a)const{DPZoneScoped;
	vec3 v;
	v.x = this->x;
	v.y = a;
	v.z = this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_z_set(vec3 lhs, f32 a){DPZoneScoped;
	vec3 v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = a;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
z_set(f32 a)const{DPZoneScoped;
	vec3 v;
	v.x = this->x;
	v.y = this->y;
	v.z = a;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_x_add(vec3 lhs, f32 a){DPZoneScoped;
	vec3 v;
	v.x = lhs.x + a;
	v.y = lhs.y;
	v.z = lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
x_add(f32 a)const{DPZoneScoped;
	vec3 v;
	v.x = this->x + a;
	v.y = this->y;
	v.z = this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_y_add(vec3 lhs, f32 a){DPZoneScoped;
	vec3 v;
	v.x = lhs.x;
	v.y = lhs.y + a;
	v.z = lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
y_add(f32 a)const{DPZoneScoped;
	vec3 v;
	v.x = this->x;
	v.y = this->y + a;
	v.z = this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_z_add(vec3 lhs, f32 a){DPZoneScoped;
	vec3 v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = lhs.z + a;
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
z_add(f32 a)const{DPZoneScoped;
	vec3 v;
	v.x = this->x;
	v.y = this->y;
	v.z = this->z + a;
	return v;
}
#endif //#ifdef __cplusplus


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec3i


DESHI_MATH_TYPE typedef struct
vec3i{
	union{
		s32 arr[3] = {};
		struct{ s32 x, y, z; };
		struct{ s32 r, g, b; };
		struct{ vec2i xy; s32 _z0; };
		struct{ s32 _x0; vec2i yz; };
	};
	
#ifdef __cplusplus
	static constexpr vec3i ZERO     = { 0, 0, 0};
	static constexpr vec3i ONE      = { 1, 1, 1};
	static constexpr vec3i LEFT     = {-1, 0, 0};
	static constexpr vec3i RIGHT    = { 1, 0, 0};
	static constexpr vec3i DOWN     = { 0,-1, 0};
	static constexpr vec3i UP       = { 0, 1, 0};
	static constexpr vec3i BACKWARD = { 0, 0,-1};
	static constexpr vec3i FORWARD  = { 0, 0, 1};
	static constexpr vec3i UNITX    = { 1, 0, 0};
	static constexpr vec3i UNITY    = { 0, 1, 0};
	static constexpr vec3i UNITZ    = { 0, 0, 1};
	s32   operator[](u32 index)const;
	s32&  operator[](u32 index);
	vec3i operator+ (const vec3i& rhs)const;
	void  operator+=(const vec3i& rhs);
	vec3i operator- (const vec3i& rhs)const;
	void  operator-=(const vec3i& rhs);
	vec3i operator* (const vec3i& rhs)const;
	void  operator*=(const vec3i& rhs);
	vec3i operator* (f32 rhs)const;
	void  operator*=(f32 rhs);
	vec3i operator/ (const vec3i& rhs)const;
	void  operator/=(const vec3i& rhs);
	vec3i operator/ (f32 rhs)const;
	void  operator/=(f32 rhs);
	vec3i operator- ()const;
	b32   operator==(const vec3i& rhs)const;
	b32   operator!=(const vec3i& rhs)const;
	vec3i abs()const;
	f32   dot(const vec3i& rhs)const;
	f32   cross(const vec3i& rhs)const;
	f32   mag()const;
	f32   mag_sq()const;
	vec3i normalize()const;
	f32   distance(const vec3i& rhs)const;
	f32   distance_sq(const vec3i& rhs)const;
	f32   projection(const vec3i& rhs)const;
	vec3i component(const vec3i& rhs)const;
	vec3i midpoint(const vec3i& rhs)const;
	f32   radians_between(const vec3i& rhs)const;
	vec3i min(const vec3i& rhs)const;
	vec3i max(const vec3i& rhs)const;
	vec3i clamp(const vec3i& min, const vec3i& max)const;
	vec3i clamp_min(const vec3i& min)const;
	vec3i clamp_max(const vec3i& max)const;
	vec3i clamp_mag(f32 min, f32 max)const;
	vec3i nudge(vec3i target, vec3i delta);
	vec3i lerp(vec3i rhs, f32 t);
	vec3i x_zero()const;
	vec3i y_zero()const;
	vec3i z_zero()const;
	vec3i x_only()const;
	vec3i y_only()const;
	vec3i z_only()const;
	vec3i x_negate()const;
	vec3i y_negate()const;
	vec3i z_negate()const;
	vec3i x_set(f32 a)const;
	vec3i y_set(f32 a)const;
	vec3i z_set(f32 a)const;
	vec3i x_add(f32 a)const;
	vec3i y_add(f32 a)const;
	vec3i z_add(f32 a)const;
	vec2  to_vec2()const;
	vec2i to_vec2i()const;
	vec3  to_vec3()const;
	vec4  to_vec4()const;
	vec4i to_vec4i()const;
#endif //#ifdef __cplusplus
} vec3i;

DESHI_MATH_FUNC inline vec3i
Vec3i(s32 x, s32 y, s32 z){
	return vec3i{x, y, z};
}

DESHI_MATH_FUNC inline vec3i vec3i_ZERO()   { return vec3i{ 0, 0, 0}; }
DESHI_MATH_FUNC inline vec3i vec3i_ONE()    { return vec3i{ 1, 1, 1}; }
DESHI_MATH_FUNC inline vec3i vec3i_LEFT()   { return vec3i{-1, 0, 0}; }
DESHI_MATH_FUNC inline vec3i vec3i_RIGHT()  { return vec3i{ 1, 0, 0}; }
DESHI_MATH_FUNC inline vec3i vec3i_DOWN()   { return vec3i{ 0,-1, 0}; }
DESHI_MATH_FUNC inline vec3i vec3i_UP()     { return vec3i{ 0, 1, 0}; }
DESHI_MATH_FUNC inline vec3i vec3i_BACK()   { return vec3i{ 0, 0,-1}; }
DESHI_MATH_FUNC inline vec3i vec3i_FORWARD(){ return vec3i{ 0, 0, 1}; }
DESHI_MATH_FUNC inline vec3i vec3i_UNITX()  { return vec3i{ 1, 0, 0}; }
DESHI_MATH_FUNC inline vec3i vec3i_UNITY()  { return vec3i{ 0, 1, 0}; }
DESHI_MATH_FUNC inline vec3i vec3i_UNITZ()  { return vec3i{ 0, 0, 1}; }

DESHI_MATH_FUNC inline s32
vec3i_index(vec3i lhs, u32 index){DPZoneScoped;
	return lhs.arr[index];
}

#ifdef __cplusplus
inline s32 vec3i::
operator[](u32 index)const{DPZoneScoped;
	return this->arr[index];
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline s32& vec3i::
operator[](u32 index){DPZoneScoped;
	return this->arr[index];
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_add(vec3i lhs, vec3i rhs){DPZoneScoped;
	vec3i v;
	v.x = lhs.x + rhs.x;
	v.y = lhs.y + rhs.y;
	v.z = lhs.z + rhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
operator+ (const vec3i& rhs)const{DPZoneScoped;
	vec3i v;
	v.x = this->x + rhs.x;
	v.y = this->y + rhs.y;
	v.z = this->z + rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec3i::
operator+=(const vec3i& rhs){DPZoneScoped;
	this->x += rhs.x;
	this->y += rhs.y;
	this->z += rhs.z;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_sub(vec3i lhs, vec3i rhs){DPZoneScoped;
	vec3i v;
	v.x = lhs.x - rhs.x;
	v.y = lhs.y - rhs.y;
	v.z = lhs.z - rhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
operator- (const vec3i& rhs)const{DPZoneScoped;
	vec3i v;
	v.x = this->x - rhs.x;
	v.y = this->y - rhs.y;
	v.z = this->z - rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec3i::
operator-=(const vec3i& rhs){DPZoneScoped;
	this->x -= rhs.x;
	this->y -= rhs.y;
	this->z -= rhs.z;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_mul(vec3i lhs, vec3i rhs){DPZoneScoped;
	vec3i v;
	v.x = lhs.x * rhs.x;
	v.y = lhs.y * rhs.y;
	v.z = lhs.z * rhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
operator* (const vec3i& rhs)const{DPZoneScoped;
	vec3i v;
	v.x = this->x * rhs.x;
	v.y = this->y * rhs.y;
	v.z = this->z * rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec3i::
operator*=(const vec3i& rhs){DPZoneScoped;
	this->x *= rhs.x;
	this->y *= rhs.y;
	this->z *= rhs.z;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_mul_f32(vec3i lhs, f32 rhs){DPZoneScoped;
	vec3i v;
	v.x = (s32)((f32)lhs.x * rhs);
	v.y = (s32)((f32)lhs.y * rhs);
	v.z = (s32)((f32)lhs.z * rhs);
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
operator* (f32 rhs)const{DPZoneScoped;
	vec3i v;
	v.x = (s32)((f32)this->x * rhs);
	v.y = (s32)((f32)this->y * rhs);
	v.z = (s32)((f32)this->z * rhs);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec3i::
operator*=(f32 rhs){DPZoneScoped;
	this->x = (s32)((f32)this->x * rhs);
	this->y = (s32)((f32)this->y * rhs);
	this->z = (s32)((f32)this->z * rhs);
}

#ifdef __cplusplus
inline vec3i vec3i::
operator* (f32 lhs, vec3i rhs){DPZoneScoped;
	return rhs * lhs;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_div(vec3i lhs, vec3i rhs){DPZoneScoped;
	vec3i v;
	v.x = lhs.x / rhs.x;
	v.y = lhs.y / rhs.y;
	v.z = lhs.z / rhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
operator/ (const vec3i& rhs)const{DPZoneScoped;
	vec3i v;
	v.x = this->x / rhs.x;
	v.y = this->y / rhs.y;
	v.z = this->z / rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec3i::
operator/=(const vec3i& rhs){DPZoneScoped;
	this->x /= rhs.x;
	this->y /= rhs.y;
	this->z /= rhs.z;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_div_f32(vec3i lhs, f32 rhs){DPZoneScoped;
	vec3i v;
	v.x = lhs.x / rhs;
	v.y = lhs.y / rhs;
	v.z = lhs.z / rhs;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
operator/ (f32 rhs)const{DPZoneScoped;
	vec3i v;
	v.x = this->x / rhs;
	v.y = this->y / rhs;
	v.z = this->z / rhs;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec3i::
operator/=(f32 rhs){DPZoneScoped;
	this->x /= rhs;
	this->y /= rhs;
	this->z /= rhs;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_negate(vec3i lhs){DPZoneScoped;
	vec3i v;
	v.x = -(lhs.x);
	v.y = -(lhs.y);
	v.z = -(lhs.z);
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
operator- ()const{DPZoneScoped;
	vec3i v;
	v.x = -(this->x);
	v.y = -(this->y);
	v.z = -(this->z);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
vec3i_equal(vec3i lhs, vec3i rhs){DPZoneScoped;
	return lhs.x == rhs.x
		&& lhs.y == rhs.y
		&& lhs.z == rhs.z;
}

#ifdef __cplusplus
inline b32 vec3i::
operator==(const vec3i& rhs)const{DPZoneScoped;
	return this->x == rhs.x
		&& this->y == rhs.y
		&& this->z == rhs.z;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
vec3i_nequal(vec3i lhs, vec3i rhs){DPZoneScoped;
	return lhs.x != rhs.x
		&& lhs.y != rhs.y
		&& lhs.z != rhs.z;
}

#ifdef __cplusplus
inline b32 vec3i::
operator!=(const vec3i& rhs)const{DPZoneScoped;
	return this->x != rhs.x
		&& this->y != rhs.y
		&& this->z != rhs.z;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_abs(vec3i lhs){DPZoneScoped;
	vec3i v;
	v.x = DESHI_ABS(lhs.x);
	v.y = DESHI_ABS(lhs.y);
	v.z = DESHI_ABS(lhs.z);
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
abs()const{DPZoneScoped;
	vec3i v;
	v.x = DESHI_ABS(this->x);
	v.y = DESHI_ABS(this->y);
	v.z = DESHI_ABS(this->z);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec3i_dot(vec3i lhs, vec3i rhs){DPZoneScoped;
	return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

#ifdef __cplusplus
inline f32 vec3i::
dot(const vec3i& rhs)const{DPZoneScoped;
	return (this->x * rhs.x) + (this->y * rhs.y) + (this->z * rhs.z);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_cross(vec3i lhs, vec3i rhs){DPZoneScoped;
	vec3i v;
	v.x = (lhs.y * rhs.z) - (lhs.z * rhs.y);
	v.y = (lhs.z * rhs.x) - (lhs.x * rhs.z);
	v.z = (lhs.x * rhs.y) - (lhs.y * rhs.x);
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
cross(const vec3i& rhs)const{DPZoneScoped;
	vec3i v;
	v.x = (this->y * rhs.z) - (this->z * rhs.y);
	v.y = (this->z * rhs.x) - (this->x * rhs.z);
	v.z = (this->x * rhs.y) - (this->y * rhs.x);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec3i_mag(vec3i lhs){DPZoneScoped;
	return DESHI_SQRTF((lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z));
}

#ifdef __cplusplus
inline f32 vec3i::
mag()const{DPZoneScoped;
	return DESHI_SQRTF((this->x * this->x) + (this->y * this->y) + (this->z * this->z));
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32 
vec3i_mag_sq(vec3i lhs){DPZoneScoped;
	return (lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z);
}

#ifdef __cplusplus
inline f32 vec3i::
mag_sq()const{DPZoneScoped;
	return (this->x * this->x) + (this->y * this->y) + (this->z * this->z);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_normalize(vec3i lhs){DPZoneScoped;
	if(lhs.x != 0 || lhs.y != 0 || lhs.z != 0){
		return vec3i_div_f32(lhs, vec3i_mag(lhs));
	}else{
		return lhs;
	}
}

#ifdef __cplusplus
inline vec3i vec3i::
normalize()const{DPZoneScoped;
	if(this->x != 0 || this->y != 0 || this->z != M_EPSILON){
		return *this / this->mag();
	}else{
		return *this;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec3i_distance(vec3i lhs, vec3i rhs){DPZoneScoped;
	return vec3i_mag(vec3i_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec3i::
distance(const vec3i& rhs)const{DPZoneScoped;
	return (*this - rhs).mag();
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec3i_distance_sq(vec3i lhs, vec3i rhs){DPZoneScoped;
	return vec3i_mag_sq(vec3i_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec3i::
distance_sq(const vec3i& rhs)const{DPZoneScoped;
	return (*this - rhs).mag_sq();
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec3i_projection(vec3i lhs, vec3i rhs){DPZoneScoped;
	f32 m = vec3i_mag(lhs);
	if(m > M_EPSILON){
		return vec3i_dot(lhs,rhs) / m;
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec3i::
projection(const vec3i& rhs)const{DPZoneScoped;
	f32 m = this->mag();
	if(m > M_EPSILON){
		return this->dot(rhs) / m;
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_component(vec3i lhs, vec3i rhs){DPZoneScoped;
	return vec3i_mul_f32(vec3i_normalize(rhs), vec3i_projection(lhs,rhs));
}

#ifdef __cplusplus
inline vec3i vec3i::
component(const vec3i& rhs)const{DPZoneScoped;
	return rhs.normalize() * this->projection(rhs);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_midpoint(vec3i lhs, vec3i rhs){DPZoneScoped;
	vec3i v;
	v.x = (lhs.x + rhs.x) / 2;
	v.y = (lhs.y + rhs.y) / 2;
	v.z = (lhs.z + rhs.z) / 2;
	return v;
}

#ifdef __cplusplus
inline vec3i
midpoint(const vec3i& rhs)const{DPZoneScoped;
	vec3i v;
	v.x = (this->x + rhs.x) / 2;
	v.y = (this->y + rhs.y) / 2;
	v.z = (this->z + rhs.z) / 2;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec3i_radians_between(vec3i lhs, vec3i rhs){DPZoneScoped;
	f32 m = vec3i_mag(lhs) * vec3i_mag(rhs);
	if(m > M_EPSILON){
		return DESHI_ACOSF(vec3i_dot(rhs) / m);
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec3i::
radians_between(const vec3i& rhs)const{DPZoneScoped;
	f32 m = this->mag() * rhs.mag();
	if(m > M_EPSILON){
		return DESHI_ACOSF(this->dot(rhs) / m);
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_min(vec3i lhs, vec3i rhs){DPZoneScoped;
	vec3i v;
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z < rhs.z) ? lhs.z : rhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
min(const vec3i& rhs){DPZoneScoped;
	vec3i v;
	v.x = (this->x < rhs.x) ? this->x : rhs.x;
	v.y = (this->y < rhs.y) ? this->y : rhs.y;
	v.z = (this->z < rhs.z) ? this->z : rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec3i
min(vec3i lhs, vec3i rhs){DPZoneScoped;
	vec3i v;
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z < rhs.z) ? lhs.z : rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_max(vec3i lhs, vec3i rhs){DPZoneScoped;
	vec3i v;
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z > rhs.z) ? lhs.z : rhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
max(const vec3i& rhs){DPZoneScoped;
	vec3i v;
	v.x = (this->x > rhs.x) ? this->x : rhs.x;
	v.y = (this->y > rhs.y) ? this->y : rhs.y;
	v.z = (this->z > rhs.z) ? this->z : rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec3i
max(vec3i lhs, vec3i rhs){DPZoneScoped;
	vec3i v;
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z > rhs.z) ? lhs.z : rhs.z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_clamp(vec3i value, vec3i min, vec3i max){DPZoneScoped;
	vec3i v;
	v.x = (value.x < min.x) ? min.x : ((value.x > max.x) ? max.x : value.x);
	v.y = (value.y < min.y) ? min.y : ((value.y > max.y) ? max.y : value.y);
	v.z = (value.z < min.z) ? min.z : ((value.z > max.z) ? max.z : value.z);
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
clamp(const vec3i& min, const vec3i& max){DPZoneScoped;
	vec3i v;
	v.x = (this->x < min.x) ? min.x : ((this->x > max.x) ? max.x : value.x);
	v.y = (this->y < min.y) ? min.y : ((this->y > max.y) ? max.y : value.y);
	v.z = (this->z < min.z) ? min.z : ((this->z > max.z) ? max.z : value.z);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec3i
clamp(vec3i value, vec3i min, vec3i max){DPZoneScoped;
	vec3i v;
	v.x = (value.x < min.x) ? min.x : ((value.x > max.x) ? max.x : value.x);
	v.y = (value.y < min.y) ? min.y : ((value.y > max.y) ? max.y : value.y);
	v.z = (value.z < min.z) ? min.z : ((value.z > max.z) ? max.z : value.z);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_clamp_min(vec3i value, vec3i min){DPZoneScoped;
	vec3i v;
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	v.z = (value.z < min.z) ? min.z : value.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
clamp_min(const vec3i& min){DPZoneScoped;
	vec3i v;
	v.x = (this->x < min.x) ? min.x : this->x;
	v.y = (this->y < min.y) ? min.y : this->y;
	v.z = (this->z < min.z) ? min.z : this->z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec3i
clamp_min(vec3i value, vec3i min){DPZoneScoped;
	vec3i v;
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	v.z = (value.z < min.z) ? min.z : value.z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_clamp_max(vec3i value, vec3i max){DPZoneScoped;
	vec3i v;
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	v.z = (value.z > max.z) ? max.z : value.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
clamp_max(const vec3i& maX){DPZoneScoped;
	vec3i v;
	v.x = (this->x > max.x) ? max.x : this->x;
	v.y = (this->y > max.y) ? max.y : this->y;
	v.z = (this->z > max.z) ? max.z : this->z;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec3i
clamp_max(vec3i lhs, vec3i rhs){DPZoneScoped;
	vec3i v;
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	v.z = (value.z > max.z) ? max.z : value.z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_clamp_mag(vec3i lhs, f32 min, f32 max){DPZoneScoped;
	f32 m = vec3i_mag(lhs);
	if      (m < min){
		return vec3i_mul_f32(vec3i_normalize(lhs), min);
	}else if(m > max){
		return vec3i_mul_f32(vec3i_normalize(lhs), max);
	}else{
		return lhs;
	}
}

#ifdef __cplusplus
inline vec3i vec3i::
clamp_mag(f32 min, f32 max)const{DPZoneScoped;
	f32 m = this->mag();
	if(m < min){
		return this->normalize() * min;
	}else if(m > max){
		return this->normalize() * max;
	}else{
		return *this;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_nudge(vec3i value, vec3i target, vec3i delta){DPZoneScoped;
	vec3i v;
	v.x = (value.x < target.x) ? ((value.x + delta.x < target.x) ? value.x + delta.x : target.x) : ((value.x - delta.x > target.x) ? value.x - delta.x : target.x);
	v.y = (value.y < target.y) ? ((value.y + delta.y < target.y) ? value.y + delta.y : target.y) : ((value.y - delta.y > target.y) ? value.y - delta.y : target.y);
	v.z = (value.z < target.z) ? ((value.z + delta.z < target.z) ? value.z + delta.z : target.z) : ((value.z - delta.z > target.z) ? value.z - delta.z : target.z);
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
nudge(vec3i target, vec3i delta){DPZoneScoped;
	vec3i v;
	v.x = (this->x < target.x) ? ((this->x + delta.x < target.x) ? this->x + delta.x : target.x) : ((this->x - delta.x > target.x) ? this->x - delta.x : target.x);
	v.y = (this->y < target.y) ? ((this->y + delta.y < target.y) ? this->y + delta.y : target.y) : ((this->y - delta.y > target.y) ? this->y - delta.y : target.y);
	v.z = (this->z < target.z) ? ((this->z + delta.z < target.z) ? this->z + delta.z : target.z) : ((this->z - delta.z > target.z) ? this->z - delta.z : target.z);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec3i
nudge(vec3i value, vec3i target, vec3i delta){DPZoneScoped;
	vec3i v;
	v.x = (value.x < target.x) ? ((value.x + delta.x < target.x) ? value.x + delta.x : target.x) : ((value.x - delta.x > target.x) ? value.x - delta.x : target.x);
	v.y = (value.y < target.y) ? ((value.y + delta.y < target.y) ? value.y + delta.y : target.y) : ((value.y - delta.y > target.y) ? value.y - delta.y : target.y);
	v.z = (value.z < target.z) ? ((value.z + delta.z < target.z) ? value.z + delta.z : target.z) : ((value.z - delta.z > target.z) ? value.z - delta.z : target.z);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_lerp(vec3i lhs, vec3i rhs, f32 t){DPZoneScoped;
	vec3i v;
	v.x = (s32)(((f32)lhs.x * (1.0f - t)) + ((f32)rhs.x * t));
	v.y = (s32)(((f32)lhs.y * (1.0f - t)) + ((f32)rhs.y * t));
	v.z = (s32)(((f32)lhs.z * (1.0f - t)) + ((f32)rhs.z * t));
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
lerp(vec3i rhs, f32 t){DPZoneScoped;
	vec3i v;
	v.x = (s32)(((f32)this->x * (1.0f - t)) + ((f32)rhs.x * t));
	v.y = (s32)(((f32)this->y * (1.0f - t)) + ((f32)rhs.y * t));
	v.z = (s32)(((f32)this->z * (1.0f - t)) + ((f32)rhs.z * t));
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec3i
lerp(vec3i lhs, vec3i rhs, f32 t){DPZoneScoped;
	vec3i v;
	v.x = (s32)(((f32)lhs.x * (1.0f - t)) + ((f32)rhs.x * t));
	v.y = (s32)(((f32)lhs.y * (1.0f - t)) + ((f32)rhs.y * t));
	v.z = (s32)(((f32)lhs.z * (1.0f - t)) + ((f32)rhs.z * t));
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_x_zero(vec3i lhs){DPZoneScoped;
	vec3i v;
	v.x = 0;
	v.y = lhs.y;
	v.z = lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
x_zero()const{DPZoneScoped;
	vec3i v;
	v.x = 0;
	v.y = this->y;
	v.z = this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_y_zero(vec3i lhs){DPZoneScoped;
	vec3i v;
	v.x = lhs.x;
	v.y = 0;
	v.z = lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
y_zero()const{DPZoneScoped;
	vec3i v;
	v.x = this->x;
	v.y = 0;
	v.z = this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_z_zero(vec3i lhs){DPZoneScoped;
	vec3i v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = 0;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
z_zero()const{DPZoneScoped;
	vec3i v;
	v.x = this->x;
	v.y = this->y;
	v.z = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_x_only(vec3i lhs){DPZoneScoped;
	vec3i v;
	v.x = lhs.x;
	v.y = 0;
	v.z = 0;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
x_only()const{DPZoneScoped;
	vec3i v;
	v.x = this->x;
	v.y = 0;
	v.z = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_y_only(vec3i lhs){DPZoneScoped;
	vec3i v;
	v.x = 0;
	v.y = lhs.y;
	v.z = 0;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
y_only()const{DPZoneScoped;
	vec3i v;
	v.x = 0;
	v.y = this->y;
	v.z = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_z_only(vec3i lhs){DPZoneScoped;
	vec3i v;
	v.x = 0;
	v.y = 0;
	v.z = lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
z_only()const{DPZoneScoped;
	vec3i v;
	v.x = 0;
	v.y = 0;
	v.z = this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_x_negate(vec3i lhs){DPZoneScoped;
	vec3i v;
	v.x = -(lhs.x);
	v.y =   lhs.y;
	v.z =   lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
x_negate()const{DPZoneScoped;
	vec3i v;
	v.x = -(this->x);
	v.y =   this->y;
	v.z =   this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_y_negate(vec3i lhs){DPZoneScoped;
	vec3i v;
	v.x =   lhs.x;
	v.y = -(lhs.y);
	v.z =   lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
y_negate()const{DPZoneScoped;
	vec3i v;
	v.x =   this->x;
	v.y = -(this->y);
	v.z =   this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_z_negate(vec3i lhs){DPZoneScoped;
	vec3i v;
	v.x =   lhs.x;
	v.y =   lhs.y;
	v.z = -(lhs.z);
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
z_negate()const{DPZoneScoped;
	vec3i v;
	v.x =   this->x;
	v.y =   this->y;
	v.z = -(this->z);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_x_set(vec3i lhs, s32 a){DPZoneScoped;
	vec3i v;
	v.x = a;
	v.y = lhs.y;
	v.z = lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
x_set(s32 a)const{DPZoneScoped;
	vec3i v;
	v.x = a;
	v.y = this->y;
	v.z = this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_y_set(vec3i lhs, s32 a){DPZoneScoped;
	vec3i v;
	v.x = lhs.x;
	v.y = a;
	v.z = lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
y_set(s32 a)const{DPZoneScoped;
	vec3i v;
	v.x = this->x;
	v.y = a;
	v.z = this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_z_set(vec3i lhs, s32 a){DPZoneScoped;
	vec3i v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = a;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
z_set(s32 a)const{DPZoneScoped;
	vec3i v;
	v.x = this->x;
	v.y = this->y;
	v.z = a;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_x_add(vec3i lhs, s32 a){DPZoneScoped;
	vec3i v;
	v.x = lhs.x + a;
	v.y = lhs.y;
	v.z = lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
x_add(s32 a)const{DPZoneScoped;
	vec3i v;
	v.x = this->x + a;
	v.y = this->y;
	v.z = this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_y_add(vec3i lhs, s32 a){DPZoneScoped;
	vec3i v;
	v.x = lhs.x;
	v.y = lhs.y + a;
	v.z = lhs.z;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
y_add(s32 a)const{DPZoneScoped;
	vec3i v;
	v.x = this->x;
	v.y = this->y + a;
	v.z = this->z;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3i
vec3i_z_add(vec3i lhs, s32 a){DPZoneScoped;
	vec3i v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = lhs.z + a;
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
z_add(s32 a)const{DPZoneScoped;
	vec3i v;
	v.x = this->x;
	v.y = this->y;
	v.z = this->z + a;
	return v;
}
#endif //#ifdef __cplusplus


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec4


DESHI_MATH_TYPE typedef struct
vec4{
	union{
		f32 arr[4];
		struct{
			union{
				vec3 xyz;
				struct{ f32 x, y, z; };
			};
			f32 w;
		};
		struct{
			union{
				vec3 rgb;
				struct{ f32 r, g, b; };
			};
			f32 a;
		};
		struct{
			vec2 xy;
			f32 _z0;
			f32 _w0;
		};
		struct{
			f32 _x0;
			vec2 yz;
			f32 _w1;
		};
		struct{
			f32 _x1;
			f32 _y0;
			vec2 zw;
		};
#ifdef DESHI_MATH_USE_SSE
		__m128 sse;
#endif //#ifdef DESHI_MATH_USE_SSE
	};
	
#ifdef __cplusplus
	static constexpr vec4 ZERO  = {0,0,0,0};
	static constexpr vec4 ONE   = {1,1,1,1};
	static constexpr vec4 UNITX = {1,0,0,0};
	static constexpr vec4 UNITY = {0,1,0,0};
	static constexpr vec4 UNITZ = {0,0,1,0};
	static constexpr vec4 UNITW = {0,0,0,1};
	f32   operator[](u32 index)const;
	f32&  operator[](u32 index);
	vec4  operator+ (const vec4& rhs)const;
	void  operator+=(const vec4& rhs);
	vec4  operator- (const vec4& rhs)const;
	void  operator-=(const vec4& rhs);
	vec4  operator* (const vec4& rhs)const;
	void  operator*=(const vec4& rhs);
	vec4  operator* (f32 rhs)const;
	void  operator*=(f32 rhs);
	vec4  operator* (const mat4& rhs)const;
	void  operator*=(const mat4& rhs);
	vec4  operator/ (const vec4& rhs)const;
	void  operator/=(const vec4& rhs);
	vec4  operator/ (f32 rhs)const;
	void  operator/=(f32 rhs);
	vec4  operator- ()const;
	b32   operator==(const vec4& rhs)const;
	b32   operator!=(const vec4& rhs)const;
	vec4  abs()const;
	f32   dot(const vec4& rhs)const;
	f32   mag()const;
	f32   mag_sq()const;
	vec4  normalize()const;
	vec4  wnormalize()const;
	f32   distance(const vec4& rhs)const;
	f32   distance_sq(const vec4& rhs)const;
	f32   projection(const vec4& rhs)const;
	vec4  component(const vec4& rhs)const;
	vec4  midpoint(const vec4& rhs)const;
	f32   radians_between(const vec4& rhs)const;
	vec4  floor()const;
	vec4  ceil()const;
	vec4  round()const;
	vec4  round_to(u32 place)const;
	vec4  min(const vec4& rhs)const;
	vec4  max(const vec4& rhs)const;
	vec4  clamp(const vec4& min, const vec4& max)const;
	vec4  clamp_min(const vec4& min)const;
	vec4  clamp_max(const vec4& max)const;
	vec4  clamp_mag(f32 min, f32 max)const;
	vec4  nudge(vec4 target, vec4 delta);
	vec4  lerp(vec4 rhs, f32 t);
	vec4  x_zero()const;
	vec4  y_zero()const;
	vec4  z_zero()const;
	vec4  w_zero()const;
	vec4  x_only()const;
	vec4  y_only()const;
	vec4  z_only()const;
	vec4  w_only()const;
	vec4  x_negate()const;
	vec4  y_negate()const;
	vec4  z_negate()const;
	vec4  w_negate()const;
	vec4  x_set(f32 a)const;
	vec4  y_set(f32 a)const;
	vec4  z_set(f32 a)const;
	vec4  w_set(f32 a)const;
	vec4  x_add(f32 a)const;
	vec4  y_add(f32 a)const;
	vec4  z_add(f32 a)const;
	vec4  w_add(f32 a)const;
	vec2  to_vec2()const;
	vec2i to_vec2i()const;
	vec3  to_vec3()const;
	vec3i to_vec3i()const;
	vec4i to_vec4i()const;
#endif //#ifdef __cplusplus
} vec4;

DESHI_MATH_FUNC inline vec4
Vec4(f32 x, f32 y, f32 z, f32 w){
	return vec4{x, y, z, w};
}

DESHI_MATH_FUNC inline vec4 vec4_ZERO() { return vec4{0,0,0,0}; }
DESHI_MATH_FUNC inline vec4 vec4_ONE()  { return vec4{1,1,1,1}; }
DESHI_MATH_FUNC inline vec4 vec4_UNITX(){ return vec4{1,0,0,0}; }
DESHI_MATH_FUNC inline vec4 vec4_UNITY(){ return vec4{0,1,0,0}; }
DESHI_MATH_FUNC inline vec4 vec4_UNITZ(){ return vec4{0,0,1,0}; }
DESHI_MATH_FUNC inline vec4 vec4_UNITW(){ return vec4{0,0,0,1}; }

DESHI_MATH_FUNC inline f32
vec4_index(vec4 lhs, u32 index){DPZoneScoped;
	return lhs.arr[index];
}

#ifdef __cplusplus
inline f32 vec4::
operator[](u32 index)const{DPZoneScoped;
	return this->arr[index];
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline f32& vec4::
operator[](u32 index){DPZoneScoped;
	return this->arr[index];
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_add(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_add_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = lhs.x + rhs.x;
	v.y = lhs.y + rhs.y;
	v.z = lhs.z + rhs.z;
	v.w = lhs.w + rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
operator+ (const vec4& rhs)const{DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_add_4f32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = this->x + rhs.x;
	v.y = this->y + rhs.y;
	v.z = this->z + rhs.z;
	v.w = this->w + rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4::
operator+=(const vec4& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	this->sse = m128_add_4f32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	this->x += rhs.x;
	this->y += rhs.y;
	this->z += rhs.z;
	this->w += rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_sub(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_sub_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = lhs.x - rhs.x;
	v.y = lhs.y - rhs.y;
	v.z = lhs.z - rhs.z;
	v.w = lhs.w - rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
operator- (const vec4& rhs)const{DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_sub_4f32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = this->x - rhs.x;
	v.y = this->y - rhs.y;
	v.z = this->z - rhs.z;
	v.w = this->w - rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4::
operator-=(const vec4& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	this->sse = m128_sub_4f32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	this->x -= rhs.x;
	this->y -= rhs.y;
	this->z -= rhs.z;
	this->w -= rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_mul(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_mul_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = lhs.x * rhs.x;
	v.y = lhs.y * rhs.y;
	v.z = lhs.z * rhs.z;
	v.w = lhs.w * rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
operator* (const vec4& rhs)const{DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_mul_4f32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = this->x * rhs.x;
	v.y = this->y * rhs.y;
	v.z = this->z * rhs.z;
	v.w = this->w * rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4::
operator*=(const vec4& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	this->sse = m128_mul_4f32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	this->x *= rhs.x;
	this->y *= rhs.y;
	this->z *= rhs.z;
	this->w *= rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_mul_f32(vec4 lhs, f32 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_mul_4f32(lhs.sse, m128_fill_4f32(rhs));
#else //#if DESHI_MATH_USE_SSE
	v.x = lhs.x * rhs;
	v.y = lhs.y * rhs;
	v.z = lhs.z * rhs;
	v.w = lhs.w * rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
operator* (const f32& rhs)const{DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_mul_4f32(this->sse, m128_fill_4f32(rhs));
#else //#if DESHI_MATH_USE_SSE
	v.x = this->x * rhs;
	v.y = this->y * rhs;
	v.z = this->z * rhs;
	v.w = this->w * rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4::
operator*=(const f32& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	sse = m128_mul_4f32(this->sse, m128_fill_4f32(rhs));
#else //#if DESHI_MATH_USE_SSE
	this->x *= rhs;
	this->y *= rhs;
	this->z *= rhs;
	this->w *= rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec4 vec4::
operator* (s32 lhs, vec4 rhs){
	return rhs * lhs;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_div(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_div_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = lhs.x / rhs.x;
	v.y = lhs.y / rhs.y;
	v.z = lhs.z / rhs.z;
	v.w = lhs.w / rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
operator/ (const vec4& rhs)const{DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_div_4f32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = this->x / rhs.x;
	v.y = this->y / rhs.y;
	v.z = this->z / rhs.z;
	v.w = this->w / rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4::
operator/=(const vec4& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	this->sse = m128_div_4f32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	this->x /= rhs.x;
	this->y /= rhs.y;
	this->z /= rhs.z;
	this->w /= rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_div_f32(vec4 lhs, f32 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_div_4f32(lhs.sse, m128_fill_4f32(rhs));
#else //#if DESHI_MATH_USE_SSE
	v.x = lhs.x / rhs;
	v.y = lhs.y / rhs;
	v.z = lhs.z / rhs;
	v.w = lhs.w / rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
operator/ (const f32& rhs)const{DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_div_4f32(this->sse, m128_fill_4f32(rhs));
#else //#if DESHI_MATH_USE_SSE
	v.x = this->x / rhs;
	v.y = this->y / rhs;
	v.z = this->z / rhs;
	v.w = this->w / rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4::
operator/=(const f32& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	this->sse = m128_div_4f32(this->sse, m128_fill_4f32(rhs));
#else //#if DESHI_MATH_USE_SSE
	this->x /= rhs;
	this->y /= rhs;
	this->z /= rhs;
	this->w /= rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_negate(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_negate_4f32(lhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = -(lhs.x);
	v.y = -(lhs.y);
	v.z = -(lhs.z);
	v.w = -(lhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
operator- ()const{DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_negate_4f32(this->sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = -(this->x);
	v.y = -(this->y);
	v.z = -(this->z);
	v.w = -(this->w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
vec4_equal(vec4 lhs, vec4 rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	return m128_equal_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	return DESHI_ABSF(lhs.x - rhs.x) < M_EPSILON 
		&& DESHI_ABSF(lhs.y - rhs.y) < M_EPSILON 
		&& DESHI_ABSF(lhs.z - rhs.z) < M_EPSILON 
		&& DESHI_ABSF(lhs.w - rhs.w) < M_EPSILON;
#endif //#else //#if DESHI_MATH_USE_SSE
}

#ifdef __cplusplus
inline bool vec4::
operator==(const vec4& rhs)const{DPZoneScoped;
#if DESHI_MATH_USE_SSE
	return m128_equal_4f32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	return DESHI_ABSF(this->x - rhs.x) < M_EPSILON 
		&& DESHI_ABSF(this->y - rhs.y) < M_EPSILON 
		&& DESHI_ABSF(this->z - rhs.z) < M_EPSILON 
		&& DESHI_ABSF(this->w - rhs.w) < M_EPSILON;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
vec4_nequal(vec4 lhs, vec4 rhs){DPZoneScoped;
	#if DESHI_MATH_USE_SSE
	return !m128_equal_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	return DESHI_ABSF(lhs.x - rhs.x) > M_EPSILON 
		|| DESHI_ABSF(lhs.y - rhs.y) > M_EPSILON 
		|| DESHI_ABSF(lhs.z - rhs.z) > M_EPSILON 
		|| DESHI_ABSF(lhs.w - rhs.w) > M_EPSILON;
#endif //#else //#if DESHI_MATH_USE_SSE
}

#ifdef __cplusplus
inline bool vec4::
operator!=(const vec4& rhs)const{DPZoneScoped;
#if DESHI_MATH_USE_SSE
	return !m128_equal_4f32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	return DESHI_ABSF(this->x - rhs.x) > M_EPSILON 
		|| DESHI_ABSF(this->y - rhs.y) > M_EPSILON 
		|| DESHI_ABSF(this->z - rhs.z) > M_EPSILON 
		|| DESHI_ABSF(this->w - rhs.w) > M_EPSILON;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_abs(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_abs_4f32(lhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = DESHI_ABSF(lhs.x);
	v.y = DESHI_ABSF(lhs.y);
	v.z = DESHI_ABSF(lhs.z);
	v.w = DESHI_ABSF(lhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
abs()const{DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_abs_4f32(this->sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = DESHI_ABSF(this->x);
	v.y = DESHI_ABSF(this->y);
	v.z = DESHI_ABSF(this->z);
	v.w = DESHI_ABSF(this->w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec4_dot(vec4 lhs, vec4 rhs){DPZoneScoped;
	f32 result;
#if DESHI_MATH_USE_SSE
	__m128 temp0 = _mm_mul_ps(lhs.sse, rhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	result = _mm_cvtss_f32(temp0);
#else //#if DESHI_MATH_USE_SSE
	result = (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline f32 vec4::
dot(const vec4& rhs)const{DPZoneScoped;
	f32 result;
#if DESHI_MATH_USE_SSE
	__m128 temp0 = _mm_mul_ps(this->sse, rhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	result = _mm_cvtss_f32(temp0);
#else //#if DESHI_MATH_USE_SSE
	result = (this->x * rhs.x) + (this->y * rhs.y) + (this->z * rhs.z) + (this->w * rhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec4_mag(vec4 lhs){DPZoneScoped;
	f32 result;
#if DESHI_MATH_USE_SSE
	__m128 temp0 = _mm_mul_ps(lhs.sse, lhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	temp0 = _mm_sqrt_ss(temp0);
	result = _mm_cvtss_f32(temp0);
#else //#if DESHI_MATH_USE_SSE
	result = DESHI_SQRTF((lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z) + (lhs.w * lhs.w));
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline f32 vec4::
mag()const{DPZoneScoped;
	f32 result;
#if DESHI_MATH_USE_SSE
	__m128 temp0 = _mm_mul_ps(this->sse, this->sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	temp0 = _mm_sqrt_ss(temp0);
	result = _mm_cvtss_f32(temp0);
#else //#if DESHI_MATH_USE_SSE
	result = DESHI_SQRTF((this->x * this->x) + (this->y * this->y) + (this->z * this->z) + (this->w * this->w));
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec4_mag_sq(vec4 lhs){DPZoneScoped;
	f32 result;
#if DESHI_MATH_USE_SSE
	__m128 temp0 = _mm_mul_ps(lhs.sse, lhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	result = _mm_cvtss_f32(temp0);
#else //#if DESHI_MATH_USE_SSE
	result = (lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z) + (lhs.w * lhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline f32 vec4::
mag_sq()const{DPZoneScoped;
	f32 result;
#if DESHI_MATH_USE_SSE
	__m128 temp0 = _mm_mul_ps(this->sse, this->sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	result = _mm_cvtss_f32(temp0);
#else //#if DESHI_MATH_USE_SSE
	result = (this->x * this->x) + (this->y * this->y) + (this->z * this->z) + (this->w * this->w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_normalize(vec4 lhs){DPZoneScoped;
	if(lhs.x > M_EPSILON || lhs.y > M_EPSILON || lhs.z > M_EPSILON || lhs.w > M_EPSILON){
		return vec4_div_f32(lhs, vec4_mag(lhs));
	}else{
		return lhs;
	}
}

#ifdef __cplusplus
inline vec4 vec4::
normalize()const{DPZoneScoped;
	if(this->x > M_EPSILON || this->y > M_EPSILON || this->z > M_EPSILON || this->w > M_EPSILON){
		return *this / this->mag(lhs);
	}else{
		return *this;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_wnormalize(vec4 lhs){DPZoneScoped;
	if(lhs.w > M_EPSILON){
		return vec4_div_f32(lhs, lhs.w);
	}else{
		return lhs;
	}
}

#ifdef __cplusplus
inline vec4 vec4::
wnormalize()const{DPZoneScoped;
	if(lhs.w > M_EPSILON){
		return *this / this->w;
	}else{
		return *this;
	}
}
#endif //#ifdef __cplusplus


DESHI_MATH_FUNC inline f32
vec4_distance(vec4 lhs, vec4 rhs){DPZoneScoped;
	return vec4_mag(vec4_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec4::
distance(const vec4& rhs)const{DPZoneScoped;
	return (*this - rhs).mag();
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec4_distance_sq(vec4 lhs, vec4 rhs){DPZoneScoped;
	return vec4_mag_sq(vec4_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec4::
distance_sq(const vec4& rhs)const{DPZoneScoped;
	return (*this - rhs).mag_sq();
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec4_projection(vec4 lhs, vec4 rhs){DPZoneScoped;
	f32 m = vec4_mag(lhs);
	if(m > M_EPSILON){
		return vec4_dot(lhs,rhs) / m;
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec4::
projection(const vec4& rhs)const{DPZoneScoped;
	f32 m = this->mag();
	if(m > M_EPSILON){
		return this->dot(rhs) / m;
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_component(vec4 lhs, vec4 rhs){DPZoneScoped;
	return vec4_mul_f32(vec4_normalize(rhs), vec4_projection(lhs,rhs));
}

#ifdef __cplusplus
inline vec4 vec4::
component(const vec4& rhs)const{DPZoneScoped;
	return rhs.normalize() * this->projection(rhs);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_midpoint(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_div_4f32(m128_add_4f32(lhs.sse, rhs.sse), m128_fill_4f32(2.0f));
#else //#if DESHI_MATH_USE_SSE
	v.x = (lhs.x + rhs.x) / 2.0f;
	v.y = (lhs.y + rhs.y) / 2.0f;
	v.z = (lhs.z + rhs.z) / 2.0f;
	v.w = (lhs.w + rhs.w) / 2.0f;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4
midpoint(const vec4& rhs)const{DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_div_4f32(m128_add_4f32(lhs.sse, rhs.sse), m128_fill_4f32(2.0f));
#else //#if DESHI_MATH_USE_SSE
	v.x = (lhs.x + rhs.x) / 2.0f;
	v.y = (lhs.y + rhs.y) / 2.0f;
	v.z = (lhs.z + rhs.z) / 2.0f;
	v.w = (lhs.w + rhs.w) / 2.0f;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec4_radians_between(vec4 lhs, vec4 rhs){DPZoneScoped;
	f32 m = vec4_mag(lhs) * vec4_mag(rhs);
	if(m > M_EPSILON){
		return DESHI_ACOSF(vec4_dot(rhs) / m);
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec4::
radians_between(const vec4& rhs)const{DPZoneScoped;
	f32 m = this->mag() * rhs.mag();
	if(m > M_EPSILON){
		return DESHI_ACOSF(this->dot(rhs) / m);
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_floor(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_floor_4f32(lhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = DESHI_FLOORF(lhs.x);
	v.y = DESHI_FLOORF(lhs.y);
	v.z = DESHI_FLOORF(lhs.z);
	v.w = DESHI_FLOORF(lhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
floor()const{DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_floor_4f32(this->sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = DESHI_FLOORF(this->x);
	v.y = DESHI_FLOORF(this->y);
	v.z = DESHI_FLOORF(this->z);
	v.w = DESHI_FLOORF(this->w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec4
floor(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_floor_4f32(lhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = DESHI_FLOORF(lhs.x);
	v.y = DESHI_FLOORF(lhs.y);
	v.z = DESHI_FLOORF(lhs.z);
	v.w = DESHI_FLOORF(lhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_ceil(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_ceil_4f32(lhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = DESHI_CEILF(lhs.x);
	v.y = DESHI_CEILF(lhs.y);
	v.z = DESHI_CEILF(lhs.z);
	v.w = DESHI_CEILF(lhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
ceil()const{DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_ceil_4f32(this->sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = DESHI_CEILF(this->x);
	v.y = DESHI_CEILF(this->y);
	v.z = DESHI_CEILF(this->z);
	v.w = DESHI_CEILF(this->w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec4
ceil(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_ceil_4f32(lhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = DESHI_CEILF(lhs.x);
	v.y = DESHI_CEILF(lhs.y);
	v.z = DESHI_CEILF(lhs.z);
	v.w = DESHI_CEILF(lhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_round(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_round_4f32(lhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = DESHI_ROUNDF(lhs.x);
	v.y = DESHI_ROUNDF(lhs.y);
	v.z = DESHI_ROUNDF(lhs.z);
	v.w = DESHI_ROUNDF(lhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
round()const{DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_round_4f32(this->sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = DESHI_ROUNDF(this->x);
	v.y = DESHI_ROUNDF(this->y);
	v.z = DESHI_ROUNDF(this->z);
	v.w = DESHI_ROUNDF(this->w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec4
round(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_round_4f32(lhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = DESHI_ROUNDF(lhs.x);
	v.y = DESHI_ROUNDF(lhs.y);
	v.z = DESHI_ROUNDF(lhs.z);
	v.w = DESHI_ROUNDF(lhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_round_to(vec4 lhs, u32 place){DPZoneScoped;
	vec3 v;
	v.x = DESHI_FLOORF(lhs.x * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.y = DESHI_FLOORF(lhs.y * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.z = DESHI_FLOORF(lhs.z * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.w = DESHI_FLOORF(lhs.w * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
round_to(u32 place)const{DPZoneScoped;
	vec4 v;
	v.x = DESHI_FLOORF(this->x * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.y = DESHI_FLOORF(this->y * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.z = DESHI_FLOORF(this->z * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.w = DESHI_FLOORF(this->w * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_min(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_min_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z < rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w < rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
min(const vec4& rhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_min_4f32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (this->x < rhs.x) ? this->x : rhs.x;
	v.y = (this->y < rhs.y) ? this->y : rhs.y;
	v.z = (this->z < rhs.z) ? this->z : rhs.z;
	v.w = (this->w < rhs.w) ? this->w : rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4
min(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_min_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z < rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w < rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_max(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_max_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z > rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w > rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
max(const vec4& rhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_max_4f32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (this->x > rhs.x) ? this->x : rhs.x;
	v.y = (this->y > rhs.y) ? this->y : rhs.y;
	v.z = (this->z > rhs.z) ? this->z : rhs.z;
	v.w = (this->w > rhs.w) ? this->w : rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4
max(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_max_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z > rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w > rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_clamp(vec4 value, vec4 min, vec4 max){DPZoneScoped;
	vec4 v;
	v.x = (value.x < min.x) ? min.x : ((value.x > max.x) ? max.x : value.x);
	v.y = (value.y < min.y) ? min.y : ((value.y > max.y) ? max.y : value.y);
	v.z = (value.z < min.z) ? min.z : ((value.z > max.z) ? max.z : value.z);
	v.w = (value.w < min.w) ? min.w : ((value.w > max.w) ? max.w : value.w);
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
clamp(const vec4& min, const vec4& max){DPZoneScoped;
	vec4 v;
	v.x = (this->x < min.x) ? min.x : ((this->x > max.x) ? max.x : value.x);
	v.y = (this->y < min.y) ? min.y : ((this->y > max.y) ? max.y : value.y);
	v.z = (this->z < min.z) ? min.z : ((this->z > max.z) ? max.z : value.z);
	v.w = (this->w < min.w) ? min.w : ((this->w > max.w) ? max.w : value.w);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4
clamp(vec4 value, vec4 min, vec4 max){DPZoneScoped;
	vec4 v;
	v.x = (value.x < min.x) ? min.x : ((value.x > max.x) ? max.x : value.x);
	v.y = (value.y < min.y) ? min.y : ((value.y > max.y) ? max.y : value.y);
	v.z = (value.z < min.z) ? min.z : ((value.z > max.z) ? max.z : value.z);
	v.w = (value.w < min.w) ? min.w : ((value.w > max.w) ? max.w : value.w);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_clamp_min(vec4 value, vec4 min){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_max_4f32(value.sse, min.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	v.z = (value.z < min.z) ? min.z : value.z;
	v.w = (value.w < min.w) ? min.w : value.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
clamp_min(const vec4& min){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_max_4f32(value.sse, min.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (this->x < min.x) ? min.x : this->x;
	v.y = (this->y < min.y) ? min.y : this->y;
	v.z = (this->z < min.z) ? min.z : this->z;
	v.w = (this->w < min.w) ? min.w : this->w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4
clamp_min(vec4 value, vec4 min){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_max_4f32(value.sse, min.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	v.z = (value.z < min.z) ? min.z : value.z;
	v.w = (value.w < min.w) ? min.w : value.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_clamp_max(vec4 value, vec4 max){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_min_4f32(value.sse, min.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	v.z = (value.z > max.z) ? max.z : value.z;
	v.w = (value.w > max.w) ? max.w : value.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
clamp_max(const vec4& maX){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_min_4f32(value.sse, min.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (this->x > max.x) ? max.x : this->x;
	v.y = (this->y > max.y) ? max.y : this->y;
	v.z = (this->z > max.z) ? max.z : this->z;
	v.w = (this->w > max.w) ? max.w : this->w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4
clamp_max(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_min_4f32(value.sse, min.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	v.z = (value.z > max.z) ? max.z : value.z;
	v.w = (value.w > max.w) ? max.w : value.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_clamp_mag(vec4 lhs, f32 min, f32 max){DPZoneScoped;
	f32 m = vec4_mag(lhs);
	if      (m < min){
		return vec4_mul_f32(vec4_normalize(lhs), min);
	}else if(m > max){
		return vec4_mul_f32(vec4_normalize(lhs), max);
	}else{
		return lhs;
	}
}

#ifdef __cplusplus
inline vec4 vec4::
clamp_mag(f32 min, f32 max)const{DPZoneScoped;
	f32 m = this->mag();
	if(m < min){
		return this->normalize() * min;
	}else if(m > max){
		return this->normalize() * max;
	}else{
		return *this;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_nudge(vec4 value, vec4 target, vec4 delta){DPZoneScoped;
	vec4 v;
	v.x = (value.x < target.x) ? ((value.x + delta.x < target.x) ? value.x + delta.x : target.x) : ((value.x - delta.x > target.x) ? value.x - delta.x : target.x);
	v.y = (value.y < target.y) ? ((value.y + delta.y < target.y) ? value.y + delta.y : target.y) : ((value.y - delta.y > target.y) ? value.y - delta.y : target.y);
	v.z = (value.z < target.z) ? ((value.z + delta.z < target.z) ? value.z + delta.z : target.z) : ((value.z - delta.z > target.z) ? value.z - delta.z : target.z);
	v.w = (value.w < target.w) ? ((value.w + delta.w < target.w) ? value.w + delta.w : target.w) : ((value.w - delta.w > target.w) ? value.w - delta.w : target.w);
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
nudge(vec4 target, vec4 delta){DPZoneScoped;
	vec4 v;
	v.x = (this->x < target.x) ? ((this->x + delta.x < target.x) ? this->x + delta.x : target.x) : ((this->x - delta.x > target.x) ? this->x - delta.x : target.x);
	v.y = (this->y < target.y) ? ((this->y + delta.y < target.y) ? this->y + delta.y : target.y) : ((this->y - delta.y > target.y) ? this->y - delta.y : target.y);
	v.z = (this->z < target.z) ? ((this->z + delta.z < target.z) ? this->z + delta.z : target.z) : ((this->z - delta.z > target.z) ? this->z - delta.z : target.z);
	v.w = (this->w < target.w) ? ((this->w + delta.w < target.w) ? this->w + delta.w : target.w) : ((this->w - delta.w > target.w) ? this->w - delta.w : target.w);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4
nudge(vec4 value, vec4 target, vec4 delta){DPZoneScoped;
	vec4 v;
	v.x = (value.x < target.x) ? ((value.x + delta.x < target.x) ? value.x + delta.x : target.x) : ((value.x - delta.x > target.x) ? value.x - delta.x : target.x);
	v.y = (value.y < target.y) ? ((value.y + delta.y < target.y) ? value.y + delta.y : target.y) : ((value.y - delta.y > target.y) ? value.y - delta.y : target.y);
	v.z = (value.z < target.z) ? ((value.z + delta.z < target.z) ? value.z + delta.z : target.z) : ((value.z - delta.z > target.z) ? value.z - delta.z : target.z);
	v.w = (value.w < target.w) ? ((value.w + delta.w < target.w) ? value.w + delta.w : target.w) : ((value.w - delta.w > target.w) ? value.w - delta.w : target.w);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_lerp(vec4 lhs, vec4 rhs, f32 t){DPZoneScoped;
	vec4 v;
	v.x = (lhs.x * (1.0f - t)) + (rhs.x * t);
	v.y = (lhs.y * (1.0f - t)) + (rhs.y * t);
	v.z = (lhs.z * (1.0f - t)) + (rhs.z * t);
	v.w = (lhs.w * (1.0f - t)) + (rhs.w * t);
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
lerp(vec4 rhs, f32 t){DPZoneScoped;
	vec4 v;
	v.x = (this->x * (1.0f - t)) + (rhs.x * t);
	v.y = (this->y * (1.0f - t)) + (rhs.y * t);
	v.z = (this->z * (1.0f - t)) + (rhs.z * t);
	v.w = (this->w * (1.0f - t)) + (rhs.w * t);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4
lerp(vec4 lhs, vec4 rhs, f32 t){DPZoneScoped;
	vec4 v;
	v.x = (lhs.x * (1.0f - t)) + (rhs.x * t);
	v.y = (lhs.y * (1.0f - t)) + (rhs.y * t);
	v.z = (lhs.z * (1.0f - t)) + (rhs.z * t);
	v.w = (lhs.w * (1.0f - t)) + (rhs.w * t);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_x_zero(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x = 0;
	v.y = lhs.y;
	v.z = lhs.z;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
x_zero()const{DPZoneScoped;
	vec4 v;
	v.x = 0;
	v.y = this->y;
	v.z = this->z;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_y_zero(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x = lhs.x;
	v.y = 0;
	v.z = lhs.z;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
y_zero()const{DPZoneScoped;
	vec4 v;
	v.x = this->x;
	v.y = 0;
	v.z = this->z;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_z_zero(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = 0;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
z_zero()const{DPZoneScoped;
	vec4 v;
	v.x = this->x;
	v.y = this->y;
	v.z = 0;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_w_zero(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = lhs.z;
	v.w = 0;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
w_zero()const{DPZoneScoped;
	vec4 v;
	v.x = this->x;
	v.y = this->y;
	v.z = this->z;
	v.w = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_x_only(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x = lhs.x;
	v.y = 0;
	v.z = 0;
	v.w = 0;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
x_only()const{DPZoneScoped;
	vec4 v;
	v.x = this->x;
	v.y = 0;
	v.z = 0;
	v.w = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_y_only(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x = 0;
	v.y = lhs.y;
	v.z = 0;
	v.w = 0;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
y_only()const{DPZoneScoped;
	vec4 v;
	v.x = 0;
	v.y = this->y;
	v.z = 0;
	v.w = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_z_only(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x = 0;
	v.y = 0;
	v.z = lhs.z;
	v.w = 0;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
z_only()const{DPZoneScoped;
	vec4 v;
	v.x = 0;
	v.y = 0;
	v.z = this->z;
	v.w = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_w_only(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x = 0;
	v.y = 0;
	v.z = 0;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
w_only()const{DPZoneScoped;
	vec4 v;
	v.x = 0;
	v.y = 0;
	v.z = 0;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_x_negate(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x = -(lhs.x);
	v.y =   lhs.y;
	v.z =   lhs.z;
	v.w =   lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
x_negate()const{DPZoneScoped;
	vec4 v;
	v.x = -(this->x);
	v.y =   this->y;
	v.z =   this->z;
	v.w =   this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_y_negate(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x =   lhs.x;
	v.y = -(lhs.y);
	v.z =   lhs.z;
	v.w =   lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
y_negate()const{DPZoneScoped;
	vec4 v;
	v.x =   this->x;
	v.y = -(this->y);
	v.z =   this->z;
	v.w =   this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_z_negate(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x =   lhs.x;
	v.y =   lhs.y;
	v.z = -(lhs.z);
	v.w =   lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
z_negate()const{DPZoneScoped;
	vec4 v;
	v.x =   this->x;
	v.y =   this->y;
	v.z = -(this->z);
	v.w =   this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_w_negate(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x =   lhs.x;
	v.y =   lhs.y;
	v.z =   lhs.z;
	v.w = -(lhs.w);
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
w_negate()const{DPZoneScoped;
	vec4 v;
	v.x =   this->x;
	v.y =   this->y;
	v.z =   this->z;
	v.w = -(this->w);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_x_set(vec4 lhs, f32 a){DPZoneScoped;
	vec4 v;
	v.x = a;
	v.y = lhs.y;
	v.z = lhs.z;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
x_set(f32 a)const{DPZoneScoped;
	vec4 v;
	v.x = a;
	v.y = this->y;
	v.z = this->z;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_y_set(vec4 lhs, f32 a){DPZoneScoped;
	vec4 v;
	v.x = lhs.x;
	v.y = a;
	v.z = lhs.z;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
y_set(f32 a)const{DPZoneScoped;
	vec4 v;
	v.x = this->x;
	v.y = a;
	v.z = this->z;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_z_set(vec4 lhs, f32 a){DPZoneScoped;
	vec4 v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = a;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
z_set(f32 a)const{DPZoneScoped;
	vec4 v;
	v.x = this->x;
	v.y = this->y;
	v.z = a;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_w_set(vec4 lhs, f32 a){DPZoneScoped;
	vec4 v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = lhs.z;
	v.w = a;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
w_set(f32 a)const{DPZoneScoped;
	vec4 v;
	v.x = this->x;
	v.y = this->y;
	v.z = this->z;
	v.w = a;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_x_add(vec4 lhs, f32 a){DPZoneScoped;
	vec4 v;
	v.x = lhs.x + a;
	v.y = lhs.y;
	v.z = lhs.z;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
x_add(f32 a)const{DPZoneScoped;
	vec4 v;
	v.x = this->x + a;
	v.y = this->y;
	v.z = this->z;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_y_add(vec4 lhs, f32 a){DPZoneScoped;
	vec4 v;
	v.x = lhs.x;
	v.y = lhs.y + a;
	v.z = lhs.z;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
y_add(f32 a)const{DPZoneScoped;
	vec4 v;
	v.x = this->x;
	v.y = this->y + a;
	v.z = this->z;
	v.w = this->w;
	return v; 
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_z_add(vec4 lhs, f32 a){DPZoneScoped;
	vec4 v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = lhs.z + a;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
z_add(f32 a)const{DPZoneScoped;
	vec4 v;
	v.x = this->x;
	v.y = this->y;
	v.z = this->z + a;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_w_add(vec4 lhs, f32 a){DPZoneScoped;
	vec4 v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = lhs.z;
	v.w = lhs.w + a;
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
w_add(f32 a)const{DPZoneScoped;
	vec4 v;
	v.x = this->x;
	v.y = this->y;
	v.z = this->z;
	v.w = this->w + a;
	return v;
}
#endif //#ifdef __cplusplus


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec4i


DESHI_MATH_TYPE typedef struct
vec4i{
	union{
		s32 arr[4] = {};
		struct{ 
			union{
				vec3i xyz;
				struct{ s32 x, y, z; };
			};
			s32 w;
		};
		struct{ 
			union{
				vec3i rgb;
				struct{ s32 r, g, b; };
			};
			s32 a;
		};
		struct{ 
			vec2i xy;
			s32 _unusedZ0;
			s32 _unusedW0;
		};
		struct{ 
			s32 _unusedX0;
			vec2i yz;
			s32 _unusedW1;
		};
		struct{ 
			s32 _unusedX1;
			s32 _unusedY0;
			vec2i zw;
		};
#ifdef DESHI_MATH_USE_SSE
		__m128i sse;
#endif //#ifdef DESHI_MATH_USE_SSE
	};
	
#ifdef __cplusplus
	static constexpr vec4i ZERO  = { 0, 0, 0, 0};
	static constexpr vec4i ONE   = { 1, 1, 1, 1};
	static constexpr vec4i UNITX = { 1, 0, 0, 0};
	static constexpr vec4i UNITY = { 0, 1, 0, 0};
	static constexpr vec4i UNITZ = { 0, 0, 1, 0};
	static constexpr vec4i UNITW = { 0, 0, 0, 1};
	s32   operator[](u32 index)const;
	s32&  operator[](u32 index);
	vec4i operator+ (const vec4i& rhs)const;
	void  operator+=(const vec4i& rhs);
	vec4i operator- (const vec4i& rhs)const;
	void  operator-=(const vec4i& rhs);
	vec4i operator* (const vec4i& rhs)const;
	void  operator*=(const vec4i& rhs);
	vec4i operator* (f32 rhs)const;
	void  operator*=(f32 rhs);
	vec4i operator/ (const vec4i& rhs)const;
	void  operator/=(const vec4i& rhs);
	vec4i operator/ (f32 rhs)const;
	void  operator/=(f32 rhs);
	vec4i operator- ()const;
	b32   operator==(const vec4i& rhs)const;
	b32   operator!=(const vec4i& rhs)const;
	vec4i abs()const;
	f32   dot(const vec4i& rhs)const;
	f32   cross(const vec4i& rhs)const;
	f32   mag()const;
	f32   mag_sq()const;
	vec4i normalize()const;
	f32   distance(const vec4i& rhs)const;
	f32   distance_sq(const vec4i& rhs)const;
	f32   projection(const vec4i& rhs)const;
	vec4i component(const vec4i& rhs)const;
	vec4i midpoint(const vec4i& rhs)const;
	f32   radians_between(const vec4i& rhs)const;
	vec4i min(const vec4i& rhs)const;
	vec4i max(const vec4i& rhs)const;
	vec4i clamp(const vec4i& min, const vec4i& max)const;
	vec4i clamp_min(const vec4i& min)const;
	vec4i clamp_max(const vec4i& max)const;
	vec4i clamp_mag(f32 min, f32 max)const;
	vec4i nudge(vec4i target, vec4i delta);
	vec4i lerp(vec4i rhs, f32 t);
	vec4i x_zero()const;
	vec4i y_zero()const;
	vec4i z_zero()const;
	vec4i x_only()const;
	vec4i y_only()const;
	vec4i z_only()const;
	vec4i x_negate()const;
	vec4i y_negate()const;
	vec4i z_negate()const;
	vec4i x_set(f32 a)const;
	vec4i y_set(f32 a)const;
	vec4i z_set(f32 a)const;
	vec4i x_add(f32 a)const;
	vec4i y_add(f32 a)const;
	vec4i z_add(f32 a)const;
	vec2  to_vec2()const;
	vec2i to_vec2i()const;
	vec3  to_vec3()const;
	vec3i to_vec3i()const;
	vec4  to_vec4()const;
#endif //#ifdef __cplusplus
} vec4i;

DESHI_MATH_FUNC inline vec4i
Vec4i(f32 x, f32 y, f32 z, f32 w){
	return vec4i{x, y, z, w};
}

DESHI_MATH_FUNC inline vec4i vec4i_ZERO() { return vec4i{0,0,0,0}; }
DESHI_MATH_FUNC inline vec4i vec4i_ONE()  { return vec4i{1,1,1,1}; }
DESHI_MATH_FUNC inline vec4i vec4i_UNITX(){ return vec4i{1,0,0,0}; }
DESHI_MATH_FUNC inline vec4i vec4i_UNITY(){ return vec4i{0,1,0,0}; }
DESHI_MATH_FUNC inline vec4i vec4i_UNITZ(){ return vec4i{0,0,1,0}; }
DESHI_MATH_FUNC inline vec4i vec4i_UNITW(){ return vec4i{0,0,0,1}; }

DESHI_MATH_FUNC inline s32
vec4i_index(vec4i lhs, u32 index){DPZoneScoped;
	return lhs.arr[index];
}

#ifdef __cplusplus
inline s32 vec4i::
operator[](u32 index)const{DPZoneScoped;
	return this->arr[index];
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline s32& vec4i::
operator[](u32 index){DPZoneScoped;
	return this->arr[index];
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_add(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_add_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = lhs.x + rhs.x;
	v.y = lhs.y + rhs.y;
	v.z = lhs.z + rhs.z;
	v.w = lhs.w + rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
operator+ (const vec4i& rhs)const{DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_add_4s32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = this->x + rhs.x;
	v.y = this->y + rhs.y;
	v.z = this->z + rhs.z;
	v.w = this->w + rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4i::
operator+=(const vec4i& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	this->sse = m128_add_4s32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	this->x += rhs.x;
	this->y += rhs.y;
	this->z += rhs.z;
	this->w += rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_sub(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_sub_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = lhs.x - rhs.x;
	v.y = lhs.y - rhs.y;
	v.z = lhs.z - rhs.z;
	v.w = lhs.w - rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
operator- (const vec4i& rhs)const{DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_sub_4s32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = this->x - rhs.x;
	v.y = this->y - rhs.y;
	v.z = this->z - rhs.z;
	v.w = this->w - rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4i::
operator-=(const vec4i& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	this->sse = m128_sub_4s32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	this->x -= rhs.x;
	this->y -= rhs.y;
	this->z -= rhs.z;
	this->w -= rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_mul(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_mul_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = lhs.x * rhs.x;
	v.y = lhs.y * rhs.y;
	v.z = lhs.z * rhs.z;
	v.w = lhs.w * rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
operator* (const vec4i& rhs)const{DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_mul_4s32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = this->x * rhs.x;
	v.y = this->y * rhs.y;
	v.z = this->z * rhs.z;
	v.w = this->w * rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4i::
operator*=(const vec4i& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	this->sse = m128_mul_4s32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	this->x *= rhs.x;
	this->y *= rhs.y;
	this->z *= rhs.z;
	this->w *= rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_mul_f32(vec4i lhs, f32 rhs){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_mul_4s32(lhs.sse, m128_fill_4s32(rhs));
#else //#if DESHI_MATH_USE_SSE
	v.x = lhs.x * rhs;
	v.y = lhs.y * rhs;
	v.z = lhs.z * rhs;
	v.w = lhs.w * rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
operator* (const f32& rhs)const{DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_mul_4s32(this->sse, m128_fill_4s32(rhs));
#else //#if DESHI_MATH_USE_SSE
	v.x = this->x * rhs;
	v.y = this->y * rhs;
	v.z = this->z * rhs;
	v.w = this->w * rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4i::
operator*=(const f32& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	this->sse = m128_mul_4s32(this->sse, m128_fill_4s32(rhs));
#else //#if DESHI_MATH_USE_SSE
	this->x *= rhs;
	this->y *= rhs;
	this->z *= rhs;
	this->w *= rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec4i vec4i::
operator* (s32 lhs, vec4i rhs){
	return rhs * lhs;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_div(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
	v.x = lhs.x / rhs.x;
	v.y = lhs.y / rhs.y;
	v.z = lhs.z / rhs.z;
	v.w = lhs.w / rhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
operator/ (const vec4i& rhs)const{DPZoneScoped;
	vec4i v;
	v.x = this->x / rhs.x;
	v.y = this->y / rhs.y;
	v.z = this->z / rhs.z;
	v.w = this->w / rhs.w;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4i::
operator/=(const vec4i& rhs){DPZoneScoped;
	this->x /= rhs.x;
	this->y /= rhs.y;
	this->z /= rhs.z;
	this->w /= rhs.w;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_div_f32(vec4i lhs, f32 rhs){DPZoneScoped;
	vec4i v;
	v.x = lhs.x / rhs;
	v.y = lhs.y / rhs;
	v.z = lhs.z / rhs;
	v.w = lhs.w / rhs;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
operator/ (const f32& rhs)const{DPZoneScoped;
	vec4i v;
	v.x = this->x / rhs;
	v.y = this->y / rhs;
	v.z = this->z / rhs;
	v.w = this->w / rhs;
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4i::
operator/=(const f32& rhs){DPZoneScoped;
	this->x /= rhs;
	this->y /= rhs;
	this->z /= rhs;
	this->w /= rhs;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_negate(vec4i lhs){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_negate_4s32(lhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = -(lhs.x);
	v.y = -(lhs.y);
	v.z = -(lhs.z);
	v.w = -(lhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
operator- ()const{DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_negate_4s32(this->sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = -(this->x);
	v.y = -(this->y);
	v.z = -(this->z);
	v.w = -(this->w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
vec4i_equal(vec4i lhs, vec4i rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	return m128_equal_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	return DESHI_ABSF(lhs.x - rhs.x) < M_EPSILON 
		&& DESHI_ABSF(lhs.y - rhs.y) < M_EPSILON 
		&& DESHI_ABSF(lhs.z - rhs.z) < M_EPSILON 
		&& DESHI_ABSF(lhs.w - rhs.w) < M_EPSILON;
#endif //#else //#if DESHI_MATH_USE_SSE
}

#ifdef __cplusplus
inline bool vec4i::
operator==(const vec4i& rhs)const{DPZoneScoped;
#if DESHI_MATH_USE_SSE
	return m128_equal_4s32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	return DESHI_ABSF(this->x - rhs.x) < M_EPSILON 
		&& DESHI_ABSF(this->y - rhs.y) < M_EPSILON 
		&& DESHI_ABSF(this->z - rhs.z) < M_EPSILON 
		&& DESHI_ABSF(this->w - rhs.w) < M_EPSILON;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
vec4i_nequal(vec4i lhs, vec4i rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	return !m128_equal_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	return DESHI_ABSF(lhs.x - rhs.x) > M_EPSILON 
		|| DESHI_ABSF(lhs.y - rhs.y) > M_EPSILON 
		|| DESHI_ABSF(lhs.z - rhs.z) > M_EPSILON 
		|| DESHI_ABSF(lhs.w - rhs.w) > M_EPSILON;
#endif //#else //#if DESHI_MATH_USE_SSE
}

#ifdef __cplusplus
inline bool vec4i::
operator!=(const vec4i& rhs)const{DPZoneScoped;
#if DESHI_MATH_USE_SSE
	return !m128_equal_4s32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	return DESHI_ABSF(this->x - rhs.x) > M_EPSILON 
		|| DESHI_ABSF(this->y - rhs.y) > M_EPSILON 
		|| DESHI_ABSF(this->z - rhs.z) > M_EPSILON 
		|| DESHI_ABSF(this->w - rhs.w) > M_EPSILON;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_abs(vec4i lhs){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_abs_4s32(lhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = DESHI_ABS(lhs.x);
	v.y = DESHI_ABS(lhs.y);
	v.z = DESHI_ABS(lhs.z);
	v.w = DESHI_ABS(lhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
abs()const{DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_abs_4s32(this->sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = DESHI_ABS(this->x);
	v.y = DESHI_ABS(this->y);
	v.z = DESHI_ABS(this->z);
	v.w = DESHI_ABS(this->w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec4i_dot(vec4i lhs, vec4i rhs){DPZoneScoped;
	f32 result;
#if DESHI_MATH_USE_SSE
	__m128 temp0 = _mm_mullo_epi32(lhs.sse, rhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_epi32(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_epi32(temp0, temp1); //add x+z with y+w
	result = (f32)_mm_cvtsi128_si32(temp0);
#else //#if DESHI_MATH_USE_SSE
	result = (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline f32 vec4i::
dot(const vec4i& rhs)const{DPZoneScoped;
	f32 result;
#if DESHI_MATH_USE_SSE
	__m128 temp0 = _mm_mullo_epi32(this->sse, rhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_epi32(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_epi32(temp0, temp1); //add x+z with y+w
	result = (f32)_mm_cvtsi128_si32(temp0);
#else //#if DESHI_MATH_USE_SSE
	result = (this->x * rhs.x) + (this->y * rhs.y) + (this->z * rhs.z) + (this->w * rhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec4i_mag(vec4i lhs){DPZoneScoped;
	f32 result;
#if DESHI_MATH_USE_SSE
	__m128 temp0 = _mm_mullo_epi32(lhs.sse, lhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_epi32(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_epi32(temp0, temp1); //add x+z with y+w
	result = DESHI_SQRTF((f32)_mm_cvtsi128_si32(temp0));
#else //#if DESHI_MATH_USE_SSE
	result = DESHI_SQRTF((lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z) + (lhs.w * lhs.w));
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline f32 vec4i::
mag()const{DPZoneScoped;
	f32 result;
#if DESHI_MATH_USE_SSE
	__m128 temp0 = _mm_mullo_epi32(this->sse, this->sse); //multiply together
	__m128 temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_epi32(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_epi32(temp0, temp1); //add x+z with y+w
	result = DESHI_SQRTF((f32)_mm_cvtsi128_si32(temp0));
#else //#if DESHI_MATH_USE_SSE
	result = DESHI_SQRTF((this->x * this->x) + (this->y * this->y) + (this->z * this->z) + (this->w * this->w));
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec4i_mag_sq(vec4i lhs){DPZoneScoped;
	f32 result;
#if DESHI_MATH_USE_SSE
	__m128 temp0 = _mm_mullo_epi32(lhs.sse, lhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_epi32(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_epi32(temp0, temp1); //add x+z with y+w
	result = (f32)_mm_cvtsi128_si32(temp0);
#else //#if DESHI_MATH_USE_SSE
	result = (lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z) + (lhs.w * lhs.w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline f32 vec4i::
mag_sq()const{DPZoneScoped;
	f32 result;
#if DESHI_MATH_USE_SSE
	__m128 temp0 = _mm_mullo_epi32(this->sse, this->sse); //multiply together
	__m128 temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_epi32(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_epi32(temp0, temp1); //add x+z with y+w
	result = (f32)_mm_cvtsi128_si32(temp0);
#else //#if DESHI_MATH_USE_SSE
	result = (this->x * this->x) + (this->y * this->y) + (this->z * this->z) + (this->w * this->w);
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_normalize(vec4i lhs){DPZoneScoped;
	if(lhs.x != 0 || lhs.y != 0 || lhs.z != 0 || lhs.w != 0){
		return vec4i_div_f32(lhs, vec4i_mag(lhs));
	}else{
		return lhs;
	}
}

#ifdef __cplusplus
inline vec4i vec4i::
normalize()const{DPZoneScoped;
	if(this->x != 0 || this->y != 0 || this->z != 0 || this->w != 0){
		return *this / this->mag(lhs);
	}else{
		return *this;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_wnormalize(vec4i lhs){DPZoneScoped;
	if(lhs.w != 0){
		return vec4i_div_f32(lhs, lhs.w);
	}else{
		return lhs;
	}
}

#ifdef __cplusplus
inline vec4i vec4i::
wnormalize()const{DPZoneScoped;
	if(lhs.w != 0){
		return *this / this->w;
	}else{
		return *this;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec4i_distance(vec4i lhs, vec4i rhs){DPZoneScoped;
	return vec4i_mag(vec4i_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec4i::
distance(const vec4i& rhs)const{DPZoneScoped;
	return (*this - rhs).mag();
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec4i_distance_sq(vec4i lhs, vec4i rhs){DPZoneScoped;
	return vec4i_mag_sq(vec4i_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec4i::
distance_sq(const vec4i& rhs)const{DPZoneScoped;
	return (*this - rhs).mag_sq();
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec4i_projection(vec4i lhs, vec4i rhs){DPZoneScoped;
	f32 m = vec4i_mag(lhs);
	if(m > M_EPSILON){
		return vec4i_dot(lhs,rhs) / m;
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec4i::
projection(const vec4i& rhs)const{DPZoneScoped;
	f32 m = this->mag();
	if(m > M_EPSILON){
		return this->dot(rhs) / m;
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_component(vec4i lhs, vec4i rhs){DPZoneScoped;
	return vec4i_mul_f32(vec4i_normalize(rhs), vec4i_projection(lhs,rhs));
}

#ifdef __cplusplus
inline vec4i vec4i::
component(const vec4i& rhs)const{DPZoneScoped;
	return rhs.normalize() * this->projection(rhs);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_midpoint(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
	v.x = (lhs.x + rhs.x) / 2.0f;
	v.y = (lhs.y + rhs.y) / 2.0f;
	v.z = (lhs.z + rhs.z) / 2.0f;
	v.w = (lhs.w + rhs.w) / 2.0f;
	return v;
}

#ifdef __cplusplus
inline vec4i
midpoint(const vec4i& rhs)const{DPZoneScoped;
	vec4i v;
	v.x = (this->x + rhs.x) / 2.0f;
	v.y = (this->y + rhs.y) / 2.0f;
	v.z = (this->z + rhs.z) / 2.0f;
	v.w = (this->w + rhs.w) / 2.0f;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
vec4i_radians_between(vec4i lhs, vec4i rhs){DPZoneScoped;
	f32 m = vec4i_mag(lhs) * vec4i_mag(rhs);
	if(m > M_EPSILON){
		return DESHI_ACOSF(vec4i_dot(rhs) / m);
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec4i::
radians_between(const vec4i& rhs)const{DPZoneScoped;
	f32 m = this->mag() * rhs.mag();
	if(m > M_EPSILON){
		return DESHI_ACOSF(this->dot(rhs) / m);
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_min(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_min_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z < rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w < rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
min(const vec4i& rhs){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_min_4s32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (this->x < rhs.x) ? this->x : rhs.x;
	v.y = (this->y < rhs.y) ? this->y : rhs.y;
	v.z = (this->z < rhs.z) ? this->z : rhs.z;
	v.w = (this->w < rhs.w) ? this->w : rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4i
min(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_min_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z < rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w < rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_max(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_max_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z > rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w > rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
max(const vec4i& rhs){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_min_4s32(this->sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (this->x > rhs.x) ? this->x : rhs.x;
	v.y = (this->y > rhs.y) ? this->y : rhs.y;
	v.z = (this->z > rhs.z) ? this->z : rhs.z;
	v.w = (this->w > rhs.w) ? this->w : rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4i
max(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_min_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z > rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w > rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_clamp(vec4i value, vec4i min, vec4i max){DPZoneScoped;
	vec4i v;
	v.x = (value.x < min.x) ? min.x : ((value.x > max.x) ? max.x : value.x);
	v.y = (value.y < min.y) ? min.y : ((value.y > max.y) ? max.y : value.y);
	v.z = (value.z < min.z) ? min.z : ((value.z > max.z) ? max.z : value.z);
	v.w = (value.w < min.w) ? min.w : ((value.w > max.w) ? max.w : value.w);
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
clamp(const vec4i& min, const vec4i& max){DPZoneScoped;
	vec4i v;
	v.x = (this->x < min.x) ? min.x : ((this->x > max.x) ? max.x : value.x);
	v.y = (this->y < min.y) ? min.y : ((this->y > max.y) ? max.y : value.y);
	v.z = (this->z < min.z) ? min.z : ((this->z > max.z) ? max.z : value.z);
	v.w = (this->w < min.w) ? min.w : ((this->w > max.w) ? max.w : value.w);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4i
clamp(vec4i value, vec4i min, vec4i max){DPZoneScoped;
	vec4i v;
	v.x = (value.x < min.x) ? min.x : ((value.x > max.x) ? max.x : value.x);
	v.y = (value.y < min.y) ? min.y : ((value.y > max.y) ? max.y : value.y);
	v.z = (value.z < min.z) ? min.z : ((value.z > max.z) ? max.z : value.z);
	v.w = (value.w < min.w) ? min.w : ((value.w > max.w) ? max.w : value.w);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_clamp_min(vec4i value, vec4i min){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_max_4s32(value.sse, min.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	v.z = (value.z < min.z) ? min.z : value.z;
	v.w = (value.w < min.w) ? min.w : value.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
clamp_min(const vec4i& min){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_max_4s32(this->sse, min.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (this->x < min.x) ? min.x : this->x;
	v.y = (this->y < min.y) ? min.y : this->y;
	v.z = (this->z < min.z) ? min.z : this->z;
	v.w = (this->w < min.w) ? min.w : this->w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4i
clamp_min(vec4i value, vec4i min){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_max_4s32(value.sse, min.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	v.z = (value.z < min.z) ? min.z : value.z;
	v.w = (value.w < min.w) ? min.w : value.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_clamp_max(vec4i value, vec4i max){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_min_4s32(value.sse, max.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	v.z = (value.z > max.z) ? max.z : value.z;
	v.w = (value.w > max.w) ? max.w : value.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
clamp_max(const vec4i& maX){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_min_4s32(this->sse, max.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (this->x > max.x) ? max.x : this->x;
	v.y = (this->y > max.y) ? max.y : this->y;
	v.z = (this->z > max.z) ? max.z : this->z;
	v.w = (this->w > max.w) ? max.w : this->w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4i
clamp_max(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_MATH_USE_SSE
	v.sse = m128_min_4s32(value.sse, max.sse);
#else //#if DESHI_MATH_USE_SSE
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	v.z = (value.z > max.z) ? max.z : value.z;
	v.w = (value.w > max.w) ? max.w : value.w;
#endif //#else //#if DESHI_MATH_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_clamp_mag(vec4i lhs, f32 min, f32 max){DPZoneScoped;
	f32 m = vec4i_mag(lhs);
	if(m < min){
		return vec4i_mul_f32(vec4i_normalize(lhs), min);
	}else if(m > max){
		return vec4i_mul_f32(vec4i_normalize(lhs), max);
	}else{
		return lhs;
	}
}

#ifdef __cplusplus
inline vec4i vec4i::
clamp_mag(f32 min, f32 max)const{DPZoneScoped;
	f32 m = this->mag();
	if(m < min){
		return this->normalize() * min;
	}else if(m > max){
		return this->normalize() * max;
	}else{
		return *this;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_nudge(vec4i value, vec4i target, vec4i delta){DPZoneScoped;
	vec4i v;
	v.x = (value.x < target.x) ? ((value.x + delta.x < target.x) ? value.x + delta.x : target.x) : ((value.x - delta.x > target.x) ? value.x - delta.x : target.x);
	v.y = (value.y < target.y) ? ((value.y + delta.y < target.y) ? value.y + delta.y : target.y) : ((value.y - delta.y > target.y) ? value.y - delta.y : target.y);
	v.z = (value.z < target.z) ? ((value.z + delta.z < target.z) ? value.z + delta.z : target.z) : ((value.z - delta.z > target.z) ? value.z - delta.z : target.z);
	v.w = (value.w < target.w) ? ((value.w + delta.w < target.w) ? value.w + delta.w : target.w) : ((value.w - delta.w > target.w) ? value.w - delta.w : target.w);
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
nudge(vec4i target, vec4i delta){DPZoneScoped;
	vec4i v;
	v.x = (this->x < target.x) ? ((this->x + delta.x < target.x) ? this->x + delta.x : target.x) : ((this->x - delta.x > target.x) ? this->x - delta.x : target.x);
	v.y = (this->y < target.y) ? ((this->y + delta.y < target.y) ? this->y + delta.y : target.y) : ((this->y - delta.y > target.y) ? this->y - delta.y : target.y);
	v.z = (this->z < target.z) ? ((this->z + delta.z < target.z) ? this->z + delta.z : target.z) : ((this->z - delta.z > target.z) ? this->z - delta.z : target.z);
	v.w = (this->w < target.w) ? ((this->w + delta.w < target.w) ? this->w + delta.w : target.w) : ((this->w - delta.w > target.w) ? this->w - delta.w : target.w);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4i
nudge(vec4i value, vec4i target, vec4i delta){DPZoneScoped;
	vec4i v;
	v.x = (value.x < target.x) ? ((value.x + delta.x < target.x) ? value.x + delta.x : target.x) : ((value.x - delta.x > target.x) ? value.x - delta.x : target.x);
	v.y = (value.y < target.y) ? ((value.y + delta.y < target.y) ? value.y + delta.y : target.y) : ((value.y - delta.y > target.y) ? value.y - delta.y : target.y);
	v.z = (value.z < target.z) ? ((value.z + delta.z < target.z) ? value.z + delta.z : target.z) : ((value.z - delta.z > target.z) ? value.z - delta.z : target.z);
	v.w = (value.w < target.w) ? ((value.w + delta.w < target.w) ? value.w + delta.w : target.w) : ((value.w - delta.w > target.w) ? value.w - delta.w : target.w);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec3i_lerp(vec4i lhs, vec4i rhs, f32 t){DPZoneScoped;
	vec4i v;
	v.x = (s32)(((f32)lhs.x * (1.0f - t)) + ((f32)rhs.x * t));
	v.y = (s32)(((f32)lhs.y * (1.0f - t)) + ((f32)rhs.y * t));
	v.z = (s32)(((f32)lhs.z * (1.0f - t)) + ((f32)rhs.z * t));
	v.w = (s32)(((f32)lhs.w * (1.0f - t)) + ((f32)rhs.w * t));
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
lerp(vec4i rhs, f32 t){DPZoneScoped;
	vec4i v;
	v.x = (s32)(((f32)this->x * (1.0f - t)) + ((f32)rhs.x * t));
	v.y = (s32)(((f32)this->y * (1.0f - t)) + ((f32)rhs.y * t));
	v.z = (s32)(((f32)this->z * (1.0f - t)) + ((f32)rhs.z * t));
	v.w = (s32)(((f32)this->w * (1.0f - t)) + ((f32)rhs.w * t));
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4i
lerp(vec4i lhs, vec4i rhs, f32 t){DPZoneScoped;
	vec4i v;
	v.x = (s32)(((f32)lhs.x * (1.0f - t)) + ((f32)rhs.x * t));
	v.y = (s32)(((f32)lhs.y * (1.0f - t)) + ((f32)rhs.y * t));
	v.z = (s32)(((f32)lhs.z * (1.0f - t)) + ((f32)rhs.z * t));
	v.w = (s32)(((f32)lhs.w * (1.0f - t)) + ((f32)rhs.w * t));
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_x_zero(vec4i lhs){DPZoneScoped;
	vec4i v;
	v.x = 0;
	v.y = lhs.y;
	v.z = lhs.z;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
x_zero()const{DPZoneScoped;
	vec4i v;
	v.x = 0;
	v.y = this->y;
	v.z = this->z;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_y_zero(vec4i lhs){DPZoneScoped;
	vec4i v;
	v.x = lhs.x;
	v.y = 0;
	v.z = lhs.z;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
y_zero()const{DPZoneScoped;
	vec4i v;
	v.x = this->x;
	v.y = 0;
	v.z = this->z;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_z_zero(vec4i lhs){DPZoneScoped;
	vec4i v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = 0;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
z_zero()const{DPZoneScoped;
	vec4i v;
	v.x = this->x;
	v.y = this->y;
	v.z = 0;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_w_zero(vec4i lhs){DPZoneScoped;
	vec4i v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = lhs.z;
	v.w = 0;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
w_zero()const{DPZoneScoped;
	vec4i v;
	v.x = this->x;
	v.y = this->y;
	v.z = this->z;
	v.w = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_x_only(vec4i lhs){DPZoneScoped;
	vec4i v;
	v.x = lhs.x;
	v.y = 0;
	v.z = 0;
	v.w = 0;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
x_only()const{DPZoneScoped;
	vec4i v;
	v.x = this->x;
	v.y = 0;
	v.z = 0;
	v.w = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_y_only(vec4i lhs){DPZoneScoped;
	vec4i v;
	v.x = 0;
	v.y = lhs.y;
	v.z = 0;
	v.w = 0;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
y_only()const{DPZoneScoped;
	vec4i v;
	v.x = 0;
	v.y = this->y;
	v.z = 0;
	v.w = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_z_only(vec4i lhs){DPZoneScoped;
	vec4i v;
	v.x = 0;
	v.y = 0;
	v.z = lhs.z;
	v.w = 0;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
z_only()const{DPZoneScoped;
	vec4i v;
	v.x = 0;
	v.y = 0;
	v.z = this->z;
	v.w = 0;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_w_only(vec4i lhs){DPZoneScoped;
	vec4i v;
	v.x = 0;
	v.y = 0;
	v.z = 0;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
w_only()const{DPZoneScoped;
	vec4i v;
	v.x = 0;
	v.y = 0;
	v.z = 0;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_x_negate(vec4i lhs){DPZoneScoped;
	vec4i v;
	v.x = -(lhs.x);
	v.y =   lhs.y;
	v.z =   lhs.z;
	v.w =   lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
x_negate()const{DPZoneScoped;
	vec4i v;
	v.x = -(this->x);
	v.y =   this->y;
	v.z =   this->z;
	v.w =   this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_y_negate(vec4i lhs){DPZoneScoped;
	vec4i v;
	v.x =   lhs.x;
	v.y = -(lhs.y);
	v.z =   lhs.z;
	v.w =   lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
y_negate()const{DPZoneScoped;
	vec4i v;
	v.x =   this->x;
	v.y = -(this->y);
	v.z =   this->z;
	v.w =   this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_z_negate(vec4i lhs){DPZoneScoped;
	vec4i v;
	v.x =   lhs.x;
	v.y =   lhs.y;
	v.z = -(lhs.z);
	v.w =   lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
z_negate()const{DPZoneScoped;
	vec4i v;
	v.x =   this->x;
	v.y =   this->y;
	v.z = -(this->z);
	v.w =   this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_w_negate(vec4i lhs){DPZoneScoped;
	vec4i v;
	v.x =   lhs.x;
	v.y =   lhs.y;
	v.z =   lhs.z;
	v.w = -(lhs.w);
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
w_negate()const{DPZoneScoped;
	vec4i v;
	v.x =   this->x;
	v.y =   this->y;
	v.z =   this->z;
	v.w = -(this->w);
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_x_set(vec4i lhs, f32 a){DPZoneScoped;
	vec4i v;
	v.x = a;
	v.y = lhs.y;
	v.z = lhs.z;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
x_set(f32 a)const{DPZoneScoped;
	vec4i v;
	v.x = a;
	v.y = this->y;
	v.z = this->z;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_y_set(vec4i lhs, f32 a){DPZoneScoped;
	vec4i v;
	v.x = lhs.x;
	v.y = a;
	v.z = lhs.z;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
y_set(f32 a)const{DPZoneScoped;
	vec4i v;
	v.x = this->x;
	v.y = a;
	v.z = this->z;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_z_set(vec4i lhs, f32 a){DPZoneScoped;
	vec4i v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = a;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
z_set(f32 a)const{DPZoneScoped;
	vec4i v;
	v.x = this->x;
	v.y = this->y;
	v.z = a;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_w_set(vec4i lhs, f32 a){DPZoneScoped;
	vec4i v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = lhs.z;
	v.w = a;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
w_set(f32 a)const{DPZoneScoped;
	vec4i v;
	v.x = this->x;
	v.y = this->y;
	v.z = this->z;
	v.w = a;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_x_add(vec4i lhs, f32 a){DPZoneScoped;
	vec4i v;
	v.x = lhs.x + a;
	v.y = lhs.y;
	v.z = lhs.z;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
x_add(f32 a)const{DPZoneScoped;
	vec4i v;
	v.x = this->x + a;
	v.y = this->y;
	v.z = this->z;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_y_add(vec4i lhs, f32 a){DPZoneScoped;
	vec4i v;
	v.x = lhs.x;
	v.y = lhs.y + a;
	v.z = lhs.z;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
y_add(f32 a)const{DPZoneScoped;
	vec4i v;
	v.x = this->x;
	v.y = this->y + a;
	v.z = this->z;
	v.w = this->w;
	return v; 
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_z_add(vec4i lhs, f32 a){DPZoneScoped;
	vec4i v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = lhs.z + a;
	v.w = lhs.w;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
z_add(f32 a)const{DPZoneScoped;
	vec4i v;
	v.x = this->x;
	v.y = this->y;
	v.z = this->z + a;
	v.w = this->w;
	return v;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4i
vec4i_w_add(vec4i lhs, f32 a){DPZoneScoped;
	vec4i v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = lhs.z;
	v.w = lhs.w + a;
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
w_add(f32 a)const{DPZoneScoped;
	vec4i v;
	v.x = this->x;
	v.y = this->y;
	v.z = this->z;
	v.w = this->w + a;
	return v;
}
#endif //#ifdef __cplusplus


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec_conversions


inline vec2i
vec2_to_vec2i(vec2 a){DPZoneScoped;
	return Vec2i((s32)a.x, (s32)a.y);
}

#ifdef __cplusplus
inline vec2i
vec2::to_vec2i()const{DPZoneScoped;
	return Vec2i((s32)this->x, (s32)this->y);
}
#endif //#ifdef __cplusplus

inline vec3
vec2_to_vec3(vec2 a){DPZoneScoped;
	return Vec3(a.x, a.y, 0.0f);
}

#ifdef __cplusplus
inline vec3
vec2::to_vec3()const{DPZoneScoped;
	return Vec3(this->x, this->y, 0.0f);
}
#endif //#ifdef __cplusplus

inline vec3i
vec2_to_vec3i(vec2 a){DPZoneScoped;
	return Vec3i((s32)a.x, (s32)a.y, 0);
}

#ifdef __cplusplus
inline vec3i
vec2::to_vec3i()const{DPZoneScoped;
	return Vec3i((s32)this->x, (s32)this->y, 0);
}
#endif //#ifdef __cplusplus

inline vec4
vec2_to_vec4(vec2 a){DPZoneScoped;
	return Vec4(a.x, a.y, 0.0f, 0.0f);
}

#ifdef __cplusplus
inline vec4
vec2::to_vec4()const{DPZoneScoped;
	return Vec4(this->x, this->y, 0.0f, 0.0f);
}
#endif //#ifdef __cplusplus

inline vec4i
vec2_to_vec4i(vec2 a){DPZoneScoped;
	return Vec4i((s32)a.x, (s32)a.y, 0, 0);
}

#ifdef __cplusplus
inline vec4i
vec2::to_vec4i()const{DPZoneScoped;
	return Vec4i(this->x, this->y, 0, 0);
}
#endif //#ifdef __cplusplus

inline vec2
vec2i_to_vec2(vec2i a){DPZoneScoped;
	return Vec2((f32)a.x, (f32)a.y);
}

#ifdef __cplusplus
inline vec2
vec2i::to_vec2()const{DPZoneScoped;
	return Vec2((f32)this->x, (f32)this->y);
}
#endif //#ifdef __cplusplus

inline vec3
vec2i_to_vec3(vec2i a){DPZoneScoped;
	return Vec3((f32)a.x, (f32)a.y, 0.0f);
}

#ifdef __cplusplus
inline vec3
vec2i::to_vec3()const{DPZoneScoped;
	return Vec3((f32)this->x, (f32)this->y, 0.0f);
}
#endif //#ifdef __cplusplus

inline vec3i
vec2i_to_vec3i(vec2i a){DPZoneScoped;
	return Vec3i(a.x, a.y, 0);
}

#ifdef __cplusplus
inline vec3i
vec2i::to_vec3i()const{DPZoneScoped;
	return Vec3i(this->x, this->y, 0);
}
#endif //#ifdef __cplusplus

inline vec4
vec2i_to_vec4(vec2i a){DPZoneScoped;
	return Vec4((f32)a.x, (f32)a.y, 0.0f, 0.0f);
}

#ifdef __cplusplus
inline vec4
vec2i::to_vec4()const{DPZoneScoped;
	return Vec4((f32)this->x, (f32)this->y, 0.0f, 0.0f);
}
#endif //#ifdef __cplusplus

inline vec4i
vec2i_to_vec4i(vec2i a){DPZoneScoped;
	return Vec4i(a.x, a.y, 0, 0);
}

#ifdef __cplusplus
inline vec4i
vec2i::to_vec4i()const{DPZoneScoped;
	return Vec4i(this->x, this->y, 0, 0);
}
#endif //#ifdef __cplusplus

inline vec2
vec3_to_vec2(vec3 a){DPZoneScoped;
	return Vec2(a.x, a.y);
}

#ifdef __cplusplus
inline vec2
vec3::to_vec2()const{DPZoneScoped;
	return Vec2(this->x, this->y);
}
#endif //#ifdef __cplusplus

inline vec2i
vec3_to_vec2i(vec3 a){DPZoneScoped;
	return Vec2i((s32)a.x, (s32)a.y);
}

#ifdef __cplusplus
inline vec2i
vec3::to_vec2i()const{DPZoneScoped;
	return Vec2i((s32)this->x, (s32)this->y);
}
#endif //#ifdef __cplusplus

inline vec3i
vec3_to_vec3i(vec3 a){DPZoneScoped;
	return Vec3i((s32)a.x, (s32)a.y, (s32)a.z);
}

#ifdef __cplusplus
inline vec3i
vec3::to_vec3i()const{DPZoneScoped;
	return Vec3i((s32)this->x, (s32)this->y, (s32)this->z);
}
#endif //#ifdef __cplusplus

inline vec4
vec3_to_vec4(vec3 a){DPZoneScoped;
	return Vec4(a.x, a.y, a.z, 0.0f);
}

#ifdef __cplusplus
inline vec4
vec3::to_vec4()const{DPZoneScoped;
	return Vec4(this->x, this->y, this->z, 0.0f);
}
#endif //#ifdef __cplusplus

inline vec4i
vec3_to_vec4i(vec3 a){DPZoneScoped;
	return Vec4i((s32)a.x, (s32)a.y, (s32)a.z, 0);
}

#ifdef __cplusplus
inline vec4i
vec3::to_vec4i()const{DPZoneScoped;
	return Vec4i((s32)this->x, (s32)this->y, 0, 0);
}
#endif //#ifdef __cplusplus

inline vec2
vec3i_to_vec2(vec3i a){DPZoneScoped;
	return Vec2((f32)a.x, (f32)a.y);
}

#ifdef __cplusplus
inline vec2
vec3i::to_vec2()const{DPZoneScoped;
	return Vec2((f32)this->x, (f32)this->y);
}
#endif //#ifdef __cplusplus

inline vec2i
vec3i_to_vec2i(vec3i a){DPZoneScoped;
	return Vec2i(a.x, a.y);
}

#ifdef __cplusplus
inline vec2i
vec3i::to_vec2i()const{DPZoneScoped;
	return Vec2i(this->x, this->y);
}
#endif //#ifdef __cplusplus

inline vec3
vec3i_to_vec3(vec3i a){DPZoneScoped;
	return Vec3((f32)a.x, (f32)a.y, (f32)a.z);
}

#ifdef __cplusplus
inline vec3
vec3i::to_vec3()const{DPZoneScoped;
	return Vec3((f32)this->x, (f32)this->y, (f32)this->z);
}
#endif //#ifdef __cplusplus

inline vec4
vec3i_to_vec4(vec3i a){DPZoneScoped;
	return Vec4((f32)a.x, (f32)a.y, (f32)a.z, 0.0f);
}

#ifdef __cplusplus
inline vec4
vec3i::to_vec4()const{DPZoneScoped;
	return Vec4((f32)this->x, (f32)this->y, (f32)this->z, 0.0f);
}
#endif //#ifdef __cplusplus

inline vec4i
vec3i_to_vec4i(vec3i a){DPZoneScoped;
	return Vec4i(a.x, a.y, a.z, 0);
}

#ifdef __cplusplus
inline vec4i
vec3i::to_vec4i()const{DPZoneScoped;
	return Vec4i(this->x, this->y, this->z, 0);
}
#endif //#ifdef __cplusplus

inline vec2
vec4_to_vec2(vec4 a){DPZoneScoped;
	return Vec2(a.x, a.y);
}

#ifdef __cplusplus
inline vec2
vec4::to_vec2()const{DPZoneScoped;
	return Vec2(this->x, this->y);
}
#endif //#ifdef __cplusplus

inline vec2i
vec4_to_vec2i(vec4 a){DPZoneScoped;
	return Vec2i((s32)a.x, (s32)a.y);
}

#ifdef __cplusplus
inline vec2i
vec4::to_vec2i()const{DPZoneScoped;
	return Vec2i((s32)this->x, (s32)this->y);
}
#endif //#ifdef __cplusplus

inline vec3
vec4_to_vec3(vec4 a){DPZoneScoped;
	return Vec3(a.x, a.y, a.z);
}

#ifdef __cplusplus
inline vec3
vec4::to_vec3()const{DPZoneScoped;
	return Vec3(this->x, this->y, this->z);
}
#endif //#ifdef __cplusplus

inline vec3i
vec4_to_vec3i(vec4 a){DPZoneScoped;
	return Vec3i((s32)a.x, (s32)a.y, (s32)a.z);
}

#ifdef __cplusplus
inline vec3i
vec4::to_vec3i()const{DPZoneScoped;
	return Vec3i((s32)this->x, (s32)this->y, (s32)this->z);
}
#endif //#ifdef __cplusplus

inline vec4i
vec4_to_vec4i(vec4 a){DPZoneScoped;
	return Vec4i((s32)a.x, (s32)a.y, (s32)a.z, (s32)a.w);
}

#ifdef __cplusplus
inline vec4i
vec4::to_vec4i()const{DPZoneScoped;
	return Vec4i((s32)this->x, (s32)this->y, (s32)this->z, (s32)this->w);
}
#endif //#ifdef __cplusplus

inline vec2
vec4i_to_vec2(vec4i a){DPZoneScoped;
	return Vec2((f32)a.x, (f32)a.y);
}

#ifdef __cplusplus
inline vec2
vec4i::to_vec2()const{DPZoneScoped;
	return Vec2((f32)this->x, (f32)this->y);
}
#endif //#ifdef __cplusplus

inline vec2i
vec4i_to_vec2i(vec4i a){DPZoneScoped;
	return Vec2i(a.x, a.y);
}

#ifdef __cplusplus
inline vec2i
vec4i::to_vec2i()const{DPZoneScoped;
	return Vec2i(this->x, this->y);
}
#endif //#ifdef __cplusplus

inline vec3
vec4i_to_vec3(vec4i a){DPZoneScoped;
	return Vec3((f32)a.x, (f32)a.y, (f32)a.z);
}

#ifdef __cplusplus
inline vec3
vec4i::to_vec3()const{DPZoneScoped;
	return Vec3((f32)this->x, (f32)this->y, (f32)this->z);
}
#endif //#ifdef __cplusplus

inline vec3i
vec4i_to_vec3i(vec4i a){DPZoneScoped;
	return Vec3i(a.x, a.y, a.z);
}

#ifdef __cplusplus
inline vec3i
vec4i::to_vec3i()const{DPZoneScoped;
	return Vec3i(this->x, this->y, this->z);
}
#endif //#ifdef __cplusplus

inline vec4
vec4i_to_vec4(vec4i a){DPZoneScoped;
	return Vec4((f32)a.x, (f32)a.y, (f32)a.z, (f32)a.w);
}

#ifdef __cplusplus
inline vec4
vec4i::to_vec4()const{DPZoneScoped;
	return Vec4((f32)this->x, (f32)this->y, (f32)this->z, (f32)this->w);
}
#endif //#ifdef __cplusplus


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec_hashing
#ifndef DESHI_MATH_DISABLE_HASHING
#ifdef __cplusplus


//TODO(sushi) always explain your hashing
template<> 
struct hash<vec2>{
	inline size_t operator()(vec2 const& v)const{DPZoneScoped;
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};

template<> 
struct hash<vec2i>{
	inline size_t operator()(vec2i const& v)const{DPZoneScoped;
		size_t seed = 0;
		hash<int> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};

template<> 
struct hash<vec3>{
	inline size_t operator()(vec3 const& v)const{DPZoneScoped;
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.z); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};

template<> 
struct hash<vec3i>{
	inline size_t operator()(vec3i const& v)const{DPZoneScoped;
		size_t seed = 0;
		hash<int> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.z); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};

template<> 
struct hash<vec4>{
	inline size_t operator()(vec4 const& v)const{DPZoneScoped;
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.z); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.w); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};

template<> 
struct hash<vec4i>{
	inline size_t operator()(vec4i const& v)const{DPZoneScoped;
		size_t seed = 0;
		hash<int> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.z); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.w); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};


#endif //#ifdef __cplusplus
#endif //#ifndef DESHI_MATH_DISABLE_HASHING
//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec_tostring
#ifndef DESHI_MATH_DISABLE_TOSTRING


DESHI_MATH_FUNC dstr8
vec2_to_dstr8(vec2 x, Allocator* a){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%g, %g)", x.x, x.y);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%g, %g)", x.x, x.y);
	return s;
}

#ifdef __cplusplus
dstr8
to_dstr8(const vec2& x, Allocator* a = KIGU_STRING_ALLOCATOR){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%g, %g)", x.x, x.y);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%g, %g)", x.x, x.y);
	return s;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC dstr8
vec2_to_dstr8p(vec2 x, int precision, Allocator* a){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%.*f, %.*f)", precision, x.x, precision, x.y);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%.*f, %.*f)", precision, x.x, precision, x.y);
	return s;
}

#ifdef __cplusplus
dstr8
to_dstr8p(const vec2& x, int precision, Allocator* a = KIGU_STRING_ALLOCATOR){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%.*f, %.*f)", precision, x.x, precision, x.y);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%.*f, %.*f)", precision, x.x, precision, x.y);
	return s;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC dstr8
vec2i_to_dstr8(vec2i x, Allocator* a){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%i, %i)", x.x, x.y);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%i, %i)", x.x, x.y);
	return s;
}

#ifdef __cplusplus
dstr8
to_dstr8(const vec2i& x, Allocator* a = KIGU_STRING_ALLOCATOR){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%i, %i)", x.x, x.y);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%i, %i)", x.x, x.y);
	return s;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC dstr8
vec3_to_dstr8(vec3 x, Allocator* a){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%g, %g, %g)", x.x, x.y, x.z);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%g, %g, %g)", x.x, x.y, x.z);
	return s;
}

#ifdef __cplusplus
dstr8
to_dstr8(const vec3& x, Allocator* a = KIGU_STRING_ALLOCATOR){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%g, %g, %g)", x.x, x.y, x.z);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%g, %g, %g)", x.x, x.y, x.z);
	return s;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC dstr8
vec3_to_dstr8p(vec3 x, int precision, Allocator* a){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%.*f, %.*f, %.*f)", precision, x.x, precision, x.y, precision, x.z);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%.*f, %.*f, %.*f)", precision, x.x, precision, x.y, precision, x.z);
	return s;
}

#ifdef __cplusplus
dstr8
to_dstr8p(const vec3& x, int precision, Allocator* a = KIGU_STRING_ALLOCATOR){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%.*f, %.*f, %.*f)", precision, x.x, precision, x.y, precision, x.z);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%.*f, %.*f, %.*f)", precision, x.x, precision, x.y, precision, x.z);
	return s;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC dstr8
vec3i_to_dstr8(vec3i x, Allocator* a){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%i, %i, %i)", x.x, x.y, x.z);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%i, %i, %i)", x.x, x.y, x.z);
	return s;
}

#ifdef __cplusplus
dstr8
to_dstr8(const vec3i& x, Allocator* a = KIGU_STRING_ALLOCATOR){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%i, %i, %i)", x.x, x.y, x.z);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%i, %i, %i)", x.x, x.y, x.z);
	return s;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC dstr8
vec4_to_dstr8(vec4 x, Allocator* a){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%g, %g, %g, %g)", x.x, x.y, x.z, x.w);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%g, %g, %g, %g)", x.x, x.y, x.z, x.w);
	return s;
}

#ifdef __cplusplus
dstr8
to_dstr8(const vec4& x, Allocator* a = KIGU_STRING_ALLOCATOR){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%g, %g, %g, %g)", x.x, x.y, x.z, x.w);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%g, %g, %g, %g)", x.x, x.y, x.z, x.w);
	return s;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC dstr8
vec4_to_dstr8p(vec4 x, int precision, Allocator* a){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%.*f, %.*f, %.*f, %.*f)", precision, x.x, precision, x.y, precision, x.z, precision, x.w);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%.*f, %.*f, %.*f, %.*f)", precision, x.x, precision, x.y, precision, x.z, precision, x.w);
	return s;
}

#ifdef __cplusplus
dstr8
to_dstr8p(const vec4& x, int precision, Allocator* a = KIGU_STRING_ALLOCATOR){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%.*f, %.*f, %.*f, %.*f)", precision, x.x, precision, x.y, precision, x.z, precision, x.w);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%.*f, %.*f, %.*f, %.*f)", precision, x.x, precision, x.y, precision, x.z, precision, x.w);
	return s;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC dstr8
vec4i_to_dstr8(vec4i x, Allocator* a){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%i, %i, %i, %i)", x.x, x.y, x.z, x.w);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%i, %i, %i, %i)", x.x, x.y, x.z, x.w);
	return s;
}

#ifdef __cplusplus
dstr8
to_dstr8(const vec4i& x, Allocator* a = KIGU_STRING_ALLOCATOR){DPZoneScoped;
	dstr8 s; s.allocator = a;
	s.count = snprintf(0, 0, "(%i, %i, %i, %i)", x.x, x.y, x.z, x.w);
	s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "(%i, %i, %i, %i)", x.x, x.y, x.z, x.w);
	return s;
}
#endif //#ifdef __cplusplus


#endif //#ifndef DESHI_MATH_DISABLE_TOSTRING
//~////////////////////////////////////////////////////////////////////////////////////////////////
// @mat3


DESHI_MATH_FUNC typedef struct mat3{
	union{
		f32 arr[9];
		struct{
			f32 _00, _10, _20;
			f32 _01, _11, _21;
			f32 _02, _12, _22;
		};
		struct{
			vec3 row0;
			vec3 row1;
			vec3 row2;
		};
	};
	
#ifdef __cplusplus
	static constexpr mat3 ZERO     = {0,0,0,0,0,0,0,0,0};
	static constexpr mat3 ONE      = {1,1,1,1,1,1,1,1,1};
	static constexpr mat3 IDENTITY = {1,0,0,0,1,0,0,0,1};
	f32  operator()(u32 row, u32 col)const;
	f32& operator()(u32 row, u32 col);
	f32  operator[](u32 index)const;
	f32& operator[](u32 index);
	mat3 operator+ (const mat3& rhs)const;
	void operator+=(const mat3& rhs);
	mat3 operator- (const mat3& rhs)const;
	void operator-=(const mat3& rhs);
	mat3 operator* (const f32& rhs)const;
	void operator*=(const f32& rhs);
	mat3 operator* (const mat3& rhs)const;
	void operator*=(const mat3& rhs);
	mat3 operator^ (const mat3& rhs)const;
	void operator^=(const mat3& rhs);
	mat3 operator/ (const f32& rhs)const;
	void operator/=(const f32& rhs);
	mat3 operator% (const mat3& rhs)const;
	void operator%=(const mat3& rhs);
	b32  operator==(const mat3& rhs)const;
	b32  operator!=(const mat3& rhs)const;
	mat3 transpose()const;
	f32  determinant()const;
	f32  minor(u32 row, u32 col)const;
	f32  cofactor(u32 row, u32 col)const;
	mat3 adjoint()const;
	mat3 inverse()const;
	mat4 to_mat4()const;
	vec3 row(u32 row)const;
	vec3 col(u32 col)const;
#endif //#ifdef __cplusplus
} mat3;

DESHI_MATH_FUNC inline mat3
Mat3(f32 _00, f32 _10, f32 _20,
	 f32 _01, f32 _11, f32 _21,
	 f32 _02, f32 _12, f32 _22){
	return mat3{_00, _10, _20, _01, _11, _21, _02, _12, _22};
}

DESHI_MATH_FUNC inline mat3
array_to_mat3(f32* arr){
	return mat3{arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8]};
}

DESHI_MATH_FUNC inline mat3 mat3_ZERO()    { return mat3{0,0,0,0,0,0,0,0,0}; }
DESHI_MATH_FUNC inline mat3 mat3_ONE()     { return mat3{1,1,1,1,1,1,1,1,1}; }
DESHI_MATH_FUNC inline mat3 mat3_IDENTITY(){ return mat3{1,0,0,0,1,0,0,0,1}; }

#define mat3_coord(m,row,col) m.arr[3*row + col]

#ifdef __cplusplus
inline f32 mat3::
operator()(u32 row, u32 col)const{DPZoneScoped;
	Assert(row < 3 && col < 3, "mat3 subscript out of bounds");
	return this->arr[3 * row + col];
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline f32& mat3::
operator()(u32 row, u32 col){DPZoneScoped;
	Assert(row < 3 && col < 3, "mat3 subscript out of bounds");
	return this->arr[3*row + col];
}
#endif //#ifdef __cplusplus

#define mat3_index(m,index) m.arr[index]

#ifdef __cplusplus
inline f32 mat3::
operator[](u32 index)const{DPZoneScoped;
	Assert(index < 9, "mat3 subscript out of bounds");
	return this->arr[index];
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline f32& mat3::
operator[](u32 index)const{DPZoneScoped;
	Assert(index < 9, "mat3 subscript out of bounds");
	return this->arr[index];
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat3
mat3_add_elements(mat3 lhs, mat3 rhs){DPZoneScoped;
	mat3 result;
	result.arr[0] = lhs.arr[0] + rhs.arr[0];
	result.arr[1] = lhs.arr[1] + rhs.arr[1];
	result.arr[2] = lhs.arr[2] + rhs.arr[2];
	result.arr[3] = lhs.arr[3] + rhs.arr[3];
	result.arr[4] = lhs.arr[4] + rhs.arr[4];
	result.arr[5] = lhs.arr[5] + rhs.arr[5];
	result.arr[6] = lhs.arr[6] + rhs.arr[6];
	result.arr[7] = lhs.arr[7] + rhs.arr[7];
	result.arr[8] = lhs.arr[8] + rhs.arr[8];
	return result;
}

#ifdef __cplusplus
inline mat3 mat3::
operator+ (const mat3& rhs)const{DPZoneScoped;
	mat3 result;
	result.arr[0] = this->arr[0] + rhs.arr[0];
	result.arr[1] = this->arr[1] + rhs.arr[1];
	result.arr[2] = this->arr[2] + rhs.arr[2];
	result.arr[3] = this->arr[3] + rhs.arr[3];
	result.arr[4] = this->arr[4] + rhs.arr[4];
	result.arr[5] = this->arr[5] + rhs.arr[5];
	result.arr[6] = this->arr[6] + rhs.arr[6];
	result.arr[7] = this->arr[7] + rhs.arr[7];
	result.arr[8] = this->arr[8] + rhs.arr[8];
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat3::
operator+=(const mat3& rhs){DPZoneScoped;
	this->arr[0] += rhs.arr[0];
	this->arr[1] += rhs.arr[1];
	this->arr[2] += rhs.arr[2];
	this->arr[3] += rhs.arr[3];
	this->arr[4] += rhs.arr[4];
	this->arr[5] += rhs.arr[5];
	this->arr[6] += rhs.arr[6];
	this->arr[7] += rhs.arr[7];
	this->arr[8] += rhs.arr[8];
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat3
mat3_sub_elements(mat3 lhs, mat3 rhs){DPZoneScoped;
	mat3 result;
	result.arr[0] = lhs.arr[0] - rhs.arr[0];
	result.arr[1] = lhs.arr[1] - rhs.arr[1];
	result.arr[2] = lhs.arr[2] - rhs.arr[2];
	result.arr[3] = lhs.arr[3] - rhs.arr[3];
	result.arr[4] = lhs.arr[4] - rhs.arr[4];
	result.arr[5] = lhs.arr[5] - rhs.arr[5];
	result.arr[6] = lhs.arr[6] - rhs.arr[6];
	result.arr[7] = lhs.arr[7] - rhs.arr[7];
	result.arr[8] = lhs.arr[8] - rhs.arr[8];
	return result;
}

#ifdef __cplusplus
inline mat3 mat3::
operator- (const mat3& rhs)const{DPZoneScoped;
	mat3 result;
	result.arr[0] = this->arr[0] - rhs.arr[0];
	result.arr[1] = this->arr[1] - rhs.arr[1];
	result.arr[2] = this->arr[2] - rhs.arr[2];
	result.arr[3] = this->arr[3] - rhs.arr[3];
	result.arr[4] = this->arr[4] - rhs.arr[4];
	result.arr[5] = this->arr[5] - rhs.arr[5];
	result.arr[6] = this->arr[6] - rhs.arr[6];
	result.arr[7] = this->arr[7] - rhs.arr[7];
	result.arr[8] = this->arr[8] - rhs.arr[8];
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat3::
operator-=(const mat3& rhs){DPZoneScoped;
	this->arr[0] -= rhs.arr[0];
	this->arr[1] -= rhs.arr[1];
	this->arr[2] -= rhs.arr[2];
	this->arr[3] -= rhs.arr[3];
	this->arr[4] -= rhs.arr[4];
	this->arr[5] -= rhs.arr[5];
	this->arr[6] -= rhs.arr[6];
	this->arr[7] -= rhs.arr[7];
	this->arr[8] -= rhs.arr[8];
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat3
mat3_mul_f32(mat3 lhs, f32 rhs){DPZoneScoped;
	mat3 result;
	result.arr[0] = lhs.arr[0] * rhs;
	result.arr[1] = lhs.arr[1] * rhs;
	result.arr[2] = lhs.arr[2] * rhs;
	result.arr[3] = lhs.arr[3] * rhs;
	result.arr[4] = lhs.arr[4] * rhs;
	result.arr[5] = lhs.arr[5] * rhs;
	result.arr[6] = lhs.arr[6] * rhs;
	result.arr[7] = lhs.arr[7] * rhs;
	result.arr[8] = lhs.arr[8] * rhs;
	return result;
}

#ifdef __cplusplus
inline mat3 mat3::
operator* (const f32& rhs)const{DPZoneScoped;
	mat3 result;
	result.arr[0] = this->arr[0] * rhs;
	result.arr[1] = this->arr[1] * rhs;
	result.arr[2] = this->arr[2] * rhs;
	result.arr[3] = this->arr[3] * rhs;
	result.arr[4] = this->arr[4] * rhs;
	result.arr[5] = this->arr[5] * rhs;
	result.arr[6] = this->arr[6] * rhs;
	result.arr[7] = this->arr[7] * rhs;
	result.arr[8] = this->arr[8] * rhs;
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat3::
operator*=(const f32& rhs){DPZoneScoped;
	this->arr[0] *= rhs;
	this->arr[1] *= rhs;
	this->arr[2] *= rhs;
	this->arr[3] *= rhs;
	this->arr[4] *= rhs;
	this->arr[5] *= rhs;
	this->arr[6] *= rhs;
	this->arr[7] *= rhs;
	this->arr[8] *= rhs;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline mat3 mat3::
operator* (const f32& lhs, const mat3& rhs)const{
	return rhs * lhs;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat3
mat3_mul_mat3(mat3 lhs, mat3 rhs){DPZoneScoped;
	mat3 result;
	result.arr[0] = lhs.arr[0]*rhs.arr[0] + lhs.arr[1]*rhs.arr[3] + lhs.arr[2]*rhs.arr[6];
	result.arr[1] = lhs.arr[0]*rhs.arr[1] + lhs.arr[1]*rhs.arr[4] + lhs.arr[2]*rhs.arr[7];
	result.arr[2] = lhs.arr[0]*rhs.arr[2] + lhs.arr[1]*rhs.arr[5] + lhs.arr[2]*rhs.arr[8];
	result.arr[3] = lhs.arr[3]*rhs.arr[0] + lhs.arr[4]*rhs.arr[3] + lhs.arr[5]*rhs.arr[6];
	result.arr[4] = lhs.arr[3]*rhs.arr[1] + lhs.arr[4]*rhs.arr[4] + lhs.arr[5]*rhs.arr[7];
	result.arr[5] = lhs.arr[3]*rhs.arr[2] + lhs.arr[4]*rhs.arr[5] + lhs.arr[5]*rhs.arr[8];
	result.arr[6] = lhs.arr[6]*rhs.arr[0] + lhs.arr[7]*rhs.arr[3] + lhs.arr[8]*rhs.arr[6];
	result.arr[7] = lhs.arr[6]*rhs.arr[1] + lhs.arr[7]*rhs.arr[4] + lhs.arr[8]*rhs.arr[7];
	result.arr[8] = lhs.arr[6]*rhs.arr[2] + lhs.arr[7]*rhs.arr[5] + lhs.arr[8]*rhs.arr[8];
	return result;
}

#ifdef __cplusplus
inline mat3 mat3::
operator* (const mat3& rhs)const{DPZoneScoped;
	mat3 result;
	result.arr[0] = this->arr[0]*rhs.arr[0] + this->arr[1]*rhs.arr[3] + this->arr[2]*rhs.arr[6];
	result.arr[1] = this->arr[0]*rhs.arr[1] + this->arr[1]*rhs.arr[4] + this->arr[2]*rhs.arr[7];
	result.arr[2] = this->arr[0]*rhs.arr[2] + this->arr[1]*rhs.arr[5] + this->arr[2]*rhs.arr[8];
	result.arr[3] = this->arr[3]*rhs.arr[0] + this->arr[4]*rhs.arr[3] + this->arr[5]*rhs.arr[6];
	result.arr[4] = this->arr[3]*rhs.arr[1] + this->arr[4]*rhs.arr[4] + this->arr[5]*rhs.arr[7];
	result.arr[5] = this->arr[3]*rhs.arr[2] + this->arr[4]*rhs.arr[5] + this->arr[5]*rhs.arr[8];
	result.arr[6] = this->arr[6]*rhs.arr[0] + this->arr[7]*rhs.arr[3] + this->arr[8]*rhs.arr[6];
	result.arr[7] = this->arr[6]*rhs.arr[1] + this->arr[7]*rhs.arr[4] + this->arr[8]*rhs.arr[7];
	result.arr[8] = this->arr[6]*rhs.arr[2] + this->arr[7]*rhs.arr[5] + this->arr[8]*rhs.arr[8];
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat3::
operator*=(const mat3& rhs){DPZoneScoped;
	mat3 result;
	result.arr[0] = this->arr[0]*rhs.arr[0] + this->arr[1]*rhs.arr[3] + this->arr[2]*rhs.arr[6];
	result.arr[1] = this->arr[0]*rhs.arr[1] + this->arr[1]*rhs.arr[4] + this->arr[2]*rhs.arr[7];
	result.arr[2] = this->arr[0]*rhs.arr[2] + this->arr[1]*rhs.arr[5] + this->arr[2]*rhs.arr[8];
	result.arr[3] = this->arr[3]*rhs.arr[0] + this->arr[4]*rhs.arr[3] + this->arr[5]*rhs.arr[6];
	result.arr[4] = this->arr[3]*rhs.arr[1] + this->arr[4]*rhs.arr[4] + this->arr[5]*rhs.arr[7];
	result.arr[5] = this->arr[3]*rhs.arr[2] + this->arr[4]*rhs.arr[5] + this->arr[5]*rhs.arr[8];
	result.arr[6] = this->arr[6]*rhs.arr[0] + this->arr[7]*rhs.arr[3] + this->arr[8]*rhs.arr[6];
	result.arr[7] = this->arr[6]*rhs.arr[1] + this->arr[7]*rhs.arr[4] + this->arr[8]*rhs.arr[7];
	result.arr[8] = this->arr[6]*rhs.arr[2] + this->arr[7]*rhs.arr[5] + this->arr[8]*rhs.arr[8];
	*this = result;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat3
mat3_mul_elements(mat3 lhs, mat3 rhs){DPZoneScoped;
	mat3 result;
	result.arr[0] = lhs.arr[0] * rhs.arr[0];
	result.arr[1] = lhs.arr[1] * rhs.arr[1];
	result.arr[2] = lhs.arr[2] * rhs.arr[2];
	result.arr[3] = lhs.arr[3] * rhs.arr[3];
	result.arr[4] = lhs.arr[4] * rhs.arr[4];
	result.arr[5] = lhs.arr[5] * rhs.arr[5];
	result.arr[6] = lhs.arr[6] * rhs.arr[6];
	result.arr[7] = lhs.arr[7] * rhs.arr[7];
	result.arr[8] = lhs.arr[8] * rhs.arr[8];
	return result;
}

#ifdef __cplusplus
inline mat3 mat3::
operator^ (const mat3& rhs)const{DPZoneScoped;
	mat3 result;
	result.arr[0] = this->arr[0] * rhs.arr[0];
	result.arr[1] = this->arr[1] * rhs.arr[1];
	result.arr[2] = this->arr[2] * rhs.arr[2];
	result.arr[3] = this->arr[3] * rhs.arr[3];
	result.arr[4] = this->arr[4] * rhs.arr[4];
	result.arr[5] = this->arr[5] * rhs.arr[5];
	result.arr[6] = this->arr[6] * rhs.arr[6];
	result.arr[7] = this->arr[7] * rhs.arr[7];
	result.arr[8] = this->arr[8] * rhs.arr[8];
	return result;
} 
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat3::
operator^=(const mat3& rhs){DPZoneScoped;
	this->arr[0] *= rhs.arr[0];
	this->arr[1] *= rhs.arr[1];
	this->arr[2] *= rhs.arr[2];
	this->arr[3] *= rhs.arr[3];
	this->arr[4] *= rhs.arr[4];
	this->arr[5] *= rhs.arr[5];
	this->arr[6] *= rhs.arr[6];
	this->arr[7] *= rhs.arr[7];
	this->arr[8] *= rhs.arr[8];
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat3
mat3_div_f32(mat3 lhs, f32 rhs){DPZoneScoped;
	Assert(rhs != 0, "mat3 elements cant be divided by zero");
	mat3 result;
	result.arr[0] = lhs.arr[0] / rhs;
	result.arr[1] = lhs.arr[1] / rhs;
	result.arr[2] = lhs.arr[2] / rhs;
	result.arr[3] = lhs.arr[3] / rhs;
	result.arr[4] = lhs.arr[4] / rhs;
	result.arr[5] = lhs.arr[5] / rhs;
	result.arr[6] = lhs.arr[6] / rhs;
	result.arr[7] = lhs.arr[7] / rhs;
	result.arr[8] = lhs.arr[8] / rhs;
	return result;
}

#ifdef __cplusplus
inline mat3 mat3::
operator/ (const f32& rhs)const{DPZoneScoped;
	Assert(rhs != 0, "mat3 elements cant be divided by zero");
	mat3 result;
	result.arr[0] = this->arr[0] / rhs;
	result.arr[1] = this->arr[1] / rhs;
	result.arr[2] = this->arr[2] / rhs;
	result.arr[3] = this->arr[3] / rhs;
	result.arr[4] = this->arr[4] / rhs;
	result.arr[5] = this->arr[5] / rhs;
	result.arr[6] = this->arr[6] / rhs;
	result.arr[7] = this->arr[7] / rhs;
	result.arr[8] = this->arr[8] / rhs;
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat3::
operator/=(const f32& rhs){DPZoneScoped;
	Assert(rhs != 0, "mat3 elements cant be divided by zero");
	this->arr[0] /= rhs;
	this->arr[1] /= rhs;
	this->arr[2] /= rhs;
	this->arr[3] /= rhs;
	this->arr[4] /= rhs;
	this->arr[5] /= rhs;
	this->arr[6] /= rhs;
	this->arr[7] /= rhs;
	this->arr[8] /= rhs;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat3
mat3_div_elements(mat3 lhs, mat3 rhs){DPZoneScoped;
	Assert(rhs.arr[0] != 0 && rhs.arr[1] != 0 && rhs.arr[2] != 0 &&
		   rhs.arr[3] != 0 && rhs.arr[4] != 0 && rhs.arr[5] != 0 &&
		   rhs.arr[6] != 0 && rhs.arr[7] != 0 && rhs.arr[8] != 0,
		   "mat3 elements cant be divided by zero");
	mat3 result;
	result.arr[0] = lhs.arr[0] / rhs.arr[0];
	result.arr[1] = lhs.arr[1] / rhs.arr[1];
	result.arr[2] = lhs.arr[2] / rhs.arr[2];
	result.arr[3] = lhs.arr[3] / rhs.arr[3];
	result.arr[4] = lhs.arr[4] / rhs.arr[4];
	result.arr[5] = lhs.arr[5] / rhs.arr[5];
	result.arr[6] = lhs.arr[6] / rhs.arr[6];
	result.arr[7] = lhs.arr[7] / rhs.arr[7];
	result.arr[8] = lhs.arr[8] / rhs.arr[8];
	return result;
}

#ifdef __cplusplus
inline mat3 mat3::
operator% (const mat3& rhs)const{DPZoneScoped;
	Assert(rhs.arr[0] != 0 && rhs.arr[1] != 0 && rhs.arr[2] != 0 &&
		   rhs.arr[3] != 0 && rhs.arr[4] != 0 && rhs.arr[5] != 0 &&
		   rhs.arr[6] != 0 && rhs.arr[7] != 0 && rhs.arr[8] != 0,
		   "mat3 elements cant be divided by zero");
	mat3 result;
	result.arr[0] = this->arr[0] / rhs.arr[0];
	result.arr[1] = this->arr[1] / rhs.arr[1];
	result.arr[2] = this->arr[2] / rhs.arr[2];
	result.arr[3] = this->arr[3] / rhs.arr[3];
	result.arr[4] = this->arr[4] / rhs.arr[4];
	result.arr[5] = this->arr[5] / rhs.arr[5];
	result.arr[6] = this->arr[6] / rhs.arr[6];
	result.arr[7] = this->arr[7] / rhs.arr[7];
	result.arr[8] = this->arr[8] / rhs.arr[8];
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat3::
operator%=(const mat3& rhs){DPZoneScoped;
	Assert(rhs.arr[0] != 0 && rhs.arr[1] != 0 && rhs.arr[2] != 0 &&
		   rhs.arr[3] != 0 && rhs.arr[4] != 0 && rhs.arr[5] != 0 &&
		   rhs.arr[6] != 0 && rhs.arr[7] != 0 && rhs.arr[8] != 0,
		   "mat3 elements cant be divided by zero");
	mat3 result;
	this->arr[0] /= rhs.arr[0];
	this->arr[1] /= rhs.arr[1];
	this->arr[2] /= rhs.arr[2];
	this->arr[3] /= rhs.arr[3];
	this->arr[4] /= rhs.arr[4];
	this->arr[5] /= rhs.arr[5];
	this->arr[6] /= rhs.arr[6];
	this->arr[7] /= rhs.arr[7];
	this->arr[8] /= rhs.arr[8];
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
mat3_equal(mat3 lhs, mat3 rhs){DPZoneScoped;
	return DESHI_ABSF(lhs.arr[0] - rhs.arr[0]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[1] - rhs.arr[1]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[2] - rhs.arr[2]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[3] - rhs.arr[3]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[4] - rhs.arr[4]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[5] - rhs.arr[5]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[6] - rhs.arr[6]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[7] - rhs.arr[7]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[8] - rhs.arr[8]) > M_EPSILON;
}

#ifdef __cplusplus
inline b32 mat3::
operator==(const mat3& rhs)const{DPZoneScoped;
	return DESHI_ABSF(this->arr[0] - rhs.arr[0]) > M_EPSILON
		&& DESHI_ABSF(this->arr[1] - rhs.arr[1]) > M_EPSILON
		&& DESHI_ABSF(this->arr[2] - rhs.arr[2]) > M_EPSILON
		&& DESHI_ABSF(this->arr[3] - rhs.arr[3]) > M_EPSILON
		&& DESHI_ABSF(this->arr[4] - rhs.arr[4]) > M_EPSILON
		&& DESHI_ABSF(this->arr[5] - rhs.arr[5]) > M_EPSILON
		&& DESHI_ABSF(this->arr[6] - rhs.arr[6]) > M_EPSILON
		&& DESHI_ABSF(this->arr[7] - rhs.arr[7]) > M_EPSILON
		&& DESHI_ABSF(this->arr[8] - rhs.arr[8]) > M_EPSILON;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
mat3_nequal(mat3 lhs, mat3 rhs){DPZoneScoped;
	return DESHI_ABSF(lhs.arr[0] - rhs.arr[0]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[1] - rhs.arr[1]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[2] - rhs.arr[2]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[3] - rhs.arr[3]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[4] - rhs.arr[4]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[5] - rhs.arr[5]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[6] - rhs.arr[6]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[7] - rhs.arr[7]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[8] - rhs.arr[8]) < M_EPSILON;
}

#ifdef __cplusplus
inline b32 mat3::
operator!=(const mat3& rhs)const{DPZoneScoped;
	return DESHI_ABSF(this->arr[0] - rhs.arr[0]) < M_EPSILON
		|| DESHI_ABSF(this->arr[1] - rhs.arr[1]) < M_EPSILON
		|| DESHI_ABSF(this->arr[2] - rhs.arr[2]) < M_EPSILON
		|| DESHI_ABSF(this->arr[3] - rhs.arr[3]) < M_EPSILON
		|| DESHI_ABSF(this->arr[4] - rhs.arr[4]) < M_EPSILON
		|| DESHI_ABSF(this->arr[5] - rhs.arr[5]) < M_EPSILON
		|| DESHI_ABSF(this->arr[6] - rhs.arr[6]) < M_EPSILON
		|| DESHI_ABSF(this->arr[7] - rhs.arr[7]) < M_EPSILON
		|| DESHI_ABSF(this->arr[8] - rhs.arr[8]) < M_EPSILON;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
mat3_transpose(mat3 lhs){DPZoneScoped;
	return Mat3(lhs.arr[0], lhs.arr[3], lhs.arr[6],
				lhs.arr[1], lhs.arr[4], lhs.arr[7],
				lhs.arr[2], lhs.arr[5], lhs.arr[8]);
}

#ifdef __cplusplus
inline mat3 mat3::
transpose()const{DPZoneScoped;
	return Mat3(this->arr[0], this->arr[3], this->arr[6],
				this->arr[1], this->arr[4], this->arr[7],
				this->arr[2], this->arr[5], this->arr[8]);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
mat3_determinant(mat3 lhs){DPZoneScoped;
	f32 aei = lhs.arr[0] * lhs.arr[4] * lhs.arr[8];
	f32 bfg = lhs.arr[1] * lhs.arr[5] * lhs.arr[6];
	f32 cdh = lhs.arr[2] * lhs.arr[3] * lhs.arr[7];
	f32 ceg = lhs.arr[2] * lhs.arr[4] * lhs.arr[6];
	f32 bdi = lhs.arr[1] * lhs.arr[3] * lhs.arr[8];
	f32 afh = lhs.arr[0] * lhs.arr[5] * lhs.arr[7];
	return aei + bfg + cdh - ceg - bdi - afh;
}

#ifdef __cplusplus
inline f32 mat3::
determinant()const{DPZoneScoped;
	f32 aei = this->arr[0] * this->arr[4] * this->arr[8];
	f32 bfg = this->arr[1] * this->arr[5] * this->arr[6];
	f32 cdh = this->arr[2] * this->arr[3] * this->arr[7];
	f32 ceg = this->arr[2] * this->arr[4] * this->arr[6];
	f32 bdi = this->arr[1] * this->arr[3] * this->arr[8];
	f32 afh = this->arr[0] * this->arr[5] * this->arr[7];
	return aei + bfg + cdh - ceg - bdi - afh;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
mat3_minor(mat3 lhs, u32 row, u32 col){DPZoneScoped;
	Assert(row < 3 && col < 3, "mat3 subscript out of bounds");
	switch(row){
		case 0:{
			switch(col){
				case 0: return (lhs.arr[4] * lhs.arr[5]) - (lhs.arr[7] * lhs.arr[8]);
				case 1: return (lhs.arr[3] * lhs.arr[5]) - (lhs.arr[6] * lhs.arr[8]);
				case 2: return (lhs.arr[4] * lhs.arr[4]) - (lhs.arr[6] * lhs.arr[7]);
			}
		}break;
		case 1:{
			switch(col){
				case 0: return (lhs.arr[1] * lhs.arr[2]) - (lhs.arr[7] * lhs.arr[8]);
				case 1: return (lhs.arr[0] * lhs.arr[2]) - (lhs.arr[6] * lhs.arr[8]);
				case 2: return (lhs.arr[0] * lhs.arr[1]) - (lhs.arr[6] * lhs.arr[7]);
			}
		}break;
		case 2:{
			switch(col){
				case 0: return (lhs.arr[1] * lhs.arr[2]) - (lhs.arr[4] * lhs.arr[5]);
				case 1: return (lhs.arr[0] * lhs.arr[2]) - (lhs.arr[3] * lhs.arr[5]);
				case 2: return (lhs.arr[0] * lhs.arr[1]) - (lhs.arr[3] * lhs.arr[4]);
			}
		}break;
	}
}

#ifdef __cplusplus
inline f32 mat3::
minor(u32 row, u32 col)const{DPZoneScoped;
	Assert(row < 3 && col < 3, "mat3 subscript out of bounds");
	switch(row){
		case 0:{
			switch(col){
				case 0: return (this->arr[4] * this->arr[5]) - (this->arr[7] * this->arr[8]);
				case 1: return (this->arr[3] * this->arr[5]) - (this->arr[6] * this->arr[8]);
				case 2: return (this->arr[4] * this->arr[4]) - (this->arr[6] * this->arr[7]);
			}
		}break;
		case 1:{
			switch(col){
				case 0: return (this->arr[1] * this->arr[2]) - (this->arr[7] * this->arr[8]);
				case 1: return (this->arr[0] * this->arr[2]) - (this->arr[6] * this->arr[8]);
				case 2: return (this->arr[0] * this->arr[1]) - (this->arr[6] * this->arr[7]);
			}
		}break;
		case 2:{
			switch(col){
				case 0: return (this->arr[1] * this->arr[2]) - (this->arr[4] * this->arr[5]);
				case 1: return (this->arr[0] * this->arr[2]) - (this->arr[3] * this->arr[5]);
				case 2: return (this->arr[0] * this->arr[1]) - (this->arr[3] * this->arr[4]);
			}
		}break;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
mat3_cofactor(mat3 lhs, u32 row, u32 col){DPZoneScoped;
	Assert(row < 3 && col < 3, "mat3 subscript out of bounds");
	switch(row){
		case 0:{
			switch(col){
				case 0: return (lhs.arr[4] * lhs.arr[5]) - (lhs.arr[7] * lhs.arr[8]);
				case 1: return (lhs.arr[6] * lhs.arr[8]) - (lhs.arr[3] * lhs.arr[5]);
				case 2: return (lhs.arr[4] * lhs.arr[4]) - (lhs.arr[6] * lhs.arr[7]);
			}
		}break;
		case 1:{
			switch(col){
				case 0: return (lhs.arr[7] * lhs.arr[8]) - (lhs.arr[1] * lhs.arr[2]);
				case 1: return (lhs.arr[0] * lhs.arr[2]) - (lhs.arr[6] * lhs.arr[8]);
				case 2: return (lhs.arr[6] * lhs.arr[7]) - (lhs.arr[0] * lhs.arr[1]);
			}
		}break;
		case 2:{
			switch(col){
				case 0: return (lhs.arr[1] * lhs.arr[2]) - (lhs.arr[4] * lhs.arr[5]);
				case 1: return (lhs.arr[3] * lhs.arr[5]) - (lhs.arr[0] * lhs.arr[2]);
				case 2: return (lhs.arr[0] * lhs.arr[1]) - (lhs.arr[3] * lhs.arr[4]);
			}
		}break;
	}
}

#ifdef __cplusplus
inline f32 mat3::
cofactor(u32 row, u32 col)const{DPZoneScoped;
	Assert(row < 3 && col < 3, "mat3 subscript out of bounds");
	switch(row){
		case 0:{
			switch(col){
				case 0: return (this->arr[4] * this->arr[5]) - (this->arr[7] * this->arr[8]);
				case 1: return (this->arr[6] * this->arr[8]) - (this->arr[3] * this->arr[5]);
				case 2: return (this->arr[4] * this->arr[4]) - (this->arr[6] * this->arr[7]);
			}
		}break;
		case 1:{
			switch(col){
				case 0: return (this->arr[7] * this->arr[8]) - (this->arr[1] * this->arr[2]);
				case 1: return (this->arr[0] * this->arr[2]) - (this->arr[6] * this->arr[8]);
				case 2: return (this->arr[6] * this->arr[7]) - (this->arr[0] * this->arr[1]);
			}
		}break;
		case 2:{
			switch(col){
				case 0: return (this->arr[1] * this->arr[2]) - (this->arr[4] * this->arr[5]);
				case 1: return (this->arr[3] * this->arr[5]) - (this->arr[0] * this->arr[2]);
				case 2: return (this->arr[0] * this->arr[1]) - (this->arr[3] * this->arr[4]);
			}
		}break;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat3
mat3_adjoint(mat3 lhs){DPZoneScoped;
	mat3 result;
	result.arr[0] = (lhs.arr[4] * lhs.arr[5]) - (lhs.arr[7] * lhs.arr[8]);
	result.arr[1] = (lhs.arr[7] * lhs.arr[8]) - (lhs.arr[1] * lhs.arr[2]);
	result.arr[2] = (lhs.arr[1] * lhs.arr[2]) - (lhs.arr[4] * lhs.arr[5]);
	result.arr[3] = (lhs.arr[6] * lhs.arr[8]) - (lhs.arr[3] * lhs.arr[5]);
	result.arr[4] = (lhs.arr[0] * lhs.arr[2]) - (lhs.arr[6] * lhs.arr[8]);
	result.arr[5] = (lhs.arr[3] * lhs.arr[5]) - (lhs.arr[0] * lhs.arr[2]);
	result.arr[6] = (lhs.arr[4] * lhs.arr[4]) - (lhs.arr[6] * lhs.arr[7]);
	result.arr[7] = (lhs.arr[6] * lhs.arr[7]) - (lhs.arr[0] * lhs.arr[1]);
	result.arr[8] = (lhs.arr[0] * lhs.arr[1]) - (lhs.arr[3] * lhs.arr[4]);
	return result;
}

#ifdef __cplusplus
inline mat3 mat3::
adjoint()const{DPZoneScoped;
	mat3 result;
	result.arr[0] = (this->arr[4] * this->arr[5]) - (this->arr[7] * this->arr[8]);
	result.arr[1] = (this->arr[7] * this->arr[8]) - (this->arr[1] * this->arr[2]);
	result.arr[2] = (this->arr[1] * this->arr[2]) - (this->arr[4] * this->arr[5]);
	result.arr[3] = (this->arr[6] * this->arr[8]) - (this->arr[3] * this->arr[5]);
	result.arr[4] = (this->arr[0] * this->arr[2]) - (this->arr[6] * this->arr[8]);
	result.arr[5] = (this->arr[3] * this->arr[5]) - (this->arr[0] * this->arr[2]);
	result.arr[6] = (this->arr[4] * this->arr[4]) - (this->arr[6] * this->arr[7]);
	result.arr[7] = (this->arr[6] * this->arr[7]) - (this->arr[0] * this->arr[1]);
	result.arr[8] = (this->arr[0] * this->arr[1]) - (this->arr[3] * this->arr[4]);
	return result;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat3
mat3_inverse(mat3 lhs){DPZoneScoped;
	f32 d = mat3_determinant(lhs);
	mat3 result;
	result.arr[0] = ((lhs.arr[4] * lhs.arr[5]) - (lhs.arr[7] * lhs.arr[8])) / d;
	result.arr[1] = ((lhs.arr[7] * lhs.arr[8]) - (lhs.arr[1] * lhs.arr[2])) / d;
	result.arr[2] = ((lhs.arr[1] * lhs.arr[2]) - (lhs.arr[4] * lhs.arr[5])) / d;
	result.arr[3] = ((lhs.arr[6] * lhs.arr[8]) - (lhs.arr[3] * lhs.arr[5])) / d;
	result.arr[4] = ((lhs.arr[0] * lhs.arr[2]) - (lhs.arr[6] * lhs.arr[8])) / d;
	result.arr[5] = ((lhs.arr[3] * lhs.arr[5]) - (lhs.arr[0] * lhs.arr[2])) / d;
	result.arr[6] = ((lhs.arr[4] * lhs.arr[4]) - (lhs.arr[6] * lhs.arr[7])) / d;
	result.arr[7] = ((lhs.arr[6] * lhs.arr[7]) - (lhs.arr[0] * lhs.arr[1])) / d;
	result.arr[8] = ((lhs.arr[0] * lhs.arr[1]) - (lhs.arr[3] * lhs.arr[4])) / d;
	return result;
}

#ifdef __cplusplus
inline mat3 mat3::
inverse()const{DPZoneScoped;
	f32 d = this->determinant(lhs);
	mat3 result;
	result.arr[0] = ((lhs.arr[4] * lhs.arr[5]) - (lhs.arr[7] * lhs.arr[8])) / d;
	result.arr[1] = ((lhs.arr[7] * lhs.arr[8]) - (lhs.arr[1] * lhs.arr[2])) / d;
	result.arr[2] = ((lhs.arr[1] * lhs.arr[2]) - (lhs.arr[4] * lhs.arr[5])) / d;
	result.arr[3] = ((lhs.arr[6] * lhs.arr[8]) - (lhs.arr[3] * lhs.arr[5])) / d;
	result.arr[4] = ((lhs.arr[0] * lhs.arr[2]) - (lhs.arr[6] * lhs.arr[8])) / d;
	result.arr[5] = ((lhs.arr[3] * lhs.arr[5]) - (lhs.arr[0] * lhs.arr[2])) / d;
	result.arr[6] = ((lhs.arr[4] * lhs.arr[4]) - (lhs.arr[6] * lhs.arr[7])) / d;
	result.arr[7] = ((lhs.arr[6] * lhs.arr[7]) - (lhs.arr[0] * lhs.arr[1])) / d;
	result.arr[8] = ((lhs.arr[0] * lhs.arr[1]) - (lhs.arr[3] * lhs.arr[4])) / d;
	return result;
}
#endif //#ifdef __cplusplus

//returns a LH rotation matrix based on input in radians
DESHI_MATH_FUNC inline mat3
mat3_rotation_matrix_x_radians(f32 angle){DPZoneScoped;
	f32 c = DESHI_COSF(angle); f32 s = DESHI_SINF(angle);
	return Mat3(1, 0, 0,
				0, c, s,
				0,-s, c);
}

//returns a LH rotation matrix based on input in degrees
#define mat3_rotation_matrix_x_degrees(angle) mat3_rotation_matrix_x_radians(DESHI_DEGREES_TO_RADIANS_F32(angle))

//returns a LH rotation matrix based on input in radians
DESHI_MATH_FUNC inline mat3
mat3_rotation_matrix_y_radians(f32 angle){DPZoneScoped;
	f32 c = DESHI_COSF(angle); f32 s = DESHI_SINF(angle);
	return Mat3(c, 0,-s,
				0, 1, 0,
				s, 0, c);
}

//returns a LH rotation matrix based on input in degrees
#define mat3_rotation_matrix_y_degrees(angle) mat3_rotation_matrix_y_radians(DESHI_DEGREES_TO_RADIANS_F32(angle))

//returns a LH rotation matrix based on input in radians
DESHI_MATH_FUNC inline mat3
mat3_rotation_matrix_z_radians(f32 angle){DPZoneScoped;
	f32 c = DESHI_COSF(angle); f32 s = DESHI_SINF(angle);
	return Mat3( c, s, 0,
				-s, c, 0,
				0,  0, 1);
}

//returns a LH rotation matrix based on input in degrees
#define mat3_rotation_matrix_z_degrees(angle) mat3_rotation_matrix_z_radians(DESHI_DEGREES_TO_RADIANS_F32(angle))

//returns a pre-multiplied X->Y->Z LH rotation matrix based on input in radians
DESHI_MATH_FUNC inline mat3
mat3_rotation_matrix_radians(f32 x, f32 y, f32 z){DPZoneScoped;
	f32 cX = DESHI_COSF(x); f32 sX = DESHI_SINF(x);
	f32 cY = DESHI_COSF(y); f32 sY = DESHI_SINF(y);
	f32 cZ = DESHI_COSF(z); f32 sZ = DESHI_SINF(z);
	mat3 result;
	result.arr[0] = cZ*cY;
	result.arr[1] = cY*sZ;
	result.arr[2] = -sY;
	result.arr[3] = cZ*sX*sY - cX*sZ;
	result.arr[4] = cZ*cX + sX*sY*sZ;
	result.arr[5] = sX*cY;
	result.arr[6] = cZ*cX*sY + sX*sZ;
	result.arr[7] = cX*sY*sZ - cZ*sX;
	result.arr[8] = cX*cY;
	return result;
}

//returns a pre-multiplied X->Y->Z LH rotation matrix based on input in degrees
#define mat3_rotation_matrix_degrees(x,y,z) mat3_rotation_matrix_radians(DESHI_DEGREES_TO_RADIANS_F32(x), DESHI_DEGREES_TO_RADIANS_F32(y), DESHI_DEGREES_TO_RADIANS_F32(z))

DESHI_MATH_FUNC inline mat3
mat3_scale_matrix(f32 x, f32 y, f32 z){DPZoneScoped;
	return Mat3(x, 0, 0,
				0, y, 0,
				0, 0, z);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @mat4


DESHI_MATH_FUNC typedef struct mat4{
	union{
		f32 arr[16];
		struct{
			vec4 row0;
			vec4 row1;
			vec4 row2;
			vec4 row3;
		};
#if DESHI_MATH_USE_SSE
		struct{
			__m128 sse_row0;
			__m128 sse_row1;
			__m128 sse_row2;
			__m128 sse_row3;
		};
#endif //#if DESHI_MATH_USE_SSE
	};
	
#ifdef __cplusplus
	static constexpr mat4 ZERO     = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	static constexpr mat4 ONE      = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
	static constexpr mat4 IDENTITY = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
	f32  operator()(u32 row, u32 col)const;
	f32& operator()(u32 row, u32 col);
	f32  operator[](u32 index)const;
	f32& operator[](u32 index);
	mat4 operator+ (const mat4& rhs)const;
	void operator+=(const mat4& rhs);
	mat4 operator- (const mat4& rhs)const;
	void operator-=(const mat4& rhs);
	mat4 operator* (const f32& rhs)const;
	void operator*=(const f32& rhs);
	mat4 operator* (const mat4& rhs)const;
	void operator*=(const mat4& rhs);
	mat4 operator^ (const mat4& rhs)const;
	void operator^=(const mat4& rhs);
	mat4 operator/ (const f32& rhs)const;
	void operator/=(const f32& rhs);
	mat4 operator% (const mat4& rhs)const;
	void operator%=(const mat4& rhs);
	b32  operator==(const mat4& rhs)const;
	b32  operator!=(const mat4& rhs)const;
	mat4 transpose()const;
	f32  determinant()const;
	f32  minor(u32 row, u32 col)const;
	f32  cofactor(u32 row, u32 col)const;
	mat4 adjoint()const;
	mat4 inverse()const;
	mat3 to_mat3()const;
	vec4 row(u32 row)const;
	vec4 col(u32 col)const;
#endif //#ifdef __cplusplus
} mat4;

DESHI_MATH_FUNC inline mat4
Mat4(f32 _00, f32 _10, f32 _20, f32 _30
	 f32 _01, f32 _11, f32 _21, f32 _31
	 f32 _02, f32 _12, f32 _22, f32 _32,
	 f32 _03, f32 _13, f32 _23, f32 _33){
	return mat4{_00, _10, _20, _30, _01, _11, _21, _31, _02, _12, _22, _32, _03, _13, _23, _33};
}

DESHI_MATH_FUNC inline mat4
array_to_mat4(f32* arr){
	return mat4{arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8], arr[9], arr[10], arr[11], arr[12], arr[13], arr[14], arr[15]};
}

DESHI_MATH_FUNC inline mat4 mat4_ZERO()    { return mat4{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; }
DESHI_MATH_FUNC inline mat4 mat4_ONE()     { return mat4{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}; }
DESHI_MATH_FUNC inline mat4 mat4_IDENTITY(){ return mat4{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}; }

#define mat4_coord(m,row,col) m.arr[4*row + col]

#ifdef __cplusplus
inline f32 mat4::
operator()(u32 row, u32 col)const{DPZoneScoped;
	Assert(row < 4 && col < 4, "mat4 subscript out of bounds");
	return this->arr[4*row + col];
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline f32& mat4::
operator()(u32 row, u32 col){DPZoneScoped;
	Assert(row < 4 && col < 4, "mat4 subscript out of bounds");
	return this->arr[4*row + col];
}
#endif //#ifdef __cplusplus

#define mat4_index(m,index) m.arr[index]

#ifdef __cplusplus
inline f32 mat4::
operator[](u32 index)const{DPZoneScoped;
	Assert(index < 16, "mat4 subscript out of bounds");
	return this->arr[index];
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline f32& mat4::
operator[](u32 index)const{DPZoneScoped;
	Assert(index < 16, "mat4 subscript out of bounds");
	return this->arr[index];
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat4
mat4_add_elements(mat4 lhs, mat4 rhs){DPZoneScoped;
	mat4 result;
#if DESHI_MATH_USE_SSE
	result.sse_row0 = m128_add_4f32(lhs.sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_add_4f32(lhs.sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_add_4f32(lhs.sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_add_4f32(lhs.sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	result.arr[0] = lhs.arr[0] + rhs.arr[0];
	result.arr[1] = lhs.arr[1] + rhs.arr[1];
	result.arr[2] = lhs.arr[2] + rhs.arr[2];
	result.arr[3] = lhs.arr[3] + rhs.arr[3];
	result.arr[4] = lhs.arr[4] + rhs.arr[4];
	result.arr[5] = lhs.arr[5] + rhs.arr[5];
	result.arr[6] = lhs.arr[6] + rhs.arr[6];
	result.arr[7] = lhs.arr[7] + rhs.arr[7];
	result.arr[8] = lhs.arr[8] + rhs.arr[8];
	result.arr[9] = lhs.arr[9] + rhs.arr[9];
	result.arr[10] = lhs.arr[10] + rhs.arr[10];
	result.arr[11] = lhs.arr[11] + rhs.arr[11];
	result.arr[12] = lhs.arr[12] + rhs.arr[12];
	result.arr[13] = lhs.arr[13] + rhs.arr[13];
	result.arr[14] = lhs.arr[14] + rhs.arr[14];
	result.arr[15] = lhs.arr[15] + rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline mat4 mat4::
operator+ (const mat4& rhs)const{DPZoneScoped;
	mat4 result;
#if DESHI_MATH_USE_SSE
	result.sse_row0 = m128_add_4f32(this->sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_add_4f32(this->sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_add_4f32(this->sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_add_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	result.arr[0] = this->arr[0] + rhs.arr[0];
	result.arr[1] = this->arr[1] + rhs.arr[1];
	result.arr[2] = this->arr[2] + rhs.arr[2];
	result.arr[3] = this->arr[3] + rhs.arr[3];
	result.arr[4] = this->arr[4] + rhs.arr[4];
	result.arr[5] = this->arr[5] + rhs.arr[5];
	result.arr[6] = this->arr[6] + rhs.arr[6];
	result.arr[7] = this->arr[7] + rhs.arr[7];
	result.arr[8] = this->arr[8] + rhs.arr[8];
	result.arr[9] = this->arr[9] + rhs.arr[9];
	result.arr[10] = this->arr[10] + rhs.arr[10];
	result.arr[11] = this->arr[11] + rhs.arr[11];
	result.arr[12] = this->arr[12] + rhs.arr[12];
	result.arr[13] = this->arr[13] + rhs.arr[13];
	result.arr[14] = this->arr[14] + rhs.arr[14];
	result.arr[15] = this->arr[15] + rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat4::
operator+=(const mat4& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	this->sse_row0 = m128_add_4f32(this->sse_row0, rhs.sse_row0);
	this->sse_row1 = m128_add_4f32(this->sse_row1, rhs.sse_row1);
	this->sse_row2 = m128_add_4f32(this->sse_row2, rhs.sse_row2);
	this->sse_row3 = m128_add_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	this->arr[0] += rhs.arr[0];
	this->arr[1] += rhs.arr[1];
	this->arr[2] += rhs.arr[2];
	this->arr[3] += rhs.arr[3];
	this->arr[4] += rhs.arr[4];
	this->arr[5] += rhs.arr[5];
	this->arr[6] += rhs.arr[6];
	this->arr[7] += rhs.arr[7];
	this->arr[8] += rhs.arr[8];
	this->arr[9] += rhs.arr[9];
	this->arr[10] += rhs.arr[10];
	this->arr[11] += rhs.arr[11];
	this->arr[12] += rhs.arr[12];
	this->arr[13] += rhs.arr[13];
	this->arr[14] += rhs.arr[14];
	this->arr[15] += rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat4
mat4_sub_elements(mat4 lhs, mat4 rhs){DPZoneScoped;
	mat4 result;
#if DESHI_MATH_USE_SSE
	result.sse_row0 = m128_sub_4f32(lhs.sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_sub_4f32(lhs.sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_sub_4f32(lhs.sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_sub_4f32(lhs.sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	result.arr[0] = lhs.arr[0] - rhs.arr[0];
	result.arr[1] = lhs.arr[1] - rhs.arr[1];
	result.arr[2] = lhs.arr[2] - rhs.arr[2];
	result.arr[3] = lhs.arr[3] - rhs.arr[3];
	result.arr[4] = lhs.arr[4] - rhs.arr[4];
	result.arr[5] = lhs.arr[5] - rhs.arr[5];
	result.arr[6] = lhs.arr[6] - rhs.arr[6];
	result.arr[7] = lhs.arr[7] - rhs.arr[7];
	result.arr[8] = lhs.arr[8] - rhs.arr[8];
	result.arr[9] = lhs.arr[9] - rhs.arr[9];
	result.arr[10] = lhs.arr[10] - rhs.arr[10];
	result.arr[11] = lhs.arr[11] - rhs.arr[11];
	result.arr[12] = lhs.arr[12] - rhs.arr[12];
	result.arr[13] = lhs.arr[13] - rhs.arr[13];
	result.arr[14] = lhs.arr[14] - rhs.arr[14];
	result.arr[15] = lhs.arr[15] - rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline mat4 mat4::
operator- (const mat4& rhs)const{DPZoneScoped;
	mat4 result;
#if DESHI_MATH_USE_SSE
	result.sse_row0 = m128_sub_4f32(this->sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_sub_4f32(this->sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_sub_4f32(this->sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_sub_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	result.arr[0] = this->arr[0] - rhs.arr[0];
	result.arr[1] = this->arr[1] - rhs.arr[1];
	result.arr[2] = this->arr[2] - rhs.arr[2];
	result.arr[3] = this->arr[3] - rhs.arr[3];
	result.arr[4] = this->arr[4] - rhs.arr[4];
	result.arr[5] = this->arr[5] - rhs.arr[5];
	result.arr[6] = this->arr[6] - rhs.arr[6];
	result.arr[7] = this->arr[7] - rhs.arr[7];
	result.arr[8] = this->arr[8] - rhs.arr[8];
	result.arr[9] = this->arr[9] - rhs.arr[9];
	result.arr[10] = this->arr[10] - rhs.arr[10];
	result.arr[11] = this->arr[11] - rhs.arr[11];
	result.arr[12] = this->arr[12] - rhs.arr[12];
	result.arr[13] = this->arr[13] - rhs.arr[13];
	result.arr[14] = this->arr[14] - rhs.arr[14];
	result.arr[15] = this->arr[15] - rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat4::
operator-=(const mat4& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	this->sse_row0 = m128_sub_4f32(this->sse_row0, rhs.sse_row0);
	this->sse_row1 = m128_sub_4f32(this->sse_row1, rhs.sse_row1);
	this->sse_row2 = m128_sub_4f32(this->sse_row2, rhs.sse_row2);
	this->sse_row3 = m128_sub_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	this->arr[0] -= rhs.arr[0];
	this->arr[1] -= rhs.arr[1];
	this->arr[2] -= rhs.arr[2];
	this->arr[3] -= rhs.arr[3];
	this->arr[4] -= rhs.arr[4];
	this->arr[5] -= rhs.arr[5];
	this->arr[6] -= rhs.arr[6];
	this->arr[7] -= rhs.arr[7];
	this->arr[8] -= rhs.arr[8];
	this->arr[9] -= rhs.arr[9];
	this->arr[10] -= rhs.arr[10];
	this->arr[11] -= rhs.arr[11];
	this->arr[12] -= rhs.arr[12];
	this->arr[13] -= rhs.arr[13];
	this->arr[14] -= rhs.arr[14];
	this->arr[15] -= rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat4
mat4_mul_f32(mat4 lhs, f32 rhs){DPZoneScoped;
	mat4 result;
#if DESHI_MATH_USE_SSE
	__m128 scalar = m128_fill_4f32(rhs);
	result.sse_row0 = m128_mul_4f32(lhs.sse_row0, scalar);
	result.sse_row1 = m128_mul_4f32(lhs.sse_row1, scalar);
	result.sse_row2 = m128_mul_4f32(lhs.sse_row2, scalar);
	result.sse_row3 = m128_mul_4f32(lhs.sse_row3, scalar);
#else //#if DESHI_MATH_USE_SSE
	result.arr[0] = lhs.arr[0] * rhs;
	result.arr[1] = lhs.arr[1] * rhs;
	result.arr[2] = lhs.arr[2] * rhs;
	result.arr[3] = lhs.arr[3] * rhs;
	result.arr[4] = lhs.arr[4] * rhs;
	result.arr[5] = lhs.arr[5] * rhs;
	result.arr[6] = lhs.arr[6] * rhs;
	result.arr[7] = lhs.arr[7] * rhs;
	result.arr[8] = lhs.arr[8] * rhs;
	result.arr[9] = lhs.arr[9] * rhs;
	result.arr[10] = lhs.arr[10] * rhs;
	result.arr[11] = lhs.arr[11] * rhs;
	result.arr[12] = lhs.arr[12] * rhs;
	result.arr[13] = lhs.arr[13] * rhs;
	result.arr[14] = lhs.arr[14] * rhs;
	result.arr[15] = lhs.arr[15] * rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline mat4 mat4::
operator* (const f32& rhs)const{DPZoneScoped;
	mat4 result;
#if DESHI_MATH_USE_SSE
	__m128 scalar = m128_fill_4f32(rhs);
	result.sse_row0 = m128_mul_4f32(this->sse_row0, scalar);
	result.sse_row1 = m128_mul_4f32(this->sse_row1, scalar);
	result.sse_row2 = m128_mul_4f32(this->sse_row2, scalar);
	result.sse_row3 = m128_mul_4f32(this->sse_row3, scalar);
#else //#if DESHI_MATH_USE_SSE
	result.arr[0] = this->arr[0] * rhs;
	result.arr[1] = this->arr[1] * rhs;
	result.arr[2] = this->arr[2] * rhs;
	result.arr[3] = this->arr[3] * rhs;
	result.arr[4] = this->arr[4] * rhs;
	result.arr[5] = this->arr[5] * rhs;
	result.arr[6] = this->arr[6] * rhs;
	result.arr[7] = this->arr[7] * rhs;
	result.arr[8] = this->arr[8] * rhs;
	result.arr[9] = this->arr[9] * rhs;
	result.arr[10] = this->arr[10] * rhs;
	result.arr[11] = this->arr[11] * rhs;
	result.arr[12] = this->arr[12] * rhs;
	result.arr[13] = this->arr[13] * rhs;
	result.arr[14] = this->arr[14] * rhs;
	result.arr[15] = this->arr[15] * rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat4::
operator*=(const f32& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	__m128 scalar = m128_fill_4f32(rhs);
	this->sse_row0 = m128_mul_4f32(this->sse_row0, scalar);
	this->sse_row1 = m128_mul_4f32(this->sse_row1, scalar);
	this->sse_row2 = m128_mul_4f32(this->sse_row2, scalar);
	this->sse_row3 = m128_mul_4f32(this->sse_row3, scalar);
#else //#if DESHI_MATH_USE_SSE
	this->arr[0] *= rhs;
	this->arr[1] *= rhs;
	this->arr[2] *= rhs;
	this->arr[3] *= rhs;
	this->arr[4] *= rhs;
	this->arr[5] *= rhs;
	this->arr[6] *= rhs;
	this->arr[7] *= rhs;
	this->arr[8] *= rhs;
	this->arr[9] *= rhs;
	this->arr[10] *= rhs;
	this->arr[11] *= rhs;
	this->arr[12] *= rhs;
	this->arr[13] *= rhs;
	this->arr[14] *= rhs;
	this->arr[15] *= rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline mat4 mat4::
operator* (const f32& lhs, const mat4& rhs)const{
	return rhs * lhs;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat4
mat4_mul_mat4(mat4 lhs, mat4 rhs){DPZoneScoped;
	mat4 result;
#if DESHI_MATH_USE_SSE
	result.sse_row0 = m128_linear_combine(lhs.sse_row0, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row1 = m128_linear_combine(lhs.sse_row1, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row2 = m128_linear_combine(lhs.sse_row2, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row3 = m128_linear_combine(lhs.sse_row3, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	result.arr[ 0] = lhs.arr[ 0]*rhs.arr[ 0] + lhs.arr[ 1]*rhs.arr[ 4] + lhs.arr[ 2]*rhs.arr[ 8] + lhs.arr[ 3]*rhs.arr[12];
	result.arr[ 1] = lhs.arr[ 0]*rhs.arr[ 1] + lhs.arr[ 1]*rhs.arr[ 5] + lhs.arr[ 2]*rhs.arr[ 9] + lhs.arr[ 3]*rhs.arr[13];
	result.arr[ 2] = lhs.arr[ 0]*rhs.arr[ 2] + lhs.arr[ 1]*rhs.arr[ 6] + lhs.arr[ 2]*rhs.arr[10] + lhs.arr[ 3]*rhs.arr[14];
	result.arr[ 3] = lhs.arr[ 0]*rhs.arr[ 3] + lhs.arr[ 1]*rhs.arr[ 7] + lhs.arr[ 2]*rhs.arr[11] + lhs.arr[ 3]*rhs.arr[15];
	result.arr[ 4] = lhs.arr[ 4]*rhs.arr[ 0] + lhs.arr[ 5]*rhs.arr[ 4] + lhs.arr[ 6]*rhs.arr[ 8] + lhs.arr[ 7]*rhs.arr[12];
	result.arr[ 5] = lhs.arr[ 4]*rhs.arr[ 1] + lhs.arr[ 5]*rhs.arr[ 5] + lhs.arr[ 6]*rhs.arr[ 9] + lhs.arr[ 7]*rhs.arr[13];
	result.arr[ 6] = lhs.arr[ 4]*rhs.arr[ 2] + lhs.arr[ 5]*rhs.arr[ 6] + lhs.arr[ 6]*rhs.arr[10] + lhs.arr[ 7]*rhs.arr[14];
	result.arr[ 7] = lhs.arr[ 4]*rhs.arr[ 3] + lhs.arr[ 5]*rhs.arr[ 7] + lhs.arr[ 6]*rhs.arr[11] + lhs.arr[ 7]*rhs.arr[15];
	result.arr[ 8] = lhs.arr[ 8]*rhs.arr[ 0] + lhs.arr[ 9]*rhs.arr[ 4] + lhs.arr[10]*rhs.arr[ 8] + lhs.arr[11]*rhs.arr[12];
	result.arr[ 9] = lhs.arr[ 8]*rhs.arr[ 1] + lhs.arr[ 9]*rhs.arr[ 5] + lhs.arr[10]*rhs.arr[ 9] + lhs.arr[11]*rhs.arr[13];
	result.arr[10] = lhs.arr[ 8]*rhs.arr[ 2] + lhs.arr[ 9]*rhs.arr[ 6] + lhs.arr[10]*rhs.arr[10] + lhs.arr[11]*rhs.arr[14];
	result.arr[11] = lhs.arr[ 8]*rhs.arr[ 3] + lhs.arr[ 9]*rhs.arr[ 7] + lhs.arr[10]*rhs.arr[11] + lhs.arr[11]*rhs.arr[15];
	result.arr[12] = lhs.arr[12]*rhs.arr[ 0] + lhs.arr[13]*rhs.arr[ 4] + lhs.arr[14]*rhs.arr[ 8] + lhs.arr[15]*rhs.arr[12];
	result.arr[13] = lhs.arr[12]*rhs.arr[ 1] + lhs.arr[13]*rhs.arr[ 5] + lhs.arr[14]*rhs.arr[ 9] + lhs.arr[16]*rhs.arr[13];
	result.arr[14] = lhs.arr[12]*rhs.arr[ 2] + lhs.arr[13]*rhs.arr[ 6] + lhs.arr[14]*rhs.arr[10] + lhs.arr[16]*rhs.arr[14];
	result.arr[15] = lhs.arr[12]*rhs.arr[ 3] + lhs.arr[13]*rhs.arr[ 7] + lhs.arr[14]*rhs.arr[11] + lhs.arr[16]*rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline mat4 mat4::
operator* (const mat4& rhs)const{DPZoneScoped;
	mat4 result;
#if DESHI_MATH_USE_SSE
	result.sse_row0 = m128_linear_combine(this->sse_row0, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row1 = m128_linear_combine(this->sse_row1, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row2 = m128_linear_combine(this->sse_row2, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row3 = m128_linear_combine(this->sse_row3, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	result.arr[ 0] = this->arr[ 0]*rhs.arr[ 0] + this->arr[ 1]*rhs.arr[ 4] + this->arr[ 2]*rhs.arr[ 8] + this->arr[ 3]*rhs.arr[12];
	result.arr[ 1] = this->arr[ 0]*rhs.arr[ 1] + this->arr[ 1]*rhs.arr[ 5] + this->arr[ 2]*rhs.arr[ 9] + this->arr[ 3]*rhs.arr[13];
	result.arr[ 2] = this->arr[ 0]*rhs.arr[ 2] + this->arr[ 1]*rhs.arr[ 6] + this->arr[ 2]*rhs.arr[10] + this->arr[ 3]*rhs.arr[14];
	result.arr[ 3] = this->arr[ 0]*rhs.arr[ 3] + this->arr[ 1]*rhs.arr[ 7] + this->arr[ 2]*rhs.arr[11] + this->arr[ 3]*rhs.arr[15];
	result.arr[ 4] = this->arr[ 4]*rhs.arr[ 0] + this->arr[ 5]*rhs.arr[ 4] + this->arr[ 6]*rhs.arr[ 8] + this->arr[ 7]*rhs.arr[12];
	result.arr[ 5] = this->arr[ 4]*rhs.arr[ 1] + this->arr[ 5]*rhs.arr[ 5] + this->arr[ 6]*rhs.arr[ 9] + this->arr[ 7]*rhs.arr[13];
	result.arr[ 6] = this->arr[ 4]*rhs.arr[ 2] + this->arr[ 5]*rhs.arr[ 6] + this->arr[ 6]*rhs.arr[10] + this->arr[ 7]*rhs.arr[14];
	result.arr[ 7] = this->arr[ 4]*rhs.arr[ 3] + this->arr[ 5]*rhs.arr[ 7] + this->arr[ 6]*rhs.arr[11] + this->arr[ 7]*rhs.arr[15];
	result.arr[ 8] = this->arr[ 8]*rhs.arr[ 0] + this->arr[ 9]*rhs.arr[ 4] + this->arr[10]*rhs.arr[ 8] + this->arr[11]*rhs.arr[12];
	result.arr[ 9] = this->arr[ 8]*rhs.arr[ 1] + this->arr[ 9]*rhs.arr[ 5] + this->arr[10]*rhs.arr[ 9] + this->arr[11]*rhs.arr[13];
	result.arr[10] = this->arr[ 8]*rhs.arr[ 2] + this->arr[ 9]*rhs.arr[ 6] + this->arr[10]*rhs.arr[10] + this->arr[11]*rhs.arr[14];
	result.arr[11] = this->arr[ 8]*rhs.arr[ 3] + this->arr[ 9]*rhs.arr[ 7] + this->arr[10]*rhs.arr[11] + this->arr[11]*rhs.arr[15];
	result.arr[12] = this->arr[12]*rhs.arr[ 0] + this->arr[13]*rhs.arr[ 4] + this->arr[14]*rhs.arr[ 8] + this->arr[15]*rhs.arr[12];
	result.arr[13] = this->arr[12]*rhs.arr[ 1] + this->arr[13]*rhs.arr[ 5] + this->arr[14]*rhs.arr[ 9] + this->arr[16]*rhs.arr[13];
	result.arr[14] = this->arr[12]*rhs.arr[ 2] + this->arr[13]*rhs.arr[ 6] + this->arr[14]*rhs.arr[10] + this->arr[16]*rhs.arr[14];
	result.arr[15] = this->arr[12]*rhs.arr[ 3] + this->arr[13]*rhs.arr[ 7] + this->arr[14]*rhs.arr[11] + this->arr[16]*rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat4::
operator*=(const mat4& rhs){DPZoneScoped;
	mat4 result;
#if DESHI_MATH_USE_SSE
	result.sse_row0 = m128_linear_combine(this->sse_row0, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row1 = m128_linear_combine(this->sse_row1, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row2 = m128_linear_combine(this->sse_row2, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row3 = m128_linear_combine(this->sse_row3, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	result.arr[ 0] = this->arr[ 0]*rhs.arr[ 0] + this->arr[ 1]*rhs.arr[ 4] + this->arr[ 2]*rhs.arr[ 8] + this->arr[ 3]*rhs.arr[12];
	result.arr[ 1] = this->arr[ 0]*rhs.arr[ 1] + this->arr[ 1]*rhs.arr[ 5] + this->arr[ 2]*rhs.arr[ 9] + this->arr[ 3]*rhs.arr[13];
	result.arr[ 2] = this->arr[ 0]*rhs.arr[ 2] + this->arr[ 1]*rhs.arr[ 6] + this->arr[ 2]*rhs.arr[10] + this->arr[ 3]*rhs.arr[14];
	result.arr[ 3] = this->arr[ 0]*rhs.arr[ 3] + this->arr[ 1]*rhs.arr[ 7] + this->arr[ 2]*rhs.arr[11] + this->arr[ 3]*rhs.arr[15];
	result.arr[ 4] = this->arr[ 4]*rhs.arr[ 0] + this->arr[ 5]*rhs.arr[ 4] + this->arr[ 6]*rhs.arr[ 8] + this->arr[ 7]*rhs.arr[12];
	result.arr[ 5] = this->arr[ 4]*rhs.arr[ 1] + this->arr[ 5]*rhs.arr[ 5] + this->arr[ 6]*rhs.arr[ 9] + this->arr[ 7]*rhs.arr[13];
	result.arr[ 6] = this->arr[ 4]*rhs.arr[ 2] + this->arr[ 5]*rhs.arr[ 6] + this->arr[ 6]*rhs.arr[10] + this->arr[ 7]*rhs.arr[14];
	result.arr[ 7] = this->arr[ 4]*rhs.arr[ 3] + this->arr[ 5]*rhs.arr[ 7] + this->arr[ 6]*rhs.arr[11] + this->arr[ 7]*rhs.arr[15];
	result.arr[ 8] = this->arr[ 8]*rhs.arr[ 0] + this->arr[ 9]*rhs.arr[ 4] + this->arr[10]*rhs.arr[ 8] + this->arr[11]*rhs.arr[12];
	result.arr[ 9] = this->arr[ 8]*rhs.arr[ 1] + this->arr[ 9]*rhs.arr[ 5] + this->arr[10]*rhs.arr[ 9] + this->arr[11]*rhs.arr[13];
	result.arr[10] = this->arr[ 8]*rhs.arr[ 2] + this->arr[ 9]*rhs.arr[ 6] + this->arr[10]*rhs.arr[10] + this->arr[11]*rhs.arr[14];
	result.arr[11] = this->arr[ 8]*rhs.arr[ 3] + this->arr[ 9]*rhs.arr[ 7] + this->arr[10]*rhs.arr[11] + this->arr[11]*rhs.arr[15];
	result.arr[12] = this->arr[12]*rhs.arr[ 0] + this->arr[13]*rhs.arr[ 4] + this->arr[14]*rhs.arr[ 8] + this->arr[15]*rhs.arr[12];
	result.arr[13] = this->arr[12]*rhs.arr[ 1] + this->arr[13]*rhs.arr[ 5] + this->arr[14]*rhs.arr[ 9] + this->arr[16]*rhs.arr[13];
	result.arr[14] = this->arr[12]*rhs.arr[ 2] + this->arr[13]*rhs.arr[ 6] + this->arr[14]*rhs.arr[10] + this->arr[16]*rhs.arr[14];
	result.arr[15] = this->arr[12]*rhs.arr[ 3] + this->arr[13]*rhs.arr[ 7] + this->arr[14]*rhs.arr[11] + this->arr[16]*rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
	*this = result;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat4
mat4_mul_elements(mat4 lhs, mat4 rhs){DPZoneScoped;
	mat4 result;
#if DESHI_MATH_USE_SSE
	result.sse_row0 = m128_mul_4f32(lhs.sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_mul_4f32(lhs.sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_mul_4f32(lhs.sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_mul_4f32(lhs.sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	result.arr[0] = lhs.arr[0] * rhs.arr[0];
	result.arr[1] = lhs.arr[1] * rhs.arr[1];
	result.arr[2] = lhs.arr[2] * rhs.arr[2];
	result.arr[3] = lhs.arr[3] * rhs.arr[3];
	result.arr[4] = lhs.arr[4] * rhs.arr[4];
	result.arr[5] = lhs.arr[5] * rhs.arr[5];
	result.arr[6] = lhs.arr[6] * rhs.arr[6];
	result.arr[7] = lhs.arr[7] * rhs.arr[7];
	result.arr[8] = lhs.arr[8] * rhs.arr[8];
	result.arr[9] = lhs.arr[9] * rhs.arr[9];
	result.arr[10] = lhs.arr[10] * rhs.arr[10];
	result.arr[11] = lhs.arr[11] * rhs.arr[11];
	result.arr[12] = lhs.arr[12] * rhs.arr[12];
	result.arr[13] = lhs.arr[13] * rhs.arr[13];
	result.arr[14] = lhs.arr[14] * rhs.arr[14];
	result.arr[15] = lhs.arr[15] * rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline mat4 mat4::
operator^ (const mat4& rhs)const{DPZoneScoped;
	mat4 result;
#if DESHI_MATH_USE_SSE
	result.sse_row0 = m128_mul_4f32(this->sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_mul_4f32(this->sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_mul_4f32(this->sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_mul_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	result.arr[0] = this->arr[0] * rhs.arr[0];
	result.arr[1] = this->arr[1] * rhs.arr[1];
	result.arr[2] = this->arr[2] * rhs.arr[2];
	result.arr[3] = this->arr[3] * rhs.arr[3];
	result.arr[4] = this->arr[4] * rhs.arr[4];
	result.arr[5] = this->arr[5] * rhs.arr[5];
	result.arr[6] = this->arr[6] * rhs.arr[6];
	result.arr[7] = this->arr[7] * rhs.arr[7];
	result.arr[8] = this->arr[8] * rhs.arr[8];
	result.arr[9] = this->arr[9] * rhs.arr[9];
	result.arr[10] = this->arr[10] * rhs.arr[10];
	result.arr[11] = this->arr[11] * rhs.arr[11];
	result.arr[12] = this->arr[12] * rhs.arr[12];
	result.arr[13] = this->arr[13] * rhs.arr[13];
	result.arr[14] = this->arr[14] * rhs.arr[14];
	result.arr[15] = this->arr[15] * rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
} 
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat4::
operator^=(const mat4& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	this->sse_row0 = m128_mul_4f32(this->sse_row0, rhs.sse_row0);
	this->sse_row1 = m128_mul_4f32(this->sse_row1, rhs.sse_row1);
	this->sse_row2 = m128_mul_4f32(this->sse_row2, rhs.sse_row2);
	this->sse_row3 = m128_mul_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	this->arr[0] *= rhs.arr[0];
	this->arr[1] *= rhs.arr[1];
	this->arr[2] *= rhs.arr[2];
	this->arr[3] *= rhs.arr[3];
	this->arr[4] *= rhs.arr[4];
	this->arr[5] *= rhs.arr[5];
	this->arr[6] *= rhs.arr[6];
	this->arr[7] *= rhs.arr[7];
	this->arr[8] *= rhs.arr[8];
	this->arr[9] *= rhs.arr[9];
	this->arr[10] *= rhs.arr[10];
	this->arr[11] *= rhs.arr[11];
	this->arr[12] *= rhs.arr[12];
	this->arr[13] *= rhs.arr[13];
	this->arr[14] *= rhs.arr[14];
	this->arr[15] *= rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat4
mat4_div_f32(mat4 lhs, f32 rhs){DPZoneScoped;
	Assert(rhs != 0, "mat4 elements cant be divided by zero");
	mat4 result;
#if DESHI_MATH_USE_SSE
	__m128 scalar = m128_fill_4f32(rhs);
	result.sse_row0 = m128_div_4f32(lhs.sse_row0, scalar);
	result.sse_row1 = m128_div_4f32(lhs.sse_row1, scalar);
	result.sse_row2 = m128_div_4f32(lhs.sse_row2, scalar);
	result.sse_row3 = m128_div_4f32(lhs.sse_row3, scalar);
#else //#if DESHI_MATH_USE_SSE
	result.arr[0] = lhs.arr[0] / rhs;
	result.arr[1] = lhs.arr[1] / rhs;
	result.arr[2] = lhs.arr[2] / rhs;
	result.arr[3] = lhs.arr[3] / rhs;
	result.arr[4] = lhs.arr[4] / rhs;
	result.arr[5] = lhs.arr[5] / rhs;
	result.arr[6] = lhs.arr[6] / rhs;
	result.arr[7] = lhs.arr[7] / rhs;
	result.arr[8] = lhs.arr[8] / rhs;
	result.arr[9] = lhs.arr[9] / rhs;
	result.arr[10] = lhs.arr[10] / rhs;
	result.arr[11] = lhs.arr[11] / rhs;
	result.arr[12] = lhs.arr[12] / rhs;
	result.arr[13] = lhs.arr[13] / rhs;
	result.arr[14] = lhs.arr[14] / rhs;
	result.arr[15] = lhs.arr[15] / rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline mat4 mat4::
operator/ (const f32& rhs)const{DPZoneScoped;
	Assert(rhs != 0, "mat4 elements cant be divided by zero");
	mat4 result;
#if DESHI_MATH_USE_SSE
	__m128 scalar = m128_fill_4f32(rhs);
	result.sse_row0 = m128_div_4f32(this->sse_row0, scalar);
	result.sse_row1 = m128_div_4f32(this->sse_row1, scalar);
	result.sse_row2 = m128_div_4f32(this->sse_row2, scalar);
	result.sse_row3 = m128_div_4f32(this->sse_row3, scalar);
#else //#if DESHI_MATH_USE_SSE
	result.arr[0] = this->arr[0] / rhs;
	result.arr[1] = this->arr[1] / rhs;
	result.arr[2] = this->arr[2] / rhs;
	result.arr[3] = this->arr[3] / rhs;
	result.arr[4] = this->arr[4] / rhs;
	result.arr[5] = this->arr[5] / rhs;
	result.arr[6] = this->arr[6] / rhs;
	result.arr[7] = this->arr[7] / rhs;
	result.arr[8] = this->arr[8] / rhs;
	result.arr[9] = this->arr[9] / rhs;
	result.arr[10] = this->arr[10] / rhs;
	result.arr[11] = this->arr[11] / rhs;
	result.arr[12] = this->arr[12] / rhs;
	result.arr[13] = this->arr[13] / rhs;
	result.arr[14] = this->arr[14] / rhs;
	result.arr[15] = this->arr[15] / rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat4::
operator/=(const f32& rhs){DPZoneScoped;
	Assert(rhs != 0, "mat4 elements cant be divided by zero");
#if DESHI_MATH_USE_SSE
	__m128 scalar = m128_fill_4f32(rhs);
	this->sse_row0 = m128_div_4f32(this->sse_row0, scalar);
	this->sse_row1 = m128_div_4f32(this->sse_row1, scalar);
	this->sse_row2 = m128_div_4f32(this->sse_row2, scalar);
	this->sse_row3 = m128_div_4f32(this->sse_row3, scalar);
#else //#if DESHI_MATH_USE_SSE
	this->arr[0] /= rhs;
	this->arr[1] /= rhs;
	this->arr[2] /= rhs;
	this->arr[3] /= rhs;
	this->arr[4] /= rhs;
	this->arr[5] /= rhs;
	this->arr[6] /= rhs;
	this->arr[7] /= rhs;
	this->arr[8] /= rhs;
	this->arr[9] /= rhs;
	this->arr[10] /= rhs;
	this->arr[11] /= rhs;
	this->arr[12] /= rhs;
	this->arr[13] /= rhs;
	this->arr[14] /= rhs;
	this->arr[15] /= rhs;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat4
mat4_div_elements(mat4 lhs, mat4 rhs){DPZoneScoped;
	Assert(rhs.arr[ 0] != 0 && rhs.arr[ 1] != 0 && rhs.arr[ 2] != 0 && rhs.arr[ 3] != 0
		   rhs.arr[ 4] != 0 && rhs.arr[ 5] != 0 && rhs.arr[ 6] != 0 && rhs.arr[ 7] != 0
		   rhs.arr[ 8] != 0 && rhs.arr[ 9] != 0 && rhs.arr[10] != 0 && rhs.arr[11] != 0
		   rhs.arr[12] != 0 && rhs.arr[13] != 0 && rhs.arr[14] != 0 && rhs.arr[15] != 0,
		   "mat4 elements cant be divided by zero");
	mat4 result;
#if DESHI_MATH_USE_SSE
	result.sse_row0 = m128_div_4f32(lhs.sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_div_4f32(lhs.sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_div_4f32(lhs.sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_div_4f32(lhs.sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	result.arr[0] = lhs.arr[0] / rhs.arr[0];
	result.arr[1] = lhs.arr[1] / rhs.arr[1];
	result.arr[2] = lhs.arr[2] / rhs.arr[2];
	result.arr[3] = lhs.arr[3] / rhs.arr[3];
	result.arr[4] = lhs.arr[4] / rhs.arr[4];
	result.arr[5] = lhs.arr[5] / rhs.arr[5];
	result.arr[6] = lhs.arr[6] / rhs.arr[6];
	result.arr[7] = lhs.arr[7] / rhs.arr[7];
	result.arr[8] = lhs.arr[8] / rhs.arr[8];
	result.arr[9] = lhs.arr[9] / rhs.arr[9];
	result.arr[10] = lhs.arr[10] / rhs.arr[10];
	result.arr[11] = lhs.arr[11] / rhs.arr[11];
	result.arr[12] = lhs.arr[12] / rhs.arr[12];
	result.arr[13] = lhs.arr[13] / rhs.arr[13];
	result.arr[14] = lhs.arr[14] / rhs.arr[14];
	result.arr[15] = lhs.arr[15] / rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline mat4 mat4::
operator% (const mat4& rhs)const{DPZoneScoped;
	Assert(rhs.arr[ 0] != 0 && rhs.arr[ 1] != 0 && rhs.arr[ 2] != 0 && rhs.arr[ 3] != 0
		   rhs.arr[ 4] != 0 && rhs.arr[ 5] != 0 && rhs.arr[ 6] != 0 && rhs.arr[ 7] != 0
		   rhs.arr[ 8] != 0 && rhs.arr[ 9] != 0 && rhs.arr[10] != 0 && rhs.arr[11] != 0
		   rhs.arr[12] != 0 && rhs.arr[13] != 0 && rhs.arr[14] != 0 && rhs.arr[15] != 0,
		   "mat4 elements cant be divided by zero");
	mat4 result;
#if DESHI_MATH_USE_SSE
	result.sse_row0 = m128_div_4f32(this->sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_div_4f32(this->sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_div_4f32(this->sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_div_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	result.arr[0] = this->arr[0] / rhs.arr[0];
	result.arr[1] = this->arr[1] / rhs.arr[1];
	result.arr[2] = this->arr[2] / rhs.arr[2];
	result.arr[3] = this->arr[3] / rhs.arr[3];
	result.arr[4] = this->arr[4] / rhs.arr[4];
	result.arr[5] = this->arr[5] / rhs.arr[5];
	result.arr[6] = this->arr[6] / rhs.arr[6];
	result.arr[7] = this->arr[7] / rhs.arr[7];
	result.arr[8] = this->arr[8] / rhs.arr[8];
	result.arr[9] = this->arr[9] / rhs.arr[9];
	result.arr[10] = this->arr[10] / rhs.arr[10];
	result.arr[11] = this->arr[11] / rhs.arr[11];
	result.arr[12] = this->arr[12] / rhs.arr[12];
	result.arr[13] = this->arr[13] / rhs.arr[13];
	result.arr[14] = this->arr[14] / rhs.arr[14];
	result.arr[15] = this->arr[15] / rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat4::
operator%=(const mat4& rhs){DPZoneScoped;
	Assert(rhs.arr[ 0] != 0 && rhs.arr[ 1] != 0 && rhs.arr[ 2] != 0 && rhs.arr[ 3] != 0
		   rhs.arr[ 4] != 0 && rhs.arr[ 5] != 0 && rhs.arr[ 6] != 0 && rhs.arr[ 7] != 0
		   rhs.arr[ 8] != 0 && rhs.arr[ 9] != 0 && rhs.arr[10] != 0 && rhs.arr[11] != 0
		   rhs.arr[12] != 0 && rhs.arr[13] != 0 && rhs.arr[14] != 0 && rhs.arr[15] != 0,
		   "mat4 elements cant be divided by zero");
#if DESHI_MATH_USE_SSE
	this->sse_row0 = m128_div_4f32(this->sse_row0, rhs.sse_row0);
	this->sse_row1 = m128_div_4f32(this->sse_row1, rhs.sse_row1);
	this->sse_row2 = m128_div_4f32(this->sse_row2, rhs.sse_row2);
	this->sse_row3 = m128_div_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	this->arr[0] /= rhs.arr[0];
	this->arr[1] /= rhs.arr[1];
	this->arr[2] /= rhs.arr[2];
	this->arr[3] /= rhs.arr[3];
	this->arr[4] /= rhs.arr[4];
	this->arr[5] /= rhs.arr[5];
	this->arr[6] /= rhs.arr[6];
	this->arr[7] /= rhs.arr[7];
	this->arr[8] /= rhs.arr[8];
	this->arr[9] /= rhs.arr[9];
	this->arr[10] /= rhs.arr[10];
	this->arr[11] /= rhs.arr[11];
	this->arr[12] /= rhs.arr[12];
	this->arr[13] /= rhs.arr[13];
	this->arr[14] /= rhs.arr[14];
	this->arr[15] /= rhs.arr[15];
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
mat4_equal(mat4 lhs, mat4 rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	return m128_equal_4f32(lhs.sse_row0, rhs.sse_row0)
		&& m128_equal_4f32(lhs.sse_row1, rhs.sse_row1)
		&& m128_equal_4f32(lhs.sse_row2, rhs.sse_row2)
		&& m128_equal_4f32(lhs.sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	return DESHI_ABSF(lhs.arr[0] - rhs.arr[0]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[1] - rhs.arr[1]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[2] - rhs.arr[2]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[3] - rhs.arr[3]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[4] - rhs.arr[4]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[5] - rhs.arr[5]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[6] - rhs.arr[6]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[7] - rhs.arr[7]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[8] - rhs.arr[8]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[9] - rhs.arr[9]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[10] - rhs.arr[10]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[11] - rhs.arr[11]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[12] - rhs.arr[12]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[13] - rhs.arr[13]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[14] - rhs.arr[14]) > M_EPSILON
		&& DESHI_ABSF(lhs.arr[15] - rhs.arr[15]) > M_EPSILON;
#endif //#else //#if DESHI_MATH_USE_SSE
}

#ifdef __cplusplus
inline b32 mat4::
operator==(const mat4& rhs)const{DPZoneScoped;
#if DESHI_MATH_USE_SSE
	return m128_equal_4f32(this->sse_row0, rhs.sse_row0)
		&& m128_equal_4f32(this->sse_row1, rhs.sse_row1)
		&& m128_equal_4f32(this->sse_row2, rhs.sse_row2)
		&& m128_equal_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	return DESHI_ABSF(this->arr[0] - rhs.arr[0]) > M_EPSILON
		&& DESHI_ABSF(this->arr[1] - rhs.arr[1]) > M_EPSILON
		&& DESHI_ABSF(this->arr[2] - rhs.arr[2]) > M_EPSILON
		&& DESHI_ABSF(this->arr[3] - rhs.arr[3]) > M_EPSILON
		&& DESHI_ABSF(this->arr[4] - rhs.arr[4]) > M_EPSILON
		&& DESHI_ABSF(this->arr[5] - rhs.arr[5]) > M_EPSILON
		&& DESHI_ABSF(this->arr[6] - rhs.arr[6]) > M_EPSILON
		&& DESHI_ABSF(this->arr[7] - rhs.arr[7]) > M_EPSILON
		&& DESHI_ABSF(this->arr[8] - rhs.arr[8]) > M_EPSILON
		&& DESHI_ABSF(this->arr[9] - rhs.arr[9]) > M_EPSILON
		&& DESHI_ABSF(this->arr[10] - rhs.arr[10]) > M_EPSILON
		&& DESHI_ABSF(this->arr[11] - rhs.arr[11]) > M_EPSILON
		&& DESHI_ABSF(this->arr[12] - rhs.arr[12]) > M_EPSILON
		&& DESHI_ABSF(this->arr[13] - rhs.arr[13]) > M_EPSILON
		&& DESHI_ABSF(this->arr[14] - rhs.arr[14]) > M_EPSILON
		&& DESHI_ABSF(this->arr[15] - rhs.arr[15]) > M_EPSILON;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
mat4_nequal(mat4 lhs, mat4 rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	return !m128_equal_4f32(lhs.sse_row0, rhs.sse_row0)
		&& !m128_equal_4f32(lhs.sse_row1, rhs.sse_row1)
		&& !m128_equal_4f32(lhs.sse_row2, rhs.sse_row2)
		&& !m128_equal_4f32(lhs.sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	return DESHI_ABSF(lhs.arr[0] - rhs.arr[0]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[1] - rhs.arr[1]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[2] - rhs.arr[2]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[3] - rhs.arr[3]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[4] - rhs.arr[4]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[5] - rhs.arr[5]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[6] - rhs.arr[6]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[7] - rhs.arr[7]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[8] - rhs.arr[8]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[9] - rhs.arr[9]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[10] - rhs.arr[10]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[11] - rhs.arr[11]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[12] - rhs.arr[12]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[13] - rhs.arr[13]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[14] - rhs.arr[14]) < M_EPSILON
		|| DESHI_ABSF(lhs.arr[15] - rhs.arr[15]) < M_EPSILON:
#endif //#else //#if DESHI_MATH_USE_SSE
}

#ifdef __cplusplus
inline b32 mat4::
operator!=(const mat4& rhs)const{DPZoneScoped;
	#if DESHI_MATH_USE_SSE
	return !m128_equal_4f32(this->sse_row0, rhs.sse_row0)
		&& !m128_equal_4f32(this->sse_row1, rhs.sse_row1)
		&& !m128_equal_4f32(this->sse_row2, rhs.sse_row2)
		&& !m128_equal_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	return DESHI_ABSF(this->arr[0] - rhs.arr[0]) < M_EPSILON
		|| DESHI_ABSF(this->arr[1] - rhs.arr[1]) < M_EPSILON
		|| DESHI_ABSF(this->arr[2] - rhs.arr[2]) < M_EPSILON
		|| DESHI_ABSF(this->arr[3] - rhs.arr[3]) < M_EPSILON
		|| DESHI_ABSF(this->arr[4] - rhs.arr[4]) < M_EPSILON
		|| DESHI_ABSF(this->arr[5] - rhs.arr[5]) < M_EPSILON
		|| DESHI_ABSF(this->arr[6] - rhs.arr[6]) < M_EPSILON
		|| DESHI_ABSF(this->arr[7] - rhs.arr[7]) < M_EPSILON
		|| DESHI_ABSF(this->arr[8] - rhs.arr[8]) < M_EPSILON
		|| DESHI_ABSF(this->arr[9] - rhs.arr[9]) < M_EPSILON
		|| DESHI_ABSF(this->arr[10] - rhs.arr[10]) < M_EPSILON
		|| DESHI_ABSF(this->arr[11] - rhs.arr[11]) < M_EPSILON
		|| DESHI_ABSF(this->arr[12] - rhs.arr[12]) < M_EPSILON
		|| DESHI_ABSF(this->arr[13] - rhs.arr[13]) < M_EPSILON
		|| DESHI_ABSF(this->arr[14] - rhs.arr[14]) < M_EPSILON
		|| DESHI_ABSF(this->arr[15] - rhs.arr[15]) < M_EPSILON:
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline b32
mat4_transpose(mat4 lhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	_MM_TRANSPOSE4_PS(lhs.sse_row0, lhs.sse_row1, lhs.sse_row2, lhs.sse_row3);
	return lhs;
#else //#if DESHI_MATH_USE_SSE
	return Mat4(lhs.arr[0], lhs.arr[4], lhs.arr[ 8], lhs.arr[12],
				lhs.arr[1], lhs.arr[5], lhs.arr[ 9], lhs.arr[13],
				lhs.arr[2], lhs.arr[6], lhs.arr[10], lhs.arr[14],
				lhs.arr[3], lhs.arr[7], lhs.arr[11], lhs.arr[15]);
#endif //#else //#if DESHI_MATH_USE_SSE
}

#ifdef __cplusplus
inline mat4 mat4::
transpose()const{DPZoneScoped;
#if DESHI_MATH_USE_SSE
	mat4 result = *this;
	_MM_TRANSPOSE4_PS(result.sse_row0, result.sse_row1, result.sse_row2, result.sse_row3);
	return result;
#else //#if DESHI_MATH_USE_SSE
	return Mat4(this->arr[0], this->arr[4], this->arr[ 8], this->arr[12],
				this->arr[1], this->arr[5], this->arr[ 9], this->arr[13],
				this->arr[2], this->arr[6], this->arr[10], this->arr[14],
				this->arr[3], this->arr[7], this->arr[11], this->arr[15]);
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
mat4_determinant(mat4 lhs){DPZoneScoped;
	return
		lhs.arr[ 0] * (lhs.arr[ 5] * (lhs.arr[10] * lhs.arr[15] - lhs.arr[11] * lhs.arr[14]) -
					   lhs.arr[ 9] * (lhs.arr[ 6] * lhs.arr[15] - lhs.arr[ 7] * lhs.arr[14]) + 
					   lhs.arr[13] * (lhs.arr[ 6] * lhs.arr[11] - lhs.arr[ 7] * lhs.arr[10]))
		-
		lhs.arr[ 4] * (lhs.arr[ 1] * (lhs.arr[10] * lhs.arr[15] - lhs.arr[11] * lhs.arr[14]) -
					   lhs.arr[ 9] * (lhs.arr[ 2] * lhs.arr[15] - lhs.arr[ 3] * lhs.arr[14]) +
					   lhs.arr[13] * (lhs.arr[ 2] * lhs.arr[11] - lhs.arr[ 3] * lhs.arr[10]))
		+
		lhs.arr[ 8] * (lhs.arr[ 1] * (lhs.arr[ 6] * lhs.arr[15] - lhs.arr[ 7] * lhs.arr[14]) -
					   lhs.arr[ 5] * (lhs.arr[ 2] * lhs.arr[15] - lhs.arr[ 3] * lhs.arr[14]) +
					   lhs.arr[13] * (lhs.arr[ 2] * lhs.arr[ 7] - lhs.arr[ 3] * lhs.arr[ 6]))
		-
		lhs.arr[12] * (lhs.arr[ 1] * (lhs.arr[ 6] * lhs.arr[11] - lhs.arr[ 7] * lhs.arr[10]) -
					   lhs.arr[ 5] * (lhs.arr[ 2] * lhs.arr[11] - lhs.arr[ 3] * lhs.arr[10]) +
					   lhs.arr[ 9] * (lhs.arr[ 2] * lhs.arr[ 7] - lhs.arr[ 3] * lhs.arr[ 6]));
}

#ifdef __cplusplus
inline f32 mat4::
determinant()const{DPZoneScoped;
	return
		this->arr[ 0] * (this->arr[ 5] * (this->arr[10] * this->arr[15] - this->arr[11] * this->arr[14]) -
						 this->arr[ 9] * (this->arr[ 6] * this->arr[15] - this->arr[ 7] * this->arr[14]) + 
						 this->arr[13] * (this->arr[ 6] * this->arr[11] - this->arr[ 7] * this->arr[10]))
		-
		this->arr[ 4] * (this->arr[ 1] * (this->arr[10] * this->arr[15] - this->arr[11] * this->arr[14]) -
						 this->arr[ 9] * (this->arr[ 2] * this->arr[15] - this->arr[ 3] * this->arr[14]) +
						 this->arr[13] * (this->arr[ 2] * this->arr[11] - this->arr[ 3] * this->arr[10]))
		+
		this->arr[ 8] * (this->arr[ 1] * (this->arr[ 6] * this->arr[15] - this->arr[ 7] * this->arr[14]) -
						 this->arr[ 5] * (this->arr[ 2] * this->arr[15] - this->arr[ 3] * this->arr[14]) +
						 this->arr[13] * (this->arr[ 2] * this->arr[ 7] - this->arr[ 3] * this->arr[ 6]))
		-
		this->arr[12] * (this->arr[ 1] * (this->arr[ 6] * this->arr[11] - this->arr[ 7] * this->arr[10]) -
						 this->arr[ 5] * (this->arr[ 2] * this->arr[11] - this->arr[ 3] * this->arr[10]) +
						 this->arr[ 9] * (this->arr[ 2] * this->arr[ 7] - this->arr[ 3] * this->arr[ 6]));
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
mat4_minor(mat4 lhs, u32 row, u32 col){DPZoneScoped;
	//NOTE(delle) I wonder if all this is really better than a loop
	Assert(row < 4 && col < 4, "mat4 subscript out of bounds");
	switch(row){
		case 0:{
			switch(col){
				case 0: return (  (lhs.arr[ 5] * lhs.arr[10] * lhs.arr[15])
								+ (lhs.arr[ 6] * lhs.arr[11] * lhs.arr[13])
								+ (lhs.arr[ 7] * lhs.arr[ 9] * lhs.arr[14])
								- (lhs.arr[ 7] * lhs.arr[10] * lhs.arr[13])
								- (lhs.arr[ 6] * lhs.arr[ 9] * lhs.arr[15])
								- (lhs.arr[ 5] * lhs.arr[11] * lhs.arr[14]));
				case 1: return (  (lhs.arr[ 4] * lhs.arr[10] * lhs.arr[15])
								+ (lhs.arr[ 6] * lhs.arr[11] * lhs.arr[12])
								+ (lhs.arr[ 7] * lhs.arr[ 8] * lhs.arr[14])
								- (lhs.arr[ 7] * lhs.arr[10] * lhs.arr[12])
								- (lhs.arr[ 6] * lhs.arr[ 8] * lhs.arr[15])
								- (lhs.arr[ 4] * lhs.arr[11] * lhs.arr[14]));
				case 2: return (  (lhs.arr[ 4] * lhs.arr[ 9] * lhs.arr[15])
								+ (lhs.arr[ 5] * lhs.arr[11] * lhs.arr[12])
								+ (lhs.arr[ 7] * lhs.arr[ 8] * lhs.arr[13])
								- (lhs.arr[ 7] * lhs.arr[ 9] * lhs.arr[12])
								- (lhs.arr[ 5] * lhs.arr[ 8] * lhs.arr[15])
								- (lhs.arr[ 4] * lhs.arr[11] * lhs.arr[13]));
				case 3: return (  (lhs.arr[ 4] * lhs.arr[ 9] * lhs.arr[14])
								+ (lhs.arr[ 5] * lhs.arr[10] * lhs.arr[12])
								+ (lhs.arr[ 6] * lhs.arr[ 8] * lhs.arr[13])
								- (lhs.arr[ 6] * lhs.arr[ 9] * lhs.arr[12])
								- (lhs.arr[ 5] * lhs.arr[ 8] * lhs.arr[14])
								- (lhs.arr[ 4] * lhs.arr[10] * lhs.arr[13]));
			}
		}break;
		case 1:{
			switch(col){
				case 0: return (  (lhs.arr[ 1] * lhs.arr[10] * lhs.arr[15])
								+ (lhs.arr[ 2] * lhs.arr[11] * lhs.arr[13])
								+ (lhs.arr[ 3] * lhs.arr[ 9] * lhs.arr[14])
								- (lhs.arr[ 3] * lhs.arr[10] * lhs.arr[13])
								- (lhs.arr[ 2] * lhs.arr[ 9] * lhs.arr[15])
								- (lhs.arr[ 1] * lhs.arr[11] * lhs.arr[14]));
				case 1: return (  (lhs.arr[ 0] * lhs.arr[10] * lhs.arr[15])
								+ (lhs.arr[ 2] * lhs.arr[11] * lhs.arr[12])
								+ (lhs.arr[ 3] * lhs.arr[ 8] * lhs.arr[14])
								- (lhs.arr[ 3] * lhs.arr[10] * lhs.arr[12])
								- (lhs.arr[ 2] * lhs.arr[ 8] * lhs.arr[15])
								- (lhs.arr[ 0] * lhs.arr[11] * lhs.arr[14]));
				case 2: return (  (lhs.arr[ 0] * lhs.arr[ 9] * lhs.arr[15])
								+ (lhs.arr[ 1] * lhs.arr[11] * lhs.arr[12])
								+ (lhs.arr[ 3] * lhs.arr[ 8] * lhs.arr[13])
								- (lhs.arr[ 3] * lhs.arr[ 9] * lhs.arr[12])
								- (lhs.arr[ 1] * lhs.arr[ 8] * lhs.arr[15])
								- (lhs.arr[ 0] * lhs.arr[11] * lhs.arr[13]));
				case 3: return (  (lhs.arr[ 0] * lhs.arr[ 9] * lhs.arr[14])
								+ (lhs.arr[ 1] * lhs.arr[10] * lhs.arr[12])
								+ (lhs.arr[ 2] * lhs.arr[ 8] * lhs.arr[13])
								- (lhs.arr[ 2] * lhs.arr[ 9] * lhs.arr[12])
								- (lhs.arr[ 1] * lhs.arr[ 8] * lhs.arr[14])
								- (lhs.arr[ 0] * lhs.arr[10] * lhs.arr[13]));
			}
		}break;
		case 2:{
			switch(col){
				case 0: return (  (lhs.arr[ 1] * lhs.arr[ 6] * lhs.arr[15])
								+ (lhs.arr[ 2] * lhs.arr[ 7] * lhs.arr[13])
								+ (lhs.arr[ 3] * lhs.arr[ 5] * lhs.arr[14])
								- (lhs.arr[ 3] * lhs.arr[ 6] * lhs.arr[13])
								- (lhs.arr[ 2] * lhs.arr[ 5] * lhs.arr[15])
								- (lhs.arr[ 1] * lhs.arr[ 7] * lhs.arr[14]));
				case 1: return (  (lhs.arr[ 0] * lhs.arr[ 6] * lhs.arr[15])
								+ (lhs.arr[ 2] * lhs.arr[ 7] * lhs.arr[12])
								+ (lhs.arr[ 3] * lhs.arr[ 4] * lhs.arr[14])
								- (lhs.arr[ 3] * lhs.arr[ 6] * lhs.arr[12])
								- (lhs.arr[ 2] * lhs.arr[ 4] * lhs.arr[15])
								- (lhs.arr[ 0] * lhs.arr[ 7] * lhs.arr[14]));
				case 2: return (  (lhs.arr[ 0] * lhs.arr[ 5] * lhs.arr[15])
								+ (lhs.arr[ 1] * lhs.arr[ 7] * lhs.arr[12])
								+ (lhs.arr[ 3] * lhs.arr[ 4] * lhs.arr[13])
								- (lhs.arr[ 3] * lhs.arr[ 5] * lhs.arr[12])
								- (lhs.arr[ 1] * lhs.arr[ 4] * lhs.arr[15])
								- (lhs.arr[ 0] * lhs.arr[ 7] * lhs.arr[13]));
				case 3: return (  (lhs.arr[ 0] * lhs.arr[ 5] * lhs.arr[14])
								+ (lhs.arr[ 1] * lhs.arr[ 6] * lhs.arr[12])
								+ (lhs.arr[ 2] * lhs.arr[ 4] * lhs.arr[13])
								- (lhs.arr[ 2] * lhs.arr[ 5] * lhs.arr[12])
								- (lhs.arr[ 1] * lhs.arr[ 4] * lhs.arr[14])
								- (lhs.arr[ 0] * lhs.arr[ 6] * lhs.arr[13]));
			}
		}break;
		case 3:{
			switch(col){
				case 0: return (  (lhs.arr[ 1] * lhs.arr[ 6] * lhs.arr[11])
								+ (lhs.arr[ 2] * lhs.arr[ 7] * lhs.arr[ 9])
								+ (lhs.arr[ 3] * lhs.arr[ 5] * lhs.arr[10])
								- (lhs.arr[ 3] * lhs.arr[ 6] * lhs.arr[ 9])
								- (lhs.arr[ 2] * lhs.arr[ 5] * lhs.arr[11])
								- (lhs.arr[ 1] * lhs.arr[ 7] * lhs.arr[10]));
				case 1: return (  (lhs.arr[ 0] * lhs.arr[ 6] * lhs.arr[11])
								+ (lhs.arr[ 2] * lhs.arr[ 7] * lhs.arr[ 8])
								+ (lhs.arr[ 3] * lhs.arr[ 4] * lhs.arr[10])
								- (lhs.arr[ 3] * lhs.arr[ 6] * lhs.arr[ 8])
								- (lhs.arr[ 2] * lhs.arr[ 4] * lhs.arr[11])
								- (lhs.arr[ 0] * lhs.arr[ 7] * lhs.arr[10]));
				case 2: return (  (lhs.arr[ 0] * lhs.arr[ 5] * lhs.arr[11])
								+ (lhs.arr[ 1] * lhs.arr[ 7] * lhs.arr[ 8])
								+ (lhs.arr[ 3] * lhs.arr[ 4] * lhs.arr[ 9])
								- (lhs.arr[ 3] * lhs.arr[ 5] * lhs.arr[ 8])
								- (lhs.arr[ 1] * lhs.arr[ 4] * lhs.arr[11])
								- (lhs.arr[ 0] * lhs.arr[ 7] * lhs.arr[ 9]));
				case 3: return (  (lhs.arr[ 0] * lhs.arr[ 5] * lhs.arr[10])
								+ (lhs.arr[ 1] * lhs.arr[ 6] * lhs.arr[ 8])
								+ (lhs.arr[ 2] * lhs.arr[ 4] * lhs.arr[ 9])
								- (lhs.arr[ 2] * lhs.arr[ 5] * lhs.arr[ 8])
								- (lhs.arr[ 1] * lhs.arr[ 4] * lhs.arr[10])
								- (lhs.arr[ 0] * lhs.arr[ 6] * lhs.arr[ 9]));
			}
		}break;
	}
}

#ifdef __cplusplus
inline f32 mat4::
minor(u32 row, u32 col)const{DPZoneScoped;
	Assert(row < 4 && col < 4, "mat4 subscript out of bounds");
	switch(row){
		case 0:{
			switch(col){
				case 0: return (  (this->arr[ 5] * this->arr[10] * this->arr[15])
								+ (this->arr[ 6] * this->arr[11] * this->arr[13])
								+ (this->arr[ 7] * this->arr[ 9] * this->arr[14])
								- (this->arr[ 7] * this->arr[10] * this->arr[13])
								- (this->arr[ 6] * this->arr[ 9] * this->arr[15])
								- (this->arr[ 5] * this->arr[11] * this->arr[14]));
				case 1: return (  (this->arr[ 4] * this->arr[10] * this->arr[15])
								+ (this->arr[ 6] * this->arr[11] * this->arr[12])
								+ (this->arr[ 7] * this->arr[ 8] * this->arr[14])
								- (this->arr[ 7] * this->arr[10] * this->arr[12])
								- (this->arr[ 6] * this->arr[ 8] * this->arr[15])
								- (this->arr[ 4] * this->arr[11] * this->arr[14]));
				case 2: return (  (this->arr[ 4] * this->arr[ 9] * this->arr[15])
								+ (this->arr[ 5] * this->arr[11] * this->arr[12])
								+ (this->arr[ 7] * this->arr[ 8] * this->arr[13])
								- (this->arr[ 7] * this->arr[ 9] * this->arr[12])
								- (this->arr[ 5] * this->arr[ 8] * this->arr[15])
								- (this->arr[ 4] * this->arr[11] * this->arr[13]));
				case 3: return (  (this->arr[ 4] * this->arr[ 9] * this->arr[14])
								+ (this->arr[ 5] * this->arr[10] * this->arr[12])
								+ (this->arr[ 6] * this->arr[ 8] * this->arr[13])
								- (this->arr[ 6] * this->arr[ 9] * this->arr[12])
								- (this->arr[ 5] * this->arr[ 8] * this->arr[14])
								- (this->arr[ 4] * this->arr[10] * this->arr[13]));
			}
		}break;
		case 1:{
			switch(col){
				case 0: return (  (this->arr[ 1] * this->arr[10] * this->arr[15])
								+ (this->arr[ 2] * this->arr[11] * this->arr[13])
								+ (this->arr[ 3] * this->arr[ 9] * this->arr[14])
								- (this->arr[ 3] * this->arr[10] * this->arr[13])
								- (this->arr[ 2] * this->arr[ 9] * this->arr[15])
								- (this->arr[ 1] * this->arr[11] * this->arr[14]));
				case 1: return (  (this->arr[ 0] * this->arr[10] * this->arr[15])
								+ (this->arr[ 2] * this->arr[11] * this->arr[12])
								+ (this->arr[ 3] * this->arr[ 8] * this->arr[14])
								- (this->arr[ 3] * this->arr[10] * this->arr[12])
								- (this->arr[ 2] * this->arr[ 8] * this->arr[15])
								- (this->arr[ 0] * this->arr[11] * this->arr[14]));
				case 2: return (  (this->arr[ 0] * this->arr[ 9] * this->arr[15])
								+ (this->arr[ 1] * this->arr[11] * this->arr[12])
								+ (this->arr[ 3] * this->arr[ 8] * this->arr[13])
								- (this->arr[ 3] * this->arr[ 9] * this->arr[12])
								- (this->arr[ 1] * this->arr[ 8] * this->arr[15])
								- (this->arr[ 0] * this->arr[11] * this->arr[13]));
				case 3: return (  (this->arr[ 0] * this->arr[ 9] * this->arr[14])
								+ (this->arr[ 1] * this->arr[10] * this->arr[12])
								+ (this->arr[ 2] * this->arr[ 8] * this->arr[13])
								- (this->arr[ 2] * this->arr[ 9] * this->arr[12])
								- (this->arr[ 1] * this->arr[ 8] * this->arr[14])
								- (this->arr[ 0] * this->arr[10] * this->arr[13]));
			}
		}break;
		case 2:{
			switch(col){
				case 0: return (  (this->arr[ 1] * this->arr[ 6] * this->arr[15])
								+ (this->arr[ 2] * this->arr[ 7] * this->arr[13])
								+ (this->arr[ 3] * this->arr[ 5] * this->arr[14])
								- (this->arr[ 3] * this->arr[ 6] * this->arr[13])
								- (this->arr[ 2] * this->arr[ 5] * this->arr[15])
								- (this->arr[ 1] * this->arr[ 7] * this->arr[14]));
				case 1: return (  (this->arr[ 0] * this->arr[ 6] * this->arr[15])
								+ (this->arr[ 2] * this->arr[ 7] * this->arr[12])
								+ (this->arr[ 3] * this->arr[ 4] * this->arr[14])
								- (this->arr[ 3] * this->arr[ 6] * this->arr[12])
								- (this->arr[ 2] * this->arr[ 4] * this->arr[15])
								- (this->arr[ 0] * this->arr[ 7] * this->arr[14]));
				case 2: return (  (this->arr[ 0] * this->arr[ 5] * this->arr[15])
								+ (this->arr[ 1] * this->arr[ 7] * this->arr[12])
								+ (this->arr[ 3] * this->arr[ 4] * this->arr[13])
								- (this->arr[ 3] * this->arr[ 5] * this->arr[12])
								- (this->arr[ 1] * this->arr[ 4] * this->arr[15])
								- (this->arr[ 0] * this->arr[ 7] * this->arr[13]));
				case 3: return (  (this->arr[ 0] * this->arr[ 5] * this->arr[14])
								+ (this->arr[ 1] * this->arr[ 6] * this->arr[12])
								+ (this->arr[ 2] * this->arr[ 4] * this->arr[13])
								- (this->arr[ 2] * this->arr[ 5] * this->arr[12])
								- (this->arr[ 1] * this->arr[ 4] * this->arr[14])
								- (this->arr[ 0] * this->arr[ 6] * this->arr[13]));
			}
		}break;
		case 3:{
			switch(col){
				case 0: return (  (this->arr[ 1] * this->arr[ 6] * this->arr[11])
								+ (this->arr[ 2] * this->arr[ 7] * this->arr[ 9])
								+ (this->arr[ 3] * this->arr[ 5] * this->arr[10])
								- (this->arr[ 3] * this->arr[ 6] * this->arr[ 9])
								- (this->arr[ 2] * this->arr[ 5] * this->arr[11])
								- (this->arr[ 1] * this->arr[ 7] * this->arr[10]));
				case 1: return (  (this->arr[ 0] * this->arr[ 6] * this->arr[11])
								+ (this->arr[ 2] * this->arr[ 7] * this->arr[ 8])
								+ (this->arr[ 3] * this->arr[ 4] * this->arr[10])
								- (this->arr[ 3] * this->arr[ 6] * this->arr[ 8])
								- (this->arr[ 2] * this->arr[ 4] * this->arr[11])
								- (this->arr[ 0] * this->arr[ 7] * this->arr[10]));
				case 2: return (  (this->arr[ 0] * this->arr[ 5] * this->arr[11])
								+ (this->arr[ 1] * this->arr[ 7] * this->arr[ 8])
								+ (this->arr[ 3] * this->arr[ 4] * this->arr[ 9])
								- (this->arr[ 3] * this->arr[ 5] * this->arr[ 8])
								- (this->arr[ 1] * this->arr[ 4] * this->arr[11])
								- (this->arr[ 0] * this->arr[ 7] * this->arr[ 9]));
				case 3: return (  (this->arr[ 0] * this->arr[ 5] * this->arr[10])
								+ (this->arr[ 1] * this->arr[ 6] * this->arr[ 8])
								+ (this->arr[ 2] * this->arr[ 4] * this->arr[ 9])
								- (this->arr[ 2] * this->arr[ 5] * this->arr[ 8])
								- (this->arr[ 1] * this->arr[ 4] * this->arr[10])
								- (this->arr[ 0] * this->arr[ 6] * this->arr[ 9]));
			}
		}break;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline f32
mat4_cofactor(mat4 lhs, u32 row, u32 col){DPZoneScoped;
	//NOTE(delle) Maybe it makes some sense here since it's dependent on the column/row pairing
	Assert(row < 4 && col < 4, "mat4 subscript out of bounds");
	switch(row){
		case 0:{
			switch(col){
				case 0: return (  (lhs.arr[ 7] * lhs.arr[10] * lhs.arr[13])
								+ (lhs.arr[ 6] * lhs.arr[ 9] * lhs.arr[15])
								+ (lhs.arr[ 5] * lhs.arr[11] * lhs.arr[14])
								- (lhs.arr[ 5] * lhs.arr[10] * lhs.arr[15])
								- (lhs.arr[ 6] * lhs.arr[11] * lhs.arr[13])
								- (lhs.arr[ 7] * lhs.arr[ 9] * lhs.arr[14]));
				case 1: return (  (lhs.arr[ 4] * lhs.arr[10] * lhs.arr[15])
								+ (lhs.arr[ 6] * lhs.arr[11] * lhs.arr[12])
								+ (lhs.arr[ 7] * lhs.arr[ 8] * lhs.arr[14])
								- (lhs.arr[ 7] * lhs.arr[10] * lhs.arr[12])
								- (lhs.arr[ 6] * lhs.arr[ 8] * lhs.arr[15])
								- (lhs.arr[ 4] * lhs.arr[11] * lhs.arr[14]));
				case 2: return (  (lhs.arr[ 7] * lhs.arr[ 9] * lhs.arr[12])
								+ (lhs.arr[ 5] * lhs.arr[ 8] * lhs.arr[15])
								+ (lhs.arr[ 4] * lhs.arr[11] * lhs.arr[13])
								- (lhs.arr[ 4] * lhs.arr[ 9] * lhs.arr[15])
								- (lhs.arr[ 5] * lhs.arr[11] * lhs.arr[12])
								- (lhs.arr[ 7] * lhs.arr[ 8] * lhs.arr[13]));
				case 3: return (  (lhs.arr[ 4] * lhs.arr[ 9] * lhs.arr[14])
								+ (lhs.arr[ 5] * lhs.arr[10] * lhs.arr[12])
								+ (lhs.arr[ 6] * lhs.arr[ 8] * lhs.arr[13])
								- (lhs.arr[ 6] * lhs.arr[ 9] * lhs.arr[12])
								- (lhs.arr[ 5] * lhs.arr[ 8] * lhs.arr[14])
								- (lhs.arr[ 4] * lhs.arr[10] * lhs.arr[13]));
			}
		}break;
		case 1:{
			switch(col){
				case 0: return (  (lhs.arr[ 1] * lhs.arr[10] * lhs.arr[15])
								+ (lhs.arr[ 2] * lhs.arr[11] * lhs.arr[13])
								+ (lhs.arr[ 3] * lhs.arr[ 9] * lhs.arr[14])
								- (lhs.arr[ 3] * lhs.arr[10] * lhs.arr[13])
								- (lhs.arr[ 2] * lhs.arr[ 9] * lhs.arr[15])
								- (lhs.arr[ 1] * lhs.arr[11] * lhs.arr[14]));
				case 1: return (  (lhs.arr[ 3] * lhs.arr[10] * lhs.arr[12])
								+ (lhs.arr[ 2] * lhs.arr[ 8] * lhs.arr[15])
								+ (lhs.arr[ 0] * lhs.arr[11] * lhs.arr[14])
								- (lhs.arr[ 0] * lhs.arr[10] * lhs.arr[15])
								- (lhs.arr[ 2] * lhs.arr[11] * lhs.arr[12])
								- (lhs.arr[ 3] * lhs.arr[ 8] * lhs.arr[14]));
				case 2: return (  (lhs.arr[ 0] * lhs.arr[ 9] * lhs.arr[15])
								+ (lhs.arr[ 1] * lhs.arr[11] * lhs.arr[12])
								+ (lhs.arr[ 3] * lhs.arr[ 8] * lhs.arr[13])
								- (lhs.arr[ 3] * lhs.arr[ 9] * lhs.arr[12])
								- (lhs.arr[ 1] * lhs.arr[ 8] * lhs.arr[15])
								- (lhs.arr[ 0] * lhs.arr[11] * lhs.arr[13]));
				case 3: return (  (lhs.arr[ 2] * lhs.arr[ 9] * lhs.arr[12])
								+ (lhs.arr[ 1] * lhs.arr[ 8] * lhs.arr[14])
								+ (lhs.arr[ 0] * lhs.arr[10] * lhs.arr[13])
								- (lhs.arr[ 0] * lhs.arr[ 9] * lhs.arr[14])
								- (lhs.arr[ 1] * lhs.arr[10] * lhs.arr[12])
								- (lhs.arr[ 2] * lhs.arr[ 8] * lhs.arr[13]));
			}
		}break;
		case 2:{
			switch(col){
				case 0: return (  (lhs.arr[ 3] * lhs.arr[ 6] * lhs.arr[13])
								+ (lhs.arr[ 2] * lhs.arr[ 5] * lhs.arr[15])
								+ (lhs.arr[ 1] * lhs.arr[ 7] * lhs.arr[14])
								- (lhs.arr[ 1] * lhs.arr[ 6] * lhs.arr[15])
								- (lhs.arr[ 2] * lhs.arr[ 7] * lhs.arr[13])
								- (lhs.arr[ 3] * lhs.arr[ 5] * lhs.arr[14]));
				case 1: return (  (lhs.arr[ 0] * lhs.arr[ 6] * lhs.arr[15])
								+ (lhs.arr[ 2] * lhs.arr[ 7] * lhs.arr[12])
								+ (lhs.arr[ 3] * lhs.arr[ 4] * lhs.arr[14])
								- (lhs.arr[ 3] * lhs.arr[ 6] * lhs.arr[12])
								- (lhs.arr[ 2] * lhs.arr[ 4] * lhs.arr[15])
								- (lhs.arr[ 0] * lhs.arr[ 7] * lhs.arr[14]));
				case 2: return (  (lhs.arr[ 3] * lhs.arr[ 5] * lhs.arr[12])
								+ (lhs.arr[ 1] * lhs.arr[ 4] * lhs.arr[15])
								+ (lhs.arr[ 0] * lhs.arr[ 7] * lhs.arr[13])
								- (lhs.arr[ 0] * lhs.arr[ 5] * lhs.arr[15])
								- (lhs.arr[ 1] * lhs.arr[ 7] * lhs.arr[12])
								- (lhs.arr[ 3] * lhs.arr[ 4] * lhs.arr[13]));
				case 3: return (  (lhs.arr[ 0] * lhs.arr[ 5] * lhs.arr[14])
								+ (lhs.arr[ 1] * lhs.arr[ 6] * lhs.arr[12])
								+ (lhs.arr[ 2] * lhs.arr[ 4] * lhs.arr[13])
								- (lhs.arr[ 2] * lhs.arr[ 5] * lhs.arr[12])
								- (lhs.arr[ 1] * lhs.arr[ 4] * lhs.arr[14])
								- (lhs.arr[ 0] * lhs.arr[ 6] * lhs.arr[13]));
			}
		}break;
		case 3:{
			switch(col){
				case 0: return (  (lhs.arr[ 1] * lhs.arr[ 6] * lhs.arr[11])
								+ (lhs.arr[ 2] * lhs.arr[ 7] * lhs.arr[ 9])
								+ (lhs.arr[ 3] * lhs.arr[ 5] * lhs.arr[10])
								- (lhs.arr[ 3] * lhs.arr[ 6] * lhs.arr[ 9])
								- (lhs.arr[ 2] * lhs.arr[ 5] * lhs.arr[11])
								- (lhs.arr[ 1] * lhs.arr[ 7] * lhs.arr[10]));
				case 1: return (  (lhs.arr[ 3] * lhs.arr[ 6] * lhs.arr[ 8])
								+ (lhs.arr[ 2] * lhs.arr[ 4] * lhs.arr[11])
								+ (lhs.arr[ 0] * lhs.arr[ 7] * lhs.arr[10])
								- (lhs.arr[ 0] * lhs.arr[ 6] * lhs.arr[11])
								- (lhs.arr[ 2] * lhs.arr[ 7] * lhs.arr[ 8])
								- (lhs.arr[ 3] * lhs.arr[ 4] * lhs.arr[10]));
				case 2: return (  (lhs.arr[ 0] * lhs.arr[ 5] * lhs.arr[11])
								+ (lhs.arr[ 1] * lhs.arr[ 7] * lhs.arr[ 8])
								+ (lhs.arr[ 3] * lhs.arr[ 4] * lhs.arr[ 9])
								- (lhs.arr[ 3] * lhs.arr[ 5] * lhs.arr[ 8])
								- (lhs.arr[ 1] * lhs.arr[ 4] * lhs.arr[11])
								- (lhs.arr[ 0] * lhs.arr[ 7] * lhs.arr[ 9]));
				case 3: return (  (lhs.arr[ 2] * lhs.arr[ 5] * lhs.arr[ 8])
								+ (lhs.arr[ 1] * lhs.arr[ 4] * lhs.arr[10])
								+ (lhs.arr[ 0] * lhs.arr[ 6] * lhs.arr[ 9])
								- (lhs.arr[ 0] * lhs.arr[ 5] * lhs.arr[10])
								- (lhs.arr[ 1] * lhs.arr[ 6] * lhs.arr[ 8])
								- (lhs.arr[ 2] * lhs.arr[ 4] * lhs.arr[ 9]));
			}
		}break;
	}
}

#ifdef __cplusplus
inline f32 mat4::
cofactor(u32 row, u32 col)const{DPZoneScoped;
	Assert(row < 4 && col < 4, "mat4 subscript out of bounds");
	switch(row){
		case 0:{
			switch(col){
				case 0: return (  (this->arr[ 7] * this->arr[10] * this->arr[13])
								+ (this->arr[ 6] * this->arr[ 9] * this->arr[15])
								+ (this->arr[ 5] * this->arr[11] * this->arr[14])
								- (this->arr[ 5] * this->arr[10] * this->arr[15])
								- (this->arr[ 6] * this->arr[11] * this->arr[13])
								- (this->arr[ 7] * this->arr[ 9] * this->arr[14]));
				case 1: return (  (this->arr[ 4] * this->arr[10] * this->arr[15])
								+ (this->arr[ 6] * this->arr[11] * this->arr[12])
								+ (this->arr[ 7] * this->arr[ 8] * this->arr[14])
								- (this->arr[ 7] * this->arr[10] * this->arr[12])
								- (this->arr[ 6] * this->arr[ 8] * this->arr[15])
								- (this->arr[ 4] * this->arr[11] * this->arr[14]));
				case 2: return (  (this->arr[ 7] * this->arr[ 9] * this->arr[12])
								+ (this->arr[ 5] * this->arr[ 8] * this->arr[15])
								+ (this->arr[ 4] * this->arr[11] * this->arr[13])
								- (this->arr[ 4] * this->arr[ 9] * this->arr[15])
								- (this->arr[ 5] * this->arr[11] * this->arr[12])
								- (this->arr[ 7] * this->arr[ 8] * this->arr[13]));
				case 3: return (  (this->arr[ 4] * this->arr[ 9] * this->arr[14])
								+ (this->arr[ 5] * this->arr[10] * this->arr[12])
								+ (this->arr[ 6] * this->arr[ 8] * this->arr[13])
								- (this->arr[ 6] * this->arr[ 9] * this->arr[12])
								- (this->arr[ 5] * this->arr[ 8] * this->arr[14])
								- (this->arr[ 4] * this->arr[10] * this->arr[13]));
			}
		}break;
		case 1:{
			switch(col){
				case 0: return (  (this->arr[ 1] * this->arr[10] * this->arr[15])
								+ (this->arr[ 2] * this->arr[11] * this->arr[13])
								+ (this->arr[ 3] * this->arr[ 9] * this->arr[14])
								- (this->arr[ 3] * this->arr[10] * this->arr[13])
								- (this->arr[ 2] * this->arr[ 9] * this->arr[15])
								- (this->arr[ 1] * this->arr[11] * this->arr[14]));
				case 1: return (  (this->arr[ 3] * this->arr[10] * this->arr[12])
								+ (this->arr[ 2] * this->arr[ 8] * this->arr[15])
								+ (this->arr[ 0] * this->arr[11] * this->arr[14])
								- (this->arr[ 0] * this->arr[10] * this->arr[15])
								- (this->arr[ 2] * this->arr[11] * this->arr[12])
								- (this->arr[ 3] * this->arr[ 8] * this->arr[14]));
				case 2: return (  (this->arr[ 0] * this->arr[ 9] * this->arr[15])
								+ (this->arr[ 1] * this->arr[11] * this->arr[12])
								+ (this->arr[ 3] * this->arr[ 8] * this->arr[13])
								- (this->arr[ 3] * this->arr[ 9] * this->arr[12])
								- (this->arr[ 1] * this->arr[ 8] * this->arr[15])
								- (this->arr[ 0] * this->arr[11] * this->arr[13]));
				case 3: return (  (this->arr[ 2] * this->arr[ 9] * this->arr[12])
								+ (this->arr[ 1] * this->arr[ 8] * this->arr[14])
								+ (this->arr[ 0] * this->arr[10] * this->arr[13])
								- (this->arr[ 0] * this->arr[ 9] * this->arr[14])
								- (this->arr[ 1] * this->arr[10] * this->arr[12])
								- (this->arr[ 2] * this->arr[ 8] * this->arr[13]));
			}
		}break;
		case 2:{
			switch(col){
				case 0: return (  (this->arr[ 3] * this->arr[ 6] * this->arr[13])
								+ (this->arr[ 2] * this->arr[ 5] * this->arr[15])
								+ (this->arr[ 1] * this->arr[ 7] * this->arr[14])
								- (this->arr[ 1] * this->arr[ 6] * this->arr[15])
								- (this->arr[ 2] * this->arr[ 7] * this->arr[13])
								- (this->arr[ 3] * this->arr[ 5] * this->arr[14]));
				case 1: return (  (this->arr[ 0] * this->arr[ 6] * this->arr[15])
								+ (this->arr[ 2] * this->arr[ 7] * this->arr[12])
								+ (this->arr[ 3] * this->arr[ 4] * this->arr[14])
								- (this->arr[ 3] * this->arr[ 6] * this->arr[12])
								- (this->arr[ 2] * this->arr[ 4] * this->arr[15])
								- (this->arr[ 0] * this->arr[ 7] * this->arr[14]));
				case 2: return (  (this->arr[ 3] * this->arr[ 5] * this->arr[12])
								+ (this->arr[ 1] * this->arr[ 4] * this->arr[15])
								+ (this->arr[ 0] * this->arr[ 7] * this->arr[13])
								- (this->arr[ 0] * this->arr[ 5] * this->arr[15])
								- (this->arr[ 1] * this->arr[ 7] * this->arr[12])
								- (this->arr[ 3] * this->arr[ 4] * this->arr[13]));
				case 3: return (  (this->arr[ 0] * this->arr[ 5] * this->arr[14])
								+ (this->arr[ 1] * this->arr[ 6] * this->arr[12])
								+ (this->arr[ 2] * this->arr[ 4] * this->arr[13])
								- (this->arr[ 2] * this->arr[ 5] * this->arr[12])
								- (this->arr[ 1] * this->arr[ 4] * this->arr[14])
								- (this->arr[ 0] * this->arr[ 6] * this->arr[13]));
			}
		}break;
		case 3:{
			switch(col){
				case 0: return (  (this->arr[ 1] * this->arr[ 6] * this->arr[11])
								+ (this->arr[ 2] * this->arr[ 7] * this->arr[ 9])
								+ (this->arr[ 3] * this->arr[ 5] * this->arr[10])
								- (this->arr[ 3] * this->arr[ 6] * this->arr[ 9])
								- (this->arr[ 2] * this->arr[ 5] * this->arr[11])
								- (this->arr[ 1] * this->arr[ 7] * this->arr[10]));
				case 1: return (  (this->arr[ 3] * this->arr[ 6] * this->arr[ 8])
								+ (this->arr[ 2] * this->arr[ 4] * this->arr[11])
								+ (this->arr[ 0] * this->arr[ 7] * this->arr[10])
								- (this->arr[ 0] * this->arr[ 6] * this->arr[11])
								- (this->arr[ 2] * this->arr[ 7] * this->arr[ 8])
								- (this->arr[ 3] * this->arr[ 4] * this->arr[10]));
				case 2: return (  (this->arr[ 0] * this->arr[ 5] * this->arr[11])
								+ (this->arr[ 1] * this->arr[ 7] * this->arr[ 8])
								+ (this->arr[ 3] * this->arr[ 4] * this->arr[ 9])
								- (this->arr[ 3] * this->arr[ 5] * this->arr[ 8])
								- (this->arr[ 1] * this->arr[ 4] * this->arr[11])
								- (this->arr[ 0] * this->arr[ 7] * this->arr[ 9]));
				case 3: return (  (this->arr[ 2] * this->arr[ 5] * this->arr[ 8])
								+ (this->arr[ 1] * this->arr[ 4] * this->arr[10])
								+ (this->arr[ 0] * this->arr[ 6] * this->arr[ 9])
								- (this->arr[ 0] * this->arr[ 5] * this->arr[10])
								- (this->arr[ 1] * this->arr[ 6] * this->arr[ 8])
								- (this->arr[ 2] * this->arr[ 4] * this->arr[ 9]));
			}
		}break;
	}
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat4
mat4_adjoint(mat4 lhs){DPZoneScoped;
	//NOTE(delle) I guess in this case it really shows how expensive this operation is
	mat4 result;
	result.arr[ 0] = (  (lhs.arr[ 7] * lhs.arr[10] * lhs.arr[13])
					  + (lhs.arr[ 6] * lhs.arr[ 9] * lhs.arr[15])
					  + (lhs.arr[ 5] * lhs.arr[11] * lhs.arr[14])
					  - (lhs.arr[ 5] * lhs.arr[10] * lhs.arr[15])
					  - (lhs.arr[ 6] * lhs.arr[11] * lhs.arr[13])
					  - (lhs.arr[ 7] * lhs.arr[ 9] * lhs.arr[14]));
	result.arr[ 1] = (  (lhs.arr[ 1] * lhs.arr[10] * lhs.arr[15])
					  + (lhs.arr[ 2] * lhs.arr[11] * lhs.arr[13])
					  + (lhs.arr[ 3] * lhs.arr[ 9] * lhs.arr[14])
					  - (lhs.arr[ 3] * lhs.arr[10] * lhs.arr[13])
					  - (lhs.arr[ 2] * lhs.arr[ 9] * lhs.arr[15])
					  - (lhs.arr[ 1] * lhs.arr[11] * lhs.arr[14]));
	result.arr[ 2] = (  (lhs.arr[ 3] * lhs.arr[ 6] * lhs.arr[13])
					  + (lhs.arr[ 2] * lhs.arr[ 5] * lhs.arr[15])
					  + (lhs.arr[ 1] * lhs.arr[ 7] * lhs.arr[14])
					  - (lhs.arr[ 1] * lhs.arr[ 6] * lhs.arr[15])
					  - (lhs.arr[ 2] * lhs.arr[ 7] * lhs.arr[13])
					  - (lhs.arr[ 3] * lhs.arr[ 5] * lhs.arr[14]));
	result.arr[ 3] = (  (lhs.arr[ 1] * lhs.arr[ 6] * lhs.arr[11])
					  + (lhs.arr[ 2] * lhs.arr[ 7] * lhs.arr[ 9])
					  + (lhs.arr[ 3] * lhs.arr[ 5] * lhs.arr[10])
					  - (lhs.arr[ 3] * lhs.arr[ 6] * lhs.arr[ 9])
					  - (lhs.arr[ 2] * lhs.arr[ 5] * lhs.arr[11])
					  - (lhs.arr[ 1] * lhs.arr[ 7] * lhs.arr[10]));
	result.arr[ 4] = (  (lhs.arr[ 4] * lhs.arr[10] * lhs.arr[15])
					  + (lhs.arr[ 6] * lhs.arr[11] * lhs.arr[12])
					  + (lhs.arr[ 7] * lhs.arr[ 8] * lhs.arr[14])
					  - (lhs.arr[ 7] * lhs.arr[10] * lhs.arr[12])
					  - (lhs.arr[ 6] * lhs.arr[ 8] * lhs.arr[15])
					  - (lhs.arr[ 4] * lhs.arr[11] * lhs.arr[14]));
	result.arr[ 5] = (  (lhs.arr[ 3] * lhs.arr[10] * lhs.arr[12])
					  + (lhs.arr[ 2] * lhs.arr[ 8] * lhs.arr[15])
					  + (lhs.arr[ 0] * lhs.arr[11] * lhs.arr[14])
					  - (lhs.arr[ 0] * lhs.arr[10] * lhs.arr[15])
					  - (lhs.arr[ 2] * lhs.arr[11] * lhs.arr[12])
					  - (lhs.arr[ 3] * lhs.arr[ 8] * lhs.arr[14]));
	result.arr[ 6] = (  (lhs.arr[ 0] * lhs.arr[ 6] * lhs.arr[15])
					  + (lhs.arr[ 2] * lhs.arr[ 7] * lhs.arr[12])
					  + (lhs.arr[ 3] * lhs.arr[ 4] * lhs.arr[14])
					  - (lhs.arr[ 3] * lhs.arr[ 6] * lhs.arr[12])
					  - (lhs.arr[ 2] * lhs.arr[ 4] * lhs.arr[15])
					  - (lhs.arr[ 0] * lhs.arr[ 7] * lhs.arr[14]));
	result.arr[ 7] = (  (lhs.arr[ 3] * lhs.arr[ 6] * lhs.arr[ 8])
					  + (lhs.arr[ 2] * lhs.arr[ 4] * lhs.arr[11])
					  + (lhs.arr[ 0] * lhs.arr[ 7] * lhs.arr[10])
					  - (lhs.arr[ 0] * lhs.arr[ 6] * lhs.arr[11])
					  - (lhs.arr[ 2] * lhs.arr[ 7] * lhs.arr[ 8])
					  - (lhs.arr[ 3] * lhs.arr[ 4] * lhs.arr[10]));
	result.arr[ 8] = (  (lhs.arr[ 7] * lhs.arr[ 9] * lhs.arr[12])
					  + (lhs.arr[ 5] * lhs.arr[ 8] * lhs.arr[15])
					  + (lhs.arr[ 4] * lhs.arr[11] * lhs.arr[13])
					  - (lhs.arr[ 4] * lhs.arr[ 9] * lhs.arr[15])
					  - (lhs.arr[ 5] * lhs.arr[11] * lhs.arr[12])
					  - (lhs.arr[ 7] * lhs.arr[ 8] * lhs.arr[13]));;
	result.arr[ 9] = (  (lhs.arr[ 0] * lhs.arr[ 9] * lhs.arr[15])
					  + (lhs.arr[ 1] * lhs.arr[11] * lhs.arr[12])
					  + (lhs.arr[ 3] * lhs.arr[ 8] * lhs.arr[13])
					  - (lhs.arr[ 3] * lhs.arr[ 9] * lhs.arr[12])
					  - (lhs.arr[ 1] * lhs.arr[ 8] * lhs.arr[15])
					  - (lhs.arr[ 0] * lhs.arr[11] * lhs.arr[13]));
	result.arr[10] = (  (lhs.arr[ 3] * lhs.arr[ 5] * lhs.arr[12])
					  + (lhs.arr[ 1] * lhs.arr[ 4] * lhs.arr[15])
					  + (lhs.arr[ 0] * lhs.arr[ 7] * lhs.arr[13])
					  - (lhs.arr[ 0] * lhs.arr[ 5] * lhs.arr[15])
					  - (lhs.arr[ 1] * lhs.arr[ 7] * lhs.arr[12])
					  - (lhs.arr[ 3] * lhs.arr[ 4] * lhs.arr[13]));
	result.arr[11] = (  (lhs.arr[ 0] * lhs.arr[ 5] * lhs.arr[11])
					  + (lhs.arr[ 1] * lhs.arr[ 7] * lhs.arr[ 8])
					  + (lhs.arr[ 3] * lhs.arr[ 4] * lhs.arr[ 9])
					  - (lhs.arr[ 3] * lhs.arr[ 5] * lhs.arr[ 8])
					  - (lhs.arr[ 1] * lhs.arr[ 4] * lhs.arr[11])
					  - (lhs.arr[ 0] * lhs.arr[ 7] * lhs.arr[ 9]));
	result.arr[12] = (  (lhs.arr[ 4] * lhs.arr[ 9] * lhs.arr[14])
					  + (lhs.arr[ 5] * lhs.arr[10] * lhs.arr[12])
					  + (lhs.arr[ 6] * lhs.arr[ 8] * lhs.arr[13])
					  - (lhs.arr[ 6] * lhs.arr[ 9] * lhs.arr[12])
					  - (lhs.arr[ 5] * lhs.arr[ 8] * lhs.arr[14])
					  - (lhs.arr[ 4] * lhs.arr[10] * lhs.arr[13]));
	result.arr[13] = (  (lhs.arr[ 2] * lhs.arr[ 9] * lhs.arr[12])
					  + (lhs.arr[ 1] * lhs.arr[ 8] * lhs.arr[14])
					  + (lhs.arr[ 0] * lhs.arr[10] * lhs.arr[13])
					  - (lhs.arr[ 0] * lhs.arr[ 9] * lhs.arr[14])
					  - (lhs.arr[ 1] * lhs.arr[10] * lhs.arr[12])
					  - (lhs.arr[ 2] * lhs.arr[ 8] * lhs.arr[13]));
	result.arr[14] = (  (lhs.arr[ 0] * lhs.arr[ 5] * lhs.arr[14])
					  + (lhs.arr[ 1] * lhs.arr[ 6] * lhs.arr[12])
					  + (lhs.arr[ 2] * lhs.arr[ 4] * lhs.arr[13])
					  - (lhs.arr[ 2] * lhs.arr[ 5] * lhs.arr[12])
					  - (lhs.arr[ 1] * lhs.arr[ 4] * lhs.arr[14])
					  - (lhs.arr[ 0] * lhs.arr[ 6] * lhs.arr[13]));
	result.arr[15] = (  (lhs.arr[ 2] * lhs.arr[ 5] * lhs.arr[ 8])
					  + (lhs.arr[ 1] * lhs.arr[ 4] * lhs.arr[10])
					  + (lhs.arr[ 0] * lhs.arr[ 6] * lhs.arr[ 9])
					  - (lhs.arr[ 0] * lhs.arr[ 5] * lhs.arr[10])
					  - (lhs.arr[ 1] * lhs.arr[ 6] * lhs.arr[ 8])
					  - (lhs.arr[ 2] * lhs.arr[ 4] * lhs.arr[ 9]));
	return result;
}

#ifdef __cplusplus
inline mat4 mat4::
adjoint()const{DPZoneScoped;
	mat4 result;
	result.arr[ 0] = (  (this->arr[ 7] * this->arr[10] * this->arr[13])
					  + (this->arr[ 6] * this->arr[ 9] * this->arr[15])
					  + (this->arr[ 5] * this->arr[11] * this->arr[14])
					  - (this->arr[ 5] * this->arr[10] * this->arr[15])
					  - (this->arr[ 6] * this->arr[11] * this->arr[13])
					  - (this->arr[ 7] * this->arr[ 9] * this->arr[14]));
	result.arr[ 1] = (  (this->arr[ 1] * this->arr[10] * this->arr[15])
					  + (this->arr[ 2] * this->arr[11] * this->arr[13])
					  + (this->arr[ 3] * this->arr[ 9] * this->arr[14])
					  - (this->arr[ 3] * this->arr[10] * this->arr[13])
					  - (this->arr[ 2] * this->arr[ 9] * this->arr[15])
					  - (this->arr[ 1] * this->arr[11] * this->arr[14]));
	result.arr[ 2] = (  (this->arr[ 3] * this->arr[ 6] * this->arr[13])
					  + (this->arr[ 2] * this->arr[ 5] * this->arr[15])
					  + (this->arr[ 1] * this->arr[ 7] * this->arr[14])
					  - (this->arr[ 1] * this->arr[ 6] * this->arr[15])
					  - (this->arr[ 2] * this->arr[ 7] * this->arr[13])
					  - (this->arr[ 3] * this->arr[ 5] * this->arr[14]));
	result.arr[ 3] = (  (this->arr[ 1] * this->arr[ 6] * this->arr[11])
					  + (this->arr[ 2] * this->arr[ 7] * this->arr[ 9])
					  + (this->arr[ 3] * this->arr[ 5] * this->arr[10])
					  - (this->arr[ 3] * this->arr[ 6] * this->arr[ 9])
					  - (this->arr[ 2] * this->arr[ 5] * this->arr[11])
					  - (this->arr[ 1] * this->arr[ 7] * this->arr[10]));
	result.arr[ 4] = (  (this->arr[ 4] * this->arr[10] * this->arr[15])
					  + (this->arr[ 6] * this->arr[11] * this->arr[12])
					  + (this->arr[ 7] * this->arr[ 8] * this->arr[14])
					  - (this->arr[ 7] * this->arr[10] * this->arr[12])
					  - (this->arr[ 6] * this->arr[ 8] * this->arr[15])
					  - (this->arr[ 4] * this->arr[11] * this->arr[14]));
	result.arr[ 5] = (  (this->arr[ 3] * this->arr[10] * this->arr[12])
					  + (this->arr[ 2] * this->arr[ 8] * this->arr[15])
					  + (this->arr[ 0] * this->arr[11] * this->arr[14])
					  - (this->arr[ 0] * this->arr[10] * this->arr[15])
					  - (this->arr[ 2] * this->arr[11] * this->arr[12])
					  - (this->arr[ 3] * this->arr[ 8] * this->arr[14]));
	result.arr[ 6] = (  (this->arr[ 0] * this->arr[ 6] * this->arr[15])
					  + (this->arr[ 2] * this->arr[ 7] * this->arr[12])
					  + (this->arr[ 3] * this->arr[ 4] * this->arr[14])
					  - (this->arr[ 3] * this->arr[ 6] * this->arr[12])
					  - (this->arr[ 2] * this->arr[ 4] * this->arr[15])
					  - (this->arr[ 0] * this->arr[ 7] * this->arr[14]));
	result.arr[ 7] = (  (this->arr[ 3] * this->arr[ 6] * this->arr[ 8])
					  + (this->arr[ 2] * this->arr[ 4] * this->arr[11])
					  + (this->arr[ 0] * this->arr[ 7] * this->arr[10])
					  - (this->arr[ 0] * this->arr[ 6] * this->arr[11])
					  - (this->arr[ 2] * this->arr[ 7] * this->arr[ 8])
					  - (this->arr[ 3] * this->arr[ 4] * this->arr[10]));
	result.arr[ 8] = (  (this->arr[ 7] * this->arr[ 9] * this->arr[12])
					  + (this->arr[ 5] * this->arr[ 8] * this->arr[15])
					  + (this->arr[ 4] * this->arr[11] * this->arr[13])
					  - (this->arr[ 4] * this->arr[ 9] * this->arr[15])
					  - (this->arr[ 5] * this->arr[11] * this->arr[12])
					  - (this->arr[ 7] * this->arr[ 8] * this->arr[13]));;
	result.arr[ 9] = (  (this->arr[ 0] * this->arr[ 9] * this->arr[15])
					  + (this->arr[ 1] * this->arr[11] * this->arr[12])
					  + (this->arr[ 3] * this->arr[ 8] * this->arr[13])
					  - (this->arr[ 3] * this->arr[ 9] * this->arr[12])
					  - (this->arr[ 1] * this->arr[ 8] * this->arr[15])
					  - (this->arr[ 0] * this->arr[11] * this->arr[13]));
	result.arr[10] = (  (this->arr[ 3] * this->arr[ 5] * this->arr[12])
					  + (this->arr[ 1] * this->arr[ 4] * this->arr[15])
					  + (this->arr[ 0] * this->arr[ 7] * this->arr[13])
					  - (this->arr[ 0] * this->arr[ 5] * this->arr[15])
					  - (this->arr[ 1] * this->arr[ 7] * this->arr[12])
					  - (this->arr[ 3] * this->arr[ 4] * this->arr[13]));
	result.arr[11] = (  (this->arr[ 0] * this->arr[ 5] * this->arr[11])
					  + (this->arr[ 1] * this->arr[ 7] * this->arr[ 8])
					  + (this->arr[ 3] * this->arr[ 4] * this->arr[ 9])
					  - (this->arr[ 3] * this->arr[ 5] * this->arr[ 8])
					  - (this->arr[ 1] * this->arr[ 4] * this->arr[11])
					  - (this->arr[ 0] * this->arr[ 7] * this->arr[ 9]));
	result.arr[12] = (  (this->arr[ 4] * this->arr[ 9] * this->arr[14])
					  + (this->arr[ 5] * this->arr[10] * this->arr[12])
					  + (this->arr[ 6] * this->arr[ 8] * this->arr[13])
					  - (this->arr[ 6] * this->arr[ 9] * this->arr[12])
					  - (this->arr[ 5] * this->arr[ 8] * this->arr[14])
					  - (this->arr[ 4] * this->arr[10] * this->arr[13]));
	result.arr[13] = (  (this->arr[ 2] * this->arr[ 9] * this->arr[12])
					  + (this->arr[ 1] * this->arr[ 8] * this->arr[14])
					  + (this->arr[ 0] * this->arr[10] * this->arr[13])
					  - (this->arr[ 0] * this->arr[ 9] * this->arr[14])
					  - (this->arr[ 1] * this->arr[10] * this->arr[12])
					  - (this->arr[ 2] * this->arr[ 8] * this->arr[13]));
	result.arr[14] = (  (this->arr[ 0] * this->arr[ 5] * this->arr[14])
					  + (this->arr[ 1] * this->arr[ 6] * this->arr[12])
					  + (this->arr[ 2] * this->arr[ 4] * this->arr[13])
					  - (this->arr[ 2] * this->arr[ 5] * this->arr[12])
					  - (this->arr[ 1] * this->arr[ 4] * this->arr[14])
					  - (this->arr[ 0] * this->arr[ 6] * this->arr[13]));
	result.arr[15] = (  (this->arr[ 2] * this->arr[ 5] * this->arr[ 8])
					  + (this->arr[ 1] * this->arr[ 4] * this->arr[10])
					  + (this->arr[ 0] * this->arr[ 6] * this->arr[ 9])
					  - (this->arr[ 0] * this->arr[ 5] * this->arr[10])
					  - (this->arr[ 1] * this->arr[ 6] * this->arr[ 8])
					  - (this->arr[ 2] * this->arr[ 4] * this->arr[ 9]));
	return result;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat4
mat4_inverse(mat4 lhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
#define mat2_mul_mat2(a,b) m128_add_4f32(m128_mul_4f32(a, m128_swizzle(b, 0,3,0,3)), m128_mul_4f32(m128_swizzle(a, 1,0,3,2), m128_swizzle(b, 2,1,2,1)))
#define mat2_adj_mul_mat2(a,b) m128_sub_4f32(m128_mul_4f32(m128_swizzle(a, 3,3,0,0), b), m128_mul_4f32(m128_swizzle(a, 1,1,2,2), m128_swizzle(b, 2,3,0,1)))
#define mat2_mul_adj_mat2(a,b) m128_sub_4f32(m128_mul_4f32(a, m128_swizzle(b, 3,0,3,0)), m128_mul_4f32(m128_swizzle(a, 1,0,3,2), m128_swizzle(b, 2,1,2,1)))
	//!ref: https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html
	
	//2x2 sub matrices (ordered internally tl->tr->bl->br)
	__m128 A = m128_shuffle_0101(lhs.sse_row0, lhs.sse_row1); //top left
	__m128 B = m128_shuffle_2323(lhs.sse_row0, lhs.sse_row1); //top right
	__m128 C = m128_shuffle_0101(lhs.sse_row2, lhs.sse_row3); //bot left
	__m128 D = m128_shuffle_2323(lhs.sse_row2, lhs.sse_row3); //bot right
	
	//calculate determinants (and broadcast across m128)
	//TODO test the shuffle determinant calculation
	__m128 detA = _mm_set1_ps(lhs.arr[ 0] * lhs.arr[ 5] - lhs.arr[ 1] * lhs.arr[ 4]);
	__m128 detB = _mm_set1_ps(lhs.arr[ 2] * lhs.arr[ 7] - lhs.arr[ 3] * lhs.arr[ 6]);
	__m128 detC = _mm_set1_ps(lhs.arr[ 8] * lhs.arr[13] - lhs.arr[ 9] * lhs.arr[12]);
	__m128 detD = _mm_set1_ps(lhs.arr[10] * lhs.arr[15] - lhs.arr[11] * lhs.arr[14]);
	
	//calculate adjugates and determinant //NOTE A# = adjugate A; |A| = determinant A; Atr = trace A
	__m128 D_C = mat2_adj_mul_mat2(D, C); //D#*C
	__m128 A_B = mat2_adj_mul_mat2(A, B); //A#*B
	__m128 X = _mm_sub_ps(_mm_mul_ps(detD, A), mat2_mul_mat2(B, D_C)); //X# = |D|*A - B*(D#*C)
	__m128 W = _mm_sub_ps(_mm_mul_ps(detA, D), mat2_mul_mat2(C, A_B)); //W# = |A|*D - C*(A#*B)
	__m128 detM = _mm_mul_ps(detA, detD); //|M| = |A|*|D| ... (to be continued)
	
	__m128 Y = _mm_sub_ps(_mm_mul_ps(detB, C), mat2_mul_adj_mat2(D, A_B)); //Y# = |B|*C - D*((A#*B)#)
	__m128 Z = _mm_sub_ps(_mm_mul_ps(detC, B), mat2_mul_adj_mat2(A, D_C)); //Z# = |C|*B - A*((D#*C)#)
	detM = _mm_add_ps(detM, _mm_mul_ps(detB, detC)); //|M| = |A|*|D| + |B|*|C| ... (to be continued)
	
	//calculate trace
	__m128 tr = _mm_mul_ps(A_B, m128_shuffle(D_C,D_C, 0,2,1,3)); //((A#*B)*(D#*C))tr
	tr = _mm_hadd_ps(tr, tr);
	tr = _mm_hadd_ps(tr, tr);
	detM = _mm_sub_ps(detM, tr); //|M| = |A|*|D| + |B|*|C| - ((A#*B)*(D#*C))tr
	
	//inverse M = 1/|M| * |X Y|
	//                    |Z W|
	__m128 rDetM = _mm_div_ps(_mm_setr_ps(1.0f, -1.0f, -1.0f, 1.0f), detM); //(1/|M|, -1/|M|, -1/|M|, 1/|M|)
	X = _mm_mul_ps(X, rDetM);
	Y = _mm_mul_ps(Y, rDetM);
	Z = _mm_mul_ps(Z, rDetM);
	W = _mm_mul_ps(W, rDetM);
	
	mat4 result;
	result.sse_row0 = m128_shuffle(X, Y, 3,1,3,1);
	result.sse_row1 = m128_shuffle(X, Y, 2,0,2,0);
	result.sse_row2 = m128_shuffle(Z, W, 3,1,3,1);
	result.sse_row3 = m128_shuffle(Z, W, 2,0,2,0);
	return result;
#undef mat2_mul_adj_mat2
#undef mat2_adj_mul_mat2
#undef mat2_mul_mat2
#else //#if DESHI_MATH_USE_SSE
	return mat4_div_f32(mat4_adjoint(lhs), mat4_determinant(lhs));
#endif //#else //#if DESHI_MATH_USE_SSE
}

#ifdef __cplusplus
inline mat4 mat4::
inverse()const{DPZoneScoped;
	#if DESHI_MATH_USE_SSE
#define mat2_mul_mat2(a,b) m128_add_4f32(m128_mul_4f32(a, m128_swizzle(b, 0,3,0,3)), m128_mul_4f32(m128_swizzle(a, 1,0,3,2), m128_swizzle(b, 2,1,2,1)))
#define mat2_adj_mul_mat2(a,b) m128_sub_4f32(m128_mul_4f32(m128_swizzle(a, 3,3,0,0), b), m128_mul_4f32(m128_swizzle(a, 1,1,2,2), m128_swizzle(b, 2,3,0,1)))
#define mat2_mul_adj_mat2(a,b) m128_sub_4f32(m128_mul_4f32(a, m128_swizzle(b, 3,0,3,0)), m128_mul_4f32(m128_swizzle(a, 1,0,3,2), m128_swizzle(b, 2,1,2,1)))
	//!ref: https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html
	
	//2x2 sub matrices (ordered internally tl->tr->bl->br)
	__m128 A = m128_shuffle_0101(this->sse_row0, this->sse_row1); //top left
	__m128 B = m128_shuffle_2323(this->sse_row0, this->sse_row1); //top right
	__m128 C = m128_shuffle_0101(this->sse_row2, this->sse_row3); //bot left
	__m128 D = m128_shuffle_2323(this->sse_row2, this->sse_row3); //bot right
	
	//calculate determinants (and broadcast across m128)
	//TODO test the shuffle determinant calculation
	__m128 detA = _mm_set1_ps(this->arr[ 0] * this->arr[ 5] - this->arr[ 1] * this->arr[ 4]);
	__m128 detB = _mm_set1_ps(this->arr[ 2] * this->arr[ 7] - this->arr[ 3] * this->arr[ 6]);
	__m128 detC = _mm_set1_ps(this->arr[ 8] * this->arr[13] - this->arr[ 9] * this->arr[12]);
	__m128 detD = _mm_set1_ps(this->arr[10] * this->arr[15] - this->arr[11] * this->arr[14]);
	
	//calculate adjugates and determinant //NOTE A# = adjugate A; |A| = determinant A; Atr = trace A
	__m128 D_C = mat2_adj_mul_mat2(D, C); //D#*C
	__m128 A_B = mat2_adj_mul_mat2(A, B); //A#*B
	__m128 X = _mm_sub_ps(_mm_mul_ps(detD, A), mat2_mul_mat2(B, D_C)); //X# = |D|*A - B*(D#*C)
	__m128 W = _mm_sub_ps(_mm_mul_ps(detA, D), mat2_mul_mat2(C, A_B)); //W# = |A|*D - C*(A#*B)
	__m128 detM = _mm_mul_ps(detA, detD); //|M| = |A|*|D| ... (to be continued)
	
	__m128 Y = _mm_sub_ps(_mm_mul_ps(detB, C), mat2_mul_adj_mat2(D, A_B)); //Y# = |B|*C - D*((A#*B)#)
	__m128 Z = _mm_sub_ps(_mm_mul_ps(detC, B), mat2_mul_adj_mat2(A, D_C)); //Z# = |C|*B - A*((D#*C)#)
	detM = _mm_add_ps(detM, _mm_mul_ps(detB, detC)); //|M| = |A|*|D| + |B|*|C| ... (to be continued)
	
	//calculate trace
	__m128 tr = _mm_mul_ps(A_B, m128_shuffle(D_C,D_C, 0,2,1,3)); //((A#*B)*(D#*C))tr
	tr = _mm_hadd_ps(tr, tr);
	tr = _mm_hadd_ps(tr, tr);
	detM = _mm_sub_ps(detM, tr); //|M| = |A|*|D| + |B|*|C| - ((A#*B)*(D#*C))tr
	
	//inverse M = 1/|M| * |X Y|
	//                    |Z W|
	__m128 rDetM = _mm_div_ps(_mm_setr_ps(1.0f, -1.0f, -1.0f, 1.0f), detM); //(1/|M|, -1/|M|, -1/|M|, 1/|M|)
	X = _mm_mul_ps(X, rDetM);
	Y = _mm_mul_ps(Y, rDetM);
	Z = _mm_mul_ps(Z, rDetM);
	W = _mm_mul_ps(W, rDetM);
	
	mat4 result;
	result.sse_row0 = m128_shuffle(X, Y, 3,1,3,1);
	result.sse_row1 = m128_shuffle(X, Y, 2,0,2,0);
	result.sse_row2 = m128_shuffle(Z, W, 3,1,3,1);
	result.sse_row3 = m128_shuffle(Z, W, 2,0,2,0);
	return result;
#undef mat2_mul_adj_mat2
#undef mat2_adj_mul_mat2
#undef mat2_mul_mat2
#else //#if DESHI_MATH_USE_SSE
	return this->adjoint() / this->determinant();
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline mat4
mat4_inverse_transformation_matrix(mat4 lhs){DPZoneScoped;
	//!ref: https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html
	mat4 result;
	
#if DESHI_MATH_USE_SSE
	//transpose scaled rotation matrix
	__m128 temp0 = m128_shuffle_0101(lhs.sse_row0, lhs.sse_row1);
	__m128 temp1 = m128_shuffle_2323(lhs.sse_row0, lhs.sse_row1);
	result.sse_row0 = m128_shuffle(temp0, lhs.sse_row2, 0,2,0,3);
	result.sse_row1 = m128_shuffle(temp0, lhs.sse_row2, 1,3,1,3);
	result.sse_row2 = m128_shuffle(temp1, lhs.sse_row2, 0,2,2,3);
	
	//extract the squared scale
	__m128 sizeSq;
	sizeSq =                       m128_mul_4f32(result.sse_row0, result.sse_row0);
	sizeSq = m128_add_4f32(sizeSq, m128_mul_4f32(result.sse_row1, result.sse_row1));
	sizeSq = m128_add_4f32(sizeSq, m128_mul_4f32(result.sse_row2, result.sse_row2));
	
	//divide by squared scale
	sizeSq = m128_div_4f32(m128_fill_4f32(1.0f), sizeSq);
	result.sse_row0 = m128_mul_4f32(result.sse_row0, sizeSq);
	result.sse_row1 = m128_mul_4f32(result.sse_row1, sizeSq);
	result.sse_row2 = m128_mul_4f32(result.sse_row2, sizeSq);
	
	//dot product against the negative translation
	result.sse_row3 =                                m128_mul_4f32(result.sse_row0, m128_swizzle(lhs.sse_row3, 0,0,0,0));
	result.sse_row3 = m128_add_4f32(result.sse_row3, m128_mul_4f32(result.sse_row1, m128_swizzle(lhs.sse_row3, 1,1,1,1)));
	result.sse_row3 = m128_add_4f32(result.sse_row3, m128_mul_4f32(result.sse_row2, m128_swizzle(lhs.sse_row3, 2,2,2,2)));
	result.sse_row3 = m128_sub_4f32(m128_set_4f32(0.0f, 0.0f, 0.0f, 1.0f), result.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	//transpose rotation matrix
	f32 temp;
	temp = result.arr[1]; result.arr[1] = result.arr[4]; result.arr[4] = temp;
	temp = result.arr[2]; result.arr[2] = result.arr[8]; result.arr[8] = temp;
	temp = result.arr[6]; result.arr[6] = result.arr[9]; result.arr[9] = temp;
	
	//extract and divide by the squared scale
	temp = (result.arr[0] * result.arr[0]) + (result.arr[4] * result.arr[4]) + (result.arr[8] * result.arr[8]);
	result.arr[0] /= temp;
	result.arr[4] /= temp;
	result.arr[8] /= temp;
	temp = (result.arr[1] * result.arr[1]) + (result.arr[5] * result.arr[5]) + (result.arr[9] * result.arr[9]);
	result.arr[1] /= temp;
	result.arr[5] /= temp;
	result.arr[9] /= temp;
	temp = (result.arr[2] * result.arr[2]) + (result.arr[6] * result.arr[6]) + (result.arr[10] * result.arr[10]);
	result.arr[ 2] /= temp;
	result.arr[ 6] /= temp;
	result.arr[10] /= temp;
	
	//dot product against the negative translation
	vec3 negative_translation = vec3(-result.arr[12], -result.arr[13], -result.arr[14]);
	result.arr[12] = (negative_translation.x * result.arr[0]) + (negative_translation.y * result.arr[4]) + (negative_translation.z * result.arr[ 8]);
	result.arr[13] = (negative_translation.x * result.arr[1]) + (negative_translation.y * result.arr[5]) + (negative_translation.z * result.arr[ 9]);
	result.arr[14] = (negative_translation.x * result.arr[2]) + (negative_translation.y * result.arr[6]) + (negative_translation.z * result.arr[10]);
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

DESHI_MATH_FUNC inline mat4
mat4_inverse_transformation_matrix_no_scale(mat4 lhs){DPZoneScoped;
	//!ref: https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html
#if DESHI_MATH_USE_SSE
	mat4 result;
	
	//transpose rotation matrix
	__m128 temp0 = m128_shuffle_0101(lhs.sse_row0, lhs.sse_row1);
	__m128 temp1 = m128_shuffle_2323(lhs.sse_row0, lhs.sse_row1);
	result.sse_row0 = m128_shuffle(temp0, lhs.sse_row2, 0,2,0,3);
	result.sse_row1 = m128_shuffle(temp0, lhs.sse_row2, 1,3,1,3);
	result.sse_row2 = m128_shuffle(temp1, lhs.sse_row2, 0,2,2,3);
	
	//dot product against the negative translation
	result.sse_row3 =                                m128_mul_4f32(result.sse_row0, m128_swizzle(lhs.sse_row3, 0,0,0,0));
	result.sse_row3 = m128_add_4f32(result.sse_row3, m128_mul_4f32(result.sse_row1, m128_swizzle(lhs.sse_row3, 1,1,1,1)));
	result.sse_row3 = m128_add_4f32(result.sse_row3, m128_mul_4f32(result.sse_row2, m128_swizzle(lhs.sse_row3, 2,2,2,2)));
	result.sse_row3 = m128_sub_4f32(m128_set_4f32(0.0f, 0.0f, 0.0f, 1.0f), result.sse_row3);
	
	return result;
#else //#if DESHI_MATH_USE_SSE
	//transpose rotation matrix
	f32 temp;
	temp = result.arr[1]; result.arr[1] = result.arr[4]; result.arr[4] = temp;
	temp = result.arr[2]; result.arr[2] = result.arr[8]; result.arr[8] = temp;
	temp = result.arr[6]; result.arr[6] = result.arr[9]; result.arr[9] = temp;
	
	//dot product against the negative translation
	vec3 negative_translation = vec3(-result.arr[12], -result.arr[13], -result.arr[14]);
	result.arr[12] = (negative_translation.x * result.arr[0]) + (negative_translation.y * result.arr[4]) + (negative_translation.z * result.arr[ 8]);
	result.arr[13] = (negative_translation.x * result.arr[1]) + (negative_translation.y * result.arr[5]) + (negative_translation.z * result.arr[ 9]);
	result.arr[14] = (negative_translation.x * result.arr[2]) + (negative_translation.y * result.arr[6]) + (negative_translation.z * result.arr[10]);
#endif //#else //#if DESHI_MATH_USE_SSE
}

//returns a LH rotation matrix based on input in radians
DESHI_MATH_FUNC inline mat4
mat4_rotation_matrix_x_radians(f32 angle){DPZoneScoped;
	f32 c = DESHI_COSF(angle); f32 s = DESHI_SINF(angle);
	return Mat4(1, 0, 0, 0,
				0, c, s, 0,
				0,-s, c, 0,
				0, 0, 0, 1);
}

//returns a LH rotation matrix based on input in degrees
#define mat4_rotation_matrix_x_degrees(angle) mat4_rotation_matrix_x_radians(DESHI_DEGREES_TO_RADIANS_F32(angle))

//returns a LH rotation matrix based on input in radians
DESHI_MATH_FUNC inline mat4
mat4_rotation_matrix_y_radians(f32 angle){DPZoneScoped;
	f32 c = DESHI_COSF(angle); f32 s = DESHI_SINF(angle);
	return Mat4(c, 0,-s, 0,
				0, 1, 0, 0,
				s, 0, c, 0,
				0, 0, 0, 1);
}

//returns a LH rotation matrix based on input in degrees
#define mat4_rotation_matrix_y_degrees(angle) mat4_rotation_matrix_y_radians(DESHI_DEGREES_TO_RADIANS_F32(angle))

//returns a LH rotation matrix based on input in radians
DESHI_MATH_FUNC inline mat4
mat4_rotation_matrix_z_radians(f32 angle){DPZoneScoped;
	f32 c = DESHI_COSF(angle); f32 s = DESHI_SINF(angle);
	return Mat4( c, s, 0, 0,
				-s, c, 0, 0,
				0,  0, 1, 0,
				0,  0, 0, 1);
}

//returns a LH rotation matrix based on input in degrees
#define mat4_rotation_matrix_z_degrees(angle) mat4_rotation_matrix_z_radians(DESHI_DEGREES_TO_RADIANS_F32(angle))

//returns a pre-multiplied X->Y->Z LH rotation matrix based on input in radians
DESHI_MATH_FUNC inline mat4
mat4_rotation_matrix_radians(f32 x, f32 y, f32 z){DPZoneScoped;
	f32 cX = DESHI_COSF(x); f32 sX = DESHI_SINF(x);
	f32 cY = DESHI_COSF(y); f32 sY = DESHI_SINF(y);
	f32 cZ = DESHI_COSF(z); f32 sZ = DESHI_SINF(z);
	mat4 result;
	result.arr[ 0] = cZ*cY;
	result.arr[ 1] = cY*sZ;
	result.arr[ 2] = -sY;
	result.arr[ 3] = 0;
	result.arr[ 4] = cZ*sX*sY - cX*sZ;
	result.arr[ 5] = cZ*cX + sX*sY*sZ;
	result.arr[ 6] = sX*cY;
	result.arr[ 7] = 0;
	result.arr[ 8] = cZ*cX*sY + sX*sZ;
	result.arr[ 9] = cX*sY*sZ - cZ*sX;
	result.arr[10] = cX*cY;
	result.arr[11] = 0;
	result.arr[12] = 0;
	result.arr[13] = 0;
	result.arr[14] = 0;
	result.arr[15] = 1;
	return result;
}

//returns a pre-multiplied X->Y->Z LH rotation matrix based on input in degrees
#define mat4_rotation_matrix_degrees(x,y,z) mat4_rotation_matrix_radians(DESHI_DEGREES_TO_RADIANS_F32(x), DESHI_DEGREES_TO_RADIANS_F32(y), DESHI_DEGREES_TO_RADIANS_F32(z))

DESHI_MATH_FUNC inline mat4
mat4_translation_matrix(f32 x, f32 y, f32 z){DPZoneScoped;
	return Mat4(1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				x, y, z, 1);
}

DESHI_MATH_FUNC inline mat4
mat4_scale_matrix(f32 x, f32 y, f32 z){DPZoneScoped;
	return Mat4(x, 0, 0, 0,
				0, y, 0, 0,
				0, 0, z, 0,
				0, 0, 0, 1);
}

//returns a pre-multiplied X->Y->Z LH transformation matrix based on input in radians
DESHI_MATH_FUNC inline mat4
mat4_transformation_matrix_radians(f32 translation_x, f32 translation_y, f32 translation_z, f32 rotation_x, f32 rotation_y, f32 rotation_z, f32 scale_x, f32 scale_y, f32 scale_z){DPZoneScoped;
	f32 cX = DESHI_COSF(rotation_x); f32 sX = DESHI_SINF(rotation_x);
	f32 cY = DESHI_COSF(rotation_y); f32 sY = DESHI_SINF(rotation_y);
	f32 cZ = DESHI_COSF(rotation_z); f32 sZ = DESHI_SINF(rotation_z);
	mat4 result;
	result.arr[ 0] = scale_x * (cZ*cY);
	result.arr[ 1] = scale_x * (cY*sZ);
	result.arr[ 2] = scale_x * (-sY);
	result.arr[ 3] = 0;
	result.arr[ 4] = scale_y * (cZ*sX*sY - cX*sZ);
	result.arr[ 5] = scale_y * (cZ*cX + sX*sY*sZ);
	result.arr[ 6] = scale_y * (sX*cY);
	result.arr[ 7] = 0;
	result.arr[ 8] = scale_y * (cZ*cX*sY + sX*sZ);
	result.arr[ 9] = scale_y * (cX*sY*sZ - cZ*sX);
	result.arr[10] = scale_z * (cX*cY);
	result.arr[11] = 0;
	result.arr[12] = translation_x;
	result.arr[13] = translation_y;
	result.arr[14] = translation_z;
	result.arr[15] = 1;
	return result;
}

//returns a pre-multiplied X->Y->Z LH transformation matrix based on input in degrees
#define mat4_transformation_matrix_degrees(tx,ty,tz,rx,ry,rz,sx,sy,sz) mat4_transformation_matrix_radians((tx), (ty), (tz), DESHI_DEGREES_TO_RADIANS_F32(rx), DESHI_DEGREES_TO_RADIANS_F32(ry), DESHI_DEGREES_TO_RADIANS_F32(rz), (sx), (sy), (sz))


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @mat_conversions


DESHI_MATH_FUNC inline mat4
mat3_to_mat4(mat3 lhs){DPZoneScoped;
	return Mat4(lhs.arr[0], lhs.arr[1], lhs.arr[2], 0,
				lhs.arr[3], lhs.arr[4], lhs.arr[5], 0,
				lhs.arr[6], lhs.arr[7], lhs.arr[8], 0,
				0,          0,          0,          1);
}

DESHI_MATH_FUNC inline mat3
mat4_to_mat3(mat4 lhs){DPZoneScoped;
	return Mat4(lhs.arr[0], lhs.arr[1], lhs.arr[ 2]
				lhs.arr[4], lhs.arr[5], lhs.arr[ 6]
				lhs.arr[8], lhs.arr[9], lhs.arr[10]);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @mat_hashing
#ifndef DESHI_MATH_DISABLE_HASHING
#ifdef __cplusplus


template<> 
struct hash<mat3>{
	inline size_t operator()(mat3 const& m)const{DPZoneScoped;
		size_t seed = 0;
		hash<float> hasher;
		size_t hash;
		for(u32 i = 0; i < 9; ++i){
			hash = hasher(m.arr[i]);
			hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= hash;
		}
		return seed;
	}
};

template<> 
struct hash<mat4>{
	inline size_t operator()(mat4 const& m)const{DPZoneScoped;
		size_t seed = 0;
		hash<float> hasher;
		size_t hash;
		for(u32 i = 0; i < 16; ++i){
			hash = hasher(m.arr[i]);
			hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= hash;
		}
		return seed;
	}
};


#endif //#ifdef __cplusplus
#endif //#ifndef DESHI_MATH_DISABLE_HASHING
//~////////////////////////////////////////////////////////////////////////////////////////////////
// @mat_tostring
#ifndef DESHI_MATH_DISABLE_TOSTRING


DESHI_MATH_FUNC inline dstr8
mat3_to_dstr8(mat3 x, Allocator* a){DPZoneScoped;
	dstr8 s;
	s.allocator = a;
	s.count = snprintf(0, 0, "|%g, %g, %g|\n|%g, %g, %g|\n|%g, %g, %g|",
					   x.arr[0], x.arr[1], x.arr[2],
					   x.arr[3], x.arr[4], x.arr[5],
					   x.arr[6], x.arr[7], x.arr[8]);
	s.str = (u8*)a->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "|%g, %g, %g|\n|%g, %g, %g|\n|%g, %g, %g|",
			 x.arr[0], x.arr[1], x.arr[2],
			 x.arr[3], x.arr[4], x.arr[5],
			 x.arr[6], x.arr[7], x.arr[8]);
	return s;
}

#ifdef __cplusplus
inline dstr8
to_dstr8(const mat3& x, Allocator* a = KIGU_STRING_ALLOCATOR){DPZoneScoped;
	dstr8 s;
	s.allocator = a;
	s.count = snprintf(0, 0, "|%g, %g, %g|\n|%g, %g, %g|\n|%g, %g, %g|",
					   x.arr[0], x.arr[1], x.arr[2],
					   x.arr[3], x.arr[4], x.arr[5],
					   x.arr[6], x.arr[7], x.arr[8]);
	s.str = (u8*)a->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "|%g, %g, %g|\n|%g, %g, %g|\n|%g, %g, %g|",
			 x.arr[0], x.arr[1], x.arr[2],
			 x.arr[3], x.arr[4], x.arr[5],
			 x.arr[6], x.arr[7], x.arr[8]);
	return s;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline dstr8
mat3_to_dstr8p(mat3 x, int precision, Allocator* a){DPZoneScoped;
	dstr8 s;
	s.allocator = a;
	s.count = snprintf(0, 0, "|%.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f|",
					   precision, x.arr[0], precision, x.arr[1], precision, x.arr[2],
					   precision, x.arr[3], precision, x.arr[4], precision, x.arr[5],
					   precision, x.arr[6], precision, x.arr[7], precision, x.arr[8]);
	s.str = (u8*)a->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "|%.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f|",
			 precision, x.arr[0], precision, x.arr[1], precision, x.arr[2],
			 precision, x.arr[3], precision, x.arr[4], precision, x.arr[5],
			 precision, x.arr[6], precision, x.arr[7], precision, x.arr[8]);
	return s;
}

#ifdef __cplusplus
inline dstr8
to_dstr8p(const mat3& x, int precision, Allocator* a = KIGU_STRING_ALLOCATOR){DPZoneScoped;
	dstr8 s;
	s.allocator = a;
	s.count = snprintf(0, 0, "|%.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f|",
					   precision, x.arr[0], precision, x.arr[1], precision, x.arr[2],
					   precision, x.arr[3], precision, x.arr[4], precision, x.arr[5],
					   precision, x.arr[6], precision, x.arr[7], precision, x.arr[8]);
	s.str = (u8*)a->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "|%.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f|",
			 precision, x.arr[0], precision, x.arr[1], precision, x.arr[2],
			 precision, x.arr[3], precision, x.arr[4], precision, x.arr[5],
			 precision, x.arr[6], precision, x.arr[7], precision, x.arr[8]);
	return s;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline dstr8
mat4_to_dstr8(mat4 x, Allocator* a){DPZoneScoped;
	dstr8 s;
	s.allocator = a;
	s.count = snprintf(0, 0, "|%g, %g, %g, %g|\n|%g, %g, %g, %g|\n|%g, %g, %g, %g|\n|%g, %g, %g, %g|",
					   x.arr[ 0], x.arr[ 1], x.arr[ 2], x.arr[ 3],
					   x.arr[ 4], x.arr[ 5], x.arr[ 6], x.arr[ 7],
					   x.arr[ 8], x.arr[ 9], x.arr[10], x.arr[11],
					   x.arr[12], x.arr[13], x.arr[14], x.arr[15]);
	s.str = (u8*)a->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "|%g, %g, %g, %g|\n|%g, %g, %g, %g|\n|%g, %g, %g, %g|\n|%g, %g, %g, %g|",
			 x.arr[ 0], x.arr[ 1], x.arr[ 2], x.arr[ 3],
			 x.arr[ 4], x.arr[ 5], x.arr[ 6], x.arr[ 7],
			 x.arr[ 8], x.arr[ 9], x.arr[10], x.arr[11],
			 x.arr[12], x.arr[13], x.arr[14], x.arr[15]);
	return s;
}

#ifdef __cplusplus
inline dstr8
to_dstr8(const mat3& x, Allocator* a = KIGU_STRING_ALLOCATOR){DPZoneScoped;
	dstr8 s;
	s.allocator = a;
	s.count = snprintf(0, 0, "|%g, %g, %g, %g|\n|%g, %g, %g, %g|\n|%g, %g, %g, %g|\n|%g, %g, %g, %g|",
					   x.arr[ 0], x.arr[ 1], x.arr[ 2], x.arr[ 3],
					   x.arr[ 4], x.arr[ 5], x.arr[ 6], x.arr[ 7],
					   x.arr[ 8], x.arr[ 9], x.arr[10], x.arr[11],
					   x.arr[12], x.arr[13], x.arr[14], x.arr[15]);
	s.str = (u8*)a->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "|%g, %g, %g, %g|\n|%g, %g, %g, %g|\n|%g, %g, %g, %g|\n|%g, %g, %g, %g|",
			 x.arr[ 0], x.arr[ 1], x.arr[ 2], x.arr[ 3],
			 x.arr[ 4], x.arr[ 5], x.arr[ 6], x.arr[ 7],
			 x.arr[ 8], x.arr[ 9], x.arr[10], x.arr[11],
			 x.arr[12], x.arr[13], x.arr[14], x.arr[15]);
	return s;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC dstr8
mat3_to_dstr8p(mat4 x, int precision, Allocator* a){DPZoneScoped;
	dstr8 s;
	s.allocator = a;
	s.count = snprintf(0, 0, "|%.*f, %.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f, %.*f|",
					   precision, x.arr[ 0], precision, x.arr[ 1], precision, x.arr[ 2], precision, x.arr[ 3],
					   precision, x.arr[ 4], precision, x.arr[ 5], precision, x.arr[ 6], precision, x.arr[ 7],
					   precision, x.arr[ 8], precision, x.arr[ 9], precision, x.arr[10], precision, x.arr[11],
					   precision, x.arr[12], precision, x.arr[13], precision, x.arr[14], precision, x.arr[15]);
	s.str = (u8*)a->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "|%.*f, %.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f, %.*f|",
			 precision, x.arr[ 0], precision, x.arr[ 1], precision, x.arr[ 2], precision, x.arr[ 3],
			 precision, x.arr[ 4], precision, x.arr[ 5], precision, x.arr[ 6], precision, x.arr[ 7],
			 precision, x.arr[ 8], precision, x.arr[ 9], precision, x.arr[10], precision, x.arr[11],
			 precision, x.arr[12], precision, x.arr[13], precision, x.arr[14], precision, x.arr[15]);
	return s;
}

#ifdef __cplusplus
global dstr8
to_dstr8p(const mat4& x, int precision, Allocator* a = KIGU_STRING_ALLOCATOR){DPZoneScoped;
	dstr8 s;
	s.allocator = a;
	s.count = snprintf(0, 0, "|%.*f, %.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f, %.*f|",
					   precision, x.arr[ 0], precision, x.arr[ 1], precision, x.arr[ 2], precision, x.arr[ 3],
					   precision, x.arr[ 4], precision, x.arr[ 5], precision, x.arr[ 6], precision, x.arr[ 7],
					   precision, x.arr[ 8], precision, x.arr[ 9], precision, x.arr[10], precision, x.arr[11],
					   precision, x.arr[12], precision, x.arr[13], precision, x.arr[14], precision, x.arr[15]);
	s.str = (u8*)a->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	snprintf((char*)s.str, s.count+1, "|%.*f, %.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f, %.*f|\n|%.*f, %.*f, %.*f, %.*f|",
			 precision, x.arr[ 0], precision, x.arr[ 1], precision, x.arr[ 2], precision, x.arr[ 3],
			 precision, x.arr[ 4], precision, x.arr[ 5], precision, x.arr[ 6], precision, x.arr[ 7],
			 precision, x.arr[ 8], precision, x.arr[ 9], precision, x.arr[10], precision, x.arr[11],
			 precision, x.arr[12], precision, x.arr[13], precision, x.arr[14], precision, x.arr[15]);
	return s;
}
#endif //#ifdef __cplusplus


#endif //#ifndef DESHI_MATH_DISABLE_TOSTRING
//~////////////////////////////////////////////////////////////////////////////////////////////////
// @mat_vec_interactions


DESHI_MATH_FUNC inline mat3
vec3_rows_to_mat3(vec3 row0, vec3 row1, vec3 row2){DPZoneScoped;
	mat3 result;
	result.row0 = row0;
	result.row1 = row1;
	result.row2 = row2;
	return result;
}

DESHI_MATH_FUNC inline mat4
vec4_rows_to_mat4(vec4 row0, vec4 row1, vec4 row2, vec4 row3){DPZoneScoped;
	mat4 result;
#if DESHI_MATH_USE_SSE
	result.row0 = row0;
	result.row1 = row1;
	result.row2 = row2;
	result.row3 = row3;
#else //#if DESHI_MATH_USE_SSE
	result.sse_row0 = row0.sse;
	result.sse_row1 = row1.sse;
	result.sse_row2 = row2.sse;
	result.sse_row3 = row3.sse;
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

DESHI_MATH_FUNC inline vec3
vec3_mul_mat3(vec3 lhs, mat3 rhs){DPZoneScoped;
	vec3 result;
	result.x = (lhs.x * rhs.arr[0]) + (lhs.y * rhs.arr[3]) + (lhs.z * rhs.arr[6]);
	result.y = (lhs.x * rhs.arr[1]) + (lhs.y * rhs.arr[4]) + (lhs.z * rhs.arr[7]);
	result.z = (lhs.x * rhs.arr[2]) + (lhs.y * rhs.arr[5]) + (lhs.z * rhs.arr[8]);
	return result;
}

#ifdef __cplusplus
inline vec3 vec3::
operator* (const mat3& rhs)const{DPZoneScoped;
	vec3 result;
	result.x = (this->x * rhs.arr[0]) + (this->y * rhs.arr[3]) + (this->z * rhs.arr[6]);
	result.y = (this->x * rhs.arr[1]) + (this->y * rhs.arr[4]) + (this->z * rhs.arr[7]);
	result.z = (this->x * rhs.arr[2]) + (this->y * rhs.arr[5]) + (this->z * rhs.arr[8]);
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec3::
operator*=(const mat3& rhs){DPZoneScoped;
	vec3 result;
	result.x = (this->x * rhs.arr[0]) + (this->y * rhs.arr[3]) + (this->z * rhs.arr[6]);
	result.y = (this->x * rhs.arr[1]) + (this->y * rhs.arr[4]) + (this->z * rhs.arr[7]);
	result.z = (this->x * rhs.arr[2]) + (this->y * rhs.arr[5]) + (this->z * rhs.arr[8]);
	*this = result;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
vec3_mul_mat4(vec3 lhs, mat4 rhs){DPZoneScoped;
	vec3 result;
#if DESHI_MATH_USE_SSE
	vec4 temp.sse =                    m128_mul_4f32(m128_fill_4f32(lhs.x), rhs.sse_row0);
	temp.sse = m128_add_4f32(temp.sse, m128_mul_4f32(m128_fill_4f32(lhs.y), rhs.sse_row1));
	temp.sse = m128_add_4f32(temp.sse, m128_mul_4f32(m128_fill_4f32(lhs.z), rhs.sse_row2));
	temp.sse = m128_add_4f32(temp.sse, rhs.sse_row3);
	result.x = temp.x;
	result.y = temp.y;
	result.z = temp.z;
#else //#if DESHI_MATH_USE_SSE
	result.x = (lhs.x * rhs.arr[0]) + (lhs.y * rhs.arr[4]) + (lhs.z * rhs.arr[ 8]) + (rhs.arr[12]);
	result.y = (lhs.x * rhs.arr[1]) + (lhs.y * rhs.arr[5]) + (lhs.z * rhs.arr[ 9]) + (rhs.arr[13]);
	result.z = (lhs.x * rhs.arr[2]) + (lhs.y * rhs.arr[6]) + (lhs.z * rhs.arr[10]) + (rhs.arr[14]);
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline vec3 vec3::
operator* (const mat4& rhs)const{DPZoneScoped;
	vec3 result;
#if DESHI_MATH_USE_SSE
	vec4 temp.sse =                    m128_mul_4f32(m128_fill_4f32(this->x), rhs.sse_row0);
	temp.sse = m128_add_4f32(temp.sse, m128_mul_4f32(m128_fill_4f32(this->y), rhs.sse_row1));
	temp.sse = m128_add_4f32(temp.sse, m128_mul_4f32(m128_fill_4f32(this->z), rhs.sse_row2));
	temp.sse = m128_add_4f32(temp.sse, rhs.sse_row3);
	result.x = temp.x;
	result.y = temp.y;
	result.z = temp.z;
#else //#if DESHI_MATH_USE_SSE
	result.x = (this->x * rhs.arr[0]) + (this->y * rhs.arr[4]) + (this->z * rhs.arr[ 8]) + (rhs.arr[12]);
	result.y = (this->x * rhs.arr[1]) + (this->y * rhs.arr[5]) + (this->z * rhs.arr[ 9]) + (rhs.arr[13]);
	result.z = (this->x * rhs.arr[2]) + (this->y * rhs.arr[6]) + (this->z * rhs.arr[10]) + (rhs.arr[14]);
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec3::
operator*=(const mat4& rhs){DPZoneScoped;
	vec3 result;
#if DESHI_MATH_USE_SSE
	vec4 temp.sse =                    m128_mul_4f32(m128_fill_4f32(this->x), rhs.sse_row0);
	temp.sse = m128_add_4f32(temp.sse, m128_mul_4f32(m128_fill_4f32(this->y), rhs.sse_row1));
	temp.sse = m128_add_4f32(temp.sse, m128_mul_4f32(m128_fill_4f32(this->z), rhs.sse_row2));
	temp.sse = m128_add_4f32(temp.sse, rhs.sse_row3);
	result.x = temp.x;
	result.y = temp.y;
	result.z = temp.z;
#else //#if DESHI_MATH_USE_SSE
	result.x = (this->x * rhs.arr[0]) + (this->y * rhs.arr[4]) + (this->z * rhs.arr[ 8]) + (rhs.arr[12]);
	result.y = (this->x * rhs.arr[1]) + (this->y * rhs.arr[5]) + (this->z * rhs.arr[ 9]) + (rhs.arr[13]);
	result.z = (this->x * rhs.arr[2]) + (this->y * rhs.arr[6]) + (this->z * rhs.arr[10]) + (rhs.arr[14]);
#endif //#else //#if DESHI_MATH_USE_SSE
	*this = result;
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
vec4_mul_mat4(vec4 lhs, mat4 rhs){DPZoneScoped;
	vec4 result;
#if DESHI_MATH_USE_SSE
	result.sse = m128_linear_combine(lhs.sse, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	result.x = (lhs.x * rhs.arr[0]) + (lhs.y * rhs.arr[4]) + (lhs.z * rhs.arr[ 8]) + (lhs.w * rhs.arr[12]);
	result.y = (lhs.x * rhs.arr[1]) + (lhs.y * rhs.arr[5]) + (lhs.z * rhs.arr[ 9]) + (lhs.w * rhs.arr[13]);
	result.z = (lhs.x * rhs.arr[2]) + (lhs.y * rhs.arr[6]) + (lhs.z * rhs.arr[10]) + (lhs.w * rhs.arr[14]);
	result.w = (lhs.x * rhs.arr[3]) + (lhs.y * rhs.arr[7]) + (lhs.z * rhs.arr[11]) + (lhs.w * rhs.arr[15]);
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}

#ifdef __cplusplus
inline vec4 vec4::
operator* (const mat4& rhs)const{DPZoneScoped;
	vec4 result;
#if DESHI_MATH_USE_SSE
	result.sse = m128_linear_combine(this->sse, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	result.x = (this->x * rhs.arr[0]) + (this->y * rhs.arr[4]) + (this->z * rhs.arr[ 8]) + (this->w * rhs.arr[12]);
	result.y = (this->x * rhs.arr[1]) + (this->y * rhs.arr[5]) + (this->z * rhs.arr[ 9]) + (this->w * rhs.arr[13]);
	result.z = (this->x * rhs.arr[2]) + (this->y * rhs.arr[6]) + (this->z * rhs.arr[10]) + (this->w * rhs.arr[14]);
	result.w = (this->x * rhs.arr[3]) + (this->y * rhs.arr[7]) + (this->z * rhs.arr[11]) + (this->w * rhs.arr[15]);
#endif //#else //#if DESHI_MATH_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4::
operator*=(const mat4& rhs){DPZoneScoped;
#if DESHI_MATH_USE_SSE
	this->sse = m128_linear_combine(this->sse, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else //#if DESHI_MATH_USE_SSE
	vec4 result;
	result.x = (this->x * rhs.arr[0]) + (this->y * rhs.arr[4]) + (this->z * rhs.arr[ 8]) + (this->w * rhs.arr[12]);
	result.y = (this->x * rhs.arr[1]) + (this->y * rhs.arr[5]) + (this->z * rhs.arr[ 9]) + (this->w * rhs.arr[13]);
	result.z = (this->x * rhs.arr[2]) + (this->y * rhs.arr[6]) + (this->z * rhs.arr[10]) + (this->w * rhs.arr[14]);
	result.w = (this->x * rhs.arr[3]) + (this->y * rhs.arr[7]) + (this->z * rhs.arr[11]) + (this->w * rhs.arr[15]);
	*this = result;
#endif //#else //#if DESHI_MATH_USE_SSE
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
mat3_row(mat3 lhs, u32 row){DPZoneScoped;
	Assert(row < 3, "mat3 subscript out of bounds");
	return Vec3(lhs.arr[3*row], lhs.arr[3*row+1], lhs.arr[3*row+2]);
}

#ifdef __cplusplus
inline vec3 mat3::
row(u32 row){DPZoneScoped;
	Assert(row < 3, "mat3 subscript out of bounds");
	return Vec3(this->arr[3*row], this->arr[3*row+1], this->arr[3*row+2]);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec3
mat3_col(mat3 lhs, u32 col){DPZoneScoped;
	Assert(col < 3, "mat3 subscript out of bounds");
	return Vec3(lhs.arr[col], lhs.arr[3+col], lhs.arr[6+col]);
}

#ifdef __cplusplus
inline vec3 mat3::
col(u32 col){DPZoneScoped;
	Assert(col < 3, "mat3 subscript out of bounds");
	return Vec3(this->arr[col], this->arr[3+col], this->arr[6+col]);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
mat4_row(mat4 lhs, u32 row){DPZoneScoped;
	Assert(row < 4, "mat4 subscript out of bounds");
	return Vec3(lhs.arr[4*row], lhs.arr[4*row+1], lhs.arr[4*row+2], lhs.arr[4*row+3]);
}

#ifdef __cplusplus
inline vec4 mat4::
row(u32 row){DPZoneScoped;
	Assert(row < 4, "mat4 subscript out of bounds");
	return Vec4(this->arr[4*row], this->arr[4*row+1], this->arr[4*row+2], this->arr[4*row+3]);
}
#endif //#ifdef __cplusplus

DESHI_MATH_FUNC inline vec4
mat4_col(mat4 lhs, u32 col){DPZoneScoped;
	Assert(col < 4, "mat4 subscript out of bounds");
	return Vec4(lhs.arr[col], lhs.arr[4+col], lhs.arr[8+col], lhs.arr[12+col]);
}

#ifdef __cplusplus
inline vec4 mat4::
col(u32 col){DPZoneScoped;
	Assert(col < 4, "mat4 subscript out of bounds");
	return Vec4(this->arr[col], this->arr[4+col], this->arr[8+col], this->arr[12+col]);
}
#endif //#ifdef __cplusplus

//returns a pre-multiplied X->Y->Z LH rotation matrix based on input in radians
DESHI_MATH_FUNC mat3
mat3_rotation_matrix_radians_vec3(vec3 rotation){DPZoneScoped;
	f32 cX = DESHI_COSF(rotation.x); f32 sX = DESHI_SINF(rotation.x);
	f32 cY = DESHI_COSF(rotation.y); f32 sY = DESHI_SINF(rotation.y);
	f32 cZ = DESHI_COSF(rotation.z); f32 sZ = DESHI_SINF(rotation.z);
	mat3 result;
	result.arr[0] = cZ*cY;
	result.arr[1] = cY*sZ;
	result.arr[2] = -sY;
	result.arr[3] = cZ*sX*sY - cX*sZ;
	result.arr[4] = cZ*cX + sX*sY*sZ;
	result.arr[5] = sX*cY;
	result.arr[6] = cZ*cX*sY + sX*sZ;
	result.arr[7] = cX*sY*sZ - cZ*sX;
	result.arr[8] = cX*cY;
	return result;
}

//returns a pre-multiplied X->Y->Z LH rotation matrix based on input in degrees
#define mat3_rotation_matrix_degrees_vec3(rotation) mat3_rotation_matrix_radians_vec3(Vec3(DESHI_DEGREES_TO_RADIANS_F32((rotation).x), DESHI_DEGREES_TO_RADIANS_F32((rotation).y), DESHI_DEGREES_TO_RADIANS_F32((rotation).z)))

DESHI_MATH_FUNC inline mat3
mat3_scale_matrix_vec3(vec3 scale){DPZoneScoped;
	return Mat3(scale.x, 0, 0
				0, scale.y, 0,
				0, 0, scale.z);
}

//returns a pre-multiplied X->Y->Z LH rotation matrix based on input in radians
DESHI_MATH_FUNC mat4
mat4_rotation_matrix_radians_vec3(vec3 rotation){DPZoneScoped;
	f32 cX = DESHI_COSF(rotation.x); f32 sX = DESHI_SINF(rotation.x);
	f32 cY = DESHI_COSF(rotation.y); f32 sY = DESHI_SINF(rotation.y);
	f32 cZ = DESHI_COSF(rotation.z); f32 sZ = DESHI_SINF(rotation.z);
	mat4 result;
	result.arr[ 0] = cZ*cY;
	result.arr[ 1] = cY*sZ;
	result.arr[ 2] = -sY;
	result.arr[ 3] = 0;
	result.arr[ 4] = cZ*sX*sY - cX*sZ;
	result.arr[ 5] = cZ*cX + sX*sY*sZ;
	result.arr[ 6] = sX*cY;
	result.arr[ 7] = 0;
	result.arr[ 8] = cZ*cX*sY + sX*sZ;
	result.arr[ 9] = cX*sY*sZ - cZ*sX;
	result.arr[10] = cX*cY;
	result.arr[11] = 0;
	result.arr[12] = 0;
	result.arr[13] = 0;
	result.arr[14] = 0;
	result.arr[15] = 1;
	return result;
}

//returns a pre-multiplied X->Y->Z LH rotation matrix based on input in degrees
#define mat4_rotation_matrix_degrees_vec3(rotation) mat4_rotation_matrix_radians_vec3(Vec3(DESHI_DEGREES_TO_RADIANS_F32((rotation).x), DESHI_DEGREES_TO_RADIANS_F32((rotation).y), DESHI_DEGREES_TO_RADIANS_F32((rotation).z)))

DESHI_MATH_FUNC mat4
mat4_translation_matrix_vec3(vec3 translation){DPZoneScoped;
	return Mat4(1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				translation.x, translation.y, translation.z, 1);
}

DESHI_MATH_FUNC mat4
mat4_scale_matrix_vec3(vec3 scale){DPZoneScoped;
	return Mat4(scale.x, 0, 0, 0,
				0, scale.y, 0, 0,
				0, 0, scale.z, 0,
				0, 0, 0,       1);
}

//returns a pre-multiplied X->Y->Z LH transformation matrix based on input in radians
DESHI_MATH_FUNC mat4
mat4_transformation_matrix_radians_vec3(vec3 translation, vec3 rotation, vec3 scale){DPZoneScoped;
	f32 cX = DESHI_COSF(rotation.x); f32 sX = DESHI_SINF(rotation.x);
	f32 cY = DESHI_COSF(rotation.y); f32 sY = DESHI_SINF(rotation.y);
	f32 cZ = DESHI_COSF(rotation.z); f32 sZ = DESHI_SINF(rotation.z);
	mat4 result;
	result.arr[ 0] = scale.x * (cZ*cY);
	result.arr[ 1] = scale.x * (cY*sZ);
	result.arr[ 2] = scale.x * (-sY);
	result.arr[ 3] = 0;
	result.arr[ 4] = scale.y * (cZ*sX*sY - cX*sZ);
	result.arr[ 5] = scale.y * (cZ*cX + sX*sY*sZ);
	result.arr[ 6] = scale.y * (sX*cY);
	result.arr[ 7] = 0;
	result.arr[ 8] = scale.y * (cZ*cX*sY + sX*sZ);
	result.arr[ 9] = scale.y * (cX*sY*sZ - cZ*sX);
	result.arr[10] = scale.z * (cX*cY);
	result.arr[11] = 0;
	result.arr[12] = translation.x;
	result.arr[13] = translation.y;
	result.arr[14] = translation.z;
	result.arr[15] = 1;
	return result;
}

//returns a pre-multiplied X->Y->Z LH transformation matrix based on input in degrees
#define mat4_transformation_matrix_degrees_vec3(translation,rotation,scale) mat4_rotation_matrix_radians_vec3(translation, Vec3(DESHI_DEGREES_TO_RADIANS_F32((rotation).x), DESHI_DEGREES_TO_RADIANS_F32((rotation).y), DESHI_DEGREES_TO_RADIANS_F32((rotation).z)), scale)


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @geometry


DESHI_MATH_FUNC inline b32
point_is_in_rectangle(vec2 point, vec2 rect_pos, vec2 rect_size){DPZoneScoped;
	return point.x >= rect_pos.x
		&& point.y >= rect_pos.y
		&& point.x <= rect_pos.x + rect_size.x
		&& point.y <= rect_pos.y + rect_size.y;
}

DESHI_MATH_FUNC inline b32
point_is_in_triangle(vec2 point, vec2 p0, vec2 p1, vec2 p2){DPZoneScoped;
	vec2 p01 = p1 - p0;
	vec2 p12 = p2 - p1;
	b32 b0 = vec2_dot(vec2_sub(point, p0), Vec2(-p01.y, p01.x)) < 0.0f;
	b32 b1 = vec2_dot(vec2_sub(point, p1), Vec2(-p12.y, p12.x)) < 0.0f;
	if(b0 != b1){
		return false;
	}
	
	vec2 p20 = p0 - p2;
	b32 b2 = vec2_dot(vec2_sub(point, p2), Vec2(-p20.y, p20.x)) < 0.0f;
	if(b1 != b2){
		return false;
	}
	
	return true;
}

DESHI_MATH_FUNC inline vec2
vec2_line_line_intersect(vec2 line0_start, vec2 line0_end, vec2 line1_start, vec2 line1_end){
	f32 m0 = vec2_slope(line0_start, line0_end);
	f32 m1 = vec2_slope(line1_start, line1_end);
	f32 b0 = line0_end.y - (m0 * line0_end.x);
	f32 b1 = line1_end.y - (m1 * line1_end.x);
	f32 x = (b1 - b0) / (m0 - m1);
	f32 y = (m0 * x) + b0;
	return Vec2(x, y);
}

DESHI_MATH_FUNC inline f32
vec3_distance_between_point_and_plane(vec3 point, vec3 plane_point, vec3 plane_normal){
	return vec3_dot(vec3_sub(point, plane_point), plane_normal);
}

//Returns where a line's vector intersects with a plane
DESHI_MATH_FUNC inline vec3
line_plane_intersect(vec3 line_start, vec3 line_end, vec3 plane_point, vec3 plane_normal){
	vec3 line_direction = vec3_normalize(vec3_sub(line_end, line_start));
	vec3 line_start_to_plane_point = vec3_sub(plane_point, line_start);
	f32 distance_along_line = vec3_dot(line_start_to_plane_point, plane_normal) / vec3_dot(line_direction, plane_normal);
	return vec3_add(line_start, vec3_mul_f32(line_direction, distance_along_line));
}

//Returns where a line's vector intersects with a plane and outputs the distance along (or beyond) that line the intersection occurred
DESHI_MATH_FUNC inline vec3
line_plane_intersect_output_distance(vec3 line_start, vec3 line_end, vec3 plane_point, vec3 plane_normal, f32* out_distance){
	vec3 line_direction = vec3_normalize(vec3_sub(line_end, line_start));
	vec3 line_start_to_plane_point = vec3_sub(plane_point, line_start);
	f32 distance_along_line = vec3_dot(line_start_to_plane_point, plane_normal) / vec3_dot(line_direction, plane_normal);
	*out_distance = distance_along_line;
	return vec3_add(line_start, vec3_mul_f32(line_direction, distance_along_line));
}

DESHI_MATH_FUNC inline f32
vec3_triangle_area(vec3 p0, vec3 p1, vec3 p3){
	return vec3_mag(vec3_cross(vec3_sub(p1, p0), vec3_sub(p2, p0))) / 2.0f;
}

//Returns a triangle's normal based on the order the points are provided
DESHI_MATH_FUNC inline vec3
vec3_triangle_normal(vec3 p0, vec3 p1, vec3 p2){
	return vec2_normalize(vec3_cross(vec3_sub(p2, p0), vec3_sub(p1, p0)));
}

DESHI_MATH_FUNC inline vec3
vec3_triangle_midpoint(vec3 p0, vec3 p1, vec3 p2){
	vec3 result;
	result.x = (p0.x + p1.x + p2.x) / 3.0f;
	result.x = (p0.y + p1.y + p2.y) / 3.0f;
	result.x = (p0.z + p1.z + p2.z) / 3.0f;
	return result;
}

DESHI_MATH_FUNC inline vec3
map_spherical_to_rectangular_radians(vec3 point){
	vec3 result;
	f32 cY = DESHI_COSF(point.y); f32 sY = DESHI_SINF(point.y);
	f32 cZ = DESHI_COSF(point.z); f32 sZ = DESHI_SINF(point.z);
	result.x = point.x * cY * sZ;
	result.y = point.x      * cZ;
	result.z = point.x * sY * sZ;
	return result;
}

DESHI_MATH_FUNC inline vec3
map_spherical_to_rectangular_degrees(vec3 point){
	vec3 result;
	f32 rY = DESHI_DEGREES_TO_RADIANS_F32(point.y);
	f32 rZ = DESHI_DEGREES_TO_RADIANS_F32(point.z);
	f32 cY = DESHI_COSF(rY); f32 sY = DESHI_SINF(rY);
	f32 cZ = DESHI_COSF(rZ); f32 sZ = DESHI_SINF(rZ);
	result.x = point.x * cY * sZ;
	result.y = point.x      * cZ;
	result.z = point.x * sY * sZ;
	return result;
}

DESHI_MATH_FUNC inline vec3
map_rectangular_to_spherical_radians(vec3 point){
	f32 m = vec3_mag(point);
	f32 rho = DESHI_SQRTF(m);
	f32 theta = DESHI_ATANF(point.y / point.z);
	f32 phi = DESHI_ACOSF(point.z / m); //!TESTME maybe use v.y instead of v.z because y is our vertical axis
	return Vec3(rho, theta, phi);
}

DESHI_MATH_FUNC inline vec3
map_rectangular_to_spherical_degrees(vec3 point){
	f32 m = vec3_mag(point);
	f32 rho = DESHI_DEGREES_TO_RADIANS_F32(DESHI_SQRTF(m));
	f32 theta = DESHI_DEGREES_TO_RADIANS_F32(DESHI_ATANF(point.y / point.z));
	f32 phi = DESHI_ACOSF(point.z / m);
	return Vec3(rho, theta, phi);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @camera


//Returns a perspective projection matrix for a window with `width` and `height` using `horizontal_fov` in degrees
//  the clipping planes `near_clip` and `far_clip` are used to determine the overall render distance
//  the depth range is from 0.0 to 1.0
DESHI_MATH_FUNC inline mat4
mat4_perspective_projection_matrix(f32 width, f32 height, f32 horizontal_fov, f32 near_clip, f32 far_clip){
	float render_distance = far_z - near_z;
	float fov = 1.0f / DESHI_TANF(DESHI_DEGREES_TO_RADIANS_F32(horizontal_fov / 2.0f));
	mat4 result;
	
	result.arr[ 0] = (height / width) * fov;
	result.arr[ 1] = 0.0f;
	result.arr[ 2] = 0.0f;
	result.arr[ 3] = 0.0f;
	
	result.arr[ 4] = 0.0f;
	result.arr[ 5] = fov;
	result.arr[ 6] = 0.0f;
	result.arr[ 7] = 0.0f;
	
	result.arr[ 8] = 0.0f;
	result.arr[ 9] = 0.0f;
	result.arr[10] = far_z / render_distance;
	result.arr[11] = 1.0f;
	
	result.arr[12] = 0.0f;
	result.arr[13] = 0.0f;
	result.arr[14] = -(far_z * near_z) / render_distance;
	result.arr[15] = 0.0f;
	
	return result;
}

//Returns an orthographic projection matrix
//  `top`, `bottom`, `left`, and `right` are the scene bounds in screen space with the 2D origin assumed to be the top-left
//  `near` and `far` are the clipping planes used to determine the overall render distance
DESHI_MATH_FUNC inline mat4
mat4_orthographic_projection_matrix(f32 top, f32 bottom, f32 left, f32 right, f32 near, f32 far){
	mat4 result;
	
	result.arr[ 0] = 2.0f / (right - left);
	result.arr[ 1] = 0.0f;
	result.arr[ 2] = 0.0f;
	result.arr[ 3] = 0.0f;
	
	result.arr[ 4] = 0.0f;
	result.arr[ 5] = 2.0f / (bottom - top);
	result.arr[ 6] = 0.0f;
	result.arr[ 7] = 0.0f;
	
	result.arr[ 8] = 0.0f;
	result.arr[ 9] = 0.0f;
	result.arr[10] = 2.0f / (far - near);
	result.arr[11] = 0.0f;
	
	result.arr[12] = -(right + left) / (right - left);
	result.arr[13] = -(top + bottom) / (bottom - top);
	result.arr[14] = -(far + near) / (far - near);
	result.arr[15] = 1.0f;
	
	return result;
}

//Returns a look-at matrix which tells a vector how to look at a specific point
DESHI_MATH_FUNC inline mat4
mat4_lookat_matrix(vec3 position, vec3 target){
	Assert(position != target);
	vec3 forward = vec3_normalize(vec3_sub(target, position));
	vec3 right = (vec3_nequal(forward, vec3_UP()) && vec3_nequal(forward, vec3_DOWN())) ? vec3_normalize(vec3_cross(vec3_UP(), forward)) : vec3_RIGHT();
	vec3 up = vec3_cross(forward, right);
	return Mat4(right.x,    right.y,    right.z,    0,
				up.x,       up.y,       up.z,       0,
				forward.x,  forward.y,  forward.z,  0,
				position.x, position.y, position.z, 1);
}

//Returns a look-at matrix which tells a vector how to look at a specific point
//  this function is unsafe in that it assumes the forward vector is not parallel to the z-axis
DESHI_MATH_FUNC inline mat4
mat4_lookat_matrix_unsafe(vec3 position, vec3 target){
	Assert(pos != target);
	vec3 forward = vec3_normalize(vec3_sub(target, position));
	vec3 right = vec3_normalize(vec3_cross(vec3_UP(), forward));
	vec3 up = vec3_cross(forward, right);
	return Mat4(right.x,    right.y,    right.z,    0,
				up.x,       up.y,       up.z,       0,
				forward.x,  forward.y,  forward.z,  0,
				position.x, position.y, position.z, 1);
}

//Returns a look-at matrix based on the `position` and `rotation`
DESHI_MATH_FUNC inline mat4
mat4_lookat_matrix_from_rotation_radians(vec3 position, vec3 rotation){
	Assert(pos != target);
	//NOTE forward vector is calculated via a simplification of (vec3_FORWARD() * mat3_rotation_matrix(rotation))
	f32 cX = DESHI_COSF(rotation.x); f32 sX = DESHI_SINF(rotation.x);
	f32 cY = DESHI_COSF(rotation.y); f32 sY = DESHI_SINF(rotation.y);
	f32 cZ = DESHI_COSF(rotation.z); f32 sZ = DESHI_SINF(rotation.z);
	vec3 forward = vec3_normalize(Vec3(cZ*cX*sY + sX*sZ, cX*sY*sZ - cZ*sX, cX*cY));
	vec3 right = (vec3_nequal(forward, vec3_UP()) && vec3_nequal(forward, vec3_DOWN())) ? vec3_normalize(vec3_cross(vec3_UP(), forward)) : vec3_RIGHT();
	vec3 up = vec3_cross(forward, right);
	return Mat4(right.x,    right.y,    right.z,    0,
				up.x,       up.y,       up.z,       0,
				forward.x,  forward.y,  forward.z,  0,
				position.x, position.y, position.z, 1);
}

#define mat4_lookat_matrix_from_rotation_degrees(translation,rotation) mat4_lookat_matrix_from_rotation_radians((translation), Vec3(DESHI_DEGREES_TO_RADIANS_F32((rotation).x), DESHI_DEGREES_TO_RADIANS_F32((rotation).y), DESHI_DEGREES_TO_RADIANS_F32((rotation).z)))

//Returns a look-at matrix based on the `position` and `rotation`
//  this function is unsafe in that it assumes the forward vector is not parallel to the z-axis
DESHI_MATH_FUNC inline mat4
mat4_lookat_matrix_unsafe_from_rotation_radians(vec3 position, vec3 rotation){
	Assert(pos != target);
	//NOTE forward vector is calculated via a simplification of (vec3_FORWARD() * mat3_rotation_matrix(rotation))
	f32 cX = DESHI_COSF(rotation.x); f32 sX = DESHI_SINF(rotation.x);
	f32 cY = DESHI_COSF(rotation.y); f32 sY = DESHI_SINF(rotation.y);
	f32 cZ = DESHI_COSF(rotation.z); f32 sZ = DESHI_SINF(rotation.z);
	vec3 forward = vec3_normalize(Vec3(cZ*cX*sY + sX*sZ, cX*sY*sZ - cZ*sX, cX*cY));
	vec3 right = vec3_normalize(vec3_cross(vec3_UP(), forward));
	vec3 up = vec3_cross(forward, right);
	return Mat4(right.x,    right.y,    right.z,    0,
				up.x,       up.y,       up.z,       0,
				forward.x,  forward.y,  forward.z,  0,
				position.x, position.y, position.z, 1);
}

#define mat4_lookat_matrix_unsafe_from_rotation_degrees(translation,rotation) mat4_lookat_matrix_unsafe_from_rotation_radians((translation), Vec3(DESHI_DEGREES_TO_RADIANS_F32((rotation).x), DESHI_DEGREES_TO_RADIANS_F32((rotation).y), DESHI_DEGREES_TO_RADIANS_F32((rotation).z)))

//Returns a look-at matrix based on the `position` and `rotation` and outputs the `out_forward`, `out_right`, and `out_up` vectors
DESHI_MATH_FUNC inline mat4
mat4_lookat_matrix_from_rotation_radians_output_directions(vec3 position, vec3 rotation, vec3* out_forward, vec3* out_right, vec3* out_up){
	Assert(pos != target);
	//NOTE forward vector is calculated via a simplification of (vec3_FORWARD() * mat3_rotation_matrix(rotation))
	f32 cX = DESHI_COSF(rotation.x); f32 sX = DESHI_SINF(rotation.x);
	f32 cY = DESHI_COSF(rotation.y); f32 sY = DESHI_SINF(rotation.y);
	f32 cZ = DESHI_COSF(rotation.z); f32 sZ = DESHI_SINF(rotation.z);
	vec3 forward = vec3_normalize(Vec3(cZ*cX*sY + sX*sZ, cX*sY*sZ - cZ*sX, cX*cY));
	vec3 right = (vec3_nequal(forward, vec3_UP()) && vec3_nequal(forward, vec3_DOWN())) ? vec3_normalize(vec3_cross(vec3_UP(), forward)) : vec3_RIGHT();
	vec3 up = vec3_cross(forward, right);
	*out_forward = forward;
	*out_right = right;
	*out_up = up;
	mat4 result = Mat4(right.x,    right.y,    right.z,    0,
					   up.x,       up.y,       up.z,       0,
					   forward.x,  forward.y,  forward.z,  0,
					   position.x, position.y, position.z, 1);
}

#define mat4_lookat_matrix_from_rotation_degrees_output_directions(translation,rotation) mat4_lookat_matrix_from_rotation_radians_output_directions((translation), Vec3(DESHI_DEGREES_TO_RADIANS_F32((rotation).x), DESHI_DEGREES_TO_RADIANS_F32((rotation).y), DESHI_DEGREES_TO_RADIANS_F32((rotation).z)))

//Returns a look-at matrix based on the `position` and `rotation` and outputs the `out_forward`, `out_right`, and `out_up` vectors
//  this function is unsafe in that it assumes the forward vector is not parallel to the z-axis
DESHI_MATH_FUNC inline mat4
mat4_lookat_matrix_unsafe_from_rotation_radians_output_directions(vec3 position, vec3 rotation, vec3* out_forward, vec3* out_right, vec3* out_up){
	Assert(pos != target);
	//NOTE forward vector is calculated via a simplification of (vec3_FORWARD() * mat3_rotation_matrix(rotation))
	f32 cX = DESHI_COSF(rotation.x); f32 sX = DESHI_SINF(rotation.x);
	f32 cY = DESHI_COSF(rotation.y); f32 sY = DESHI_SINF(rotation.y);
	f32 cZ = DESHI_COSF(rotation.z); f32 sZ = DESHI_SINF(rotation.z);
	vec3 forward = vec3_normalize(Vec3(cZ*cX*sY + sX*sZ, cX*sY*sZ - cZ*sX, cX*cY));
	vec3 right = vec3_normalize(vec3_cross(vec3_UP(), forward));
	vec3 up = vec3_cross(forward, right);
	*out_forward = forward;
	*out_right = right;
	*out_up = up;
	return Mat4(right.x,    right.y,    right.z,    0,
				up.x,       up.y,       up.z,       0,
				forward.x,  forward.y,  forward.z,  0,
				position.x, position.y, position.z, 1);
}

#define mat4_lookat_matrix_unsafe_from_rotation_degrees_output_directions(translation,rotation) mat4_lookat_matrix_unsafe_from_rotation_radians_output_directions((translation), Vec3(DESHI_DEGREES_TO_RADIANS_F32((rotation).x), DESHI_DEGREES_TO_RADIANS_F32((rotation).y), DESHI_DEGREES_TO_RADIANS_F32((rotation).z)))

DESHI_MATH_FUNC inline vec3
vec3_projection_multiply(vec3 lhs, mat4 rhs){
	vec4 result = vec4_mul_mat4(Vec4(lhs.x, lhs.y, lhs.z, 1.0f), rhs); Assert(result.w != 0.0f);
	result.x /= result.w;
	result.y /= result.w;
	result.z /= result.w;
	return Vec3(result.x, result.y, result.z);
}

DESHI_MATH_FUNC inline vec4
vec4_projection_multiply(vec4 lhs, mat4 rhs){
	vec4 result = vec4_mul_mat4(lhs, rhs); Assert(result.w != 0.0f);
	result.x /= result.w;
	result.y /= result.w;
	result.z /= result.w;
	return result;
}

DESHI_MATH_FUNC inline vec3
map_world_to_camera(vec3 point, mat4 view_matrix){
	return vec3_projection_multiply(point, view_matrix);
}

DESHI_MATH_FUNC inline vec3
map_camera_to_world(vec3 point, mat4 view_matrix){
	//TODO(delle) can we use the fast matrix inverse functions here?
	return vec3_projection_multiply(point, mat4_inverse(view_matrix));
}

DESHI_MATH_FUNC inline vec2
map_camera_to_screen(vec3 point, vec2 screen_dimensions, mat4 projection_matrix){
	vec3 result = vec3_projection_multiply(point, projection_matrix);
	result.x = (result.x + 1.0f) * (0.5f * screen_dimensions.x);
	result.y = (result.y + 1.0f) * (0.5f * screen_dimensions.y);
	return Vec2(result.x, result.y);
}

DESHI_MATH_FUNC inline vec2
map_world_to_screen(vec3 point, vec2 screen_dimensions, mat4 projection_matrix, mat4 view_matrix){
	return map_camera_to_screen(map_world_to_camera(point, view_matrix), screen_dimensions, projection_matrix);
}

static vec3 ScreenToWorld(vec2 pos, const mat4& ProjMat, const mat4& view, vec2 screenDimensions){
	vec4 out{
		2.0f*(pos.x / screenDimensions.x) - 1.0f,
		2.0f*(pos.y / screenDimensions.y) - 1.0f,
		-1.0f,
		1.0f
	};
	out = Math::ProjMult(out, ProjMat.Inverse());
	out = Math::ProjMult(out, view.Inverse());
	return out.toVec3();
}

DESHI_MATH_FUNC inline vec3
map_screen_to_world(vec2 point, vec2 screen_dimensions, mat4 projection_matrix, mat4 view_matrix){
	//TODO(delle) can we use the fast matrix inverse functions here?
	vec4 result;
	result.x = (2.0f * (point.x / screen_dimensions.x)) - 1.0f;
	result.y = (2.0f * (point.y / screen_dimensions.y)) - 1.0f;
	result.z = -1.0f;
	result.w = 1.0f;
	result = vec4_projection_multiply(result, mat4_inverse(projection_matrix));
	result = vec4_projection_multiply(result, mat4_inverse(view_matrix));
	return Vec3(result.x, result.y, result.z);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @cpp_only
#ifdef __cplusplus


//returns the average of the last `width` values
#define RUNNING_AVGF(width,value) ([&]{ \
	persist f32 floats[width] = {0};    \
	persist f32 average = 0;            \
	persist f32 sum = 0;                \
	persist u32 offset = 0;             \
	if(offset == width) offset = 0;     \
	sum -= floats[offset];              \
	floats[offset] = value;             \
	sum += floats[offset];              \
	average = sum / width;              \
	offset += 1;                        \
	return average;                     \
}())

template<typename T> global inline T
lerp(T lhs, T rhs, f32 t){
	return (lhs * (1.0f - t)) + (rhs * t);
}


#endif //#ifdef __cplusplus
//~////////////////////////////////////////////////////////////////////////////////////////////////
// @other


DESHI_MATH_FUNC s32
order_of_magnitude(f32 a){DPZoneScoped;
	if(a == 0.0f) return 0;
	if(DESHI_FLOORF(DESHI_ABSF(a)) == 1) return 0;
	if(DESHI_CEILF(DESHI_ABSF(a)) == 1) return -1;
	
	if(a < 0.0f){
		a = -a;
	}
	
	s32 order = 0;
	if(a > 1.0f){
		for(;;){
			f32 temp = a / 10.0f;
			if(DESHI_CEILF(temp) != 1.0f){
				break;
			}
			a = temp;
			order += 1;
		}
		return order;
	}else{
		for(;;){
			f32 temp = a * 10.0f;
			if(DESHI_FLOORF(temp) != 1.0f){
				break;
			}
			a = temp;
			order -= 1;
		}
		return order - 1;
	}
}

//oscillates between a given `upper` and `lower` value based on a given `t` value
DESHI_MATH_FUNC f32
bounded_oscillation(f32 lower, f32 upper, float t){DPZoneScoped;
	Assert(upper > lower);
	return (((upper - lower) * DESHI_COSF(t)) + (upper + lower)) / 2.0f;
}


#ifdef __cplusplus
}
#endif //#ifdef __cplusplus
#endif //#ifndef DESHI_MATH_H