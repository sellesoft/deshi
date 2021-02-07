#include "Vector3.h"

//// Constructors ////

Vector3::Vector3() {
	this->x = 0; this->y = 0; this->z = 0;
}

Vector3::Vector3(float inX, float inY, float inZ) {
	this->x = inX; this->y = inY; this->z = inZ;
}

Vector3::Vector3(float inX, float inY) {
	this->x = inX; this->y = inY; this->z = 0;
}

Vector3::Vector3(const Vector3& v) {
	this->x = v.x; this->y = v.y; this->z = v.z;
}

//// Static Constants ////

const Vector3 Vector3::ZERO =		Vector3( 0, 0, 0);
const Vector3 Vector3::ONE =		Vector3( 1, 1, 1);
const Vector3 Vector3::RIGHT =		Vector3( 1, 0, 0);
const Vector3 Vector3::LEFT =		Vector3(-1, 0, 0);
const Vector3 Vector3::UP =			Vector3( 0, 1, 0);
const Vector3 Vector3::DOWN =		Vector3( 0,-1, 0);
const Vector3 Vector3::FORWARD =	Vector3( 0, 0, 1);
const Vector3 Vector3::BACK =		Vector3( 0, 0,-1);
const Vector3 Vector3::UNITX =		Vector3( 1, 0, 0);
const Vector3 Vector3::UNITY =		Vector3( 0, 1, 0);
const Vector3 Vector3::UNITZ =		Vector3( 0, 0, 1);
