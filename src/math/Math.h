#pragma once
#ifndef DESHI_MATH_H
#define DESHI_MATH_H

#include "vector.h"
#include "matrix.h"
#include "quaternion.h"
#include "matN.h"
#include "../defines.h"
#include "../utils/tuple.h"

#include <math.h>
#include <algorithm>
#include <numeric>
#include <vector>

#define F_AVG(i, f) ([&] { \
persist std::vector<float> floats; \
persist float nf; \
persist int iter = 0; \
if(i == floats.size()){ \
floats.erase(floats.begin()); \
floats.push_back(f); \
iter++; \
} \
else{ \
floats.push_back(f); \
iter++; \
}\
if(iter == i){ \
nf = Math::average(floats, floats.size()); \
iter = 0; \
} \
return nf; \
}())

//////////////
//// vec3 ////
//////////////
inline vec3 vec3::
operator* (const mat3& rhs) const{
	return vec3(x*rhs.arr[0] + y*rhs.arr[3] + z*rhs.arr[6], 
				x*rhs.arr[1] + y*rhs.arr[4] + z*rhs.arr[7], 
				x*rhs.arr[2] + y*rhs.arr[5] + z*rhs.arr[8]);
}

inline void vec3::
operator*=(const mat3& rhs){
	*this = vec3(x*rhs.arr[0] + y*rhs.arr[3] + z*rhs.arr[6],
				 x*rhs.arr[1] + y*rhs.arr[4] + z*rhs.arr[7],
				 x*rhs.arr[2] + y*rhs.arr[5] + z*rhs.arr[8]);
}

