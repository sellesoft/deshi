#include "vector.h"
#include "kigu/profiling.h"
#pragma once
#ifndef DESHI_VEC2_INL
#define DESHI_VEC2_INL

//////////////////////
//// constructors ////
//////////////////////

inline vec2i::
vec2i(s32 inX, s32 inY) {DPZoneScoped; 
	x = inX; y = inY; 
}

inline vec2i::
vec2i(const vec2i& v){DPZoneScoped; 
	x = v.x; y = v.y; 
}

inline vec2i::
vec2i(s32* ptr){DPZoneScoped; 
	x = *ptr; y = *(ptr+1);
}

inline vec2i::
vec2i(vec2 v){DPZoneScoped; 
	x = (s32)v.x; y = (s32)v.y;
}


///////////////////
//// constants ////
///////////////////

inline const vec2i vec2i::ZERO =  vec2i( 0, 0);
inline const vec2i vec2i::ONE =   vec2i( 1, 1);
inline const vec2i vec2i::RIGHT = vec2i( 1, 0);
inline const vec2i vec2i::LEFT =  vec2i(-1, 0);
inline const vec2i vec2i::UP =    vec2i( 0, 1);
inline const vec2i vec2i::DOWN =  vec2i( 0,-1);
inline const vec2i vec2i::UNITX = vec2i( 1, 0);
inline const vec2i vec2i::UNITY = vec2i( 0, 1);

///////////////////
//// operators ////
///////////////////

inline void vec2i::
operator=(const vec2i& rhs) {DPZoneScoped;
	this->x = rhs.x; this->y = rhs.y;
}

inline vec2i vec2i::
operator*(s32 rhs) const{DPZoneScoped;
	return vec2i(this->x * rhs, this->y * rhs);
}

inline void vec2i::
operator*=(s32 rhs) {DPZoneScoped;
	this->x *= rhs; this->y *= rhs;
}

inline vec2i vec2i::
operator/(s32 rhs) const{DPZoneScoped;
	return vec2i(this->x / rhs, this->y / rhs);
}

inline void vec2i::
operator/=(s32 rhs) {DPZoneScoped;
	this->x /= rhs; this->y /= rhs;
}

inline vec2i vec2i::
operator+(const vec2i& rhs) const{DPZoneScoped;
	return vec2i(this->x + rhs.x, this->y + rhs.y);
}

inline void vec2i::
operator+=(const vec2i& rhs) {DPZoneScoped;
	this->x += rhs.x; this->y += rhs.y;
}

inline vec2i vec2i::
operator-(const vec2i& rhs) const{DPZoneScoped;
	return vec2i(this->x - rhs.x, this->y - rhs.y);
}

inline void vec2i::
operator-=(const vec2i& rhs) {DPZoneScoped;
	this->x -= rhs.x; this->y -= rhs.y;
}

inline vec2i vec2i::
operator*(const vec2i& rhs) const{DPZoneScoped;
	return vec2i(this->x * rhs.x, this->y * rhs.y);
}

inline void vec2i::
operator*=(const vec2i& rhs) {DPZoneScoped;
	this->x *= rhs.x; this->y *= rhs.y;
}

inline vec2i vec2i::
operator/(const vec2i& rhs) const{DPZoneScoped;
	return vec2i(this->x / rhs.x, this->y / rhs.y);
}

inline void vec2i::
operator/=(const vec2i& rhs) {DPZoneScoped;
	this->x /= rhs.x; this->y /= rhs.y;
}

inline vec2i vec2i::
operator-() const{DPZoneScoped;
	return vec2i(-x, -y);
}

inline bool vec2i::
operator==(const vec2i& rhs) const{DPZoneScoped;
	return this->x == rhs.x && this->y == rhs.y;
}

inline bool vec2i::
operator!=(const vec2i& rhs) const{DPZoneScoped;
	return !(*this == rhs);
}

///////////////////
//// functions ////
///////////////////
inline void vec2i::
set(s32 _x, s32 _y){DPZoneScoped;
	x = _x; y = _y;
}

inline vec2i vec2i::
absV() const{DPZoneScoped;
	return vec2i(abs(x), abs(y));
}

inline vec2i vec2i::
copy() const{DPZoneScoped;
	return vec2i(x, y);
}

inline s32 vec2i::
dot(const vec2i& rhs) const{DPZoneScoped;
	return (this->x * rhs.x) + (this->y * rhs.y);
}

inline vec2i vec2i::
perp() const{DPZoneScoped;
	return vec2i(-y, x);
}

inline s32 vec2i::
mag() const{DPZoneScoped;
	return sqrt(x * x + y * y);
}

inline s32 vec2i::
magSq() const{DPZoneScoped;
	return x*x + y*y;
}

inline void vec2i::
normalize() {DPZoneScoped;
	if (*this != vec2i::ZERO) {DPZoneScoped;
		*this /= this->mag();
	}
}

inline vec2i vec2i::
normalized() const{DPZoneScoped;
	if(*this != vec2i::ZERO){DPZoneScoped;
		return *this / this->mag();
	}
	return *this;
}

