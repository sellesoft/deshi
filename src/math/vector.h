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
typedef union vec2i{
	//// data ////
	s32 arr[2] = {0};
	struct{ s32 x, y; };
	struct{ s32 r, g; };
	struct{ s32 w, h; };
	struct{ s32 u, v; };
	
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
		v.x = abs(x);
		v.y = abs(y);
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
		return sqrt(x * x + y * y);
	}
	
	inline s32   magSq()const{DPZoneScoped;
		return x*x + y*y;
	}
	
	inline void  normalize(){DPZoneScoped;
		if(*this != vec2i::ZERO){
			*this /= this->mag();
		}
	}
	
	inline vec2i normalized()const{DPZoneScoped;
		if(*this != vec2i::ZERO){
			return *this / this->mag();
		}
		return *this;
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
			return normalized() * min;
		}else if(m > max){
			return normalized() * max;
		}else{
			return *this;
		}
	}
	
	inline s32   distanceTo(const vec2i& rhs)const{DPZoneScoped;
		return (*this - rhs).mag();
	}
	
	inline vec2i compOn(const vec2i& rhs)const{DPZoneScoped;
		return rhs.normalized() * this->projectOn(rhs);
	}
	
	inline s32   projectOn(const vec2i& rhs)const{DPZoneScoped;
		if(this->mag() > M_EPSILON){
			return this->dot(rhs) / this->mag();
		}else{
			return 0;
		}
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
	
	//// interactions ////
	vec3 toVec3()const;
	vec4 toVec4()const;
#endif //#ifdef __cplusplus
} vec2i;


//// member constants ////
inline const vec2i vec2i::ZERO  = vec2i{ 0, 0};
inline const vec2i vec2i::ONE   = vec2i{ 1, 1};
inline const vec2i vec2i::UP    = vec2i{ 0, 1};
inline const vec2i vec2i::DOWN  = vec2i{ 0,-1};
inline const vec2i vec2i::LEFT  = vec2i{-1, 0};
inline const vec2i vec2i::RIGHT = vec2i{ 1, 0};
inline const vec2i vec2i::UNITX = vec2i{ 1, 0};
inline const vec2i vec2i::UNITY = vec2i{ 0, 1};


//// nonmember constructor ////
inline vec2i Vec2i(s32 x, s32 y){DPZoneScoped;
	vec2i v;
	v.x = x;
	v.y = y;
	return v;
}


//// nonmember constants ////
FORCE_INLINE vec2i vec2i_ZERO(){
	vec2i v;
	v.x = 0;
	v.y = 0;
	return v;
};

FORCE_INLINE vec2i vec2i_ONE(){
	vec2i v;
	v.x = 1;
	v.y = 1;
	return v;
};

FORCE_INLINE vec2i vec2i_UP(){
	vec2i v;
	v.x = 0;
	v.y = 1;
	return v;
};

FORCE_INLINE vec2i vec2i_DOWN(){
	vec2i v;
	v.x = 0;
	v.y = -1;
	return v;
};

FORCE_INLINE vec2i vec2i_LEFT(){
	vec2i v;
	v.x = -1;
	v.y = 0;
	return v;
};

FORCE_INLINE vec2i vec2i_RIGHT(){
	vec2i v;
	v.x = 1;
	v.y = 0;
	return v;
};

FORCE_INLINE vec2i vec2i_UNITX(){
	vec2i v;
	v.x = 1;
	v.y = 0;
	return v;
};

FORCE_INLINE vec2i vec2i_UNITY(){
	vec2i v;
	v.x = 0;
	v.y = 1;
	return v;
};


//// nonmember operators ////
FORCE_INLINE vec2i operator* (s32 lhs, vec2i rhs){
	return rhs * lhs;
}

inline vec2i vec2i_add(vec2i lhs, vec2i rhs){DPZoneScoped;
	return lhs + rhs;
}

