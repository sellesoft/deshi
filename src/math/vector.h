/* deshi Vector Math Module
Index:
@vec2i
@vec2
@vec3i
@vec3
@vec4i
@vec4
@vec_interactions
@vec_macros
@vec_rounding
@vec_minmax
@vec_hashing
@vec_tostring
*/

#pragma once
#ifndef DESHI_VECTOR_H
#define DESHI_VECTOR_H

#include "math_utils.h"

struct vec2g; //graphing's vec2
struct vec2;
struct vec3;
struct vec4;
struct matN;
struct mat3;
struct mat4;
struct quat;

#ifdef __cplusplus
StartLinkageC();
#endif //#ifdef __cplusplus

//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec2i
typedef struct vec2i{
	//// data ////
	union{
		s32 arr[2] = {0};
		struct{ s32 x, y; };
		struct{ s32 r, g; };
		struct{ s32 w, h; };
		struct{ s32 u, v; };
	};
	
	
#ifdef __cplusplus
	//// member constants ////
	static const vec2i ZERO;
	static const vec2i ONE;
	static const vec2i UP;
	static const vec2i DOWN;
	static const vec2i LEFT;
	static const vec2i RIGHT;
	static const vec2i UNITX;
	static const vec2i UNITY;
	
	
	//// member operators ////
	inline vec2i operator+ (const vec2i& rhs)const{DPZoneScoped;
		vec2i v;
		v.x = this->x + rhs.x;
		v.y = this->y + rhs.y;
		return v;
	}
	
	inline void  operator+=(const vec2i& rhs){DPZoneScoped;
		this->x += rhs.x;
		this->y += rhs.y;
	}
	
	inline vec2i operator- (const vec2i& rhs)const{DPZoneScoped;
		vec2i v;
		v.x = this->x - rhs.x;
		v.y = this->y - rhs.y;
		return v;
	}
	
	inline void  operator-=(const vec2i& rhs){DPZoneScoped;
		this->x -= rhs.x;
		this->y -= rhs.y;
	}
	
	inline vec2i operator* (const vec2i& rhs)const{DPZoneScoped;
		vec2i v;
		v.x = this->x * rhs.x;
		v.y = this->y * rhs.y;
		return v;
	}
	
	inline void  operator*=(const vec2i& rhs){DPZoneScoped;
		this->x *= rhs.x;
		this->y *= rhs.y;
	}
	
	inline vec2i operator* (s32 rhs)const{DPZoneScoped;
		vec2i v;
		v.x = this->x * rhs;
		v.y = this->y * rhs;
		return v;
	}
	
	inline void  operator*=(s32 rhs){DPZoneScoped;
		this->x *= rhs;
		this->y *= rhs;
	}
	
	inline vec2i operator/ (const vec2i& rhs)const{DPZoneScoped;
		vec2i v;
		v.x = this->x / rhs.x;
		v.y = this->y / rhs.y;
		return v;
	}
	
	inline void  operator/=(const vec2i& rhs){DPZoneScoped;
		this->x /= rhs.x;
		this->y /= rhs.y;
	}
	
	inline vec2i operator/ (s32 rhs)const{DPZoneScoped;
		vec2i v;
		v.x = this->x / rhs;
		v.y = this->y / rhs;
		return v;
	}
	
	inline void  operator/=(s32 rhs){DPZoneScoped;
		this->x /= rhs;
		this->y /= rhs;
	}
	
	inline vec2i operator- ()const{DPZoneScoped;
		vec2i v;
		v.x = -(this->x);
		v.y = -(this->y);
		return v;
	}
	
	inline b32   operator==(const vec2i& rhs)const{DPZoneScoped;
		return (this->x == rhs.x) && (this->y == rhs.y);
	}
	
	inline b32   operator!=(const vec2i& rhs)const{DPZoneScoped;
		return !(*this == rhs);
	}
	
	
	//// member functions ////
	inline void  set(s32 x, s32 y){DPZoneScoped;
		this->x = x;
		this->y = y;
	}
	
	inline vec2i absV()const{DPZoneScoped;
		vec2i v;
		v.x = abs(this->x);
		v.y = abs(this->y);
		return v;
	}
	
	inline vec2i copy()const{DPZoneScoped;
		vec2i v;
		v.x = this->x;
		v.y = this->y;
		return v;
	}
	
	inline s32   dot(const vec2i& rhs)const{DPZoneScoped;
		return (this->x * rhs.x) + (this->y * rhs.y);
	}
	
	inline vec2i perp()const{DPZoneScoped;
		vec2i v;
		v.x = -(this->y);
		v.y =   this->x;
		return v;
	}
	
	inline s32   mag()const{DPZoneScoped;
		return sqrt((this->x * this->x) + (this->y * this->y));
	}
	
	inline s32   magSq()const{DPZoneScoped;
		return (this->x * this->x) + (this->y * this->y);
	}
	
	inline void  normalize(){DPZoneScoped;
		if(this->x != 0 || this->y != 0){
			*this /= this->mag();
		}
	}
	
	inline vec2i normalized()const{DPZoneScoped;
		if(this->x != 0 || this->y != 0){
			return *this / this->mag();
		}else{
			return *this;
		}
	}
	
	inline void  clampMag(s32 min, s32 max){DPZoneScoped;
		s32 m = this->mag();
		if      (m < min){
			this->normalize();
			*this *= min;
		}else if(m > max){
			this->normalize();
			*this *= max;
		}
	}
	
	inline vec2i clampedMag(s32 min, s32 max)const{DPZoneScoped;
		s32 m = this->mag();
		if      (m < min){
			return this->normalized() * min;
		}else if(m > max){
			return this->normalized() * max;
		}else{
			return *this;
		}
	}
	
	inline s32   distanceTo(const vec2i& rhs)const{DPZoneScoped;
		return (*this - rhs).mag();
	}
	
	inline s32   projectOn(const vec2i& rhs)const{DPZoneScoped;
		if(this->mag() != 0){
			return this->dot(rhs) / this->mag();
		}else{
			return 0;
		}
	}
	
	inline vec2i compOn(const vec2i& rhs)const{DPZoneScoped;
		return rhs.normalized() * this->projectOn(rhs);
	}
	
	inline vec2i midpoint(const vec2i& rhs)const{DPZoneScoped;
		vec2i v;
		v.x = (this->x + rhs.x) / 2;
		v.y = (this->y + rhs.y) / 2;
		return v;
	}
	
	inline vec2i xComp()const{DPZoneScoped;
		vec2i v;
		v.x = this->x;
		v.y = 0;
		return v;
	}
	
	inline vec2i yComp()const{DPZoneScoped;
		vec2i v;
		v.x = 0;
		v.y = this->y;
		return v;
	}
	
	inline vec2i xInvert()const{DPZoneScoped;
		vec2i v;
		v.x = -(this->x);
		v.y =   this->y;
		return v;
	}
	
	inline vec2i yInvert()const{DPZoneScoped;
		vec2i v;
		v.x =   this->x;
		v.y = -(this->y);
		return v;
	}
	
	inline vec2i xSet(s32 a)const{DPZoneScoped;
		vec2i v;
		v.x = a;
		v.y = this->y;
		return v;
	}
	
	inline vec2i ySet(s32 a)const{DPZoneScoped;
		vec2i v;
		v.x = this->x;
		v.y = a;
		return v;
	}
	
	inline vec2i xAdd(s32 a)const{DPZoneScoped;
		vec2i v;
		v.x = this->x + a;
		v.y = this->y;
		return v;
	}
	
	inline vec2i yAdd(s32 a)const{DPZoneScoped;
		vec2i v;
		v.x = this->x;
		v.y = this->y + a;
		return v;
	}
	
	//// member interactions ////
	vec2  toVec2()const;
	vec3  toVec3()const;
	vec4  toVec4()const;
#endif //#ifdef __cplusplus
} vec2i;


#ifdef __cplusplus
//// member constants ////
inline const vec2i vec2i::ZERO  = vec2i{ 0, 0};
inline const vec2i vec2i::ONE   = vec2i{ 1, 1};
inline const vec2i vec2i::UP    = vec2i{ 0, 1};
inline const vec2i vec2i::DOWN  = vec2i{ 0,-1};
inline const vec2i vec2i::LEFT  = vec2i{-1, 0};
inline const vec2i vec2i::RIGHT = vec2i{ 1, 0};
inline const vec2i vec2i::UNITX = vec2i{ 1, 0};
inline const vec2i vec2i::UNITY = vec2i{ 0, 1};
#endif //#ifdef __cplusplus


//// nonmember constructor ////
FORCE_INLINE vec2i Vec2i(s32 x, s32 y){DPZoneScoped;
	return vec2i{x, y};
}

//// nonmember constants ////
FORCE_INLINE vec2i vec2i_ZERO() { return vec2i{ 0, 0}; }
FORCE_INLINE vec2i vec2i_ONE()  { return vec2i{ 0, 0}; }
FORCE_INLINE vec2i vec2i_UP()   { return vec2i{ 0, 0}; }
FORCE_INLINE vec2i vec2i_DOWN() { return vec2i{ 0, 0}; }
FORCE_INLINE vec2i vec2i_LEFT() { return vec2i{ 0, 0}; }
FORCE_INLINE vec2i vec2i_RIGHT(){ return vec2i{ 0, 0}; }
FORCE_INLINE vec2i vec2i_UNITX(){ return vec2i{ 0, 0}; }
FORCE_INLINE vec2i vec2i_UNITY(){ return vec2i{ 0, 0}; }


//// nonmember operators ////
#ifdef __cplusplus
EndLinkageC();
FORCE_INLINE vec2i operator* (s32 lhs, vec2i rhs){
	return rhs * lhs;
}
StartLinkageC();
#endif //#ifdef __cplusplus


inline vec2i vec2i_add(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = lhs.x + rhs.x;
	v.y = lhs.y + rhs.y;
	return v;
}

inline vec2i vec2i_subtract(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = lhs.x - rhs.x;
	v.y = lhs.y - rhs.y;
	return v;
}

