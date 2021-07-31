#pragma once
#ifndef GAME_EDITOR_H
#define GAME_EDITOR_H

#include "UndoManager.h"
#include "../defines.h"
#include "../utils/color.h"
#include "../math/vector.h"

#include <vector>

struct Admin;
struct Entity;
struct Camera;

enum TransformationAxisBits{
	TransformationAxis_Free, TransformationAxis_X, TransformationAxis_Y, TransformationAxis_Z
}; typedef u32 TransformationAxis;

struct EditorSettings{
	
};

struct Editor{
	Admin* admin;
	EditorSettings settings;
	
	std::vector<Entity*> selected;
	Camera* camera;
	UndoManager undo_manager;
	
	std::string level_name;
	vec3 camera_pos;
	vec3 camera_rot;
	
	bool popoutInspector;
	
	bool showInspector;
	bool showTimes;
	bool showDebugBar;
	bool showMenuBar;
	bool showImGuiDemoWindow;
	bool showDebugLayer;
	bool showWorldGrid;
	bool ConsoleHovFlag; //this can be done better
	
	void Init(Admin* a);
	void Update();
	void Reset();
	void Cleanup();
	
	//TODO(delle,Cl) move these to be local inside the .cpp
	void TranslateEntity(Entity* e, TransformationAxis axis);
	void RotateEntity(Entity* e, TransformationAxis axis);
	
	void MenuBar();
	void DebugLayer();
	void DebugBar();
	void Inspector();
	void DrawTimes();
	void WorldGrid(Vector3 cpos);
	void ShowWorldAxis();
	void ShowSelectedEntityNormals();
};

namespace ImGui {
	void BeginDebugLayer();
	void EndDebugLayer(); 
	void DebugDrawCircle(Vector2 pos, float radius, Color color = Color::WHITE);
	void DebugDrawCircle3(Vector3 pos, float radius, Color color = Color::WHITE);
	void DebugDrawCircleFilled3(Vector3 pos, float radius, Color color = Color::WHITE);
	void DebugDrawLine(Vector2 pos1, Vector2 pos2, Color color = Color::WHITE);
	void DebugDrawLine3(Vector3 pos1, Vector3 pos2, Color color = Color::WHITE);
	void DebugDrawText(const char* text, Vector2 pos, Color color = Color::WHITE);
	void DebugDrawText3(const char* text, Vector3 pos, Color color = Color::WHITE, Vector2 twoDoffset = Vector2::ZERO);
	void DebugDrawTriangle(Vector2 p1, Vector2 p2, Vector2 p3, Color color = Color::WHITE);
	void DebugFillTriangle(Vector2 p1, Vector2 p2, Vector2 p3, Color color = Color::WHITE);
	void DebugDrawTriangle3(Vector3 p1, Vector3 p2, Vector3 p3, Color color = Color::WHITE);
	void DebugFillTriangle3(Vector3 p1, Vector3 p2, Vector3 p3, Color color = Color::WHITE);
	void DebugDrawGraphFloat(Vector2 pos, float inval, float sizex = 100, float sizey = 100);
	void AddPadding(float x);
}

#endif //GAME_EDITOR_H
