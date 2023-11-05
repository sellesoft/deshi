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
@...

Ref:
https://github.com/HandmadeMath/HandmadeMath
https://github.com/vectorclass/version2 (Agner Fog)

TODOs:
- API documentation (types, funcs, macros)
- maybe remove division by zero prevention?
- maybe remove dependence on kigu?
*/
#ifndef DESHI_MATH_H
#define DESHI_MATH_H


#include "kigu/common.h"
#include "kigu/profiling.h"
#include <math.h>

#if !defined(DESHI_DISABLE_SSE)
#  if defined(_MSC_VER)
/* MSVC supports SSE in amd64 mode or _M_IX86_FP >= 1 (2 means SSE2) */
#    if defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#      define DESHI_USE_SSE 1
#      include <xmmintrin.h>
#      include <pmmintrin.h>
#      include <smmintrin.h>
#      include <emmintrin.h>
#    endif //#if defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#  else //#if defined(_MSC_VER)
/* non-MSVC usually #define __SSE__ if it's supported */
#    if defined(__SSE__)
#      define DESHI_USE_SSE 1
#      include <xmmintrin.h>
#      include <pmmintrin.h>
#      include <smmintrin.h>
#      include <emmintrin.h>
#    endif  //#if defined(__SSE__)
#  endif //#else //#if defined(_MSC_VER)
#endif //#else //#if !defined(DESHI_DISABLE_SSE)


#ifdef __cplusplus
#  define EXTERN_C extern "C"
#else //#ifdef __cplusplus
#  define EXTERN_C
#endif //#else //#ifdef __cplusplus


struct vec2;
struct vec2i;
struct vec3;
struct vec3i;
struct vec4;
struct vec4i;
struct mat3;
struct mat4;


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @simd
#if DESHI_USE_SSE


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

EXTERN_C inline b32
m128_equal_4f32(__m128 lhs, __m128 rhs){DPZoneScoped;
	__m128 temp0 = _mm_sub_ps(lhs, rhs);
	temp0 = m128_abs(temp0);
	temp0 = _mm_cmpgt_ps(temp0, _mm_set1_ps(M_EPSILON));
	return !(_mm_movemask_ps(temp0));
}

EXTERN_C inline b32
m128_equal_4s32(__m128i lhs, __m128i rhs){DPZoneScoped;
	return !(_mm_movemask_epi8(_mm_cmpeq_epi32(lhs, rhs)));
}

EXTERN_C inline __m128
m128_linear_combine(__m128 vec, __m128 mat_row0, __m128 mat_row1, __m128 mat_row2, __m128 mat_row3){DPZoneScoped;
	__m128 result =                m128_mul_4f32(_mm_shuffle_ps(vec, vec, _MM_SHUFFLE(0,0,0,0)), mat_row0);
	result = m128_add_4f32(result, m128_mul_4f32(_mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1,1,1,1)), mat_row1));
	result = m128_add_4f32(result, m128_mul_4f32(_mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2,2,2,2)), mat_row2));
	result = m128_add_4f32(result, m128_mul_4f32(_mm_shuffle_ps(vec, vec, _MM_SHUFFLE(3,3,3,3)), mat_row3));
	return result;
}


#endif //#else //#if DESHI_USE_SSE
//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec2


