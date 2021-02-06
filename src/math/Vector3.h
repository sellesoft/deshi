#pragma once
#include "../utils/dsh_Debug.h"

struct MatrixN;
struct Matrix3;
struct Matrix4;
struct Vector2;
struct Vector4;
struct Quaternion;

struct Vector3 {
	float x{};
	float y{};
	float z{};

	Vector3();
	Vector3(float inX, float inY, float inZ);
	Vector3(float inX, float inY);
	Vector3(const Vector3& v);

	static const Vector3 ZERO;
	static const Vector3 ONE;
	static const Vector3 RIGHT;
	static const Vector3 LEFT;
	static const Vector3 UP;
	static const Vector3 DOWN;
	static const Vector3 FORWARD;
	static const Vector3 BACK;
	static const Vector3 UNITX;
	static const Vector3 UNITY;
	static const Vector3 UNITZ;

	void    operator =	(const Vector3& rhs);
	Vector3 operator *  (float rhs) const;
	void    operator *= (float rhs);
	Vector3 operator /  (float rhs) const;
	void    operator /= (float rhs);
	Vector3 operator +  (const Vector3& rhs) const;
	void    operator += (const Vector3& rhs);
	Vector3 operator -  (const Vector3& rhs) const;
	void    operator -= (const Vector3& rhs);
	Vector3 operator *  (const Vector3& rhs) const;
	void    operator *= (const Vector3& rhs);
	Vector3 operator /  (const Vector3& rhs) const;
	void    operator /= (const Vector3& rhs);
	Vector3 operator -  () const;
	bool    operator == (const Vector3& rhs) const;
	bool    operator != (const Vector3& rhs) const;
	friend Vector3 operator * (float lhs, const Vector3& rhs) { return   rhs * lhs; }

	const std::string str() const;
	const std::string str2f() const;
	Vector3 copy() const;
	float	dot(const Vector3& rhs) const;
	Vector3 cross(const Vector3& rhs) const;
	float	mag() const;
	void	normalize();
	Vector3	normalized() const;
	void    clampMag(float min, float max);
	Vector3 clampedMag(float min, float max) const;
	float	distanceTo(const Vector3& rhs) const;
	Vector3	compOn(Vector3 rhs);
	float projectOn(Vector3 rhs);
	Vector3	xComp() const;
	Vector3 yComp() const;
	Vector3 zComp() const;
	Vector3 xInvert() const;
	Vector3 yInvert() const;
	Vector3 zInvert() const;

	//Non-Vector vs Vector interactions defined in Math.h
	Vector3(const Vector2& v);

	Vector3 operator *  (const Matrix3& rhs) const;
	void    operator *= (const Matrix3& rhs);
	Vector3 operator *  (const Matrix4& rhs) const;
	void    operator *= (const Matrix4& rhs);

	Vector4 ToVector4() const;
	Vector2 ToVector2() const; 
	MatrixN ToM1x3() const;
	MatrixN ToM1x4(float w) const;
	Vector3 ProjectionMultiply(Matrix4 projection) const;
	
};


//// Operators ////

inline void    Vector3::operator =	(const Vector3& rhs) {
	this->x = rhs.x; this->y = rhs.y; this->z = rhs.z;
}

inline Vector3 Vector3::operator *  (float rhs) const {
	return Vector3(this->x * rhs, this->y * rhs, this->z * rhs);
}

inline void    Vector3::operator *= (float rhs) {
	this->x *= rhs; this->y *= rhs; this->z *= rhs;
}

inline Vector3 Vector3::operator /  (float rhs) const {
	return Vector3(this->x / rhs, this->y / rhs, this->z / rhs);
}

inline void    Vector3::operator /= (float rhs) {
	this->x /= rhs; this->y /= rhs; this->z /= rhs;
}

inline Vector3 Vector3::operator +  (const Vector3& rhs) const {
	return Vector3(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z);
}

inline void    Vector3::operator += (const Vector3& rhs) {
	this->x += rhs.x; this->y += rhs.y; this->z += rhs.z;
}

inline Vector3 Vector3::operator -  (const Vector3& rhs) const {
	return Vector3(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z);
}

inline void    Vector3::operator -= (const Vector3& rhs) {
	this->x -= rhs.x; this->y -= rhs.y; this->z -= rhs.z;
}

inline Vector3 Vector3::operator *  (const Vector3& rhs) const {
	return Vector3(this->x * rhs.x, this->y * rhs.y, this->z * rhs.z);
}

inline void    Vector3::operator *= (const Vector3& rhs) {
	this->x *= rhs.x; this->y *= rhs.y; this->z *= rhs.z;
}

inline Vector3 Vector3::operator /  (const Vector3& rhs) const {
	return Vector3(this->x / rhs.x, this->y / rhs.y, this->z / rhs.z);
}

inline void    Vector3::operator /= (const Vector3& rhs) {
	this->x /= rhs.x; this->y /= rhs.y; this->z /= rhs.z;
}

inline Vector3 Vector3::operator -  () const {
	return Vector3( -x, -y, -z );
}

