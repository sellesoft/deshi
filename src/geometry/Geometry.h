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
	
	inline static vec3 ClosestPointOnPlane(vec3 plane_point, vec3 plane_normal, vec3 target){
		return target + (plane_normal * -Math::DistPointToPlane(target, plane_normal, plane_point));
	}
	
	inline static vec3 ClosestPointOnAABB(vec3 center, vec3 halfDims, vec3 target) {
		return vec3(fmaxf(center.x - halfDims.x, fminf(target.x, center.x + halfDims.x)),
					fmaxf(center.y - halfDims.y, fminf(target.y, center.y + halfDims.y)),
					fmaxf(center.y - halfDims.z, fminf(target.z, center.z + halfDims.z)));
	}
	
	inline static vec3 ClosestPointOnSphere(vec3 center, float radius, vec3 target) {
		return (target - center).normalized() * radius;
	}
	
	static vec3 ClosestPointOnConvexMesh(Mesh* mesh, vec3 position, vec3 rotation, vec3 scale, vec3 target){
		vec3 target_normal = (target - position).normalized();
		mat3 rotation_matrix = mat3::RotationMatrix(rotation);
		f32  furthest_dot = -INFINITY;
		vec3 plane_normal{};
		
		MeshFace* closest_face = 0;
		forE(mesh->faces){
			vec3 face_normal = it->normal * rotation_matrix;
			f32  local_dot = target_normal.dot(face_normal);
			if(local_dot > furthest_dot){
				furthest_dot = local_dot;
				closest_face = it;
				plane_normal = face_normal;
			}
		}
		
		Assert(closest_face);
		vec3 plane_point = mesh->vertexes[closest_face->vertexes[0]].pos * mat4::TransformationMatrix(position, rotation, scale);
		return ClosestPointOnPlane(plane_point, plane_normal, target);
	}
	
	inline static vec3 MeshTriangleMidpoint(Mesh::Triangle* tri){
		return (tri->p[0] + tri->p[1] + tri->p[2]) / 3.f;
	}
};