#pragma once
#ifndef DESHI_VECTOR3_INL
#define DESHI_VECTOR3_INL

//////////////////////
//// constructors ////
//////////////////////

inline Vector3::
Vector3(float inX, float inY, float inZ) {
	this->x = inX; this->y = inY; this->z = inZ;
}

inline Vector3::
Vector3(float inX, float inY) {
	this->x = inX; this->y = inY; this->z = 0;
}

inline Vector3::
Vector3(const Vector3& v) {
	this->x = v.x; this->y = v.y; this->z = v.z;
}

inline Vector3::
Vector3(float* ptr){ 
	memcpy(&x, ptr, 3*sizeof(float));
}

///////////////////
//// constants ////
///////////////////

inline const Vector3 Vector3::ZERO =    Vector3( 0, 0, 0);
inline const Vector3 Vector3::ONE =     Vector3( 1, 1, 1);
inline const Vector3 Vector3::RIGHT =   Vector3( 1, 0, 0);
inline const Vector3 Vector3::LEFT =    Vector3(-1, 0, 0);
inline const Vector3 Vector3::UP =      Vector3( 0, 1, 0);
inline const Vector3 Vector3::DOWN =    Vector3( 0,-1, 0);
inline const Vector3 Vector3::FORWARD = Vector3( 0, 0, 1);
inline const Vector3 Vector3::BACK =    Vector3( 0, 0,-1);
inline const Vector3 Vector3::UNITX =   Vector3( 1, 0, 0);
inline const Vector3 Vector3::UNITY =   Vector3( 0, 1, 0);
inline const Vector3 Vector3::UNITZ =   Vector3( 0, 0, 1);

///////////////////
//// operators ////
///////////////////

inline void Vector3::
operator =  (const Vector3& rhs) {
	this->x = rhs.x; this->y = rhs.y; this->z = rhs.z;
}

inline void Vector3::
operator =  (Vector3& rhs) {
	this->x = rhs.x; this->y = rhs.y; this->z = rhs.z;
}

inline Vector3 Vector3::
operator *  (float rhs) const {
	return Vector3(this->x * rhs, this->y * rhs, this->z * rhs);
}

inline void Vector3::
operator *= (float rhs) {
	this->x *= rhs; this->y *= rhs; this->z *= rhs;
}

inline Vector3 Vector3::operator /  (float rhs) const {
	return Vector3(this->x / rhs, this->y / rhs, this->z / rhs);
}

inline void Vector3::
operator /= (float rhs) {
	this->x /= rhs; this->y /= rhs; this->z /= rhs;
}

inline Vector3 Vector3::
operator +  (const Vector3& rhs) const {
	return Vector3(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z);
}

inline void Vector3::
operator += (const Vector3& rhs) {
	this->x += rhs.x; this->y += rhs.y; this->z += rhs.z;
}

inline Vector3 Vector3::
operator -  (const Vector3& rhs) const {
	return Vector3(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z);
}

inline void Vector3::
operator -= (const Vector3& rhs) {
	this->x -= rhs.x; this->y -= rhs.y; this->z -= rhs.z;
}

inline Vector3 Vector3::
operator *  (const Vector3& rhs) const {
	return Vector3(this->x * rhs.x, this->y * rhs.y, this->z * rhs.z);
}

inline void Vector3::
operator *= (const Vector3& rhs) {
	this->x *= rhs.x; this->y *= rhs.y; this->z *= rhs.z;
}

inline Vector3 Vector3::
operator /  (const Vector3& rhs) const {
	return Vector3(this->x / rhs.x, this->y / rhs.y, this->z / rhs.z);
}

inline void Vector3::
operator /= (const Vector3& rhs) {
	this->x /= rhs.x; this->y /= rhs.y; this->z /= rhs.z;
}

inline Vector3 Vector3::
operator -  () const {
	return Vector3( -x, -y, -z );
}

//floating point accuracy in our vectors is .001 :)
inline bool Vector3::
operator == (const Vector3& rhs) const {
	return abs(this->x - rhs.x) < .001f && abs(this->y - rhs.y) < .001f && abs(this->z - rhs.z) < .001f;
	//return this->y == rhs.y  && this->y == rhs.y && this->z == rhs.z;
}

inline bool Vector3::
operator != (const Vector3& rhs) const {
	return !(*this == rhs);
}

///////////////////
//// functions ////
///////////////////

inline Vector3 Vector3::
absV() const{
	return Vector3(abs(x), abs(y), abs(z));
}

inline Vector3 Vector3::
copy() const {
	return Vector3(x, y, z);
}

inline float Vector3::
dot(const Vector3& rhs) const {
	return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z;
}

//left hand cross product
inline Vector3 Vector3::
cross(const Vector3& rhs) const {
	return Vector3(this->y * rhs.z - rhs.y * this->z, this->x * rhs.z - rhs.x * this->z, this->x * rhs.y - rhs.x * this->y);
}

inline float Vector3::
mag() const {
	return sqrt(x * x + y * y + z * z);
}

