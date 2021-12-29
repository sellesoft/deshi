#pragma once
#ifndef DESHI_VECTOR_H
#define DESHI_VECTOR_H
#include "math_utils.h"

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

struct vec2 {
	union{
		f32 arr[2] = {};
		struct{ f32 x, y; };
		struct{ f32 r, g; };
		struct{ f32 w, h; };
		struct{ f32 u, v; };
	};
	
	vec2(){};
	vec2(f32 inX, f32 inY);
	vec2(const vec2& v);
	vec2(f32* ptr);
	
	static const vec2 ZERO;
	static const vec2 ONE;
	static const vec2 UP;
	static const vec2 DOWN;
	static const vec2 LEFT;
	static const vec2 RIGHT;
	static const vec2 UNITX;
	static const vec2 UNITY;
	
	void operator= (const vec2& rhs);
	vec2 operator* (f32 rhs) const;
	void operator*=(f32 rhs);
	vec2 operator/ (f32 rhs) const;
	void operator/=(f32 rhs);
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
	friend vec2 operator* (f32 lhs, const vec2& rhs){ return rhs * lhs; }
	
	void  set(f32 x, f32 y);
	vec2  absV() const;
	vec2  copy() const;
	f32   dot(const vec2& rhs) const;
	vec2  perp() const;
	f32   mag()  const;
	void  normalize();
	vec2  normalized() const;
	void  clampMag(f32 min, f32 max);
	vec2  clampedMag(f32 min, f32 max) const;
	f32   distanceTo(const vec2& rhs)  const;
	vec2  compOn(const vec2& rhs)      const;
	f32   projectOn(const vec2& rhs)   const;
	vec2  midpoint(const vec2& rhs)    const;
	vec2  xComp() const;
	vec2  yComp() const;
	vec2  xInvert() const;
	vec2  yInvert() const;
	vec2  xSet(f32 set) const; 
	vec2  ySet(f32 set) const; 
	vec2  xAdd(f32 set) const; 
	vec2  yAdd(f32 set) const; 
	
	//vector interactions
	vec2(const vec3& v);
	vec2(const vec4& v);
	vec3 toVec3() const;
	vec4 toVec4() const;
};
#include "vec2.inl"

struct vec3 {
	union{
		f32 arr[3] = {};
		struct{ f32 x, y, z; };
		struct{ f32 r, g, b; };
		struct{ vec2 xy; f32 _unusedZ0; };
		struct{ f32 _unusedX0; vec2 yz; };
	};
	
	vec3(){};
	vec3(f32 inX, f32 inY, f32 inZ);
	vec3(f32 inX, f32 inY);
	vec3(const vec3& v);
	vec3(f32* ptr);
	
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
	vec3 operator* (f32 rhs) const;
	void operator*=(f32 rhs);
	vec3 operator/ (f32 rhs) const;
	void operator/=(f32 rhs);
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
	friend vec3 operator* (f32 lhs, const vec3& rhs){ return rhs * lhs; }
	
	void set(f32 x, f32 y, f32 z);
	vec3 absV() const;
	vec3 copy() const;
	f32  dot(const vec3& rhs) const;
	vec3 cross(const vec3& rhs) const;
	f32  mag() const;
	f32  magSq() const;
	void normalize();
	vec3 normalized() const;
	vec3 clamp(f32 lo, f32 hi) const;
	void clampMag(f32 min, f32 max);
	vec3 clampedMag(f32 min, f32 max) const;
	void round(s32 place);
	vec3 rounded(s32 place) const;
	f32  distanceTo(const vec3& rhs) const;
	vec3 compOn(const vec3& rhs) const;
	f32  projectOn(const vec3& rhs) const;
	vec3 midpoint(const vec3& rhs) const;
	f32  angleBetween(const vec3& rhs) const;
	vec3 xComp() const;
	vec3 yComp() const;
	vec3 zComp() const;
	vec3 xZero() const;
	vec3 yZero() const;
	vec3 zZero() const;
	vec3 xInvert() const;
	vec3 yInvert() const;
	vec3 zInvert() const;
	
	//vector interactions
	vec3(const vec2& v);
	vec3(const vec4& v);
	vec2 toVec2() const;
	vec4 toVec4() const;
	
	//matrix u32eractions
	vec3 operator* (const mat3& rhs) const;
	void operator*=(const mat3& rhs);
	vec3 operator* (const mat4& rhs) const;
	void operator*=(const mat4& rhs);
	//matN ToM1x3() const;
	//matN ToM1x4(f32 w) const;
	
	//quaternion interactions
	vec3 operator* (const quat& rhs) const;
};
#include "vec3.inl"

