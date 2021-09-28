#pragma once
#ifndef DESHI_MATH_H
#define DESHI_MATH_H

#include "VectorMatrix.h"
#include "quaternion.h"
#include "matN.h"
#include "../defines.h"

#include <math.h>
#include <algorithm>
#include <numeric>

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

namespace Math {
	static vec4 ProjMult(vec4 v, mat4 m) {
		vec4 nv = v * m;
		if (nv.w != 0) { nv.x /= nv.w; nv.y /= nv.w; nv.z /= nv.w; }
		return nv;
	}
	
	static mat4 LocalToWorld(vec3 offsetFromOrigin) {
		return mat4::TranslationMatrix(offsetFromOrigin);
	}
	
	static mat4 WorldToLocal(vec3 offsetFromOrigin) {
		return mat4::TranslationMatrix(offsetFromOrigin).Inverse();
	}
	
	//returns how far along a polynomial fit for a set of data you are
	//you are not allowed to have 2 points with the same x value here
	//using Lagrange Polynomials
	//TODO(sushi, Ma) look into maybe implementing Newton's Polynomials at some point idk if they're better but they look more simple
	static float PolynomialCurveInterpolation(std::vector<vec2> vs, float t) {
		float sum = 0;
		for (int j = 0; j < vs.size(); j++) {
			float vy = vs[j].y; 
			float jx = vs[j].x;
			float lbp = 1;
			for (int m = 0; m < vs.size(); m++) {
				if (lbp != 0) {
					if (m != j) {
						float mx = vs[m].x;
						lbp *= (t - mx) / (jx - mx);
					}
				} else break;
			}
			sum += vy * lbp;
		}
		return sum;
	}
	
	static vec2 vec2RotateByAngle(float angle, vec2 v) {
		angle = RADIANS(angle);
		return vec2(v.x * cosf(angle) - v.y * sinf(angle), v.x * sin(angle) + v.y * cos(angle));
	}
	
	inline global_ bool PointInRectangle(vec2 point, vec2 rectPos, vec2 rectDims) {
		return
			point.x >= rectPos.x &&
			point.y >= rectPos.y &&
			point.x <= rectPos.x + rectDims.x &&
			point.y <= rectPos.y + rectDims.y;
	}
	
	//oscillates between a given upper and lower value based on a given x value
	inline global_ float BoundedOscillation(float lower, float upper, float x) {
		assert(upper > lower);
		return ((upper - lower) * cosf(x) + (upper + lower)) / 2;
	}

}

//// Non-quat vs quat Interactions ////

inline vec3 quat::operator *(const vec3& rhs) {
	return (quat(rhs.x, rhs.y, rhs.z, 0) * *this).toVec3();
}

//TODO(sushi, MaCl) move this and others into their our quat / quat nonQuat file
inline vec3 vec3::operator *(const quat& rhs) const {
	return (quat(x, y, z, 0) * rhs).toVec3();
}

inline quat::quat(const vec3& rotation) {
	this->RotVecToQuat(rotation);
}

inline quat::quat(const vec3& axis, float theta) {
	*this = AxisAngleToQuat(theta, axis);
}

//https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles?oldformat=true
inline vec3 quat::toVec3() {
	vec3 angles;
	
	// roll (x-axis rotation)
	double sinr_cosp = 2 * (w * x + y * z);
	double cosr_cosp = 1 - 2 * (x * x + y * y);
	angles.x = atan2(sinr_cosp, cosr_cosp);
	
	// pitch (y-axis rotation)
	double sinp = 2 * (w * y - z * x);
	if (std::abs(sinp) >= 1)
		angles.y = copysign(M_PI / 2, sinp); // use 90 degrees if out of range
	else
		angles.y = asin(sinp);
	
	// yaw (z-axis rotation)
	double siny_cosp = 2 * (w * z + x * y);
	double cosy_cosp = 1 - 2 * (y * y + z * z);
	angles.z = atan2(siny_cosp, cosy_cosp);
	
	return angles;
}

//converts an angle and an axis to a quaternion
//im not sure how this works or where it will be used and im not even sure if its
//set up properly (sorry)
inline quat quat::AxisAngleToQuat(float angle, vec3 axis) {
	float angler = RADIANS(angle);
	return quat(sinf(angler / 2) * axis.x, sinf(angler / 2) * axis.y, sinf(angler / 2) * axis.z, cosf(angler / 2));
}

