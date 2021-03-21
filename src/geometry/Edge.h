#pragma once
#include "../math/Math.h"

struct Entity;

//attached to entities to allow different forms of checking sides of more complex objects
struct Edge {
	Vector3 p[2];
	//if lead is true then p[1] is the right most point.
	//ditto for high but on y
	bool lead = false;
	bool high = false;

	Edge() {}
	Edge(Vector3 point1, Vector3 point2) {
		p[0] = point1;
		p[1] = point2;
		if (point1.x > point2.x) { lead = false; }
		else { lead = true; }
		if (point1.y > point2.y) { high = false; }
		else { high = true; }
	}

	void update(Vector3 point1, Vector3 point2) {
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
	bool within_range(Vector3 point)  { return (point.y <= p[high].y&& point.y >= p[!high].y); }
	bool within_range(float y_point)  { return (y_point <= p[high].y&& y_point >= p[!high].y); }
	bool within_domain(Vector3 point) { return (point.x <= p[lead].x&& point.x >= p[!lead].x); }
	bool within_domain(float x_point) { return (x_point <= p[lead].x&& x_point >= p[!lead].x); }

	//returns edge's normal
	//returns a normal rotated -90
	Vector3 edge_normal() {
		Vector3 v = p[1] - p[0];
		return Vector3(v.y, -v.x, 0).normalized();
	}

	Vector3 edge_midpoint() {
		return Vector3((p[0].x + p[1].x) / 2, (p[0].y + p[1].y) / 2, 0);
	}

	//is a point on, above, or below edge
	bool on_edge(Vector3 point) {
		if ((point.y == slope() * point.x + ycross()) && within_domain(point)) { return true; }
		else { return false; }
	}

	//these signs may look wrong but its to accomidate for the top left coord (maybe)
	bool above_edge(Vector3 point) {
		int bp = 0;
		if (point.y < slope() * point.x + ycross()) { return true; }
		else { return false; }
	}

	bool below_edge(Vector3 point) {
		if (point.y > slope() * point.x + ycross()) { return true; }
		else { return false; }
	}

	bool right_of_edge(Vector3 point) {
		if ((point.x > (point.y - ycross()) / slope())) { return true; }
		else { return false; }
	}

	bool left_of_edge(Vector3 point) {
		if ((point.x < (point.y - ycross()) / slope())) { return true; }
		else { return false; }
	}

	//checks if two edges intersect by finding their line representation's
	//intersection and then seeing if that point lies on either of them
	bool edge_intersect(Edge e) {
		if (slope() == e.slope() && (!on_edge(e.p[0]) || !on_edge(e.p[1]))) { return false; }
		Vector3 cross = Math::LineIntersect2(slope(), ycross(), e.slope(), e.ycross());
		if (within_domain(cross) && within_range(cross) &&
			e.within_domain(cross) && e.within_range(cross)) {
			return true;
		}
		else {
			return false;
		}
	}

	std::string str() { return "{(" + p[0].str() + "), (" + p[1].str() + ")}"; }
	std::string str2f() { return "{(" + p[0].str2f() + "), (" + p[1].str2f() + ")}"; }
};

struct Edge3D {
	Vector3 p[2];
	//if lead is true then p[1] is the right most point.
	//ditto for high but on y and for deep on z
	bool lead = false;
	bool high = false;
	bool deep = false;
	Entity* e = nullptr;

	Edge3D() {}
	Edge3D(Vector3 point1, Vector3 point2) {
		p[0] = point1;
		p[1] = point2;
		if (point1.x > point2.x) { lead = false; }
		else { lead = true; }
		if (point1.y > point2.y) { high = false; }
		else { high = true; }
		if (point1.z > point2.z) { deep = false; }
		else { deep = true; }
	}

	bool within_range(Vector3 point)  { return (point.y <= p[high].y && point.y >= p[!high].y); }
	bool within_range(float y_point)  { return (y_point <= p[high].y && y_point >= p[!high].y); }
	bool within_domain(Vector3 point) { return (point.x <= p[lead].x && point.x >= p[!lead].x); }
	bool within_domain(float x_point) { return (x_point <= p[lead].x && x_point >= p[!lead].x); }
	bool within_depth(Vector3 point)  { return (point.z <= p[deep].z && point.z >= p[!deep].z); }
	bool within_depth(float z_point)  { return (z_point <= p[deep].z && z_point >= p[!deep].z); }

	virtual std::string str() { return "{(" + p[0].str() + "), (" + p[1].str() + ")}"; }
	virtual std::string str2f() { return "{(" + p[0].str2f() + "), (" + p[1].str2f() + ")}"; }

	Vector3 edge_midpoint() {
		return Vector3((p[0].x + p[1].x) / 2, (p[0].y + p[1].y) / 2, (p[0].z + p[1].z) / 2);
	}

	Vector3 direction() { return p[1] - p[0]; }

	bool point_on_edge(Vector3 point) {
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

	RenderedEdge3D(Vector3 point1, Vector3 point2, Color color = Color::WHITE) {
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