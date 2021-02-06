#include "dsh_RenderSceneSystem.h"
#include "dsh_ConsoleSystem.h"
#include "../math/dsh_Math.h"

#include "../components/dsh_Scene.h"
#include "../components/dsh_Mesh.h"
#include "../components/dsh_Camera.h"
#include "../components/dsh_Light.h"
#include "../components/dsh_Screen.h"
#include "../components/dsh_Transform.h"
#include "../components/dsh_Physics.h"
#include "../components/dsh_Time.h"

void RenderSceneSystem::Init() {
	
}

void TexturedTriangle(Scene* scene, Screen* screen, Triangle* tri){	
	int x1 = tri->proj_points[0].x; int x2 = tri->proj_points[1].x; int x3 = tri->proj_points[2].x;
	int y1 = tri->proj_points[0].y; int y2 = tri->proj_points[1].y; int y3 = tri->proj_points[2].y;
		
	float u1 = tri->proj_tex_points[0].x; float u2 = tri->proj_tex_points[1].x; float u3 = tri->proj_tex_points[2].x;
	float v1 = tri->proj_tex_points[0].y; float v2 = tri->proj_tex_points[1].y; float v3 = tri->proj_tex_points[2].y;
	float w1 = tri->proj_tex_points[0].z; float w2 = tri->proj_tex_points[1].z; float w3 = tri->proj_tex_points[2].z;

	if (y2 < y1) { std::swap(y1, y2); std::swap(x1, x2); std::swap(u1, u2); std::swap(v1, v2); std::swap(w1, w2); }
	if (y3 < y1) { std::swap(y1, y3); std::swap(x1, x3); std::swap(u1, u3); std::swap(v1, v3); std::swap(w1, w3); }
	if (y3 < y2) { std::swap(y2, y3); std::swap(x2, x3); std::swap(u2, u3); std::swap(v2, v3); std::swap(w2, w3); }

	int dy1 = y2 - y1;
	int dx1 = x2 - x1;
	float dv1 = v2 - v1;
	float du1 = u2 - u1;
	float dw1 = w2 - w1;

	int dy2 = y3 - y1;
	int dx2 = x3 - x1;
	float dv2 = v3 - v1;
	float du2 = u3 - u1;
	float dw2 = w3 - w1;

	float tex_u, tex_v, tex_w;

	float	dax_step = 0, dbx_step = 0,
			du1_step = 0, dv1_step = 0,
			du2_step = 0, dv2_step = 0,
			dw1_step = 0, dw2_step = 0;

	if (dy1) dax_step = dx1 / (float)abs(dy1);
	if (dy2) dbx_step = dx2 / (float)abs(dy2);

	if (dy1) du1_step = du1 / (float)abs(dy1);
	if (dy1) dv1_step = dv1 / (float)abs(dy1);
	if (dy1) dw1_step = dw1 / (float)abs(dy1);

	if (dy2) du2_step = du2 / (float)abs(dy2);
	if (dy2) dv2_step = dv2 / (float)abs(dy2);
	if (dy2) dw2_step = dw2 / (float)abs(dy2);

	if (dy1) {
		for (int i = y1; i <= y2; i++) {
			int ax = x1 + (float)(i - y1) * dax_step;
			int bx = x1 + (float)(i - y1) * dbx_step;

			float tex_su = u1 + (float)(i - y1) * du1_step;
			float tex_sv = v1 + (float)(i - y1) * dv1_step;
			float tex_sw = w1 + (float)(i - y1) * dw1_step;

			float tex_eu = u1 + (float)(i - y1) * du2_step;
			float tex_ev = v1 + (float)(i - y1) * dv2_step;
			float tex_ew = w1 + (float)(i - y1) * dw2_step;

			if (ax > bx) {
				std::swap(ax, bx);
				std::swap(tex_su, tex_eu);
				std::swap(tex_sv, tex_ev);
				std::swap(tex_sw, tex_ew);
			}

			tex_u = tex_su;
			tex_v = tex_sv;
			tex_w = tex_sw;

			float tstep = 1.0f / ((float)(bx - ax));
			float t = 0.0f;

			for (int j = ax; j < bx; j++) {
				tex_u = (1.0f - t) * tex_su + t * tex_eu;
				tex_v = (1.0f - t) * tex_sv + t * tex_ev; 
				tex_w = (1.0f - t) * tex_sw + t * tex_ew; 

				//LOG(tex_w);

				if (tex_w > scene->pixelDepthBuffer[i * (size_t)screen->width + j]) {
					p->Draw(j, i, tri->e->GetComponent<Mesh>()->texture->Sample(tex_u / tex_w, tex_v / tex_w));
					scene->pixelDepthBuffer[i * (size_t)screen->width + j] = tex_w;
				}
				t += tstep;
			}
		}
	}

	dy1 = y3 - y2;
	dx1 = x3 - x2;
	dv1 = v3 - v2;
	du1 = u3 - u2;
	dw1 = w3 - w2;

	if (dy1) dax_step = dx1 / (float)abs(dy1);
	if (dy2) dbx_step = dx2 / (float)abs(dy2);

	du1_step = 0, dv1_step = 0;
	if (dy1) du1_step = du1 / (float)abs(dy1);
	if (dy1) dv1_step = dv1 / (float)abs(dy1);
	if (dy1) dw1_step = dw1 / (float)abs(dy1);

	if (dy1) {
		for (int i = y2; i <= y3; i++) {
			int ax = x2 + (float)(i - y2) * dax_step;
			int bx = x1 + (float)(i - y1) * dbx_step;

			float tex_su = u2 + (float)(i - y2) * du1_step;
			float tex_sv = v2 + (float)(i - y2) * dv1_step;
			float tex_sw = w2 + (float)(i - y2) * dw1_step;

			float tex_eu = u1 + (float)(i - y1) * du2_step;
			float tex_ev = v1 + (float)(i - y1) * dv2_step;
			float tex_ew = w1 + (float)(i - y1) * dw2_step;

			if (ax > bx) {
				std::swap(ax, bx);
				std::swap(tex_su, tex_eu);
				std::swap(tex_sv, tex_ev);
				std::swap(tex_sw, tex_ew);
			}

			tex_u = tex_su;
			tex_v = tex_sv;
			tex_w = tex_sw;

			float tstep = 1.0f / ((float)(bx - ax));
			float t = 0.0f;

			for (int j = ax; j < bx; j++) {
				tex_u = (1.0f - t) * tex_su + t * tex_eu;
				tex_v = (1.0f - t) * tex_sv + t * tex_ev;
				tex_w = (1.0f - t) * tex_sw + t * tex_ew;

				if (tex_w > scene->pixelDepthBuffer[i * (size_t)screen->width + j]) {

					p->Draw(j, i, tri->e->GetComponent<Mesh>()->texture->Sample(tex_u / tex_w, tex_v / tex_w));
					scene->pixelDepthBuffer[i * (size_t)screen->width + j] = tex_w;
				}
				t += tstep;
			}
		}
	}
}

