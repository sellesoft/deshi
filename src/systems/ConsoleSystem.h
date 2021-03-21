#pragma once
#include "System.h"

#include "../utils/Color.h"

struct ImGuiInputTextCallbackData;

struct Console : public System {

	char inputBuf[256]{};
	std::vector<std::pair<std::string, Color>> buffer; //text, color
	std::vector<std::string> history;
	int historyPos = -1;

	bool autoScroll = true;
	bool scrollToBottom = false;

	void Init() override;
	void Update() override;
	void DrawConsole();
	void PushConsole(std::string s);
	void AddLog(std::string input);
	void FlushBuffer();
	std::string ExecCommand(std::string command, std::string args);
	int TextEditCallback(ImGuiInputTextCallbackData* data);
	static int TextEditCallbackStub(ImGuiInputTextCallbackData* data);

	//Console* console;

	bool dispcon = false;
};