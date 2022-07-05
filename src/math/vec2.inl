#pragma once
#ifndef DESHI_VEC2_INL
#define DESHI_VEC2_INL

//////////////////////
//// constructors ////
//////////////////////

inline vec2::
vec2(f32 inX, f32 inY) {DPZoneScoped; 
	x = inX; y = inY; 
}

inline vec2::
vec2(const vec2& v){DPZoneScoped; 
	x = v.x; y = v.y; 
}

inline vec2::
vec2(f32* ptr){DPZoneScoped; 
	x = *ptr; y = *(ptr+1);
}

inline vec2::
vec2(vec2i v){DPZoneScoped; 
	x = (f32)v.x; y = (f32)v.y;
}

///////////////////
//// constants ////
///////////////////

inline const vec2 vec2::ZERO =  vec2{ 0, 0};
inline const vec2 vec2::ONE =   vec2{ 1, 1};
inline const vec2 vec2::RIGHT = vec2{ 1, 0};
inline const vec2 vec2::LEFT =  vec2{-1, 0};
inline const vec2 vec2::UP =    vec2{ 0, 1};
inline const vec2 vec2::DOWN =  vec2{ 0,-1};
inline const vec2 vec2::UNITX = vec2{ 1, 0};
inline const vec2 vec2::UNITY = vec2{ 0, 1};

///////////////////
//// operators ////
///////////////////

inline void vec2::
operator=(const vec2& rhs) {DPZoneScoped;
	this->x = rhs.x; this->y = rhs.y;
}

inline vec2 vec2::
operator*(f32 rhs) const{DPZoneScoped;
	return vec2(this->x * rhs, this->y * rhs);
}

inline void vec2::
operator*=(f32 rhs) {DPZoneScoped;
	this->x *= rhs; this->y *= rhs;
}

inline vec2 vec2::
operator/(f32 rhs) const{DPZoneScoped;
	return vec2(this->x / rhs, this->y / rhs);
}

inline void vec2::
operator/=(f32 rhs) {DPZoneScoped;
	this->x /= rhs; this->y /= rhs;
}

inline vec2 vec2::
operator+(const vec2& rhs) const{DPZoneScoped;
	return vec2(this->x + rhs.x, this->y + rhs.y);
}

inline void vec2::
operator+=(const vec2& rhs) {DPZoneScoped;
	this->x += rhs.x; this->y += rhs.y;
}

inline vec2 vec2::
operator-(const vec2& rhs) const{DPZoneScoped;
	return vec2(this->x - rhs.x, this->y - rhs.y);
}

inline void vec2::
operator-=(const vec2& rhs) {DPZoneScoped;
	this->x -= rhs.x; this->y -= rhs.y;
}

inline vec2 vec2::
operator*(const vec2& rhs) const{DPZoneScoped;
	return vec2(this->x * rhs.x, this->y * rhs.y);
}

inline void vec2::
operator*=(const vec2& rhs) {DPZoneScoped;
	this->x *= rhs.x; this->y *= rhs.y;
}

inline vec2 vec2::
operator/(const vec2& rhs) const{DPZoneScoped;
	return vec2(this->x / rhs.x, this->y / rhs.y);
}

inline void vec2::
operator/=(const vec2& rhs) {DPZoneScoped;
	this->x /= rhs.x; this->y /= rhs.y;
}

inline vec2 vec2::
operator-() const{DPZoneScoped;
	return vec2(-x, -y);
}

inline bool vec2::
operator==(const vec2& rhs) const{DPZoneScoped;
	return fabs(this->x - rhs.x) < M_EPSILON 
		&& fabs(this->y - rhs.y) < M_EPSILON;
}

inline bool vec2::
operator!=(const vec2& rhs) const{DPZoneScoped;
	return !(*this == rhs);
}

///////////////////
//// functions ////
///////////////////
inline void vec2::
set(f32 _x, f32 _y){DPZoneScoped;
	x = _x; y = _y;
}

inline vec2 vec2::
absV() const{DPZoneScoped;
	return vec2(fabs(x), fabs(y));
}

inline vec2 vec2::
copy() const{DPZoneScoped;
	return vec2(x, y);
}

inline f32 vec2::
dot(const vec2& rhs) const{DPZoneScoped;
	return (this->x * rhs.x) + (this->y * rhs.y);
}

inline vec2 vec2::
perp() const{DPZoneScoped;
	return vec2(-y, x);
}

inline f32 vec2::
mag() const{DPZoneScoped;
	return sqrt(x * x + y * y);
}

inline f32 vec2::
magSq() const{DPZoneScoped;
	return x*x + y*y;
}

inline void vec2::
normalize() {DPZoneScoped;
	if (*this != vec2::ZERO) {DPZoneScoped;
		*this /= this->mag();
	}
}

inline vec2 vec2::
normalized() const{DPZoneScoped;
	if(*this != vec2::ZERO){DPZoneScoped;
		return *this / this->mag();
	}
	return *this;
}

inline void vec2::
clampMag(f32 min, f32 max) {DPZoneScoped;
	f32 mag = this->mag();
	if (mag < min) {DPZoneScoped;
		this->normalize();
		*this *= min;
	}
	else if (mag > max) {DPZoneScoped;
		this->normalize();
		*this *= max;
	}
}

inline vec2 vec2::
clampedMag(f32 min, f32 max) const{DPZoneScoped;
	f32 mag = this->mag();
	if (mag < min) {DPZoneScoped;
		return normalized() * min;
	}
	else if (mag > max) {DPZoneScoped;
		return normalized() * max;
	}
	else {
		return vec2(this->x, this->y);
	}
}

inline f32 vec2::
distanceTo(const vec2& rhs) const{DPZoneScoped;
	return (*this - rhs).mag();
}

inline vec2 vec2::
compOn(const vec2& rhs) const{DPZoneScoped;
	return rhs.normalized() * this->projectOn(rhs);
}

inline f32 vec2::
projectOn(const vec2& rhs) const{DPZoneScoped;
	if(this->mag() > M_EPSILON){DPZoneScoped;
		return this->dot(rhs) / this->mag();
	}else{
		return 0;
	}
}

inline vec2 vec2::
midpoint(const vec2& rhs) const{DPZoneScoped;
	return vec2((x+rhs.x)/2.f, (y+rhs.y)/2.f);
}

inline vec2 vec2::
xComp() const{DPZoneScoped;
	return vec2(x, 0);
}

inline vec2 vec2::
yComp() const{DPZoneScoped;
	return vec2(0, y);
}

inline vec2 vec2::
xInvert() const{DPZoneScoped;
	return vec2(-x, y);
}

inline vec2 vec2::
yInvert() const{DPZoneScoped;
	return vec2(x, -y);
}

inline vec2 vec2::
xSet(f32 set) const{DPZoneScoped;
	return vec2(set, y);
}

inline vec2 vec2::
ySet(f32 set) const{DPZoneScoped;
	return vec2(x, set);
}

inline vec2 vec2::
xAdd(f32 add) const{DPZoneScoped;
	return vec2(x + add, y);
}

inline vec2 vec2::
yAdd(f32 add) const{DPZoneScoped;
	return vec2(x, y + add);
}

inline vec2 vec2::
ceil() const{DPZoneScoped;
	return vec2(std::ceil(x), std::ceil(y));
};

inline vec2 vec2::
floor() const{DPZoneScoped;
	return vec2(std::floor(x), std::floor(y));
};

#endif //DESHI_VEC2_INL