inline vec2i vec2i_multiply(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = lhs.x * rhs.x;
	v.y = lhs.y * rhs.y;
	return v;
}

inline vec2i vec2i_multiply_constant(vec2i lhs, s32 rhs){DPZoneScoped;
	vec2i v;
	v.x = lhs.x * rhs;
	v.y = lhs.y * rhs;
	return v;
}

inline vec2i vec2i_divide(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = lhs.x / rhs.x;
	v.y = lhs.y / rhs.y;
	return v;
}

inline vec2i vec2i_divide_constant(vec2i lhs, s32 rhs){DPZoneScoped;
	vec2i v;
	v.x = lhs.x / rhs;
	v.y = lhs.y / rhs;
	return v;
}

inline vec2i vec2i_negate(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = -(lhs.x);
	v.y = -(lhs.y);
	return v;
}

inline b32   vec2i_equal(vec2i lhs, vec2i rhs){DPZoneScoped;
	return (lhs.x == rhs.x) && (lhs.y == rhs.y);
}


//// nonmember functions ////
inline vec2i vec2i_absV(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = abs(lhs.x);
	v.y = abs(lhs.y);
	return v;
}

inline s32   vec2i_dot(vec2i lhs, vec2i rhs){DPZoneScoped;
	return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}

inline vec2i vec2i_perp(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = -(lhs.y);
	v.y =   lhs.x;
	return v;
}

inline s32   vec2i_mag(vec2i lhs){DPZoneScoped;
	return sqrt((lhs.x * lhs.x) + (lhs.y * lhs.y));
}

inline s32   vec2i_magSq(vec2i lhs){DPZoneScoped;
	return (lhs.x * lhs.x) + (lhs.y * lhs.y);
}

inline vec2i vec2i_normalized(vec2i lhs){DPZoneScoped;
	if(lhs.x != 0 || lhs.y != 0){
		return vec2i_divide_constant(lhs, vec2i_mag(lhs));
	}else{
		return lhs;
	}
}

inline vec2i vec2i_clampedMag(vec2i lhs, s32 min, s32 max){DPZoneScoped;
	s32 m = vec2i_mag(lhs);
	if      (m < min){
		return vec2i_multiply_constant(vec2i_normalized(lhs), min);
	}else if(m > max){
		return vec2i_multiply_constant(vec2i_normalized(lhs), max);
	}else{
		return lhs;
	}
}

inline s32   vec2i_distanceTo(vec2i lhs, vec2i rhs){DPZoneScoped;
	return vec2i_mag(vec2i_subtract(lhs,rhs));
}

inline s32   vec2i_projectOn(vec2i lhs, vec2i rhs){DPZoneScoped;
	s32 m = vec2i_mag(lhs);
	if(m != 0){
		return vec2i_dot(lhs,rhs) / m;
	}else{
		return 0;
	}
}

inline vec2i vec2i_compOn(vec2i lhs, vec2i rhs){DPZoneScoped;
	return vec2i_multiply_constant(vec2i_normalized(rhs), vec2i_projectOn(lhs,rhs));
}

inline vec2i vec2i_midpoint(vec2i lhs, vec2i rhs){DPZoneScoped;
	vec2i v;
	v.x = (lhs.x + rhs.x) / 2;
	v.y = (lhs.y + rhs.y) / 2;
	return v;
}

inline vec2i vec2i_xComp(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = lhs.x;
	v.y = 0;
	return v;
}

inline vec2i vec2i_yComp(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = 0;
	v.y = lhs.y;
	return v;
}

inline vec2i vec2i_xInvert(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x = -(lhs.x);
	v.y =   lhs.y;
	return v;
}

inline vec2i vec2i_yInvert(vec2i lhs){DPZoneScoped;
	vec2i v;
	v.x =   lhs.x;
	v.y = -(lhs.y);
	return v;
}

inline vec2i vec2i_xSet(vec2i lhs, s32 a){DPZoneScoped;
	vec2i v;
	v.x = a;
	v.y = lhs.y;
	return v;
}

inline vec2i vec2i_ySet(vec2i lhs, s32 a){DPZoneScoped;
	vec2i v;
	v.x = lhs.x;
	v.y = a;
	return v;
}

inline vec2i vec2i_xAdd(vec2i lhs, s32 a){DPZoneScoped;
	vec2i v;
	v.x = lhs.x + a;
	v.y = lhs.y;
	return v;
}

