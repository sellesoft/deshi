#pragma once
#ifndef DESHI_VEC4_INL
#define DESHI_VEC4_INL

//////////////////////
//// constructors ////
//////////////////////

inline vec4::
vec4(f32 _x, f32 _y, f32 _z, f32 _w) {
#if DESHI_USE_SSE
	sse = _mm_setr_ps(_x, _y, _z, _w);
#else
	x = _x; 
	y = _y; 
	z = _z; 
	w = _w;
#endif
}

inline vec4::
vec4(const vec4& v) {
#if DESHI_USE_SSE
	sse = v.sse;
#else
	x = v.x; 
	y = v.y; 
	z = v.z; 
	w = v.w;
#endif
}

//!TestMe
inline vec4::
vec4(f32* ptr){ 
#if DESHI_USE_SSE
	sse = _mm_loadr_ps(ptr);
#else
	x = *ptr; 
	y = *(ptr+1); 
	z = *(ptr+2); 
	z = *(ptr+3);
#endif
}

///////////////////
//// constants ////
///////////////////

inline const vec4 vec4::ZERO = vec4(0,0,0,0);
inline const vec4 vec4::ONE  = vec4(1,1,1,1);

///////////////////
//// operators ////
///////////////////

inline void vec4::
operator =  (const vec4& rhs) {
#if DESHI_USE_SSE
	sse = rhs.sse;
#else
	x = rhs.x; 
	y = rhs.y; 
	z = rhs.z; 
	w = rhs.w;
#endif
}

inline vec4 vec4::
operator *  (const f32& rhs) const {
	vec4 result;
#if DESHI_USE_SSE
	__m128 scalar = _mm_set1_ps(rhs);
	result.sse = _mm_mul_ps(sse, scalar);
#else
	result.x = x * rhs;
	result.y = y * rhs;
	result.z = z * rhs;
	result.w = w * rhs;
#endif
	return result;
}

inline void vec4::
operator *= (const f32& rhs) {
#if DESHI_USE_SSE
	__m128 scalar = _mm_set1_ps(rhs);
	sse = _mm_mul_ps(sse, scalar);
#else
	x *= rhs;
	y *= rhs;
	z *= rhs;
	w *= rhs;
#endif
}

inline vec4 vec4::
operator /  (const f32& rhs) const {
	vec4 result;
#if DESHI_USE_SSE
	__m128 scalar = _mm_set1_ps(rhs);
	result.sse = _mm_div_ps(sse, scalar);
#else
	result.x = x * rhs;
	result.y = y * rhs;
	result.z = z * rhs;
	result.w = w * rhs;
#endif
	return result;
}

inline void vec4::
operator /= (const f32& rhs) {
#if DESHI_USE_SSE
	__m128 scalar = _mm_set1_ps(rhs);
	sse = _mm_div_ps(sse, scalar);
#else
	x /= rhs;
	y /= rhs;
	z /= rhs;
	w /= rhs;
#endif
}

inline vec4 vec4::
operator +  (const vec4& rhs) const {
	vec4 result;
#if DESHI_USE_SSE
	result.sse = _mm_add_ps(sse, rhs.sse);
#else
	result.x = x + rhs.x;
	result.y = y + rhs.y;
	result.z = z + rhs.z;
	result.w = w + rhs.w;
#endif
	return result;
}

inline void vec4::
operator += (const vec4& rhs) {
#if DESHI_USE_SSE
	sse = _mm_add_ps(sse, rhs.sse);
#else
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	w += rhs.w;
#endif
}

inline vec4 vec4::
operator -  (const vec4& rhs) const {
	vec4 result;
#if DESHI_USE_SSE
	result.sse = _mm_sub_ps(sse, rhs.sse);
#else
	result.x = x - rhs.x;
	result.y = y - rhs.y;
	result.z = z - rhs.z;
	result.w = w - rhs.w;
#endif
	return result;
}

inline void vec4::
operator -= (const vec4& rhs) {
#if DESHI_USE_SSE
	sse = _mm_sub_ps(sse, rhs.sse);
#else
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	w -= rhs.w;
#endif
}

inline vec4 vec4::
operator *  (const vec4& rhs) const {
	vec4 result;
#if DESHI_USE_SSE
	result.sse = _mm_mul_ps(sse, rhs.sse);
#else
	result.x = x * rhs.x;
	result.y = y * rhs.y;
	result.z = z * rhs.z;
	result.w = w * rhs.w;
#endif
	return result;
}