inline vec2i vec2i_subtract(vec2i lhs, vec2i rhs){DPZoneScoped;
	return lhs - rhs;
}

inline vec2i vec2i_multipy(vec2i lhs, vec2i rhs){DPZoneScoped;
	return lhs * rhs;
}

inline vec2i vec2i_multipy_constant(vec2i lhs, s32 rhs){DPZoneScoped;
	return lhs * rhs;
}

inline vec2i vec2i_divide(vec2i lhs, vec2i rhs){DPZoneScoped;
	return lhs / rhs;
}

inline vec2i vec2i_divide_constant(vec2i lhs, s32 rhs){DPZoneScoped;
	return lhs / rhs;
}

inline vec2i vec2i_negate(vec2i lhs){DPZoneScoped;
	return -lhs;
}

inline b32   vec2i_equal(vec2i lhs, vec2i rhs){DPZoneScoped;
	return lhs == rhs;
}


//// nonmember functions ////
inline vec2i vec2i_absV(vec2i lhs){DPZoneScoped;
	return lhs.absV();
}

inline s32   vec2i_dot(vec2i lhs, vec2i rhs){DPZoneScoped;
	return lhs.dot(rhs);
}

inline vec2i vec2i_perp(vec2i lhs){DPZoneScoped;
	return lhs.perp();
}

inline s32   vec2i_mag(vec2i lhs){DPZoneScoped;
	return lhs.mag();
}

inline s32   vec2i_magSq(vec2i lhs){DPZoneScoped;
	return lhs.magSq();
}

inline vec2i vec2i_normalized(vec2i lhs){DPZoneScoped;
	return lhs.normalized();
}

inline vec2i vec2i_clampedMag(vec2i lhs, s32 min, s32 max){DPZoneScoped;
	return lhs.clampedMag(min, max);
}

inline s32   vec2i_distanceTo(vec2i lhs, vec2i rhs){DPZoneScoped;
	return lhs.distanceTo(rhs);
}

inline vec2i vec2i_compOn(vec2i lhs, vec2i rhs){DPZoneScoped;
	return lhs.compOn(rhs);
}

inline s32   vec2i_projectOn(vec2i lhs, vec2i rhs){DPZoneScoped;
	return lhs.projectOn(rhs);
}

inline vec2i vec2i_midpoint(vec2i lhs, vec2i rhs){DPZoneScoped;
	return lhs.midpoint(rhs);
}

inline vec2i vec2i_xComp(vec2i lhs){DPZoneScoped;
	return lhs.xComp();
}

inline vec2i vec2i_yComp(vec2i lhs){DPZoneScoped;
	return lhs.yComp();
}

inline vec2i vec2i_xInvert(vec2i lhs){DPZoneScoped;
	return lhs.xInvert();
}

inline vec2i vec2i_yInvert(vec2i lhs){DPZoneScoped;
	return lhs.yInvert();
}

inline vec2i vec2i_xSet(vec2i lhs, s32 a){DPZoneScoped;
	return lhs.xSet(a);
}

inline vec2i vec2i_ySet(vec2i lhs, s32 a){DPZoneScoped;
	return lhs.ySet(a);
}

inline vec2i vec2i_xAdd(vec2i lhs, s32 a){DPZoneScoped;
	return lhs.xAdd(a);
}

inline vec2i vec2i_yAdd(vec2i lhs, s32 a){DPZoneScoped;
	return lhs.yAdd(a);
}

#ifdef __cplusplus
EndLinkageC();
#endif //#ifdef __cplusplus
//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec2
struct vec2 {
	union{
		f32 arr[2] = {};
		struct{ f32 x, y; };
		struct{ f32 r, g; };
		struct{ f32 w, h; };
		struct{ f32 u, v; };
	};
	
	vec2(){};
	vec2(f32 inX, f32 inY);
	vec2(const vec2& v);
	vec2(f32* ptr);
	vec2(vec2i v);
	
