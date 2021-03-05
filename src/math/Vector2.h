#pragma once
#include "../utils/Debug.h"

struct MatrixN;
struct Matrix3;
struct Matrix4;
struct Vector4;
struct Vector3;
struct Quaternion;

//THIS PROBABLY HAS SOME ERRORS cause i just copied it from Vector3 and am not going to test it right now :D

struct Vector2 {
	float x{};
	float y{};
	
	Vector2(){};
	Vector2(float inX, float inY) {
		x = inX; y = inY;
	};
	Vector2(const Vector2& v){
		*this = v;
	};
	
	static const Vector2 ZERO;
	static const Vector2 ONE;
	static const Vector2 UP;
	static const Vector2 DOWN;
	static const Vector2 LEFT;
	static const Vector2 RIGHT;
	static const Vector2 FORWARD;
	static const Vector2 BACK;
	static const Vector2 UNITX;
	static const Vector2 UNITY;
	static const Vector2 UNITZ;
	
	void    operator =	(const Vector2& rhs);
	Vector2 operator *  (float rhs) const;
	void    operator *= (float rhs);
	Vector2 operator /  (float rhs) const;
	void    operator /= (float rhs);
	Vector2 operator +  (const Vector2& rhs) const;
	void    operator += (const Vector2& rhs);
	Vector2 operator -  (const Vector2& rhs) const;
	void    operator -= (const Vector2& rhs);
	Vector2 operator *  (const Vector2& rhs) const;
	void    operator *= (const Vector2& rhs);
	Vector2 operator /  (const Vector2& rhs) const;
	void    operator /= (const Vector2& rhs);
	Vector2 operator -  () const;	 
	bool    operator == (const Vector2& rhs) const;
	bool    operator != (const Vector2& rhs) const;
	friend Vector2 operator * (float lhs, const Vector2& rhs) { return   rhs * lhs; }
	
	const std::string str() const;
	const std::string str2f() const;
	Vector2 copy() const;
	float	dot(const Vector2& rhs) const;
	Vector2 cross(const Vector2& rhs) const;
	float	mag() const;
	void	normalize();
	Vector2	normalized() const;
	void    clampMag(float min, float max);
	Vector2 clampedMag(float min, float max) const;
	float	distanceTo(const Vector2& rhs) const;
	Vector2	compOn(Vector2 rhs);
	float projectOn(Vector2 rhs);
	Vector2	xComp() const;
	Vector2 yComp() const;
	Vector2 xInvert() const;
	Vector2 yInvert() const;
	
	//is this necessary?
	//Non-Vector vs Vector interactions defined in Math.h
	//Vector2(const Vector2& v);
	
	Vector3 ToVector3() const;
	Vector4 ToVector4() const;
	
	//in my haste i dont think these are necessary either
	//MatrixN ToM1x3() const;
	//MatrixN ToM1x4(float w) const;
	//Vector2 ProjectionMultiply(Matrix4 projection) const;
	
};


//// Constants ////

inline static const Vector2 VZERO  = Vector2(0,  0);
inline static const Vector2 VONE   = Vector2(1,  1);
inline static const Vector2 VRIGHT = Vector2(1,  0);
inline static const Vector2 VLEFT  = Vector2(-1, 0);
inline static const Vector2 VUP    = Vector2(0,  1);
inline static const Vector2 VDOWN  = Vector2(0, -1);
inline static const Vector2 UNITX  = Vector2(1,  0);
inline static const Vector2 UNITY  = Vector2(0,  1);

//ref: glm::hash
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

//// Operators ////

inline void    Vector2::operator =	(const Vector2& rhs) {
	this->x = rhs.x; this->y = rhs.y;
}

inline Vector2 Vector2::operator *  (float rhs) const {
	return Vector2(this->x * rhs, this->y * rhs);
}

inline void    Vector2::operator *= (float rhs) {
	this->x *= rhs; this->y *= rhs; ;
}

inline Vector2 Vector2::operator /  (float rhs) const {
	return Vector2(this->x / rhs, this->y / rhs);
}

inline void    Vector2::operator /= (float rhs) {
	this->x /= rhs; this->y /= rhs;
}

inline Vector2 Vector2::operator +  (const Vector2& rhs) const {
	return Vector2(this->x + rhs.x, this->y + rhs.y);
}

inline void    Vector2::operator += (const Vector2& rhs) {
	this->x += rhs.x; this->y += rhs.y;
}

inline Vector2 Vector2::operator -  (const Vector2& rhs) const {
	return Vector2(this->x - rhs.x, this->y - rhs.y) ;
}

