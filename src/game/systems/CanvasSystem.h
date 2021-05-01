#pragma once
#ifndef SYSTEM_CANVAS_H
#define SYSTEM_CANVAS_H

#include "../../utils/defines.h"
#include "../../utils/color.h"
#include "../../math/vector.h"

struct EntityAdmin;

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
}

struct CanvasSystem {
	EntityAdmin* admin;
	bool showDebugTools;
	bool showDebugBar;
	bool showMenuBar;
	bool showImGuiDemoWindow;
	bool showDebugLayer;
	bool ConsoleHovFlag; //this can be done better
	
	void Init(EntityAdmin* admin);
	void Update();
	
	void MenuBar();
	void DebugLayer();
	void DebugBar();
	void DebugTools();
};

#endif //SYSTEM_CANVAS_H