int ClipTriangles(const Vector3& plane_p, Vector3 plane_n, Triangle* in_tri, std::array<Triangle*, 2>& out_tris) {
	plane_n.normalize();

	//temp storage to classify points on either side of plane
	Vector3* inside_points[3];  int nInsidePointCount = 0;
	Vector3* outside_points[3]; int nOutsidePointCount = 0;
	Vector3* inside_tex[3];		int nInsideTexCount = 0;
	Vector3* outside_tex[3];	int nOutsideTexCount = 0;

	auto dist = [&](const Vector3& p)
	{
		return (plane_n.x * p.x + plane_n.y * p.y + plane_n.z * p.z - plane_n.dot(plane_p));
	};

	//signed distance of each point in triangle to plane
	float d0 = dist(in_tri->proj_points[0]);
	float d1 = dist(in_tri->proj_points[1]);
	float d2 = dist(in_tri->proj_points[2]);

	if (d0 >= 0) { inside_points[nInsidePointCount++]   = &in_tri->proj_points[0]; inside_tex[nInsideTexCount++]   = &in_tri->proj_tex_points[0]; }
	else         { outside_points[nOutsidePointCount++] = &in_tri->proj_points[0]; outside_tex[nOutsideTexCount++] = &in_tri->proj_tex_points[0]; }
	if (d1 >= 0) { inside_points[nInsidePointCount++]   = &in_tri->proj_points[1]; inside_tex[nInsideTexCount++]   = &in_tri->proj_tex_points[1]; }
	else         { outside_points[nOutsidePointCount++] = &in_tri->proj_points[1]; outside_tex[nOutsideTexCount++] = &in_tri->proj_tex_points[1]; }
	if (d2 >= 0) { inside_points[nInsidePointCount++]   = &in_tri->proj_points[2]; inside_tex[nInsideTexCount++]   = &in_tri->proj_tex_points[2]; }
	else         { outside_points[nOutsidePointCount++] = &in_tri->proj_points[2]; outside_tex[nOutsideTexCount++] = &in_tri->proj_tex_points[2]; }

	//classify points and break input triangle into smaller trangles if
	//required. there are four possible outcomes

	float t;
	if (nInsidePointCount == 0) { 
	//all points lie outside the plane
		return 0; 
	} else if (nInsidePointCount == 3) { 
	//all points lie inside the plane so do nothing and allow triangle to pass
		out_tris = {in_tri, 0};
		return 1; 
	} else if (nInsidePointCount == 1 && nOutsidePointCount == 2) {
		Triangle* newTri = new Triangle();
		newTri->color = in_tri->color;
		newTri->e = in_tri->e;
		newTri->e->GetComponent<Mesh>()->texture = in_tri->e->GetComponent<Mesh>()->texture;
		newTri->is_clip = true; 

		//the inside point is valid so we keep it
		newTri->proj_points[0] = *inside_points[0];
		newTri->proj_tex_points[0] = *inside_tex[0];
		
		//but the two new points are not where the original triangle intersects with the plane
		newTri->proj_points[1] = Math::VectorPlaneIntersect(plane_p, plane_n, *inside_points[0], *outside_points[0], t);
		newTri->proj_tex_points[1].x = t * (outside_tex[0]->x - inside_tex[0]->x) + inside_tex[0]->x;
		newTri->proj_tex_points[1].y = t * (outside_tex[0]->y - inside_tex[0]->y) + inside_tex[0]->y;
		newTri->proj_tex_points[1].z = t * (outside_tex[0]->z - inside_tex[0]->z) + inside_tex[0]->z;

		newTri->proj_points[2] = Math::VectorPlaneIntersect(plane_p, plane_n, *inside_points[0], *outside_points[1], t);
		newTri->proj_tex_points[2].x = t * (outside_tex[1]->x - inside_tex[0]->x) + inside_tex[0]->x;
		newTri->proj_tex_points[2].y = t * (outside_tex[1]->y - inside_tex[0]->y) + inside_tex[0]->y;
		newTri->proj_tex_points[2].z = t * (outside_tex[1]->z - inside_tex[0]->z) + inside_tex[0]->z;

		out_tris = {newTri, 0};
		return 1; //return new triangle
	} else if (nInsidePointCount == 2 && nOutsidePointCount == 1) {
	//triangle will be clipped and becomes a quad which is cut into two more triangles
		Triangle* newTri = new Triangle();
		newTri->e = in_tri->e;
		newTri->e->GetComponent<Mesh>()->texture = in_tri->e->GetComponent<Mesh>()->texture;
		newTri->is_clip = true;
		newTri->color = in_tri->color;

		newTri->proj_points[0] = *inside_points[0];
		newTri->proj_points[1] = *inside_points[1];
		newTri->proj_tex_points[0] = *inside_tex[0];
		newTri->proj_tex_points[1] = *inside_tex[1];

		newTri->proj_points[2] = Math::VectorPlaneIntersect(plane_p, plane_n, *inside_points[0], *outside_points[0], t);
		newTri->proj_tex_points[2].x = t * (outside_tex[0]->x - inside_tex[0]->x) + inside_tex[0]->x;
		newTri->proj_tex_points[2].y = t * (outside_tex[0]->y - inside_tex[0]->y) + inside_tex[0]->y;
		newTri->proj_tex_points[2].z = t * (outside_tex[0]->z - inside_tex[0]->z) + inside_tex[0]->z;

		Triangle* newTri2 = new Triangle(); //TODO(, sushi) figure out why this is leaking
		newTri2->color = in_tri->color;
		newTri2->e = in_tri->e;
		newTri2->e->GetComponent<Mesh>()->texture = in_tri->e->GetComponent<Mesh>()->texture;
		newTri2->is_clip = true;

		newTri2->proj_points[0] = *inside_points[1];
		newTri2->proj_tex_points[0] = *inside_tex[1];
		newTri2->proj_points[1] = newTri->proj_points[2];
		newTri2->proj_tex_points[1] = newTri->proj_tex_points[2];

		newTri2->proj_points[2] = Math::VectorPlaneIntersect(plane_p, plane_n, *inside_points[1], *outside_points[0], t);
		newTri2->proj_tex_points[2].x = t * (outside_tex[0]->x - inside_tex[1]->x) + inside_tex[1]->x;
		newTri2->proj_tex_points[2].y = t * (outside_tex[0]->y - inside_tex[1]->y) + inside_tex[1]->y;
		newTri2->proj_tex_points[2].z = t * (outside_tex[0]->z - inside_tex[1]->z) + inside_tex[1]->z;

		out_tris = {newTri, newTri2};
		return 2;
	} else {
		ASSERT(false, "This shouldnt happen");
		return -1;
	}
} //ClipTriangles

