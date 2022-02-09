#pragma once
#ifndef DESHI_QUATERNION_H
#define DESHI_QUATERNION_H

#include "kigu/common.h"

struct vec3;
struct mat3;
struct mat4;

//TODO(delle,Ma) implement quaternions
// https://github.com/erich666/GraphicsGems/blob/master/gemsiv/euler_angle/EulerAngles.c
struct quat {
	f32 x, y, z, w;
	
	quat() : x(0), y(0), z(0), w(1){}
	quat(f32 _x, f32 _y, f32 _z, f32 _w) : x(_x), y(_y), z(_z), w(_w){}
	
	void operator= (const quat& rhs);
	quat operator/ (f32 rhs) const;
	void operator/=(f32 rhs);
	quat operator* (f32 rhs) const;
	void operator*=(f32 rhs);
	quat operator+ (const quat& rhs) const;
	void operator+=(const quat& rhs);
	quat operator- (const quat& rhs) const;
	void operator-=(const quat& rhs);
	quat operator* (const quat& rhs) const;
	void operator*=(const quat& rhs);
	quat operator/ (const quat& rhs) const;
	void operator/=(const quat& rhs);
	quat operator- () const;
	
	void normalize();
	f32  mag() const;
	f32  dot(const quat& rhs) const;
	quat normalized() const;
	quat conjugate() const;
	quat inverse() const;
	
	static quat QuatSlerp(quat from, quat to, f32 t);
	
	//vector interactions
	quat(const vec3& rotation);
	quat(const vec3& axis, f32 theta);
	vec3 operator* (const vec3& rhs) const;
	vec3 toVec3() const;
	static quat AxisAngleToQuat(const vec3& axis, f32 angle);
	static quat RotVecToQuat(const vec3& rotation);
	static quat QuatSlerp(const vec3& from, const vec3& to, f32 t);
};


////////////////////
//// @operators ////
////////////////////
inline void quat::
operator= (const quat& rhs){
	x = rhs.x; y = rhs.y; z = rhs.z; w = rhs.w;
}

inline quat quat::
operator/ (f32 rhs) const{
	return quat(x / rhs, y / rhs, z / rhs, w / rhs);
}

inline void quat::
operator/=(f32 rhs){
	x /= rhs; y /= rhs; z /= rhs; w /= rhs;
}

inline quat quat::
operator* (f32 rhs) const{
	return quat(x * rhs, y * rhs, z * rhs, w * rhs);
}

inline void quat::
operator*=(f32 rhs){
	x *= rhs; y *= rhs; z *= rhs; w *= rhs;
}

inline quat quat::
operator+ (const quat& rhs) const{
	return quat(rhs.x + x, rhs.y + y, rhs.z + z, rhs.w + w);
}

inline void quat::
operator+=(const quat& rhs){
	x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w;
}

inline quat quat::
operator- (const quat& rhs) const{
	return quat(rhs.x - x, rhs.y - y, rhs.z - z, rhs.w - w);
}

inline void quat::
operator-=(const quat& rhs){
	x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w;
}

//!ref: https://www.gamasutra.com/view/feature/131686/rotating_objects_using_quaternions.php?page=2
inline quat quat::
operator* (const quat& rhs) const{
	f32 A, B, C, D, E, F, G, H;
	A = (w + x) * (rhs.w + rhs.x);
	B = (z - y) * (rhs.y - rhs.z);
	C = (w - x) * (rhs.y + rhs.z);
	D = (y + z) * (rhs.w - rhs.x);
	E = (x + z) * (rhs.x + rhs.y);
	F = (x - z) * (rhs.x - rhs.y);
	G = (w + y) * (rhs.w - rhs.z);
	H = (w - y) * (rhs.w + rhs.z);
	return quat(A - ( E + F + G + H) / 2.f,
				C + ( E - F + G - H) / 2.f,
				D + ( E - F - G + H) / 2.f,
				B + (-E - F + G + H) / 2.f);
}

inline void quat::
operator*=(const quat& rhs){
	x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w;
	
	f32 A, B, C, D, E, F, G, H;
	A = (w + x) * (rhs.w + rhs.x);
	B = (z - y) * (rhs.y - rhs.z);
	C = (w - x) * (rhs.y + rhs.z);
	D = (y + z) * (rhs.w - rhs.x);
	E = (x + z) * (rhs.x + rhs.y);
	F = (x - z) * (rhs.x - rhs.y);
	G = (w + y) * (rhs.w - rhs.z);
	H = (w - y) * (rhs.w + rhs.z);
	
	x = A - ( E + F + G + H) / 2.f;
	y = C + ( E - F + G - H) / 2.f;
	z = D + ( E - F - G + H) / 2.f;
	w = B + (-E - F + G + H) / 2.f;
}

//this probably isn't how this works considering how multiplication works
//idk if there is division with quaternions
inline quat quat::
operator/ (const quat& rhs) const{
	return quat(rhs.x * x, rhs.y * y, rhs.z * z, rhs.w * w);
}

inline void quat::
operator/=(const quat& rhs){
	x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w;
}

inline quat quat::
operator- () const{
	return quat(-x, -y, -z, -w);
}


////////////////////
//// @functions ////
////////////////////
inline f32 quat::
mag() const{
	return sqrtf(x*x + y*y + z*z + w*w);
}

inline void quat::
normalize(){
	*this / this->mag();
}

inline quat quat::
normalized() const{
	return *this / this->mag();
}

inline quat quat::
conjugate() const{
	return quat(-x, -y, -z, w);
}

inline quat quat::
inverse() const{
	return this->conjugate() / this->normalized();
}

inline f32 quat::
dot(const quat& rhs) const{
	return x*rhs.x + y*rhs.y + z*rhs.z + w*rhs.w;
}

//this implements Spherical Linear intERPoplation
//it interpolates between two quaternions along the shortest arc on a sphere formed by them
//taken from https://www.wikiwand.com/en/Slerp#/quat_Slerp
inline quat quat::
QuatSlerp(quat from, quat to, f32 t){
	from.normalize();
	to.normalize();
	
	f32 dot = to.dot(from);
	
	if(dot < 0){
		to = -to;
		dot = -dot;
	}
	
	const f32 dot_thresh = 0.9995f;
	
	// calculate coefficients
	if(dot > dot_thresh){
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
}

#endif //DESHI_QUATERNION_H