	static const vec2 ZERO;
	static const vec2 ONE;
	static const vec2 UP;
	static const vec2 DOWN;
	static const vec2 LEFT;
	static const vec2 RIGHT;
	static const vec2 UNITX;
	static const vec2 UNITY;
	
	void operator= (const vec2& rhs);
	vec2 operator* (f32 rhs) const;
	void operator*=(f32 rhs);
	vec2 operator/ (f32 rhs) const;
	void operator/=(f32 rhs);
	vec2 operator+ (const vec2& rhs) const;
	void operator+=(const vec2& rhs);
	vec2 operator- (const vec2& rhs) const;
	void operator-=(const vec2& rhs);
	vec2 operator* (const vec2& rhs) const;
	void operator*=(const vec2& rhs);
	vec2 operator/ (const vec2& rhs) const;
	void operator/=(const vec2& rhs);
	vec2 operator- () const;
	bool operator==(const vec2& rhs) const;
	bool operator!=(const vec2& rhs) const;
	friend vec2 operator* (f32 lhs, const vec2& rhs){ return rhs * lhs; }
	
	void set(f32 x, f32 y);
	vec2 absV() const;
	vec2 copy() const;
	f32  dot(const vec2& rhs) const;
	vec2 perp() const;
	f32  mag()  const;
	f32  magSq()  const;
	void normalize();
	vec2 normalized() const;
	void clampMag(f32 min, f32 max);
	vec2 clampedMag(f32 min, f32 max) const;
	f32  distanceTo(const vec2& rhs)  const;
	vec2 compOn(const vec2& rhs)      const;
	f32  projectOn(const vec2& rhs)   const;
	vec2 midpoint(const vec2& rhs)    const;
	vec2 xComp() const;
	vec2 yComp() const;
	vec2 xInvert() const;
	vec2 yInvert() const;
	vec2 xSet(f32 set) const; 
	vec2 ySet(f32 set) const; 
	vec2 xAdd(f32 set) const; 
	vec2 yAdd(f32 set) const; 
	vec2 ceil() const;
	vec2 floor() const;
	
	//vector interactions
	vec2(const vec3& v);
	vec2(const vec4& v);
	vec2(const vec2g& v);
	vec3 toVec3() const;
	vec4 toVec4() const;
};
#include "vec2.inl"


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec3i
union vec3i{
	s32 arr[3] = {};
	struct{ s32 x, y, z; };
	struct{ s32 r, g, b; };
	struct{ vec2i xy; s32 _unusedZ0; };
	struct{ s32 _unusedX0; vec2i yz; };
};


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec3
struct vec3 {
	union {
		f32 arr[3] = {};
		struct{ f32 x, y, z; };
		struct{ f32 r, g, b; };
		struct{ vec2 xy; f32 _unusedZ0; };
		struct{ f32 _unusedX0; vec2 yz; };
	};
	
	vec3(){};
	vec3(f32 inX, f32 inY, f32 inZ);
	vec3(f32 inX, f32 inY);
	vec3(const vec3& v);
	vec3(f32* ptr);
	vec3(vec3i v);
	
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
	
	void operator= (const vec3& rhs);
	void operator= (vec3& rhs);
	vec3 operator* (f32 rhs) const;
	void operator*=(f32 rhs);
	vec3 operator/ (f32 rhs) const;
	void operator/=(f32 rhs);
	vec3 operator+ (const vec3& rhs) const;
	void operator+=(const vec3& rhs);
	vec3 operator- (const vec3& rhs) const;
	void operator-=(const vec3& rhs);
	vec3 operator* (const vec3& rhs) const;
	void operator*=(const vec3& rhs);
	vec3 operator/ (const vec3& rhs) const;
	void operator/=(const vec3& rhs);
	vec3 operator- () const;
	bool operator==(const vec3& rhs) const;
	bool operator!=(const vec3& rhs) const;
	friend vec3 operator* (f32 lhs, const vec3& rhs){ return rhs * lhs; }
	