int DrawTriangles(Scene* scene, Screen* screen,  std::list<Triangle*>* list) {
	int drawnCount = 0;

	for(Triangle* tr : *list) {
		//draw textures
		if(scene->RENDER_TEXTURES) {				
			TexturedTriangle(scene, screen, p, tr);
		}

		//draw wireframe
		if(scene->RENDER_WIREFRAME) {
			p->DrawTriangle(tr->proj_points[0].x, tr->proj_points[0].y,
				tr->proj_points[1].x, tr->proj_points[1].y,
				tr->proj_points[2].x, tr->proj_points[2].y,
				Color::WHITE);
		}

		//draw edges numbers
		if(scene->RENDER_EDGE_NUMBERS) {
			tr->display_edges(p);
		}

		//delete new clipping triangles
		if(tr->is_clip) {
			delete tr;
		}
		drawnCount++;
	}

	return drawnCount;
}

int RenderTriangles(Scene* scene, Camera* camera, Screen* screen) {
	int drawnTriCount = 0;
	for(Mesh* mesh : scene->meshes) {
		std::vector<Vector3*> screenSpaceVertices;
		for(Triangle& t : mesh->triangles) {
			if(scene->RENDER_MESH_VERTICES) {
				scene->lines.push_back(new RenderedEdge3D(t.points[0], t.points[0] + Vector3(0, .01f, 0),	Color::GREEN));
				scene->lines.push_back(new RenderedEdge3D(t.points[1], t.points[1] + Vector3(0, .01f, 0),	Color::GREEN));
				scene->lines.push_back(new RenderedEdge3D(t.points[2], t.points[2] + Vector3(0, .01f, 0),	Color::GREEN));
			}

			t.copy_points(); //copy worldspace points to proj_points
			t.set_normal();
			t.set_area();

			Vector3 triNormal = t.get_normal();

			if(scene->RENDER_MESH_NORMALS) {
				Vector3 mid = t.midpoint();
				scene->lines.push_back(new RenderedEdge3D(mid, mid + (triNormal * .1f), Color::GREEN));
			}

			//if the angle between the middle of the triangle and the camera is greater less 90 degrees, it should show
			if(triNormal.dot(t.midpoint() - camera->position) < 0) {  //TODO(or,delle) see if zClipIndex can remove the .midpoint()
		//project points to view/camera space
				for(Vector3& pp : t.proj_points) {
					pp = Math::WorldToCamera(pp, camera->viewMatrix).ToVector3();
				}

		//clip to the nearZ plane in view/clip space
				std::array<Triangle*, 2> zClipped = {};
				int numZClipped = ClipTriangles(Vector3(0, 0, camera->nearZ), Vector3::FORWARD, &t, zClipped);

				for(int zClipIndex = 0; zClipIndex < numZClipped; ++zClipIndex) {
					//project to screen
					for(int pIndex = 0; pIndex < 3; ++pIndex) {
						float w;
						zClipped[zClipIndex]->proj_points[pIndex] = Math::CameraToScreen(zClipped[zClipIndex]->proj_points[pIndex], camera->projectionMatrix, screen->dimensions, w);
						zClipped[zClipIndex]->proj_tex_points[pIndex].x /= w;
						zClipped[zClipIndex]->proj_tex_points[pIndex].y /= w;
						zClipped[zClipIndex]->proj_tex_points[pIndex].z = 1.f / w;

						if(scene->RENDER_SCREEN_BOUNDING_BOX) {
							screenSpaceVertices.push_back(&zClipped[zClipIndex]->proj_points[pIndex]);
						}
					}
					zClipped[zClipIndex]->orig = &t;
					//zClipped[zClipIndex]->sprite = t.sprite;

		//clip to screen borders in screen space
					std::list<Triangle*> borderClippedTris;
					borderClippedTris.push_back(zClipped[zClipIndex]);
					int newBClippedTris = 1;
					for(int bClipSide = 0; bClipSide < 4; ++bClipSide) {
						while(newBClippedTris > 0) {
							std::array<Triangle*, 2> bClipped = {};
							Triangle* tri = borderClippedTris.front();
							borderClippedTris.pop_front();
							newBClippedTris--;

							int numBClipped = 0;
							switch(bClipSide) {
								case 0: { numBClipped = ClipTriangles(Vector3::ZERO,					Vector3::UP,    tri, bClipped); } break;
								case 1: { numBClipped = ClipTriangles(Vector3(0, screen->height-1, 0),	Vector3::DOWN,  tri, bClipped); } break;
								case 2: { numBClipped = ClipTriangles(Vector3::ZERO,					Vector3::RIGHT, tri, bClipped); } break;
								case 3: { numBClipped = ClipTriangles(Vector3(screen->width-1, 0, 0),	Vector3::LEFT,  tri, bClipped); } break;
							}

							for(int bClipIndex = 0; bClipIndex < numBClipped; ++bClipIndex) {
								bClipped[bClipIndex]->e->GetComponent<Mesh>()->texture = zClipped[zClipIndex]->e->GetComponent<Mesh>()->texture;
								bClipped[bClipIndex]->orig = tri->orig;
								borderClippedTris.push_back(bClipped[bClipIndex]);
							}
						}
						newBClippedTris = borderClippedTris.size();
					}

		//draw triangles
					drawnTriCount += DrawTriangles(scene, screen, p, &borderClippedTris);
				}
			}
		}
		if(scene->RENDER_SCREEN_BOUNDING_BOX && screenSpaceVertices.size() > 0) {
			Vector3* leftMost;
			Vector3* rightMost;
			Vector3* topMost;
			Vector3* bottomMost;

			std::sort(screenSpaceVertices.begin(), screenSpaceVertices.end(), ([](Vector3* v1, Vector3* v2) {return v1->x > v2->x; }));
			leftMost = screenSpaceVertices[screenSpaceVertices.size() - 1];
			rightMost = screenSpaceVertices[0];
			
			std::sort(screenSpaceVertices.begin(), screenSpaceVertices.end(), ([](Vector3* v1, Vector3* v2) {return v1->y > v2->y; }));
			topMost = screenSpaceVertices[screenSpaceVertices.size() - 1];
			bottomMost = screenSpaceVertices[0];

			p->DrawRect(Vector2(leftMost->x, topMost->y), Vector2(rightMost->x - leftMost->x, bottomMost->y - topMost->y));
		}
	}
	return drawnTriCount;
} //RenderTriangles

