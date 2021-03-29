#pragma once
#include "../utils/Debug.h"

struct Vector3;
struct Matrix4;

struct Vector4 {
	float x{};
	float y{};
	float z{};
	float w{};
	
	Vector4() {}
	Vector4(float inX, float inY, float inZ, float inW);
	Vector4(const Vector4& v);
	
	void    operator =	(const Vector4& rhs);
	Vector4 operator *  (const float& rhs) const;
	void    operator *= (const float& rhs);
	Vector4 operator /  (const float& rhs) const;
	void    operator /= (const float& rhs);
	Vector4 operator +  (const Vector4& rhs) const;
	void    operator += (const Vector4& rhs);
	Vector4 operator -  (const Vector4& rhs) const;
	void    operator -= (const Vector4& rhs);
	Vector4 operator *  (const Vector4& rhs) const;
	void    operator *= (const Vector4& rhs);
	Vector4 operator /  (const Vector4& rhs) const;
	void    operator /= (const Vector4& rhs);
	Vector4 operator -  () const;
	bool    operator == (const Vector4& rhs) const;
	bool    operator != (const Vector4& rhs) const;
	friend Vector4 operator * (const float& lhs, const Vector4& rhs) { return   rhs * lhs; }
	
	const std::string str() const;
	const std::string str2f() const;
	Vector4 copy() const;
	float	dot(const Vector4& rhs) const;
	float	mag() const;
	Vector4	normalized() const;
	Vector4	xComp() const;
	Vector4 yComp() const;
	Vector4 zComp() const;
	Vector4 wComp() const;
	Vector4 xInvert() const;
	Vector4 yInvert() const;
	Vector4 zInvert() const;
	Vector4 wInvert() const;
	
	void takeVec3(Vector3 v); //dunno what to call this but it just takes data from a vector3 and leaves w alone
	
	//Non-Vector vs Vector interactions defined in Math.h
	Vector4(const Vector3& v, const float& w);
	
	Vector4 operator *  (const Matrix4& rhs) const;
	void    operator *= (const Matrix4& rhs);
	
	Vector3 ToVector3() const;
};

//ref: glm::hash
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

//// Constructors ////

inline Vector4::Vector4(float inX, float inY, float inZ, float inW) {
	this->x = inX; this->y = inY; this->z = inZ; this->w = inW;
}

inline Vector4::Vector4(const Vector4& v) {
	this->x = v.x; this->y = v.y; this->z = v.z; this->w = v.w;
}



//// Operators ////

inline void    Vector4::operator =	(const Vector4& rhs) {
	this->x = rhs.x; this->y = rhs.y; this->z = rhs.z; this->w = rhs.w;
}

inline Vector4 Vector4::operator *  (const float& rhs) const {
	return Vector4(this->x * rhs, this->y * rhs, this->z * rhs, this->w * rhs);
}

inline void    Vector4::operator *= (const float& rhs) {
	this->x *= rhs; this->y *= rhs; this->z *= rhs; this->w *= rhs;
}

inline Vector4 Vector4::operator /  (const float& rhs) const {
	return Vector4(this->x / rhs, this->y / rhs, this->z / rhs, this->w / rhs);
}

inline void    Vector4::operator /= (const float& rhs) {
	this->x /= rhs; this->y /= rhs; this->z /= rhs; this->w /= rhs;
}

inline Vector4 Vector4::operator +  (const Vector4& rhs) const {
	return Vector4(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z, this->w + rhs.w);
}

inline void    Vector4::operator += (const Vector4& rhs) {
	this->x += rhs.x; this->y += rhs.y; this->z += rhs.z;  this->w += rhs.w;
}

inline Vector4 Vector4::operator -  (const Vector4& rhs) const {
	return Vector4(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z, this->w - rhs.w);
}

inline void    Vector4::operator -= (const Vector4& rhs) {
	this->x -= rhs.x; this->y -= rhs.y; this->z -= rhs.z; this->w -= rhs.w;
}

inline Vector4 Vector4::operator *  (const Vector4& rhs) const {
	return Vector4(this->x * rhs.x, this->y * rhs.y, this->z * rhs.z, this->w * rhs.w);
}

inline void    Vector4::operator *= (const Vector4& rhs) {
	this->x *= rhs.x; this->y *= rhs.y; this->z *= rhs.z; this->w *= rhs.w;
}

inline Vector4 Vector4::operator /  (const Vector4& rhs) const {
	return Vector4(this->x / rhs.x, this->y / rhs.y, this->z / rhs.z,  this->w / rhs.w);
}

inline void    Vector4::operator /= (const Vector4& rhs) {
	this->x /= rhs.x; this->y /= rhs.y; this->z /= rhs.z; this->w /= rhs.w;
}

inline Vector4 Vector4::operator -  () const {
	return Vector4(-x, -y, -z, -w);
}

inline bool    Vector4::operator == (const Vector4& rhs) const {
	return this->x == rhs.x && this->y == rhs.y && this->z == rhs.z && this->w == rhs.w;
}

inline bool    Vector4::operator != (const Vector4& rhs) const {
	return this->x != rhs.x || this->y != rhs.y || this->z != rhs.z || this->w != rhs.w;
}



//// Functions ////

inline const std::string Vector4::str() const {
	return std::string("(") + std::to_string(this->x) + "," + std::to_string(this->y) + "," + std::to_string(this->z) + "," + std::to_string(this->w) + ")";
}

inline const std::string Vector4::str2f() const {
	char buffer[50];
	std::snprintf(buffer, 50, "(%+.2f, %+.2f, %+.2f, %+.2f)", this->x, this->y, this->z, this->w);
	return std::string(buffer);
}

inline Vector4 Vector4::copy() const {
	return Vector4(x, y, z, w);
}

inline float Vector4::dot(const Vector4& rhs) const {
	return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z + this->w * rhs.w;
}

inline float Vector4::mag() const {
	return std::sqrtf(x * x + y * y + z * z + w * w);
}

//NOTE: normalizing a Vector4 means dividing all parts by W
inline Vector4 Vector4::normalized() const {
	if (w != 0) {
		return *this / w;
	}
	return Vector4(*this);
}

inline Vector4 Vector4::xComp() const {
	return Vector4(x, 0, 0, 0);
}

inline Vector4 Vector4::yComp() const {
	return Vector4(0, y, 0, 0);
}

inline Vector4 Vector4::zComp() const {
	return Vector4(0, 0, z, 0);
}

inline Vector4 Vector4::wComp() const {
	return Vector4(0, 0, 0, w);
}

inline Vector4 Vector4::xInvert() const {
	return Vector4(-x, y, z, w);
}

inline Vector4 Vector4::yInvert() const {
	return Vector4(x, -y, z, w);
}

inline Vector4 Vector4::zInvert() const {
	return Vector4(x, y, -z, w);
}

inline Vector4 Vector4::wInvert() const {
	return Vector4(x, y, z,-w);
}

inline void Vector4::takeVec3(Vector3 v) {
	x = v.x; y = v.y; z = v.z;
}