////ref: https://betterexplained.com/articles/understanding-quakes-fast-inverse-square-root/
//inline float Vector3::
//mag() const {
//	Assert(CHAR_BIT*sizeof(float) == 32 && CHAR_BIT*sizeof(int32) == 32, "This mag method only works if float and int are 32bit");
//	float k = x * x + y * y + z * z;
//	float kHalf = .5f * k;
//	int32 i = *(int32*)&k;
//	i = 0x5f3759df - (i >> 1);
//	k = *(float*)&i;
//	k = k*(1.5f - kHalf*k*k);
//	return 1.f / k;
//}

inline Vector3 Vector3::
normalize() {
	if (*this != Vector3(0, 0, 0)) {
		*this /= this->mag();
	}
	return *this;
}

//inline void Vector3::
//normalize() {
//	if (*this != Vector3(0, 0, 0)) {
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

inline Vector3 Vector3::
normalized() const {
	if (*this != Vector3(0, 0, 0)) {
		return *this / this->mag();
	}
	return *this;
}

//inline Vector3 Vector3::
//normalized() const {
//	if (*this != Vector3(0, 0, 0)) {
//		Assert(CHAR_BIT*sizeof(float) == 32 && CHAR_BIT*sizeof(int32) == 32, "This mag method only works if float and int are 32bit");
//		float k = x * x + y * y + z * z;
//		float kHalf = .5f * k;
//		int32 i = *(int32*)&k;
//		i = 0x5f3759df - (i >> 1);
//		k = *(float*)&i;
//		k = k*(1.5f - kHalf*k*k);
//		return *this * k;
//	}
//	return Vector3(*this);
//}

//clamps all values of a Vector3 between two floats
inline Vector3 Vector3::
clamp(float lo, float hi){
	if(lo > hi) float temp = lo; lo = hi; hi = lo;
	return Vector3((x < lo) ? lo : (hi < x) ? hi : x,
				   (y < lo) ? lo : (hi < y) ? hi : y,
				   (z < lo) ? lo : (hi < z) ? hi : z);
}

inline void Vector3::
clampMag(float min, float max) {
	float mag = this->mag();
	if (mag < min) {
		this->normalize();
		*this *= min; 
	} else if(mag > max){ 
		this->normalize();
		*this *= max; 
	}
}

inline Vector3 Vector3::
clampedMag(float min, float max) const {
	float mag = this->mag();
	if (mag < min) {
		return normalized() * min; 
	} else if(mag > max){ 
		return normalized() * max; 
	} else {
		return Vector3(this->x, this->y, this->z);
	}
}

//round to a decimal place
inline void Vector3::round(int place) {
	x = floor(x * place * 10 + 0.5) / (place * 10);
	y = floor(y * place * 10 + 0.5) / (place * 10);
	z = floor(z * place * 10 + 0.5) / (place * 10);
}

//round to a decimal place
inline Vector3 Vector3::rounded(int place) {
	return Vector3(
				   floor(x * place * 10 + 0.5) / (place * 10),
				   floor(y * place * 10 + 0.5) / (place * 10),
				   floor(z * place * 10 + 0.5) / (place * 10));
}

inline float Vector3::
distanceTo(const Vector3& rhs) const {
	return (*this - rhs).mag();
}

inline Vector3 Vector3::
compOn(Vector3 rhs) {
	return rhs.normalized() * this->projectOn(rhs);
}

inline float Vector3::
projectOn(Vector3 rhs) {
	if (this->mag() != 0) return this->dot(rhs) / rhs.mag();
	else return 0;
}

inline Vector3 Vector3::
midpoint(Vector3 rhs){
	return Vector3((x+rhs.x)/2.f, (y+rhs.y)/2.f, (z+rhs.z)/2.f);
}

inline Vector3 Vector3::
xComp() const {
	return Vector3(x, 0, 0);
}

inline Vector3 Vector3::
yComp() const {
	return Vector3(0, y, 0);
}

inline Vector3 Vector3::
zComp() const {
	return Vector3(0, 0, z);
}

inline Vector3 Vector3::
xInvert() const {
	return Vector3(-x, y, z);
}

inline Vector3 Vector3::
yInvert() const {
	return Vector3(x, -y, z);
}

inline Vector3 Vector3::
zInvert() const {
	return Vector3(x, y, -z);
}

inline const std::string Vector3::
str() const {
	return std::string("(") + std::to_string(this->x) + "," + std::to_string(this->y) + "," + std::to_string(this->z) + ")";
}

inline const std::string Vector3::
str2f() const {
	char buffer[50];
	std::snprintf(buffer, 50, "(%+.2f, %+.2f, %+.2f)", this->x, this->y, this->z);
	return std::string(buffer);
}

//////////////
//// hash ////
//////////////

//ref: glm::hash
namespace std{
	template<> struct hash<Vector3>{
		inline size_t operator()(Vector3 const& v) const{
			size_t seed = 0;
			hash<float> hasher; size_t hash;
			hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
			hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
			hash = hasher(v.z); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
			return seed;
		}
	};
};

#endif //DESHI_VECTOR3_INL