inline void vec2i::
clampMag(s32 min, s32 max) {DPZoneScoped;
	s32 mag = this->mag();
	if (mag < min) {DPZoneScoped;
		this->normalize();
		*this *= min;
	}
	else if (mag > max) {DPZoneScoped;
		this->normalize();
		*this *= max;
	}
}

inline vec2i vec2i::
clampedMag(s32 min, s32 max) const{DPZoneScoped;
	s32 mag = this->mag();
	if (mag < min) {DPZoneScoped;
		return normalized() * min;
	}
	else if (mag > max) {DPZoneScoped;
		return normalized() * max;
	}
	else {
		return vec2i(this->x, this->y);
	}
}

inline s32 vec2i::
distanceTo(const vec2i& rhs) const{DPZoneScoped;
	return (*this - rhs).mag();
}

inline vec2i vec2i::
compOn(const vec2i& rhs) const{DPZoneScoped;
	return rhs.normalized() * this->projectOn(rhs);
}

inline s32 vec2i::
projectOn(const vec2i& rhs) const{DPZoneScoped;
	if(this->mag() > M_EPSILON){DPZoneScoped;
		return this->dot(rhs) / this->mag();
	}else{
		return 0;
	}
}

inline vec2i vec2i::
midpoint(const vec2i& rhs) const{DPZoneScoped;
	return vec2i((x+rhs.x)/2.f, (y+rhs.y)/2.f);
}

inline vec2i vec2i::
xComp() const{DPZoneScoped;
	return vec2i(x, 0);
}

inline vec2i vec2i::
yComp() const{DPZoneScoped;
	return vec2i(0, y);
}

inline vec2i vec2i::
xInvert() const{DPZoneScoped;
	return vec2i(-x, y);
}

inline vec2i vec2i::
yInvert() const{DPZoneScoped;
	return vec2i(x, -y);
}

inline vec2i vec2i::
xSet(s32 set) const{DPZoneScoped;
	return vec2i(set, y);
}

inline vec2i vec2i::
ySet(s32 set) const{DPZoneScoped;
	return vec2i(x, set);
}

inline vec2i vec2i::
xAdd(s32 add) const{DPZoneScoped;
	return vec2i(x + add, y);
}

inline vec2i vec2i::
yAdd(s32 add) const{DPZoneScoped;
	return vec2i(x, y + add);
}

inline vec2i vec2i::
ceil() const{DPZoneScoped;
	return vec2i(std::ceil(x), std::ceil(y));
};

inline vec2i vec2i::
floor() const{DPZoneScoped;
	return vec2i(std::floor(x), std::floor(y));
};

//////////////////////
//// constructors ////
//////////////////////

inline vec2::
vec2(f32 inX, f32 inY) {DPZoneScoped; 
	x = inX; y = inY; 
}

inline vec2::
vec2(const vec2& v){DPZoneScoped; 
	x = v.x; y = v.y; 
}

inline vec2::
vec2(f32* ptr){DPZoneScoped; 
	x = *ptr; y = *(ptr+1);
}

inline vec2::
vec2(vec2i v){DPZoneScoped; 
	x = (f32)v.x; y = (f32)v.y;
}


///////////////////
//// constants ////
///////////////////

inline const vec2 vec2::ZERO =  vec2( 0, 0);
inline const vec2 vec2::ONE =   vec2( 1, 1);
inline const vec2 vec2::RIGHT = vec2( 1, 0);
inline const vec2 vec2::LEFT =  vec2(-1, 0);
inline const vec2 vec2::UP =    vec2( 0, 1);
inline const vec2 vec2::DOWN =  vec2( 0,-1);
inline const vec2 vec2::UNITX = vec2( 1, 0);
inline const vec2 vec2::UNITY = vec2( 0, 1);

///////////////////
//// operators ////
///////////////////

inline void vec2::
operator=(const vec2& rhs) {DPZoneScoped;
	this->x = rhs.x; this->y = rhs.y;
}

inline vec2 vec2::
operator*(f32 rhs) const{DPZoneScoped;
	return vec2(this->x * rhs, this->y * rhs);
}

inline void vec2::
operator*=(f32 rhs) {DPZoneScoped;
	this->x *= rhs; this->y *= rhs;
}

inline vec2 vec2::
operator/(f32 rhs) const{DPZoneScoped;
	return vec2(this->x / rhs, this->y / rhs);
}

inline void vec2::
operator/=(f32 rhs) {DPZoneScoped;
	this->x /= rhs; this->y /= rhs;
}

inline vec2 vec2::
operator+(const vec2& rhs) const{DPZoneScoped;
	return vec2(this->x + rhs.x, this->y + rhs.y);
}

inline void vec2::
operator+=(const vec2& rhs) {DPZoneScoped;
	this->x += rhs.x; this->y += rhs.y;
}

inline vec2 vec2::
operator-(const vec2& rhs) const{DPZoneScoped;
	return vec2(this->x - rhs.x, this->y - rhs.y);
}