//the input vectors should be in viewMatrix/camera space
//returns true if the line can be rendered after clipping, false otherwise
bool ClipLineToZPlanes(Vector3& start, Vector3& end, Camera* camera) {
	//clip to the near plane
	Vector3 planePoint = Vector3(0, 0, camera->nearZ);
	Vector3 planeNormal = Vector3::FORWARD;
	float d = planeNormal.dot(planePoint);
	bool startBeyondPlane = planeNormal.dot(start) - d < 0;
	bool endBeyondPlane = planeNormal.dot(end) - d < 0;
	float t;
	if (startBeyondPlane && !endBeyondPlane) {
		start = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, t);
	} else if (!startBeyondPlane && endBeyondPlane) {
		end = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, t);
	} else if (startBeyondPlane && endBeyondPlane) {
		return false;
	}

	//clip to the far plane
	planePoint = Vector3(0, 0, camera->farZ);
	planeNormal = Vector3::BACK;
	d = planeNormal.dot(planePoint);
	startBeyondPlane = planeNormal.dot(start) - d < 0;
	endBeyondPlane = planeNormal.dot(end) - d < 0;
	if (startBeyondPlane && !endBeyondPlane) {
		start = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, t);
	} else if (!startBeyondPlane && endBeyondPlane) {
		end = Math::VectorPlaneIntersect(planePoint, planeNormal, start, end, t);
	} else if (startBeyondPlane && endBeyondPlane) {
		return false;
	}
	return true;
} //ClipLineToZPlanes

