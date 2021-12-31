#pragma once
#ifndef DESHI_EDGE_H
#define DESHI_EDGE_H

#include "../math/Math.h"
#include "../utils/color.h"

struct Entity;

//attached to entities to allow different forms of checking sides of more complex objects
struct Edge {
	vec2 p[2];
	//if lead is true then p[1] is the right most point.
	//ditto for high but on y
	b32 lead = false;
	b32 high = false;
	
	Edge() {}
	Edge(vec2 point1, vec2 point2) {
		p[0] = point1;
		p[1] = point2;
		if (point1.x > point2.x) { lead = false; }
		else { lead = true; }
		if (point1.y > point2.y) { high = false; }
		else { high = true; }
	}
	
	void update(vec2 point1, vec2 point2) {
		p[0] = point1;
		p[1] = point2;
		if (point1.x > point2.x) { lead = false; }
		else { lead = true; }
		if (point1.y > point2.y) { high = false; }
		else { high = true; }
	}
	
	f32 slope() {
		if (p[1].x == p[0].x || p[1].y == p[0].y) { return 0; }
		else { return (p[1].y - p[0].y) / (p[1].x - p[0].x); }
	}
	
	//y intercept and range/domain checks
	f32 ycross() { return p[!lead].y - slope() * p[!lead].x; }
	b32 within_range(vec2 point)   { return (point.y <= p[high].y && point.y >= p[!high].y); }
	b32 within_range(f32 y_point)  { return (y_point <= p[high].y && y_point >= p[!high].y); }
	b32 within_domain(vec2 point)  { return (point.x <= p[lead].x && point.x >= p[!lead].x); }
	b32 within_domain(f32 x_point) { return (x_point <= p[lead].x && x_point >= p[!lead].x); }
	
	//returns edge's normal
	//returns a normal rotated -90
	vec2 edge_normal() {
		vec2 v = p[1] - p[0];
		return vec2(v.y, -v.x).normalized();
	}
	
	vec2 edge_midpoint() {
		return vec2((p[0].x + p[1].x) / 2, (p[0].y + p[1].y) / 2);
	}
	
	//is a point on, above, or below edge
	b32 on_edge(vec2 point) {
		if ((point.y == slope() * point.x + ycross()) && within_domain(point)) { return true; }
		else { return false; }
	}
	
	//these signs may look wrong but its to accomidate for the top left coord (maybe)
	b32 above_edge(vec2 point) {
		int bp = 0;
		if (point.y < slope() * point.x + ycross()) { return true; }
		else { return false; }
	}
	
	b32 below_edge(vec2 point) {
		if (point.y > slope() * point.x + ycross()) { return true; }
		else { return false; }
	}
	
	b32 right_of_edge(vec2 point) {
		if ((point.x > (point.y - ycross()) / slope())) { return true; }
		else { return false; }
	}
	
	b32 left_of_edge(vec2 point) {
		if ((point.x < (point.y - ycross()) / slope())) { return true; }
		else { return false; }
	}
	
	//checks if two edges intersect by finding their line representation's
	//intersection and then seeing if that point lies on either of them
	b32 edge_intersect(Edge e) {
		if (slope() == e.slope() && (!on_edge(e.p[0]) || !on_edge(e.p[1]))) { return false; }
		//vec2 cross = Math::LineIntersect2(slope(), ycross(), e.slope(), e.ycross());
		vec2 cross = Math::LineIntersect2(p[!lead], p[lead], e.p[!e.lead], e.p[e.lead]);
		if (within_domain(cross) && within_range(cross) &&
			e.within_domain(cross) && e.within_range(cross)) {
			return true;
		} else return false;
	}
};

struct Edge3D {
	vec3 p[2];
	//if lead is true then p[1] is the right most point.
	//ditto for high but on y and for deep on z
	b32 lead = false;
	b32 high = false;
	b32 deep = false;
	Entity* e = nullptr;
	
	Edge3D() {}
	Edge3D(vec3 point1, vec3 point2) {
		p[0] = point1;
		p[1] = point2;
		if (point1.x > point2.x) { lead = false; }
		else { lead = true; }
		if (point1.y > point2.y) { high = false; }
		else { high = true; }
		if (point1.z > point2.z) { deep = false; }
		else { deep = true; }
	}
	
	b32 within_range(vec3 point)   { return (point.y <= p[high].y && point.y >= p[!high].y); }
	b32 within_range(f32 y_point)  { return (y_point <= p[high].y && y_point >= p[!high].y); }
	b32 within_domain(vec3 point)  { return (point.x <= p[lead].x && point.x >= p[!lead].x); }
	b32 within_domain(f32 x_point) { return (x_point <= p[lead].x && x_point >= p[!lead].x); }
	b32 within_depth(vec3 point)   { return (point.z <= p[deep].z && point.z >= p[!deep].z); }
	b32 within_depth(f32 z_point)  { return (z_point <= p[deep].z && z_point >= p[!deep].z); }
	
	vec3 edge_midpoint() {
		return vec3((p[0].x + p[1].x) / 2, (p[0].y + p[1].y) / 2, (p[0].z + p[1].z) / 2);
	}
	
	vec3 direction() { return p[1] - p[0]; }
	
	b32 point_on_edge(vec3 point) {
		if (/*p[0] == point  || p[1] == point ||*/
			within_range(point) && within_domain(point) && within_depth(point) &&
			Math::round(direction().normalized()) == Math::round(point.normalized())) {
			return true;
		}
		return false;
	}
	
};

struct RenderedEdge3D : public Edge3D {
	color col;
	
	RenderedEdge3D(vec3 point1, vec3 point2, color col = Color_White) {
		p[0] = point1;
		p[1] = point2;
		if (point1.x > point2.x) { lead = false; }
		else { lead = true; }
		if (point1.y > point2.y) { high = false; }
		else { high = true; }
		if (point1.z > point2.z) { deep = false; }
		else { deep = true; }
		this->col = col;
	}
};

#endif //DESHI_EDGE_H