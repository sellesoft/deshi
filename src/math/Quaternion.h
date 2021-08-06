#pragma once
#include "../utils/debug.h"

struct vec3;
struct mat4;
struct mat3;

//TODO(delle,Ma) implement quaternions
// https://github.com/erich666/GraphicsGems/blob/master/gemsiv/euler_angle/EulerAngles.c
struct quat {
	float x{}, y{}, z{}, w{};
	
	quat() : x(0), y(0), z(0), w(1) {}
	quat(float inX, float inY, float inZ, float inW) : x(inX), y(inY), z(inZ), w(inW) {}
	
	//Non-Quat vs Quat interactions defined in Math.h
	quat(const vec3& rotation);
	quat(const vec3& axis, float theta);
	
	void operator =			(const quat& rhs);
	quat operator /	(const float& rhs);
	void operator /=		(const float& rhs);
	quat operator *	(const float& rhs);
	void operator *=		(const float& rhs);
	vec3 operator *      (const vec3& rhs);
	quat operator +	(const quat& rhs);
	void operator +=		(const quat& rhs);
	quat operator -	(const quat& rhs);
	void operator -=		(const quat& rhs);
	quat operator *	(const quat& rhs);
	void operator *=		(const quat& rhs);
	quat operator /	(const quat& rhs);
	void operator /=		(const quat& rhs);
	quat operator - ();
	
	float mag();
	void normalize();
	quat normalized();
	quat conjugate();
	quat inverse();
	float dot(quat q);
	vec3 toVec3();
	
	static quat AxisAngleToQuat(float angle, vec3 axis);
	static quat RotVecToQuat(vec3 rotation);
	
	static quat QuatSlerp(quat from, quat to, float t);
	static quat QuatSlerp(vec3 from, vec3 to, float t);
	
};



inline void quat::operator = (const quat& rhs) {
	x = rhs.x; y = rhs.y; z = rhs.z; w = rhs.w;
}



//quat vs float
inline quat quat::operator / (const float& rhs) {
	return quat(x / rhs, y / rhs, z / rhs, w / rhs);
}

inline void quat::operator /= (const float& rhs) {
	x /= rhs; y /= rhs; z /= rhs; w /= rhs;
}

inline quat quat::operator * (const float& rhs) {
	
	return quat(x * rhs, y * rhs, z * rhs, w * rhs);
}

inline void quat::operator *= (const float& rhs) {
	x *= rhs; y *= rhs; z *= rhs; w *= rhs;
}



//quat vs quat
inline quat quat::operator + (const quat& rhs) {
	return quat(rhs.x + x, rhs.y + y, rhs.z + z, rhs.w + w);
}

inline void quat::operator += (const quat& rhs) {
	x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w;
}

inline quat quat::operator - (const quat& rhs) {
	return quat(rhs.x - x, rhs.y - y, rhs.z - z, rhs.w - w);
}

inline void quat::operator -= (const quat& rhs) {
	x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w;
}

inline quat quat::operator * (const quat& rhs) {
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
	return quat(
					  A - (E + F + G + H) / 2,
					  C + (E - F + G - H) / 2,
					  D + (E - F - G + H) / 2,
					  B + (-E - F + G + H) / 2);
}

inline void quat::operator *= (const quat& rhs) {
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
inline quat quat::operator / (const quat& rhs) {
	return quat(rhs.x * x, rhs.y * y, rhs.z * z, rhs.w * w);
}

inline void quat::operator /= (const quat& rhs) {
	x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w;
}

inline quat quat::operator - () {
	return quat(-x, -y, -z, -w);
}



//quat functions
inline float quat::mag() {
	return sqrtf(x * x + y * y + z * z + w * w);
}

inline void quat::normalize() {
	*this / this->mag();
}

inline quat quat::normalized() {
	return *this / this->mag();
}

inline quat quat::conjugate() {
	return quat(-x, -y, -z, w);
}

inline quat quat::inverse() {
	return this->conjugate() / this->normalized();
}

inline float quat::dot(quat q) {
	return x * q.x + y * q.y + z * q.z + w * q.w;
}

inline quat quat::QuatSlerp(quat from, quat to, float t) {
	//this implements Spherical Linear intERPoplation
	//it interpolates between two quaternions along the shortest arc on a sphere formed by them
	//taken from https://www.wikiwand.com/en/Slerp#/quat_Slerp
	
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
		quat result = from + ((to - from) * t);
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