//cohen-sutherland algorithm https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm
//the input vectors should be in screen space
//returns true if the line can be rendered after clipping, false otherwise
bool ClipLineToBorderPlanes(Vector3& start, Vector3& end, Screen* screen) {
	//clip to the vertical and horizontal planes
	const int CLIP_INSIDE = 0;
	const int CLIP_LEFT = 1;
	const int CLIP_RIGHT = 2;
	const int CLIP_BOTTOM = 4;
	const int CLIP_TOP = 8;
	auto ComputeOutCode = [&](Vector3& vertex) {
		int code = CLIP_INSIDE;
		if (vertex.x < 0) {
			code |= CLIP_LEFT;
		}
		else if (vertex.x > screen->width) {
			code |= CLIP_RIGHT;
		}
		if (vertex.y < 0) { //these are inverted because we are in screen space
			code |= CLIP_TOP;
		}
		else if (vertex.y > screen->height) {
			code |= CLIP_BOTTOM;
		}
		return code;
	};

	int lineStartCode = ComputeOutCode(start);
	int lineEndCode = ComputeOutCode(end);

	//loop until all points are within or outside the screen zone
	while (true) {
		if (!(lineStartCode | lineEndCode)) {
			//both points are inside the screen zone
			return true;
		}
		else if (lineStartCode & lineEndCode) {
			//both points are in the same outside zone
			return false;
		}
		else {
			float x, y;
			//select one of the points outside
			int code = lineEndCode > lineStartCode ? lineEndCode : lineStartCode;

			//clip the points the the screen bounds by finding the intersection point
			if			(code & CLIP_TOP) {		//point is above screen
				x = start.x + (end.x - start.x) * (-start.y) / (end.y - start.y);
				y = 0;
			} 
			else if	(code & CLIP_BOTTOM) {		//point is below screen
				x = start.x + (end.x - start.x) * (screen->height - start.y) / (end.y - start.y);
				y = screen->height;
			} 
			else if	(code & CLIP_RIGHT) {		//point is right of screen
				y = start.y + (end.y - start.y) * (screen->width - start.x) / (end.x - start.x);
				x = screen->width;
			} 
			else if	(code & CLIP_LEFT) {		//point is left of screen
				y = start.y + (end.y - start.y) * (-start.x) / (end.x - start.x);
				x = 0;
			}

			//update the vector's points and restart loop
			if (code == lineStartCode) {
				start.x = x;
				start.y = y;
				lineStartCode = ComputeOutCode(start);
			}
			else {
				end.x = x;
				end.y = y;
				lineEndCode = ComputeOutCode(end);
			}
		}
	}
} //ClipLineToBorderPlanes

int RenderLines(Scene* scene, Camera* camera, Screen* screen) {
	int out = 0;
	for(Edge3D* l : scene->lines) {
	//convert vertexes from world to camera/viewMatrix space
		Vector3 startVertex = Math::WorldToCamera(l->p[0], camera->viewMatrix).ToVector3();
		Vector3 endVertex = Math::WorldToCamera(l->p[1], camera->viewMatrix).ToVector3();

	//clip vertexes to the near and far z planes in camera/viewMatrix space
		if (!ClipLineToZPlanes(startVertex, endVertex, camera)) { continue; }

	//convert vertexes from camera/viewMatrix space to clip space
		startVertex = Math::CameraToScreen(startVertex, camera->projectionMatrix, screen->dimensions);
		endVertex = Math::CameraToScreen(endVertex, camera->projectionMatrix, screen->dimensions);

	//clip vertexes to border planes in clip space
		if (!ClipLineToBorderPlanes(startVertex, endVertex, screen)) { continue; }

	//draw the lines after all clipping and space conversion
		++out;
		p->DrawLine(startVertex.ToVector2(), endVertex.ToVector2(), ((RenderedEdge3D*)l)->color);
	}
	return out;
} //RenderLines


