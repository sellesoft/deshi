#pragma once
#include "../math/Math.h"

struct Entity;

//attached to entities to allow different forms of checking sides of more complex objects
struct Edge {
	vec2 p[2];
	//if lead is true then p[1] is the right most point.
	//ditto for high but on y
	bool lead = false;
	bool high = false;

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

	float slope() {
		if (p[1].x == p[0].x || p[1].y == p[0].y) { return 0; }
		else { return (p[1].y - p[0].y) / (p[1].x - p[0].x); }
	}

	//y intercept and range/domain checks
	float ycross() { return p[!lead].y - slope() * p[!lead].x; }
	bool within_range(vec2 point)  { return (point.y <= p[high].y&& point.y >= p[!high].y); }
	bool within_range(float y_point)  { return (y_point <= p[high].y&& y_point >= p[!high].y); }
	bool within_domain(vec2 point) { return (point.x <= p[lead].x&& point.x >= p[!lead].x); }
	bool within_domain(float x_point) { return (x_point <= p[lead].x&& x_point >= p[!lead].x); }

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
	bool on_edge(vec2 point) {
		if ((point.y == slope() * point.x + ycross()) && within_domain(point)) { return true; }
		else { return false; }
	}

	//these signs may look wrong but its to accomidate for the top left coord (maybe)
	bool above_edge(vec2 point) {
		int bp = 0;
		if (point.y < slope() * point.x + ycross()) { return true; }
		else { return false; }
	}

	bool below_edge(vec2 point) {
		if (point.y > slope() * point.x + ycross()) { return true; }
		else { return false; }
	}

	bool right_of_edge(vec2 point) {
		if ((point.x > (point.y - ycross()) / slope())) { return true; }
		else { return false; }
	}

	bool left_of_edge(vec2 point) {
		if ((point.x < (point.y - ycross()) / slope())) { return true; }
		else { return false; }
	}

	//checks if two edges intersect by finding their line representation's
	//intersection and then seeing if that point lies on either of them
	bool edge_intersect(Edge e) {
		if (slope() == e.slope() && (!on_edge(e.p[0]) || !on_edge(e.p[1]))) { return false; }
		vec2 cross = Math::LineIntersect2(slope(), ycross(), e.slope(), e.ycross());
		if (within_domain(cross) && within_range(cross) &&
			e.within_domain(cross) && e.within_range(cross)) {
			return true;
		} else return false;
	}

	std::string str() { return "{(" + p[0].str() + "), (" + p[1].str() + ")}"; }
	std::string str2f() { return "{(" + p[0].str2f() + "), (" + p[1].str2f() + ")}"; }
};

struct Edge3D {
	vec3 p[2];
	//if lead is true then p[1] is the right most point.
	//ditto for high but on y and for deep on z
	bool lead = false;
	bool high = false;
	bool deep = false;
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

	bool within_range(vec3 point)  { return (point.y <= p[high].y && point.y >= p[!high].y); }
	bool within_range(float y_point)  { return (y_point <= p[high].y && y_point >= p[!high].y); }
	bool within_domain(vec3 point) { return (point.x <= p[lead].x && point.x >= p[!lead].x); }
	bool within_domain(float x_point) { return (x_point <= p[lead].x && x_point >= p[!lead].x); }
	bool within_depth(vec3 point)  { return (point.z <= p[deep].z && point.z >= p[!deep].z); }
	bool within_depth(float z_point)  { return (z_point <= p[deep].z && z_point >= p[!deep].z); }

	virtual std::string str() { return "{(" + p[0].str() + "), (" + p[1].str() + ")}"; }
	virtual std::string str2f() { return "{(" + p[0].str2f() + "), (" + p[1].str2f() + ")}"; }

	vec3 edge_midpoint() {
		return vec3((p[0].x + p[1].x) / 2, (p[0].y + p[1].y) / 2, (p[0].z + p[1].z) / 2);
	}

	vec3 direction() { return p[1] - p[0]; }

	bool point_on_edge(vec3 point) {
		if (/*p[0] == point  || p[1] == point ||*/
			within_range(point) && within_domain(point) && within_depth(point) &&
			Math::round2v(direction().normalized()) == Math::round2v(point.normalized())) {
			return true;
		}
		return false;
	}

};

struct RenderedEdge3D : public Edge3D {
	Color color;

	RenderedEdge3D(vec3 point1, vec3 point2, Color color = Color::WHITE) {
		p[0] = point1;
		p[1] = point2;
		if (point1.x > point2.x) { lead = false; }
		else { lead = true; }
		if (point1.y > point2.y) { high = false; }
		else { high = true; }
		if (point1.z > point2.z) { deep = false; }
		else { deep = true; }
		this->color = color;
	}
};