#pragma once
#ifndef DESHI_VEC3_INL
#define DESHI_VEC3_INL

//////////////////////
//// constructors ////
//////////////////////

inline vec3::
vec3(float inX, float inY, float inZ) {
	this->x = inX; this->y = inY; this->z = inZ;
}

inline vec3::
vec3(float inX, float inY) {
	this->x = inX; this->y = inY; this->z = 0;
}

inline vec3::
vec3(const vec3& v) {
	this->x = v.x; this->y = v.y; this->z = v.z;
}

inline vec3::
vec3(float* ptr){ 
	x = *ptr; y = *(ptr+1); z = *(ptr+2);
}

///////////////////
//// constants ////
///////////////////

inline const vec3 vec3::ZERO =    vec3( 0, 0, 0);
inline const vec3 vec3::ONE =     vec3( 1, 1, 1);
inline const vec3 vec3::RIGHT =   vec3( 1, 0, 0);
inline const vec3 vec3::LEFT =    vec3(-1, 0, 0);
inline const vec3 vec3::UP =      vec3( 0, 1, 0);
inline const vec3 vec3::DOWN =    vec3( 0,-1, 0);
inline const vec3 vec3::FORWARD = vec3( 0, 0, 1);
inline const vec3 vec3::BACK =    vec3( 0, 0,-1);
inline const vec3 vec3::UNITX =   vec3( 1, 0, 0);
inline const vec3 vec3::UNITY =   vec3( 0, 1, 0);
inline const vec3 vec3::UNITZ =   vec3( 0, 0, 1);

///////////////////
//// operators ////
///////////////////

inline void vec3::
operator =  (const vec3& rhs) {
	this->x = rhs.x; this->y = rhs.y; this->z = rhs.z;
}

inline void vec3::
operator =  (vec3& rhs) {
	this->x = rhs.x; this->y = rhs.y; this->z = rhs.z;
}

inline vec3 vec3::
operator *  (float rhs) const{
	return vec3(this->x * rhs, this->y * rhs, this->z * rhs);
}

inline void vec3::
operator *= (float rhs) {
	this->x *= rhs; this->y *= rhs; this->z *= rhs;
}

inline vec3 vec3::operator /  (float rhs) const{
	return vec3(this->x / rhs, this->y / rhs, this->z / rhs);
}

inline void vec3::
operator /= (float rhs) {
	this->x /= rhs; this->y /= rhs; this->z /= rhs;
}

inline vec3 vec3::
operator +  (const vec3& rhs) const{
	return vec3(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z);
}

inline void vec3::
operator += (const vec3& rhs) {
	this->x += rhs.x; this->y += rhs.y; this->z += rhs.z;
}

inline vec3 vec3::
operator -  (const vec3& rhs) const{
	return vec3(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z);
}

inline void vec3::
operator -= (const vec3& rhs) {
	this->x -= rhs.x; this->y -= rhs.y; this->z -= rhs.z;
}

inline vec3 vec3::
operator *  (const vec3& rhs) const{
	return vec3(this->x * rhs.x, this->y * rhs.y, this->z * rhs.z);
}

inline void vec3::
operator *= (const vec3& rhs) {
	this->x *= rhs.x; this->y *= rhs.y; this->z *= rhs.z;
}

inline vec3 vec3::
operator /  (const vec3& rhs) const{
	return vec3(this->x / rhs.x, this->y / rhs.y, this->z / rhs.z);
}

inline void vec3::
operator /= (const vec3& rhs) {
	this->x /= rhs.x; this->y /= rhs.y; this->z /= rhs.z;
}

inline vec3 vec3::
operator -  () const{
	return vec3( -x, -y, -z );
}

inline bool vec3::
operator == (const vec3& rhs) const{
	return abs(this->x - rhs.x) < VEC_EPSILON 
		&& abs(this->y - rhs.y) < VEC_EPSILON 
		&& abs(this->z - rhs.z) < VEC_EPSILON;
}

inline bool vec3::
operator != (const vec3& rhs) const{
	return !(*this == rhs);
}

///////////////////
//// functions ////
///////////////////
inline void vec3::
set(f32 _x, f32 _y, f32 _z){
	x = _x; y = _y; z = _z;
}

inline vec3 vec3::
absV() const{
	return vec3(abs(x), abs(y), abs(z));
}

inline vec3 vec3::
copy() const{
	return vec3(x, y, z);
}

inline float vec3::
dot(const vec3& rhs) const{
	return this->x*rhs.x + this->y*rhs.y + this->z*rhs.z;
}

//left hand cross product
inline vec3 vec3::
cross(const vec3& rhs) const{
	return vec3(this->y*rhs.z - this->z*rhs.y, this->z*rhs.x - this->x*rhs.z, this->x*rhs.y - this->y*rhs.x);
}

inline float vec3::
mag() const{
	return sqrt(x*x + y*y + z*z);
}

