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
	
	static MeshFace* FurthestConvexMeshFaceAlongNormal(Mesh* mesh, vec3 mesh_rotation, vec3 target_normal){
		mat3 rotation_matrix = mat3::RotationMatrix(mesh_rotation);
		f32  furthest_dot = -INFINITY;
		
		MeshFace* closest_face = 0;
		forE(mesh->faces){
			vec3 face_normal = it->normal * rotation_matrix;
			f32  local_dot = target_normal.dot(face_normal);
			if(local_dot > furthest_dot){
				furthest_dot = local_dot;
				closest_face = it;
			}
		}
		return closest_face;
	}
	
	//convert to barycentric coords, clamp the triangle, convert back to cartesian coords
	static vec3 ClampPointToTriangle(vec3 point, vec3 tri0, vec3 tri1, vec3 tri2){
		vec3 v0p = point - tri0;
		vec3 v01 = tri1 - tri0;
		vec3 v02 = tri2 - tri0;
		f32  dot00 = v01.dot(v01);
		f32  dot01 = v01.dot(v02);
		f32  dot11 = v02.dot(v02);
		f32  dot20 = v0p.dot(v01);
		f32  dot21 = v0p.dot(v02);
		f32  denom = dot00 * dot11 - dot01 * dot01;
		f32  v = (dot11 * dot20 - dot01 * dot21) / denom;
		f32  w = (dot00 * dot21 - dot01 * dot20) / denom;
		f32  u = 1.0f - v - w;
		if      (u < 0.0f){
			vec3 v12 = tri2 - tri1;
			f32  t = Math::clamp(v12.dot(point - tri1) / v12.dot(v12), 0.0f, 1.0f);
			u = 0.0f; v = 1.0f - t; w = t;
		}else if(v < 0.0f){
			vec3 v20 = tri0 - tri2;
			f32  t = Math::clamp(v20.dot(point - tri2) / v20.dot(v20), 0.0f, 1.0f);
			u = t; v = 0.0f; w = 1.0f - t;
		}else if(w < 0.0f){
			f32 t = Math::clamp(dot20 / dot00, 0.0f, 1.0f);
			u = 1.0f - t; v = t; w = 0.0f;
		}
		return vec3(u*tri0.x + v*tri1.x + w*tri2.x, u*tri0.y + v*tri1.y + w*tri2.y, u*tri0.z + v*tri1.z + w*tri2.z);
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
		//convert target to mesh space
		mat4 mesh_rotation = mat4::RotationMatrix(rotation);
		mat4 mesh_transform = mesh_rotation;
		mesh_transform.data[12] = position.x; mesh_transform.data[13] = position.y; mesh_transform.data[14] = position.z;
		mesh_transform.data[0] *= scale.x;    mesh_transform.data[5] *= scale.y;    mesh_transform.data[10] *= scale.z;
		target *= mesh_transform.Inverse();
		
		//find closest face to target based on the face normal
		vec3 target_normal = target.normalized();
		vec3 closest_normal = vec3::ZERO;
		f32  furthest_dot = -INFINITY;
		MeshFace* closest_face = &mesh->faces[0];
		forE(mesh->faces){
			vec3 face_normal = it->normal;
			f32  local_dot = target_normal.dot(face_normal);
			if(local_dot > furthest_dot){
				furthest_dot = local_dot;
				closest_face = it;
				closest_normal = face_normal;
			}
		}
		
		//find three closest vertexes to make a triangle from
		f32 smallest_distance0 = FLT_MAX, smallest_distance1 = FLT_MAX, smallest_distance2 = FLT_MAX;
		vec3 closest_vert0{}; vec3 closest_vert1{}; vec3 closest_vert2{};
		for(u32 vert_idx : closest_face->outerVertexes){
			f32 local_distance = mesh->vertexes[vert_idx].pos.distanceTo(target);
			if(local_distance < smallest_distance0){
				smallest_distance2 = smallest_distance1;
				smallest_distance1 = smallest_distance0;
				smallest_distance0 = local_distance;
				closest_vert2 = closest_vert1;
				closest_vert1 = closest_vert0;
				closest_vert0 = mesh->vertexes[vert_idx].pos;
			}else if(local_distance < smallest_distance1){
				smallest_distance2 = smallest_distance1;
				smallest_distance1 = local_distance;
				closest_vert2 = closest_vert1;
				closest_vert1 = mesh->vertexes[vert_idx].pos;
			}else if(local_distance < smallest_distance2){
				smallest_distance2 = local_distance;
				closest_vert2 = mesh->vertexes[vert_idx].pos;
			}
		}
		
		//clamp target to triangle and transform back to world space
		vec3 plane_point = ClosestPointOnPlane(closest_vert0, closest_normal, target);
		vec3 clamped_point = ClampPointToTriangle(plane_point, closest_vert0, closest_vert1, closest_vert2);
		return clamped_point * mesh_transform;
		//return ClampPointToTriangle(nearest_plane_point, closest_vert0, closest_vert1, closest_vert2) * mesh_transform;
		//return ClampPointToTriangle(target, closest_vert0, closest_vert1, closest_vert2) * mesh_transform;
	}
	
	inline static vec3 MeshTriangleMidpoint(Mesh::Triangle* tri){
		return (tri->p[0] + tri->p[1] + tri->p[2]) / 3.f;
	}
};