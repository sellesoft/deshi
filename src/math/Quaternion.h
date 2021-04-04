#pragma once
#include "../utils/debug.h"

struct Vector3;
struct Matrix4;
struct Matrix3;

//TODO(delle,Ma) implement quaternions
struct Quaternion {
	float x{}, y{}, z{}, w{};
	
	Quaternion() : x(0), y(0), z(0), w(1) {}
	Quaternion(float inX, float inY, float inZ, float inW) : x(inX), y(inY), z(inZ), w(inW) {}
	
	//Non-Quat vs Quat interactions defined in Math.h
	Quaternion(const Vector3& rotation);
	Quaternion(const Vector3& axis, float theta);
	
	void operator =			(const Quaternion& rhs);
	Quaternion operator /	(const float& rhs);
	void operator /=		(const float& rhs);
	Quaternion operator *	(const float& rhs);
	void operator *=		(const float& rhs);
	Quaternion operator +	(const Quaternion& rhs);
	void operator +=		(const Quaternion& rhs);
	Quaternion operator -	(const Quaternion& rhs);
	void operator -=		(const Quaternion& rhs);
	Quaternion operator *	(const Quaternion& rhs);
	void operator *=		(const Quaternion& rhs);
	Quaternion operator /	(const Quaternion& rhs);
	void operator /=		(const Quaternion& rhs);
	Quaternion operator - ();
	
	float mag();
	void normalize();
	Quaternion normalized();
	Quaternion conjugate();
	Quaternion inverse();
	float dot(Quaternion q);
	Vector3 ToVector3();
	
	Quaternion AxisAngleToQuat(float angle, Vector3 axis);
	static Quaternion RotVecToQuat(Vector3 rotation);
	
	static Quaternion QuatSlerp(Quaternion from, Quaternion to, float t);
	static Quaternion QuatSlerp(Vector3 from, Vector3 to, float t);
	
};



inline void Quaternion::operator = (const Quaternion& rhs) {
	x = rhs.x; y = rhs.y; z = rhs.z; w = rhs.w;
}



//Quaternion vs float
inline Quaternion Quaternion::operator / (const float& rhs) {
	return Quaternion(x / rhs, y / rhs, z / rhs, w / rhs);
}

inline void Quaternion::operator /= (const float& rhs) {
	x /= rhs; y /= rhs; z /= rhs; w /= rhs;
}

inline Quaternion Quaternion::operator * (const float& rhs) {
	
	return Quaternion(x * rhs, y * rhs, z * rhs, w * rhs);
}

inline void Quaternion::operator *= (const float& rhs) {
	x *= rhs; y *= rhs; z *= rhs; w *= rhs;
}



//Quaternion vs Quaternion
inline Quaternion Quaternion::operator + (const Quaternion& rhs) {
	return Quaternion(rhs.x + x, rhs.y + y, rhs.z + z, rhs.w + w);
}

inline void Quaternion::operator += (const Quaternion& rhs) {
	x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w;
}

inline Quaternion Quaternion::operator - (const Quaternion& rhs) {
	return Quaternion(rhs.x - x, rhs.y - y, rhs.z - z, rhs.w - w);
}

inline void Quaternion::operator -= (const Quaternion& rhs) {
	x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w;
}

inline Quaternion Quaternion::operator * (const Quaternion& rhs) {
	//efficient quaternion multiplication from https://www.gamasutra.com/view/feature/131686/rotating_objects_using_quaternions.php?page=2
	float A, B, C, D, E, F, G, H;
	A = (w + x) * (rhs.w + rhs.x);
	B = (z - y) * (rhs.y - rhs.z);
	C = (w - x) * (rhs.y + rhs.z);
	D = (y + z) * (rhs.w - rhs.x);
	E = (x + z) * (rhs.x + rhs.y);
	F = (x - z) * (rhs.x - rhs.y);
	G = (w + y) * (rhs.w - rhs.z);
	H = (w - y) * (rhs.w + rhs.z);
	return Quaternion(
					  A - (E + F + G + H) / 2,
					  C + (E - F + G - H) / 2,
					  D + (E - F - G + H) / 2,
					  B + (-E - F + G + H) / 2);
}

inline void Quaternion::operator *= (const Quaternion& rhs) {
	x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w;
	
	float A, B, C, D, E, F, G, H;
	A = (w + x) * (rhs.w + rhs.x);
	B = (z - y) * (rhs.y - rhs.z);
	C = (w - x) * (rhs.y + rhs.z);
	D = (y + z) * (rhs.w - rhs.x);
	E = (x + z) * (rhs.x + rhs.y);
	F = (x - z) * (rhs.x - rhs.y);
	G = (w + y) * (rhs.w - rhs.z);
	H = (w - y) * (rhs.w + rhs.z);
	
	x = A - (E + F + G + H) / 2;
	y = C + (E - F + G - H) / 2;
	z = D + (E - F - G + H) / 2;
	w = B + (-E - F + G + H) / 2;
}

//this probably isn't how this works considering how multiplication works
//idk if there is division with quaternions
inline Quaternion Quaternion::operator / (const Quaternion& rhs) {
	return Quaternion(rhs.x * x, rhs.y * y, rhs.z * z, rhs.w * w);
}

inline void Quaternion::operator /= (const Quaternion& rhs) {
	x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w;
}

inline Quaternion Quaternion::operator - () {
	return Quaternion(-x, -y, -z, -w);
}



//Quaternion functions
inline float Quaternion::mag() {
	return std::sqrtf(x * x + y * y + z * z + w * w);
}

inline void Quaternion::normalize() {
	*this / this->mag();
}

inline Quaternion Quaternion::normalized() {
	return *this / this->mag();
}

inline Quaternion Quaternion::conjugate() {
	return Quaternion(-x, -y, -z, w);
}

inline Quaternion Quaternion::inverse() {
	return this->conjugate() / this->normalized();
}

inline float Quaternion::dot(Quaternion q) {
	return x * q.x + y * q.y + z * q.z + w * q.w;
}

inline Quaternion Quaternion::QuatSlerp(Quaternion from, Quaternion to, float t) {
	//this implements Spherical Linear intERPoplation
	//it interpolates between two quaternions along the shortest arc on a sphere formed by them
	//taken from https://www.wikiwand.com/en/Slerp#/Quaternion_Slerp
	
	from.normalize();
	to.normalize();
	
	float dot = to.dot(from);
	
	if (dot < 0) {
		to = -to;
		dot = -dot;
	}
	
	const float dot_thresh = 0.9995;
	
	// calculate coefficients
	if (dot > dot_thresh) {
		// standard case (slerp)
		Quaternion result = from + ((to - from) * t);
		result.normalize();
		return result;
	}
	
	//since dot is in range [0, DOT_THRESHOLD], acos is safe
	double theta_0 = acos(dot);			//theta_0 = angle between input vectors
	double theta = theta_0 * t;			//theta = angle between v0 and result
	double sin_theta = sin(theta);		//compute this value only once
	double sin_theta_0 = sin(theta_0);	//compute this value only once
	
	double s0 = cos(theta) - dot * sin_theta / sin_theta_0;  // == sin(theta_0 - theta) / sin(theta_0)
	double s1 = sin_theta / sin_theta_0;
	
	return (from * s0) + (to * s1);
	
	
	// calculate final values
}
