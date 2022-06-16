#pragma once
#ifndef DESHI_VECTOR_H
#define DESHI_VECTOR_H

#include "math_utils.h"

struct vec2g; //graphing's vec2
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
union vec2i{
	s32 arr[2] = {};
	struct{ s32 x, y; };
	
	vec2i(){};
	vec2i(s32 inX, s32 inY);
	vec2i(const vec2i& v);
	vec2i(s32* ptr);
	vec2i(vec2 v);
	
	static const vec2i ZERO;
	static const vec2i ONE;
	static const vec2i UP;
	static const vec2i DOWN;
	static const vec2i LEFT;
	static const vec2i RIGHT;
	static const vec2i UNITX;
	static const vec2i UNITY;
	
	void  operator= (const vec2i& rhs);
	vec2i operator* (s32 rhs) const;
	void  operator*=(s32 rhs);
	vec2i operator/ (s32 rhs) const;
	void  operator/=(s32 rhs);
	vec2i operator+ (const vec2i& rhs) const;
	void  operator+=(const vec2i& rhs);
	vec2i operator- (const vec2i& rhs) const;
	void  operator-=(const vec2i& rhs);
	vec2i operator* (const vec2i& rhs) const;
	void  operator*=(const vec2i& rhs);
	vec2i operator/ (const vec2i& rhs) const;
	void  operator/=(const vec2i& rhs);
	vec2i operator- () const;
	bool  operator==(const vec2i& rhs) const;
	bool  operator!=(const vec2i& rhs) const;
	friend vec2i operator* (s32 lhs, const vec2i& rhs){ return rhs * lhs; }
	
	void  set(s32 x, s32 y);
	vec2i absV() const;
	vec2i copy() const;
	s32   dot(const vec2i& rhs) const;
	vec2i perp() const;
	s32   mag()  const;
	s32   magSq()  const;
	void  normalize();
	vec2i normalized() const;
	void  clampMag(s32 min, s32 max);
	vec2i clampedMag(s32 min, s32 max) const;
	s32   distanceTo(const vec2i& rhs)  const;
	vec2i compOn(const vec2i& rhs)      const;
	s32   projectOn(const vec2i& rhs)   const;
	vec2i midpoint(const vec2i& rhs)    const;
	vec2i xComp() const;
	vec2i yComp() const;
	vec2i xInvert() const;
	vec2i yInvert() const;
	vec2i xSet(s32 set) const; 
	vec2i ySet(s32 set) const; 
	vec2i xAdd(s32 set) const; 
	vec2i yAdd(s32 set) const; 
	vec2i ceil() const;
	vec2i floor() const;
	
	//vector interactions
	vec2i(const vec3& v);
	vec2i(const vec4& v);
	vec2i(const vec2g& v);
	vec3 toVec3() const;
	vec4 toVec4() const;
};

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
	vec2(vec2i v);
	
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
	
	void set(f32 x, f32 y);
	vec2 absV() const;
	vec2 copy() const;
	f32  dot(const vec2& rhs) const;
	vec2 perp() const;
	f32  mag()  const;
	f32  magSq()  const;
	void normalize();
	vec2 normalized() const;
	void clampMag(f32 min, f32 max);
	vec2 clampedMag(f32 min, f32 max) const;
	f32  distanceTo(const vec2& rhs)  const;
	vec2 compOn(const vec2& rhs)      const;
	f32  projectOn(const vec2& rhs)   const;
	vec2 midpoint(const vec2& rhs)    const;
	vec2 xComp() const;
	vec2 yComp() const;
	vec2 xInvert() const;
	vec2 yInvert() const;
	vec2 xSet(f32 set) const; 
	vec2 ySet(f32 set) const; 
	vec2 xAdd(f32 set) const; 
	vec2 yAdd(f32 set) const; 
	vec2 ceil() const;
	vec2 floor() const;
	
	//vector interactions
	vec2(const vec3& v);
	vec2(const vec4& v);
	vec2(const vec2g& v);
	vec3 toVec3() const;
	vec4 toVec4() const;
};
#include "vec2.inl"


union vec3i{
	s32 arr[3] = {};
	struct { s32 x, y, z; };
	struct { s32 r, g, b; };
	struct { vec2i xy; s32 _unusedZ0; };
	struct { s32 _unusedX0; vec2i yz; };
};

