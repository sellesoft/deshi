#include "vector.h"
#pragma once
#ifndef DESHI_VEC2_INL
#define DESHI_VEC2_INL

//////////////////////
//// constructors ////
//////////////////////

inline vec2::
vec2(float inX, float inY) { 
	x = inX; y = inY; 
}

inline vec2::
vec2(const vec2& v){ 
	x = v.x; y = v.y; 
}

inline vec2::
vec2(float* ptr){ 
	x = *ptr; y = *(ptr+1);
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
operator=(const vec2& rhs) {
	this->x = rhs.x; this->y = rhs.y;
}

inline vec2 vec2::
operator*(float rhs) const{
	return vec2(this->x * rhs, this->y * rhs);
}

inline void vec2::
operator*=(float rhs) {
	this->x *= rhs; this->y *= rhs; ;
}

inline vec2 vec2::
operator/(float rhs) const{
	return vec2(this->x / rhs, this->y / rhs);
}

inline void vec2::
operator/=(float rhs) {
	this->x /= rhs; this->y /= rhs;
}

inline vec2 vec2::
operator+(const vec2& rhs) const{
	return vec2(this->x + rhs.x, this->y + rhs.y);
}

inline void vec2::
operator+=(const vec2& rhs) {
	this->x += rhs.x; this->y += rhs.y;
}

inline vec2 vec2::
operator-(const vec2& rhs) const{
	return vec2(this->x - rhs.x, this->y - rhs.y) ;
}

inline void vec2::
operator-=(const vec2& rhs) {
	this->x -= rhs.x; this->y -= rhs.y; ;
}

inline vec2 vec2::
operator*(const vec2& rhs) const{
	return vec2(this->x * rhs.x, this->y * rhs.y) ;
}

inline void vec2::
operator*=(const vec2& rhs) {
	this->x *= rhs.x; this->y *= rhs.y; ;
}

inline vec2 vec2::
operator/(const vec2& rhs) const{
	return vec2(this->x / rhs.x, this->y / rhs.y) ;
}

inline void vec2::
operator/=(const vec2& rhs) {
	this->x /= rhs.x; this->y /= rhs.y; ;
}

inline vec2 vec2::
operator-() const{
	return vec2(-x, -y);
}

inline bool vec2::
operator==(const vec2& rhs) const{
	return abs(this->x - rhs.x) < M_EPSILON 
		&& abs(this->y - rhs.y) < M_EPSILON;
}

inline bool vec2::
operator!=(const vec2& rhs) const{
	return !(*this == rhs);
}

///////////////////
//// functions ////
///////////////////
inline void vec2::
set(f32 _x, f32 _y){
	x = _x; y = _y;
}

inline vec2 vec2::
absV() const{
	return vec2(abs(x), abs(y));
}

inline vec2 vec2::
copy() const{
	return vec2(x, y);
}

inline float vec2::
dot(const vec2& rhs) const{
	return (this->x * rhs.x) + (this->y * rhs.y);
}

inline vec2 vec2::
perp() const{
	return vec2(-y, x);
}

inline float vec2::
mag() const{
	return sqrt(x * x + y * y);
}

inline void vec2::
normalize() {
	if (*this != vec2::ZERO) {
		*this /= this->mag();
	}
}

inline vec2 vec2::
normalized() const{
	if(*this != vec2::ZERO){
		return *this / this->mag();
	}
	return *this;
}

inline void vec2::
clampMag(float min, float max) {
	float mag = this->mag();
	if (mag < min) {
		this->normalize();
		*this *= min;
	}
	else if (mag > max) {
		this->normalize();
		*this *= max;
	}
}

inline vec2 vec2::
clampedMag(float min, float max) const{
	float mag = this->mag();
	if (mag < min) {
		return normalized() * min;
	}
	else if (mag > max) {
		return normalized() * max;
	}
	else {
		return vec2(this->x, this->y);
	}
}

inline float vec2::
distanceTo(const vec2& rhs) const{
	return (*this - rhs).mag();
}

inline vec2 vec2::
compOn(const vec2& rhs) const{
	return rhs.normalized() * this->projectOn(rhs);
}

inline float vec2::
projectOn(const vec2& rhs) const{
	if(this->mag() > M_EPSILON){
		return this->dot(rhs) / this->mag();
	}else{
		return 0;
	}
}

inline vec2 vec2::
midpoint(const vec2& rhs) const{
	return vec2((x+rhs.x)/2.f, (y+rhs.y)/2.f);
}

inline vec2 vec2::
xComp() const{
	return vec2(x, 0);
}

inline vec2 vec2::
yComp() const{
	return vec2(0, y);
}

inline vec2 vec2::
xInvert() const{
	return vec2(-x, y);
}

inline vec2 vec2::
yInvert() const{
	return vec2(x, -y);
}

inline vec2 vec2::
xSet(float set) const{
	return vec2(set, y);
}

inline vec2 vec2::
ySet(float set) const{
	return vec2(x, set);
}

inline vec2 vec2::
xAdd(float add) const{
	return vec2(x + add, y);
}

inline vec2 vec2::
yAdd(float add) const{
	return vec2(x, y + add);
}


#endif //DESHI_VEC2_INL