EXTERN_C typedef struct vec2{
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
	f32   radians_between(const vec2& rhs)const;
	vec2  floor()const;
	vec2  ceil()const;
	vec2  round()const;
	vec2  round_to(s32 place)const;
	vec2  min(const vec2& rhs)const;
	vec2  max(const vec2& rhs)const;
	vec2  clamp(const vec2& min, const vec2& max)const;
	vec2  clamp_min(const vec2& min)const;
	vec2  clamp_max(const vec2& max)const;
	vec2  clamp_mag(f32 min, f32 max)const;
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

EXTERN_C inline vec2
Vec2(f32 x, f32 y){
	return vec2{x, y};
}

EXTERN_C inline vec2 vec2_ZERO() { return vec2{ 0, 0}; }
EXTERN_C inline vec2 vec2_ONE()  { return vec2{ 1, 1}; }
EXTERN_C inline vec2 vec2_UP()   { return vec2{ 0, 1}; }
EXTERN_C inline vec2 vec2_DOWN() { return vec2{ 0,-1}; }
EXTERN_C inline vec2 vec2_LEFT() { return vec2{-1, 0}; }
EXTERN_C inline vec2 vec2_RIGHT(){ return vec2{ 1, 0}; }
EXTERN_C inline vec2 vec2_UNITX(){ return vec2{ 1, 0}; }
EXTERN_C inline vec2 vec2_UNITY(){ return vec2{ 0, 1}; }

EXTERN_C inline f32
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline b32
vec2_equal(vec2 lhs, vec2 rhs){DPZoneScoped;
	return (fabs(lhs.x - rhs.x) < M_EPSILON)
		&& (fabs(lhs.y - rhs.y) < M_EPSILON);
}

#ifdef __cplusplus
inline b32 vec2::
operator==(const vec2& rhs)const{DPZoneScoped;
	return (fabs(this->x - rhs.x) < M_EPSILON)
		&& (fabs(this->y - rhs.y) < M_EPSILON);
}
#endif //#ifdef __cplusplus

EXTERN_C inline b32
vec2_nequal(vec2 lhs, vec2 rhs){DPZoneScoped;
	return (fabs(lhs.x - rhs.x) > M_EPSILON)
		|| (fabs(lhs.y - rhs.y) > M_EPSILON);
}

#ifdef __cplusplus
inline b32 vec2::
operator!=(const vec2& rhs)const{DPZoneScoped;
	return (fabs(this->x - rhs.x) > M_EPSILON)
		|| (fabs(this->y - rhs.y) > M_EPSILON);
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec2
vec2_abs(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = fabs(lhs.x);
	v.y = fabs(lhs.y);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
abs()const{DPZoneScoped;
	vec2 v;
	v.x = fabs(this->x);
	v.y = fabs(this->y);
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec2_dot(vec2 lhs, vec2 rhs){DPZoneScoped;
	return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}

#ifdef __cplusplus
inline f32 vec2::
dot(const vec2& rhs)const{DPZoneScoped;
	return (this->x * rhs.x) + (this->y * rhs.y);
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec2
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

EXTERN_C inline f32
vec2_mag(vec2 lhs){DPZoneScoped;
	return sqrtf((lhs.x * lhs.x) + (lhs.y * lhs.y));
}

#ifdef __cplusplus
inline f32 vec2::
mag()const{DPZoneScoped;
	return sqrtf((this->x * this->x) + (this->y * this->y));
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32 
vec2_mag_sq(vec2 lhs){DPZoneScoped;
	return (lhs.x * lhs.x) + (lhs.y * lhs.y);
}

#ifdef __cplusplus
inline f32 vec2::
mag_sq()const{DPZoneScoped;
	return (this->x * this->x) + (this->y * this->y);
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec2
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

EXTERN_C inline f32
vec2_distance(vec2 lhs, vec2 rhs){DPZoneScoped;
	return vec2_mag(vec2_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec2::
distance(const vec2& rhs)const{DPZoneScoped;
	return (*this - rhs).mag();
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec2_distance_sq(vec2 lhs, vec2 rhs){DPZoneScoped;
	return vec2_mag_sq(vec2_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec2::
distance_sq(const vec2& rhs)const{DPZoneScoped;
	return (*this - rhs).mag_sq();
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
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

EXTERN_C inline vec2
vec2_component(vec2 lhs, vec2 rhs){DPZoneScoped;
	return vec2_mul_f32(vec2_normalize(rhs), vec2_projection(lhs,rhs));
}

#ifdef __cplusplus
inline vec2 vec2::
component(const vec2& rhs)const{DPZoneScoped;
	return rhs.normalize() * this->projection(rhs);
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec2
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

EXTERN_C inline f32
vec2_radians_between(vec2 lhs, vec2 rhs){DPZoneScoped;
	f32 m = vec2_mag(lhs) * vec2_mag(rhs);
	if(m > M_EPSILON){
		return acosf(vec2_dot(rhs) / m);
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec2::
radians_between(const vec2& rhs)const{DPZoneScoped;
	f32 m = this->mag() * rhs.mag();
	if(m > M_EPSILON){
		return acosf(this->dot(rhs) / m);
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec2
vec2_floor(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = floorf(lhs.x);
	v.y = floorf(lhs.y);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
floor()const{DPZoneScoped;
	vec2 v;
	v.x = floorf(this->x);
	v.y = floorf(this->y);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec2
floor(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = floorf(lhs.x);
	v.y = floorf(lhs.y);
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec2
vec2_ceil(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = ceilf(lhs.x);
	v.y = ceilf(lhs.y);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
ceil()const{DPZoneScoped;
	vec2 v;
	v.x = ceilf(this->x);
	v.y = ceilf(this->y);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec2
ceil(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = ceilf(lhs.x);
	v.y = ceilf(lhs.y);
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec2
vec2_round(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = roundf(lhs.x);
	v.y = roundf(lhs.y);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
round()const{DPZoneScoped;
	vec2 v;
	v.x = roundf(this->x);
	v.y = roundf(this->y);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec2
round(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = roundf(lhs.x);
	v.y = roundf(lhs.y);
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec2
vec2_round_to(vec2 lhs, s32 place){DPZoneScoped;
	vec2 v;
	v.x = floorf(lhs.x * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.y = floorf(lhs.y * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	return v;
}

#ifdef __cplusplus
inline vec2 vec2::
round_to(s32 place)const{DPZoneScoped;
	vec2 v;
	v.x = floorf(this->x * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.y = floorf(this->y * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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

EXTERN_C inline vec2
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


EXTERN_C typedef struct vec2i{
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

EXTERN_C inline vec2i
Vec2i(s32 x, s32 y){
	return vec2i{x, y};
}

EXTERN_C inline vec2i vec2i_ZERO() { return vec2i{ 0, 0}; }
EXTERN_C inline vec2i vec2i_ONE()  { return vec2i{ 1, 1}; }
EXTERN_C inline vec2i vec2i_UP()   { return vec2i{ 0, 1}; }
EXTERN_C inline vec2i vec2i_DOWN() { return vec2i{ 0,-1}; }
EXTERN_C inline vec2i vec2i_LEFT() { return vec2i{-1, 0}; }
EXTERN_C inline vec2i vec2i_RIGHT(){ return vec2i{ 1, 0}; }
EXTERN_C inline vec2i vec2i_UNITX(){ return vec2i{ 1, 0}; }
EXTERN_C inline vec2i vec2i_UNITY(){ return vec2i{ 0, 1}; }

EXTERN_C inline s32
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline b32
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

EXTERN_C inline b32
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

EXTERN_C inline vec2i
vec2i_abs(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = abs(lhs.x);
	v.y = abs(lhs.y);
	return v;
}

#ifdef __cplusplus
inline vec2i vec2i::
abs()const{DPZoneScoped;
	vec2i v;
	v.x = abs(this->x);
	v.y = abs(this->y);
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec2i_dot(vec2i lhs, vec2i rhs){DPZoneScoped;
	return (f32)((lhs.x * rhs.x) + (lhs.y * rhs.y));
}

#ifdef __cplusplus
inline f32 vec2i::
dot(const vec2i& rhs)const{DPZoneScoped;
	return (f32)((this->x * rhs.x) + (this->y * rhs.y));
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec2i
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

EXTERN_C inline f32
vec2i_mag(vec2i lhs){DPZoneScoped;
	return sqrtf((f32)((lhs.x * lhs.x) + (lhs.y * lhs.y)));
}

#ifdef __cplusplus
inline f32 vec2i::
mag()const{DPZoneScoped;
	return sqrtf((f32)((this->x * this->x) + (this->y * this->y)));
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec2i_mag_sq(vec2i lhs){DPZoneScoped;
	return (f32)((lhs.x * lhs.x) + (lhs.y * lhs.y));
}

#ifdef __cplusplus
inline f32 vec2i::
mag_sq()const{DPZoneScoped;
	return (f32)((this->x * this->x) + (this->y * this->y));
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec2i
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

EXTERN_C inline f32
vec2i_distance(vec2i lhs, vec2i rhs){DPZoneScoped;
	return vec2i_mag(vec2i_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec2i::
distance(const vec2i& rhs)const{DPZoneScoped;
	return (*this - rhs).mag();
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec2i_distance_sq(vec2i lhs, vec2i rhs){DPZoneScoped;
	return vec2i_mag_sq(vec2i_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec2i::
distance_sq(const vec2i& rhs)const{DPZoneScoped;
	return (*this - rhs).mag_sq();
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
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

EXTERN_C inline vec2i
vec2i_component(vec2i lhs, vec2i rhs){DPZoneScoped;
	return vec2i_mul_f32(vec2i_normalize(rhs), vec2i_projection(lhs,rhs));
}

#ifdef __cplusplus
inline vec2i vec2i::
component(const vec2i& rhs)const{DPZoneScoped;
	return rhs.normalize() * this->projection(rhs);
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec2i
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

EXTERN_C inline f32
vec2i_radians_between(vec2i lhs, vec2i rhs){DPZoneScoped;
	f32 m = vec2i_mag(lhs) * vec2i_mag(rhs);
	if(m > M_EPSILON){
		return acosf(vec2i_dot(lhs,rhs) / m);
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec2i::
radians_between(const vec2i& rhs)const{DPZoneScoped;
	f32 m = this->mag() * rhs.mag();
	if(m > M_EPSILON){
		return acosf(this->dot(rhs) / m);
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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

EXTERN_C inline vec2i
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


EXTERN_C typedef struct vec3{
	union{
		f32 arr[3];
		struct{ f32 x, y, z; };
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
	vec3  round_to(s32 place)const;
	vec3  min(const vec3& rhs)const;
	vec3  max(const vec3& rhs)const;
	vec3  clamp(const vec3& min, const vec3& max)const;
	vec3  clamp_min(const vec3& min)const;
	vec3  clamp_max(const vec3& max)const;
	vec3  clamp_mag(f32 min, f32 max)const;
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

EXTERN_C inline vec3
Vec3(f32 x, f32 y, f32 z){
	return vec3{x, y, z};
}

EXTERN_C inline vec3 vec3_ZERO()   { return vec3{ 0, 0, 0}; }
EXTERN_C inline vec3 vec3_ONE()    { return vec3{ 1, 1, 1}; }
EXTERN_C inline vec3 vec3_LEFT()   { return vec3{-1, 0, 0}; }
EXTERN_C inline vec3 vec3_RIGHT()  { return vec3{ 1, 0, 0}; }
EXTERN_C inline vec3 vec3_DOWN()   { return vec3{ 0,-1, 0}; }
EXTERN_C inline vec3 vec3_UP()     { return vec3{ 0, 1, 0}; }
EXTERN_C inline vec3 vec3_BACK()   { return vec3{ 0, 0,-1}; }
EXTERN_C inline vec3 vec3_FORWARD(){ return vec3{ 0, 0, 1}; }
EXTERN_C inline vec3 vec3_UNITX()  { return vec3{ 1, 0, 0}; }
EXTERN_C inline vec3 vec3_UNITY()  { return vec3{ 0, 1, 0}; }
EXTERN_C inline vec3 vec3_UNITZ()  { return vec3{ 0, 0, 1}; }

EXTERN_C inline f32
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline b32
vec3_equal(vec3 lhs, vec3 rhs){DPZoneScoped;
	return (fabs(lhs.x - rhs.x) < M_EPSILON)
		&& (fabs(lhs.y - rhs.y) < M_EPSILON)
		&& (fabs(lhs.z - rhs.z) < M_EPSILON);
}

#ifdef __cplusplus
inline b32 vec3::
operator==(const vec3& rhs)const{DPZoneScoped;
	return (fabs(this->x - rhs.x) < M_EPSILON)
		&& (fabs(this->y - rhs.y) < M_EPSILON)
		&& (fabs(this->z - rhs.z) < M_EPSILON);
}
#endif //#ifdef __cplusplus

EXTERN_C inline b32
vec3_nequal(vec3 lhs, vec3 rhs){DPZoneScoped;
	return (fabs(lhs.x - rhs.x) > M_EPSILON)
		|| (fabs(lhs.y - rhs.y) > M_EPSILON)
		|| (fabs(lhs.z - rhs.z) > M_EPSILON);
}

#ifdef __cplusplus
inline b32 vec3::
operator!=(const vec3& rhs)const{DPZoneScoped;
	return (fabs(this->x - rhs.x) > M_EPSILON)
		|| (fabs(this->y - rhs.y) > M_EPSILON)
		|| (fabs(this->z - rhs.z) > M_EPSILON);
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec3
vec3_abs(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = fabs(lhs.x);
	v.y = fabs(lhs.y);
	v.z = fabs(lhs.z);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
abs()const{DPZoneScoped;
	vec3 v;
	v.x = fabs(this->x);
	v.y = fabs(this->y);
	v.z = fabs(this->z);
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec3_dot(vec3 lhs, vec3 rhs){DPZoneScoped;
	return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

#ifdef __cplusplus
inline f32 vec3::
dot(const vec3& rhs)const{DPZoneScoped;
	return (this->x * rhs.x) + (this->y * rhs.y) + (this->z * rhs.z);
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec3
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

EXTERN_C inline f32
vec3_mag(vec3 lhs){DPZoneScoped;
	return sqrtf((lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z));
}

#ifdef __cplusplus
inline f32 vec3::
mag()const{DPZoneScoped;
	return sqrtf((this->x * this->x) + (this->y * this->y) + (this->z * this->z));
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32 
vec3_mag_sq(vec3 lhs){DPZoneScoped;
	return (lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z);
}

#ifdef __cplusplus
inline f32 vec3::
mag_sq()const{DPZoneScoped;
	return (this->x * this->x) + (this->y * this->y) + (this->z * this->z);
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec3
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

EXTERN_C inline f32
vec3_distance(vec3 lhs, vec3 rhs){DPZoneScoped;
	return vec3_mag(vec3_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec3::
distance(const vec3& rhs)const{DPZoneScoped;
	return (*this - rhs).mag();
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec3_distance_sq(vec3 lhs, vec3 rhs){DPZoneScoped;
	return vec3_mag_sq(vec3_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec3::
distance_sq(const vec3& rhs)const{DPZoneScoped;
	return (*this - rhs).mag_sq();
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
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

EXTERN_C inline vec3
vec3_component(vec3 lhs, vec3 rhs){DPZoneScoped;
	return vec3_mul_f32(vec3_normalize(rhs), vec3_projection(lhs,rhs));
}

#ifdef __cplusplus
inline vec3 vec3::
component(const vec3& rhs)const{DPZoneScoped;
	return rhs.normalize() * this->projection(rhs);
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec3
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

EXTERN_C inline f32
vec3_radians_between(vec3 lhs, vec3 rhs){DPZoneScoped;
	f32 m = vec3_mag(lhs) * vec3_mag(rhs);
	if(m > M_EPSILON){
		return acosf(vec3_dot(rhs) / m);
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec3::
radians_between(const vec3& rhs)const{DPZoneScoped;
	f32 m = this->mag() * rhs.mag();
	if(m > M_EPSILON){
		return acosf(this->dot(rhs) / m);
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec3
vec3_floor(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = floorf(lhs.x);
	v.y = floorf(lhs.y);
	v.z = floorf(lhs.z);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
floor()const{DPZoneScoped;
	vec3 v;
	v.x = floorf(this->x);
	v.y = floorf(this->y);
	v.z = floorf(this->z);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec3
floor(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = floorf(lhs.x);
	v.y = floorf(lhs.y);
	v.z = floorf(lhs.z);
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec3
vec3_ceil(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = ceilf(lhs.x);
	v.y = ceilf(lhs.y);
	v.z = ceilf(lhs.z);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
ceil()const{DPZoneScoped;
	vec3 v;
	v.x = ceilf(this->x);
	v.y = ceilf(this->y);
	v.z = ceilf(this->z);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec3
ceil(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = ceilf(lhs.x);
	v.y = ceilf(lhs.y);
	v.z = ceilf(lhs.z);
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec3
vec3_round(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = roundf(lhs.x);
	v.y = roundf(lhs.y);
	v.z = roundf(lhs.z);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
round()const{DPZoneScoped;
	vec3 v;
	v.x = roundf(this->x);
	v.y = roundf(this->y);
	v.z = roundf(this->z);
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec3
round(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = roundf(lhs.x);
	v.y = roundf(lhs.y);
	v.z = roundf(lhs.z);
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec3
vec3_round_to(vec3 lhs, s32 place){DPZoneScoped;
	vec3 v;
	v.x = floorf(lhs.x * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.y = floorf(lhs.y * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.z = floorf(lhs.z * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	return v;
}

#ifdef __cplusplus
inline vec3 vec3::
round_to(s32 place)const{DPZoneScoped;
	vec3 v;
	v.x = floorf(this->x * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.y = floorf(this->y * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.z = floorf(this->z * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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

EXTERN_C inline vec3
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


EXTERN_C typedef struct vec3i{
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

EXTERN_C inline vec3i
Vec3i(s32 x, s32 y, s32 z){
	return vec3i{x, y, z};
}

EXTERN_C inline vec3i vec3i_ZERO()   { return vec3i{ 0, 0, 0}; }
EXTERN_C inline vec3i vec3i_ONE()    { return vec3i{ 1, 1, 1}; }
EXTERN_C inline vec3i vec3i_LEFT()   { return vec3i{-1, 0, 0}; }
EXTERN_C inline vec3i vec3i_RIGHT()  { return vec3i{ 1, 0, 0}; }
EXTERN_C inline vec3i vec3i_DOWN()   { return vec3i{ 0,-1, 0}; }
EXTERN_C inline vec3i vec3i_UP()     { return vec3i{ 0, 1, 0}; }
EXTERN_C inline vec3i vec3i_BACK()   { return vec3i{ 0, 0,-1}; }
EXTERN_C inline vec3i vec3i_FORWARD(){ return vec3i{ 0, 0, 1}; }
EXTERN_C inline vec3i vec3i_UNITX()  { return vec3i{ 1, 0, 0}; }
EXTERN_C inline vec3i vec3i_UNITY()  { return vec3i{ 0, 1, 0}; }
EXTERN_C inline vec3i vec3i_UNITZ()  { return vec3i{ 0, 0, 1}; }

EXTERN_C inline s32
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline b32
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

EXTERN_C inline b32
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

EXTERN_C inline vec3i
vec3i_abs(vec3i lhs){DPZoneScoped;
	vec3i v;
	v.x = abs(lhs.x);
	v.y = abs(lhs.y);
	v.z = abs(lhs.z);
	return v;
}

#ifdef __cplusplus
inline vec3i vec3i::
abs()const{DPZoneScoped;
	vec3i v;
	v.x = abs(this->x);
	v.y = abs(this->y);
	v.z = abs(this->z);
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec3i_dot(vec3i lhs, vec3i rhs){DPZoneScoped;
	return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

#ifdef __cplusplus
inline f32 vec3i::
dot(const vec3i& rhs)const{DPZoneScoped;
	return (this->x * rhs.x) + (this->y * rhs.y) + (this->z * rhs.z);
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec3i
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

EXTERN_C inline f32
vec3i_mag(vec3i lhs){DPZoneScoped;
	return sqrtf((lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z));
}

#ifdef __cplusplus
inline f32 vec3i::
mag()const{DPZoneScoped;
	return sqrtf((this->x * this->x) + (this->y * this->y) + (this->z * this->z));
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32 
vec3i_mag_sq(vec3i lhs){DPZoneScoped;
	return (lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z);
}

#ifdef __cplusplus
inline f32 vec3i::
mag_sq()const{DPZoneScoped;
	return (this->x * this->x) + (this->y * this->y) + (this->z * this->z);
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec3i
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

EXTERN_C inline f32
vec3i_distance(vec3i lhs, vec3i rhs){DPZoneScoped;
	return vec3i_mag(vec3i_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec3i::
distance(const vec3i& rhs)const{DPZoneScoped;
	return (*this - rhs).mag();
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec3i_distance_sq(vec3i lhs, vec3i rhs){DPZoneScoped;
	return vec3i_mag_sq(vec3i_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec3i::
distance_sq(const vec3i& rhs)const{DPZoneScoped;
	return (*this - rhs).mag_sq();
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
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

EXTERN_C inline vec3i
vec3i_component(vec3i lhs, vec3i rhs){DPZoneScoped;
	return vec3i_mul_f32(vec3i_normalize(rhs), vec3i_projection(lhs,rhs));
}

#ifdef __cplusplus
inline vec3i vec3i::
component(const vec3i& rhs)const{DPZoneScoped;
	return rhs.normalize() * this->projection(rhs);
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec3i
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

EXTERN_C inline f32
vec3i_radians_between(vec3i lhs, vec3i rhs){DPZoneScoped;
	f32 m = vec3i_mag(lhs) * vec3i_mag(rhs);
	if(m > M_EPSILON){
		return acosf(vec3i_dot(rhs) / m);
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec3i::
radians_between(const vec3i& rhs)const{DPZoneScoped;
	f32 m = this->mag() * rhs.mag();
	if(m > M_EPSILON){
		return acosf(this->dot(rhs) / m);
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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

EXTERN_C inline vec3i
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


EXTERN_C typedef struct vec4{
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
#ifdef DESHI_USE_SSE
		__m128 sse;
#endif //#ifdef DESHI_USE_SSE
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
	vec4  round_to(s32 place)const;
	vec4  min(const vec4& rhs)const;
	vec4  max(const vec4& rhs)const;
	vec4  clamp(const vec4& min, const vec4& max)const;
	vec4  clamp_min(const vec4& min)const;
	vec4  clamp_max(const vec4& max)const;
	vec4  clamp_mag(f32 min, f32 max)const;
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

EXTERN_C inline vec4
Vec4(f32 x, f32 y, f32 z, f32 w){
	return vec4{x, y, z, w};
}

EXTERN_C inline vec4 vec4_ZERO() { return vec4{0,0,0,0}; }
EXTERN_C inline vec4 vec4_ONE()  { return vec4{1,1,1,1}; }
EXTERN_C inline vec4 vec4_UNITX(){ return vec4{1,0,0,0}; }
EXTERN_C inline vec4 vec4_UNITY(){ return vec4{0,1,0,0}; }
EXTERN_C inline vec4 vec4_UNITZ(){ return vec4{0,0,1,0}; }
EXTERN_C inline vec4 vec4_UNITW(){ return vec4{0,0,0,1}; }

EXTERN_C inline f32
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

EXTERN_C inline vec4
vec4_add(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_add_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = lhs.x + rhs.x;
	v.y = lhs.y + rhs.y;
	v.z = lhs.z + rhs.z;
	v.w = lhs.w + rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
operator+ (const vec4& rhs)const{DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_add_4f32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = this->x + rhs.x;
	v.y = this->y + rhs.y;
	v.z = this->z + rhs.z;
	v.w = this->w + rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4::
operator+=(const vec4& rhs){DPZoneScoped;
#if DESHI_USE_SSE
	this->sse = m128_add_4f32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	this->x += rhs.x;
	this->y += rhs.y;
	this->z += rhs.z;
	this->w += rhs.w;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_sub(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_sub_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = lhs.x - rhs.x;
	v.y = lhs.y - rhs.y;
	v.z = lhs.z - rhs.z;
	v.w = lhs.w - rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
operator- (const vec4& rhs)const{DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_sub_4f32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = this->x - rhs.x;
	v.y = this->y - rhs.y;
	v.z = this->z - rhs.z;
	v.w = this->w - rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4::
operator-=(const vec4& rhs){DPZoneScoped;
#if DESHI_USE_SSE
	this->sse = m128_sub_4f32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	this->x -= rhs.x;
	this->y -= rhs.y;
	this->z -= rhs.z;
	this->w -= rhs.w;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_mul(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_mul_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = lhs.x * rhs.x;
	v.y = lhs.y * rhs.y;
	v.z = lhs.z * rhs.z;
	v.w = lhs.w * rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
operator* (const vec4& rhs)const{DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_mul_4f32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = this->x * rhs.x;
	v.y = this->y * rhs.y;
	v.z = this->z * rhs.z;
	v.w = this->w * rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4::
operator*=(const vec4& rhs){DPZoneScoped;
#if DESHI_USE_SSE
	this->sse = m128_mul_4f32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	this->x *= rhs.x;
	this->y *= rhs.y;
	this->z *= rhs.z;
	this->w *= rhs.w;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_mul_f32(vec4 lhs, f32 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_mul_4f32(lhs.sse, m128_fill_4f32(rhs));
#else //#if DESHI_USE_SSE
	v.x = lhs.x * rhs;
	v.y = lhs.y * rhs;
	v.z = lhs.z * rhs;
	v.w = lhs.w * rhs;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
operator* (const f32& rhs)const{DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_mul_4f32(this->sse, m128_fill_4f32(rhs));
#else //#if DESHI_USE_SSE
	v.x = this->x * rhs;
	v.y = this->y * rhs;
	v.z = this->z * rhs;
	v.w = this->w * rhs;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4::
operator*=(const f32& rhs){DPZoneScoped;
#if DESHI_USE_SSE
	sse = m128_mul_4f32(this->sse, m128_fill_4f32(rhs));
#else //#if DESHI_USE_SSE
	this->x *= rhs;
	this->y *= rhs;
	this->z *= rhs;
	this->w *= rhs;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec4 vec4::
operator* (s32 lhs, vec4 rhs){
	return rhs * lhs;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_div(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_div_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = lhs.x / rhs.x;
	v.y = lhs.y / rhs.y;
	v.z = lhs.z / rhs.z;
	v.w = lhs.w / rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
operator/ (const vec4& rhs)const{DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_div_4f32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = this->x / rhs.x;
	v.y = this->y / rhs.y;
	v.z = this->z / rhs.z;
	v.w = this->w / rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4::
operator/=(const vec4& rhs){DPZoneScoped;
#if DESHI_USE_SSE
	this->sse = m128_div_4f32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	this->x /= rhs.x;
	this->y /= rhs.y;
	this->z /= rhs.z;
	this->w /= rhs.w;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_div_f32(vec4 lhs, f32 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_div_4f32(lhs.sse, m128_fill_4f32(rhs));
#else //#if DESHI_USE_SSE
	v.x = lhs.x / rhs;
	v.y = lhs.y / rhs;
	v.z = lhs.z / rhs;
	v.w = lhs.w / rhs;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
operator/ (const f32& rhs)const{DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_div_4f32(this->sse, m128_fill_4f32(rhs));
#else //#if DESHI_USE_SSE
	v.x = this->x / rhs;
	v.y = this->y / rhs;
	v.z = this->z / rhs;
	v.w = this->w / rhs;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4::
operator/=(const f32& rhs){DPZoneScoped;
#if DESHI_USE_SSE
	this->sse = m128_div_4f32(this->sse, m128_fill_4f32(rhs));
#else //#if DESHI_USE_SSE
	this->x /= rhs;
	this->y /= rhs;
	this->z /= rhs;
	this->w /= rhs;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_negate(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_negate_4f32(lhs.sse);
#else //#if DESHI_USE_SSE
	v.x = -(lhs.x);
	v.y = -(lhs.y);
	v.z = -(lhs.z);
	v.w = -(lhs.w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
operator- ()const{DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_negate_4f32(this->sse);
#else //#if DESHI_USE_SSE
	v.x = -(this->x);
	v.y = -(this->y);
	v.z = -(this->z);
	v.w = -(this->w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline b32
vec4_equal(vec4 lhs, vec4 rhs){DPZoneScoped;
#if DESHI_USE_SSE
	return m128_equal_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	return fabs(lhs.x - rhs.x) < M_EPSILON 
		&& fabs(lhs.y - rhs.y) < M_EPSILON 
		&& fabs(lhs.z - rhs.z) < M_EPSILON 
		&& fabs(lhs.w - rhs.w) < M_EPSILON;
#endif //#else //#if DESHI_USE_SSE
}

#ifdef __cplusplus
inline bool vec4::
operator==(const vec4& rhs)const{DPZoneScoped;
#if DESHI_USE_SSE
	return m128_equal_4f32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	return fabs(this->x - rhs.x) < M_EPSILON 
		&& fabs(this->y - rhs.y) < M_EPSILON 
		&& fabs(this->z - rhs.z) < M_EPSILON 
		&& fabs(this->w - rhs.w) < M_EPSILON;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline b32
vec4_nequal(vec4 lhs, vec4 rhs){DPZoneScoped;
	#if DESHI_USE_SSE
	return !m128_equal_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	return fabs(lhs.x - rhs.x) > M_EPSILON 
		|| fabs(lhs.y - rhs.y) > M_EPSILON 
		|| fabs(lhs.z - rhs.z) > M_EPSILON 
		|| fabs(lhs.w - rhs.w) > M_EPSILON;
#endif //#else //#if DESHI_USE_SSE
}

#ifdef __cplusplus
inline bool vec4::
operator!=(const vec4& rhs)const{DPZoneScoped;
#if DESHI_USE_SSE
	return !m128_equal_4f32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	return fabs(this->x - rhs.x) > M_EPSILON 
		|| fabs(this->y - rhs.y) > M_EPSILON 
		|| fabs(this->z - rhs.z) > M_EPSILON 
		|| fabs(this->w - rhs.w) > M_EPSILON;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_abs(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_abs_4f32(lhs.sse);
#else //#if DESHI_USE_SSE
	v.x = fabs(lhs.x);
	v.y = fabs(lhs.y);
	v.z = fabs(lhs.z);
	v.w = fabs(lhs.w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
abs()const{DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_abs_4f32(this->sse);
#else //#if DESHI_USE_SSE
	v.x = fabs(this->x);
	v.y = fabs(this->y);
	v.z = fabs(this->z);
	v.w = fabs(this->w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec4_dot(vec4 lhs, vec4 rhs){DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mul_ps(lhs.sse, rhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	result = _mm_cvtss_f32(temp0);
#else //#if DESHI_USE_SSE
	result = (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
#endif //#else //#if DESHI_USE_SSE
	return result;
}

#ifdef __cplusplus
inline f32 vec4::
dot(const vec4& rhs)const{DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mul_ps(this->sse, rhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	result = _mm_cvtss_f32(temp0);
#else //#if DESHI_USE_SSE
	result = (this->x * rhs.x) + (this->y * rhs.y) + (this->z * rhs.z) + (this->w * rhs.w);
#endif //#else //#if DESHI_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec4_mag(vec4 lhs){DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mul_ps(lhs.sse, lhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	temp0 = _mm_sqrt_ss(temp0);
	result = _mm_cvtss_f32(temp0);
#else //#if DESHI_USE_SSE
	result = sqrtf((lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z) + (lhs.w * lhs.w));
#endif //#else //#if DESHI_USE_SSE
	return result;
}

#ifdef __cplusplus
inline f32 vec4::
mag()const{DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mul_ps(this->sse, this->sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	temp0 = _mm_sqrt_ss(temp0);
	result = _mm_cvtss_f32(temp0);
#else //#if DESHI_USE_SSE
	result = sqrtf((this->x * this->x) + (this->y * this->y) + (this->z * this->z) + (this->w * this->w));
#endif //#else //#if DESHI_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec4_mag_sq(vec4 lhs){DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mul_ps(lhs.sse, lhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	result = _mm_cvtss_f32(temp0);
#else //#if DESHI_USE_SSE
	result = (lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z) + (lhs.w * lhs.w);
#endif //#else //#if DESHI_USE_SSE
	return result;
}

#ifdef __cplusplus
inline f32 vec4::
mag_sq()const{DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mul_ps(this->sse, this->sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	result = _mm_cvtss_f32(temp0);
#else //#if DESHI_USE_SSE
	result = (this->x * this->x) + (this->y * this->y) + (this->z * this->z) + (this->w * this->w);
#endif //#else //#if DESHI_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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


EXTERN_C inline f32
vec4_distance(vec4 lhs, vec4 rhs){DPZoneScoped;
	return vec4_mag(vec4_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec4::
distance(const vec4& rhs)const{DPZoneScoped;
	return (*this - rhs).mag();
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec4_distance_sq(vec4 lhs, vec4 rhs){DPZoneScoped;
	return vec4_mag_sq(vec4_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec4::
distance_sq(const vec4& rhs)const{DPZoneScoped;
	return (*this - rhs).mag_sq();
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
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

EXTERN_C inline vec4
vec4_component(vec4 lhs, vec4 rhs){DPZoneScoped;
	return vec4_mul_f32(vec4_normalize(rhs), vec4_projection(lhs,rhs));
}

#ifdef __cplusplus
inline vec4 vec4::
component(const vec4& rhs)const{DPZoneScoped;
	return rhs.normalize() * this->projection(rhs);
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_midpoint(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_div_4f32(m128_add_4f32(lhs.sse, rhs.sse), m128_fill_4f32(2.0f));
#else //#if DESHI_USE_SSE
	v.x = (lhs.x + rhs.x) / 2.0f;
	v.y = (lhs.y + rhs.y) / 2.0f;
	v.z = (lhs.z + rhs.z) / 2.0f;
	v.w = (lhs.w + rhs.w) / 2.0f;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4
midpoint(const vec4& rhs)const{DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_div_4f32(m128_add_4f32(lhs.sse, rhs.sse), m128_fill_4f32(2.0f));
#else //#if DESHI_USE_SSE
	v.x = (lhs.x + rhs.x) / 2.0f;
	v.y = (lhs.y + rhs.y) / 2.0f;
	v.z = (lhs.z + rhs.z) / 2.0f;
	v.w = (lhs.w + rhs.w) / 2.0f;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec4_radians_between(vec4 lhs, vec4 rhs){DPZoneScoped;
	f32 m = vec4_mag(lhs) * vec4_mag(rhs);
	if(m > M_EPSILON){
		return acosf(vec4_dot(rhs) / m);
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec4::
radians_between(const vec4& rhs)const{DPZoneScoped;
	f32 m = this->mag() * rhs.mag();
	if(m > M_EPSILON){
		return acosf(this->dot(rhs) / m);
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_floor(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_floor_4f32(lhs.sse);
#else //#if DESHI_USE_SSE
	v.x = floorf(lhs.x);
	v.y = floorf(lhs.y);
	v.z = floorf(lhs.z);
	v.w = floorf(lhs.w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
floor()const{DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_floor_4f32(this->sse);
#else //#if DESHI_USE_SSE
	v.x = floorf(this->x);
	v.y = floorf(this->y);
	v.z = floorf(this->z);
	v.w = floorf(this->w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec4
floor(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_floor_4f32(lhs.sse);
#else //#if DESHI_USE_SSE
	v.x = floorf(lhs.x);
	v.y = floorf(lhs.y);
	v.z = floorf(lhs.z);
	v.w = floorf(lhs.w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_ceil(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_ceil_4f32(lhs.sse);
#else //#if DESHI_USE_SSE
	v.x = ceilf(lhs.x);
	v.y = ceilf(lhs.y);
	v.z = ceilf(lhs.z);
	v.w = ceilf(lhs.w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
ceil()const{DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_ceil_4f32(this->sse);
#else //#if DESHI_USE_SSE
	v.x = ceilf(this->x);
	v.y = ceilf(this->y);
	v.z = ceilf(this->z);
	v.w = ceilf(this->w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec4
ceil(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_ceil_4f32(lhs.sse);
#else //#if DESHI_USE_SSE
	v.x = ceilf(lhs.x);
	v.y = ceilf(lhs.y);
	v.z = ceilf(lhs.z);
	v.w = ceilf(lhs.w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_round(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_round_4f32(lhs.sse);
#else //#if DESHI_USE_SSE
	v.x = roundf(lhs.x);
	v.y = roundf(lhs.y);
	v.z = roundf(lhs.z);
	v.w = roundf(lhs.w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
round()const{DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_round_4f32(this->sse);
#else //#if DESHI_USE_SSE
	v.x = roundf(this->x);
	v.y = roundf(this->y);
	v.z = roundf(this->z);
	v.w = roundf(this->w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec4
round(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_round_4f32(lhs.sse);
#else //#if DESHI_USE_SSE
	v.x = roundf(lhs.x);
	v.y = roundf(lhs.y);
	v.z = roundf(lhs.z);
	v.w = roundf(lhs.w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_round_to(vec4 lhs, s32 place){DPZoneScoped;
	vec3 v;
	v.x = floorf(lhs.x * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.y = floorf(lhs.y * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.z = floorf(lhs.z * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.w = floorf(lhs.w * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
round_to(s32 place)const{DPZoneScoped;
	vec4 v;
	v.x = floorf(this->x * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.y = floorf(this->y * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.z = floorf(this->z * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	v.w = floorf(this->w * (f32)place * 10.0f + 0.5f) / ((f32)place * 10.0f);
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_min(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_min_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z < rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w < rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
min(const vec4& rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_min_4f32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = (this->x < rhs.x) ? this->x : rhs.x;
	v.y = (this->y < rhs.y) ? this->y : rhs.y;
	v.z = (this->z < rhs.z) ? this->z : rhs.z;
	v.w = (this->w < rhs.w) ? this->w : rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4
min(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_min_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z < rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w < rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_max(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_max_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z > rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w > rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
max(const vec4& rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_max_4f32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = (this->x > rhs.x) ? this->x : rhs.x;
	v.y = (this->y > rhs.y) ? this->y : rhs.y;
	v.z = (this->z > rhs.z) ? this->z : rhs.z;
	v.w = (this->w > rhs.w) ? this->w : rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4
max(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_max_4f32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z > rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w > rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
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

EXTERN_C inline vec4
vec4_clamp_min(vec4 value, vec4 min){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_max_4f32(value.sse, min.sse);
#else //#if DESHI_USE_SSE
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	v.z = (value.z < min.z) ? min.z : value.z;
	v.w = (value.w < min.w) ? min.w : value.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
clamp_min(const vec4& min){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_max_4f32(value.sse, min.sse);
#else //#if DESHI_USE_SSE
	v.x = (this->x < min.x) ? min.x : this->x;
	v.y = (this->y < min.y) ? min.y : this->y;
	v.z = (this->z < min.z) ? min.z : this->z;
	v.w = (this->w < min.w) ? min.w : this->w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4
clamp_min(vec4 value, vec4 min){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_max_4f32(value.sse, min.sse);
#else //#if DESHI_USE_SSE
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	v.z = (value.z < min.z) ? min.z : value.z;
	v.w = (value.w < min.w) ? min.w : value.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
vec4_clamp_max(vec4 value, vec4 max){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_min_4f32(value.sse, min.sse);
#else //#if DESHI_USE_SSE
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	v.z = (value.z > max.z) ? max.z : value.z;
	v.w = (value.w > max.w) ? max.w : value.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4 vec4::
clamp_max(const vec4& maX){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_min_4f32(value.sse, min.sse);
#else //#if DESHI_USE_SSE
	v.x = (this->x > max.x) ? max.x : this->x;
	v.y = (this->y > max.y) ? max.y : this->y;
	v.z = (this->z > max.z) ? max.z : this->z;
	v.w = (this->w > max.w) ? max.w : this->w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4
clamp_max(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = m128_min_4f32(value.sse, min.sse);
#else //#if DESHI_USE_SSE
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	v.z = (value.z > max.z) ? max.z : value.z;
	v.w = (value.w > max.w) ? max.w : value.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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

EXTERN_C inline vec4
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


EXTERN_C typedef struct vec4i{
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
#ifdef DESHI_USE_SSE
		__m128i sse;
#endif //#ifdef DESHI_USE_SSE
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

EXTERN_C inline vec4i
Vec4i(f32 x, f32 y, f32 z, f32 w){
	return vec4i{x, y, z, w};
}

EXTERN_C inline vec4i vec4i_ZERO() { return vec4i{0,0,0,0}; }
EXTERN_C inline vec4i vec4i_ONE()  { return vec4i{1,1,1,1}; }
EXTERN_C inline vec4i vec4i_UNITX(){ return vec4i{1,0,0,0}; }
EXTERN_C inline vec4i vec4i_UNITY(){ return vec4i{0,1,0,0}; }
EXTERN_C inline vec4i vec4i_UNITZ(){ return vec4i{0,0,1,0}; }
EXTERN_C inline vec4i vec4i_UNITW(){ return vec4i{0,0,0,1}; }

EXTERN_C inline s32
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

EXTERN_C inline vec4i
vec4i_add(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_add_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = lhs.x + rhs.x;
	v.y = lhs.y + rhs.y;
	v.z = lhs.z + rhs.z;
	v.w = lhs.w + rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
operator+ (const vec4i& rhs)const{DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_add_4s32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = this->x + rhs.x;
	v.y = this->y + rhs.y;
	v.z = this->z + rhs.z;
	v.w = this->w + rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4i::
operator+=(const vec4i& rhs){DPZoneScoped;
#if DESHI_USE_SSE
	this->sse = m128_add_4s32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	this->x += rhs.x;
	this->y += rhs.y;
	this->z += rhs.z;
	this->w += rhs.w;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4i
vec4i_sub(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_sub_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = lhs.x - rhs.x;
	v.y = lhs.y - rhs.y;
	v.z = lhs.z - rhs.z;
	v.w = lhs.w - rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
operator- (const vec4i& rhs)const{DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_sub_4s32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = this->x - rhs.x;
	v.y = this->y - rhs.y;
	v.z = this->z - rhs.z;
	v.w = this->w - rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4i::
operator-=(const vec4i& rhs){DPZoneScoped;
#if DESHI_USE_SSE
	this->sse = m128_sub_4s32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	this->x -= rhs.x;
	this->y -= rhs.y;
	this->z -= rhs.z;
	this->w -= rhs.w;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4i
vec4i_mul(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_mul_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = lhs.x * rhs.x;
	v.y = lhs.y * rhs.y;
	v.z = lhs.z * rhs.z;
	v.w = lhs.w * rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
operator* (const vec4i& rhs)const{DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_mul_4s32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = this->x * rhs.x;
	v.y = this->y * rhs.y;
	v.z = this->z * rhs.z;
	v.w = this->w * rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4i::
operator*=(const vec4i& rhs){DPZoneScoped;
#if DESHI_USE_SSE
	this->sse = m128_mul_4s32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	this->x *= rhs.x;
	this->y *= rhs.y;
	this->z *= rhs.z;
	this->w *= rhs.w;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4i
vec4i_mul_f32(vec4i lhs, f32 rhs){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_mul_4s32(lhs.sse, m128_fill_4s32(rhs));
#else //#if DESHI_USE_SSE
	v.x = lhs.x * rhs;
	v.y = lhs.y * rhs;
	v.z = lhs.z * rhs;
	v.w = lhs.w * rhs;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
operator* (const f32& rhs)const{DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_mul_4s32(this->sse, m128_fill_4s32(rhs));
#else //#if DESHI_USE_SSE
	v.x = this->x * rhs;
	v.y = this->y * rhs;
	v.z = this->z * rhs;
	v.w = this->w * rhs;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void vec4i::
operator*=(const f32& rhs){DPZoneScoped;
#if DESHI_USE_SSE
	this->sse = m128_mul_4s32(this->sse, m128_fill_4s32(rhs));
#else //#if DESHI_USE_SSE
	this->x *= rhs;
	this->y *= rhs;
	this->z *= rhs;
	this->w *= rhs;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline vec4i vec4i::
operator* (s32 lhs, vec4i rhs){
	return rhs * lhs;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
vec4i_negate(vec4i lhs){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_negate_4s32(lhs.sse);
#else //#if DESHI_USE_SSE
	v.x = -(lhs.x);
	v.y = -(lhs.y);
	v.z = -(lhs.z);
	v.w = -(lhs.w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
operator- ()const{DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_negate_4s32(this->sse);
#else //#if DESHI_USE_SSE
	v.x = -(this->x);
	v.y = -(this->y);
	v.z = -(this->z);
	v.w = -(this->w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline b32
vec4i_equal(vec4i lhs, vec4i rhs){DPZoneScoped;
#if DESHI_USE_SSE
	return m128_equal_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	return fabs(lhs.x - rhs.x) < M_EPSILON 
		&& fabs(lhs.y - rhs.y) < M_EPSILON 
		&& fabs(lhs.z - rhs.z) < M_EPSILON 
		&& fabs(lhs.w - rhs.w) < M_EPSILON;
#endif //#else //#if DESHI_USE_SSE
}

#ifdef __cplusplus
inline bool vec4i::
operator==(const vec4i& rhs)const{DPZoneScoped;
#if DESHI_USE_SSE
	return m128_equal_4s32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	return fabs(this->x - rhs.x) < M_EPSILON 
		&& fabs(this->y - rhs.y) < M_EPSILON 
		&& fabs(this->z - rhs.z) < M_EPSILON 
		&& fabs(this->w - rhs.w) < M_EPSILON;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline b32
vec4i_nequal(vec4i lhs, vec4i rhs){DPZoneScoped;
	#if DESHI_USE_SSE
	return !m128_equal_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	return fabs(lhs.x - rhs.x) > M_EPSILON 
		|| fabs(lhs.y - rhs.y) > M_EPSILON 
		|| fabs(lhs.z - rhs.z) > M_EPSILON 
		|| fabs(lhs.w - rhs.w) > M_EPSILON;
#endif //#else //#if DESHI_USE_SSE
}

#ifdef __cplusplus
inline bool vec4i::
operator!=(const vec4i& rhs)const{DPZoneScoped;
#if DESHI_USE_SSE
	return !m128_equal_4s32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	return fabs(this->x - rhs.x) > M_EPSILON 
		|| fabs(this->y - rhs.y) > M_EPSILON 
		|| fabs(this->z - rhs.z) > M_EPSILON 
		|| fabs(this->w - rhs.w) > M_EPSILON;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4i
vec4i_abs(vec4i lhs){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_abs_4s32(lhs.sse);
#else //#if DESHI_USE_SSE
	v.x = fabs(lhs.x);
	v.y = fabs(lhs.y);
	v.z = fabs(lhs.z);
	v.w = fabs(lhs.w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
abs()const{DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_abs_4s32(this->sse);
#else //#if DESHI_USE_SSE
	v.x = fabs(this->x);
	v.y = fabs(this->y);
	v.z = fabs(this->z);
	v.w = fabs(this->w);
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec4i_dot(vec4i lhs, vec4i rhs){DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mullo_epi32(lhs.sse, rhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_epi32(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_epi32(temp0, temp1); //add x+z with y+w
	result = (f32)_mm_cvtsi128_si32(temp0);
#else //#if DESHI_USE_SSE
	result = (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
#endif //#else //#if DESHI_USE_SSE
	return result;
}

#ifdef __cplusplus
inline f32 vec4i::
dot(const vec4i& rhs)const{DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mullo_epi32(this->sse, rhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_epi32(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_epi32(temp0, temp1); //add x+z with y+w
	result = (f32)_mm_cvtsi128_si32(temp0);
#else //#if DESHI_USE_SSE
	result = (this->x * rhs.x) + (this->y * rhs.y) + (this->z * rhs.z) + (this->w * rhs.w);
#endif //#else //#if DESHI_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec4i_mag(vec4i lhs){DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mullo_epi32(lhs.sse, lhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_epi32(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_epi32(temp0, temp1); //add x+z with y+w
	result = sqrtf((f32)_mm_cvtsi128_si32(temp0));
#else //#if DESHI_USE_SSE
	result = sqrtf((lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z) + (lhs.w * lhs.w));
#endif //#else //#if DESHI_USE_SSE
	return result;
}

#ifdef __cplusplus
inline f32 vec4i::
mag()const{DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mullo_epi32(this->sse, this->sse); //multiply together
	__m128 temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_epi32(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_epi32(temp0, temp1); //add x+z with y+w
	result = sqrtf((f32)_mm_cvtsi128_si32(temp0));
#else //#if DESHI_USE_SSE
	result = sqrtf((this->x * this->x) + (this->y * this->y) + (this->z * this->z) + (this->w * this->w));
#endif //#else //#if DESHI_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec4i_mag_sq(vec4i lhs){DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mullo_epi32(lhs.sse, lhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_epi32(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_epi32(temp0, temp1); //add x+z with y+w
	result = (f32)_mm_cvtsi128_si32(temp0);
#else //#if DESHI_USE_SSE
	result = (lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z) + (lhs.w * lhs.w);
#endif //#else //#if DESHI_USE_SSE
	return result;
}

#ifdef __cplusplus
inline f32 vec4i::
mag_sq()const{DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mullo_epi32(this->sse, this->sse); //multiply together
	__m128 temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_epi32(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_epi32(temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_epi32(temp0, temp1); //add x+z with y+w
	result = (f32)_mm_cvtsi128_si32(temp0);
#else //#if DESHI_USE_SSE
	result = (this->x * this->x) + (this->y * this->y) + (this->z * this->z) + (this->w * this->w);
#endif //#else //#if DESHI_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline f32
vec4i_distance(vec4i lhs, vec4i rhs){DPZoneScoped;
	return vec4i_mag(vec4i_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec4i::
distance(const vec4i& rhs)const{DPZoneScoped;
	return (*this - rhs).mag();
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
vec4i_distance_sq(vec4i lhs, vec4i rhs){DPZoneScoped;
	return vec4i_mag_sq(vec4i_subtract(lhs,rhs));
}

#ifdef __cplusplus
inline f32 vec4i::
distance_sq(const vec4i& rhs)const{DPZoneScoped;
	return (*this - rhs).mag_sq();
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
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

EXTERN_C inline vec4i
vec4i_component(vec4i lhs, vec4i rhs){DPZoneScoped;
	return vec4i_mul_f32(vec4i_normalize(rhs), vec4i_projection(lhs,rhs));
}

#ifdef __cplusplus
inline vec4i vec4i::
component(const vec4i& rhs)const{DPZoneScoped;
	return rhs.normalize() * this->projection(rhs);
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4i
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

EXTERN_C inline f32
vec4i_radians_between(vec4i lhs, vec4i rhs){DPZoneScoped;
	f32 m = vec4i_mag(lhs) * vec4i_mag(rhs);
	if(m > M_EPSILON){
		return acosf(vec4i_dot(rhs) / m);
	}else{
		return 0;
	}
}

#ifdef __cplusplus
inline f32 vec4i::
radians_between(const vec4i& rhs)const{DPZoneScoped;
	f32 m = this->mag() * rhs.mag();
	if(m > M_EPSILON){
		return acosf(this->dot(rhs) / m);
	}else{
		return 0;
	}
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4i
vec4i_min(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_min_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z < rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w < rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
min(const vec4i& rhs){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_min_4s32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = (this->x < rhs.x) ? this->x : rhs.x;
	v.y = (this->y < rhs.y) ? this->y : rhs.y;
	v.z = (this->z < rhs.z) ? this->z : rhs.z;
	v.w = (this->w < rhs.w) ? this->w : rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4i
min(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_min_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = (lhs.x < rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y < rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z < rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w < rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4i
vec4i_max(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_max_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z > rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w > rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
max(const vec4i& rhs){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_min_4s32(this->sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = (this->x > rhs.x) ? this->x : rhs.x;
	v.y = (this->y > rhs.y) ? this->y : rhs.y;
	v.z = (this->z > rhs.z) ? this->z : rhs.z;
	v.w = (this->w > rhs.w) ? this->w : rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4i
max(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_min_4s32(lhs.sse, rhs.sse);
#else //#if DESHI_USE_SSE
	v.x = (lhs.x > rhs.x) ? lhs.x : rhs.x;
	v.y = (lhs.y > rhs.y) ? lhs.y : rhs.y;
	v.z = (lhs.z > rhs.z) ? lhs.z : rhs.z;
	v.w = (lhs.w > rhs.w) ? lhs.w : rhs.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
vec4i_clamp_min(vec4i value, vec4i min){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_max_4s32(value.sse, min.sse);
#else //#if DESHI_USE_SSE
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	v.z = (value.z < min.z) ? min.z : value.z;
	v.w = (value.w < min.w) ? min.w : value.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
clamp_min(const vec4i& min){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_max_4s32(this->sse, min.sse);
#else //#if DESHI_USE_SSE
	v.x = (this->x < min.x) ? min.x : this->x;
	v.y = (this->y < min.y) ? min.y : this->y;
	v.z = (this->z < min.z) ? min.z : this->z;
	v.w = (this->w < min.w) ? min.w : this->w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4i
clamp_min(vec4i value, vec4i min){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_max_4s32(value.sse, min.sse);
#else //#if DESHI_USE_SSE
	v.x = (value.x < min.x) ? min.x : value.x;
	v.y = (value.y < min.y) ? min.y : value.y;
	v.z = (value.z < min.z) ? min.z : value.z;
	v.w = (value.w < min.w) ? min.w : value.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4i
vec4i_clamp_max(vec4i value, vec4i max){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_min_4s32(value.sse, max.sse);
#else //#if DESHI_USE_SSE
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	v.z = (value.z > max.z) ? max.z : value.z;
	v.w = (value.w > max.w) ? max.w : value.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}

#ifdef __cplusplus
inline vec4i vec4i::
clamp_max(const vec4i& maX){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_min_4s32(this->sse, max.sse);
#else //#if DESHI_USE_SSE
	v.x = (this->x > max.x) ? max.x : this->x;
	v.y = (this->y > max.y) ? max.y : this->y;
	v.z = (this->z > max.z) ? max.z : this->z;
	v.w = (this->w > max.w) ? max.w : this->w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
template<> inline vec4i
clamp_max(vec4i lhs, vec4i rhs){DPZoneScoped;
	vec4i v;
#if DESHI_USE_SSE
	v.sse = m128_min_4s32(value.sse, max.sse);
#else //#if DESHI_USE_SSE
	v.x = (value.x > max.x) ? max.x : value.x;
	v.y = (value.y > max.y) ? max.y : value.y;
	v.z = (value.z > max.z) ? max.z : value.z;
	v.w = (value.w > max.w) ? max.w : value.w;
#endif //#else //#if DESHI_USE_SSE
	return v;
}
#endif //#ifdef __cplusplus

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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

EXTERN_C inline vec4i
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
	return Vec2i(a.x, a.y);
}

#ifdef __cplusplus
inline vec2i
vec2::to_vec2i()const{DPZoneScoped;
	return Vec2i(this->x, this->y);
}
#endif //#ifdef __cplusplus

inline vec3
vec2_to_vec3(vec2 a){DPZoneScoped;
	return Vec3(a.x, a.y, 0);
}

#ifdef __cplusplus
inline vec3
vec2::to_vec3()const{DPZoneScoped;
	return Vec3(this->x, this->y, 0);
}
#endif //#ifdef __cplusplus

inline vec3i
vec2_to_vec3i(vec2 a){DPZoneScoped;
	return Vec3i(a.x, a.y, 0);
}

#ifdef __cplusplus
inline vec3i
vec2::to_vec3i()const{DPZoneScoped;
	return Vec3i(this->x, this->y, 0);
}
#endif //#ifdef __cplusplus

inline vec4
vec2_to_vec4(vec2 a){DPZoneScoped;
	return Vec4(a.x, a.y, 0, 0);
}

#ifdef __cplusplus
inline vec4
vec2::to_vec4()const{DPZoneScoped;
	return Vec4(this->x, this->y, 0, 0);
}
#endif //#ifdef __cplusplus

inline vec4i
vec2_to_vec4i(vec2 a){DPZoneScoped;
	return Vec4i(a.x, a.y, 0, 0);
}

#ifdef __cplusplus
inline vec4i
vec2::to_vec4i()const{DPZoneScoped;
	return Vec4i(this->x, this->y, 0, 0);
}
#endif //#ifdef __cplusplus

inline vec2
vec2i_to_vec2(vec2i a){DPZoneScoped;
	return Vec2(a.x, a.y);
}

#ifdef __cplusplus
inline vec2
vec2i::to_vec2()const{DPZoneScoped;
	return Vec2(this->x, this->y);
}
#endif //#ifdef __cplusplus

inline vec3
vec2i_to_vec3(vec2i a){DPZoneScoped;
	return Vec3(a.x, a.y, 0);
}

#ifdef __cplusplus
inline vec3
vec2i::to_vec3()const{DPZoneScoped;
	return Vec3(this->x, this->y, 0);
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
	return Vec4(a.x, a.y, 0, 0);
}

#ifdef __cplusplus
inline vec4
vec2i::to_vec4()const{DPZoneScoped;
	return Vec4(this->x, this->y, 0, 0);
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
	return Vec2i(a.x, a.y);
}

#ifdef __cplusplus
inline vec2i
vec3::to_vec2i()const{DPZoneScoped;
	return Vec2i(this->x, this->y);
}
#endif //#ifdef __cplusplus

inline vec3i
vec3_to_vec3i(vec3 a){DPZoneScoped;
	return Vec3i(a.x, a.y, 0);
}

#ifdef __cplusplus
inline vec3i
vec3::to_vec3i()const{DPZoneScoped;
	return Vec3i(this->x, this->y, 0);
}
#endif //#ifdef __cplusplus

inline vec4
vec3_to_vec4(vec3 a){DPZoneScoped;
	return Vec4(a.x, a.y, 0, 0);
}

#ifdef __cplusplus
inline vec4
vec3::to_vec4()const{DPZoneScoped;
	return Vec4(this->x, this->y, 0, 0);
}
#endif //#ifdef __cplusplus

inline vec4i
vec3_to_vec4i(vec3 a){DPZoneScoped;
	return Vec4i(a.x, a.y, 0, 0);
}

#ifdef __cplusplus
inline vec4i
vec3::to_vec4i()const{DPZoneScoped;
	return Vec4i(this->x, this->y, 0, 0);
}
#endif //#ifdef __cplusplus

inline vec2
vec3i_to_vec2(vec3i a){DPZoneScoped;
	return Vec2(a.x, a.y);
}

#ifdef __cplusplus
inline vec2
vec3i::to_vec2()const{DPZoneScoped;
	return Vec2(this->x, this->y);
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
	return Vec3(a.x, a.y, 0);
}

#ifdef __cplusplus
inline vec3
vec3i::to_vec3()const{DPZoneScoped;
	return Vec3(this->x, this->y, 0);
}
#endif //#ifdef __cplusplus

inline vec4
vec3i_to_vec4(vec3i a){DPZoneScoped;
	return Vec4(a.x, a.y, 0, 0);
}

#ifdef __cplusplus
inline vec4
vec3i::to_vec4()const{DPZoneScoped;
	return Vec4(this->x, this->y, 0, 0);
}
#endif //#ifdef __cplusplus

inline vec4i
vec3i_to_vec4i(vec3i a){DPZoneScoped;
	return Vec4i(a.x, a.y, 0, 0);
}

#ifdef __cplusplus
inline vec4i
vec3i::to_vec4i()const{DPZoneScoped;
	return Vec4i(this->x, this->y, 0, 0);
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
	return Vec2i(a.x, a.y);
}

#ifdef __cplusplus
inline vec2i
vec4::to_vec2i()const{DPZoneScoped;
	return Vec2i(this->x, this->y);
}
#endif //#ifdef __cplusplus

inline vec3
vec4_to_vec3(vec4 a){DPZoneScoped;
	return Vec3(a.x, a.y, 0);
}

#ifdef __cplusplus
inline vec3
vec4::to_vec3()const{DPZoneScoped;
	return Vec3(this->x, this->y, 0);
}
#endif //#ifdef __cplusplus

inline vec3i
vec4_to_vec3i(vec4 a){DPZoneScoped;
	return Vec3i(a.x, a.y, 0);
}

#ifdef __cplusplus
inline vec3i
vec4::to_vec3i()const{DPZoneScoped;
	return Vec3i(this->x, this->y, 0);
}
#endif //#ifdef __cplusplus

inline vec4i
vec4_to_vec4i(vec4 a){DPZoneScoped;
	return Vec4i(a.x, a.y, 0, 0);
}

#ifdef __cplusplus
inline vec4i
vec4::to_vec4i()const{DPZoneScoped;
	return Vec4i(this->x, this->y, 0, 0);
}
#endif //#ifdef __cplusplus

inline vec2
vec4i_to_vec2(vec4i a){DPZoneScoped;
	return Vec2(a.x, a.y);
}

#ifdef __cplusplus
inline vec2
vec4i::to_vec2()const{DPZoneScoped;
	return Vec2(this->x, this->y);
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
	return Vec3(a.x, a.y, 0);
}

#ifdef __cplusplus
inline vec3
vec4i::to_vec3()const{DPZoneScoped;
	return Vec3(this->x, this->y, 0);
}
#endif //#ifdef __cplusplus

inline vec3i
vec4i_to_vec3i(vec4i a){DPZoneScoped;
	return Vec3i(a.x, a.y, 0);
}

#ifdef __cplusplus
inline vec3i
vec4i::to_vec3i()const{DPZoneScoped;
	return Vec3i(this->x, this->y, 0);
}
#endif //#ifdef __cplusplus

inline vec4
vec4i_to_vec4(vec4i a){DPZoneScoped;
	return Vec4(a.x, a.y, 0, 0);
}

#ifdef __cplusplus
inline vec4
vec4i::to_vec4()const{DPZoneScoped;
	return Vec4(this->x, this->y, 0, 0);
}
#endif //#ifdef __cplusplus


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec_hashing
#ifndef DESHI_MATH_DISABLE_HASHING
#ifdef __cplusplus
#include "kigu/hash.h"


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


EXTERN_C dstr8
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

EXTERN_C dstr8
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

EXTERN_C dstr8
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

EXTERN_C dstr8
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

EXTERN_C dstr8
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

EXTERN_C dstr8
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

EXTERN_C dstr8
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

EXTERN_C dstr8
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

EXTERN_C dstr8
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


EXTERN_C typedef struct mat3{
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

EXTERN_C inline mat3
Mat3(f32 _00, f32 _10, f32 _20,
	 f32 _01, f32 _11, f32 _21,
	 f32 _02, f32 _12, f32 _22){
	return mat3{_00, _10, _20, _01, _11, _21, _02, _12, _22};
}

EXTERN_C inline mat3
array_to_mat3(f32* arr){
	return mat3{arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8]};
}

EXTERN_C inline mat3 mat3_ZERO()    { return mat3{0,0,0,0,0,0,0,0,0}; }
EXTERN_C inline mat3 mat3_ONE()     { return mat3{1,1,1,1,1,1,1,1,1}; }
EXTERN_C inline mat3 mat3_IDENTITY(){ return mat3{1,0,0,0,1,0,0,0,1}; }

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

EXTERN_C inline mat3
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

EXTERN_C inline mat3
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

EXTERN_C inline mat3
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

EXTERN_C inline mat3
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

EXTERN_C inline mat3
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

EXTERN_C inline mat3
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

EXTERN_C inline mat3
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

EXTERN_C inline b32
mat3_equal(mat3 lhs, mat3 rhs){DPZoneScoped;
	return fabs(lhs.arr[0] - rhs.arr[0]) > M_EPSILON
		&& fabs(lhs.arr[1] - rhs.arr[1]) > M_EPSILON
		&& fabs(lhs.arr[2] - rhs.arr[2]) > M_EPSILON
		&& fabs(lhs.arr[3] - rhs.arr[3]) > M_EPSILON
		&& fabs(lhs.arr[4] - rhs.arr[4]) > M_EPSILON
		&& fabs(lhs.arr[5] - rhs.arr[5]) > M_EPSILON
		&& fabs(lhs.arr[6] - rhs.arr[6]) > M_EPSILON
		&& fabs(lhs.arr[7] - rhs.arr[7]) > M_EPSILON
		&& fabs(lhs.arr[8] - rhs.arr[8]) > M_EPSILON;
}

#ifdef __cplusplus
inline b32 mat3::
operator==(const mat3& rhs)const{DPZoneScoped;
	return fabs(this->arr[0] - rhs.arr[0]) > M_EPSILON
		&& fabs(this->arr[1] - rhs.arr[1]) > M_EPSILON
		&& fabs(this->arr[2] - rhs.arr[2]) > M_EPSILON
		&& fabs(this->arr[3] - rhs.arr[3]) > M_EPSILON
		&& fabs(this->arr[4] - rhs.arr[4]) > M_EPSILON
		&& fabs(this->arr[5] - rhs.arr[5]) > M_EPSILON
		&& fabs(this->arr[6] - rhs.arr[6]) > M_EPSILON
		&& fabs(this->arr[7] - rhs.arr[7]) > M_EPSILON
		&& fabs(this->arr[8] - rhs.arr[8]) > M_EPSILON;
}
#endif //#ifdef __cplusplus

EXTERN_C inline b32
mat3_nequal(mat3 lhs, mat3 rhs){DPZoneScoped;
	return fabs(lhs.arr[0] - rhs.arr[0]) < M_EPSILON
		|| fabs(lhs.arr[1] - rhs.arr[1]) < M_EPSILON
		|| fabs(lhs.arr[2] - rhs.arr[2]) < M_EPSILON
		|| fabs(lhs.arr[3] - rhs.arr[3]) < M_EPSILON
		|| fabs(lhs.arr[4] - rhs.arr[4]) < M_EPSILON
		|| fabs(lhs.arr[5] - rhs.arr[5]) < M_EPSILON
		|| fabs(lhs.arr[6] - rhs.arr[6]) < M_EPSILON
		|| fabs(lhs.arr[7] - rhs.arr[7]) < M_EPSILON
		|| fabs(lhs.arr[8] - rhs.arr[8]) < M_EPSILON;
}

#ifdef __cplusplus
inline b32 mat3::
operator!=(const mat3& rhs)const{DPZoneScoped;
	return fabs(this->arr[0] - rhs.arr[0]) < M_EPSILON
		|| fabs(this->arr[1] - rhs.arr[1]) < M_EPSILON
		|| fabs(this->arr[2] - rhs.arr[2]) < M_EPSILON
		|| fabs(this->arr[3] - rhs.arr[3]) < M_EPSILON
		|| fabs(this->arr[4] - rhs.arr[4]) < M_EPSILON
		|| fabs(this->arr[5] - rhs.arr[5]) < M_EPSILON
		|| fabs(this->arr[6] - rhs.arr[6]) < M_EPSILON
		|| fabs(this->arr[7] - rhs.arr[7]) < M_EPSILON
		|| fabs(this->arr[8] - rhs.arr[8]) < M_EPSILON;
}
#endif //#ifdef __cplusplus

EXTERN_C inline b32
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

EXTERN_C inline f32
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

EXTERN_C inline f32
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

EXTERN_C inline f32
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

EXTERN_C inline mat3
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

EXTERN_C inline mat3
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

//returns a LH rotation transformation matrix based on input in radians
EXTERN_C inline mat3
mat3_rotation_matrix_x_radians(f32 angle){DPZoneScoped;
	f32 c = cosf(angle); f32 s = sinf(angle);
	return Mat3(1, 0, 0,
				0, c, s,
				0,-s, c);
}

//returns a LH rotation transformation matrix based on input in degrees
EXTERN_C inline mat3
mat3_rotation_matrix_x_degrees(f32 angle){DPZoneScoped;
	angle = Radians(angle);
	f32 c = cosf(angle); f32 s = sinf(angle);
	return Mat3(1, 0, 0,
				0, c, s,
				0,-s, c);
}

//returns a LH rotation transformation matrix based on input in radians
EXTERN_C inline mat3
mat3_rotation_matrix_y_radians(f32 angle){DPZoneScoped;
	f32 c = cosf(angle); f32 s = sinf(angle);
	return Mat3(c, 0,-s,
				0, 1, 0,
				s, 0, c);
}

//returns a LH rotation transformation matrix based on input in degrees
EXTERN_C inline mat3
mat3_rotation_matrix_y_degrees(f32 angle){DPZoneScoped;
	angle = Radians(angle);
	f32 c = cosf(angle); f32 s = sinf(angle);
	return Mat3(c, 0,-s,
				0, 1, 0,
				s, 0, c);
}

//returns a LH rotation transformation matrix based on input in radians
EXTERN_C inline mat3
mat3_rotation_matrix_z_radians(f32 angle){DPZoneScoped;
	f32 c = cosf(angle); f32 s = sinf(angle);
	return Mat3( c, s, 0,
				-s, c, 0,
				0,  0, 1);
}

//returns a LH rotation transformation matrix based on input in degrees
EXTERN_C inline mat3
mat3_rotation_matrix_z_degrees(f32 angle){DPZoneScoped;
	angle = Radians(angle);
	f32 c = cosf(angle); f32 s = sinf(angle);
	return Mat3( c, s, 0,
				-s, c, 0,
				0,  0, 1);
}

//returns a pre-multiplied X->Y->Z LH rotation transformation matrix based on input in radians
EXTERN_C inline mat3
mat3_rotation_matrix_radians(f32 x, f32 y, f32 z){DPZoneScoped;
	f32 cX = cosf(x); f32 sX = sinf(x);
	f32 cY = cosf(y); f32 sY = sinf(y);
	f32 cZ = cosf(z); f32 sZ = sinf(z);
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

//returns a pre-multiplied X->Y->Z LH rotation transformation matrix based on input in degrees
EXTERN_C inline mat3
mat3_rotation_matrix_degrees(f32 x, f32 y, f32 z){DPZoneScoped;
	x = Radians(x); y = Radians(y); z = Radians(z);
	f32 cX = cosf(x); f32 sX = sinf(x);
	f32 cY = cosf(y); f32 sY = sinf(y);
	f32 cZ = cosf(z); f32 sZ = sinf(z);
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


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @mat4


EXTERN_C typedef struct mat4{
	union{
		f32 arr[16];
		struct{
			vec4 row0;
			vec4 row1;
			vec4 row2;
			vec4 row3;
		};
#if DESHI_USE_SSE
		struct{
			__m128 sse_row0;
			__m128 sse_row1;
			__m128 sse_row2;
			__m128 sse_row3;
		};
#endif
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

EXTERN_C inline mat4
Mat4(f32 _00, f32 _10, f32 _20, f32 _30
	 f32 _01, f32 _11, f32 _21, f32 _31
	 f32 _02, f32 _12, f32 _22, f32 _32,
	 f32 _03, f32 _13, f32 _23, f32 _33){
	return mat4{_00, _10, _20, _30, _01, _11, _21, _31, _02, _12, _22, _32, _03, _13, _23, _33};
}

EXTERN_C inline mat4
array_to_mat4(f32* arr){
	return mat4{arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8], arr[9], arr[10], arr[11], arr[12], arr[13], arr[14], arr[15]};
}

EXTERN_C inline mat4 mat4_ZERO()    { return mat4{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; }
EXTERN_C inline mat4 mat4_ONE()     { return mat4{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}; }
EXTERN_C inline mat4 mat4_IDENTITY(){ return mat4{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}; }

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

EXTERN_C inline mat4
mat4_add_elements(mat4 lhs, mat4 rhs){DPZoneScoped;
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = m128_add_4f32(lhs.sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_add_4f32(lhs.sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_add_4f32(lhs.sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_add_4f32(lhs.sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
	return result;
}

#ifdef __cplusplus
inline mat4 mat4::
operator+ (const mat4& rhs)const{DPZoneScoped;
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = m128_add_4f32(this->sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_add_4f32(this->sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_add_4f32(this->sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_add_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat4::
operator+=(const mat4& rhs){DPZoneScoped;
#if DESHI_USE_SSE
	this->sse_row0 = m128_add_4f32(this->sse_row0, rhs.sse_row0);
	this->sse_row1 = m128_add_4f32(this->sse_row1, rhs.sse_row1);
	this->sse_row2 = m128_add_4f32(this->sse_row2, rhs.sse_row2);
	this->sse_row3 = m128_add_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline mat4
mat4_sub_elements(mat4 lhs, mat4 rhs){DPZoneScoped;
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = m128_sub_4f32(lhs.sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_sub_4f32(lhs.sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_sub_4f32(lhs.sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_sub_4f32(lhs.sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
	return result;
}

#ifdef __cplusplus
inline mat4 mat4::
operator- (const mat4& rhs)const{DPZoneScoped;
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = m128_sub_4f32(this->sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_sub_4f32(this->sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_sub_4f32(this->sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_sub_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat4::
operator-=(const mat4& rhs){DPZoneScoped;
#if DESHI_USE_SSE
	this->sse_row0 = m128_sub_4f32(this->sse_row0, rhs.sse_row0);
	this->sse_row1 = m128_sub_4f32(this->sse_row1, rhs.sse_row1);
	this->sse_row2 = m128_sub_4f32(this->sse_row2, rhs.sse_row2);
	this->sse_row3 = m128_sub_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline mat4
mat4_mul_f32(mat4 lhs, f32 rhs){DPZoneScoped;
	mat4 result;
#if DESHI_USE_SSE
	__m128 scalar = m128_fill_4f32(rhs);
	result.sse_row0 = m128_mul_4f32(lhs.sse_row0, scalar);
	result.sse_row1 = m128_mul_4f32(lhs.sse_row1, scalar);
	result.sse_row2 = m128_mul_4f32(lhs.sse_row2, scalar);
	result.sse_row3 = m128_mul_4f32(lhs.sse_row3, scalar);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
	return result;
}

#ifdef __cplusplus
inline mat4 mat4::
operator* (const f32& rhs)const{DPZoneScoped;
	mat4 result;
#if DESHI_USE_SSE
	__m128 scalar = m128_fill_4f32(rhs);
	result.sse_row0 = m128_mul_4f32(this->sse_row0, scalar);
	result.sse_row1 = m128_mul_4f32(this->sse_row1, scalar);
	result.sse_row2 = m128_mul_4f32(this->sse_row2, scalar);
	result.sse_row3 = m128_mul_4f32(this->sse_row3, scalar);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat4::
operator*=(const f32& rhs){DPZoneScoped;
#if DESHI_USE_SSE
	__m128 scalar = m128_fill_4f32(rhs);
	this->sse_row0 = m128_mul_4f32(this->sse_row0, scalar);
	this->sse_row1 = m128_mul_4f32(this->sse_row1, scalar);
	this->sse_row2 = m128_mul_4f32(this->sse_row2, scalar);
	this->sse_row3 = m128_mul_4f32(this->sse_row3, scalar);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline mat4 mat4::
operator* (const f32& lhs, const mat4& rhs)const{
	return rhs * lhs;
}
#endif //#ifdef __cplusplus

EXTERN_C inline mat4
mat4_mul_mat4(mat4 lhs, mat4 rhs){DPZoneScoped;
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = m128_linear_combine(lhs.sse_row0, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row1 = m128_linear_combine(lhs.sse_row1, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row2 = m128_linear_combine(lhs.sse_row2, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row3 = m128_linear_combine(lhs.sse_row3, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
	return result;
}

#ifdef __cplusplus
inline mat4 mat4::
operator* (const mat4& rhs)const{DPZoneScoped;
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = m128_linear_combine(this->sse_row0, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row1 = m128_linear_combine(this->sse_row1, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row2 = m128_linear_combine(this->sse_row2, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row3 = m128_linear_combine(this->sse_row3, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat4::
operator*=(const mat4& rhs){DPZoneScoped;
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = m128_linear_combine(this->sse_row0, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row1 = m128_linear_combine(this->sse_row1, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row2 = m128_linear_combine(this->sse_row2, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row3 = m128_linear_combine(this->sse_row3, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
	*this = result;
}
#endif //#ifdef __cplusplus

EXTERN_C inline mat4
mat4_mul_elements(mat4 lhs, mat4 rhs){DPZoneScoped;
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = m128_mul_4f32(lhs.sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_mul_4f32(lhs.sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_mul_4f32(lhs.sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_mul_4f32(lhs.sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
	return result;
}

#ifdef __cplusplus
inline mat4 mat4::
operator^ (const mat4& rhs)const{DPZoneScoped;
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = m128_mul_4f32(this->sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_mul_4f32(this->sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_mul_4f32(this->sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_mul_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
	return result;
} 
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat4::
operator^=(const mat4& rhs){DPZoneScoped;
#if DESHI_USE_SSE
	this->sse_row0 = m128_mul_4f32(this->sse_row0, rhs.sse_row0);
	this->sse_row1 = m128_mul_4f32(this->sse_row1, rhs.sse_row1);
	this->sse_row2 = m128_mul_4f32(this->sse_row2, rhs.sse_row2);
	this->sse_row3 = m128_mul_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline mat4
mat4_div_f32(mat4 lhs, f32 rhs){DPZoneScoped;
	Assert(rhs != 0, "mat4 elements cant be divided by zero");
	mat4 result;
#if DESHI_USE_SSE
	__m128 scalar = m128_fill_4f32(rhs);
	result.sse_row0 = m128_div_4f32(lhs.sse_row0, scalar);
	result.sse_row1 = m128_div_4f32(lhs.sse_row1, scalar);
	result.sse_row2 = m128_div_4f32(lhs.sse_row2, scalar);
	result.sse_row3 = m128_div_4f32(lhs.sse_row3, scalar);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
	return result;
}

#ifdef __cplusplus
inline mat4 mat4::
operator/ (const f32& rhs)const{DPZoneScoped;
	Assert(rhs != 0, "mat4 elements cant be divided by zero");
	mat4 result;
#if DESHI_USE_SSE
	__m128 scalar = m128_fill_4f32(rhs);
	result.sse_row0 = m128_div_4f32(this->sse_row0, scalar);
	result.sse_row1 = m128_div_4f32(this->sse_row1, scalar);
	result.sse_row2 = m128_div_4f32(this->sse_row2, scalar);
	result.sse_row3 = m128_div_4f32(this->sse_row3, scalar);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
	return result;
}
#endif //#ifdef __cplusplus

#ifdef __cplusplus
inline void mat4::
operator/=(const f32& rhs){DPZoneScoped;
	Assert(rhs != 0, "mat4 elements cant be divided by zero");
#if DESHI_USE_SSE
	__m128 scalar = m128_fill_4f32(rhs);
	this->sse_row0 = m128_div_4f32(this->sse_row0, scalar);
	this->sse_row1 = m128_div_4f32(this->sse_row1, scalar);
	this->sse_row2 = m128_div_4f32(this->sse_row2, scalar);
	this->sse_row3 = m128_div_4f32(this->sse_row3, scalar);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline mat4
mat4_div_elements(mat4 lhs, mat4 rhs){DPZoneScoped;
	Assert(rhs.arr[ 0] != 0 && rhs.arr[ 1] != 0 && rhs.arr[ 2] != 0 && rhs.arr[ 3] != 0
		   rhs.arr[ 4] != 0 && rhs.arr[ 5] != 0 && rhs.arr[ 6] != 0 && rhs.arr[ 7] != 0
		   rhs.arr[ 8] != 0 && rhs.arr[ 9] != 0 && rhs.arr[10] != 0 && rhs.arr[11] != 0
		   rhs.arr[12] != 0 && rhs.arr[13] != 0 && rhs.arr[14] != 0 && rhs.arr[15] != 0,
		   "mat4 elements cant be divided by zero");
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = m128_div_4f32(lhs.sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_div_4f32(lhs.sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_div_4f32(lhs.sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_div_4f32(lhs.sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
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
#if DESHI_USE_SSE
	result.sse_row0 = m128_div_4f32(this->sse_row0, rhs.sse_row0);
	result.sse_row1 = m128_div_4f32(this->sse_row1, rhs.sse_row1);
	result.sse_row2 = m128_div_4f32(this->sse_row2, rhs.sse_row2);
	result.sse_row3 = m128_div_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
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
#if DESHI_USE_SSE
	this->sse_row0 = m128_div_4f32(this->sse_row0, rhs.sse_row0);
	this->sse_row1 = m128_div_4f32(this->sse_row1, rhs.sse_row1);
	this->sse_row2 = m128_div_4f32(this->sse_row2, rhs.sse_row2);
	this->sse_row3 = m128_div_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
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
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline b32
mat4_equal(mat4 lhs, mat4 rhs){DPZoneScoped;
#if DESHI_USE_SSE
	return m128_equal_4f32(lhs.sse_row0, rhs.sse_row0)
		&& m128_equal_4f32(lhs.sse_row1, rhs.sse_row1)
		&& m128_equal_4f32(lhs.sse_row2, rhs.sse_row2)
		&& m128_equal_4f32(lhs.sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
	return fabs(lhs.arr[0] - rhs.arr[0]) > M_EPSILON
		&& fabs(lhs.arr[1] - rhs.arr[1]) > M_EPSILON
		&& fabs(lhs.arr[2] - rhs.arr[2]) > M_EPSILON
		&& fabs(lhs.arr[3] - rhs.arr[3]) > M_EPSILON
		&& fabs(lhs.arr[4] - rhs.arr[4]) > M_EPSILON
		&& fabs(lhs.arr[5] - rhs.arr[5]) > M_EPSILON
		&& fabs(lhs.arr[6] - rhs.arr[6]) > M_EPSILON
		&& fabs(lhs.arr[7] - rhs.arr[7]) > M_EPSILON
		&& fabs(lhs.arr[8] - rhs.arr[8]) > M_EPSILON
		&& fabs(lhs.arr[9] - rhs.arr[9]) > M_EPSILON
		&& fabs(lhs.arr[10] - rhs.arr[10]) > M_EPSILON
		&& fabs(lhs.arr[11] - rhs.arr[11]) > M_EPSILON
		&& fabs(lhs.arr[12] - rhs.arr[12]) > M_EPSILON
		&& fabs(lhs.arr[13] - rhs.arr[13]) > M_EPSILON
		&& fabs(lhs.arr[14] - rhs.arr[14]) > M_EPSILON
		&& fabs(lhs.arr[15] - rhs.arr[15]) > M_EPSILON;
#endif //#else //#if DESHI_USE_SSE
}

#ifdef __cplusplus
inline b32 mat4::
operator==(const mat4& rhs)const{DPZoneScoped;
#if DESHI_USE_SSE
	return m128_equal_4f32(this->sse_row0, rhs.sse_row0)
		&& m128_equal_4f32(this->sse_row1, rhs.sse_row1)
		&& m128_equal_4f32(this->sse_row2, rhs.sse_row2)
		&& m128_equal_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
	return fabs(this->arr[0] - rhs.arr[0]) > M_EPSILON
		&& fabs(this->arr[1] - rhs.arr[1]) > M_EPSILON
		&& fabs(this->arr[2] - rhs.arr[2]) > M_EPSILON
		&& fabs(this->arr[3] - rhs.arr[3]) > M_EPSILON
		&& fabs(this->arr[4] - rhs.arr[4]) > M_EPSILON
		&& fabs(this->arr[5] - rhs.arr[5]) > M_EPSILON
		&& fabs(this->arr[6] - rhs.arr[6]) > M_EPSILON
		&& fabs(this->arr[7] - rhs.arr[7]) > M_EPSILON
		&& fabs(this->arr[8] - rhs.arr[8]) > M_EPSILON
		&& fabs(this->arr[9] - rhs.arr[9]) > M_EPSILON
		&& fabs(this->arr[10] - rhs.arr[10]) > M_EPSILON
		&& fabs(this->arr[11] - rhs.arr[11]) > M_EPSILON
		&& fabs(this->arr[12] - rhs.arr[12]) > M_EPSILON
		&& fabs(this->arr[13] - rhs.arr[13]) > M_EPSILON
		&& fabs(this->arr[14] - rhs.arr[14]) > M_EPSILON
		&& fabs(this->arr[15] - rhs.arr[15]) > M_EPSILON;
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline b32
mat3_nequal(mat4 lhs, mat4 rhs){DPZoneScoped;
#if DESHI_USE_SSE
	return !m128_equal_4f32(lhs.sse_row0, rhs.sse_row0)
		&& !m128_equal_4f32(lhs.sse_row1, rhs.sse_row1)
		&& !m128_equal_4f32(lhs.sse_row2, rhs.sse_row2)
		&& !m128_equal_4f32(lhs.sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
	return fabs(lhs.arr[0] - rhs.arr[0]) < M_EPSILON
		|| fabs(lhs.arr[1] - rhs.arr[1]) < M_EPSILON
		|| fabs(lhs.arr[2] - rhs.arr[2]) < M_EPSILON
		|| fabs(lhs.arr[3] - rhs.arr[3]) < M_EPSILON
		|| fabs(lhs.arr[4] - rhs.arr[4]) < M_EPSILON
		|| fabs(lhs.arr[5] - rhs.arr[5]) < M_EPSILON
		|| fabs(lhs.arr[6] - rhs.arr[6]) < M_EPSILON
		|| fabs(lhs.arr[7] - rhs.arr[7]) < M_EPSILON
		|| fabs(lhs.arr[8] - rhs.arr[8]) < M_EPSILON
		|| fabs(lhs.arr[9] - rhs.arr[9]) < M_EPSILON
		|| fabs(lhs.arr[10] - rhs.arr[10]) < M_EPSILON
		|| fabs(lhs.arr[11] - rhs.arr[11]) < M_EPSILON
		|| fabs(lhs.arr[12] - rhs.arr[12]) < M_EPSILON
		|| fabs(lhs.arr[13] - rhs.arr[13]) < M_EPSILON
		|| fabs(lhs.arr[14] - rhs.arr[14]) < M_EPSILON
		|| fabs(lhs.arr[15] - rhs.arr[15]) < M_EPSILON:
#endif //#else //#if DESHI_USE_SSE
}

#ifdef __cplusplus
inline b32 mat4::
operator!=(const mat4& rhs)const{DPZoneScoped;
	#if DESHI_USE_SSE
	return !m128_equal_4f32(this->sse_row0, rhs.sse_row0)
		&& !m128_equal_4f32(this->sse_row1, rhs.sse_row1)
		&& !m128_equal_4f32(this->sse_row2, rhs.sse_row2)
		&& !m128_equal_4f32(this->sse_row3, rhs.sse_row3);
#else //#if DESHI_USE_SSE
	return fabs(this->arr[0] - rhs.arr[0]) < M_EPSILON
		|| fabs(this->arr[1] - rhs.arr[1]) < M_EPSILON
		|| fabs(this->arr[2] - rhs.arr[2]) < M_EPSILON
		|| fabs(this->arr[3] - rhs.arr[3]) < M_EPSILON
		|| fabs(this->arr[4] - rhs.arr[4]) < M_EPSILON
		|| fabs(this->arr[5] - rhs.arr[5]) < M_EPSILON
		|| fabs(this->arr[6] - rhs.arr[6]) < M_EPSILON
		|| fabs(this->arr[7] - rhs.arr[7]) < M_EPSILON
		|| fabs(this->arr[8] - rhs.arr[8]) < M_EPSILON
		|| fabs(this->arr[9] - rhs.arr[9]) < M_EPSILON
		|| fabs(this->arr[10] - rhs.arr[10]) < M_EPSILON
		|| fabs(this->arr[11] - rhs.arr[11]) < M_EPSILON
		|| fabs(this->arr[12] - rhs.arr[12]) < M_EPSILON
		|| fabs(this->arr[13] - rhs.arr[13]) < M_EPSILON
		|| fabs(this->arr[14] - rhs.arr[14]) < M_EPSILON
		|| fabs(this->arr[15] - rhs.arr[15]) < M_EPSILON:
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline b32
mat4_transpose(mat4 lhs){DPZoneScoped;
#if DESHI_USE_SSE
	_MM_TRANSPOSE4_PS(lhs.sse_row0, lhs.sse_row1, lhs.sse_row2, lhs.sse_row3);
	return lhs;
#else //#if DESHI_USE_SSE
	return Mat4(lhs.arr[0], lhs.arr[4], lhs.arr[ 8], lhs.arr[12],
				lhs.arr[1], lhs.arr[5], lhs.arr[ 9], lhs.arr[13],
				lhs.arr[2], lhs.arr[6], lhs.arr[10], lhs.arr[14],
				lhs.arr[3], lhs.arr[7], lhs.arr[11], lhs.arr[15]);
#endif //#else //#if DESHI_USE_SSE
}

#ifdef __cplusplus
inline mat4 mat4::
transpose()const{DPZoneScoped;
	#if DESHI_USE_SSE
	mat4 result = *this;
	_MM_TRANSPOSE4_PS(result.sse_row0, result.sse_row1, result.sse_row2, result.sse_row3);
	return result;
#else //#if DESHI_USE_SSE
	return Mat4(this->arr[0], this->arr[4], this->arr[ 8], this->arr[12],
				this->arr[1], this->arr[5], this->arr[ 9], this->arr[13],
				this->arr[2], this->arr[6], this->arr[10], this->arr[14],
				this->arr[3], this->arr[7], this->arr[11], this->arr[15]);
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline f32
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

EXTERN_C inline f32
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

EXTERN_C inline f32
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
inline f32 mat3::
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

EXTERN_C inline mat4
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
inline mat3 mat3::
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

EXTERN_C inline mat4
mat4_inverse(mat4 lhs){DPZoneScoped;
#if DESHI_USE_SSE
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
#else //#if DESHI_USE_SSE
	return mat4_div_f32(mat4_adjoint(lhs), mat4_determinant(lhs));
#endif //#else //#if DESHI_USE_SSE
}

#ifdef __cplusplus
inline mat4 mat4::
inverse()const{DPZoneScoped;
	#if DESHI_USE_SSE
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
#else //#if DESHI_USE_SSE
	return this->adjoint() / this->determinant();
#endif //#else //#if DESHI_USE_SSE
}
#endif //#ifdef __cplusplus

EXTERN_C inline mat4
mat4_inverse_transformation_matrix(mat4 lhs){
	//!ref: https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html
	mat4 result;
	
	//transpose scaled rotation matrix
	__m128 temp0 = m128_shuffle_0101(lhs.sse_row0, lhs.sse_row1);
	__m128 temp1 = m128_shuffle_2323(lhs.sse_row0, lhs.sse_row1);
	result.sse_row0 = m128_shuffle(temp0, lhs.sse_row2, 0,2,0,3);
	result.sse_row1 = m128_shuffle(temp0, lhs.sse_row2, 1,3,1,3);
	result.sse_row2 = m128_shuffle(temp1, lhs.sse_row2, 0,2,2,3);
	
	//extract the recipricol squared scale (1/|x|^2, 1/|y|^2, 1/|z|^2, 0)
	__m128 rSizeSq;
	rSizeSq =                       m128_mul_4f32(result.sse_row0, result.sse_row0);
	rSizeSq = m128_add_4f32(sizeSq, m128_mul_4f32(result.sse_row1, result.sse_row1));
	rSizeSq = m128_add_4f32(sizeSq, m128_mul_4f32(result.sse_row2, result.sse_row2));
	rSizeSq = m128_div_4f32(m128_fill_4f32(1.0f), sizeSq);
	
	//divide by squared scale
	result.sse_row0 = m128_mul_4f32(result.sse_row0, rSizeSq);
	result.sse_row1 = m128_mul_4f32(result.sse_row1, rSizeSq);
	result.sse_row2 = m128_mul_4f32(result.sse_row2, rSizeSq);
	
	//dot product against the negative translation
	result.sse_row3 =                                m128_mul_4f32(result.sse_row0, m128_swizzle(lhs.sse_row3, 0,0,0,0));
	result.sse_row3 = m128_add_4f32(result.sse_row3, m128_mul_4f32(result.sse_row1, m128_swizzle(lhs.sse_row3, 1,1,1,1)));
	result.sse_row3 = m128_add_4f32(result.sse_row3, m128_mul_4f32(result.sse_row2, m128_swizzle(lhs.sse_row3, 2,2,2,2)));
	result.sse_row3 = m128_sub_4f32(m128_set_4f32(0.0f, 0.0f, 0.0f, 1.0f), result.sse_row3);
	
	return resuult;
}

EXTERN_C inline mat4
mat4_inverse_transformation_matrix_no_scale(mat4 lhs){
	//!ref: https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html
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
	
	return resuult;
}

//returns a LH rotation transformation matrix based on input in radians
EXTERN_C inline mat4
mat4_rotation_matrix_x_radians(f32 angle){DPZoneScoped;
	f32 c = cosf(angle); f32 s = sinf(angle);
	return Mat4(1, 0, 0, 0,
				0, c, s, 0,
				0,-s, c, 0,
				0, 0, 0, 1);
}

//returns a LH rotation transformation matrix based on input in degrees
EXTERN_C inline mat4
mat4_rotation_matrix_x_degrees(f32 angle){DPZoneScoped;
	angle = Radians(angle);
	f32 c = cosf(angle); f32 s = sinf(angle);
	return Mat4(1, 0, 0, 0,
				0, c, s, 0,
				0,-s, c, 0,
				0, 0, 0, 1);
}

//returns a LH rotation transformation matrix based on input in radians
EXTERN_C inline mat4
mat4_rotation_matrix_y_radians(f32 angle){DPZoneScoped;
	f32 c = cosf(angle); f32 s = sinf(angle);
	return Mat4(c, 0,-s, 0,
				0, 1, 0, 0,
				s, 0, c, 0,
				0, 0, 0, 1);
}

//returns a LH rotation transformation matrix based on input in degrees
EXTERN_C inline mat4
mat4_rotation_matrix_y_degrees(f32 angle){DPZoneScoped;
	angle = Radians(angle);
	f32 c = cosf(angle); f32 s = sinf(angle);
	return Mat4(c, 0,-s, 0,
				0, 1, 0, 0,
				s, 0, c, 0,
				0, 0, 0, 1);
}

//returns a LH rotation transformation matrix based on input in radians
EXTERN_C inline mat4
mat4_rotation_matrix_z_radians(f32 angle){DPZoneScoped;
	f32 c = cosf(angle); f32 s = sinf(angle);
	return Mat4( c, s, 0, 0,
				-s, c, 0, 0,
				0,  0, 1, 0,
				0,  0, 0, 1);
}

//returns a LH rotation transformation matrix based on input in degrees
EXTERN_C inline mat4
mat4_rotation_matrix_z_degrees(f32 angle){DPZoneScoped;
	angle = Radians(angle);
	f32 c = cosf(angle); f32 s = sinf(angle);
	return Mat4( c, s, 0, 0,
				-s, c, 0, 0,
				0,  0, 1, 0,
				0,  0, 0, 1);
}

//returns a pre-multiplied X->Y->Z LH rotation transformation matrix based on input in radians
EXTERN_C inline mat4
mat4_rotation_matrix_radians(f32 x, f32 y, f32 z){DPZoneScoped;
	f32 cX = cosf(x); f32 sX = sinf(x);
	f32 cY = cosf(y); f32 sY = sinf(y);
	f32 cZ = cosf(z); f32 sZ = sinf(z);
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

//returns a pre-multiplied X->Y->Z LH rotation transformation matrix based on input in degrees
EXTERN_C inline mat4
mat4_rotation_matrix_degrees(f32 x, f32 y, f32 z){DPZoneScoped;
	x = Radians(x); y = Radians(y); z = Radians(z);
	f32 cX = cosf(x); f32 sX = sinf(x);
	f32 cY = cosf(y); f32 sY = sinf(y);
	f32 cZ = cosf(z); f32 sZ = sinf(z);
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

EXTERN_C inline mat4
mat4_translation_matrix(f32 x, f32 y, f32 z){
	return Mat4(1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				x, y, z, 1);
}

EXTERN_C inline mat4
mat4_scale_matrix(f32 x, f32 y, f32 z){
	return Mat4(x, 0, 0, 0,
				0, y, 0, 0,
				0, 0, z, 0,
				0, 0, 0, 1);
}

EXTERN_C inline mat4
mat4_transformation_matrix_radians(f32 translation_x, f32 translation_y, f32 translation_z, f32 rotation_x, f32 rotation_y, f32 rotation_z, f32 scale_x, f32 scale_y, f32 scale_z){
	f32 cX = cosf(rotation_x); f32 sX = sinf(rotation_x);
	f32 cY = cosf(rotation_y); f32 sY = sinf(rotation_y);
	f32 cZ = cosf(rotation_z); f32 sZ = sinf(rotation_z);
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

EXTERN_C inline mat4
mat4_transformation_matrix_degrees(f32 translation_x, f32 translation_y, f32 translation_z, f32 rotation_x, f32 rotation_y, f32 rotation_z, f32 scale_x, f32 scale_y, f32 scale_z){
	rotation_x = Radians(rotation_x); rotation_y = Radians(rotation_y); rotation_z = Radians(rotation_z);
	f32 cX = cosf(rotation_x); f32 sX = sinf(rotation_x);
	f32 cY = cosf(rotation_y); f32 sY = sinf(rotation_y);
	f32 cZ = cosf(rotation_z); f32 sZ = sinf(rotation_z);
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


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @mat_conversions


EXTERN_C inline mat4
mat3_to_mat4(mat3 lhs){
	return Mat4(lhs.arr[0], lhs.arr[1], lhs.arr[2], 0,
				lhs.arr[3], lhs.arr[4], lhs.arr[5], 0,
				lhs.arr[6], lhs.arr[7], lhs.arr[8], 0,
				0,          0,          0,          1);
}



//~////////////////////////////////////////////////////////////////////////////////////////////////
// @mat_vec_interactions


EXTERN_C inline mat3
vec3_rows_to_mat3(vec3 row0, vec3 row1, vec3 row2){
	return mat3{row0.x, row0.y, row0.z, row1.x, row1.y, row1.z, row2.x, row2.y, row2.z};
}

inline vec3 vec3::
operator* (const mat3& rhs) const{
	return Vec3(x*rhs.arr[0] + y*rhs.arr[3] + z*rhs.arr[6],
				x*rhs.arr[1] + y*rhs.arr[4] + z*rhs.arr[7],
				x*rhs.arr[2] + y*rhs.arr[5] + z*rhs.arr[8]);
}

inline void vec3::
operator*=(const mat3& rhs){
	*this = Vec3(x*rhs.arr[0] + y*rhs.arr[3] + z*rhs.arr[6],
				 x*rhs.arr[1] + y*rhs.arr[4] + z*rhs.arr[7],
				 x*rhs.arr[2] + y*rhs.arr[5] + z*rhs.arr[8]);
}

inline vec3 vec3::
operator* (const mat4& rhs) const{
	vec3 result;
#if DESHI_USE_SSE
	vec4 temp = Vec4(x, y, z, 0);
	temp.sse = m128_linear_combine(temp.sse, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.x = temp.x;
	result.y = temp.y;
	result.z = temp.z;
#else //#if DESHI_USE_SSE
	result.x = x*rhs.arr[0] + y*rhs.arr[4] + z*rhs.arr[8]  + rhs.arr[12];
	result.y = x*rhs.arr[1] + y*rhs.arr[5] + z*rhs.arr[9]  + rhs.arr[13];
	result.z = x*rhs.arr[2] + y*rhs.arr[6] + z*rhs.arr[10] + rhs.arr[14];
#endif //#else //#if DESHI_USE_SSE
	return result;
}

inline void vec3::
operator*=(const mat4& rhs){
	vec3 result;
#if DESHI_USE_SSE
	vec4 temp = Vec4(x, y, z, 0);
	temp.sse = m128_linear_combine(temp.sse, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.x = temp.x;
	result.y = temp.y;
	result.z = temp.z;
#else //#if DESHI_USE_SSE
	result.x = x*rhs.arr[0] + y*rhs.arr[4] + z*rhs.arr[8]  + rhs.arr[12];
	result.y = x*rhs.arr[1] + y*rhs.arr[5] + z*rhs.arr[9]  + rhs.arr[13];
	result.z = x*rhs.arr[2] + y*rhs.arr[6] + z*rhs.arr[10] + rhs.arr[14];
#endif //#else //#if DESHI_USE_SSE
	*this = result;
}


//////////////
//// vec4 ////
//////////////
inline vec4 vec4::
operator* (const mat4& rhs) const{
	vec4 result;
#if DESHI_USE_SSE
	result.sse = m128_linear_combine(sse, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else
	result.x = x*rhs.arr[0] + y*rhs.arr[4] + z*rhs.arr[8]  + w*rhs.arr[12];
	result.y = x*rhs.arr[1] + y*rhs.arr[5] + z*rhs.arr[9]  + w*rhs.arr[13];
	result.z = x*rhs.arr[2] + y*rhs.arr[6] + z*rhs.arr[10] + w*rhs.arr[14];
	result.w = x*rhs.arr[3] + y*rhs.arr[7] + z*rhs.arr[11] + w*rhs.arr[15];
#endif
	return result;
}

inline void vec4::
operator*=(const mat4& rhs){
#if DESHI_USE_SSE
	sse = m128_linear_combine(sse, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else
	vec4 result;
	result.x = x*rhs.arr[0] + y*rhs.arr[4] + z*rhs.arr[8]  + w*rhs.arr[12];
	result.y = x*rhs.arr[1] + y*rhs.arr[5] + z*rhs.arr[9]  + w*rhs.arr[13];
	result.z = x*rhs.arr[2] + y*rhs.arr[6] + z*rhs.arr[10] + w*rhs.arr[14];
	result.w = x*rhs.arr[3] + y*rhs.arr[7] + z*rhs.arr[11] + w*rhs.arr[15];
	*this = result;
#endif
}

inline vec3 mat3::
row(u32 row){
	Assert(row < 3, "mat3 subscript out of bounds");
	return Vec3(arr[3*row+0], arr[3*row+1], arr[3*row+2]);
}

inline vec3 mat3::
col(u32 col){
	Assert(col < 3, "mat3 subscript out of bounds");
	return Vec3(arr[col], arr[3+col], arr[6+col]);
}

//returns a pre-multiplied X->Y->Z LH rotation transformation matrix based on input in degrees
inline mat3 mat3::
RotationMatrix(vec3 rotation){
	rotation = Radians(rotation);
	float cX = cosf(rotation.x); float sX = sinf(rotation.x);
	float cY = cosf(rotation.y); float sY = sinf(rotation.y);
	float cZ = cosf(rotation.z); float sZ = sinf(rotation.z);
	float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
	float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
	float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
	return mat3(r00, r01, r02,
				r10, r11, r12,
				r20, r21, r22);
}

//returns a scale matrix where (0,0) = scale.x, (1,1) = scale.y, (2,2) = scale.z
inline mat3 mat3::
ScaleMatrix(vec3 scale){
	return mat3(scale.x, 0, 0,
				0, scale.y, 0,
				0, 0, scale.z);
}


//////////////
//// mat4 ////
//////////////
inline vec4 mat4::
row(u32 row){
	Assert(row < 4, "mat4 subscript out of bounds");
	return Vec4(arr[4*row+0], arr[4*row+1], arr[4*row+2], arr[4*row+3]);
}

inline vec4 mat4::
col(u32 col){
	Assert(col < 4, "mat4 subscript out of bounds");
	return Vec4(arr[col], arr[4+col], arr[8+col], arr[12+col]);
}

inline vec3 mat4::
Translation(){
	return Vec3(arr[12], arr[13], arr[14]);
}

//returns a pre-multiplied X->Y->Z LH rotation transformation matrix based on input in degrees
inline mat4 mat4::
RotationMatrix(vec3 rotation){
	rotation = Radians(rotation);
	float cX = cosf(rotation.x); float sX = sinf(rotation.x);
	float cY = cosf(rotation.y); float sY = sinf(rotation.y);
	float cZ = cosf(rotation.z); float sZ = sinf(rotation.z);
	float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
	float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
	float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
	return mat4(r00, r01, r02, 0,
				r10, r11, r12, 0,
				r20, r21, r22, 0,
				0,   0,   0,   1);
}

//https://github.com/microsoft/DirectXMath/blob/7c30ba5932e081ca4d64ba4abb8a8986a7444ec9/Inc/DirectXMathMatrix.inl
//line 1675 i do not know how this works but it does so :D
//return a matrix to rotate a vector around an arbitrary axis by some angle
//TODO(sushi, Ma) redo this function, I think its completely wrong around any axis thats not a world axis
inline mat4 mat4::
AxisAngleRotationMatrix(float angle, vec4 axis){
	angle = Radians(angle);
	float mag = axis.mag();
	axis = Vec4(axis.x / mag, axis.y / mag, axis.z / mag, axis.w / mag);
	//axis.normalize();
	float c = cosf(angle); float s = sinf(angle);
	
	vec4 A = Vec4(s, c, 1 - c, 0);
	
	vec4 C2 = Vec4(A.z, A.z, A.z, A.z);
	vec4 C1 = Vec4(A.y, A.y, A.y, A.y);
	vec4 C0 = Vec4(A.x, A.x, A.x, A.x);
	
	vec4 N0 = Vec4(axis.y, axis.z, axis.x, axis.w);
	vec4 N1 = Vec4(axis.z, axis.x, axis.y, axis.w);
	
	vec4 V0 = C2 * N0;
	V0 *= N1;
	
	vec4 R0 = C2 * axis;
	R0 = R0 * axis + C1;
	
	vec4 R1 = C0 * axis + V0;
	vec4 R2 = (V0 - C0) * axis;
	
	V0 = Vec4(R0.x, R0.y, R0.z, A.w);
	vec4 V1 = Vec4(R1.z, R2.y, R2.z, R1.x);
	vec4 V2 = Vec4(R1.y, R2.x, R1.y, R2.x);
	
	return mat4(V0.x, V1.x, V1.y, V0.w,
				V1.z, V0.y, V1.w, V0.w,
				V2.x, V2.y, V0.z, V0.w,
				0,    0,    0,    1);
}

//returns a transformation matrix of the combined translation, rotation, and scale matrices from input vectors
//rotates over the Y, then Z then X, ref: https://www.euclideanspace.com/maths/geometry/affine/aroundPoint/index.htm
inline mat4 mat4::RotationMatrixAroundPoint(vec3 pivot, vec3 rotation){
	//pivot = -pivot; //gotta negate this for some reason :)
	rotation = Radians(rotation);
	float cX = cosf(rotation.x); float sX = sinf(rotation.x);
	float cY = cosf(rotation.y); float sY = sinf(rotation.y);
	float cZ = cosf(rotation.z); float sZ = sinf(rotation.z);
	float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
	float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
	float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
	
	return mat4(r00, r01, r02, 0,
				r10, r11, r12, 0,
				r20, r21, r22, 0,
				pivot.x - r00*pivot.x - r01*pivot.y - r02*pivot.z, pivot.y - r10*pivot.x - r11*pivot.y - r12*pivot.z, pivot.z - r20*pivot.x - r21*pivot.y - r22*pivot.z, 1);
}

//returns a translation matrix where (3,0) = translation.x, (3,1) = translation.y, (3,2) = translation.z
inline mat4 mat4::
TranslationMatrix(vec3 translation){
	return mat4::TranslationMatrix(translation.x, translation.y, translation.z);
}

//returns a scale matrix where (0,0) = scale.x, (1,1) = scale.y, (2,2) = scale.z
inline mat4 mat4::
ScaleMatrix(vec3 scale){
	return mat4::ScaleMatrix(scale.x, scale.y, scale.z);
}

//returns a transformation matrix of the combined translation, rotation, and scale matrices from input vectors
inline mat4 mat4::
TransformationMatrix(vec3 tr, vec3 rot, vec3 scale){
	rot = Radians(rot);
	float cX = cosf(rot.x); float sX = sinf(rot.x);
	float cY = cosf(rot.y); float sY = sinf(rot.y);
	float cZ = cosf(rot.z); float sZ = sinf(rot.z);
	float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
	float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
	float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
	return mat4(scale.x*r00, scale.x*r01, scale.x*r02, 0,
				scale.y*r10, scale.y*r11, scale.y*r12, 0,
				scale.z*r20, scale.z*r21, scale.z*r22, 0,
				tr.x,        tr.y,        tr.z, 1);
}

//returns euler angles from a rotation matrix
//TODO(sushi, Ma) confirm that this works at some point
inline vec3 mat4::
Rotation(){
	if((*this)(0,2) < 1){
		if((*this)(0,2) > -1){
			return -Vec3(Degrees(atan2(-(*this)(1,2), (*this)(2,2))), Degrees(asin((*this)(0,2))), Degrees(atan2(-(*this)(0,1), (*this)(0,0))));
		}else{
			return -Vec3(Degrees(-atan2((*this)(1,0), (*this)(1,1))), Degrees(-M_HALFPI), 0);
		}
	}else{
		return -Vec3(Degrees(atan2((*this)(1,0), (*this)(1,1))), Degrees(M_HALFPI), 0);
	}
	
	
}

#define F_AVG(i, f) ([&]{                      \
persist std::vector<float> floats;           \
persist float nf;                            \
persist int iter = 0;                        \
if(i == floats.size()){                      \
floats.erase(floats.begin());              \
floats.push_back(f);                       \
iter++;                                    \
}else{                                       \
floats.push_back(f);                       \
iter++;                                    \
}                                            \
if(iter == i){                               \
nf = Math::average(floats, floats.size()); \
iter = 0;                                  \
}                                            \
return nf;                                   \
}())

//////////////
//// math ////
//////////////
namespace Math {
	
	constexpr global s32 pow(s32 base, u32 exp){
		int result = 1;
		for(;;){
			if(exp & 1) result *= base;
			exp >>= 1;
			if(exp == 0) break;
			base *= base;
		}
		return result;
	}
	
	//rounding
	template<int decimals = 2> inline global f32 round(f32 a){
		constexpr f32 multiple = f32(pow(10,decimals));
		return f32(s32(a * multiple + .5f)) / multiple;
	}
	
	template<int decimals = 2> inline global vec3 round(vec3 a){
		constexpr f32 multiple = f32(pow(10,decimals));
		return Vec3(f32(s32(a.x * multiple + .5f)) / multiple,
					f32(s32(a.y * multiple + .5f)) / multiple,
					f32(s32(a.z * multiple + .5f)) / multiple);
	}
	
	global s32 order_of_magnitude(f32 in){DPZoneScoped;
		if(in==0) return 0;
		if(floor(abs(in))==1) return 0;
		if(ceil(abs(in))==1) return -1;
		f32 absin = in;
		if(absin<0) absin=-absin;
		s32 order = 0;
		if(absin > 1){
			while(ceil(absin/10)!=1) order++, absin/=10;
			return order;
		}
		else{
			while(floor(absin*10)!=1) order--, absin*=10;
			return order - 1;
		}
	}
	
	global s32 order_of_magnitude(f64 in){DPZoneScoped;
		if(in==0) return 0;
		if(floor(in)==1) return 0;
		f64 absin = in;
		if(absin<0) absin=-absin;
		s32 order = 0;
		if(absin > 1){
			while(absin > 1) order++, absin/=10;
			return order - 1;
		}
		else{
			while(absin < 1) order--, absin*=10;
			return order;
		}
	}
	
	template<class FWIt> static float average(FWIt a, const FWIt b, int size){ return std::accumulate(a, b, 0.0) / size; }
	template<class T> static f64 average(const T& container, int size){ return average(std::begin(container), std::end(container), size); }
	
	//interpolating
	template<typename T> global T lerp(T a, T b, f32 t){ return a*(1.0f-t) + b*t; }
	
	//returns how far along a polynomial fit for a set of data you are
	//you are not allowed to have 2 points with the same x value here
	//using Lagrange Polynomials
	//TODO(sushi, Ma) look into maybe implementing Newton's Polynomials at some point idk if they're better but they look more simple
	static float PolynomialCurveInterpolation(std::vector<vec2> vs, float t){
		float sum = 0;
		for (int j = 0; j < vs.size(); j++){
			float vy = vs[j].y;
			float jx = vs[j].x;
			float lbp = 1;
			for (int m = 0; m < vs.size(); m++){
				if(lbp != 0){
					if(m != j){
						float mx = vs[m].x;
						lbp *= (t - mx) / (jx - mx);
					}
				}else break;
			}
			sum += vy * lbp;
		}
		return sum;
	}
	
	static vec2 vec2RotateByAngle(float angle, vec2 v){
		if (!angle) return v;
		angle = Radians(angle);
		return Vec2(v.x * cosf(angle) - v.y * sinf(angle), v.x * sin(angle) + v.y * cos(angle));
	}
	
	inline global bool PointInRectangle(vec2 point, vec2 rectPos, vec2 rectDims){
		return
			point.x >= rectPos.x &&
			point.y >= rectPos.y &&
			point.x <= rectPos.x + rectDims.x &&
			point.y <= rectPos.y + rectDims.y;
	}
	
	inline global b32 PointInTriangle(vec2 point, vec2 p0, vec2 p1, vec2 p2) {
		vec2 p01 = p1 - p0;
		vec2 p12 = p2 - p1;
		vec2 p20 = p0 - p2;
		
		b32 b0 = (point - p0).dot(-Vec2(p01.y, -p01.x)) < 0;
		b32 b1 = (point - p1).dot(-Vec2(p12.y, -p12.x)) < 0;
		b32 b2 = (point - p2).dot(-Vec2(p20.y, -p20.x)) < 0;
		
		return b0==b1 && b1==b2;
	}
	
#define BoundTimeOsc(x, y) Math::BoundedOscillation(x, y, DeshTotalTime/1000)
	
	//oscillates between a given upper and lower value based on a given x value
	inline global float BoundedOscillation(float lower, float upper, float x){
		Assert(upper > lower);
		return ((upper - lower) * cosf(x) + (upper + lower)) / 2;
	}
	
	//returns in degrees
	//this doesn't really work in 3D but this function is here anyways
	static float AngBetweenVectors(vec3 v1, vec3 v2){
		return Degrees(acosf(v1.dot(v2) / (v1.mag() * v2.mag())));
	}
	
	//returns in degrees
	static float AngBetweenVectors(vec2 v1, vec2 v2){
		return Degrees(atan2(v1.x * v2.y - v1.y * v2.x, v1.dot(v2)));
	}
	
	//returns in degrees between 0 and 360
	static float AngBetweenVectors360(vec2 v1, vec2 v2){
		float ang = Degrees(atan2(v1.x * v2.y - v1.y * v2.x, v1.dot(v2)));
		return (ang < 0) ? 360 + ang : ang;
	}
	
	//NOTE 0-1 depth range
	static mat4 PerspectiveProjectionMatrix(f32 width, f32 height, f32 hFOV, f32 nearZ, f32 farZ){
		float renderDistance = farZ - nearZ;
		float aspectRatio = (f32)height / (f32)width;
		float fovRad = 1.0f / tanf(Radians(hFOV / 2.0f));
		return mat4(aspectRatio * fovRad, 0,	   0,							  0,
					0,					fovRad,  0,							  0,
					0,					0,	   farZ / renderDistance,		  1,
					0,					0,	   -(farZ*nearZ) / renderDistance, 0);
	}
	
	//this function returns a matrix that tells a vector how to look at a specific point in space.
	static mat4 LookAtMatrix(const vec3& pos, const vec3& target, vec3* out_up = 0){
		if(pos == target){ return LookAtMatrix(pos, target + Vec3(.01f, 0, 0)); }
		
		//get new forward direction
		vec3 newFor = (target - pos).normalized();
		
		//get right direction
		vec3 newRight;
		if(newFor == vec3::UP || newFor == vec3::DOWN){
			newRight = vec3::RIGHT;
		}else{
			newRight = (vec3::UP.cross(newFor)).normalized();
		}
		
		//get up direction
		vec3 newUp = newFor.cross(newRight);
		if(out_up) *out_up = newUp;
		
		//make look-at matrix
		return mat4(newRight.x, newRight.y, newRight.z, 0,
					newUp.x,    newUp.y,    newUp.z,    0,
					newFor.x,   newFor.y,   newFor.z,   0,
					pos.x,      pos.y,      pos.z,      1);
	}
	
	//this assumes its in degrees
	static vec3 SphericalToRectangularCoords(vec3 v){
		float y = Radians(v.y);
		float z = Radians(v.z);
		return Vec3(v.x * sinf(z) * cosf(y), v.x * cosf(z), v.x * sinf(z) * sinf(y));
	}
	
	static vec3 RectangularToSphericalCoords(vec3 v){
		float rho = Radians(sqrtf(v.mag()));
		float theta = Radians(atan(v.y / v.z));
		float phi = acos(v.z / v.mag()); //maybe use v.y instead of v.z because y is our vertical axis
		return Vec3(rho, theta, phi);
		
	}
	
	static float DistTwoPoints(vec3 a, vec3 b){
		return sqrtf((a.x - b.x) * (a.x - b.x) +
					 (a.y - b.y) * (a.y - b.y) +
					 (a.z - b.z) * (a.z - b.z));
	}
	
	static inline float DistPointToPlane(vec3 point, vec3 plane_n, vec3 plane_p){
		return (point - plane_p).dot(plane_n);
	}
	
	//where a line intersects with a plane, 'returns' how far along line you were as t value
	static vec3 VectorPlaneIntersect(vec3 plane_p, vec3 plane_n, vec3 line_start, vec3 line_end, float* out_t = 0){
		vec3 lstole = (line_end - line_start).normalized();
		vec3 lptopp = plane_p - line_start;
		f32 t = lptopp.dot(plane_n) / lstole.dot(plane_n);
		if(out_t) *out_t = t;
		return line_start + t * lstole;
	}
	
	//TODO(sushi) figure out how this worked
	//returns where two lines intersect on the x axis with slope and the y-intercept
	//static vec2 LineIntersect2(float slope1, float ycross1, float slope2, float ycross2){
	//	matN lhs(2,2,{ slope1, ycross1, slope2, ycross2 });
	//	matN rhs(2,1,{ 1, 1 });
	//	matN det = lhs.Inverse() * rhs;
	//	float x = 1 / det(1,0) * det(0,0);
	//	float y = slope1 * x + ycross1;
	//
	//	f32 x = (ycross1 - ycross2 + slope)
	//	return vec2(x, y);
	//}
	
	static vec2 LineIntersect2(vec2 p1, vec2 p2, vec2 p3, vec2 p4) {
		f32 m1 = (p2.y - p1.y) / (p2.x - p1.x);
		f32 m2 = (p4.y - p3.y) / (p4.x - p3.x);
		f32 b1 = p2.y - m1 * p2.x;
		f32 b2 = p4.y - m2 * p4.x;
		f32 x = (b2 - b1) / (m1 - m2);
		f32 y = m1 * x + b1;
		return{ x,y };
		
		
	}
	
	static vec3 LineMidpoint(vec3 start, vec3 end){
		return (start+end)/2.0f;
	}
	
	//the input vectors should be in viewMat/camera space
	//returns true if the line can be rendered after clipping, false otherwise
	static bool ClipLineToZPlanes(vec3& start, vec3& end, f32 nearZ, f32 farZ){
		//clip to the near plane
		vec3 planePoint = Vec3(0, 0, nearZ);
		vec3 planeNormal = vec3::FORWARD;
		float d = planeNormal.dot(planePoint);
		bool startBeyondPlane = planeNormal.dot(start) - d < 0;
		bool endBeyondPlane = planeNormal.dot(end) - d < 0;
		float t;
		if(startBeyondPlane && !endBeyondPlane){
			start = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, &t);
		}else if(!startBeyondPlane && endBeyondPlane){
			end = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, &t);
		}else if(startBeyondPlane && endBeyondPlane){
			return false;
		}
		
		//clip to the far plane
		planePoint = Vec3(0, 0, farZ);
		planeNormal = vec3::BACK;
		d = planeNormal.dot(planePoint);
		startBeyondPlane = planeNormal.dot(start) - d < 0;
		endBeyondPlane = planeNormal.dot(end) - d < 0;
		if(startBeyondPlane && !endBeyondPlane){
			start = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, &t);
		}else if(!startBeyondPlane && endBeyondPlane){
			end = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, &t);
		}else if(startBeyondPlane && endBeyondPlane){
			return false;
		}
		return true;
	} //ClipLineToZPlanes
	
	//cohen-sutherland algorithm https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm
	//the input vectors should be in screen space
	//returns true if the line can be rendered after clipping, false otherwise
	static bool ClipLineToBorderPlanes(vec2& start, vec2& end, vec2 dimensions){
		//clip to the vertical and horizontal planes
		const int CLIP_INSIDE = 0;
		const int CLIP_LEFT = 1;
		const int CLIP_RIGHT = 2;
		const int CLIP_BOTTOM = 4;
		const int CLIP_TOP = 8;
		auto ComputeOutCode = [&](vec2& vertex){
			int code = CLIP_INSIDE;
			if(vertex.x < 0){
				code |= CLIP_LEFT;
			}else if(vertex.x > dimensions.x){
				code |= CLIP_RIGHT;
			}
			if(vertex.y < 0){ //these are inverted because we are in screen space
				code |= CLIP_TOP;
			}else if(vertex.y > dimensions.y){
				code |= CLIP_BOTTOM;
			}
			return code;
		};
		
		int lineStartCode = ComputeOutCode(start);
		int lineEndCode = ComputeOutCode(end);
		
		//loop until all points are within or outside the screen zone
		while (true){
			if(!(lineStartCode | lineEndCode)){
				//both points are inside the screen zone
				return true;
			}else if(lineStartCode & lineEndCode){
				//both points are in the same outside zone
				return false;
			}else{
				float x = 0, y = 0;
				//select one of the points outside
				int code = lineEndCode > lineStartCode ? lineEndCode : lineStartCode;
				
				//clip the points the the screen bounds by finding the intersection point
				if      (code & CLIP_TOP){    //point is above screen
					x = start.x + (end.x - start.x) * (-start.y) / (end.y - start.y);
					y = 0;
				}else if(code & CLIP_BOTTOM){ //point is below screen
					x = start.x + (end.x - start.x) * (dimensions.y - start.y) / (end.y - start.y);
					y = dimensions.y;
				}else if(code & CLIP_RIGHT){  //point is right of screen
					y = start.y + (end.y - start.y) * (dimensions.x - start.x) / (end.x - start.x);
					x = dimensions.x;
				}else if(code & CLIP_LEFT){   //point is left of screen
					y = start.y + (end.y - start.y) * (-start.x) / (end.x - start.x);
					x = 0;
				}
				
				//update the vector's points and restart loop
				if(code == lineStartCode){
					start.x = x;
					start.y = y;
					lineStartCode = ComputeOutCode(start);
				}else{
					end.x = x;
					end.y = y;
					lineEndCode = ComputeOutCode(end);
				}
			}
		}
	} //ClipLineToBorderPlanes
	
	//returns area of a triangle of sides a and b
	static float TriangleArea(vec3 a, vec3 b){
		return a.cross(b).mag() / 2.0f;
	}
	
	//The normal this returns heavily depends on how you give it the points
	static vec3 TriangleNormal(vec3 p1, vec3 p2, vec3 p3){
		return (p3 - p1).cross(p2 - p1).normalized();
	}
	
	static vec3 TriangleMidpoint(vec3 p1, vec3 p2, vec3 p3){
		return (p1 + p2 + p3) / 3.0f;
	}
	
	inline static vec4 ProjMult(vec4 v, const mat4& m){
		vec4 nv = v * m;
		Assert(nv.w != 0);
		nv.x /= nv.w;
		nv.y /= nv.w;
		nv.z /= nv.w;
		return nv;
	}
	
	static vec3 WorldToCamera3(vec3 vertex, const mat4& viewMat){
		return Math::ProjMult(vertex.toVec4(), viewMat).toVec3();
	}
	
	static vec4 WorldToCamera4(vec3 vertex, const mat4& viewMat){
		return Math::ProjMult(vertex.toVec4(), viewMat);
	}
	
	static vec3 CameraToWorld3(vec3 vertex, const mat4& viewMat){
		return Math::ProjMult(vertex.toVec4(), viewMat.Inverse()).toVec3();
	}
	
	static vec4 CameraToWorld4(vec3 vertex, const mat4& viewMat){
		return Math::ProjMult(vertex.toVec4(), viewMat.Inverse());
	}
	
	static vec2 CameraToScreen2(vec3 csVertex, const mat4& projMat, vec2 screenDimensions){
		vec3 vm = Math::ProjMult(csVertex.toVec4(), projMat).toVec3();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * screenDimensions.x;
		vm.y *= 0.5f * screenDimensions.y;
		return vm.toVec2();
	}
	
	static vec3 CameraToScreen3(vec3 csVertex, const mat4& projMat, vec2 screenDimensions){
		vec3 vm = Math::ProjMult(csVertex.toVec4(), projMat).toVec3();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * screenDimensions.x;
		vm.y *= 0.5f * screenDimensions.y;
		return vm;
	}
	
	static vec3 CameraToScreen3(vec3 csVertex, const mat4& projMat, vec2 screenDimensions, float& w){
		vec4 bleh = csVertex.toVec4() * projMat;
		w = bleh.w;
		vec3 vm = bleh.wnormalized().toVec3();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * screenDimensions.x;
		vm.y *= 0.5f * screenDimensions.y;
		return vm;
	}
	
	static vec3 CameraToScreen3(vec4 csVertex, const mat4& projMat, vec2 screenDimensions){
		vec3 vm = Math::ProjMult(csVertex, projMat).toVec3();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * screenDimensions.x;
		vm.y *= 0.5f * screenDimensions.y;
		return vm;
	}
	
	static vec4 CameraToScreen4(vec4 csVertex, const mat4& projMat, vec2 screenDimensions){
		vec4 vm = (csVertex * projMat).wnormalized();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * screenDimensions.x;
		vm.y *= 0.5f * screenDimensions.y;
		return vm;
	}
	
	static vec3 WorldToScreen(vec3 point, const mat4& ProjMat, const mat4& ViewMat, vec2 screenDimensions){
		return CameraToScreen3(WorldToCamera4(point, ViewMat), ProjMat, screenDimensions);
	}
	
	static vec2 WorldToScreen2(vec3 point, const mat4& ProjMat, const mat4& ViewMat, vec2 screenDimensions){
		vec3 v = CameraToScreen3(WorldToCamera4(point, ViewMat), ProjMat, screenDimensions);
		return Vec2(v.x, v.y);
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
};

#ifdef __cplusplus
}
#endif //#ifdef __cplusplus
#endif //#ifndef DESHI_MATH_H