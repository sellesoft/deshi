#pragma once
#ifndef DESHI_VECTOR_H
#define DESHI_VECTOR_H

#include "../defines.h"

struct vec2;
struct vec3;
struct vec4;
struct matN;
struct mat3;
struct mat4;
struct quat;

//////////////////////
//// declarations ////
//////////////////////

static constexpr float VEC_EPSILON = 0.00001f;

struct vec2 {
	union{ float x = 0; float r; float w; float u; };
	union{ float y = 0; float g; float h; float v; };
	
	vec2(){};
	vec2(float inX, float inY);
	vec2(const vec2& v);
	vec2(float* ptr);
	
	static const vec2 ZERO;
	static const vec2 ONE;
	static const vec2 UP;
	static const vec2 DOWN;
	static const vec2 LEFT;
	static const vec2 RIGHT;
	static const vec2 UNITX;
	static const vec2 UNITY;
	
	void operator= (const vec2& rhs);
	vec2 operator* (float rhs) const;
	void operator*=(float rhs);
	vec2 operator/ (float rhs) const;
	void operator/=(float rhs);
	vec2 operator+ (const vec2& rhs) const;
	void operator+=(const vec2& rhs);
	vec2 operator- (const vec2& rhs) const;
	void operator-=(const vec2& rhs);
	vec2 operator* (const vec2& rhs) const;
	void operator*=(const vec2& rhs);
	vec2 operator/ (const vec2& rhs) const;
	void operator/=(const vec2& rhs);
	vec2 operator- () const;
	bool operator==(const vec2& rhs) const;
	bool operator!=(const vec2& rhs) const;
	friend vec2 operator* (float lhs, const vec2& rhs){ return rhs * lhs; }
	
	vec2  absV() const;
	vec2  copy() const;
	float dot(const vec2& rhs) const;
	vec2  cross(const vec2& rhs) const;
	vec2  perp() const;
	float mag() const;
	void  normalize();
	vec2  normalized() const;
	void  clampMag(float min, float max);
	vec2  clampedMag(float min, float max) const;
	float distanceTo(const vec2& rhs) const;
	vec2  compOn(const vec2& rhs) const;
	float projectOn(const vec2& rhs) const;
	vec2  midpoint(const vec2& rhs) const;
	vec2  xComp() const;
	vec2  yComp() const;
	vec2  xInvert() const;
	vec2  yInvert() const;
	vec2  xSet(float set) const; //for single line vector arithmetic eg. ShowDebugWindowOf() when I call BeginWindow
	vec2  ySet(float set) const; //for single line vector arithmetic eg. ShowDebugWindowOf() when I call BeginWindow
	vec2  xAdd(float set) const; //for single line vector arithmetic eg. ShowDebugWindowOf() when I call BeginWindow
	vec2  yAdd(float set) const; //for single line vector arithmetic eg. ShowDebugWindowOf() when I call BeginWindow
	
	//vector interactions
	vec2(const vec3& v);
	vec2(const vec4& v);
	vec3 toVec3() const;
	vec4 toVec4() const;
};
#include "vec2.inl"

struct vec3 {
	union{ float x = 0; float r; };
	union{ float y = 0; float g; };
	union{ float z = 0; float b; };
	
	vec3(){};
	vec3(float inX, float inY, float inZ);
	vec3(float inX, float inY);
	vec3(const vec3& v);
	vec3(float* ptr);
	
	static const vec3 ZERO;
	static const vec3 ONE;
	static const vec3 RIGHT;
	static const vec3 LEFT;
	static const vec3 UP;
	static const vec3 DOWN;
	static const vec3 FORWARD;
	static const vec3 BACK;
	static const vec3 UNITX;
	static const vec3 UNITY;
	static const vec3 UNITZ;
	
	void operator= (const vec3& rhs);
	void operator= (vec3& rhs);
	vec3 operator* (float rhs) const;
	void operator*=(float rhs);
	vec3 operator/ (float rhs) const;
	void operator/=(float rhs);
	vec3 operator+ (const vec3& rhs) const;
	void operator+=(const vec3& rhs);
	vec3 operator- (const vec3& rhs) const;
	void operator-=(const vec3& rhs);
	vec3 operator* (const vec3& rhs) const;
	void operator*=(const vec3& rhs);
	vec3 operator/ (const vec3& rhs) const;
	void operator/=(const vec3& rhs);
	vec3 operator- () const;
	bool operator==(const vec3& rhs) const;
	bool operator!=(const vec3& rhs) const;
	friend vec3 operator* (float lhs, const vec3& rhs){ return rhs * lhs; }
	
	vec3  absV() const;
	vec3  copy() const;
	float dot(const vec3& rhs) const;
	vec3  cross(const vec3& rhs) const;
	float mag() const;
	void  normalize();
	vec3  normalized() const;
	vec3  clamp(float lo, float hi) const;
	void  clampMag(float min, float max);
	vec3  clampedMag(float min, float max) const;
	void  round(int place);
	vec3  rounded(int place) const;
	float distanceTo(const vec3& rhs) const;
	vec3  compOn(const vec3& rhs) const;
	float projectOn(const vec3& rhs) const;
	vec3  midpoint(const vec3& rhs) const;
	vec3  xComp() const;
	vec3  yComp() const;
	vec3  zComp() const;
	vec3  xInvert() const;
	vec3  yInvert() const;
	vec3  zInvert() const;
	
