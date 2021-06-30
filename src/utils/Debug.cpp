#include "Debug.h"
#include "Color.h"
#include "../core/renderer.h"
#include "../core/time.h"
#include "../math/Math.h"


void Debug::DrawLine(Vector3 v1, Vector3 v2, size_t unique, Color color = Color::WHITE) {
	
	if (meshes.find(unique) != meshes.end()) {
		u32 id = meshes[unique].meshID;
		meshes[unique].calledThisFrame = true;
		if(*meshes[unique].last[0] != v1 || *meshes[unique].last[1] != v2){
			DengRenderer->UpdateDebugLine(id, v1, v2, color);
			//meshes[unique].last[0] = v1;
			memcpy(meshes[unique].last[0], (void*)&v1, sizeof(Vector3));
			//meshes[unique].last[1] = v2;
			memcpy(meshes[unique].last[1], (void*)&v2, sizeof(Vector3));
		}
	}
	else {
		MeshInfo mi;
		mi.last[0] = new Vector3();
		mi.last[1] = new Vector3();
		mi.meshID = DengRenderer->CreateDebugLine(v1, v2, color, true);
		mi.idleTime = 0;
		mi.allowedTime = -1;
		mi.calledThisFrame = true;
		meshes[unique] = mi;
		//meshes[unique].last[0] = v1;
		memcpy(meshes[unique].last[0], (void*)&v1, sizeof(Vector3));
		//meshes[unique].last[1] = v2;
		memcpy(meshes[unique].last[1], (void*)&v2, sizeof(Vector3));
	}
}

void Debug::DrawLine(Vector3 v1, Vector3 v2, size_t unique, float time = -2, Color color = Color::WHITE) {
	if (meshes.find(unique) != meshes.end()) {
		u32 id = meshes[unique].meshID;
		meshes[unique].calledThisFrame = true;
		if(*meshes[unique].last[0] != v1 || *meshes[unique].last[1] != v2){
			DengRenderer->UpdateDebugLine(id, v1, v2, color);
			//meshes[unique].last[0] = v1;
			memcpy(meshes[unique].last[0], (void*)&v1, sizeof(Vector3));
			//meshes[unique].last[1] = v2;
			memcpy(meshes[unique].last[1], (void*)&v2, sizeof(Vector3));
		}
	}
	else {
		MeshInfo mi;
		mi.last[0] = new Vector3();
		mi.last[1] = new Vector3();
		mi.meshID = DengRenderer->CreateDebugLine(v1, v2, color, true);
		mi.idleTime = 0;
		mi.allowedTime = time;
		mi.calledThisFrame = true;
		meshes[unique] = mi;
		//meshes[unique].last[0] = v1;
		memcpy(meshes[unique].last[0], (void*)&v1, sizeof(Vector3));
		//meshes[unique].last[1] = v2;
		memcpy(meshes[unique].last[1], (void*)&v2, sizeof(Vector3));
	}
}

void Debug::DrawLine(int i, Vector3 v1, Vector3 v2, size_t unique, Color color = Color::WHITE) {
	if (i == -1) { //if i = -1 then the user is requesting a new line everytime the function is called, regardless of a loop
		unique = unique + miter;
		miter++;
	}
	else {
		unique = unique + i;
	}
	
	if (meshes.find(unique) != meshes.end()) {
		u32 id = meshes[unique].meshID;
		meshes[unique].calledThisFrame = true;
		if(*meshes[unique].last[0] != v1 || *meshes[unique].last[1] != v2){
			DengRenderer->UpdateDebugLine(id, v1, v2, color);
			//meshes[unique].last[0] = v1;
			memcpy(meshes[unique].last[0], (void*)&v1, sizeof(Vector3));
			//meshes[unique].last[1] = v2;
			memcpy(meshes[unique].last[1], (void*)&v2, sizeof(Vector3));
		}
		else {
			PRINTLN("dupe");
		}
		
	}
	else {
		MeshInfo mi;
		mi.last[0] = new Vector3();
		mi.last[1] = new Vector3();
		mi.meshID = DengRenderer->CreateDebugLine(v1, v2, color, true);
		mi.idleTime = 0;
		mi.allowedTime = -1;
		mi.calledThisFrame = true;
		meshes[unique] = mi;
		//meshes[unique].last[0] = v1;
		memcpy(meshes[unique].last[0], (void*)&v1, sizeof(Vector3));
		//meshes[unique].last[1] = v2;
		memcpy(meshes[unique].last[1], (void*)&v2, sizeof(Vector3));
	}
}

