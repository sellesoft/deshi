#pragma once
#ifndef DESHI_GEOMETRY_H
#define DESHI_GEOMETRY_H

#include "../math/math.h"
#include "../core/model.h"

//NOTE all targets are expected to be in the space of the reference object

//convert to barycentric coords, clamp the triangle, convert back to cartesian coords
global_ vec3 ClampPointToTriangle(vec3 point, vec3 tri0, vec3 tri1, vec3 tri2){
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

inline global_ vec3 MeshTriangleMidpoint(MeshTriangle* tri){
	return (tri->p[0] + tri->p[1] + tri->p[2]) / 3.f;
}

global_ MeshFace* FurthestHullFaceAlongNormal(Mesh* mesh, vec3 target_normal){
	MeshFace* closest_face = 0;
	f32 max_projection = -INFINITY;
	forE(mesh->faces){
		f32 local_dot = target_normal.dot(it->normal);
		if(local_dot > max_projection){
			max_projection = local_dot;
			closest_face = it;
		}
	}
	return closest_face;
}

global_ vec3 FurthestHullVertexPositionAlongNormal(Mesh* mesh, vec3 target_normal){
	MeshVertex* closest_vertex = 0;
	f32 max_projection = -INFINITY;
	forE(mesh->vertexes){
		f32 local_dot = target_normal.dot(it->pos);
		if(local_dot > max_projection){
			max_projection = local_dot;
			closest_vertex = it;
		}
	}
	return closest_vertex->pos;
}

inline global_ vec3 ClosestPointOnPlane(vec3 plane_point, vec3 plane_normal, vec3 target){
	return target + (plane_normal * -Math::DistPointToPlane(target, plane_normal, plane_point));
}

inline global_ vec3 ClosestPointOnAABB(vec3 halfDims, vec3 target) {
	return Math::clamp(target, -halfDims, halfDims);
}

inline global_ vec3 ClosestPointOnSphere(float radius, vec3 target) {
	return target.normalized() * radius;
}

global_ vec3 ClosestPointOnHull(Mesh* mesh, vec3 target){
	//find closest face to target based on the face normal
	vec3 target_normal = target.normalized();
	vec3 closest_normal = vec3::ZERO;
	f32  max_projection = -INFINITY;
	MeshFace* closest_face = &mesh->faces[0];
	forE(mesh->faces){
		vec3 face_normal = it->normal;
		f32  local_dot = target_normal.dot(face_normal);
		if(local_dot > max_projection){
			max_projection = local_dot;
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
	return ClampPointToTriangle(plane_point, closest_vert0, closest_vert1, closest_vert2);
}

#endif //DESHI_GEOMETRY_H