struct vec3 {
	union {
		f32 arr[3] = {};
		struct { f32 x, y, z; };
		struct { f32 r, g, b; };
		struct { vec2 xy; f32 _unusedZ0; };
		struct { f32 _unusedX0; vec2 yz; };
	};
	
	vec3(){};
	vec3(f32 inX, f32 inY, f32 inZ);
	vec3(f32 inX, f32 inY);
	vec3(const vec3& v);
	vec3(f32* ptr);
	vec3(vec3i v);
	
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
	vec3 ceil() const;
	vec3 floor() const;
	
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
	//matN ToM1x3() const;
	//matN ToM1x4(f32 w) const;
	
	//quaternion interactions
	vec3 operator* (const quat& rhs) const;
};
#include "vec3.inl"


union vec4i{
	s32 arr[4] = {};
	struct{ 
		union{
			vec3i xyz;
			struct{ s32 x, y, z; };
		};
		s32 w;
	};
	struct{ 
		union{
			vec3i rgb;
			struct{ s32 r, g, b; };
		};
		s32 a;
	};
	struct{ 
		vec2i xy;
		s32 _unusedZ0;
		s32 _unusedW0;
	};
	struct{ 
		s32 _unusedX0;
		vec2i yz;
		s32 _unusedW1;
	};
	struct{ 
		s32 _unusedX1;
		s32 _unusedY0;
		vec2i zw;
	};
#ifdef DESHI_USE_SSE
	__m128 sse;
#endif
};

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
	vec4(vec4i v);
	
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
	vec4 ceil() const;
	vec4 floor() const;
	
	//vector interactions
	vec4(const vec2& v, f32 z, f32 w);
	vec4(const vec3& v, f32 w);
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

#define randvec2(a) vec2(rand() % a + 1, rand() % a + 1)
#define randvec3(a) vec3(rand() % a + 1, rand() % a + 1, rand() % a + 1)
#define randvec4(a) vec4(rand() % a + 1, rand() % a + 1, rand() % a + 1, rand() % a + 1);


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

FORCE_INLINE vec2 floor(vec2 in) { return vec2(floor(in.x), floor(in.y)); }
FORCE_INLINE vec3 floor(vec3 in) { return vec3(floor(in.x), floor(in.y), floor(in.z)); }
FORCE_INLINE vec4 floor(vec4 in) { return vec4(floor(in.x), floor(in.y), floor(in.z), floor(in.w)); }
FORCE_INLINE vec2 ceil(vec2 in)  { return vec2(ceil(in.x), ceil(in.y)); }
FORCE_INLINE vec3 ceil(vec3 in)  { return vec3(ceil(in.x), ceil(in.y), ceil(in.z)); }
FORCE_INLINE vec4 ceil(vec4 in)  { return vec4(ceil(in.x), ceil(in.y), ceil(in.z), ceil(in.w)); }
FORCE_INLINE vec2 round(vec2 in) { return vec2(round(in.x), round(in.y)); }
FORCE_INLINE vec3 round(vec3 in) { return vec3(round(in.x), round(in.y), round(in.z)); }
FORCE_INLINE vec4 round(vec4 in) { return vec4(round(in.x), round(in.y), round(in.z), round(in.w)); }


