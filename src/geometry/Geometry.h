#pragma once
#include "../math/Math.h"

namespace Geometry {

	static std::vector<Vector2> GenerateOutlinePoints(Mesh* m, Matrix4 transform, Matrix4 proj, Matrix4 view, Vector2 windimen) {

		std::vector<Vector2> outline;
		
		std::vector<Triangle> nonculled;
		for (Triangle* t : m->triangles) {
			t->removed = false;
			if (t->norm().yInvert().dot(g_admin->mainCamera->position - t->p[0] * transform) > 0) {
				Triangle tri = *t;
				tri.p[0] = Math::WorldToScreen(t->p[0] * transform, proj, view, windimen);
				tri.p[1] = Math::WorldToScreen(t->p[1] * transform, proj, view, windimen);
				tri.p[2] = Math::WorldToScreen(t->p[2] * transform, proj, view, windimen);

				nonculled.push_back(tri);
			} else t->removed = true;
		}

		for (Triangle t : nonculled) {
			for (int i = 0; i < t.nbrs.size(); i++) {
				if (t.nbrs[i]->removed) {
					outline.push_back(t.p[t.sharededge[i]].ToVector2());
					outline.push_back(t.p[(t.sharededge[i] + 1) % 3].ToVector2());
				}
			}
		}
		return outline;
	}


	static Vector3 ClosestPointOnAABB(Vector3 center, Vector3 halfDims, Vector3 target) {
		return Vector3(
			fmaxf(center.x - halfDims.x, fminf(target.x, center.x + halfDims.x)),
			fmaxf(center.y - halfDims.y, fminf(target.y, center.y + halfDims.y)),
			fmaxf(center.y - halfDims.z, fminf(target.z, center.z + halfDims.z)));
	}

	static Vector3 ClosestPointOnSphere(Vector3 center, float radius, Vector3 target) {
		return (target - center).normalized() * radius;
	}

	static Vector3 ClosestPointOnBox(Vector3 center, Vector3 halfDims, Vector3 rotation, Vector3 target) {
		target *= Matrix4::RotationMatrixAroundPoint(center, rotation).Inverse(); //TODO(delle,Geo) test ClosestPointOnBox
		return Vector3(
			fmaxf(center.x - halfDims.x, fminf(target.x, center.x + halfDims.x)),
			fmaxf(center.y - halfDims.y, fminf(target.y, center.y + halfDims.y)),
			fmaxf(center.y - halfDims.z, fminf(target.z, center.z + halfDims.z)));
	}

};