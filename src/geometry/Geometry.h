#pragma once
#include "../math/Math.h"

namespace Geometry {

	Vector3 ClosestPointOnAABB(Vector3 center, Vector3 halfDims, Vector3 target) {
		return Vector3(
			fmaxf(center.x - halfDims.x, fminf(target.x, center.x + halfDims.x)),
			fmaxf(center.y - halfDims.y, fminf(target.y, center.y + halfDims.y)),
			fmaxf(center.y - halfDims.z, fminf(target.z, center.z + halfDims.z)));
	}

	Vector3 ClosestPointOnSphere(Vector3 center, float radius, Vector3 target) {
		return (target - center).normalized() * radius;
	}

	Vector3 ClosestPointOnBox(Vector3 center, Vector3 halfDims, Vector3 rotation, Vector3 target) {
		target *= Matrix4::RotationMatrixAroundPoint(center, rotation).Inverse(); //TODO(delle,Geo) test ClosestPointOnBox
		return Vector3(
			fmaxf(center.x - halfDims.x, fminf(target.x, center.x + halfDims.x)),
			fmaxf(center.y - halfDims.y, fminf(target.y, center.y + halfDims.y)),
			fmaxf(center.y - halfDims.z, fminf(target.z, center.z + halfDims.z)));
	}

};