struct vec4{
	union{
		f32 arr[4] = {};
		struct{ 
			union{
				vec3 xyz;
				struct{ f32 x, y, z; };
			};
			f32 w;
		};
		struct{ 
			union{
				vec3 rgb;
				struct{ f32 r, g, b; };
			};
			f32 a;
		};
		struct{ 
			vec2 xy;
			f32 _unusedZ0;
			f32 _unusedW0;
		};
		struct{ 
			f32 _unusedX0;
			vec2 yz;
			f32 _unusedW1;
		};
		struct{ 
			f32 _unusedX1;
			f32 _unusedY0;
			vec2 zw;
		};
#ifdef DESHI_USE_SSE
		__m128 sse;
#endif
	};
	
	vec4(){};
	vec4(f32 inX, f32 inY, f32 inZ, f32 inW);
	vec4(const vec4& v);
	vec4(f32* ptr);
	
	static const vec4 ZERO;
	static const vec4 ONE;
	
	void operator= (const vec4& rhs);
	vec4 operator* (const f32& rhs) const;
	void operator*=(const f32& rhs);
	vec4 operator/ (const f32& rhs) const;
	void operator/=(const f32& rhs);
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
	friend vec4 operator* (const f32& lhs, const vec4& rhs){ return rhs * lhs; }
	
	void set(f32 x, f32 y, f32 z, f32 w);
	vec4 absV() const;
	vec4 copy() const;
	f32  dot(const vec4& rhs) const;
	f32  magSq() const;
	f32  mag() const;
	vec4 wnormalized() const;
	vec4 xComp() const;
	vec4 yComp() const;
	vec4 zComp() const;
	vec4 wComp() const;
	vec4 xInvert() const;
	vec4 yInvert() const;
	vec4 zInvert() const;
	vec4 wInvert() const;
	
	//vector interactions
	vec4(const vec2& v, f32 z, f32 w);
	vec4(const vec3& v, f32 w);
	vec3 toVec3() const;
	void takeVec3(const vec3& v);
	
	//matrix u32eractions
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
vec4(const vec2& v, f32 inZ, f32 inW) {
	x = v.x; y = v.y; z = inZ; w = inW;
}

inline vec4::
vec4(const vec3& v, f32 inW) {
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
persist s32 iter = 0; \
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


template<> FORCE_INLINE vec2 Min(vec2 a, vec2 b) { return vec2(Min(a.x, b.x), Min(a.y, b.y));}
template<> FORCE_INLINE vec2 Max(vec2 a, vec2 b) { return vec2(Max(a.x, b.x), Max(a.y, b.y)); }
template<> FORCE_INLINE vec2 Clamp(vec2 value, vec2 min, vec2 max) { return vec2(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y)); };
template<> FORCE_INLINE vec2 ClampMin(vec2 value, vec2 min) { return vec2(ClampMin(value.x, min.x), ClampMin(value.y, min.y)); };
template<> FORCE_INLINE vec2 ClampMax(vec2 value,  vec2 max) { return vec2(ClampMax(value.x, max.x), ClampMax(value.y, max.y)); };
template<> FORCE_INLINE vec3 Min(vec3 a, vec3 b) { return vec3(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z)); }
template<> FORCE_INLINE vec3 Max(vec3 a, vec3 b) { return vec3(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z)); }
template<> FORCE_INLINE vec3 Clamp(vec3 value, vec3 min, vec3 max) { return vec3(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y), Clamp(value.z, min.z, max.z)); };
template<> FORCE_INLINE vec3 ClampMin(vec3 value, vec3 min) { return vec3(ClampMin(value.x, min.x), ClampMin(value.y, min.y), ClampMin(value.z, min.z)); };
template<> FORCE_INLINE vec3 ClampMax(vec3 value, vec3 max) { return vec3(ClampMax(value.x, max.x), ClampMax(value.y, max.y), ClampMax(value.z, max.z)); };
template<> FORCE_INLINE vec4 Min(vec4 a, vec4 b) { return vec4(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z), Min(a.w, b.w)); }
template<> FORCE_INLINE vec4 Max(vec4 a, vec4 b) { return vec4(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z), Max(a.w, b.w)); }
template<> FORCE_INLINE vec4 Clamp(vec4 value, vec4 min, vec4 max) { return vec4(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y), Clamp(value.z, min.z, max.z), Clamp(value.w, min.w, max.w)); };
template<> FORCE_INLINE vec4 ClampMin(vec4 value, vec4 min) { return vec4(ClampMin(value.x, min.x), ClampMin(value.y, min.y), ClampMin(value.z, min.z), ClampMin(value.w, min.w)); };
template<> FORCE_INLINE vec4 ClampMax(vec4 value, vec4 max) { return vec4(ClampMax(value.x, max.x), ClampMax(value.y, max.y), ClampMax(value.z, max.z), ClampMax(value.w, max.w)); };
#endif //DESHI_VECTOR_H