#pragma once
#ifndef DESHI_VEC4_INL
#define DESHI_VEC4_INL

//////////////////////
//// constructors ////
//////////////////////

inline vec4::
vec4(float inX, float inY, float inZ, float inW) {
	this->x = inX; this->y = inY; this->z = inZ; this->w = inW;
}

inline vec4::
vec4(const vec4& v) {
	this->x = v.x; this->y = v.y; this->z = v.z; this->w = v.w;
}

inline vec4::
vec4(float* ptr){ 
	memcpy(&x, ptr, 4*sizeof(float));
}

///////////////////
//// constants ////
///////////////////

inline const vec4 vec4::ZERO = vec4(0,0,0,0);
inline const vec4 vec4::ONE  = vec4(1,1,1,1);

///////////////////
//// operators ////
///////////////////

inline void vec4::
operator =  (const vec4& rhs) {
	this->x = rhs.x; this->y = rhs.y; this->z = rhs.z; this->w = rhs.w;
}

inline vec4 vec4::
operator *  (const float& rhs) const {
	return vec4(this->x * rhs, this->y * rhs, this->z * rhs, this->w * rhs);
}

inline void vec4::
operator *= (const float& rhs) {
	this->x *= rhs; this->y *= rhs; this->z *= rhs; this->w *= rhs;
}

inline vec4 vec4::
operator /  (const float& rhs) const {
	return vec4(this->x / rhs, this->y / rhs, this->z / rhs, this->w / rhs);
}

inline void vec4::
operator /= (const float& rhs) {
	this->x /= rhs; this->y /= rhs; this->z /= rhs; this->w /= rhs;
}

inline vec4 vec4::
operator +  (const vec4& rhs) const {
	return vec4(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z, this->w + rhs.w);
}

inline void vec4::
operator += (const vec4& rhs) {
	this->x += rhs.x; this->y += rhs.y; this->z += rhs.z;  this->w += rhs.w;
}

inline vec4 vec4::
operator -  (const vec4& rhs) const {
	return vec4(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z, this->w - rhs.w);
}

inline void vec4::
operator -= (const vec4& rhs) {
	this->x -= rhs.x; this->y -= rhs.y; this->z -= rhs.z; this->w -= rhs.w;
}

inline vec4 vec4::
operator *  (const vec4& rhs) const {
	return vec4(this->x * rhs.x, this->y * rhs.y, this->z * rhs.z, this->w * rhs.w);
}

inline void vec4::
operator *= (const vec4& rhs) {
	this->x *= rhs.x; this->y *= rhs.y; this->z *= rhs.z; this->w *= rhs.w;
}

inline vec4 vec4::
operator /  (const vec4& rhs) const {
	return vec4(this->x / rhs.x, this->y / rhs.y, this->z / rhs.z,  this->w / rhs.w);
}

inline void vec4::
operator /= (const vec4& rhs) {
	this->x /= rhs.x; this->y /= rhs.y; this->z /= rhs.z; this->w /= rhs.w;
}

inline vec4 vec4::
operator -  () const {
	return vec4(-x, -y, -z, -w);
}

inline bool vec4::
operator == (const vec4& rhs) const {
	return abs(this->x - rhs.x) < VEC_EPSILON 
		&& abs(this->y - rhs.y) < VEC_EPSILON 
		&& abs(this->z - rhs.z) < VEC_EPSILON 
		&& abs(this->w - rhs.w) < VEC_EPSILON;
}

inline bool vec4::
operator != (const vec4& rhs) const {
	return !(*this == rhs);
}

///////////////////
//// functions ////
///////////////////

inline vec4 vec4::
absV() const{
	return vec4(abs(x), abs(y), abs(z), abs(w));
}


inline vec4 vec4::
copy() const {
	return vec4(x, y, z, w);
}

inline float vec4::
dot(const vec4& rhs) const {
	return this->x*rhs.x + this->y*rhs.y + this->z*rhs.z + this->w*rhs.w;
}

inline float vec4::
mag() const {
	return sqrtf(x*x + y*y + z*z + w*w);
}

inline vec4 vec4::
wnormalized() const {
	if (w != 0) {
		return *this / w;
	}
	return vec4(*this);
}

inline vec4 vec4::
xComp() const {
	return vec4(x, 0, 0, 0);
}

inline vec4 vec4::
yComp() const {
	return vec4(0, y, 0, 0);
}

inline vec4 vec4::
zComp() const {
	return vec4(0, 0, z, 0);
}

inline vec4 vec4::
wComp() const {
	return vec4(0, 0, 0, w);
}

inline vec4 vec4::
xInvert() const {
	return vec4(-x, y, z, w);
}

inline vec4 vec4::
yInvert() const {
	return vec4(x, -y, z, w);
}

inline vec4 vec4::
zInvert() const {
	return vec4(x, y, -z, w);
}

inline vec4 vec4::
wInvert() const {
	return vec4(x, y, z,-w);
}

#endif //DESHI_VEC4_INL