//this (probably) generates a light's depth texture for use in shadow casting
//currently set up to work for directional lights
void LightDepthTex(Light* li, Camera* c, Scene* s, Screen* sc) {
	int resx = 1024;
	int resy = 1024; //resolution of depth texture
	li->depthTexture = std::vector<int>((size_t)resx * (size_t)resy);
	
	//once we implement point lights we'll start making a bounding box around
	//only what the camera sees but for now this is how it works with ortho projection
	std::pair<Vector3, Vector3> bbox = s->SceneBoundingBox();

	Matrix4 lightViewMat = Math::LookAtMatrix(li->position, li->direction);

	//convert bounding box to light's "view" space
	Vector3 maxcam = Math::WorldToCamera(bbox.first,  lightViewMat).ToVector3();
	Vector3 mincam = Math::WorldToCamera(bbox.second, lightViewMat).ToVector3();

	//make screen box from camera space bounding box
	float maxx = std::max(fabs(mincam.x), fabs(maxcam.x));
	float maxy = std::max(fabs(mincam.y), fabs(maxcam.y));
	float max = std::max(maxx, maxy);

	float aspectRatio = sc->height / sc->width;
	float r = max * aspectRatio, t = max;
	float l = -r, b = -t;

	//make ortho proj mat for directional light
	//NOTE: ortho projection may not work properly yet
	Matrix4 ortho = Math::MakeOrthoProjMat(r, l, t, b, c->farZ, c->nearZ);

	//std::vector<Triangle> tris;

	Vector2 res(resx, resy);

	//render the scene with respect to the light
	//what im dloing is just projecting each triangle and rendering them instead
	//of projecting them each individually. i dont know if this is more efficient or not
	for (Mesh* m : s->meshes) {
		for (Triangle tri : m->triangles) {

			//project triangle to light's 'view'
			tri.copy_points();
			for (Vector3 v : tri.proj_points) {
				v = Math::WorldToCamera(v, lightViewMat).ToVector3();
				//LOG(v);
				
				
			}

			//project triangle to light's 'screen'
			for (int i = 0; i < 3; i++) {
				float w;
				tri.proj_points[i] = Math::CameraToScreen(tri.proj_points[i], ortho, res, w);
				tri.proj_tex_points[i].x /= w;
				tri.proj_tex_points[i].y /= w;
				tri.proj_tex_points[i].z = 1.f / w;
				//LOG(w);
			}

			//gather world vertices to lerp between later
			Vector3 ve1 = tri.points[0];
			Vector3 ve2 = tri.points[1];
			Vector3 ve3 = tri.points[2];
			
			int x1 = tri.proj_points[0].x; int x2 = tri.proj_points[1].x; int x3 = tri.proj_points[2].x;
			int y1 = tri.proj_points[0].y; int y2 = tri.proj_points[1].y; int y3 = tri.proj_points[2].y;

			float u1 = tri.proj_tex_points[0].x; float u2 = tri.proj_tex_points[1].x; float u3 = tri.proj_tex_points[2].x;
			float v1 = tri.proj_tex_points[0].y; float v2 = tri.proj_tex_points[1].y; float v3 = tri.proj_tex_points[2].y;
			float w1 = tri.proj_tex_points[0].z; float w2 = tri.proj_tex_points[1].z; float w3 = tri.proj_tex_points[2].z;

			if (y2 < y1) { std::swap(y1, y2); std::swap(x1, x2); std::swap(u1, u2); std::swap(v1, v2); std::swap(w1, w2); }
			if (y3 < y1) { std::swap(y1, y3); std::swap(x1, x3); std::swap(u1, u3); std::swap(v1, v3); std::swap(w1, w3); }
			if (y3 < y2) { std::swap(y2, y3); std::swap(x2, x3); std::swap(u2, u3); std::swap(v2, v3); std::swap(w2, w3); }

			//sort world vertices so ve1 is highest and ve3 is lowest
			if (ve2.y < ve1.y) { std::swap(ve1, ve2); }
			if (ve3.y < ve1.y) { std::swap(ve1, ve3); }
			if (ve3.y < ve2.y) { std::swap(ve2, ve3); }

			int dy1 = y2 - y1;
			int dx1 = x2 - x1;
			float dv1 = v2 - v1;
			float du1 = u2 - u1;
			float dw1 = w2 - w1;

			int dy2 = y3 - y1;
			int dx2 = x3 - x1;
			float dv2 = v3 - v1;
			float du2 = u3 - u1;
			float dw2 = w3 - w1;

			float tex_u, tex_v, tex_w;

			float	dax_step = 0, dbx_step = 0,
					du1_step = 0, dv1_step = 0,
					du2_step = 0, dv2_step = 0,
					dw1_step = 0, dw2_step = 0;

			if (dy1) dax_step = dx1 / (float)abs(dy1);
			if (dy2) dbx_step = dx2 / (float)abs(dy2);

			if (dy1) du1_step = du1 / (float)abs(dy1);
			if (dy1) dv1_step = dv1 / (float)abs(dy1);
			if (dy1) dw1_step = dw1 / (float)abs(dy1);

			if (dy2) du2_step = du2 / (float)abs(dy2);
			if (dy2) dv2_step = dv2 / (float)abs(dy2);
			if (dy2) dw2_step = dw2 / (float)abs(dy2);

			if (dy1) {
				for (int i = y1; i <= y2; i++) {
					int ax = x1 + (float)(i - y1) * dax_step;
					int bx = x1 + (float)(i - y1) * dbx_step;

					float tex_su = u1 + (float)(i - y1) * du1_step;
					float tex_sv = v1 + (float)(i - y1) * dv1_step;
					float tex_sw = w1 + (float)(i - y1) * dw1_step;

					float tex_eu = u1 + (float)(i - y1) * du2_step;
					float tex_ev = v1 + (float)(i - y1) * dv2_step;
					float tex_ew = w1 + (float)(i - y1) * dw2_step;

					if (ax > bx) {
						std::swap(ax, bx);
						std::swap(tex_su, tex_eu);
						std::swap(tex_sv, tex_ev);
						std::swap(tex_sw, tex_ew);
					}

					tex_u = tex_su;
					tex_v = tex_sv;
					tex_w = tex_sw;

					float tstep = 1.0f / ((float)(bx - ax));
					float t = 0.0f;

					for (int j = ax; j < bx; j++) {
						tex_u = (1.0f - t) * tex_su + t * tex_eu;
						tex_v = (1.0f - t) * tex_sv + t * tex_ev;
						tex_w = (1.0f - t) * tex_sw + t * tex_ew;

						//use the interpolation to find the point the pixel is drawing in world space 
						//Vector3 worldp()


						if (tex_w > li->depthTexture[i * (size_t)resx + j]) {
							li->depthTexture[i * (size_t)resx + j] = tex_w;
							//LOG(tex_w);
						}
						t += tstep;
					}
				}
			}

			dy1 = y3 - y2;
			dx1 = x3 - x2;
			dv1 = v3 - v2;
			du1 = u3 - u2;
			dw1 = w3 - w2;

			if (dy1) dax_step = dx1 / (float)abs(dy1);
			if (dy2) dbx_step = dx2 / (float)abs(dy2);

			du1_step = 0, dv1_step = 0;
			if (dy1) du1_step = du1 / (float)abs(dy1);
			if (dy1) dv1_step = dv1 / (float)abs(dy1);
			if (dy1) dw1_step = dw1 / (float)abs(dy1);

			if (dy1) {
				for (int i = y2; i <= y3; i++) {
					int ax = x2 + (float)(i - y2) * dax_step;
					int bx = x1 + (float)(i - y1) * dbx_step;

					float tex_su = u2 + (float)(i - y2) * du1_step;
					float tex_sv = v2 + (float)(i - y2) * dv1_step;
					float tex_sw = w2 + (float)(i - y2) * dw1_step;

					float tex_eu = u1 + (float)(i - y1) * du2_step;
					float tex_ev = v1 + (float)(i - y1) * dv2_step;
					float tex_ew = w1 + (float)(i - y1) * dw2_step;

					if (ax > bx) {
						std::swap(ax, bx);
						std::swap(tex_su, tex_eu);
						std::swap(tex_sv, tex_ev);
						std::swap(tex_sw, tex_ew);
					}

					tex_u = tex_su;
					tex_v = tex_sv;
					tex_w = tex_sw;

					float tstep = 1.0f / ((float)(bx - ax));
					float t = 0.0f;

					for (int j = ax; j < bx; j++) {
						tex_u = (1.0f - t) * tex_su + t * tex_eu;
						tex_v = (1.0f - t) * tex_sv + t * tex_ev;
						tex_w = (1.0f - t) * tex_sw + t * tex_ew;

						if (tex_w > li->depthTexture[i * (size_t)resx + j]) {
							li->depthTexture[i * (size_t)resx + j] = tex_w;
							//LOG(tex_w);
						}
						t += tstep;
					}
				}
			}
		}
	}



}

