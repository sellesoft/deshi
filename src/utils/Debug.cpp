#include "Debug.h"
#include "../core.h"

#include "../math/Math.h"
#include "Color.h"

void Debug::DrawLine(Vector3 v1, Vector3 v2, int unique, Color color = Color::WHITE) {
	
	if (meshes.find(unique) != meshes.end()) {
		u32 id = meshes[unique].meshID;
		meshes[unique].calledThisFrame = true;
		DengRenderer->UpdateDebugLine(id, v1, v2, color);
	}
	else {
		MeshInfo mi;
		mi.meshID = DengRenderer->CreateDebugLine(v1, v2, color, true);
		mi.idleTime = 0;
		mi.allowedTime = -1;
		mi.calledThisFrame = true;
		meshes[unique] = mi;
	}
}

void Debug::DrawLine(Vector3 v1, Vector3 v2, int unique, float time = -2, Color color = Color::WHITE) {
	if (meshes.find(unique) != meshes.end()) {
		u32 id = meshes[unique].meshID;
		meshes[unique].calledThisFrame = true;
		DengRenderer->UpdateDebugLine(id, v1, v2, color);

	}
	else {
		MeshInfo mi;
		mi.meshID = DengRenderer->CreateDebugLine(v1, v2, color, true);
		mi.idleTime = 0;
		mi.allowedTime = time;
		mi.calledThisFrame = true;
		meshes[unique] = mi;
	}
}

void Debug::DrawLine(int i, Vector3 v1, Vector3 v2, int unique, Color color = Color::WHITE) {
	if (i == -1) { //if i = -1 then the user is requesting a new line everytime the function is called, regardless of a loop
		unique = unique << miter;
		miter++;
	}
	else {
		unique = unique << i;
	}
	
	if (meshes.find(unique) != meshes.end()) {
		u32 id = meshes[unique].meshID;
		meshes[unique].calledThisFrame = true;
		DengRenderer->UpdateDebugLine(id, v1, v2, color);

	}
	else {
		MeshInfo mi;
		mi.meshID = DengRenderer->CreateDebugLine(v1, v2, color, true);
		mi.idleTime = 0;
		mi.allowedTime = -1;
		mi.calledThisFrame = true;
		meshes[unique] = mi;
	}
}

void Debug::DrawLine(int i, Vector3 v1, Vector3 v2, int unique, float time = -2, Color color = Color::WHITE) {
	int uniqueInt;; //this has massive potential to break something hehe
	if (i == -1) { //if i = -1 then the user is requesting a new line everytime the function is called, regardless of a loop
		unique = unique << miter;
		miter++;
	}
	else {
		unique = unique << i;
	}

	if (meshes.find(unique) != meshes.end()) {
		u32 id = meshes[unique].meshID;
		meshes[unique].calledThisFrame = true;
		DengRenderer->UpdateDebugLine(id, v1, v2, color);
	}
	else {
		MeshInfo mi;
		mi.meshID = DengRenderer->CreateDebugLine(v1, v2, color, true);
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