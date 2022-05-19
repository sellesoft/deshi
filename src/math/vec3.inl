#pragma once
#ifndef DESHI_VEC3_INL
#define DESHI_VEC3_INL

//////////////////////
//// constructors ////
//////////////////////

inline vec3::
vec3(f32 inX, f32 inY, f32 inZ) {
	this->x = inX; this->y = inY; this->z = inZ;
}

inline vec3::
vec3(f32 inX, f32 inY) {
	this->x = inX; this->y = inY; this->z = 0;
}

inline vec3::
vec3(const vec3& v) {
	this->x = v.x; this->y = v.y; this->z = v.z;
}

inline vec3::
vec3(f32* ptr){ 
	x = *ptr; y = *(ptr+1); z = *(ptr+2);
}

inline vec3::
vec3(vec3i v){ 
	x = (f32)v.x; y = (f32)v.y; z = (f32)v.z;
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
operator *  (f32 rhs) const{
	return vec3(this->x * rhs, this->y * rhs, this->z * rhs);
}

inline void vec3::
operator *= (f32 rhs) {
	this->x *= rhs; this->y *= rhs; this->z *= rhs;
}

inline vec3 vec3::operator /  (f32 rhs) const{
	return vec3(this->x / rhs, this->y / rhs, this->z / rhs);
}

inline void vec3::
operator /= (f32 rhs) {
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
	return abs(this->x - rhs.x) < M_EPSILON 
		&& abs(this->y - rhs.y) < M_EPSILON 
		&& abs(this->z - rhs.z) < M_EPSILON;
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

inline f32 vec3::
dot(const vec3& rhs) const{
	return this->x*rhs.x + this->y*rhs.y + this->z*rhs.z;
}

//left hand cross product
inline vec3 vec3::
cross(const vec3& rhs) const{
	return vec3(this->y*rhs.z - this->z*rhs.y, this->z*rhs.x - this->x*rhs.z, this->x*rhs.y - this->y*rhs.x);
}

inline f32 vec3::
mag() const{
	return sqrt(x*x + y*y + z*z);
}

////ref: https://betterexplained.com/articles/understanding-quakes-fast-inverse-square-root/
//inline f32 vec3::
//mag() const{
//	Assert(CHAR_BIT*sizeof(f32) == 32 && CHAR_BIT*sizeof(s32) == 32, "This mag method only works if f32 and s32 are 32bit");
//	f32 k = x * x + y * y + z * z;
//	f32 kHalf = .5f * k;
//	s32 i = *(s32*)&k;
//	i = 0x5f3759df - (i >> 1);
//	k = *(f32*)&i;
//	k = k*(1.5f - kHalf*k*k);
//	return 1.f / k;
//}

inline f32 vec3::
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
//		Assert(CHAR_BIT*sizeof(f32) == 32 && CHAR_BIT*sizeof(s32) == 32, "This mag method only works if f32 and s32 are 32bit");
//		f32 k = x * x + y * y + z * z;
//		f32 kHalf = .5f * k;
//		s32 i = *(s32*)&k;
//		i = 0x5f3759df - (i >> 1);
//		k = *(f32*)&i;
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
//		Assert(CHAR_BIT*sizeof(f32) == 32 && CHAR_BIT*sizeof(s32) == 32, "This mag method only works if f32 and s32 are 32bit");
//		f32 k = x * x + y * y + z * z;
//		f32 kHalf = .5f * k;
//		s32 i = *(s32*)&k;
//		i = 0x5f3759df - (i >> 1);
//		k = *(f32*)&i;
//		k = k*(1.5f - kHalf*k*k);
//		return *this * k;
//	}
//	return vec3(*this);
//}

//clamps all values of a vec3 between two floats
inline vec3 vec3::
clamp(f32 lo, f32 hi) const{
	if(lo > hi) f32 temp = lo; lo = hi; hi = lo;
	return vec3((x < lo) ? lo : (hi < x) ? hi : x,
				(y < lo) ? lo : (hi < y) ? hi : y,
				(z < lo) ? lo : (hi < z) ? hi : z);
}

inline void vec3::
clampMag(f32 min, f32 max){
	f32 mag = this->mag();
	if (mag < min) {
		this->normalize();
		*this *= min; 
	} else if(mag > max){ 
		this->normalize();
		*this *= max; 
	}
}

inline vec3 vec3::
clampedMag(f32 min, f32 max) const{
	f32 mag = this->mag();
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
round(s32 place){
	x = floorf(x * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f);
	y = floorf(y * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f);
	z = floorf(z * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f);
}

//round to a decimal place
inline vec3 vec3::
rounded(s32 place) const{
	return vec3(floorf(x * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f),
				floorf(y * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f),
				floorf(z * (f32)place * 10.f + 0.5f) / ((f32)place * 10.f));
}

inline f32 vec3::
distanceTo(const vec3& rhs) const{
	return (*this - rhs).mag();
}

inline vec3 vec3::
compOn(const vec3& rhs) const{
	return rhs.normalized() * this->projectOn(rhs);
}

inline f32 vec3::
projectOn(const vec3& rhs) const{
	if (this->mag() != 0) return this->dot(rhs) / rhs.mag();
	else return 0;
}

inline vec3 vec3::
midpoint(const vec3& rhs) const{
	return vec3((x+rhs.x)/2.f, (y+rhs.y)/2.f, (z+rhs.z)/2.f);
}

//returns angle in radians
inline f32 vec3::
angleBetween(const vec3& rhs) const{
	f32 mags = this->mag()*rhs.mag();
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

inline vec3 vec3::
ceil() const{
	return vec3(std::ceil(x), std::ceil(y), std::ceil(z));
};

inline vec3 vec3::
floor() const{
	return vec3(std::floor(x), std::floor(y), std::floor(z));
};

#endif //DESHI_VEC3_INL