////ref: https://betterexplained.com/articles/understanding-quakes-fast-inverse-square-root/
//inline float vec3::
//mag() const{
//	Assert(CHAR_BIT*sizeof(float) == 32 && CHAR_BIT*sizeof(int32) == 32, "This mag method only works if float and int are 32bit");
//	float k = x * x + y * y + z * z;
//	float kHalf = .5f * k;
//	int32 i = *(int32*)&k;
//	i = 0x5f3759df - (i >> 1);
//	k = *(float*)&i;
//	k = k*(1.5f - kHalf*k*k);
//	return 1.f / k;
//}

inline float vec3::
magSq() const{
	return x*x + y*y + z*z;
}

inline void vec3::
normalize(){
	if (*this != vec3::ZERO) {
		*this /= this->mag();
	}
}

//inline void vec3::
//normalize() {
//	if (*this != vec3(0, 0, 0)) {
//		Assert(CHAR_BIT*sizeof(float) == 32 && CHAR_BIT*sizeof(int32) == 32, "This mag method only works if float and int are 32bit");
//		float k = x * x + y * y + z * z;
//		float kHalf = .5f * k;
//		int32 i = *(int32*)&k;
//		i = 0x5f3759df - (i >> 1);
//		k = *(float*)&i;
//		k = k*(1.5f - kHalf*k*k);
//		*this *= k;
//	}
//}

inline vec3 vec3::
normalized() const{
	if (*this != vec3::ZERO) {
		return *this / this->mag();
	}
	return *this;
}

//inline vec3 vec3::
//normalized() const{
//	if (*this != vec3(0, 0, 0)) {
//		Assert(CHAR_BIT*sizeof(float) == 32 && CHAR_BIT*sizeof(int32) == 32, "This mag method only works if float and int are 32bit");
//		float k = x * x + y * y + z * z;
//		float kHalf = .5f * k;
//		int32 i = *(int32*)&k;
//		i = 0x5f3759df - (i >> 1);
//		k = *(float*)&i;
//		k = k*(1.5f - kHalf*k*k);
//		return *this * k;
//	}
//	return vec3(*this);
//}

//clamps all values of a vec3 between two floats
inline vec3 vec3::
clamp(float lo, float hi) const{
	if(lo > hi) float temp = lo; lo = hi; hi = lo;
	return vec3((x < lo) ? lo : (hi < x) ? hi : x,
				(y < lo) ? lo : (hi < y) ? hi : y,
				(z < lo) ? lo : (hi < z) ? hi : z);
}

inline void vec3::
clampMag(float min, float max){
	float mag = this->mag();
	if (mag < min) {
		this->normalize();
		*this *= min; 
	} else if(mag > max){ 
		this->normalize();
		*this *= max; 
	}
}

inline vec3 vec3::
clampedMag(float min, float max) const{
	float mag = this->mag();
	if (mag < min) {
		return normalized() * min; 
	} else if(mag > max){ 
		return normalized() * max; 
	} else {
		return vec3(this->x, this->y, this->z);
	}
}

//round to a decimal place
inline void vec3::
round(int place){
	x = floor(x * place * 10 + 0.5) / (place * 10);
	y = floor(y * place * 10 + 0.5) / (place * 10);
	z = floor(z * place * 10 + 0.5) / (place * 10);
}

//round to a decimal place
inline vec3 vec3::
rounded(int place) const{
	return vec3(floor(x * place * 10 + 0.5) / (place * 10),
				floor(y * place * 10 + 0.5) / (place * 10),
				floor(z * place * 10 + 0.5) / (place * 10));
}

inline float vec3::
distanceTo(const vec3& rhs) const{
	return (*this - rhs).mag();
}

inline vec3 vec3::
compOn(const vec3& rhs) const{
	return rhs.normalized() * this->projectOn(rhs);
}

inline float vec3::
projectOn(const vec3& rhs) const{
	if (this->mag() != 0) return this->dot(rhs) / rhs.mag();
	else return 0;
}

inline vec3 vec3::
midpoint(const vec3& rhs) const{
	return vec3((x+rhs.x)/2.f, (y+rhs.y)/2.f, (z+rhs.z)/2.f);
}

//returns angle in radians
inline float vec3::
angleBetween(const vec3& rhs) const{
	float mags = this->mag()*rhs.mag();
	if(mags == 0) return 0;
	return acosf(this->dot(rhs) / mags);
}

inline vec3 vec3::
xComp() const{
	return vec3(x, 0, 0);
}

inline vec3 vec3::
yComp() const{
	return vec3(0, y, 0);
}

inline vec3 vec3::
zComp() const{
	return vec3(0, 0, z);
}

inline vec3 vec3::
xZero() const{
	return vec3(0, x, y);
}

inline vec3 vec3::
yZero() const{
	return vec3(x, 0, z);
}

inline vec3 vec3::
zZero() const{
	return vec3(x, y, 0);
}

inline vec3 vec3::
xInvert() const{
	return vec3(-x, y, z);
}

inline vec3 vec3::
yInvert() const{
	return vec3(x, -y, z);
}

inline vec3 vec3::
zInvert() const{
	return vec3(x, y, -z);
}

#endif //DESHI_VEC3_INL