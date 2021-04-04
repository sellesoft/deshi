#pragma once
#ifndef DESHI_VECTOR4_INL
#define DESHI_VECTOR4_INL

//////////////////////
//// constructors ////
//////////////////////

inline Vector4::
Vector4(float inX, float inY, float inZ, float inW) {
	this->x = inX; this->y = inY; this->z = inZ; this->w = inW;
}

inline Vector4::
Vector4(const Vector4& v) {
	this->x = v.x; this->y = v.y; this->z = v.z; this->w = v.w;
}

inline Vector4::
Vector4(float* ptr){ 
	memcpy(&x, ptr, 4*sizeof(float));
}

///////////////////
//// constants ////
///////////////////

inline const Vector4 Vector4::ZERO = Vector4(0,0,0,0);
inline const Vector4 Vector4::ONE  = Vector4(1,1,1,1);

///////////////////
//// operators ////
///////////////////

inline void Vector4::
operator =  (const Vector4& rhs) {
	this->x = rhs.x; this->y = rhs.y; this->z = rhs.z; this->w = rhs.w;
}

inline Vector4 Vector4::
operator *  (const float& rhs) const {
	return Vector4(this->x * rhs, this->y * rhs, this->z * rhs, this->w * rhs);
}

inline void Vector4::
operator *= (const float& rhs) {
	this->x *= rhs; this->y *= rhs; this->z *= rhs; this->w *= rhs;
}

inline Vector4 Vector4::
operator /  (const float& rhs) const {
	return Vector4(this->x / rhs, this->y / rhs, this->z / rhs, this->w / rhs);
}

inline void Vector4::
operator /= (const float& rhs) {
	this->x /= rhs; this->y /= rhs; this->z /= rhs; this->w /= rhs;
}

inline Vector4 Vector4::
operator +  (const Vector4& rhs) const {
	return Vector4(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z, this->w + rhs.w);
}

inline void Vector4::
operator += (const Vector4& rhs) {
	this->x += rhs.x; this->y += rhs.y; this->z += rhs.z;  this->w += rhs.w;
}

inline Vector4 Vector4::
operator -  (const Vector4& rhs) const {
	return Vector4(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z, this->w - rhs.w);
}

inline void Vector4::
operator -= (const Vector4& rhs) {
	this->x -= rhs.x; this->y -= rhs.y; this->z -= rhs.z; this->w -= rhs.w;
}

inline Vector4 Vector4::
operator *  (const Vector4& rhs) const {
	return Vector4(this->x * rhs.x, this->y * rhs.y, this->z * rhs.z, this->w * rhs.w);
}

inline void Vector4::
operator *= (const Vector4& rhs) {
	this->x *= rhs.x; this->y *= rhs.y; this->z *= rhs.z; this->w *= rhs.w;
}

inline Vector4 Vector4::
operator /  (const Vector4& rhs) const {
	return Vector4(this->x / rhs.x, this->y / rhs.y, this->z / rhs.z,  this->w / rhs.w);
}

inline void Vector4::
operator /= (const Vector4& rhs) {
	this->x /= rhs.x; this->y /= rhs.y; this->z /= rhs.z; this->w /= rhs.w;
}

inline Vector4 Vector4::
operator -  () const {
	return Vector4(-x, -y, -z, -w);
}

inline bool Vector4::
operator == (const Vector4& rhs) const {
	return abs(this->x - rhs.x) < .001f && abs(this->y - rhs.y) < .001f && abs(this->z - rhs.z) < .001f && abs(this->w - rhs.w) < .001f;
	//return this->x == rhs.x && this->y == rhs.y && this->z == rhs.z && this->w == rhs.w;
}

inline bool Vector4::
operator != (const Vector4& rhs) const {
	return !(*this == rhs);
}

///////////////////
//// functions ////
///////////////////

inline const std::string Vector4::
str() const {
	return std::string("(") + std::to_string(this->x) + "," + std::to_string(this->y) + "," + std::to_string(this->z) + "," + std::to_string(this->w) + ")";
}

inline const std::string Vector4::
str2f() const {
	char buffer[50];
	std::snprintf(buffer, 50, "(%+.2f, %+.2f, %+.2f, %+.2f)", this->x, this->y, this->z, this->w);
	return std::string(buffer);
}

inline Vector4 Vector4::
copy() const {
	return Vector4(x, y, z, w);
}

inline float Vector4::
dot(const Vector4& rhs) const {
	return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z + this->w * rhs.w;
}

inline float Vector4::
mag() const {
	return std::sqrtf(x * x + y * y + z * z + w * w);
}

//NOTE: normalizing a Vector4 means dividing all parts by W
inline Vector4 Vector4::
normalized() const {
	if (w != 0) {
		return *this / w;
	}
	return Vector4(*this);
}

inline Vector4 Vector4::
xComp() const {
	return Vector4(x, 0, 0, 0);
}

inline Vector4 Vector4::
yComp() const {
	return Vector4(0, y, 0, 0);
}

inline Vector4 Vector4::
zComp() const {
	return Vector4(0, 0, z, 0);
}

inline Vector4 Vector4::
wComp() const {
	return Vector4(0, 0, 0, w);
}

inline Vector4 Vector4::
xInvert() const {
	return Vector4(-x, y, z, w);
}

inline Vector4 Vector4::
yInvert() const {
	return Vector4(x, -y, z, w);
}

inline Vector4 Vector4::
zInvert() const {
	return Vector4(x, y, -z, w);
}

inline Vector4 Vector4::
wInvert() const {
	return Vector4(x, y, z,-w);
}

//////////////
//// hash ////
//////////////

namespace std{
	template<> struct hash<Vector4>{
		inline size_t operator()(Vector4 const& v) const{
			size_t seed = 0;
			hash<float> hasher; size_t hash;
			hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
			hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
			hash = hasher(v.z); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
			hash = hasher(v.w); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
			return seed;
		}
	};
};

#endif //DESHI_VECTOR4_INL