inline vec3 vec3::
operator* (const mat4& rhs) const{
	vec3 result;
#if DESHI_USE_SSE
	vec4 temp(x, y, z, 0);
	temp.sse = LinearCombineSSE(temp.sse, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result = temp.xyz;
#else
	result.x = x*rhs.arr[0] + y*rhs.arr[4] + z*rhs.arr[8]  + rhs.arr[12];
	result.y = x*rhs.arr[1] + y*rhs.arr[5] + z*rhs.arr[9]  + rhs.arr[13];
	result.z = x*rhs.arr[2] + y*rhs.arr[6] + z*rhs.arr[10] + rhs.arr[14];
#endif
	return result;
}

inline void vec3::
operator*=(const mat4& rhs){
	vec3 result;
#if DESHI_USE_SSE
	vec4 temp(x, y, z, 0);
	temp.sse = LinearCombineSSE(temp.sse, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result = temp.xyz;
#else
	result.x = x*rhs.arr[0] + y*rhs.arr[4] + z*rhs.arr[8]  + rhs.arr[12];
	result.y = x*rhs.arr[1] + y*rhs.arr[5] + z*rhs.arr[9]  + rhs.arr[13];
	result.z = x*rhs.arr[2] + y*rhs.arr[6] + z*rhs.arr[10] + rhs.arr[14];
#endif
	*this = result;
}

inline vec3 vec3::
operator* (const quat& rhs) const{
	return (quat(x, y, z, 0) * rhs).toVec3();
}


//////////////
//// vec4 ////
//////////////
inline vec4 vec4::
operator* (const mat4& rhs) const{
	vec4 result;
#if DESHI_USE_SSE
	result.sse = LinearCombineSSE(sse, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else
	result.x = x*rhs.arr[0] + y*rhs.arr[4] + z*rhs.arr[8]  + w*rhs.arr[12];
	result.y = x*rhs.arr[1] + y*rhs.arr[5] + z*rhs.arr[9]  + w*rhs.arr[13];
	result.z = x*rhs.arr[2] + y*rhs.arr[6] + z*rhs.arr[10] + w*rhs.arr[14];
	result.w = x*rhs.arr[3] + y*rhs.arr[7] + z*rhs.arr[11] + w*rhs.arr[15];
#endif
	return result;
}

inline void vec4::
operator*=(const mat4& rhs){
#if DESHI_USE_SSE
	sse = LinearCombineSSE(sse, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else
	vec4 result;
	result.x = x*rhs.arr[0] + y*rhs.arr[4] + z*rhs.arr[8]  + w*rhs.arr[12];
	result.y = x*rhs.arr[1] + y*rhs.arr[5] + z*rhs.arr[9]  + w*rhs.arr[13];
	result.z = x*rhs.arr[2] + y*rhs.arr[6] + z*rhs.arr[10] + w*rhs.arr[14];
	result.w = x*rhs.arr[3] + y*rhs.arr[7] + z*rhs.arr[11] + w*rhs.arr[15];
	*this = result;
#endif
}


//////////////
//// quat ////
//////////////
inline quat::
quat(const vec3& rotation){
	this->RotVecToQuat(rotation);
}

inline quat::
quat(const vec3& axis, float theta){
	*this = AxisAngleToQuat(axis, theta);
}

inline vec3 quat::
operator* (const vec3& rhs) const{
	return (quat(rhs.x, rhs.y, rhs.z, 0) * *this).toVec3();
}

//https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles?oldformat=true
inline vec3 quat::
toVec3() const{
	vec3 angles;
	
	// roll (x-axis rotation)
	f64 sinr_cosp = 2.0 * ((f64)w * x + (f64)y * z);
	f64 cosr_cosp = 1.0 - 2.0 * ((f64)x * x + (f64)y * y);
	angles.x = atan2(sinr_cosp, cosr_cosp);
	
	// pitch (y-axis rotation)
	f64 sinp = 2.0 * ((f64)w * y - (f64)z * x);
	if(std::abs(sinp) >= 1)
		angles.y = copysign(M_PI / 2, sinp); // use 90 degrees if out of range
	else
		angles.y = asin(sinp);
	
	// yaw (z-axis rotation)
	f64 siny_cosp = 2.0 * ((f64)w * z +(f64)x * y);
	f64 cosy_cosp = 1.0 - 2.0 * ((f64)y * y + (f64)z * z);
	angles.z = atan2(siny_cosp, cosy_cosp);
	
	return angles;
}

//converts an angle and an axis to a quaternion
//im not sure how this works or where it will be used and im not even sure if its
//set up properly (sorry)
inline quat quat::
AxisAngleToQuat(const vec3& axis, float angle){
	float angler = Radians(angle);
	return quat(sinf(angler / 2) * axis.x, sinf(angler / 2) * axis.y, sinf(angler / 2) * axis.z, cosf(angler / 2));
}

//this may be wrong but I think a rotation vector would be 
//vec3(roll, pitch, yaw)
//https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles?oldformat=true
inline quat quat::
RotVecToQuat(const vec3& rotation){
	//this is probably necessary although he didn't do this in the Gamasutra article
	vec3 rotationrad = Radians(rotation);
	float cy = cos(rotationrad.z * 0.5);
	float sy = sin(rotationrad.z * 0.5);
	float cp = cos(rotationrad.y * 0.5);
	float sp = sin(rotationrad.y * 0.5);
	float cr = cos(rotationrad.x * 0.5);
	float sr = sin(rotationrad.x * 0.5);
	
	quat q;
	q.w = cr * cp * cy + sr * sp * sy;
	q.x = sr * cp * cy - cr * sp * sy;
	q.y = cr * sp * cy + sr * cp * sy;
	q.z = cr * cp * sy - sr * sp * cy;
	
	return q;
}

inline quat quat::
QuatSlerp(const vec3& fromv, const vec3& tov, float t){
	//this implements Spherical Linear intERPoplation
	//it interpolates between two quaternions along the shortest arc on a sphere formed by them
	//taken from https://www.wikiwand.com/en/Slerp#/Quaternion_Slerp
	
	quat from = RotVecToQuat(fromv);
	quat to = RotVecToQuat(tov);
	
	from.normalize();
	to.normalize();
	
	float dot = to.dot(from);
	
	if(dot < 0){
		to = -to;
		dot = -dot;
	}
	
	const float dot_thresh = 0.9995f;
	
	// calculate coefficients
	if(dot > dot_thresh){
		// standard case (slerp)
		quat result = from + ((to - from) * t);
		result.normalize();
		return result;
	}
	
	//since dot is in range [0, DOT_THRESHOLD], acos is safe
	f64 theta_0 = acos(dot);			//theta_0 = angle between input vectors
	f64 theta = theta_0 * t;			//theta = angle between v0 and result
	f64 sin_theta = sin(theta);		//compute this value only once
	f64 sin_theta_0 = sin(theta_0);	//compute this value only once
	
	f64 s0 = cos(theta) - dot * sin_theta / sin_theta_0;  // == sin(theta_0 - theta) / sin(theta_0)
	f64 s1 = sin_theta / sin_theta_0;
	
	return (from * s0) + (to * s1);
}


//////////////
//// mat3 ////
//////////////
inline vec3 mat3::
row(u32 row){
	Assert(row < 3, "mat3 subscript out of bounds");
	return vec3(&arr[4*row]);
}

inline vec3 mat3::
col(u32 col){
	Assert(col < 3, "mat3 subscript out of bounds");
	return vec3(arr[col], arr[4+col], arr[8+col]);
}

//returns a pre-multiplied X->Y->Z LH rotation transformation matrix based on input in degrees
inline mat3 mat3::
RotationMatrix(vec3 rotation){
	rotation = Radians(rotation);
	float cX = cosf(rotation.x); float sX = sinf(rotation.x);
	float cY = cosf(rotation.y); float sY = sinf(rotation.y);
	float cZ = cosf(rotation.z); float sZ = sinf(rotation.z);
	float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
	float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
	float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
	return mat3(r00, r01, r02,
				r10, r11, r12,
				r20, r21, r22);
}

//returns a scale matrix where (0,0) = scale.x, (1,1) = scale.y, (2,2) = scale.z
inline mat3 mat3::
ScaleMatrix(vec3 scale){
	return mat3(scale.x, 0, 0,
				0, scale.y, 0,
				0, 0, scale.z);
}


//////////////
//// mat4 ////
//////////////
inline vec4 mat4::
row(u32 row){
	Assert(row < 4, "mat4 subscript out of bounds");
	return vec4(&arr[4*row]);
}

inline vec4 mat4::
col(u32 col){
	Assert(col < 4, "mat4 subscript out of bounds");
	return vec4(arr[col], arr[4+col], arr[8+col], arr[12+col]);
}

inline vec3 mat4::
Translation(){
	return vec3(arr[12], arr[13], arr[14]);
}

//returns a pre-multiplied X->Y->Z LH rotation transformation matrix based on input in degrees
inline mat4 mat4::
RotationMatrix(vec3 rotation){
	rotation = Radians(rotation);
	float cX = cosf(rotation.x); float sX = sinf(rotation.x);
	float cY = cosf(rotation.y); float sY = sinf(rotation.y);
	float cZ = cosf(rotation.z); float sZ = sinf(rotation.z);
	float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
	float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
	float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
	return mat4(r00, r01, r02, 0,
				r10, r11, r12, 0,
				r20, r21, r22, 0,
				0,   0,   0,   1);
}

//https://github.com/microsoft/DirectXMath/blob/7c30ba5932e081ca4d64ba4abb8a8986a7444ec9/Inc/DirectXMathMatrix.inl
//line 1675 i do not know how this works but it does so :D
//return a matrix to rotate a vector around an arbitrary axis by some angle
//TODO(sushi, Ma) redo this function, I think its completely wrong around any axis thats not a world axis
inline mat4 mat4::
AxisAngleRotationMatrix(float angle, vec4 axis){
	angle = Radians(angle); 
	float mag = axis.mag();
	axis = vec4(axis.x / mag, axis.y / mag, axis.z / mag, axis.w / mag);
	//axis.normalize();
	float c = cosf(angle); float s = sinf(angle); 
	
	vec4 A = vec4(s, c, 1 - c, 0);
	
	vec4 C2 = vec4(A.z, A.z, A.z, A.z);
	vec4 C1 = vec4(A.y, A.y, A.y, A.y);
	vec4 C0 = vec4(A.x, A.x, A.x, A.x);
	
	vec4 N0 = vec4(axis.y, axis.z, axis.x, axis.w);
	vec4 N1 = vec4(axis.z, axis.x, axis.y, axis.w);
	
	vec4 V0 = C2 * N0;
	V0 *= N1;
	
	vec4 R0 = C2 * axis;
	R0 = R0 * axis + C1;
	
	vec4 R1 = C0 * axis + V0;
	vec4 R2 = (V0 - C0) * axis;
	
	V0 = vec4(R0.x, R0.y, R0.z, A.w);
	vec4 V1 = vec4(R1.z, R2.y, R2.z, R1.x);
	vec4 V2 = vec4(R1.y, R2.x, R1.y, R2.x);
	
	return mat4(V0.x, V1.x, V1.y, V0.w,
				V1.z, V0.y, V1.w, V0.w,
				V2.x, V2.y, V0.z, V0.w,
				0,    0,    0,    1);
}

//returns a transformation matrix of the combined translation, rotation, and scale matrices from input vectors
//rotates over the Y, then Z then X, ref: https://www.euclideanspace.com/maths/geometry/affine/aroundPoint/index.htm
inline mat4 mat4::RotationMatrixAroundPoint(vec3 pivot, vec3 rotation){
	//pivot = -pivot; //gotta negate this for some reason :)
	rotation = Radians(rotation);
	float cX = cosf(rotation.x); float sX = sinf(rotation.x);
	float cY = cosf(rotation.y); float sY = sinf(rotation.y);
	float cZ = cosf(rotation.z); float sZ = sinf(rotation.z);
	float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
	float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
	float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
	
	return mat4(r00, r01, r02, 0,
				r10, r11, r12, 0,
				r20, r21, r22, 0,
				pivot.x - r00*pivot.x - r01*pivot.y - r02*pivot.z, pivot.y - r10*pivot.x - r11*pivot.y - r12*pivot.z, pivot.z - r20*pivot.x - r21*pivot.y - r22*pivot.z, 1);
}

//returns a translation matrix where (3,0) = translation.x, (3,1) = translation.y, (3,2) = translation.z
inline mat4 mat4::
TranslationMatrix(vec3 translation){
	return mat4::TranslationMatrix(translation.x, translation.y, translation.z);
}

//returns a scale matrix where (0,0) = scale.x, (1,1) = scale.y, (2,2) = scale.z
inline mat4 mat4::
ScaleMatrix(vec3 scale){
	return mat4::ScaleMatrix(scale.x, scale.y, scale.z);
}

//returns a transformation matrix of the combined translation, rotation, and scale matrices from input vectors
inline mat4 mat4::
TransformationMatrix(vec3 tr, vec3 rot, vec3 scale){
	rot = Radians(rot);
	float cX = cosf(rot.x); float sX = sinf(rot.x);
	float cY = cosf(rot.y); float sY = sinf(rot.y);
	float cZ = cosf(rot.z); float sZ = sinf(rot.z);
	float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
	float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
	float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
	return mat4(scale.x*r00, scale.x*r01, scale.x*r02, 0,
				scale.y*r10, scale.y*r11, scale.y*r12, 0,
				scale.z*r20, scale.z*r21, scale.z*r22, 0,
				tr.x,        tr.y,        tr.z, 1);
}

//returns euler angles from a rotation matrix
//TODO(sushi, Ma) confirm that this works at some point
inline vec3 mat4::
Rotation(){
	if((*this)(0,2) < 1){
		if((*this)(0,2) > -1){
			return -vec3(Degrees(atan2(-(*this)(1,2), (*this)(2,2))), Degrees(asin((*this)(0,2))), Degrees(atan2(-(*this)(0,1), (*this)(0,0))));
		}else{
			return -vec3(Degrees(-atan2((*this)(1,0), (*this)(1,1))), Degrees(-M_HALFPI), 0);
		}
	}else{
		return -vec3(Degrees(atan2((*this)(1,0), (*this)(1,1))), Degrees(M_HALFPI), 0);
	}
	
	
}

//////////////
//// math ////
//////////////
namespace Math {
	
	constexpr global_ s32 pow(s32 base, u32 exp){
		int result = 1;
		for(;;){
			if(exp & 1) result *= base;
			exp >>= 1;
			if(exp == 0) break;
			base *= base;
		}
		return result;
	}
	
	//rounding
	template<int decimals = 2> inline global_ f32 round(f32 a){
		constexpr f32 multiple = f32(pow(10,decimals));
		return f32(s32(a * multiple + .5f)) / multiple;
	}
	template<int decimals = 2> inline global_ vec3 round(vec3 a){
		constexpr f32 multiple = f32(pow(10,decimals));
		return vec3(f32(s32(a.x * multiple + .5f)) / multiple,
					f32(s32(a.y * multiple + .5f)) / multiple,
					f32(s32(a.z * multiple + .5f)) / multiple);
	}
	
	//average any std container probably
	template<class FWIt> static float average(FWIt a, const FWIt b, int size){ return std::accumulate(a, b, 0.0) / size; }
	template<class T> static f64 average(const T& container, int size){ return average(std::begin(container), std::end(container), size); }
	
	//interpolating
	template<typename T> global_ T lerp(T a, T b, f32 t){ return a*(1.f-t) + b*t; }
	
	//clamping
	template<typename T> inline global_ T clamp(T value, T lo, T hi){ 
		return (value < lo) ? lo : ((value > hi) ? hi : value); 
	}
	template<> inline global_ vec3 clamp<vec3>(vec3 value, vec3 lo, vec3 hi){ 
		return vec3(clamp(value.x,lo.x,hi.x), clamp(value.y,lo.y,hi.y), clamp(value.z,lo.z,hi.z));
	}
	
	//returns how far along a polynomial fit for a set of data you are
	//you are not allowed to have 2 points with the same x value here
	//using Lagrange Polynomials
	//TODO(sushi, Ma) look into maybe implementing Newton's Polynomials at some point idk if they're better but they look more simple
	static float PolynomialCurveInterpolation(std::vector<vec2> vs, float t){
		float sum = 0;
		for (int j = 0; j < vs.size(); j++){
			float vy = vs[j].y; 
			float jx = vs[j].x;
			float lbp = 1;
			for (int m = 0; m < vs.size(); m++){
				if(lbp != 0){
					if(m != j){
						float mx = vs[m].x;
						lbp *= (t - mx) / (jx - mx);
					}
				}else break;
			}
			sum += vy * lbp;
		}
		return sum;
	}
	
	static vec2 vec2RotateByAngle(float angle, vec2 v){
		if (!angle) return v;
		angle = Radians(angle);
		return vec2(v.x * cosf(angle) - v.y * sinf(angle), v.x * sin(angle) + v.y * cos(angle));
	}
	
	inline global_ bool PointInRectangle(vec2 point, vec2 rectPos, vec2 rectDims){
		return
			point.x >= rectPos.x &&
			point.y >= rectPos.y &&
			point.x <= rectPos.x + rectDims.x &&
			point.y <= rectPos.y + rectDims.y;
	}
	
#define BoundTimeOsc(x, y) Math::BoundedOscillation(x, y, DeshTotalTime)
	
	//oscillates between a given upper and lower value based on a given x value
	inline global_ float BoundedOscillation(float lower, float upper, float x){
		Assert(upper > lower);
		return ((upper - lower) * cosf(x) + (upper + lower)) / 2;
	}
	
	//returns in degrees
	//this doesn't really work in 3D but this function is here anyways
	static float AngBetweenVectors(vec3 v1, vec3 v2){
		return Degrees(acosf(v1.dot(v2) / (v1.mag() * v2.mag())));
	}
	
	//returns in degrees
	static float AngBetweenVectors(vec2 v1, vec2 v2){
		return Degrees(atan2(v1.x * v2.y - v1.y * v2.x, v1.dot(v2)));
	}
	
	//returns in degrees between 0 and 360
	static float AngBetweenVectors360(vec2 v1, vec2 v2){
		float ang = Degrees(atan2(v1.x * v2.y - v1.y * v2.x, v1.dot(v2)));
		return (ang < 0) ? 360 + ang : ang;
	}
	
	//NOTE 0-1 depth range
	static mat4 PerspectiveProjectionMatrix(f32 width, f32 height, f32 hFOV, f32 nearZ, f32 farZ){
		float renderDistance = farZ - nearZ;
		float aspectRatio = (f32)height / (f32)width;
		float fovRad = 1.0f / tanf(Radians(hFOV / 2.0f));
		return mat4( //NOTE setting (1,1) to negative flips the y-axis so y is up when x is right and z is forward
					aspectRatio * fovRad, 0,	   0,							  0,
					0,					-fovRad, 0,							  0,
					0,					0,	   farZ / renderDistance,		  1,
					0,					0,	   -(farZ*nearZ) / renderDistance, 0);
	}
	
	//this function returns a matrix that tells a vector how to look at a specific point in space.
	static mat4 LookAtMatrix(const vec3& pos, const vec3& target, vec3* out_up = 0){
		if(pos == target){ return LookAtMatrix(pos, target + vec3(.01f, 0, 0)); }
		
		//get new forward direction
		vec3 newFor = (target - pos).normalized();
		
		//get right direction
		vec3 newRight; 
		if(newFor == vec3::UP || newFor == vec3::DOWN){ 
			newRight = vec3::RIGHT; 
		}else{
			newRight = (vec3::UP.cross(newFor)).normalized(); 
		}
		
		//get up direction
		vec3 newUp = newFor.cross(newRight); 
		if(out_up) *out_up = newUp;
		
		//make look-at matrix
		return mat4(newRight.x, newRight.y, newRight.z, 0,
					newUp.x,    newUp.y,    newUp.z,    0,
					newFor.x,   newFor.y,   newFor.z,   0,
					pos.x,      pos.y,      pos.z,      1);
	}
	
	//this assumes its in degrees
	static vec3 SphericalToRectangularCoords(vec3 v){
		float y = Radians(v.y);
		float z = Radians(v.z);
		return vec3(v.x * sinf(z) * cosf(y), v.x * cosf(z), v.x * sinf(z) * sinf(y));
	}
	
	static vec3 RectangularToSphericalCoords(vec3 v){
		float rho = Radians(sqrt(v.mag()));
		float theta = Radians(atan(v.y / v.z));
		float phi = acos(v.z / v.mag()); //maybe use v.y instead of v.z because y is our vertical axis
		return vec3(rho, theta, phi);
		
	}
	
	static float DistTwoPoints(vec3 a, vec3 b){
		return sqrtf((a.x - b.x) * (a.x - b.x) +
					 (a.y - b.y) * (a.y - b.y) +
					 (a.z - b.z) * (a.z - b.z));
	}
	
	static inline float DistPointToPlane(vec3 point, vec3 plane_n, vec3 plane_p){
		return (point - plane_p).dot(plane_n);
	}
	
	//where a line intersects with a plane, 'returns' how far along line you were as t value
	static vec3 VectorPlaneIntersect(vec3 plane_p, vec3 plane_n, vec3 line_start, vec3 line_end, float* out_t = 0){
		vec3 lstole = (line_end - line_start).normalized();
		vec3 lptopp = plane_p - line_start;
		f32 t = lptopp.dot(plane_n) / lstole.dot(plane_n);
		if(out_t) *out_t = t;
		return line_start + t * lstole;
	}
	
	//TODO(sushi) figure out how this worked
	//returns where two lines intersect on the x axis with slope and the y-intercept
	//static vec2 LineIntersect2(float slope1, float ycross1, float slope2, float ycross2){
	//	matN lhs(2,2,{ slope1, ycross1, slope2, ycross2 });
	//	matN rhs(2,1,{ 1, 1 });
	//	matN det = lhs.Inverse() * rhs;
	//	float x = 1 / det(1,0) * det(0,0);
	//	float y = slope1 * x + ycross1;
	//
	//	f32 x = (ycross1 - ycross2 + slope)
	//	return vec2(x, y);
	//}

	static vec2 LineIntersect2(vec2 p1, vec2 p2, vec2 p3, vec2 p4) {
		f32 m1 = (p2.y - p1.y) / (p2.x - p1.x);
		f32 m2 = (p4.y - p3.y) / (p4.x - p3.x);
		f32 b1 = p2.y - m1 * p2.x;
		f32 b2 = p4.y - m2 * p4.x;
		f32 x = (b2 - b1) / (m1 - m2);
		f32 y = m1 * x + b1;
		return{ x,y };


	}
	
	static vec3 LineMidpoint(vec3 start, vec3 end){
		return (start+end)/2.f;
	}
	
	//the input vectors should be in viewMat/camera space
	//returns true if the line can be rendered after clipping, false otherwise
	static bool ClipLineToZPlanes(vec3& start, vec3& end, f32 nearZ, f32 farZ){
		//clip to the near plane
		vec3 planePoint = vec3(0, 0, nearZ);
		vec3 planeNormal = vec3::FORWARD;
		float d = planeNormal.dot(planePoint);
		bool startBeyondPlane = planeNormal.dot(start) - d < 0;
		bool endBeyondPlane = planeNormal.dot(end) - d < 0;
		float t;
		if(startBeyondPlane && !endBeyondPlane){
			start = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, &t);
		}else if(!startBeyondPlane && endBeyondPlane){
			end = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, &t);
		}else if(startBeyondPlane && endBeyondPlane){
			return false;
		}
		
		//clip to the far plane
		planePoint = vec3(0, 0, farZ);
		planeNormal = vec3::BACK;
		d = planeNormal.dot(planePoint);
		startBeyondPlane = planeNormal.dot(start) - d < 0;
		endBeyondPlane = planeNormal.dot(end) - d < 0;
		if(startBeyondPlane && !endBeyondPlane){
			start = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, &t);
		}else if(!startBeyondPlane && endBeyondPlane){
			end = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, &t);
		}else if(startBeyondPlane && endBeyondPlane){
			return false;
		}
		return true;
	} //ClipLineToZPlanes
	
	//cohen-sutherland algorithm https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm
	//the input vectors should be in screen space
	//returns true if the line can be rendered after clipping, false otherwise
	static bool ClipLineToBorderPlanes(vec2& start, vec2& end, vec2 dimensions){
		//clip to the vertical and horizontal planes
		const int CLIP_INSIDE = 0;
		const int CLIP_LEFT = 1;
		const int CLIP_RIGHT = 2;
		const int CLIP_BOTTOM = 4;
		const int CLIP_TOP = 8;
		auto ComputeOutCode = [&](vec2& vertex){
			int code = CLIP_INSIDE;
			if(vertex.x < 0){
				code |= CLIP_LEFT;
			}else if(vertex.x > dimensions.x){
				code |= CLIP_RIGHT;
			}
			if(vertex.y < 0){ //these are inverted because we are in screen space
				code |= CLIP_TOP;
			}else if(vertex.y > dimensions.y){
				code |= CLIP_BOTTOM;
			}
			return code;
		};
		
		int lineStartCode = ComputeOutCode(start);
		int lineEndCode = ComputeOutCode(end);
		
		//loop until all points are within or outside the screen zone
		while (true){
			if(!(lineStartCode | lineEndCode)){
				//both points are inside the screen zone
				return true;
			}else if(lineStartCode & lineEndCode){
				//both points are in the same outside zone
				return false;
			}else{
				float x = 0, y = 0;
				//select one of the points outside
				int code = lineEndCode > lineStartCode ? lineEndCode : lineStartCode;
				
				//clip the points the the screen bounds by finding the intersection point
				if      (code & CLIP_TOP){    //point is above screen
					x = start.x + (end.x - start.x) * (-start.y) / (end.y - start.y);
					y = 0;
				}else if(code & CLIP_BOTTOM){ //point is below screen
					x = start.x + (end.x - start.x) * (dimensions.y - start.y) / (end.y - start.y);
					y = dimensions.y;
				}else if(code & CLIP_RIGHT){  //point is right of screen
					y = start.y + (end.y - start.y) * (dimensions.x - start.x) / (end.x - start.x);
					x = dimensions.x;
				}else if(code & CLIP_LEFT){   //point is left of screen
					y = start.y + (end.y - start.y) * (-start.x) / (end.x - start.x);
					x = 0;
				}
				
				//update the vector's points and restart loop
				if(code == lineStartCode){
					start.x = x;
					start.y = y;
					lineStartCode = ComputeOutCode(start);
				}else{
					end.x = x;
					end.y = y;
					lineEndCode = ComputeOutCode(end);
				}
			}
		}
	} //ClipLineToBorderPlanes
	
	//returns area of a triangle of sides a and b
	static float TriangleArea(vec3 a, vec3 b){ 
		return a.cross(b).mag() / 2.f; 
	}
	
	//The normal this returns heavily depends on how you give it the points
	static vec3 TriangleNormal(vec3 p1, vec3 p2, vec3 p3){
		return (p3 - p1).cross(p2 - p1).normalized();
	}
	
	static vec3 TriangleMidpoint(vec3 p1, vec3 p2, vec3 p3){
		return (p1 + p2 + p3) / 3.f;
	}
	
	inline static vec4 ProjMult(vec4 v, const mat4& m){
		vec4 nv = v * m;
		Assert(nv.w != 0);
		nv.x /= nv.w; 
		nv.y /= nv.w; 
		nv.z /= nv.w;
		return nv;
	}
	
	static vec3 WorldToCamera3(vec3 vertex, const mat4& viewMat){
		return Math::ProjMult(vertex.toVec4(), viewMat).toVec3();
	}
	
	static vec4 WorldToCamera4(vec3 vertex, const mat4& viewMat){
		return Math::ProjMult(vertex.toVec4(), viewMat);
	}
	
	static vec3 CameraToWorld3(vec3 vertex, const mat4& viewMat){
		return Math::ProjMult(vertex.toVec4(), viewMat.Inverse()).toVec3();
	}
	
	static vec4 CameraToWorld4(vec3 vertex, const mat4& viewMat){
		return Math::ProjMult(vertex.toVec4(), viewMat.Inverse());
	}
	
	static vec2 CameraToScreen2(vec3 csVertex, const mat4& projMat, vec2 screenDimensions){
		vec3 vm = Math::ProjMult(csVertex.toVec4(), projMat).toVec3();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * screenDimensions.x;
		vm.y *= 0.5f * screenDimensions.y;
		return vm.toVec2();
	}
	
	static vec3 CameraToScreen3(vec3 csVertex, const mat4& projMat, vec2 screenDimensions){
		vec3 vm = Math::ProjMult(csVertex.toVec4(), projMat).toVec3();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * screenDimensions.x;
		vm.y *= 0.5f * screenDimensions.y;
		return vm;
	}
	
	static vec3 CameraToScreen3(vec3 csVertex, const mat4& projMat, vec2 screenDimensions, float& w){
		vec4 bleh = csVertex.toVec4() * projMat;
		w = bleh.w;
		vec3 vm = bleh.wnormalized().toVec3();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * screenDimensions.x;
		vm.y *= 0.5f * screenDimensions.y;
		return vm;
	}
	
	static vec3 CameraToScreen3(vec4 csVertex, const mat4& projMat, vec2 screenDimensions){
		vec3 vm = Math::ProjMult(csVertex, projMat).toVec3();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * screenDimensions.x;
		vm.y *= 0.5f * screenDimensions.y;
		return vm;
	}
	
	static vec4 CameraToScreen4(vec4 csVertex, const mat4& projMat, vec2 screenDimensions){
		vec4 vm = (csVertex * projMat).wnormalized();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * screenDimensions.x;
		vm.y *= 0.5f * screenDimensions.y;
		return vm;
	}
	
	static vec3 WorldToScreen(vec3 point, const mat4& ProjMat, const mat4& ViewMat, vec2 screenDimensions){
		return CameraToScreen3(WorldToCamera4(point, ViewMat), ProjMat, screenDimensions);
	}
	
	static vec2 WorldToScreen2(vec3 point, const mat4& ProjMat, const mat4& ViewMat, vec2 screenDimensions){
		vec3 v = CameraToScreen3(WorldToCamera4(point, ViewMat), ProjMat, screenDimensions);
		return vec2(v.x, v.y);
	}
	
	static vec3 ScreenToWorld(vec2 pos, const mat4& ProjMat, const mat4& view, vec2 screenDimensions){
		vec4 out{
			2.0f*(pos.x / screenDimensions.x) - 1.0f,
			2.0f*(pos.y / screenDimensions.y) - 1.0f,
			-1.0f,
			1.0f
		};
		out = Math::ProjMult(out, ProjMat.Inverse());
		out = Math::ProjMult(out, view.Inverse());
		return out.toVec3();
	}
};

#endif //DESHI_MATH_H