inline void    Vector2::operator -= (const Vector2& rhs) {
	this->x -= rhs.x; this->y -= rhs.y; ;
}

inline Vector2 Vector2::operator *  (const Vector2& rhs) const {
	return Vector2(this->x * rhs.x, this->y * rhs.y) ;
}

inline void    Vector2::operator *= (const Vector2& rhs) {
	this->x *= rhs.x; this->y *= rhs.y; ;
}

inline Vector2 Vector2::operator /  (const Vector2& rhs) const {
	return Vector2(this->x / rhs.x, this->y / rhs.y) ;
}

inline void    Vector2::operator /= (const Vector2& rhs) {
	this->x /= rhs.x; this->y /= rhs.y; ;
}

inline Vector2 Vector2::operator -  () const {
	return Vector2(-x, -y);
}

//floating point accuracy in our vectors is .001 :)
inline bool    Vector2::operator == (const Vector2& rhs) const {
	return abs(this->x - rhs.x) < .001f && abs(this->y - rhs.y) < .001f;
}

//inline bool    Vector2::operator == (const Vector2& rhs) const {
//	return this->y == rhs.y  && this->y == rhs.y && ;
//}

inline bool    Vector2::operator != (const Vector2& rhs) const {
	return !(*this == rhs);
}

//// Functions ////

inline const std::string Vector2::str() const {
	return std::string("(") + std::to_string(this->x) + "," + std::to_string(this->y) + ")";
}

inline const std::string Vector2::str2f() const {
	char buffer[50];
	std::snprintf(buffer, 50, "(%+.2f, %+.2f)", this->x, this->y);
	return std::string(buffer);
}

inline Vector2 Vector2::copy() const {
	return Vector2(x, y);
}

inline float Vector2::dot(const Vector2& rhs) const {
	return this->x * rhs.x + this->y * rhs.y ;
}

//not necessary I dont think?
//inline Vector2 Vector2::cross(const Vector2& rhs) const {
//	return Vector2(this->y * rhs.z - rhs.y * this->z, this->x * rhs.z - rhs.x * this->z);
//}

inline float Vector2::mag() const {
	return std::sqrt(x * x + y * y);
}

////ref: https://betterexplained.com/articles/understanding-quakes-fast-inverse-square-root/
//inline float Vector2::mag() const {
//	ASSERT(CHAR_BIT*sizeof(float) == 32 && CHAR_BIT*sizeof(int32) == 32, "This mag method only works if float and int are 32bit");
//	float k = x * x + y * y + z * z;
//	float kHalf = .5f * k;
//	int32 i = *(int32*)&k;
//	i = 0x5f3759df - (i >> 1);
//	k = *(float*)&i;
//	k = k*(1.5f - kHalf*k*k);
//	return 1.f / k;
//}

inline void Vector2::normalize() {
	if (*this != Vector2(0, 0)) {
		*this /= this->mag();
	}
}

//inline void Vector2::normalize() {
//	if (*this != Vector2(0, 0)) {
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

inline Vector2 Vector2::normalized() const {
	if (*this != Vector2(0, 0)) {
		return *this / this->mag();
	}
	return Vector2(*this);
}

//inline Vector2 Vector2::normalized() const {
//	if (*this != Vector2(0, 0)) {
//		ASSERT(CHAR_BIT*sizeof(float) == 32 && CHAR_BIT*sizeof(int32) == 32, "This mag method only works if float and int are 32bit");
//		float k = x * x + y * y + z * z;
//		float kHalf = .5f * k;
//		int32 i = *(int32*)&k;
//		i = 0x5f3759df - (i >> 1);
//		k = *(float*)&i;
//		k = k*(1.5f - kHalf*k*k);
//		return *this * k;
//	}
//	return Vector2(*this);
//}

inline void Vector2::clampMag(float min, float max) {
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

inline Vector2 Vector2::clampedMag(float min, float max) const {
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

inline float Vector2::distanceTo(const Vector2& rhs) const {
	return (*this - rhs).mag();
}

inline Vector2 Vector2::compOn(Vector2 rhs) {
	return rhs.normalized() * this->projectOn(rhs);
}

inline float Vector2::projectOn(Vector2 rhs) {
	if (this->mag() != 0) return this->dot(rhs) / this->mag();
	else return 0;
}

inline Vector2 Vector2::xComp() const {
	return Vector2(x, 0);
}

inline Vector2 Vector2::yComp() const {
	return Vector2(0, y);
}

inline Vector2 Vector2::xInvert() const {
	return Vector2(-x, y);
}

inline Vector2 Vector2::yInvert() const {
	return Vector2(x, -y);
}