//this may be wrong but I think a rotation vector would be 
//vec3(roll, pitch, yaw)
//https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles?oldformat=true
inline quat quat::RotVecToQuat(vec3 rotation) {
	//this is probably necessary although he didn't do this in the Gamasutra article
	vec3 rotationrad = RADIANS(rotation);
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

inline quat quat::QuatSlerp(vec3 fromv, vec3 tov, float t) {
	//this implements Spherical Linear intERPoplation
	//it interpolates between two quaternions along the shortest arc on a sphere formed by them
	//taken from https://www.wikiwand.com/en/Slerp#/Quaternion_Slerp
	
	quat from = RotVecToQuat(fromv);
	quat to = RotVecToQuat(tov);
	
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
}

namespace Math {
	
	//ref: https://en.cppreference.com/w/cpp/algorithm/clamp
	static float clamp(float v, float lo, float hi) {
		Assert(lo < hi, "The low must be less than the high clamp");
		return (v < lo) ? lo : (hi < v) ? hi : v;
	}
	
	static void clampr(float& v, float lo, float hi) {
		Assert(lo <= hi, "The low must be less than the high clamp");
		v = (v < lo) ? lo : (hi < v) ? hi : v;
	}
	
	static vec3 clamp(vec3 v, float lo, float hi) {
		Assert(lo <= hi, "The low must be less than the high clamp");
		return v.clamp(lo, hi);
	}
	
	//for debugging with floats or doubles
	static std::string append_decimal(std::string s) {
		while (s.back() != '.') {
			s.pop_back();
		}
		s.pop_back();
		return s;
	}
	
	//append all trailing zeros
	static std::string append_zeroes(std::string s) {
		while (s.back() == '0') {
			s.pop_back();
		}
		s.pop_back();
		return s;
	}
	
	//append to two decimal places
	static std::string append_two_decimal(std::string s) {
		if (s.length() >= 2) {
			while (s.at(s.length() - 4) != '.') {
				s.pop_back();
			}
			s.pop_back();
		}
		return s;
	}
	
	static std::string str2f(float s) {
		char buffer[50];
		std::snprintf(buffer, 50, "%-.2f", s);
		return std::string(buffer);
	}
	
	//round a float to two decimal places
	static float round2f(float f) { return (float)((int)(f * 100 + .5)) / 100; }
	static float round4f(float f) { return (float)((int)(f * 10000 + .5)) / 10000; }
	
	
	static vec3 round2v(vec3 v) {
		return vec3(
					(float)((int)(v.x * 100 + .5)) / 100,
					(float)((int)(v.y * 100 + .5)) / 100,
					(float)((int)(v.z * 100 + .5)) / 100);
	}
	
	//average any std container probably
	template<class FWIt>
		static float average(FWIt a, const FWIt b, int size) { return std::accumulate(a, b, 0.0) / size; }
	
	template<class T>
		static double average(const T& container, int size) { return average(std::begin(container), std::end(container), size); }
	
	//interpolating
	template<typename T> global_ T lerp(T a, T b, f32 t){ return a*(1.f-t) + b*t; }
	
	[[deprecated("Use Math::lerp() instead")]]
		static float lerpf(float p1, float p2, float t) { return (1.f - t) * p1 + t * p2; }
	[[deprecated("Use Math::lerp() instead")]]
		static vec3 lerpv(vec3 v1, vec3 v2, float t) { return  v1 * (1.f - t) + v2 * t; }
	[[deprecated("Use Math::lerp() instead")]]
		static vec2 lerpv(vec2 v1, vec2 v2, float t) { return  v1 * (1.f - t) + v2 * t; }
	[[deprecated("Use Math::lerp() instead")]]
		static mat4 lerpm4(mat4 m1, mat4 m2, float t) { return m1 * (1.f - t) + m2 * t; }
	
	
	//returns in degrees
	//this doesn't really work in 3D but this function is here anyways
	static float AngBetweenVectors(vec3 v1, vec3 v2) {
		return DEGREES(acosf(v1.dot(v2) / (v1.mag() * v2.mag())));
	}
	
	//returns in degrees
	static float AngBetweenVectors(vec2 v1, vec2 v2) {
		return DEGREES(atan2(v1.x * v2.y - v1.y * v2.x, v1.dot(v2)));
	}
	
	//returns in degrees between 0 and 360
	static float AngBetweenVectors360(vec2 v1, vec2 v2) {
		float ang = DEGREES(atan2(v1.x * v2.y - v1.y * v2.x, v1.dot(v2)));
		return (ang < 0) ? 360 + ang : ang;
	}
	
	//NOTE 0-1 depth range
	static mat4 PerspectiveProjectionMatrix(f32 width, f32 height, f32 hFOV, f32 nearZ, f32 farZ){
		float renderDistance = farZ - nearZ;
		float aspectRatio = (f32)height / (f32)width;
		float fovRad = 1.0f / tanf(RADIANS(hFOV / 2.0f));
		return mat4( //NOTE setting (1,1) to negative flips the y-axis
					aspectRatio * fovRad, 0,	   0,							  0,
					0,					-fovRad, 0,							  0,
					0,					0,	   farZ / renderDistance,		  1,
					0,					0,	   -(farZ*nearZ) / renderDistance, 0);
	}
	
	//this function returns a matrix that tells a vector how to look at a specific point in space.
	static mat4 LookAtMatrix(const vec3& pos, const vec3& target) {
		if(pos == target) { return LookAtMatrix(pos, target + vec3(.01f, 0, 0)); }
		
		//get new forward direction
		vec3 newFor = (target - pos).normalized();
		
		//get right direction
		vec3 newRight; 
		if(newFor == vec3::UP || newFor == vec3::DOWN) { 
			newRight = vec3::RIGHT; 
		} 
		else {
			newRight = (vec3::UP.cross(newFor)).normalized(); 
		}
		
		//get up direction
		vec3 newUp = newFor.cross(newRight); 
		
		//make look-at matrix
		return mat4(newRight.x, newRight.y, newRight.z, 0,
					newUp.x,    newUp.y,   newUp.z,    0,
					newFor.x,   newFor.y,   newFor.z,   0,
					pos.x,      pos.y,      pos.z,      1);
	}
	
	//this ones for getting the up vector back for sound orientation
	static mat4 LookAtMatrix(const vec3& pos, const vec3& target, vec3& up) {
		if (pos == target) { return LookAtMatrix(pos, target + vec3(.01f, 0, 0)); }
		
		vec3 newFor = (target - pos).normalized();
		
		vec3 newRight;
		if (newFor == vec3::UP || newFor == vec3::DOWN) {
			newRight = vec3::RIGHT;
		} 
		else {
			newRight = (vec3::UP.cross(newFor)).normalized();
		}
		
		//get up direction
		up = newFor.cross(newRight);
		
		//make look-at matrix
		return mat4(newRight.x, newRight.y, newRight.z, 0,
					up.x,       -up.y,      up.z,       0,
					newFor.x,   newFor.y,   newFor.z,   0,
					pos.x,      pos.y,      pos.z,      1);
	}
	
	//this assumes its in degrees
	static vec3 SphericalToRectangularCoords(vec3 v) {
		float y = RADIANS(v.y);
		float z = RADIANS(v.z);
		return vec3(v.x * sinf(z) * cosf(y), v.x * cosf(z), v.x * sinf(z) * sinf(y));
	}
	
	static vec3 RectangularToSphericalCoords(vec3 v) {
		float rho = RADIANS(sqrt(v.mag()));
		float theta = RADIANS(atan(v.y / v.z));
		float phi = acos(v.z / v.mag()); //maybe use v.y instead of v.z because y is our vertical axis
		return vec3(rho, theta, phi);
		
	}
	
	static float DistTwoPoints(vec3 a, vec3 b) {
		return sqrtf(
					 (a.x - b.x) * (a.x - b.x) +
					 (a.y - b.y) * (a.y - b.y) +
					 (a.z - b.z) * (a.z - b.z));
	}
	
	static inline float DistPointToPlane(vec3 point, vec3 plane_n, vec3 plane_p) {
		return (point - plane_p).dot(plane_n);
	}
	
	//where a line intersects with a plane, 'returns' how far along line you were as t value
	static vec3 VectorPlaneIntersect(vec3 plane_p, vec3 plane_n, vec3 line_start, vec3 line_end, float& t) {
		vec3 lstole = (line_end - line_start).normalized();
		vec3 lptopp = plane_p - line_start;
		t = lptopp.dot(plane_n) / lstole.dot(plane_n);
		return line_start + t * lstole;
	}
	
	//where a line intersects with a plane
	static vec3 VectorPlaneIntersect(vec3 plane_p, vec3 plane_n, vec3 line_start, vec3 line_end) {
		vec3 lstole = (line_end - line_start).normalized();
		vec3 lptopp = plane_p - line_start;
		float t = lptopp.dot(plane_n) / lstole.dot(plane_n);
		return line_start + t * lstole;
	}
	
	//return where two lines intersect on the x axis with slope and the y-intercept
	static vec2 LineIntersect2(float slope1, float ycross1, float slope2, float ycross2) {
		matN lhs(2,2,{ slope1, ycross1, slope2, ycross2 });
		matN rhs(2,1,{ 1, 1 });
		matN det = lhs.Inverse() * rhs;
		float x = 1 / det(1,0) * det(0,0);
		float y = slope1 * x + ycross1;
		return vec2(x, y);
	}
	
	//returns where two lines intersect in 3D space //TODO(sushi, MaGe) implement this
	static vec3 LineIntersect3(vec3 adir, vec3 ap, vec3 bdir, vec3 bp) {}
	
	static vec3 Midpointv3(std::vector<vec3> vectors) {
		vec3 sum = vec3::ZERO;
		for (vec3 v : vectors) sum += v;
		return sum / vectors.size();
	}
	
	static vec3 LineMidpoint(vec3 start, vec3 end){
		return (start+end)/2.f;
	}
	
	//the input vectors should be in viewMat/camera space
	//returns true if the line can be rendered after clipping, false otherwise
	static bool ClipLineToZPlanes(vec3& start, vec3& end, f32 nearZ, f32 farZ) {
		//clip to the near plane
		vec3 planePoint = vec3(0, 0, nearZ);
		vec3 planeNormal = vec3::FORWARD;
		float d = planeNormal.dot(planePoint);
		bool startBeyondPlane = planeNormal.dot(start) - d < 0;
		bool endBeyondPlane = planeNormal.dot(end) - d < 0;
		float t;
		if (startBeyondPlane && !endBeyondPlane) {
			start = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, t);
		} else if (!startBeyondPlane && endBeyondPlane) {
			end = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, t);
		} else if (startBeyondPlane && endBeyondPlane) {
			return false;
		}
		
		
		//clip to the far plane
		planePoint = vec3(0, 0, farZ);
		planeNormal = vec3::BACK;
		d = planeNormal.dot(planePoint);
		startBeyondPlane = planeNormal.dot(start) - d < 0;
		endBeyondPlane = planeNormal.dot(end) - d < 0;
		if (startBeyondPlane && !endBeyondPlane) {
			start = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, t);
		} else if (!startBeyondPlane && endBeyondPlane) {
			end = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, t);
		} else if (startBeyondPlane && endBeyondPlane) {
			return false;
		}
		return true;
	} //ClipLineToZPlanes
	
	//cohen-sutherland algorithm https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm
	//the input vectors should be in screen space
	//returns true if the line can be rendered after clipping, false otherwise
	static bool ClipLineToBorderPlanes(vec2& start, vec2& end, vec2 dimensions) {
		//clip to the vertical and horizontal planes
		const int CLIP_INSIDE = 0;
		const int CLIP_LEFT = 1;
		const int CLIP_RIGHT = 2;
		const int CLIP_BOTTOM = 4;
		const int CLIP_TOP = 8;
		auto ComputeOutCode = [&](vec2& vertex) {
			int code = CLIP_INSIDE;
			if (vertex.x < 0) {
				code |= CLIP_LEFT;
			}
			else if (vertex.x > dimensions.x) {
				code |= CLIP_RIGHT;
			}
			if (vertex.y < 0) { //these are inverted because we are in screen space
				code |= CLIP_TOP;
			}
			else if (vertex.y > dimensions.y) {
				code |= CLIP_BOTTOM;
			}
			return code;
		};
		
		int lineStartCode = ComputeOutCode(start);
		int lineEndCode = ComputeOutCode(end);
		
		//loop until all points are within or outside the screen zone
		while (true) {
			if (!(lineStartCode | lineEndCode)) {
				//both points are inside the screen zone
				return true;
			}
			else if (lineStartCode & lineEndCode) {
				//both points are in the same outside zone
				return false;
			}
			else {
				float x, y;
				//select one of the points outside
				int code = lineEndCode > lineStartCode ? lineEndCode : lineStartCode;
				
				//clip the points the the screen bounds by finding the intersection point
				if			(code & CLIP_TOP) {		//point is above screen
					x = start.x + (end.x - start.x) * (-start.y) / (end.y - start.y);
					y = 0;
				} 
				else if	(code & CLIP_BOTTOM) {		//point is below screen
					x = start.x + (end.x - start.x) * (dimensions.y - start.y) / (end.y - start.y);
					y = dimensions.y;
				} 
				else if	(code & CLIP_RIGHT) {		//point is right of screen
					y = start.y + (end.y - start.y) * (dimensions.x - start.x) / (end.x - start.x);
					x = dimensions.x;
				} 
				else if	(code & CLIP_LEFT) {		//point is left of screen
					y = start.y + (end.y - start.y) * (-start.x) / (end.x - start.x);
					x = 0;
				}
				
				//update the vector's points and restart loop
				if (code == lineStartCode) {
					start.x = x;
					start.y = y;
					lineStartCode = ComputeOutCode(start);
				}
				else {
					end.x = x;
					end.y = y;
					lineEndCode = ComputeOutCode(end);
				}
			}
		}
	} //ClipLineToBorderPlanes
	
	//NOTE these triangle functions are quite useless I think but I put them here in case
	//since I deleted Triangle.h
	
	//returns area of a triangle of sides a and b
	static float TriangleArea(vec3 a, vec3 b) { return a.cross(b).mag() / 2; }
	
	//The normal this returns heavily depends on how you give it the points
	static vec3 TriangleNormal(vec3 p1, vec3 p2, vec3 p3) {
		return (p3 - p1).cross(p2 - p1).normalized();
	}
	
	static vec3 TriangleMidpoint(vec3 p1, vec3 p2, vec3 p3) {
		return (p1 + p2 + p3) / 3.f;
	}
	
	static vec3 WorldToCamera3(vec3 vertex, mat4 viewMat) {
		return Math::ProjMult(vertex.toVec4(), viewMat).toVec3();
	}
	
	static vec4 WorldToCamera4(vec3 vertex, mat4 viewMat) {
		return Math::ProjMult(vertex.toVec4(), viewMat);
	}
	
	static vec3 CameraToWorld3(vec3 vertex, mat4 viewMat) {
		return Math::ProjMult(vertex.toVec4(), viewMat.Inverse()).toVec3();
	}
	
	static vec4 CameraToWorld4(vec3 vertex, mat4 viewMat) {
		return Math::ProjMult(vertex.toVec4(), viewMat.Inverse());
	}
	
	static vec2 CameraToScreen2(vec3 csVertex, mat4 projMat, vec2 screenDimensions) {
		vec3 vm = Math::ProjMult(csVertex.toVec4(), projMat).toVec3();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * (float)screenDimensions.x;
		vm.y *= 0.5f * (float)screenDimensions.y;
		return vm.toVec2();
	}
	
	static vec3 CameraToScreen3(vec3 csVertex, mat4 projMat, vec2 screenDimensions) {
		vec3 vm = Math::ProjMult(csVertex.toVec4(), projMat).toVec3();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * (float)screenDimensions.x;
		vm.y *= 0.5f * (float)screenDimensions.y;
		return vm;
	}
	
	static vec3 CameraToScreen3(vec3 csVertex, mat4 projMat, vec2 screenDimensions, float& w) {
		vec4 bleh = csVertex.toVec4() * projMat;
		w = bleh.w;
		vec3 vm = bleh.wnormalized().toVec3();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * (float)screenDimensions.x;
		vm.y *= 0.5f * (float)screenDimensions.y;
		return vm;
	}
	
	static vec3 CameraToScreen3(vec4 csVertex, mat4 projMat, vec2 screenDimensions) {
		vec3 vm = Math::ProjMult(csVertex, projMat).toVec3();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * (float)screenDimensions.x;
		vm.y *= 0.5f * (float)screenDimensions.y;
		return vm;
	}
	
	static vec4 CameraToScreen4(vec4 csVertex, mat4 projMat, vec2 screenDimensions) {
		vec4 vm = (csVertex * projMat).wnormalized();
		vm.x += 1.0f; vm.y += 1.0f;
		vm.x *= 0.5f * (float)screenDimensions.x;
		vm.y *= 0.5f * (float)screenDimensions.y;
		return vm;
	}
	
	static vec3 WorldToScreen(vec3 point, mat4 ProjMat, mat4 ViewMat, vec2 screenDimensions) {
		return CameraToScreen3(WorldToCamera4(point, ViewMat), ProjMat, screenDimensions);
	}
	
	static vec2 WorldToScreen2(vec3 point, mat4 ProjMat, mat4 ViewMat, vec2 screenDimensions) {
		vec3 v = CameraToScreen3(WorldToCamera4(point, ViewMat), ProjMat, screenDimensions);
		return vec2(v.x, v.y);
	}
	
	static vec3 ScreenToWorld(vec2 pos, mat4 ProjMat, mat4 view, vec2 screenDimensions) {
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