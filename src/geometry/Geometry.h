#pragma once
#include "../math/math.h"
#include "../core/model.h"

namespace Geometry {
	
	//returns the point that's furthest along a normal 
	static u32 FurthestPointAlongNormal(std::vector<vec3>& p, vec3 n) {
		float furthest = -INFINITY;
		u32 furthestid = 0;
		for (int i = 0; i < p.size(); i++) {
			float dist = n.dot(p[i]);
			if (dist > furthest) {
				furthest = dist;
				furthestid = i;
			}
		}
		return furthestid;
	}
	
	//incase i forget
	//this function is set up to work specifically with convex meshes, because it literally 
	//just finds the triangle with the closest normal which we can safely assume is farthest along it, 
	//this would not work with concave meshes
	static u32 FurthestTriangleAlongNormal(Mesh* m, mat4 rotation, vec3 n) {
		float furthest = -INFINITY;
		u32 furthestTriId = 0;
		for (int i = 0; i < m->triangleCount; i++) {
			vec3 norm = m->triangleArray[i].normal * rotation;
			
			float dp = norm.dot(n);
			
			if (dp > furthest) {
				furthestTriId = i;
				furthest = dp;
			}
		}
		
		return furthestTriId;
	}
	
	
	static vec3 ClosestPointOnAABB(vec3 center, vec3 halfDims, vec3 target) {
		return vec3(fmaxf(center.x - halfDims.x, fminf(target.x, center.x + halfDims.x)),
					fmaxf(center.y - halfDims.y, fminf(target.y, center.y + halfDims.y)),
					fmaxf(center.y - halfDims.z, fminf(target.z, center.z + halfDims.z)));
	}
	
	static vec3 ClosestPointOnSphere(vec3 center, float radius, vec3 target) {
		return (target - center).normalized() * radius;
	}
	
	inline static vec3 MeshTriangleMidpoint(Mesh::Triangle* tri){
		return (tri->p[0] + tri->p[1] + tri->p[2]) / 3.f;
	}
};