inline void vec2::
operator-=(const vec2& rhs) {DPZoneScoped;
	this->x -= rhs.x; this->y -= rhs.y;
}

inline vec2 vec2::
operator*(const vec2& rhs) const{DPZoneScoped;
	return vec2(this->x * rhs.x, this->y * rhs.y);
}

inline void vec2::
operator*=(const vec2& rhs) {DPZoneScoped;
	this->x *= rhs.x; this->y *= rhs.y;
}

inline vec2 vec2::
operator/(const vec2& rhs) const{DPZoneScoped;
	return vec2(this->x / rhs.x, this->y / rhs.y);
}

inline void vec2::
operator/=(const vec2& rhs) {DPZoneScoped;
	this->x /= rhs.x; this->y /= rhs.y;
}

inline vec2 vec2::
operator-() const{DPZoneScoped;
	return vec2(-x, -y);
}

inline bool vec2::
operator==(const vec2& rhs) const{DPZoneScoped;
	return fabs(this->x - rhs.x) < M_EPSILON 
		&& fabs(this->y - rhs.y) < M_EPSILON;
}

inline bool vec2::
operator!=(const vec2& rhs) const{DPZoneScoped;
	return !(*this == rhs);
}

///////////////////
//// functions ////
///////////////////
inline void vec2::
set(f32 _x, f32 _y){DPZoneScoped;
	x = _x; y = _y;
}

inline vec2 vec2::
absV() const{DPZoneScoped;
	return vec2(fabs(x), fabs(y));
}

inline vec2 vec2::
copy() const{DPZoneScoped;
	return vec2(x, y);
}

inline f32 vec2::
dot(const vec2& rhs) const{DPZoneScoped;
	return (this->x * rhs.x) + (this->y * rhs.y);
}

inline vec2 vec2::
perp() const{DPZoneScoped;
	return vec2(-y, x);
}

inline f32 vec2::
mag() const{DPZoneScoped;
	return sqrt(x * x + y * y);
}

inline f32 vec2::
magSq() const{DPZoneScoped;
	return x*x + y*y;
}

inline void vec2::
normalize() {DPZoneScoped;
	if (*this != vec2::ZERO) {DPZoneScoped;
		*this /= this->mag();
	}
}

inline vec2 vec2::
normalized() const{DPZoneScoped;
	if(*this != vec2::ZERO){DPZoneScoped;
		return *this / this->mag();
	}
	return *this;
}

inline void vec2::
clampMag(f32 min, f32 max) {DPZoneScoped;
	f32 mag = this->mag();
	if (mag < min) {DPZoneScoped;
		this->normalize();
		*this *= min;
	}
	else if (mag > max) {DPZoneScoped;
		this->normalize();
		*this *= max;
	}
}

inline vec2 vec2::
clampedMag(f32 min, f32 max) const{DPZoneScoped;
	f32 mag = this->mag();
	if (mag < min) {DPZoneScoped;
		return normalized() * min;
	}
	else if (mag > max) {DPZoneScoped;
		return normalized() * max;
	}
	else {
		return vec2(this->x, this->y);
	}
}

inline f32 vec2::
distanceTo(const vec2& rhs) const{DPZoneScoped;
	return (*this - rhs).mag();
}

inline vec2 vec2::
compOn(const vec2& rhs) const{DPZoneScoped;
	return rhs.normalized() * this->projectOn(rhs);
}

inline f32 vec2::
projectOn(const vec2& rhs) const{DPZoneScoped;
	if(this->mag() > M_EPSILON){DPZoneScoped;
		return this->dot(rhs) / this->mag();
	}else{
		return 0;
	}
}

inline vec2 vec2::
midpoint(const vec2& rhs) const{DPZoneScoped;
	return vec2((x+rhs.x)/2.f, (y+rhs.y)/2.f);
}

inline vec2 vec2::
xComp() const{DPZoneScoped;
	return vec2(x, 0);
}

inline vec2 vec2::
yComp() const{DPZoneScoped;
	return vec2(0, y);
}

inline vec2 vec2::
xInvert() const{DPZoneScoped;
	return vec2(-x, y);
}

inline vec2 vec2::
yInvert() const{DPZoneScoped;
	return vec2(x, -y);
}

inline vec2 vec2::
xSet(f32 set) const{DPZoneScoped;
	return vec2(set, y);
}

inline vec2 vec2::
ySet(f32 set) const{DPZoneScoped;
	return vec2(x, set);
}

inline vec2 vec2::
xAdd(f32 add) const{DPZoneScoped;
	return vec2(x + add, y);
}

inline vec2 vec2::
yAdd(f32 add) const{DPZoneScoped;
	return vec2(x, y + add);
}

inline vec2 vec2::
ceil() const{DPZoneScoped;
	return vec2(std::ceil(x), std::ceil(y));
};

inline vec2 vec2::
floor() const{DPZoneScoped;
	return vec2(std::floor(x), std::floor(y));
};

#endif //DESHI_VEC2_INL