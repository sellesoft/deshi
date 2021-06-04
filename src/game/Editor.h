#pragma once
#ifndef GAME_EDITOR_H
#define GAME_EDITOR_H
#include "../utils/defines.h"

#include "UndoManager.h"
#include "../utils/color.h"
#include "../math/vector.h"

#include <vector>

struct EntityAdmin;
struct Entity;
struct Camera;

enum TransformationAxis : u32{
	TransformationAxis_Free, TransformationAxis_X, TransformationAxis_Y, TransformationAxis_Z
};

struct EditorSettings{
	b32 fast_outline;
};

struct Editor{
	EntityAdmin* admin;
	EditorSettings settings;
	
	std::vector<Entity*> selected;
	Camera* camera;
	UndoManager undo_manager;
	std::string level_name;
	
	bool showDebugTools;
	bool showTimes;
	bool showDebugBar;
	bool showMenuBar;
	bool showImGuiDemoWindow;
	bool showDebugLayer;
	bool ConsoleHovFlag; //this can be done better
	
	void Init(EntityAdmin* a);
	void Update();
	void Reset();
	
	//TODO(delle,Cl) move these to be local inside the .cpp
	Entity* SelectEntityRaycast();
	void TranslateEntity(Entity* e, TransformationAxis axis);
	void RotateEntity(Entity* e, TransformationAxis axis);
	
	void MenuBar();
	void DebugLayer();
	void DebugBar();
	void DebugTools();
	void DrawTimes();
	void WorldGrid(Vector3 cpos);
	void ShowSelectedEntityNormals();
};

namespace ImGui {
	void BeginDebugLayer();
	void EndDebugLayer(); 
	void DebugDrawCircle(Vector2 pos, float radius, Color color = Color::WHITE);
	void DebugDrawCircle3(Vector3 pos, float radius, Color color = Color::WHITE);
	void DebugDrawLine(Vector2 pos1, Vector2 pos2, Color color = Color::WHITE);
	void DebugDrawLine3(Vector3 pos1, Vector3 pos2, Color color = Color::WHITE);
	void DebugDrawText(const char* text, Vector2 pos, Color color = Color::WHITE);
	void DebugDrawText3(const char* text, Vector3 pos, Color color = Color::WHITE, Vector2 twoDoffset = Vector2::ZERO);
	void DebugDrawTriangle(Vector2 p1, Vector2 p2, Vector2 p3, Color color = Color::WHITE);
	void DebugFillTriangle(Vector2 p1, Vector2 p2, Vector2 p3, Color color = Color::WHITE);
	void DebugDrawTriangle3(Vector3 p1, Vector3 p2, Vector3 p3, Color color = Color::WHITE);
	void DebugFillTriangle3(Vector3 p1, Vector3 p2, Vector3 p3, Color color = Color::WHITE);
	void DebugDrawGraphFloat(Vector2 pos, float inval, float sizex = 100, float sizey = 100);
	void CopyButton(const char* text);
	bool InputVector2(const char* id, Vector2* vecPtr, bool inputUpdate = false);
	bool InputVector3(const char* id, Vector3* vecPtr, bool inputUpdate = false);
	bool InputVector4(const char* id, Vector4* vecPtr, bool inputUpdate = false);
	void AddPadding(float x);
}

#endif //GAME_EDITOR_H