	//vector interactions
	vec3(const vec2& v);
	vec3(const vec4& v);
	vec2 toVec2() const;
	vec4 toVec4() const;
	
	//matrix interactions
	vec3 operator* (const mat3& rhs) const;
	void operator*=(const mat3& rhs);
	vec3 operator* (const mat4& rhs) const;
	void operator*=(const mat4& rhs);
	matN ToM1x3() const;
	matN ToM1x4(float w) const;
	
    //quaternion interactions
    vec3 operator* (const quat& rhs) const;
};
#include "vec3.inl"

struct vec4 {
	union{ float x = 0; float r; };
	union{ float y = 0; float g; };
	union{ float z = 0; float b; };
	union{ float w = 0; float a; };
	
	vec4(){};
	vec4(float inX, float inY, float inZ, float inW);
	vec4(const vec4& v);
	vec4(float* ptr);
	
	static const vec4 ZERO;
	static const vec4 ONE;
	
	void operator= (const vec4& rhs);
	vec4 operator* (const float& rhs) const;
	void operator*=(const float& rhs);
	vec4 operator/ (const float& rhs) const;
	void operator/=(const float& rhs);
	vec4 operator+ (const vec4& rhs) const;
	void operator+=(const vec4& rhs);
	vec4 operator- (const vec4& rhs) const;
	void operator-=(const vec4& rhs);
	vec4 operator* (const vec4& rhs) const;
	void operator*=(const vec4& rhs);
	vec4 operator/ (const vec4& rhs) const;
	void operator/=(const vec4& rhs);
	vec4 operator- () const;
	bool operator==(const vec4& rhs) const;
	bool operator!=(const vec4& rhs) const;
	friend vec4 operator* (const float& lhs, const vec4& rhs){ return rhs * lhs; }
	
	vec4  absV() const;
	vec4  copy() const;
	float dot(const vec4& rhs) const;
	float mag() const;
	vec4  wnormalized() const;
	vec4  xComp() const;
	vec4  yComp() const;
	vec4  zComp() const;
	vec4  wComp() const;
	vec4  xInvert() const;
	vec4  yInvert() const;
	vec4  zInvert() const;
	vec4  wInvert() const;
	
	//vector interactions
	vec4(const vec2& v, float z, float w);
	vec4(const vec3& v, float w);
	vec3 toVec3() const;
	void takeVec3(const vec3& v);
	
	//matrix interactions
	vec4 operator* (const mat4& rhs) const;
	void operator*=(const mat4& rhs);
};
#include "vec4.inl"

//////////////////////
//// interactions ////
//////////////////////

//// vec2 ////

inline vec2::
vec2(const vec3& v){
	x = v.x; y = v.y;
}

inline vec2::
vec2(const vec4& v){
	x = v.x; y = v.y;
}

inline vec3 vec2::
toVec3() const {
	return vec3(x, y, 0);
}

inline vec4 vec2::
toVec4() const {
	return vec4(x, y, 0, 0);
}

//// vec3 ////

inline vec3::
vec3(const vec2& v) {
	x = v.x; y = v.y; z = 0;
}

inline vec3::
vec3(const vec4& v) {
	x = v.x; y = v.y; z = v.z;
}

inline vec2 vec3::
toVec2() const {
	return vec2(x, y);
}

inline vec4 vec3::
toVec4() const {
	return vec4(x, y, z, 1);
}

//// vec4 ////

inline vec4::
vec4(const vec2& v, float inZ, float inW) {
	x = v.x; y = v.y; z = inZ; w = inW;
}

inline vec4::
vec4(const vec3& v, float inW) {
	x = v.x; y = v.y; z = v.z; w = inW;
}

inline vec3 vec4::
toVec3() const {
	return vec3(x, y, z);
}

//takes data from a vec3 and leaves w alone
inline void vec4::
takeVec3(const vec3& v) {
	x = v.x; y = v.y; z = v.z;
}

/////////////////
//// defines ////
/////////////////

#define RANDVEC(a) vec3(rand() % a + 1, rand() % a + 1, rand() % a + 1)

//averages a vector v over an interval i and returns that average
#define V_AVG(i, v) \
([&] { \
persist std::vector<vec3> vectors; \
persist vec3 nv; \
persist int iter = 0; \
if(i == vectors.size()){ \
vectors.erase(vectors.begin()); \
vectors.push_back(v); \
iter++; \
} \
else{ \
vectors.push_back(v); \
iter++; \
}\
if(iter == i){ \
nv = Math::averagevec3(vectors); \
iter = 0; \
} \
return nv; \
}())

//averages vectors but consistently returns the value
#define V_AVGCON(i, v) \
([&] { \
persist std::vector<vec3> vectors; \
persist vec3 nv; \
if(i == vectors.size()){ \
vectors.erase(vectors.begin()); \
vectors.push_back(v); \
} \
else{ \
vectors.push_back(v); \
} \
nv = Math::averagevec3(vectors); \
return nv; \
}())

//this stores an input vector and returns the previously stored vector
//if you pass true for the second param it will replace the stored vector and return it
//else it just returns the stored vector
#define V_STORE(v, t) \
([&]()->vec3{\
persist vec3 vect[1];\
vec3 vr = vect[0];\
if(t){ vect[0] = v; return vr; } \
else return vr; }\
())

#endif //DESHI_VECTOR_H