inline vec2i vec2i_yAdd(vec2i lhs, s32 a){DPZoneScoped;
	vec2i v;
	v.x = lhs.x;
	v.y = lhs.y + a;
	return v;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec2
typedef struct vec2{
	//// data ////
	union{
		f32 arr[2] = {};
		struct{ f32 x, y; };
		struct{ f32 r, g; };
		struct{ f32 w, h; };
		struct{ f32 u, v; };
	};
	
	
#ifdef __cplusplus
	//// member constants ////
	static const vec2 ZERO;
	static const vec2 ONE;
	static const vec2 UP;
	static const vec2 DOWN;
	static const vec2 LEFT;
	static const vec2 RIGHT;
	static const vec2 UNITX;
	static const vec2 UNITY;
	
	
	//// member operators ////
	inline vec2 operator+ (const vec2& rhs)const{DPZoneScoped;
		vec2 v;
		v.x = this->x + rhs.x;
		v.y = this->y + rhs.y;
		return v;
	}
	
	inline void  operator+=(const vec2& rhs){DPZoneScoped;
		this->x += rhs.x;
		this->y += rhs.y;
	}
	
	inline vec2 operator- (const vec2& rhs)const{DPZoneScoped;
		vec2 v;
		v.x = this->x - rhs.x;
		v.y = this->y - rhs.y;
		return v;
	}
	
	inline void  operator-=(const vec2& rhs){DPZoneScoped;
		this->x -= rhs.x;
		this->y -= rhs.y;
	}
	
	inline vec2 operator* (const vec2& rhs)const{DPZoneScoped;
		vec2 v;
		v.x = this->x * rhs.x;
		v.y = this->y * rhs.y;
		return v;
	}
	
	inline void  operator*=(const vec2& rhs){DPZoneScoped;
		this->x *= rhs.x;
		this->y *= rhs.y;
	}
	
	inline vec2 operator* (f32 rhs)const{DPZoneScoped;
		vec2 v;
		v.x = this->x * rhs;
		v.y = this->y * rhs;
		return v;
	}
	
	inline void  operator*=(f32 rhs){DPZoneScoped;
		this->x *= rhs;
		this->y *= rhs;
	}
	
	inline vec2 operator/ (const vec2& rhs)const{DPZoneScoped;
		vec2 v;
		v.x = this->x / rhs.x;
		v.y = this->y / rhs.y;
		return v;
	}
	
	inline void  operator/=(const vec2& rhs){DPZoneScoped;
		this->x /= rhs.x;
		this->y /= rhs.y;
	}
	
	inline vec2 operator/ (f32 rhs)const{DPZoneScoped;
		vec2 v;
		v.x = this->x / rhs;
		v.y = this->y / rhs;
		return v;
	}
	
	inline void  operator/=(f32 rhs){DPZoneScoped;
		this->x /= rhs;
		this->y /= rhs;
	}
	
	inline vec2 operator- ()const{DPZoneScoped;
		vec2 v;
		v.x = -(this->x);
		v.y = -(this->y);
		return v;
	}
	
	inline b32   operator==(const vec2& rhs)const{DPZoneScoped;
		return (fabs(this->x - rhs.x) < M_EPSILON)
			&& (fabs(this->y - rhs.y) < M_EPSILON);
	}
	
	inline b32   operator!=(const vec2& rhs)const{DPZoneScoped;
		return !(*this == rhs);
	}
	
	
	//// member functions ////
	inline void  set(f32 x, f32 y){DPZoneScoped;
		this->x = x;
		this->y = y;
	}
	
	inline vec2 absV()const{DPZoneScoped;
		vec2 v;
		v.x = abs(this->x);
		v.y = abs(this->y);
		return v;
	}
	
	inline vec2 copy()const{DPZoneScoped;
		vec2 v;
		v.x = this->x;
		v.y = this->y;
		return v;
	}
	
	inline f32   dot(const vec2& rhs)const{DPZoneScoped;
		return (this->x * rhs.x) + (this->y * rhs.y);
	}
	
	inline vec2 perp()const{DPZoneScoped;
		vec2 v;
		v.x = -(this->y);
		v.y =   this->x;
		return v;
	}
	
	inline f32   mag()const{DPZoneScoped;
		return sqrt((this->x * this->x) + (this->y * this->y));
	}
	
	inline f32   magSq()const{DPZoneScoped;
		return (this->x * this->x) + (this->y * this->y);
	}
	
	inline void  normalize(){DPZoneScoped;
		if(this->x != 0 || this->y != 0){
			*this /= this->mag();
		}
	}
	
	inline vec2 normalized()const{DPZoneScoped;
		if(this->x != 0 || this->y != 0){
			return *this / this->mag();
		}else{
			return *this;
		}
	}
	
	inline void  clampMag(f32 min, f32 max){DPZoneScoped;
		f32 m = this->mag();
		if      (m < min){
			this->normalize();
			*this *= min;
		}else if(m > max){
			this->normalize();
			*this *= max;
		}
	}
	
	inline vec2 clampedMag(f32 min, f32 max)const{DPZoneScoped;
		f32 m = this->mag();
		if      (m < min){
			return this->normalized() * min;
		}else if(m > max){
			return this->normalized() * max;
		}else{
			return *this;
		}
	}
	
	inline f32   distanceTo(const vec2& rhs)const{DPZoneScoped;
		return (*this - rhs).mag();
	}
	
	inline f32   projectOn(const vec2& rhs)const{DPZoneScoped;
		if(this->mag() > M_EPSILON){
			return this->dot(rhs) / this->mag();
		}else{
			return 0;
		}
	}
	
	inline vec2 compOn(const vec2& rhs)const{DPZoneScoped;
		return rhs.normalized() * this->projectOn(rhs);
	}
	
	inline vec2 midpoint(const vec2& rhs)const{DPZoneScoped;
		vec2 v;
		v.x = (this->x + rhs.x) / 2.f;
		v.y = (this->y + rhs.y) / 2.f;
		return v;
	}
	
	inline vec2 xComp()const{DPZoneScoped;
		vec2 v;
		v.x = this->x;
		v.y = 0;
		return v;
	}
	
	inline vec2 yComp()const{DPZoneScoped;
		vec2 v;
		v.x = 0;
		v.y = this->y;
		return v;
	}
	
	inline vec2 xInvert()const{DPZoneScoped;
		vec2 v;
		v.x = -(this->x);
		v.y =   this->y;
		return v;
	}
	
	inline vec2 yInvert()const{DPZoneScoped;
		vec2 v;
		v.x =   this->x;
		v.y = -(this->y);
		return v;
	}
	
	inline vec2 xSet(f32 a)const{DPZoneScoped;
		vec2 v;
		v.x = a;
		v.y = this->y;
		return v;
	}
	
	inline vec2 ySet(f32 a)const{DPZoneScoped;
		vec2 v;
		v.x = this->x;
		v.y = a;
		return v;
	}
	
	inline vec2 xAdd(f32 a)const{DPZoneScoped;
		vec2 v;
		v.x = this->x + a;
		v.y = this->y;
		return v;
	}
	
	inline vec2 yAdd(f32 a)const{DPZoneScoped;
		vec2 v;
		v.x = this->x;
		v.y = this->y + a;
		return v;
	}
	
	inline vec2 ceil()const{DPZoneScoped;
		vec2 v;
		v.x = ceilf(this->x);
		v.y = ceilf(this->y);
		return v;
	}
	
	inline vec2 floor()const{DPZoneScoped;
		vec2 v;
		v.x = floorf(this->x);
		v.y = floorf(this->y);
		return v;
	}
	
	inline vec2 round()const{DPZoneScoped;
		vec2 v;
		v.x = roundf(this->x);
		v.y = roundf(this->y);
		return v;
	}
	
	//// member interactions ////
	vec2i toVec2i() const;
	vec3  toVec3() const;
	vec4  toVec4() const;
#endif //#ifdef __cplusplus
} vec2;


#ifdef __cplusplus
//// member constants ////
inline const vec2 vec2::ZERO  = vec2{ 0, 0};
inline const vec2 vec2::ONE   = vec2{ 1, 1};
inline const vec2 vec2::UP    = vec2{ 0, 1};
inline const vec2 vec2::DOWN  = vec2{ 0,-1};
inline const vec2 vec2::LEFT  = vec2{-1, 0};
inline const vec2 vec2::RIGHT = vec2{ 1, 0};
inline const vec2 vec2::UNITX = vec2{ 1, 0};
inline const vec2 vec2::UNITY = vec2{ 0, 1};
#endif //#ifdef __cplusplus


//// nonmember constructor ////
FORCE_INLINE vec2 Vec2(f32 x, f32 y){
	return vec2{x, y};
}


//// nonmember constants ////
FORCE_INLINE vec2 vec2_ZERO() { return vec2{ 0, 0}; }
FORCE_INLINE vec2 vec2_ONE()  { return vec2{ 0, 0}; }
FORCE_INLINE vec2 vec2_UP()   { return vec2{ 0, 0}; }
FORCE_INLINE vec2 vec2_DOWN() { return vec2{ 0, 0}; }
FORCE_INLINE vec2 vec2_LEFT() { return vec2{ 0, 0}; }
FORCE_INLINE vec2 vec2_RIGHT(){ return vec2{ 0, 0}; }
FORCE_INLINE vec2 vec2_UNITX(){ return vec2{ 0, 0}; }
FORCE_INLINE vec2 vec2_UNITY(){ return vec2{ 0, 0}; }


//// nonmember operators ////
#ifdef __cplusplus
EndLinkageC(); //NOTE(delle) C Linkage doesn't let you overload operators
FORCE_INLINE vec2 operator* (f32 lhs, vec2 rhs){
	return rhs * lhs;
}
StartLinkageC();
#endif //#ifdef __cplusplus

inline vec2 vec2_add(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x + rhs.x;
	v.y = lhs.y + rhs.y;
	return v;
}

inline vec2 vec2_subtract(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x - rhs.x;
	v.y = lhs.y - rhs.y;
	return v;
}

inline vec2 vec2_multiply(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x * rhs.x;
	v.y = lhs.y * rhs.y;
	return v;
}

inline vec2 vec2_multiply_constant(vec2 lhs, f32 rhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x * rhs;
	v.y = lhs.y * rhs;
	return v;
}

inline vec2 vec2_divide(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x / rhs.x;
	v.y = lhs.y / rhs.y;
	return v;
}

inline vec2 vec2_divide_constant(vec2 lhs, f32 rhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x / rhs;
	v.y = lhs.y / rhs;
	return v;
}

inline vec2 vec2_negate(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = -(lhs.x);
	v.y = -(lhs.y);
	return v;
}

inline b32   vec2_equal(vec2 lhs, vec2 rhs){DPZoneScoped;
	return (fabs(lhs.x - rhs.x) < M_EPSILON) && (fabs(lhs.y - rhs.y) < M_EPSILON);
}


//// nonmember functions ////
inline vec2 vec2_absV(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = abs(lhs.x);
	v.y = abs(lhs.y);
	return v;
}

inline f32   vec2_dot(vec2 lhs, vec2 rhs){DPZoneScoped;
	return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}

inline vec2 vec2_perp(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = -(lhs.y);
	v.y =   lhs.x;
	return v;
}

inline f32   vec2_mag(vec2 lhs){DPZoneScoped;
	return sqrt((lhs.x * lhs.x) + (lhs.y * lhs.y));
}

inline f32   vec2_magSq(vec2 lhs){DPZoneScoped;
	return (lhs.x * lhs.x) + (lhs.y * lhs.y);
}

inline vec2 vec2_normalized(vec2 lhs){DPZoneScoped;
	if(lhs.x != 0 || lhs.y != 0){
		return vec2_divide_constant(lhs, vec2_mag(lhs));
	}else{
		return lhs;
	}
}

inline vec2 vec2_clampedMag(vec2 lhs, f32 min, f32 max){DPZoneScoped;
	f32 m = vec2_mag(lhs);
	if      (m < min){
		return vec2_multiply_constant(vec2_normalized(lhs), min);
	}else if(m > max){
		return vec2_multiply_constant(vec2_normalized(lhs), max);
	}else{
		return lhs;
	}
}

inline f32   vec2_distanceTo(vec2 lhs, vec2 rhs){DPZoneScoped;
	return vec2_mag(vec2_subtract(lhs,rhs));
}

inline f32   vec2_projectOn(vec2 lhs, vec2 rhs){DPZoneScoped;
	f32 m = vec2_mag(lhs);
	if(m > M_EPSILON){
		return vec2_dot(lhs,rhs) / m;
	}else{
		return 0;
	}
}

inline vec2 vec2_compOn(vec2 lhs, vec2 rhs){DPZoneScoped;
	return vec2_multiply_constant(vec2_normalized(rhs), vec2_projectOn(lhs,rhs));
}

inline vec2 vec2_midpoint(vec2 lhs, vec2 rhs){DPZoneScoped;
	vec2 v;
	v.x = (lhs.x + rhs.x) / 2.f;
	v.y = (lhs.y + rhs.y) / 2.f;
	return v;
}

inline vec2 vec2_xComp(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = lhs.x;
	v.y = 0;
	return v;
}

inline vec2 vec2_yComp(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = 0;
	v.y = lhs.y;
	return v;
}

inline vec2 vec2_xInvert(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = -(lhs.x);
	v.y =   lhs.y;
	return v;
}

inline vec2 vec2_yInvert(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x =   lhs.x;
	v.y = -(lhs.y);
	return v;
}

inline vec2 vec2_xSet(vec2 lhs, f32 a){DPZoneScoped;
	vec2 v;
	v.x = a;
	v.y = lhs.y;
	return v;
}

inline vec2 vec2_ySet(vec2 lhs, f32 a){DPZoneScoped;
	vec2 v;
	v.x = lhs.x;
	v.y = a;
	return v;
}

inline vec2 vec2_xAdd(vec2 lhs, f32 a){DPZoneScoped;
	vec2 v;
	v.x = lhs.x + a;
	v.y = lhs.y;
	return v;
}

inline vec2 vec2_yAdd(vec2 lhs, f32 a){DPZoneScoped;
	vec2 v;
	v.x = lhs.x;
	v.y = lhs.y + a;
	return v;
}

inline vec2 vec2_ceil(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = ceil(lhs.x);
	v.y = ceil(lhs.y);
	return v;
}

inline vec2 vec2_floor(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = floor(lhs.x);
	v.y = floor(lhs.y);
	return v;
}

inline vec2 vec2_round(vec2 lhs){DPZoneScoped;
	vec2 v;
	v.x = round(lhs.x);
	v.y = round(lhs.y);
	return v;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec3i
typedef struct vec3i{
	union{
		s32 arr[3] = {};
		struct{ s32 x, y, z; };
		struct{ s32 r, g, b; };
		struct{ vec2i xy; s32 _unusedZ0; };
		struct{ s32 _unusedX0; vec2i yz; };
	};
} vec3i;


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec3
typedef struct vec3{
	//// data ////
	union{
		f32 arr[3] = {};
		struct{ f32 x, y, z; };
		struct{ f32 r, g, b; };
		struct{ vec2 xy; f32 _unusedZ0; };
		struct{ f32 _unusedX0; vec2 yz; };
	};
	
	
#ifdef __cplusplus
	//// member constants ////
	static const vec3 ZERO;
	static const vec3 ONE;
	static const vec3 RIGHT;
	static const vec3 LEFT;
	static const vec3 UP;
	static const vec3 DOWN;
	static const vec3 FORWARD;
	static const vec3 BACK;
	static const vec3 UNITX;
	static const vec3 UNITY;
	static const vec3 UNITZ;
	
	
	//// member operators ////
	inline vec3 operator+ (const vec3& rhs)const{DPZoneScoped;
		vec3 v;
		v.x = this->x + rhs.x;
		v.y = this->y + rhs.y;
		v.z = this->z + rhs.z;
		return v;
	}
	
	inline void operator+=(const vec3& rhs){DPZoneScoped;
		this->x += rhs.x;
		this->y += rhs.y;
		this->z += rhs.z;
	}
	
	inline vec3 operator- (const vec3& rhs)const{DPZoneScoped;
		vec3 v;
		v.x = this->x - rhs.x;
		v.y = this->y - rhs.y;
		v.z = this->z - rhs.z;
		return v;
	}
	
	inline void operator-=(const vec3& rhs){DPZoneScoped;
		this->x -= rhs.x;
		this->y -= rhs.y;
		this->z -= rhs.z;
	}
	
	inline vec3 operator* (const vec3& rhs)const{DPZoneScoped;
		vec3 v;
		v.x = this->x * rhs.x;
		v.y = this->y * rhs.y;
		v.z = this->z * rhs.z;
		return v;
	}
	
	inline void operator*=(const vec3& rhs){DPZoneScoped;
		this->x *= rhs.x;
		this->y *= rhs.y;
		this->z *= rhs.z;
	}
	
	inline vec3 operator* (f32 rhs)const{DPZoneScoped;
		vec3 v;
		v.x = this->x * rhs;
		v.y = this->y * rhs;
		v.z = this->z * rhs;
		return v;
	}
	
	inline void operator*=(f32 rhs){DPZoneScoped;
		this->x *= rhs;
		this->y *= rhs;
		this->z *= rhs;
	}
	
	inline vec3 operator/ (const vec3& rhs)const{DPZoneScoped;
		vec3 v;
		v.x = this->x / rhs.x;
		v.y = this->y / rhs.y;
		v.z = this->z / rhs.z;
		return v;
	}
	
	inline void operator/=(const vec3& rhs){DPZoneScoped;
		this->x /= rhs.x;
		this->y /= rhs.y;
		this->z /= rhs.z;
	}
	
	inline vec3 operator/ (f32 rhs)const{DPZoneScoped;
		vec3 v;
		v.x = this->x / rhs;
		v.y = this->y / rhs;
		v.z = this->z / rhs;
		return v;
	}
	
	inline void operator/=(f32 rhs){DPZoneScoped;
		this->x /= rhs;
		this->y /= rhs;
		this->z /= rhs;
	}
	
	inline vec3 operator- ()const{DPZoneScoped;
		vec3 v;
		v.x = -(this->x);
		v.y = -(this->y);
		v.z = -(this->z);
		return v;
	}
	
	inline bool operator==(const vec3& rhs)const{DPZoneScoped;
		return (fabs(this->x - rhs.x) < M_EPSILON)
			&& (fabs(this->y - rhs.y) < M_EPSILON)
			&& (fabs(this->z - rhs.z) < M_EPSILON);
	}
	
	inline bool operator!=(const vec3& rhs)const{DPZoneScoped;
		return !(*this == rhs);
	}
	
	
	//// member functions ////
	inline void set(f32 x, f32 y, f32 z){DPZoneScoped;
		this->x = x;
		this->y = y;
		this->z = z;
	}
	
	inline vec3 absV()const{DPZoneScoped;
		vec3 v;
		v.x = abs(this->x);
		v.y = abs(this->y);
		v.z = abs(this->z);
		return v;
	}
	
	inline vec3 copy()const{DPZoneScoped;
		vec3 v;
		v.x = this->x;
		v.y = this->y;
		v.z = this->z;
		return v;
	}
	
	inline f32  dot(const vec3& rhs)const{DPZoneScoped;
		return (this->x * rhs.x) + (this->y * rhs.y) + (this->z * rhs.z);
	}
	
	inline vec3 cross(const vec3& rhs)const{DPZoneScoped;
		vec3 v;
		v.x = (this->y * rhs.z) - (this->z * rhs.y);
		v.y = (this->z * rhs.x) - (this->x * rhs.z);
		v.z = (this->x * rhs.y) - (this->y * rhs.x);
		return v;
	}
	
	inline f32  mag()const{DPZoneScoped;
		return sqrt((this->x * this->x) + (this->y * this->y) + (this->z * this->z));
		
		////!ref: https://betterexplained.com/articles/understanding-quakes-fast-inverse-square-root/
		//Assert(sizeof(f32) == 4 && sizeof(s32) == 4, "This mag method only works if f32 and s32 are 32bit");
		//f32 k = (this->x * this->x) + (this->y * this->y) + (this->z * this->z);
		//f32 kHalf = .5f * k;
		//s32 i = *(s32*)&k;
		//i = 0x5f3759df - (i >> 1);
		//k = *(f32*)&i;
		//k = k*(1.5f - kHalf*k*k);
		//return 1.f / k;
	}
	
	inline f32  magSq()const{DPZoneScoped;
		return (this->x * this->x) + (this->y * this->y) + (this->z * this->z);
	}
	
	inline void normalize(){DPZoneScoped;
		if(this->x != 0 || this->y != 0 || this->z != 0){
			*this /= this->mag();
		}
	}
	
	inline vec3 normalized()const{DPZoneScoped;
		if(this->x != 0 || this->y != 0 || this->z != 0){
			return *this / this->mag();
		}else{
			return *this;
		}
	}
	
	inline vec3 clamp(f32 lo, f32 hi)const{DPZoneScoped;
		if(lo > hi) Swap(lo, hi);
		vec3 v;
		v.x = (this->x < lo) ? lo : (this->x > hi) ? hi : this->x;
		v.y = (this->y < lo) ? lo : (this->y > hi) ? hi : this->y;
		v.z = (this->z < lo) ? lo : (this->z > hi) ? hi : this->z;
		return v;
	}
	
	inline void clampMag(f32 min, f32 max){DPZoneScoped;
		f32 m = this->mag();
		if      (m < min){
			this->normalize();
			*this *= min;
		}else if(m > max){
			this->normalize();
			*this *= max;
		}
	}
	
	inline vec3 clampedMag(f32 min, f32 max)const{DPZoneScoped;
		f32 m = this->mag();
		if      (m < min){
			return this->normalized() * min;
		}else if(m > max){
			return this->normalized() * max;
		}else{
			return *this;
		}
	}
	
	inline f32  distanceTo(const vec3& rhs)const{DPZoneScoped;
		return (*this - rhs).mag();
	}
	
	inline f32  projectOn(const vec3& rhs)const{DPZoneScoped;
		f32 m = this->mag();
		if(m > M_EPSILON){
			return this->dot(rhs) / m;
		}else{
			return 0;
		}
	}
	
	inline vec3 compOn(const vec3& rhs)const{DPZoneScoped;
		return rhs.normalized() * this->projectOn(rhs);
	}
	
	inline vec3 midpoint(const vec3& rhs)const{DPZoneScoped;
		vec3 v;
		v.x = (this->x + rhs.x) / 2.f;
		v.y = (this->y + rhs.y) / 2.f;
		v.z = (this->z + rhs.z) / 2.f;
		return v;
	}
	
	//returns the angle in radians
	inline f32  angleBetween(const vec3& rhs)const{DPZoneScoped;
		f32 m = this->mag() * rhs.mag();
		if(m != 0){
			return acos(this->dot(rhs) / m);
		}else{
			return 0;
		}
	}
	
	inline vec3 xComp()const{DPZoneScoped;
		vec3 v;
		v.x = this->x;
		v.y = 0;
		v.z = 0;
		return v;
	}
	
	inline vec3 yComp()const{DPZoneScoped;
		vec3 v;
		v.x = 0;
		v.y = this->y;
		v.z = 0;
		return v;
	}
	
	inline vec3 zComp()const{DPZoneScoped;
		vec3 v;
		v.x = 0;
		v.y = 0;
		v.z = this->z;
		return v;
	}
	
	inline vec3 xZero()const{DPZoneScoped;
		vec3 v;
		v.x = 0;
		v.y = this->y;
		v.z = this->z;
		return v;
	}
	
	inline vec3 yZero()const{DPZoneScoped;
		vec3 v;
		v.x = this->x;
		v.y = 0;
		v.z = this->z;
		return v;
	}
	
	inline vec3 zZero()const{DPZoneScoped;
		vec3 v;
		v.x = this->x;
		v.y = this->y;
		v.z = 0;
		return v;
	}
	
	inline vec3 xInvert()const{DPZoneScoped;
		vec3 v;
		v.x = -(this->x);
		v.y =   this->y;
		v.z =   this->z;
		return v;
	}
	
	inline vec3 yInvert()const{DPZoneScoped;
		vec3 v;
		v.x =   this->x;
		v.y = -(this->y);
		v.z =   this->z;
		return v;
	}
	
	inline vec3 zInvert()const{DPZoneScoped;
		vec3 v;
		v.x =   this->x;
		v.y =   this->y;
		v.z = -(this->z);
		return v;
	}
	
	inline vec3 ceil()const{DPZoneScoped;
		vec3 v;
		v.x = ceilf(this->x);
		v.y = ceilf(this->y);
		v.z = ceilf(this->z);
		return v;
	}
	
	inline vec3 floor()const{DPZoneScoped;
		vec3 v;
		v.x = floorf(this->x);
		v.y = floorf(this->y);
		v.z = floorf(this->z);
		return v;
	}
	
	inline vec3 round()const{DPZoneScoped;
		vec3 v;
		v.x = roundf(this->x);
		v.y = roundf(this->y);
		v.z = roundf(this->z);
		return v;
	}
	
	inline void round(s32 place){DPZoneScoped;
		this->x = floorf(this->x * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f);
		this->y = floorf(this->y * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f);
		this->z = floorf(this->z * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f);
	}
	
	inline vec3 rounded(s32 place)const{DPZoneScoped;
		vec3 v;
		v.x = floorf(this->x * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f);
		v.y = floorf(this->y * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f);
		v.z = floorf(this->z * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f);
		return v;
	}
	
	
	//// member interactions ////
	vec3 operator* (const mat3& rhs)const;
	void operator*=(const mat3& rhs);
	vec3 operator* (const mat4& rhs)const;
	void operator*=(const mat4& rhs);
	vec3 operator* (const quat& rhs)const;
	vec2 toVec2()const;
	vec4 toVec4()const;
#endif //#ifdef __cplusplus
} vec3;


#ifdef __cplusplus
//// member constants ////
inline const vec3 vec3::ZERO    = vec3{ 0, 0, 0};
inline const vec3 vec3::ONE     = vec3{ 1, 1, 1};
inline const vec3 vec3::UP      = vec3{ 0, 1, 0};
inline const vec3 vec3::DOWN    = vec3{ 0,-1, 0};
inline const vec3 vec3::LEFT    = vec3{-1, 0, 0};
inline const vec3 vec3::RIGHT   = vec3{ 1, 0, 0};
inline const vec3 vec3::FORWARD = vec3{ 0, 0, 1};
inline const vec3 vec3::BACK    = vec3{ 0, 0,-1};
inline const vec3 vec3::UNITX   = vec3{ 1, 0, 0};
inline const vec3 vec3::UNITY   = vec3{ 0, 1, 0};
inline const vec3 vec3::UNITZ   = vec3{ 0, 0, 1};
#endif //#ifdef __cplusplus


//// nonmember constructor ////
FORCE_INLINE vec3 Vec3(f32 x, f32 y, f32 z){
	return vec3{x, y, z};
}


//// nonmember constants ////
FORCE_INLINE vec3 vec3_ZERO()   { return vec3{ 0, 0, 0}; }
FORCE_INLINE vec3 vec3_ONE()    { return vec3{ 1, 1, 1}; }
FORCE_INLINE vec3 vec3_UP()     { return vec3{ 0, 1, 0}; }
FORCE_INLINE vec3 vec3_DOWN()   { return vec3{ 0,-1, 0}; }
FORCE_INLINE vec3 vec3_LEFT()   { return vec3{-1, 0, 0}; }
FORCE_INLINE vec3 vec3_RIGHT()  { return vec3{ 1, 0, 0}; }
FORCE_INLINE vec3 vec3_FORWARD(){ return vec3{ 0, 0, 1}; }
FORCE_INLINE vec3 vec3_BACK()   { return vec3{ 0, 0,-1}; }
FORCE_INLINE vec3 vec3_UNITX()  { return vec3{ 1, 0, 0}; }
FORCE_INLINE vec3 vec3_UNITY()  { return vec3{ 0, 1, 0}; }
FORCE_INLINE vec3 vec3_UNITZ()  { return vec3{ 0, 0, 1}; }


//// nonmember operators ////
#ifdef __cplusplus
EndLinkageC(); //NOTE(delle) C Linkage doesn't let you overload operators
FORCE_INLINE vec3 operator* (s32 lhs, vec3 rhs){
	return rhs * lhs;
}
StartLinkageC();
#endif //#ifdef __cplusplus

inline vec3 vec3_add(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x + rhs.x;
	v.y = lhs.y + rhs.y;
	v.z = lhs.z + rhs.z;
	return v;
}

inline vec3 vec3_subtract(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x - rhs.x;
	v.y = lhs.y - rhs.y;
	v.z = lhs.z - rhs.z;
	return v;
}

inline vec3 vec3_multiply(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x * rhs.x;
	v.y = lhs.y * rhs.y;
	v.z = lhs.z * rhs.z;
	return v;
}

inline vec3 vec3_multiply_constant(vec3 lhs, f32 rhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x * rhs;
	v.y = lhs.y * rhs;
	v.z = lhs.z * rhs;
	return v;
}

inline vec3 vec3_divide(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x / rhs.x;
	v.y = lhs.y / rhs.y;
	v.z = lhs.z / rhs.z;
	return v;
}

inline vec3 vec3_divide_constant(vec3 lhs, f32 rhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x / rhs;
	v.y = lhs.y / rhs;
	v.z = lhs.z / rhs;
	return v;
}

inline vec3 vec3_negate(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = -(lhs.x);
	v.y = -(lhs.y);
	v.z = -(lhs.z);
	return v;
}

inline b32   vec3_equal(vec3 lhs, vec3 rhs){DPZoneScoped;
	return (fabs(lhs.x - rhs.x) < M_EPSILON)
		&& (fabs(lhs.y - rhs.y) < M_EPSILON)
		&& (fabs(lhs.z - rhs.z) < M_EPSILON);
}


//// nonmember functions ////
inline vec3 vec3_absV(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = abs(lhs.x);
	v.y = abs(lhs.y);
	v.z = abs(lhs.z);
	return v;
}

inline f32  vec3_dot(vec3 lhs, vec3 rhs){DPZoneScoped;
	return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

inline vec3 vec3_cross(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = (lhs.y * rhs.z) - (lhs.z * rhs.y);
	v.y = (lhs.z * rhs.x) - (lhs.x * rhs.z);
	v.z = (lhs.x * rhs.y) - (lhs.y * rhs.x);
	return v;
}

inline f32  vec3_mag(vec3 lhs){DPZoneScoped;
	return sqrt((lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z));
	
	////!ref: https://betterexplained.com/articles/understanding-quakes-fast-inverse-square-root/
	//Assert(sizeof(f32) == 4 && sizeof(s32) == 4, "This mag method only works if f32 and s32 are 32bit");
	//f32 k = (lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z);
	//f32 kHalf = .5f * k;
	//s32 i = *(s32*)&k;
	//i = 0x5f3759df - (i >> 1);
	//k = *(f32*)&i;
	//k = k*(1.5f - kHalf*k*k);
	//return 1.f / k;
}

inline f32  vec3_magSq(vec3 lhs){DPZoneScoped;
	return (lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z);
}

inline vec3 vec3_normalized(vec3 lhs){DPZoneScoped;
	if(lhs.x != 0 || lhs.y != 0 || lhs.z != 0){
		return vec3_divide_constant(lhs, vec3_mag(lhs));
	}else{
		return lhs;
	}
}

inline vec3 vec3_clamp(vec3 lhs, f32 lo, f32 hi){DPZoneScoped;
	if(lo > hi) Swap(lo, hi);
	vec3 v;
	v.x = (lhs.x < lo) ? lo : (lhs.x > hi) ? hi : lhs.x;
	v.y = (lhs.y < lo) ? lo : (lhs.y > hi) ? hi : lhs.y;
	v.z = (lhs.z < lo) ? lo : (lhs.z > hi) ? hi : lhs.z;
	return v;
}

inline vec3 vec3_clampedMag(vec3 lhs, f32 min, f32 max){DPZoneScoped;
	f32 m = vec3_mag(lhs);
	if      (m < min){
		return vec3_multiply_constant(vec3_normalized(lhs), min);
	}else if(m > max){
		return vec3_multiply_constant(vec3_normalized(lhs), max);
	}else{
		return lhs;
	}
}

inline f32  vec3_distanceTo(vec3 lhs, vec3 rhs){DPZoneScoped;
	return vec3_mag(vec3_subtract(lhs,rhs));
}

inline f32  vec3_projectOn(vec3 lhs, vec3 rhs){DPZoneScoped;
	f32 m = vec3_mag(lhs);
	if(m > M_EPSILON){
		return vec3_dot(lhs,rhs) / m;
	}else{
		return 0;
	}
}

inline vec3 vec3_compOn(vec3 lhs, vec3 rhs){DPZoneScoped;
	return vec3_multiply_constant(vec3_normalized(rhs), vec3_projectOn(lhs,rhs));
}

inline vec3 vec3_midpoint(vec3 lhs, vec3 rhs){DPZoneScoped;
	vec3 v;
	v.x = (lhs.x + rhs.x) / 2.f;
	v.y = (lhs.y + rhs.y) / 2.f;
	v.z = (lhs.z + rhs.z) / 2.f;
	return v;
}

//returns the angle in radians
inline f32  vec3_angleBetween(vec3 lhs, vec3 rhs){DPZoneScoped;
	f32 m = vec3_mag(lhs) * vec3_mag(rhs);
	if(m != 0){
		return acos(vec3_dot(lhs,rhs) / m);
	}else{
		return 0;
	}
}

inline vec3 vec3_xComp(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x;
	v.y = 0;
	v.z = 0;
	return v;
}

inline vec3 vec3_yComp(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = 0;
	v.y = lhs.y;
	v.z = 0;
	return v;
}

inline vec3 vec3_zComp(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = 0;
	v.y = 0;
	v.z = lhs.z;
	return v;
}

inline vec3 vec3_xZero(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = 0;
	v.y = lhs.y;
	v.z = lhs.z;
	return v;
}

inline vec3 vec3_yZero(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x;
	v.y = 0;
	v.z = lhs.z;
	return v;
}

inline vec3 vec3_zZero(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = lhs.x;
	v.y = lhs.y;
	v.z = 0;
	return v;
}

inline vec3 vec3_xInvert(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = -(lhs.x);
	v.y =   lhs.y;
	v.z =   lhs.z;
	return v;
}

inline vec3 vec3_yInvert(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x =   lhs.x;
	v.y = -(lhs.y);
	v.z =   lhs.z;
	return v;
}

inline vec3 vec3_zInvert(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x =   lhs.x;
	v.y =   lhs.y;
	v.z = -(lhs.z);
	return v;
}

inline vec3 vec3_ceil(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = ceil(lhs.x);
	v.y = ceil(lhs.y);
	v.z = ceil(lhs.z);
	return v;
}

inline vec3 vec3_floor(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = floor(lhs.x);
	v.y = floor(lhs.y);
	v.z = floor(lhs.z);
	return v;
}

inline vec3 vec3_round(vec3 lhs){DPZoneScoped;
	vec3 v;
	v.x = round(lhs.x);
	v.y = round(lhs.y);
	v.z = round(lhs.z);
	return v;
}

inline vec3 vec3_rounded(vec3 lhs, s32 place){DPZoneScoped;
	vec3 v;
	v.x = floor(lhs.x * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f);
	v.y = floor(lhs.y * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f);
	v.z = floor(lhs.z * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f);
	return v;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec4i
typedef struct vec4i{
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
		__m128 sse;
#endif
	};
} vec4i;


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec4
typedef struct vec4{
	//// data ////
	union{
		f32 arr[4] = {};
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
			f32 _unusedZ0;
			f32 _unusedW0;
		};
		struct{ 
			f32 _unusedX0;
			vec2 yz;
			f32 _unusedW1;
		};
		struct{ 
			f32 _unusedX1;
			f32 _unusedY0;
			vec2 zw;
		};
#ifdef DESHI_USE_SSE
		__m128 sse;
#endif
	};
	
	
#ifdef __cplusplus
	//// member constants ////
	static const vec4 ZERO;
	static const vec4 ONE;
	
	
	//// member operators ////
	inline vec4 operator+ (const vec4& rhs)const{DPZoneScoped;
		vec4 v;
#if DESHI_USE_SSE
		v.sse = _mm_add_ps(this->sse, rhs.sse);
#else
		v.x = this->x + rhs.x;
		v.y = this->y + rhs.y;
		v.z = this->z + rhs.z;
		v.w = this->w + rhs.w;
#endif
		return v;
	}
	
	inline void operator+=(const vec4& rhs){DPZoneScoped;
#if DESHI_USE_SSE
		this->sse = _mm_add_ps(this->sse, rhs.sse);
#else
		this->x += rhs.x;
		this->y += rhs.y;
		this->z += rhs.z;
		this->w += rhs.w;
#endif
	}
	
	inline vec4 operator- (const vec4& rhs)const{DPZoneScoped;
		vec4 v;
#if DESHI_USE_SSE
		v.sse = _mm_sub_ps(this->sse, rhs.sse);
#else
		v.x = this->x - rhs.x;
		v.y = this->y - rhs.y;
		v.z = this->z - rhs.z;
		v.w = this->w - rhs.w;
#endif
		return v;
	}
	
	inline void operator-=(const vec4& rhs){DPZoneScoped;
#if DESHI_USE_SSE
		this->sse = _mm_sub_ps(this->sse, rhs.sse);
#else
		this->x -= rhs.x;
		this->y -= rhs.y;
		this->z -= rhs.z;
		this->w -= rhs.w;
#endif
	}
	
	inline vec4 operator* (const vec4& rhs)const{DPZoneScoped;
		vec4 v;
#if DESHI_USE_SSE
		v.sse = _mm_mul_ps(this->sse, rhs.sse);
#else
		v.x = this->x * rhs.x;
		v.y = this->y * rhs.y;
		v.z = this->z * rhs.z;
		v.w = this->w * rhs.w;
#endif
		return v;
	}
	
	inline void operator*=(const vec4& rhs){DPZoneScoped;
#if DESHI_USE_SSE
		this->sse = _mm_mul_ps(this->sse, rhs.sse);
#else
		this->x *= rhs.x;
		this->y *= rhs.y;
		this->z *= rhs.z;
		this->w *= rhs.w;
#endif
	}
	
	inline vec4 operator* (const f32& rhs)const{DPZoneScoped;
		vec4 v;
#if DESHI_USE_SSE
		__m128 scalar = _mm_set1_ps(rhs);
		v.sse = _mm_mul_ps(this->sse, scalar);
#else
		v.x = this->x * rhs;
		v.y = this->y * rhs;
		v.z = this->z * rhs;
		v.w = this->w * rhs;
#endif
		return v;
	}
	
	inline void operator*=(const f32& rhs){DPZoneScoped;
#if DESHI_USE_SSE
		__m128 scalar = _mm_set1_ps(rhs);
		sse = _mm_mul_ps(this->sse, scalar);
#else
		this->x *= rhs;
		this->y *= rhs;
		this->z *= rhs;
		this->w *= rhs;
#endif
	}
	
	inline vec4 operator/ (const vec4& rhs)const{DPZoneScoped;
		vec4 v;
#if DESHI_USE_SSE
		v.sse = _mm_div_ps(this->sse, rhs.sse);
#else
		v.x = this->x / rhs.x;
		v.y = this->y / rhs.y;
		v.z = this->z / rhs.z;
		v.w = this->w / rhs.w;
#endif
		return v;
	}
	
	inline void operator/=(const vec4& rhs){DPZoneScoped;
#if DESHI_USE_SSE
		this->sse = _mm_div_ps(this->sse, rhs.sse);
#else
		this->x /= rhs.x;
		this->y /= rhs.y;
		this->z /= rhs.z;
		this->w /= rhs.w;
#endif
	}
	
	inline vec4 operator/ (const f32& rhs)const{DPZoneScoped;
		vec4 v;
#if DESHI_USE_SSE
		__m128 scalar = _mm_set1_ps(rhs);
		v.sse = _mm_div_ps(this->sse, scalar);
#else
		v.x = this->x / rhs;
		v.y = this->y / rhs;
		v.z = this->z / rhs;
		v.w = this->w / rhs;
#endif
		return v;
	}
	
	inline void operator/=(const f32& rhs){DPZoneScoped;
#if DESHI_USE_SSE
		__m128 scalar = _mm_set1_ps(rhs);
		sse = _mm_div_ps(this->sse, scalar);
#else
		this->x /= rhs;
		this->y /= rhs;
		this->z /= rhs;
		this->w /= rhs;
#endif
	}
	
	inline vec4 operator- ()const{DPZoneScoped;
		vec4 v;
#if DESHI_USE_SSE
		v.sse = NegateSSE(this->sse);
#else
		v.x = -(this->x);
		v.y = -(this->y);
		v.z = -(this->z);
		v.w = -(this->w);
#endif
		return v;
	}
	
	inline bool operator==(const vec4& rhs)const{DPZoneScoped;
#if DESHI_USE_SSE
		return EpsilonEqualSSE(this->sse, rhs.sse);
#else
		return abs(this->x - rhs.x) < M_EPSILON 
			&& abs(this->y - rhs.y) < M_EPSILON 
			&& abs(this->z - rhs.z) < M_EPSILON 
			&& abs(this->w - rhs.w) < M_EPSILON;
#endif
	}
	
	inline bool operator!=(const vec4& rhs)const{DPZoneScoped;
		return !(*this == rhs);
	}
	
	
	//// member functions ////
	inline void set(f32 x, f32 y, f32 z, f32 w){DPZoneScoped;
#if DESHI_USE_SSE
		this->sse = _mm_setr_ps(x, y, z, w);
#else
		this->x = x; 
		this->y = y; 
		this->z = z; 
		this->w = w;
#endif
	}
	
	inline vec4 absV()const{DPZoneScoped;
		vec4 v;
#if DESHI_USE_SSE
		v.sse = AbsoluteSSE(sse);
#else
		v.x = abs(x);
		v.y = abs(y);
		v.z = abs(z);
		v.w = abs(w);
#endif
		return v;
	}
	
	inline vec4 copy()const{DPZoneScoped;
		vec4 v;
		v.x = this->x;
		v.y = this->y;
		v.z = this->z;
		v.w = this->w;
		return v;
	}
	
	inline f32  dot(const vec4& rhs)const{DPZoneScoped;
		f32 result;
#if DESHI_USE_SSE
		__m128 temp0 = _mm_mul_ps(this->sse, rhs.sse); //multiply together
		__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
		temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
		temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
		temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
		result = _mm_cvtss_f32(temp0);
#else
		result = (this->x * rhs.x) + (this->y * rhs.y) + (this->z * rhs.z) + (this->w * rhs.w);
#endif
		return result;
	}
	
	inline f32  magSq()const{DPZoneScoped;
		return this->dot(*this);
	}
	
	inline f32  mag()const{DPZoneScoped;
		return Sqrt(this->magSq());
	}
	
	inline vec4 wnormalized()const{DPZoneScoped;
		if(this->w != 0){
			return *this / this->w;
		}else{
			return *this;
		}
	}
	
	inline vec4 xComp()const{DPZoneScoped;
		vec4 v;
		v.x = this->x;
		v.y = 0;
		v.z = 0;
		v.w = 0;
		return v;
	}
	
	inline vec4 yComp()const{DPZoneScoped;
		vec4 v;
		v.x = 0;
		v.y = this->y;
		v.z = 0;
		v.w = 0;
		return v;
	}
	
	inline vec4 zComp()const{DPZoneScoped;
		vec4 v;
		v.x = 0;
		v.y = 0;
		v.z = this->z;
		v.w = 0;
		return v;
	}
	
	inline vec4 wComp()const{DPZoneScoped;
		vec4 v;
		v.x = 0;
		v.y = 0;
		v.z = 0;
		v.w = this->w;
		return v;
	}
	
	inline vec4 xInvert()const{DPZoneScoped;
		vec4 v;
		v.x = -(this->x);
		v.y =   this->y;
		v.z =   this->z;
		v.w =   this->w;
		return v;
	}
	
	inline vec4 yInvert()const{DPZoneScoped;
		vec4 v;
		v.x =   this->x;
		v.y = -(this->y);
		v.z =   this->z;
		v.w =   this->w;
		return v;
	}
	
	inline vec4 zInvert()const{DPZoneScoped;
		vec4 v;
		v.x =   this->x;
		v.y =   this->y;
		v.z = -(this->z);
		v.w =   this->w;
		return v;
	}
	
	inline vec4 wInvert()const{DPZoneScoped;
		vec4 v;
		v.x =   this->x;
		v.y =   this->y;
		v.z =   this->z;
		v.w = -(this->w);
		return v;
	}
	
	inline vec4 ceil()const{DPZoneScoped;
		vec4 v;
		v.x = ceilf(this->x);
		v.y = ceilf(this->y);
		v.z = ceilf(this->z);
		v.z = ceilf(this->w);
		return v;
	}
	
	inline vec4 floor()const{DPZoneScoped;
		vec4 v;
		v.x = floorf(this->x);
		v.y = floorf(this->y);
		v.z = floorf(this->z);
		v.z = floorf(this->w);
		return v;
	}
	
	inline vec4 round()const{DPZoneScoped;
		vec4 v;
		v.x = roundf(this->x);
		v.y = roundf(this->y);
		v.z = roundf(this->z);
		v.z = roundf(this->w);
		return v;
	}
	
	
	//// member interactions ////
	vec4 operator* (const mat4& rhs)const;
	void operator*=(const mat4& rhs);
	vec3 toVec3()const;
	void takeVec3(const vec3& v);
#endif //#ifdef __cplusplus
} vec4;


#ifdef __cplusplus
//// member constants ////
inline const vec4 vec4::ZERO = vec4{0,0,0,0};
inline const vec4 vec4::ONE  = vec4{1,1,1,1};
#endif //#ifdef __cplusplus


//// nonmember constructor ////
FORCE_INLINE vec4 Vec4(f32 x, f32 y, f32 z, f32 w){
	return vec4{x, y, z, w};
}


//// nonmember constants ////
FORCE_INLINE vec4 vec4_ZERO(){ return vec4{0,0,0,0}; }
FORCE_INLINE vec4 vec4_ONE() { return vec4{1,1,1,1}; }


//// nonmember operators ////
#ifdef __cplusplus
EndLinkageC(); //NOTE(delle) C Linkage doesn't let you overload operators
FORCE_INLINE vec4 operator* (s32 lhs, vec4 rhs){
	return rhs * lhs;
}
StartLinkageC();
#endif //#ifdef __cplusplus

inline vec4 vec4_add(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = _mm_add_ps(lhs.sse, rhs.sse);
#else
	v.x = lhs.x + rhs.x;
	v.y = lhs.y + rhs.y;
	v.z = lhs.z + rhs.z;
	v.w = lhs.w + rhs.w;
#endif
	return v;
}

inline vec4 vec4_subtract(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = _mm_sub_ps(lhs.sse, rhs.sse);
#else
	v.x = lhs.x - rhs.x;
	v.y = lhs.y - rhs.y;
	v.z = lhs.z - rhs.z;
	v.w = lhs.w - rhs.w;
#endif
	return v;
}

inline vec4 vec4_multiply(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = _mm_mul_ps(lhs.sse, rhs.sse);
#else
	v.x = lhs.x * rhs.x;
	v.y = lhs.y * rhs.y;
	v.z = lhs.z * rhs.z;
	v.w = lhs.w * rhs.w;
#endif
	return v;
}

inline vec4 vec4_multiply_constant(vec4 lhs, f32 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	__m128 scalar = _mm_set1_ps(rhs);
	v.sse = _mm_mul_ps(lhs.sse, scalar);
#else
	v.x = lhs.x * rhs;
	v.y = lhs.y * rhs;
	v.z = lhs.z * rhs;
	v.w = lhs.w * rhs;
#endif
	return v;
}

inline vec4 vec4_divide(vec4 lhs, vec4 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = _mm_div_ps(lhs.sse, rhs.sse);
#else
	v.x = lhs.x / rhs.x;
	v.y = lhs.y / rhs.y;
	v.z = lhs.z / rhs.z;
	v.w = lhs.w / rhs.w;
#endif
	return v;
}

inline vec4 vec4_divide_constant(vec4 lhs, f32 rhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	__m128 scalar = _mm_set1_ps(rhs);
	v.sse = _mm_div_ps(lhs.sse, scalar);
#else
	v.x = lhs.x / rhs;
	v.y = lhs.y / rhs;
	v.z = lhs.z / rhs;
	v.w = lhs.w / rhs;
#endif
	return v;
}

inline vec4 vec4_negate(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = NegateSSE(lhs.sse);
#else
	v.x = -(lhs.x);
	v.y = -(lhs.y);
	v.z = -(lhs.z);
	v.w = -(lhs.w);
#endif
	return v;
}

inline b32   vec4_equal(vec4 lhs, vec4 rhs){DPZoneScoped;
#if DESHI_USE_SSE
	return EpsilonEqualSSE(lhs.sse, rhs.sse);
#else
	return abs(lhs.x - rhs.x) < M_EPSILON 
		&& abs(lhs.y - rhs.y) < M_EPSILON 
		&& abs(lhs.z - rhs.z) < M_EPSILON 
		&& abs(lhs.w - rhs.w) < M_EPSILON;
#endif
}


//// nonmember functions ////
inline vec4 vec4_absV(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = AbsoluteSSE(lhs.sse);
#else
	v.x = abs(lhs.x);
	v.y = abs(lhs.y);
	v.z = abs(lhs.z);
	v.w = abs(lhs.w);
#endif
	return v;
}

inline f32  vec4_dot(vec4 lhs, vec4 rhs){DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mul_ps(lhs.sse, rhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	result = _mm_cvtss_f32(temp0);
#else
	result = (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
#endif
	return result;
}

inline f32  vec4_magSq(vec4 lhs){DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mul_ps(lhs.sse, lhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	result = _mm_cvtss_f32(temp0);
#else
	result = (lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z) + (lhs.w * lhs.w);
#endif
	return result;
}

inline f32  vec4_mag(vec4 lhs){DPZoneScoped;
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mul_ps(lhs.sse, lhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	temp0 = _mm_sqrt_ss(temp0);
	result = _mm_cvtss_f32(temp0);
#else
	result = Sqrt((lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z) + (lhs.w * lhs.w));
#endif
	return result;
}

inline vec4 vec4_wnormalized(vec4 lhs){DPZoneScoped;
	if(lhs.w != 0){
		return vec4_divide_constant(lhs, lhs.w);
	}else{
		return lhs;
	}
}

inline vec4 vec4_xComp(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x = lhs.x;
	v.y = 0;
	v.z = 0;
	v.w = 0;
	return v;
}

inline vec4 vec4_yComp(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x = 0;
	v.y = lhs.y;
	v.z = 0;
	v.w = 0;
	return v;
}

inline vec4 vec4_zComp(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x = 0;
	v.y = 0;
	v.z = lhs.z;
	v.w = 0;
	return v;
}

inline vec4 vec4_wComp(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x = 0;
	v.y = 0;
	v.z = 0;
	v.w = lhs.w;
	return v;
}

inline vec4 vec4_xInvert(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x = -(lhs.x);
	v.y =   lhs.y;
	v.z =   lhs.z;
	v.w =   lhs.w;
	return v;
}

inline vec4 vec4_yInvert(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x =   lhs.x;
	v.y = -(lhs.y);
	v.z =   lhs.z;
	v.w =   lhs.w;
	return v;
}

inline vec4 vec4_zInvert(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x =   lhs.x;
	v.y =   lhs.y;
	v.z = -(lhs.z);
	v.w =   lhs.w;
	return v;
}

inline vec4 vec4_wInvert(vec4 lhs){DPZoneScoped;
	vec4 v;
	v.x =   lhs.x;
	v.y =   lhs.y;
	v.z =   lhs.z;
	v.w = -(lhs.w);
	return v;
}

inline vec4 vec4_ceil(vec4 lhs){DPZoneScoped;
	
	vec4 v;
#if DESHI_USE_SSE
	v.sse = _mm_ceil_ps(lhs.sse);
#else
	v.x = ceil(lhs.x);
	v.y = ceil(lhs.y);
	v.z = ceil(lhs.z);
	v.z = ceil(lhs.w); 
#endif
	return v;
}

inline vec4 vec4_floor(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = _mm_floor_ps(lhs.sse);
#else
	v.x = floor(lhs.x);
	v.y = floor(lhs.y);
	v.z = floor(lhs.z);
	v.z = floor(lhs.w);
#endif
	return v;
}

inline vec4 vec4_round(vec4 lhs){DPZoneScoped;
	vec4 v;
#if DESHI_USE_SSE
	v.sse = _mm_round_ps(lhs.sse, (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
#else
	v.x = round(lhs.x);
	v.y = round(lhs.y);
	v.z = round(lhs.z);
	v.z = round(lhs.w);
#endif
	return v;
}


#ifdef __cplusplus
EndLinkageC();
#endif //#ifdef __cplusplus
//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec_interations
inline vec2 vec2i::
toVec2()const{
	return vec2{f32(x), f32(y)};
}

inline vec2i vec2::
toVec2i()const{
	return vec2i{s32(this->x), s32(this->y)};
}

inline vec3 vec2::
toVec3()const{
	return Vec3(this->x, this->y, 0);
}

inline vec4 vec2::
toVec4()const{
	return Vec4(this->x, this->y, 0, 0);
}

inline vec2 vec3::
toVec2()const{
	vec2 v;
	v.x = this->x;
	v.y = this->y;
	return v;
}

inline vec4 vec3::
toVec4()const{
	vec4 v;
	v.x = this->x;
	v.y = this->y;
	v.z = this->z;
	v.w = 1;
	return v;
}

inline vec3 vec4::
toVec3()const{
	return Vec3(x, y, z);
}

//takes data from a vec3 and leaves w alone
inline void vec4::
takeVec3(const vec3& v){
	x = v.x;
	y = v.y;
	z = v.z;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec_macros
#define randvec2(a) Vec2(rand() % a + 1, rand() % a + 1)
#define randvec3(a) Vec3(rand() % a + 1, rand() % a + 1, rand() % a + 1)
#define randvec4(a) Vec4(rand() % a + 1, rand() % a + 1, rand() % a + 1, rand() % a + 1);


//averages a vector v over an interval i and returns that average
#define V_AVG(i, v) \
([&] { \
persist std::vector<vec3> vectors; \
persist vec3 nv; \
persist s32 iter = 0; \
if(i == vectors.size()){ \
vectors.erase(vectors.begin()); \
vectors.push_back(v); \
iter++; \
} \
else{ \
vectors.push_back(v); \
iter++; \
}\
if(iter == i){ \
nv = Math::averagevec3(vectors); \
iter = 0; \
} \
return nv; \
}())

//averages vectors but consistently returns the value
#define V_AVGCON(i, v) \
([&] { \
persist std::vector<vec3> vectors; \
persist vec3 nv; \
if(i == vectors.size()){ \
vectors.erase(vectors.begin()); \
vectors.push_back(v); \
} \
else{ \
vectors.push_back(v); \
} \
nv = Math::averagevec3(vectors); \
return nv; \
}())

//this stores an input vector and returns the previously stored vector
//if you pass true for the second param it will replace the stored vector and return it
//else it just returns the stored vector
#define V_STORE(v, t) \
([&]()->vec3{\
persist vec3 vect[1];\
vec3 vr = vect[0];\
if(t){ vect[0] = v; return vr; } \
else return vr; }\
())



//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec_rounding
FORCE_INLINE vec2 floor(vec2 in){ return vec2_floor(in); }
FORCE_INLINE vec3 floor(vec3 in){ return vec3_floor(in); }
FORCE_INLINE vec4 floor(vec4 in){ return vec4_floor(in); }
FORCE_INLINE vec2 ceil(vec2 in) { return vec2_ceil(in); }
FORCE_INLINE vec3 ceil(vec3 in) { return vec3_ceil(in); }
FORCE_INLINE vec4 ceil(vec4 in) { return vec4_ceil(in); }
FORCE_INLINE vec2 round(vec2 in){ return vec2_round(in); }
FORCE_INLINE vec3 round(vec3 in){ return vec3_round(in); }
FORCE_INLINE vec4 round(vec4 in){ return vec4_round(in); }


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec_minmax
template<> FORCE_INLINE vec2i Min(vec2i a, vec2i b)                        { return Vec2i(Min(a.x, b.x), Min(a.y, b.y));}
template<> FORCE_INLINE vec2i Max(vec2i a, vec2i b)                        { return Vec2i(Max(a.x, b.x), Max(a.y, b.y)); }
template<> FORCE_INLINE vec2i Clamp(vec2i value, vec2i min, vec2i max)     { return Vec2i(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y)); };
template<> FORCE_INLINE vec2i ClampMin(vec2i value, vec2i min)             { return Vec2i(ClampMin(value.x, min.x), ClampMin(value.y, min.y)); };
template<> FORCE_INLINE vec2i ClampMax(vec2i value,  vec2i max)            { return Vec2i(ClampMax(value.x, max.x), ClampMax(value.y, max.y)); };
template<> FORCE_INLINE vec2i Nudge(vec2i value, vec2i target, vec2i delta){ return Vec2i(Nudge(value.x, target.x, delta.x), Nudge(value.y, target.y, delta.y)); }
template<> FORCE_INLINE vec2 Min(vec2 a, vec2 b)                           { return Vec2(Min(a.x, b.x), Min(a.y, b.y));}
template<> FORCE_INLINE vec2 Max(vec2 a, vec2 b)                           { return Vec2(Max(a.x, b.x), Max(a.y, b.y)); }
template<> FORCE_INLINE vec2 Clamp(vec2 value, vec2 min, vec2 max)         { return Vec2(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y)); };
template<> FORCE_INLINE vec2 ClampMin(vec2 value, vec2 min)                { return Vec2(ClampMin(value.x, min.x), ClampMin(value.y, min.y)); };
template<> FORCE_INLINE vec2 ClampMax(vec2 value,  vec2 max)               { return Vec2(ClampMax(value.x, max.x), ClampMax(value.y, max.y)); };
template<> FORCE_INLINE vec2 Nudge(vec2 value, vec2 target, vec2 delta)    { return Vec2(Nudge(value.x, target.x, delta.x), Nudge(value.y, target.y, delta.y)); }
template<> FORCE_INLINE vec3 Min(vec3 a, vec3 b)                           { return Vec3(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z)); }
template<> FORCE_INLINE vec3 Max(vec3 a, vec3 b)                           { return Vec3(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z)); }
template<> FORCE_INLINE vec3 Clamp(vec3 value, vec3 min, vec3 max)         { return Vec3(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y), Clamp(value.z, min.z, max.z)); };
template<> FORCE_INLINE vec3 ClampMin(vec3 value, vec3 min)                { return Vec3(ClampMin(value.x, min.x), ClampMin(value.y, min.y), ClampMin(value.z, min.z)); };
template<> FORCE_INLINE vec3 ClampMax(vec3 value, vec3 max)                { return Vec3(ClampMax(value.x, max.x), ClampMax(value.y, max.y), ClampMax(value.z, max.z)); };
template<> FORCE_INLINE vec3 Nudge(vec3 value, vec3 target, vec3 delta)    { return Vec3(Nudge(value.x, target.x, delta.x), Nudge(value.y, target.y, delta.y), Nudge(value.z, target.z, delta.z)); }
template<> FORCE_INLINE vec4 Min(vec4 a, vec4 b)                           { return Vec4(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z), Min(a.w, b.w)); }
template<> FORCE_INLINE vec4 Max(vec4 a, vec4 b)                           { return Vec4(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z), Max(a.w, b.w)); }
template<> FORCE_INLINE vec4 Clamp(vec4 value, vec4 min, vec4 max)         { return Vec4(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y), Clamp(value.z, min.z, max.z), Clamp(value.w, min.w, max.w)); };
template<> FORCE_INLINE vec4 ClampMin(vec4 value, vec4 min)                { return Vec4(ClampMin(value.x, min.x), ClampMin(value.y, min.y), ClampMin(value.z, min.z), ClampMin(value.w, min.w)); };
template<> FORCE_INLINE vec4 ClampMax(vec4 value, vec4 max)                { return Vec4(ClampMax(value.x, max.x), ClampMax(value.y, max.y), ClampMax(value.z, max.z), ClampMax(value.w, max.w)); };
template<> FORCE_INLINE vec4 Nudge(vec4 value, vec4 target, vec4 delta)    { return Vec4(Nudge(value.x, target.x, delta.x), Nudge(value.y, target.y, delta.y), Nudge(value.z, target.z, delta.z), Nudge(value.w, target.w, delta.w)); }


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec_hashing
#include "kigu/hash.h"

//TODO(sushi) always explain your hashing
template<> 
struct hash<vec2>{
	inline size_t operator()(vec2 const& v)const{
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};

template<> 
struct hash<vec3>{
	inline size_t operator()(vec3 const& v)const{
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.z); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};

template<> 
struct hash<vec4>{
	inline size_t operator()(vec4 const& v)const{
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.z); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.w); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec_tostring
#include "kigu/string.h"

global string 
to_string(const vec2& x, bool trunc = true, Allocator* a = KIGU_STRING_ALLOCATOR){
	string s(a);
	if(trunc){
		s.count = snprintf(nullptr, 0, "(%g, %g)", x.x, x.y);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%g, %g)", x.x, x.y);
	}else{
		s.count = snprintf(nullptr, 0, "(%+f, %+f)", x.x, x.y);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%+f, %+f)", x.x, x.y);
	}
	return s;
}

