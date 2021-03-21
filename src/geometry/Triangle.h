#pragma once
#include "../math/Math.h"
#include "Edge.h"

struct Entity;

struct Triangle {
	//this can probably be different but it works for now
	Vector3 points[3];
	Vector3 proj_points[3];
	Vector3 poffsets[3]; //the triangle's points stored as offsets from the mesh's pos. so we always know what the orig shape of the mesh was
	Vector3 tex_points[3];
	Vector3 proj_tex_points[3];

	Triangle* orig		= nullptr;
	Entity* e			= nullptr;

	bool is_clip = false;

	Vector3 normal;
	float area;

	//maybe edges can be cleared when they're not actually needed,
	//and only spawned when used?
	Edge edges[3];

	bool selected = false;
	Color color = Color::WHITE;
	Color debug_color = Color::RED;

	Triangle() {}
	//constructing triangles without attached entity
	Triangle(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 position) {
		poffsets[0] = p1;
		poffsets[1] = p2;
		poffsets[2] = p3;

		points[0] = p1 + position;
		points[1] = p1 + position;
		points[2] = p1 + position;

		copy_points();

		edges[0] = Edge(p1, p2);
		edges[1] = Edge(p2, p3);
		edges[2] = Edge(p3, p1);

		tex_points[0] = Vector3(0, 0, 1);
		tex_points[1] = Vector3(0, 1, 1);
		tex_points[2] = Vector3(1, 0, 1);
	};

	Triangle(Vector3 p1, Vector3 p2, Vector3 p3, Entity* e, Vector3 position) {
		poffsets[0] = p1;
		poffsets[1] = p2;
		poffsets[2] = p3;

		points[0] = p1 + position;
		points[1] = p1 + position;
		points[2] = p1 + position;

		copy_points();

		edges[0] = Edge(p1, p2);
		edges[1] = Edge(p2, p3);
		edges[2] = Edge(p3, p1);

		this->e = e;
	}

	//for constructing a triangle with texture points
	Triangle(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 t1, Vector3 t2, Vector3 t3, Entity* e, Vector3 position) {
		poffsets[0] = p1;
		poffsets[1] = p2;
		poffsets[2] = p3;

		points[0] = p1 + position;
		points[1] = p1 + position;
		points[2] = p1 + position;

		copy_points();

		edges[0] = Edge(p1, p2);
		edges[1] = Edge(p2, p3);
		edges[2] = Edge(p3, p1);

		
		tex_points[0] = t1;
		tex_points[1] = t2;
		tex_points[2] = t3;

		this->e = e;
	}

	~Triangle() {}

	void copy_points() {
		for (int p = 0; p < 3; p++) { proj_points[p] = points[p]; }
		for (int p = 0; p < 3; p++) { proj_tex_points[p] = tex_points[p]; }
	}

	void update_edges() {
		edges[0].update(proj_points[0], proj_points[1]);
		edges[1].update(proj_points[1], proj_points[2]);
		edges[2].update(proj_points[2], proj_points[0]);
	}

	void set_color(Color newColor) {
		color = newColor; 
	}

	Color get_color() {
		if (selected) { return debug_color; }
		else { return color; }
	}

	Vector3 get_normal() {
		return (points[1] - points[0]).cross(points[2] - points[0]).yInvert().normalized();
	}

	void set_normal() {
		normal = (points[1] - points[0]).cross(points[2] - points[0]).yInvert().normalized();
	}

	Vector3 get_proj_normal() {
		Vector3 l1 = proj_points[1] - proj_points[0];
		Vector3 l2 = proj_points[2] - proj_points[0];
		return l1.cross(l2).yInvert().normalized();
	}

	//checks if a triangle contains a point in screen space
	//this works by forming a line between each point on the triangle
	//and checks to see if the test point is within the region enclosed
	//by those three lines
	bool contains_point(Vector3 point) {
		update_edges();

		//if normal points up then bool is true
		bool norms[3] = {
			(edges[0].edge_normal().y < 0) ? true : false,
			(edges[1].edge_normal().y < 0) ? true : false,
			(edges[2].edge_normal().y < 0) ? true : false
		};

		bool truths[3];
		for (int b = 0; b < 3; b++) {
			std::cout << "Norm " << norms[b] << std::endl;
			if (norms[b]) {
				truths[b] = edges[b].above_edge(point);
			}
			else {
				truths[b] = edges[b].below_edge(point);
			}
		}

		std::cout << truths[0] << " " << truths[1] << " " << truths[2] << std::endl;

		bool the_final_truth = true;
		for (bool b : truths) {
			if (!b) { 
				the_final_truth = false; 
			}
		}

		if (the_final_truth) { return true; }
		else { return false; }
	}

	float get_area() { return Math::TriangleArea(points[1] - points[0], points[2] - points[0]); }
	void set_area()	 { area = Math::TriangleArea(points[1] - points[0], points[2] - points[0]); }

	bool line_intersect(Edge3D* e) {
		float t = 0;

		Vector3 i = Math::VectorPlaneIntersect(points[0], get_normal(), e->p[0], e->p[1], t);

		float a1 = Math::TriangleArea(points[0] - i, points[2] - i);
		float a2 = Math::TriangleArea(points[2] - i, points[1] - i);
		float a3 = Math::TriangleArea(points[1] - i, points[0] - i);

		float ta = Math::round2f(a1 + a2 + a3);

		if (ta > Math::round2f(area) || !e->point_on_edge(i)) { 
			return false; 
		} else {
			return true; 
		}
	}

	//debug
	void display_edges() {
		update_edges();

		//ERROR_LOC("display_edges is not yet implemented for Vulkan yet!!");

		int n = 1;
		for (Edge e : edges) {
			//p->DrawString((e.edge_midpoint() + e.edge_normal() * -10).ToVector2(), std::to_string(n));
			n++;
		}

	}

	Vector3 midpoint() {
		float x_mid = (points[0].x + points[1].x + points[2].x) / 3;
		float y_mid = (points[0].y + points[1].y + points[2].y) / 3;
		float z_mid = (points[0].z + points[1].z + points[2].z) / 3;

		return (points[0] + points[1] + points[2]) / 3;
	}

	std::string str() { return "{(" + points[0].str() + "), (" + points[1].str() + "), (" + points[2].str() + ")}"; }
	std::string str2f() { return "{(" + points[0].str2f() + "), (" + points[1].str2f() + "), (" + points[2].str2f() + ")}"; }
};