void RenderSceneSystem::Update() {
	Scene* scene = admin->currentScene;
	Camera* camera = admin->currentCamera;
	Input* input = admin->input;
	Keybinds* binds = admin->currentKeybinds;
	Screen* screen = admin->screen;
	 = admin->p;

//// Scene Manangement ////

	//reset the scene
	scene->pixelDepthBuffer = std::vector<float>((size_t)screen->width * (size_t)screen->height);
	scene->meshes.clear();
	for(auto l : scene->lights) { if(!l->entity) delete l; }
	scene->lights.clear();
	for(Edge3D* l : scene->lines) { if(!l->e) delete l; }
	scene->lines.clear();

	//collect all meshes and transform lines
	int totalTriCount = 0;
	std::vector<std::pair<Vector2, std::string>> render_transforms;
	for(auto pair : admin->entities) {
		for(Component* comp : pair.second->components) {
			if(Mesh* mesh = dynamic_cast<Mesh*>(comp)) {
				scene->meshes.push_back(mesh);
				totalTriCount += mesh->triangles.size();
			}
			/*if(SpriteRenderer* sr = dynamic_cast<SpriteRenderer*>(comp)) { //idea for 2d drawing
			
			}*/
			if(Transform* t = dynamic_cast<Transform*>(comp)) {
				if(scene->RENDER_LOCAL_AXIS) {
					scene->lines.push_back(new RenderedEdge3D(t->position, t->position + t->Right(), Color::RED));
					scene->lines.push_back(new RenderedEdge3D(t->position, t->position + t->Up(), Color::GREEN));
					scene->lines.push_back(new RenderedEdge3D(t->position, t->position + t->Forward(), Color::BLUE));
				}
				if(scene->RENDER_TRANSFORMS) {
					Vector2 pos = Math::WorldToScreen2D(t->position, camera->projectionMatrix, camera->viewMatrix, screen->dimensions);
					render_transforms.push_back(std::make_pair(pos, t->position.str2f()));
					render_transforms.push_back(std::make_pair(pos + Vector2(0, 10), t->rotation.str2f()));
				}
			}
			if(scene->RENDER_PHYSICS) {
				if(Physics* phys = dynamic_cast<Physics*>(comp)) {
					scene->lines.push_back(new RenderedEdge3D(phys->position + phys->velocity, phys->position, Color::DARK_MAGENTA));
					scene->lines.push_back(new RenderedEdge3D(phys->position + phys->acceleration, phys->position, Color::DARK_YELLOW));
				}
			}
		}
	}

	scene->lights.push_back(new Light(Vector3(0, 1.5, 1), Vector3(0, 0, 1))); //TODO replace this with light components on entities

//// Scene Rendering ////

	//render world grid
	if(scene->RENDER_GRID) {
		for(int i = -20; i < 21; ++i) {
			scene->lines.push_back(new RenderedEdge3D(Vector3(-100, 0, i*5), Vector3(100, 0, i*5), Color::GREY));
			scene->lines.push_back(new RenderedEdge3D(Vector3(i*5, 0, -100), Vector3(i*5, 0, 100), Color::GREY));
		}
		scene->lines.push_back(new RenderedEdge3D(Vector3(-100, 0, 0), Vector3(100, 0, 0), Color::RED));
		scene->lines.push_back(new RenderedEdge3D(Vector3(0, 0, -100), Vector3(0, 0, 100), Color::BLUE));
	}

	//render light rays
	if(scene->RENDER_LIGHT_RAYS) {
		for(Light* l : scene->lights) {
			scene->lines.push_back(new RenderedEdge3D(l->position, l->position + (l->direction * l->strength), Color::YELLOW));
		}
	}

	//generate light's depth texture
	//for (Light* l : scene->lights) {
	//	LightDepthTex(l, camera, scene, screen);
	//}


	//render triangles
	int drawnTriCount = RenderTriangles(scene, camera, screen, p);

	//render lines
	int drawnLineCount = RenderLines(scene, camera, screen, p);

	//render light depth texture for debugin'
	//for (int i : scene->lights[0]->depthTexture) {
	//	p->Draw(Vector2(i / 1028, i % 1028), Color(50 * i, 50 * i, 50 * i));
	//}

	//render transform texts
	if(scene->RENDER_TRANSFORMS) {
		for(auto& pair : render_transforms) {
			p->DrawString(pair.first, pair.second);
		}
	}

	//render global axis in top right of screen
	if(scene->RENDER_GLOBAL_AXIS) {
		Vector2 zeroVertex = Math::WorldToScreen2D(Vector3::ZERO, camera->projectionMatrix, camera->viewMatrix, screen->dimensions);
		Vector2 yVertex =	 Math::WorldToScreen2D(Vector3::UP, camera->projectionMatrix, camera->viewMatrix, screen->dimensions);
		Vector2 xVertex =	 Math::WorldToScreen2D(Vector3::RIGHT, camera->projectionMatrix, camera->viewMatrix, screen->dimensions);
		Vector2 zVertex =	 Math::WorldToScreen2D(Vector3::FORWARD, camera->projectionMatrix, camera->viewMatrix, screen->dimensions);
		p->DrawLine(Vector2(screen->dimensions.x-50, 50), Vector2(screen->dimensions.x-50, 50) + (yVertex-zeroVertex).norm()*20, Color::GREEN);
		p->DrawLine(Vector2(screen->dimensions.x-50, 50), Vector2(screen->dimensions.x-50, 50) + (xVertex-zeroVertex).norm()*20, Color::RED);
		p->DrawLine(Vector2(screen->dimensions.x-50, 50), Vector2(screen->dimensions.x-50, 50) + (zVertex-zeroVertex).norm()*20, Color::BLUE);
	}

	p->DrawCircle(Math::WorldToScreen2D(scene->lights[0]->position, camera->projectionMatrix, camera->viewMatrix, screen->dimensions), 10);
	p->DrawStringDecal(Color::vf2d(screen->width-300, screen->height - 10), "Tri Total: " + std::to_string(totalTriCount) + "  Tri Drawn: " + std::to_string(drawnTriCount));

	if (admin->paused) {
		Vector2 tsize = p->GetTextSize("ENGINE PAUSED") * 5;
		Color col(30, 168, 150);
		static float tstep = 1;
		float modmod = cosf(4 * admin->time->totalTime);
		float mod = (sinf((4 * admin->time->totalTime) + modmod) + 1) / 2; //nice looking flash effect
		p->DrawStringDecal(Vector2(0, admin->screen->height - tsize.y), "ENGINE PAUSED", col * mod , Vector2(5, 5));
	}


} //Update