global string 
to_string(const vec3& x, bool trunc = true, Allocator* a = KIGU_STRING_ALLOCATOR){
	string s(a);
	if(trunc){
		s.count = snprintf(nullptr, 0, "(%g, %g, %g)", x.x, x.y, x.z);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%g, %g, %g)", x.x, x.y, x.z);
	}else{
		s.count = snprintf(nullptr, 0, "(%+f, %+f, %+f)", x.x, x.y, x.z);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%+f, %+f, %+f)", x.x, x.y, x.z);
	}
	return s;
}

global string 
to_string(const vec4& x, bool trunc = true, Allocator* a = KIGU_STRING_ALLOCATOR){
	string s(a);
	if(trunc){
		s.count = snprintf(nullptr, 0, "(%g, %g, %g, %g)", x.x, x.y, x.z, x.w);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%g, %g, %g, %g)", x.x, x.y, x.z, x.w);
	}else{
		s.count = snprintf(nullptr, 0, "(%+f, %+f, %+f, %+f)", x.x, x.y, x.z, x.w);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%+f, %+f, %+f, %+f)", x.x, x.y, x.z, x.w);
	}
	return s;
}

global str8
to_str8(const vec2& x, bool trunc = true, Allocator* a = KIGU_STRING_ALLOCATOR){
	str8b s; s.allocator = a;
	if(trunc){
		s.count = snprintf(nullptr, 0, "(%g, %g)", x.x, x.y);
		s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf((char*)s.str, s.count+1, "(%g, %g)", x.x, x.y);
	}else{
		s.count = snprintf(nullptr, 0, "(%+f, %+f)", x.x, x.y);
		s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf((char*)s.str, s.count+1, "(%+f, %+f)", x.x, x.y);
	}
	return s.fin;
}