void Debug::DrawLine(int i, Vector3 v1, Vector3 v2, size_t unique, float time = -2, Color color = Color::WHITE) {
	int uniqueInt;; //this has massive potential to break something hehe
	if (i == -1) { //if i = -1 then the user is requesting a new line everytime the function is called, regardless of a loop
		unique = unique + miter;
		miter++;
	}
	else {
		unique = unique + i;
	}
	
	if (meshes.find(unique) != meshes.end()) {
		u32 id = meshes[unique].meshID;
		meshes[unique].calledThisFrame = true;
		if (*meshes[unique].last[0] != v1 || *meshes[unique].last[1] != v2) {
			DengRenderer->UpdateDebugLine(id, v1, v2, color);
			//meshes[unique].last[0] = v1;
			memcpy(meshes[unique].last[0], (void*)&v1, sizeof(Vector3));
			//meshes[unique].last[1] = v2;
			memcpy(meshes[unique].last[1], (void*)&v2, sizeof(Vector3));
		}
	}
	else {
		MeshInfo mi;
		mi.last[0] = new Vector3();
		mi.last[1] = new Vector3();
		mi.meshID = DengRenderer->CreateDebugLine(v1, v2, color, true);
		mi.idleTime = 0;
		mi.allowedTime = time;
		mi.calledThisFrame = true;
		meshes[unique] = mi;
		//meshes[unique].last[0] = v1;
		memcpy(meshes[unique].last[0], (void*)&v1, sizeof(Vector3));
		//meshes[unique].last[1] = v2;
		memcpy(meshes[unique].last[1], (void*)&v2, sizeof(Vector3));
	}
}

void Debug::DrawMesh(Mesh* mesh, Matrix4 transform, size_t unique, Color color = Color::WHITE) {
	if (meshes.find(unique) != meshes.end()) {
		u32 id = meshes[unique].meshID;
		meshes[unique].calledThisFrame = true;
		DengRenderer->UpdateMeshBrushMatrix(id, transform);
	}
	else {
		MeshInfo mi;
		mi.meshID = DengRenderer->CreateMeshBrush(mesh, transform);
		mi.idleTime = 0;
		mi.allowedTime = -1;
		mi.calledThisFrame = true;
		meshes[unique] = mi;
	}
}

void Debug::DrawMesh(Mesh* mesh, Matrix4 transform, size_t unique, float time = -2, Color color = Color::WHITE) {
	if (meshes.find(unique) != meshes.end()) {
		u32 id = meshes[unique].meshID;
		meshes[unique].calledThisFrame = true;
		DengRenderer->UpdateMeshBrushMatrix(id, transform);
	}
	else {
		MeshInfo mi;
		mi.meshID = DengRenderer->CreateMeshBrush(mesh, transform);
		mi.idleTime = 0;
		mi.allowedTime = -1;
		mi.calledThisFrame = true;
		meshes[unique] = mi;
	}
}

void Debug::DrawMesh(int i, Mesh* mesh, Matrix4 transform, size_t unique, Color color = Color::WHITE) {
	if (i == -1) { //if i = -1 then the user is requesting a new line everytime the function is called, regardless of a loop
		unique = unique + miter;
		miter++;
	}
	else {
		unique = unique + i;
	}
	
	if (meshes.find(unique) != meshes.end()) {
		u32 id = meshes[unique].meshID;
		meshes[unique].calledThisFrame = true;
		DengRenderer->UpdateMeshBrushMatrix(id, transform);
	}
	else {
		MeshInfo mi;
		mi.meshID = DengRenderer->CreateMeshBrush(mesh, transform);
		mi.idleTime = 0;
		mi.allowedTime = -1;
		mi.calledThisFrame = true;
		meshes[unique] = mi;
	}
}

void Debug::DrawMesh(int i, Mesh* mesh, Matrix4 transform, size_t unique, float time = -2, Color color = Color::WHITE) {
	if (i == -1) { //if i = -1 then the user is requesting a new line everytime the function is called, regardless of a loop
		unique = unique + miter;
		miter++;
	}
	else {
		unique = unique + i;
	}
	
	if (meshes.find(unique) != meshes.end()) {
		u32 id = meshes[unique].meshID;
		meshes[unique].calledThisFrame = true;
		DengRenderer->UpdateMeshBrushMatrix(id, transform);
	}
	else {
		MeshInfo mi;
		mi.meshID = DengRenderer->CreateMeshBrush(mesh, transform);
		mi.idleTime = 0;
		mi.allowedTime = time;
		mi.calledThisFrame = true;
		meshes[unique] = mi;
	}
}

void Debug::Update() {
	
	for (auto& c : meshes) {
		MeshInfo* mi = &c.second;
		if (!mi->calledThisFrame) {
			mi->idleTime += DengTime->deltaTime;
			if (mi->allowedTime == -1) { /*indefinite*/ }
			else if (mi->allowedTime == -2) {
				if (mi->clearNextFrame) DengRenderer->UpdateMeshBrushVisibility(mi->meshID, false);
				else mi->clearNextFrame = true;
			}
			else if (mi->idleTime < mi->allowedTime) {
				if (mi->wasInvis) { DengRenderer->UpdateMeshBrushVisibility(mi->meshID, true);  mi->wasInvis = false; }
			}
			else {
				if (!mi->wasInvis) { DengRenderer->UpdateMeshBrushVisibility(mi->meshID, false);  mi->wasInvis = true; }
			}
		}
		else {
			mi->idleTime = 0;
			if (mi->wasInvis) { DengRenderer->UpdateMeshBrushVisibility(mi->meshID, true); mi->wasInvis = false; }
			mi->calledThisFrame = false;
			mi->clearNextFrame = true;
		}
	}
}