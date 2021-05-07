#pragma once
#ifndef DESHI_CONSOLE_H
#define DESHI_CONSOLE_H

#include "../utils/defines.h"
#include "../utils/Color.h"

#include <vector>
#include <string>     // std::string, std::stoi
#include <map>

struct ImGuiInputTextCallbackData;
struct Command;

struct Console  {
	std::map<std::string, Command*> commands;
	
	char inputBuf[256]{};
	std::vector<std::pair<std::string, Color>> buffer; //text, color
	std::vector<std::string> history;
	int historyPos = -1;
	
	bool dispcon = false;
	bool autoScroll = true;
	bool scrollToBottom = false;
	
	//console error warn flag and last error
	bool cons_error_warn = false;
	std::string last_error;
	
	//imgui capture flags
	bool IMGUI_KEY_CAPTURE = false;
	//im diofferenciating between console and imgui so external things (like canvas)
	//can set imgui key capture without console overriding it 
	bool CONSOLE_KEY_CAPTURE = false;
	bool IMGUI_MOUSE_CAPTURE = false;
	
	Console() {}
	
	void Init();
	void Update();
	void DrawConsole();
	void PushConsole(std::string s);
	void AddLog(std::string input);
	void FlushBuffer();
	int TextEditCallback(ImGuiInputTextCallbackData* data);
	static int TextEditCallbackStub(ImGuiInputTextCallbackData* data);
	
	Command* GetCommand(std::string command);
	std::string ExecCommand(std::string command);
	std::string ExecCommand(std::string command, std::string args);
	void AddRandomCommands();
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
#define DengConsole g_console

#define LOG(...)     g_console->PushConsole(TOSTRING(__VA_ARGS__))
#define ERROR(...)   g_console->PushConsole(TOSTRING("[c:error]", __VA_ARGS__, "[c]"))
#define SUCCESS(...) g_console->PushConsole(TOSTRING("[c:green]", __VA_ARGS__, "[c]"))
#define WARNING(...) g_console->PushConsole(TOSTRING("[c:yellow]", __VA_ARGS__, "[c]"))

//additionally prints where function was called
#define LOG_LOC(...)     g_console->PushConsole(TOSTRING("In ", __FILENAME__, " at ", __LINE__ , ": \n", __VA_ARGS__))
#define ERROR_LOC(...)   g_console->PushConsole(TOSTRING("[c:error]In ", __FILENAME__, " at ", __LINE__, ": \n[c]", "[c:error]", __VA_ARGS__, "[c]"))
#define SUCCESS_LOC(...) g_console->PushConsole(TOSTRING("[c:green]In ", __FILENAME__, " at ", __LINE__, ": \n[c]", "[c:green]", __VA_ARGS__, "[c]")
#define WARNING_LOC(...) g_console->PushConsole(TOSTRING("[c:yellow]In ", __FILENAME__, " at ", __LINE__, ": \n[c]", "[c:yellow]", __VA_ARGS__, "[c]")

#define LOGFUNC LOG(__FUNCTION__, " called")
#define LOGFUNCM(...) LOG(__FUNCTION__, " called ", __VA_ARGS__)

#endif //DESHI_CONSOLE_H