inline void vec4::
operator *= (const vec4& rhs) {
#if DESHI_USE_SSE
	sse = _mm_mul_ps(sse, rhs.sse);
#else
	x *= rhs.x;
	y *= rhs.y;
	z *= rhs.z;
	w *= rhs.w;
#endif
}

inline vec4 vec4::
operator /  (const vec4& rhs) const {
	vec4 result;
#if DESHI_USE_SSE
	result.sse = _mm_div_ps(sse, rhs.sse);
#else
	result.x = x / rhs.x;
	result.y = y / rhs.y;
	result.z = z / rhs.z;
	result.w = w / rhs.w;
#endif
	return result;
}

inline void vec4::
operator /= (const vec4& rhs) {
#if DESHI_USE_SSE
	sse = _mm_div_ps(sse, rhs.sse);
#else
	x /= rhs.x;
	y /= rhs.y;
	z /= rhs.z;
	w /= rhs.w;
#endif
}

inline vec4 vec4::
operator -  () const {
	vec4 result;
#if DESHI_USE_SSE
	result.sse = NegateSSE(sse);
#else
	result.x = x * -1.0f;
	result.y = y * -1.0f;
	result.z = z * -1.0f;
	result.w = w * -1.0f;
#endif
	return result;
}

inline bool vec4::
operator == (const vec4& rhs) const {
	bool result;
#if DESHI_USE_SSE
	result = EpsilonEqualSSE(sse, rhs.sse);
#else
	result = abs(x - rhs.x) < M_EPSILON 
		&&   abs(y - rhs.y) < M_EPSILON 
		&&   abs(z - rhs.z) < M_EPSILON 
		&&   abs(w - rhs.w) < M_EPSILON;
#endif
	return result;
}

inline bool vec4::
operator != (const vec4& rhs) const {
	return !(*this == rhs);
}

///////////////////
//// functions ////
///////////////////
inline void vec4::
set(f32 _x, f32 _y, f32 _z, f32 _w){
#if DESHI_USE_SSE
	sse = _mm_setr_ps(_x, _y, _z, _w);
#else
	x = _x; 
	y = _y; 
	z = _z; 
	w = _w;
#endif
}

inline vec4 vec4::
absV() const{
	vec4 result;
#if DESHI_USE_SSE
	result.sse = AbsoluteSSE(sse);
#else
	result.x = abs(x);
	result.y = abs(y);
	result.z = abs(z);
	result.w = abs(w);
#endif
	return result;
}


inline vec4 vec4::
copy() const {
	return *this;
}

inline f32 vec4::
dot(const vec4& rhs) const {
	f32 result;
#if DESHI_USE_SSE
	__m128 temp0 = _mm_mul_ps(sse, rhs.sse); //multiply together
	__m128 temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(2, 3, 0, 1));
	temp0 = _mm_add_ps(temp0, temp1); //add x, y with z, w
	temp1 = _mm_shuffle_ps(temp0, temp0, _MM_SHUFFLE(0, 1, 2, 3));
	temp0 = _mm_add_ps(temp0, temp1); //add x+z with y+w
	result = _mm_cvtss_f32(temp0);
#else
	result = x*rhs.x + y*rhs.y + z*rhs.z + w*rhs.w;
#endif
	return result;
}

inline f32 vec4::
magSq() const {
	return dot(*this);
}

inline f32 vec4::
mag() const {
	return Sqrt(magSq());
}

inline vec4 vec4::
wnormalized() const {
	if (w != 0) {
		return *this / w;
	}
	return vec4(*this);
}

inline vec4 vec4::
xComp() const {
	return vec4(x, 0, 0, 0);
}

inline vec4 vec4::
yComp() const {
	return vec4(0, y, 0, 0);
}

inline vec4 vec4::
zComp() const {
	return vec4(0, 0, z, 0);
}

inline vec4 vec4::
wComp() const {
	return vec4(0, 0, 0, w);
}

inline vec4 vec4::
xInvert() const {
	return vec4(-x, y, z, w);
}

inline vec4 vec4::
yInvert() const {
	return vec4(x, -y, z, w);
}

inline vec4 vec4::
zInvert() const {
	return vec4(x, y, -z, w);
}

inline vec4 vec4::
wInvert() const {
	return vec4(x, y, z,-w);
}

#endif //DESHI_VEC4_INL