	void set(f32 x, f32 y, f32 z);
	vec3 absV() const;
	vec3 copy() const;
	f32  dot(const vec3& rhs) const;
	vec3 cross(const vec3& rhs) const;
	f32  mag() const;
	f32  magSq() const;
	void normalize();
	vec3 normalized() const;
	vec3 clamp(f32 lo, f32 hi) const;
	void clampMag(f32 min, f32 max);
	vec3 clampedMag(f32 min, f32 max) const;
	void round(s32 place);
	vec3 rounded(s32 place) const;
	f32  distanceTo(const vec3& rhs) const;
	vec3 compOn(const vec3& rhs) const;
	f32  projectOn(const vec3& rhs) const;
	vec3 midpoint(const vec3& rhs) const;
	f32  angleBetween(const vec3& rhs) const;
	vec3 xComp() const;
	vec3 yComp() const;
	vec3 zComp() const;
	vec3 xZero() const;
	vec3 yZero() const;
	vec3 zZero() const;
	vec3 xInvert() const;
	vec3 yInvert() const;
	vec3 zInvert() const;
	vec3 ceil() const;
	vec3 floor() const;
	
	//vector interactions
	vec3(const vec2& v);
	vec3(const vec4& v);
	vec2 toVec2() const;
	vec4 toVec4() const;
	
	//matrix interactions
	vec3 operator* (const mat3& rhs) const;
	void operator*=(const mat3& rhs);
	vec3 operator* (const mat4& rhs) const;
	void operator*=(const mat4& rhs);
	//matN ToM1x3() const;
	//matN ToM1x4(f32 w) const;
	
	//quaternion interactions
	vec3 operator* (const quat& rhs) const;
};
#include "vec3.inl"


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec4i
union vec4i{
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


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec4
struct vec4{
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
	
	vec4(){};
	vec4(f32 inX, f32 inY, f32 inZ, f32 inW);
	vec4(const vec4& v);
	vec4(f32* ptr);
	vec4(vec4i v);
	
	static const vec4 ZERO;
	static const vec4 ONE;
	
	void operator= (const vec4& rhs);
	vec4 operator* (const f32& rhs) const;
	void operator*=(const f32& rhs);
	vec4 operator/ (const f32& rhs) const;
	void operator/=(const f32& rhs);
	vec4 operator+ (const vec4& rhs) const;
	void operator+=(const vec4& rhs);
	vec4 operator- (const vec4& rhs) const;
	void operator-=(const vec4& rhs);
	vec4 operator* (const vec4& rhs) const;
	void operator*=(const vec4& rhs);
	vec4 operator/ (const vec4& rhs) const;
	void operator/=(const vec4& rhs);
	vec4 operator- () const;
	bool operator==(const vec4& rhs) const;
	bool operator!=(const vec4& rhs) const;
	friend vec4 operator* (const f32& lhs, const vec4& rhs){ return rhs * lhs; }
	
	void set(f32 x, f32 y, f32 z, f32 w);
	vec4 absV() const;
	vec4 copy() const;
	f32  dot(const vec4& rhs) const;
	f32  magSq() const;
	f32  mag() const;
	vec4 wnormalized() const;
	vec4 xComp() const;
	vec4 yComp() const;
	vec4 zComp() const;
	vec4 wComp() const;
	vec4 xInvert() const;
	vec4 yInvert() const;
	vec4 zInvert() const;
	vec4 wInvert() const;
	vec4 ceil() const;
	vec4 floor() const;
	
	//vector interactions
	vec4(const vec2& v, f32 z, f32 w);
	vec4(const vec3& v, f32 w);
	vec3 toVec3() const;
	void takeVec3(const vec3& v);
	
