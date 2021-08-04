#pragma once
#ifndef DESHI_VECTOR2_INL
#define DESHI_VECTOR2_INL

//////////////////////
//// constructors ////
//////////////////////

inline Vector2::
Vector2(float inX, float inY) { 
	x = inX; y = inY; 
}

inline Vector2::
Vector2(const Vector2& v){ 
	*this = v; 
}

inline Vector2::
Vector2(float* ptr){ 
	memcpy(&x, ptr, 2*sizeof(float));
}

///////////////////
//// constants ////
///////////////////

inline const Vector2 Vector2::ZERO =  Vector2( 0, 0);
inline const Vector2 Vector2::ONE =   Vector2( 1, 1);
inline const Vector2 Vector2::RIGHT = Vector2( 1, 0);
inline const Vector2 Vector2::LEFT =  Vector2(-1, 0);
inline const Vector2 Vector2::UP =    Vector2( 0, 1);
inline const Vector2 Vector2::DOWN =  Vector2( 0,-1);
inline const Vector2 Vector2::UNITX = Vector2( 1, 0);
inline const Vector2 Vector2::UNITY = Vector2( 0, 1);

///////////////////
//// operators ////
///////////////////

inline void Vector2::
operator=(const Vector2& rhs) {
	this->x = rhs.x; this->y = rhs.y;
}

inline Vector2 Vector2::
operator*(float rhs) const {
	return Vector2(this->x * rhs, this->y * rhs);
}

inline void Vector2::
operator*=(float rhs) {
	this->x *= rhs; this->y *= rhs; ;
}

inline Vector2 Vector2::
operator/(float rhs) const {
	return Vector2(this->x / rhs, this->y / rhs);
}

inline void Vector2::
operator/=(float rhs) {
	this->x /= rhs; this->y /= rhs;
}

inline Vector2 Vector2::
operator+(const Vector2& rhs) const {
	return Vector2(this->x + rhs.x, this->y + rhs.y);
}

inline void Vector2::
operator+=(const Vector2& rhs) {
	this->x += rhs.x; this->y += rhs.y;
}

inline Vector2 Vector2::
operator-(const Vector2& rhs) const {
	return Vector2(this->x - rhs.x, this->y - rhs.y) ;
}

inline void Vector2::
operator-=(const Vector2& rhs) {
	this->x -= rhs.x; this->y -= rhs.y; ;
}

inline Vector2 Vector2::
operator*(const Vector2& rhs) const {
	return Vector2(this->x * rhs.x, this->y * rhs.y) ;
}

inline void Vector2::
operator*=(const Vector2& rhs) {
	this->x *= rhs.x; this->y *= rhs.y; ;
}

inline Vector2 Vector2::
operator/(const Vector2& rhs) const {
	return Vector2(this->x / rhs.x, this->y / rhs.y) ;
}

inline void Vector2::
operator/=(const Vector2& rhs) {
	this->x /= rhs.x; this->y /= rhs.y; ;
}

inline Vector2 Vector2::
operator-() const {
	return Vector2(-x, -y);
}

//compares if the difference is greater than .001
inline bool Vector2::
operator==(const Vector2& rhs) const {
	return abs(this->x - rhs.x) < .001f && abs(this->y - rhs.y) < .001f;
	//return this->y == rhs.y  && this->y == rhs.y;
}

inline bool Vector2::
operator!=(const Vector2& rhs) const {
	return !(*this == rhs);
}

///////////////////
//// functions ////
///////////////////

inline Vector2 Vector2::
absV() const{
	return Vector2(abs(x), abs(y));
}

inline Vector2 Vector2::
copy() const {
	return Vector2(x, y);
}

inline float Vector2::
dot(const Vector2& rhs) const {
	return this->x * rhs.x + this->y * rhs.y ;
}

inline Vector2 Vector2::
perp() const {
	return Vector2(-y, x);
}

inline float Vector2::
mag() const {
	return sqrt(x * x + y * y);
}

inline Vector2 Vector2::
normalize() {
	if (*this != Vector2(0, 0)) {
		*this /= this->mag();
	}
	return *this;
}

inline Vector2 Vector2::
normalized() const {
	if (*this != Vector2(0, 0)) {
		return *this / this->mag();
	}
	return *this;
}

inline void Vector2::
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

inline Vector2 Vector2::
clampedMag(float min, float max) const {
	float mag = this->mag();
	if (mag < min) {
		return normalized() * min;
	}
	else if (mag > max) {
		return normalized() * max;
	}
	else {
		return Vector2(this->x, this->y);
	}
}

inline float Vector2::
distanceTo(const Vector2& rhs) const {
	return (*this - rhs).mag();
}

inline Vector2 Vector2::
compOn(Vector2 rhs) {
	return rhs.normalized() * this->projectOn(rhs);
}

inline float Vector2::
projectOn(Vector2 rhs) {
	if (this->mag() != 0) return this->dot(rhs) / this->mag();
	else return 0;
}

inline Vector2 Vector2::
midpoint(Vector2 rhs){
	return Vector2((x+rhs.x)/2.f, (y+rhs.y)/2.f);
}

inline Vector2 Vector2::
xComp() const {
	return Vector2(x, 0);
}

inline Vector2 Vector2::
yComp() const {
	return Vector2(0, y);
}

inline Vector2 Vector2::
xInvert() const {
	return Vector2(-x, y);
}

inline Vector2 Vector2::
yInvert() const {
	return Vector2(x, -y);
}

inline Vector2 Vector2::
xSet(float set) const {
	return Vector2(set, y);
}

inline Vector2 Vector2::
ySet(float set) const {
	return Vector2(x, set);
}

inline Vector2 Vector2::
xAdd(float add) const {
	return Vector2(x + add, y);
}

inline Vector2 Vector2::
yAdd(float add) const {
	return Vector2(x, y + add);
}

inline const std::string Vector2::
str() const {
	return std::string("(") + std::to_string(this->x) + "," + std::to_string(this->y) + ")";
}

inline const std::string Vector2::
str2f() const {
	char buffer[50];
	std::snprintf(buffer, 50, "(%+.2f, %+.2f)", this->x, this->y);
	return std::string(buffer);
}

//////////////
//// hash ////
//////////////

namespace std{
	template<> struct hash<Vector2>{
		inline size_t operator()(Vector2 const& v) const{
			size_t seed = 0;
			hash<float> hasher; size_t hash;
			hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
			hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
			return seed;
		}
	};
};

#endif //DESHI_VECTOR2_INL