template<> FORCE_INLINE vec2 Min(vec2 a, vec2 b)                        { return vec2(Min(a.x, b.x), Min(a.y, b.y));}
template<> FORCE_INLINE vec2 Max(vec2 a, vec2 b)                        { return vec2(Max(a.x, b.x), Max(a.y, b.y)); }
template<> FORCE_INLINE vec2 Clamp(vec2 value, vec2 min, vec2 max)      { return vec2(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y)); };
template<> FORCE_INLINE vec2 ClampMin(vec2 value, vec2 min)             { return vec2(ClampMin(value.x, min.x), ClampMin(value.y, min.y)); };
template<> FORCE_INLINE vec2 ClampMax(vec2 value,  vec2 max)            { return vec2(ClampMax(value.x, max.x), ClampMax(value.y, max.y)); };
template<> FORCE_INLINE vec2 Nudge(vec2 value, vec2 target, vec2 delta) { return vec2(Nudge(value.x, target.x, delta.x), Nudge(value.y, target.y, delta.y)); }
template<> FORCE_INLINE vec3 Min(vec3 a, vec3 b)                        { return vec3(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z)); }
template<> FORCE_INLINE vec3 Max(vec3 a, vec3 b)                        { return vec3(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z)); }
template<> FORCE_INLINE vec3 Clamp(vec3 value, vec3 min, vec3 max)      { return vec3(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y), Clamp(value.z, min.z, max.z)); };
template<> FORCE_INLINE vec3 ClampMin(vec3 value, vec3 min)             { return vec3(ClampMin(value.x, min.x), ClampMin(value.y, min.y), ClampMin(value.z, min.z)); };
template<> FORCE_INLINE vec3 ClampMax(vec3 value, vec3 max)             { return vec3(ClampMax(value.x, max.x), ClampMax(value.y, max.y), ClampMax(value.z, max.z)); };
template<> FORCE_INLINE vec3 Nudge(vec3 value, vec3 target, vec3 delta) { return vec3(Nudge(value.x, target.x, delta.x), Nudge(value.y, target.y, delta.y), Nudge(value.z, target.z, delta.z)); }
template<> FORCE_INLINE vec4 Min(vec4 a, vec4 b)                        { return vec4(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z), Min(a.w, b.w)); }
template<> FORCE_INLINE vec4 Max(vec4 a, vec4 b)                        { return vec4(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z), Max(a.w, b.w)); }
template<> FORCE_INLINE vec4 Clamp(vec4 value, vec4 min, vec4 max)      { return vec4(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y), Clamp(value.z, min.z, max.z), Clamp(value.w, min.w, max.w)); };
template<> FORCE_INLINE vec4 ClampMin(vec4 value, vec4 min)             { return vec4(ClampMin(value.x, min.x), ClampMin(value.y, min.y), ClampMin(value.z, min.z), ClampMin(value.w, min.w)); };
template<> FORCE_INLINE vec4 ClampMax(vec4 value, vec4 max)             { return vec4(ClampMax(value.x, max.x), ClampMax(value.y, max.y), ClampMax(value.z, max.z), ClampMax(value.w, max.w)); };
template<> FORCE_INLINE vec4 Nudge(vec4 value, vec4 target, vec4 delta) { return vec4(Nudge(value.x, target.x, delta.x), Nudge(value.y, target.y, delta.y), Nudge(value.z, target.z, delta.z), Nudge(value.w, target.w, delta.w)); }

/////////////////
//// hashing ////
/////////////////
#include "kigu/hash.h"

template<> 
struct hash<vec2>{
	inline size_t operator()(vec2 const& v) const{
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};

template<> 
struct hash<vec3>{
	inline size_t operator()(vec3 const& v) const{
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.z); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};

template<> 
struct hash<vec4>{
	inline size_t operator()(vec4 const& v) const{
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.z); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.w); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};

///////////////////
//// to_string ////
///////////////////
#include "kigu/string.h"

global string 
to_string(const vec2& x, bool trunc = true, Allocator* a = KIGU_STRING_ALLOCATOR){
	string s(a);
	if(trunc){
		s.count = snprintf(nullptr, 0, "(%g, %g)", x.x, x.y);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%g, %g)", x.x, x.y);
	}else{
		s.count = snprintf(nullptr, 0, "(%+f, %+f)", x.x, x.y);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%+f, %+f)", x.x, x.y);
	}
	return s;
}

global string 
to_string(const vec3& x, bool trunc = true, Allocator* a = KIGU_STRING_ALLOCATOR){
	string s(a);
	if(trunc){
		s.count = snprintf(nullptr, 0, "(%g, %g, %g)", x.x, x.y, x.z);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%g, %g, %g)", x.x, x.y, x.z);
	}else{
		s.count = snprintf(nullptr, 0, "(%+f, %+f, %+f)", x.x, x.y, x.z);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%+f, %+f, %+f)", x.x, x.y, x.z);
	}
	return s;
}

global string 
to_string(const vec4& x, bool trunc = true, Allocator* a = KIGU_STRING_ALLOCATOR){
	string s(a);
	if(trunc){
		s.count = snprintf(nullptr, 0, "(%g, %g, %g, %g)", x.x, x.y, x.z, x.w);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%g, %g, %g, %g)", x.x, x.y, x.z, x.w);
	}else{
		s.count = snprintf(nullptr, 0, "(%+f, %+f, %+f, %+f)", x.x, x.y, x.z, x.w);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "(%+f, %+f, %+f, %+f)", x.x, x.y, x.z, x.w);
	}
	return s;
}

#endif //DESHI_VECTOR_H