//floating point accuracy in our vectors is .001 :)
inline bool    Vector3::operator == (const Vector3& rhs) const {
	return abs(this->x - rhs.x) < .001f && abs(this->y - rhs.y) < .001f && abs(this->z - rhs.z) < .001f;
}

//inline bool    Vector3::operator == (const Vector3& rhs) const {
//	return this->y == rhs.y  && this->y == rhs.y && this->z == rhs.z;
//}

inline bool    Vector3::operator != (const Vector3& rhs) const {
	return !(*this == rhs);
}

//// Functions ////

inline const std::string Vector3::str() const {
	return std::string("(") + std::to_string(this->x) + "," + std::to_string(this->y) + "," + std::to_string(this->z) + ")";
}

inline const std::string Vector3::str2f() const {
	char buffer[50];
	std::snprintf(buffer, 50, "(%+.2f, %+.2f, %+.2f)", this->x, this->y, this->z);
	return std::string(buffer);
}

inline Vector3 Vector3::copy() const {
	return Vector3(x, y, z);
}

inline float Vector3::dot(const Vector3& rhs) const {
	return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z;
}

inline Vector3 Vector3::cross(const Vector3& rhs) const {
	return Vector3(this->y * rhs.z - rhs.y * this->z, this->x * rhs.z - rhs.x * this->z, this->x * rhs.y - rhs.x * this->y);
}

inline float Vector3::mag() const {
	return std::sqrt(x * x + y * y + z * z);
}

////ref: https://betterexplained.com/articles/understanding-quakes-fast-inverse-square-root/
//inline float Vector3::mag() const {
//	ASSERT(CHAR_BIT*sizeof(float) == 32 && CHAR_BIT*sizeof(int32) == 32, "This mag method only works if float and int are 32bit");
//	float k = x * x + y * y + z * z;
//	float kHalf = .5f * k;
//	int32 i = *(int32*)&k;
//	i = 0x5f3759df - (i >> 1);
//	k = *(float*)&i;
//	k = k*(1.5f - kHalf*k*k);
//	return 1.f / k;
//}

inline void Vector3::normalize() {
	if (*this != Vector3(0, 0, 0)) {
		*this /= this->mag();
	}
}

//inline void Vector3::normalize() {
//	if (*this != Vector3(0, 0, 0)) {
//		ASSERT(CHAR_BIT*sizeof(float) == 32 && CHAR_BIT*sizeof(int32) == 32, "This mag method only works if float and int are 32bit");
//		float k = x * x + y * y + z * z;
//		float kHalf = .5f * k;
//		int32 i = *(int32*)&k;
//		i = 0x5f3759df - (i >> 1);
//		k = *(float*)&i;
//		k = k*(1.5f - kHalf*k*k);
//		*this *= k;
//	}
//}

inline Vector3 Vector3::normalized() const {
	if (*this != Vector3(0, 0, 0)) {
		return *this / this->mag();
	}
	return Vector3(*this);
}

//inline Vector3 Vector3::normalized() const {
//	if (*this != Vector3(0, 0, 0)) {
//		ASSERT(CHAR_BIT*sizeof(float) == 32 && CHAR_BIT*sizeof(int32) == 32, "This mag method only works if float and int are 32bit");
//		float k = x * x + y * y + z * z;
//		float kHalf = .5f * k;
//		int32 i = *(int32*)&k;
//		i = 0x5f3759df - (i >> 1);
//		k = *(float*)&i;
//		k = k*(1.5f - kHalf*k*k);
//		return *this * k;
//	}
//	return Vector3(*this);
//}

inline void Vector3::clampMag(float min, float max) {
	float mag = this->mag();
	if (mag < min) {
		this->normalize();
		*this *= min; 
	} else if(mag > max){ 
		this->normalize();
		*this *= max; 
	}
}

inline Vector3 Vector3::clampedMag(float min, float max) const {
	float mag = this->mag();
	if (mag < min) {
		return normalized() * min; 
	} else if(mag > max){ 
		return normalized() * max; 
	} else {
		return Vector3(this->x, this->y, this->z);
	}
}

inline float Vector3::distanceTo(const Vector3& rhs) const {
	return (*this - rhs).mag();
}

inline Vector3 Vector3::compOn(Vector3 rhs) {
	return rhs.normalized() * this->projectOn(rhs);
}

inline float Vector3::projectOn(Vector3 rhs) {
	if (this->mag() != 0) return this->dot(rhs) / this->mag();
	else return 0;
}

inline Vector3 Vector3::xComp() const {
	return Vector3(x, 0, 0);
}

inline Vector3 Vector3::yComp() const {
	return Vector3(0, y, 0);
}

inline Vector3 Vector3::zComp() const {
	return Vector3(0, 0, z);
}

inline Vector3 Vector3::xInvert() const {
	return Vector3(-x, y, z);
}

inline Vector3 Vector3::yInvert() const {
	return Vector3(x, -y, z);
}

inline Vector3 Vector3::zInvert() const {
	return Vector3(x, y, -z);
}