	//matrix interactions
	vec4 operator* (const mat4& rhs) const;
	void operator*=(const mat4& rhs);
};
#include "vec4.inl"


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec_interations
inline vec2::
vec2(const vec3& v){
	x = v.x; y = v.y;
}

inline vec2::
vec2(const vec4& v){
	x = v.x; y = v.y;
}

inline vec3 vec2::
toVec3() const {
	return vec3(x, y, 0);
}

inline vec4 vec2::
toVec4() const {
	return vec4(x, y, 0, 0);
}

inline vec3::
vec3(const vec2& v) {
	x = v.x; y = v.y; z = 0;
}

inline vec3::
vec3(const vec4& v) {
	x = v.x; y = v.y; z = v.z;
}

inline vec2 vec3::
toVec2() const {
	return vec2(x, y);
}

inline vec4 vec3::
toVec4() const {
	return vec4(x, y, z, 1);
}

inline vec4::
vec4(const vec2& v, f32 inZ, f32 inW) {
	x = v.x; y = v.y; z = inZ; w = inW;
}

inline vec4::
vec4(const vec3& v, f32 inW) {
	x = v.x; y = v.y; z = v.z; w = inW;
}

inline vec3 vec4::
toVec3() const {
	return vec3(x, y, z);
}

//takes data from a vec3 and leaves w alone
inline void vec4::
takeVec3(const vec3& v) {
	x = v.x; y = v.y; z = v.z;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec_macros
#define randvec2(a) vec2(rand() % a + 1, rand() % a + 1)
#define randvec3(a) vec3(rand() % a + 1, rand() % a + 1, rand() % a + 1)
#define randvec4(a) vec4(rand() % a + 1, rand() % a + 1, rand() % a + 1, rand() % a + 1);


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
FORCE_INLINE vec2 floor(vec2 in){ return vec2(floor(in.x), floor(in.y)); }
FORCE_INLINE vec3 floor(vec3 in){ return vec3(floor(in.x), floor(in.y), floor(in.z)); }
FORCE_INLINE vec4 floor(vec4 in){ return vec4(floor(in.x), floor(in.y), floor(in.z), floor(in.w)); }
FORCE_INLINE vec2 ceil(vec2 in) { return vec2(ceil(in.x), ceil(in.y)); }
FORCE_INLINE vec3 ceil(vec3 in) { return vec3(ceil(in.x), ceil(in.y), ceil(in.z)); }
FORCE_INLINE vec4 ceil(vec4 in) { return vec4(ceil(in.x), ceil(in.y), ceil(in.z), ceil(in.w)); }
FORCE_INLINE vec2 round(vec2 in){ return vec2(round(in.x), round(in.y)); }
FORCE_INLINE vec3 round(vec3 in){ return vec3(round(in.x), round(in.y), round(in.z)); }
FORCE_INLINE vec4 round(vec4 in){ return vec4(round(in.x), round(in.y), round(in.z), round(in.w)); }


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec_minmax
template<> FORCE_INLINE vec2i Min(vec2i a, vec2i b)                        { return Vec2i(Min(a.x, b.x), Min(a.y, b.y));}
template<> FORCE_INLINE vec2i Max(vec2i a, vec2i b)                        { return Vec2i(Max(a.x, b.x), Max(a.y, b.y)); }
template<> FORCE_INLINE vec2i Clamp(vec2i value, vec2i min, vec2i max)     { return Vec2i(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y)); };
template<> FORCE_INLINE vec2i ClampMin(vec2i value, vec2i min)             { return Vec2i(ClampMin(value.x, min.x), ClampMin(value.y, min.y)); };
template<> FORCE_INLINE vec2i ClampMax(vec2i value,  vec2i max)            { return Vec2i(ClampMax(value.x, max.x), ClampMax(value.y, max.y)); };
template<> FORCE_INLINE vec2i Nudge(vec2i value, vec2i target, vec2i delta){ return Vec2i(Nudge(value.x, target.x, delta.x), Nudge(value.y, target.y, delta.y)); }
template<> FORCE_INLINE vec2 Min(vec2 a, vec2 b)                           { return vec2(Min(a.x, b.x), Min(a.y, b.y));}
template<> FORCE_INLINE vec2 Max(vec2 a, vec2 b)                           { return vec2(Max(a.x, b.x), Max(a.y, b.y)); }
template<> FORCE_INLINE vec2 Clamp(vec2 value, vec2 min, vec2 max)         { return vec2(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y)); };
template<> FORCE_INLINE vec2 ClampMin(vec2 value, vec2 min)                { return vec2(ClampMin(value.x, min.x), ClampMin(value.y, min.y)); };
template<> FORCE_INLINE vec2 ClampMax(vec2 value,  vec2 max)               { return vec2(ClampMax(value.x, max.x), ClampMax(value.y, max.y)); };
template<> FORCE_INLINE vec2 Nudge(vec2 value, vec2 target, vec2 delta)    { return vec2(Nudge(value.x, target.x, delta.x), Nudge(value.y, target.y, delta.y)); }
template<> FORCE_INLINE vec3 Min(vec3 a, vec3 b)                           { return vec3(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z)); }
template<> FORCE_INLINE vec3 Max(vec3 a, vec3 b)                           { return vec3(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z)); }
template<> FORCE_INLINE vec3 Clamp(vec3 value, vec3 min, vec3 max)         { return vec3(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y), Clamp(value.z, min.z, max.z)); };
template<> FORCE_INLINE vec3 ClampMin(vec3 value, vec3 min)                { return vec3(ClampMin(value.x, min.x), ClampMin(value.y, min.y), ClampMin(value.z, min.z)); };
template<> FORCE_INLINE vec3 ClampMax(vec3 value, vec3 max)                { return vec3(ClampMax(value.x, max.x), ClampMax(value.y, max.y), ClampMax(value.z, max.z)); };
template<> FORCE_INLINE vec3 Nudge(vec3 value, vec3 target, vec3 delta)    { return vec3(Nudge(value.x, target.x, delta.x), Nudge(value.y, target.y, delta.y), Nudge(value.z, target.z, delta.z)); }
template<> FORCE_INLINE vec4 Min(vec4 a, vec4 b)                           { return vec4(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z), Min(a.w, b.w)); }
template<> FORCE_INLINE vec4 Max(vec4 a, vec4 b)                           { return vec4(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z), Max(a.w, b.w)); }
template<> FORCE_INLINE vec4 Clamp(vec4 value, vec4 min, vec4 max)         { return vec4(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y), Clamp(value.z, min.z, max.z), Clamp(value.w, min.w, max.w)); };
template<> FORCE_INLINE vec4 ClampMin(vec4 value, vec4 min)                { return vec4(ClampMin(value.x, min.x), ClampMin(value.y, min.y), ClampMin(value.z, min.z), ClampMin(value.w, min.w)); };
template<> FORCE_INLINE vec4 ClampMax(vec4 value, vec4 max)                { return vec4(ClampMax(value.x, max.x), ClampMax(value.y, max.y), ClampMax(value.z, max.z), ClampMax(value.w, max.w)); };
template<> FORCE_INLINE vec4 Nudge(vec4 value, vec4 target, vec4 delta)    { return vec4(Nudge(value.x, target.x, delta.x), Nudge(value.y, target.y, delta.y), Nudge(value.z, target.z, delta.z), Nudge(value.w, target.w, delta.w)); }


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @vec_hashing
#include "kigu/hash.h"

//TODO(sushi) always explain your hashing
template<> 
struct hash<vec2>{
	inline size_t operator()(vec2 const& v) const{
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};

template<> 
struct hash<vec3>{
	inline size_t operator()(vec3 const& v) const{
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
	inline size_t operator()(vec4 const& v) const{
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

global str8b 
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
	return s;
}

global str8b 
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
	return s;
}

global str8b 
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
	return s;
}

#endif //DESHI_VECTOR_H