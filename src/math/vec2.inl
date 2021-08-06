#pragma once
#ifndef DESHI_vec2_INL
#define DESHI_vec2_INL

//////////////////////
//// constructors ////
//////////////////////

inline vec2::
vec2(float inX, float inY) { 
	x = inX; y = inY; 
}

inline vec2::
vec2(const vec2& v){ 
	*this = v; 
}

inline vec2::
vec2(float* ptr){ 
	memcpy(&x, ptr, 2*sizeof(float));
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
operator*(float rhs) const {
	return vec2(this->x * rhs, this->y * rhs);
}

inline void vec2::
operator*=(float rhs) {
	this->x *= rhs; this->y *= rhs; ;
}

inline vec2 vec2::
operator/(float rhs) const {
	return vec2(this->x / rhs, this->y / rhs);
}

inline void vec2::
operator/=(float rhs) {
	this->x /= rhs; this->y /= rhs;
}

inline vec2 vec2::
operator+(const vec2& rhs) const {
	return vec2(this->x + rhs.x, this->y + rhs.y);
}

inline void vec2::
operator+=(const vec2& rhs) {
	this->x += rhs.x; this->y += rhs.y;
}

inline vec2 vec2::
operator-(const vec2& rhs) const {
	return vec2(this->x - rhs.x, this->y - rhs.y) ;
}

inline void vec2::
operator-=(const vec2& rhs) {
	this->x -= rhs.x; this->y -= rhs.y; ;
}

inline vec2 vec2::
operator*(const vec2& rhs) const {
	return vec2(this->x * rhs.x, this->y * rhs.y) ;
}

inline void vec2::
operator*=(const vec2& rhs) {
	this->x *= rhs.x; this->y *= rhs.y; ;
}

inline vec2 vec2::
operator/(const vec2& rhs) const {
	return vec2(this->x / rhs.x, this->y / rhs.y) ;
}

inline void vec2::
operator/=(const vec2& rhs) {
	this->x /= rhs.x; this->y /= rhs.y; ;
}

inline vec2 vec2::
operator-() const {
	return vec2(-x, -y);
}

//compares if the difference is greater than .001
inline bool vec2::
operator==(const vec2& rhs) const {
	return abs(this->x - rhs.x) < .001f && abs(this->y - rhs.y) < .001f;
	//return this->y == rhs.y  && this->y == rhs.y;
}

inline bool vec2::
operator!=(const vec2& rhs) const {
	return !(*this == rhs);
}

///////////////////
//// functions ////
///////////////////

inline vec2 vec2::
absV() const{
	return vec2(abs(x), abs(y));
}

inline vec2 vec2::
copy() const {
	return vec2(x, y);
}

inline float vec2::
dot(const vec2& rhs) const {
	return this->x * rhs.x + this->y * rhs.y ;
}

inline vec2 vec2::
perp() const {
	return vec2(-y, x);
}

inline float vec2::
mag() const {
	return sqrt(x * x + y * y);
}

inline vec2 vec2::
normalize() {
	if (*this != vec2(0, 0)) {
		*this /= this->mag();
	}
	return *this;
}

inline vec2 vec2::
normalized() const {
	if (*this != vec2(0, 0)) {
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
clampedMag(float min, float max) const {
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
distanceTo(const vec2& rhs) const {
	return (*this - rhs).mag();
}

inline vec2 vec2::
compOn(vec2 rhs) {
	return rhs.normalized() * this->projectOn(rhs);
}

inline float vec2::
projectOn(vec2 rhs) {
	if (this->mag() != 0) return this->dot(rhs) / this->mag();
	else return 0;
}

inline vec2 vec2::
midpoint(vec2 rhs){
	return vec2((x+rhs.x)/2.f, (y+rhs.y)/2.f);
}

inline vec2 vec2::
xComp() const {
	return vec2(x, 0);
}

inline vec2 vec2::
yComp() const {
	return vec2(0, y);
}

inline vec2 vec2::
xInvert() const {
	return vec2(-x, y);
}

inline vec2 vec2::
yInvert() const {
	return vec2(x, -y);
}

inline vec2 vec2::
xSet(float set) const {
	return vec2(set, y);
}

inline vec2 vec2::
ySet(float set) const {
	return vec2(x, set);
}

inline vec2 vec2::
xAdd(float add) const {
	return vec2(x + add, y);
}

inline vec2 vec2::
yAdd(float add) const {
	return vec2(x, y + add);
}

inline const std::string vec2::
str() const {
	return std::string("(") + std::to_string(this->x) + "," + std::to_string(this->y) + ")";
}

inline const std::string vec2::
str2f() const {
	char buffer[50];
	std::snprintf(buffer, 50, "(%+.2f, %+.2f)", this->x, this->y);
	return std::string(buffer);
}

//////////////
//// hash ////
//////////////

namespace std{
	template<> struct hash<vec2>{
		inline size_t operator()(vec2 const& v) const{
			size_t seed = 0;
			hash<float> hasher; size_t hash;
			hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
			hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
			return seed;
		}
	};
};

#endif //DESHI_vec2_INL