global str8
to_str8(const vec3& x, bool trunc = true, Allocator* a = KIGU_STRING_ALLOCATOR){
	str8b s; s.allocator = a;
	if(trunc){
		s.count = snprintf(nullptr, 0, "(%g, %g, %g)", x.x, x.y, x.z);
		s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf((char*)s.str, s.count+1, "(%g, %g, %g)", x.x, x.y, x.z);
	}else{
		s.count = snprintf(nullptr, 0, "(%+f, %+f, %+f)", x.x, x.y, x.z);
		s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf((char*)s.str, s.count+1, "(%+f, %+f, %+f)", x.x, x.y, x.z);
	}
	return s.fin;
}

global str8
to_str8(const vec4& x, bool trunc = true, Allocator* a = KIGU_STRING_ALLOCATOR){
	str8b s; s.allocator = a;
	if(trunc){
		s.count = snprintf(nullptr, 0, "(%g, %g, %g, %g)", x.x, x.y, x.z, x.w);
		s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf((char*)s.str, s.count+1, "(%g, %g, %g, %g)", x.x, x.y, x.z, x.w);
	}else{
		s.count = snprintf(nullptr, 0, "(%+f, %+f, %+f, %+f)", x.x, x.y, x.z, x.w);
		s.str   = (u8*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf((char*)s.str, s.count+1, "(%+f, %+f, %+f, %+f)", x.x, x.y, x.z, x.w);
	}
	return s.fin;
}

#endif //DESHI_VECTOR_H