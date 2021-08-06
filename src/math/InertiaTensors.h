#pragma once
#include "../math/Math.h"

namespace InertiaTensors {
	static mat3 SolidSphere(float radius, float mass) {
		float value = .4f * mass * radius * radius;
		return mat3(value, 0, 0,
					   0, value, 0,
					   0, 0, value);
	}
	
	static mat3 HollowSphere(float radius, float mass) {
		float value = (2.f/3.f) * mass * radius * radius;
		return mat3(value, 0, 0,
					   0, value, 0,
					   0, 0, value);
	}
	
	static mat3 SolidEllipsoid(vec3 halfDimensions, float mass) {
		float oneFifthMass = .2f * mass;
		float aSqrd = halfDimensions.x * halfDimensions.x;
		float bSqrd = halfDimensions.y * halfDimensions.y;
		float cSqrd = halfDimensions.z * halfDimensions.z;
		return mat3(oneFifthMass* (bSqrd + cSqrd), 0, 0,
					   0, oneFifthMass* (bSqrd + cSqrd), 0,
					   0, 0, oneFifthMass* (bSqrd + cSqrd));
	}
	
	static mat3 SolidCuboid(float width, float height, float depth, float mass) {
		float oneTwelfthMass = (1.f/12.f) * mass;
		float wSqrd = width * width;
		float hSqrd = height * height;
		float dSqrd = depth * depth;
		return mat3(oneTwelfthMass* (hSqrd + dSqrd), 0, 0,
					   0, oneTwelfthMass* (wSqrd + dSqrd), 0,
					   0, 0, oneTwelfthMass* (wSqrd + hSqrd));
	}
	
	static mat3 SolidCylinder(float radius, float height, float mass) {
		float rSqrd = radius * radius;
		float value = (1.f/12.f) * mass * (3 * rSqrd + height * height);
		return mat3(value, 0, 0,
					   0, value, 0,
					   0, 0, mass*rSqrd/2.f);
	}
};