#pragma once
#ifndef DESHI_CONSOLE_H
#define DESHI_CONSOLE_H

#include "../utils/defines.h"
#include "../utils/Color.h"

#include <vector>
#include <string>     // std::string, std::stoi
#include <map>

struct ImGuiInputTextCallbackData;
struct Input;
struct Window;
struct Time;
struct EntityAdmin;
struct Command;

struct Console  {
	
	Console() {}
	
	Time* time;
	Input* input;
	Window* window;
	EntityAdmin* admin;
	
	std::map<std::string, Command*> commands;
	
	char inputBuf[256]{};
	std::vector<std::pair<std::string, Color>> buffer; //text, color
	std::vector<std::string> history;
	int historyPos = -1;
	
	bool dispcon = false;
	bool autoScroll = true;
	bool scrollToBottom = false;
	
	void Init(Time* t, Input* i, Window* w, EntityAdmin* ea);
	void Update();
	void DrawConsole();
	void PushConsole(std::string s);
	void AddLog(std::string input);
	void FlushBuffer();
	std::string ExecCommand(std::string command, std::string args);
	int TextEditCallback(ImGuiInputTextCallbackData* data);
	static int TextEditCallbackStub(ImGuiInputTextCallbackData* data);
	
	void AddRenderCommands();
	void AddCameraCommands();
	void AddConsoleCommands();
	void AddSelectedEntityCommands();
	void AddWindowCommands();
	void AddAliases();
	
	void CleanUp();
};

//global console pointer
extern Console* g_console;

#define LOG(...)     g_console->PushConsole(TOSTRING("[c:yellow]", __VA_ARGS__, "[c]"))
#define ERROR(...)   g_console->PushConsole(TOSTRING("[c:error]", __VA_ARGS__, "[c]"))
#define SUCCESS(...) g_console->PushConsole(TOSTRING("[c:green]", __VA_ARGS__, "[c]"))
//#define PRINT(...)   admin->GetSystem<Console>()->PushConsole(TOSTRING(__VA_ARGS__))

//additionally prints where function was called
#define LOG_LOC(...)     g_console->PushConsole(TOSTRING("[c:yellow]In ", __FILENAME__, " at ", __LINE__ , ": \n[c]", "[c:yellow]", __VA_ARGS__, "[c]"))
#define ERROR_LOC(...)   g_console->PushConsole(TOSTRING("[c:error]In ", __FILENAME__, " at ", __LINE__, ": \n[c]", "[c:error]", __VA_ARGS__, "[c]"))
#define SUCCESS_LOC(...) g_console->PushConsole(TOSTRING("[c:green]In ", __FILENAME__, " at ", __LINE__, ": \n[c]", "[c:green]", __VA_ARGS__, "[c]")

#endif //DESHI_CONSOLE_H